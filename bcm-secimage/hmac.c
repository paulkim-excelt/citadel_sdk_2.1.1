
#include <stdio.h>
#include <stdlib.h> 
#include <string.h>
#include "secimage.h"

#define DEBUG

/*
 * NIST Secure Hash Algorithm.
 *
 * Written 2 September 1992, Peter C. Gutmann.
 * This implementation placed in the public domain.
 *
 * Modified 1 June 1993, Colin Plumb.
 * Renamed to SHA and comments updated a bit 1 November 1995 Colin Plumb.
 * These modifications placed in the public domain.
 *
 * Comments to pgut1@cs.aukuni.ac.nz
 */

/* Typedefs for various word sizes */
typedef unsigned long word32;
typedef unsigned char word8;

/* The SHA block size and message digest sizes, in bytes */
#define SHA_BLOCKSIZE   64
#define SHA_DIGESTSIZE  20

/*
 * The structure for storing SHA info.
 * data[] is placed first in case offsets of 0 are faster
 * for some reason; it's the most often accessed field.
 */

struct SHAContext {
    word32 data[ 16 ];      /* SHA data buffer */
    word32 digest[ 5 ];     /* Message digest */
#ifdef HAVE64
    word64 count;
#else
    word32 countHi, countLo;    /* 64-bit byte count */
#endif
};

/* Which standard?  FIPS 180 or FIPS 180.1? */

#define SHA_VERSION 1

/* Whether the machine is little-endian or not */

void shaInit(struct SHAContext *sha);
void shaUpdate(struct SHAContext *sha, word8 const *buffer, unsigned count);
void shaFinal(struct SHAContext *shaInfo, word8 *hash);
void shaTransform(struct SHAContext *sha);

/*
 * NIST Secure Hash Algorithm.
 *
 * Written 2 September 1992, Peter C. Gutmann.
 * This implementation placed in the public domain.
 *
 * Modified 1 June 1993, Colin Plumb.
 * Modified for the new SHS based on Peter Gutmann's work,
 * 18 July 1994, Colin Plumb.
 * Gutmann's work.
 * Renamed to SHA and comments updated a bit 1 November 1995, Colin Plumb.
 * These modifications placed in the public domain.
 *
 * Comments to pgut1@cs.aukuni.ac.nz
 */

/*
 * The SHA f()-functions.  The f1 and f3 functions can be optimized to
 * save one boolean operation each - thanks to Rich Schroeppel,
 * rcs@cs.arizona.edu for discovering this
 */
#define f1(x,y,z)   ( z ^ (x & (y ^ z) ) )      /* Rounds  0-19 */
#define f2(x,y,z)   ( x ^ y ^ z )           /* Rounds 20-39 */
#define f3(x,y,z)   ( (x & y) | (z & (x | y) ) )    /* Rounds 40-59 */
#define f4(x,y,z)   ( x ^ y ^ z )           /* Rounds 60-79 */

/*
 * The SHA Mysterious Constants.
 * K1 = floor(sqrt(2) * 2^30)
 * K2 = floor(sqrt(3) * 2^30)
 * K3 = floor(sqrt(5) * 2^30)
 * K4 = floor(sqrt(10) * 2^30)
 */
#define K1  0x5A827999L /* Rounds  0-19 */
#define K2  0x6ED9EBA1L /* Rounds 20-39 */
#define K3  0x8F1BBCDCL /* Rounds 40-59 */
#define K4  0xCA62C1D6L /* Rounds 60-79 */

/* SHA initial values */

#define h0init  0x67452301L
#define h1init  0xEFCDAB89L
#define h2init  0x98BADCFEL
#define h3init  0x10325476L
#define h4init  0xC3D2E1F0L

/*
 * Note that it may be necessary to add parentheses to these macros
 * if they are to be called with expressions as arguments.
 */

/* 32-bit rotate left - kludged with shifts */

#define ROTL(n,X)  ( (X << n) | (X >> (32-n)) )

/*
 * The initial expanding function
 *
 * The hash function is defined over an 80-word expanded input array W,
 * where the first 16 are copies of the input data, and the remaining 64
 * are defined by W[i] = W[i-16] ^ W[i-14] ^ W[i-8] ^ W[i-3].  This
 * implementation generates these values on the fly in a circular buffer.
 */

