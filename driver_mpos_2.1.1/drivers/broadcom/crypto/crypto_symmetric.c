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

/* @file crypto_symmetric.c
 *
 * This file contains
 *
 *
 */
/*
 * TODO
 * 1.#ifdef CITADEL_DMA to be integrated once DMA API's are ready
 *
 * */

/* ***************************************************************************
 * Includes
 * ***************************************************************************/
#include <string.h>
#include <misc/printk.h>
#include <logging/sys_log.h>
#include <zephyr/types.h>
#include <broadcom/dma.h>
#include <crypto/crypto.h>
#include <pka/crypto_pka_util.h>
#include <crypto/crypto_rng.h>
#include <crypto/crypto_symmetric.h>
#include <crypto/crypto_smau.h>
#include <misc/util.h>
#include <board.h>
#include <sys_io.h>
#include "crypto_sw_utils.h"
#include "gf8_func.h"

/* ***************************************************************************
 * MACROS/Defines
 * ***************************************************************************/
#define AES_BLOCK_SIZE		16

#define WPA_PUT_BE64(a, val)				\
	do {						\
		(a)[0] = (u8_t) (((u64_t) (val)) >> 56);	\
		(a)[1] = (u8_t) (((u64_t) (val)) >> 48);	\
		(a)[2] = (u8_t) (((u64_t) (val)) >> 40);	\
		(a)[3] = (u8_t) (((u64_t) (val)) >> 32);	\
		(a)[4] = (u8_t) (((u64_t) (val)) >> 24);	\
		(a)[5] = (u8_t) (((u64_t) (val)) >> 16);	\
		(a)[6] = (u8_t) (((u64_t) (val)) >> 8);	\
		(a)[7] = (u8_t) (((u64_t) (val)) & 0xff);	\
	} while (0)

#define WPA_GET_BE32(a) ((((u32_t) (a)[0]) << 24) | \
			 (((u32_t) (a)[1]) << 16) | \
			 (((u32_t) (a)[2]) << 8) |  \
			 ((u32_t) (a)[3]))

#define WPA_PUT_BE32(a, val)					\
	do {							\
		(a)[0] = (u8_t) ((((u32_t) (val)) >> 24) & 0xff);	\
		(a)[1] = (u8_t) ((((u32_t) (val)) >> 16) & 0xff);	\
		(a)[2] = (u8_t) ((((u32_t) (val)) >> 8) & 0xff);	\
		(a)[3] = (u8_t) (((u32_t) (val)) & 0xff);		\
	} while (0)

#define DPA_REGS_AES_CFG		(DPA_BASE_ADDR + 0x00)
#define IFSR_SEED_LOAD_DATA_BIT		(7)
#define IFSR_SEED_LOAD_KEYS_BIT		(6)
#define AES_DATA_DPA_ENABLE_BIT		(1)
#define AES_KEY_DPA_ENABLE_BIT		(0)

#define DPA_REGS_KEY_1_SEED		(DPA_BASE_ADDR + 0x04)
#define DPA_REGS_DATA_1_SEED		(DPA_BASE_ADDR + 0x30)
#define DPA_REGS_CRKEY_MASK_1		(DPA_BASE_ADDR + 0x60)
#define DPA_REGS_DES_CFG		(DPA_BASE_ADDR + 0xd0)
#define DES_DPA_ENABLE_BIT		(4)
#define IFSR_SEED_LOAD			(7)

/* Data and Key seed registers for AES DPA */
#define DPA_AES_SEED_REGISTERS		(17)

/* ***************************************************************************
 * Types/Structure Declarations
 * ***************************************************************************/
static fips_rng_context rngctx;
static u8_t cryptoHandle[CRYPTO_LIB_HANDLE_SIZE];

enum DPA_POLICY {
	DPA_NONE = 0,      /* Do not perform DPA */
	DPA_LOW = 0,       /* Perform low level DPA operation */
	DPA_HIGH = 1,      /* Perform high level DPA operation */
};

static u32_t des_dpa_ratio = 10;
static u32_t des_dpa_policy = DPA_NONE;

/* ***************************************************************************
 * Global and Static Variables
 * Total Size: NNNbytes
 * ***************************************************************************/

/* ***************************************************************************
 * Private Functions Prototypes
 * ****************************************************************************/
static BCM_SCAPI_STATUS crypto_symmetric_aes(crypto_lib_handle *pHandle,
											 u8_t *pMsg, u32_t nMsgLen,
											 u8_t *pAESKey,u32_t AESKeyLen,
											 u8_t *pCipherIV, u32_t crypto_len,
											 u32_t crypto_offset, u8_t *output,
											 BCM_SCAPI_CIPHER_MODE cipher_mode);
static void aes_gcm_ghash(const u8_t *H, const u8_t *aad, size_t aad_len,
			  const u8_t *crypt, size_t crypt_len, u8_t *S);

static void aes_gcm_prepare_j0(const u8_t *iv, size_t iv_len, const u8_t *H,
			       u8_t *J0);

static BCM_SCAPI_STATUS aes_gcm_gctr(crypto_lib_handle *pHandle,
				     const u8_t *key, size_t key_len,
				     const u8_t *J0, const u8_t *in,
				     size_t len, u8_t *out);

static BCM_SCAPI_STATUS aes_gctr(crypto_lib_handle *pHandle, const u8_t *key,
				 size_t key_len, const u8_t *icb, const u8_t *x,
				 size_t xlen, u8_t *y);

static BCM_SCAPI_STATUS crypto_aes_ecb_encrypt(crypto_lib_handle *pHandle,
					       const u8_t *key,
					       u32_t key_length, u8_t *plain,
					       u32_t plain_length,
					       u8_t *cipher);

static BCM_SCAPI_STATUS crypto_des_encrypt(crypto_lib_handle *pHandle,
					   u8_t *key, u32_t key_length,
					   u8_t *plain, u32_t plain_length,
					   u8_t *iv, u32_t iv_length,
					   u8_t *cipher);

static BCM_SCAPI_STATUS crypto_des_decrypt(crypto_lib_handle *pHandle,
					   u8_t *key, u32_t key_length,
					   u8_t *cipher, u32_t cipher_length,
					   u8_t *iv, u32_t iv_length,
					   u8_t *plain);

static BCM_SCAPI_STATUS crypto_3des_encrypt(crypto_lib_handle *pHandle,
					    u8_t *key, u32_t key_length,
					    u8_t *plain, u32_t plain_length,
					    u8_t *iv, u32_t iv_length,
					    u8_t *cipher);

static BCM_SCAPI_STATUS crypto_3des_decrypt(crypto_lib_handle *pHandle,
					    u8_t *key, u32_t key_length,
					    u8_t *cipher, u32_t cipher_length,
					    u8_t *iv, u32_t iv_length,
					    u8_t *plain);
/* ***************************************************************************
 * Public Functions
 * ****************************************************************************/
/*---------------------------------------------------------------
 * Name    : crypto_symmetric_hmac_sha1
 * Purpose : Perform the HMAC SHA1 or SHA1 operation
 *           (depends on if the keys are passed in), output the hash
 * Input   : the crypto library handle, the original message, the key
 *           //fSwapIn: if needs to swap input in bytes
 *           (this will result in swap the smau header ourselves.
 * Output  : the calculated digest
 * Return  : Appropriate status
 * Remark  : Will use DMA Controller 0
 * Note    : Always swaps endianness on input and output
 *--------------------------------------------------------------*/
BCM_SCAPI_STATUS crypto_symmetric_hmac_sha1(crypto_lib_handle *pHandle,
										u8_t *pMsg, u32_t nMsgLen,
										u8_t *pKey, u32_t nKeyLen,
										u8_t *output, u32_t bSwap)
{
	BCM_SCAPI_STATUS status;
	bcm_bulk_cipher_context ctx;
	BCM_BULK_CIPHER_CMD     cmd;
	ARG_UNUSED(bSwap);

	if ((pKey && (nKeyLen == 0)) || ((pKey == NULL) && nKeyLen)) {
		return BCM_SCAPI_STATUS_PARAMETER_INVALID;
	}

	cmd.cipher_mode  = BCM_SCAPI_CIPHER_MODE_AUTHONLY;
	cmd.encr_alg     = BCM_SCAPI_ENCR_ALG_NONE;
	cmd.encr_mode    = BCM_SCAPI_ENCR_MODE_NONE;
	cmd.auth_alg     = BCM_SCAPI_AUTH_ALG_SHA1;
	cmd.auth_mode    = (pKey?BCM_SCAPI_AUTH_MODE_HMAC:BCM_SCAPI_AUTH_MODE_HASH);
	cmd.auth_optype  = BCM_SCAPI_AUTH_OPTYPE_FULL;
	cmd.cipher_order = BCM_SCAPI_CIPHER_ORDER_NULL;

	status = crypto_smau_cipher_init(pHandle, &cmd, NULL, pKey, nKeyLen, &ctx);
	if (status != BCM_SCAPI_STATUS_OK)
		return status;

	status = crypto_smau_cipher_start(pHandle, pMsg, nMsgLen, NULL, 0, 0,
								    nMsgLen, 0, output, &ctx);

	crypto_smau_cipher_deinit(pHandle, &ctx);

	return status;
}

/**
 * @brief Perform the HMAC SHA or SHA part operation
 *        (depends on if the keys are passed in), output the hash
 *
 * @param handle handle
 * @param msg Pointer to message
 * @param total_len Total length of the data. Required only in finish operation
 * @param msg_len message length
 * @param key Pointer to key
 * @param key_len key length
 * @param alg either SHA256 or SHA1
 * @param opt opt type (Init/ update /final)
 * @param previous output in Init/Update operation
 * @param output Pointer to output
 *
 * @return BCM_SCAPI_STATUS
 *
 * @note Will use DMA Controller 0
 *       Always swaps endianness on input and output
 */
static BCM_SCAPI_STATUS _crypto_symmetric_sha_op(crypto_lib_handle *handle,
						 u8_t *msg, u32_t msg_len,
						 u32_t total_len,
						 u8_t *key, u32_t key_len,
						 BCM_SCAPI_AUTH_ALG alg,
						 BCM_SCAPI_AUTH_OPTYPE opt,
						 u8_t *previous, u8_t *output)
{
	BCM_SCAPI_STATUS status;
	bcm_bulk_cipher_context ctx;
	BCM_BULK_CIPHER_CMD cmd;
	u8_t hash_len;
	u8_t *data;
	u32_t data_len;

	if ((handle == NULL) || (msg == NULL) || (output == NULL))
		return BCM_SCAPI_STATUS_PARAMETER_INVALID;

	if (alg == BCM_SCAPI_AUTH_ALG_SHA256)
		hash_len = SHA256_HASH_SIZE;
	else if (alg == BCM_SCAPI_AUTH_ALG_SHA1)
		hash_len = SHA1_HASH_SIZE;
	else
		return BCM_SCAPI_STATUS_PARAMETER_INVALID;

	cmd.cipher_mode = BCM_SCAPI_CIPHER_MODE_AUTHONLY;
	cmd.encr_alg = BCM_SCAPI_ENCR_ALG_NONE;
	cmd.encr_mode = BCM_SCAPI_ENCR_MODE_NONE;
	cmd.auth_alg = alg;
	cmd.auth_mode = (key ?
			BCM_SCAPI_AUTH_MODE_HMAC:BCM_SCAPI_AUTH_MODE_HASH);
	cmd.auth_optype  = opt;
	cmd.cipher_order = BCM_SCAPI_CIPHER_ORDER_NULL;

	status = crypto_smau_cipher_init(handle, &cmd, NULL, key,
					 key_len, &ctx);
	if (status != BCM_SCAPI_STATUS_OK) {
#ifdef CRYPTO_DEBUG
		printk("Cipher init failed %d\n", status);
#endif
		return status;
	}

	switch (opt) {
	case BCM_SCAPI_AUTH_OPTYPE_UPDATE:
	case BCM_SCAPI_AUTH_OPTYPE_FINAL:
		data_len = msg_len + hash_len;
		data = cache_line_aligned_alloc(data_len);
		if (data == NULL)
			return BCM_SCAPI_STATUS_NOT_ENOUGH_MEMORY;

		memcpy(data, previous, hash_len);
		memcpy(data + hash_len, msg, msg_len);
		break;

	case BCM_SCAPI_AUTH_OPTYPE_INIT:
		ctx.pad_length = 0;
		data_len = msg_len;
		data = msg;
		break;

	default:
		return BCM_SCAPI_STATUS_PARAMETER_INVALID;
	}

	if (opt == BCM_SCAPI_AUTH_OPTYPE_FINAL)
		ctx.pad_length = total_len;

	status = crypto_smau_cipher_start(handle, data, data_len, NULL, 0, 0,
						data_len, 0, output, &ctx);

	if ((opt == BCM_SCAPI_AUTH_OPTYPE_UPDATE) ||
	    (opt == BCM_SCAPI_AUTH_OPTYPE_FINAL))
		cache_line_aligned_free(data);

	crypto_smau_cipher_deinit(handle, &ctx);

#ifdef CRYPTO_DEBUG
	printk("Exit crypto_symmetric_hmac_sha256\n");
#endif

	return status;
}


/**
 * @brief Perform the HMAC SHA1 (depends on if the keys are passed in) or
 *        SHA1 init operation with hash output. This function is required in
 *        case of generating hash using parts of the complete data.
 *
 * @param handle handle
 * @param alg Either SHA256 or SHA1
 * @param msg Pointer to message
 * @param msg_len message length
 * @param key Pointer to key
 * @param key_len key length
 * @param output Pointer to output
 *
 * @return BCM_SCAPI_STATUS
 *
 * @note Will use DMA Controller 0
 *       Always swaps endianness on input and output. The message length
 *       should be SHA block aligned.
 */
BCM_SCAPI_STATUS crypto_symmetric_hmac_sha_init(crypto_lib_handle *handle,
						BCM_SCAPI_AUTH_ALG alg,
						u8_t *msg, u32_t msg_len,
						u8_t *key, u32_t key_len,
						u8_t *output)
{
	return _crypto_symmetric_sha_op(handle, msg, msg_len, 0, key, key_len,
					alg, BCM_SCAPI_AUTH_OPTYPE_INIT,
					NULL, output);
}


