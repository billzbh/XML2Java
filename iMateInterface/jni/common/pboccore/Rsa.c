/* RSA.C - RSA routines for RSAREF
 */

/* Copyright (C) 1991-2 RSA Laboratories, a division of RSA Data
   Security, Inc. All rights reserved.
 */

#include <string.h>
#include "Nn.h"
#include "RsaRef.h"

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
                   unsigned char *input, unsigned short inputLen,
                   R_RSA_PUBLIC_KEY *publicKey)
{
  NN_DIGIT c[MAX_NN_DIGITS], e[MAX_NN_DIGITS], m[MAX_NN_DIGITS],
    n[MAX_NN_DIGITS];
  unsigned short eDigits, nDigits;

  NN_Decode (m, MAX_NN_DIGITS, input, inputLen);
  NN_Decode (n, MAX_NN_DIGITS, publicKey->modulus, MAX_RSA_MODULUS_LEN);
  NN_Decode (e, MAX_NN_DIGITS, publicKey->exponent, MAX_RSA_MODULUS_LEN);
  nDigits = NN_Digits (n, MAX_NN_DIGITS);
  eDigits = NN_Digits (e, MAX_NN_DIGITS);
  
  if (NN_Cmp (m, n, nDigits) >= 0)
    return (RE_DATA);
  
  /* Compute c = m^e mod n.
   */
  NN_ModExp (c, m, e, eDigits, n, nDigits);

  *outputLen = (publicKey->bits + 7) / 8;
  NN_Encode (output, *outputLen, c, nDigits);
  
  /* Zeroize sensitive information.
   */
  memset ((POINTER)c, 0, sizeof (c));
  memset ((POINTER)m, 0, sizeof (m));

  return (0);
}

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
                    unsigned char *input, unsigned short inputLen,
                    R_RSA_PRIVATE_KEY *privateKey)
{
  NN_DIGIT c[MAX_NN_DIGITS], cP[MAX_NN_DIGITS], cQ[MAX_NN_DIGITS],
    dP[MAX_NN_DIGITS], dQ[MAX_NN_DIGITS], mP[MAX_NN_DIGITS], d[MAX_NN_DIGITS],
    mQ[MAX_NN_DIGITS], n[MAX_NN_DIGITS], p[MAX_NN_DIGITS], q[MAX_NN_DIGITS],
    qInv[MAX_NN_DIGITS], t[MAX_NN_DIGITS];
  unsigned short cDigits, nDigits, pDigits, dDigits;
  int i;
  
  for(i=0; i<sizeof(privateKey->prime[0]); i++) {
	  if(privateKey->prime[0][i]) {
		  i = -1; // -1 means not zero
		  break;
	  }
  }
  if(i != -1) {
	  // 如果该私钥是用RSASetPrivateKey()函数设置的，则privateKey->prime[0]为0，此时用该较慢算法
      NN_Decode (c, MAX_NN_DIGITS, input, inputLen);
      NN_Decode (n, MAX_NN_DIGITS, privateKey->modulus, MAX_RSA_MODULUS_LEN);
      NN_Decode (d, MAX_NN_DIGITS, privateKey->exponent, MAX_RSA_MODULUS_LEN);
      nDigits = NN_Digits (n, MAX_NN_DIGITS);
      dDigits = NN_Digits (d, MAX_NN_DIGITS);
      /* Compute t = c^d mod n.
       */
      NN_ModExp (t, c, d, dDigits, n, nDigits);

      *outputLen = (privateKey->bits + 7) / 8;
      NN_Encode (output, *outputLen, t, nDigits);
  
      /* Zeroize sensitive information.
       */
      memset ((POINTER)c, 0, sizeof (c));
      memset ((POINTER)t, 0, sizeof (t));
      memset ((POINTER)d, 0, sizeof (d));

      return (0);
  }

  NN_Decode (c, MAX_NN_DIGITS, input, inputLen);
  NN_Decode (n, MAX_NN_DIGITS, privateKey->modulus, MAX_RSA_MODULUS_LEN);
  NN_Decode (p, MAX_NN_DIGITS, privateKey->prime[0], MAX_RSA_PRIME_LEN);
  NN_Decode (q, MAX_NN_DIGITS, privateKey->prime[1], MAX_RSA_PRIME_LEN);
  NN_Decode 
    (dP, MAX_NN_DIGITS, privateKey->primeExponent[0], MAX_RSA_PRIME_LEN);
  NN_Decode 
    (dQ, MAX_NN_DIGITS, privateKey->primeExponent[1], MAX_RSA_PRIME_LEN);
  NN_Decode (qInv, MAX_NN_DIGITS, privateKey->coefficient, MAX_RSA_PRIME_LEN);
  cDigits = NN_Digits (c, MAX_NN_DIGITS);
  nDigits = NN_Digits (n, MAX_NN_DIGITS);
  pDigits = NN_Digits (p, MAX_NN_DIGITS);

  if (NN_Cmp (c, n, nDigits) >= 0)
    return (RE_DATA);
  
  /* Compute mP = cP^dP mod p  and  mQ = cQ^dQ mod q. (Assumes q has
     length at most pDigits, i.e., p > q.)
   */
  NN_Mod (cP, c, cDigits, p, pDigits);
  NN_Mod (cQ, c, cDigits, q, pDigits);
  NN_ModExp (mP, cP, dP, pDigits, p, pDigits);
  NN_AssignZero (mQ, nDigits);
  NN_ModExp (mQ, cQ, dQ, pDigits, q, pDigits);
  
  /* Chinese Remainder Theorem:
       m = ((((mP - mQ) mod p) * qInv) mod p) * q + mQ.
   */
  if (NN_Cmp (mP, mQ, pDigits) >= 0)
    NN_Sub (t, mP, mQ, pDigits);
  else {
    NN_Sub (t, mQ, mP, pDigits);
    NN_Sub (t, p, t, pDigits);
  }
  NN_ModMult (t, t, qInv, p, pDigits);
  NN_Mult (t, t, q, pDigits);
  NN_Add (t, t, mQ, nDigits);

  *outputLen = (privateKey->bits + 7) / 8;
  NN_Encode (output, *outputLen, t, nDigits);

  /* Zeroize sensitive information.
   */
  memset ((POINTER)c, 0, sizeof (c));
  memset ((POINTER)cP, 0, sizeof (cP));
  memset ((POINTER)cQ, 0, sizeof (cQ));
  memset ((POINTER)dP, 0, sizeof (dP));
  memset ((POINTER)dQ, 0, sizeof (dQ));
  memset ((POINTER)mP, 0, sizeof (mP));
  memset ((POINTER)mQ, 0, sizeof (mQ));
  memset ((POINTER)p, 0, sizeof (p));
  memset ((POINTER)q, 0, sizeof (q));
  memset ((POINTER)qInv, 0, sizeof (qInv));
  memset ((POINTER)t, 0, sizeof (t));

  return (0);
}

