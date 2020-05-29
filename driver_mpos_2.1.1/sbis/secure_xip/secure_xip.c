/******************************************************************************
 *  Copyright (c) 2005-2018 Broadcom. All Rights Reserved. The term "Broadcom"
 *  refers to Broadcom Inc. and/or its subsidiaries.
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

#include <platform.h>
#include <post_log.h>
#include <smau.h>
#include <crypto/crypto.h>
#include <qspi_flash.h>
#include <sotp.h>
#include <string.h>
#include <utils.h>
#include <crypto_util.h>
#include <misc/util.h>
#include "secure_xip.h"

#define DEBUG_LOG(A,...)

/* Uncomment this to get the SCD buffer logs */
/* #define ENABLE_SCD_LOGS */

#ifdef ENABLE_SCD_LOGS
#define LOG_SCD(A, B)	do {		\
		post_log("SCD\n===\n");	\
		log_buf(A, B);		\
	} while (0);
#else
#define LOG_SCD(A, B);
#endif

// scd header dump by tools/aai_tool/mkaai.py version 1
struct aai_scd_t {
	uint32_t magic0;    // 0xfacefeed
	uint32_t magic1;    // 0xdeadbeaf
	uint32_t version;    // 0x1
	uint32_t valid;    // 0x0
	uint32_t faddr;    // 0x10000
	uint32_t xip;    // 0x0
	uint32_t size;    // 0x5e100
	uint32_t wbase;    // 0x0
	uint32_t hbase;    // 0x0
	uint32_t aeskey[8];
	uint32_t hkey[8];
	uint32_t rev[31];
	uint32_t hmac[8];
};

struct xip_cb_t {
	uint32_t a0key[X_A_KLENW];
	uint32_t h0key[X_H_KLENW];
	/* FIXME: Performing encryption along with hmac calculation
	 * overwrites the memory between the end of the iv and output
	 * buffer when when crypto offset is non-zero. These reserved
	 * words are to workaround that overwrite. The right fix is to
	 * split the cyrpto_smau_cipher_start() calls to 2. to individually
	 * do the encryption and hmac in crypto_block_encrypt_auth()
	 */
	uint32_t reserved[4];
	uint32_t cipher[X_A_BSIZEW + X_H_BSIZEW];
	uint32_t clear[X_IV_SIZEW + X_A_BSIZEW];
	uint32_t chunk_size;
	uint32_t chunk_blks;
	uint32_t chunks;
	uint32_t blocks;
	uint32_t ablk;
	uint32_t hblk;
	uint32_t wbase;
	uint32_t hbase;
	struct aai_scd_t scd;
};

static struct xip_cb_t xcb;
static uint8_t crypto_cipher_iv[] __attribute__((aligned(4)))= {
        0x00, 0x01, 0x02, 0x03,
        0x04, 0x05, 0x06, 0x07,
        0x08, 0x09, 0x0a, 0x0b,
        0x0c, 0x0d, 0x0e, 0x0f};

int xip_rng_init(void)
{
	static int init = 0;
	uint32_t v;

	if (!init) {
		reg_bit_set(rng_CTRL, rng_CTRL__RNG_RBGEN);

		/* warm up */
		while ((reg_read(rng_STATUS) & 0xfffff) != 0xfffff);
		init = 1;
	}

	return 0;
}

int xip_rng_deinit(void)
{
	reg_bit_clr(rng_CTRL, rng_CTRL__RNG_RBGEN);
	return 0;
}

int xip_rng(uint32_t *buf, int cnt)
{
	int i = 0;

	while (cnt--) {
		while (!(reg_read(rng_STATUS) & 0xff000000));
		buf[i++] = reg_read(rng_DATA);
	}

	return 0;
}