/**
 * @brief Perform the HMAC SHA1 (depends on if the keys are passed in)or
 *        SHA1 update operation with hash output
 *
 * @param handle handle
 * @param alg Either SHA256 or SHA1
 * @param msg Pointer to message
 * @param msg_len message length
 * @param key Pointer to key
 * @param key_len key length
 * @param pre_sha previous output
 * @param output Pointer to output
 *
 * @return BCM_SCAPI_STATUS
 *
 * @note Will use DMA Controller 0
 *       Always swaps endianness on input and output. The message length
 *       should be SHA block aligned.
 */
BCM_SCAPI_STATUS crypto_symmetric_hmac_sha_update(crypto_lib_handle *handle,
						  BCM_SCAPI_AUTH_ALG alg,
						  u8_t *msg, u32_t msg_len,
						  u8_t *key, u32_t key_len,
						  u8_t *pre_sha, u8_t *output)
{
	return _crypto_symmetric_sha_op(handle, msg, msg_len, 0, key, key_len,
					alg, BCM_SCAPI_AUTH_OPTYPE_UPDATE,
					pre_sha, output);
}

/**
 * @brief Perform the HMAC SHA (depends on if the keys are passed in) or
 *        SHA finish operation with hash output
 *
 * @param handle handle
 * @param alg Either SHA256 or SHA1
 * @param msg Pointer to message
 * @param msg_len message length
 * @param total_len Total length of the data. Required only in finish operation
 * @param key Pointer to key
 * @param key_len key length
 * @param pre_sha Pointer to previous output
 * @param output Pointer to output
 *
 * @return BCM_SCAPI_STATUS
 *
 * @note Will use DMA Controller 0
 *       Always swaps endianness on input and output
 */
BCM_SCAPI_STATUS crypto_symmetric_hmac_sha_final(crypto_lib_handle *handle,
						 BCM_SCAPI_AUTH_ALG alg,
						 u8_t *msg, u32_t msg_len,
						 u32_t total_len,
						 u8_t *key, u32_t key_len,
						 u8_t *pre_sha, u8_t *output)
{
	return _crypto_symmetric_sha_op(handle, msg, msg_len, total_len, key,
					key_len, alg,
					BCM_SCAPI_AUTH_OPTYPE_FINAL,
					pre_sha, output);
}

/*---------------------------------------------------------------
 * Name    : crypto_symmetric_hmac_sha256
 * Purpose : Perform the HMAC SHA256 or SHA256 operation
 * 			 (depends on if the keys are passed in), output the hash
 * Input   : the crypto library handle, the original message, the key
 *           //fSwapIn: if needs to swap input in bytes
 *           (this will result in swap the smau header ourselves.
 * Output  : the calculated digest
 * Return  : Appropriate status
 * Remark  : Will use DMA Controller 0
 * Note    : Always swaps endianness on input and output
 *--------------------------------------------------------------*/
BCM_SCAPI_STATUS crypto_symmetric_hmac_sha256(crypto_lib_handle *pHandle,
										u8_t *pMsg, u32_t nMsgLen,
										u8_t *pKey, u32_t nKeyLen,
										u8_t *output, u32_t bSwap)
{
	BCM_SCAPI_STATUS status;
	bcm_bulk_cipher_context ctx;
	BCM_BULK_CIPHER_CMD     cmd;
	ARG_UNUSED(bSwap);

#ifdef CRYPTO_DEBUG
printk("Enter crypto_symmetric_hmac_sha256\n");
#endif
	if ((pKey && (nKeyLen == 0)) || ((pKey == NULL) && nKeyLen)) {
#ifdef CRYPTO_DEBUG
printk("Key not valid\n");
#endif
		return BCM_SCAPI_STATUS_PARAMETER_INVALID;
	}

	cmd.cipher_mode  = BCM_SCAPI_CIPHER_MODE_AUTHONLY;
	cmd.encr_alg     = BCM_SCAPI_ENCR_ALG_NONE;
	cmd.encr_mode    = BCM_SCAPI_ENCR_MODE_NONE;
	cmd.auth_alg     = BCM_SCAPI_AUTH_ALG_SHA256;
	cmd.auth_mode    = (pKey?BCM_SCAPI_AUTH_MODE_HMAC:BCM_SCAPI_AUTH_MODE_HASH);
	cmd.auth_optype  = BCM_SCAPI_AUTH_OPTYPE_FULL;
	cmd.cipher_order = BCM_SCAPI_CIPHER_ORDER_AUTH_CRYPT;

	status = crypto_smau_cipher_init(pHandle, &cmd, NULL, pKey, nKeyLen, &ctx);
	if (status != BCM_SCAPI_STATUS_OK) {
#ifdef CRYPTO_DEBUG
		printk("Cipher init failed %d\n",status);
#endif
		return status;
	}

	status = crypto_smau_cipher_start(pHandle, pMsg,nMsgLen, NULL, 0, 0,
								nMsgLen, 0, output, &ctx);

	crypto_smau_cipher_deinit(pHandle, &ctx);

#ifdef CRYPTO_DEBUG
	printk("Exit crypto_symmetric_hmac_sha256\n");
#endif

	return status;
}

/*---------------------------------------------------------------
 * Name    : crypto_symmetric_hmac_sha3
 * Purpose : Perform the Sha3 operation
 *
 * Input   :
 * Output  :
 * Return  :
 * Remark  :
 * Note    :
 *--------------------------------------------------------------*/
BCM_SCAPI_STATUS crypto_symmetric_sha3(crypto_lib_handle *pHandle,
										u8_t *pMsg, u32_t nMsgLen,
										u8_t *output, BCM_SCAPI_AUTH_ALG eAlg)
{
	BCM_SCAPI_STATUS status;
	bcm_bulk_cipher_context ctx;
	BCM_BULK_CIPHER_CMD     cmd;

#ifdef CRYPTO_DEBUG
printk("Enter crypto_symmetric_hmac_sha256\n");
#endif

	cmd.cipher_mode  = BCM_SCAPI_CIPHER_MODE_AUTHONLY;
	cmd.encr_alg     = BCM_SCAPI_ENCR_ALG_NONE;
	cmd.encr_mode    = BCM_SCAPI_ENCR_MODE_NONE;
	cmd.auth_alg     = (u8_t)(eAlg);
	cmd.auth_mode    = BCM_SCAPI_AUTH_MODE_HASH;
	cmd.auth_optype  = BCM_SCAPI_AUTH_OPTYPE_FULL;
	cmd.cipher_order = BCM_SCAPI_CIPHER_ORDER_AUTH_CRYPT;

	status = crypto_smau_cipher_init(pHandle, &cmd, NULL, NULL, 0, &ctx);
	if (status != BCM_SCAPI_STATUS_OK) {
#ifdef CRYPTO_DEBUG
		printk("Cipher init failed %d\n",status);
#endif
		return status;
	}

	status = crypto_smau_cipher_start(pHandle, pMsg,nMsgLen, NULL, 0, 0,
								nMsgLen, 0, output, &ctx);

	crypto_smau_cipher_deinit(pHandle, &ctx);

#ifdef CRYPTO_DEBUG
printk("Exit crypto_symmetric_hmac_sha256\n");
#endif

	return status;
}

/*---------------------------------------------------------------
 * Name    : crypto_symmetric_fips_aes
 * Purpose : Perform a generic AES operation
 * Input   : [in]  pHandle : crypto context handle
 *           [in]  encr_mode : encryption mode (CBC, ECB, CTR)
 *           [in]  encr_alg : ecryption alg (AES 128, 192, 256)
 *           [in]  cipher_mode : encrypt or decrypt
 *           [in]  pAESKey : buffer containing AES key
 *           [in]  pAESIV : buffer containing AES IV
 *           [in]  pMsg : buffer containing AAD (padded) + Msg (padded)
 *           [in]  nMsgLen : length of Msg
 *           [in]  crypto_len : length of Msg (unpadded)
 *           [in]  crypto_offset : length of AAD (unpadded, without length field)
 *           [out] output : output buffer
 * Output  : the calculated aes enc or dec
 * Return  : Appropriate status
 * Remark  : Will use DMA Controller 0
 *  nMsgLen : total message length including padding
 *  crypto_len : plain/cipher text length and pad length (actual)
 * Note    : Always swaps endianness on input and output
 *
 *--------------------------------------------------------------*/
BCM_SCAPI_STATUS crypto_symmetric_fips_aes(crypto_lib_handle *pHandle,
			BCM_SCAPI_ENCR_MODE encr_mode,
			BCM_SCAPI_ENCR_ALG encr_alg,
			BCM_SCAPI_CIPHER_MODE cipher_mode,
			u8_t *pAESKey, u8_t *pAESIV,
			u8_t *pMsg, u32_t nMsgLen,
			u32_t crypto_len, u32_t crypto_offset,
			u8_t *output)
{
	BCM_SCAPI_STATUS        status;
	bcm_bulk_cipher_context ctx;
	BCM_BULK_CIPHER_CMD     cmd;

	/*
	 * don't use this for CCM and GCM,
	 * we have a dedicated function for that
	 */
	if ((encr_mode == BCM_SCAPI_ENCR_MODE_CCM) ||
			(encr_mode == BCM_SCAPI_ENCR_MODE_GCM))
		return BCM_SCAPI_STATUS_PARAMETER_INVALID;

	cmd.cipher_mode  = cipher_mode;
	cmd.encr_mode    = encr_mode;
	cmd.auth_alg     = BCM_SCAPI_AUTH_ALG_NULL;
	cmd.auth_mode    = BCM_SCAPI_AUTH_MODE_HASH;
	cmd.auth_optype  = BCM_SCAPI_AUTH_OPTYPE_INIT;
	cmd.cipher_order = BCM_SCAPI_CIPHER_ORDER_NULL;
	cmd.encr_alg     = encr_alg;

	status = crypto_smau_cipher_init(pHandle, &cmd, pAESKey, NULL, 0, &ctx);
	if (BCM_SCAPI_STATUS_OK != status)
		return status;

	/* setup pad_length properly */
	output[0] = 0x00;

	status = crypto_smau_cipher_start(pHandle, pMsg, nMsgLen, pAESIV,
					crypto_len, crypto_offset, 0, 0,
					output, &ctx);
	if (status != BCM_SCAPI_STATUS_OK)
		return status;

	crypto_smau_cipher_deinit(pHandle, &ctx);

	return status;
}


/*---------------------------------------------------------------
 * Name    : crypto_symmetric_aes_ccm
 * Purpose : Perform the AES CCM operation
 * Input   : [in]  pHandle : crypto context handle
 *           [in]  encr_alg : ecryption alg (AES 128, 192, 256)
 *           [in]  cipher_mode : encrypt or decrypt
 *           [in]  pAESKey : buffer containing AES key
 *           [in]  pAESIV : buffer containing AES IV
 *           [in]  pMac : buffer containing MAC (can be in pMsg)
 *           [in]  nMacLen : length of MAC
 *           [in]  pMsg : buffer containing AAD (padded) + Msg (padded)
 *           [in]  nMsgLen : length of Msg
 *           [in]  crypto_len : length of Msg (unpadded)
 *           [in]  crypto_offset : length of AAD (unpadded, without length field)
 *           [out] output : output buffer
 * Output  : the calculated digest
 * Return  : Appropriate status
 * Remark  : Will use DMA Controller 0
 *  nMsgLen : total message length including padding
 *  crypto_len : plain/cipher text length and pad length (actual)
 *  Note    : Always swaps endianness on input and output
 *--------------------------------------------------------------*/
BCM_SCAPI_STATUS crypto_symmetric_aes_ccm(crypto_lib_handle *pHandle,
				BCM_SCAPI_ENCR_ALG encr_alg,
				BCM_SCAPI_CIPHER_MODE cipher_mode,
				u8_t *pAESKey, u8_t *pAESIV,
				u8_t *pMac, u32_t nMacLen,
				u8_t *pMsg, u32_t nMsgLen,
				u32_t crypto_len, u32_t crypto_offset,
				u8_t *output)
{
	BCM_SCAPI_STATUS        status;
	bcm_bulk_cipher_context ctx;
	BCM_BULK_CIPHER_CMD     cmd;
	unsigned paddedMsgLen = ((crypto_len + 0xf) & 0xfffffff0);

	if (nMacLen > 16)
		return BCM_SCAPI_STATUS_PARAMETER_INVALID;

	cmd.cipher_mode  = cipher_mode;
	cmd.encr_mode    = BCM_SCAPI_ENCR_MODE_CCM;
	cmd.auth_alg     = BCM_SCAPI_AUTH_ALG_NULL;
	cmd.auth_mode    = BCM_SCAPI_AUTH_MODE_HASH;
	cmd.auth_optype  = BCM_SCAPI_AUTH_OPTYPE_INIT;
	cmd.cipher_order = BCM_SCAPI_CIPHER_ORDER_NULL;
	cmd.encr_alg     = encr_alg;

	status = crypto_smau_cipher_init(pHandle, &cmd, pAESKey, NULL, 0, &ctx);
	if (BCM_SCAPI_STATUS_OK != status)
		return status;

	/* setup pad_length properly */
	output[0] = 0x00;

	status = crypto_smau_cipher_start(pHandle, pMsg, nMsgLen, pAESIV,
					crypto_len, crypto_offset,
					0, 0,
					output, &ctx);
	if (status != BCM_SCAPI_STATUS_OK)
		return status;

	/* move the response to the front of the buffer */
	secure_memcpy(output, output + AES_IV_SIZE, crypto_len);

	if (cipher_mode == BCM_SCAPI_CIPHER_MODE_ENCRYPT) {
		/* copy the result mac into place */
		secure_memcpy(output + crypto_len,
			      output + AES_IV_SIZE + paddedMsgLen,
			      nMacLen);
	} else {
		if (!secure_memeql(pMac,
				   output + AES_IV_SIZE + paddedMsgLen,
				   nMacLen)) {
			/* CCM standard says: no output if MAC is invalid,
			 * so erase what the HW reported,
			 * HW macLen is always 16 */
			secure_memclr(output, AES_IV_SIZE + paddedMsgLen + 16);
			status = BCM_SCAPI_STATUS_CRYPTO_NOT_DONE;
		}
	}

	crypto_smau_cipher_deinit(pHandle, &ctx);

	return status;
}

