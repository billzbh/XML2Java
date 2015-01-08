#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "VposFace.h"
#include "hsmproc.h"
#include "hsmsimu.h"

//#define WESTONE_SUPPORT		// ����ú��ʾ֧����ʿͨ���ܻ�
#ifdef WESTONE_SUPPORT
#include "sockcomm.h"
#endif

static int  sg_iMode = 0;           // ģʽ, 0:ģ������Կģʽ 1:���������ļ�ģʽ 2:��ʿͨ���ܻ�ģʽ
static char sg_szHsmIp[20] = {0};   // ���ܻ�IP��ַ
static char sg_szHsmPort[10] = {0}; // ���ܻ��˿ں�

#define MAX_RSA_KEY_LEN     248
static int iInsProc(uchar *psRecvBuf, int iRecvLen, uchar *psSendBuf, int *piSendLen);

// ���ò���
// in  : iMode    : ģʽ 0:ģ������Կģʽ 1:���������ļ�ģʽ 2:��ʿͨ���ܻ�ģʽ
//       pszIp    : ���ܻ�IP��ַ, ģʽ2��ģʽ3����
//       iPortNo  : ���ܻ��˿ں�, ģʽ2��ģʽ3����
// ret : 0        : �ɹ�
//       -1       : ʧ��, ���ܻ�δ����
//       -2       : ʧ��, ���������ļ���
//       -3       : iMode��
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
// ���ܻ�����
// in  : iSendLen  : �������ݳ���
//       pSendBuf  : ��������
//       piRecvLen : �������ݻ���������
// out : piRecvLen : �������ݳ���
//       pRecvBuf  : ��������
// ret : 0         : �ɹ�
//       -1        : ͨѶʧ��
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
            iTimeout = 120; // ���ָ���漰����RSA��Կ, �Ӵ�ʱʱ��(�������, ����ܻ���Ȼ���ܲ����������Ƚϴ����Կ)
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

// ��ȡ��Կ����
// in  : psKeyIndex : 2�ֽڵ���Կ����
// ret : -1         : ��Կ����Ϊ"\xFF\xFF"
// ret : -2         : ��Կ����Ϊ"\xEE\xEE"
//       >=0        : ��Կ����
static int iGetKeyIndex(char *psKeyIndex)
{
    if(memcmp(psKeyIndex, "\xFF\xFF", 2) == 0)
        return(-1);
    if(memcmp(psKeyIndex, "\xEE\xEE", 2) == 0)
        return(-2);
    return(ulStrToLong(psKeyIndex, 2));
}

// ��ģ�������DER��ʽ
// in  : iKeyLen : �ֽڵ�λ��ģ������
//       lE      : ����ָ��
//       pN      : ģ��
// out : pDerN   : DER��ʽ�Ĺ�Կ
// ret : 0       : ����
//       ����    : DER��ʽ��Կ����
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
        sBuf[iLen++] = iKeyLen+1; // ģ��ǰ�油һ��0, �Ա���ģ��Ϊ����
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
    // ��sBuf(����=iLen)�����T30
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
// ��DER��ʽ��Կ�⿪��ģ��
// in  : iDerNLen : DER��ʽ��Կ����
//       pDerN    : DER��ʽ��Կ
// out : plE      : ����ָ��
//       pN       : ģ��
// ret : 0        : ����
//       ����     : �ֽڵ�λ��ģ������
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
    // psDerN����ָ��ģ��TLV�����빫��ָ��TLV����, iLenΪģ��TLV�����빫��ָ��TLV���󳤶�֮��
    // �Ƚ������ָ��
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
    // iLenΪģ��TLV���󳤶�
    // �ٽ��ģ��
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