int xip_dump_otp(void)
{
	uint32_t buf[8];
	uint32_t Key[8];
	uint16_t len;
	sotp_bcm58202_status_t status;
	int row;
	uint64_t row_data;

	xip_rng_init();
	xip_rng(buf, 8);
	post_log("random number %08x %08x %08x %08x\n", buf[0], buf[1], buf[2], buf[3]);

	for (row = 0; row < 64; row++) {
		row_data = sotp_mem_read(row, 0);
		post_log("%2d: 0x%08x-%08x\n",
				row,
				(uint32_t)((row_data >> 32) & 0xffffffff),
				(uint32_t)(row_data & 0xffffffff));
	}

	status = sotp_read_key((uint8_t *)&Key[0], &len, OTPKey_DAUTH);
	if (status != IPROC_OTP_INVALID)
	{
		post_log("OTPKey_DAUTH is already programmed \n", status);
		post_log(" %08x %08x %08x %08x : %08x %08x %08x %08x\n", Key[0], Key[1], Key[2], Key[3],
			Key[4], Key[5], Key[6], Key[7]);
	}

	status = sotp_read_key ((uint8_t *)&Key[0], &len, OTPKey_HMAC_SHA256);
	if (status != IPROC_OTP_INVALID)
	{
		post_log("KHMAC is already programmed\n", status);
	}

	status = sotp_read_key ((uint8_t *)&Key[0], &len, OTPKey_AES);
	if (status != IPROC_OTP_INVALID)
	{
		post_log("KAES is already programmed: %d\n", status);
		post_log(" %08x %08x %08x %08x : %08x %08x %08x %08x\n", Key[0], Key[1], Key[2], Key[3],
			Key[4], Key[5], Key[6], Key[7]);
	}

	return 0;
}

int xip_get_sotpkey(uint32_t *a0key, uint32_t *h0key)
{
	uint32_t keySize = 32;
	uint16_t len;
	sotp_bcm58202_status_t status;

	/* after program otp, need reset to let hw crc check function */

	status = sotp_read_key((uint8_t *)a0key, &len, OTPKey_AES);
	if (status != IPROC_OTP_INVALID) {
		post_log("KAES is already programmed: %d\n", status);
	} else {
		xip_rng_init();
		xip_rng(a0key, 8);

		status = sotp_set_key((uint8_t*)a0key, keySize, OTPKey_AES);
		if(status != IPROC_OTP_VALID)
		{
			post_log("sotp_set_key() AES Key programmed failed [%d]\n", status);
			return -1;
		}
		post_log("AES Key programmed\n");
	}

	status = sotp_read_key((uint8_t *)h0key, &len, OTPKey_HMAC_SHA256);
	if (status != IPROC_OTP_INVALID) {
		post_log("KHMAC is already programmed: %d\n", status);
	} else {
		xip_rng_init();
		xip_rng(h0key, 8);

		status = sotp_set_key((uint8_t*)h0key, keySize, OTPKey_HMAC_SHA256);
		if(status != IPROC_OTP_VALID)
		{
			post_log("sotp_set_key() HMAC Key programmed failed [%d]\n", status);
			return -2;
		}
		post_log("HMAC Key programmed\n");
	}

	return 0;
}

static uint32_t smau_get_hmac_pa(uint32_t haddr)
{
	return (haddr / X_A_BSIZE) * X_H_KLEN;
}