/**
 * This API Perform the AES GCM encryption operation
 *
 * @param[in] pHandle : Pointer to the crypto context
 * @param[in] key : buffer containing AES key
 * @param[in] key_len : length of the AES key
 * @param[in] iv : buffer containing AES IV
 * @param[in] iv_len : length of the IV
 * @param[in] plain : plain text buffer pointer
 * @param[in] plain_len : plain text length
 * @param[in] aad : Additional authenticated data
 * @param[in] aad_len : aad length
 * @param[out] crypt : cipher pointer
 * @param[out] tag : tag pointer
 *
 * @return Status of the Operation
 * @retval #BCM_SCAPI_STATUS
 */
BCM_SCAPI_STATUS crypto_symmetric_aes_gcm_encrypt(crypto_lib_handle *pHandle,
						  const u8_t *key,
						  size_t key_len,
						  const u8_t *iv, size_t iv_len,
						  const u8_t *plain,
						  size_t plain_len,
						  const u8_t *aad,
						  size_t aad_len,
						  u8_t *crypt, u8_t *tag)
{
	BCM_SCAPI_STATUS rv;
	u8_t H[AES_BLOCK_SIZE];
	u8_t J0[AES_BLOCK_SIZE];
	u8_t S[16];

	memset(H, 0, AES_BLOCK_SIZE);
	rv = crypto_aes_ecb_encrypt(pHandle, key, key_len/8, H, AES_BLOCK_SIZE,
									H);
	if (rv)
		return rv;

	aes_gcm_prepare_j0(iv, iv_len, H, J0);

	/* C = GCTR_K(inc_32(J_0), P) */
	rv = aes_gcm_gctr(pHandle, key, key_len, J0, plain, plain_len, crypt);
	if (rv)
		return rv;

	aes_gcm_ghash(H, aad, aad_len, crypt, plain_len, S);

	/* T = MSB_t(GCTR_K(J_0, S)) */
	aes_gctr(pHandle, key, key_len, J0, S, sizeof(S), tag);

	/* The output is AAD || C || T */

	return 0;
}

/**
 * This API Perform the AES GCM decryption operation
 *
 * @param[in] pHandle : Pointer to the crypto context
 * @param[in] key : buffer containing AES key
 * @param[in] key_len : length of the AES key
 * @param[in] iv : buffer containing AES IV
 * @param[in] iv_len : length of the IV
 * @param[in] crypt : cipher text buffer pointer
 * @param[in] crypt_len : cipher text length
 * @param[in] aad : Additional authenticated data
 * @param[in] aad_len : aad length
 * @param[] tag : tag pointer
 * @param[out] plain : plain text buffer pointer
 *
 * @return Status of the Operation
 * @retval #BCM_SCAPI_STATUS
 */
BCM_SCAPI_STATUS crypto_symmetric_aes_gcm_decrypt(crypto_lib_handle *pHandle,
						  const u8_t *key,
						  size_t key_len,
						  const u8_t *iv, size_t iv_len,
						  const u8_t *crypt,
						  size_t crypt_len,
						  const u8_t *aad,
						  size_t aad_len,
						  const u8_t *tag, u8_t *plain)
{
	BCM_SCAPI_STATUS rv;
	u8_t H[AES_BLOCK_SIZE];
	u8_t J0[AES_BLOCK_SIZE];
	u8_t S[16], T[16];

	memset(H, 0, AES_BLOCK_SIZE);
	rv = crypto_aes_ecb_encrypt(pHandle, key, key_len/8, H, AES_BLOCK_SIZE,
		 H);
	if (rv)
		return rv;

	aes_gcm_prepare_j0(iv, iv_len, H, J0);

	/* P = GCTR_K(inc_32(J_0), C) */
	rv = aes_gcm_gctr(pHandle, key, key_len, J0, crypt, crypt_len, plain);
	if (rv)
		return rv;

	aes_gcm_ghash(H, aad, aad_len, crypt, crypt_len, S);

	/* T' = MSB_t(GCTR_K(J_0, S)) */
	rv = aes_gctr(pHandle, key, key_len, J0, S, sizeof(S), T);
	if (rv)
		return rv;

	if (memcmp(tag, T, 16) != 0) {
		SYS_LOG_DBG("TAG Mismatch\n");
		return -1;
	}

	return 0;
}

BCM_SCAPI_STATUS crypto_symmetric_aes_hmacsha256(crypto_lib_handle *pHandle,
									u8_t *pMsg, u32_t nMsgLen,
									u8_t *pAESKey, u32_t AESKeyLen,
									u8_t *pAuthKey, u32_t AuthKeylen,
									u8_t *pCipherIV,
									u32_t crypto_len,u32_t crypto_offset,
									u32_t auth_len,u32_t auth_offset,
			     		    		u8_t *HashOutput,
									BCM_SCAPI_CIPHER_MODE cipher_mode,
									BCM_SCAPI_CIPHER_ORDER cipher_order)
{
	BCM_SCAPI_STATUS status;
	bcm_bulk_cipher_context ctx;
	BCM_BULK_CIPHER_CMD     cmd;
#ifndef BUILD_LYNX
	if (( nMsgLen > SPUM_MAX_PAYLOAD_SIZE)&&(pAESKey)) {
		/* This is a AES+ HMAC Combo case */
		if (cipher_order == BCM_SCAPI_CIPHER_ORDER_AUTH_CRYPT) {
			status =  crypto_symmetric_hmac_sha256(pHandle, pMsg, nMsgLen,
									   pAuthKey, AuthKeylen,
									   (u8_t *)((u32_t)HashOutput+crypto_len),
									   FALSE );
			if(status == BCM_SCAPI_STATUS_OK)
				status = crypto_symmetric_aes(pHandle,pMsg, nMsgLen,
						                pAESKey, AESKeyLen,
										pCipherIV,
										crypto_len,crypto_offset,
										HashOutput, cipher_mode);

			return status;
		} else if (cipher_order == BCM_SCAPI_CIPHER_ORDER_CRYPT_AUTH) {
			status = crypto_symmetric_aes(pHandle,pMsg, nMsgLen, pAESKey,AESKeyLen,
									pCipherIV,
									crypto_len,crypto_offset,
									HashOutput, cipher_mode);
			if(status == BCM_SCAPI_STATUS_OK) {
				status = crypto_symmetric_hmac_sha256(pHandle, pMsg, nMsgLen,
												   pAuthKey, AuthKeylen,
												   (u8_t *)
												   ((u32_t)HashOutput+
														   crypto_len),
												   FALSE );
			}

			return status;
		}
	}
#endif /* BUILD_LYNX */

	if(pAESKey)
	{
		cmd.cipher_mode  = cipher_mode;    			  /* encrypt or decrypt */
		if(AESKeyLen == AES_128_KEY_LEN_BYTES)
		cmd.encr_alg = BCM_SCAPI_ENCR_ALG_AES_128; 	  /* encryption algorithm */
		else if (AESKeyLen == AES_256_KEY_LEN_BYTES)
		cmd.encr_alg = BCM_SCAPI_ENCR_ALG_AES_256; 	  /* encryption algorithm */
		else
		return BCM_SCAPI_STATUS_INVALID_AES_KEYSIZE;

		cmd.encr_mode    = BCM_SCAPI_ENCR_MODE_CBC;
		cmd.cipher_order = cipher_order;
	}
	else
	{
		cmd.cipher_mode  = BCM_SCAPI_CIPHER_MODE_AUTHONLY;
		cmd.encr_alg 	 = BCM_SCAPI_ENCR_ALG_NONE;
		cmd.encr_mode 	 = BCM_SCAPI_ENCR_MODE_NONE;
		cmd.cipher_order = BCM_SCAPI_CIPHER_ORDER_NULL;
	}

	/* authentication algorithm */
	cmd.auth_alg   = BCM_SCAPI_AUTH_ALG_SHA256;
	cmd.auth_mode  = pAuthKey?BCM_SCAPI_AUTH_MODE_HMAC:BCM_SCAPI_AUTH_MODE_HASH;
	cmd.auth_optype = BCM_SCAPI_AUTH_OPTYPE_FULL;

	status = crypto_smau_cipher_init ( pHandle,
									&cmd,
									pAESKey,
									pAuthKey,
									AuthKeylen,
									&ctx);
	if ( status != BCM_SCAPI_STATUS_OK) {
#ifdef CRYPTO_DEBUG
		printk("Cipher Init Failed\n");
#endif
		return status;
	}

	status = crypto_smau_cipher_start(pHandle,pMsg,nMsgLen, pCipherIV,
									crypto_len, crypto_offset,
									auth_len, auth_offset,
									HashOutput,&ctx);

#ifdef CRYPTO_DEBUG
	{
	u32_t i;
	printk("*** bcm_aes_hmacsha256 HashOutput ***\n");
	for(i=0; i< crypto_len+SHA256_HASH_SIZE; i+=4)
	printk("0x%x 0x%x 0x%x 0x%x \n", *(HashOutput + i),
			*(HashOutput + i + 1), *(HashOutput + i + 2),
			*(HashOutput + i + 3));
	}
#endif

	crypto_smau_cipher_deinit(pHandle, &ctx);

	return status;
}

/*---------------------------------------------------------------
 * Name    : bcm_cust_aes_crypto
 * Purpose : Crypto function to encrypt/decrypt/Auth using AES with cipher mode ECB/CBC
 * Input   : pointers to the data parameters
 *			pHandle : Pointer to the Crypto handle
 *			pMsg  : Pointer to the Message to be encrypted or decrypted
 *			nMsgLen : Length of the message.
 *			pAESKey : Pointer to the string containg AES Key
 *			AESKeyLen  : Length of the AES key 128/256
 *			pCipherIV : pointer to Cipher Init Vector ( For ECB this should be NULL)
 *			crypto_len : Length of the crypto
 *			outputCipherText : Pointer to the output string to store the results.
 *			cipher_mode: Mode to be specified ENCRYPTION/DECRYPTION
 *			encr_mode : Encryption mode to be used ECB or CBC
 * Return  : if success (=0) returns the output
 *--------------------------------------------------------------*/
BCM_SCAPI_STATUS crypto_symmetric_cust_aes_crypto(
									 crypto_lib_handle *pHandle,u8_t *pMsg,
									 u32_t nMsgLen, u8_t *pAESKey,
                                     u32_t AESKeyLen,
                                     u8_t *pCipherIV, u32_t crypto_len,
                                     u8_t *outputCipherText,
                                     BCM_SCAPI_CIPHER_MODE cipher_mode,
                                     BCM_SCAPI_ENCR_MODE   encr_mode)
{
	BCM_SCAPI_STATUS status = BCM_SCAPI_STATUS_CRYPTO_ERROR;
	bcm_bulk_cipher_context ctx;
	BCM_BULK_CIPHER_CMD     cmd;

	if (pAESKey == NULL) {
		return BCM_SCAPI_STATUS_INVALID_KEY;
	}
	if (cipher_mode != BCM_SCAPI_CIPHER_MODE_ENCRYPT &&
			cipher_mode != BCM_SCAPI_CIPHER_MODE_DECRYPT) {
		return BCM_SCAPI_STATUS_PARAMETER_INVALID;
	}

	cmd.cipher_mode  = cipher_mode;

	if (AESKeyLen == AES_128_KEY_LEN_BYTES) {
		cmd.encr_alg = BCM_SCAPI_ENCR_ALG_AES_128; 	/* encryption algorithm */
	} else if (AESKeyLen == AES_256_KEY_LEN_BYTES) {
		cmd.encr_alg = BCM_SCAPI_ENCR_ALG_AES_256; 	/* encryption algorithm */
	} else {
		return BCM_SCAPI_STATUS_INVALID_KEY;
	}

	if (encr_mode != BCM_SCAPI_ENCR_MODE_CBC &&
	    encr_mode != BCM_SCAPI_ENCR_MODE_ECB) {
		return BCM_SCAPI_STATUS_PARAMETER_INVALID;
	}

	cmd.encr_mode    = encr_mode;
	cmd.cipher_order = BCM_SCAPI_CIPHER_ORDER_NULL;
	// The following initializations are not relevant since auth key is NULL
	cmd.auth_alg     = BCM_SCAPI_AUTH_ALG_NULL;
	cmd.auth_mode    = BCM_SCAPI_AUTH_MODE_HASH;
	cmd.auth_optype  = BCM_SCAPI_AUTH_OPTYPE_INIT;

	status = crypto_smau_cipher_init ( pHandle,
									&cmd,
									pAESKey,
									NULL,
									0,
									&ctx);
	if ( status != BCM_SCAPI_STATUS_OK) {
		return status;
	}

	status = crypto_smau_cipher_start(pHandle, pMsg, nMsgLen,
									  pCipherIV, crypto_len,
									  0, 0, 0, outputCipherText, &ctx);

	crypto_smau_cipher_deinit (pHandle, &ctx);

	return status;
}

/*
 * Name    : crypto_symmetric_des_encrypt
 * Purpose : Perform a generic DES operation
 *
 * @param[in] handle Crypto context handle
 * @param[in] encr_alg Ecryption alg (des or 3des)
 * @param[in] key Buffer containing key
 * @param[in] key_len Key length
 * @param[in] plain Plain text buffer pointer
 * @param[in] plain_len Plain text length
 * @param[in] iv Buffer containing IV
 * @param[in] iv_len IV length
 * @param[out] Output The calculated des enc
 *
 * Return  : Appropriate status
 * Remark  : Will use DMA Controller
 *
 */
BCM_SCAPI_STATUS crypto_symmetric_des_encrypt(crypto_lib_handle *pHandle,
					      BCM_SCAPI_ENCR_ALG encr_alg,
					      u8_t *key, u8_t key_len,
					      u8_t *plain, u8_t plain_length,
					      u8_t *iv, u8_t iv_length,
					      u8_t *output)
{
	BCM_SCAPI_STATUS rv = 0;

	if (encr_alg == BCM_SCAPI_ENCR_ALG_DES)
		rv = crypto_des_encrypt(pHandle, key, key_len, plain,
					plain_length, iv, iv_length, output);
	else if (encr_alg == BCM_SCAPI_ENCR_ALG_3DES)
		rv = crypto_3des_encrypt(pHandle, key, key_len, plain,
					 plain_length, iv, iv_length, output);
	else
		rv = BCM_SCAPI_STATUS_PARAMETER_INVALID;

	return rv;
}

