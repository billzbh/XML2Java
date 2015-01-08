/**************************************
File name     : KeyFace.c
Function      : PBOC2.0 密钥访问接口模块
                本接口提供了与加密机无关的PBOC2.0密钥访问接口
Author        : Yu Jun
Desc          : KeyFace.c
                HsmProc.c
                HsmSimu.c
                Pub.c R_KeyGen.c Rsa.c SockComm.c Des.c nn.c
First edition : Jul 5th, 2011
**************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdarg.h>
#include "VposFace.h"
#include "nn.h"
#include "Rsaref.h"
#include "hsmproc.h"
#include "KeyFace.h"

static int sg_iMode = 0;    // 模式, 0:模块内密钥模式 1:本地配置文件模式 2:卫士通加密机模式

// 将模数与指数打包成DER格式公钥
// in  : iKeyLen     : 模数长度(字节)
//       pModulus    : 模数
//       lE          : 指数
// out : piDerKeyLen : DER格式公钥长度
//       pDerKey     : DER格式公钥
// ret : KFR_OK      : OK
//       KFR_PARA    : 参数错误
static int iPackPublicKey(int iKeyLen, void *pModulus, long lE, int *piDerKeyLen, void *pDerKey)
{
    int iLen, iDerNLen;
    unsigned char sBuf[300];
    unsigned char *psDerN;
    
    if(iKeyLen<4 || iKeyLen>248)
        return(KFR_PARA);
    if(lE!=3 && lE!=65537L)
        return(KFR_PARA);
    memset(sBuf, 0, sizeof(sBuf));
    iLen = 0;
    sBuf[iLen++] = 0x02;
    if(iKeyLen+1 < 0x80)
        sBuf[iLen++] = iKeyLen+1; // 模数前面补一个0, 以保持模数为正数
    else if(iKeyLen+1 < 0x100) {
        sBuf[iLen++] = 0x81;
        sBuf[iLen++] = iKeyLen+1;
    } else {
        sBuf[iLen++] = 0x82;
        vLongToStr(iKeyLen+1, 2, sBuf+iLen);
        iLen += 2;
    }
    memcpy(sBuf+iLen+1, pModulus, iKeyLen);
    iLen += iKeyLen+1;
    if(lE == 3) {
        memcpy(sBuf+iLen, "\x02\x01\x03", 3);
        iLen += 3;
    } else {
        memcpy(sBuf+iLen, "\x02\x03\x01\x00\x01", 5);
        iLen += 5;
    }
    // 将sBuf(长度=iLen)打包到T30
    psDerN = (unsigned char *)pDerKey;
    iDerNLen = 0;
    psDerN[iDerNLen++] = 0x30;
    if(iLen < 80)
        psDerN[iDerNLen++] = iLen;
    else if(iLen < 0x100) {
        psDerN[iDerNLen++] = 0x81;
        psDerN[iDerNLen++] = iLen;
    } else {
        psDerN[iDerNLen++] = 0x82;
        vLongToStr(iLen, 2, psDerN+iDerNLen);
        iDerNLen += 2;
    }
    memcpy(psDerN+iDerNLen, sBuf, iLen);
    iDerNLen += iLen;
    *piDerKeyLen = iDerNLen;
    return(KFR_OK);
}
// 将DER格式公钥分解成模数与指数
// in  : piDerKeyLen : DER格式公钥长度
//       pDerKey     : DER格式公钥
// out : piKeyLen    : 模数长度(字节)
//       pModulus    : 模数
//       plE         : 指数
// ret : KFR_OK      : OK
//       KFR_PARA    : 参数错误
static int iUnpackPublicKey(int iDerKeyLen, void *pDerKey, int *piKeyLen, void *pModulus, long *plE)
{
    int iLen, iNLen, iTmp;
    unsigned char *psDerN;

    if(iDerKeyLen<4 || iDerKeyLen>270)
        return(KFR_PARA);
    psDerN = (unsigned char *)pDerKey;
    if(*psDerN++ != 0x30)
        return(KFR_PARA);
    if(psDerN[0] & 0x80) {
        iTmp = psDerN[0] & 0x7F;
        iLen = ulStrToLong(psDerN+1, iTmp);
        psDerN += 1+iTmp;
        if(iDerKeyLen != 2+iTmp+iLen)
            return(KFR_PARA);
    } else {
        iLen = psDerN[0];
        psDerN ++;
        if(iDerKeyLen != 2+iLen)
            return(KFR_PARA);
    }
    // psDerN现在指向模数TLV对象与公共指数TLV对象, iLen为模数TLV对象与公共指数TLV对象长度之和
    // 先解出公共指数
    if(iLen < 5)
        return(KFR_PARA);
    if(memcmp(psDerN+iLen-5, "\x02\x03\x01\x00\x01", 5) == 0) {
        *plE = 65537L;
        iLen -= 5;
    } else if(memcmp(psDerN+iLen-3, "\x02\x01\x03", 3) == 0) {
        *plE = 3;
        iLen -= 3;
    } else
        return(KFR_PARA);
    // iLen为模数TLV对象长度
    // 再解出模数
    if(*psDerN++ != 0x02)
        return(KFR_PARA);
    if(psDerN[0] & 0x80) {
        iTmp = psDerN[0] & 0x7F;
        iNLen = ulStrToLong(psDerN+1, iTmp);
        psDerN += 1+iTmp;
        if(iLen != 2+iTmp+iNLen)
            return(KFR_PARA);
    } else {
        iNLen = *psDerN++;
        if(iLen != 2+iNLen)
            return(0);
    }
    if(iNLen > 248)
        return(KFR_PARA);
    memcpy(pModulus, psDerN+1, iNLen-1);
    *piKeyLen = iNLen - 1;
    return(KFR_OK);
}

// 设置参数
// in  : iMode    : 模式 0:模块内密钥模式 1:本地配置文件模式 2:卫士通加密机模式
//       pszIp    : 加密机IP地址, 模式2与模式3可用
//       iPortNo  : 加密机端口号, 模式2与模式3可用
// ret : KFR_OK   : 成功
//       其它     : 失败
int iKeySetMode(int iMode, char *pszIp, int iPortNo)
{
    int iRet;

    if(iMode > 2)
        return(KFR_PARA); // 不支持的模式
    sg_iMode = iMode;
    iRet = iHsmSetMode(iMode, pszIp, iPortNo);
    if(iRet == 0)
        return(KFR_OK);
    if(iRet == -1)
        return(KFR_COMM);
    if(iRet == -2)
        return(KFR_INTERNAL);
    if(iRet == -3)
        return(KFR_PARA);
    return(KFR_OTHER);
}

// 密钥备份
// in  : iKeyType  : 1:DES密钥 2:RSA密钥
//       iKeyIndex : 密钥索引
// out : piOutLen  : 备份数据长度
//       pOutData  : 备份数据
// ret : KFR_OK    : 成功
//       其它      : 失败
int iKeyBackup(int iKeyType, int iKeyIndex, int *piOutLen, void *pOutData)
{
	uchar sSendBuf[2048], sRecvBuf[2048];
    uchar *p;
	int   iSendLen, iRecvLen;
	int   iRet;

    if(sg_iMode <= 2) {
        // 模式0、1、2采用同一接口(卫士通加密机接口)
        p = sSendBuf;
        switch(iKeyType) {
        case 1: // DES密钥
            vTwoOne("D230", 4, p);
            p += 2;
            *p++ = 0x04; // 0x04:用LMK保护
            vLongToStr(iKeyIndex, 2, p);
            p += 2;
            iSendLen = p - sSendBuf;
            iRecvLen = sizeof(sRecvBuf);
            iRet = iHsmProc(iSendLen, sSendBuf, &iRecvLen, sRecvBuf);
            if(iRet)
                return(KFR_COMM);
            if(sRecvBuf[0] != 'A') {
                return(KFR_WESTONE+sRecvBuf[1]);
            }
            if(iRecvLen != 1+1+16+8)
                return(KFR_DATA_CHK);
            *piOutLen = 1+16+8;
            memcpy(pOutData, sRecvBuf+1, *piOutLen);
            break;
        case 2: // RSA密钥
            vTwoOne("D233", 4, p);
            p += 2;
            vLongToStr(iKeyIndex, 2, p);
            p += 2;
            *p++ = 1; // 1:取出私钥
            *p++ = 1; // 1:私钥格式为DER
            memcpy(p, "11111111", 8);
            p += 8;
            iSendLen = p - sSendBuf;
            iRecvLen = sizeof(sRecvBuf);
            iRet = iHsmProc(iSendLen, sSendBuf, &iRecvLen, sRecvBuf);
            if(iRet)
                return(KFR_COMM);
            if(sRecvBuf[0] != 'A')
                return(KFR_WESTONE+sRecvBuf[1]);
            *piOutLen = ulStrToLong(sRecvBuf+1, 2);
            if(iRecvLen != 1+2+*piOutLen)
                return(KFR_DATA_CHK);
            memcpy(pOutData, sRecvBuf+3, *piOutLen);
            break;
        default:
            return(KFR_KEY_TYPE);
        }
    } // if(sg_iMode <= 2) {
    return(KFR_OK);
}

// 密钥恢复
// in  : iKeyType  : 1:DES密钥 2:RSA密钥
//       iKeyIndex : 密钥索引
//       iInLen    : 备份数据长度
//       pInData   : 备份数据
// ret : KFR_OK    : 成功
//       其它      : 失败
int iKeyRestore(int iKeyType, int iKeyIndex, int iInLen, void *pInData)
{
	uchar sSendBuf[2048], sRecvBuf[2048];
    uchar *p;
	int   iSendLen, iRecvLen;
	int   iRet;

    if(sg_iMode <= 2) {
        // 模式0、1、2采用同一接口(卫士通加密机接口)
        p = sSendBuf;
        switch(iKeyType) {
        case 1: // DES密钥
            vTwoOne("D231", 4, p);
            p += 2;
            *p++ = 0x04; // 0x04:用LMK保护
            vLongToStr(iKeyIndex, 2, p);
            p += 2;
            memcpy(p, pInData, iInLen);
            p += iInLen;
            iSendLen = p - sSendBuf;
            iRecvLen = sizeof(sRecvBuf);
            iRet = iHsmProc(iSendLen, sSendBuf, &iRecvLen, sRecvBuf);
            if(iRet)
                return(KFR_COMM);
            if(sRecvBuf[0] != 'A')
                return(KFR_WESTONE+sRecvBuf[1]);
            if(iRecvLen != 1)
                return(KFR_DATA_CHK);
            break;
        case 2: // RSA密钥
            vTwoOne("D234", 4, p);
            p += 2;
            vLongToStr(iKeyIndex, 2, p);
            p += 2;
            memcpy(p, "11111111", 8);
            p += 8;
            vLongToStr(iInLen, 2, p);
            p += 2;
            memcpy(p, pInData, iInLen);
            p += iInLen;
            *p++ = 1; // 1:DER格式
            iSendLen = p - sSendBuf;
            iRecvLen = sizeof(sRecvBuf);
            iRet = iHsmProc(iSendLen, sSendBuf, &iRecvLen, sRecvBuf);
            if(iRet)
                return(KFR_COMM);
            if(sRecvBuf[0] != 'A')
                return(KFR_WESTONE+sRecvBuf[1]);
            if(iRecvLen != 1)
                return(KFR_DATA_CHK);
            break;
        default:
            return(KFR_KEY_TYPE);
        }
    } // if(sg_iMode <= 2) {
    return(KFR_OK);
}

// 密钥删除
// in  : iKeyType  : 1:DES密钥 2:RSA密钥
//       iKeyIndex : 密钥索引
// ret : KFR_OK    : 成功
//       其它      : 失败
int iKeyDelete(int iKeyType, int iKeyIndex)
{
	uchar sSendBuf[2048], sRecvBuf[2048];
    uchar *p;
	int   iSendLen, iRecvLen;
	int   iRet;

    if(sg_iMode <= 2) {
        // 模式0、1、2采用同一接口(卫士通加密机接口)
        p = sSendBuf;
        vTwoOne("D22A", 4, p);
        p += 2;
        if(iKeyType == 1)
            *p++ = 2; // 2:次主密钥
        else if(iKeyType == 2)
            *p++ = 1; // 1:RSA密钥
        else
            return(KFR_KEY_TYPE);
        vLongToStr(iKeyIndex, 2, p);
        p += 2;
        iSendLen = p - sSendBuf;
        iRecvLen = sizeof(sRecvBuf);
        iRet = iHsmProc(iSendLen, sSendBuf, &iRecvLen, sRecvBuf);
        if(iRet)
            return(KFR_COMM);
        if(sRecvBuf[0] != 'A')
            return(KFR_WESTONE+sRecvBuf[1]);
        if(iRecvLen != 1)
            return(KFR_DATA_CHK);
    } // if(sg_iMode <= 2) {
    return(KFR_OK);
}

// 生成DES密钥
// in  : iKeyIndex : 密钥索引(0-999), -1表示不存储
//       pKey      : LMK加密后输出, NULL表示不输出
// out : pKey      : LMK加密后新密钥[16]+cks[8], NULL表示不输出
// ret : KFR_OK    : 成功
//       其它      : 失败
// Note: iKeyIndex为-1时pKey不能为NULL
int iKeyNewDesKey(int iKeyIndex, void *pKey)
{
	uchar sSendBuf[2048], sRecvBuf[2048];
    uchar *p;
	int   iSendLen, iRecvLen;
	int   iRet;

    if(iKeyIndex==-1 && pKey==NULL)
        return(KFR_PARA); // 即不保存又不输出, 无意义, 认为参数错误
    if(sg_iMode <= 2) {
        // 模式0、1、2采用同一接口(卫士通加密机接口)
        p = sSendBuf;
        vTwoOne("D244", 4, p);
        p += 2;
        if(iKeyIndex == -1)
            memcpy(p, "\xFF\xFF", 2);
        else
            vLongToStr(iKeyIndex, 2, p);
        p += 2;
        *p++ = 16;
        *p++ = 1; // 1:被LMK导出
        iSendLen = p - sSendBuf;
        iRecvLen = sizeof(sRecvBuf);
        iRet = iHsmProc(iSendLen, sSendBuf, &iRecvLen, sRecvBuf);
        if(iRet)
            return(KFR_COMM);
        if(sRecvBuf[0] != 'A')
            return(KFR_WESTONE+sRecvBuf[1]);
        if(iRecvLen != 1+16+8)
            return(KFR_DATA_CHK);
        if(pKey)
            memcpy(pKey, sRecvBuf+1, 16+8);
    } // if(sg_iMode <= 2) {
    return(KFR_OK);
}

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
int iKeyNewRsaKey(int iKeyIndex, int iKeyLen, long lE, void *pModulus, int *piOutLen, void *pKey)
{
	uchar sSendBuf[2048], sRecvBuf[2048];
    uchar *p;
	int   iSendLen, iRecvLen;
	int   iRet;

    if(iKeyIndex==-1 && pKey==NULL)
        return(KFR_PARA); // 即不保存又不输出, 无意义, 认为参数错误
    if(sg_iMode <= 2) {
        // 模式0、1、2采用同一接口(卫士通加密机接口)
        int iPublicKeyLen, iPrivateKeyLen;
        p = sSendBuf;
        vTwoOne("D232", 4, p);
        p += 2;
        vLongToStr(iKeyLen*8, 2, p);
        p += 2;
        vLongToStr(lE, 4, p);
        p += 4;
        if(iKeyIndex == -1)
            memcpy(p, "\xFF\xFF", 2);
        else
            vLongToStr(iKeyIndex, 2, p);
        p += 2;
        memcpy(p, "11111111", 8);
        p += 8;
        *p++ = 0x02; // 0x02:输出公私钥
        *p++ = 1; // 1:公钥格式为DER
        *p++ = 1; // 1:私钥格式为DER
        iSendLen = p - sSendBuf;
        iRecvLen = sizeof(sRecvBuf);
        iRet = iHsmProc(iSendLen, sSendBuf, &iRecvLen, sRecvBuf);
        if(iRet)
            return(KFR_COMM);
        if(sRecvBuf[0] != 'A')
            return(KFR_WESTONE+sRecvBuf[1]);
        iPublicKeyLen = ulStrToLong(sRecvBuf+1, 2);
        if(iPublicKeyLen+3 > iRecvLen)
            return(KFR_DATA_CHK);
        iPrivateKeyLen = ulStrToLong(sRecvBuf+3+iPublicKeyLen, 2);
        if(iRecvLen != 3+iPublicKeyLen+2+iPrivateKeyLen)
            return(KFR_DATA_CHK);
        if(pModulus) {
            int  iModulusLen;
            long lTmpE;
            iRet = iUnpackPublicKey(iPublicKeyLen, sRecvBuf+3, &iModulusLen, pModulus, &lTmpE);
            if(iRet)
                return(KFR_RSA_PARSE);
            if(iModulusLen!=iKeyLen || lTmpE!=lE)
                return(KFR_RSA_PARSE);
        }
        if(pKey) {
            *piOutLen = iPrivateKeyLen;
            memcpy(pKey, sRecvBuf+3+iPublicKeyLen+2, iPrivateKeyLen);
        }
    } // if(sg_iMode <= 2) {
    return(KFR_OK);
}

// RSA私钥运算
// in  : iKeyIndex : RSA密钥索引
//       iKeyLen   : LMK加密后RSA私钥长度
//       pKey      : LMK加密后RSA私钥, NULL表示使用索引密钥
//       iLen      : 运算数据长度
//       pIn       : 运算数据
// out : pOut      : 运算结果, 长度等于iLen
// ret : KFR_OK    : 成功
//       其它      : 失败
int iKeyRsaPrivateBlock(int iKeyIndex, int iKeyLen, void *pKey, int iLen, void *pIn, void *pOut)
{
	uchar sSendBuf[2048], sRecvBuf[2048];
    uchar *p;
	int   iSendLen, iRecvLen;
	int   iRet;

    if(sg_iMode <= 2) {
        // 模式0、1、2采用同一接口(卫士通加密机接口)
        if(pKey) {
            // 用传入的密钥运算
            p = sSendBuf;
            vTwoOne("D20C", 4, p);
            p += 2;
            *p++ = 0;
            *p++ = 0;
            if(iKeyLen > 1500)
                return(KFR_PARA);
            vLongToStr(iKeyLen, 2, p);
            p += 2;
            if(iLen > 256)
                return(KFR_PARA);
            vLongToStr(iLen, 2, p);
            p += 2;
            memcpy(p, pKey, iKeyLen);
            p += iKeyLen;
            memcpy(p, pIn, iLen);
            p += iLen;
            iSendLen = p - sSendBuf;
            iRecvLen = sizeof(sRecvBuf);
            iRet = iHsmProc(iSendLen, sSendBuf, &iRecvLen, sRecvBuf);
            if(iRet)
                return(KFR_COMM);
            if(sRecvBuf[0] != 'A')
                return(KFR_WESTONE+sRecvBuf[1]);
            if(iRecvLen != 3+iLen)
                return(KFR_DATA_CHK);
            if(ulStrToLong(sRecvBuf+1, 2) != (ulong)iLen)
                return(KFR_DATA_CHK);
            memcpy(pOut, sRecvBuf+3, iLen);
        } else {
            // 用索引密钥运算
            p = sSendBuf;
            vTwoOne("C042", 4, p);
            p += 2;
            memcpy(p, "ABCDEFGH", 8);
            p += 8;
            *p++ = 0;
            vLongToStr(iKeyIndex, 2, p);
            p += 2;
            memcpy(p, "11111111", 8);
            p += 8;
            if(iLen > 256)
                return(KFR_PARA);
            vLongToStr(iLen, 2, p);
            p += 2;
            memcpy(p, pIn, iLen);
            p += iLen;
            iSendLen = p - sSendBuf;
            iRecvLen = sizeof(sRecvBuf);
            iRet = iHsmProc(iSendLen, sSendBuf, &iRecvLen, sRecvBuf);
            if(iRet)
                return(KFR_COMM);
            if(sRecvBuf[0] != 'A')
                return(KFR_WESTONE+sRecvBuf[1]);
            if(iRecvLen != 11+iLen)
                return(KFR_DATA_CHK);
            if(ulStrToLong(sRecvBuf+9, 2) != (ulong)iLen)
                return(KFR_DATA_CHK);
            memcpy(pOut, sRecvBuf+11, iLen);
        }
    } // if(sg_iMode <= 2) {
    return(KFR_OK);
}

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
int iKeyRsaPublicBlock(int iKeyIndex, int iKeyLen, void *pModulus, long lE, int iLen, void *pIn, void *pOut)
{
	uchar sSendBuf[2048], sRecvBuf[2048];
    uchar *p;
	int   iSendLen, iRecvLen;
	int   iRet;

    if(sg_iMode <= 2) {
        // 模式0、1、2采用同一接口(卫士通加密机接口)
        p = sSendBuf;
        vTwoOne("D235", 4, p);
        p += 2;
        if(pModulus == NULL) {
            vLongToStr(iKeyIndex, 2, p);
            p += 2;
        } else {
            int iDerKeyLen;
            memcpy(p, "\xFF\xFF", 2);
            p += 2;
            if(iKeyLen > 256)
                return(KFR_PARA);
            iRet = iPackPublicKey(iKeyLen, pModulus, lE, &iDerKeyLen, p+2);
            if(iRet)
                return(KFR_PARA);
            vLongToStr(iDerKeyLen, 2, p);
            p += 2+iDerKeyLen;
        }
        *p++ = 1; // 1:公钥格式为DER
        *p++ = 1; // 1:加密
        *p++ = 0; // 0:不填充
        if(iLen > 256)
            return(KFR_PARA);
        vLongToStr(iLen, 2, p);
        p += 2;
        memcpy(p, pIn, iLen);
        p += iLen;
        iSendLen = p - sSendBuf;
        iRecvLen = sizeof(sRecvBuf);
        iRet = iHsmProc(iSendLen, sSendBuf, &iRecvLen, sRecvBuf);
        if(iRet)
            return(KFR_COMM);
        if(sRecvBuf[0] != 'A')
            return(KFR_WESTONE+sRecvBuf[1]);
        if(iRecvLen != 3+iLen)
            return(KFR_DATA_CHK);
        if(ulStrToLong(sRecvBuf+1, 2) != (ulong)iLen)
            return(KFR_DATA_CHK);
        memcpy(pOut, sRecvBuf+3, iLen);
    } // if(sg_iMode <= 2) {
    return(KFR_OK);
}

// 获取RSA公钥
// in  : iKeyIndex : RSA密钥索引
// out : piLen     : RSA密钥长度, 单位:字节
//       plE       : 公共指数
//       pModulus  : 模数
// ret : KFR_OK    : 成功
//       其它      : 失败
int iKeyRsaGetInfo(int iKeyIndex, int *piLen, long *plE, void *pModulus)
{
	uchar sSendBuf[2048], sRecvBuf[2048];
    uchar *p;
	int   iSendLen, iRecvLen;
	int   iRet;

    if(sg_iMode <= 2) {
        // 模式0、1、2采用同一接口(卫士通加密机接口)
        int iDerKeyLen;
        p = sSendBuf;
        vTwoOne("D233", 4, p);
        p += 2;
        vLongToStr(iKeyIndex, 2, p);
        p += 2;
        *p++ = 0; // 0:取出公钥
        *p++ = 1; // 1:公钥格式为DER
        iSendLen = p - sSendBuf;
        iRecvLen = sizeof(sRecvBuf);
        iRet = iHsmProc(iSendLen, sSendBuf, &iRecvLen, sRecvBuf);
        if(iRet)
            return(KFR_COMM);
        if(sRecvBuf[0] != 'A')
            return(KFR_WESTONE+sRecvBuf[1]);
        iDerKeyLen = ulStrToLong(sRecvBuf+1, 2);
        if(iRecvLen != 1+2+iDerKeyLen)
            return(KFR_DATA_CHK);
        iRet = iUnpackPublicKey(iDerKeyLen, sRecvBuf+3, piLen, pModulus, plE);
        if(iRet)
            return(KFR_RSA_PARSE);
    } // if(sg_iMode <= 2) {
    return(KFR_OK);
}

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
int iKeyCalExtAuth(int iKey1Index, int iKey1Len, void *pKey1, int iDiv1Level, void *pDiv1Data, void *pTermRand, void *pCardData, void *pOut)
{
	uchar sSendBuf[2048], sRecvBuf[2048];
    uchar *p;
	int   iSendLen, iRecvLen;
	int   iRet;

    if(sg_iMode <= 2) {
        // 模式0、1、2采用同一接口(卫士通加密机接口)
        p = sSendBuf;
        vTwoOne("D237", 4, p);
        p += 2;
        if(pKey1) {
            // 使用传入密钥
            memcpy(p, "\xFF\xFF", 2);
            p += 2;
            *p++ = ((uchar *)pKey1)[0];
            memcpy(p, ((uchar *)pKey1)+1, ((uchar *)pKey1)[0]);
            p += ((uchar *)pKey1)[0];
        } else {
            // 使用索引密钥
            vLongToStr(iKey1Index, 2, p);
            p += 2;
        }
        *p++ = iDiv1Level;
        if(iDiv1Level)
            memcpy(p, pDiv1Data, iDiv1Level*8);
        p += iDiv1Level*8;
        memcpy(p, pTermRand, 8);
        p += 8;
        memcpy(p, pCardData, 28);
        p += 28;
        iSendLen = p - sSendBuf;
        iRecvLen = sizeof(sRecvBuf);
        iRet = iHsmProc(iSendLen, sSendBuf, &iRecvLen, sRecvBuf);
        if(iRet)
            return(KFR_COMM);
        if(sRecvBuf[0] != 'A')
            return(KFR_WESTONE+sRecvBuf[1]);
        if(iRecvLen != 1+16)
            return(KFR_DATA_CHK);
        memcpy(pOut, sRecvBuf+1, 16);
    } // if(sg_iMode <= 2) {
    return(KFR_OK);
}

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
int iKeyCalChangeKmc(int iKey1Index, int iKey1Len, void *pKey1, int iDiv1Level, void *pDiv1Data, void *pTermRand, void *pCardData, int iKey2Index, int iKey2Len, void *pKey2, int iDiv2Level, void *pDiv2Data, void *pOut)
{
	uchar sSendBuf[2048], sRecvBuf[2048];
    uchar *p;
	int   iSendLen, iRecvLen;
	int   iRet;
    int   i;

    if(sg_iMode <= 2) {
        // 模式0、1、2采用同一接口(卫士通加密机接口)
        p = sSendBuf;
        vTwoOne("D23D", 4, p);
        p += 2;
        if(pKey1) {
            // 使用传入密钥
            memcpy(p, "\xFF\xFF", 2);
            p += 2;
            *p++ = ((uchar *)pKey1)[0];
            memcpy(p, ((uchar *)pKey1)+1, ((uchar *)pKey1)[0]);
            p += ((uchar *)pKey1)[0];
        } else {
            // 使用索引密钥
            vLongToStr(iKey1Index, 2, p);
            p += 2;
        }
        *p++ = iDiv1Level;
        if(iDiv1Level)
            memcpy(p, pDiv1Data, iDiv1Level*8);
        p += iDiv1Level*8;
        memcpy(p, pTermRand, 8);
        p += 8;
        memcpy(p, pCardData, 28);
        p += 28;
        if(pKey2) {
            // 使用传入密钥
            memcpy(p, "\xFF\xFF", 2);
            p += 2;
            *p++ = ((uchar *)pKey2)[0];
            memcpy(p, ((uchar *)pKey2)+1, ((uchar *)pKey2)[0]);
            p += ((uchar *)pKey2)[0];
        } else {
            // 使用索引密钥
            vLongToStr(iKey2Index, 2, p);
            p += 2;
        }
        *p++ = iDiv2Level;
        if(iDiv2Level)
            memcpy(p, pDiv2Data, iDiv2Level*8);
        p += iDiv2Level*8;
        iSendLen = p - sSendBuf;
        iRecvLen = sizeof(sRecvBuf);
        iRet = iHsmProc(iSendLen, sSendBuf, &iRecvLen, sRecvBuf);
        if(iRet)
            return(KFR_COMM);
        if(sRecvBuf[0] != 'A')
            return(KFR_WESTONE+sRecvBuf[1]);
        if(iRecvLen != 1+72)
            return(KFR_DATA_CHK);
        p = (uchar *)pOut;
        for(i=0; i<3; i++)
            memcpy(p+19*i, sRecvBuf+1+24*i, 19);
    } // if(sg_iMode <= 2) {
    return(KFR_OK);
}

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
int iKeyCalDesKey(int iKey1Index, int iKey1Len, void *pKey1, int iDiv1Level, void *pDiv1Data, void *pTermRand, void *pCardData, int iKey2Index, int iKey2Len, void *pKey2, int iDiv2Level, void *pDiv2Data, void *pOut)
{
	uchar sSendBuf[2048], sRecvBuf[2048];
    uchar *p;
	int   iSendLen, iRecvLen;
	int   iRet;

    if(sg_iMode <= 2) {
        // 模式0、1、2采用同一接口(卫士通加密机接口)
        p = sSendBuf;
        vTwoOne("D238", 4, p);
        p += 2;
        if(pKey1) {
            // 使用传入密钥
            memcpy(p, "\xFF\xFF", 2);
            p += 2;
            *p++ = ((uchar *)pKey1)[0];
            memcpy(p, ((uchar *)pKey1)+1, ((uchar *)pKey1)[0]);
            p += ((uchar *)pKey1)[0];
        } else {
            // 使用索引密钥
            vLongToStr(iKey1Index, 2, p);
            p += 2;
        }
        *p++ = iDiv1Level;
        if(iDiv1Level)
            memcpy(p, pDiv1Data, iDiv1Level*8);
        p += iDiv1Level*8;
        memcpy(p, pTermRand, 8);
        p += 8;
        memcpy(p, pCardData, 28);
        p += 28;
        if(pKey2) {
            // 使用传入密钥
            memcpy(p, "\xFF\xFF", 2);
            p += 2;
            *p++ = ((uchar *)pKey2)[0];
            memcpy(p, ((uchar *)pKey2)+1, ((uchar *)pKey2)[0]);
            p += ((uchar *)pKey2)[0];
        } else {
            // 使用索引密钥
            vLongToStr(iKey2Index, 2, p);
            p += 2;
        }
        *p++ = iDiv2Level;
        if(iDiv2Level)
            memcpy(p, pDiv2Data, iDiv2Level*8);
        p += iDiv2Level*8;
        iSendLen = p - sSendBuf;
        iRecvLen = sizeof(sRecvBuf);
        iRet = iHsmProc(iSendLen, sSendBuf, &iRecvLen, sRecvBuf);
        if(iRet)
            return(KFR_COMM);
        if(sRecvBuf[0] != 'A')
            return(KFR_WESTONE+sRecvBuf[1]);
        if(iRecvLen != 1+24)
            return(KFR_DATA_CHK);
        memcpy(pOut, sRecvBuf+1, 19);
    } // if(sg_iMode <= 2) {
    return(KFR_OK);
}

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
int iKeyCalPin(int iKey1Index, int iKey1Len, void *pKey1, int iDiv1Level, void *pDiv1Data, void *pTermRand, void *pCardData, void *pPinBlock, void *pOut)
{
	uchar sSendBuf[2048], sRecvBuf[2048];
    uchar *p;
	int   iSendLen, iRecvLen;
	int   iRet;

    if(sg_iMode <= 2) {
        // 模式0、1、2采用同一接口(卫士通加密机接口)
        p = sSendBuf;
        vTwoOne("D239", 4, p);
        p += 2;
        if(pKey1) {
            // 使用传入密钥
            memcpy(p, "\xFF\xFF", 2);
            p += 2;
            *p++ = ((uchar *)pKey1)[0];
            memcpy(p, ((uchar *)pKey1)+1, ((uchar *)pKey1)[0]);
            p += ((uchar *)pKey1)[0];
        } else {
            // 使用索引密钥
            vLongToStr(iKey1Index, 2, p);
            p += 2;
        }
        *p++ = iDiv1Level;
        if(iDiv1Level)
            memcpy(p, pDiv1Data, iDiv1Level*8);
        p += iDiv1Level*8;
        memcpy(p, pTermRand, 8);
        p += 8;
        memcpy(p, pCardData, 28);
        p += 28;
        memcpy(p, pPinBlock, 8);
        p += 8;
        iSendLen = p - sSendBuf;
        iRecvLen = sizeof(sRecvBuf);
        iRet = iHsmProc(iSendLen, sSendBuf, &iRecvLen, sRecvBuf);
        if(iRet)
            return(KFR_COMM);
        if(sRecvBuf[0] != 'A')
            return(KFR_WESTONE+sRecvBuf[1]);
        if(iRecvLen != 1+8)
            return(KFR_DATA_CHK);
        memcpy(pOut, sRecvBuf+1, 8);
    } // if(sg_iMode <= 2) {
    return(KFR_OK);
}

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
int iKeyCalRsaKey(int iKey1Index, int iKey1Len, void *pKey1, int iDiv1Level, void *pDiv1Data, void *pTermRand, void *pCardData, int iRsaKeyLen, long lE, void *pOut)
{
	uchar sSendBuf[2048], sRecvBuf[2048];
    uchar *p;
	int   iSendLen, iRecvLen;
	int   iRet;
    int   i;

    if(sg_iMode <= 2) {
        // 模式0、1、2采用同一接口(卫士通加密机接口)
        int  iDerKeyLen, iNLen;
        int  iDLen; // 导出的D值长度
        int  iPLen; // 导出的P值长度(Q、dP、dQ、qInv长度同)
        long lTmpE;
        p = sSendBuf;
        vTwoOne("D23A", 4, p);
        p += 2;
        vLongToStr(iRsaKeyLen*8, 2, p);
        p += 2;
        vLongToStr(lE, 4, p);
        p += 4;
        *p++ = 1; // 1:公钥格式为DER
        *p++ = 2; // 2:私钥格式为IC卡写入需要的加密格式
        if(pKey1) {
            // 使用传入密钥
            memcpy(p, "\xFF\xFF", 2);
            p += 2;
            *p++ = ((uchar *)pKey1)[0];
            memcpy(p, ((uchar *)pKey1)+1, ((uchar *)pKey1)[0]);
            p += ((uchar *)pKey1)[0];
        } else {
            // 使用索引密钥
            vLongToStr(iKey1Index, 2, p);
            p += 2;
        }
        *p++ = iDiv1Level;
        if(iDiv1Level)
            memcpy(p, pDiv1Data, iDiv1Level*8);
        p += iDiv1Level*8;
        memcpy(p, pTermRand, 8);
        p += 8;
        memcpy(p, pCardData, 28);
        p += 28;
        iSendLen = p - sSendBuf;
        iRecvLen = sizeof(sRecvBuf);
        iRet = iHsmProc(iSendLen, sSendBuf, &iRecvLen, sRecvBuf);
        if(iRet)
            return(KFR_COMM);
        if(sRecvBuf[0] != 'A')
            return(KFR_WESTONE+sRecvBuf[1]);

        iDerKeyLen = ulStrToLong(sRecvBuf+1, 2);
        if(iRecvLen < 1+2+iDerKeyLen)
            return(KFR_DATA_CHK);
        iRet = iUnpackPublicKey(iDerKeyLen, sRecvBuf+3, &iNLen, pOut, &lTmpE);
        if(iRet)
            return(KFR_RSA_PARSE);
        if(iNLen!=iRsaKeyLen || lTmpE!=lE)
            return(KFR_RSA_PARSE);
        iDLen = (iNLen+8)/8*8;
        iPLen = (iNLen/2+8)/8*8;
        if(iRecvLen != 1+2*7+iDerKeyLen+iDLen+5*iPLen)
            return(KFR_DATA_CHK);
        p = (uchar *)pOut;
        p += iNLen;
        memcpy(p, sRecvBuf+3+iDerKeyLen+2, iDLen);
        p += iDLen;
        for(i=0; i<5; i++)
            memcpy(p+iPLen*i, sRecvBuf+3+iDerKeyLen+2+iDLen+(2+iPLen)*i+2, iPLen);
    } // if(sg_iMode <= 2) {
    return(KFR_OK);
}

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
int iKeyVerifyAc(int iKey1Index, int iKey1Len, void *pKey1, int iDiv1Level, void *pDiv1Data, void *pATC, int iTransDataLen, void *pTransData, void *pARQC, void *pARC, void *pARPC)
{
	uchar sSendBuf[2048], sRecvBuf[2048];
    uchar *p;
	int   iSendLen, iRecvLen;
	int   iRet;

    if(sg_iMode <= 2) {
        // 模式0、1、2采用同一接口(卫士通加密机接口)
        p = sSendBuf;
        vTwoOne("D23C", 4, p);
        p += 2;
        if(pARC) {
            // 交易应答码存在, 需要计算ARPC
            *p++ = 1; // 1:验证ARQC并计算ARPC
        } else {
            // 交易应答码不存在, 不需要计算ARPC
            *p++ = 0; // 0:只验证ARQC
        }
        if(pKey1) {
            // 使用传入密钥
            memcpy(p, "\xFF\xFF", 2);
            p += 2;
            *p++ = ((uchar *)pKey1)[0];
            memcpy(p, ((uchar *)pKey1)+1, ((uchar *)pKey1)[0]);
            p += ((uchar *)pKey1)[0];
        } else {
            // 使用索引密钥
            vLongToStr(iKey1Index, 2, p);
            p += 2;
        }
        *p++ = iDiv1Level;
        if(iDiv1Level)
            memcpy(p, pDiv1Data, iDiv1Level*8);
        p += iDiv1Level*8;
        memcpy(p, pATC, 2);
        p += 2;
        memcpy(p, pARQC, 8);
        p += 8;
        vLongToStr(iTransDataLen, 2, p);
        p += 2;
        memcpy(p, pTransData, iTransDataLen);
        p += iTransDataLen;
        if(pARC) {
            memcpy(p, pARC, 2);
            p += 2;
        }
        iSendLen = p - sSendBuf;
        iRecvLen = sizeof(sRecvBuf);
        iRet = iHsmProc(iSendLen, sSendBuf, &iRecvLen, sRecvBuf);
        if(iRet)
            return(KFR_COMM);
        if(sRecvBuf[0] != 'A')
            return(KFR_WESTONE+sRecvBuf[1]);
        if(pARC && iRecvLen!=9)
            return(KFR_DATA_CHK);
        if(!pARC && iRecvLen != 1)
            return(KFR_DATA_CHK);
        if(pARC)
            memcpy(pARPC, sRecvBuf+1, 8);
    } // if(sg_iMode <= 2) {
    return(KFR_OK);
}

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
int iKeyScriptEncData(int iKey1Index, int iKey1Len, void *pKey1, int iDiv1Level, void *pDiv1Data, void *pATC, int iPinFlag, int iDataLen, void *pData, int *piOutLen, void *pOutData)
{
	uchar sSendBuf[2048], sRecvBuf[2048];
    uchar *p;
	int   iSendLen, iRecvLen;
	int   iRet;

    if(sg_iMode <= 2) {
        // 模式0、1、2采用同一接口(卫士通加密机接口)
        if(iPinFlag) {
            // 加密PIN脚本
            if(iDataLen != 8)
                return(KFR_PARA);
            p = sSendBuf;
            vTwoOne("D241", 4, p);
            p += 2;
            *p++ = 1; // 1:PBOC2.0规范卡
            if(pKey1) {
                // 使用传入密钥
                memcpy(p, "\xFF\xFF", 2);
                p += 2;
                *p++ = ((uchar *)pKey1)[0];
                memcpy(p, ((uchar *)pKey1)+1, ((uchar *)pKey1)[0]);
                p += ((uchar *)pKey1)[0];
            } else {
                // 使用索引密钥
                vLongToStr(iKey1Index, 2, p);
                p += 2;
            }
            *p++ = iDiv1Level;
            if(iDiv1Level)
                memcpy(p, pDiv1Data, iDiv1Level*8);
            p += iDiv1Level*8;
            memcpy(p, pATC, 2);
            p += 2;
            memcpy(p, pData, iDataLen);
            p += iDataLen;
            iSendLen = p - sSendBuf;
            iRecvLen = sizeof(sRecvBuf);
            iRet = iHsmProc(iSendLen, sSendBuf, &iRecvLen, sRecvBuf);
            if(iRet)
                return(KFR_COMM);
            if(sRecvBuf[0] != 'A')
                return(KFR_WESTONE+sRecvBuf[1]);
            if(iRecvLen != 1+2+16)
                return(KFR_DATA_CHK);
            memcpy(pOutData, sRecvBuf+3, 16);
            *piOutLen = 16;
        } else {
            p = sSendBuf;
            vTwoOne("D240", 4, p);
            p += 2;
            *p++ = 1; // 1:加密
            *p++ = 1; // 1:PBOC2.0规范卡
            if(pKey1) {
                // 使用传入密钥
                memcpy(p, "\xFF\xFF", 2);
                p += 2;
                *p++ = ((uchar *)pKey1)[0];
                memcpy(p, ((uchar *)pKey1)+1, ((uchar *)pKey1)[0]);
                p += ((uchar *)pKey1)[0];
            } else {
                // 使用索引密钥
                vLongToStr(iKey1Index, 2, p);
                p += 2;
            }
            *p++ = iDiv1Level;
            if(iDiv1Level)
                memcpy(p, pDiv1Data, iDiv1Level*8);
            p += iDiv1Level*8;
            memcpy(p, pATC, 2);
            p += 2;
            *p++ = 0; // 0:ECB
            vLongToStr(iDataLen, 2, p);
            p += 2;
            memcpy(p, pData, iDataLen);
            p += iDataLen;
            iSendLen = p - sSendBuf;
            iRecvLen = sizeof(sRecvBuf);
            iRet = iHsmProc(iSendLen, sSendBuf, &iRecvLen, sRecvBuf);
            if(iRet)
                return(KFR_COMM);
            if(sRecvBuf[0] != 'A')
                return(KFR_WESTONE+sRecvBuf[1]);
            *piOutLen = ulStrToLong(sRecvBuf+1, 2);
            if(iRecvLen != 3+*piOutLen)
                return(KFR_DATA_CHK);
            memcpy(pOutData, sRecvBuf+3, *piOutLen);
        }
    } // if(sg_iMode <= 2) {
    return(KFR_OK);
}

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
int iKeyScriptMac(int iKey1Index, int iKey1Len, void *pKey1, int iDiv1Level, void *pDiv1Data, void *pATC, int iDataLen, void *pData, void *pMac)
{
	uchar sSendBuf[2048], sRecvBuf[2048];
    uchar *p;
	int   iSendLen, iRecvLen;
	int   iRet;

    if(sg_iMode <= 2) {
        p = sSendBuf;
        vTwoOne("D242", 4, p);
        p += 2;
        *p++ = 1; // 1:PBOC2.0规范卡
        if(pKey1) {
            // 使用传入密钥
            memcpy(p, "\xFF\xFF", 2);
            p += 2;
            *p++ = ((uchar *)pKey1)[0];
            memcpy(p, ((uchar *)pKey1)+1, ((uchar *)pKey1)[0]);
            p += ((uchar *)pKey1)[0];
        } else {
            // 使用索引密钥
            vLongToStr(iKey1Index, 2, p);
            p += 2;
        }
        *p++ = iDiv1Level;
        if(iDiv1Level)
            memcpy(p, pDiv1Data, iDiv1Level*8);
        p += iDiv1Level*8;
        memcpy(p, pATC, 2);
        p += 2;
        vLongToStr(iDataLen, 2, p);
        p += 2;
        memcpy(p, pData, iDataLen);
        p += iDataLen;
        iSendLen = p - sSendBuf;
        iRecvLen = sizeof(sRecvBuf);
        iRet = iHsmProc(iSendLen, sSendBuf, &iRecvLen, sRecvBuf);
        if(iRet)
            return(KFR_COMM);
        if(sRecvBuf[0] != 'A')
            return(KFR_WESTONE+sRecvBuf[1]);
        if(iRecvLen != 9)
            return(KFR_DATA_CHK);
        memcpy(pMac, sRecvBuf+1, 8);
    } // if(sg_iMode <= 2) {
    return(KFR_OK);
}

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
int iKeyReEncData(int iKey1Index, int iKey1Len, void *pKey1, int iDiv1Level, void *pDiv1Data, void *pTermRand, void *pCardData, int iKey2Index, int iKey2Len, void *pKey2, int iDiv2Level, void *pDiv2Data, int iDataLen, void *pData, void *pOut)
{
	uchar sSendBuf[2048], sRecvBuf[2048];
    uchar *p;
	int   iSendLen, iRecvLen;
	int   iRet;
    int   i;

    if(sg_iMode <= 2) {
        // 模式0、1、2采用同一接口(卫士通加密机接口)
        p = sSendBuf;
        vTwoOne("D23E", 4, p);
        p += 2;
        if(pKey2) {
            // 使用传入密钥
            memcpy(p, "\xFF\xFF", 2);
            p += 2;
            *p++ = ((uchar *)pKey2)[0];
            memcpy(p, ((uchar *)pKey2)+1, ((uchar *)pKey2)[0]);
            p += ((uchar *)pKey2)[0];
        } else {
            // 使用索引密钥
            vLongToStr(iKey2Index, 2, p);
            p += 2;
        }
        *p++ = iDiv2Level;
        if(iDiv2Level)
            memcpy(p, pDiv2Data, iDiv2Level*8);
        p += iDiv2Level*8;
        if(pKey1) {
            // 使用传入密钥
            memcpy(p, "\xFF\xFF", 2);
            p += 2;
            *p++ = ((uchar *)pKey1)[0];
            memcpy(p, ((uchar *)pKey1)+1, ((uchar *)pKey1)[0]);
            p += ((uchar *)pKey1)[0];
        } else {
            // 使用索引密钥
            vLongToStr(iKey1Index, 2, p);
            p += 2;
        }
        *p++ = iDiv1Level;
        if(iDiv1Level)
            memcpy(p, pDiv1Data, iDiv1Level*8);
        p += iDiv1Level*8;
        memcpy(p, pTermRand, 8);
        p += 8;
        memcpy(p, pCardData, 28);
        p += 28;
        *p++ = 0; // 0:ECB模式
        vLongToStr(iDataLen, 2, p);
        p += 2;
        memcpy(p, pData, iDataLen);
        p += iDataLen;
        iSendLen = p - sSendBuf;
        iRecvLen = sizeof(sRecvBuf);
        iRet = iHsmProc(iSendLen, sSendBuf, &iRecvLen, sRecvBuf);
        if(iRet)
            return(KFR_COMM);
        if(sRecvBuf[0] != 'A')
            return(KFR_WESTONE+sRecvBuf[1]);
        i = ulStrToLong(sRecvBuf+1, 2);
        if(i != iDataLen)
            return(KFR_DATA_CHK);
        if(iRecvLen != 3+iDataLen)
            return(KFR_DATA_CHK);
        memcpy(pOut, sRecvBuf+3, iDataLen);
    } // if(sg_iMode <= 2) {
    return(KFR_OK);
}

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
int iKeyReEncPin(int iKey1Index, int iKey1Len, void *pKey1, int iDiv1Level, void *pDiv1Data, void *pTermRand, void *pCardData, int iKey2Index, int iKey2Len, void *pKey2, int iDiv2Level, void *pDiv2Data, int iPinFormat, void *pPinBlock, void *pOut)
{
	uchar sSendBuf[2048], sRecvBuf[2048];
    uchar *p;
	int   iSendLen, iRecvLen;
	int   iRet;

    if(sg_iMode <= 2) {
        // 模式0、1、2采用同一接口(卫士通加密机接口)
        p = sSendBuf;
        vTwoOne("D23F", 4, p);
        p += 2;
        if(pKey2) {
            // 使用传入密钥
            memcpy(p, "\xFF\xFF", 2);
            p += 2;
            *p++ = ((uchar *)pKey2)[0];
            memcpy(p, ((uchar *)pKey2)+1, ((uchar *)pKey2)[0]);
            p += ((uchar *)pKey2)[0];
        } else {
            // 使用索引密钥
            vLongToStr(iKey2Index, 2, p);
            p += 2;
        }
        *p++ = iDiv2Level;
        if(iDiv2Level)
            memcpy(p, pDiv2Data, iDiv2Level*8);
        p += iDiv2Level*8;
        if(pKey1) {
            // 使用传入密钥
            memcpy(p, "\xFF\xFF", 2);
            p += 2;
            *p++ = ((uchar *)pKey1)[0];
            memcpy(p, ((uchar *)pKey1)+1, ((uchar *)pKey1)[0]);
            p += ((uchar *)pKey1)[0];
        } else {
            // 使用索引密钥
            vLongToStr(iKey1Index, 2, p);
            p += 2;
        }
        *p++ = iDiv1Level;
        if(iDiv1Level)
            memcpy(p, pDiv1Data, iDiv1Level*8);
        p += iDiv1Level*8;
        memcpy(p, pTermRand, 8);
        p += 8;
        memcpy(p, pCardData, 28);
        p += 28;
        *p++ = iPinFormat; // 转换前格式
        *p++ = 2; // 转换后格式, 2:PinBlock格式2
        memcpy(p, pPinBlock, 8);
        p += 8;
        iSendLen = p - sSendBuf;
        iRecvLen = sizeof(sRecvBuf);
        iRet = iHsmProc(iSendLen, sSendBuf, &iRecvLen, sRecvBuf);
        if(iRet)
            return(KFR_COMM);
        if(sRecvBuf[0] != 'A')
            return(KFR_WESTONE+sRecvBuf[1]);
        if(iRecvLen != 9)
            return(KFR_DATA_CHK);
        memcpy(pOut, sRecvBuf+1, 8);
    } // if(sg_iMode <= 2) {
    return(KFR_OK);
}

// Mac计算(数据准备)
// in  : iKey1Index  : 应用密钥索引
//       iKey1Len    : LMK加密后应用密钥长度
//       pKey1       : LMK加密后应用密钥, NULL表示使用索引密钥
//       iDiv1Level  : 应用密钥分散次数(0-3)
//       pDiv1Data   : 应用密钥分散数据[8*n]
//       pIv         : 初始向量[8]
//       iDataLen    : 数据长度, 必须为8的倍数
//       pData       : 数据
//       pMacKey     : 被应用密钥加密后的Mac密钥
//       iFinalFlag  : 最后一块标志, 0:不是最后一块 1:是最后一块
// out : pMac        : Mac[8]
// ret : KFR_OK      : 成功
//       其它        : 失败
// Note: 计算分解后的单包报文, 给iKeyDpCalMac()提供服务
static int iKeyDpCalMacPart(int iKey1Index, int iKey1Len, void *pKey1, int iDiv1Level, void *pDiv1Data, void *pIv, int iDataLen, void *pData, void *pMacKey, int iFinalFlag, void *pMac)
{
	uchar sSendBuf[2048], sRecvBuf[2048];
    uchar *p;
	int   iSendLen, iRecvLen;
	int   iRet;

    if(sg_iMode <= 2) {
        p = sSendBuf;
        vTwoOne("D243", 4, p);
        p += 2;
        memcpy(p, "\xEE\xEE", 2); // 'EEEE':密钥被次主密钥保护
        p += 2;
        *p++ = 16;
        memcpy(p, pMacKey, 16);
        p += 16;
        *p++ = 0;
        *p++ = 0; // 0:不使用过程密钥
        if(pKey1) {
            // 使用传入密钥
            memcpy(p, "\xFF\xFF", 2);
            p += 2;
            *p++ = ((uchar *)pKey1)[0];
            memcpy(p, ((uchar *)pKey1)+1, ((uchar *)pKey1)[0]);
            p += ((uchar *)pKey1)[0];
        } else {
            // 使用索引密钥
            vLongToStr(iKey1Index, 2, p);
            p += 2;
        }
        *p++ = iDiv1Level;
        if(iDiv1Level)
            memcpy(p, pDiv1Data, iDiv1Level*8);
        p += iDiv1Level*8;
        if(iDiv1Level)
            *p++ = 1; // 保护密钥分散算法, 1:Pboc
        *p++ = 0; // 0:保护密钥不使用过程密钥
//        *p++ = 4; // 4:pboc 3Des算法
        *p++ = 2; // 2:ansi919算法, 3Des Retail方式
        memcpy(p, pIv, 8);
        p += 8;
        if(iFinalFlag)
            *p++ = 2; // 1:唯一块或最后一块
        else
            *p++ = 1; // 1:第一块或中间块
        vLongToStr(iDataLen, 2, p);
        p += 2;
        memcpy(p, pData, iDataLen);
        p += iDataLen;
        iSendLen = p - sSendBuf;
        iRecvLen = sizeof(sRecvBuf);
        iRet = iHsmProc(iSendLen, sSendBuf, &iRecvLen, sRecvBuf);
        if(iRet)
            return(KFR_COMM);
        if(sRecvBuf[0] != 'A')
            return(KFR_WESTONE+sRecvBuf[1]);
        if(iRecvLen != 9)
            return(KFR_DATA_CHK);
        memcpy(pMac, sRecvBuf+1, 8);
    } // if(sg_iMode <= 2) {
    return(KFR_OK);
}
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
// Note: 产生临时密钥, 然后调用一或多次iKeyDpCalMacPart()计算Mac
int iKeyDpCalMac(int iKey1Index, int iKey1Len, void *pKey1, int iDiv1Level, void *pDiv1Data, void *pIv, int iDataLen, void *pData, void *pMacKey, void *pMac)
{
	uchar sSendBuf[2048], sRecvBuf[2048];
    uchar *p;
	int   iSendLen, iRecvLen;
	int   iRet;
    uchar sTmpMacKey[16];
    int   i;
    uchar sIv[8], *psData;
    int   iFinalFlag;
    int   iLen;

    if(sg_iMode <= 2) {
        // 1. 生成临时Mac密钥
        p = sSendBuf;
        vTwoOne("D244", 4, p);
        p += 2;
        memcpy(p, "\xFF\xFF", 2); // 不保存密钥
        p += 2;
        *p++ = 16;
        *p++ = 2; // 2:被保护密钥导出
        if(pKey1) {
            // 使用传入密钥
            memcpy(p, "\xFF\xFF", 2);
            p += 2;
            *p++ = ((uchar *)pKey1)[0];
            memcpy(p, ((uchar *)pKey1)+1, ((uchar *)pKey1)[0]);
            p += ((uchar *)pKey1)[0];
        } else {
            // 使用索引密钥
            vLongToStr(iKey1Index, 2, p);
            p += 2;
        }
        *p++ = iDiv1Level;
        if(iDiv1Level)
            memcpy(p, pDiv1Data, iDiv1Level*8);
        p += iDiv1Level*8;
        iSendLen = p - sSendBuf;
        iRecvLen = sizeof(sRecvBuf);
        iRet = iHsmProc(iSendLen, sSendBuf, &iRecvLen, sRecvBuf);
        if(iRet)
            return(KFR_COMM);
        if(sRecvBuf[0] != 'A')
            return(KFR_WESTONE+sRecvBuf[1]);
        if(iRecvLen != 1+16+8)
            return(KFR_DATA_CHK);
        memcpy(sTmpMacKey, sRecvBuf+1, 16); // sTmpMacKey为被保护密钥加密了的临时密钥

        // 2. 计算Mac
        memcpy(sIv, pIv, 8);
        psData = (uchar *)pData;
        for(i=0; i<iDataLen; i+=1024) {
            iLen = iDataLen-i > 1024 ? 1024 : iDataLen-i;
            iFinalFlag = 0;
            if(iDataLen-i <= 1024)
                iFinalFlag = 1; // 最后一块或唯一块
            iRet = iKeyDpCalMacPart(iKey1Index, iKey1Len, pKey1, iDiv1Level, pDiv1Data, sIv, iLen, psData, sTmpMacKey, iFinalFlag, pMac);
            if(iRet)
                return(iRet);
            psData += iLen;
            memcpy(sIv, pMac, 8);
        }
        memcpy(pMacKey, sTmpMacKey, 16);
    } // if(sg_iMode <= 2) {
    return(KFR_OK);
}

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
int iKeyDpCalDesKey(int iKey1Index, int iKey1Len, void *pKey1, int iDiv1Level, void *pDiv1Data, int iKey2Index, int iKey2Len, void *pKey2, int iDiv2Level, void *pDiv2Data, void *pOut)
{
	uchar sSendBuf[2048], sRecvBuf[2048];
    uchar *p;
	int   iSendLen, iRecvLen;
	int   iRet;

    if(sg_iMode <= 2) {
        // 模式0、1、2采用同一接口(卫士通加密机接口)
        p = sSendBuf;
        vTwoOne("D245", 4, p);
        p += 2;
        if(pKey1) {
            // 使用传入密钥
            memcpy(p, "\xFF\xFF", 2);
            p += 2;
            *p++ = ((uchar *)pKey1)[0];
            memcpy(p, ((uchar *)pKey1)+1, ((uchar *)pKey1)[0]);
            p += ((uchar *)pKey1)[0];
        } else {
            // 使用索引密钥
            vLongToStr(iKey1Index, 2, p);
            p += 2;
        }
        *p++ = iDiv1Level;
        if(iDiv1Level)
            memcpy(p, pDiv1Data, iDiv1Level*8);
        p += iDiv1Level*8;
        if(iDiv1Level)
            *p++ = 1; // TK分散算法, 1:pboc
        if(pKey2) {
            // 使用传入密钥
            memcpy(p, "\xFF\xFF", 2);
            p += 2;
            *p++ = ((uchar *)pKey2)[0];
            memcpy(p, ((uchar *)pKey2)+1, ((uchar *)pKey2)[0]);
            p += ((uchar *)pKey2)[0];
        } else {
            // 使用索引密钥
            vLongToStr(iKey2Index, 2, p);
            p += 2;
        }
        *p++ = iDiv2Level;
        if(iDiv2Level)
            memcpy(p, pDiv2Data, iDiv2Level*8);
        p += iDiv2Level*8;
        if(iDiv2Level)
            *p++ = 1; // 应用密钥分散算法, 1:pboc
        iSendLen = p - sSendBuf;
        iRecvLen = sizeof(sRecvBuf);
        iRet = iHsmProc(iSendLen, sSendBuf, &iRecvLen, sRecvBuf);
        if(iRet)
            return(KFR_COMM);
        if(sRecvBuf[0] != 'A')
            return(KFR_WESTONE+sRecvBuf[1]);
        if(iRecvLen != 2+16+8)
            return(KFR_DATA_CHK);
        memcpy(pOut, sRecvBuf+2, 16+3);
    } // if(sg_iMode <= 2) {
    return(KFR_OK);
}

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
int iKeyDpCalRsaKey(int iKey1Index, int iKey1Len, void *pKey1, int iDiv1Level, void *pDiv1Data, int iRsaKeyLen, long lE, void *pOut)
{
	uchar sSendBuf[2048], sRecvBuf[2048];
    uchar *p;
	int   iSendLen, iRecvLen;
	int   iRet;
    int   i;

    if(sg_iMode <= 2) {
        // 模式0、1、2采用同一接口(卫士通加密机接口)
        int  iDerKeyLen, iNLen;
        int  iDLen; // 导出的D值长度
        int  iPLen; // 导出的P值长度(Q、dP、dQ、qInv长度同)
        long lTmpE;
        p = sSendBuf;
        vTwoOne("D23B", 4, p);
        p += 2;
        vLongToStr(iRsaKeyLen*8, 2, p);
        p += 2;
        vLongToStr(lE, 4, p);
        p += 4;
        *p++ = 1; // 1:公钥格式为DER
        *p++ = 2; // 2:私钥格式为数据准备需要的加密格式
        if(pKey1) {
            // 使用传入密钥
            memcpy(p, "\xFF\xFF", 2);
            p += 2;
            *p++ = ((uchar *)pKey1)[0];
            memcpy(p, ((uchar *)pKey1)+1, ((uchar *)pKey1)[0]);
            p += ((uchar *)pKey1)[0];
        } else {
            // 使用索引密钥
            vLongToStr(iKey1Index, 2, p);
            p += 2;
        }
        *p++ = iDiv1Level;
        if(iDiv1Level)
            memcpy(p, pDiv1Data, iDiv1Level*8);
        p += iDiv1Level*8;
        iSendLen = p - sSendBuf;
        iRecvLen = sizeof(sRecvBuf);
        iRet = iHsmProc(iSendLen, sSendBuf, &iRecvLen, sRecvBuf);
        if(iRet)
            return(KFR_COMM);
        if(sRecvBuf[0] != 'A')
            return(KFR_WESTONE+sRecvBuf[1]);

        iDerKeyLen = ulStrToLong(sRecvBuf+1, 2);
        if(iRecvLen < 1+2+iDerKeyLen)
            return(KFR_DATA_CHK);
        iRet = iUnpackPublicKey(iDerKeyLen, sRecvBuf+3, &iNLen, pOut, &lTmpE);
        if(iRet)
            return(KFR_RSA_PARSE);
        if(iNLen!=iRsaKeyLen || lTmpE!=lE)
            return(KFR_RSA_PARSE);
        iDLen = (iNLen+8)/8*8;
        iPLen = (iNLen/2+8)/8*8;
        if(iRecvLen != 1+2*7+iDerKeyLen+iDLen+5*iPLen)
            return(KFR_DATA_CHK);
        p = (uchar *)pOut;
        p += iNLen;
        memcpy(p, sRecvBuf+3+iNLen+2, iDLen);
        p += iNLen;
        for(i=0; i<5; i++)
            memcpy(p+iPLen*i, sRecvBuf+3+iNLen+2+iDLen+2+iPLen*i, iPLen);
    } // if(sg_iMode <= 2) {
    return(KFR_OK);
}
