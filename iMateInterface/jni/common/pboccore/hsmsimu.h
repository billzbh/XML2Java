/**************************************
File name     : HSMSIMU.H
Function      : 加密机处理函数
Author        : Yu Jun
First edition : Mar 18th, 2011
Modified      : 
**************************************/
#ifndef _HSMSIMU_H
#define _HSMSIMU_H

// 错误码, 同卫士通加密机
#define HSMERR_OK           0x00    // 成功
#define HSMERR_NO_MASTERKEY 0x01    // LMK不存在
#define HSMERR_MSG_LENGTH   0x02	// 消息长度错
#define HSMERR_RSA_E        0x08    // RSA密钥指数错误
#define HSMERR_NOT_ECBCBC   0x10    // 加/解密模式不是ECB/CBC
#define HSMERR_PIN_BLOCK    0x13    // PINBLOCK错误
#define HSMERR_PIN_FORMAT   0x14    // PINBLOCK格式错误
#define HSMERR_MAC_ALGO     0x24    // MAC算法错误
#define HSMERR_CHECK        0x2D    // 校验失败
#define HSMERR_PIN_LEN      0x31    // PIN长度错误
#define HSMERR_PIN          0x47    // PIN非法
#define HSMERR_SAVE_DES     0x52    // 保存次主密钥失败
#define HSMERR_KEY_LEN      0x5F    // 密钥长度错误
#define HSMERR_IN_DATA      0x67    // 输入数据错误
#define HSMERR_LEN_CHECK    0x68    // 长度与实际输入不符
#define HSMERR_INDEX        0x70    // 次主密钥索引越界
#define HSMERR_NO_DESKEY    0x72    // 次主密钥不存在
#define HSMERR_FLAG         0x74    // 标志错误
#define HSMERR_TYPE         0x75    // 密钥类型错误
#define HSMERR_DIV_NUM      0x76    // 分散次数错误
#define HSMERR_KEY_CHECK    0x81    // 校验值错误
#define HSMERR_MODULUS_LEN  0xB0    // RSA密钥模长错误
#define HSMERR_DERREF_PUB   0xB2    // 将DER格式公钥转换为REF格式失败
#define HSMERR_RSA_INDEX    0xB4    // RSA密钥索引越界
#define HSMERR_GEN_RSAKEY   0xB5    // 产生RSA密钥失败
#define HSMERR_REFDER_PUB   0xB6    // 将REF格式公钥转为DER格式失败
#define HSMERR_REFDER_PRI   0xBE    // 将REF格式私钥转换为DER格式失败
#define HSMERR_DERREF_PRI   0xBF    // 将DER格式私钥转换为REF格式失败
//#define HSMERR_RSA_INDEX    0xC1    // RSA密钥索引越界 卫士通指令错误码同时提供了C1及B4, 本程序采用B4
#define HSMERR_DATAIN_LEN   0xC2    // 输入数据长度错误
#define HSMERR_PASSWORD     0xC4    // 口令错
#define HSMERR_SAVE_RSA     0xC9    // 保存RSA密钥失败
#define HSMERR_NO_RSAKEY    0xCB    // RSA密钥不存在
#define HSMERR_MODE         0xE0    // Mode错误
#define HSMERR_RSA_FORMAT   0xF1    // 公/私钥格式错误
// 以下错误码卫士通加密机没有提供, 软加密机专用
#define HSMERR_INTERNAL     0xFA	// 内部错误
#define HSMERR_AC           0xFB	// AC校验失败
#define HSMERR_NO_ENCKEY    0xFC    // 加密密钥不存在
#define HSMERR_PAD_FORMAT   0xFD    // 填充模式非法
#define HSMERR_PARA         0xFE    // 参数错
#define HSMERR_INS          0xFF    // 指令不识别