#if SHA_VERSION
/* The new ("corrected") SHA, FIPS 180.1 */
/* Same as below, but then rotate left one bit */
#define expand(W,i) (W[i&15] ^= W[(i-14)&15] ^ W[(i-8)&15] ^ W[(i-3)&15], \
                     W[i&15] = ROTL(1, W[i&15]))
#else
/* The old (pre-correction) SHA, FIPS 180 */
#define expand(W,i) (W[i&15] ^= W[(i-14)&15] ^ W[(i-8)&15] ^ W[(i-3)&15])
#endif

/*
 * The prototype SHA sub-round
 *
 * The fundamental sub-round is
 * a' = e + ROTL(5,a) + f(b, c, d) + k + data;
 * b' = a;
 * c' = ROTL(30,b);
 * d' = c;
 * e' = d;
 * ... but this is implemented by unrolling the loop 5 times and renaming
 * the variables (e,a,b,c,d) = (a',b',c',d',e') each iteration.
 */
#define subRound(a, b, c, d, e, f, k, data) \
    ( e += ROTL(5,a) + f(b, c, d) + k + data, b = ROTL(30, b) )
/*
 * The above code is replicated 20 times for each of the 4 functions,
 * using the next 20 values from the W[] array each time.
 */

/* Initialize the SHA values */

void
shaInit(struct SHAContext *sha)
{
    /* Set the h-vars to their initial values */
    sha->digest[0] = h0init;
    sha->digest[1] = h1init;
    sha->digest[2] = h2init;
    sha->digest[3] = h3init;
    sha->digest[4] = h4init;

    /* Initialise bit count */
#ifdef HAVE64
    sha->count = 0;
#else
    sha->countLo = sha->countHi = 0;
#endif
}

/*
 * Perform the SHA transformation.  Note that this code, like MD5, seems to
 * break some optimizing compilers due to the complexity of the expressions
 * and the size of the basic block.  It may be necessary to split it into
 * sections, e.g. based on the four subrounds
 *
 * Note that this corrupts the sha->data area
 */
#ifndef ASM

