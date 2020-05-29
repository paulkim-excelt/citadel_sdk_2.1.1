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

#include <post_log.h>
#include <utils.h>
#include <string.h>

#include <sbi.h>
#include <iproc_sw.h>
#include <regs.h>
#include <crypto/crypto.h>
#include <crypto/crypto_smau.h>
#include <crypto/crypto_symmetric.h>
#include <reg_access.h>
#include <sotp.h>
#include <customization_key.h>

#include <pka/q_lip.h>
#include <pka/q_lip_aux.h>
#include <pka/q_pka_hw.h>
#include <pka/q_elliptic.h>
#include <pka/q_rsa.h>
#include <pka/q_rsa_4k.h>
#include <pka/q_dsa.h>
#include <pka/q_dh.h>
#include <pka/q_lip_utils.h>

#ifndef RSA_2048_KEY_BYTES
#define RSA_2048_KEY_BYTES (2048 >> 3)
#endif

#ifndef RSA_4096_KEY_BYTES
#define RSA_4096_KEY_BYTES (4096 >> 3)
#endif

#define RSA4K_PLUS_MONT_CTX_SIZE 2088 // For the RSA 4K keys, the total key size is 512 + 1576 = 2088 bytes.
#define EC_P256_KEY_BYTES 64

sotp_bcm58202_status_t sotp_set_customer_config(
        uint16_t CustomerRevisionID, uint16_t SBIRevisionID);
sotp_bcm58202_status_t sotp_set_config_abprod(void);
sotp_bcm58202_status_t sotp_set_sbl_config(u32_t SBLConfig);
sotp_bcm58202_status_t sotp_set_customer_id (u32_t customerID);

/* Required for qlip library usage */
q_status_t YieldHandler(void);

#define QLIP_HEAP_SIZE (1024 * 16) /* Heapsize is in Words */
#define QLIP_CONTEXT &qlip_ctx
#define OS_YIELD_FUNCTION YieldHandler

#define LOCAL_QLIP_CONTEXT_DEFINITION \
q_lip_ctx_t qlip_ctx; \
u32_t qlip_heap[QLIP_HEAP_SIZE]; \
q_ctx_init(&qlip_ctx,&qlip_heap[0],QLIP_HEAP_SIZE,OS_YIELD_FUNCTION);

static IPROC_STATUS addMontContext(const uint8_t *pkeyPtr,
                                   int *index,
                                   uint8_t* DAUTHPtr);


static IPROC_STATUS createDAUTHblob(const uint8_t* publicKeyModulusPtr, const uint32_t publicKeyModulusLen,
                                    const uint32_t CustomerID, const uint16_t ProductID, const uint16_t BRCMRevisionID,
                                    uint8_t* DAUTHPtr, uint32_t DAUTHLen);


/* Include the HMAC key here */
#ifdef KHMAC_SHA_BIN_FILE
extern uint8_t HmacKey;
__asm__ (
        "HmacKey:\n"
        "   .incbin \"" KHMAC_SHA_BIN_FILE "\"\n"
    );
#endif

void err_handle(char *format, IPROC_STATUS nErrStatus)
{
    post_log(format, nErrStatus);
    //resetIPROC();
}

extern int encryption;
/* RSA2048 public key modulus - 256 bytes */
extern uint8_t RSA2KpubKeyModulus[256];
/* RSA4096 public key modulus - 512 bytes */
extern uint8_t RSA4KpubKeyModulus[512];
/* ECDSA P256 public key - 64 bytes */
extern uint8_t ECDSApubKey[64];
extern uint32_t CustomerID;
extern uint32_t CustomerSBLConfiguration;
extern uint16_t CustomerRevisionID;
extern uint16_t SBIRevisionID;