/*
 * Name    : crypto_symmetric_des_decrypt
 * Purpose : Perform a generic DES operation
 * Input   : [in]  pHandle : crypto context handle
 *           [in]  encr_alg : ecryption alg (AES 128, 192, 256)
 *           [in]  key : buffer containing key
 *           [in]  key_len : key length
 *           [in]  cipher : buffer containing cipher
 *           [in]  cipher_length : Cipher length
 *           [in]  iv : buffer containing IV
 *           [in]  iv_len : IV length
 *           [out] output : output buffer
 * Output  : the calculated des dec
 * Return  : Appropriate status
 * Remark  : Will use DMA Controller
 *
 */
BCM_SCAPI_STATUS crypto_symmetric_des_decrypt(crypto_lib_handle *pHandle,
					      BCM_SCAPI_ENCR_ALG encr_alg,
					      u8_t *key, u8_t key_len,
					      u8_t *cipher, u8_t cipher_length,
					      u8_t *iv, u8_t iv_length,
					      u8_t *output)
{
	BCM_SCAPI_STATUS rv = 0;

	if (encr_alg == BCM_SCAPI_ENCR_ALG_DES)
		rv = crypto_des_decrypt(pHandle, key, key_len, cipher,
					 cipher_length, iv, iv_length, output);
	else if (encr_alg == BCM_SCAPI_ENCR_ALG_3DES)
		rv = crypto_3des_decrypt(pHandle, key, key_len, cipher,
					 cipher_length, iv, iv_length, output);
	else
		rv = BCM_SCAPI_STATUS_PARAMETER_INVALID;

	return rv;
}
/* ***************************************************************************
 * Private Functions 
 * ****************************************************************************/
static BCM_SCAPI_STATUS crypto_symmetric_aes(crypto_lib_handle *pHandle,
										 u8_t *pMsg, u32_t nMsgLen,
										 u8_t *pAESKey, u32_t AESKeyLen,
										 u8_t *pCipherIV,
										 u32_t crypto_len,u32_t crypto_offset,
										 u8_t *output,
										 BCM_SCAPI_CIPHER_MODE cipher_mode)
{
	BCM_SCAPI_STATUS status;
	bcm_bulk_cipher_context ctx;
	BCM_BULK_CIPHER_CMD     cmd;

	cmd.cipher_mode  = cipher_mode;                  /* encrypt or decrypt */
	if(AESKeyLen == AES_128_KEY_LEN_BYTES)
		cmd.encr_alg = BCM_SCAPI_ENCR_ALG_AES_128;   /* encryption algorithm */
	else if (AESKeyLen == AES_256_KEY_LEN_BYTES)
		cmd.encr_alg = BCM_SCAPI_ENCR_ALG_AES_256;   /* encryption algorithm */
	else
		return BCM_SCAPI_STATUS_INVALID_AES_KEYSIZE;

	cmd.encr_mode    = BCM_SCAPI_ENCR_MODE_CBC;
	cmd.cipher_order = BCM_SCAPI_CIPHER_ORDER_NULL;
	/* authentication algorithm */
	cmd.auth_alg     = BCM_SCAPI_AUTH_ALG_NULL;
	cmd.auth_mode    = BCM_SCAPI_AUTH_MODE_HASH;
	cmd.auth_optype  = BCM_SCAPI_AUTH_OPTYPE_FULL;

	status = crypto_smau_cipher_init ( pHandle,
									&cmd,
									pAESKey,
									NULL,
									0,
									&ctx);
	if ( status != BCM_SCAPI_STATUS_OK) {
#ifdef CRYPTO_DEBUG
		printk("Cipher Init Failed\n");
#endif
		return status;
	}

	status = crypto_smau_cipher_start(pHandle,pMsg,nMsgLen,
								   pCipherIV,
								   crypto_len, crypto_offset,
								   0, 0,output,&ctx);

#ifdef CRYPTO_DEBUG
	{
		u32_t i;
		printk("*** bcm_aes_crypto Output ***\n");
		for(i=0; i<16/*crypto_len*/; i+=4)
		printk("0x%x 0x%x 0x%x 0x%x \n", *(output + i),
										   *(output + i + 1), *(output + i + 2),
										   *(output + i + 3));
	}
#endif

	crypto_smau_cipher_deinit(pHandle, &ctx);

	return status;
}

static void inc32(u8_t *block)
{
	u32_t val;

	val = WPA_GET_BE32(block + AES_BLOCK_SIZE - 4);
	val++;
	WPA_PUT_BE32(block + AES_BLOCK_SIZE - 4, val);
}

static void shift_right_block(u8_t *v)
{
	u32_t val;

	val = WPA_GET_BE32(v + 12);
	val >>= 1;
	if (v[11] & 0x01)
		val |= 0x80000000;
	WPA_PUT_BE32(v + 12, val);

	val = WPA_GET_BE32(v + 8);
	val >>= 1;
	if (v[7] & 0x01)
		val |= 0x80000000;
	WPA_PUT_BE32(v + 8, val);

	val = WPA_GET_BE32(v + 4);
	val >>= 1;
	if (v[3] & 0x01)
		val |= 0x80000000;
	WPA_PUT_BE32(v + 4, val);

	val = WPA_GET_BE32(v);
	val >>= 1;
	WPA_PUT_BE32(v, val);
}

static void ghash_start(u8_t *y)
{
	/* Y_0 = 0^128 */
	memset(y, 0, 16);
}

static void xor_block(u8_t *dst, const u8_t *src)
{
	u32_t *d = (u32_t *) dst;
	u32_t *s = (u32_t *) src;

	*d++ ^= *s++;
	*d++ ^= *s++;
	*d++ ^= *s++;
	*d++ ^= *s++;
}

/* Multiplication in GF(2^128) */
static void gf_mult(const u8_t *x, const u8_t *y, u8_t *z)
{
	u8_t v[16];
	int i, j;

	memset(z, 0, 16); /* Z_0 = 0^128 */
	memcpy(v, y, 16); /* V_0 = Y */

	for (i = 0; i < 16; i++) {
		for (j = 0; j < 8; j++) {
			if (x[i] & BIT(7 - j)) {
				/* Z_(i + 1) = Z_i XOR V_i */
				xor_block(z, v);
			} else {
				/* Z_(i + 1) = Z_i */
			}

			if (v[15] & 0x01) {
				/* V_(i + 1) = (V_i >> 1) XOR R */
				shift_right_block(v);
				/* R = 11100001 || 0^120 */
				v[0] ^= 0xe1;
			} else {
				/* V_(i + 1) = V_i >> 1 */
				shift_right_block(v);
			}
		}
	}
}

static void ghash(const u8_t *h, const u8_t *x, size_t xlen, u8_t *y)
{
	size_t m, i;
	const u8_t *xpos = x;
	u8_t tmp[16];

	m = xlen / 16;

	for (i = 0; i < m; i++) {
		/* Y_i = (Y^(i-1) XOR X_i) dot H */
		xor_block(y, xpos);
		xpos += 16;

		/*
		 * dot operation:
		 * multiplication operation for binary Galois (finite) field of
		 * 2^128 elements
		 */
		gf_mult(y, h, tmp);
		memcpy(y, tmp, 16);
	}

	if (x + xlen > xpos) {
		/* Add zero padded last block */
		size_t last = x + xlen - xpos;

		memcpy(tmp, xpos, last);
		memset(tmp + last, 0, sizeof(tmp) - last);

		/* Y_i = (Y^(i-1) XOR X_i) dot H */
		xor_block(y, tmp);

		/* dot operation:
		 * multiplication operation for binary Galois (finite) field of
		 * 2^128 elements
		 */
		gf_mult(y, h, tmp);
		memcpy(y, tmp, 16);
	}

	/* Return Y_m */
}

static BCM_SCAPI_STATUS crypto_aes_ecb_encrypt(crypto_lib_handle *pHandle,
					       const u8_t *key,
					       u32_t key_length, u8_t *plain,
					       u32_t plain_length, u8_t *cipher)
{
	s32_t rv = 0;
	bcm_bulk_cipher_context ctx;
	BCM_BULK_CIPHER_CMD cmd;

	if ((key == NULL) || (plain == NULL) || (cipher == NULL) ||
	    (plain_length > CRYPTO_DATA_SIZE))
		return BCM_SCAPI_STATUS_PARAMETER_INVALID;

	cmd.cipher_mode  = BCM_SCAPI_CIPHER_MODE_ENCRYPT; /* encrypt/ decrypt */

	switch (key_length) {
	case AES_128_KEY_LEN_BYTES:
		cmd.encr_alg = BCM_SCAPI_ENCR_ALG_AES_128;
		break;
	case AES_192_KEY_LEN_BYTES:
		cmd.encr_alg = BCM_SCAPI_ENCR_ALG_AES_192;
		break;
	case AES_256_KEY_LEN_BYTES:
		cmd.encr_alg = BCM_SCAPI_ENCR_ALG_AES_256;
		break;
	default:
		return BCM_SCAPI_STATUS_PARAMETER_INVALID;
		 /* invalid key length */
		break;
	}

	cmd.encr_mode = BCM_SCAPI_ENCR_MODE_ECB;
	cmd.auth_alg = BCM_SCAPI_AUTH_ALG_NULL;
	cmd.auth_mode = BCM_SCAPI_AUTH_MODE_HASH;
	cmd.auth_optype = BCM_SCAPI_AUTH_OPTYPE_INIT;
	cmd.cipher_order = BCM_SCAPI_CIPHER_ORDER_NULL;

	rv = crypto_smau_cipher_init(pHandle, &cmd, (u8_t *)key, NULL, 0, &ctx);
	if (rv != BCM_SCAPI_STATUS_OK) {
		SYS_LOG_DBG("cipher init failed %d\n", rv);
		return rv;
	}

	rv = crypto_smau_cipher_start(pHandle, plain, plain_length, NULL,
					 plain_length, 0, 0, 0, cipher, &ctx);
	if (rv != BCM_SCAPI_STATUS_OK) {
		SYS_LOG_DBG("cipher start failed %d\n", rv);
		return rv;
	}

	return rv;
}

static BCM_SCAPI_STATUS aes_gctr(crypto_lib_handle *pHandle, const u8_t *key,
				 size_t key_len, const u8_t *icb, const u8_t *x,
				 size_t xlen, u8_t *y)
{
	size_t i, n, last;
	u8_t cb[AES_BLOCK_SIZE], tmp[AES_BLOCK_SIZE];
	const u8_t *xpos = x;
	BCM_SCAPI_STATUS rv;
	u8_t *ypos = y;

	n = xlen / 16;

	memcpy(cb, icb, AES_BLOCK_SIZE);

	/* Full blocks */
	for (i = 0; i < n; i++) {
		rv = crypto_aes_ecb_encrypt(pHandle, key, key_len/8, cb,
					AES_BLOCK_SIZE, ypos);
		if (rv)
			return rv;
		xor_block(ypos, xpos);

		xpos += AES_BLOCK_SIZE;
		ypos += AES_BLOCK_SIZE;
		inc32(cb);
	}

	last = x + xlen - xpos;
	if (last) {
		/* Last, partial block */

		rv = crypto_aes_ecb_encrypt(pHandle, key, key_len/8, cb,
							AES_BLOCK_SIZE, tmp);
		if (rv)
			return rv;

		for (i = 0; i < last; i++)
			*ypos++ = *xpos++ ^ tmp[i];
	}

	return rv;
}

static BCM_SCAPI_STATUS aes_gcm_gctr(crypto_lib_handle *pHandle,
				     const u8_t *key, size_t key_len,
				     const u8_t *J0, const u8_t *in,
				     size_t len, u8_t *out)
{
	u8_t J0inc[AES_BLOCK_SIZE];
	BCM_SCAPI_STATUS rv;

	memcpy(J0inc, J0, AES_BLOCK_SIZE);
	inc32(J0inc);
	rv = aes_gctr(pHandle, key, key_len, J0inc, in, len, out);

	return rv;
}

static void aes_gcm_prepare_j0(const u8_t *iv, size_t iv_len, const u8_t *H,
			       u8_t *J0)
{
	u8_t len_buf[16];

	if (iv_len == 12) {
		/* Prepare block J_0 = IV || 0^31 || 1 [len(IV) = 96] */
		memcpy(J0, iv, iv_len);
		memset(J0 + iv_len, 0, AES_BLOCK_SIZE - iv_len);
		J0[AES_BLOCK_SIZE - 1] = 0x01;
	} else {
		/*
		 * s = 128 * ceil(len(IV)/128) - len(IV)
		 * J_0 = GHASH_H(IV || 0^(s+64) || [len(IV)]_64)
		 */
		ghash_start(J0);
		ghash(H, iv, iv_len, J0);
		WPA_PUT_BE64(len_buf, 0);
		WPA_PUT_BE64(len_buf + 8, iv_len * 8);
		ghash(H, len_buf, sizeof(len_buf), J0);
	}
}

static void aes_gcm_ghash(const u8_t *H, const u8_t *aad, size_t aad_len,
			  const u8_t *crypt, size_t crypt_len, u8_t *S)
{
	u8_t len_buf[16];

	/*
	 * u = 128 * ceil[len(C)/128] - len(C)
	 * v = 128 * ceil[len(A)/128] - len(A)
	 * S = GHASH_H(A || 0^v || C || 0^u || [len(A)]64 || [len(C)]64)
	 * (i.e., zero padded to block size A || C and lengths of each in bits)
	 */
	ghash_start(S);
	ghash(H, aad, aad_len, S);
	ghash(H, crypt, crypt_len, S);
	WPA_PUT_BE64(len_buf, aad_len * 8);
	WPA_PUT_BE64(len_buf + 8, crypt_len * 8);
	ghash(H, len_buf, sizeof(len_buf), S);
}