void shaTransform(struct SHAContext *sha)
{
    register word32 A, B, C, D, E;

    /* Set up first buffer */
    A = sha->digest[0];
    B = sha->digest[1];
    C = sha->digest[2];
    D = sha->digest[3];
    E = sha->digest[4];

    /* Heavy mangling, in 4 sub-rounds of 20 interations each. */
    subRound( A, B, C, D, E, f1, K1, sha->data[ 0] );
    subRound( E, A, B, C, D, f1, K1, sha->data[ 1] );
    subRound( D, E, A, B, C, f1, K1, sha->data[ 2] );
    subRound( C, D, E, A, B, f1, K1, sha->data[ 3] );
    subRound( B, C, D, E, A, f1, K1, sha->data[ 4] );
    subRound( A, B, C, D, E, f1, K1, sha->data[ 5] );
    subRound( E, A, B, C, D, f1, K1, sha->data[ 6] );
    subRound( D, E, A, B, C, f1, K1, sha->data[ 7] );
    subRound( C, D, E, A, B, f1, K1, sha->data[ 8] );
    subRound( B, C, D, E, A, f1, K1, sha->data[ 9] );
    subRound( A, B, C, D, E, f1, K1, sha->data[10] );
    subRound( E, A, B, C, D, f1, K1, sha->data[11] );
    subRound( D, E, A, B, C, f1, K1, sha->data[12] );
    subRound( C, D, E, A, B, f1, K1, sha->data[13] );
    subRound( B, C, D, E, A, f1, K1, sha->data[14] );
    subRound( A, B, C, D, E, f1, K1, sha->data[15] );
    subRound( E, A, B, C, D, f1, K1, expand(sha->data, 16) );
    subRound( D, E, A, B, C, f1, K1, expand(sha->data, 17) );
    subRound( C, D, E, A, B, f1, K1, expand(sha->data, 18) );
    subRound( B, C, D, E, A, f1, K1, expand(sha->data, 19) );

    subRound( A, B, C, D, E, f2, K2, expand(sha->data, 20) );
    subRound( E, A, B, C, D, f2, K2, expand(sha->data, 21) );
    subRound( D, E, A, B, C, f2, K2, expand(sha->data, 22) );
    subRound( C, D, E, A, B, f2, K2, expand(sha->data, 23) );
    subRound( B, C, D, E, A, f2, K2, expand(sha->data, 24) );
    subRound( A, B, C, D, E, f2, K2, expand(sha->data, 25) );
    subRound( E, A, B, C, D, f2, K2, expand(sha->data, 26) );
    subRound( D, E, A, B, C, f2, K2, expand(sha->data, 27) );
    subRound( C, D, E, A, B, f2, K2, expand(sha->data, 28) );
    subRound( B, C, D, E, A, f2, K2, expand(sha->data, 29) );
    subRound( A, B, C, D, E, f2, K2, expand(sha->data, 30) );
    subRound( E, A, B, C, D, f2, K2, expand(sha->data, 31) );
    subRound( D, E, A, B, C, f2, K2, expand(sha->data, 32) );
    subRound( C, D, E, A, B, f2, K2, expand(sha->data, 33) );
    subRound( B, C, D, E, A, f2, K2, expand(sha->data, 34) );
    subRound( A, B, C, D, E, f2, K2, expand(sha->data, 35) );
    subRound( E, A, B, C, D, f2, K2, expand(sha->data, 36) );
    subRound( D, E, A, B, C, f2, K2, expand(sha->data, 37) );
    subRound( C, D, E, A, B, f2, K2, expand(sha->data, 38) );
    subRound( B, C, D, E, A, f2, K2, expand(sha->data, 39) );

    subRound( A, B, C, D, E, f3, K3, expand(sha->data, 40) );
    subRound( E, A, B, C, D, f3, K3, expand(sha->data, 41) );
    subRound( D, E, A, B, C, f3, K3, expand(sha->data, 42) );
    subRound( C, D, E, A, B, f3, K3, expand(sha->data, 43) );
    subRound( B, C, D, E, A, f3, K3, expand(sha->data, 44) );
    subRound( A, B, C, D, E, f3, K3, expand(sha->data, 45) );
    subRound( E, A, B, C, D, f3, K3, expand(sha->data, 46) );
    subRound( D, E, A, B, C, f3, K3, expand(sha->data, 47) );
    subRound( C, D, E, A, B, f3, K3, expand(sha->data, 48) );
    subRound( B, C, D, E, A, f3, K3, expand(sha->data, 49) );
    subRound( A, B, C, D, E, f3, K3, expand(sha->data, 50) );
    subRound( E, A, B, C, D, f3, K3, expand(sha->data, 51) );
    subRound( D, E, A, B, C, f3, K3, expand(sha->data, 52) );
    subRound( C, D, E, A, B, f3, K3, expand(sha->data, 53) );
    subRound( B, C, D, E, A, f3, K3, expand(sha->data, 54) );
    subRound( A, B, C, D, E, f3, K3, expand(sha->data, 55) );
    subRound( E, A, B, C, D, f3, K3, expand(sha->data, 56) );
    subRound( D, E, A, B, C, f3, K3, expand(sha->data, 57) );
    subRound( C, D, E, A, B, f3, K3, expand(sha->data, 58) );
    subRound( B, C, D, E, A, f3, K3, expand(sha->data, 59) );

    subRound( A, B, C, D, E, f4, K4, expand(sha->data, 60) );
    subRound( E, A, B, C, D, f4, K4, expand(sha->data, 61) );
    subRound( D, E, A, B, C, f4, K4, expand(sha->data, 62) );
    subRound( C, D, E, A, B, f4, K4, expand(sha->data, 63) );
    subRound( B, C, D, E, A, f4, K4, expand(sha->data, 64) );
    subRound( A, B, C, D, E, f4, K4, expand(sha->data, 65) );
    subRound( E, A, B, C, D, f4, K4, expand(sha->data, 66) );
    subRound( D, E, A, B, C, f4, K4, expand(sha->data, 67) );
    subRound( C, D, E, A, B, f4, K4, expand(sha->data, 68) );
    subRound( B, C, D, E, A, f4, K4, expand(sha->data, 69) );
    subRound( A, B, C, D, E, f4, K4, expand(sha->data, 70) );
    subRound( E, A, B, C, D, f4, K4, expand(sha->data, 71) );
    subRound( D, E, A, B, C, f4, K4, expand(sha->data, 72) );
    subRound( C, D, E, A, B, f4, K4, expand(sha->data, 73) );
    subRound( B, C, D, E, A, f4, K4, expand(sha->data, 74) );
    subRound( A, B, C, D, E, f4, K4, expand(sha->data, 75) );
    subRound( E, A, B, C, D, f4, K4, expand(sha->data, 76) );
    subRound( D, E, A, B, C, f4, K4, expand(sha->data, 77) );
    subRound( C, D, E, A, B, f4, K4, expand(sha->data, 78) );
    subRound( B, C, D, E, A, f4, K4, expand(sha->data, 79) );

    /* Build message digest */
    sha->digest[0] += A;
    sha->digest[1] += B;
    sha->digest[2] += C;
    sha->digest[3] += D;
    sha->digest[4] += E;
}