#ifdef MPOS_CONVERT
int single_step_convert()
{
    IPROC_STATUS status;
    sotp_bcm58202_status_t sotp_status;

    uint32_t RSA2KDAUTHLen = RSA_2048_KEY_BYTES + 3*sizeof(uint32_t);
    uint8_t RSA2KDAUTHBlob[RSA_2048_KEY_BYTES + 3*sizeof(uint32_t)];

    uint32_t RSA4KDAUTHLen = RSA4K_PLUS_MONT_CTX_SIZE + 3*sizeof(uint32_t);
    uint8_t RSA4KDAUTHBlob[RSA4K_PLUS_MONT_CTX_SIZE + 3*sizeof(uint32_t)];

    uint32_t ECDSADAUTHLen = EC_P256_KEY_BYTES + 3*sizeof(uint32_t);
    uint8_t ECDSADAUTHBlob[EC_P256_KEY_BYTES + 3*sizeof(uint32_t)];

    uint8_t cryptoHandle[CRYPTO_LIB_HANDLE_SIZE]={0};
    uint8_t DAUTHHash[SHA256_HASH_SIZE];

    struct sotp_bcm58202_dev_config devConfig = {0};


    u32_t    cID;
    uint8_t     Key[SHA256_HASH_SIZE];
    uint16_t    len;
    uint16_t cRevID, sRevID;
#ifdef  SBI_DEBUG
    uint32_t i;
#endif
#ifdef SBI_DEBUG
    post_log("In Citadel Customization from Unassigned Mode\n");
#endif // SBI_DEBUG

    sotp_status = sotp_read_dev_config(&devConfig);
    if(sotp_status != IPROC_OTP_VALID)
    {
        err_handle("readDevCfg() failed [%d]\n", sotp_status);
        return 1;
    }

#ifdef SBI_DEBUG
    post_log("devConfig.BRCMRevisionID is %08x\n", devConfig.BRCMRevisionID);
    post_log("devConfig.ProductID is %08x\n", devConfig.ProductID);
    post_log("devConfig.devSecurityConfig is %08x\n", devConfig.devSecurityConfig);
#endif // SBI_DEBUG

    /*
     * First, check that private sections - KHMAC, KAES, CREV (Customer
     * Revision ID) and SBI_REV (SBI Revision ID) - have not been programmed
     */
    sotp_status = sotp_read_key (&Key[0], &len, OTPKey_HMAC_SHA256);
    if (sotp_status != IPROC_OTP_INVALID)
    {
        post_log("KHMAC is already programmed - abort customization\n");
        //resetIPROC();
        return 1;
    }

    sotp_status = sotp_read_key (&Key[0], &len, OTPKey_AES);
    if (sotp_status != IPROC_OTP_INVALID)
    {
        post_log("KAES is already programmed - abort customization\n");
        //resetIPROC();
        return 1;
    }

    sotp_status = sotp_read_customer_config(&cRevID, &sRevID);
    if ((sotp_status != IPROC_OTP_VALID) || (cRevID != 0) || (sRevID != 0))
    {
        post_log("CustomerConfig is already programmed - abort customization\n");
        //resetIPROC();
        return 1;
    }

    /*
     * Then check that the critical sections - Customer ID, DAUTH - have not been programmed
     */
    sotp_status = sotp_read_customer_id(&cID);
    if ((sotp_status != IPROC_OTP_VALID) || (cID != 0))
    {
        post_log("CustomerID is already programmed - abort customization\n");
        //resetIPROC();
        return 1;
    }

    sotp_status = sotp_read_key (&Key[0], &len, OTPKey_DAUTH);
    if (sotp_status != IPROC_OTP_INVALID)
    {
        post_log("DAUTH is already programmed - abort customization\n");
        //resetIPROC();
        return 1;
    }

    /* Validate Customer ID */
    if ( ((CustomerID & CID_TYPE_MASK) != CID_TYPE_DEV) && ((CustomerID & CID_TYPE_MASK) != CID_TYPE_PROD) )
    {
        post_log("Invalid CustomerID [%08lx]\n", CustomerID);
        //resetIPROC();
        return 1;
    }

    /* Construct the DAUTH */
    if (encryption == ENCRYPT_RSA2048)
    {
        status = createDAUTHblob(&RSA2KpubKeyModulus[0], RSA_2048_KEY_BYTES,
                     CustomerID, devConfig.ProductID, devConfig.BRCMRevisionID,
                     &RSA2KDAUTHBlob[0], RSA2KDAUTHLen);
    }
    else if (encryption == ENCRYPT_RSA4096)
    {
        status = createDAUTHblob(&RSA4KpubKeyModulus[0], RSA_4096_KEY_BYTES,
                     CustomerID, devConfig.ProductID, devConfig.BRCMRevisionID,
                     &RSA4KDAUTHBlob[0], RSA4KDAUTHLen);
    }
    else if (encryption == ENCRYPT_EC_P256)
    {
        status = createDAUTHblob(&ECDSApubKey[0], EC_P256_KEY_BYTES,
                     CustomerID, devConfig.ProductID, devConfig.BRCMRevisionID,
                     &ECDSADAUTHBlob[0], ECDSADAUTHLen);
    }
    else
        status = IPROC_STATUS_FAIL;


    if (status != IPROC_STATUS_OK)
    {
        err_handle("createDAUTHblob() failed [%d]\n", status);
        return 1;
    }

#ifdef SBI_DEBUG
    if (encryption == ENCRYPT_RSA2048)
    {
        for(i=0; i<RSA2KDAUTHLen; i+=4)
        {
            post_log("RSA2KDAUTHBlob : 0x%02x 0x%02x 0x%02x 0x%02x \n", RSA2KDAUTHBlob[i], RSA2KDAUTHBlob[i+1], RSA2KDAUTHBlob[i+2], RSA2KDAUTHBlob[i+3]);
        }
    }
    else if (encryption == ENCRYPT_RSA4096)
    {
        for(i=0; i<RSA4KDAUTHLen; i+=4)
        {
            post_log("RSA4KDAUTHBlob : 0x%02x 0x%02x 0x%02x 0x%02x \n", RSA4KDAUTHBlob[i], RSA4KDAUTHBlob[i+1], RSA4KDAUTHBlob[i+2], RSA4KDAUTHBlob[i+3]);
        }
    }
    else if (encryption == ENCRYPT_EC_P256)
    {
        for(i=0; i<ECDSADAUTHLen; i+=4)
        {
            post_log("ECDSADAUTHBlob : 0x%02x 0x%02x 0x%02x 0x%02x \n", ECDSADAUTHBlob[i], ECDSADAUTHBlob[i+1], ECDSADAUTHBlob[i+2], ECDSADAUTHBlob[i+3]);
        }
    }
#endif // SBI_DEBUG

    crypto_init((crypto_lib_handle *)&cryptoHandle);

    /* Get hash of DAUTHblob and write it to SOTP */
    if (encryption == ENCRYPT_RSA2048)
    {
        status = crypto_symmetric_hmac_sha256((crypto_lib_handle *)&cryptoHandle, &RSA2KDAUTHBlob[0], RSA2KDAUTHLen, NULL, 0, &DAUTHHash[0], TRUE);
    }
    else if (encryption == ENCRYPT_RSA4096)
    {
        status = crypto_symmetric_hmac_sha256((crypto_lib_handle *)&cryptoHandle, &RSA4KDAUTHBlob[0], RSA4KDAUTHLen, NULL, 0, &DAUTHHash[0], TRUE);
    }
    else if (encryption == ENCRYPT_EC_P256)
    {
        status = crypto_symmetric_hmac_sha256((crypto_lib_handle *)&cryptoHandle, &ECDSADAUTHBlob[0], ECDSADAUTHLen, NULL, 0, &DAUTHHash[0], TRUE);
    }

    if (status != IPROC_STATUS_OK)
    {
        err_handle("crypto_symmetric_hmac_sha256() failed [%d]\n", status);
        return 1;
    }

#ifdef  SBI_DEBUG
    for(i=0; i< SHA256_HASH_SIZE; i+=4)
    {
        post_log("DAUTHHash : 0x%02x 0x%02x 0x%02x 0x%02x \n", DAUTHHash[i], DAUTHHash[i+1], DAUTHHash[i+2], DAUTHHash[i+3]);
    }

#endif // SBI_DEBUG

    /* allow read/write access to all SOTP sections */
    reg32setbit(SOTP_REGS_CHIP_CTRL, SOTP_REGS_CHIP_CTRL__SW_OVERRIDE_CHIP_STATES);
    reg32setbit(SOTP_REGS_CHIP_CTRL, SOTP_REGS_CHIP_CTRL__SW_MANU_PROG);
    reg32setbit(SOTP_REGS_CHIP_CTRL, SOTP_REGS_CHIP_CTRL__SW_CID_PROG);
    reg32setbit(SOTP_REGS_CHIP_CTRL, SOTP_REGS_CHIP_CTRL__SW_AB_DEVICE);

    /* Program DAUTH value and program section 2 to lock DAUTH for write access */
    sotp_status = sotp_set_key (&DAUTHHash[0], SHA256_HASH_SIZE, OTPKey_DAUTH);
    if(sotp_status != IPROC_OTP_VALID)
    {
        err_handle("sotp_set_key() DAUTH failed [%d]\n", sotp_status);
        return 1;
    }

    /* Program HMAC SHA256 key */
#ifdef KHMAC_SHA_BIN_FILE
    sotp_status = sotp_set_key (&HmacKey, SHA256_HASH_SIZE, OTPKey_HMAC_SHA256);
    if(sotp_status != IPROC_OTP_VALID)
    {
        err_handle("sotp_set_key() HMAC SHA256 failed [%d]\n", sotp_status);
        return 1;
    }
#endif

#ifdef SBI_DEBUG
    {
        /* Test - Read back data to make sure it works correctly */
        uint8_t DAUTHHashRead[SHA256_HASH_SIZE];
        uint16_t len;
        sotp_status = sotp_read_key (&DAUTHHashRead[0], &len, OTPKey_DAUTH);
        post_log("sotp_read_key() DAUTH returns %d\n", sotp_status);
        /*
         * Printing DAUTH values doesn't give the correct values unless
         * the chip is reset, hence eliminating the code to print DAUTH
         */

    }
#endif // SBI_DEBUG

    /* Program CustomerID value and program section 2 to lock CustomerID for write access */
    sotp_status = sotp_set_customer_id (CustomerID);
    if (sotp_status != IPROC_OTP_VALID)
    {
        err_handle("sotp_set_customer_id() failed [%d]\n", sotp_status);
        return 1;
    }
#ifdef SBI_DEBUG
    {
        /* Test - Read back data to make sure it works correctly */
        u32_t cID;
        sotp_status = sotp_read_customer_id(&cID);
        post_log("sotp_read_customer_id() returns %d: cID = %08x\n", sotp_status, cID);
    }
#endif //SBI_DEBUG

    /* Program Section 3 to update device status */
    if ((CustomerID & CID_TYPE_MASK) == CID_TYPE_PROD)
    {
#ifdef SBI_DEBUG
        post_log("*** Production part customization ***\n");
#endif
        sotp_status = sotp_set_config_abprod();
        if (sotp_status != IPROC_OTP_VALID)
        {
            err_handle("sotp_set_config_abprod() failed [%d]\n", sotp_status);
            return 1;
        }
    }

    /* Programming Section 5 */
#ifdef SBI_DEBUG
    post_log("CustomerSBLConfiguration: %08lx\n", CustomerSBLConfiguration);
#endif //SBI_DEBUG

    /* Write SBLConfiguration to SOTP */
    sotp_status = sotp_set_sbl_config (CustomerSBLConfiguration);
    if (sotp_status != IPROC_OTP_VALID)
    {
        err_handle("sotp_set_sbl_configuration() failed [%d]\n", sotp_status);
        return 1;
    }

#ifdef SBI_DEBUG
    {
        /* Test - Read back data to make sure it works correctly */
        u32_t cSBLConfiguration;
        sotp_status = sotp_read_sbl_config(&cSBLConfiguration);
        post_log("sotp_read_sbl_configuration() returns %d: cSBLConfiguration = %08x\n", sotp_status, cSBLConfiguration);
    }
#endif //SBI_DEBUG

    /* Programming Section 6 - Write CustomerConfig to SOTP */
    sotp_status = sotp_set_customer_config (CustomerRevisionID, SBIRevisionID);
    if (sotp_status != IPROC_OTP_VALID)
    {
        err_handle("sotp_set_customer_config() failed [%d]\n", sotp_status);
        return 1;
    }

#ifdef SBI_DEBUG
    {
        /* Test - Read back data to make sure it works correctly */
        uint16_t cRevID, sRevID;
        sotp_status = sotp_read_customer_config(&cRevID, &sRevID);
        post_log("sotp_read_customer_config() returns %d: cRevID = %04x, sRevID = %04x\n", sotp_status, cRevID, sRevID);
    }
#endif //SBI_DEBUG

    post_log("*** Customization Successful ***\n");
    //resetIPROC();

    return 0;
}
#endif /* MPOS_CONVERT */

