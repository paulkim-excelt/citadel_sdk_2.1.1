/******************************************************************************
 *  Copyright (C) 2017 Broadcom. The term "Broadcom" refers to Broadcom Limited
 *  and/or its subsidiaries.
 *
 *  This program is the proprietary software of Broadcom and/or its licensors,
 *  and may only be used, duplicated, modified or distributed pursuant to the
 *  terms and conditions of a separate, written license agreement executed
 *  between you and Broadcom (an "Authorized License").  Except as set forth in
 *  an Authorized License, Broadcom grants no license (express or implied),
 *  right to use, or waiver of any kind with respect to the Software, and
 *  Broadcom expressly reserves all rights in and to the Software and all
 *  intellectual property rights therein.  IF YOU HAVE NO AUTHORIZED LICENSE,
 *  THEN YOU HAVE NO RIGHT TO USE THIS SOFTWARE IN ANY WAY, AND SHOULD
 *  IMMEDIATELY NOTIFY BROADCOM AND DISCONTINUE ALL USE OF THE SOFTWARE.
 *
 *  Except as expressly set forth in the Authorized License,
 *
 *  1.     This program, including its structure, sequence and organization,
 *  constitutes the valuable trade secrets of Broadcom, and you shall use all
 *  reasonable efforts to protect the confidentiality thereof, and to use this
 *  information only in connection with your use of Broadcom integrated circuit
 *  products.
 *
 *  2.     TO THE MAXIMUM EXTENT PERMITTED BY LAW, THE SOFTWARE IS PROVIDED
 *  "AS IS" AND WITH ALL FAULTS AND BROADCOM MAKES NO PROMISES, REPRESENTATIONS
 *  OR WARRANTIES, EITHER EXPRESS, IMPLIED, STATUTORY, OR OTHERWISE, WITH
 *  RESPECT TO THE SOFTWARE.  BROADCOM SPECIFICALLY DISCLAIMS ANY AND ALL
 *  IMPLIED WARRANTIES OF TITLE, MERCHANTABILITY, NONINFRINGEMENT, FITNESS FOR A
 *  PARTICULAR PURPOSE, LACK OF VIRUSES, ACCURACY OR COMPLETENESS, QUIET
 *  ENJOYMENT, QUIET POSSESSION OR CORRESPONDENCE TO DESCRIPTION. YOU ASSUME THE
 *  ENTIRE RISK ARISING OUT OF USE OR PERFORMANCE OF THE SOFTWARE.
 *
 *  3.     TO THE MAXIMUM EXTENT PERMITTED BY LAW, IN NO EVENT SHALL BROADCOM OR
 *  ITS LICENSORS BE LIABLE FOR (i) CONSEQUENTIAL, INCIDENTAL, SPECIAL,
 *  INDIRECT, OR EXEMPLARY DAMAGES WHATSOEVER ARISING OUT OF OR IN ANY WAY
 *  RELATING TO YOUR USE OF OR INABILITY TO USE THE SOFTWARE EVEN IF BROADCOM
 *  HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES; OR (ii) ANY AMOUNT IN
 *  EXCESS OF THE AMOUNT ACTUALLY PAID FOR THE SOFTWARE ITSELF OR U.S. $1,
 *  WHICHEVER IS GREATER. THESE LIMITATIONS SHALL APPLY NOTWITHSTANDING ANY
 *  FAILURE OF ESSENTIAL PURPOSE OF ANY LIMITED REMEDY.
 ******************************************************************************/

/* @file crypto_smau.c
 *
 * This file contains the low level API's for symmetric crypto, Hash and HMAC
 *
 *
 */

/*
 * TODO
 *
 *
 * */

/* APIS flow 
1) SBL will use the DMA channel 0 for Crypto inbound packet
   (CMD -> AES key -> HMAC key -> IV -> Data )) and Channel 1 for
   Crypto Outbound packet (IV + Crypto Data followed by SHA/HMAC value )
2) SBL will call the bcm_spum_create_packet  function to populate the
   Crypto header inside SRAM starting at address SMAU_SRAM_HEADER_ADDR
3) SBL will cal the DMA configuration function ,which will perform
   following configurations .
    a) API will assign Channel 0 -Desc 0 for Crypto header and
       subsequent Linked list element for Inbound data
    b) API will assign Channel 1 -Desc 0 and linked list element
       for Outbound data in case of ECB mode
    b) API will assign Channel 1 -Desc 0 for IV and linked list element
       for Outbound data in case of CBC mode
 */

/* ***************************************************************************
 * Includes
 * ***************************************************************************/
#include <string.h>
#include <misc/printk.h>
#include <zephyr/types.h>
#include <board.h>
#include <crypto/crypto.h>
#include <smau.h>
#include <crypto/crypto_smau.h>
#include <broadcom/dma.h>
#include <cortex_a/cache.h>
#include "crypto_sw_utils.h"

/* ***************************************************************************
 * MACROS/Defines
 * ***************************************************************************/
/* This is done locally here for this file only since it is legacy code ported
 * */
#define BUILD_LYNX

/* ***************************************************************************
 * Types/Structure Declarations
 * ***************************************************************************/

/* ***************************************************************************
 * Global and Static Variables
 * Total Size: NNNbytes
 * ***************************************************************************/

/* ***************************************************************************
 * Private Functions Prototypes
 * ****************************************************************************/

static int crypto_smau_create_packet(crypto_lib_handle *pHandle,
		                       smu_bulkcrypto_packet *ctx, u32_t loop );

static void crypto_smau_configuration (crypto_lib_handle *pHandle,
		 	 	 	 	 	 	smu_bulkcrypto_packet *ctx);
static void crypto_smau_dmac_configuration(crypto_lib_handle *pHandle,
					smu_bulkcrypto_packet *ctx,
					u8_t * outPtrOriginal);
static BCM_SCAPI_STATUS struct_init(smu_bulkcrypto_packet *cur_pkt);

/* ***************************************************************************
 * Public Functions
 * ****************************************************************************/

void crypto_init(crypto_lib_handle *pHandle)
{

#ifdef BULKCRYPTO_DEBUG
        printk("Enter crypto_init\n");
#endif
        /* Initialize handle to all 0. */
        memset(pHandle, 0, CRYPTO_LIB_HANDLE_SIZE);

#ifdef BULKCRYPTO_DEBUG
        printk("Exit crypto_init\n");
#endif

        return;
}

BCM_SCAPI_STATUS crypto_smau_cipher_deinit (
                    crypto_lib_handle *pHandle,
                    bcm_bulk_cipher_context *ctx)
{
	ARG_UNUSED(pHandle);
	ARG_UNUSED(ctx);
	return BCM_SCAPI_STATUS_OK;
}

void crypto_smau_reset_blocks(void)
{
	/* FIXME This API needs to be enabled once DMU block is ready */
#ifdef CONFIG_DMU
	phx_dmu_block_enable_rst(DMU_DMA_PWR_ENABLE);
#endif /* CONFIG_DMU */
	/* Soft resets the SMAU blocks
	 * FIXME do we need to wait for sometime after the
	 * reset check with HW guys ? */
	reg32setbit(SMU_CFG_1, SMU_CFG_1__DMA_SOFT_RST);
	reg32setbit(SMU_CFG_1, SMU_CFG_1__CACHE_SOFT_RST);

	/* FIXME This API needs to be enabled once DMU block is ready */
#ifdef CONFIG_DMU
	phx_dmu_block_enable_rst(DMU_DMA_PWR_ENABLE);
#endif /* CONFIG_DMU */
}

