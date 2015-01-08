#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "VposFace.h"
#include "hsmproc.h"
#include "hsmsimu.h"

//#define WESTONE_SUPPORT		// 定义该宏表示支持卫士通加密机
#ifdef WESTONE_SUPPORT
#include "sockcomm.h"
#endif

static int  sg_iMode = 0;           // 模式, 0:模块内密钥模式 1:本地配置文件模式 2:卫士通加密机模式
static char sg_szHsmIp[20] = {0};   // 加密机IP地址
static char sg_szHsmPort[10] = {0}; // 加密机端口号

#define MAX_RSA_KEY_LEN     248
static int iInsProc(uchar *psRecvBuf, int iRecvLen, uchar *psSendBuf, int *piSendLen);

// 设置参数
// in  : iMode    : 模式 0:模块内密钥模式 1:本地配置文件模式 2:卫士通加密机模式
//       pszIp    : 加密机IP地址, 模式2与模式3可用
//       iPortNo  : 加密机端口号, 模式2与模式3可用
// ret : 0        : 成功
//       -1       : 失败, 加密机未连接
//       -2       : 失败, 处理配置文件错
//       -3       : iMode错
int iHsmSetMode(int iMode, char *pszIp, int iPortNo)
{
    int  iRet;
#ifdef WESTONE_SUPPORT
    int  iRecvLen;
    char sRecvBuf[100];
#endif
    sg_iMode = iMode;

    switch(iMode) {
    case 0:
        iRet = iHsmInit0(NULL);
        break;
    case 1:
        iRet = iHsmInit0("key.dat");
        if(iRet)
            return(-2);
        break;
    case 2:
#ifdef WESTONE_SUPPORT
        memset(sg_szHsmIp, 0, sizeof(sg_szHsmIp));
        strncpy(sg_szHsmIp, pszIp, sizeof(sg_szHsmIp)-1);
        sprintf(sg_szHsmPort, "%d", iPortNo);
        iRecvLen = sizeof(sRecvBuf);
        iRet = iHsmProc(1, "\x00", &iRecvLen, sRecvBuf);
        if(iRet != 0)
            return(-1);
#else
		return(-3);
#endif
        break;
    default:
        return(-3);
    }
    return(0);
}
// 加密机处理
// in  : iSendLen  : 发送数据长度
//       pSendBuf  : 发送数据
//       piRecvLen : 接收数据缓冲区长度
// out : piRecvLen : 接收数据长度
//       pRecvBuf  : 接收数据
// ret : 0         : 成功
//       -1        : 通讯失败
int iHsmProc(int iSendLen, void *pSendBuf, int *piRecvLen, void *pRecvBuf)
{
#ifdef WESTONE_SUPPORT
    int  iSocket;
    int  iTimeout;
    long lIns;
#endif
    int  iRet;

    switch(sg_iMode) {
    case 0:
    case 1:
        iRet = iInsProc((char *)pSendBuf, iSendLen, (char *)pRecvBuf, piRecvLen);
        break;
    case 2:
#ifdef WESTONE_SUPPORT
        iSocket = iConnectTCP(sg_szHsmIp, sg_szHsmPort, 10);
        if(iSocket < 0)
            return(-1);
        iRet = iWriteSock(iSocket, (char *)pSendBuf, iSendLen, 5);
        if(iRet != 0) {
            vCloseSock(iSocket, 0);
            return(-1);
        }
        lIns = ulStrToLong(pSendBuf, 2);
        iTimeout = 5;
        if(lIns==0xD232 || lIns==0xD23A || lIns==0xD23B)
            iTimeout = 120; // 如果指令涉及产生RSA密钥, 加大超时时间(即便如此, 软加密机仍然可能产生不出长度较大的密钥)
        iRet = iReadSock2(iSocket, (char *)pRecvBuf, *piRecvLen, iTimeout);
        vCloseSock(iSocket, 0);
        if(iRet <= 0)
            return(-1);
        *piRecvLen = iRet;
#else
		return(-1);
#endif
        break;
    }
    return(0);
}

// 读取密钥索引
// in  : psKeyIndex : 2字节的密钥索引
// ret : -1         : 密钥索引为"\xFF\xFF"
// ret : -2         : 密钥索引为"\xEE\xEE"
//       >=0        : 密钥索引
static int iGetKeyIndex(char *psKeyIndex)
{
    if(memcmp(psKeyIndex, "\xFF\xFF", 2) == 0)
        return(-1);
    if(memcmp(psKeyIndex, "\xEE\xEE", 2) == 0)
        return(-2);
    return(ulStrToLong(psKeyIndex, 2));
}

// 将模数打包成DER格式
// in  : iKeyLen : 字节单位的模数长度
//       lE      : 公共指数
//       pN      : 模数
// out : pDerN   : DER格式的公钥
// ret : 0       : 出错
//       其它    : DER格式公钥长度
static int iPackPublicKey(int iKeyLen, long lE, void *pN, void *pDerN)
{
    int iLen, iDerNLen;
    unsigned char sBuf[300];
    unsigned char *psDerN;
    
    if(iKeyLen<4 || iKeyLen>256)
        return(0);
    if(lE!=3 && lE!=65537L)
        return(0);
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
    memcpy(sBuf+iLen+1, pN, iKeyLen);
    iLen += iKeyLen+1;
    if(lE == 3) {
        memcpy(sBuf+iLen, "\x02\x01\x03", 3);
        iLen += 3;
    } else {
        memcpy(sBuf+iLen, "\x02\x03\x01\x00\x01", 5);
        iLen += 5;
    }
    // 将sBuf(长度=iLen)打包到T30
    psDerN = (unsigned char *)pDerN;
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
    return(iDerNLen);
}
// 将DER格式公钥解开成模数
// in  : iDerNLen : DER格式公钥长度
//       pDerN    : DER格式公钥
// out : plE      : 公共指数
//       pN       : 模数
// ret : 0        : 出错
//       其它     : 字节单位的模数长度
static int iUnpackPublicKey(int iDerNLen, void *pDerN, long *plE, void *pN)
{
    int iLen, iNLen, iTmp;
    unsigned char *psDerN;

    if(iDerNLen<4 || iDerNLen>270)
        return(0);
    psDerN = (unsigned char *)pDerN;
    if(*psDerN++ != 0x30)
        return(0);
    if(psDerN[0] & 0x80) {
        iTmp = psDerN[0] & 0x7F;
        iLen = ulStrToLong(psDerN+1, iTmp);
        psDerN += 1+iTmp;
        if(iDerNLen != 2+iTmp+iLen)
            return(0);
    } else {
        iLen = psDerN[0];
        psDerN ++;
        if(iDerNLen != 2+iLen)
            return(0);
    }
    // psDerN现在指向模数TLV对象与公共指数TLV对象, iLen为模数TLV对象与公共指数TLV对象长度之和
    // 先解出公共指数
    if(iLen < 5)
        return(0);
    if(memcmp(psDerN+iLen-5, "\x02\x03\x01\x00\x01", 5) == 0) {
        *plE = 65537L;
        iLen -= 5;
    } else if(memcmp(psDerN+iLen-3, "\x02\x01\x03", 3) == 0) {
        *plE = 3;
        iLen -= 3;
    } else
        return(0);
    // iLen为模数TLV对象长度
    // 再解出模数
    if(*psDerN++ != 0x02)
        return(0);
    if(psDerN[0] & 0x80) {
        iTmp = psDerN[0] & 0x7F;
        iNLen = ulStrToLong(psDerN+1, iTmp);
        psDerN += 1+iTmp;
        if(iLen != 2+iTmp+iNLen)
            return(0);
    } else {
        iNLen = *psDerN++;
        if(iLen != 2+iNLen)
            return(0);
    }
    if(iNLen > 256)
        return(0);
    memcpy(pN, psDerN+1, iNLen-1);
    return(iNLen-1);
}

