#define DEFAULT_RAW_RNG_SIZE_WORD    5
#define MIN_RAW_RNG_SIZE_WORD        5 /* 20 bytes, 160 bit */
#define MAX_RAW_RNG_SIZE_WORD        16/* 64 bytes, 512 bit */



typedef void scapi_callback(uint32_t status, void *handle, void *arg);

/* crypto context, we need it here to get the size */
/* crypto context space is allocated by upper layer*/
typedef struct _crypto_lib_handle
{
	uint32_t       cmd;
	uint32_t       busy;
	uint8_t        *result;
	uint32_t       result_len;
	uint32_t       *presult_len; /* tell the caller the result length */
	scapi_callback *callback;    /* callback functions for async calls */
	void           *arg;

	/* context pointers */
	/* These two context can exist at the same time, so can not share one pointer */
	uint32_t *bulkctx; /* points to current active async bulk cipher context */
	uint32_t *rngctx;  /* pionts to the fips rng context */
} crypto_lib_handle;

typedef struct _bcm_bulk_cipher_cmd {
	uint8_t cipher_mode;     /* encrypt or decrypt */
	uint8_t encr_alg ;    /* encryption algorithm */
	uint8_t encr_mode ;
	uint8_t auth_alg  ;    /* authentication algorithm */
	uint8_t auth_mode ;
	uint8_t auth_optype;
	uint8_t cipher_order;     /* encrypt first or auth first */
} BCM_BULK_CIPHER_CMD;

struct cryptoiv{
	/* Crypto Keys*/
	uint32_t DES_IV[2];
	uint32_t AES_IV[4];
	uint32_t iv_len_from_packet;
};

struct cryptokey{
	/* Crypto Keys*/
	uint32_t DES_key[2];
	uint32_t _3DES_key[6];
	uint32_t AES_128_key[4];
	uint32_t AES_192_key[6];
	uint32_t AES_256_key[8];
};

struct hashkey{
	/* Hash Keys*/
	uint32_t HMAC_key[16];              /* HMAC key coulbe be max 64 bytes in a multiple of 4 */
	uint32_t auth_key_val ;             /* this field should be set after the HMAC population*/
};
/* fips rng context */
typedef struct _fips_rng_context{
	uint32_t raw_rng;                            /* rng directly from rng core, we compare/save one word (minimum is 16bit) */
	uint32_t sha_rng[DEFAULT_RAW_RNG_SIZE_WORD]; /* rng after sha1 */
} fips_rng_context;

struct msg_hdr0{
	uint32_t crypto_algo_0        : 1,
	auth_enable          : 1,
	sctx_order           : 1,
	encrypt_decrypt      : 1,
	sctx_crypt_mode      : 4,
	sctx_aes_key_size    : 2,
	rsvd1                : 1,
	sctx_sha             : 1,
	sctx_hash_mode       : 4,
	sctx_hash_type       : 4,
	check_key_length_sel : 1,
	length_override      : 1,
	send_hash_only       : 1,
	crypto_algo_1        : 1,
	auth_key_sz          : 8;
};

struct msg_hdr1{
	uint32_t pad_length ;
};

struct msg_hdr2{
	uint32_t aes_offset:16 ,
	auth_offset :16 ;
};
struct msg_hdr3{
	uint32_t aes_length ;
};
struct msg_hdr4{
	uint32_t auth_length ;
};

typedef struct _msg_hdr_top {
	struct msg_hdr0  *crypt_hdr0;
	struct msg_hdr1  *crypt_hdr1;
	struct msg_hdr2  *crypt_hdr2;
	struct msg_hdr3  *crypt_hdr3;
	struct msg_hdr4  *crypt_hdr4;
} msg_hdr_top;

typedef struct smu_bulkcrypto_packet_s {
	msg_hdr_top *msg_hdr;
	struct cryptokey *cryptokey;
	struct hashkey   *hashkey;
	struct cryptoiv  *cryptoiv;
	uint32_t  header_length ;        /* header length is in words */
	uint32_t  in_data_length ;      /* Must for 128 bit aligned for AES and 64 bit aligned for 	DES*/
	uint32_t  out_data_length ;     /* this will have both the crypto data+ IV + hash value */
	uint8_t * in_data ;
	uint8_t * out_data ;
	/* callback functions for bulk crypto */
	scapi_callback *callback;
	uint32_t    flags;
	uint8_t SMAU_SRAM_HEADER_ADDR[256];
} smu_bulkcrypto_packet;





typedef struct bcm_bulk_cipher_context_s {

	BCM_BULK_CIPHER_CMD *cmd;

	uint32_t    flags;
	struct smu_bulkcrypto_packet_s cur_pkt;

	/* callback functions for bulk crypto */
	scapi_callback *callback;
/*      void     *arg;*/
} bcm_bulk_cipher_context;

#define NUM_CRYPTO_CTX       3  /* PKE/MATH, BULK, RNG */
#define PHX_CRYPTO_LIB_HANDLE_SIZE (NUM_CRYPTO_CTX*sizeof(crypto_lib_handle))



