/* R_KEYGEN.H - header file for RSAREF key-pair generation
 */

/* Copyright (C) 1991-2 RSA Laboratories, a division of RSA Data
   Security, Inc. All rights reserved.
 */

/* Key-pair generation.
 */
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */
/* Generates an RSA key pair with a given length and public exponent.
 */
// in : R_RSA_PROTO_KEY *protoKey;           /* RSA prototype key */
//						protoKey->useFermat4 /* 1:public exponent 65537, 0:public exponent 3 */
// out: R_RSA_PUBLIC_KEY *publicKey;         /* new RSA public key */
//      R_RSA_PRIVATE_KEY *privateKey;       /* new RSA private key */
// note:refer Rsaref.h for definition
short R_GeneratePEMKeys(R_RSA_PUBLIC_KEY *pPublicKey,
						R_RSA_PRIVATE_KEY *pPrivateKey,
                        R_RSA_PROTO_KEY *pProtoKey);
#ifdef __cplusplus
};
#endif /* __cplusplus */
