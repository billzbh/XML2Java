/* NN.H - header file for NN.C
 */

/* Copyright (C) 1991-2 RSA Laboratories, a division of RSA Data
   Security, Inc. All rights reserved.
 */

/* Type definitions.
 */
typedef unsigned char *POINTER;

/* UINT2 defines a two byte word */
typedef unsigned short UINT2;

/* UINT4 defines a four byte word */
typedef unsigned long UINT4;

typedef UINT4 NN_DIGIT;
typedef UINT2 NN_HALF_DIGIT;

/* Constants.

   Note: MAX_NN_DIGITS is long enough to hold any RSA modulus, plus
   one more digit as required by R_GeneratePEMKeys (for n and phiN,
   whose lengths must be even). All natural numbers have at most
   MAX_NN_DIGITS digits, except for double-length intermediate values
   in NN_Mult (t), NN_ModMult (t), NN_ModInv (w), and NN_Div (c).
 */
/* Length of digit in bits */
#define NN_DIGIT_BITS 32
#define NN_HALF_DIGIT_BITS 16
/* Length of digit in bytes */
#define NN_DIGIT_LEN (NN_DIGIT_BITS / 8)
/* Maximum length in digits */
#define MAX_NN_DIGITS \
  ((MAX_RSA_MODULUS_LEN + NN_DIGIT_LEN - 1) / NN_DIGIT_LEN + 1)
/* Maximum digits */
#define MAX_NN_DIGIT 0xffffffff
#define MAX_NN_HALF_DIGIT 0xffff

/* Macros.
 */
#define LOW_HALF(x) (NN_HALF_DIGIT)((x) & MAX_NN_HALF_DIGIT)
#define HIGH_HALF(x) \
  (NN_HALF_DIGIT)(((x) >> NN_HALF_DIGIT_BITS) & MAX_NN_HALF_DIGIT)
#define TO_HIGH_HALF(x) (((NN_DIGIT)(x)) << NN_HALF_DIGIT_BITS)
#define DIGIT_MSB(x) (unsigned short)(((x) >> (NN_DIGIT_BITS - 1)) & 1)
#define DIGIT_2MSB(x) (unsigned short)(((x) >> (NN_DIGIT_BITS - 2)) & 3)

/* CONVERSIONS
   NN_Decode (a, aDigits, b, bLen)   Decodes character string b into a.
   NN_Encode (a, aLen, b, bDigits)   Encodes b into character string a.

   ASSIGNMENTS
   NN_Assign (a, b, abDigits)        Assigns a = b.
   NN_ASSIGN_DIGIT (a, b, aDigits)   Assigns a = b, where b is a digit.
   NN_AssignZero (a, aDigits)        Assigns a = 0.
   NN_Assign2Exp (a, b, aDigits)     Assigns a = 2^b where b is unsigned short.
     
   ARITHMETIC OPERATIONS
   NN_Add (a, b, c, abcDigits)       Computes a = b + c.
   NN_Sub (a, b, c, abcDigits)       Computes a = b - c.
   NN_Mult (a, b, c, abcDigits)      Computes a = b * c.
   NN_Div(a, b, c, acDigits, d, bdDigits) Computes a = c div d and b = c mod d

   NN_Mod (a, b, bDigits, c, acDigits)    Computes a = b mod c.
   NN_ModMult (a, b, c, d, abcdDigits)    Computes a = b * c mod d.
   NN_ModExp (a, b, c, cDigits, d, abdDigits)  Computes a = b^c mod d.
   NN_ModInv (a, b, c, abcDigits)         Computes a = 1/b mod c.
   NN_Gcd (a, b, c, abcDigits)            Computes a = gcd (b, c).

   OTHER OPERATIONS
   NN_EVEN (a, aDigits)              Returns 1 iff a is even.
   NN_Cmp (a, b, abDigits)           Returns sign of a - b.
   NN_EQUAL (a, b, abDigits)         Returns 1 iff a = b.
   NN_Zero (a, aDigits)              Returns 1 iff a = 0.
   NN_Digits (a, aDigits)            Returns significant length of a in digits.
   NN_Bits (a, aDigits)              Returns significant length of a in bits.
*/
void NN_Decode (NN_DIGIT *, unsigned short, unsigned char *, unsigned short);
void NN_Encode (unsigned char *, unsigned short, NN_DIGIT *, unsigned short);

void NN_Assign (NN_DIGIT *, NN_DIGIT *, unsigned short);
void NN_AssignZero (NN_DIGIT *, unsigned short);
void NN_Assign2Exp (NN_DIGIT *, unsigned short, unsigned short);

NN_DIGIT NN_Add (NN_DIGIT *, NN_DIGIT *, NN_DIGIT *, unsigned short);
NN_DIGIT NN_Sub (NN_DIGIT *, NN_DIGIT *, NN_DIGIT *, unsigned short);
void NN_Mult (NN_DIGIT *, NN_DIGIT *, NN_DIGIT *, unsigned short);
void NN_Div (NN_DIGIT *, NN_DIGIT *, NN_DIGIT *, unsigned short, NN_DIGIT *,
             unsigned short);
void NN_Mod (NN_DIGIT *, NN_DIGIT *, unsigned short, NN_DIGIT *, unsigned short);
void NN_ModMult (NN_DIGIT *, NN_DIGIT *, NN_DIGIT *, NN_DIGIT *, unsigned short);
void NN_ModExp (NN_DIGIT *, NN_DIGIT *, NN_DIGIT *, unsigned short, NN_DIGIT *,
                unsigned short);
void NN_ModInv (NN_DIGIT *, NN_DIGIT *, NN_DIGIT *, unsigned short);
void NN_Gcd (NN_DIGIT *, NN_DIGIT *, NN_DIGIT *, unsigned short);

short NN_Cmp (NN_DIGIT *, NN_DIGIT *, unsigned short);
short NN_Zero (NN_DIGIT *, unsigned short);
unsigned short NN_bits (NN_DIGIT *, unsigned short);
unsigned short NN_Digits (NN_DIGIT *, unsigned short);

#define NN_ASSIGN_DIGIT(a, b, digits) {NN_AssignZero (a, digits); a[0] = b;}
#define NN_EQUAL(a, b, digits) (! NN_Cmp (a, b, digits))
#define NN_EVEN(a, digits) (((digits) == 0) || ! (a[0] & 1))