#endif /* !ASM */

/*
 * SHA is defined in big-endian form, so this converts the buffer from
 * bytes to words, independent of the machine's native endianness.
 *
 * Assuming a consistent byte ordering for the machine, this also
 * has the magic property of being self-inverse.  It is used as
 * such.
 */

static void byteReverse(word32 *buffer, unsigned byteCount)
{
    word32 value;

    byteCount /= sizeof(word32);
    while ( byteCount-- ) {
        value = (word32)((unsigned)((word8 *)buffer)[0] << 8 |
                               ((word8 *)buffer)[1]) << 16 |
                        ((unsigned)((word8 *)buffer)[2] << 8 |
                               ((word8 *)buffer)[3]);
        *buffer++ = value;
    }
}

/* Update SHA for a block of data. */

void
shaUpdate(struct SHAContext *sha, word8 const *buffer, unsigned count)
{
    word32 t;

    /* Update bitcount */

#ifdef HAVE64
    t = (word32)sha->count & 0x3f;
    sha->count += count;
#else
    t = sha->countLo;
    if ( ( sha->countLo = t + count ) < t )
        sha->countHi++; /* Carry from low to high */

    t &= 0x3f;  /* Bytes already in sha->data */
#endif

    /* Handle any leading odd-sized chunks */

    if (t) {
        word8 *p = (word8 *)sha->data + t;

        t = 64-t;
        if (count < t) {
            memcpy(p, buffer, count);
            return;
        }
        memcpy(p, buffer, t);
        byteReverse(sha->data, SHA_BLOCKSIZE);
        shaTransform(sha);
        buffer += t;
        count -= t;
    }

    /* Process data in SHA_BLOCKSIZE chunks */

    while (count >= SHA_BLOCKSIZE) {
        memcpy(sha->data, buffer, SHA_BLOCKSIZE);
        byteReverse(sha->data, SHA_BLOCKSIZE);
        shaTransform(sha);
        buffer += SHA_BLOCKSIZE;
        count -= SHA_BLOCKSIZE;
    }

    /* Handle any remaining bytes of data. */

    memcpy(sha->data, buffer, count);
}

/* Final wrapup - pad to 64-byte boundary with the bit pattern
   1 0* (64-bit count of bits processed, MSB-first) */