int xip_config(uint32_t *scd, uint32_t chunk_size)
{
	int ret = 0;

	if (sizeof(struct aai_scd_t) != X_SCD_SIZE) {
		DEBUG_LOG("sizeof aai_scd_t: %d, should be %d\n", sizeof(struct aai_scd_t), X_SCD_SIZE);
		return -1;
	}

	if (!((scd[0] == 0xfacefeed) && (scd[1] == 0xdeadbeaf))) {
		DEBUG_LOG("scd magic error: %x %x\n", scd[0], scd[1]);
		return -2;
	}

	memcpy((uint8_t *)&xcb.scd, scd, X_SCD_SIZE);

	if (xcb.scd.size & (X_A_BSIZE - 1)) {
		DEBUG_LOG("aai size not aligned: %x \n", xcb.scd.size);
		return -3;
	}

	if (xcb.scd.faddr & (X_FLH_SS - 1)) {
		DEBUG_LOG("scd begin address not aligned to flash section: %x \n", xcb.scd.faddr);
		return -4;
	}

	if (xcb.scd.xip) {
		ret = xip_get_sotpkey(xcb.a0key, xcb.h0key);
		if (ret) 
			return -5;
	}

	xcb.wbase = xcb.scd.faddr;
	xcb.blocks = xcb.scd.size / X_A_BSIZE;
	xcb.hbase = xcb.wbase + xcb.scd.size;
	xcb.hbase = xcb.hbase + X_FLH_SS - xcb.hbase % X_FLH_SS;
	xcb.ablk = 0;
	/* HMACSHA block index for the first block in the image at window base */
	xcb.hblk = smau_get_hmac_pa(xcb.wbase) / X_H_BSIZE + 1;
	xcb.chunk_size = chunk_size;
	xcb.chunk_blks = chunk_size/X_A_BSIZE;
	xcb.chunks = (xcb.scd.size - 1) / chunk_size + 1;

	DEBUG_LOG("scd: xip: %d size: %x \n", xcb.scd.xip, xcb.scd.size);
	DEBUG_LOG("wbase: %x hbase: %x blocks: %d hblk: %d \n", xcb.wbase, xcb.hbase, xcb.blocks, xcb.hblk);
	DEBUG_LOG("chunks: %x chunk_blks: %x \n", xcb.chunks, xcb.chunk_blks);

	xcb.scd.valid = 1;

	if (xcb.scd.xip) {

		xcb.scd.wbase = xcb.wbase;
		xcb.scd.hbase = xcb.hbase;

		xip_rng_init();
		xip_rng(xcb.scd.aeskey, X_A_KLENW);
		xip_rng(xcb.scd.hkey, X_H_KLENW);

		//log_buf(&xcb.scd, X_SCD_SIZE);
	}

	return xcb.chunks;
}

int xip_do_one_block(uint32_t *p)
{
	int haddr = 0;
	struct device *qspi_dev = 
		device_get_binding(CONFIG_PRIMARY_FLASH_DEV_NAME);

	if (xcb.ablk > xcb.blocks) {
		DEBUG_LOG("aseblock: %d exceeds %d \n", xcb.ablk, xcb.blocks);
		return -1;
	}

	if (xcb.ablk == 1) {
		qspi_flash_erase(qspi_dev, xcb.wbase, X_FLH_SS);
		qspi_flash_erase(qspi_dev, xcb.hbase, X_FLH_SS);
	}

	memset(xcb.cipher, 0, sizeof(xcb.cipher));

	haddr = (xcb.wbase | (FLASH_START)) +
		 xcb.ablk * X_A_BSIZE;
	smau_get_pma_iv(device_get_binding(CONFIG_SMAU_NAME),
			haddr, (uint8_t *)xcb.clear);
	haddr &= 0x00FFFFFF;
	swap_end(xcb.clear, xcb.clear, X_IV_SIZE);
	memcpy(&xcb.clear[X_IV_SIZEW], p, X_A_BSIZE);
	//DEBUG_LOG("-> aes addr: %x, %d \n", haddr);
	//log_buf((uint32_t *)xcb.clear,  X_A_BSIZE + X_IV_SIZE);

	if (!(haddr & (X_FLH_SS - 1))) {
		qspi_flash_erase(qspi_dev, haddr, X_FLH_SS);
		DEBUG_LOG(">>>> erase haddr: %x\n", haddr);
	}

#ifdef ALGO_HMAC
    crypto_hmac_sha256_digest((uint8_t *)xcb.scd.hkey, 32, (uint8_t *)xcb.clear, X_IV_SIZE+X_A_BSIZE, (uint8_t *)(&xcb.cipher[X_A_BSIZEW]));
    qspi_flash_write(qspi_dev, haddr, X_A_BSIZE, (uint8_t *)p);
#else
    /* caculate AES & auth use only one dma */
    crypto_block_encrypt_auth((uint8_t *)xcb.scd.aeskey, X_A_KLEN, (uint8_t *)xcb.scd.hkey,
        32, (uint8_t *)xcb.clear, X_IV_SIZE, X_A_BSIZE, (uint8_t *)xcb.cipher);
    //log_buf((uint8_t *)xcb.cipher,  X_A_BSIZE + X_H_BSIZE);

    qspi_flash_write(qspi_dev, haddr, X_A_BSIZE, (uint8_t *)xcb.cipher);
#endif

	haddr = xcb.hbase + xcb.hblk * X_H_KLEN;
	if (!(haddr & (X_FLH_SS - 1))) {
		qspi_flash_erase(qspi_dev, haddr, X_FLH_SS);
		DEBUG_LOG(">>>> erase haddr: %x\n", haddr);
	}

	qspi_flash_write(qspi_dev, haddr, X_H_BSIZE, (uint8_t *)(&xcb.cipher[X_A_BSIZEW]));

	//DEBUG_LOG("-> haddr: %x %08x %08x\n", haddr, xcb.cipher[0], xcb.cipher[X_A_BSIZEW]);

	xcb.ablk++;
	xcb.hblk++;

	//if (xcb.ablk >= 3)
		//while(1);

	return 0;
}