BCM_SCAPI_STATUS aes_dpa_enable(void)
{
	BCM_SCAPI_STATUS rv = 0;
	u32_t random[DPA_AES_SEED_REGISTERS];
	u32_t cnt;

	for (cnt = 0; cnt < DPA_AES_SEED_REGISTERS; cnt++) {
		rv = bcm_rng_readwords(&random[cnt], 1);
		if (rv != 0)
			return rv;
	}

	/* Enable AES DPA and clear DES DPA */
	sys_set_bit(DPA_REGS_AES_CFG, AES_DATA_DPA_ENABLE_BIT);
	sys_set_bit(DPA_REGS_AES_CFG, AES_KEY_DPA_ENABLE_BIT);
	sys_clear_bit(DPA_REGS_DES_CFG, DES_DPA_ENABLE_BIT);

	for (cnt = 0; cnt < 9; cnt++) {
		sys_write32(random[cnt], DPA_REGS_KEY_1_SEED + (cnt * 4));
		if (cnt < 8) {
			sys_write32(random[9 + cnt],
					DPA_REGS_DATA_1_SEED + (cnt * 4));
			sys_write32(0, DPA_REGS_CRKEY_MASK_1 + (cnt * 4));
		}
	}

	/* Seed load */
	sys_clear_bit(DPA_REGS_AES_CFG, IFSR_SEED_LOAD_DATA_BIT);
	sys_clear_bit(DPA_REGS_AES_CFG, IFSR_SEED_LOAD_KEYS_BIT);
	sys_set_bit(DPA_REGS_AES_CFG, IFSR_SEED_LOAD_DATA_BIT);
	sys_set_bit(DPA_REGS_AES_CFG, IFSR_SEED_LOAD_KEYS_BIT);
	sys_clear_bit(DPA_REGS_AES_CFG, IFSR_SEED_LOAD_DATA_BIT);
	sys_clear_bit(DPA_REGS_AES_CFG, IFSR_SEED_LOAD_KEYS_BIT);

	return rv;
}

s32_t dpa_set_policy(enum DPA_POLICY policy)
{
	s32_t result = 0;

	if ((policy == DPA_NONE) || (policy == DPA_HIGH))
		des_dpa_policy = policy;
	else
		result = BCM_SCAPI_STATUS_PARAMETER_INVALID;

	return result;
}

s32_t dpa_get_policy(enum DPA_POLICY *policy)
{
	if (policy != NULL)
		*policy = des_dpa_policy;
	else
		return BCM_SCAPI_STATUS_PARAMETER_INVALID;

	return 0;
}

s32_t dpa_set_ratio(u32_t ratio)
{
	s32_t result = 0;

	if (ratio >= 5)
		des_dpa_ratio = ratio;
	else
		result = BCM_SCAPI_STATUS_PARAMETER_INVALID;

	return result;
}

s32_t dpa_get_ratio(u32_t *ratio)
{
	s32_t result = 0;

	if (ratio != NULL)
		*ratio = des_dpa_ratio;
	else
		result = BCM_SCAPI_STATUS_PARAMETER_INVALID;

	return result;
}

static s32_t dpa_enable(u32_t *key_seed, u32_t *data_seed, u32_t *key_mask,
			s32_t limit)
{
	s32_t rc = 0;
	s32_t i = 0;

	sys_clear_bit(DPA_REGS_AES_CFG, AES_DATA_DPA_ENABLE_BIT);
	sys_clear_bit(DPA_REGS_AES_CFG, AES_KEY_DPA_ENABLE_BIT);
	/* Step 1: Set bit 4 (des_dpa_enable) in DPA_REGS_DES_CFG to 1 */
	sys_set_bit(DPA_REGS_DES_CFG, DES_DPA_ENABLE_BIT);

	/*
	 * Step 2: Generate random from RNG and write them into the seed
	 * registers DPA_REGS_KEY_#_SEED and DPA_REGS_DATA_#_SEED.
	 */
	for (i = 0; i < limit; i++) {
		sys_write32(key_seed[i], DPA_REGS_KEY_1_SEED + (i * 4));
		sys_write32(data_seed[i], DPA_REGS_DATA_1_SEED + (i * 4));
	}

	/*
	 * Step 3: Bit 7 (lfsr_seed_load) in DPA_REGS_DES_CFG must be toggled
	 * from 0 to 1 to make the seeds take effect
	 */
	sys_clear_bit(DPA_REGS_DES_CFG, IFSR_SEED_LOAD);
	sys_set_bit(DPA_REGS_DES_CFG, IFSR_SEED_LOAD);

	/* Step 4: Side-load the key instead of directly loading the key. */
	for (i = 0; i < limit; i++)
		sys_write32(key_mask[i], DPA_REGS_CRKEY_MASK_1 + (i * 4));

	return rc;
}

static s32_t dpa_disable(void)
{
	s32_t rc = 0;
	s32_t i = 0, limit = 6;

	sys_clear_bit(DPA_REGS_DES_CFG, DES_DPA_ENABLE_BIT);
	sys_clear_bit(DPA_REGS_DES_CFG, IFSR_SEED_LOAD);

	for (i = 0; i < limit; i++)
		sys_write32(0, DPA_REGS_CRKEY_MASK_1 + (i * 4));

	return rc;
}

BCM_SCAPI_STATUS dpa_init(u8_t *key, u32_t key_length, u32_t encrypt)
{
	BCM_SCAPI_STATUS rc = 0;
	u32_t i = 0, size = key_length / 4;
	u32_t key_mask[6];
	u32_t key_seed[6];
	u32_t data_seed[6];
	u32_t local_mask[6] = { 0 };
	u32_t *key_ptr = (u32_t *) key;

	rc = bcm_rng_readwords(key_seed, size);
	CRYPTO_RESULT(rc, "generate key seed failed");
	rc = bcm_rng_readwords(data_seed, size);
	CRYPTO_RESULT(rc, "generate data seed failed");
	rc = bcm_rng_readwords(key_mask, size);
	CRYPTO_RESULT(rc, "generate key mask failed");
	rc = dpa_enable(key_seed, data_seed, key_mask, size);
	CRYPTO_RESULT(rc, "dpa_enable failed");
	/*
	 * For 3des encryption,
	 * ciphertext = EK3(DK2(EK1(plaintext)))
	 * For 3des decryption,
	 * plaintext = DK1(EK2(DK3(ciphertext)))
	 * Encryption and decryption are using key in reverse order,
	 * So we should apply mask in reverse order
	 */
	if ((key_length == CRYPTO_3DES_KEY_SIZE) && (encrypt == 0)) {
		local_mask[0] = key_mask[4];
		local_mask[1] = key_mask[5];
		local_mask[2] = key_mask[2];
		local_mask[3] = key_mask[3];
		local_mask[4] = key_mask[0];
		local_mask[5] = key_mask[1];
	} else {
		memcpy(local_mask, key_mask, key_length);
	}

	for (i = 0; i < (key_length / 4); i++) {
		crypto_swap_end(&local_mask[i]);
		key_ptr[i] = key_ptr[i] ^ local_mask[i];
	}

	return rc;
}

#ifdef CONFIG_CRYPTO_DPA
/*
 * Generate random index instead of (rand % size)
 */
static s32_t dpa_random_index(u32_t size, u32_t mask)
{
	s32_t rc = 0;
	u32_t random = 0;

	while (1) {
		rc = bcm_rng_readwords(&random, 1);
		if (rc != 0)
			return -1;

		random = random & mask;
		if (random < size)
			return random;
	}

	return -1;
}

/*
 * Generate random sequence
 * Suppose output number is 5, ratio is 10
 * There should be 1 real block in every 10 blocks
 * For example, 2, 16, 20, 33, 48
 */
static s32_t dpa_random_sequence(u32_t output_number, u32_t dpa_ratio,
				 u32_t *output_sequence)
{
	s32_t rc = 0;
	s32_t index = 0;
	u32_t i = 0, mask = 0;

	/*
	 * Generate mask
	 * For example, size is 0x3c, then mask will be 0x3f
	 */
	mask = dpa_ratio - 1;
	mask = mask | (mask >> 1);
	mask = mask | (mask >> 2);
	mask = mask | (mask >> 4);
	mask = mask | (mask >> 8);
	mask = mask | (mask >> 16);

	for (i = 0; i < output_number; i++) {
		/* The last block is always dummy */
		index = dpa_random_index(dpa_ratio - 1, mask);
		if (index < 0) {
			rc = BCM_SCAPI_STATUS_CRYPTO_RNG_NO_ENOUGH_BITS;
			goto RANDOM_ERROR;
		}
		output_sequence[i] = index + (i * dpa_ratio);
	}
RANDOM_ERROR:
	return rc;
}

static s32_t dpa_xor_array(u8_t *dst, u8_t *src1, u32_t *src2_ptr, u32_t length)
{
	s32_t rc = 0;
	u32_t i = 0, j = 0;
	u32_t temp = 0;
	u8_t *mask = NULL;
	u8_t *ptr = NULL;

	mask = k_malloc(length);
	if (mask == NULL)
		goto XOR_ERROR;

	/* Generate random for mask */
	for (i = 0; i < length; i++) {
		do {
			rc = bcm_rng_readwords(&temp, 1);
			if (rc != 0)
				goto XOR_ERROR;

			temp = (u8_t) temp;
		} while (temp == 0);

		mask[i] = (u8_t) temp;
	}
	/* Use gf8_get_a_xor_x  to replace xor */
	for (i = 0; i < length; i += CRYPTO_3DES_BLK_SIZE) {
		ptr = (u8_t *) src2_ptr[i / CRYPTO_3DES_BLK_SIZE];
		for (j = 0; j < CRYPTO_3DES_BLK_SIZE; j++) {
			dst[i + j] = gf8_get_a_xor_x(src1[i + j], ptr[j],
					mask[i + j], &_masked_G_invf, MASK_G);
		}
	}

XOR_ERROR:
	if (mask != NULL)
		k_free(mask);

	return rc;
}
#endif

static BCM_SCAPI_STATUS _crypto_des_cbc_encrypt(crypto_lib_handle *pHandle,
						u8_t *key, u32_t key_length,
						u8_t *plain, u32_t plain_length,
						u8_t *iv, u32_t iv_length,
						u8_t *cipher)
{
	BCM_SCAPI_STATUS rc = 0;
	bcm_bulk_cipher_context ctx;
	BCM_BULK_CIPHER_CMD cmd;

	ARG_UNUSED(key_length);
	ARG_UNUSED(iv_length);
	cmd.cipher_mode = BCM_SCAPI_CIPHER_MODE_ENCRYPT;
	cmd.encr_alg = BCM_SCAPI_ENCR_ALG_DES;
	cmd.encr_mode = BCM_SCAPI_ENCR_MODE_CBC;
	cmd.auth_alg = BCM_SCAPI_AUTH_ALG_NULL;
	cmd.auth_mode = BCM_SCAPI_AUTH_MODE_HASH;
	cmd.auth_optype = BCM_SCAPI_AUTH_OPTYPE_INIT;
	cmd.cipher_order = BCM_SCAPI_CIPHER_ORDER_NULL;

	rc = crypto_smau_cipher_init(pHandle, &cmd, key, NULL, 0, &ctx);
	if (rc != BCM_SCAPI_STATUS_OK)
		return rc;

	rc = crypto_smau_cipher_start(pHandle, plain, plain_length, iv,
				   plain_length, 0, 0, 0, cipher, &ctx);

	return rc;
}

static BCM_SCAPI_STATUS _crypto_des_cbc_decrypt(crypto_lib_handle *pHandle,
						u8_t *key, u32_t key_length,
						u8_t *cipher,
						u32_t cipher_length,
						u8_t *iv, u32_t iv_length,
						u8_t *plain)
{
	BCM_SCAPI_STATUS rc = 0;
	bcm_bulk_cipher_context ctx;
	BCM_BULK_CIPHER_CMD cmd;

	ARG_UNUSED(key_length);
	ARG_UNUSED(iv_length);
	cmd.cipher_mode = BCM_SCAPI_CIPHER_MODE_DECRYPT;
	cmd.encr_alg = BCM_SCAPI_ENCR_ALG_DES;
	cmd.encr_mode = BCM_SCAPI_ENCR_MODE_CBC;
	cmd.auth_alg = BCM_SCAPI_AUTH_ALG_NULL;
	cmd.auth_mode = BCM_SCAPI_AUTH_MODE_HASH;
	cmd.auth_optype = BCM_SCAPI_AUTH_OPTYPE_INIT;
	cmd.cipher_order = BCM_SCAPI_CIPHER_ORDER_NULL;

	rc = crypto_smau_cipher_init(pHandle, &cmd, key, NULL, 0, &ctx);
	if (rc != BCM_SCAPI_STATUS_OK)
		return rc;

	rc = crypto_smau_cipher_start(pHandle, cipher, cipher_length, iv,
				   cipher_length, 0, 0, 0, plain, &ctx);

	return rc;
}

static BCM_SCAPI_STATUS _crypto_3des_cbc_encrypt(crypto_lib_handle *pHandle,
						 u8_t *key, u32_t key_length,
						 u8_t *plain,
						 u32_t plain_length,
						 u8_t *iv, u32_t iv_length,
						 u8_t *cipher)
{
	BCM_SCAPI_STATUS rc = 0;
	bcm_bulk_cipher_context ctx;
	BCM_BULK_CIPHER_CMD cmd;

	ARG_UNUSED(key_length);
	ARG_UNUSED(iv_length);
	cmd.cipher_mode = BCM_SCAPI_CIPHER_MODE_ENCRYPT;
	cmd.encr_alg = BCM_SCAPI_ENCR_ALG_3DES;
	cmd.encr_mode = BCM_SCAPI_ENCR_MODE_CBC;
	cmd.auth_alg = BCM_SCAPI_AUTH_ALG_NULL;
	cmd.auth_mode = BCM_SCAPI_AUTH_MODE_HASH;
	cmd.auth_optype = BCM_SCAPI_AUTH_OPTYPE_INIT;
	cmd.cipher_order = BCM_SCAPI_CIPHER_ORDER_NULL;

	rc = crypto_smau_cipher_init(pHandle, &cmd, (u8_t *) key, NULL, 0,
				  &ctx);
	if (rc != BCM_SCAPI_STATUS_OK)
		return rc;

	rc = crypto_smau_cipher_start(pHandle, plain, plain_length, iv,
				   plain_length, 0, 0, 0, cipher, &ctx);

	return rc;
}