void
shaFinal(struct SHAContext *sha, word8 *hash)
{
    int count;
    word8 *p;

    /* Compute number of bytes mod 64 */
#ifdef HAVE64
    count = (int)sha->count & 0x3F;
#else
    count = (int)sha->countLo & 0x3F;
#endif

    /*
     * Set the first char of padding to 0x80.
     * This is safe since there is always at least one byte free
     */
    p = (word8 *)sha->data + count;
    *p++ = 0x80;

    /* Bytes of padding needed to make 64 bytes */
    count = SHA_BLOCKSIZE - 1 - count;

    /* Pad out to 56 mod 64 */
    if (count < 8) {
        /* Two lots of padding:  Pad the first block to 64 bytes */
        memset(p, 0, count);
        byteReverse(sha->data, SHA_BLOCKSIZE);
        shaTransform(sha);

        /* Now fill the next block with 56 bytes */
        memset(sha->data, 0, SHA_BLOCKSIZE-8);
    } else {
        /* Pad block to 56 bytes */
        memset(p, 0, count-8);
    }
    byteReverse(sha->data, SHA_BLOCKSIZE-8);

    /* Append length in *bits* and transform */
#if HAVE64
    sha->data[14] = (word32)(sha->count >> 29);
    sha->data[15] = (word32)sha->count << 3;
#else
    sha->data[14] = sha->countHi << 3 | sha->countLo >> 29;
    sha->data[15] = sha->countLo << 3;
#endif

    shaTransform(sha);

    /* Store output hash in buffer */
    byteReverse(sha->digest, SHA_DIGESTSIZE);
    memcpy(hash, sha->digest, SHA_DIGESTSIZE);
    memset(sha, 0, sizeof(*sha));
}


#define USE_OPENSSL_SHA
#undef USE_OPENSSL_HMAC

#ifdef USE_OPENSSL_SHA
#include "crypto.h"

#define SHAINIT(x)				{EVP_MD_CTX_init(x);\
	EVP_DigestInit_ex(x, EVP_sha1(), NULL);}
#define SHAUPDATE(x,y,z)		EVP_DigestUpdate(x,y,z)
#define SHAFINAL(x,y)			EVP_DigestFinal_ex(x,y,NULL)
#define	SHACONTEXT				EVP_MD_CTX
#define	SHACLEAN(x) 			EVP_MD_CTX_cleanup(x)
#else
#define SHAINIT(x)				shaInit(x)
#define SHAUPDATE(x,y,z)		shaUpdate(x,y,z)
#define SHAFINAL(x,y)			shaFinal(x,y)
#define	SHACLEAN(x)
typedef struct SHAContext		SHACONTEXT;
#endif

#ifdef USE_OPENSSL_HMAC
#define HMACSHA(x,y,z,t)		HmacSha1Hash(y,z,t,x)
#else
#define HMACSHA(x,y,z,t)		HmacSha(x,y,z,t)
#endif


void HmacSha(word8 *key, word8 *data, int datalen, word8 *digest)
//  key;                 /* pointer to authentication key */
//  data;                /* start of data to be hashed */
//  datalen;             /* length of the data */
//  digest;              /* the result of the HMAC calculation */
{
   /*
    * The HMAC_SHA transform looks like:
    *
    * SHA(K XOR opad, SHA(K XOR ipad, data))
    *
    * where K is an n byte key
    * ipad is the byte 0x36 repeated 64 times
    * opad is the byte 0x5c repeated 64 times
    * and text is the data being protected
    *
    * This function starts the calulation of the
    * inner hash and leaves it at the point where
    * it needs to process the text
    *
    * SHA (K XOR ipad, text)
    */
	SHACONTEXT context;
    unsigned char k_ipad[65];    /* inner padding - key XORd with ipad */
    unsigned char k_opad[65];    /* outer padding - key XORd with opad */
    unsigned char buf[SHA_DIGESTSIZE];
    int i;

    /* start out by storing key in pads */
    memset (k_ipad, 0, 64);
    memset (k_opad, 0, 64);
    memcpy (k_ipad, key, SHA_DIGESTSIZE);
    memcpy (k_opad, key, SHA_DIGESTSIZE);

    /* XOR key with ipad and opad values */
    for (i=0; i<64; i++) {
         k_ipad[i] ^= 0x36;
         k_opad[i] ^= 0x5c;
    }
    /*
    * calcualte inner SHA operation
    */
    SHAINIT(&context);                   /* init context for 1st pass */
    SHAUPDATE(&context, k_ipad, 64);     /* start with inner pad */
    SHAUPDATE(&context, data, datalen);
    SHAFINAL(&context, buf);
    /*
    * calcualte outer SHA operation
    */
    SHAINIT(&context);                   /* init context for 2nd pass */
    SHAUPDATE(&context, k_opad, 64);     /* start with outer pad */
    SHAUPDATE(&context, buf, SHA_DIGESTSIZE);
    SHAFINAL(&context, digest);
#ifdef DEBUG
    DataDump("Hmac Data", data, datalen);
    printf ("Hmac key        : ");
    for (i = 0; i < SHA_DIGESTSIZE; i++)
        printf ("%02x" , key[i]);
    puts("");
    printf ("Hmac-SHA1 digest: ");
    for (i = 0; i < SHA_DIGESTSIZE; i++)
        printf ("%02x" , digest[i]);
    puts("\n");
#endif /* DEBUG */
    SHACLEAN(&context);
}