int xip_prg_aai(uint32_t *p)
{
    if (!xcb.ablk) {
        xcb.ablk = 1;
        return 0;
    } else {
        return xip_do_one_block(p);
    }
}

int xip_crypto_hmac_scd(void)
{
	int rc = 0;
	uint32_t output[16];
	uint32_t digest[8];

	LOG_SCD((u32_t *)&xcb.scd, X_SCD_SIZE);
	rc = crypto_aes_128_cbc_encrypt((uint8_t *)xcb.a0key, 16, (uint8_t *)xcb.scd.aeskey, 64, crypto_cipher_iv, 16, (uint8_t *)output);
	if (rc) {
		DEBUG_LOG("crypto_aes_128_cbc_encrypt error: %d\n", rc);
		while (1);
	}

	memcpy(xcb.scd.aeskey, output, 64);
	rc = crypto_hmac_sha256_digest((uint8_t *)xcb.h0key, 32, (uint8_t *)&xcb.scd, (sizeof(struct aai_scd_t) - 32), (uint8_t *)digest);
	if (rc) {
		DEBUG_LOG("crypto_hmac_sha256_digest error: %d\n", rc);
		while (1);
	}
	memcpy(xcb.scd.hmac, digest, 32);

	LOG_SCD((u32_t *)&xcb.scd, X_SCD_SIZE);
	
	return 0;
}

int xip_recover_scd(struct aai_scd_t *scd)
{
	int rc = 0;
	uint32_t output[16];
	uint32_t digest[8];

	LOG_SCD((u32_t *)scd, X_SCD_SIZE);
	rc = crypto_hmac_sha256_digest((uint8_t *)xcb.h0key, 32, (uint8_t *)scd, (sizeof(struct aai_scd_t) - 32), (uint8_t *)digest);
	if (rc) {
		DEBUG_LOG("crypto_hmac_sha256_digest error: %d\n", rc);
		while (1);
	}
	if (memcmp(digest, scd->hmac, 32)) {
		DEBUG_LOG("Digest mismatch for SCD header!");
		log_buf((u32_t *)digest, 32);
		log_buf((u32_t *)scd->hmac, 32);
		while (1);
	}

	rc = crypto_aes_128_cbc_decrypt((uint8_t *)xcb.a0key, 16, (uint8_t *)scd->aeskey, 64, crypto_cipher_iv, 16, (uint8_t *)output);
	if (rc) {
		DEBUG_LOG("crypto_aes_128_cbc_encrypt error: %d\n", rc);
		while (1);
	}

	memcpy(scd->aeskey, output, 64);
	LOG_SCD((u32_t *)scd, X_SCD_SIZE);
	return 0;
}

int xip_prg_chunk(uint32_t *p)
{
    u32_t i;
    int haddr = 0;
    struct device *qspi_dev = 
	device_get_binding(CONFIG_PRIMARY_FLASH_DEV_NAME);

    if (!xcb.scd.xip) {
        haddr = xcb.wbase + xcb.ablk * X_A_BSIZE;
        qspi_flash_erase(qspi_dev, haddr, xcb.chunk_size);
        if (xcb.ablk == 0) {
            qspi_flash_write(qspi_dev, xcb.wbase, X_SCD_SIZE, (uint8_t *) &xcb.scd);
            qspi_flash_write(qspi_dev, xcb.wbase + X_SCD_SIZE, xcb.chunk_size - X_SCD_SIZE, (uint8_t *) &p[X_A_BSIZEW]);
        } else {
            qspi_flash_write(qspi_dev, haddr, xcb.chunk_size, (uint8_t *) p);
        }
        xcb.ablk += xcb.chunk_blks;
        post_log("qspi_flash_write plain AAI: addr %x, %d\n", haddr, xcb.chunk_size);
        return 0;
    } else {
        for (i = 0; i < xcb.chunk_blks; i++) {
            xip_prg_aai(&p[i * X_A_BSIZEW]);
            if (xcb.ablk > xcb.blocks) {
                xip_crypto_hmac_scd();
                qspi_flash_write(qspi_dev, xcb.wbase, X_SCD_SIZE, (uint8_t *) &xcb.scd);
                post_log("Done scd write \n");
                break;
            }
        }
    }

    return 0;
}

