# ifndef _SHA_H
# define _SHA_H

typedef struct {
    unsigned long state[5];
    unsigned long count[2];
    unsigned char buffer[64];
} SHA1_CTX;

#ifdef __cplusplus
extern "C" {
#endif

// 原型1
void SHA1Init(SHA1_CTX* context);
void SHA1Update(SHA1_CTX* context, unsigned char* data, unsigned short len);
void SHA1Final(unsigned char digest[20], SHA1_CTX* context);

// 原型2
void vSHA1Init(void);
void vSHA1Update(void *pBuffer, unsigned short iLen);
void vSHA1Result(void *pResult);

void vSHA1Result2(void *pResult);

// 原型3
void vSHA1(void *pBuffer, unsigned short iLen, void *pResult);

#ifdef __cplusplus
}
#endif

# endif
