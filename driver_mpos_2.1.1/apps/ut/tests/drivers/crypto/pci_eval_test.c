/******************************************************************************
 *  Copyright (C) 2018 Broadcom. The term "Broadcom" refers to Broadcom Limited
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

/*
 * This file implements tests to run DES/3DES encryption/decryption
 * interactively. A sample of how to run these tests from the command line
 * is below.
 *	shell> test crypto_driver SetKey 0x1234567912345678
 *	Key set to
 *	=====
 *	00: 12 34 56 79 12 34 56 78
 *	08: 00 00 00 00 00 00 00 00
 *	10: 00 00 00 00 00 00 00 00
 *	shell> test crypto_driver PlainText 0x0 1234567887654321
 *	shell> test crypto_driver PlainText 0x8 1234567887654322
 *	shell> test crypto_driver Encrypt des
 *	Encrypted Text
 *	=====
 *	0000: dd 7d 5e 22 72 8e ec 3e 34 47 fc 76 d8 32 3b f9
 *
 *	shell> test crypto_driver EncText 0x0 dd7d5e22728eec3e
 *	shell> test crypto_driver EncText 0x8 3447fc76d8323bf9
 *	shell> test crypto_driver Decrypt des
 *	Plain Text
 *	=====
 *	0000: 12 34 56 78 87 65 43 21 12 34 56 78 87 65 43 22
 *
 *	shell> test crypto_driver SetIV 0xabcdef0987654321
 *	IV set to
 *	=====
 *	00: ab cd ef 09 87 65 43 21
 */

#include <board.h>
#include <device.h>
#include <crypto/crypto_selftest.h>
#include <crypto/crypto.h>
#include <crypto/crypto_smau.h>
#include <crypto/crypto_symmetric.h>
#include <broadcom/gpio.h>
#include <broadcom/dma.h>
#include <errno.h>
#include <test.h>

#define AON_GPIO_PIN	1

#define MAX_TEXT_SIZE	0x1000

#define ASCII_TO_HEX(A)	(((A) <= '9') ? (A) - '0' :			\
				        ((A) <= 'F') ? (A) - 'A' + 10 :	\
						       (A) - 'a' + 10)

#define MAX_KEY_SIZE	32
#define MAX_IV_SIZE	16

static u8_t cryptoHandle[CRYPTO_LIB_HANDLE_SIZE] = {0};
static u8_t *plain_text, *enc_text;
static u32_t text_size = 0;

static u8_t key[MAX_KEY_SIZE];
static u8_t iv[MAX_IV_SIZE];
static bool iv_set = false;

static int setup_gpio(void)
{
	int ret;
	struct device *gpio_dev;

	gpio_dev = device_get_binding(CONFIG_GPIO_AON_DEV_NAME);
	zassert_not_null(gpio_dev, NULL);

	ret = gpio_pin_configure(gpio_dev, AON_GPIO_PIN, GPIO_DIR_OUT);
	zassert_true(ret == 0, "GPIO config failed!");

	ret = gpio_pin_write(gpio_dev, AON_GPIO_PIN, 0);
	zassert_true(ret == 0, "GPIO write failed!");

	return 0;
}

static int init(void)
{
	int ret;
	static bool init_done = false;

	if (init_done)
		return 0;

	crypto_init((crypto_lib_handle *)&cryptoHandle);
	ret = setup_gpio();
	if (ret)
		return ret;

	plain_text = (u8_t *)cache_line_aligned_alloc(MAX_TEXT_SIZE*2);
	if (plain_text == NULL)
		return -ENOMEM;

	enc_text = plain_text + MAX_TEXT_SIZE;

	init_done = true;

	return 0;
}

static void print_enc_text(void)
{
	u32_t i;

	TC_PRINT("Encrypted Text\n=====\n");
	for (i = 0; i < text_size; i++) {
		if ((i & 0xF) == 0)
			TC_PRINT("%04X:", i);
		TC_PRINT(" %02x", enc_text[i]);
		if ((i & 0xF) == 0xF)
			TC_PRINT("\n");
	}
	TC_PRINT("\n");
}

static void print_plain_text(void)
{
	u32_t i;

	TC_PRINT("Plain Text\n=====\n");
	for (i = 0; i < text_size; i++) {
		if ((i & 0xF) == 0)
			TC_PRINT("%04X:", i);
		TC_PRINT(" %02x", plain_text[i]);
		if ((i & 0xF) == 0xF)
			TC_PRINT("\n");
	}
	TC_PRINT("\n");
}

static int set_gpio(u8_t val)
{
	int ret;
	struct device *gpio_dev;

	gpio_dev = device_get_binding(CONFIG_GPIO_AON_DEV_NAME);
	zassert_not_null(gpio_dev, NULL);

	ret = gpio_pin_write(gpio_dev, AON_GPIO_PIN, val);
	zassert_true(ret == 0, "GPIO write failed!");

	return 0;
}