/*******************************
    Test Cases for HMAC-SHA-1
********************************/
unsigned char hdata1[9]    = "Hi There";
int           hdata1_len   =  8;
unsigned char hkey1[20]    = {
    0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b,
    0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b
};

unsigned char hdigest1[20] = {
    0xb6, 0x17, 0x31, 0x86, 0x55, 0x05, 0x72, 0x64, 0xe2, 0x8b, 0xc0, 0xb6,
    0xfb, 0x37, 0x8c, 0x8e, 0xf1, 0x46, 0xbe, 0x00
};

unsigned char hdata2[50]    = {
    0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd,
    0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd,
    0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd,
    0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd,
    0xdd, 0xdd
};
int           hdata2_len   =  50;
unsigned char hkey2[20]    = {
    0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa,
    0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa
};
unsigned char hdigest2[20] = {
    0x12, 0x5d, 0x73, 0x42, 0xb9, 0xac, 0x11, 0xcd, 0x91, 0xa3, 0x9a, 0xf4,
    0x8a, 0xa1, 0x7b, 0x4f, 0x63, 0xf1, 0x75, 0xd3
};

unsigned char hdata3[21]   = "Test With Truncation";
int             hdata3_len = 20;
unsigned char hkey3[20]    = {
    0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c,
    0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c
};
unsigned char hdigest3[20] = {
       0x4c, 0x1a, 0x03, 0x42, 0x4b, 0x55, 0xe0, 0x7f, 0xe7, 0xf2, 0x7b, 0xe1,
       0xd5, 0x8b, 0xb9, 0x32, 0x4a, 0x9a, 0x5a, 0x04
};

int HmacShaTest (void)
{
    unsigned char buf[SHA_DIGESTSIZE];

#ifdef USE_OPENSSL_HMAC
	puts ("Testing HMAC function using OpenSSL engine");
#else
	puts ("Testing HMAC function with local code");
#endif
    HMACSHA(hkey1, hdata1, hdata1_len, buf);
    if (memcmp(buf, hdigest1, SHA_DIGESTSIZE)) {
        puts("HMAC sha test1 failed");
        DataDump("hdigest1", hdigest1, SHA_DIGESTSIZE);
        DataDump("calculated", buf, SHA_DIGESTSIZE);
        return -1;
    }
    else
        puts("HMAC SHA test1 passed");
    HMACSHA(hkey2, hdata2, hdata2_len, buf);
    if (memcmp(buf, hdigest2, SHA_DIGESTSIZE)) {
        puts("HMAC sha test2 failed");
        DataDump("hdigest2", hdigest2, SHA_DIGESTSIZE);
        DataDump("calculated", buf, SHA_DIGESTSIZE);
        return -1;
    }
    else
        puts("HMAC SHA test2 passed");
    HMACSHA(hkey3, hdata3, hdata3_len, buf);
    if (memcmp(buf, hdigest3, SHA_DIGESTSIZE)) {
        puts("HMAC sha test3 failed");
        DataDump("hdigest3", hdigest3, SHA_DIGESTSIZE);
        DataDump("calculated", buf, SHA_DIGESTSIZE);
        return -1;
    }
    else
        puts("HMAC SHA test3 passed");
    return 0;
}


/* Size of buffer for SHA speed test data */

#define TEST_BLOCK_SIZE ( SHA_DIGESTSIZE * 100 )

