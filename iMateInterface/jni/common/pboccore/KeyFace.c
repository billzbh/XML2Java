/**************************************
File name     : KeyFace.c
Function      : PBOC2.0 ��Կ���ʽӿ�ģ��
                ���ӿ��ṩ������ܻ��޹ص�PBOC2.0��Կ���ʽӿ�
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

static int sg_iMode = 0;    // ģʽ, 0:ģ������Կģʽ 1:���������ļ�ģʽ 2:��ʿͨ���ܻ�ģʽ

// ��ģ����ָ�������DER��ʽ��Կ
// in  : iKeyLen     : ģ������(�ֽ�)
//       pModulus    : ģ��
//       lE          : ָ��
// out : piDerKeyLen : DER��ʽ��Կ����
//       pDerKey     : DER��ʽ��Կ
// ret : KFR_OK      : OK
//       KFR_PARA    : ��������
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
        sBuf[iLen++] = iKeyLen+1; // ģ��ǰ�油һ��0, �Ա���ģ��Ϊ����
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
    // ��sBuf(����=iLen)�����T30
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
// ��DER��ʽ��Կ�ֽ��ģ����ָ��
// in  : piDerKeyLen : DER��ʽ��Կ����
//       pDerKey     : DER��ʽ��Կ
// out : piKeyLen    : ģ������(�ֽ�)
//       pModulus    : ģ��
//       plE         : ָ��
// ret : KFR_OK      : OK
//       KFR_PARA    : ��������
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
    // psDerN����ָ��ģ��TLV�����빫��ָ��TLV����, iLenΪģ��TLV�����빫��ָ��TLV���󳤶�֮��
    // �Ƚ������ָ��
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
    // iLenΪģ��TLV���󳤶�
    // �ٽ��ģ��
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

// ���ò���
// in  : iMode    : ģʽ 0:ģ������Կģʽ 1:���������ļ�ģʽ 2:��ʿͨ���ܻ�ģʽ
//       pszIp    : ���ܻ�IP��ַ, ģʽ2��ģʽ3����
//       iPortNo  : ���ܻ��˿ں�, ģʽ2��ģʽ3����
// ret : KFR_OK   : �ɹ�
//       ����     : ʧ��
int iKeySetMode(int iMode, char *pszIp, int iPortNo)
{
    int iRet;

    if(iMode > 2)
        return(KFR_PARA); // ��֧�ֵ�ģʽ
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

// ��Կ����
// in  : iKeyType  : 1:DES��Կ 2:RSA��Կ
//       iKeyIndex : ��Կ����
// out : piOutLen  : �������ݳ���
//       pOutData  : ��������
// ret : KFR_OK    : �ɹ�
//       ����      : ʧ��
int iKeyBackup(int iKeyType, int iKeyIndex, int *piOutLen, void *pOutData)
{
	uchar sSendBuf[2048], sRecvBuf[2048];
    uchar *p;
	int   iSendLen, iRecvLen;
	int   iRet;

    if(sg_iMode <= 2) {
        // ģʽ0��1��2����ͬһ�ӿ�(��ʿͨ���ܻ��ӿ�)
        p = sSendBuf;
        switch(iKeyType) {
        case 1: // DES��Կ
            vTwoOne("D230", 4, p);
            p += 2;
            *p++ = 0x04; // 0x04:��LMK����
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
        case 2: // RSA��Կ
            vTwoOne("D233", 4, p);
            p += 2;
            vLongToStr(iKeyIndex, 2, p);
            p += 2;
            *p++ = 1; // 1:ȡ��˽Կ
            *p++ = 1; // 1:˽Կ��ʽΪDER
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

// ��Կ�ָ�
// in  : iKeyType  : 1:DES��Կ 2:RSA��Կ
//       iKeyIndex : ��Կ����
//       iInLen    : �������ݳ���
//       pInData   : ��������
// ret : KFR_OK    : �ɹ�
//       ����      : ʧ��
int iKeyRestore(int iKeyType, int iKeyIndex, int iInLen, void *pInData)
{
	uchar sSendBuf[2048], sRecvBuf[2048];
    uchar *p;
	int   iSendLen, iRecvLen;
	int   iRet;

    if(sg_iMode <= 2) {
        // ģʽ0��1��2����ͬһ�ӿ�(��ʿͨ���ܻ��ӿ�)
        p = sSendBuf;
        switch(iKeyType) {
        case 1: // DES��Կ
            vTwoOne("D231", 4, p);
            p += 2;
            *p++ = 0x04; // 0x04:��LMK����
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
        case 2: // RSA��Կ
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
            *p++ = 1; // 1:DER��ʽ
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

// ��Կɾ��
// in  : iKeyType  : 1:DES��Կ 2:RSA��Կ
//       iKeyIndex : ��Կ����
// ret : KFR_OK    : �ɹ�
//       ����      : ʧ��
int iKeyDelete(int iKeyType, int iKeyIndex)
{
	uchar sSendBuf[2048], sRecvBuf[2048];
    uchar *p;
	int   iSendLen, iRecvLen;
	int   iRet;

    if(sg_iMode <= 2) {
        // ģʽ0��1��2����ͬһ�ӿ�(��ʿͨ���ܻ��ӿ�)
        p = sSendBuf;
        vTwoOne("D22A", 4, p);
        p += 2;
        if(iKeyType == 1)
            *p++ = 2; // 2:������Կ
        else if(iKeyType == 2)
            *p++ = 1; // 1:RSA��Կ
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

// ����DES��Կ
// in  : iKeyIndex : ��Կ����(0-999), -1��ʾ���洢
//       pKey      : LMK���ܺ����, NULL��ʾ�����
// out : pKey      : LMK���ܺ�����Կ[16]+cks[8], NULL��ʾ�����
// ret : KFR_OK    : �ɹ�
//       ����      : ʧ��
// Note: iKeyIndexΪ-1ʱpKey����ΪNULL
int iKeyNewDesKey(int iKeyIndex, void *pKey)
{
	uchar sSendBuf[2048], sRecvBuf[2048];
    uchar *p;
	int   iSendLen, iRecvLen;
	int   iRet;

    if(iKeyIndex==-1 && pKey==NULL)
        return(KFR_PARA); // ���������ֲ����, ������, ��Ϊ��������
    if(sg_iMode <= 2) {
        // ģʽ0��1��2����ͬһ�ӿ�(��ʿͨ���ܻ��ӿ�)
        p = sSendBuf;
        vTwoOne("D244", 4, p);
        p += 2;
        if(iKeyIndex == -1)
            memcpy(p, "\xFF\xFF", 2);
        else
            vLongToStr(iKeyIndex, 2, p);
        p += 2;
        *p++ = 16;
        *p++ = 1; // 1:��LMK����
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

// ����RSA��Կ
// in  : iKeyIndex : ��Կ����(0-19), -1��ʾ���洢
//       iKeyLen   : ��Կ����, ��λ:�ֽ� 64-248, ����Ϊż��
//       lE        : ����ָ��, 3��65537
//       pKey      : LMK���ܺ�����Կ, NULL��ʾ�����
// out : pModulus  : ����ģ��������ΪiKeyLen, NULL��ʾ�����
//       piOutLen  : ����ļ��ܺ���Կ����
//       pKey      : LMK���ܺ�����Կ, NULL��ʾ�����
// ret : KFR_OK    : �ɹ�
//       ����      : ʧ��
// Note: iKeyIndexΪ-1ʱpKey����ΪNULL
int iKeyNewRsaKey(int iKeyIndex, int iKeyLen, long lE, void *pModulus, int *piOutLen, void *pKey)
{
	uchar sSendBuf[2048], sRecvBuf[2048];
    uchar *p;
	int   iSendLen, iRecvLen;
	int   iRet;

    if(iKeyIndex==-1 && pKey==NULL)
        return(KFR_PARA); // ���������ֲ����, ������, ��Ϊ��������
    if(sg_iMode <= 2) {
        // ģʽ0��1��2����ͬһ�ӿ�(��ʿͨ���ܻ��ӿ�)
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
        *p++ = 0x02; // 0x02:�����˽Կ
        *p++ = 1; // 1:��Կ��ʽΪDER
        *p++ = 1; // 1:˽Կ��ʽΪDER
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

// RSA˽Կ����
// in  : iKeyIndex : RSA��Կ����
//       iKeyLen   : LMK���ܺ�RSA˽Կ����
//       pKey      : LMK���ܺ�RSA˽Կ, NULL��ʾʹ��������Կ
//       iLen      : �������ݳ���
//       pIn       : ��������
// out : pOut      : ������, ���ȵ���iLen
// ret : KFR_OK    : �ɹ�
//       ����      : ʧ��
int iKeyRsaPrivateBlock(int iKeyIndex, int iKeyLen, void *pKey, int iLen, void *pIn, void *pOut)
{
	uchar sSendBuf[2048], sRecvBuf[2048];
    uchar *p;
	int   iSendLen, iRecvLen;
	int   iRet;

    if(sg_iMode <= 2) {
        // ģʽ0��1��2����ͬһ�ӿ�(��ʿͨ���ܻ��ӿ�)
        if(pKey) {
            // �ô������Կ����
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
            // ��������Կ����
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

// RSA��Կ����
// in  : iKeyIndex : RSA��Կ����
//       iKeyLen   : ģ������(�ֽ�)
//       pModulus  : ģ��, NULL��ʾʹ��������Կ
//       lE        : ����ָ��, pMudulus!=NULLʱʹ��
//       iLen      : �������ݳ���
//       pIn       : ��������
// out : pOut      : ������, ���ȵ���iLen
// ret : KFR_OK    : �ɹ�
//       ����      : ʧ��
int iKeyRsaPublicBlock(int iKeyIndex, int iKeyLen, void *pModulus, long lE, int iLen, void *pIn, void *pOut)
{
	uchar sSendBuf[2048], sRecvBuf[2048];
    uchar *p;
	int   iSendLen, iRecvLen;
	int   iRet;

    if(sg_iMode <= 2) {
        // ģʽ0��1��2����ͬһ�ӿ�(��ʿͨ���ܻ��ӿ�)
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
        *p++ = 1; // 1:��Կ��ʽΪDER
        *p++ = 1; // 1:����
        *p++ = 0; // 0:�����
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

// ��ȡRSA��Կ
// in  : iKeyIndex : RSA��Կ����
// out : piLen     : RSA��Կ����, ��λ:�ֽ�
//       plE       : ����ָ��
//       pModulus  : ģ��
// ret : KFR_OK    : �ɹ�
//       ����      : ʧ��
int iKeyRsaGetInfo(int iKeyIndex, int *piLen, long *plE, void *pModulus)
{
	uchar sSendBuf[2048], sRecvBuf[2048];
    uchar *p;
	int   iSendLen, iRecvLen;
	int   iRet;

    if(sg_iMode <= 2) {
        // ģʽ0��1��2����ͬһ�ӿ�(��ʿͨ���ܻ��ӿ�)
        int iDerKeyLen;
        p = sSendBuf;
        vTwoOne("D233", 4, p);
        p += 2;
        vLongToStr(iKeyIndex, 2, p);
        p += 2;
        *p++ = 0; // 0:ȡ����Կ
        *p++ = 1; // 1:��Կ��ʽΪDER
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

// ������Ƭ�ⲿ��֤����
// in  : iKey1Index : ��Կ����
//       iKey1Len   : LMK���ܺ���Կ����
//       pKey1      : LMK���ܺ���Կ, NULL��ʾʹ��������Կ
//       iDiv1Level : ��Կ��ɢ����(0-3)
//       pDiv1Data  : ��Կ��ɢ����[8*n]
//       pTermRand  : �ն������[8]
//       pCardData  : ��Ƭ����[28]
// out : pOut       : �ⲿ��֤���ļ�Mac[16]
// ret : KFR_OK     : �ɹ�
//       ����       : ʧ��
int iKeyCalExtAuth(int iKey1Index, int iKey1Len, void *pKey1, int iDiv1Level, void *pDiv1Data, void *pTermRand, void *pCardData, void *pOut)
{
	uchar sSendBuf[2048], sRecvBuf[2048];
    uchar *p;
	int   iSendLen, iRecvLen;
	int   iRet;

    if(sg_iMode <= 2) {
        // ģʽ0��1��2����ͬһ�ӿ�(��ʿͨ���ܻ��ӿ�)
        p = sSendBuf;
        vTwoOne("D237", 4, p);
        p += 2;
        if(pKey1) {
            // ʹ�ô�����Կ
            memcpy(p, "\xFF\xFF", 2);
            p += 2;
            *p++ = ((uchar *)pKey1)[0];
            memcpy(p, ((uchar *)pKey1)+1, ((uchar *)pKey1)[0]);
            p += ((uchar *)pKey1)[0];
        } else {
            // ʹ��������Կ
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

// �����޸�KMC����
// in  : iKey1Index : ԭ��Կ����
//       iKey1Len   : LMK���ܺ�ԭ��Կ����
//       pKey1      : LMK���ܺ�ԭ��Կ, NULL��ʾʹ��������Կ
//       iDiv1Level : ԭ��Կ��ɢ����(0-3)
//       pDiv1Data  : ԭ��Կ��ɢ����[8*n]
//       pTermRand  : �ն������[8]
//       pCardData  : ��Ƭ����[28]
//       iKey2Index : ����Կ����
//       iKey2Len   : LMK���ܺ�����Կ����
//       pKey2      : LMK���ܺ�����Կ, NULL��ʾʹ��������Կ
//       iDiv2Level : ����Կ��ɢ����(0-3)
//       pDiv2Data  : ����Կ��ɢ����[8*n]
// out : pOut       : ����AuthKey[16]+AuthKeyKcv[3]+����MacKey[16]+MacKeyKcv[3]+����EncKey[16]+EncKeyKcv[3]
// ret : KFR_OK     : �ɹ�
//       ����       : ʧ��
int iKeyCalChangeKmc(int iKey1Index, int iKey1Len, void *pKey1, int iDiv1Level, void *pDiv1Data, void *pTermRand, void *pCardData, int iKey2Index, int iKey2Len, void *pKey2, int iDiv2Level, void *pDiv2Data, void *pOut)
{
	uchar sSendBuf[2048], sRecvBuf[2048];
    uchar *p;
	int   iSendLen, iRecvLen;
	int   iRet;
    int   i;

    if(sg_iMode <= 2) {
        // ģʽ0��1��2����ͬһ�ӿ�(��ʿͨ���ܻ��ӿ�)
        p = sSendBuf;
        vTwoOne("D23D", 4, p);
        p += 2;
        if(pKey1) {
            // ʹ�ô�����Կ
            memcpy(p, "\xFF\xFF", 2);
            p += 2;
            *p++ = ((uchar *)pKey1)[0];
            memcpy(p, ((uchar *)pKey1)+1, ((uchar *)pKey1)[0]);
            p += ((uchar *)pKey1)[0];
        } else {
            // ʹ��������Կ
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
            // ʹ�ô�����Կ
            memcpy(p, "\xFF\xFF", 2);
            p += 2;
            *p++ = ((uchar *)pKey2)[0];
            memcpy(p, ((uchar *)pKey2)+1, ((uchar *)pKey2)[0]);
            p += ((uchar *)pKey2)[0];
        } else {
            // ʹ��������Կ
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

// ����IC��DES��Կ����(��ʱ����)
// in  : iKey1Index : ������Կ����
//       iKey1Len   : LMK���ܺ�������Կ����
//       pKey1      : LMK���ܺ�������Կ, NULL��ʾʹ��������Կ
//       iDiv1Level : ������Կ��ɢ����(0-3)
//       pDiv1Data  : ������Կ��ɢ����[8*n]
//       pTermRand  : �ն������[8]
//       pCardData  : ��Ƭ����[28]
//       iKey2Index : Ӧ����Կ����
//       iKey2Len   : LMK���ܺ�Ӧ����Կ����
//       pKey2      : LMK���ܺ�Ӧ����Կ, NULL��ʾʹ��������Կ
//       iDiv2Level : Ӧ����Կ��ɢ����(0-3)
//       pDiv2Data  : Ӧ����Կ��ɢ����[8*n]
// out : pOut       : ����Key[16]+KeyKcv[3]
// ret : KFR_OK     : �ɹ�
//       ����       : ʧ��
int iKeyCalDesKey(int iKey1Index, int iKey1Len, void *pKey1, int iDiv1Level, void *pDiv1Data, void *pTermRand, void *pCardData, int iKey2Index, int iKey2Len, void *pKey2, int iDiv2Level, void *pDiv2Data, void *pOut)
{
	uchar sSendBuf[2048], sRecvBuf[2048];
    uchar *p;
	int   iSendLen, iRecvLen;
	int   iRet;

    if(sg_iMode <= 2) {
        // ģʽ0��1��2����ͬһ�ӿ�(��ʿͨ���ܻ��ӿ�)
        p = sSendBuf;
        vTwoOne("D238", 4, p);
        p += 2;
        if(pKey1) {
            // ʹ�ô�����Կ
            memcpy(p, "\xFF\xFF", 2);
            p += 2;
            *p++ = ((uchar *)pKey1)[0];
            memcpy(p, ((uchar *)pKey1)+1, ((uchar *)pKey1)[0]);
            p += ((uchar *)pKey1)[0];
        } else {
            // ʹ��������Կ
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
            // ʹ�ô�����Կ
            memcpy(p, "\xFF\xFF", 2);
            p += 2;
            *p++ = ((uchar *)pKey2)[0];
            memcpy(p, ((uchar *)pKey2)+1, ((uchar *)pKey2)[0]);
            p += ((uchar *)pKey2)[0];
        } else {
            // ʹ��������Կ
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

// ����IC��PIN����(��ʱ����)
// in  : iKey1Index : ������Կ����
//       iKey1Len   : LMK���ܺ�������Կ����
//       pKey1      : LMK���ܺ�������Կ, NULL��ʾʹ��������Կ
//       iDiv1Level : ������Կ��ɢ����(0-3)
//       pDiv1Data  : ������Կ��ɢ����[8*n]
//       pTermRand  : �ն������[8]
//       pCardData  : ��Ƭ����[28]
//       pPinBlock  : �����������ݿ�[8]
// out : pOut       : ���ĸ����������ݿ�[8]
// ret : KFR_OK     : �ɹ�
//       ����       : ʧ��
int iKeyCalPin(int iKey1Index, int iKey1Len, void *pKey1, int iDiv1Level, void *pDiv1Data, void *pTermRand, void *pCardData, void *pPinBlock, void *pOut)
{
	uchar sSendBuf[2048], sRecvBuf[2048];
    uchar *p;
	int   iSendLen, iRecvLen;
	int   iRet;

    if(sg_iMode <= 2) {
        // ģʽ0��1��2����ͬһ�ӿ�(��ʿͨ���ܻ��ӿ�)
        p = sSendBuf;
        vTwoOne("D239", 4, p);
        p += 2;
        if(pKey1) {
            // ʹ�ô�����Կ
            memcpy(p, "\xFF\xFF", 2);
            p += 2;
            *p++ = ((uchar *)pKey1)[0];
            memcpy(p, ((uchar *)pKey1)+1, ((uchar *)pKey1)[0]);
            p += ((uchar *)pKey1)[0];
        } else {
            // ʹ��������Կ
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

// ����IC��RSA��Կ����(��ʱ����)
// in  : iKey1Index : ������Կ����
//       iKey1Len   : LMK���ܺ�������Կ����
//       pKey1      : LMK���ܺ�������Կ, NULL��ʾʹ��������Կ
//       iDiv1Level : ������Կ��ɢ����(0-3)
//       pDiv1Data  : ������Կ��ɢ����[8*n]
//       pTermRand  : �ն������[8]
//       pCardData  : ��Ƭ����[28]
//       iRsaKeyLen : RSA��Կ����, ��λ:�ֽ�, 64-248, ����Ϊż��
//       lE         : ����ָ��ֵ, 3����65537
// out : pOut       : RSA��Կ��Կ����[n]+D����[n1]+P����[n2]+Q����[n2]+dP����[n2]+dQ����[n2]+qInv����[n2]
//                    n = iRsaKeyLen, n1 = (iRsaKeyLen+8)/8*8, n2 = (iRsaKeyLen/2+8)/8*8
// ret : KFR_OK     : �ɹ�
//       ����       : ʧ��
int iKeyCalRsaKey(int iKey1Index, int iKey1Len, void *pKey1, int iDiv1Level, void *pDiv1Data, void *pTermRand, void *pCardData, int iRsaKeyLen, long lE, void *pOut)
{
	uchar sSendBuf[2048], sRecvBuf[2048];
    uchar *p;
	int   iSendLen, iRecvLen;
	int   iRet;
    int   i;

    if(sg_iMode <= 2) {
        // ģʽ0��1��2����ͬһ�ӿ�(��ʿͨ���ܻ��ӿ�)
        int  iDerKeyLen, iNLen;
        int  iDLen; // ������Dֵ����
        int  iPLen; // ������Pֵ����(Q��dP��dQ��qInv����ͬ)
        long lTmpE;
        p = sSendBuf;
        vTwoOne("D23A", 4, p);
        p += 2;
        vLongToStr(iRsaKeyLen*8, 2, p);
        p += 2;
        vLongToStr(lE, 4, p);
        p += 4;
        *p++ = 1; // 1:��Կ��ʽΪDER
        *p++ = 2; // 2:˽Կ��ʽΪIC��д����Ҫ�ļ��ܸ�ʽ
        if(pKey1) {
            // ʹ�ô�����Կ
            memcpy(p, "\xFF\xFF", 2);
            p += 2;
            *p++ = ((uchar *)pKey1)[0];
            memcpy(p, ((uchar *)pKey1)+1, ((uchar *)pKey1)[0]);
            p += ((uchar *)pKey1)[0];
        } else {
            // ʹ��������Կ
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

// ��֤ARQC��TC, ����ARPC
// in  : iKey1Index     : ��Կ����
//       iKey1Len       : LMK���ܺ���Կ����
//       pKey1          : LMK���ܺ���Կ, NULL��ʾʹ��������Կ
//       iDiv1Level     : ��Կ��ɢ����(0-3)
//       pDiv1Data      : ��Կ��ɢ����[8*n]
//       pATC           : �������к�[2]
//       iTransDataLen  : �������ݳ���
//       pTransData     : ��������
//       pARQC          : ARQC��TC
//       pARC           : ����Ӧ����, NULL��ʾֻ��֤ARQC��TC
// out : pARPC          : ARPC
// ret : KFR_OK         : �ɹ�
//       ����           : ʧ��
int iKeyVerifyAc(int iKey1Index, int iKey1Len, void *pKey1, int iDiv1Level, void *pDiv1Data, void *pATC, int iTransDataLen, void *pTransData, void *pARQC, void *pARC, void *pARPC)
{
	uchar sSendBuf[2048], sRecvBuf[2048];
    uchar *p;
	int   iSendLen, iRecvLen;
	int   iRet;

    if(sg_iMode <= 2) {
        // ģʽ0��1��2����ͬһ�ӿ�(��ʿͨ���ܻ��ӿ�)
        p = sSendBuf;
        vTwoOne("D23C", 4, p);
        p += 2;
        if(pARC) {
            // ����Ӧ�������, ��Ҫ����ARPC
            *p++ = 1; // 1:��֤ARQC������ARPC
        } else {
            // ����Ӧ���벻����, ����Ҫ����ARPC
            *p++ = 0; // 0:ֻ��֤ARQC
        }
        if(pKey1) {
            // ʹ�ô�����Կ
            memcpy(p, "\xFF\xFF", 2);
            p += 2;
            *p++ = ((uchar *)pKey1)[0];
            memcpy(p, ((uchar *)pKey1)+1, ((uchar *)pKey1)[0]);
            p += ((uchar *)pKey1)[0];
        } else {
            // ʹ��������Կ
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

// �ù�����Կ��������(�ű�)
// in  : iKey1Index    : ��Կ����
//       iKey1Len      : LMK���ܺ���Կ����
//       pKey1         : LMK���ܺ���Կ, NULL��ʾʹ��������Կ
//       iDiv1Level    : ��Կ��ɢ����(0-3)
//       pDiv1Data     : ��Կ��ɢ����[8*n]
//       pATC          : �������к�[2]
//       iPinFlag      : PinBlock���ܱ�־, 0:����PinBlock, 1:��PinBlock
//       iDataLen      : ���ݳ���
//       pData         : ����
// out : piOutLen      : ������ݳ���
//       pOutData      : �������
// ret : KFR_OK        : �ɹ�
//       ����          : ʧ��
int iKeyScriptEncData(int iKey1Index, int iKey1Len, void *pKey1, int iDiv1Level, void *pDiv1Data, void *pATC, int iPinFlag, int iDataLen, void *pData, int *piOutLen, void *pOutData)
{
	uchar sSendBuf[2048], sRecvBuf[2048];
    uchar *p;
	int   iSendLen, iRecvLen;
	int   iRet;

    if(sg_iMode <= 2) {
        // ģʽ0��1��2����ͬһ�ӿ�(��ʿͨ���ܻ��ӿ�)
        if(iPinFlag) {
            // ����PIN�ű�
            if(iDataLen != 8)
                return(KFR_PARA);
            p = sSendBuf;
            vTwoOne("D241", 4, p);
            p += 2;
            *p++ = 1; // 1:PBOC2.0�淶��
            if(pKey1) {
                // ʹ�ô�����Կ
                memcpy(p, "\xFF\xFF", 2);
                p += 2;
                *p++ = ((uchar *)pKey1)[0];
                memcpy(p, ((uchar *)pKey1)+1, ((uchar *)pKey1)[0]);
                p += ((uchar *)pKey1)[0];
            } else {
                // ʹ��������Կ
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
            *p++ = 1; // 1:����
            *p++ = 1; // 1:PBOC2.0�淶��
            if(pKey1) {
                // ʹ�ô�����Կ
                memcpy(p, "\xFF\xFF", 2);
                p += 2;
                *p++ = ((uchar *)pKey1)[0];
                memcpy(p, ((uchar *)pKey1)+1, ((uchar *)pKey1)[0]);
                p += ((uchar *)pKey1)[0];
            } else {
                // ʹ��������Կ
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

// �ù�����Կ����Mac(�ű�)
// in  : iKey1Index    : ��Կ����
//       iKey1Len      : LMK���ܺ���Կ����
//       pKey1         : LMK���ܺ���Կ, NULL��ʾʹ��������Կ
//       iDiv1Level    : ��Կ��ɢ����(0-3)
//       pDiv1Data     : ��Կ��ɢ����[8*n]
//       pATC          : �������к�[2]
//       iDataLen      : ���ݳ���
//       pData         : ����
// out : pMac          : Mac[8]
// ret : KFR_OK        : �ɹ�
//       ����          : ʧ��
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
        *p++ = 1; // 1:PBOC2.0�淶��
        if(pKey1) {
            // ʹ�ô�����Կ
            memcpy(p, "\xFF\xFF", 2);
            p += 2;
            *p++ = ((uchar *)pKey1)[0];
            memcpy(p, ((uchar *)pKey1)+1, ((uchar *)pKey1)[0]);
            p += ((uchar *)pKey1)[0];
        } else {
            // ʹ��������Կ
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

// ����������Կת��(����׼���ļ�����)
// in  : iKey1Index : ������Կ����
//       iKey1Len   : LMK���ܺ�������Կ����
//       pKey1      : LMK���ܺ�������Կ, NULL��ʾʹ��������Կ
//       iDiv1Level : ������Կ��ɢ����(0-3)
//       pDiv1Data  : ������Կ��ɢ����[8*n]
//       pTermRand  : �ն������[8]
//       pCardData  : ��Ƭ����[28]
//       iKey2Index : Ӧ����Կ����
//       iKey2Len   : LMK���ܺ�Ӧ����Կ����
//       pKey2      : LMK���ܺ�Ӧ����Կ, NULL��ʾʹ��������Կ
//       iDiv2Level : Ӧ����Կ��ɢ����(0-3)
//       pDiv2Data  : Ӧ����Կ��ɢ����[8*n]
//       iDataLen   : ԭ�������ݳ���, ����Ϊ8�ı���
//       pData      : ԭ��������
// out : pOut       : ת���ܺ�����, ���ȵ���ԭ�������ݳ���
// ret : KFR_OK     : �ɹ�
//       ����       : ʧ��
// Note: ����ԭΪӦ����Կ����, ��������Կ�Ĺ�����Կת����
int iKeyReEncData(int iKey1Index, int iKey1Len, void *pKey1, int iDiv1Level, void *pDiv1Data, void *pTermRand, void *pCardData, int iKey2Index, int iKey2Len, void *pKey2, int iDiv2Level, void *pDiv2Data, int iDataLen, void *pData, void *pOut)
{
	uchar sSendBuf[2048], sRecvBuf[2048];
    uchar *p;
	int   iSendLen, iRecvLen;
	int   iRet;
    int   i;

    if(sg_iMode <= 2) {
        // ģʽ0��1��2����ͬһ�ӿ�(��ʿͨ���ܻ��ӿ�)
        p = sSendBuf;
        vTwoOne("D23E", 4, p);
        p += 2;
        if(pKey2) {
            // ʹ�ô�����Կ
            memcpy(p, "\xFF\xFF", 2);
            p += 2;
            *p++ = ((uchar *)pKey2)[0];
            memcpy(p, ((uchar *)pKey2)+1, ((uchar *)pKey2)[0]);
            p += ((uchar *)pKey2)[0];
        } else {
            // ʹ��������Կ
            vLongToStr(iKey2Index, 2, p);
            p += 2;
        }
        *p++ = iDiv2Level;
        if(iDiv2Level)
            memcpy(p, pDiv2Data, iDiv2Level*8);
        p += iDiv2Level*8;
        if(pKey1) {
            // ʹ�ô�����Կ
            memcpy(p, "\xFF\xFF", 2);
            p += 2;
            *p++ = ((uchar *)pKey1)[0];
            memcpy(p, ((uchar *)pKey1)+1, ((uchar *)pKey1)[0]);
            p += ((uchar *)pKey1)[0];
        } else {
            // ʹ��������Կ
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
        *p++ = 0; // 0:ECBģʽ
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

// ����������Կת��(����׼���ļ�����)
// in  : iKey1Index : ������Կ����
//       iKey1Len   : LMK���ܺ�������Կ����
//       pKey1      : LMK���ܺ�������Կ, NULL��ʾʹ��������Կ
//       iDiv1Level : ������Կ��ɢ����(0-3)
//       pDiv1Data  : ������Կ��ɢ����[8*n]
//       pTermRand  : �ն������[8]
//       pCardData  : ��Ƭ����[28]
//       iKey2Index : Ӧ����Կ����
//       iKey2Len   : LMK���ܺ�Ӧ����Կ����
//       pKey2      : LMK���ܺ�Ӧ����Կ, NULL��ʾʹ��������Կ
//       iDiv2Level : Ӧ����Կ��ɢ����(0-3)
//       pDiv2Data  : Ӧ����Կ��ɢ����[8*n]
//       iPinFormat : ������ʽ, 1:��ʽ 2:��ʽ2
//       pPinBlock  : ԭ���ܺ������[8]
// out : pOut       : ת���ܺ������[8]
// ret : KFR_OK     : �ɹ�
//       ����       : ʧ��
// Note: ����ԭΪӦ����Կ����, ��������Կ�Ĺ�����Կת����
int iKeyReEncPin(int iKey1Index, int iKey1Len, void *pKey1, int iDiv1Level, void *pDiv1Data, void *pTermRand, void *pCardData, int iKey2Index, int iKey2Len, void *pKey2, int iDiv2Level, void *pDiv2Data, int iPinFormat, void *pPinBlock, void *pOut)
{
	uchar sSendBuf[2048], sRecvBuf[2048];
    uchar *p;
	int   iSendLen, iRecvLen;
	int   iRet;

    if(sg_iMode <= 2) {
        // ģʽ0��1��2����ͬһ�ӿ�(��ʿͨ���ܻ��ӿ�)
        p = sSendBuf;
        vTwoOne("D23F", 4, p);
        p += 2;
        if(pKey2) {
            // ʹ�ô�����Կ
            memcpy(p, "\xFF\xFF", 2);
            p += 2;
            *p++ = ((uchar *)pKey2)[0];
            memcpy(p, ((uchar *)pKey2)+1, ((uchar *)pKey2)[0]);
            p += ((uchar *)pKey2)[0];
        } else {
            // ʹ��������Կ
            vLongToStr(iKey2Index, 2, p);
            p += 2;
        }
        *p++ = iDiv2Level;
        if(iDiv2Level)
            memcpy(p, pDiv2Data, iDiv2Level*8);
        p += iDiv2Level*8;
        if(pKey1) {
            // ʹ�ô�����Կ
            memcpy(p, "\xFF\xFF", 2);
            p += 2;
            *p++ = ((uchar *)pKey1)[0];
            memcpy(p, ((uchar *)pKey1)+1, ((uchar *)pKey1)[0]);
            p += ((uchar *)pKey1)[0];
        } else {
            // ʹ��������Կ
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
        *p++ = iPinFormat; // ת��ǰ��ʽ
        *p++ = 2; // ת�����ʽ, 2:PinBlock��ʽ2
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

// Mac����(����׼��)
// in  : iKey1Index  : Ӧ����Կ����
//       iKey1Len    : LMK���ܺ�Ӧ����Կ����
//       pKey1       : LMK���ܺ�Ӧ����Կ, NULL��ʾʹ��������Կ
//       iDiv1Level  : Ӧ����Կ��ɢ����(0-3)
//       pDiv1Data   : Ӧ����Կ��ɢ����[8*n]
//       pIv         : ��ʼ����[8]
//       iDataLen    : ���ݳ���, ����Ϊ8�ı���
//       pData       : ����
//       pMacKey     : ��Ӧ����Կ���ܺ��Mac��Կ
//       iFinalFlag  : ���һ���־, 0:�������һ�� 1:�����һ��
// out : pMac        : Mac[8]
// ret : KFR_OK      : �ɹ�
//       ����        : ʧ��
// Note: ����ֽ��ĵ�������, ��iKeyDpCalMac()�ṩ����
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
        memcpy(p, "\xEE\xEE", 2); // 'EEEE':��Կ��������Կ����
        p += 2;
        *p++ = 16;
        memcpy(p, pMacKey, 16);
        p += 16;
        *p++ = 0;
        *p++ = 0; // 0:��ʹ�ù�����Կ
        if(pKey1) {
            // ʹ�ô�����Կ
            memcpy(p, "\xFF\xFF", 2);
            p += 2;
            *p++ = ((uchar *)pKey1)[0];
            memcpy(p, ((uchar *)pKey1)+1, ((uchar *)pKey1)[0]);
            p += ((uchar *)pKey1)[0];
        } else {
            // ʹ��������Կ
            vLongToStr(iKey1Index, 2, p);
            p += 2;
        }
        *p++ = iDiv1Level;
        if(iDiv1Level)
            memcpy(p, pDiv1Data, iDiv1Level*8);
        p += iDiv1Level*8;
        if(iDiv1Level)
            *p++ = 1; // ������Կ��ɢ�㷨, 1:Pboc
        *p++ = 0; // 0:������Կ��ʹ�ù�����Կ
//        *p++ = 4; // 4:pboc 3Des�㷨
        *p++ = 2; // 2:ansi919�㷨, 3Des Retail��ʽ
        memcpy(p, pIv, 8);
        p += 8;
        if(iFinalFlag)
            *p++ = 2; // 1:Ψһ������һ��
        else
            *p++ = 1; // 1:��һ����м��
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
// Mac����(����׼��)
// in  : iKey1Index  : Ӧ����Կ����
//       iKey1Len    : LMK���ܺ�Ӧ����Կ����
//       pKey1       : LMK���ܺ�Ӧ����Կ, NULL��ʾʹ��������Կ
//       iDiv1Level  : Ӧ����Կ��ɢ����(0-3)
//       pDiv1Data   : Ӧ����Կ��ɢ����[8*n]
//       pMacKey     : ��Ӧ����Կ���ܺ��Mac��Կ
//       pIv         : ��ʼ����[8]
//       iDataLen    : ���ݳ���, ����Ϊ8�ı���
//       pData       : ����
// out : pMacKey     : ��Ӧ����Կ���ܺ�������ʱ��Կ[16]
//       pMac        : Mac[8]
// ret : KFR_OK      : �ɹ�
//       ����        : ʧ��
// Note: ������ʱ��Կ, Ȼ�����һ����iKeyDpCalMacPart()����Mac
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
        // 1. ������ʱMac��Կ
        p = sSendBuf;
        vTwoOne("D244", 4, p);
        p += 2;
        memcpy(p, "\xFF\xFF", 2); // ��������Կ
        p += 2;
        *p++ = 16;
        *p++ = 2; // 2:��������Կ����
        if(pKey1) {
            // ʹ�ô�����Կ
            memcpy(p, "\xFF\xFF", 2);
            p += 2;
            *p++ = ((uchar *)pKey1)[0];
            memcpy(p, ((uchar *)pKey1)+1, ((uchar *)pKey1)[0]);
            p += ((uchar *)pKey1)[0];
        } else {
            // ʹ��������Կ
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
        memcpy(sTmpMacKey, sRecvBuf+1, 16); // sTmpMacKeyΪ��������Կ�����˵���ʱ��Կ

        // 2. ����Mac
        memcpy(sIv, pIv, 8);
        psData = (uchar *)pData;
        for(i=0; i<iDataLen; i+=1024) {
            iLen = iDataLen-i > 1024 ? 1024 : iDataLen-i;
            iFinalFlag = 0;
            if(iDataLen-i <= 1024)
                iFinalFlag = 1; // ���һ���Ψһ��
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

// ��������׼��DES��Կ����(����׼��)
// in  : iKey1Index  : TK��Կ����
//       iKey1Len    : LMK���ܺ�TK��Կ����
//       pKey1       : LMK���ܺ�TK��Կ, NULL��ʾʹ��������Կ
//       iDiv1Level  : TK��Կ��ɢ����(0-3)
//       pDiv1Data   : TK��Կ��ɢ����[8*n]
//       iKey2Index  : Ӧ����Կ����
//       iKey2Len    : LMK���ܺ�Ӧ����Կ����
//       pKey2       : LMK���ܺ�Ӧ����Կ, NULL��ʾʹ��������Կ
//       iDiv2Level  : Ӧ����Կ��ɢ����(0-3)
//       pDiv2Data   : Ӧ����Կ��ɢ����[8*n]
// out : pOut        : OutKey[16]+Cks[3]
// ret : KFR_OK     : �ɹ�
//       ����       : ʧ��
int iKeyDpCalDesKey(int iKey1Index, int iKey1Len, void *pKey1, int iDiv1Level, void *pDiv1Data, int iKey2Index, int iKey2Len, void *pKey2, int iDiv2Level, void *pDiv2Data, void *pOut)
{
	uchar sSendBuf[2048], sRecvBuf[2048];
    uchar *p;
	int   iSendLen, iRecvLen;
	int   iRet;

    if(sg_iMode <= 2) {
        // ģʽ0��1��2����ͬһ�ӿ�(��ʿͨ���ܻ��ӿ�)
        p = sSendBuf;
        vTwoOne("D245", 4, p);
        p += 2;
        if(pKey1) {
            // ʹ�ô�����Կ
            memcpy(p, "\xFF\xFF", 2);
            p += 2;
            *p++ = ((uchar *)pKey1)[0];
            memcpy(p, ((uchar *)pKey1)+1, ((uchar *)pKey1)[0]);
            p += ((uchar *)pKey1)[0];
        } else {
            // ʹ��������Կ
            vLongToStr(iKey1Index, 2, p);
            p += 2;
        }
        *p++ = iDiv1Level;
        if(iDiv1Level)
            memcpy(p, pDiv1Data, iDiv1Level*8);
        p += iDiv1Level*8;
        if(iDiv1Level)
            *p++ = 1; // TK��ɢ�㷨, 1:pboc
        if(pKey2) {
            // ʹ�ô�����Կ
            memcpy(p, "\xFF\xFF", 2);
            p += 2;
            *p++ = ((uchar *)pKey2)[0];
            memcpy(p, ((uchar *)pKey2)+1, ((uchar *)pKey2)[0]);
            p += ((uchar *)pKey2)[0];
        } else {
            // ʹ��������Կ
            vLongToStr(iKey2Index, 2, p);
            p += 2;
        }
        *p++ = iDiv2Level;
        if(iDiv2Level)
            memcpy(p, pDiv2Data, iDiv2Level*8);
        p += iDiv2Level*8;
        if(iDiv2Level)
            *p++ = 1; // Ӧ����Կ��ɢ�㷨, 1:pboc
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

// ��������׼��RSA��Կ����(����׼��)
// in  : iKey1Index  : TK��Կ����
//       iKey1Len    : LMK���ܺ�TK��Կ����
//       pKey1       : LMK���ܺ�TK��Կ, NULL��ʾʹ��������Կ
//       iDiv1Level  : TK��Կ��ɢ����(0-3)
//       pDiv1Data   : TK��Կ��ɢ����[8*n]
//       iRsaKeyLen  : RSA��Կ����, ��λ:�ֽ�, 64-248, ����Ϊż��
//       lE          : ����ָ��ֵ, 3����65537
// out : pOut        : RSA��Կ��Կ����[n]+D����[n1]+P����[n2]+Q����[n2]+dP����[n2]+dQ����[n2]+qInv����[n2]
//                     n = iRsaKeyLen, n1 = (iRsaKeyLen+8)/8*8, n2 = (iRsaKeyLen/2+8)/8*8
// ret : KFR_OK      : �ɹ�
//       ����        : ʧ��
int iKeyDpCalRsaKey(int iKey1Index, int iKey1Len, void *pKey1, int iDiv1Level, void *pDiv1Data, int iRsaKeyLen, long lE, void *pOut)
{
	uchar sSendBuf[2048], sRecvBuf[2048];
    uchar *p;
	int   iSendLen, iRecvLen;
	int   iRet;
    int   i;

    if(sg_iMode <= 2) {
        // ģʽ0��1��2����ͬһ�ӿ�(��ʿͨ���ܻ��ӿ�)
        int  iDerKeyLen, iNLen;
        int  iDLen; // ������Dֵ����
        int  iPLen; // ������Pֵ����(Q��dP��dQ��qInv����ͬ)
        long lTmpE;
        p = sSendBuf;
        vTwoOne("D23B", 4, p);
        p += 2;
        vLongToStr(iRsaKeyLen*8, 2, p);
        p += 2;
        vLongToStr(lE, 4, p);
        p += 4;
        *p++ = 1; // 1:��Կ��ʽΪDER
        *p++ = 2; // 2:˽Կ��ʽΪ����׼����Ҫ�ļ��ܸ�ʽ
        if(pKey1) {
            // ʹ�ô�����Կ
            memcpy(p, "\xFF\xFF", 2);
            p += 2;
            *p++ = ((uchar *)pKey1)[0];
            memcpy(p, ((uchar *)pKey1)+1, ((uchar *)pKey1)[0]);
            p += ((uchar *)pKey1)[0];
        } else {
            // ʹ��������Կ
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
