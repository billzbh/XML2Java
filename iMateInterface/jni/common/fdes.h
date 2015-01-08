#ifndef __DES_H__
#define __DES_H__

#define ID_OK	0
#define RE_LEN	1

typedef unsigned short int UINT2;
typedef unsigned long int UINT4;

typedef struct {
  UINT4 subkeys[32];                                             /* subkeys */
  UINT4 iv[2];                                       /* initializing vector */
  UINT4 originalIV[2];                        /* for restarting the context */
  int encrypt;                                              /* encrypt flag */
} DES_CBC_CTX;

typedef struct {
  UINT4 subkeys[32];                                             /* subkeys */
  UINT4 iv[2];                                       /* initializing vector */
  UINT4 inputWhitener[2];                                 /* input whitener */
  UINT4 outputWhitener[2];                               /* output whitener */
  UINT4 originalIV[2];                        /* for restarting the context */
  int encrypt;                                              /* encrypt flag */
} DESX_CBC_CTX;

typedef struct {
  UINT4 subkeys[3][32];                     /* subkeys for three operations */
  UINT4 iv[2];                                       /* initializing vector */
  UINT4 originalIV[2];                        /* for restarting the context */
  int encrypt;                                              /* encrypt flag */
} DES3_CBC_CTX;

#ifdef __cplusplus
extern "C" {
#endif

void DES_CBCInit(DES_CBC_CTX *, unsigned char *, unsigned char *, int);
int DES_CBCUpdate(DES_CBC_CTX *, unsigned char *, unsigned char *, unsigned int);
void DES_CBCRestart(DES_CBC_CTX *);

void DESX_CBCInit(DESX_CBC_CTX *, unsigned char *, unsigned char *, int);
int DESX_CBCUpdate(DESX_CBC_CTX *, unsigned char *, unsigned char *, unsigned int);
void DESX_CBCRestart(DESX_CBC_CTX *);

void DES3_CBCInit(DES3_CBC_CTX *, unsigned char *, unsigned char *, int);
int DES3_CBCUpdate(DES3_CBC_CTX *, unsigned char *, unsigned char *, unsigned int);
int DES3_CBCUpdate2(DES3_CBC_CTX *context, unsigned char *output, unsigned char *input);
void DES3_CBCRestart(DES3_CBC_CTX *);
    
void _fDes(unsigned int uiMode, unsigned char *psSource, unsigned char *psKey, unsigned char *psResult);

#ifdef __cplusplus
}
#endif

#endif // __DES_H__