static BCM_SCAPI_STATUS _crypto_3des_cbc_decrypt(crypto_lib_handle *pHandle,
						 u8_t *key, u32_t key_length,
						 u8_t *cipher,
						 u32_t cipher_length,
						 u8_t *iv, u32_t iv_length,
						 u8_t *plain)
{
	BCM_SCAPI_STATUS rc = 0;
	bcm_bulk_cipher_context ctx;
	BCM_BULK_CIPHER_CMD cmd;

	ARG_UNUSED(key_length);
	ARG_UNUSED(iv_length);
	cmd.cipher_mode = BCM_SCAPI_CIPHER_MODE_DECRYPT;
	cmd.encr_alg = BCM_SCAPI_ENCR_ALG_3DES;
	cmd.encr_mode = BCM_SCAPI_ENCR_MODE_CBC;
	cmd.auth_alg = BCM_SCAPI_AUTH_ALG_NULL;
	cmd.auth_mode = BCM_SCAPI_AUTH_MODE_HASH;
	cmd.auth_optype = BCM_SCAPI_AUTH_OPTYPE_INIT;
	cmd.cipher_order = BCM_SCAPI_CIPHER_ORDER_NULL;

	rc = crypto_smau_cipher_init(pHandle, &cmd, (u8_t *) key, NULL, 0,
				  &ctx);
	if (rc != BCM_SCAPI_STATUS_OK)
		return rc;

	rc = crypto_smau_cipher_start(pHandle, cipher, cipher_length, iv,
				   cipher_length, 0, 0, 0, plain, &ctx);

	return rc;
}

static BCM_SCAPI_STATUS _crypto_des_ecb_encrypt(crypto_lib_handle *pHandle,
						u8_t *key, u32_t key_length,
						u8_t *plain, u32_t plain_length,
						u8_t *cipher)
{
	BCM_SCAPI_STATUS rc = 0;
	bcm_bulk_cipher_context ctx;
	BCM_BULK_CIPHER_CMD cmd;

	ARG_UNUSED(key_length);
	cmd.cipher_mode = BCM_SCAPI_CIPHER_MODE_ENCRYPT;
	cmd.encr_alg = BCM_SCAPI_ENCR_ALG_DES;
	cmd.encr_mode = BCM_SCAPI_ENCR_MODE_ECB;
	cmd.auth_alg = BCM_SCAPI_AUTH_ALG_NULL;
	cmd.auth_mode = BCM_SCAPI_AUTH_MODE_HASH;
	cmd.auth_optype = BCM_SCAPI_AUTH_OPTYPE_INIT;
	cmd.cipher_order = BCM_SCAPI_CIPHER_ORDER_NULL;

	rc = crypto_smau_cipher_init(pHandle, &cmd, key, NULL, 0, &ctx);
	if (rc != BCM_SCAPI_STATUS_OK)
		return rc;

	rc = crypto_smau_cipher_start(pHandle, plain, plain_length, NULL,
				   plain_length, 0, 0, 0, cipher, &ctx);

	return rc;
}

static BCM_SCAPI_STATUS _crypto_des_ecb_decrypt(crypto_lib_handle *pHandle,
						u8_t *key, u32_t key_length,
						u8_t *cipher,
						u32_t cipher_length,
						u8_t *plain)
{
	BCM_SCAPI_STATUS rc = 0;
	bcm_bulk_cipher_context ctx;
	BCM_BULK_CIPHER_CMD cmd;

	ARG_UNUSED(key_length);
	cmd.cipher_mode = BCM_SCAPI_CIPHER_MODE_DECRYPT;
	cmd.encr_alg = BCM_SCAPI_ENCR_ALG_DES;
	cmd.encr_mode = BCM_SCAPI_ENCR_MODE_ECB;
	cmd.auth_alg = BCM_SCAPI_AUTH_ALG_NULL;
	cmd.auth_mode = BCM_SCAPI_AUTH_MODE_HASH;
	cmd.auth_optype = BCM_SCAPI_AUTH_OPTYPE_INIT;
	cmd.cipher_order = BCM_SCAPI_CIPHER_ORDER_NULL;

	rc = crypto_smau_cipher_init(pHandle, &cmd, key, NULL, 0, &ctx);
	if (rc != BCM_SCAPI_STATUS_OK)
		return rc;

	rc = crypto_smau_cipher_start(pHandle, cipher, cipher_length, NULL,
				   cipher_length, 0, 0, 0, plain, &ctx);

	return rc;
}

static BCM_SCAPI_STATUS _crypto_3des_ecb_encrypt(crypto_lib_handle *pHandle,
						 u8_t *key, u32_t key_length,
						 u8_t *plain,
						 u32_t plain_length,
						 u8_t *cipher)
{
	BCM_SCAPI_STATUS rc = 0;
	bcm_bulk_cipher_context ctx;
	BCM_BULK_CIPHER_CMD cmd;

	ARG_UNUSED(key_length);
	cmd.cipher_mode = BCM_SCAPI_CIPHER_MODE_ENCRYPT;
	cmd.encr_alg = BCM_SCAPI_ENCR_ALG_3DES;
	cmd.encr_mode = BCM_SCAPI_ENCR_MODE_ECB;
	cmd.auth_alg = BCM_SCAPI_AUTH_ALG_NULL;
	cmd.auth_mode = BCM_SCAPI_AUTH_MODE_HASH;
	cmd.auth_optype = BCM_SCAPI_AUTH_OPTYPE_INIT;
	cmd.cipher_order = BCM_SCAPI_CIPHER_ORDER_NULL;

	rc = crypto_smau_cipher_init(pHandle, &cmd, (u8_t *) key, NULL, 0,
				  &ctx);
	if (rc != BCM_SCAPI_STATUS_OK)
		return rc;

	rc = crypto_smau_cipher_start(pHandle, plain, plain_length, NULL,
				   plain_length, 0, 0, 0, cipher, &ctx);

	return rc;
}

static BCM_SCAPI_STATUS _crypto_3des_ecb_decrypt(crypto_lib_handle *pHandle,
						 u8_t *key, u32_t key_length,
						 u8_t *cipher,
						 u32_t cipher_length,
						 u8_t *plain)
{
	BCM_SCAPI_STATUS rc = 0;
	bcm_bulk_cipher_context ctx;
	BCM_BULK_CIPHER_CMD cmd;

	ARG_UNUSED(key_length);
	cmd.cipher_mode = BCM_SCAPI_CIPHER_MODE_DECRYPT;
	cmd.encr_alg = BCM_SCAPI_ENCR_ALG_3DES;
	cmd.encr_mode = BCM_SCAPI_ENCR_MODE_ECB;
	cmd.auth_alg = BCM_SCAPI_AUTH_ALG_NULL;
	cmd.auth_mode = BCM_SCAPI_AUTH_MODE_HASH;
	cmd.auth_optype = BCM_SCAPI_AUTH_OPTYPE_INIT;
	cmd.cipher_order = BCM_SCAPI_CIPHER_ORDER_NULL;

	rc = crypto_smau_cipher_init(pHandle, &cmd, (u8_t *) key, NULL, 0,
							&ctx);
	if (rc != BCM_SCAPI_STATUS_OK)
		return rc;

	rc = crypto_smau_cipher_start(pHandle, cipher, cipher_length, NULL,
					cipher_length, 0, 0, 0, plain, &ctx);

	return rc;
}

static BCM_SCAPI_STATUS crypto_des_encrypt(crypto_lib_handle *pHandle,
					   u8_t *key, u32_t key_length,
					   u8_t *plain, u32_t plain_length,
					   u8_t *iv, u32_t iv_length,
					   u8_t *cipher)
{
	BCM_SCAPI_STATUS rc = 0;
#ifdef CONFIG_CRYPTO_DPA
	u32_t i = 0, j = 0, k = 0;
	u8_t *local_iv = NULL;
	u8_t *local_plain = NULL;
	u8_t local_key[CRYPTO_DES_KEY_SIZE] = { 0 };
	u8_t dummy_key[CRYPTO_DES_KEY_SIZE] = { 0 };
	u8_t dummy_plain[CRYPTO_DES_BLK_SIZE] = { 0 };
	u8_t dummy_output[CRYPTO_DES_BLK_SIZE] = { 0 };
	u32_t total_number = 0, actual_number;
	u32_t *src, *dst;
#endif
	u32_t *random_sequence = NULL;
	u8_t *random_plain = NULL;
	u8_t *temp_output = NULL;
	u32_t *cur_ptr_array = NULL;
	u32_t *next_ptr_array = NULL;

	crypto_assert(key != NULL);
	crypto_assert(plain != NULL);
	crypto_assert(cipher != NULL);
	crypto_assert(key_length == CRYPTO_DES_KEY_SIZE);
	crypto_assert(plain_length > 0);
	crypto_assert(plain_length <= CRYPTO_DATA_SIZE);
	if ((plain_length % CRYPTO_DES_BLK_SIZE) != 0) {
		SYS_LOG_DBG("Input length is %d bytes, need padding!!!",
			     plain_length);
		return BCM_SCAPI_STATUS_PARAMETER_INVALID;
	}

	if (des_dpa_policy == DPA_NONE) {
		rc = dpa_disable();
		CRYPTO_RESULT(rc, "dpa_disable failed");
		if (iv) {
			rc = _crypto_des_cbc_encrypt(pHandle, key, key_length,
						     plain, plain_length,
						     iv, iv_length, cipher);
		} else {
			rc = _crypto_des_ecb_encrypt(pHandle, key, key_length,
						     plain, plain_length,
						     cipher);
		}

		CRYPTO_RESULT(rc, "_crypto_des_cbc_encrypt failed");
		return rc;
	}
#ifdef CONFIG_CRYPTO_DPA
	else if (des_dpa_policy == DPA_LOW) {
		memcpy(local_key, key, key_length);
		rc = dpa_init(local_key, key_length, 1);
		CRYPTO_RESULT(rc, "dpa_init failed");
		rc = _crypto_des_cbc_encrypt(pHandle, local_key, key_length,
					     plain, plain_length,
					     iv, iv_length, cipher);
		CRYPTO_RESULT(rc, "_crypto_des_cbc_encrypt failed");
		return rc;
	} else if (des_dpa_policy == DPA_HIGH) {
		local_iv = iv;
		memcpy(local_key, key, key_length);
		rc = dpa_init(local_key, key_length, 1);
		CRYPTO_RESULT(rc, "dpa_init failed");
		if (des_dpa_ratio >= 5) {
			actual_number = plain_length / CRYPTO_DES_BLK_SIZE;
			total_number = actual_number * des_dpa_ratio;
			random_sequence = k_malloc(actual_number * 4);
			if (random_sequence == NULL)
				goto DES_ERROR;

			random_plain =
			    k_malloc(total_number * CRYPTO_DES_BLK_SIZE);
			if (random_plain == NULL)
				goto DES_ERROR;

			/* Generate random for plain */
			for (i = 0; i < total_number; i++) {
				rc = bcm_rng_readwords((u32_t *) &
					random_plain[i * CRYPTO_DES_BLK_SIZE],
						       CRYPTO_DES_BLK_SIZE / 4);
				if (rc != 0)
					goto DES_ERROR;
			}
			temp_output =
			    k_malloc(total_number * CRYPTO_DES_BLK_SIZE);
			if (temp_output == NULL)
				goto DES_ERROR;

			cur_ptr_array = k_malloc(des_dpa_ratio * sizeof(u32_t));
			if (cur_ptr_array == NULL)
				goto DES_ERROR;

			next_ptr_array =
			    k_malloc(des_dpa_ratio * sizeof(u32_t));
			if (next_ptr_array == NULL)
				goto DES_ERROR;

			rc = dpa_random_sequence(actual_number, des_dpa_ratio,
						 random_sequence);
			if (rc != 0)
				goto DES_ERROR;

			rc = bcm_rng_readwords((u32_t *) dummy_key,
					       CRYPTO_DES_KEY_SIZE / 4);
			if (rc != 0)
				goto DES_ERROR;

			rc = bcm_rng_readwords((u32_t *) dummy_plain,
					       CRYPTO_DES_BLK_SIZE / 4);
			if (rc != 0)
				goto DES_ERROR;

			/* Insert real data to dummy data */
			for (i = 0; i < actual_number; i++) {
				for (j = 0; j < CRYPTO_DES_BLK_SIZE; j++) {
					random_plain[random_sequence[i] *
						     CRYPTO_DES_BLK_SIZE + j] =
					    plain[i * CRYPTO_DES_BLK_SIZE + j];
				}
			}
			/*
			 * Prepare initial IV
			 * Copy the IV (passed in as an argument) to all the
			 * blocks at the beginning.
			 */
			local_iv = &temp_output[0];
			for (i = 0; i < des_dpa_ratio; i++) {
				dst =
				    (u32_t *) &local_iv[i *
							 CRYPTO_DES_BLK_SIZE];
				src = (u32_t *) &iv[0];
				dst[0] = src[0];
				dst[1] = src[1];
			}
			/* Initializing the pointer array */
			for (k = 0; k < des_dpa_ratio; k++) {
				cur_ptr_array[k] =
				    (u32_t) &temp_output[k *
							  CRYPTO_DES_BLK_SIZE];
			}
			/* Perform encryption */
			j = 0;
			for (i = 0; i < total_number; i += des_dpa_ratio) {
				local_plain =
				    &random_plain[i * CRYPTO_DES_BLK_SIZE];
				/* xor IV before ecb operation */
				dst = (u32_t *) &local_plain[0];
				src = cur_ptr_array;
				if ((j + 1) < actual_number) {
					/*
					 * Construct the pointer array for the
					 * next block IV shuffling here
					 */
					for (k = 0; k < des_dpa_ratio; k++) {
						next_ptr_array[k] =
						    (u32_t) &
						    temp_output[(i + k) *
							CRYPTO_DES_BLK_SIZE];
					}
					/*
					 * Shuffle other encrypted block output
					 * using array of pointers
					 */
					for (k = 0; k < des_dpa_ratio; k++) {
						next_ptr_array[(random_sequence
								[j + 1] + k) %
							       des_dpa_ratio] =
						 next_ptr_array[(random_sequence
						      [j] + k) % des_dpa_ratio];
					}
				}
				dpa_xor_array((u8_t *) dst, (u8_t *) dst,
					      src, des_dpa_ratio *
					      CRYPTO_DES_BLK_SIZE);
				rc = _crypto_des_ecb_encrypt(pHandle, local_key,
							   CRYPTO_DES_KEY_SIZE,
							   local_plain,
							   CRYPTO_DES_BLK_SIZE
							   * des_dpa_ratio,
							   &temp_output[i *
							  CRYPTO_DES_BLK_SIZE]);
				if (rc != 0)
					goto DES_ERROR;

				/* Copy next to current */
				for (k = 0; k < des_dpa_ratio; k++)
					cur_ptr_array[k] = next_ptr_array[k];

				j++;
			}
			rc = _crypto_des_ecb_encrypt(pHandle, dummy_key,
						     CRYPTO_DES_KEY_SIZE,
						     dummy_plain,
						     CRYPTO_DES_BLK_SIZE,
						     dummy_output);
			if (rc != 0)
				goto DES_ERROR;

			/* Select result from dummy output */
			for (i = 0; i < actual_number; i++) {
				for (j = 0; j < CRYPTO_DES_BLK_SIZE; j++) {
					cipher[i * CRYPTO_DES_BLK_SIZE + j] =
					    temp_output[random_sequence[i] *
						CRYPTO_DES_BLK_SIZE + j];
				}
			}
		} else {
			SYS_LOG_DBG("des_dpa_ratio must be > or = to 5");
			return BCM_SCAPI_STATUS_CRYPTO_ERROR;
		}
	}
DES_ERROR:
#endif
	if (random_sequence)
		k_free(random_sequence);

	if (random_plain)
		k_free(random_plain);

	if (temp_output)
		k_free(temp_output);

	if (cur_ptr_array)
		k_free(cur_ptr_array);

	if (next_ptr_array)
		k_free(next_ptr_array);

	return rc;
}

