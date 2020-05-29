/*
 * $Copyright Broadcom Corporation$
 *
 */
#ifndef __SECURE_XIP_H__
#define __SECURE_XIP_H__

/*
1. AES-CBC with HMAC_SHA256, AES key length 128bits
*/

#define X_A_KLEN		(16)
#define X_H_KLEN		(32)
#define X_A_BSIZE 		(256)
#define X_H_BSIZE 		(32)
#define X_IV_SIZE 		(16)
#define X_SCD_SIZE		(256)

#define X_FLH_SS		(0x10000)
#define X_A_BPS			(X_FLH_SS/X_A_BSIZE)
#define X_H_BPS 		(X_FLH_SS/X_H_BSIZE)

#define X_A_KLENW		(X_A_KLEN/4)
#define X_H_KLENW		(X_H_KLEN/4)
#define X_A_BSIZEW		(X_A_BSIZE/4)
#define X_H_BSIZEW		(X_H_BSIZE/4)
#define X_IV_SIZEW		(X_IV_SIZE/4)
#define X_SCD_SIZEW		(X_SCD_SIZE/4)

/* Default SMAU window size. This should be a power of 2 and
 * should be larger than the AAI image size
 */
#define DEF_WINDOW_SIZE		MB(8)

int xip_config(uint32_t *scd, uint32_t chunk_size);
int xip_prg_chunk(uint32_t *p);
int xip_check_image(uint32_t addr);
void xip_configure_smau(uint32_t addr);

#endif /* __SECURE_XIP_H__ */