BCM_SCAPI_STATUS crypto_smau_cipher_init (
                    crypto_lib_handle *pHandle,
                    BCM_BULK_CIPHER_CMD *cmd,
                    u8_t *encr_key,
                    u8_t *auth_key,
                    u32_t auth_key_len,
                    bcm_bulk_cipher_context *ctx)
{

	BCM_SCAPI_STATUS status;
	smu_bulkcrypto_packet *cur_pkt = &(ctx->cur_pkt);

#ifdef BULKCRYPTO_DEBUG
printk("Enter crypto_smau_cipher_init\n");
#endif

	ARG_UNUSED(pHandle);
	ARG_UNUSED(auth_key_len);

	status = struct_init(cur_pkt);
	if(status != BCM_SCAPI_STATUS_OK) {
		return status;
	}

	cur_pkt->msg_hdr->crypt_hdr0->sha3En                = 0;
	cur_pkt->msg_hdr->crypt_hdr0->send_hash_only       = 1;
	cur_pkt->msg_hdr->crypt_hdr0->crypto_algo_0        = 0;
	cur_pkt->msg_hdr->crypt_hdr0->crypto_algo_1        = 0;
	cur_pkt->msg_hdr->crypt_hdr0->sctx_crypt_mode      = 0;
	cur_pkt->msg_hdr->crypt_hdr0->sctx_aes_key_size    = 0;
	cur_pkt->msg_hdr->crypt_hdr0->sctx_sha             = 0;
	cur_pkt->msg_hdr->crypt_hdr0->sctx_hash_mode       = 0;
	cur_pkt->msg_hdr->crypt_hdr0->sctx_hash_type       = 0;
	cur_pkt->msg_hdr->crypt_hdr0->check_key_length_sel = 0;
	cur_pkt->msg_hdr->crypt_hdr0->length_override      = 0; // This parameter should only be used for SHA finish command
	cur_pkt->msg_hdr->crypt_hdr0->auth_key_sz          = 0; // 32 bytes HMAC keys

	switch (cmd->cipher_order) {
		case BCM_SCAPI_CIPHER_ORDER_NULL:
		cur_pkt->msg_hdr->crypt_hdr0->sctx_order   = ENCR_AUTH;
		break;
		case BCM_SCAPI_CIPHER_ORDER_AUTH_CRYPT:
		cur_pkt->msg_hdr->crypt_hdr0->sctx_order           = AUTH_ENCR;
		break;
		case BCM_SCAPI_CIPHER_ORDER_CRYPT_AUTH:
		cur_pkt->msg_hdr->crypt_hdr0->sctx_order           = ENCR_AUTH;
		break;
	}

	if (cmd->cipher_mode == BCM_SCAPI_CIPHER_MODE_ENCRYPT)
		cur_pkt->msg_hdr->crypt_hdr0->encrypt_decrypt = ENCRYPT;
	else if (cmd->cipher_mode == BCM_SCAPI_CIPHER_MODE_DECRYPT)
		cur_pkt->msg_hdr->crypt_hdr0->encrypt_decrypt = DECRYPT;
	else if (cmd->cipher_mode == BCM_SCAPI_CIPHER_MODE_AUTHONLY) {
		cur_pkt->msg_hdr->crypt_hdr0->send_hash_only = 1;
		cur_pkt->msg_hdr->crypt_hdr0->sctx_order = ENCR_AUTH;
		cur_pkt->msg_hdr->crypt_hdr0->encrypt_decrypt = DECRYPT; //No functionality. Just to avoid uninitialized bit
	}
	cur_pkt->msg_hdr->crypt_hdr0->auth_enable          = AUTH_DISABLE;

	switch (cmd->encr_alg) {
		case BCM_SCAPI_ENCR_ALG_AES_128:
		case BCM_SCAPI_ENCR_ALG_AES_192:
		case BCM_SCAPI_ENCR_ALG_AES_256:
		cur_pkt->msg_hdr->crypt_hdr0->crypto_algo_0 = (AES & 0x1);
		cur_pkt->msg_hdr->crypt_hdr0->crypto_algo_1 = ((AES & 0x2)>>1);
		cur_pkt->msg_hdr->crypt_hdr0->send_hash_only = 0; // override below in CCM section;

		if (cmd->encr_mode == BCM_SCAPI_ENCR_MODE_ECB){
		    cur_pkt->msg_hdr->crypt_hdr0->sctx_crypt_mode = ECB;
		}
		else if (cmd->encr_mode == BCM_SCAPI_ENCR_MODE_CBC){
		    cur_pkt->msg_hdr->crypt_hdr0->sctx_crypt_mode = CBC;
		}
		else if (cmd->encr_mode == BCM_SCAPI_ENCR_MODE_CTR){
		    cur_pkt->msg_hdr->crypt_hdr0->sctx_crypt_mode = CTR;
		}
		else if (cmd->encr_mode == BCM_SCAPI_ENCR_MODE_CCM){
		    cur_pkt->msg_hdr->crypt_hdr0->sctx_crypt_mode = CCM;
		    cur_pkt->msg_hdr->crypt_hdr0->send_hash_only = 1;
		    cur_pkt->msg_hdr->crypt_hdr0->crypto_algo_1 = 0;
		}

		if(cmd->encr_alg == BCM_SCAPI_ENCR_ALG_AES_128){
		    cur_pkt->msg_hdr->crypt_hdr0->sctx_aes_key_size = AES_128;
		    memcpy(cur_pkt->cryptokey->AES_128_key,encr_key,16);
		}
		else if(cmd->encr_alg == BCM_SCAPI_ENCR_ALG_AES_192){
		    cur_pkt->msg_hdr->crypt_hdr0->sctx_aes_key_size = AES_192;
		    memcpy(cur_pkt->cryptokey->AES_192_key,encr_key,24);
		}
		else if(cmd->encr_alg == BCM_SCAPI_ENCR_ALG_AES_256){
		    cur_pkt->msg_hdr->crypt_hdr0->sctx_aes_key_size = AES_256;
		    memcpy(cur_pkt->cryptokey->AES_256_key,encr_key,32);
		}
		if (cmd->cipher_order == BCM_SCAPI_CIPHER_ORDER_NULL)
		    cur_pkt->msg_hdr->crypt_hdr0->sctx_order = ENCR_AUTH;

		break;

		case    BCM_SCAPI_ENCR_ALG_DES:
		cur_pkt->msg_hdr->crypt_hdr0->crypto_algo_0 = DES & 0x1;
		cur_pkt->msg_hdr->crypt_hdr0->crypto_algo_1 = (DES & 0x2) >> 1;
		if (cmd->encr_mode == BCM_SCAPI_ENCR_MODE_ECB)
			cur_pkt->msg_hdr->crypt_hdr0->sctx_crypt_mode = ECB;
		else if (cmd->encr_mode == BCM_SCAPI_ENCR_MODE_CBC)
			cur_pkt->msg_hdr->crypt_hdr0->sctx_crypt_mode = CBC;

		cur_pkt->msg_hdr->crypt_hdr0->send_hash_only = 0;
		memcpy(cur_pkt->cryptokey->DES_key, encr_key, 8);
		break;

		case    BCM_SCAPI_ENCR_ALG_3DES:
		cur_pkt->msg_hdr->crypt_hdr0->crypto_algo_0 = (_3DES & 0x1);
		cur_pkt->msg_hdr->crypt_hdr0->crypto_algo_1 =
							(_3DES & 0x2) >> 1;
		if (cmd->encr_mode == BCM_SCAPI_ENCR_MODE_ECB)
			cur_pkt->msg_hdr->crypt_hdr0->sctx_crypt_mode = ECB;
		else if (cmd->encr_mode == BCM_SCAPI_ENCR_MODE_CBC)
			cur_pkt->msg_hdr->crypt_hdr0->sctx_crypt_mode = CBC;
		cur_pkt->msg_hdr->crypt_hdr0->send_hash_only = 0;
		memcpy(cur_pkt->cryptokey->_3DES_key, encr_key, 24);
		break;
	}

	switch( cmd->auth_alg){
		case BCM_SCAPI_AUTH_ALG_SHA256:
		cur_pkt->msg_hdr->crypt_hdr0->sctx_sha = SHA2;
		if (cmd->auth_mode == BCM_SCAPI_AUTH_MODE_HASH) {
		    cur_pkt->msg_hdr->crypt_hdr0->sctx_hash_mode = HASH;
		    cur_pkt->msg_hdr->crypt_hdr0->auth_key_sz = 0;
		} else if (cmd->auth_mode == BCM_SCAPI_AUTH_MODE_HMAC) {
		    cur_pkt->msg_hdr->crypt_hdr0->sctx_hash_mode = HMAC;
		    cur_pkt->msg_hdr->crypt_hdr0->auth_key_sz = 32; // 32 byte HMAC key
		    memcpy(cur_pkt->hashkey->HMAC_key,auth_key,32);
		}

		if ((cmd->auth_optype == INIT) || (cmd->auth_optype == UPDT) || (cmd->auth_optype == FIN)) {
		    cur_pkt->msg_hdr->crypt_hdr0->sctx_hash_type = cmd->auth_optype;
		    cur_pkt->msg_hdr->crypt_hdr0->check_key_length_sel = HMAC_KEY_SZ_20bytes;
		    cur_pkt->msg_hdr->crypt_hdr0->length_override = (cmd->auth_optype == FIN) ? 1 : 0;
		} else {
		    cur_pkt->msg_hdr->crypt_hdr0->sctx_hash_type = CMPLTE;
		    cur_pkt->msg_hdr->crypt_hdr0->check_key_length_sel = HMAC_KEY_SZ_FRM_AUTH_KEY_LEN;
		    cur_pkt->msg_hdr->crypt_hdr0->length_override = 0; // only be used for SHA finish command
		}

		cur_pkt->msg_hdr->crypt_hdr0->auth_enable = AUTH_ENABLE;
		if (cmd->cipher_order == BCM_SCAPI_CIPHER_ORDER_NULL)
		    cur_pkt->msg_hdr->crypt_hdr0->sctx_order = AUTH_ENCR;
		break;

		case BCM_SCAPI_AUTH_ALG_SHA1:
		cur_pkt->msg_hdr->crypt_hdr0->sctx_sha = SHA1;
		if (cmd->auth_mode == BCM_SCAPI_AUTH_MODE_HASH) {
		    cur_pkt->msg_hdr->crypt_hdr0->sctx_hash_mode = HASH;
		    cur_pkt->msg_hdr->crypt_hdr0->auth_key_sz = 0;
		} else if (cmd->auth_mode == BCM_SCAPI_AUTH_MODE_HMAC) {
		    cur_pkt->msg_hdr->crypt_hdr0->sctx_hash_mode = HMAC;
		    cur_pkt->msg_hdr->crypt_hdr0->auth_key_sz = 20; // 20 byte HMAC key
		    memcpy(cur_pkt->hashkey->HMAC_key,auth_key,20);
		}

		if ((cmd->auth_optype == INIT) || (cmd->auth_optype == UPDT) || (cmd->auth_optype == FIN)) {
		    cur_pkt->msg_hdr->crypt_hdr0->sctx_hash_type = cmd->auth_optype;
		    cur_pkt->msg_hdr->crypt_hdr0->check_key_length_sel = HMAC_KEY_SZ_20bytes;
		    cur_pkt->msg_hdr->crypt_hdr0->length_override = (cmd->auth_optype == FIN) ? 1 : 0;
		} else {
		    cur_pkt->msg_hdr->crypt_hdr0->sctx_hash_type= CMPLTE;
		    cur_pkt->msg_hdr->crypt_hdr0->check_key_length_sel = HMAC_KEY_SZ_FRM_AUTH_KEY_LEN;
		    cur_pkt->msg_hdr->crypt_hdr0->length_override = 0; // only be used for SHA finish command
		}

		cur_pkt->msg_hdr->crypt_hdr0->auth_enable = AUTH_ENABLE;

		break;
		case BCM_SCAPI_AUTH_ALG_SHA3_224:
			cur_pkt->msg_hdr->crypt_hdr0->sctx_sha = SHA1;
			cur_pkt->msg_hdr->crypt_hdr0->sha3En = 1;

			cur_pkt->msg_hdr->crypt_hdr5->crypto_type = 0;
			cur_pkt->msg_hdr->crypt_hdr5->hash_type = 1;
			cur_pkt->msg_hdr->crypt_hdr5->SHA3_algorithm = 0;

			cur_pkt->msg_hdr->crypt_hdr0->sctx_hash_mode = HASH;
			cur_pkt->msg_hdr->crypt_hdr0->auth_key_sz = 0;

			cur_pkt->msg_hdr->crypt_hdr0->sctx_hash_type = CMPLTE;
			cur_pkt->msg_hdr->crypt_hdr0->check_key_length_sel = HMAC_KEY_SZ_FRM_AUTH_KEY_LEN;
			// This parameter should only be used for SHA finish command
			cur_pkt->msg_hdr->crypt_hdr0->length_override = 0;
			cur_pkt->msg_hdr->crypt_hdr0->auth_enable = AUTH_ENABLE;

			if (cmd->cipher_order == BCM_SCAPI_CIPHER_ORDER_NULL) {
				cur_pkt->msg_hdr->crypt_hdr0->sctx_order = AUTH_ENCR;
			}
		break;

		case BCM_SCAPI_AUTH_ALG_SHA3_256:
			cur_pkt->msg_hdr->crypt_hdr0->sctx_sha = SHA1;
			cur_pkt->msg_hdr->crypt_hdr0->sha3En = 1;

			cur_pkt->msg_hdr->crypt_hdr5->crypto_type = 0;
			cur_pkt->msg_hdr->crypt_hdr5->hash_type = 1;
			cur_pkt->msg_hdr->crypt_hdr5->SHA3_algorithm = 1;

			cur_pkt->msg_hdr->crypt_hdr0->sctx_hash_mode = HASH;
			cur_pkt->msg_hdr->crypt_hdr0->auth_key_sz = 0;

			cur_pkt->msg_hdr->crypt_hdr0->sctx_hash_type = CMPLTE;
			cur_pkt->msg_hdr->crypt_hdr0->check_key_length_sel = HMAC_KEY_SZ_FRM_AUTH_KEY_LEN;
			// This parameter should only be used for SHA finish command
			cur_pkt->msg_hdr->crypt_hdr0->length_override = 0;
			cur_pkt->msg_hdr->crypt_hdr0->auth_enable = AUTH_ENABLE;

			if (cmd->cipher_order == BCM_SCAPI_CIPHER_ORDER_NULL) {
				cur_pkt->msg_hdr->crypt_hdr0->sctx_order = AUTH_ENCR;
			}
		break;

		case BCM_SCAPI_AUTH_ALG_SHA3_384:
			cur_pkt->msg_hdr->crypt_hdr0->sctx_sha = SHA1;
			cur_pkt->msg_hdr->crypt_hdr0->sha3En = 1;

			cur_pkt->msg_hdr->crypt_hdr5->crypto_type = 0;
			cur_pkt->msg_hdr->crypt_hdr5->hash_type = 1;
			cur_pkt->msg_hdr->crypt_hdr5->SHA3_algorithm = 2;

			cur_pkt->msg_hdr->crypt_hdr0->sctx_hash_mode = HASH;
			cur_pkt->msg_hdr->crypt_hdr0->auth_key_sz = 0;

			cur_pkt->msg_hdr->crypt_hdr0->sctx_hash_type = CMPLTE;
			cur_pkt->msg_hdr->crypt_hdr0->check_key_length_sel = HMAC_KEY_SZ_FRM_AUTH_KEY_LEN;
			// This parameter should only be used for SHA finish command
			cur_pkt->msg_hdr->crypt_hdr0->length_override = 0;
			cur_pkt->msg_hdr->crypt_hdr0->auth_enable = AUTH_ENABLE;

			if (cmd->cipher_order == BCM_SCAPI_CIPHER_ORDER_NULL) {
				cur_pkt->msg_hdr->crypt_hdr0->sctx_order = AUTH_ENCR;
			}
		break;

		case BCM_SCAPI_AUTH_ALG_SHA3_512:
			cur_pkt->msg_hdr->crypt_hdr0->sctx_sha = SHA1;
			cur_pkt->msg_hdr->crypt_hdr0->sha3En = 1;

			cur_pkt->msg_hdr->crypt_hdr5->crypto_type = 0;
			cur_pkt->msg_hdr->crypt_hdr5->hash_type = 1;
			cur_pkt->msg_hdr->crypt_hdr5->SHA3_algorithm = 3;

			cur_pkt->msg_hdr->crypt_hdr0->sctx_hash_mode = HASH;
			cur_pkt->msg_hdr->crypt_hdr0->auth_key_sz = 0;

			cur_pkt->msg_hdr->crypt_hdr0->sctx_hash_type = CMPLTE;
			cur_pkt->msg_hdr->crypt_hdr0->check_key_length_sel = HMAC_KEY_SZ_FRM_AUTH_KEY_LEN;
			// This parameter should only be used for SHA finish command
			cur_pkt->msg_hdr->crypt_hdr0->length_override = 0;
			cur_pkt->msg_hdr->crypt_hdr0->auth_enable = AUTH_ENABLE;

			if (cmd->cipher_order == BCM_SCAPI_CIPHER_ORDER_NULL) {
				cur_pkt->msg_hdr->crypt_hdr0->sctx_order = AUTH_ENCR;
			}
		break;

		default:break;

	}

#ifdef BULKCRYPTO_DEBUG
printk("Exit crypto_smau_cipher_init\n");
#endif

	return BCM_SCAPI_STATUS_OK;
}