static BCM_SCAPI_STATUS crypto_des_decrypt(crypto_lib_handle *pHandle,
					   u8_t *key, u32_t key_length,
					   u8_t *cipher, u32_t cipher_length,
					   u8_t *iv, u32_t iv_length,
					   u8_t *plain)
{
	BCM_SCAPI_STATUS rc = 0;
#ifdef CONFIG_CRYPTO_DPA
	u32_t i = 0, j = 0, k = 0;
	u8_t *local_iv = NULL;
	u8_t *local_cipher = NULL;
	u8_t local_key[CRYPTO_DES_KEY_SIZE] = { 0 };
	u8_t dummy_key[CRYPTO_DES_KEY_SIZE] = { 0 };
	u8_t dummy_cipher[CRYPTO_DES_BLK_SIZE] = { 0 };
	u8_t dummy_output[CRYPTO_DES_BLK_SIZE] = { 0 };
	u32_t total_number = 0, actual_number;
	u32_t *src, *dst;
#endif
	u32_t *random_sequence = NULL;
	u8_t *random_cipher = NULL;
	u8_t *temp_output = NULL;
	u8_t *temp_iv = NULL;
	u32_t *ptr_array = NULL;

	crypto_assert(key != NULL);
	crypto_assert(cipher != NULL);
	crypto_assert(plain != NULL);
	crypto_assert(key_length == CRYPTO_DES_KEY_SIZE);
	crypto_assert(cipher_length > 0);
	crypto_assert(cipher_length <= CRYPTO_DATA_SIZE);
	if ((cipher_length % CRYPTO_DES_BLK_SIZE) != 0) {
		SYS_LOG_DBG("Input length is %d bytes, need padding!!!",
			     cipher_length);
		return BCM_SCAPI_STATUS_PARAMETER_INVALID;
	}

	if (des_dpa_policy == DPA_NONE) {
		rc = dpa_disable();
		CRYPTO_RESULT(rc, "dpa_disable failed");
		if (iv)
			rc = _crypto_des_cbc_decrypt(pHandle, key, key_length,
						     cipher, cipher_length,
						     iv, iv_length, plain);
		else
			rc = _crypto_des_ecb_decrypt(pHandle, key, key_length,
						     cipher, cipher_length,
						     plain);

		CRYPTO_RESULT(rc, "_crypto_des_cbc_decrypt failed");
		return rc;
	}
#ifdef CONFIG_CRYPTO_DPA
	else if (des_dpa_policy == DPA_LOW) {
		memcpy(local_key, key, key_length);
		rc = dpa_init(local_key, key_length, 0);
		CRYPTO_RESULT(rc, "dpa_init failed");
		rc = _crypto_des_cbc_decrypt(pHandle, local_key, key_length,
					     cipher, cipher_length,
					     iv, iv_length, plain);
		CRYPTO_RESULT(rc, "_crypto_des_cbc_decrypt failed");
		return rc;
	} else if (des_dpa_policy == DPA_HIGH) {
		local_iv = iv;
		memcpy(local_key, key, key_length);
		rc = dpa_init(local_key, key_length, 0);
		CRYPTO_RESULT(rc, "dpa_init failed");
		if (des_dpa_ratio >= 5) {
			actual_number = cipher_length / CRYPTO_DES_BLK_SIZE;
			total_number = actual_number * des_dpa_ratio;
			random_sequence = k_malloc(actual_number * 4);
			if (random_sequence == NULL)
				goto DES_ERROR;

			random_cipher =
			    k_malloc(total_number * CRYPTO_DES_BLK_SIZE);
			if (random_cipher == NULL)
				goto DES_ERROR;

			/* Generate random for cipher */
			for (i = 0; i < total_number; i++) {
				rc = bcm_rng_readwords((u32_t *) &
					random_cipher[i * CRYPTO_DES_BLK_SIZE],
						       CRYPTO_DES_BLK_SIZE / 4);
				if (rc != 0)
					goto DES_ERROR;
			}
			temp_output =
			    k_malloc(total_number * CRYPTO_DES_BLK_SIZE);
			if (temp_output == NULL)
				goto DES_ERROR;

			temp_iv = k_malloc(des_dpa_ratio * CRYPTO_DES_BLK_SIZE);
			if (temp_iv == NULL)
				goto DES_ERROR;

			ptr_array = k_malloc(des_dpa_ratio * sizeof(u32_t));
			if (ptr_array == NULL)
				goto DES_ERROR;

			rc = dpa_random_sequence(actual_number, des_dpa_ratio,
						 random_sequence);
			if (rc != 0)
				goto DES_ERROR;

			rc = bcm_rng_readwords((u32_t *) dummy_key,
					       CRYPTO_DES_KEY_SIZE / 4);
			if (rc != 0)
				goto DES_ERROR;

			rc = bcm_rng_readwords((u32_t *) dummy_cipher,
					       CRYPTO_DES_BLK_SIZE / 4);
			if (rc != 0)
				goto DES_ERROR;

			/* Insert real data to dummy data */
			for (i = 0; i < actual_number; i++) {
				for (j = 0; j < CRYPTO_DES_BLK_SIZE; j++) {
					random_cipher[random_sequence[i] *
						      CRYPTO_DES_BLK_SIZE + j] =
					    cipher[i * CRYPTO_DES_BLK_SIZE + j];
				}
			}
			/*
			 * Prepare initial IV
			 * Copy the IV (passed in as an argument) to all the
			 * blocks at the beginning.
			 */
			local_iv = &temp_iv[0];
			for (i = 0; i < des_dpa_ratio; i++) {
				dst =
				    (u32_t *) &local_iv[i *
							 CRYPTO_DES_BLK_SIZE];
				src = (u32_t *) &iv[0];
				dst[0] = src[0];
				dst[1] = src[1];
			}
			/* Initializing the pointer array */
			for (k = 0; k < des_dpa_ratio; k++) {
				ptr_array[k] =
				    (u32_t) &temp_iv[k * CRYPTO_DES_BLK_SIZE];
			}
			/* Perform decryption */
			j = 0;
			for (i = 0; i < total_number; i += des_dpa_ratio) {
				local_cipher =
				    &random_cipher[i * CRYPTO_DES_BLK_SIZE];
				rc = _crypto_des_ecb_decrypt(pHandle, local_key,
					CRYPTO_DES_KEY_SIZE, local_cipher,
					CRYPTO_DES_BLK_SIZE * des_dpa_ratio,
					&temp_output[i * CRYPTO_DES_BLK_SIZE]);
				if (rc != 0)
					goto DES_ERROR;

				/* xor IV after ecb operation */
				dst = (u32_t *) &temp_output[i *
							 CRYPTO_DES_BLK_SIZE];
				src = ptr_array;
				dpa_xor_array((u8_t *) dst, (u8_t *) dst,
					      src, des_dpa_ratio *
					      CRYPTO_DES_BLK_SIZE);
				if ((j + 1) < actual_number) {
					/*
					 * Construct the pointer array for the
					 * next block IV shuffling here
					 */
					for (k = 0; k < des_dpa_ratio; k++) {
						ptr_array[k] =
						    (u32_t) &
						    random_cipher[(i + k) *
							 CRYPTO_DES_BLK_SIZE];
					}
					/*
					 * Shuffle other decrypted block output
					 * using array of pointers
					 */
					for (k = 0; k < des_dpa_ratio; k++) {
						ptr_array[(random_sequence
							   [j + 1] + k) %
							   des_dpa_ratio] =
						    ptr_array[(random_sequence
							       [j] + k) %
							      des_dpa_ratio];
					}
				}
				j++;
			}
			rc = _crypto_des_ecb_decrypt(pHandle, dummy_key,
						     CRYPTO_DES_KEY_SIZE,
						     dummy_cipher,
						     CRYPTO_DES_BLK_SIZE,
						     dummy_output);
			if (rc != 0)
				goto DES_ERROR;

			/* Select result from dummy output */
			for (i = 0; i < actual_number; i++) {
				for (j = 0; j < CRYPTO_DES_BLK_SIZE; j++) {
					plain[i * CRYPTO_DES_BLK_SIZE + j] =
					    temp_output[random_sequence[i] *
						CRYPTO_DES_BLK_SIZE + j];
				}
			}
		} else {
			SYS_LOG_DBG("des_dpa_ratio must be > or = to 5");
			return BCM_SCAPI_STATUS_CRYPTO_ERROR;
		}
	}
DES_ERROR:
#endif
	if (random_sequence)
		k_free(random_sequence);

	if (random_cipher)
		k_free(random_cipher);

	if (temp_output)
		k_free(temp_output);

	if (temp_iv)
		k_free(temp_iv);

	if (ptr_array)
		k_free(ptr_array);

	return rc;
}

static BCM_SCAPI_STATUS crypto_3des_encrypt(crypto_lib_handle *pHandle,
					    u8_t *key, u32_t key_length,
					    u8_t *plain, u32_t plain_length,
					    u8_t *iv, u32_t iv_length,
					    u8_t *cipher)
{
	BCM_SCAPI_STATUS rc = 0;
#ifdef CONFIG_CRYPTO_DPA
	u32_t i = 0, j = 0, k = 0;
	u8_t *local_iv = NULL;
	u8_t *local_plain = NULL;
	u8_t local_key[CRYPTO_3DES_KEY_SIZE] = { 0 };
	u8_t dummy_key[CRYPTO_3DES_KEY_SIZE] = { 0 };
	u8_t dummy_plain[CRYPTO_3DES_BLK_SIZE] = { 0 };
	u8_t dummy_output[CRYPTO_3DES_BLK_SIZE] = { 0 };
	u32_t total_number = 0, actual_number;
	u32_t *src, *dst;
#endif
	u32_t *random_sequence = NULL;
	u8_t *random_plain = NULL;
	u8_t *temp_output = NULL;
	u32_t *cur_ptr_array = NULL;
	u32_t *next_ptr_array = NULL;

	crypto_assert(key != NULL);
	crypto_assert(plain != NULL);
	crypto_assert(cipher != NULL);
	crypto_assert(key_length == CRYPTO_3DES_KEY_SIZE);
	crypto_assert(plain_length > 0);
	crypto_assert(plain_length <= CRYPTO_DATA_SIZE);
	if ((plain_length % CRYPTO_3DES_BLK_SIZE) != 0) {
		SYS_LOG_DBG("Input length is %d bytes, need padding!!!",
			     plain_length);
		return BCM_SCAPI_STATUS_PARAMETER_INVALID;
	}

	if (des_dpa_policy == DPA_NONE) {
		rc = dpa_disable();
		CRYPTO_RESULT(rc, "dpa_disable failed");
		if (iv)
			rc = _crypto_3des_cbc_encrypt(pHandle, key, key_length,
						      plain, plain_length,
						      iv, iv_length, cipher);
		else
			rc = _crypto_3des_ecb_encrypt(pHandle, key, key_length,
						      plain, plain_length,
						      cipher);
		CRYPTO_RESULT(rc, "_crypto_3des_cbc_encrypt failed");
		return rc;
	}
#ifdef CONFIG_CRYPTO_DPA
	else if (des_dpa_policy == DPA_LOW) {
		memcpy(local_key, key, key_length);
		rc = dpa_init(local_key, key_length, 1);
		CRYPTO_RESULT(rc, "dpa_init failed");
		if (iv)
			rc = _crypto_3des_cbc_encrypt(pHandle, local_key,
						      key_length,
						      plain, plain_length,
						      iv, iv_length, cipher);
		else
			rc = _crypto_3des_ecb_encrypt(pHandle, local_key,
						      key_length,
						      plain, plain_length,
						      cipher);

		CRYPTO_RESULT(rc, "_crypto_3des_cbc_encrypt failed");
		return rc;
	} else if (des_dpa_policy == DPA_HIGH) {
		memcpy(local_key, key, key_length);
		rc = dpa_init(local_key, key_length, 1);
		CRYPTO_RESULT(rc, "dpa_init failed");
		if (des_dpa_ratio >= 5) {
			actual_number = plain_length / CRYPTO_3DES_BLK_SIZE;
			total_number = actual_number * des_dpa_ratio;
			random_sequence = k_malloc(actual_number * 4);
			if (random_sequence == NULL)
				goto DES_ERROR;

			random_plain =
			    k_malloc(total_number * CRYPTO_3DES_BLK_SIZE);
			if (random_plain == NULL)
				goto DES_ERROR;

			/* Generate random for plain */
			for (i = 0; i < total_number; i++) {
				rc = bcm_rng_readwords((u32_t *) &
					random_plain[i * CRYPTO_3DES_BLK_SIZE],
					CRYPTO_3DES_BLK_SIZE / 4);
				if (rc != 0)
					goto DES_ERROR;

			}
			temp_output =
			    k_malloc(total_number * CRYPTO_3DES_BLK_SIZE);
			if (temp_output == NULL)
				goto DES_ERROR;

			cur_ptr_array = k_malloc(des_dpa_ratio * sizeof(u32_t));
			if (cur_ptr_array == NULL)
				goto DES_ERROR;

			next_ptr_array =
			    k_malloc(des_dpa_ratio * sizeof(u32_t));
			if (next_ptr_array == NULL)
				goto DES_ERROR;

			rc = dpa_random_sequence(actual_number, des_dpa_ratio,
						 random_sequence);
			if (rc != 0)
				goto DES_ERROR;

			rc = bcm_rng_readwords((u32_t *) dummy_key,
					       CRYPTO_3DES_KEY_SIZE / 4);
			if (rc != 0)
				goto DES_ERROR;

			rc = bcm_rng_readwords((u32_t *) dummy_plain,
					       CRYPTO_3DES_BLK_SIZE / 4);
			if (rc != 0)
				goto DES_ERROR;

			/* Insert real data to dummy data */
			for (i = 0; i < actual_number; i++) {
				for (j = 0; j < CRYPTO_3DES_BLK_SIZE; j++) {
					random_plain[random_sequence[i] *
						     CRYPTO_3DES_BLK_SIZE + j] =
					    plain[i * CRYPTO_3DES_BLK_SIZE + j];
				}
			}
			/*
			 * Prepare initial IV
			 * Copy the IV (passed in as an argument) to all the
			 * blocks at the beginning.
			 */
			local_iv = &temp_output[0];
			for (i = 0; i < des_dpa_ratio; i++) {
				dst = (u32_t *) &local_iv[i *
							 CRYPTO_3DES_BLK_SIZE];
				src = (u32_t *) &iv[0];
				dst[0] = src[0];
				dst[1] = src[1];
			}
			/* Initializing the pointer array */
			for (k = 0; k < des_dpa_ratio; k++) {
				cur_ptr_array[k] =
				    (u32_t) &temp_output[k *
							  CRYPTO_3DES_BLK_SIZE];
			}
			/* Perform encryption */
			j = 0;
			for (i = 0; i < total_number; i += des_dpa_ratio) {
				local_plain =
				    &random_plain[i * CRYPTO_3DES_BLK_SIZE];
				/* xor IV before ecb operation */
				dst = (u32_t *) &local_plain[0];
				src = cur_ptr_array;
				if ((j + 1) < actual_number) {
					/*
					 * Construct the pointer array for the
					 * next block IV shuffling here
					 */
					for (k = 0; k < des_dpa_ratio; k++) {
						next_ptr_array[k] =
						    (u32_t) &
						    temp_output[(i + k) *
							CRYPTO_3DES_BLK_SIZE];
					}
					/*
					 * Shuffle other encrypted block output
					 * using array of pointers
					 */
					for (k = 0; k < des_dpa_ratio; k++) {
						next_ptr_array[(random_sequence
								[j + 1] + k) %
							       des_dpa_ratio] =
						next_ptr_array[(random_sequence
						      [j] + k) % des_dpa_ratio];
					}
				}
				dpa_xor_array((u8_t *) dst, (u8_t *) dst,
					      src, des_dpa_ratio *
					      CRYPTO_3DES_BLK_SIZE);
				rc = _crypto_3des_ecb_encrypt(pHandle,
					local_key,
					CRYPTO_3DES_KEY_SIZE, local_plain,
					CRYPTO_3DES_BLK_SIZE * des_dpa_ratio,
					&temp_output[i * CRYPTO_3DES_BLK_SIZE]);
				if (rc != 0)
					goto DES_ERROR;

				/* Copy next to current */
				for (k = 0; k < des_dpa_ratio; k++)
					cur_ptr_array[k] = next_ptr_array[k];

				j++;
			}
			rc = _crypto_3des_ecb_encrypt(pHandle, dummy_key,
						      CRYPTO_3DES_KEY_SIZE,
						      dummy_plain,
						      CRYPTO_3DES_BLK_SIZE,
						      dummy_output);
			if (rc != 0)
				goto DES_ERROR;

			/* Select result from dummy output */
			for (i = 0; i < actual_number; i++) {
				for (j = 0; j < CRYPTO_3DES_BLK_SIZE; j++) {
					cipher[i * CRYPTO_3DES_BLK_SIZE + j] =
					    temp_output[random_sequence[i] *
						CRYPTO_3DES_BLK_SIZE + j];
				}
			}
		} else {
			SYS_LOG_DBG("des_dpa_ratio must be > or = to 5");
			return BCM_SCAPI_STATUS_CRYPTO_ERROR;
		}
	}
DES_ERROR:
#endif
	if (random_sequence)
		k_free(random_sequence);

	if (random_plain)
		k_free(random_plain);

	if (temp_output)
		k_free(temp_output);

	if (cur_ptr_array)
		k_free(cur_ptr_array);

	if (next_ptr_array)
		k_free(next_ptr_array);

	return rc;
}

