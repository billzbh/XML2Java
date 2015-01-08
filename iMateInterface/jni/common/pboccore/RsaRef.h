/* RSAREF.H - header file for RSAREF cryptographic toolkit
 */

/* Copyright (C) 1991-2 RSA Laboratories, a division of RSA Data
   Security, Inc. All rights reserved.
 */

/* RSA key lengths.
 */
#ifndef RSAREF_H
#define RSAREF_H
#define MIN_RSA_MODULUS_BITS 32
#define MAX_RSA_MODULUS_BITS 1984 // 248*8 = 1984
#define MAX_RSA_MODULUS_LEN ((MAX_RSA_MODULUS_BITS + 7) / 8)
#define MAX_RSA_PRIME_BITS ((MAX_RSA_MODULUS_BITS + 1) / 2)
#define MAX_RSA_PRIME_LEN ((MAX_RSA_PRIME_BITS + 7) / 8)

/* Error codes.
 */
#define RE_CONTENT_ENCODING 0x0400
#define RE_DATA 0x0401
#define RE_DIGEST_ALGORITHM 0x0402
#define RE_ENCODING 0x0403
#define RE_KEY 0x0404
#define RE_KEY_ENCODING 0x0405
#define RE_LEN 0x0406
#define RE_MODULUS_LEN 0x0407
#define RE_NEED_RANDOM 0x0408
#define RE_PRIVATE_KEY 0x0409
#define RE_PUBLIC_KEY 0x040a
#define RE_SIGNATURE 0x040b
#define RE_SIGNATURE_ENCODING 0x040c

#define ERR_NOT_SORPORT 0x7f00

/* RSA public and private key.
 */
typedef struct {
  unsigned short bits;                         /* length in bits of modulus */
  unsigned char modulus[MAX_RSA_MODULUS_LEN];                    /* modulus */
  unsigned char exponent[MAX_RSA_MODULUS_LEN];           /* public exponent */
} R_RSA_PUBLIC_KEY;

typedef struct {
  unsigned short bits;                         /* length in bits of modulus */
  unsigned char modulus[MAX_RSA_MODULUS_LEN];                    /* modulus */
  unsigned char publicExponent[MAX_RSA_MODULUS_LEN];     /* public exponent */
  unsigned char exponent[MAX_RSA_MODULUS_LEN];          /* private exponent */
  unsigned char prime[2][MAX_RSA_PRIME_LEN];               /* prime factors */
  unsigned char primeExponent[2][MAX_RSA_PRIME_LEN];   /* exponents for CRT */
  unsigned char coefficient[MAX_RSA_PRIME_LEN];          /* CRT coefficient */
} R_RSA_PRIVATE_KEY;

/* RSA prototype key.
 */
typedef struct {
  unsigned short bits;                         /* length in bits of modulus */
  short useFermat4;                      /* public exponent (1 = F4, 0 = 3) */
} R_RSA_PROTO_KEY;

#ifdef __cplusplus
extern "C" {
#endif

/* Raw RSA public-key operation. Output has same length as modulus.
   Assumes inputLen < length of modulus.
   Requires input < modulus.
 */
// output;                          /* output block */
// outputLen;                       /* length of output block */
// input;                           /* input block */
// inputLen;                        /* length of input block */
// publicKey;                       /* RSA public key */
short RSAPublicBlock(unsigned char *output, unsigned short *outputLen,
                   unsigned char *input,  unsigned short inputLen,
                   R_RSA_PUBLIC_KEY *publicKey);

/* Raw RSA private-key operation. Output has same length as modulus.
   Assumes inputLen < length of modulus.
   Requires input < modulus.
 */
// output;                          /* output block */
// outputLen;                       /* length of output block */
// input;                           /* input block */
// inputLen;                        /* length of input block */
// privateKey;                      /* RSA private key */
short RSAPrivateBlock(unsigned char *output, unsigned short *outputLen,
                    unsigned char *input,  unsigned short inputLen,
                    R_RSA_PRIVATE_KEY *privateKey);

/* set public key
 */
// in  : psModulus          // RSA modulus
//       pProtoKey          // RSA modulus length & public exponent
// out : pPublicKey         // RSA public key
short RSASetPublicKey(R_RSA_PUBLIC_KEY *pPublicKey, unsigned char *pModulus,
                  R_RSA_PROTO_KEY *pProtoKey);

/* set private key
 */
// in  : psModulus          // RSA modulus
//       psPrivateExponent  // d value
//       pProtoKey          // RSA modulus length & public exponent
// out : pPrivateKey        // RSA private key
short RSASetPrivateKey(R_RSA_PRIVATE_KEY *pPrivateKey, unsigned char *psModulus,
                     unsigned char *psPrivateExponent, R_RSA_PROTO_KEY *pProtoKey);

/* Set private key
 */
// in  : psModulus          // RSA modulus
//       p, q, dp, dq, qinv // RSA private crt key
//       pProtoKey          // RSA modulus length
// out : pPrivateKey        // RSA private key
short RSASetPrivateKeyCRT(R_RSA_PRIVATE_KEY *pPrivateKey, unsigned char *psModulus,
						  unsigned char *p,
						  unsigned char *q,
						  unsigned char *dp,
						  unsigned char *dq,
						  unsigned char *qinv,
						  R_RSA_PROTO_KEY *pProtoKey);

#ifdef __cplusplus
};
#endif /* __cplusplus */
#endif