BCM_SCAPI_STATUS crypto_smau_cipher_start( crypto_lib_handle *pHandle,
					u8_t *data_in,
					u32_t data_len,
					u8_t *iv,
					u32_t crypto_len,
					u16_t crypto_offset,
					u32_t auth_len,
					u16_t auth_offset,
					u8_t *output,
					bcm_bulk_cipher_context *ctx)
{
	unsigned int iv_len  = 0;
	u8_t temp[16];
	smu_bulkcrypto_packet *cur_pkt = &(ctx->cur_pkt);
	BCM_SCAPI_STATUS rv = BCM_SCAPI_STATUS_OK;
#ifdef BULKCRYPTO_DEBUG
	printk("Enter crypto_smau_cipher_start\n");
#endif

    // Only takes effect if (length_override && finish && CMD_HASH) is true; reusing *output, since it will be overwritten anyway
	if (cur_pkt->msg_hdr->crypt_hdr0->sctx_hash_type == FIN)
		cur_pkt->msg_hdr->crypt_hdr1->pad_length = ctx->pad_length;
	else
		cur_pkt->msg_hdr->crypt_hdr1->pad_length = *output;

    if (cur_pkt->msg_hdr->crypt_hdr0->sctx_crypt_mode == CCM) {

	//this field describes the offset in bytes from the beginning of the header to the IV/counter  //crypto_offset ;
	    // In lynx there were only 5 bytes of headers present, but in citadel
	    // 6 headers are present. So sizeof(msg_hdr_top) - 4, so for CCM
	    // negate the extra 4 byte header for crypto offset
	cur_pkt->msg_hdr->crypt_hdr2->aes_offset =
	    sizeof(msg_hdr_top) - 4 + cur_pkt->msg_hdr->crypt_hdr0->sctx_aes_key_size * 8 + 16;

	cur_pkt->msg_hdr->crypt_hdr3->aes_length = crypto_len;
	cur_pkt->msg_hdr->crypt_hdr4->auth_length = crypto_offset ;
    } else {
	cur_pkt->msg_hdr->crypt_hdr2->aes_offset = crypto_offset ;  //  decryption starts from offset 0 of SBI
	cur_pkt->msg_hdr->crypt_hdr3->aes_length = crypto_len ;
	cur_pkt->msg_hdr->crypt_hdr4->auth_length = auth_len ;
    }

    cur_pkt->msg_hdr->crypt_hdr2->auth_offset = auth_offset ;  //  authentication starts from offset 0 of SBI



	if (cur_pkt->msg_hdr->crypt_hdr0->crypto_algo_1 == 1) {
		if (cur_pkt->msg_hdr->crypt_hdr0->sctx_crypt_mode == ECB) {
			iv_len = 0;
		} else if (cur_pkt->msg_hdr->crypt_hdr0->sctx_crypt_mode ==
									CBC) {
			iv_len = 8;
			memcpy(cur_pkt->cryptoiv->DES_IV, iv, 8);
		}
	} else if (cur_pkt->msg_hdr->crypt_hdr0->sctx_crypt_mode == ECB) {
	iv_len = 0;
    } else if ((cur_pkt->msg_hdr->crypt_hdr0->sctx_crypt_mode == CBC) ||
	       (cur_pkt->msg_hdr->crypt_hdr0->sctx_crypt_mode == CCM) ||
	       (cur_pkt->msg_hdr->crypt_hdr0->sctx_crypt_mode == CTR)) {
		if (iv == NULL) {
			if (cur_pkt->msg_hdr->crypt_hdr0->send_hash_only == 0) {
#ifdef BULKCRYPTO_DEBUG
				printk("%s: Incorrect IV parameter for CBC encryption \n",
						__FUNCTION__);
#endif
			}
			iv_len = 16;
			memset(cur_pkt->cryptoiv->AES_IV,0,16);
		} else {
			iv_len = 16;
			memcpy(cur_pkt->cryptoiv->AES_IV,iv,16);
		}
    }

    cur_pkt->in_data = (unsigned char *)data_in;

    if (crypto_len == 0) {
	//auth only, so no header
	cur_pkt->out_data = (unsigned char *)output;
    } else {
	if (cur_pkt->msg_hdr->crypt_hdr0->sctx_crypt_mode == CCM) {
	    cur_pkt->out_data = (unsigned char *)output;
	} else {
	    memcpy(temp,output-iv_len-crypto_offset,iv_len);
	    cur_pkt->out_data = (unsigned char *)(output-iv_len-crypto_offset);
	}
    }

#ifdef BULKCRYPTO_DEBUG
	printk("crypto len!!! 0x%x\n",(u32_t)(crypto_len));
	printk("Output datai!!! 0x%x\n",(u32_t)(output));
	printk("Crypto offset !!! 0x%x\n",crypto_offset);
	printk("Output datai!!! 0x%x\n",(u32_t)(cur_pkt->out_data));
#endif

    cur_pkt->in_data_length  = data_len ; // One block length input data

	// Initialization function call
	crypto_smau_create_packet(pHandle, cur_pkt, 0);

#ifdef CONFIG_CRYPTO_DPA
	if (((cur_pkt->msg_hdr->crypt_hdr0->crypto_algo_1 << 1) |
		(cur_pkt->msg_hdr->crypt_hdr0->crypto_algo_0)) == AES) {
		rv = aes_dpa_enable();
		if (rv)
			return rv;
	}
#endif
	crypto_smau_configuration(pHandle, cur_pkt);
	crypto_smau_dmac_configuration(pHandle, cur_pkt, output);

	if(crypto_len > 0) {
		if (cur_pkt->msg_hdr->crypt_hdr0->sctx_crypt_mode != CCM) {
		    memcpy(output-iv_len-crypto_offset, temp, iv_len);
		}
	}

#ifdef BULKCRYPTO_DEBUG
	printk("Exit crypto_smau_cipher_start\n");
#endif

	return rv;
}