__attribute__((__section__("text.fastcode")))
int xip_set_smau(uint32_t wbase, uint32_t hbase, uint32_t *akey, uint32_t *hkey)
{
	uint32_t akey_s[X_A_KLENW];
	uint32_t hkey_s[X_H_KLENW];
	uint32_t i, key;
	int win = 0;
	struct device *smau_dev;

	smau_dev = device_get_binding(CONFIG_SMAU_NAME);

	/* Initialize the keys */
	swap_end(akey, akey_s, X_A_KLEN);
	swap_end(hkey, hkey_s, X_H_KLEN);

	key = irq_lock();
	/* Configure the smu */
	smau_disable_cache(smau_dev);
	smau_mem_init_extra(smau_dev);

	smau_config_aeskeys(smau_dev, win, X_A_KLEN, (u32_t *) akey_s);
	smau_config_authkeys(smau_dev, win, X_H_KLEN, (u32_t*) hkey_s);
#ifdef ALGO_HMAC
	smau_config_windows(smau_dev, win, wbase & ~(DEF_WINDOW_SIZE-1), ~(DEF_WINDOW_SIZE-1), hbase,
            1 /* spi_mapped */, 0 /* auth_bypass */, 1 /* enc_bypass */,
            0x8400 /* sha256 */, 1/* swap */);
#else
	smau_config_windows(smau_dev, win, wbase & ~(DEF_WINDOW_SIZE-1), ~(DEF_WINDOW_SIZE-1), hbase,
            1 /* spi_mapped */, 0 /* auth_bypass */, 0 /* enc_bypass */,
            0x8400 /* sha256 */, 1/* swap */);
#endif
	irq_unlock(key);
	return 0;
}

/* 16 MB size windows on Citadel */
#define SMAU_WINDOW_SIZE	0x1000000

__attribute__((__section__("text.fastcode")))
int xip_check_image(uint32_t addr)
{
    struct aai_scd_t scd;
    int32_t ret = 0;

    memcpy(&scd, (void *)addr, sizeof(scd));

    if (!((scd.magic0 == 0xfacefeed) && (scd.magic1 == 0xdeadbeaf))) {
        DEBUG_LOG("scd magic error: %x %x, no valid aai on flash\n", scd.magic0, scd.magic1);
        return -1;
    }

    //log_buf(&scd, X_SCD_SIZE);

    if (scd.xip) {
        DEBUG_LOG("scd xip: do xip \n");
        if (!(scd.valid)) {
            DEBUG_LOG("scd valid bit not assert\n");
            return -2;
        }
        ret = xip_get_sotpkey(xcb.a0key, xcb.h0key);
        if (ret)
            return -3;

    }
    // Workaround for flash address
    scd.faddr = addr & 0x00ffffff;

    return (scd.faddr + sizeof(struct aai_scd_t)) | (addr & 0xff000000);
}

__attribute__((__section__("text.fastcode")))
void xip_configure_smau(uint32_t addr)
{
    int win = 0; /* Configuring SMAU window 0 */
    struct aai_scd_t scd;

    memcpy(&scd, (void *)addr, sizeof(scd));

    if (scd.xip) {
        xip_recover_scd(&scd);
        xip_set_smau(scd.wbase | (FLASH_START + win*SMAU_WINDOW_SIZE),
		     scd.hbase | (FLASH_START + win*SMAU_WINDOW_SIZE),
		     scd.aeskey,
		     scd.hkey);
    }
}