static int set_key(char *key_str)
{
	u32_t i, j, len, offset;
	memset(key, 0, sizeof(key));

	if ((key_str[0] == '0') && (key_str[1] == 'x'))
		key_str += 2;

	offset = 0;
	/* Handle odd nibbles */
	if (strlen(key_str) & 0x1) {
		key[0] = ASCII_TO_HEX(key_str[0]);
		offset = 1;
		key_str++;
	}

	len = min(MAX_KEY_SIZE - offset, strlen(key_str)/2);
	for (i = 0; i < len; i++) {
		key[offset + i] = (ASCII_TO_HEX(key_str[2*i]) << 4) +
				   ASCII_TO_HEX(key_str[2*i + 1]);
	}

	TC_PRINT("Key set to\n=====\n");
	for (i = 0; i < 4; i++) {
		TC_PRINT("%02X:", i*8);
		for (j = 0; j < 8; j++) {
			TC_PRINT(" %02x", key[i*8 + j]);
		}

		TC_PRINT("\n");
	}

	return 0;
}

static int set_text(char *start_addr, char *text, bool plain)
{
	u8_t *p;
	u32_t i, len, offset, start;

	if ((text[0] == '0') && (text[1] == 'x'))
		text += 2;

	if ((start_addr[0] == '0') && (start_addr[1] == 'x'))
		start_addr += 2;

	p = plain ? plain_text : enc_text;

	start = strtoul(start_addr, NULL, 16);

	offset = 0;
	/* Handle odd nibbles */
	if (strlen(text) & 0x1) {
		p[start] = ASCII_TO_HEX(text[0]);
		offset = 1;
		text++;
	}

	len = min(32 - offset, strlen(text)/2);
	for (i = 0; i < len; i++) {
		p[start + offset + i] = (ASCII_TO_HEX(text[2*i]) << 4) +
					 ASCII_TO_HEX(text[2*i + 1]);
	}

	text_size = start + offset + len;

	return 0;
}

static int set_iv(char *iv_str)
{
	u32_t i, len, offset;
	memset(iv, 0, sizeof(iv));

	if ((iv_str[0] == '0') && (iv_str[1] == 'x'))
		iv_str += 2;

	offset = 0;
	/* Handle odd nibbles */
	if (strlen(iv_str) & 0x1) {
		iv[0] = ASCII_TO_HEX(iv_str[0]);
		offset = 1;
		iv_str++;
	}

	len = min(MAX_IV_SIZE - offset, strlen(iv_str)/2);
	for (i = 0; i < len; i++) {
		iv[offset + i] = (ASCII_TO_HEX(iv_str[2*i]) << 4) +
				  ASCII_TO_HEX(iv_str[2*i + 1]);
	}

	TC_PRINT("IV set to\n=====\n");
	TC_PRINT("%02X:", 0);
	for (i = 0; i < 16; i++)
		TC_PRINT(" %02x", iv[i]);
	TC_PRINT("\n");

	iv_set = true;
	return 0;
}

static int encrypt_des(void)
{
	int ret;

	set_gpio(1);
	ret = crypto_symmetric_des_encrypt(
			(crypto_lib_handle *)cryptoHandle,
			BCM_SCAPI_ENCR_ALG_DES,
			key, 8,
			plain_text, text_size,
			iv_set ? iv : NULL, 8, enc_text);
	set_gpio(0);

	iv_set = false;

	if (ret) {
		TC_ERROR("3DES Encrypt failed: %d\n", ret);
		return ret;
	}

	print_enc_text();

	return 0;
}

static int encrypt_3des(void)
{
	int ret;

	set_gpio(1);
	ret = crypto_symmetric_des_encrypt(
			(crypto_lib_handle *)cryptoHandle,
			BCM_SCAPI_ENCR_ALG_3DES,
			key, 24,
			plain_text, text_size,
			iv_set ? iv : NULL, 8, enc_text);
	set_gpio(0);

	iv_set = false;

	if (ret) {
		TC_ERROR("3DES Encrypt failed: %d\n", ret);
		return ret;
	}

	print_enc_text();

	return 0;
}

static int decrypt_des(void)
{
	int ret;

	set_gpio(1);
	ret = crypto_symmetric_des_decrypt(
			(crypto_lib_handle *)cryptoHandle,
			BCM_SCAPI_ENCR_ALG_DES,
			key, 8,
			enc_text, text_size,
			iv_set ? iv : NULL, 8, plain_text);
	set_gpio(0);

	iv_set = false;

	if (ret) {
		TC_ERROR("3DES Encrypt failed: %d\n", ret);
		return ret;
	}

	print_plain_text();

	return 0;
}

static int decrypt_3des(void)
{
	int ret;

	set_gpio(1);
	ret = crypto_symmetric_des_decrypt(
			(crypto_lib_handle *)cryptoHandle,
			BCM_SCAPI_ENCR_ALG_3DES,
			key, 24,
			enc_text, text_size,
			iv_set ? iv : NULL, 8, plain_text);
	set_gpio(0);

	iv_set = false;

	if (ret) {
		TC_ERROR("3DES Encrypt failed: %d\n", ret);
		return ret;
	}

	print_plain_text();

	return 0;
}