static IPROC_STATUS createDAUTHblob(const uint8_t* publicKeyPtr, const uint32_t publicKeyLen,
                                    const uint32_t CustomerID, const uint16_t ProductID, const uint16_t BRCMRevisionID,
                                    uint8_t* DAUTHPtr, uint32_t DAUTHLen)
{
    int index = 0;
    uint16_t KeyType;
    uint16_t Length;
    IPROC_STATUS status = IPROC_STATUS_OK;

    /* Determine key type based on length of public key */
    if (publicKeyLen == RSA_2048_KEY_BYTES)
    {
        KeyType = IPROC_SBI_RSA2048;
        Length = 3*sizeof(uint32_t) + RSA_2048_KEY_BYTES;
    }
    else if (publicKeyLen == RSA_4096_KEY_BYTES)
    {
        KeyType = IPROC_SBI_RSA4096;
        Length = 3*sizeof(uint32_t) + RSA4K_PLUS_MONT_CTX_SIZE;
    }
    else if (publicKeyLen == EC_P256_KEY_BYTES)
    {
        KeyType = IPROC_SBI_EC_P256;
        Length = 3*sizeof(uint32_t) + EC_P256_KEY_BYTES;
    }
    else
        return IPROC_STATUS_FAIL;

    if(Length < DAUTHLen)
        return IPROC_STATUS_FAIL;

    /* Construct DAUTH */
    *(uint16_t *)(DAUTHPtr) = (uint16_t)KeyType;
    index += sizeof(uint16_t);
    *(uint16_t *)(DAUTHPtr + index) = (uint16_t)Length;
    index += sizeof(uint16_t);
    memcpy((DAUTHPtr + index), publicKeyPtr, publicKeyLen);
    index += publicKeyLen;

    /* If using RSA4096, add montgomery multiplication for speed */
    if (publicKeyLen == RSA_4096_KEY_BYTES)
    {
        status = addMontContext(publicKeyPtr, &index, DAUTHPtr);
        if (status != IPROC_STATUS_OK)
            return IPROC_STATUS_FAIL;
    }

    *(uint32_t *)(DAUTHPtr + index) = CustomerID;
    index += sizeof(uint32_t);
    *(uint16_t *)(DAUTHPtr + index) = ProductID;
    index += sizeof(uint16_t);
    *(uint16_t *)(DAUTHPtr + index) = BRCMRevisionID;
    index += sizeof(uint16_t);

    return IPROC_STATUS_OK;
}