/* Set public key
 */
// in  : psModulus          // RSA modulus
//       pProtoKey          // RSA modulus length & public exponent
// out : pPublicKey         // RSA public key
short RSASetPublicKey(R_RSA_PUBLIC_KEY *pPublicKey, unsigned char *psModulus, R_RSA_PROTO_KEY *pProtoKey)
{
    short iLen;

    iLen = (pProtoKey->bits+7) / 8;
    if(iLen < 4 || iLen >248)
        return(RE_MODULUS_LEN);

	memset(pPublicKey, 0, sizeof(R_RSA_PUBLIC_KEY));
    pPublicKey->bits = pProtoKey->bits;
    memcpy(pPublicKey->modulus+sizeof(pPublicKey->modulus)-iLen, psModulus, iLen);
    if(pProtoKey->useFermat4) {
        // exponent 65537
        pPublicKey->exponent[sizeof(pPublicKey->exponent)-3] = 0x01;
        pPublicKey->exponent[sizeof(pPublicKey->exponent)-1] = 0x01;
    }
    else {
        // exponent 3
        pPublicKey->exponent[sizeof(pPublicKey->exponent)-1] = 0x03;
    }

    return(0);
}

/* Set private key
 */
// in  : psModulus          // RSA modulus
//       psPrivateExponent  // d value
//       pProtoKey          // RSA modulus length
// out : pPrivateKey        // RSA private key
// note: 如果用这种方式设置私钥(只设置私钥的PrivateExponent，没有设置其它与因子有关的项)
//       那么后续利用此私钥计算时会消耗比较长的时间
short RSASetPrivateKey(R_RSA_PRIVATE_KEY *pPrivateKey, unsigned char *psModulus,
                   unsigned char *psPrivateExponent, R_RSA_PROTO_KEY *pProtoKey)
{
    short iLen;

    iLen = (pProtoKey->bits+7) / 8;
    if(iLen < 4 || iLen >248)
        return(RE_MODULUS_LEN);

	memset(pPrivateKey, 0, sizeof(R_RSA_PRIVATE_KEY));
	pPrivateKey->bits = pProtoKey->bits;
    memcpy(pPrivateKey->modulus+sizeof(pPrivateKey->modulus)-iLen, psModulus, iLen);
    memcpy(pPrivateKey->exponent+sizeof(pPrivateKey->exponent)-iLen, psPrivateExponent, iLen);

    return(0);
}

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
						  R_RSA_PROTO_KEY *pProtoKey)
{
    short iLen;

    iLen = (pProtoKey->bits+7) / 8;
    if(iLen < 4 || iLen >248 || iLen%2)
        return(RE_MODULUS_LEN);

	memset(pPrivateKey, 0, sizeof(R_RSA_PRIVATE_KEY));
	pPrivateKey->bits = pProtoKey->bits;
    memcpy(pPrivateKey->modulus+sizeof(pPrivateKey->modulus)-iLen, psModulus, iLen);
    memcpy(pPrivateKey->prime[0]+sizeof(pPrivateKey->prime[0])-iLen/2, p, iLen/2);
    memcpy(pPrivateKey->prime[1]+sizeof(pPrivateKey->prime[1])-iLen/2, q, iLen/2);
    memcpy(pPrivateKey->primeExponent[0]+sizeof(pPrivateKey->primeExponent[0])-iLen/2, dp, iLen/2);
    memcpy(pPrivateKey->primeExponent[1]+sizeof(pPrivateKey->primeExponent[1])-iLen/2, dq, iLen/2);
    memcpy(pPrivateKey->coefficient+sizeof(pPrivateKey->coefficient)-iLen/2, qinv, iLen/2);

    return(0);
}
