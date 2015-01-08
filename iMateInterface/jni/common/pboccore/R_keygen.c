/* R_KEYGEN.C - key-pair generation for RSAREF
 */

/* Copyright (C) 1991-2 RSA Laboratories, a division of RSA Data
   Security, Inc. All rights reserved.
 */

#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "nn.h"
#include "rsaref.h"

static short GenerateDigits (NN_DIGIT *, unsigned short);
static unsigned short SMALL_PRIMES[] = { 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41};

static short RSAPrime (NN_DIGIT *, unsigned short, NN_DIGIT *, unsigned short);
static short ProbablePrime (NN_DIGIT *, unsigned short);
static short SmallFactor (NN_DIGIT *, unsigned short);
static short FermatTest (NN_DIGIT *, unsigned short);
static short RelativelyPrime
	   (NN_DIGIT *, unsigned short, NN_DIGIT *, unsigned short);

/* Find a probable prime a between 3*2^(b-2) and 2^b-1, starting at
   3*2^(b-2) + (c mod 2^(b-2)), such that gcd (a-1, d) = 1.

   Lengths: a[cDigits], c[cDigits], d[dDigits].
   Assumes b > 2, b < cDigits * NN_DIGIT_BITS, d is odd,
           cDigits < MAX_NN_DIGITS, dDigits < MAX_NN_DIGITS, and a
           probable prime can be found.
 */
void FindRSAPrime(NN_DIGIT *a, unsigned short b, NN_DIGIT *c, unsigned short cDigits, NN_DIGIT *d, unsigned short dDigits)
{
  NN_DIGIT t[MAX_NN_DIGITS], u[MAX_NN_DIGITS], v[MAX_NN_DIGITS],
    w[MAX_NN_DIGITS];
  
  /* Compute t = 2^(b-2), u = 3*2^(b-2).
   */
  NN_Assign2Exp (t, (unsigned short)(b-2), cDigits);
  NN_Assign2Exp (u, (unsigned short)(b-1), cDigits);
  NN_Add (u, u, t, cDigits);
  
  /* Compute v = 3*2^(b-2) + (c mod 2^(b-2)); add one if even.
   */
  NN_Mod (v, c, cDigits, t, cDigits);
  NN_Add (v, v, u, cDigits);
  if (NN_EVEN (v, cDigits)) {
    NN_ASSIGN_DIGIT (w, 1, cDigits);
    NN_Add (v, v, w, cDigits);
  }
  
  /* Compute w = 2, u = 2^b - 2.
   */
  NN_ASSIGN_DIGIT (w, 2, cDigits);
  NN_Sub (u, u, w, cDigits);
  NN_Add (u, u, t, cDigits);

  /* Search to 2^b-1 from starting point, then from 3*2^(b-2)+1.
   */
  while (! RSAPrime (v, cDigits, d, dDigits)) {
    if (NN_Cmp (v, u, cDigits) > 0)
      NN_Sub (v, v, t, cDigits);
    NN_Add (v, v, w, cDigits);
  }
  
  NN_Assign (a, v, cDigits);
  
  /* Zeroize sensitive information.
   */
  memset ((POINTER)v, 0, sizeof (v));
}

/* Returns nonzero iff a is a probable prime and GCD (a-1, b) = 1.

   Lengths: a[aDigits], b[bDigits].
   Assumes aDigits < MAX_NN_DIGITS, bDigits < MAX_NN_DIGITS.
 */
static short RSAPrime(NN_DIGIT *a, unsigned short aDigits, NN_DIGIT *b, unsigned short bDigits)
{
  short status;
  NN_DIGIT aMinus1[MAX_NN_DIGITS], t[MAX_NN_DIGITS];
  
  NN_ASSIGN_DIGIT (t, 1, aDigits);
  NN_Sub (aMinus1, a, t, aDigits);
  
  status = ProbablePrime (a, aDigits) &&
    RelativelyPrime (aMinus1, aDigits, b, bDigits);

  /* Zeroize sensitive information.
   */
  memset ((POINTER)aMinus1, 0, sizeof (aMinus1));
  
  return (status);
}

/* Returns nonzero iff a is a probable prime.

   Lengths: a[aDigits].
   Assumes aDigits < MAX_NN_DIGITS.
 */