/* ***************************************************************************
 * Private Functions
 * ****************************************************************************/

static int crypto_smau_create_packet(crypto_lib_handle *pHandle,
		                       smu_bulkcrypto_packet *ctx, u32_t loop )
{
	u32_t crypto_command_length;
	u32_t  num_w ,auth_key_len , iv_len , crypto_algo;
	//pack the cur packet structure into an array for CRYPTO Engine
	u32_t * crypto_hdr_ptr = ((u32_t *) (ctx->SMAU_SRAM_HEADER_ADDR));
	u32_t hdr_temp;

	ARG_UNUSED(pHandle);
	ARG_UNUSED(loop);

#ifdef BULKCRYPTO_DEBUG
	printk("Enter bcm_spum_create_packet\n");
#endif

	//  crypto_hdr_ptr[0] = *((unsigned int *)(ctx->msg_hdr->crypt_hdr0));
	hdr_temp = *((unsigned int *)(ctx->msg_hdr->crypt_hdr0)); 
	crypto_hdr_ptr[0] = (((hdr_temp & 0xFF)<<24)|((hdr_temp & 0xFF00)<<8) |
			 ((hdr_temp & 0xFF0000)>>8) | ((hdr_temp & 0xFF000000)>>24));

	//crypto_hdr_ptr[1] = *((unsigned int *)(ctx->msg_hdr->crypt_hdr1));
	hdr_temp = *((unsigned int *)(ctx->msg_hdr->crypt_hdr1));
	crypto_hdr_ptr[1] = (((hdr_temp & 0xFF)<<24)|((hdr_temp & 0xFF00)<<8) |
			 ((hdr_temp & 0xFF0000)>>8) | ((hdr_temp & 0xFF000000)>>24));

	crypto_command_length = 5;
	/* For SM3 and SM4 and SHA3 this new header has to be sent
	 * for all other cases this is not required.
	 * The new header is indicated in the header[0] 10th bit */
	if(ctx->msg_hdr->crypt_hdr0->sha3En)
	{
		hdr_temp = *((unsigned int *)(ctx->msg_hdr->crypt_hdr5));
		crypto_hdr_ptr[5] = (((hdr_temp & 0xFF)<<24)|((hdr_temp & 0xFF00)<<8) |
		((hdr_temp & 0xFF0000)>>8) | ((hdr_temp & 0xFF000000)>>24));
		crypto_command_length++;
	}
	crypto_algo  = ((ctx->msg_hdr->crypt_hdr0->crypto_algo_1 <<1)  |(ctx->msg_hdr->crypt_hdr0->crypto_algo_0 ));
	//-------------------------------------------
	// Crypto key initialization in SRAM
	//-------------------------------------------
	if ((crypto_algo == AES)  && (ctx->msg_hdr->crypt_hdr0->sctx_aes_key_size == AES_128)) {
		for (num_w =0;num_w < 4;num_w++ ) {
			crypto_hdr_ptr[crypto_command_length+num_w] = (ctx->cryptokey->AES_128_key[num_w] );
		}
	} else if ((crypto_algo == AES)  && (ctx->msg_hdr->crypt_hdr0->sctx_aes_key_size == AES_192)) {
		for (num_w =0;num_w < 6;num_w++ ) {
			crypto_hdr_ptr[crypto_command_length+num_w] = (ctx->cryptokey->AES_192_key[num_w] );
		}
	} else if ((crypto_algo == AES)  && (ctx->msg_hdr->crypt_hdr0->sctx_aes_key_size == AES_256)) {
		for (num_w =0;num_w < 8;num_w++ ) {
			crypto_hdr_ptr[crypto_command_length+num_w] = (ctx->cryptokey->AES_256_key[num_w] );
		}
	}  else if ((crypto_algo == DES)) {
		for (num_w =0;num_w < 2;num_w++ ) {
			crypto_hdr_ptr[crypto_command_length+num_w] = (ctx->cryptokey->DES_key[num_w] );
		}
	} else if ((crypto_algo == _3DES)) {
		for (num_w =0;num_w < 6;num_w++ ) {
			crypto_hdr_ptr[crypto_command_length+num_w] = (ctx->cryptokey->_3DES_key[num_w] );
		}
	} else if ((crypto_algo == ZERO)) {
		num_w =0 ;
	} else {
		return BCM_SCAPI_STATUS_INVALID_KEY; /*Invalid key selection*/
	}

	crypto_command_length += num_w ;
	//-------------------------------------------
	// HMAC key initialization in SRAM
	// Here auth_key_sz is in bytes
	//-------------------------------------------
	if (ctx->msg_hdr->crypt_hdr0->auth_enable == 0 ) {
	auth_key_len = 0;
	} else if ((ctx->msg_hdr->crypt_hdr0->auth_enable ==1) &&
	       (ctx->msg_hdr->crypt_hdr0->sctx_hash_mode == HASH )) {
		auth_key_len = 0;
	} else if ((ctx->msg_hdr->crypt_hdr0->auth_enable ==1) &&
	       (ctx->msg_hdr->crypt_hdr0->check_key_length_sel == 0 ) &&
	       (ctx->msg_hdr->crypt_hdr0->sctx_hash_mode== HMAC ) &&
	       (ctx->msg_hdr->crypt_hdr0->sctx_sha == 0 /*SHA1*/)) {
		auth_key_len = 20;
	}  else if ((ctx->msg_hdr->crypt_hdr0->auth_enable ==1) &&
		(ctx->msg_hdr->crypt_hdr0->check_key_length_sel == 0 ) &&
		(ctx->msg_hdr->crypt_hdr0->sctx_hash_mode== HMAC ) &&
		(ctx->msg_hdr->crypt_hdr0->sctx_sha == 1/*SHA256*/)) {
		auth_key_len = 32;
	} else if ((ctx->msg_hdr->crypt_hdr0->auth_enable ==1) &&
	       (ctx->msg_hdr->crypt_hdr0->check_key_length_sel == 1 ) &&
	       (ctx->msg_hdr->crypt_hdr0->sctx_hash_mode== HMAC )) {
		auth_key_len= ctx->msg_hdr->crypt_hdr0->auth_key_sz;
	} else {
		return BCM_SCAPI_STATUS_INVALID_KEY; /* Unsupported HMAC key selection*/
	}

	//--  auth key programming (auth key not required for CCM /CMAC) ..also for HMAC Send same key twice - SMU Design limitation
	for (num_w=0 ; num_w < (auth_key_len/4);num_w++) {
		crypto_hdr_ptr[crypto_command_length+num_w]= (ctx->hashkey->HMAC_key[num_w]);
	}
	crypto_command_length +=  num_w;

	for (num_w=0 ; num_w < (auth_key_len/4);num_w++) {
		crypto_hdr_ptr[crypto_command_length+num_w]= (ctx->hashkey->HMAC_key[num_w]);
	}
	crypto_command_length +=  num_w;

	//-----------------------------------------------------------------------------------------
	// IV initialization in SRAM
	// Packet generation  for DES -3DES -CBC it will have 64 bit IV followed by Data
	// Packet generation  for AES -CBC it will have 128 bit IV followed by Data
	// Packet generation  for ECB, AES-CMAC mode it will only have the Data component
	// Packet generation  for AES-CTR  mode it will have IV + data component
	// Packet generation  for DES -3DES -CBC it will have 64 bit IV followed by Data
	//-----------------------------------------------------------------------------------------
	if  ( ctx->msg_hdr->crypt_hdr0->sctx_crypt_mode == ECB  )                                       { iv_len = 0; }
	else if  ( crypto_algo     == ZERO )                                                            { iv_len = 0; }
	else if  ( ctx->msg_hdr->crypt_hdr0->sctx_crypt_mode == CMAC )                                  { iv_len = 0; }
	else if  ((crypto_algo == AES    )     &&  (ctx->msg_hdr->crypt_hdr0->sctx_crypt_mode ==CBC))   { iv_len = 4; }
	else if  ((crypto_algo == AES    )     &&  (ctx->msg_hdr->crypt_hdr0->sctx_crypt_mode ==CTR))   { iv_len = 4; }
	else if  ((crypto_algo == AES    )     &&  (ctx->msg_hdr->crypt_hdr0->sctx_crypt_mode ==CCM))   { iv_len = 4; }
	else if  ((crypto_algo == DES    )     &&  (ctx->msg_hdr->crypt_hdr0->sctx_crypt_mode ==CBC))   { iv_len = 2; }
	else if  ((crypto_algo == DES    )     &&  (ctx->msg_hdr->crypt_hdr0->sctx_crypt_mode ==CTR))   { iv_len = 2; }
	else if  ((crypto_algo == _3DES  )     &&  (ctx->msg_hdr->crypt_hdr0->sctx_crypt_mode ==CBC))   { iv_len = 2; }
	else if  ((crypto_algo == _3DES  )     &&  (ctx->msg_hdr->crypt_hdr0->sctx_crypt_mode ==CTR))   { iv_len = 2; }
	else { return BCM_SCAPI_STATUS_PARAMETER_INVALID; } /* Unsupported IV selection*/

	if ((crypto_algo == DES) || (crypto_algo == _3DES)) {
	for (num_w=0 ; num_w < (iv_len);num_w++) {
	    crypto_hdr_ptr[crypto_command_length+num_w]= (ctx->cryptoiv->DES_IV[num_w]);
	}
	} else {
		/*AES IV programming*/
		for (num_w=0 ; num_w < (iv_len);num_w++) {
			crypto_hdr_ptr[crypto_command_length+num_w]= (ctx->cryptoiv->AES_IV[num_w]);
		}
	}

	/* NOTE : if required need to program the aes_offset and auth_offset  */
	ctx->cryptoiv->iv_len_from_packet = iv_len;   // Will be used in DMA function
	// crypto_hdr_ptr[2] = (((ctx->msg_hdr->crypt_hdr2->auth_offset + (crypto_command_length*4) + (iv_len*4)) <<16) |
	//                      ((ctx->msg_hdr->crypt_hdr2->aes_offset + (crypto_command_length*4) )& 0xffff));

	if  ((crypto_algo == AES) && (ctx->msg_hdr->crypt_hdr0->sctx_crypt_mode == CCM)) {
		hdr_temp = ctx->msg_hdr->crypt_hdr2->aes_offset;
	} else if (crypto_algo == ZERO) {
		hdr_temp = ((ctx->msg_hdr->crypt_hdr2->auth_offset + (crypto_command_length*4) + (iv_len*4)) <<16);
	} else {
		hdr_temp = (((ctx->msg_hdr->crypt_hdr2->auth_offset + (crypto_command_length*4) + (iv_len*4)) <<16) |
		    ((ctx->msg_hdr->crypt_hdr2->aes_offset + (crypto_command_length*4) )& 0xffff));
	}
	crypto_hdr_ptr[2] = (((hdr_temp & 0xFF)<<24)|((hdr_temp & 0xFF00)<<8) |
			 ((hdr_temp & 0xFF0000)>>8) | ((hdr_temp & 0xFF000000)>>24));


	//  crypto_hdr_ptr[3] = ((ctx->msg_hdr->crypt_hdr3->aes_length) + (iv_len*4));
	if ((crypto_algo == AES) && (ctx->msg_hdr->crypt_hdr0->sctx_crypt_mode ==CCM)) {
		hdr_temp =  (ctx->msg_hdr->crypt_hdr3->aes_length) ;
	} else {
		hdr_temp =  ((ctx->msg_hdr->crypt_hdr3->aes_length) + (iv_len*4));
	}
	crypto_hdr_ptr[3] = (((hdr_temp & 0xFF)<<24)|((hdr_temp & 0xFF00)<<8) |
			 ((hdr_temp & 0xFF0000)>>8) | ((hdr_temp & 0xFF000000)>>24));

	//  crypto_hdr_ptr[4] = *(unsigned int *)ctx->msg_hdr->crypt_hdr4;
	hdr_temp = *((unsigned int *)(ctx->msg_hdr->crypt_hdr4));
	crypto_hdr_ptr[4] = (((hdr_temp & 0xFF)<<24)|((hdr_temp & 0xFF00)<<8) |
			 ((hdr_temp & 0xFF0000)>>8) | ((hdr_temp & 0xFF000000)>>24));

	ctx->out_data_length = 0;

	if(ctx->msg_hdr->crypt_hdr0->auth_enable==1) {
		if(ctx->msg_hdr->crypt_hdr0->sha3En) {

			if(ctx->msg_hdr->crypt_hdr5->SHA3_algorithm == 00)
				ctx->out_data_length += 7;
			else if (ctx->msg_hdr->crypt_hdr5->SHA3_algorithm == 01)
				ctx->out_data_length += 8;
			else if (ctx->msg_hdr->crypt_hdr5->SHA3_algorithm == 02)
				ctx->out_data_length += 12;
			else if (ctx->msg_hdr->crypt_hdr5->SHA3_algorithm == 03)
				ctx->out_data_length += 16;

		} else {

			ctx->out_data_length +=
					 ((ctx->msg_hdr->crypt_hdr0->sctx_sha == 0)?5:8);
		}

	}

	if(crypto_algo != ZERO)
		ctx->out_data_length += (iv_len +  ctx->in_data_length/4);

	ctx->header_length = crypto_command_length  + iv_len;

	//   DEBUG_TRIGGER(d_event8);
#ifdef BULKCRYPTO_DEBUG
	printk("Exit bcm_spum_create_packet\n");
#endif

	return BCM_SCAPI_STATUS_OK;
}