static int writeMontEntry(q_lint_t *lint, uint8_t* DAUTHPtr)
{
    int index = 0;
    int i;
    int length = index;

    *(uint32_t *)(DAUTHPtr + index) = lint->size;
    index += sizeof(uint32_t);

    for (i = 0;i < lint->size;i++) {
        *(uint32_t *)(DAUTHPtr + index) = lint->limb[i];
        index += sizeof(uint32_t);
    }

    *(uint32_t *)(DAUTHPtr + index) = lint->alloc;
    index += sizeof(uint32_t);
    *(uint32_t *)(DAUTHPtr + index) = lint->neg;
    index += sizeof(uint32_t);

    return (index - length);
}

static IPROC_STATUS addMontContext(const uint8_t *pkeyPtr, int *index, uint8_t* DAUTHPtr)
{
    q_lint_t n;
    q_mont_t mont;
    IPROC_STATUS status;
    int length;

    LOCAL_QLIP_CONTEXT_DEFINITION;

    status  = q_init (&qlip_ctx, &n, RSA_4096_KEY_BYTES/2);
    status += q_init (&qlip_ctx, &mont.n, n.alloc);
    status += q_init (&qlip_ctx, &mont.np, (n.alloc+1));
    status += q_init (&qlip_ctx, &mont.rr, (n.alloc+1));

    if (status != Q_SUCCESS )
    {
        post_log("\r\n ERROR:     q_init failed ......\r\n ");
        return IPROC_STATUS_FAIL;
    }

    n.size = RSA_4096_KEY_BYTES/4;
    n.alloc = 0x100;
    n.neg = 0;
    mont.br = 0;

    bcm_int2bignum((uint32_t*)pkeyPtr, RSA_4096_KEY_BYTES);
    memcpy( n.limb, pkeyPtr, RSA_4096_KEY_BYTES);

    status = q_mont_init_sw (&qlip_ctx, &mont, &n);

    if( status != Q_SUCCESS )
    {
        post_log("\r\n ERROR:    q_mont_init_sw failed ......\r\n ");
        return IPROC_STATUS_FAIL;
    }

    length =  sizeof(mont.n.size) + sizeof(mont.n.alloc) + sizeof(mont.n.neg) + mont.n.size*4 + \
        sizeof(mont.np.size) + sizeof(mont.np.alloc) + sizeof(mont.np.neg) + mont.np.size*4 + \
        sizeof(mont.rr.size) + sizeof(mont.rr.alloc) + sizeof(mont.rr.neg) + mont.rr.size*4 + \
        sizeof(mont.br);

    /*
     * Write n, np and rr to DAUTH blob
     */
    length = writeMontEntry(&mont.n, (DAUTHPtr + *index));
    if (length < 0)
    {
        post_log("\nERROR: Montgomery Context \"n\" write error.\n");
        return IPROC_STATUS_FAIL;
    }
    *index += length;

    length = writeMontEntry(&mont.np, (DAUTHPtr + *index));
    if (length < 0)
    {
        post_log("\nERROR: Montgomery Context \"np\" write error.\n");
        return IPROC_STATUS_FAIL;
    }
    *index += length;

    length = writeMontEntry(&mont.rr, (DAUTHPtr + *index));
    if (length < 0)
    {
        post_log("\nERROR: Montgomery Context \"rr\" write error.\n");
        return IPROC_STATUS_FAIL;
    }
    *index += length;

    /*
     * Secimage does not write this field so set to zero
     */
    *(uint32_t *)(DAUTHPtr + *index) = 0;
    *index += sizeof(uint32_t);

    if (*index > (int)(RSA4K_PLUS_MONT_CTX_SIZE + sizeof(uint32_t)))
    {
        post_log("\nERROR: Montgomery Context overflow\n");
        return IPROC_STATUS_FAIL;
    }

    return IPROC_STATUS_OK;
}