static int encrypt_aes(BCM_SCAPI_ENCR_ALG algo)
{
	int ret;

	set_gpio(1);
	ret = crypto_symmetric_fips_aes(
			(crypto_lib_handle *)cryptoHandle,
			iv_set ? BCM_SCAPI_ENCR_MODE_CBC :
				 BCM_SCAPI_ENCR_MODE_ECB,
			algo,
			BCM_SCAPI_CIPHER_MODE_ENCRYPT,
			key, iv, plain_text, text_size, text_size, 0, enc_text);
	set_gpio(0);

	iv_set = false;

	if (ret) {
		TC_ERROR("AES Encrypt failed: %d\n", ret);
		return ret;
	}

	print_enc_text();

	return 0;
}

static int decrypt_aes(BCM_SCAPI_ENCR_ALG algo)
{
	int ret;

	set_gpio(1);
	ret = crypto_symmetric_fips_aes(
			(crypto_lib_handle *)cryptoHandle,
			iv_set ? BCM_SCAPI_ENCR_MODE_CBC :
				 BCM_SCAPI_ENCR_MODE_ECB,
			algo,
			BCM_SCAPI_CIPHER_MODE_DECRYPT,
			key, iv, plain_text, text_size, text_size, 0, enc_text);
	set_gpio(0);

	iv_set = false;

	if (ret) {
		TC_ERROR("AES Encrypt failed: %d\n", ret);
		return ret;
	}

	print_plain_text();

	return 0;

}

SHELL_TEST_DECLARE(pci_eval_test)
{
	int ret;

	ret = init();
	if (ret)
		return ret;

	if (strcmp("SetKey", argv[1]) == 0) {
		if (argc == 3)
			return set_key(argv[2]);
		TC_PRINT("Usage: test crypto_driver SetKey <key>\n");
		return -1;
	} else if (strcmp("PlainText", argv[1]) == 0) {
		if (argc == 4)
			return set_text(argv[2], argv[3], true);
		TC_PRINT("Usage: test crypto_driver PlainText "
			 "<offset> <Upto 32 bytes text>\n");
		return -1;
	} else if (strcmp("EncText", argv[1]) == 0) {
		if (argc == 4)
			return set_text(argv[2], argv[3], false);
		TC_PRINT("Usage: test crypto_driver EncText "
			 "<offset> <Upto 32 bytes text>\n");
		return -1;
	} else if (strcmp("SetIV", argv[1]) == 0) {
		if (argc == 3)
			return set_iv(argv[2]);
		TC_PRINT("Usage: test crypto_driver SetIV <IV>\n");
		return -1;
	} else if (strcmp("Encrypt", argv[1]) == 0) {
		if (argc == 3) {
			if (strcmp("des", argv[2]) == 0)
				return encrypt_des();
			else if (strcmp("3des", argv[2]) == 0)
				return encrypt_3des();
			else if (strcmp("aes", argv[2]) == 0)
				return encrypt_aes(BCM_SCAPI_ENCR_ALG_AES_128);
			else if (strcmp("aes128", argv[2]) == 0)
				return encrypt_aes(BCM_SCAPI_ENCR_ALG_AES_128);
			else if (strcmp("aes192", argv[2]) == 0)
				return encrypt_aes(BCM_SCAPI_ENCR_ALG_AES_192);
			else if (strcmp("aes256", argv[2]) == 0)
				return encrypt_aes(BCM_SCAPI_ENCR_ALG_AES_256);
		}
		TC_PRINT("Usage: test crypto_driver Encrypt "
			 "<aes|aes192|aes256|des|3des>\n");
		return -1;
	} else if (strcmp("Decrypt", argv[1]) == 0) {
		if (argc == 3) {
			if (strcmp("des", argv[2]) == 0)
				return decrypt_des();
			else if (strcmp("3des", argv[2]) == 0)
				return decrypt_3des();
			else if (strcmp("aes", argv[2]) == 0)
				return decrypt_aes(BCM_SCAPI_ENCR_ALG_AES_128);
			else if (strcmp("aes128", argv[2]) == 0)
				return decrypt_aes(BCM_SCAPI_ENCR_ALG_AES_128);
			else if (strcmp("aes192", argv[2]) == 0)
				return decrypt_aes(BCM_SCAPI_ENCR_ALG_AES_192);
			else if (strcmp("aes256", argv[2]) == 0)
				return decrypt_aes(BCM_SCAPI_ENCR_ALG_AES_256);

		}
		TC_PRINT("Usage: test crypto_driver Decrypt "
			 "<aes|aes192|aes256|des|3des>\n");
		return -1;
	}

	TC_ERROR("Unknown command : %s\n", argv[1]);
	return -1;
}