static void crypto_smau_configuration (crypto_lib_handle *pHandle,
					smu_bulkcrypto_packet *ctx)
{
	unsigned int loop ;
	unsigned int dma_count;
	unsigned int data_length_in_words;

	ARG_UNUSED(pHandle);
	//----------------------------------
	// Reset the Crypto -DMA logic
	//----------------------------------
	reg32setbit(SMU_CFG_1 ,2);
	for (loop =0 ; loop < 10 ; loop++) {;}
	reg32clrbit(SMU_CFG_1 ,2);
	//----------------------------------

	reg32setbit(SMU_CFG_1 ,5);
	reg32setbit(SMU_CFG_1 ,6);
	reg32setbit(SMU_CFG_1 ,7);

	reg32setbit(SMU_CFG_1 ,3);   // Crypto clock enable
	reg32setbit(SMU_CFG_1 ,4);   // DMA clock enable

	data_length_in_words = ((ctx->in_data_length) / 4);
	dma_count = 1 /* here 1 is for header packet*/+ (data_length_in_words >> 12);   // 12 bit word is supported per DMA linked list

	reg32setbit(SMU_DMA_CONTROL,1);   // DMA chain count enable
	reg32_write(SMU_DMA_CONTROL , (dma_count <<20) | (1/*dma_chain_en*/ << 1 ));
	reg32setbit(SMU_CONTROL,3);   // DMA enable
}