/* Sotp Wrapper APIs */
int check_row_12(uint64_t result)
{
    /*   
     * Row 12
     *
     * Bits 0->21 Pairs
     * Bits 32->40 - Single bits fails
     */
    if (bit_pair_count(result, 0, 21) != 0)
        return 1;
    if (result & 0x1FF00000000LL)
        return 1;

    return 0;
}

int check_row_13(uint64_t result)
{
    /*   
     * Row 13
     *
     * Bits 0->22 Single bit fails
     * Bits 23->40 - Pairs
     */
    if (bit_pair_count(result, 23, 40) != 0)
        return 1;
    if (result & 0x7FFFFFLL)
        return 1;

    return 0;
}

sotp_bcm58202_status_t sotp_set_customer_id (u32_t customerID)
{
//Section 5: Region 5: Rows 20 to 23
//Fail bits, ECC bits, Row 23: CRC
    uint64_t write_result;

    write_result = sotp_mem_write(20,1,customerID);
    if (bit_count(write_result) >= 2)
        return IPROC_OTP_INVALID;

    return IPROC_OTP_VALID;
}

sotp_bcm58202_status_t sotp_set_sbl_config(u32_t SBLConfig)
{
    uint8_t data[12];
    uint64_t row_data;
    uint32_t crc32 , wr_lock;
    uint64_t write_result;

    //Row 21 : If programmed and ECC Error not detected, 
    //this row contains Customer SBLConfiguration 
    write_result = sotp_mem_write(21,1,SBLConfig);
    if (bit_count(write_result) >= 2)
        return IPROC_OTP_INVALID;
    
    row_data = sotp_mem_read(20,1);
    *(uint32_t *)data = row_data & 0xFFFFFFFF;

    row_data = sotp_mem_read(21,1);
    *(uint32_t *)&data[4] = row_data & 0xFFFFFFFF;
        
    row_data = sotp_mem_read(22,1);
    *(uint32_t *)&data[8] = row_data & 0xFFFFFFFF;
        
    crc32 = calc_crc32(0xFFFFFFFF,data,12);
    write_result = sotp_mem_write(23,1,crc32);
    if (bit_count(write_result) >= 2)
        return IPROC_OTP_INVALID;
            
    wr_lock =cpu_rd_single(SOTP_REGS_OTP_WR_LOCK,4);
    wr_lock = wr_lock | 0x20 ;
    cpu_wr_single(SOTP_REGS_OTP_WR_LOCK,wr_lock,4);

    return IPROC_OTP_VALID;
}