#if SHA_VERSION
static char const *shaTestResults[] = {
    "A9993E364706816ABA3E25717850C26C9CD0D89D",
    "84983E441C3BD26EBAAE4AA1F95129E5E54670F1",
    "34AA973CD4C4DAA4F61EEB2BDBAD27316534016F",
    "34AA973CD4C4DAA4F61EEB2BDBAD27316534016F",
    "34AA973CD4C4DAA4F61EEB2BDBAD27316534016F" };
#else
static char const *shaTestResults[] = {
    "0164B8A914CD2A5E74C4F7FF082C4D97F1EDF880",
    "D2516EE1ACFA5BAF33DFC1C471E438449EF134C8",
    "3232AFFA48628A26653B5AAA44541FD90D690603",
    "3232AFFA48628A26653B5AAA44541FD90D690603",
    "3232AFFA48628A26653B5AAA44541FD90D690603" };
#endif

static int
compareSHAresults(word8 *hash, int level)
{
    char buf[41];
    int i;

    for (i = 0; i < SHA_DIGESTSIZE; i++)
        sprintf(buf+2*i, "%02X", hash[i]);

    if (strcmp(buf, shaTestResults[level-1]) == 0) {
        printf("SHA Test %d passed, result = %s\n", level, buf);
        return 0;
    } else {
        printf("Error in SHA implementation: Test %d failed\n", level);
        printf("  Result = %s\n", buf);
        printf("Expected = %s\n", shaTestResults[level-1]);
        return -1;
    }
}

word8 data0[4] =
    "abc";
int data0_len = 3;

word8 data1[57] =
    "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq";
int data1_len = 56;

word8 data2[65] =
    "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
int data2_len = 64;

word8 data3[26] =
    "aaaaaaaaaaaaaaaaaaaaaaaaa";
int data3_len = 25;

word8 data4[126] =
    "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
int data4_len = 125;

int VerifyShaHmac(void)
{
	SHACONTEXT sha;
    word8 hash[SHA_DIGESTSIZE];
    long i;
    word32 t;

    /* Check that LITTLE_ENDIAN is set correctly */
    t = 0x12345678;

#if LITTLE_ENDIAN
        if (*(word8 *)&t != 0x78) {
            puts("Error: Define BIG_ENDIAN in SHA.H and recompile");
            exit(-1);
        }
#elif BIG_ENDIAN
        if (*(word8 *)&t != 0x12) {
            puts("Error: Define LITTLE_ENDIAN in SHA.H and recompile");
            exit(-1);
        }
#endif

#ifdef USE_OPENSSL_SHA
	puts ("Testing SHA function using OpenSSL engine");
#else
	puts ("Testing SHA function with NIST code");
#endif
    /*
     * Test output data (these are the only test data given in the
     * Secure Hash Standard document, but chances are if it works
     * for this it'll work for anything)
     */
    SHAINIT(&sha);
    SHAUPDATE(&sha, (word8 *)"abc", 3);
    SHAFINAL(&sha, hash);
    if (compareSHAresults(hash, 1) < 0)
        exit (-1);

    SHAINIT(&sha);
    SHAUPDATE(&sha, data1, data1_len);
    SHAFINAL(&sha, hash);
    if (compareSHAresults(hash, 2) < 0)
        exit (-1);

    /* 1,000,000 bytes of ASCII 'a' (0x61), by 64's */
    SHAINIT(&sha);
    for (i = 0; i < 15625; i++)
        SHAUPDATE(&sha, data2, data2_len);
    SHAFINAL(&sha, hash);
    if (compareSHAresults(hash, 3) < 0)
        exit (-1);

    /* 1,000,000 bytes of ASCII 'a' (0x61), by 25's */
    SHAINIT(&sha);
    for (i = 0; i < 40000; i++)
        SHAUPDATE(&sha, data3, data3_len);
    SHAFINAL(&sha, hash);
    if (compareSHAresults(hash, 4) < 0)
        exit (-1);

    /* 1,000,000 bytes of ASCII 'a' (0x61), by 125's */
    SHAINIT(&sha);
    for (i = 0; i < 8000; i++)
        SHAUPDATE(&sha, data4, data4_len);
    SHAFINAL(&sha, hash);
    if (compareSHAresults(hash, 5) < 0)
        exit (-1);
    SHACLEAN(&sha);

    HmacShaTest ();
    return 0;
}