static void crypto_smau_dmac_configuration(crypto_lib_handle *pHandle,
					smu_bulkcrypto_packet *ctx,
					u8_t * outPtrOriginal)
{
	u32_t i = 0;
	u32_t pendingTransferSize =  ((ctx->in_data_length));
	u32_t u_srcAddr = (u32_t) ctx->in_data;//SMAU_SRAM_DST_BASE_ADDR;
	u32_t dmaStatus;
	u32_t inchannel = 0XFFFF, outchannel = 0XFFFF;

	struct dma_config dma_cfg[2] = {0};
	struct dma_block_config dma_block_cfg[CRYPTO_SMAU_DMA_PACKET_COUNT];
	struct dma_block_config dma_block_cfg_1;
	struct device *dma = device_get_binding(CONFIG_DMA_DEV_NAME);

	/* Calculate the number of packets/blocks to be transfered
	 * Total = Data + header(1) */
	u32_t packet_count = (pendingTransferSize/0x3FFC) + 1;

#ifdef CONFIG_DATA_CACHE_SUPPORT

#define PL080_Cx_DST_ADDR(Cx)			((0x104 + (Cx * 0x20)))
	u32_t outSize;
	u32_t base = DMA_BASE_ADDR;
#else
	ARG_UNUSED(outPtrOriginal);
#endif

	ARG_UNUSED(pHandle);

	do {

#ifdef CONFIG_CRYPTO_DCACHE_WORKAROUND
/* Use this workaround if the existing solution for the data cache does not
 * work. The existing data cache solution works for all the unit tests and
 * all the test are passing with or without the data cache. */
		disable_dcache();
#endif
		/* If there are any remainder packets add the count */
		if(pendingTransferSize % 0x3FFC)
			packet_count++;

		if (!dma) {
			printk("Cannot get dma controller\n");
			break;
		}

	#ifdef BULKCRYPTO_DEBUG
		printk("Enter crypto_smau_dmac_configuration\n");

		printk("Input header length %d\n",ctx->header_length);
		printk("Input packet length %d\n",ctx->in_data_length);

		printk("Input header\n");
		for(i=0; i<(ctx->header_length*4);i++) {
			printk("0x%x\t", ctx->SMAU_SRAM_HEADER_ADDR[i]);
		}

		printk("Input data\n");
		for(i=0; i<32;i=i+4)
			printk("0x%x\n",*((u32_t *)&(ctx->in_data
					[i+ctx->msg_hdr->crypt_hdr2->aes_offset])));
		printk("packetcount = %d\n",packet_count);

		printk("Output data pointer before encryption 0x%x  \n",
				(u32_t)(ctx->out_data));

	#endif

		dma_cfg[0].channel_direction = MEMORY_TO_PERIPHERAL;
		dma_cfg[0].source_data_size = 4;
		dma_cfg[0].dest_data_size = 4;
		dma_cfg[0].source_burst_length = 4;
		dma_cfg[0].dest_burst_length = 4;
		dma_cfg[0].dma_callback = NULL;
		dma_cfg[0].complete_callback_en = 0;
		dma_cfg[0].error_callback_en = 0;
		dma_cfg[0].block_count = packet_count;
		dma_cfg[0].head_block = &dma_block_cfg[0];
		dma_cfg[0].endian_mode = 0x00;
		dma_cfg[0].source_peripheral = 0x1F;
		dma_cfg[0].dest_peripheral = 0x5; /* SMAU Inbound */

		/* Add the first packet, i.e. the Header for crypto  */
		dma_block_cfg[0].block_size = (ctx->header_length*4);
		dma_block_cfg[0].source_address = (u32_t)ctx->SMAU_SRAM_HEADER_ADDR;
		dma_block_cfg[0].dest_address = (u32_t)SMU_DMA_IN;
		dma_block_cfg[0].source_addr_adj = ADDRESS_INCREMENT;
		dma_block_cfg[0].dest_addr_adj = ADDRESS_NO_CHANGE;

		/* Add the data packets */
		i = 1;
		while(pendingTransferSize > 0x3FFC) {

			/* Put the next block pointer */
			dma_block_cfg[i-1].next_block = &dma_block_cfg[i];

			dma_block_cfg[i].block_size = 0x3FFC;
			dma_block_cfg[i].source_address = u_srcAddr;
			dma_block_cfg[i].dest_address = (u32_t)SMU_DMA_IN;
			dma_block_cfg[i].source_addr_adj = ADDRESS_INCREMENT;
			dma_block_cfg[i].dest_addr_adj = ADDRESS_NO_CHANGE;

			pendingTransferSize -= 0x3FFC;
			u_srcAddr  += 0x3FFC;
			i++;
		}

		/* Put the next block pointer */
		dma_block_cfg[i-1].next_block = &dma_block_cfg[i];

		/* Add the last packet, i.e. the remainder data bytes */
		dma_block_cfg[i].block_size = pendingTransferSize;
		dma_block_cfg[i].source_address = u_srcAddr;
		dma_block_cfg[i].dest_address = (u32_t)SMU_DMA_IN;
		dma_block_cfg[i].source_addr_adj = ADDRESS_INCREMENT;
		dma_block_cfg[i].dest_addr_adj = ADDRESS_NO_CHANGE;

		if (dma_request(dma, &inchannel, NORMAL_CHANNEL)) {
			printk("ERROR: Channel not free\n");
			break;
		}

		if (dma_config(dma, inchannel, &dma_cfg[0])) {
				printk("ERROR: transfer\n");
				break;
		}

		/* Start the in channel dma, from Memory to SMAU */
		if (dma_start(dma, inchannel)) {
				printk("ERROR: transfer\n");
				break;
		}

		/* Configure the channel 1 for the receive data from SMAU to Memory */
		dma_cfg[1].channel_direction = PERIPHERAL_TO_MEMORY_PER_CTRL;
		dma_cfg[1].source_data_size = 4;
		dma_cfg[1].dest_data_size = 4;
		dma_cfg[1].source_burst_length = 4;
		dma_cfg[1].dest_burst_length = 4;
		dma_cfg[1].dma_callback = NULL;
		dma_cfg[1].complete_callback_en = 0;
		dma_cfg[1].error_callback_en = 0;
		dma_cfg[1].block_count = 1;
		dma_cfg[1].head_block = &dma_block_cfg_1;
		dma_cfg[1].endian_mode = 0x00;
		dma_cfg[1].source_peripheral = 0x06; /* SMAU Outbound */
		dma_cfg[1].dest_peripheral = 0x1F;

		/* Receive the packets in the following block */
		dma_block_cfg_1.block_size = 0;
		dma_block_cfg_1.source_address = (u32_t)SMU_DMA_OUT;
		dma_block_cfg_1.dest_address = (u32_t)ctx->out_data;
		dma_block_cfg_1.source_addr_adj = ADDRESS_NO_CHANGE;
		dma_block_cfg_1.dest_addr_adj = ADDRESS_INCREMENT;

		if (dma_request(dma, &outchannel, NORMAL_CHANNEL)) {
			printk("ERROR: Channel not free\n");
			break;
		}

		/* Configure the out channel for SMAU to memory transfer */
		if (dma_config(dma, outchannel, &dma_cfg[1])) {
				printk("ERROR: transfer\n");
				break;
		}

#ifdef CONFIG_DATA_CACHE_SUPPORT
		/* Clean the D-cache by the crypto length + one block for
		 * authentication if present(max hash output can be 32 byte) */
		outSize = ctx->msg_hdr->crypt_hdr3->aes_length + 64;

		/* Clean the D-cache to make sure if any changes happened
		 * in the cache is flushed to the SRAM area so that there are
		 * no pending requests for the cache clean */
		clean_dcache_by_addr((u32_t)(outPtrOriginal), outSize);
#endif
		/* Start the out channel dma, from SMAU to Memory */
		if (dma_start(dma, outchannel)) {
				printk("ERROR: transfer\n");
				break;
		}

		/* Wait for the in channel to complete */
		dmaStatus = 0x01;
		while(dmaStatus) {
			dma_status(dma, inchannel, &dmaStatus);
		}

	#ifdef BULKCRYPTO_DEBUG
		printk("Input DMA completed\n");
	#endif

		/* Wait for the out channel to complete */
		dmaStatus = 0x01;
		while(dmaStatus) {
			dma_status(dma, outchannel, &dmaStatus);
		}

#ifdef CONFIG_DATA_CACHE_SUPPORT

		/* calculate the last buffer written by the DMA to determine
		 * how much cache has to be invalidated,
		 * +4 is for the burst length for the last burst of dma */
		outSize = (u32_t) (sys_read32((base + PL080_Cx_DST_ADDR(outchannel)))
				              - (u32_t)(outPtrOriginal) + 4);

		/* Invalidate the dcache lines to make sure, the cpu will read
		 * from the SRAM area which is written by the DMA  */
		invalidate_dcache_by_addr((u32_t)(outPtrOriginal), outSize);
#endif

	}while(0);

	/* Release the in and out channels if allocated */
	if (inchannel != 0xFFFF) {
		dma_release(dma, inchannel);
	}

	if (outchannel != 0xFFFF) {
		dma_release(dma, outchannel);
	}

#ifdef CONFIG_CRYPTO_DCACHE_WORKAROUND
/* Use this workaround if the existing solution for the data cache does not
 * work. The existing data cache solution works for all the unit tests and
 * all the test are passing with or without the data cache. */
		enable_dcache();
#endif


#ifdef BULKCRYPTO_DEBUG
    printk("Output DMA completed\n");
	printk("Output data 0x%x  \n",(u32_t)(ctx->out_data));
	for(i=0; i< 32; i=i+4)
	printk("0x%x\n",*((u32_t *)&(ctx->out_data[i/*+ctx->msg_hdr->crypt_hdr2->aes_offset*/])));
	printk("Exit crypto_smau_dmac_configuration\n");
#endif

}