int iHsmInit0(char *pszKeyFileName);
int iLmkProcess0(int iFlag, void *pIn, void *pOut);
int iKeyBackup0(int iKeyType, int iKeyIndex, int *piOutLen, void *pOutData);
int iKeyRestore0(int iKeyType, int iKeyIndex, int iInLen, void *pInData);
int iKeyDelete0(int iKeyType, int iKeyIndex);
int iKeyGenDesKey0(int iKeyIndex, void *pKey);
int iKeyGenRsaKey0(int iKeyIndex, int iKeyLen, long lE, void *pN, void *pKey, int *piOutLen);
int iKeyRsaPrivateBlock0(int iKeyIndex, void *pKey, int iLen, void *pIn, void *pOut);
int iKeyRsaPublicBlock0(int iKeyIndex, int iLen, void *pIn, void *pOut);
int iKeyRsaGetInfo0(int iKeyIndex, void *pKey, int *piLen, long *plE, void *pModulus);
int iKeyRsaPublicBlock20(int iLen, long lE, void *pN, void *pIn, void *pOut);
int iKeyCalExtAuth0(int iKey1Index, void *pKey1, int iDiv1Level, void *pDiv1Data, void *pTermRand, void *pCardData, void *pOut);
int iKeyCalChangeKmc0(int iKey1Index, void *pKey1, int iDiv1Level, void *pDiv1Data, void *pTermRand, void *pCardData, int iKey2Index, void *pKey2, int iDiv2Level, void *pDiv2Data, void *pOut);
int iKeyCalDesKey0(int iKey1Index, void *pKey1, int iDiv1Level, void *pDiv1Data, void *pTermRand, void *pCardData, int iKey2Index, void *pKey2, int iDiv2Level, void *pDiv2Data, void *pOut);
int iKeyCalPin0(int iKey1Index, void *pKey1, int iDiv1Level, void *pDiv1Data, void *pTermRand, void *pCardData, void *pPinBlock, void *pOut);
int iKeyCalRsaKey0(int iKey1Index, void *pKey1, int iDiv1Level, void *pDiv1Data, void *pTermRand, void *pCardData, int iRsaKeyLen, long lE, void *pOut);
int iKeyVerifyAc0(int iKey1Index, void *pKey1, int iDiv1Level, void *pDiv1Data, void *pATC, int iTransDataLen, void *pTransData, void *pARQC, void *pARC, void *pARPC);
int iKeyScriptEncData0(int iKey1Index, void *pKey1, int iDiv1Level, void *pDiv1Data, void *pATC, int iPinFlag, int iDataLen, void *pData, int *piOutLen, void *pOutData);
int iKeyScriptMac0(int iKey1Index, void *pKey1, int iDiv1Level, void *pDiv1Data, void *pATC, int iDataLen, void *pData, void *pMac);
int iKeyReEncData0(int iKey1Index, void *pKey1, int iDiv1Level, void *pDiv1Data, void *pTermRand, void *pCardData, int iKey2Index, void *pKey2, int iDiv2Level, void *pDiv2Data, int iDataLen, void *pData, void *pOut);
int iKeyReEncPin0(int iKey1Index, void *pKey1, int iDiv1Level, void *pDiv1Data, void *pTermRand, void *pCardData, int iKey2Index, void *pKey2, int iDiv2Level, void *pDiv2Data, void *pPinBlock, void *pOut);
int iKeyDpGenDesKey0(int iKey1Index, void *pKey1, int iDiv1Level, void *pDiv1Data, void *pOut);
int iKeyDpCalMac0(int iKey1Index, void *pKey1, int iDiv1Level, void *pDiv1Data, void *pMacKey, void *pIv, int iDataLen, void *pData, void *pMac, int iBlockFlag);
int iKeyDpCalMacFull0(int iKey1Index, void *pKey1, int iDiv1Level, void *pDiv1Data, void *pMacKey, int iDiv2Level, void *pDiv2Data, int iAlgoFlag, void *pIv, int iDataLen, void *pData, void *pMac, int iBlockFlag);
int iKeyDpCalDesKey0(int iKey1Index, void *pKey1, int iDiv1Level, void *pDiv1Data, int iKey2Index, void *pKey2, int iDiv2Level, void *pDiv2Data, void *pOut);
int iKeyDpCalRsaKey0(int iKey1Index, void *pKey1, int iDiv1Level, void *pDiv1Data, int iRsaKeyLen, long lE, void *pOut);
int iKeyEncDec0(int iKey1Index, void *pKey1, int iDiv1Level, void *pDiv1Data, void *pKey2, int iDiv2Level, void *pDiv2Data, int iEncDecFlag, int iPadFlag, void *pIv, int iDataLen, void *pData, int *piOutLen, void *pOutData);
int iKeyEncDecEcb0(int iKey1Index, void *pKey1, int iDiv1Level, void *pDiv1Data, int iFlag, int iDataLen, void *pData, void *pOut);
int iKeyDecEncData0(int iKey1Index, void *pKey1, int iDiv1Level, void *pDiv1Data, int iKey1IndexP, void *pKey1P, int iDiv1LevelP, void *pDiv1DataP, void *pIv1,
                    int iKey2Index, void *pKey2, int iDiv2Level, void *pDiv2Data, int iKey2IndexP, void *pKey2P, int iDiv2LevelP, void *pDiv2DataP, void *pIv2,
                    int iDataLen, void *pData, void *pOut);

//QINGBO
void vKeyCalSetDivMode(int iDivMode);
void vHsmChangeKmcKey(uchar *psNewKey);

#endif
