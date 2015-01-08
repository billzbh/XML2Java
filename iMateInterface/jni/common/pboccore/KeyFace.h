/**************************************
File name     : KeyFace.h
Function      : PBOC2.0 密钥访问接口模块
                本接口提供了与加密机无关的PBOC2.0密钥访问接口
                密钥可以存在于 1:模块内部 2:配置文件 3:华信软加密机 4:卫士通加密机
Author        : Yu Jun
First edition : Jul 5th, 2011
**************************************/
#ifndef _KEYFACE_H
#define _KEYFACE_H
#ifdef __cplusplus
extern "C" {
#endif

// Key Face Ret value
#define KFR_OK                  0x0000 // 成功
#define KFR_PARA                0x0001 // 参数错误
#define KFR_COMM                0x0002 // 通讯失败
#define KFR_INTERNAL            0x0003 // 内部错误
#define KFR_DATA_CHK            0x0004 // 加密机数据完整性检查错误
#define KFR_KEY_TYPE            0x0005 // 密钥类型不支持
#define KFR_RSA_PARSE           0x0006 // RSA公钥解析错或结果非预期
#define KFR_OTHER               0x00FF // 未知错误
#define KFR_WESTONE             0x1000 // 0x10XX表示卫士通加密机错误, XX为错误代码

// 设置参数
// in  : iMode    : 模式 0:模块内密钥模式 1:本地配置文件模式 2:卫士通加密机模式
//       pszIp    : 加密机IP地址, 模式2与模式3可用
//       iPortNo  : 加密机端口号, 模式2与模式3可用
// ret : KFR_OK   : 成功
//       其它     : 失败
int iKeySetMode(int iMode, char *pszIp, int iPortNo);

// 密钥备份
// in  : iKeyType  : 1:DES密钥 2:RSA密钥
//       iKeyIndex : 密钥索引
// out : piOutLen  : 备份数据长度
//       pOutData  : 备份数据
// ret : KFR_OK    : 成功
//       其它      : 失败
int iKeyBackup(int iKeyType, int iKeyIndex, int *piOutLen, void *pOutData);

// 密钥恢复
// in  : iKeyType  : 1:DES密钥 2:RSA密钥
//       iKeyIndex : 密钥索引
//       iInLen    : 备份数据长度
//       pInData   : 备份数据
// ret : KFR_OK    : 成功
//       其它      : 失败
int iKeyRestore(int iKeyType, int iKeyIndex, int iInLen, void *pInData);

// 密钥删除
// in  : iKeyType  : 1:DES密钥 2:RSA密钥
//       iKeyIndex : 密钥索引
// ret : KFR_OK    : 成功
//       其它      : 失败
int iKeyDelete(int iKeyType, int iKeyIndex);

// 生成DES密钥
// in  : iKeyIndex : 密钥索引(0-999), -1表示不存储
//       pKey      : LMK加密后输出, NULL表示不输出
// out : pKey      : LMK加密后新密钥[16]+cks[8], NULL表示不输出
// ret : KFR_OK    : 成功
//       其它      : 失败
// Note: iKeyIndex为-1时pKey不能为NULL
int iKeyNewDesKey(int iKeyIndex, void *pKey);

// 生成RSA密钥
// in  : iKeyIndex : 密钥索引(0-19), -1表示不存储
//       iKeyLen   : 密钥长度, 单位:字节 64-248, 必须为偶数
//       lE        : 公共指数, 3或65537
//       pKey      : LMK加密后新密钥, NULL表示不输出
// out : pModulus  : 明文模数，长度为iKeyLen, NULL表示不输出
//       piOutLen  : 输出的加密后密钥长度
//       pKey      : LMK加密后新密钥, NULL表示不输出
// ret : KFR_OK    : 成功
//       其它      : 失败
// Note: iKeyIndex为-1时pKey不能为NULL
int iKeyNewRsaKey(int iKeyIndex, int iKeyLen, long lE, void *pModulus, int *piOutLen, void *pKey);

// RSA私钥运算
// in  : iKeyIndex : RSA密钥索引
//       iKeyLen   : LMK加密后RSA私钥长度
//       pKey      : LMK加密后RSA私钥, NULL表示使用索引密钥
//       iLen      : 运算数据长度
//       pIn       : 运算数据
// out : pOut      : 运算结果, 长度等于iLen
// ret : KFR_OK    : 成功
//       其它      : 失败
int iKeyRsaPrivateBlock(int iKeyIndex, int iKeyLen, void *pKey, int iLen, void *pIn, void *pOut);

// RSA公钥运算
// in  : iKeyIndex : RSA密钥索引
//       iKeyLen   : 模数长度(字节)
//       pModulus  : 模数, NULL表示使用索引密钥
//       lE        : 公共指数, pMudulus!=NULL时使用
//       iLen      : 运算数据长度
//       pIn       : 运算数据
// out : pOut      : 运算结果, 长度等于iLen
// ret : KFR_OK    : 成功
//       其它      : 失败
int iKeyRsaPublicBlock(int iKeyIndex, int iKeyLen, void *pModulus, long lE, int iLen, void *pIn, void *pOut);

// 获取RSA公钥
// in  : iKeyIndex : RSA密钥索引
// out : piLen     : RSA密钥长度, 单位:字节
//       plE       : 公共指数
//       pModulus  : 模数
// ret : KFR_OK    : 成功
//       其它      : 失败
int iKeyRsaGetInfo(int iKeyIndex, int *piLen, long *plE, void *pModulus);

// 产生卡片外部认证密文
// in  : iKey1Index : 密钥索引
//       iKey1Len   : LMK加密后密钥长度
//       pKey1      : LMK加密后密钥, NULL表示使用索引密钥
//       iDiv1Level : 密钥分散次数(0-3)
//       pDiv1Data  : 密钥分散数据[8*n]
//       pTermRand  : 终端随机数[8]
//       pCardData  : 卡片数据[28]
// out : pOut       : 外部认证密文及Mac[16]
// ret : KFR_OK     : 成功
//       其它       : 失败
int iKeyCalExtAuth(int iKey1Index, int iKey1Len, void *pKey1, int iDiv1Level, void *pDiv1Data, void *pTermRand, void *pCardData, void *pOut);

// 生成修改KMC密文
// in  : iKey1Index : 原密钥索引
//       iKey1Len   : LMK加密后原密钥长度
//       pKey1      : LMK加密后原密钥, NULL表示使用索引密钥
//       iDiv1Level : 原密钥分散次数(0-3)
//       pDiv1Data  : 原密钥分散数据[8*n]
//       pTermRand  : 终端随机数[8]
//       pCardData  : 卡片数据[28]
//       iKey2Index : 新密钥索引
//       iKey2Len   : LMK加密后新密钥长度
//       pKey2      : LMK加密后新密钥, NULL表示使用索引密钥
//       iDiv2Level : 新密钥分散次数(0-3)
//       pDiv2Data  : 新密钥分散数据[8*n]
// out : pOut       : 密文AuthKey[16]+AuthKeyKcv[3]+密文MacKey[16]+MacKeyKcv[3]+密文EncKey[16]+EncKeyKcv[3]
// ret : KFR_OK     : 成功
//       其它       : 失败
int iKeyCalChangeKmc(int iKey1Index, int iKey1Len, void *pKey1, int iDiv1Level, void *pDiv1Data, void *pTermRand, void *pCardData, int iKey2Index, int iKey2Len, void *pKey2, int iDiv2Level, void *pDiv2Data, void *pOut);

// 产生IC卡DES密钥密文(即时发卡)
// in  : iKey1Index : 主控密钥索引
//       iKey1Len   : LMK加密后主控密钥长度
//       pKey1      : LMK加密后主控密钥, NULL表示使用索引密钥
//       iDiv1Level : 主控密钥分散次数(0-3)
//       pDiv1Data  : 主控密钥分散数据[8*n]
//       pTermRand  : 终端随机数[8]
//       pCardData  : 卡片数据[28]
//       iKey2Index : 应用密钥索引
//       iKey2Len   : LMK加密后应用密钥长度
//       pKey2      : LMK加密后应用密钥, NULL表示使用索引密钥
//       iDiv2Level : 应用密钥分散次数(0-3)
//       pDiv2Data  : 应用密钥分散数据[8*n]
// out : pOut       : 密文Key[16]+KeyKcv[3]
// ret : KFR_OK     : 成功
//       其它       : 失败
int iKeyCalDesKey(int iKey1Index, int iKey1Len, void *pKey1, int iDiv1Level, void *pDiv1Data, void *pTermRand, void *pCardData, int iKey2Index, int iKey2Len, void *pKey2, int iDiv2Level, void *pDiv2Data, void *pOut);

// 产生IC卡PIN密文(即时发卡)
// in  : iKey1Index : 主控密钥索引
//       iKey1Len   : LMK加密后主控密钥长度
//       pKey1      : LMK加密后主控密钥, NULL表示使用索引密钥
//       iDiv1Level : 主控密钥分散次数(0-3)
//       pDiv1Data  : 主控密钥分散数据[8*n]
//       pTermRand  : 终端随机数[8]
//       pCardData  : 卡片数据[28]
//       pPinBlock  : 个人密码数据块[8]
// out : pOut       : 密文个人密码数据块[8]
// ret : KFR_OK     : 成功
//       其它       : 失败
int iKeyCalPin(int iKey1Index, int iKey1Len, void *pKey1, int iDiv1Level, void *pDiv1Data, void *pTermRand, void *pCardData, void *pPinBlock, void *pOut);

// 生成IC卡RSA密钥密文(即时发卡)
// in  : iKey1Index : 主控密钥索引
//       iKey1Len   : LMK加密后主控密钥长度
//       pKey1      : LMK加密后主控密钥, NULL表示使用索引密钥
//       iDiv1Level : 主控密钥分散次数(0-3)
//       pDiv1Data  : 主控密钥分散数据[8*n]
//       pTermRand  : 终端随机数[8]
//       pCardData  : 卡片数据[28]
//       iRsaKeyLen : RSA密钥长度, 单位:字节, 64-248, 必须为偶数
//       lE         : 公共指数值, 3或者65537
// out : pOut       : RSA密钥公钥明文[n]+D密文[n1]+P密文[n2]+Q密文[n2]+dP密文[n2]+dQ密文[n2]+qInv密文[n2]
//                    n = iRsaKeyLen, n1 = (iRsaKeyLen+8)/8*8, n2 = (iRsaKeyLen/2+8)/8*8
// ret : KFR_OK     : 成功
//       其它       : 失败
int iKeyCalRsaKey(int iKey1Index, int iKey1Len, void *pKey1, int iDiv1Level, void *pDiv1Data, void *pTermRand, void *pCardData, int iRsaKeyLen, long lE, void *pOut);

// 验证ARQC、TC, 产生ARPC
// in  : iKey1Index     : 密钥索引
//       iKey1Len       : LMK加密后密钥长度
//       pKey1          : LMK加密后密钥, NULL表示使用索引密钥
//       iDiv1Level     : 密钥分散次数(0-3)
//       pDiv1Data      : 密钥分散数据[8*n]
//       pATC           : 交易序列号[2]
//       iTransDataLen  : 交易数据长度
//       pTransData     : 交易数据
//       pARQC          : ARQC或TC
//       pARC           : 交易应答码, NULL表示只验证ARQC或TC
// out : pARPC          : ARPC
// ret : KFR_OK         : 成功
//       其它           : 失败
int iKeyVerifyAc(int iKey1Index, int iKey1Len, void *pKey1, int iDiv1Level, void *pDiv1Data, void *pATC, int iTransDataLen, void *pTransData, void *pARQC, void *pARC, void *pARPC);

// 用过程密钥加密数据(脚本)
// in  : iKey1Index    : 密钥索引
//       iKey1Len      : LMK加密后密钥长度
//       pKey1         : LMK加密后密钥, NULL表示使用索引密钥
//       iDiv1Level    : 密钥分散次数(0-3)
//       pDiv1Data     : 密钥分散数据[8*n]
//       pATC          : 交易序列号[2]
//       iPinFlag      : PinBlock加密标志, 0:不是PinBlock, 1:是PinBlock
//       iDataLen      : 数据长度
//       pData         : 数据
// out : piOutLen      : 输出数据长度
//       pOutData      : 输出数据
// ret : KFR_OK        : 成功
//       其它          : 失败
int iKeyScriptEncData(int iKey1Index, int iKey1Len, void *pKey1, int iDiv1Level, void *pDiv1Data, void *pATC, int iPinFlag, int iDataLen, void *pData, int *piOutLen, void *pOutData);

// 用过程密钥计算Mac(脚本)
// in  : iKey1Index    : 密钥索引
//       iKey1Len      : LMK加密后密钥长度
//       pKey1         : LMK加密后密钥, NULL表示使用索引密钥
//       iDiv1Level    : 密钥分散次数(0-3)
//       pDiv1Data     : 密钥分散数据[8*n]
//       pATC          : 交易序列号[2]
//       iDataLen      : 数据长度
//       pData         : 数据
// out : pMac          : Mac[8]
// ret : KFR_OK        : 成功
//       其它          : 失败
int iKeyScriptMac(int iKey1Index, int iKey1Len, void *pKey1, int iDiv1Level, void *pDiv1Data, void *pATC, int iDataLen, void *pData, void *pMac);

// 加密数据密钥转换(数据准备文件发卡)
// in  : iKey1Index : 主控密钥索引
//       iKey1Len   : LMK加密后主控密钥长度
//       pKey1      : LMK加密后主控密钥, NULL表示使用索引密钥
//       iDiv1Level : 主控密钥分散次数(0-3)
//       pDiv1Data  : 主控密钥分散数据[8*n]
//       pTermRand  : 终端随机数[8]
//       pCardData  : 卡片数据[28]
//       iKey2Index : 应用密钥索引
//       iKey2Len   : LMK加密后应用密钥长度
//       pKey2      : LMK加密后应用密钥, NULL表示使用索引密钥
//       iDiv2Level : 应用密钥分散次数(0-3)
//       pDiv2Data  : 应用密钥分散数据[8*n]
//       iDataLen   : 原加密数据长度, 必须为8的倍数
//       pData      : 原加密数据
// out : pOut       : 转加密后数据, 长度等于原加密数据长度
// ret : KFR_OK     : 成功
//       其它       : 失败
// Note: 数据原为应用密钥加密, 用主控密钥的过程密钥转加密
int iKeyReEncData(int iKey1Index, int iKey1Len, void *pKey1, int iDiv1Level, void *pDiv1Data, void *pTermRand, void *pCardData, int iKey2Index, int iKey2Len, void *pKey2, int iDiv2Level, void *pDiv2Data, int iDataLen, void *pData, void *pOut);

// 加密密码密钥转换(数据准备文件发卡)
// in  : iKey1Index : 主控密钥索引
//       iKey1Len   : LMK加密后主控密钥长度
//       pKey1      : LMK加密后主控密钥, NULL表示使用索引密钥
//       iDiv1Level : 主控密钥分散次数(0-3)
//       pDiv1Data  : 主控密钥分散数据[8*n]
//       pTermRand  : 终端随机数[8]
//       pCardData  : 卡片数据[28]
//       iKey2Index : 应用密钥索引
//       iKey2Len   : LMK加密后应用密钥长度
//       pKey2      : LMK加密后应用密钥, NULL表示使用索引密钥
//       iDiv2Level : 应用密钥分散次数(0-3)
//       pDiv2Data  : 应用密钥分散数据[8*n]
//       iPinFormat : 密码块格式, 1:格式 2:格式2
//       pPinBlock  : 原加密后密码块[8]
// out : pOut       : 转加密后密码块[8]
// ret : KFR_OK     : 成功
//       其它       : 失败
// Note: 数据原为应用密钥加密, 用主控密钥的过程密钥转加密
int iKeyReEncPin(int iKey1Index, int iKey1Len, void *pKey1, int iDiv1Level, void *pDiv1Data, void *pTermRand, void *pCardData, int iKey2Index, int iKey2Len, void *pKey2, int iDiv2Level, void *pDiv2Data, int iPinFormat, void *pPinBlock, void *pOut);

// Mac计算(数据准备)
// in  : iKey1Index  : 应用密钥索引
//       iKey1Len    : LMK加密后应用密钥长度
//       pKey1       : LMK加密后应用密钥, NULL表示使用索引密钥
//       iDiv1Level  : 应用密钥分散次数(0-3)
//       pDiv1Data   : 应用密钥分散数据[8*n]
//       pMacKey     : 被应用密钥加密后的Mac密钥
//       pIv         : 初始向量[8]
//       iDataLen    : 数据长度, 必须为8的倍数
//       pData       : 数据
// out : pMacKey     : 被应用密钥加密后的随机临时密钥[16]
//       pMac        : Mac[8]
// ret : KFR_OK      : 成功
//       其它        : 失败
int iKeyDpCalMac(int iKey1Index, int iKey1Len, void *pKey1, int iDiv1Level, void *pDiv1Data, void *pIv, int iDataLen, void *pData, void *pMacKey, void *pMac);

// 生成数据准备DES密钥密文(数据准备)
// in  : iKey1Index  : TK密钥索引
//       iKey1Len    : LMK加密后TK密钥长度
//       pKey1       : LMK加密后TK密钥, NULL表示使用索引密钥
//       iDiv1Level  : TK密钥分散次数(0-3)
//       pDiv1Data   : TK密钥分散数据[8*n]
//       iKey2Index  : 应用密钥索引
//       iKey2Len    : LMK加密后应用密钥长度
//       pKey2       : LMK加密后应用密钥, NULL表示使用索引密钥
//       iDiv2Level  : 应用密钥分散次数(0-3)
//       pDiv2Data   : 应用密钥分散数据[8*n]
// out : pOut        : OutKey[16]+Cks[3]
// ret : KFR_OK     : 成功
//       其它       : 失败
int iKeyDpCalDesKey(int iKey1Index, int iKey1Len, void *pKey1, int iDiv1Level, void *pDiv1Data, int iKey2Index, int iKey2Len, void *pKey2, int iDiv2Level, void *pDiv2Data, void *pOut);

// 生成数据准备RSA密钥密文(数据准备)
// in  : iKey1Index  : TK密钥索引
//       iKey1Len    : LMK加密后TK密钥长度
//       pKey1       : LMK加密后TK密钥, NULL表示使用索引密钥
//       iDiv1Level  : TK密钥分散次数(0-3)
//       pDiv1Data   : TK密钥分散数据[8*n]
//       iRsaKeyLen  : RSA密钥长度, 单位:字节, 64-248, 必须为偶数
//       lE          : 公共指数值, 3或者65537
// out : pOut        : RSA密钥公钥明文[n]+D密文[n1]+P密文[n2]+Q密文[n2]+dP密文[n2]+dQ密文[n2]+qInv密文[n2]
//                     n = iRsaKeyLen, n1 = (iRsaKeyLen+8)/8*8, n2 = (iRsaKeyLen/2+8)/8*8
// ret : KFR_OK      : 成功
//       其它        : 失败
int iKeyDpCalRsaKey(int iKey1Index, int iKey1Len, void *pKey1, int iDiv1Level, void *pDiv1Data, int iRsaKeyLen, long lE, void *pOut);

#ifdef __cplusplus
}
#endif
#endif