static BCM_SCAPI_STATUS struct_init(smu_bulkcrypto_packet *cur_pkt)
{
	/* FIXME there is kmalloc used here but this is not necessary,
	 * However this is not a bug, but followed the legecy code.
	 * But if you enable the k_malloc version, this needs to be freed.
	 * Otherwise expect crashes.*/
#ifdef BUILD_LYNX
	static u8_t mht[sizeof(msg_hdr_top)] __aligned(4);
	cur_pkt->msg_hdr = (msg_hdr_top *)mht;
#else
	cur_pkt->msg_hdr = (msg_hdr_top *)k_malloc( sizeof(msg_hdr_top));
#endif
	if(cur_pkt->msg_hdr == NULL){
#ifdef BULKCRYPTO_DEBUG
		printk("cur_pkt->msg_hdr k_malloc failed\n");
#endif
		return BCM_SCAPI_STATUS_NOT_ENOUGH_MEMORY;
	}
	memset(cur_pkt->msg_hdr,0,sizeof(msg_hdr_top));

#ifdef BUILD_LYNX
	static u8_t ck[sizeof(struct cryptokey)];
	cur_pkt->cryptokey = (struct cryptokey *)ck;
#else
	cur_pkt->cryptokey = (struct cryptokey *)k_malloc(sizeof( struct cryptokey));
#endif
	if(cur_pkt->cryptokey == NULL){
#ifdef BULKCRYPTO_DEBUG
		printk("cur_pkt->cryptokey k_malloc failed\n");
#endif
		return BCM_SCAPI_STATUS_NOT_ENOUGH_MEMORY;
	}
	memset(cur_pkt->cryptokey,0,sizeof( struct cryptokey));

#ifdef BUILD_LYNX
	static u8_t hk[sizeof(struct hashkey)];
	cur_pkt->hashkey = (struct hashkey  *)hk;
#else
	cur_pkt->hashkey = (struct hashkey  *)k_malloc(sizeof(struct hashkey));
#endif
	if(cur_pkt->hashkey == NULL){
#ifdef BULKCRYPTO_DEBUG
		printk("cur_pkt->hashkey k_malloc failed\n");
#endif
		return BCM_SCAPI_STATUS_NOT_ENOUGH_MEMORY;
	}
	memset(cur_pkt->hashkey,0,sizeof( struct hashkey));

#ifdef BUILD_LYNX
	static u8_t iv[sizeof(struct cryptoiv)];
	cur_pkt->cryptoiv = (struct cryptoiv *)iv;
#else
	cur_pkt->cryptoiv = (struct cryptoiv *)k_malloc(sizeof(struct cryptoiv));
#endif
	if(cur_pkt->cryptoiv == NULL){
#ifdef BULKCRYPTO_DEBUG
		printk("cur_pkt->cryptoiv k_malloc failed\n");
#endif
		return BCM_SCAPI_STATUS_NOT_ENOUGH_MEMORY;
	}
	memset(cur_pkt->cryptoiv,0,sizeof( struct cryptoiv));

#ifdef BUILD_LYNX
	static u8_t mh0[sizeof(struct msg_hdr0)];
	cur_pkt->msg_hdr->crypt_hdr0 = (struct msg_hdr0 *)mh0;
#else
	cur_pkt->msg_hdr->crypt_hdr0 = (struct msg_hdr0 *)k_malloc(sizeof(struct msg_hdr0));
#endif
	if(cur_pkt->msg_hdr->crypt_hdr0 == NULL){
#ifdef BULKCRYPTO_DEBUG
		printk("cur_pkt->crypt_hdr0 k_malloc failed\n");
#endif
		return BCM_SCAPI_STATUS_NOT_ENOUGH_MEMORY;
	}
	*((u32_t *)cur_pkt->msg_hdr->crypt_hdr0) = 0;

#ifdef BUILD_LYNX
	static u8_t mh1[sizeof(struct msg_hdr1)];
	cur_pkt->msg_hdr->crypt_hdr1 = (struct msg_hdr1 *)mh1;
#else
	cur_pkt->msg_hdr->crypt_hdr1 = (struct msg_hdr1 *)k_malloc(sizeof(struct msg_hdr1));
#endif
	if(cur_pkt->msg_hdr->crypt_hdr1 == NULL){
#ifdef BULKCRYPTO_DEBUG
		printk("cur_pkt->crypt_hdr1 k_malloc failed\n");
#endif
		return BCM_SCAPI_STATUS_NOT_ENOUGH_MEMORY;
	}
	*((u32_t *)cur_pkt->msg_hdr->crypt_hdr1) = 0;

#ifdef BUILD_LYNX
	static u8_t mh2[sizeof(struct msg_hdr2)];
	cur_pkt->msg_hdr->crypt_hdr2 = (struct msg_hdr2 *)mh2;
#else
	cur_pkt->msg_hdr->crypt_hdr2 = (struct msg_hdr2 *)k_malloc(sizeof(struct msg_hdr2));
#endif
	if(cur_pkt->msg_hdr->crypt_hdr2 == NULL){
#ifdef BULKCRYPTO_DEBUG
		printk("cur_pkt->crypt_hdr2 k_malloc failed\n");
#endif
		return BCM_SCAPI_STATUS_NOT_ENOUGH_MEMORY;
	}
	*((u32_t *)cur_pkt->msg_hdr->crypt_hdr2) = 0;

#ifdef BUILD_LYNX
	static u8_t mh3[sizeof(struct msg_hdr3)];
	cur_pkt->msg_hdr->crypt_hdr3 = (struct msg_hdr3 *)mh3;
#else
	cur_pkt->msg_hdr->crypt_hdr3 = (struct msg_hdr3 *)k_malloc(sizeof(struct msg_hdr3));
#endif
	if(cur_pkt->msg_hdr->crypt_hdr3 == NULL){
#ifdef BULKCRYPTO_DEBUG
printk("cur_pkt->crypt_hdr3 k_malloc failed\n");
#endif
		return BCM_SCAPI_STATUS_NOT_ENOUGH_MEMORY;
	}
	*((u32_t *)cur_pkt->msg_hdr->crypt_hdr3) = 0;

#ifdef BUILD_LYNX
	static u8_t mh4[sizeof(struct msg_hdr4)];
	cur_pkt->msg_hdr->crypt_hdr4 = (struct msg_hdr4 *)mh4;
#else
	cur_pkt->msg_hdr->crypt_hdr4 = (struct msg_hdr4 *)k_malloc(sizeof(struct msg_hdr4));
#endif
	if(cur_pkt->msg_hdr->crypt_hdr4 == NULL){
#ifdef BULKCRYPTO_DEBUG
		printk("cur_pkt->crypt_hdr4 k_malloc failed\n");
#endif
		return BCM_SCAPI_STATUS_NOT_ENOUGH_MEMORY;
	}
	*((u32_t *)cur_pkt->msg_hdr->crypt_hdr4) = 0;

#ifdef BUILD_LYNX
	static u8_t mh5[sizeof(struct msg_hdr5)];
	cur_pkt->msg_hdr->crypt_hdr5 = (struct msg_hdr5 *)mh5;
#else
	cur_pkt->msg_hdr->crypt_hdr5 = (struct msg_hdr5 *)k_malloc(sizeof(struct msg_hdr5));
#endif
	if(cur_pkt->msg_hdr->crypt_hdr5 == NULL){
#ifdef BULKCRYPTO_DEBUG
		printk("cur_pkt->crypt_hdr5 k_malloc failed\n");
#endif
		return BCM_SCAPI_STATUS_NOT_ENOUGH_MEMORY;
	}
	*((u32_t *)cur_pkt->msg_hdr->crypt_hdr5) = 0;

	return BCM_SCAPI_STATUS_OK;
}