static short ProbablePrime(NN_DIGIT *a, unsigned short aDigits)
{
  return (! SmallFactor (a, aDigits) && FermatTest (a, aDigits));
}

/* Returns nonzero iff a has a prime factor in SMALL_PRIMES.

   Lengths: a[aDigits].
   Assumes aDigits < MAX_NN_DIGITS.
 */
static short SmallFactor(NN_DIGIT *a, unsigned short aDigits)
{
  short status;
  NN_DIGIT t[1];
  unsigned short i;
  
  status = 0;
  
  for (i = 0; i < sizeof(SMALL_PRIMES)/sizeof(SMALL_PRIMES)[0]; i++) {
    NN_ASSIGN_DIGIT (t, SMALL_PRIMES[i], 1);
    NN_Mod (t, a, aDigits, t, 1);
    if (NN_Zero (t, 1)) {
      status = 1;
      break;
    }
  }
  
  /* Zeroize sensitive information.
   */
  i = 0;
  memset ((POINTER)t, 0, sizeof (t));

  return (status);
}

/* Returns nonzero iff a passes Fermat's test for witness 2.
   (All primes pass the test, and nearly all composites fail.)
     
   Lengths: a[aDigits].
   Assumes aDigits < MAX_NN_DIGITS.
 */
static short FermatTest(NN_DIGIT *a, unsigned short aDigits)
{
  short status;
  NN_DIGIT t[MAX_NN_DIGITS], u[MAX_NN_DIGITS];
  
  NN_ASSIGN_DIGIT (t, 2, aDigits);
  NN_ModExp (u, t, a, aDigits, a, aDigits);
  
  status = NN_EQUAL (t, u, aDigits);
  
  /* Zeroize sensitive information.
   */
  memset ((POINTER)u, 0, sizeof (u));
  
  return (status);
}

/* Returns nonzero iff a and b are relatively prime.

   Lengths: a[aDigits], b[bDigits].
   Assumes aDigits >= bDigits, aDigits < MAX_NN_DIGITS.
 */
static short RelativelyPrime (NN_DIGIT *a, unsigned short aDigits, NN_DIGIT *b, unsigned short bDigits)
{
  short status;
  NN_DIGIT t[MAX_NN_DIGITS], u[MAX_NN_DIGITS];
  
  NN_AssignZero (t, aDigits);
  NN_Assign (t, b, bDigits);
  NN_Gcd (t, a, t, aDigits);
  NN_ASSIGN_DIGIT (u, 1, aDigits);

  status = NN_EQUAL (t, u, aDigits);
  
  /* Zeroize sensitive information.
   */
  memset ((POINTER)t, 0, sizeof (t));
  
  return (status);
}

/* Generates an RSA key pair with a given length and public exponent.
 */