sotp_bcm58202_status_t sotp_set_customer_config(
        uint16_t CustomerRevisionID, uint16_t SBIRevisionID)
{
//Section 6: Region 6: Rows 24 to 27
//Fail bits, No ECC Error check 
//No CRC, Only Redundancy

    uint64_t configdata,rowdata;
    uint32_t row = 24;
    uint32_t i=0;
    uint64_t write_result;
    uint32_t results[4];
    int j;
    int e;

    configdata = CustomerRevisionID & 0xFFFF;
    configdata = (configdata << 16) | (SBIRevisionID & 0xFFFF);

    //Write customerConfig to all valid rows in Section 6
    do{
        write_result = sotp_mem_write(row,0,configdata);
        results[i] = (uint32_t)write_result;
        i++;
        row++;
    } while(row < 27);

    /* Check for more than one error in each bit column */
    for (i = 0;i < 32;i++) {
        e = 0;
        for (j = 0;j < 3;j++) {
            if ((results[j] >> i) & 0x1)
                e++;
        }

        if (e > 1)
            return IPROC_OTP_INVALID;
    }

    return IPROC_OTP_VALID;
}

sotp_bcm58202_status_t sotp_set_config_abprod(void)
{   
    uint32_t row_data_low,row_data_up;
    // Security Configuration  - Customer ID programmed, AB part
    uint64_t row_data = 0x00000000000000cf;
    uint64_t write_result;

    write_result = sotp_mem_write(12,0,row_data);
    if (check_row_12(write_result) != 0)
        return IPROC_OTP_INVALID;

    row_data = 0x000001e79980FF80; //Encoding type
/* Uncomment following line to disable JTAG when converting to ABPROD */
/* #define DISABLE_JTAG */
#ifdef DISABLE_JTAG
    row_data |= 0x7C;
#endif
    write_result = sotp_mem_write(13,0,row_data);
    if (check_row_13(write_result) != 0)
        return IPROC_OTP_INVALID;

    row_data = sotp_mem_read(13,0);

    return  IPROC_OTP_VALID;
}