static BCM_SCAPI_STATUS crypto_3des_decrypt(crypto_lib_handle *pHandle,
					    u8_t *key, u32_t key_length,
					    u8_t *cipher, u32_t cipher_length,
					    u8_t *iv, u32_t iv_length,
					    u8_t *plain)
{
	BCM_SCAPI_STATUS rc = 0;
#ifdef CONFIG_CRYPTO_DPA
	u32_t i = 0, j = 0, k = 0;
	u8_t *local_iv = NULL;
	u8_t *local_cipher = NULL;
	u8_t local_key[CRYPTO_3DES_KEY_SIZE] = { 0 };
	u8_t dummy_key[CRYPTO_3DES_KEY_SIZE] = { 0 };
	u8_t dummy_cipher[CRYPTO_3DES_BLK_SIZE] = { 0 };
	u8_t dummy_output[CRYPTO_3DES_BLK_SIZE] = { 0 };
	u32_t total_number = 0, actual_number;
	u32_t *src, *dst;
#endif
	u32_t *random_sequence = NULL;
	u8_t *random_cipher = NULL;
	u8_t *temp_output = NULL;
	u8_t *temp_iv = NULL;
	u32_t *ptr_array = NULL;

	crypto_assert(key != NULL);
	crypto_assert(cipher != NULL);
	crypto_assert(plain != NULL);
	crypto_assert(key_length == CRYPTO_3DES_KEY_SIZE);
	crypto_assert(cipher_length > 0);
	crypto_assert(cipher_length <= CRYPTO_DATA_SIZE);
	if ((cipher_length % CRYPTO_3DES_BLK_SIZE) != 0) {
		SYS_LOG_DBG("Input length is %d bytes, need padding!!!",
			     cipher_length);
		return BCM_SCAPI_STATUS_PARAMETER_INVALID;
	}

	if (des_dpa_policy == DPA_NONE) {
		rc = dpa_disable();
		CRYPTO_RESULT(rc, "dpa_disable failed");
		if (iv)
			rc = _crypto_3des_cbc_decrypt(pHandle, key, key_length,
						      cipher, cipher_length,
						      iv, iv_length, plain);
		else
			rc = _crypto_3des_ecb_decrypt(pHandle, key, key_length,
						      cipher, cipher_length,
						      plain);
		CRYPTO_RESULT(rc, "_crypto_3des_cbc_decrypt failed");
		return rc;
	}
#ifdef CONFIG_CRYPTO_DPA
	else if (des_dpa_policy == DPA_LOW) {
		memcpy(local_key, key, key_length);
		rc = dpa_init(local_key, key_length, 0);
		CRYPTO_RESULT(rc, "dpa_init failed");
		if (iv)
			rc = _crypto_3des_cbc_decrypt(pHandle, local_key,
							key_length,
							cipher, cipher_length,
							iv, iv_length, plain);
		else
			rc = _crypto_3des_ecb_decrypt(pHandle, key, key_length,
						      cipher, cipher_length,
						      plain);
		CRYPTO_RESULT(rc, "_crypto_3des_cbc_decrypt failed");
		return rc;
	} else if (des_dpa_policy == DPA_HIGH) {
		memcpy(local_key, key, key_length);
		rc = dpa_init(local_key, key_length, 0);
		CRYPTO_RESULT(rc, "dpa_init failed");
		if (des_dpa_ratio >= 5) {
			actual_number = cipher_length / CRYPTO_3DES_BLK_SIZE;
			total_number = actual_number * des_dpa_ratio;
			random_sequence = k_malloc(actual_number * 4);
			if (random_sequence == NULL)
				goto DES_ERROR;

			random_cipher =
			    k_malloc(total_number * CRYPTO_3DES_BLK_SIZE);
			if (random_cipher == NULL)
				goto DES_ERROR;

			/* Generate random for cipher */
			for (i = 0; i < total_number; i++) {
				rc = bcm_rng_readwords(
					(u32_t *) &random_cipher[i *
						     CRYPTO_3DES_BLK_SIZE],
					       CRYPTO_3DES_BLK_SIZE / 4);
				if (rc != 0)
					goto DES_ERROR;

			}
			temp_output =
			    k_malloc(total_number * CRYPTO_3DES_BLK_SIZE);
			if (temp_output == NULL)
				goto DES_ERROR;

			temp_iv =
			    k_malloc(des_dpa_ratio * CRYPTO_3DES_BLK_SIZE);
			if (temp_iv == NULL)
				goto DES_ERROR;

			ptr_array = k_malloc(des_dpa_ratio * sizeof(u32_t));
			if (ptr_array == NULL)
				goto DES_ERROR;

			rc = dpa_random_sequence(actual_number, des_dpa_ratio,
						 random_sequence);
			if (rc != 0)
				goto DES_ERROR;

			rc = bcm_rng_readwords((u32_t *) dummy_key,
					       CRYPTO_3DES_KEY_SIZE / 4);
			if (rc != 0)
				goto DES_ERROR;

			rc = bcm_rng_readwords((u32_t *) dummy_cipher,
					       CRYPTO_3DES_BLK_SIZE / 4);
			if (rc != 0)
				goto DES_ERROR;

			/* Insert real data to dummy data */
			for (i = 0; i < actual_number; i++) {
				for (j = 0; j < CRYPTO_3DES_BLK_SIZE; j++) {
					random_cipher[random_sequence[i] *
						     CRYPTO_3DES_BLK_SIZE + j] =
					   cipher[i * CRYPTO_3DES_BLK_SIZE + j];
				}
			}
			/*
			 * Prepare initial IV
			 * Copy the IV (passed in as an argument) to all the
			 * blocks at the beginning
			 */
			local_iv = &temp_iv[0];
			for (i = 0; i < des_dpa_ratio; i++) {
				dst = (u32_t *) &local_iv[i *
							 CRYPTO_3DES_BLK_SIZE];
				src = (u32_t *) &iv[0];
				dst[0] = src[0];
				dst[1] = src[1];
			}
			/* Initializing the pointer array */
			for (k = 0; k < des_dpa_ratio; k++) {
				ptr_array[k] =
				    (u32_t) &temp_iv[k * CRYPTO_3DES_BLK_SIZE];
			}
			/* Perform decryption */
			j = 0;
			for (i = 0; i < total_number; i += des_dpa_ratio) {
				local_cipher =
				    &random_cipher[i * CRYPTO_3DES_BLK_SIZE];
				rc = _crypto_3des_ecb_decrypt(pHandle,
							local_key,
							CRYPTO_3DES_KEY_SIZE,
							local_cipher,
							CRYPTO_3DES_BLK_SIZE
							*des_dpa_ratio,
							&temp_output[i *
							 CRYPTO_3DES_BLK_SIZE]);
				if (rc != 0)
					goto DES_ERROR;

				/* xor IV after ecb operation */
				dst = (u32_t *) &temp_output[i *
							 CRYPTO_3DES_BLK_SIZE];
				src = ptr_array;
				dpa_xor_array((u8_t *) dst, (u8_t *) dst,
					      src, des_dpa_ratio *
					      CRYPTO_3DES_BLK_SIZE);
				if ((j + 1) < actual_number) {
					/*
					 * Construct the pointer array for the
					 * next block IV shuffling here
					 */
					for (k = 0; k < des_dpa_ratio; k++) {
						ptr_array[k] =
						    (u32_t) &
						    random_cipher[(i + k) *
							  CRYPTO_3DES_BLK_SIZE];
					}
					/*
					 * Shuffle other decrypted block output
					 * using array of pointers
					 */
					for (k = 0; k < des_dpa_ratio; k++) {
						ptr_array[(random_sequence
							   [j + 1] +
							   k) % des_dpa_ratio] =
						    ptr_array[(random_sequence
							       [j] + k) %
							      des_dpa_ratio];
					}
				}
				j++;
			}
			rc = _crypto_3des_ecb_decrypt(pHandle, dummy_key,
					CRYPTO_3DES_KEY_SIZE, dummy_cipher,
					CRYPTO_3DES_BLK_SIZE, dummy_output);
			if (rc != 0)
				goto DES_ERROR;

			/* Select result from dummy output */
			for (i = 0; i < actual_number; i++) {
				for (j = 0; j < CRYPTO_DES_BLK_SIZE; j++) {
					plain[i * CRYPTO_DES_BLK_SIZE + j] =
					    temp_output[random_sequence[i] *
						       CRYPTO_DES_BLK_SIZE + j];
				}
			}
		} else {
			SYS_LOG_DBG("des_dpa_ratio must be > or = to 5");
			return BCM_SCAPI_STATUS_CRYPTO_ERROR;
		}
	}
DES_ERROR:
#endif
	if (random_sequence)
		k_free(random_sequence);

	if (random_cipher)
		k_free(random_cipher);

	if (temp_output)
		k_free(temp_output);

	if (temp_iv)
		k_free(temp_iv);

	if (ptr_array)
		k_free(ptr_array);

	return rc;
}

BCM_SCAPI_STATUS bcm_rng_readwords(u32_t *word, s32_t word_num)
{
	crypto_lib_handle *p_handle = (crypto_lib_handle *)&cryptoHandle;
	BCM_SCAPI_STATUS rv;

	if (p_handle->rngctx == NULL) {
		crypto_init((crypto_lib_handle *)&cryptoHandle);
		rv = crypto_rng_init((crypto_lib_handle *)&cryptoHandle,
					&rngctx, 1, NULL, 0, NULL, 0, NULL, 0);
		if (rv != BCM_SCAPI_STATUS_OK) {
			SYS_LOG_ERR("Couldn't initialise rng\n");
			return rv;
		}
	}

	rv = crypto_rng_raw_generate((crypto_lib_handle *)&cryptoHandle,
					(u8_t *)word, word_num, NULL, 0);

	if (rv != BCM_SCAPI_STATUS_OK)
		SYS_LOG_ERR("Couldn't initialise rng\n");

	return rv;
}

s32_t crypto_swap_end(u32_t *ptr)
{
	u32_t data = 0;

	data = ((ptr[0] & 0xff) << 24) |
		((ptr[0] & 0xff00) << 8) |
		((ptr[0] & 0xff0000) >> 8) |
		((ptr[0] & 0xff000000) >> 24);
	ptr[0] = data;

	return 0;
}