// R_RSA_PUBLIC_KEY *publicKey;         /* new RSA public key */
// R_RSA_PRIVATE_KEY *privateKey;       /* new RSA private key */
// R_RSA_PROTO_KEY *protoKey;           /* RSA prototype key */
short R_GeneratePEMKeys(R_RSA_PUBLIC_KEY *publicKey,
                      R_RSA_PRIVATE_KEY *privateKey,
                      R_RSA_PROTO_KEY *protoKey)
{
  NN_DIGIT d[MAX_NN_DIGITS], dP[MAX_NN_DIGITS], dQ[MAX_NN_DIGITS],
    e[MAX_NN_DIGITS], n[MAX_NN_DIGITS], p[MAX_NN_DIGITS], phiN[MAX_NN_DIGITS],
    pMinus1[MAX_NN_DIGITS], q[MAX_NN_DIGITS], qInv[MAX_NN_DIGITS],
    qMinus1[MAX_NN_DIGITS], t[MAX_NN_DIGITS];
  short status;
  unsigned short nDigits, pDigits;
  time_t CurTime;

  if ((protoKey->bits < MIN_RSA_MODULUS_BITS) ||
      (protoKey->bits > MAX_RSA_MODULUS_BITS))
    return (RE_MODULUS_LEN);
  nDigits = (protoKey->bits + NN_DIGIT_BITS - 1) / NN_DIGIT_BITS;
  pDigits = (nDigits + 1) / 2;

  /* Generate random RSA primes p and q so that product has correct length.
   */
  srand(time(&CurTime));
  if (((status = GenerateDigits (p, pDigits)) != 0) ||
      ((status = GenerateDigits (q, pDigits))) != 0)
    return (status);

  /* NOTE: for 65537, this assumes NN_DIGIT is at least 17 bits. */
  NN_ASSIGN_DIGIT
    (e, protoKey->useFermat4 ? (NN_DIGIT)65537 : (NN_DIGIT)3, nDigits);
  FindRSAPrime (p, (unsigned short)((protoKey->bits + 1) / 2), p, pDigits, e, 1);
  FindRSAPrime (q, (unsigned short)(protoKey->bits / 2), q, pDigits, e, 1);

  /* Sort so that p > q. (p = q case is extremely unlikely.)
   */
  if (NN_Cmp (p, q, pDigits) < 0) {
    NN_Assign (t, p, pDigits);
    NN_Assign (p, q, pDigits);
    NN_Assign (q, t, pDigits);
  }

  /* Compute n = pq, qInv = q^{-1} mod p, d = e^{-1} mod (p-1)(q-1),
     dP = d mod p-1, dQ = d mod q-1.
   */
  NN_Mult (n, p, q, pDigits);
  NN_ModInv (qInv, q, p, pDigits);
  
  NN_ASSIGN_DIGIT (t, 1, pDigits);
  NN_Sub (pMinus1, p, t, pDigits);
  NN_Sub (qMinus1, q, t, pDigits);
  NN_Mult (phiN, pMinus1, qMinus1, pDigits);

  NN_ModInv (d, e, phiN, nDigits);
  NN_Mod (dP, d, nDigits, pMinus1, pDigits);
  NN_Mod (dQ, d, nDigits, qMinus1, pDigits);
  
  publicKey->bits = privateKey->bits = protoKey->bits;
  NN_Encode (publicKey->modulus, MAX_RSA_MODULUS_LEN, n, nDigits);
  NN_Encode (publicKey->exponent, MAX_RSA_MODULUS_LEN, e, 1);
  memcpy
    ((POINTER)privateKey->modulus, (POINTER)publicKey->modulus,
     MAX_RSA_MODULUS_LEN);
  memcpy
    ((POINTER)privateKey->publicExponent, (POINTER)publicKey->exponent,
     MAX_RSA_MODULUS_LEN);
  NN_Encode (privateKey->exponent, MAX_RSA_MODULUS_LEN, d, nDigits);
  NN_Encode (privateKey->prime[0], MAX_RSA_PRIME_LEN, p, pDigits);
  NN_Encode (privateKey->prime[1], MAX_RSA_PRIME_LEN, q, pDigits);
  NN_Encode (privateKey->primeExponent[0], MAX_RSA_PRIME_LEN, dP, pDigits);
  NN_Encode (privateKey->primeExponent[1], MAX_RSA_PRIME_LEN, dQ, pDigits);
  NN_Encode (privateKey->coefficient, MAX_RSA_PRIME_LEN, qInv, pDigits);
   
  /* Zeroize sensitive information.
   */
  memset ((POINTER)d, 0, sizeof (d));
  memset ((POINTER)dP, 0, sizeof (dP));
  memset ((POINTER)dQ, 0, sizeof (dQ));
  memset ((POINTER)p, 0, sizeof (p));
  memset ((POINTER)phiN, 0, sizeof (phiN));
  memset ((POINTER)pMinus1, 0, sizeof (pMinus1));
  memset ((POINTER)q, 0, sizeof (q));
  memset ((POINTER)qInv, 0, sizeof (qInv));
  memset ((POINTER)qMinus1, 0, sizeof (qMinus1));
  memset ((POINTER)t, 0, sizeof (t));
  
  return (0);
}

static short GenerateDigits(NN_DIGIT *a, unsigned short digits)
{
  unsigned short status;
  unsigned char t[MAX_RSA_MODULUS_LEN];

  memset ((POINTER)t, 0, sizeof (t));
  for (status = 0; status < digits * NN_DIGIT_LEN;)
     t[status++] = (unsigned char)(rand()%255 + 1);
  NN_Decode (a, digits, t, (unsigned short)(digits * NN_DIGIT_LEN));

  /* Zeroize sensitive information.
   */
  memset ((POINTER)t, 0, sizeof (t));

  return (0);
}