// 处理报文指令
// in  : psRecvBuf : 接收的指令
//       iRecvLen  : 接收的指令长度
// out : psSendBuf : 发送的应答
//       piSendLen : 发送的应答长度
// ret : 0         : OK
static int iInsProc(uchar *psRecvBuf, int iRecvLen, uchar *psSendBuf, int *piSendLen)
{
    unsigned char sBuf[300], sBuf2[300];
    unsigned char sPrivateKey[1600];
    unsigned int uiIns; // 指令
    int  iRetCode;
    int  iInLen, iOutLen, iKeyLen, iDataLen;

    int  iKeyIndex, iKey2Index;
    int  iDivLevel, iDiv2Level;
    unsigned char *psDivData, *psDiv2Data;
    unsigned char *psKeyData, *psKey2Data;
    // 以下4行'X'结尾的变量用于通用转加密数据时表示加密密钥(以上四行用于表示解密密钥)
    int  iKeyIndexX, iKey2IndexX;
    int  iDivLevelX, iDiv2LevelX;
    unsigned char *psDivDataX, *psDiv2DataX;
    unsigned char *psKeyDataX, *psKey2DataX;

    unsigned char *psTermRand, *psCardData;
    int  iAppDataLen;
    unsigned char *psAppData, *psATC, *psAC;
    unsigned char *psAnswerCode;
    int  iMode, iModeX, iBlockFlag;
    int  iEncDecFlag, iPadFlag;
    int  iL1, iL2, iL3;
    long lE;
    char *p, *p2, *psIv, *psIvX;
    int  i;

    if(psRecvBuf[0] == 0x00)
        uiIns = 0; // 版本号查询指令, 只有这一个指令是单字节指令
    else {
        if(iRecvLen < 2) {
            psSendBuf[0] = 'E';
            psSendBuf[1] = HSMERR_MSG_LENGTH;
            *piSendLen = 2;
            return(0);
        }
        uiIns = ulStrToLong(psRecvBuf, 2);
    }

    iRetCode = 0;       // 执行成功且没有返回信息的缺省值
    *piSendLen = 1;     // 执行成功且没有返回信息的缺省值
    psSendBuf[0] = 'A'; // 执行成功的缺省报文头值
    switch(uiIns) {
    case 0x00: // 版本号查询
        // in  : Ins[1]
        // out : 'A'+版本号[4]+协议[4]+系统最后修订日期[8]
        if(iRecvLen != 1) {
            iRetCode = HSMERR_MSG_LENGTH;
            break;
        }
        memcpy(psSendBuf+1, "0100" "HXCo" "20110708", 16);
        *piSendLen = 1+16;
        break;
    case 0xC040: // 用指定索引的RSA私钥加密
    case 0xC042: // 用指定索引的RSA私钥解密
        // 软加密机不支持填充方式, 所以加解密操作结果相同
        // in  : Ins[2]+保留字[8]+填充模式[1]+Index[2]+Password[8]+InLen[2]+Data[n]
        // out : 'A'+保留字[8]+OutLen[2]+OutData[n]
        // 由于本指令包含保留字, 出错时返回报文不能正确填充, 所以预先填好出错返回信息及长度, 如果出错, iRetCode填0
        psSendBuf[0] = 'E';
        memcpy(psSendBuf+1, psRecvBuf+2, 8); // 保留字
        *piSendLen = 10;
        iRetCode = 0;

        iInLen = ulStrToLong(psRecvBuf+21, 2);
        if(iRecvLen != 23+iInLen) {
            psSendBuf[9] = HSMERR_MSG_LENGTH;
            break;
        }
        if(psRecvBuf[10] != 0) {
            psSendBuf[9] = HSMERR_PAD_FORMAT;
            break;
        }
        /* 软加密机不认证密码
        if(memcmp(psRecvBuf+13, "11111111", 8) != 0) {
            psSendBuf[9] = HSMERR_PASSWORD;
            break;
        }
        */
        iKeyIndex = iGetKeyIndex(psRecvBuf+11);
        
        iRetCode = iKeyRsaPrivateBlock0(iKeyIndex, NULL, iInLen, psRecvBuf+23, psSendBuf+11);
        if(iRetCode) {
            psSendBuf[9] = iRetCode;
            break;
        }
        
        // 执行成功
        psSendBuf[0] = 'A';
        vLongToStr(iInLen, 2, psSendBuf+9);
        *piSendLen = 11+iInLen;
        break;
    case 0xD20C: // 用传入的RSA私钥解密
        // in  : Ins[2]+加解密标志[1]+填充模式[1]+KeyLen[2]+InLen[2]+Key[n]+Data[m]
        // out : 'A'+OutLen[2]+OutData[n]
        iKeyLen = ulStrToLong(psRecvBuf+4, 2);
        iInLen = ulStrToLong(psRecvBuf+6, 2);
        if(iRecvLen != 8+iKeyLen+iInLen) {
            iRetCode = HSMERR_MSG_LENGTH;
            break;
        }
        iRetCode = iKeyRsaPrivateBlock0(-1, psRecvBuf+8, iInLen, psRecvBuf+8+iKeyLen, psSendBuf+3);
        if(iRetCode)
            break;
        vLongToStr(iInLen, 2, psSendBuf+1);
        *piSendLen = 3 + iInLen;
        break;
    case 0xD22A: // 删除密钥
        // in  : Ins[2]+KeyType[1]+KeyIndex[2]
        // out : 'A'
        if(iRecvLen != 5) {
            iRetCode = HSMERR_MSG_LENGTH;
            break;
        }
        iKeyIndex = iGetKeyIndex(psRecvBuf+3);
        iRetCode = iKeyDelete0(psRecvBuf[2], iKeyIndex);
        if(iRetCode)
            break;
        break;
    case 0xD230: // 用LMK导出指定索引的DES密钥
        // in  : Ins[2]+KeyType[1]+KeyIndex[2]
        // out : 'A'+KeyLen[2]+Key[n]+Cks[8]
        if(iRecvLen != 5) {
            iRetCode = HSMERR_MSG_LENGTH;
            break;
        }
        if(psRecvBuf[2] != 4) {
            // 4:用LMK保护
            iRetCode = HSMERR_TYPE;
            break;
        }
        iKeyIndex = iGetKeyIndex(psRecvBuf+3);
        iRetCode = iKeyBackup0(1/*1:DES密钥*/, iKeyIndex, &iOutLen, psSendBuf+2);
        if(iRetCode)
            break;
        psSendBuf[1] = 16;
        *piSendLen = 2 + 16 + 8;
        break;
    case 0xD231: // 用LMK导入DES密钥
        // in  : Ins[2]+KeyType[1]+KeyIndex[2]+KeyLen[1]+Key[16]+Cks[8]
        // out : 'A'
        if(iRecvLen != 30) {
            iRetCode = HSMERR_MSG_LENGTH;
            break;
        }
        if(psRecvBuf[2] != 4) {
            // 4:用LMK保护
            iRetCode = HSMERR_TYPE;
            break;
        }
        if(psRecvBuf[5] != 16) {
            iRetCode = HSMERR_KEY_LEN;
            break;
        }
        iKeyIndex = iGetKeyIndex(psRecvBuf+3);
        iRetCode = iKeyRestore0(1/*1:DES密钥*/, iKeyIndex, 24, psRecvBuf+6);
        break;
    case 0xD232: // 产生RSA密钥对
        // in  : Ins[2]+ModulusLen[2]+E[4]+KeyIndex[2]+Password[8](KeyIndex!=0xFFFF时存在)+Mode[1]
        //       +公钥格式[1](Mode==0、2时存在)+私钥格式[1](Mode==1、2时存在)
        // out : 'A'+公钥数据长度[2]+公钥数据[n] // Mode==0
        // out : 'A'+私钥数据长度[2]+私钥数据[m] // Mode==1
        // out : 'A'+公钥数据长度[2]+公钥数据[n]+私钥数据长度[2]+私钥数据[m] // Mode==2
        // out : 'A' // Mode==3
        p = psRecvBuf+2;
        iKeyLen = ulStrToLong(p, 2); // 模数按位长度
        iKeyLen /= 8; // 模数按字节长度
        if(iKeyLen%2 || iKeyLen>MAX_RSA_KEY_LEN) {
            iRetCode = HSMERR_KEY_LEN;
            break;
        }
        p += 2;
        lE = ulStrToLong(p, 4); // 模数按位长度
        p += 4;
        if(lE!=3 && lE!=65537L) {
            iRetCode = HSMERR_RSA_E;
            break;
        }
        iKeyIndex = iGetKeyIndex(p);
        p += 2;
        if(iKeyIndex != -1)
            p += 8; // 软加密机不处理password
        iMode = *p++;
        if((iMode!=2&&iKeyIndex==-1) || iMode<0 || iMode>3) {
            iRetCode = HSMERR_MODE;
            break;
        }
        if(iMode==0 || iMode==2) {
            if(*p++ != 0x01) {
                iRetCode = HSMERR_MODE; // 公钥必须是DER格式
                break;
            }
        }
        if(iMode==1 || iMode==2) {
            if(*p++ != 0x01) {
                iRetCode = HSMERR_MODE; // 私钥必须是DER格式
                break;
            }
        }
        if(iRecvLen != (uchar*)p-psRecvBuf) {
            iRetCode = HSMERR_MSG_LENGTH;
            break;
        }

        iRetCode = iKeyGenRsaKey0(iKeyIndex, iKeyLen, lE, sBuf/*N值*/, sPrivateKey/*私钥数据*/, &iOutLen/*私钥数据长度*/);
        iL1 = iPackPublicKey(iKeyLen, lE, sBuf, sBuf2); // 将模数打包成DER格式
        if(iMode == 0) {
            // 要输出公钥
            // 计算DER格式公钥长度
            vLongToStr(iL1, 2, psSendBuf+1);
            memcpy(psSendBuf+3, sBuf2, iL1);
            *piSendLen = 3+iL1;
        } else if(iMode == 1) {
            // 要输出私钥
            // 计算DER格式公钥长度
            vLongToStr(iOutLen, 2, psSendBuf+1);
            memcpy(psSendBuf+3, sPrivateKey, iOutLen);
            *piSendLen = 3+iOutLen;
        } else if(iMode == 2) {
            // 要输出公私钥
            // 计算DER格式公钥长度
            vLongToStr(iL1, 2, psSendBuf+1);
            memcpy(psSendBuf+3, sBuf2, iL1);
            vLongToStr(iOutLen, 2, psSendBuf+3+iL1);
            memcpy(psSendBuf+3+iL1+2, sPrivateKey, iOutLen);
            *piSendLen = 3+iL1+2+iOutLen;
        }
        break;
    case 0xD233: // 用LMK导出RSA密钥
        // in  : Ins[2]+KeyIndex[2]+Mode[1]+公钥格式[1](Mode==0、2时存在)+私钥格式[1](Mode==1、2时存在)+Password[8](Mode==1、2时存在)
        // out : (Mode=0)'A'+公钥数据长度[2]+公钥数据[n]
        // out : (Mode=1)'A'+私钥数据长度[2]+私钥数据[m]
        // out : (Mode=2)'A'+公钥数据长度[2]+公钥数据[n]+私钥数据长度[2]+私钥数据[m]
        p = psRecvBuf+2;
        iKeyIndex = iGetKeyIndex(p);
        p += 2;
        iMode = *p++;
        if(iMode==0 || iMode==2) {
            // 要导出公钥
            if(*p++ != 1) {
                iRetCode = HSMERR_MODE; // 公钥私钥必须是DER格式
                break;
            }
        }
        if(iMode==1 || iMode==2) {
            // 要导出私钥
            if(*p++ != 1) {
                iRetCode = HSMERR_MODE; // 公钥私钥必须是DER格式
                break;
            }
            p += 8; // 软加密机不处理password
        }
        if(iRecvLen != (uchar*)p-psRecvBuf) {
            iRetCode = HSMERR_MSG_LENGTH;
            break;
        }

        switch(iMode) {
        case 0: // 导出公钥
            iRetCode = iKeyRsaGetInfo0(iKeyIndex, NULL, &iL1, &lE, sBuf); // iL1 = DER格式的公钥长度
            if(iRetCode)
                break;
            iL2 = iPackPublicKey(iL1, lE, sBuf, psSendBuf+3); // 将模数打包成DER格式
            vLongToStr(iL2, 2, psSendBuf+1);
            *piSendLen = 3 + iL2;
            break;
        case 1: // 导出私钥
            iRetCode = iKeyBackup0(2/*2:RSA*/, iKeyIndex, &iL1, psSendBuf+3);
            if(iRetCode)
                break;
            vLongToStr(iL1, 2, psSendBuf+1);
            *piSendLen = 3 + iL1;
            break;
        case 2: // 导出公私钥
            iRetCode = iKeyRsaGetInfo0(iKeyIndex, NULL, &iL1, &lE, sBuf); // iL1 = DER格式的公钥长度
            if(iRetCode)
                break;
            iL2 = iPackPublicKey(iL1, lE, sBuf, psSendBuf+3); // 将模数打包成DER格式
            vLongToStr(iL2, 2, psSendBuf+1);
            *piSendLen = 3 + iL2;
            iRetCode = iKeyBackup0(2/*2:RSA*/, iKeyIndex, &iL1, psSendBuf+*piSendLen+2);
            if(iRetCode)
                break;
            vLongToStr(iL1, 2, psSendBuf+*piSendLen);
            *piSendLen += 2+iL1;
            break;
        default:
            iRetCode = HSMERR_PARA;
            break;
        }
        break;
    case 0xD234: // 用LMK导入RSA密钥
        // in  : Ins[2]+KeyIndex[2]+Password[8]+RSA密钥长度[2]+LMK加密后的RSA密钥[n]+私钥格式[1]
        // out : 'A'
        iKeyIndex = iGetKeyIndex(psRecvBuf+2);
        iKeyLen = ulStrToLong(psRecvBuf+12, 2);
        if(psRecvBuf[14+iKeyLen] != 1) {
            iRetCode = HSMERR_RSA_FORMAT; // 公钥私钥必须是DER格式
            break;
        }
        if(iRecvLen != 15+iKeyLen) {
            iRetCode = HSMERR_MSG_LENGTH;
            break;
        }
        iRetCode = iKeyRestore0(2/*2:RSA*/, iKeyIndex, iKeyLen, psRecvBuf+14);
        break;
    case 0xD235: // RSA公钥加解密
        // in  : Ins[2]+KeyIndex[2]+KeyLen[2](KeyIndex!=0xFFFF时存在)+Key[n](KeyIndex!=0xFFFF时存在)
        //       +公钥格式[1](KeyIndex!=0xFFFF时存在)+加解密标志[1]+填充模式[1]+数据长度[2]+数据[m]
        // out : 'A'+数据长度[2]+数据[m]
        p = psRecvBuf+2;
        iKeyIndex = iGetKeyIndex(p);
        p += 2;
        if(iKeyIndex == -1) {
            // 用传入的密钥加解密
            iKeyLen = ulStrToLong(p, 2); // DER格式公钥长度
            p += 2;
            psKeyData = p; // psKeyData为传入的Der格式公钥
            p += iKeyLen;
            if(*p++ != 1) {
                iRetCode = HSMERR_RSA_FORMAT; // 公钥私钥必须是DER格式
                break;
            }
        }
        p ++; // 忽略加解密标志
        if(*p++ != 0) {
            iRetCode = HSMERR_PAD_FORMAT; // 必须没有填充
            break;
        }
        iDataLen = ulStrToLong(p, 2);
        p += 2;
        p2 = p; // p2指向数据
        p += iDataLen;
        if(iRecvLen != (uchar*)p-psRecvBuf) {
            iRetCode = HSMERR_MSG_LENGTH;
            break;
        }
        if(iKeyIndex == -1) {
            iL1 = iUnpackPublicKey(iKeyLen, psKeyData, &lE, sBuf); // iL1=模长 sBuf=模数
            if(iL1 == 0) {
                iRetCode = HSMERR_RSA_FORMAT;
                break;
            }
            if(iL1 != iDataLen) {
                iRetCode = HSMERR_DATAIN_LEN;
                break;
            }
            iRetCode = iKeyRsaPublicBlock20(iDataLen, lE, sBuf, p2, psSendBuf+3);
            if(iRetCode)
                break;
        } else {
            // 用索引指定的密钥加解密
            iRetCode = iKeyRsaPublicBlock0(iKeyIndex, iDataLen, p2, psSendBuf+3);
            if(iRetCode)
                break;
        }
        vLongToStr(iDataLen, 2, psSendBuf+1);
        *piSendLen = 3 + iDataLen;
        break;
    case 0xD237: // 产生IC卡外部认证密文
        // in  : Ins[2]+KmcIndex[2]+KmcLen[1](KmcIndex==0xFFFF时存在)+Kmc[n](KmcIndex==0xFFFF时存在)+KmcDivLevel[1]+KmcDivData[n]+TermRand[8]+CardData[28]
        // out : 'A'+外部认证密文[8]+外部认证指令Mac[8]
        p = psRecvBuf+2;
        iKeyIndex = iGetKeyIndex(p);
        p += 2;
        psKeyData = NULL;
        if(iKeyIndex == -1) {
            if(*p++ != 16) {
                iRetCode = HSMERR_KEY_LEN;
                break;
            }
            psKeyData = p;
            p += 16;
        }
        iDivLevel = *p++;
        psDivData = p;
        p += iDivLevel*8;
        psTermRand = p;
        p += 8;
        psCardData = p;
        p += 28;
        if(iRecvLen != (uchar*)p-psRecvBuf) {
            iRetCode = HSMERR_MSG_LENGTH;
            break;
        }
        iRetCode = iKeyCalExtAuth0(iKeyIndex, psKeyData, iDivLevel, psDivData, psTermRand, psCardData, psSendBuf+1);
        if(iRetCode)
            break;
        *piSendLen = 17;
        break;
    case 0xD238: // 产生IC卡Des密钥密文(即时发卡)
        // in  : Ins[2]+KmcIndex[2]+KmcLen[1](KmcIndex==0xFFFF时存在)+Kmc[n](KmcIndex==0xFFFF时存在)+KmcDivLevel[1]+KmcDivData[n]
        //       +TermRand[8]+CardData[28]+AppKeyIndex[2]+AppKeyLen[1](AppKeyIndex==0xFFFF时存在)+AppKey[n](AppKeyIndex==0xFFFF时存在)
        //       +AppDivLevel[1]+AppDivData[n]
        // out : 'A'+IC卡密钥密文[16]+IC卡密钥校验值[8]
        p = psRecvBuf+2;
        iKeyIndex = iGetKeyIndex(p); // iKeyIndex、psKeyData、iDivLevel、psDivData用于指示KMC
        p += 2;
        psKeyData = NULL;
        if(iKeyIndex == -1) {
            if(*p++ != 16) {
                iRetCode = HSMERR_KEY_LEN;
                break;
            }
            psKeyData = p;
            p += 16;
        }
        iDivLevel = *p++;
        psDivData = p;
        p += iDivLevel*8;
        psTermRand = p;
        p += 8;
        psCardData = p;
        p += 28;
        iKey2Index = iGetKeyIndex(p); // iKey2Index、psKey2Data、iDiv2Level、psDiv2Data用于指示AppKey
        p += 2;
        psKey2Data = NULL;
        if(iKey2Index == -1) {
            if(*p++ != 16) {
                iRetCode = HSMERR_KEY_LEN;
                break;
            }
            psKey2Data = p;
            p += 16;
        }
        iDiv2Level = *p++;
        psDiv2Data = p;
        p += iDiv2Level*8;
        if(iRecvLen != (uchar*)p-psRecvBuf) {
            iRetCode = HSMERR_MSG_LENGTH;
            break;
        }
        // 计算AppKey密文
        iRetCode = iKeyCalDesKey0(iKeyIndex, psKeyData, iDivLevel, psDivData, psTermRand, psCardData, iKey2Index, psKey2Data, iDiv2Level, psDiv2Data, psSendBuf+1);
        if(iRetCode)
            break;
        *piSendLen = 25;
        break;
    case 0xD239: // 产生IC卡个人密码密文(即时发卡)
        // in  : Ins[2]+KmcIndex[2]+KmcLen[1](KmcIndex==0xFFFF时存在)+Kmc[n](KmcIndex==0xFFFF时存在)+KmcDivLevel[1]+KmcDivData[n]
        //       +TermRand[8]+CardData[28]+PinBlock[8]
        // out : 'A'+IC卡个人密码密文[8]
        p = psRecvBuf+2;
        iKeyIndex = iGetKeyIndex(p); // iKeyIndex、psKeyData、iDivLevel、psDivData用于指示KMC
        p += 2;
        psKeyData = NULL;
        if(iKeyIndex == -1) {
            if(*p++ != 16) {
                iRetCode = HSMERR_KEY_LEN;
                break;
            }
            psKeyData = p;
            p += 16;
        }
        iDivLevel = *p++;
        psDivData = p;
        p += iDivLevel*8;
        psTermRand = p;
        p += 8;
        psCardData = p;
        p += 28; // 当前p指向PinBlock
        if(iRecvLen != (uchar*)p-psRecvBuf+8) {
            iRetCode = HSMERR_MSG_LENGTH;
            break;
        }
        iRetCode = iKeyCalPin0(iKeyIndex, psKeyData, iDivLevel, psDivData, psTermRand, psCardData, p, psSendBuf+1);
        if(iRetCode)
            break;
        *piSendLen = 9;
        break;
    case 0xD23A: // 产生IC卡RSA密钥密文(即时发卡)
        // in  : Ins[2]+模长(按位)[2]+公共指数[4]+公钥格式[1]+私钥格式[1]+
        //       KmcIndex[2]+KmcLen[1](KmcIndex==0xFFFF时存在)+Kmc[n](KmcIndex==0xFFFF时存在)+KmcDivLevel[1]+KmcDivData[n]
        //       +TermRand[8]+CardData[28]
        // out : 'A'+公钥数据长度[2]+公钥数据[n]+
        //           D长度[2]+D[n]+
        //           P长度[2]+P[n]+
        //           Q长度[2]+Q[n]+
        //           dP长度[2]+dP[n]+
        //           dQ长度[2]+dQ[n]+
        //           qInv长度[2]+qInv[n]
        p = psRecvBuf+2;
        iKeyLen = ulStrToLong(p, 2); // 模长(按位)
        iKeyLen /= 8; // 模长(按字节)
        p += 2;
        lE = ulStrToLong(p, 4);
        p += 4;
        if(*p++ != 0x01) {
            iRetCode = HSMERR_RSA_FORMAT;
            break;
        }
        if(*p++ != 0x02) {
            iRetCode = HSMERR_PARA;
            break;
        }
        iKeyIndex = iGetKeyIndex(p); // iKeyIndex、psKeyData、iDivLevel、psDivData用于指示KMC
        p += 2;
        psKeyData = NULL;
        if(iKeyIndex == -1) {
            if(*p++ != 16) {
                iRetCode = HSMERR_KEY_LEN;
                break;
            }
            psKeyData = p;
            p += 16;
        }
        iDivLevel = *p++;
        psDivData = p;
        p += iDivLevel*8;
        psTermRand = p;
        p += 8;
        psCardData = p;
        p += 28;
        if(iRecvLen != (uchar*)p-psRecvBuf) {
            iRetCode = HSMERR_MSG_LENGTH;
            break;
        }
        iRetCode = iKeyCalRsaKey0(iKeyIndex, psKeyData, iDivLevel, psDivData, psTermRand, psCardData, iKeyLen, lE, sPrivateKey);
        if(iRetCode)
            break;
        iL1 = (iKeyLen+8)/8*8;   // D长度
        iL2 = (iKeyLen/2+8)/8*8; // p、q等长度
        p = psSendBuf+1;  // p用来跟踪应答报文位置
        p2 = sPrivateKey; // p2用来跟踪本地计算结果数据
        iL3 = iPackPublicKey(iKeyLen, lE, p2, p+2); // Der格式公钥长度
        if(iL3 == 0) {
            iRetCode = HSMERR_PARA;
            break;
        }
        vLongToStr(iL3, 2, p); // N
        p += iL3+2;
        p2 += iKeyLen;
        vLongToStr(iL1, 2, p); // D
        memcpy(p+2, p2, iL1);
        p += iL1+2;
        p2 += iL1;
        for(i=0; i<5; i++) { // p q dp dq qinv
            vLongToStr(iL2, 2, p);
            memcpy(p+2, p2, iL2);
            p += iL2+2;
            p2 += iL2;
        }
        *piSendLen = 1+14+iL3+iL1+5*iL2;
        break;
    case 0xD23B: // 产生IC卡RSA密钥密文(数据准备)
        // in  : Ins[2]+模长(按位)[2]+公共指数[4]+公钥格式[1]+私钥格式[1]+
        //       TkIndex[2]+TkLen[1](TkIndex==0xFFFF时存在)+Tk[n](TkIndex==0xFFFF时存在)+TkDivLevel[1]+TkDivData[n]
        // out : 'A'+公钥数据长度[2]+公钥数据[n]+
        //           D长度[2]+D[n]+
        //           P长度[2]+P[n]+
        //           Q长度[2]+Q[n]+
        //           dP长度[2]+dP[n]+
        //           dQ长度[2]+dQ[n]+
        //           qInv长度[2]+qInv[n]
        p = psRecvBuf+2;
        iKeyLen = ulStrToLong(p, 2); // 模长(按位)
        iKeyLen /= 8; // 模长(按字节)
        p += 2;
        lE = ulStrToLong(p, 4);
        p += 4;
        if(*p++ != 0x01) {
            iRetCode = HSMERR_RSA_FORMAT;
            break;
        }
        if(*p++ != 0x02) {
            iRetCode = HSMERR_PARA;
            break;
        }
        iKeyIndex = iGetKeyIndex(p); // iKeyIndex、psKeyData、iDivLevel、psDivData用于指示Tk
        p += 2;
        psKeyData = NULL;
        if(iKeyIndex == -1) {
            if(*p++ != 16) {
                iRetCode = HSMERR_KEY_LEN;
                break;
            }
            psKeyData = p;
            p += 16;
        }
        iDivLevel = *p++;
        psDivData = p;
        p += iDivLevel*8;
        if(iRecvLen != (uchar*)p-psRecvBuf) {
            iRetCode = HSMERR_MSG_LENGTH;
            break;
        }
        iRetCode = iKeyDpCalRsaKey0(iKeyIndex, psKeyData, iDivLevel, psDivData, iKeyLen, lE, sPrivateKey);
        if(iRetCode)
            break;
        iL1 = (iKeyLen+8)/8*8;   // D长度
        iL2 = (iKeyLen/2+8)/8*8; // p、q等长度
        p = psSendBuf+1;  // p用来跟踪应答报文位置
        p2 = sPrivateKey; // p2用来跟踪本地计算结果数据
        iL3 = iPackPublicKey(iKeyLen, lE, p2, p+2); // Der格式公钥长度
        if(iL3 == 0) {
            iRetCode = HSMERR_PARA;
            break;
        }
        vLongToStr(iL3, 2, p); // N
        p += iL3+2;
        p2 += iKeyLen;
        vLongToStr(iL1, 2, p); // D
        memcpy(p+2, p2, iL1);
        p += iL1+2;
        p2 += iL1;
        for(i=0; i<5; i++) { // p q dp dq qinv
            vLongToStr(iL2, 2, p);
            memcpy(p+2, p2, iL2);
            p += iL2+2;
            p2 += iL2;
        }
        *piSendLen = 1+14+iL3+iL1+5*iL2;
        break;
    case 0xD23C: // 验证AC, 并产生ARPC
        // in  : Ins[2]+Mode[1]+KeyIndex[2]+KeyLen[1](KeyIndex==0xFFFF时存在)+Key[n](KeyIndex==0xFFFF时存在)
        //       KeyDivLevel[1]+KeyDivData[n]+ATC[2]+AC[8]+AppDataLen[2](Mode==0、1时存在)+AppData[m](Mode==0、1时存在)+应答码[2](Mode==1、2时存在)
        // out(Mode==0)    : 'A'
        // out(Mode==1、2) : 'A'+ARPC[8]
        p = psRecvBuf+2;
        iMode = *p++;
        if(iMode!=0 && iMode!=1 && iMode!=2) {
            iRetCode = HSMERR_PARA;
            break;
        }
        iKeyIndex = iGetKeyIndex(p); // iKeyIndex、psKeyData、iDivLevel、psDivData用于指示AppKey
        p += 2;
        psKeyData = NULL;
        if(iKeyIndex == -1) {
            if(*p++ != 16) {
                iRetCode = HSMERR_KEY_LEN;
                break;
            }
            psKeyData = p;
            p += 16;
        }
        iDivLevel = *p++;
        psDivData = p;
        p += iDivLevel*8;
        
        psATC = p;
        p += 2;
        psAC = p;
        p += 8;
        if(iMode==0 || iMode==1) {
            iAppDataLen = ulStrToLong(p, 2);
            p += 2;
            psAppData = p;
            p += iAppDataLen;
        } else
            iAppDataLen = 0;
        if(iMode==1 || iMode==2) {
            psAnswerCode = p;
            p += 2;
        } else
            psAnswerCode = NULL;
        if(iRecvLen != (uchar*)p-psRecvBuf) {
            iRetCode = HSMERR_MSG_LENGTH;
            break;
        }
        iRetCode = iKeyVerifyAc0(iKeyIndex, psKeyData, iDivLevel, psDivData, psATC, iAppDataLen, psAppData, psAC, psAnswerCode, psSendBuf+1);
        if(iRetCode)
            break;
        if(iMode==1 || iMode==2)
            *piSendLen = 9;
        break;
    case 0xD23D: // 产生IC卡修改KMC密文
        // in  : Ins[2]+KmcIndex[2]+KmcLen[1](KmcIndex==0xFFFF时存在)+Kmc[n](KmcIndex==0xFFFF时存在)+KmcDivLevel[1]+KmcDivData[n]
        //       +TermRand[8]+CardData[28]
        //       +NewKmcIndex[2]+NewKmcLen[1](NewKmcIndex==0xFFFF时存在)+NewKmc[n](NewKmcIndex==0xFFFF时存在)+NewKmcDivLevel[1]+NewKmcDivData[n]
        // out : 'A'+Auth子密钥密文[16]+Auth子密钥校验值[8]+Mac子密钥密文[16]+Mac子密钥校验值[8]+Enc子密钥密文[16]+Enc子密钥校验值[8]
        p = psRecvBuf+2;
        iKeyIndex = iGetKeyIndex(p); // iKeyIndex、psKeyData、iDivLevel、psDivData用于指示KMC
        p += 2;
        psKeyData = NULL;
        if(iKeyIndex == -1) {
            if(*p++ != 16) {
                iRetCode = HSMERR_KEY_LEN;
                break;
            }
            psKeyData = p;
            p += 16;
        }
        iDivLevel = *p++;
        psDivData = p;
        p += iDivLevel*8;
        psTermRand = p;
        p += 8;
        psCardData = p;
        p += 28;
        iKey2Index = iGetKeyIndex(p); // iKey2Index、psKey2Data、iDiv2Level、psDiv2Data用于指示NewKMC
        p += 2;
        psKey2Data = NULL;
        if(iKey2Index == -1) {
            if(*p++ != 16) {
                iRetCode = HSMERR_KEY_LEN;
                break;
            }
            psKey2Data = p;
            p += 16;
        }
        iDiv2Level = *p++;
        psDiv2Data = p;
        p += iDiv2Level*8;
        if(iRecvLen != (uchar*)p-psRecvBuf) {
            iRetCode = HSMERR_MSG_LENGTH;
            break;
        }
        iRetCode = iKeyCalChangeKmc0(iKeyIndex, psKeyData, iDivLevel, psDivData, psTermRand, psCardData,
                                    iKey2Index, psKey2Data, iDiv2Level, psDiv2Data, psSendBuf+1);
        if(iRetCode)
            break;
        *piSendLen = 1+24*3;
        break;
    case 0xD23E: // 转加密发卡数据(用数据准备数据发卡)
        // in  : Ins[2]+KeyIndex[2]+KeyLen[1](KeyIndex==0xFFFF时存在)+Key[n](KeyIndex==0xFFFF时存在)+KeyDivLevel[1]+KeyDivData[n]
        //       +KmcIndex[2]+KmcLen[1](KmcIndex==0xFFFF时存在)+Kmc[n](KmcIndex==0xFFFF时存在)+KmcDivLevel[1]+KmcDivData[n]
        //       +TermRand[8]+CardData[28]+解密模式[1]+IV[8](解密模式==1时存在)+DataLen[2]+Data[m]
        // out : 'A'+OutLen[2]+OutData[n]
        p = psRecvBuf+2;
        iKeyIndex = iGetKeyIndex(p); // iKeyIndex、psKeyData、iDivLevel、psDivData用于指示原加密密钥
        p += 2;
        psKeyData = NULL;
        if(iKeyIndex == -1) {
            if(*p++ != 16) {
                iRetCode = HSMERR_KEY_LEN;
                break;
            }
            psKeyData = p;
            p += 16;
        }
        iDivLevel = *p++;
        psDivData = p;
        p += iDivLevel*8;
        iKey2Index = iGetKeyIndex(p); // iKey2Index、psKey2Data、iDiv2Level、psDiv2Data用于指示KMC
        p += 2;
        psKey2Data = NULL;
        if(iKey2Index == -1) {
            if(*p++ != 16) {
                iRetCode = HSMERR_KEY_LEN;
                break;
            }
            psKey2Data = p;
            p += 16;
        }
        iDiv2Level = *p++;
        psDiv2Data = p;
        p += iDiv2Level*8;
        psTermRand = p;
        p += 8;
        psCardData = p;
        p += 28;
        if(*p++ != 0) {
            iRetCode = HSMERR_NOT_ECBCBC;
            break;
        }
        iAppDataLen = ulStrToLong(p, 2);
        p += 2;
        psAppData = p;
        p += iAppDataLen;
        if(iRecvLen != (uchar*)p-psRecvBuf) {
            iRetCode = HSMERR_MSG_LENGTH;
            break;
        }
        iRetCode = iKeyReEncData0(iKey2Index, psKey2Data, iDiv2Level, psDiv2Data, psTermRand, psCardData,
                                 iKeyIndex, psKeyData, iDivLevel, psDivData, iAppDataLen, psAppData,
                                 psSendBuf+3);
        if(iRetCode)
            break;
        vLongToStr(iAppDataLen, 2, psSendBuf+1);
        *piSendLen = 3+iAppDataLen;
        break;
    case 0xD23F: // 转加密PinBlock(用数据准备数据发卡)
        // in  : Ins[2]+KeyIndex[2]+KeyLen[1](KeyIndex==0xFFFF时存在)+Key[n](KeyIndex==0xFFFF时存在)+KeyDivLevel[1]+KeyDivData[n]
        //       +KmcIndex[2]+KmcLen[1](KmcIndex==0xFFFF时存在)+Kmc[n](KmcIndex==0xFFFF时存在)+KmcDivLevel[1]+KmcDivData[n]
        //       +TermRand[8]+CardData[28]+原PinBlock格式[1]+新PinBlock格式[1]+PinBlock[8]
        // out : 'A'+OutData[8]
        p = psRecvBuf+2;
        iKeyIndex = iGetKeyIndex(p); // iKeyIndex、psKeyData、iDivLevel、psDivData用于指示原加密密钥
        p += 2;
        psKeyData = NULL;
        if(iKeyIndex == -1) {
            if(*p++ != 16) {
                iRetCode = HSMERR_KEY_LEN;
                break;
            }
            psKeyData = p;
            p += 16;
        }
        iDivLevel = *p++;
        psDivData = p;
        p += iDivLevel*8;
        iKey2Index = iGetKeyIndex(p); // iKey2Index、psKey2Data、iDiv2Level、psDiv2Data用于指示KMC
        p += 2;
        psKey2Data = NULL;
        if(iKey2Index == -1) {
            if(*p++ != 16) {
                iRetCode = HSMERR_KEY_LEN;
                break;
            }
            psKey2Data = p;
            p += 16;
        }
        iDiv2Level = *p++;
        psDiv2Data = p;
        p += iDiv2Level*8;
        psTermRand = p;
        p += 8;
        psCardData = p;
        p += 28;
        p++; // 不关心原加密格式
        if(*p++ != 0x02) {
            // 必须加密成格式2
            iRetCode = HSMERR_PARA;
            break;
        }
        psAppData = p; // psAppData即PinBlock
        p += 8;
        if(iRecvLen != (uchar*)p-psRecvBuf) {
            iRetCode = HSMERR_MSG_LENGTH;
            break;
        }
        iRetCode = iKeyReEncPin0(iKey2Index, psKey2Data, iDiv2Level, psDiv2Data, psTermRand, psCardData,
                                iKeyIndex, psKeyData, iDivLevel, psDivData, psAppData, psSendBuf+1);
        if(iRetCode)
            break;
        *piSendLen = 9;
        break;
    case 0xD240: // 发卡行脚本加密
        // in  : Ins[2]+Mode[1]+IcType[1]+KeyIndex[2]+KeyLen[1](KeyIndex==0xFFFF时存在)+Key[n](KeyIndex==0xFFFF时存在)+KeyDivLevel[1]+KeyDivData[n]
        //       +ATC[2]+加密模式[1]+IV[8](加密模式==1时存在)+DataLen[2]+Data[m]
        // out : 'A'+OutLen[2]+OutData[n]
        p = psRecvBuf+2;
        if(*p++ != 1) {
            // 只允许进行加密
            iRetCode = HSMERR_PARA;
            break;
        }
        if(*p++ != 1) {
            // 只允许PBOC2.0类型卡片
            iRetCode = HSMERR_PARA;
            break;
        }
        iKeyIndex = iGetKeyIndex(p); // iKeyIndex、psKeyData、iDivLevel、psDivData用于指示AppKey
        p += 2;
        psKeyData = NULL;
        if(iKeyIndex == -1) {
            if(*p++ != 16) {
                iRetCode = HSMERR_KEY_LEN;
                break;
            }
            psKeyData = p;
            p += 16;
        }
        iDivLevel = *p++;
        psDivData = p;
        p += iDivLevel*8;
        
        psATC = p;
        p += 2;
        if(*p++ != 0) {
            // 只支持ECB模式
            iRetCode = HSMERR_PARA;
            break;
        }
        iAppDataLen = ulStrToLong(p, 2);
        p += 2;
        psAppData = p;
        p += iAppDataLen;
        if(iRecvLen != (uchar*)p-psRecvBuf) {
            iRetCode = HSMERR_MSG_LENGTH;
            break;
        }
        iRetCode = iKeyScriptEncData0(iKeyIndex, psKeyData, iDivLevel, psDivData, psATC, 0/*不是密码块*/, iAppDataLen, psAppData,
                                     &iOutLen, psSendBuf+3);
        if(iRetCode)
            break;
        vLongToStr(iOutLen, 2, psSendBuf+1);
        *piSendLen = 3+iOutLen;
        break;
    case 0xD241: // 发卡行脚本Pin加密
        // in  : Ins[2]+IcType[1]+KeyIndex[2]+KeyLen[1](KeyIndex==0xFFFF时存在)+Key[n](KeyIndex==0xFFFF时存在)+KeyDivLevel[1]+KeyDivData[n]
        //       +ATC[2]+PinBlock[8]
        // out : 'A'+OutLen[2]+OutData[n]
        p = psRecvBuf+2;
        if(*p++ != 1) {
            // 只允许PBOC2.0类型卡片
            iRetCode = HSMERR_PARA;
            break;
        }
        iKeyIndex = iGetKeyIndex(p); // iKeyIndex、psKeyData、iDivLevel、psDivData用于指示AppKey
        p += 2;
        psKeyData = NULL;
        if(iKeyIndex == -1) {
            if(*p++ != 16) {
                iRetCode = HSMERR_KEY_LEN;
                break;
            }
            psKeyData = p;
            p += 16;
        }
        iDivLevel = *p++;
        psDivData = p;
        p += iDivLevel*8;
        
        psATC = p;
        p += 2;
        psAppData = p; // psAppData即是PinBlock
        p += 8;
        if(iRecvLen != (uchar*)p-psRecvBuf) {
            iRetCode = HSMERR_MSG_LENGTH;
            break;
        }
        iRetCode = iKeyScriptEncData0(iKeyIndex, psKeyData, iDivLevel, psDivData, psATC, 1/*是密码块*/, 8, psAppData,
                                     &iOutLen, psSendBuf+3);
        if(iRetCode)
            break;
        vLongToStr(iOutLen, 2, psSendBuf+1);
        *piSendLen = 3+iOutLen;
        break;
    case 0xD242: // 发卡行脚本Mac计算
        // in  : Ins[2]+IcType[1]+KeyIndex[2]+KeyLen[1](KeyIndex==0xFFFF时存在)+Key[n](KeyIndex==0xFFFF时存在)+KeyDivLevel[1]+KeyDivData[n]
        //       +ATC[2]+AppDataLen[2]+AppData[n]
        // out : 'A'+Mac[8]
        p = psRecvBuf+2;
        if(*p++ != 1) {
            // 只允许PBOC2.0类型卡片
            iRetCode = HSMERR_PARA;
            break;
        }
        iKeyIndex = iGetKeyIndex(p); // iKeyIndex、psKeyData、iDivLevel、psDivData用于指示AppKey
        p += 2;
        psKeyData = NULL;
        if(iKeyIndex == -1) {
            if(*p++ != 16) {
                iRetCode = HSMERR_KEY_LEN;
                break;
            }
            psKeyData = p;
            p += 16;
        }
        iDivLevel = *p++;
        psDivData = p;
        p += iDivLevel*8;
        
        psATC = p;
        p += 2;
        iAppDataLen = ulStrToLong(p, 2);
        p += 2;
        psAppData = p;
        p += iAppDataLen;
        if(iRecvLen != (uchar*)p-psRecvBuf) {
            iRetCode = HSMERR_MSG_LENGTH;
            break;
        }
        iRetCode = iKeyScriptMac0(iKeyIndex, psKeyData, iDivLevel, psDivData, psATC, iAppDataLen, psAppData, psSendBuf+1);
        if(iRetCode)
            break;
        *piSendLen = 9;
        break;
    case 0xD243: // 计算Mac
        // in  : Ins[2]+Mac密钥索引[2](0xFFFF:LMK加密输入, 0xEEEE:保护密钥加密输入)
        //       +Mac密钥长度[1]+Mac密钥[n]+Mac密钥分散次数[1]+Mac密钥分散数据[n]+Mac密钥分散算法[1](Mac密钥分散次数不为0时存在)
        //       +Mac密钥使用过程密钥标志[1]+保护密钥索引[2](0xFFFF:LMK加密输入)+保护密钥长度[1]+保护密钥[n]+保护密钥分散次数[1]
        //       +保护密钥分散数据[n]+保护密钥分散算法[1]+保护密钥使用过程密钥标志[1]+Mac算法[1]+IV[8]+数据块标志[1]+Mac数据长度[2]+Mac数据[n]
        // out : 'A'+Mac[8]
        p = psRecvBuf+2;
        iKeyIndex = iGetKeyIndex(p); // iKeyIndex、psKeyData、iDivLevel、psDivData用于指示Mac密钥
        p += 2;
        psKeyData = NULL;
        if(iKeyIndex < 0) {
            // LMK加密或保护密钥加密
            if(*p++ != 16) {
                iRetCode = HSMERR_KEY_LEN;
                break;
            }
            psKeyData = p;
            p += 16;
        }
        iDivLevel = *p++;
        psDivData = p;
        p += iDivLevel*8;
        if(iDivLevel) {
            if(*p++ != 1) {
                iRetCode = HSMERR_PARA; // 分散算法必须为1
                break;
            }
        }
        if(*p++ != 0) {
            // Mac密钥必须不使用过程密钥
            iRetCode = HSMERR_PARA;
            break;
        }
        if(iKeyIndex == -2) {
            // 保护密钥加密
            iKey2Index = iGetKeyIndex(p); // iKey2Index、psKey2Data、iDiv2Level、psDiv2Data用于指示保护密钥
            p += 2;
            psKey2Data = NULL;
            if(iKey2Index == -1) {
                // LMK加密
                if(*p++ != 16) {
                    iRetCode = HSMERR_KEY_LEN;
                    break;
                }
                psKey2Data = p;
                p += 16;
            }
            iDiv2Level = *p++;
            psDiv2Data = p;
            p += iDiv2Level*8;
            if(iDiv2Level) {
                if(*p++ != 1) {
                    iRetCode = HSMERR_PARA; // 分散算法必须为1
                    break;
                }
            }
            if(*p++ != 0) {
                // 保护密钥必须不使用过程密钥
                iRetCode = HSMERR_PARA;
                break;
            }
        }
        
        iMode = *p++;   // iMode=Mac算法 1:ANSI99(full) 2:ANSI919(retail) 3:XOR 4:PBOC3DES
                        // 算法1、2、3在数据长度不是8的倍数时, 后补0x00至8的倍数
                        // 算法4先补0x80, 之后如果长度不是8的倍数时, 后补0x00至8的倍数
        if(iMode<1 || iMode>4) {
            iRetCode = HSMERR_MODE;
            break;
        }
        psIv = p;
        p += 8;
        iBlockFlag = *p++; // 数据块标志, 1:第一块或中间块 2:唯一块或最后一块
        if(iBlockFlag!=1 && iBlockFlag!=2) {
            iRetCode = HSMERR_FLAG;
            break;
        }
        iAppDataLen = ulStrToLong(p, 2);
        p += 2;
        psAppData = p;
        p += iAppDataLen;
        if(iRecvLen != (uchar*)p-psRecvBuf) {
            iRetCode = HSMERR_MSG_LENGTH;
            break;
        }

        if(iKeyIndex != -2) {
            // Mac密钥被LMK保护或Mac密钥为索引密钥
            iRetCode = iKeyDpCalMacFull0(iKeyIndex, psKeyData, iDivLevel, psDivData, NULL, 0, NULL, iMode, psIv, iAppDataLen, psAppData, psSendBuf+1, iBlockFlag);
        } else {
            iRetCode = iKeyDpCalMacFull0(iKey2Index, psKey2Data, iDiv2Level, psDiv2Data, psKeyData, iDivLevel, psDivData, iMode, psIv, iAppDataLen, psAppData, psSendBuf+1, iBlockFlag);
        }
        if(iRetCode)
            break;
        *piSendLen = 9;
        break;
    case 0xD244: // 生成DES密钥
        // in  : Ins[2]+保存位置[2]+KeyLen[1]+Mode[1]+保护密钥索引[1]+保护密钥长度(保护密钥索引==0xFFFF时存在)+保护密钥[n](保护密钥索引==0xFFFF时存在)
        //       +KeyDivLevel[1]+KeyDivData[n]
        // out : 'A'+密钥长度[1]+Mac保护密钥加密后的密钥[n]+Cks[8]
        p = psRecvBuf+2;
        iKeyIndex = iGetKeyIndex(p); // iKeyIndex用于指示保存位置
        p += 2;
        if(*p++ != 16) {
            iRetCode = HSMERR_KEY_LEN;
            break;
        }
        iMode = *p++;
        if(iMode<0 || iMode>3 || (iMode==0&&iKeyIndex==-1)) {
            iRetCode = HSMERR_MODE;
            break;
        }
        if(iMode==2 || iMode==3) {
            // 需要保护密钥
            iKey2Index = iGetKeyIndex(p); // iKey2Index、psKey2Data、iDiv2Level、psDiv2Data用于指示保护密钥索引
            p += 2;
            psKey2Data = NULL;
            if(iKey2Index == -1) {
                if(*p++ != 16) {
                    iRetCode = HSMERR_KEY_LEN;
                    break;
                }
                psKey2Data = p;
                p += 16;
            }
            iDiv2Level = *p++;
            psDiv2Data = p;
            p += iDiv2Level*8;
        } // 需要保护密钥
        if(iRecvLen != (uchar*)p-psRecvBuf) {
            iRetCode = HSMERR_MSG_LENGTH;
            break;
        }
        if(iMode == 0) {
            // 不导出
            iRetCode = iKeyGenDesKey0(iKeyIndex, NULL);
        } else if(iMode == 1) {
            // LMK导出
            iRetCode = iKeyGenDesKey0(iKeyIndex, psSendBuf+1);
            if(iRetCode)
                break;
            *piSendLen = 1+16+8;
        } else if(iMode == 2) {
            // 保护密钥导出
            iRetCode = iKeyDpGenDesKey0(iKey2Index, psKey2Data, iDiv2Level, psDiv2Data, psSendBuf+1);
            if(iRetCode)
                break;
            *piSendLen = 1+16+8;
        } else {
            // 被LMK和保护密钥导出
            iRetCode = HSMERR_PARA; // 暂不支持
            break;
        }
        break;
    case 0xD245: // 导出分散密钥
        // in  : Ins[2]+TkIndex[2]+TkLen[1](TkIndex==0xFFFF时存在)+Tk[n](TkIndex==0xFFFF时存在)+TkDivLevel[1]+TkDivData[n]
        //       +Tk分散算法[1](TkDivLevel不为0时存在)+AppKeyIndex[2]+AppKeyLen[1](AppKeyIndex==0xFFFF时存在)+AppKey[n](AppKeyIndex==0xFFFF时存在)
        //       +AppKeyDivLevel[1]+AppKeyDivData[n]+AppKey分散算法[1](AppKeyDivLevel不为0时存在)
        // out : 'A'+密钥长度[1]+密钥[n]+Cks[8]
        p = psRecvBuf+2;
        iKeyIndex = iGetKeyIndex(p); // iKeyIndex、psKeyData、iDivLevel、psDivData用于指示Tk
        p += 2;
        psKeyData = NULL;
        if(iKeyIndex == -1) {
            if(*p++ != 16) {
                iRetCode = HSMERR_KEY_LEN;
                break;
            }
            psKeyData = p;
            p += 16;
        }
        iDivLevel = *p++;
        psDivData = p;
        p += iDivLevel*8;
        if(iDivLevel) {
            if(*p++ != 1) {
                iRetCode = HSMERR_PARA;
                break;
            }
        }
        iKey2Index = iGetKeyIndex(p); // iKey2Index、psKey2Data、iDiv2Level、psDiv2Data用于指示应用密钥
        p += 2;
        psKey2Data = NULL;
        if(iKey2Index == -1) {
            if(*p++ != 16) {
                iRetCode = HSMERR_KEY_LEN;
                break;
            }
            psKey2Data = p;
            p += 16;
        }
        iDiv2Level = *p++;
        psDiv2Data = p;
        p += iDiv2Level*8;
        if(iDiv2Level) {
            if(*p++ != 1) {
                iRetCode = HSMERR_PARA;
                break;
            }
        }
        if(iRecvLen != (uchar*)p-psRecvBuf) {
            iRetCode = HSMERR_MSG_LENGTH;
            break;
        }
        iRetCode = iKeyDpCalDesKey0(iKeyIndex, psKeyData, iDivLevel, psDivData, iKey2Index, psKey2Data, iDiv2Level, psDiv2Data, psSendBuf+2);
        if(iRetCode)
            break;
        psSendBuf[1] = 16;
        *piSendLen = 2+16+8;
        break;
    case 0xD246: // 加解密数据
        // in  : Ins[2]+加解密密钥索引[2](0xFFFF:LMK加密输入, 0xEEEE:保护密钥加密输入)
        //       +加解密密钥长度[1]+加解密密钥[n]+加解密密钥分散次数[1]+加解密密钥分散数据[n]+加解密密钥分散算法[1](加解密密钥分散次数不为0时存在)
        //       +加解密密钥使用过程密钥标志[1]+保护密钥索引[2](0xFFFF:LMK加密输入)+保护密钥长度[1]+保护密钥[n]+保护密钥分散次数[1]
        //       +保护密钥分散数据[n]+保护密钥分散算法[1]+保护密钥使用过程密钥标志[1]+数据长度[2]+数据[n]
        //       +填充模式[1]+加解密标志[1]+加解密模式[1]+Iv[8](CBC模式存在)
        // out : 'A'+输出长度[2]+输出数据[n]
        p = psRecvBuf+2;
        iKeyIndex = iGetKeyIndex(p);
        p += 2;
        psKeyData = NULL;
        if(iKeyIndex < 0) {
            // LMK加密或保护密钥加密
            if(*p++ != 16) {
                iRetCode = HSMERR_KEY_LEN;
                break;
            }
            psKeyData = p;
            p += 16;
        }
        iDivLevel = *p++;
        psDivData = p;
        p += iDivLevel*8;
        if(iDivLevel) {
            if(*p++ != 1) {
                iRetCode = HSMERR_PARA; // 分散算法必须为1
                break;
            }
        }
        if(*p++ != 0) {
            // Mac密钥必须不使用过程密钥
            iRetCode = HSMERR_PARA;
            break;
        }
        if(iKeyIndex == -2) {
            // 保护密钥加密
            iKey2Index = iGetKeyIndex(p); // iKey2Index、psKey2Data、iDiv2Level、psDiv2Data用于指示保护密钥
            p += 2;
            psKey2Data = NULL;
            if(iKey2Index == -1) {
                // LMK加密
                if(*p++ != 16) {
                    iRetCode = HSMERR_KEY_LEN;
                    break;
                }
                psKey2Data = p;
                p += 16;
            }
            iDiv2Level = *p++;
            psDiv2Data = p;
            p += iDiv2Level*8;
            if(iDiv2Level) {
                if(*p++ != 1) {
                    iRetCode = HSMERR_PARA; // 分散算法必须为1
                    break;
                }
            }
            if(*p++ != 0) {
                // 保护密钥必须不使用过程密钥
                iRetCode = HSMERR_PARA;
                break;
            }
        }
        iAppDataLen = ulStrToLong(p, 2);
        p += 2;
        psAppData = p;
        p += iAppDataLen;
        iPadFlag = *p++;
        iEncDecFlag = *p++;
        iMode = *p++; // ecb or cbc mode
        psIv = NULL;
        if(iMode == 1) {
            // CBC mode
            psIv = p;
            p += 8;
        }
        if(iRecvLen != (uchar*)p-psRecvBuf) {
            iRetCode = HSMERR_MSG_LENGTH;
            break;
        }

        if(iKeyIndex != -2) {
            // 加解密密钥被LMK保护或Mac密钥为索引密钥
            iRetCode = iKeyEncDec0(iKeyIndex, psKeyData, iDivLevel, psDivData, NULL, 0, NULL, iEncDecFlag, iPadFlag, psIv, iAppDataLen, psAppData, &iOutLen, psSendBuf+3);
        } else {
            // 加解密密钥被应用密钥保护
            iRetCode = iKeyEncDec0(iKey2Index, psKey2Data, iDiv2Level, psDiv2Data, psKeyData, iDivLevel, psDivData, iEncDecFlag, iPadFlag, psIv, iAppDataLen, psAppData, &iOutLen, psSendBuf+3);
        }
        if(iRetCode)
            break;
        vLongToStr(iOutLen, 2, psSendBuf+1);
        *piSendLen = 3+iOutLen;
        break;
    case 0xD247: // 通用加密数据密钥转换
//new        // in  : Ins[2]
        //       +解密密钥索引[2](0xFFFF:LMK加密输入, 0xEEEE:保护密钥加密输入)
        //       +解密密钥长度[1]+解密密钥[n]+解密密钥分散次数[1]+解密密钥分散数据[n]+解密密钥分散算法[1](加解密密钥分散次数不为0时存在)
        //       +解密密钥是否使用过程密钥[1]
        //       +解密密钥保护密钥索引[2](0xFFFF:LMK加密输入)+保护密钥长度[1]+保护密钥[n]+保护密钥分散次数[1]
        //       +保护密钥分散数据[n]+保护密钥分散算法[1]+保护密钥是否使用过程密钥[1]
        //       +加密密钥索引[2](0xFFFF:LMK加密输入, 0xEEEE:保护密钥加密输入)
        //       +加密密钥长度[1]+加密密钥[n]+加密密钥分散次数[1]+加密密钥分散数据[n]+加密密钥分散算法[1](加密密密钥分散次数不为0时存在)
        //       +加密密钥是否使用过程密钥[1]
        //       +加密密钥保护密钥索引[2](0xFFFF:LMK加密输入)+保护密钥长度[1]+保护密钥[n]+保护密钥分散次数[1]
        //       +保护密钥分散数据[n]+保护密钥分散算法[1]+保护密钥是否使用过程密钥[1]
        //       +解密模式[1]+解密数据IV[8](CBC模式存在)+加密模式[1]+加密数据Iv[8](CBC模式存在)
        //       +加密时数据填充模式[1]+数据长度[2]+数据[n]
        // out : 'A'+输出长度[2]+输出数据[n]
        p = psRecvBuf+2;
        // 开始解析解密密钥
        iKeyIndex = iGetKeyIndex(p);
        p += 2;
        psKeyData = NULL;
        if(iKeyIndex < 0) {
            // LMK加密或保护密钥加密
            if(*p++ != 16) {
                iRetCode = HSMERR_KEY_LEN;
                break;
            }
            psKeyData = p;
            p += 16;
        }
        iDivLevel = *p++;
        psDivData = p;
        p += iDivLevel*8;
        if(iDivLevel) {
            if(*p++ != 1) {
                iRetCode = HSMERR_PARA; // 分散算法必须为1
                break;
            }
        }
        if(*p++ != 0) {
            iRetCode = HSMERR_PARA; // 解密密钥必须不使用过程密钥
            break;
        }
        if(iKeyIndex == -1) {
            // LMK加密的解密密钥
            iKey2Index = -1;
        } else if(iKeyIndex == -2) {
            // 保护密钥加密
            iKey2Index = iGetKeyIndex(p); // iKey2Index、psKey2Data、iDiv2Level、psDiv2Data用于指示保护密钥
            p += 2;
            psKey2Data = NULL;
            if(iKey2Index == -1) {
                // LMK加密的保护密钥
                if(*p++ != 16) {
                    iRetCode = HSMERR_KEY_LEN;
                    break;
                }
                psKey2Data = p;
                p += 16;
                iKey2Index = 0; // 因为-1表示不使用保护密钥, 所以要置成非-1
            }
            iDiv2Level = *p++;
            psDiv2Data = p;
            p += iDiv2Level*8;
            if(iDiv2Level) {
                if(*p++ != 1) {
                    iRetCode = HSMERR_PARA; // 分散算法必须为1
                    break;
                }
            }
            if(*p++ != 0) {
                iRetCode = HSMERR_PARA; // 保护密钥必须不使用过程密钥
                break;
            }
        }
        // 开始解析加密密钥
        iKeyIndexX = iGetKeyIndex(p);
        p += 2;
        psKeyDataX = NULL;
        if(iKeyIndexX < 0) {
            // LMK加密或保护密钥加密
            if(*p++ != 16) {
                iRetCode = HSMERR_KEY_LEN;
                break;
            }
            psKeyDataX = p;
            p += 16;
        }
        iDivLevelX = *p++;
        psDivDataX = p;
        p += iDivLevelX*8;
        if(iDivLevelX) {
            if(*p++ != 1) {
                iRetCode = HSMERR_PARA; // 分散算法必须为1
                break;
            }
        }
        if(*p++ != 0) {
            iRetCode = HSMERR_PARA; // 加密密钥必须不使用过程密钥
            break;
        }
        if(iKeyIndexX == -1) {
            // LMK加密的加密密钥
            iKey2IndexX = -1;
        } else if(iKeyIndexX == -2) {
            // 保护密钥加密
            iKey2IndexX = iGetKeyIndex(p); // iKey2Index、psKey2Data、iDiv2Level、psDiv2Data用于指示保护密钥
            p += 2;
            psKey2DataX = NULL;
            if(iKey2IndexX == -1) {
                // LMK加密的保护密钥
                if(*p++ != 16) {
                    iRetCode = HSMERR_KEY_LEN;
                    break;
                }
                psKey2DataX = p;
                p += 16;
                iKey2IndexX = 0; // 因为-1表示不使用保护密钥, 所以要置成非-1
            }
            iDiv2LevelX = *p++;
            psDiv2DataX = p;
            p += iDiv2LevelX*8;
            if(iDiv2LevelX) {
                if(*p++ != 1) {
                    iRetCode = HSMERR_PARA; // 分散算法必须为1
                    break;
                }
            }
            if(*p++ != 0) {
                iRetCode = HSMERR_PARA; // 保护密钥必须不使用过程密钥
                break;
            }
        }
        iMode = *p++; // ecb or cbc mode
        psIv = NULL;
        if(iMode == 1) {
            // CBC mode
            psIv = p;
            p += 8;
        }
        iModeX = *p++; // ecb or cbc mode
        psIvX = NULL;
        if(iModeX == 1) {
            // CBC mode
            psIvX = p;
            p += 8;
        }
        if(*p++ != 3) {
            iRetCode = HSMERR_PARA; // 加密时数据填充模式只支持模式3, 3:不填充
            break;
        }
        // 开始解析要转加密的数据
        iAppDataLen = ulStrToLong(p, 2);
        p += 2;
        psAppData = p;
        p += iAppDataLen;
        if(iRecvLen != (uchar*)p-psRecvBuf) {
            iRetCode = HSMERR_MSG_LENGTH;
            break;
        }
        iRetCode = iKeyDecEncData0(iKeyIndex, psKeyData, iDivLevel, psDivData, iKey2Index, psKey2Data, iDiv2Level, psDiv2Data, psIv,
                                   iKeyIndexX, psKeyDataX, iDivLevelX, psDivDataX, iKey2IndexX, psKey2DataX, iDiv2LevelX, psDiv2DataX, psIvX,
                                   iAppDataLen, psAppData, psSendBuf+3);
        if(iRetCode)
            break;
        vLongToStr(iAppDataLen, 2, psSendBuf+1);
        *piSendLen = 3+iAppDataLen;
        break;
    default:
        iRetCode = HSMERR_INS;
        break;
    }
    if(iRetCode) {
        // 只有无保留字的指令出错才可以使用缺省应答
        // 对于带保留字的指令, 出错后要填好错误信息及长度, 然后置iRetCode为0
        psSendBuf[0] = 'E';
        psSendBuf[1] = iRetCode;
        *piSendLen = 2;
    }
    return(0);
}