// ������ָ��
// in  : psRecvBuf : ���յ�ָ��
//       iRecvLen  : ���յ�ָ���
// out : psSendBuf : ���͵�Ӧ��
//       piSendLen : ���͵�Ӧ�𳤶�
// ret : 0         : OK
static int iInsProc(uchar *psRecvBuf, int iRecvLen, uchar *psSendBuf, int *piSendLen)
{
    unsigned char sBuf[300], sBuf2[300];
    unsigned char sPrivateKey[1600];
    unsigned int uiIns; // ָ��
    int  iRetCode;
    int  iInLen, iOutLen, iKeyLen, iDataLen;

    int  iKeyIndex, iKey2Index;
    int  iDivLevel, iDiv2Level;
    unsigned char *psDivData, *psDiv2Data;
    unsigned char *psKeyData, *psKey2Data;
    // ����4��'X'��β�ı�������ͨ��ת��������ʱ��ʾ������Կ(�����������ڱ�ʾ������Կ)
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
        uiIns = 0; // �汾�Ų�ѯָ��, ֻ����һ��ָ���ǵ��ֽ�ָ��
    else {
        if(iRecvLen < 2) {
            psSendBuf[0] = 'E';
            psSendBuf[1] = HSMERR_MSG_LENGTH;
            *piSendLen = 2;
            return(0);
        }
        uiIns = ulStrToLong(psRecvBuf, 2);
    }

    iRetCode = 0;       // ִ�гɹ���û�з�����Ϣ��ȱʡֵ
    *piSendLen = 1;     // ִ�гɹ���û�з�����Ϣ��ȱʡֵ
    psSendBuf[0] = 'A'; // ִ�гɹ���ȱʡ����ͷֵ
    switch(uiIns) {
    case 0x00: // �汾�Ų�ѯ
        // in  : Ins[1]
        // out : 'A'+�汾��[4]+Э��[4]+ϵͳ����޶�����[8]
        if(iRecvLen != 1) {
            iRetCode = HSMERR_MSG_LENGTH;
            break;
        }
        memcpy(psSendBuf+1, "0100" "HXCo" "20110708", 16);
        *piSendLen = 1+16;
        break;
    case 0xC040: // ��ָ��������RSA˽Կ����
    case 0xC042: // ��ָ��������RSA˽Կ����
        // ����ܻ���֧����䷽ʽ, ���Լӽ��ܲ��������ͬ
        // in  : Ins[2]+������[8]+���ģʽ[1]+Index[2]+Password[8]+InLen[2]+Data[n]
        // out : 'A'+������[8]+OutLen[2]+OutData[n]
        // ���ڱ�ָ�����������, ����ʱ���ر��Ĳ�����ȷ���, ����Ԥ����ó�������Ϣ������, �������, iRetCode��0
        psSendBuf[0] = 'E';
        memcpy(psSendBuf+1, psRecvBuf+2, 8); // ������
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
        /* ����ܻ�����֤����
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
        
        // ִ�гɹ�
        psSendBuf[0] = 'A';
        vLongToStr(iInLen, 2, psSendBuf+9);
        *piSendLen = 11+iInLen;
        break;
    case 0xD20C: // �ô����RSA˽Կ����
        // in  : Ins[2]+�ӽ��ܱ�־[1]+���ģʽ[1]+KeyLen[2]+InLen[2]+Key[n]+Data[m]
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
    case 0xD22A: // ɾ����Կ
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
    case 0xD230: // ��LMK����ָ��������DES��Կ
        // in  : Ins[2]+KeyType[1]+KeyIndex[2]
        // out : 'A'+KeyLen[2]+Key[n]+Cks[8]
        if(iRecvLen != 5) {
            iRetCode = HSMERR_MSG_LENGTH;
            break;
        }
        if(psRecvBuf[2] != 4) {
            // 4:��LMK����
            iRetCode = HSMERR_TYPE;
            break;
        }
        iKeyIndex = iGetKeyIndex(psRecvBuf+3);
        iRetCode = iKeyBackup0(1/*1:DES��Կ*/, iKeyIndex, &iOutLen, psSendBuf+2);
        if(iRetCode)
            break;
        psSendBuf[1] = 16;
        *piSendLen = 2 + 16 + 8;
        break;
    case 0xD231: // ��LMK����DES��Կ
        // in  : Ins[2]+KeyType[1]+KeyIndex[2]+KeyLen[1]+Key[16]+Cks[8]
        // out : 'A'
        if(iRecvLen != 30) {
            iRetCode = HSMERR_MSG_LENGTH;
            break;
        }
        if(psRecvBuf[2] != 4) {
            // 4:��LMK����
            iRetCode = HSMERR_TYPE;
            break;
        }
        if(psRecvBuf[5] != 16) {
            iRetCode = HSMERR_KEY_LEN;
            break;
        }
        iKeyIndex = iGetKeyIndex(psRecvBuf+3);
        iRetCode = iKeyRestore0(1/*1:DES��Կ*/, iKeyIndex, 24, psRecvBuf+6);
        break;
    case 0xD232: // ����RSA��Կ��
        // in  : Ins[2]+ModulusLen[2]+E[4]+KeyIndex[2]+Password[8](KeyIndex!=0xFFFFʱ����)+Mode[1]
        //       +��Կ��ʽ[1](Mode==0��2ʱ����)+˽Կ��ʽ[1](Mode==1��2ʱ����)
        // out : 'A'+��Կ���ݳ���[2]+��Կ����[n] // Mode==0
        // out : 'A'+˽Կ���ݳ���[2]+˽Կ����[m] // Mode==1
        // out : 'A'+��Կ���ݳ���[2]+��Կ����[n]+˽Կ���ݳ���[2]+˽Կ����[m] // Mode==2
        // out : 'A' // Mode==3
        p = psRecvBuf+2;
        iKeyLen = ulStrToLong(p, 2); // ģ����λ����
        iKeyLen /= 8; // ģ�����ֽڳ���
        if(iKeyLen%2 || iKeyLen>MAX_RSA_KEY_LEN) {
            iRetCode = HSMERR_KEY_LEN;
            break;
        }
        p += 2;
        lE = ulStrToLong(p, 4); // ģ����λ����
        p += 4;
        if(lE!=3 && lE!=65537L) {
            iRetCode = HSMERR_RSA_E;
            break;
        }
        iKeyIndex = iGetKeyIndex(p);
        p += 2;
        if(iKeyIndex != -1)
            p += 8; // ����ܻ�������password
        iMode = *p++;
        if((iMode!=2&&iKeyIndex==-1) || iMode<0 || iMode>3) {
            iRetCode = HSMERR_MODE;
            break;
        }
        if(iMode==0 || iMode==2) {
            if(*p++ != 0x01) {
                iRetCode = HSMERR_MODE; // ��Կ������DER��ʽ
                break;
            }
        }
        if(iMode==1 || iMode==2) {
            if(*p++ != 0x01) {
                iRetCode = HSMERR_MODE; // ˽Կ������DER��ʽ
                break;
            }
        }
        if(iRecvLen != (uchar*)p-psRecvBuf) {
            iRetCode = HSMERR_MSG_LENGTH;
            break;
        }

        iRetCode = iKeyGenRsaKey0(iKeyIndex, iKeyLen, lE, sBuf/*Nֵ*/, sPrivateKey/*˽Կ����*/, &iOutLen/*˽Կ���ݳ���*/);
        iL1 = iPackPublicKey(iKeyLen, lE, sBuf, sBuf2); // ��ģ�������DER��ʽ
        if(iMode == 0) {
            // Ҫ�����Կ
            // ����DER��ʽ��Կ����
            vLongToStr(iL1, 2, psSendBuf+1);
            memcpy(psSendBuf+3, sBuf2, iL1);
            *piSendLen = 3+iL1;
        } else if(iMode == 1) {
            // Ҫ���˽Կ
            // ����DER��ʽ��Կ����
            vLongToStr(iOutLen, 2, psSendBuf+1);
            memcpy(psSendBuf+3, sPrivateKey, iOutLen);
            *piSendLen = 3+iOutLen;
        } else if(iMode == 2) {
            // Ҫ�����˽Կ
            // ����DER��ʽ��Կ����
            vLongToStr(iL1, 2, psSendBuf+1);
            memcpy(psSendBuf+3, sBuf2, iL1);
            vLongToStr(iOutLen, 2, psSendBuf+3+iL1);
            memcpy(psSendBuf+3+iL1+2, sPrivateKey, iOutLen);
            *piSendLen = 3+iL1+2+iOutLen;
        }
        break;
    case 0xD233: // ��LMK����RSA��Կ
        // in  : Ins[2]+KeyIndex[2]+Mode[1]+��Կ��ʽ[1](Mode==0��2ʱ����)+˽Կ��ʽ[1](Mode==1��2ʱ����)+Password[8](Mode==1��2ʱ����)
        // out : (Mode=0)'A'+��Կ���ݳ���[2]+��Կ����[n]
        // out : (Mode=1)'A'+˽Կ���ݳ���[2]+˽Կ����[m]
        // out : (Mode=2)'A'+��Կ���ݳ���[2]+��Կ����[n]+˽Կ���ݳ���[2]+˽Կ����[m]
        p = psRecvBuf+2;
        iKeyIndex = iGetKeyIndex(p);
        p += 2;
        iMode = *p++;
        if(iMode==0 || iMode==2) {
            // Ҫ������Կ
            if(*p++ != 1) {
                iRetCode = HSMERR_MODE; // ��Կ˽Կ������DER��ʽ
                break;
            }
        }
        if(iMode==1 || iMode==2) {
            // Ҫ����˽Կ
            if(*p++ != 1) {
                iRetCode = HSMERR_MODE; // ��Կ˽Կ������DER��ʽ
                break;
            }
            p += 8; // ����ܻ�������password
        }
        if(iRecvLen != (uchar*)p-psRecvBuf) {
            iRetCode = HSMERR_MSG_LENGTH;
            break;
        }

        switch(iMode) {
        case 0: // ������Կ
            iRetCode = iKeyRsaGetInfo0(iKeyIndex, NULL, &iL1, &lE, sBuf); // iL1 = DER��ʽ�Ĺ�Կ����
            if(iRetCode)
                break;
            iL2 = iPackPublicKey(iL1, lE, sBuf, psSendBuf+3); // ��ģ�������DER��ʽ
            vLongToStr(iL2, 2, psSendBuf+1);
            *piSendLen = 3 + iL2;
            break;
        case 1: // ����˽Կ
            iRetCode = iKeyBackup0(2/*2:RSA*/, iKeyIndex, &iL1, psSendBuf+3);
            if(iRetCode)
                break;
            vLongToStr(iL1, 2, psSendBuf+1);
            *piSendLen = 3 + iL1;
            break;
        case 2: // ������˽Կ
            iRetCode = iKeyRsaGetInfo0(iKeyIndex, NULL, &iL1, &lE, sBuf); // iL1 = DER��ʽ�Ĺ�Կ����
            if(iRetCode)
                break;
            iL2 = iPackPublicKey(iL1, lE, sBuf, psSendBuf+3); // ��ģ�������DER��ʽ
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
    case 0xD234: // ��LMK����RSA��Կ
        // in  : Ins[2]+KeyIndex[2]+Password[8]+RSA��Կ����[2]+LMK���ܺ��RSA��Կ[n]+˽Կ��ʽ[1]
        // out : 'A'
        iKeyIndex = iGetKeyIndex(psRecvBuf+2);
        iKeyLen = ulStrToLong(psRecvBuf+12, 2);
        if(psRecvBuf[14+iKeyLen] != 1) {
            iRetCode = HSMERR_RSA_FORMAT; // ��Կ˽Կ������DER��ʽ
            break;
        }
        if(iRecvLen != 15+iKeyLen) {
            iRetCode = HSMERR_MSG_LENGTH;
            break;
        }
        iRetCode = iKeyRestore0(2/*2:RSA*/, iKeyIndex, iKeyLen, psRecvBuf+14);
        break;
    case 0xD235: // RSA��Կ�ӽ���
        // in  : Ins[2]+KeyIndex[2]+KeyLen[2](KeyIndex!=0xFFFFʱ����)+Key[n](KeyIndex!=0xFFFFʱ����)
        //       +��Կ��ʽ[1](KeyIndex!=0xFFFFʱ����)+�ӽ��ܱ�־[1]+���ģʽ[1]+���ݳ���[2]+����[m]
        // out : 'A'+���ݳ���[2]+����[m]
        p = psRecvBuf+2;
        iKeyIndex = iGetKeyIndex(p);
        p += 2;
        if(iKeyIndex == -1) {
            // �ô������Կ�ӽ���
            iKeyLen = ulStrToLong(p, 2); // DER��ʽ��Կ����
            p += 2;
            psKeyData = p; // psKeyDataΪ�����Der��ʽ��Կ
            p += iKeyLen;
            if(*p++ != 1) {
                iRetCode = HSMERR_RSA_FORMAT; // ��Կ˽Կ������DER��ʽ
                break;
            }
        }
        p ++; // ���Լӽ��ܱ�־
        if(*p++ != 0) {
            iRetCode = HSMERR_PAD_FORMAT; // ����û�����
            break;
        }
        iDataLen = ulStrToLong(p, 2);
        p += 2;
        p2 = p; // p2ָ������
        p += iDataLen;
        if(iRecvLen != (uchar*)p-psRecvBuf) {
            iRetCode = HSMERR_MSG_LENGTH;
            break;
        }
        if(iKeyIndex == -1) {
            iL1 = iUnpackPublicKey(iKeyLen, psKeyData, &lE, sBuf); // iL1=ģ�� sBuf=ģ��
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
            // ������ָ������Կ�ӽ���
            iRetCode = iKeyRsaPublicBlock0(iKeyIndex, iDataLen, p2, psSendBuf+3);
            if(iRetCode)
                break;
        }
        vLongToStr(iDataLen, 2, psSendBuf+1);
        *piSendLen = 3 + iDataLen;
        break;
    case 0xD237: // ����IC���ⲿ��֤����
        // in  : Ins[2]+KmcIndex[2]+KmcLen[1](KmcIndex==0xFFFFʱ����)+Kmc[n](KmcIndex==0xFFFFʱ����)+KmcDivLevel[1]+KmcDivData[n]+TermRand[8]+CardData[28]
        // out : 'A'+�ⲿ��֤����[8]+�ⲿ��ָ֤��Mac[8]
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
    case 0xD238: // ����IC��Des��Կ����(��ʱ����)
        // in  : Ins[2]+KmcIndex[2]+KmcLen[1](KmcIndex==0xFFFFʱ����)+Kmc[n](KmcIndex==0xFFFFʱ����)+KmcDivLevel[1]+KmcDivData[n]
        //       +TermRand[8]+CardData[28]+AppKeyIndex[2]+AppKeyLen[1](AppKeyIndex==0xFFFFʱ����)+AppKey[n](AppKeyIndex==0xFFFFʱ����)
        //       +AppDivLevel[1]+AppDivData[n]
        // out : 'A'+IC����Կ����[16]+IC����ԿУ��ֵ[8]
        p = psRecvBuf+2;
        iKeyIndex = iGetKeyIndex(p); // iKeyIndex��psKeyData��iDivLevel��psDivData����ָʾKMC
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
        iKey2Index = iGetKeyIndex(p); // iKey2Index��psKey2Data��iDiv2Level��psDiv2Data����ָʾAppKey
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
        // ����AppKey����
        iRetCode = iKeyCalDesKey0(iKeyIndex, psKeyData, iDivLevel, psDivData, psTermRand, psCardData, iKey2Index, psKey2Data, iDiv2Level, psDiv2Data, psSendBuf+1);
        if(iRetCode)
            break;
        *piSendLen = 25;
        break;
    case 0xD239: // ����IC��������������(��ʱ����)
        // in  : Ins[2]+KmcIndex[2]+KmcLen[1](KmcIndex==0xFFFFʱ����)+Kmc[n](KmcIndex==0xFFFFʱ����)+KmcDivLevel[1]+KmcDivData[n]
        //       +TermRand[8]+CardData[28]+PinBlock[8]
        // out : 'A'+IC��������������[8]
        p = psRecvBuf+2;
        iKeyIndex = iGetKeyIndex(p); // iKeyIndex��psKeyData��iDivLevel��psDivData����ָʾKMC
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
        p += 28; // ��ǰpָ��PinBlock
        if(iRecvLen != (uchar*)p-psRecvBuf+8) {
            iRetCode = HSMERR_MSG_LENGTH;
            break;
        }
        iRetCode = iKeyCalPin0(iKeyIndex, psKeyData, iDivLevel, psDivData, psTermRand, psCardData, p, psSendBuf+1);
        if(iRetCode)
            break;
        *piSendLen = 9;
        break;
    case 0xD23A: // ����IC��RSA��Կ����(��ʱ����)
        // in  : Ins[2]+ģ��(��λ)[2]+����ָ��[4]+��Կ��ʽ[1]+˽Կ��ʽ[1]+
        //       KmcIndex[2]+KmcLen[1](KmcIndex==0xFFFFʱ����)+Kmc[n](KmcIndex==0xFFFFʱ����)+KmcDivLevel[1]+KmcDivData[n]
        //       +TermRand[8]+CardData[28]
        // out : 'A'+��Կ���ݳ���[2]+��Կ����[n]+
        //           D����[2]+D[n]+
        //           P����[2]+P[n]+
        //           Q����[2]+Q[n]+
        //           dP����[2]+dP[n]+
        //           dQ����[2]+dQ[n]+
        //           qInv����[2]+qInv[n]
        p = psRecvBuf+2;
        iKeyLen = ulStrToLong(p, 2); // ģ��(��λ)
        iKeyLen /= 8; // ģ��(���ֽ�)
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
        iKeyIndex = iGetKeyIndex(p); // iKeyIndex��psKeyData��iDivLevel��psDivData����ָʾKMC
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
        iL1 = (iKeyLen+8)/8*8;   // D����
        iL2 = (iKeyLen/2+8)/8*8; // p��q�ȳ���
        p = psSendBuf+1;  // p��������Ӧ����λ��
        p2 = sPrivateKey; // p2�������ٱ��ؼ���������
        iL3 = iPackPublicKey(iKeyLen, lE, p2, p+2); // Der��ʽ��Կ����
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
    case 0xD23B: // ����IC��RSA��Կ����(����׼��)
        // in  : Ins[2]+ģ��(��λ)[2]+����ָ��[4]+��Կ��ʽ[1]+˽Կ��ʽ[1]+
        //       TkIndex[2]+TkLen[1](TkIndex==0xFFFFʱ����)+Tk[n](TkIndex==0xFFFFʱ����)+TkDivLevel[1]+TkDivData[n]
        // out : 'A'+��Կ���ݳ���[2]+��Կ����[n]+
        //           D����[2]+D[n]+
        //           P����[2]+P[n]+
        //           Q����[2]+Q[n]+
        //           dP����[2]+dP[n]+
        //           dQ����[2]+dQ[n]+
        //           qInv����[2]+qInv[n]
        p = psRecvBuf+2;
        iKeyLen = ulStrToLong(p, 2); // ģ��(��λ)
        iKeyLen /= 8; // ģ��(���ֽ�)
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
        iKeyIndex = iGetKeyIndex(p); // iKeyIndex��psKeyData��iDivLevel��psDivData����ָʾTk
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
        iL1 = (iKeyLen+8)/8*8;   // D����
        iL2 = (iKeyLen/2+8)/8*8; // p��q�ȳ���
        p = psSendBuf+1;  // p��������Ӧ����λ��
        p2 = sPrivateKey; // p2�������ٱ��ؼ���������
        iL3 = iPackPublicKey(iKeyLen, lE, p2, p+2); // Der��ʽ��Կ����
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
    case 0xD23C: // ��֤AC, ������ARPC
        // in  : Ins[2]+Mode[1]+KeyIndex[2]+KeyLen[1](KeyIndex==0xFFFFʱ����)+Key[n](KeyIndex==0xFFFFʱ����)
        //       KeyDivLevel[1]+KeyDivData[n]+ATC[2]+AC[8]+AppDataLen[2](Mode==0��1ʱ����)+AppData[m](Mode==0��1ʱ����)+Ӧ����[2](Mode==1��2ʱ����)
        // out(Mode==0)    : 'A'
        // out(Mode==1��2) : 'A'+ARPC[8]
        p = psRecvBuf+2;
        iMode = *p++;
        if(iMode!=0 && iMode!=1 && iMode!=2) {
            iRetCode = HSMERR_PARA;
            break;
        }
        iKeyIndex = iGetKeyIndex(p); // iKeyIndex��psKeyData��iDivLevel��psDivData����ָʾAppKey
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
    case 0xD23D: // ����IC���޸�KMC����
        // in  : Ins[2]+KmcIndex[2]+KmcLen[1](KmcIndex==0xFFFFʱ����)+Kmc[n](KmcIndex==0xFFFFʱ����)+KmcDivLevel[1]+KmcDivData[n]
        //       +TermRand[8]+CardData[28]
        //       +NewKmcIndex[2]+NewKmcLen[1](NewKmcIndex==0xFFFFʱ����)+NewKmc[n](NewKmcIndex==0xFFFFʱ����)+NewKmcDivLevel[1]+NewKmcDivData[n]
        // out : 'A'+Auth����Կ����[16]+Auth����ԿУ��ֵ[8]+Mac����Կ����[16]+Mac����ԿУ��ֵ[8]+Enc����Կ����[16]+Enc����ԿУ��ֵ[8]
        p = psRecvBuf+2;
        iKeyIndex = iGetKeyIndex(p); // iKeyIndex��psKeyData��iDivLevel��psDivData����ָʾKMC
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
        iKey2Index = iGetKeyIndex(p); // iKey2Index��psKey2Data��iDiv2Level��psDiv2Data����ָʾNewKMC
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
    case 0xD23E: // ת���ܷ�������(������׼�����ݷ���)
        // in  : Ins[2]+KeyIndex[2]+KeyLen[1](KeyIndex==0xFFFFʱ����)+Key[n](KeyIndex==0xFFFFʱ����)+KeyDivLevel[1]+KeyDivData[n]
        //       +KmcIndex[2]+KmcLen[1](KmcIndex==0xFFFFʱ����)+Kmc[n](KmcIndex==0xFFFFʱ����)+KmcDivLevel[1]+KmcDivData[n]
        //       +TermRand[8]+CardData[28]+����ģʽ[1]+IV[8](����ģʽ==1ʱ����)+DataLen[2]+Data[m]
        // out : 'A'+OutLen[2]+OutData[n]
        p = psRecvBuf+2;
        iKeyIndex = iGetKeyIndex(p); // iKeyIndex��psKeyData��iDivLevel��psDivData����ָʾԭ������Կ
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
        iKey2Index = iGetKeyIndex(p); // iKey2Index��psKey2Data��iDiv2Level��psDiv2Data����ָʾKMC
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
    case 0xD23F: // ת����PinBlock(������׼�����ݷ���)
        // in  : Ins[2]+KeyIndex[2]+KeyLen[1](KeyIndex==0xFFFFʱ����)+Key[n](KeyIndex==0xFFFFʱ����)+KeyDivLevel[1]+KeyDivData[n]
        //       +KmcIndex[2]+KmcLen[1](KmcIndex==0xFFFFʱ����)+Kmc[n](KmcIndex==0xFFFFʱ����)+KmcDivLevel[1]+KmcDivData[n]
        //       +TermRand[8]+CardData[28]+ԭPinBlock��ʽ[1]+��PinBlock��ʽ[1]+PinBlock[8]
        // out : 'A'+OutData[8]
        p = psRecvBuf+2;
        iKeyIndex = iGetKeyIndex(p); // iKeyIndex��psKeyData��iDivLevel��psDivData����ָʾԭ������Կ
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
        iKey2Index = iGetKeyIndex(p); // iKey2Index��psKey2Data��iDiv2Level��psDiv2Data����ָʾKMC
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
        p++; // ������ԭ���ܸ�ʽ
        if(*p++ != 0x02) {
            // ������ܳɸ�ʽ2
            iRetCode = HSMERR_PARA;
            break;
        }
        psAppData = p; // psAppData��PinBlock
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
    case 0xD240: // �����нű�����
        // in  : Ins[2]+Mode[1]+IcType[1]+KeyIndex[2]+KeyLen[1](KeyIndex==0xFFFFʱ����)+Key[n](KeyIndex==0xFFFFʱ����)+KeyDivLevel[1]+KeyDivData[n]
        //       +ATC[2]+����ģʽ[1]+IV[8](����ģʽ==1ʱ����)+DataLen[2]+Data[m]
        // out : 'A'+OutLen[2]+OutData[n]
        p = psRecvBuf+2;
        if(*p++ != 1) {
            // ֻ������м���
            iRetCode = HSMERR_PARA;
            break;
        }
        if(*p++ != 1) {
            // ֻ����PBOC2.0���Ϳ�Ƭ
            iRetCode = HSMERR_PARA;
            break;
        }
        iKeyIndex = iGetKeyIndex(p); // iKeyIndex��psKeyData��iDivLevel��psDivData����ָʾAppKey
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
            // ֻ֧��ECBģʽ
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
        iRetCode = iKeyScriptEncData0(iKeyIndex, psKeyData, iDivLevel, psDivData, psATC, 0/*���������*/, iAppDataLen, psAppData,
                                     &iOutLen, psSendBuf+3);
        if(iRetCode)
            break;
        vLongToStr(iOutLen, 2, psSendBuf+1);
        *piSendLen = 3+iOutLen;
        break;
    case 0xD241: // �����нű�Pin����
        // in  : Ins[2]+IcType[1]+KeyIndex[2]+KeyLen[1](KeyIndex==0xFFFFʱ����)+Key[n](KeyIndex==0xFFFFʱ����)+KeyDivLevel[1]+KeyDivData[n]
        //       +ATC[2]+PinBlock[8]
        // out : 'A'+OutLen[2]+OutData[n]
        p = psRecvBuf+2;
        if(*p++ != 1) {
            // ֻ����PBOC2.0���Ϳ�Ƭ
            iRetCode = HSMERR_PARA;
            break;
        }
        iKeyIndex = iGetKeyIndex(p); // iKeyIndex��psKeyData��iDivLevel��psDivData����ָʾAppKey
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
        psAppData = p; // psAppData����PinBlock
        p += 8;
        if(iRecvLen != (uchar*)p-psRecvBuf) {
            iRetCode = HSMERR_MSG_LENGTH;
            break;
        }
        iRetCode = iKeyScriptEncData0(iKeyIndex, psKeyData, iDivLevel, psDivData, psATC, 1/*�������*/, 8, psAppData,
                                     &iOutLen, psSendBuf+3);
        if(iRetCode)
            break;
        vLongToStr(iOutLen, 2, psSendBuf+1);
        *piSendLen = 3+iOutLen;
        break;
    case 0xD242: // �����нű�Mac����
        // in  : Ins[2]+IcType[1]+KeyIndex[2]+KeyLen[1](KeyIndex==0xFFFFʱ����)+Key[n](KeyIndex==0xFFFFʱ����)+KeyDivLevel[1]+KeyDivData[n]
        //       +ATC[2]+AppDataLen[2]+AppData[n]
        // out : 'A'+Mac[8]
        p = psRecvBuf+2;
        if(*p++ != 1) {
            // ֻ����PBOC2.0���Ϳ�Ƭ
            iRetCode = HSMERR_PARA;
            break;
        }
        iKeyIndex = iGetKeyIndex(p); // iKeyIndex��psKeyData��iDivLevel��psDivData����ָʾAppKey
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
    case 0xD243: // ����Mac
        // in  : Ins[2]+Mac��Կ����[2](0xFFFF:LMK��������, 0xEEEE:������Կ��������)
        //       +Mac��Կ����[1]+Mac��Կ[n]+Mac��Կ��ɢ����[1]+Mac��Կ��ɢ����[n]+Mac��Կ��ɢ�㷨[1](Mac��Կ��ɢ������Ϊ0ʱ����)
        //       +Mac��Կʹ�ù�����Կ��־[1]+������Կ����[2](0xFFFF:LMK��������)+������Կ����[1]+������Կ[n]+������Կ��ɢ����[1]
        //       +������Կ��ɢ����[n]+������Կ��ɢ�㷨[1]+������Կʹ�ù�����Կ��־[1]+Mac�㷨[1]+IV[8]+���ݿ��־[1]+Mac���ݳ���[2]+Mac����[n]
        // out : 'A'+Mac[8]
        p = psRecvBuf+2;
        iKeyIndex = iGetKeyIndex(p); // iKeyIndex��psKeyData��iDivLevel��psDivData����ָʾMac��Կ
        p += 2;
        psKeyData = NULL;
        if(iKeyIndex < 0) {
            // LMK���ܻ򱣻���Կ����
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
                iRetCode = HSMERR_PARA; // ��ɢ�㷨����Ϊ1
                break;
            }
        }
        if(*p++ != 0) {
            // Mac��Կ���벻ʹ�ù�����Կ
            iRetCode = HSMERR_PARA;
            break;
        }
        if(iKeyIndex == -2) {
            // ������Կ����
            iKey2Index = iGetKeyIndex(p); // iKey2Index��psKey2Data��iDiv2Level��psDiv2Data����ָʾ������Կ
            p += 2;
            psKey2Data = NULL;
            if(iKey2Index == -1) {
                // LMK����
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
                    iRetCode = HSMERR_PARA; // ��ɢ�㷨����Ϊ1
                    break;
                }
            }
            if(*p++ != 0) {
                // ������Կ���벻ʹ�ù�����Կ
                iRetCode = HSMERR_PARA;
                break;
            }
        }
        
        iMode = *p++;   // iMode=Mac�㷨 1:ANSI99(full) 2:ANSI919(retail) 3:XOR 4:PBOC3DES
                        // �㷨1��2��3�����ݳ��Ȳ���8�ı���ʱ, ��0x00��8�ı���
                        // �㷨4�Ȳ�0x80, ֮��������Ȳ���8�ı���ʱ, ��0x00��8�ı���
        if(iMode<1 || iMode>4) {
            iRetCode = HSMERR_MODE;
            break;
        }
        psIv = p;
        p += 8;
        iBlockFlag = *p++; // ���ݿ��־, 1:��һ����м�� 2:Ψһ������һ��
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
            // Mac��Կ��LMK������Mac��ԿΪ������Կ
            iRetCode = iKeyDpCalMacFull0(iKeyIndex, psKeyData, iDivLevel, psDivData, NULL, 0, NULL, iMode, psIv, iAppDataLen, psAppData, psSendBuf+1, iBlockFlag);
        } else {
            iRetCode = iKeyDpCalMacFull0(iKey2Index, psKey2Data, iDiv2Level, psDiv2Data, psKeyData, iDivLevel, psDivData, iMode, psIv, iAppDataLen, psAppData, psSendBuf+1, iBlockFlag);
        }
        if(iRetCode)
            break;
        *piSendLen = 9;
        break;
    case 0xD244: // ����DES��Կ
        // in  : Ins[2]+����λ��[2]+KeyLen[1]+Mode[1]+������Կ����[1]+������Կ����(������Կ����==0xFFFFʱ����)+������Կ[n](������Կ����==0xFFFFʱ����)
        //       +KeyDivLevel[1]+KeyDivData[n]
        // out : 'A'+��Կ����[1]+Mac������Կ���ܺ����Կ[n]+Cks[8]
        p = psRecvBuf+2;
        iKeyIndex = iGetKeyIndex(p); // iKeyIndex����ָʾ����λ��
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
            // ��Ҫ������Կ
            iKey2Index = iGetKeyIndex(p); // iKey2Index��psKey2Data��iDiv2Level��psDiv2Data����ָʾ������Կ����
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
        } // ��Ҫ������Կ
        if(iRecvLen != (uchar*)p-psRecvBuf) {
            iRetCode = HSMERR_MSG_LENGTH;
            break;
        }
        if(iMode == 0) {
            // ������
            iRetCode = iKeyGenDesKey0(iKeyIndex, NULL);
        } else if(iMode == 1) {
            // LMK����
            iRetCode = iKeyGenDesKey0(iKeyIndex, psSendBuf+1);
            if(iRetCode)
                break;
            *piSendLen = 1+16+8;
        } else if(iMode == 2) {
            // ������Կ����
            iRetCode = iKeyDpGenDesKey0(iKey2Index, psKey2Data, iDiv2Level, psDiv2Data, psSendBuf+1);
            if(iRetCode)
                break;
            *piSendLen = 1+16+8;
        } else {
            // ��LMK�ͱ�����Կ����
            iRetCode = HSMERR_PARA; // �ݲ�֧��
            break;
        }
        break;
    case 0xD245: // ������ɢ��Կ
        // in  : Ins[2]+TkIndex[2]+TkLen[1](TkIndex==0xFFFFʱ����)+Tk[n](TkIndex==0xFFFFʱ����)+TkDivLevel[1]+TkDivData[n]
        //       +Tk��ɢ�㷨[1](TkDivLevel��Ϊ0ʱ����)+AppKeyIndex[2]+AppKeyLen[1](AppKeyIndex==0xFFFFʱ����)+AppKey[n](AppKeyIndex==0xFFFFʱ����)
        //       +AppKeyDivLevel[1]+AppKeyDivData[n]+AppKey��ɢ�㷨[1](AppKeyDivLevel��Ϊ0ʱ����)
        // out : 'A'+��Կ����[1]+��Կ[n]+Cks[8]
        p = psRecvBuf+2;
        iKeyIndex = iGetKeyIndex(p); // iKeyIndex��psKeyData��iDivLevel��psDivData����ָʾTk
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
        iKey2Index = iGetKeyIndex(p); // iKey2Index��psKey2Data��iDiv2Level��psDiv2Data����ָʾӦ����Կ
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
    case 0xD246: // �ӽ�������
        // in  : Ins[2]+�ӽ�����Կ����[2](0xFFFF:LMK��������, 0xEEEE:������Կ��������)
        //       +�ӽ�����Կ����[1]+�ӽ�����Կ[n]+�ӽ�����Կ��ɢ����[1]+�ӽ�����Կ��ɢ����[n]+�ӽ�����Կ��ɢ�㷨[1](�ӽ�����Կ��ɢ������Ϊ0ʱ����)
        //       +�ӽ�����Կʹ�ù�����Կ��־[1]+������Կ����[2](0xFFFF:LMK��������)+������Կ����[1]+������Կ[n]+������Կ��ɢ����[1]
        //       +������Կ��ɢ����[n]+������Կ��ɢ�㷨[1]+������Կʹ�ù�����Կ��־[1]+���ݳ���[2]+����[n]
        //       +���ģʽ[1]+�ӽ��ܱ�־[1]+�ӽ���ģʽ[1]+Iv[8](CBCģʽ����)
        // out : 'A'+�������[2]+�������[n]
        p = psRecvBuf+2;
        iKeyIndex = iGetKeyIndex(p);
        p += 2;
        psKeyData = NULL;
        if(iKeyIndex < 0) {
            // LMK���ܻ򱣻���Կ����
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
                iRetCode = HSMERR_PARA; // ��ɢ�㷨����Ϊ1
                break;
            }
        }
        if(*p++ != 0) {
            // Mac��Կ���벻ʹ�ù�����Կ
            iRetCode = HSMERR_PARA;
            break;
        }
        if(iKeyIndex == -2) {
            // ������Կ����
            iKey2Index = iGetKeyIndex(p); // iKey2Index��psKey2Data��iDiv2Level��psDiv2Data����ָʾ������Կ
            p += 2;
            psKey2Data = NULL;
            if(iKey2Index == -1) {
                // LMK����
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
                    iRetCode = HSMERR_PARA; // ��ɢ�㷨����Ϊ1
                    break;
                }
            }
            if(*p++ != 0) {
                // ������Կ���벻ʹ�ù�����Կ
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
            // �ӽ�����Կ��LMK������Mac��ԿΪ������Կ
            iRetCode = iKeyEncDec0(iKeyIndex, psKeyData, iDivLevel, psDivData, NULL, 0, NULL, iEncDecFlag, iPadFlag, psIv, iAppDataLen, psAppData, &iOutLen, psSendBuf+3);
        } else {
            // �ӽ�����Կ��Ӧ����Կ����
            iRetCode = iKeyEncDec0(iKey2Index, psKey2Data, iDiv2Level, psDiv2Data, psKeyData, iDivLevel, psDivData, iEncDecFlag, iPadFlag, psIv, iAppDataLen, psAppData, &iOutLen, psSendBuf+3);
        }
        if(iRetCode)
            break;
        vLongToStr(iOutLen, 2, psSendBuf+1);
        *piSendLen = 3+iOutLen;
        break;
    case 0xD247: // ͨ�ü���������Կת��
//new        // in  : Ins[2]
        //       +������Կ����[2](0xFFFF:LMK��������, 0xEEEE:������Կ��������)
        //       +������Կ����[1]+������Կ[n]+������Կ��ɢ����[1]+������Կ��ɢ����[n]+������Կ��ɢ�㷨[1](�ӽ�����Կ��ɢ������Ϊ0ʱ����)
        //       +������Կ�Ƿ�ʹ�ù�����Կ[1]
        //       +������Կ������Կ����[2](0xFFFF:LMK��������)+������Կ����[1]+������Կ[n]+������Կ��ɢ����[1]
        //       +������Կ��ɢ����[n]+������Կ��ɢ�㷨[1]+������Կ�Ƿ�ʹ�ù�����Կ[1]
        //       +������Կ����[2](0xFFFF:LMK��������, 0xEEEE:������Կ��������)
        //       +������Կ����[1]+������Կ[n]+������Կ��ɢ����[1]+������Կ��ɢ����[n]+������Կ��ɢ�㷨[1](��������Կ��ɢ������Ϊ0ʱ����)
        //       +������Կ�Ƿ�ʹ�ù�����Կ[1]
        //       +������Կ������Կ����[2](0xFFFF:LMK��������)+������Կ����[1]+������Կ[n]+������Կ��ɢ����[1]
        //       +������Կ��ɢ����[n]+������Կ��ɢ�㷨[1]+������Կ�Ƿ�ʹ�ù�����Կ[1]
        //       +����ģʽ[1]+��������IV[8](CBCģʽ����)+����ģʽ[1]+��������Iv[8](CBCģʽ����)
        //       +����ʱ�������ģʽ[1]+���ݳ���[2]+����[n]
        // out : 'A'+�������[2]+�������[n]
        p = psRecvBuf+2;
        // ��ʼ����������Կ
        iKeyIndex = iGetKeyIndex(p);
        p += 2;
        psKeyData = NULL;
        if(iKeyIndex < 0) {
            // LMK���ܻ򱣻���Կ����
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
                iRetCode = HSMERR_PARA; // ��ɢ�㷨����Ϊ1
                break;
            }
        }
        if(*p++ != 0) {
            iRetCode = HSMERR_PARA; // ������Կ���벻ʹ�ù�����Կ
            break;
        }
        if(iKeyIndex == -1) {
            // LMK���ܵĽ�����Կ
            iKey2Index = -1;
        } else if(iKeyIndex == -2) {
            // ������Կ����
            iKey2Index = iGetKeyIndex(p); // iKey2Index��psKey2Data��iDiv2Level��psDiv2Data����ָʾ������Կ
            p += 2;
            psKey2Data = NULL;
            if(iKey2Index == -1) {
                // LMK���ܵı�����Կ
                if(*p++ != 16) {
                    iRetCode = HSMERR_KEY_LEN;
                    break;
                }
                psKey2Data = p;
                p += 16;
                iKey2Index = 0; // ��Ϊ-1��ʾ��ʹ�ñ�����Կ, ����Ҫ�óɷ�-1
            }
            iDiv2Level = *p++;
            psDiv2Data = p;
            p += iDiv2Level*8;
            if(iDiv2Level) {
                if(*p++ != 1) {
                    iRetCode = HSMERR_PARA; // ��ɢ�㷨����Ϊ1
                    break;
                }
            }
            if(*p++ != 0) {
                iRetCode = HSMERR_PARA; // ������Կ���벻ʹ�ù�����Կ
                break;
            }
        }
        // ��ʼ����������Կ
        iKeyIndexX = iGetKeyIndex(p);
        p += 2;
        psKeyDataX = NULL;
        if(iKeyIndexX < 0) {
            // LMK���ܻ򱣻���Կ����
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
                iRetCode = HSMERR_PARA; // ��ɢ�㷨����Ϊ1
                break;
            }
        }
        if(*p++ != 0) {
            iRetCode = HSMERR_PARA; // ������Կ���벻ʹ�ù�����Կ
            break;
        }
        if(iKeyIndexX == -1) {
            // LMK���ܵļ�����Կ
            iKey2IndexX = -1;
        } else if(iKeyIndexX == -2) {
            // ������Կ����
            iKey2IndexX = iGetKeyIndex(p); // iKey2Index��psKey2Data��iDiv2Level��psDiv2Data����ָʾ������Կ
            p += 2;
            psKey2DataX = NULL;
            if(iKey2IndexX == -1) {
                // LMK���ܵı�����Կ
                if(*p++ != 16) {
                    iRetCode = HSMERR_KEY_LEN;
                    break;
                }
                psKey2DataX = p;
                p += 16;
                iKey2IndexX = 0; // ��Ϊ-1��ʾ��ʹ�ñ�����Կ, ����Ҫ�óɷ�-1
            }
            iDiv2LevelX = *p++;
            psDiv2DataX = p;
            p += iDiv2LevelX*8;
            if(iDiv2LevelX) {
                if(*p++ != 1) {
                    iRetCode = HSMERR_PARA; // ��ɢ�㷨����Ϊ1
                    break;
                }
            }
            if(*p++ != 0) {
                iRetCode = HSMERR_PARA; // ������Կ���벻ʹ�ù�����Կ
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
            iRetCode = HSMERR_PARA; // ����ʱ�������ģʽֻ֧��ģʽ3, 3:�����
            break;
        }
        // ��ʼ����Ҫת���ܵ�����
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
        // ֻ���ޱ����ֵ�ָ�����ſ���ʹ��ȱʡӦ��
        // ���ڴ������ֵ�ָ��, �����Ҫ��ô�����Ϣ������, Ȼ����iRetCodeΪ0
        psSendBuf[0] = 'E';
        psSendBuf[1] = iRetCode;
        *piSendLen = 2;
    }
    return(0);
}
