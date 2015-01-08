/**************************************
File name     : HSMSIMU.C
Function      : ������ܻ�
Author        : Yu Jun
First edition : Mar 18th, 2011
Modified      : 
Note          : 20110323���ɶ���̷�ʽ�ĳɶ��̷߳�ʽ�������˱���KeyMutex����Կ���¿��ƣ�
                          ����ʱ���ϵ��ֻ������Կ����ʱ�Ļ�����ƣ�ʹ����Կʱû����������ƣ�
                          ��������ܻ�������ʱ��������������Ժ�Ҫ�ø÷�����ܻ�����ʽ��;��
                          ��Ҫȫ�������Կʹ��ʱ�Ļ�����ơ�
                20110706, ��д��Կ����ӿ�, ֧���ⲿ������Կ����
				20140519, �����ⲿ��֤����ʱ�Զ�����212 202��ɢ��ʽ, ����ǵ�����sg_iGpDivMethod��
**************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "VposFace.h"
#include "rsaref.h"
#include "r_keygen.h"
#include "hsmsimu.h"

// #define USE_KEY_POOL

#ifdef USE_KEY_POOL
#include "KeyPool.h"
#endif

#define MIN_RSA_KEY_LEN     64
#define MAX_RSA_KEY_LEN     256

// for des use
#define ENCRYPT             1
#define DECRYPT             2
#define TRI_ENCRYPT         3
#define TRI_DECRYPT         4

// QINGBO
static unsigned char sg_ucDivMode = 1; // ��Ƭ��ɢģʽ(0:����ɢ 1:��ɢ)

static int sg_iMasterKeyIndex;      // ����������Կ������Կ����
static int sg_iGpDivMethod = 212;   // ���ڱ�ǿ�ƬKMC��ɢ��ʽ, 212:Gp212��ɢ��ʽ 202:Gp202��ɢ��ʽ

typedef struct {
    char szInf[40+1];               // ��Կ˵��
    char ucOKFlag;                  // ��Կ���ñ�־��0:��Կ������ 1:OK
    char sKey[16];                  // ��Կ
} stDesKey;
typedef struct {
    char szInf[40+1];               // ��Կ˵��
    char ucOKFlag;                  // ��Կ���ñ�־��0:��Կ������ 1:Dֵ���� 2:CRT���� 3:Dֵ��CRT������
    char sPassword[8];              // ����
    int  iKeyLen;                   // ��Կ���ȣ��ֽ�Ϊ��λ������Ϊż��
    long lE;                        // E��3��65537
    char sN[MAX_RSA_KEY_LEN];       // N
    char sD[MAX_RSA_KEY_LEN];       // D
    char sP[MAX_RSA_KEY_LEN/2];     // P
    char sQ[MAX_RSA_KEY_LEN/2];     // Q
    char sDP[MAX_RSA_KEY_LEN/2];    // dP
    char sDQ[MAX_RSA_KEY_LEN/2];    // dQ
    char sQINV[MAX_RSA_KEY_LEN/2];  // qInv
} stRsaKey;

typedef struct {
    int  iIndex;                    // ɨ����е���Կ������
    char *pszName;                  // ɨ����е�����(ȥ����index***_����)������"key"��"inf"��"n"��"e"...
    char *pszContent;               // ɨ����е�����
} stRow; // һ�����ݵĽṹ�����硾index012_e = 65537����������,stRow.iIndex=12 stRow.psNameָ��"e" stRow.psContentָ��"65537"

static stDesKey sg_aMasterKey[3];       // ����Կ�洢��
static stDesKey sg_aDesKey[1000];       // DES��Կ�洢��
static stRsaKey sg_aRsaKey[20];         // RSA��Կ�洢��
static int      sg_iMaxMasterKeyNum;    // �������Կ����
static int      sg_iMaxDesKeyNum;       // ���des��Կ����
static int      sg_iMaxRsaKeyNum;       // ���rsa��Կ����
static char     sg_szKeyFileName[256];  // ��Կ�ļ�����


/*
extern void _Des(char cryp_decrypt,unsigned char *DES_DATA,unsigned char *DESKEY,unsigned char *DES_RESULT);

static void _vDes(uint uiMode, uchar *psSource, uchar *psKey, uchar *psResult)
{
    uchar sBuf[8];
    switch(uiMode) {
        case ENCRYPT:
        case DECRYPT:
            _Des((char)uiMode, (uchar*)psSource, (uchar*)psKey, psResult);
            break;
        case TRI_ENCRYPT:
            _Des(ENCRYPT, (uchar*)psSource, (uchar*)psKey, psResult);
            _Des(DECRYPT, (uchar*)psResult, (uchar*)psKey+8, sBuf);
            _Des(ENCRYPT, (uchar*)sBuf, (uchar*)psKey, psResult);
            break;
        case TRI_DECRYPT:
            _Des(DECRYPT, (uchar*)psSource, (uchar*)psKey, psResult);
            _Des(ENCRYPT, (uchar*)psResult, (uchar*)psKey+8, sBuf);
            _Des(DECRYPT, (uchar*)sBuf, (uchar*)psKey, psResult);
			break;
    }
}

static void _vGetTime(uchar *pszTime)
{
    time_t t;
    struct tm tm;
 
    t = time( NULL );
    memcpy(&tm, localtime(&t), sizeof(struct tm));
    sprintf(pszTime, "%04d%02d%02d%02d%02d%02d",
                tm.tm_year+1900, tm.tm_mon+1, tm.tm_mday,
                tm.tm_hour, tm.tm_min, tm.tm_sec);
}
 */

// ��ż������Կ
// in  : iFlag : ��ż��־, 0:ż���� 1:������
//       pKey  : ��Կ
// out : pKey  : ��������Կ
static void vReformKey(int iFlag, void *pKey)
{
    uchar *psKey;
    int   i, j;
    int   iCounter;
    uchar ucTmp;
    psKey = (uchar *)pKey;
    for(i=0; i<16; i++,psKey++) {
        ucTmp = *psKey;
        iCounter = 0;
        for(j=0; j<8; j++) {
            if(ucTmp & 0x80)
                iCounter ++;
            ucTmp <<= 1;
        }
        if((iCounter%2 && iFlag==0) || (iCounter%2==0 && iFlag==1))
            *psKey ^= 0x01;
    }
}

// ����һ������
// in  : pszRow : һ������
// out : pRow   : �����õ�����
// ret : 0      : OK
//       1      : ��������
static int iFactRow(char *pszRow, stRow *pRow)
{
    char *p;
    int  i;
    p = pszRow;
    if(strlen(p) > 600)
        return(1);
    
    while(*p==' ' || *p=='\t')
        p ++;
    if(memcmp(p, "index", 5) != 0)
        return(1);
    p += 5;
    if(strlen(p) < 3)
        return(1);
    pRow->iIndex = (p[0]-'0')*100 + (p[1]-'0')*10 + (p[2]-'0');
    if(pRow->iIndex < 0)
        return(1);
    p += 3;
    if(p[0] != '_')
        return(1);
    p ++;
    pRow->pszName = p;
    while(*p!='=' && *p!=' ' && *p!='\t' && *p!='\r' && *p!='\n' && *p!=0)
        p ++;
    if(p[0] == '=') {
        *p++ = 0;
    } else {
        *p++ = 0;
        while(*p==' ' || *p=='\t')
            p ++;
        if(*p++ != '=')
            return(1);
    }
    while(*p==' ' || *p=='\t')
        p ++;
    pRow->pszContent = p;
    while(*p) {
        if(*p=='\r' || *p=='\n') {
            *p = 0;
            break;
        }
        p ++;
    }
    for(i=strlen(pRow->pszContent); i>0; i--) {
        if(pRow->pszContent[i]!=' ' && pRow->pszContent[i]!='\t')
            break;
        pRow->pszContent[i] = 0;
    }
    return(0);
}

// ECBģʽ�ӽ�������
// in  : iFlag     : ENCRYPT or DECRYPT or TRI_ENCRYPT or TRI_DECRYPT
//       iLen      : ����, ����Ϊ8�ı���
//       pInData   : ��������
//       pKey      : ��Կ
// out : pOutData  : ������ݣ����ȵ����������ݳ���
static void vDesEcb(int iFlag, int iLen, void *pInData, void *pKey, void *pOutData)
{
    uchar *psInData, *psKey, *psOutData;
    int i;
    psInData = (uchar *)pInData;
    psKey = (uchar *)pKey;
    psOutData = (uchar *)pOutData;

    for(i=0; i<(iLen+7)/8; i++)
        _vDes(iFlag, psInData+i*8, psKey, psOutData+i*8);
}
// CBCģʽ�ӽ�������
// in  : iFlag     : ENCRYPT or DECRYPT or TRI_ENCRYPT or TRI_DECRYPT
//       pIv       : IV
//       iLen      : ����, ����Ϊ8�ı���
//       pInData   : ��������
//       pKey      : ��Կ
// out : pOutData  : ������ݣ����ȵ����������ݳ���
static void vDesCbc(int iFlag, void *pIv, int iLen, void *pInData, void *pKey, void *pOutData)
{
    uchar *psInData, *psKey, *psOutData;
    uchar sIv[8];
    int i;
    psInData = (uchar *)pInData;
    psKey = (uchar *)pKey;
    psOutData = (uchar *)pOutData;

    memcpy(sIv, pIv, 8);
    if(iFlag==ENCRYPT || iFlag==TRI_ENCRYPT) {
        for(i=0; i<(iLen+7)/8; i++) {
            vXor(sIv, psInData+i*8, 8);
            _vDes(iFlag, sIv, psKey, psOutData+i*8);
            memcpy(sIv, psOutData+i*8, 8);
        }
    } else {
        for(i=0; i<(iLen+7)/8; i++) {
            _vDes(iFlag, psInData+i*8, psKey, psOutData+i*8);
            vXor(psOutData+i*8, sIv, 8);
            memcpy(sIv, psInData+i*8, 8);
        }
    }
}

// ����Կ�ļ���ȡ��Կ
// ret : 0 : OK
//       ����       : �ο�hsmsimu.h������
static int iKeyFileLoad(void)
{
    FILE *fp;
    char     szKeyBuf[1024], *p;
    enum     {SecNone, SecMasterKey, SecDesKey, SecRsaKey} eCurSection; // ����ɨ�����Կ��
    stRow    Row;   // ��ǰ������
    int      i;
    int      iRet;
    
    if(sg_szKeyFileName[0] == 0)
        return(0);

    memset(sg_aMasterKey, 0, sizeof(sg_aMasterKey));
    memset(sg_aDesKey, 0, sizeof(sg_aDesKey));
    memset(sg_aRsaKey, 0, sizeof(sg_aRsaKey));
    fp = fopen(sg_szKeyFileName, "rt");
    if(fp == NULL) {
        return(0); // ���ļ�ʧ�ܣ���Ϊ��ԿΪ�գ�������
    }
    
    eCurSection = SecNone;
    for(;;) {
        memset(szKeyBuf, 0, sizeof(szKeyBuf));
        p = fgets(szKeyBuf, sizeof(szKeyBuf), fp);
        if(p == NULL)
            break;
        if(szKeyBuf[0] == '#')
            continue; // ��һ���ֽ�Ϊ��#����ʾע��
        if(strstr(szKeyBuf, "[master key]")) {
            eCurSection = SecMasterKey;
            continue;
        } else if(strstr(szKeyBuf, "[des key]")) {
            eCurSection = SecDesKey;
            continue;
        } else if(strstr(szKeyBuf, "[rsa key]")) {
            eCurSection = SecRsaKey;
            continue;
        }
        iRet = iFactRow(szKeyBuf, &Row);
        if(iRet)
            continue;
        if(eCurSection == SecNone)
            continue;
        if(eCurSection == SecMasterKey) {
            if(Row.iIndex > sg_iMaxMasterKeyNum)
                continue;
            if(strcmp(Row.pszName, "inf") == 0) {
                strncpy(sg_aMasterKey[Row.iIndex].szInf, Row.pszContent, sizeof(sg_aMasterKey[Row.iIndex].szInf)-1);
                continue;
            }
            if(strcmp(Row.pszName, "key") == 0) {
                vTwoOne(Row.pszContent, 32, sg_aMasterKey[Row.iIndex].sKey);
                sg_aMasterKey[Row.iIndex].ucOKFlag = 1;
                continue;
            }
            continue;
        } else if(eCurSection == SecDesKey) {
            if(Row.iIndex > sg_iMaxDesKeyNum)
                continue;
            if(strcmp(Row.pszName, "inf") == 0) {
                strncpy(sg_aDesKey[Row.iIndex].szInf, Row.pszContent, sizeof(sg_aDesKey[Row.iIndex].szInf)-1);
                continue;
            }
            if(strcmp(Row.pszName, "key") == 0) {
                vTwoOne(Row.pszContent, 32, sg_aDesKey[Row.iIndex].sKey);
                sg_aDesKey[Row.iIndex].ucOKFlag = 1;
                continue;
            }
            continue;
        } else if(eCurSection == SecRsaKey) {
            if(Row.iIndex > sg_iMaxRsaKeyNum)
                continue;
            if(strcmp(Row.pszName, "inf") == 0) {
                strncpy(sg_aRsaKey[Row.iIndex].szInf, Row.pszContent, sizeof(sg_aRsaKey[Row.iIndex].szInf)-1);
                continue;
            }
            if(strcmp(Row.pszName, "pwd") == 0) {
                if(strlen(Row.pszContent) != 16)
                    continue;
                vTwoOne(Row.pszContent, 16, sg_aRsaKey[Row.iIndex].sPassword);
                continue;
            }
            if(strcmp(Row.pszName, "len") == 0) {
                if(atoi(Row.pszContent)<MIN_RSA_KEY_LEN || atoi(Row.pszContent)>MAX_RSA_KEY_LEN)
                    continue;
                if(atoi(Row.pszContent) % 2)
                    continue;
                sg_aRsaKey[Row.iIndex].iKeyLen = atoi(Row.pszContent);
                continue;
            }
            if(strcmp(Row.pszName, "e") == 0) {
                if(atol(Row.pszContent)!=3 && atol(Row.pszContent)!=65537L)
                    continue;
                sg_aRsaKey[Row.iIndex].lE = atol(Row.pszContent);
                continue;
            }
            if(strcmp(Row.pszName, "n") == 0) {
                if((int)strlen(Row.pszContent) != sg_aRsaKey[Row.iIndex].iKeyLen*2)
                    continue;
                vTwoOne(Row.pszContent, sg_aRsaKey[Row.iIndex].iKeyLen*2, sg_aRsaKey[Row.iIndex].sN);
                continue;
            }
            if(strcmp(Row.pszName, "d") == 0) {
                if((int)strlen(Row.pszContent) != sg_aRsaKey[Row.iIndex].iKeyLen*2)
                    continue;
                vTwoOne(Row.pszContent, sg_aRsaKey[Row.iIndex].iKeyLen*2, sg_aRsaKey[Row.iIndex].sD);
                continue;
            }
            if(strcmp(Row.pszName, "p") == 0) {
                if((int)strlen(Row.pszContent) != sg_aRsaKey[Row.iIndex].iKeyLen)
                    continue;
                vTwoOne(Row.pszContent, sg_aRsaKey[Row.iIndex].iKeyLen, sg_aRsaKey[Row.iIndex].sP);
                continue;
            }
            if(strcmp(Row.pszName, "q") == 0) {
                if((int)strlen(Row.pszContent) != sg_aRsaKey[Row.iIndex].iKeyLen)
                    continue;
                vTwoOne(Row.pszContent, sg_aRsaKey[Row.iIndex].iKeyLen, sg_aRsaKey[Row.iIndex].sQ);
                continue;
            }
            if(strcmp(Row.pszName, "dp") == 0) {
                if((int)strlen(Row.pszContent) != sg_aRsaKey[Row.iIndex].iKeyLen)
                    continue;
                vTwoOne(Row.pszContent, sg_aRsaKey[Row.iIndex].iKeyLen, sg_aRsaKey[Row.iIndex].sDP);
                continue;
            }
            if(strcmp(Row.pszName, "dq") == 0) {
                if((int)strlen(Row.pszContent) != sg_aRsaKey[Row.iIndex].iKeyLen)
                    continue;
                vTwoOne(Row.pszContent, sg_aRsaKey[Row.iIndex].iKeyLen, sg_aRsaKey[Row.iIndex].sDQ);
                continue;
            }
            if(strcmp(Row.pszName, "qinv") == 0) {
                if((int)strlen(Row.pszContent) != sg_aRsaKey[Row.iIndex].iKeyLen)
                    continue;
                vTwoOne(Row.pszContent, sg_aRsaKey[Row.iIndex].iKeyLen, sg_aRsaKey[Row.iIndex].sQINV);
                continue;
            }
            continue;
        } // else if(eCurSection == RsaKey) {
    } // for(;;

    fclose(fp);

    // ���RSA��Կ����Կ������
    memset(szKeyBuf, 0, sizeof(szKeyBuf)); // ���ڱȽϣ���RSA��Կ�Ƿ�Ϊȫ0
    for(i=0; i<sg_iMaxRsaKeyNum; i++) {
        sg_aRsaKey[i].ucOKFlag = 0x03;  // ��Կ���ñ�־��0:��Կ������ 1:Dֵ���� 2:CRT���� 3:Dֵ��CRT������
                                     // �ȼ���RSA��Կ��ȫ����
        if(sg_aRsaKey[i].iKeyLen==0 || sg_aRsaKey[i].lE==0) {
            sg_aRsaKey[i].ucOKFlag = 0; // �޳��Ȼ�Eֵ��������
            continue;
        }
        if(memcmp(sg_aRsaKey[i].sN, szKeyBuf, MAX_RSA_KEY_LEN) == 0) {
            sg_aRsaKey[i].ucOKFlag = 0; // ��Nֵ��������
            continue;
        }
        if(memcmp(sg_aRsaKey[i].sD, szKeyBuf, MAX_RSA_KEY_LEN) == 0) {
            sg_aRsaKey[i].ucOKFlag &= 0xFE; // ��Dֵ
            continue;
        }
        if(memcmp(sg_aRsaKey[i].sP, szKeyBuf, MAX_RSA_KEY_LEN/2)==0 ||
                memcmp(sg_aRsaKey[i].sQ, szKeyBuf, MAX_RSA_KEY_LEN/2)==0 ||
                memcmp(sg_aRsaKey[i].sDP, szKeyBuf, MAX_RSA_KEY_LEN/2)==0 ||
                memcmp(sg_aRsaKey[i].sDQ, szKeyBuf, MAX_RSA_KEY_LEN/2)==0 ||
                memcmp(sg_aRsaKey[i].sQINV, szKeyBuf, MAX_RSA_KEY_LEN/2)==0) {
            sg_aRsaKey[i].ucOKFlag &= 0xFD; // ��CRTֵ
            continue;
        }
    } // for(i=0; i<sg_iMaxRsaKeyNum; i++) {

    return(0);
}
#if 1
// ��������Կ(˫�������Ҳ��ֲ�ͬ,��У��)
// ret : 0 : OK
//       ����       : �ο�hsmsimu.h������
static int iKeySet(void)
{
    memset(sg_aMasterKey, 0, sizeof(sg_aMasterKey));
    memset(sg_aDesKey, 0, sizeof(sg_aDesKey));
    memset(sg_aRsaKey, 0, sizeof(sg_aRsaKey));

    strcpy(sg_aMasterKey[0].szInf, "LMK");
    vTwoOne("1334577991ABCDEF0102020404070708", 32, sg_aMasterKey[0].sKey);
    sg_aMasterKey[0].ucOKFlag = 1;
    strcpy(sg_aDesKey[1].szInf, "Kmc");
    vTwoOne("46454C5851524552524F52404C514C45", 32, sg_aDesKey[1].sKey);
    sg_aDesKey[1].ucOKFlag = 1;
    strcpy(sg_aDesKey[2].szInf, "Psk");
    vTwoOne("20202020202020203131313131313131", 32, sg_aDesKey[2].sKey);
    sg_aDesKey[2].ucOKFlag = 1;
    strcpy(sg_aDesKey[3].szInf, "Auth");
    vTwoOne("31313131313131314040404040404040", 32, sg_aDesKey[3].sKey);
    sg_aDesKey[3].ucOKFlag = 1;
    strcpy(sg_aDesKey[4].szInf, "Mac");
    vTwoOne("40404040404040405151515151515151", 32, sg_aDesKey[4].sKey);
    sg_aDesKey[4].ucOKFlag = 1;
    strcpy(sg_aDesKey[5].szInf, "Enc");
    vTwoOne("51515151515151516161616161616161", 32, sg_aDesKey[5].sKey);
    sg_aDesKey[5].ucOKFlag = 1;
    strcpy(sg_aDesKey[6].szInf, "Dcvv");
    vTwoOne("61616161616161617070707070707070", 32, sg_aDesKey[6].sKey);
    sg_aDesKey[6].ucOKFlag = 1;
    strcpy(sg_aDesKey[7].szInf, "New Kmc");
    vTwoOne("70707070707070708080808080808080", 32, sg_aDesKey[7].sKey);
    sg_aDesKey[7].ucOKFlag = 1;
    strcpy(sg_aDesKey[8].szInf, "Dp Mk");
    vTwoOne("80808080808080809191919191919191", 32, sg_aDesKey[8].sKey);
    sg_aDesKey[8].ucOKFlag = 1;
    strcpy(sg_aDesKey[9].szInf, "Dp Tk");
    vTwoOne("9191919191919191A1A1A1A1A1A1A1A1", 32, sg_aDesKey[9].sKey);
    sg_aDesKey[9].ucOKFlag = 1;

    strcpy(sg_aRsaKey[1].szInf, "Ca rsa key");
    memcpy(sg_aRsaKey[1].sPassword, "11111111", 8);
    sg_aRsaKey[1].iKeyLen = 192; // 0xC0
    sg_aRsaKey[1].lE = 3;
    vTwoOne("ABFBB295F61AFAD608DB342756E1ECDB30C98E3A50189F308FBAB903E3F8E7E574CCE93FE80D619732F32E81262AD084F24540718F28AFB60C367888A2706C9968539B5F673A0A0CED46E1AB2B9FEAAA5D30945E5CC1E5C5D19C3A6B26198750756A7F4BEEDCA4B9982103001E87C05679A5D47D2F35F7E50B064244C23311116034C56BCF0E40F82425A6ED4AC71E97E2AAF8F45F543117A9B470FC3FAD67BF4F3091349414FC7E88BADC9CC045F50C82C3BE5A4E16939810174CC495D8DC23", sg_aRsaKey[1].iKeyLen*2, sg_aRsaKey[1].sN);
    vTwoOne("72A7CC63F96751E405E7781A39EBF33CCB31097C3565BF75B527260297FB4543A333462A9AB39664CCA21F00C41C8B034C2E2AF65F7075240824505B16F59DBB9AE26794EF7C06B348D9EBC7726A9C719375B83EE88143D9366826F219665A348AE50319B69DD5E2FD40F808CFE77F480969B4557D8E119EC3F2E8174F2FF6A35778B60588BE96DA26036EC902A2BDDE9234C57D5844AD833E46885416E0AAFE4D15CA028F21CC6575DD54E5772613154EF97D8F908F0526ED6674AF6B78053B", sg_aRsaKey[1].iKeyLen*2, sg_aRsaKey[1].sD);
    vTwoOne("E3CDEB0BD114BC985210E5A596FE60D2CEE24A3434DC89F1035F938119D8BBCE4A5426E920B6CE91162737A41331B8549A3E80AC877431C74E06C5B28524957240F8A69CFF7229F635C0C51A9F46A2739467BD46587109E62F90E25CCF776165", sg_aRsaKey[1].iKeyLen, sg_aRsaKey[1].sP);
    vTwoOne("C1450F998BDB274CCA2EA94D4FAE20979CA4FBC8BE045385E1BA52A0B192634E12AB8D7A6139901FD4F9491BB3A149756D1D500BD378FB0B7E43DECB9837D1CF9A973B93BDF01FF0222E1829EE4635F8F7E5C4BC9CCF01F77C6CBB60A52D72E7", sg_aRsaKey[1].iKeyLen, sg_aRsaKey[1].sQ);
    vTwoOne("97DE9CB28B6328658C0B43C3B9FEEB373496DC22CDE85BF6023FB7AB66907D34318D6F4615CF3460B96F7A6D6221258DBC29AB1DAFA2CBDA340483CC58C30E4C2B506F1354F6C6A423D5D8BC6A2F16F7B84528D9904B5BEECA6096E88A4F9643", sg_aRsaKey[1].iKeyLen, sg_aRsaKey[1].sDP);
    vTwoOne("80D8B51107E76F8886C9C6338A7415BA686DFD307EAD8D03EBD18C6B210C42340C725E519626601538A630BD226B864E48BE355D37A5FCB2542D3F32657A8BDFBC64D2627EA0154AC174101BF42ECEA5FA992DD31334ABFA52F32795C373A1EF", sg_aRsaKey[1].iKeyLen, sg_aRsaKey[1].sDQ);
    vTwoOne("A6BD4DF401A23D38F876D35EA4B11CA366C7955FB6D0E21D3FCB74A6BAEA00E7A9748DC821D4C5739A1A02741A363D1740FE44D12E6E097334D4FD18B05DAFBBE422402FA778640452B1E2FDE027C89B415DBE2A3FF818C01BCC30F0E8EDAFCF", sg_aRsaKey[1].iKeyLen, sg_aRsaKey[1].sQINV);
    sg_aRsaKey[1].ucOKFlag = 1;
    strcpy(sg_aRsaKey[2].szInf, "Issuer rsa key");
    memcpy(sg_aRsaKey[2].sPassword, "11111111", 8);
    sg_aRsaKey[2].iKeyLen = 160; // 0xA0
    sg_aRsaKey[2].lE = 3;
    vTwoOne("DC049819455574FDC444D1A562F65AFA7D64EABB71517B3F9345123A0C6662EB280539E0576B7062C06386F56889448DAE793A65F1928E50F1B8389BE75658422D7BD249E7338A5A23F5F9B4454040CAD1A56AE60071C747608E7A3AF4B36544143CC381816DE69A6230EEEBA864A6F5DC3CE389068D17683D203A992D56E0CF6F4F14C801349B39A504B9AB8C2F7F24BD098BE214F384051E5726D84981416F", sg_aRsaKey[2].iKeyLen*2, sg_aRsaKey[2].sN);
    vTwoOne("92ADBABB838E4DFE82D88BC3974EE751A8EDF1D24B8BA77FB783617C084441F21AAE26958F9CF5972AED04A39B062DB3C9A626EEA10C5EE0A1257B129A39902C1E528C3144CD06E6C2A3FBCD83802B30A420D6ABCE59FFF4470C23DB3EB91759A6200DA6B23B3E56489BAA25294AA43F6E248E95A680EC619C0ED574F55A16839805E4EA9BFF5EEFAB801280BC5004695BDA599C1CDA9422B35B76F2572BF7BB", sg_aRsaKey[2].iKeyLen*2, sg_aRsaKey[2].sD);
    vTwoOne("FB7DE5823BF3A6A425450B652AA65139A417039920E0518B0315EC9EF9F28FE742870DDA79CF960B243F1730727C43FF0811CCABCD124F9B8D5768BD5A4C1D7411095C83EA22E57E7265DAA34BFAAB5B", sg_aRsaKey[2].iKeyLen, sg_aRsaKey[2].sP);
    vTwoOne("DFF643620EF720B4D0B7390CEBF77103F6F5AB6E5534B78DF2318314F08220AF747EFFCE12FC1ECAAECAE3394AD37B0B033470BC4A233D36966D352D176B5B12A238A8F3FF88C0529EE819C97AC4A27D", sg_aRsaKey[2].iKeyLen, sg_aRsaKey[2].sQ);
    vTwoOne("A7A943AC27F7C46D6E2E0798C7198B7BC2BA026615EAE1075763F314A6A1B544D704B3E6FBDFB95CC2D4BA204C52D7FF5AB6887288B6DFBD08E4F07E3C32BE4D60B0E857F16C98FEF6EE91C232A71CE7", sg_aRsaKey[2].iKeyLen, sg_aRsaKey[2].sDP);
    vTwoOne("954ED796B4A4C0788B24D0B347FA4B57F9F91CF438CDCFB3F6CBACB8A056C074F854AA8961FD69DC7487422631E2520757784B28316CD379B99E237364F23CB716D070A2AA5B2AE1BF45668651D86C53", sg_aRsaKey[2].iKeyLen, sg_aRsaKey[2].sDQ);
    vTwoOne("24DAD4C4C473507C6ABC9C68E4CE29EB758EFBE3EEFBA1AA0A787D4BBB742EAB76927995A37CC8ABB0C5D281C16668C83C612D4E51BAECE5147EF8D355B5C7165C554E26AD3163851A669966DEF1881B", sg_aRsaKey[2].iKeyLen, sg_aRsaKey[2].sQINV);
    sg_aRsaKey[2].ucOKFlag = 1;

    return(0);
}
#else
// ���þ���Կ
// ret : 0 : OK
//       ����       : �ο�hsmsimu.h������
static int iKeySet(void)
{
    memset(sg_aMasterKey, 0, sizeof(sg_aMasterKey));
    memset(sg_aDesKey, 0, sizeof(sg_aDesKey));
    memset(sg_aRsaKey, 0, sizeof(sg_aRsaKey));

    strcpy(sg_aMasterKey[0].szInf, "LMK");
    vTwoOne("1234567890ABCDEF0102030405060708", 32, sg_aMasterKey[0].sKey);
    sg_aMasterKey[0].ucOKFlag = 1;
    
    strcpy(sg_aDesKey[1].szInf, "Kmc");
    vTwoOne("47454D5850524553534F53414D504C45", 32, sg_aDesKey[1].sKey);
    sg_aDesKey[1].ucOKFlag = 1;
    strcpy(sg_aDesKey[2].szInf, "Psk");
    vTwoOne("22222222222222222222222222222222", 32, sg_aDesKey[2].sKey);
    sg_aDesKey[2].ucOKFlag = 1;
    strcpy(sg_aDesKey[3].szInf, "Auth");
    vTwoOne("33333333333333333333333333333333", 32, sg_aDesKey[3].sKey);
    sg_aDesKey[3].ucOKFlag = 1;
    strcpy(sg_aDesKey[4].szInf, "Mac");
    vTwoOne("44444444444444444444444444444444", 32, sg_aDesKey[4].sKey);
    sg_aDesKey[4].ucOKFlag = 1;
    strcpy(sg_aDesKey[5].szInf, "Enc");
    vTwoOne("55555555555555555555555555555555", 32, sg_aDesKey[5].sKey);
    sg_aDesKey[5].ucOKFlag = 1;
    strcpy(sg_aDesKey[6].szInf, "Dcvv");
    vTwoOne("66666666666666666666666666666666", 32, sg_aDesKey[6].sKey);
    sg_aDesKey[6].ucOKFlag = 1;
    strcpy(sg_aDesKey[7].szInf, "New Kmc");
    vTwoOne("77777777777777777777777777777777", 32, sg_aDesKey[7].sKey);
    sg_aDesKey[7].ucOKFlag = 1;
    strcpy(sg_aDesKey[8].szInf, "Dp Mk");
    vTwoOne("88888888888888888888888888888888", 32, sg_aDesKey[8].sKey);
    sg_aDesKey[8].ucOKFlag = 1;
    strcpy(sg_aDesKey[9].szInf, "Dp Tk");
    vTwoOne("99999999999999999999999999999999", 32, sg_aDesKey[9].sKey);
    sg_aDesKey[9].ucOKFlag = 1;

    strcpy(sg_aRsaKey[1].szInf, "Ca rsa key");
    memcpy(sg_aRsaKey[1].sPassword, "11111111", 8);
    sg_aRsaKey[1].iKeyLen = 192; // 0xC0
    sg_aRsaKey[1].lE = 3;
    vTwoOne("ABFBB295F61AFAD608DB342756E1ECDB30C98E3A50189F308FBAB903E3F8E7E574CCE93FE80D619732F32E81262AD084F24540718F28AFB60C367888A2706C9968539B5F673A0A0CED46E1AB2B9FEAAA5D30945E5CC1E5C5D19C3A6B26198750756A7F4BEEDCA4B9982103001E87C05679A5D47D2F35F7E50B064244C23311116034C56BCF0E40F82425A6ED4AC71E97E2AAF8F45F543117A9B470FC3FAD67BF4F3091349414FC7E88BADC9CC045F50C82C3BE5A4E16939810174CC495D8DC23", sg_aRsaKey[1].iKeyLen*2, sg_aRsaKey[1].sN);
    vTwoOne("72A7CC63F96751E405E7781A39EBF33CCB31097C3565BF75B527260297FB4543A333462A9AB39664CCA21F00C41C8B034C2E2AF65F7075240824505B16F59DBB9AE26794EF7C06B348D9EBC7726A9C719375B83EE88143D9366826F219665A348AE50319B69DD5E2FD40F808CFE77F480969B4557D8E119EC3F2E8174F2FF6A35778B60588BE96DA26036EC902A2BDDE9234C57D5844AD833E46885416E0AAFE4D15CA028F21CC6575DD54E5772613154EF97D8F908F0526ED6674AF6B78053B", sg_aRsaKey[1].iKeyLen*2, sg_aRsaKey[1].sD);
    vTwoOne("E3CDEB0BD114BC985210E5A596FE60D2CEE24A3434DC89F1035F938119D8BBCE4A5426E920B6CE91162737A41331B8549A3E80AC877431C74E06C5B28524957240F8A69CFF7229F635C0C51A9F46A2739467BD46587109E62F90E25CCF776165", sg_aRsaKey[1].iKeyLen, sg_aRsaKey[1].sP);
    vTwoOne("C1450F998BDB274CCA2EA94D4FAE20979CA4FBC8BE045385E1BA52A0B192634E12AB8D7A6139901FD4F9491BB3A149756D1D500BD378FB0B7E43DECB9837D1CF9A973B93BDF01FF0222E1829EE4635F8F7E5C4BC9CCF01F77C6CBB60A52D72E7", sg_aRsaKey[1].iKeyLen, sg_aRsaKey[1].sQ);
    vTwoOne("97DE9CB28B6328658C0B43C3B9FEEB373496DC22CDE85BF6023FB7AB66907D34318D6F4615CF3460B96F7A6D6221258DBC29AB1DAFA2CBDA340483CC58C30E4C2B506F1354F6C6A423D5D8BC6A2F16F7B84528D9904B5BEECA6096E88A4F9643", sg_aRsaKey[1].iKeyLen, sg_aRsaKey[1].sDP);
    vTwoOne("80D8B51107E76F8886C9C6338A7415BA686DFD307EAD8D03EBD18C6B210C42340C725E519626601538A630BD226B864E48BE355D37A5FCB2542D3F32657A8BDFBC64D2627EA0154AC174101BF42ECEA5FA992DD31334ABFA52F32795C373A1EF", sg_aRsaKey[1].iKeyLen, sg_aRsaKey[1].sDQ);
    vTwoOne("A6BD4DF401A23D38F876D35EA4B11CA366C7955FB6D0E21D3FCB74A6BAEA00E7A9748DC821D4C5739A1A02741A363D1740FE44D12E6E097334D4FD18B05DAFBBE422402FA778640452B1E2FDE027C89B415DBE2A3FF818C01BCC30F0E8EDAFCF", sg_aRsaKey[1].iKeyLen, sg_aRsaKey[1].sQINV);
    sg_aRsaKey[1].ucOKFlag = 1;
    strcpy(sg_aRsaKey[2].szInf, "Issuer rsa key");
    memcpy(sg_aRsaKey[2].sPassword, "11111111", 8);
    sg_aRsaKey[2].iKeyLen = 160; // 0xA0
    sg_aRsaKey[2].lE = 3;
    vTwoOne("DC049819455574FDC444D1A562F65AFA7D64EABB71517B3F9345123A0C6662EB280539E0576B7062C06386F56889448DAE793A65F1928E50F1B8389BE75658422D7BD249E7338A5A23F5F9B4454040CAD1A56AE60071C747608E7A3AF4B36544143CC381816DE69A6230EEEBA864A6F5DC3CE389068D17683D203A992D56E0CF6F4F14C801349B39A504B9AB8C2F7F24BD098BE214F384051E5726D84981416F", sg_aRsaKey[2].iKeyLen*2, sg_aRsaKey[2].sN);
    vTwoOne("92ADBABB838E4DFE82D88BC3974EE751A8EDF1D24B8BA77FB783617C084441F21AAE26958F9CF5972AED04A39B062DB3C9A626EEA10C5EE0A1257B129A39902C1E528C3144CD06E6C2A3FBCD83802B30A420D6ABCE59FFF4470C23DB3EB91759A6200DA6B23B3E56489BAA25294AA43F6E248E95A680EC619C0ED574F55A16839805E4EA9BFF5EEFAB801280BC5004695BDA599C1CDA9422B35B76F2572BF7BB", sg_aRsaKey[2].iKeyLen*2, sg_aRsaKey[2].sD);
    vTwoOne("FB7DE5823BF3A6A425450B652AA65139A417039920E0518B0315EC9EF9F28FE742870DDA79CF960B243F1730727C43FF0811CCABCD124F9B8D5768BD5A4C1D7411095C83EA22E57E7265DAA34BFAAB5B", sg_aRsaKey[2].iKeyLen, sg_aRsaKey[2].sP);
    vTwoOne("DFF643620EF720B4D0B7390CEBF77103F6F5AB6E5534B78DF2318314F08220AF747EFFCE12FC1ECAAECAE3394AD37B0B033470BC4A233D36966D352D176B5B12A238A8F3FF88C0529EE819C97AC4A27D", sg_aRsaKey[2].iKeyLen, sg_aRsaKey[2].sQ);
    vTwoOne("A7A943AC27F7C46D6E2E0798C7198B7BC2BA026615EAE1075763F314A6A1B544D704B3E6FBDFB95CC2D4BA204C52D7FF5AB6887288B6DFBD08E4F07E3C32BE4D60B0E857F16C98FEF6EE91C232A71CE7", sg_aRsaKey[2].iKeyLen, sg_aRsaKey[2].sDP);
    vTwoOne("954ED796B4A4C0788B24D0B347FA4B57F9F91CF438CDCFB3F6CBACB8A056C074F854AA8961FD69DC7487422631E2520757784B28316CD379B99E237364F23CB716D070A2AA5B2AE1BF45668651D86C53", sg_aRsaKey[2].iKeyLen, sg_aRsaKey[2].sDQ);
    vTwoOne("24DAD4C4C473507C6ABC9C68E4CE29EB758EFBE3EEFBA1AA0A787D4BBB742EAB76927995A37CC8ABB0C5D281C16668C83C612D4E51BAECE5147EF8D355B5C7165C554E26AD3163851A669966DEF1881B", sg_aRsaKey[2].iKeyLen, sg_aRsaKey[2].sQINV);
    sg_aRsaKey[2].ucOKFlag = 1;

    return(0);
}
#endif

// �洢��Կ���ļ�
// ret : 0 : OK
//       ����       : �ο�hsmsimu.h������
static int iKeyFileSave(void)
{
    FILE *fp;
    char szKeyBuf[1024];
    int  i;
    
    if(sg_szKeyFileName[0] == 0)
        return(0);
    
    fp = fopen(sg_szKeyFileName, "wt");
    if(fp == NULL) {
        return(HSMERR_INTERNAL); // ���ļ�ʧ��
    }

    // д˵������
    fprintf(fp, "# ���ļ�Ϊ������ʿͨ���ܻ���Կ�ļ�\n");
    fprintf(fp, "# ��Կ������������[master key]����Ϊ����Կ������[des key]����ΪӦ����Կ������[rsa key]����ΪRSA��Կ��\n");
    fprintf(fp, "# ��������3λʮ��������ʾ������Կ000-002��Ӧ����ԿΪ000-255��rsa��ԿΪ000-019\n");
    fprintf(fp, "# ����Կ����0-2�ֱ��Ӧ��ʿͨ���ܻ�LMK������Կһ������Կ��\n");
    fprintf(fp, "# index***_infΪ��Կ˵����˵�������������40�ֽ�\n");
    fprintf(fp, "# ����Կ��Ӧ����Կ�̶�Ϊ˫������Կ\n");
    fprintf(fp, "# rsa��Կ�е�index***_len��ʮ�����ֽ�����ʾ��ȡֵ64-256\n");
    fprintf(fp, "# rsa��Կ�е�index***_e��ʮ���Ʊ�ʾ��ȡֵ3��65537\n");
    fprintf(fp, "\n");

    // дmaster key
    fprintf(fp, "[master key]\n");
    for(i=0; i<sg_iMaxMasterKeyNum; i++) {
        if(sg_aMasterKey[i].ucOKFlag == 0)
            continue;
        if(strlen(sg_aMasterKey[i].szInf))
            fprintf(fp, "index%03d_inf  = %s\n", i, sg_aMasterKey[i].szInf);
        vOneTwo0(sg_aMasterKey[i].sKey, 16, szKeyBuf);
        fprintf(fp, "index%03d_key  = %s\n", i, szKeyBuf);
        fprintf(fp, "\n");
    }
    fprintf(fp, "\n");

    // дdes key
    fprintf(fp, "[des key]\n");
    for(i=0; i<sg_iMaxDesKeyNum; i++) {
        if(sg_aDesKey[i].ucOKFlag == 0)
            continue;
        if(strlen(sg_aDesKey[i].szInf))
            fprintf(fp, "index%03d_inf  = %s\n", i, sg_aDesKey[i].szInf);
        vOneTwo0(sg_aDesKey[i].sKey, 16, szKeyBuf);
        fprintf(fp, "index%03d_key  = %s\n", i, szKeyBuf);
        fprintf(fp, "\n");
    }
    fprintf(fp, "\n");

    // дrsa key
    fprintf(fp, "[rsa key]\n");
    for(i=0; i<sg_iMaxRsaKeyNum; i++) {
        if(sg_aRsaKey[i].ucOKFlag == 0)
            continue;
        if(strlen(sg_aRsaKey[i].szInf))
            fprintf(fp, "index%03d_inf  = %s\n", i, sg_aRsaKey[i].szInf);
        vOneTwo0(sg_aRsaKey[i].sPassword, 8, szKeyBuf);
        fprintf(fp, "index%03d_pwd  = %s\n", i, szKeyBuf);
        fprintf(fp, "index%03d_len  = %d\n", i, sg_aRsaKey[i].iKeyLen);
        fprintf(fp, "index%03d_e    = %ld\n", i, sg_aRsaKey[i].lE);
        vOneTwo0(sg_aRsaKey[i].sN, sg_aRsaKey[i].iKeyLen, szKeyBuf);
        fprintf(fp, "index%03d_n    = %s\n", i, szKeyBuf);

        if(sg_aRsaKey[i].ucOKFlag & 0x01) {
            // D����
            vOneTwo0(sg_aRsaKey[i].sD, sg_aRsaKey[i].iKeyLen, szKeyBuf);
            fprintf(fp, "index%03d_d    = %s\n", i, szKeyBuf);
        }
        if(sg_aRsaKey[i].ucOKFlag & 0x02) {
            // CRT����
            vOneTwo0(sg_aRsaKey[i].sP, sg_aRsaKey[i].iKeyLen/2, szKeyBuf);
            fprintf(fp, "index%03d_p    = %s\n", i, szKeyBuf);
            vOneTwo0(sg_aRsaKey[i].sQ, sg_aRsaKey[i].iKeyLen/2, szKeyBuf);
            fprintf(fp, "index%03d_q    = %s\n", i, szKeyBuf);
            vOneTwo0(sg_aRsaKey[i].sDP, sg_aRsaKey[i].iKeyLen/2, szKeyBuf);
            fprintf(fp, "index%03d_dp   = %s\n", i, szKeyBuf);
            vOneTwo0(sg_aRsaKey[i].sDQ, sg_aRsaKey[i].iKeyLen/2, szKeyBuf);
            fprintf(fp, "index%03d_dq   = %s\n", i, szKeyBuf);
            vOneTwo0(sg_aRsaKey[i].sQINV, sg_aRsaKey[i].iKeyLen/2, szKeyBuf);
            fprintf(fp, "index%03d_qinv = %s\n", i, szKeyBuf);
        }
        fprintf(fp, "\n");
    }

    fclose(fp);

    return(0);
}

// ��������ɢ��Կ
// in  : iKeyIndex  : ��Կ����
//       pKey       : LMK��������Կ, �������NULL, ��ʾ�ô���Կ
//       ucDivLevel : ��ɢ���� 0-3
//       pDivData   : ��ɢ����, ����=ucDivLevel*8
// out : pDivKey    : ��ɢ����Կ
// ret : 0          : OK
//       ����       : �ο�hsmsimu.h������
static int iDivKey(int iKeyIndex, void *pKey, uchar ucDivLevel, void *pDivData, void *pDivKey)
{
	int   i;
	uchar *psKey, *psDivData, *psDivKey;
	
	psKey = (uchar *)pKey;
	psDivData = (uchar *)pDivData;
	psDivKey = (uchar *)pDivKey;
	
	if(psKey == NULL) {
    	if(iKeyIndex<0 || iKeyIndex>sg_iMaxDesKeyNum)
	        return(HSMERR_INDEX);
	    if(sg_aDesKey[iKeyIndex].ucOKFlag == 0) {
	        return(HSMERR_NO_DESKEY);
	    }
	}
	if(ucDivLevel<0 || ucDivLevel>3)
	    return(HSMERR_DIV_NUM);

	// ��ɢ
	if(psKey)
        vDesEcb(TRI_DECRYPT, 16, psKey, sg_aMasterKey[sg_iMasterKeyIndex].sKey, psDivKey);
	else
	    memcpy(psDivKey, sg_aDesKey[iKeyIndex].sKey, 16);
    
	if(ucDivLevel == 0)
	    return(0);
	for(i=0; i<ucDivLevel; i++) {
		uchar sKey[16], sDivData[8];
		memcpy(sKey, psDivKey, 16);
		memcpy(sDivData, psDivData+i*8, 8);
		_vDes(TRI_ENCRYPT, sDivData, sKey, psDivKey);
		vXor(sDivData, "\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF", 8);
		_vDes(TRI_ENCRYPT, sDivData, sKey, psDivKey+8);
	}
    vReformKey(1, psDivKey); // ����Կ����Ϊ��У��
	return(0);
}
// ��ɢ��Կ
// in  : pKey       : ��Կ
//       ucDivLevel : ��ɢ���� 0-3
//       pDivData   : ��ɢ����, ����=ucDivLevel*8
// out : pDivKey    : ��ɢ����Կ
// ret : 0          : OK
//       ����       : �ο�hsmsimu.h������
static int iDivKey2(void *pKey, uchar ucDivLevel, void *pDivData, void *pDivKey)
{
	int   i;
	uchar *psKey, *psDivData, *psDivKey;
	
	psKey = (uchar *)pKey;
	psDivData = (uchar *)pDivData;
	psDivKey = (uchar *)pDivKey;
	
	if(ucDivLevel<0 || ucDivLevel>3)
	    return(HSMERR_DIV_NUM);
    memcpy(psDivKey, pKey, 16);
	for(i=0; i<ucDivLevel; i++) {
		uchar sKey[16], sDivData[8];
		memcpy(sKey, psDivKey, 16);
		memcpy(sDivData, psDivData+i*8, 8);
		_vDes(TRI_ENCRYPT, sDivData, sKey, psDivKey);
		vXor(sDivData, "\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF", 8);
		_vDes(TRI_ENCRYPT, sDivData, sKey, psDivKey+8);
	}
    vReformKey(1, psDivKey); // ����Կ����Ϊ��У��
	return(0);
}

// ��Ƭ��ɢ��Կ, ����KMC��Կ����
// div keys(GP2.02�淶)
// in  : ucKeyType    : ��ɢ���� 1:S-AUTH(�ⲿ��֤) 2:C-MAC(Command APDU Mac) 3:R-MAC(Response APDU Mac) 4:ENC(Sensitive Data Encryption)
//       pCardData    : ��ƬInitializeָ���ֵ(KeyDivData[10]+KeyInfoData[2]+CardRand[8]+CardCrypto[8])
//       pMKey        : ����Կ [16]
// out : pKey         : ��Կ���� [16]
// ret : 0            : OK
//       ����       : �ο�hsmsimu.h������
static int iGpDivKey(uchar ucKeyType, void *pCardData, void *pMKey, void *pKey)
{
	uchar sDivData[16];
	uchar *psCardData, *psMKey, *psKey;
	
	psCardData = (uchar *)pCardData;
	psMKey = (uchar *)pMKey;
	psKey = (uchar *)pKey;

	if(sg_iGpDivMethod == 202) {
		// GCP202
		memset(sDivData, 0, 16);
		memcpy(sDivData+2, psCardData+4, 4); //Chip Id (CSN) 4 bytes + XX 2 bytes
		memcpy(sDivData+8+2, psCardData+4, 4);  // repeat
	} else {
		// GCP212, ����202����Ϊ��212
		memcpy(sDivData, psCardData+4, 6); //Chip Id (CSN) 4 bytes + XX 2 bytes
		memcpy(sDivData+8, sDivData, 6);  // repeat
	}

	sDivData[6] = 0xf0;
	sDivData[14] = 0x0f;
	switch(ucKeyType) {
	case 1: // 1:S-AUTH(�ⲿ��֤)
   		sDivData[7] = sDivData[15] = 1;
		break;
	case 2: // 2:C-MAC(Command APDU Mac)
	case 3: // 3:R-MAC(Response APDU Mac)
   		sDivData[7] = sDivData[15] = 2;
		break;
	case 4: // 4:ENC(Sensitive Data Encryption)
   		sDivData[7] = sDivData[15] = 3;
		break;
	default:
		return(HSMERR_INTERNAL);
	}
	_vDes(TRI_ENCRYPT, sDivData, psMKey, psKey);
	_vDes(TRI_ENCRYPT, sDivData+8, psMKey, psKey+8);
    vReformKey(1, psKey); // ����Կ����Ϊ��У��
	return(0);
}

// ��Ƭ��ɢ��Կ, ����������Կ
// in  : ucKeyType    : ��ɢ���� 1:S-AUTH(�ⲿ��֤) 2:C-MAC(Command APDU Mac) 3:R-MAC(Response APDU Mac) 4:ENC(Sensitive Data Encryption)
//       ucDivMode    : ��ɢ��־, 0:����ɢ  1:��ɢ
//       pTermRand    : �ն������ [8]
//       pCardData    : ��ƬInitializeָ���ֵ(KeyDivData[10]+KeyInfoData[2]+CardRand[8]+CardCrypto[8])
//       pMKey        : ����Կ [16]
// out : pSessionKey  : ������Կ [16]
// ret : 0            : OK
//       ����       : �ο�hsmsimu.h������
static int iGpDivChannelKey(uchar ucKeyType, uchar ucDivMode, void *pTermRand, void *pCardData,
   			         void *pMKey, void *pSessionKey)
{
	uchar ucSCPFlag;
	uchar sCardRand[8];
	uchar sDivKey[16+1];
	uchar sRand[8+1];
	int   iRet;
	uchar *psTermRand, *psCardData, *psMKey, *psSessionKey;

    psTermRand = (uchar *)pTermRand;
	psCardData = (uchar *)pCardData;
	psMKey = (uchar *)pMKey;
	psSessionKey = (uchar *)pSessionKey;

	ucSCPFlag = psCardData[11];
	memcpy(sCardRand, psCardData+10+2, 8);
	// Div Key first
	if( ucDivMode )   //need div
	{
	    iRet = iGpDivKey(ucKeyType, psCardData, psMKey, sDivKey);
	    if(iRet)
	        return(iRet);
	}else
		memcpy( sDivKey, psMKey, 16 );

	// cal session key
	if(ucSCPFlag == 1) {
		// SCP01
		if(ucDivMode == 0) {
			// Div ModeΪ0, ������ͬһ�㷨���������Կ
        	memcpy( sRand, sCardRand+4, 4);
			memcpy( sRand+4, psTermRand, 4);
			_vDes(TRI_ENCRYPT, sRand, sDivKey, psSessionKey);
			memcpy( sRand, sCardRand, 4);
			memcpy( sRand+4, psTermRand+4, 4);
			_vDes(TRI_ENCRYPT, sRand, sDivKey, psSessionKey+8);
		} else {
			// Div ModeΪ1, �ֱ���������Կ
			if(ucKeyType == 4) {
				// cal KEK session key
				// with SCP01, KEK session key is equal to KEK
				memcpy(psSessionKey, sDivKey, 16);
			} else {
				// cal keys other than KEK session key
				memcpy( sRand, sCardRand+4, 4);
				memcpy( sRand+4, psTermRand, 4);
				_vDes(TRI_ENCRYPT, sRand, sDivKey, psSessionKey);
				memcpy( sRand, sCardRand, 4);
				memcpy( sRand+4, psTermRand+4, 4);
				_vDes(TRI_ENCRYPT, sRand, sDivKey, psSessionKey+8);
			}
		}
	} else {
		// SCP02
		switch(ucKeyType) {
		case 1: // 1:S-AUTH(�ⲿ��֤)
			memset(sRand, 0, 8);
			memcpy(sRand, "\x01\x82", 2);
			memcpy(sRand+2, sCardRand, 2); // Sequence Counter
			break;
		case 2: // 2:C-MAC(Command APDU Mac)
			memset(sRand, 0, 8);
			memcpy(sRand, "\x01\x01", 2);
			memcpy(sRand+2, sCardRand, 2); // Sequence Counter
			break;
		case 3: // 3:R-MAC(Response APDU Mac)
			memset(sRand, 0, 8);
			memcpy(sRand, "\x01\x02", 2);
			memcpy(sRand+2, sCardRand, 2); // Sequence Counter
			break;
		case 4: // 4:ENC(Sensitive Data Encryption)
			memset(sRand, 0, 8);
			memcpy(sRand, "\x01\x81", 2);
			memcpy(sRand+2, sCardRand, 2); // Sequence Counter
			break;
		default:
			return(HSMERR_INTERNAL);
		}
		_vDes(TRI_ENCRYPT, sRand, sDivKey, psSessionKey);
		_vDes(TRI_ENCRYPT, psSessionKey, sDivKey, psSessionKey+8);
	}
    vReformKey(1, psSessionKey); // ����Կ����Ϊ��У��
	return(0);
}

// ����Mac, Single DES plus final Triple Des
// in  : pKey     : ��Կ
//       pMsg     : ��Ϣ
//       ucLength : ��Ϣ����
// out : pMac     : Mac [8]
// ret : 0        : OK
//       ����     : �ο�hsmsimu.h������
static int iMac3DesCBCRetail(void *pKey, void *pMsg, uchar ucLength, void *pMac)
{
    uchar ucBlock;
    uchar sOutMAC[8], sBuf[8];
    uchar *psKey, *psMsg, *psMac;
    
    psKey = (uchar *)pKey;
    psMsg = (uchar *)pMsg;
    psMac = (uchar *)pMac;

    memcpy(sOutMAC, (char *)psMac, 8);
    ucBlock=0;
    while(ucLength > ucBlock) {
        if((ucLength - ucBlock) <= 8) {
            if((ucLength - ucBlock) == 8) {
                vXor(sOutMAC, &psMsg[ucBlock], (ushort)(ucLength-ucBlock));
                _vDes(ENCRYPT, sOutMAC, psKey, sOutMAC);
                memcpy(sBuf, "\x80\x0\x0\x0\x0\x0\x0\x0", 8);
                vXor(sOutMAC, sBuf, 8);
                _vDes(ENCRYPT, sOutMAC, psKey, sOutMAC);
                    _vDes(DECRYPT, sOutMAC, &psKey[8], sOutMAC);
                    _vDes(ENCRYPT, sOutMAC, psKey, sOutMAC);
                memcpy((char *)psMac, sOutMAC, 8);
                return(0);
            } else {
                memset(sBuf, 0, sizeof(sBuf));
                memcpy(sBuf, &psMsg[ucBlock], (ucLength-ucBlock));
                sBuf[ucLength-ucBlock] = 0x80;
                vXor(sOutMAC, sBuf, 8);
                _vDes(ENCRYPT, sOutMAC, psKey, sOutMAC);
                    _vDes(DECRYPT, sOutMAC, &psKey[8], sOutMAC);
                    _vDes(ENCRYPT, sOutMAC, psKey, sOutMAC);
                memcpy((char *)psMac, sOutMAC, 8);
                return(0);
            }
        }
        vXor(sOutMAC, &psMsg[ucBlock], 8);
        _vDes(ENCRYPT, sOutMAC, psKey, sOutMAC);
        ucBlock += 8;
    }
	return(0);
}

// ����Mac, Full Triple Des
// in  : pKey     : ��Կ
//       pMsg     : ��Ϣ
//       ucLength : ��Ϣ����
// out : pMac     : Mac [8]
// ret : 0        : OK
//       ����     : �ο�hsmsimu.h������
static int iMac3DesCBC(void *pKey, void *pMsg, uchar ucLength, void *pMac)
{
    uchar ucBlock;
    uchar sOutMAC[8], sBuf[8];
    uchar *psKey, *psMsg, *psMac;
    
    psKey = (uchar *)pKey;
    psMsg = (uchar *)pMsg;
    psMac = (uchar *)pMac;

	//ICV in psMac
    memcpy(sOutMAC, (char *)psMac, 8);
    ucBlock=0;
    while(ucLength > ucBlock) {
        if((ucLength - ucBlock) <= 8) {
            if((ucLength - ucBlock) == 8) {
                vXor(sOutMAC, &psMsg[ucBlock], (ushort)(ucLength-ucBlock));
                _vDes(TRI_ENCRYPT, sOutMAC, psKey, sOutMAC);
                memcpy(sBuf, "\x80\x0\x0\x0\x0\x0\x0\x0", 8);
                vXor(sOutMAC, sBuf, 8);
                _vDes(TRI_ENCRYPT, sOutMAC, psKey, sOutMAC);
                memcpy((char *)psMac, sOutMAC, 8);
                return(0);
            } else {
                memset(sBuf, 0, sizeof(sBuf));
                memcpy(sBuf, &psMsg[ucBlock], (ucLength-ucBlock));
                sBuf[ucLength-ucBlock] = 0x80;
                vXor(sOutMAC, sBuf, 8);
                _vDes(TRI_ENCRYPT, sOutMAC, psKey, sOutMAC);
                memcpy((char *)psMac, sOutMAC, 8);
                return(0);
            }
        }
        vXor(sOutMAC, &psMsg[ucBlock], 8);
        _vDes(TRI_ENCRYPT, sOutMAC, psKey, sOutMAC);
        ucBlock += 8;
    }
	return(0);
}

// ���ܻ���ʼ��
// in  : pszKeyFileName : ��Կ�ļ�����, NULL��ʾʹ��ģ���ڲ���Կ
// ret : 0 : OK
//       ����       : �ο�hsmsimu.h������
int iHsmInit0(char *pszKeyFileName)
{
    int iRet;

#ifdef USE_KEY_POOL
    iRet = iRsaKeyPoolInit(1);
#endif
    if(pszKeyFileName)
        strcpy(sg_szKeyFileName, pszKeyFileName);
    else
        sg_szKeyFileName[0] = 0;
    sg_iMaxMasterKeyNum = sizeof(sg_aMasterKey) / sizeof(sg_aMasterKey[0]);
    sg_iMaxDesKeyNum = sizeof(sg_aDesKey) / sizeof(sg_aDesKey[0]);
    sg_iMaxRsaKeyNum = sizeof(sg_aRsaKey) / sizeof(sg_aRsaKey[0]);
    memset(sg_aMasterKey, 0, sizeof(sg_aMasterKey));
    memset(sg_aDesKey, 0, sizeof(sg_aDesKey));
    memset(sg_aRsaKey, 0, sizeof(sg_aRsaKey));
    srand(time(0));
    sg_iMasterKeyIndex = 0; // 0:LMK

    if(pszKeyFileName)
        iRet = iKeyFileLoad();
    else
        iRet = iKeySet();
    if(iRet)
        return(iRet);
    return(0);
}

// ����RSA��Կ
// in  : pRsaKey  : RSA��Կ
// out : piOutLen : ���ܺ���Կ����
//       pOutData : ���ܺ���Կ
// ret : 0        : OK
// Note: �ȴ����DER��ʽ, Ȼ��������
static int iPackRsaKey(stRsaKey *pRsaKey, int *piOutLen, void *pOutData)
{
    uchar sDerBuf[1280];
    int  i;
    char *p;
    int  iDerLen;
    int  iPatchFlag;
    int  iLen;
    // DER��ʽ, ��0x80, �ٲ���8�ı���, LMK��ECBģʽ����
    // DER�������
    //     ����ͬBER�������
    //     RSAPublicKey ::= SEQUENCE {
    //                          modulus INTEGER,
    //                          publicExponent INTEGER 
    //                      }
    //     RSAPrivateKey::= SEQUENCE {
    //                          version Version,
    //                          modulus INTEGER,
    //                          publicExponent INTEGER,
    //                          privateExponent INTEGER,
    //                          prime1 INTEGER,
    //                          prime2 INTEGER,
    //                          exponent1 INTEGER,
    //                          exponent2 INTEGER,
    //                          coefficient INTEGER
    //                      }
    //     SEQUENCE��TagΪ0x30�������������ݵ�Tag��Ϊ0x02
    //     ע�⣬����INTEGER��˵�����λ��ʾ����λ�������ڱ�ʾ���λΪ1��ģ���ȴ�����ʱ��
    //           Ҫ��ǰ�油һ���ֽڵ�0x00����˵������Ϊ������TLV�ĳ�����ҲҪ��ӳ��һ�ֽ�
    // DER���
    iDerLen = 0;
    for(i=0; i<9; i++) {
        switch(i) {
        case 0: // Version
            memcpy(sDerBuf+iDerLen, "\x02\x01\x00", 3);
            iDerLen += 3;
            continue;
        case 1: // N
            iLen = pRsaKey->iKeyLen;
            p = pRsaKey->sN;
            break;
        case 2: // E
            if(pRsaKey->lE == 3) {
                memcpy(sDerBuf+iDerLen, "\x02\x01\x03", 3);
                iDerLen += 3;
            } else {
                memcpy(sDerBuf+iDerLen, "\x02\x03\x01\x00\x01", 5);
                iDerLen += 5;
            }
            continue;
        case 3: // D
            iLen = pRsaKey->iKeyLen;
            p = pRsaKey->sD;
            break;
        case 4: // P
            iLen = pRsaKey->iKeyLen / 2;
            p = pRsaKey->sP;
            break;
        case 5: // Q
            iLen = pRsaKey->iKeyLen / 2;
            p = pRsaKey->sQ;
            break;
        case 6: // dP
            iLen = pRsaKey->iKeyLen / 2;
            p = pRsaKey->sDP;
            break;
        case 7: // dQ
            iLen = pRsaKey->iKeyLen / 2;
            p = pRsaKey->sDQ;
            break;
        case 8: // qInv
            iLen = pRsaKey->iKeyLen / 2;
            p = pRsaKey->sQINV;
            break;
        } // switch(i)
        if(p[0] & 0x80) {
            iPatchFlag = 1; // ���ڻ����0x80, ��Ҫǰ��0
            iLen ++;
        } else
            iPatchFlag = 0;
        sDerBuf[iDerLen++] = 0x02; // tag
        if(iLen < 128) {
            sDerBuf[iDerLen++] = iLen;
        } else if(iLen < 256) {
            sDerBuf[iDerLen++] = 0x81;
            sDerBuf[iDerLen++] = iLen;
        } else {
            sDerBuf[iDerLen++] = 0x82;
            sDerBuf[iDerLen++] = iLen / 256;
            sDerBuf[iDerLen++] = iLen % 256;
        }
        if(iPatchFlag) {
            sDerBuf[iDerLen] = 0;
            memcpy(sDerBuf+iDerLen+1, p, iLen-1);
        } else
            memcpy(sDerBuf+iDerLen, p, iLen);
        iDerLen += iLen;
    } // for(;;
    
    // ���Tag30
    if(iDerLen < 128) {
        memmove(sDerBuf+2, sDerBuf, iDerLen);
        sDerBuf[0] = 0x30;
        sDerBuf[1] = iDerLen;
        iDerLen += 2;
    } else if(iDerLen < 256) {
        memmove(sDerBuf+3, sDerBuf, iDerLen);
        sDerBuf[0] = 0x30;
        sDerBuf[1] = 0x81;
        sDerBuf[2] = iDerLen;
        iDerLen += 3;
    } else {
        memmove(sDerBuf+4, sDerBuf, iDerLen);
        sDerBuf[0] = 0x30;
        sDerBuf[1] = 0x82;
        sDerBuf[2] = iDerLen / 256;
        sDerBuf[3] = iDerLen % 256;
        iDerLen += 4;
    }
    // DER�������, sDerBuf, iDerLen
    
    // ��0x80
    memcpy(sDerBuf+iDerLen, "\x80\x00\x00\x00\x00\x00\x00\x00", 8);
    iDerLen = (iDerLen+8) / 8 * 8;

    // �������
    // deskey, EncryptedKey[16]+cks[8]
    vDesEcb(TRI_ENCRYPT, iDerLen, sDerBuf, sg_aMasterKey[sg_iMasterKeyIndex].sKey, pOutData);
    *piOutLen = iDerLen;
    return(0);
}

// ��LMK�ӽ�����Կ
// in  : iFlag : �ӽ��ܱ�־ 0:���� 1:����
//       pIn   : Դ����[16]
// out : pOut  : Ŀ������[16]
// �ṩ���߲�����ʵ��ĳЩ����ָ��
int iLmkProcess0(int iFlag, void *pIn, void *pOut)
{
    if(sg_aMasterKey[sg_iMasterKeyIndex].ucOKFlag == 0)
        return(HSMERR_NO_MASTERKEY);
    if(iFlag)
        vDesEcb(TRI_ENCRYPT, 16, pIn, sg_aMasterKey[sg_iMasterKeyIndex].sKey, pOut);
    else
        vDesEcb(TRI_DECRYPT, 16, pIn, sg_aMasterKey[sg_iMasterKeyIndex].sKey, pOut);
    return(0);
}

// ��Կ����
// in  : iKeyType  : 1:DES��Կ 2:RSA��Կ
//       iKeyIndex : ��Կ����
// out : piOutLen  : �������ݳ���
//       pOutData  : ��������, DES:LMK��ECBģʽ����[16]+cks[8]
//                             RSA:DER��ʽ, ��0x80, �ٲ���8�ı���, LMK��ECBģʽ����
// ret : 0         : �ɹ�
//       ����      : �ο�hsmsimu.h������
int iKeyBackup0(int iKeyType, int iKeyIndex, int *piOutLen, void *pOutData)
{
    char sBuf[8];
    int  iRet;
    
    if(iKeyType!=1 && iKeyType!=2)
        return(HSMERR_TYPE);
    if(iKeyType==1 && (iKeyIndex<0 || iKeyIndex>sg_iMaxDesKeyNum))
        return(HSMERR_INDEX);
    if(iKeyType==2 && (iKeyIndex<0 || iKeyIndex>sg_iMaxRsaKeyNum))
        return(HSMERR_INDEX);
    if(sg_aMasterKey[sg_iMasterKeyIndex].ucOKFlag == 0)
        return(HSMERR_NO_MASTERKEY);
    if(iKeyType==1 && sg_aDesKey[iKeyIndex].ucOKFlag==0)
        return(HSMERR_NO_DESKEY);
    if(iKeyType==2 && sg_aRsaKey[iKeyIndex].ucOKFlag==0)
        return(HSMERR_NO_RSAKEY);

    if(iKeyType == 1) {
        // deskey, EncryptedKey[16]+cks[8]
        vDesEcb(TRI_ENCRYPT, 16, sg_aDesKey[iKeyIndex].sKey, sg_aMasterKey[sg_iMasterKeyIndex].sKey, pOutData);
        memset(sBuf, 0, 8);
        _vDes(TRI_ENCRYPT, sBuf, sg_aDesKey[iKeyIndex].sKey, sBuf);
        memcpy((char *)pOutData+16, sBuf, 8); // cks
        *piOutLen = 16+8;
    } else {
        if(sg_aRsaKey[iKeyIndex].iKeyLen % 2) {
            return(HSMERR_INTERNAL);
        }
        iRet = iPackRsaKey(&sg_aRsaKey[iKeyIndex], piOutLen, pOutData);
    }
    return(0);
}

// ���RSA��Կ
// in  : iInLen  : ���ܺ�DER��ʽRSA��Կ����
//       pInData : ���ܺ�DER��ʽRSA��Կ
// out : pRsaKey : �������Կ
// ret : 0       : OK
//       1       : ��������
static int iExtractRsaKey(int iInLen, void *pInData, stRsaKey *pRsaKey)
{
    int  iKeyLen, iLen;
    char sDerBuf[1280];
    int  i;
    char *p;
    int  iDerLen;  // DER��ʽRSA��Կ����
    char *pDest;   // ��Կ����ָ��
    int  iDestLen; // ��Կ��������

    memset(sDerBuf, 0, sizeof(sDerBuf));
    vDesEcb(TRI_DECRYPT, iInLen, pInData, sg_aMasterKey[sg_iMasterKeyIndex].sKey, sDerBuf);
    if(sDerBuf[0] != 0x30)
        return(1);
    if(sDerBuf[1] & 0x80) {
        iDerLen = ulStrToLong(&sDerBuf[2], sDerBuf[1]&0x7F);
        if(iDerLen > iInLen)
            return(1);
        memmove(sDerBuf, &sDerBuf[2+(sDerBuf[1]&0x7F)], iDerLen);
    } else {
        iDerLen = sDerBuf[1];
        memmove(sDerBuf, &sDerBuf[2], iDerLen);
    }
    // ����, sDerBufΪDER Tag30������, iDerLenΪDER Tag30�����ݳ���
    memset(pRsaKey, 0, sizeof(*pRsaKey));
    p = sDerBuf;
    for(i=0; i<9; i++) {
        switch(i) {
        case 0: // Version
            if(memcmp(p, "\x02\x01\x00", 3) != 0) {
                // �����ǰ汾0
                return(1);
            }
            p += 3;
            continue;
        case 1: // N
            if(p[1] & 0x80)
                iKeyLen = ulStrToLong(p+2, p[1]&0x7F);
            else
                iKeyLen = p[1];
            iKeyLen --; // iKeyLenΪRSAģ������,��һ�ֽ�һ���Ǵ��ڵ���0x80��, ȥ��ǰ�油��0
            if(iKeyLen<MIN_RSA_KEY_LEN || iKeyLen>MAX_RSA_KEY_LEN || iKeyLen%2) {
                return(1);
            }
            pRsaKey->iKeyLen = iKeyLen;
            break;
        case 2: // E
            if(memcmp(p, "\x02\x01\x03", 3) == 0) {
                pRsaKey->lE = 3;
                p += 3;
            } else if(memcmp(p, "\x02\x03\x01\x00\x01", 5) == 0) {
                pRsaKey->lE = 65537L;
                p += 5;
            } else {
                return(1);
            }
            continue;
        default:
            break;
        } // switch(0, 1, 2
        
        // ȡ��DER��Կ����
        if(p[0] != 0x02) {
            // Tag����
            return(1);
        }
        if(p[1] & 0x80) {
            iLen = ulStrToLong(p+2, p[1]&0x7F);
            p += 2 + (p[1]&0x7F);
        } else {
            iLen = p[1];
            p += 2;
        }
        // pָ��DER Tlv����ֵ, iLenΪDER Tlv����ֵ����
        if(p+iLen > sDerBuf+iDerLen) {
            // ���
            return(1);
        }
        switch(i) {
        case 1: // N
            pDest = pRsaKey->sN;
            iDestLen = iKeyLen;
            break;
        case 3: // D
            pDest = pRsaKey->sD;
            iDestLen = iKeyLen;
            break;
        case 4: // P
            pDest = pRsaKey->sP;
            iDestLen = iKeyLen / 2;
            break;
        case 5: // Q
            pDest = pRsaKey->sQ;
            iDestLen = iKeyLen / 2;
            break;
        case 6: // dP
            pDest = pRsaKey->sDP;
            iDestLen = iKeyLen / 2;
            break;
        case 7: // dQ
            pDest = pRsaKey->sDQ;
            iDestLen = iKeyLen / 2;
            break;
        case 8: // qInv
            pDest = pRsaKey->sQINV;
            iDestLen = iKeyLen / 2;
            break;
        } // switch(1, 3, 4, 5, 6, 7, 8
        
        // ������Կ����
        if(iLen > iDestLen) {
            // Tlv���󳤶ȴ�����Կ��������, ǰ���ǲ�����0
            if(iLen-1!=iDestLen || p[0]!=0)
                return(1); // ֻ����һ��0
            memcpy(pDest, p+1, iDestLen);
        } else {
            // Tlv���󳤶�С�ڻ������Կ��������, ȫ��������Ŀ�ĵ�
            memcpy(pDest+(iDestLen-iLen), p, iLen);
        }
        p += iLen;
    } // for(
    pRsaKey->ucOKFlag = 3;
    
    return(0);
}

// ���RSA��Կ
// in  : pInData : ���ܺ�DER��ʽRSA��Կ
// out : pRsaKey : �������Կ
// ret : 0       : OK
//       1       : ��������
static int iExtractRsaKey2(void *pInData, stRsaKey *pRsaKey)
{
    char sDerBuf[8];
    int  iDerLen;
    vDesEcb(TRI_DECRYPT, 8, pInData, sg_aMasterKey[sg_iMasterKeyIndex].sKey, sDerBuf); // ���ǰ8�ֽ�
    // Tag[1]+Len[n]
    if(sDerBuf[1] & 0x80) {
        iDerLen = ulStrToLong(&sDerBuf[2], sDerBuf[1]&0x7F);
        iDerLen += 2+(sDerBuf[1]&0x7F);
    } else {
        iDerLen = sDerBuf[1];
        iDerLen += 2;
    }
    return(iExtractRsaKey(iDerLen, pInData, pRsaKey));
}

// ��Կ�ָ�
// in  : iKeyType  : 1:DES��Կ 2:RSA��Կ
//       iKeyIndex : ��Կ����
//       iInLen    : �������ݳ���
//       pInData   : ��������
// ret : 0         : �ɹ�
//       ����      : ʧ��
int iKeyRestore0(int iKeyType, int iKeyIndex, int iInLen, void *pInData)
{
    char szDateTime[15];
    char sKey[16], sBuf[20];
    stRsaKey RsaKey;
    int  iRet;
    
    if(iKeyType!=1 && iKeyType!=2)
        return(HSMERR_TYPE);
    if(iKeyType==1 && (iKeyIndex<0 || iKeyIndex>sg_iMaxDesKeyNum))
        return(HSMERR_INDEX);
    if(iKeyType==2 && (iKeyIndex<0 || iKeyIndex>sg_iMaxRsaKeyNum))
        return(HSMERR_INDEX);
    if(sg_aMasterKey[sg_iMasterKeyIndex].ucOKFlag == 0)
        return(HSMERR_NO_MASTERKEY);

    if(iKeyType == 1) {
        // deskey, EncryptedKey[16]+cks[3]+inf[40]
        if(iInLen != 16+8) {
            return(HSMERR_DATAIN_LEN);
        }
/*
// ��ʿͨ���ܻ�û�����ж�
        if(sg_aDesKey[iKeyIndex].ucOKFlag) {
            return(HSMERR_KEY_EXIST);
        }
*/
        vDesEcb(TRI_DECRYPT, 16, pInData, sg_aMasterKey[sg_iMasterKeyIndex].sKey, sKey);
        memset(sBuf, 0, 8);
        _vDes(TRI_ENCRYPT, sBuf, sKey, sBuf);
        if(memcmp(sBuf, (char *)pInData+16, 8) != 0) {
            return(HSMERR_KEY_CHECK);
        }
        memcpy(sg_aDesKey[iKeyIndex].sKey, sKey, 16);
        memset(sKey, 0, 16);
        _vGetTime(szDateTime);
	    sprintf(sg_aDesKey[iKeyIndex].szInf, "����ԿΪ���ݻָ�, ʱ��:%s", szDateTime);
        sg_aDesKey[iKeyIndex].ucOKFlag = 1;
    } else {
        // RSA��ԿΪ���ܺ�DER��ʽ
        /*
        if(sg_aRsaKey[iKeyIndex].ucOKFlag) {
            return(HSMERR_KEY_EXIST);
        }
        */
        if(iInLen % 8) {
            return(HSMERR_DATAIN_LEN);
        }
        iRet = iExtractRsaKey(iInLen, pInData, &RsaKey);
        if(iRet) {
            return(HSMERR_DATAIN_LEN);
        }
        memcpy(&sg_aRsaKey[iKeyIndex], &RsaKey, sizeof(RsaKey));
        _vGetTime(szDateTime);
	    sprintf(sg_aRsaKey[iKeyIndex].szInf, "����ԿΪ���ݻָ�, ʱ��:%s", szDateTime);
    }

    iRet = iKeyFileSave();

    return(iRet);
}

// ��Կɾ��
// in  : iKeyType  : 1:RSA��Կ 2:DES��Կ
//       iKeyIndex : ��Կ����
// ret : 0         : �ɹ�
//       ����      : ʧ��
int iKeyDelete0(int iKeyType, int iKeyIndex)
{
    int iRet;
    if(iKeyType!=1 && iKeyType!=2)
        return(HSMERR_TYPE);
    if(iKeyType==1 && (iKeyIndex<0 || iKeyIndex>sg_iMaxRsaKeyNum))
        return(HSMERR_INDEX);
    if(iKeyType==2 && (iKeyIndex<0 || iKeyIndex>sg_iMaxDesKeyNum))
        return(HSMERR_INDEX);

    if(iKeyType == 1) {
        memset(&sg_aRsaKey[iKeyIndex], 0, sizeof(sg_aDesKey[iKeyIndex]));
    } else {
        memset(&sg_aDesKey[iKeyIndex], 0, sizeof(sg_aDesKey[iKeyIndex]));
    }
    iRet = iKeyFileSave();
    return(iRet);
}

// ����DES��Կ
// in  : iKeyIndex : ��Կ����(0-999), -1��ʾ���洢
//       pKey      : LMK���ܺ����, NULL��ʾ�����
// out : pKey      : LMK���ܺ�����Կ[16]+cks[8], NULL��ʾ�����
// ret : 0         : �ɹ�
//       ����      : ʧ��
// Note: iKeyIndexΪ-1ʱpKey����ΪNULL
int iKeyGenDesKey0(int iKeyIndex, void *pKey)
{
	char szDateTime[15];
	char sKey[16];
	int iRet;
	int i;

    if(iKeyIndex==-1 && pKey==NULL)
        return(HSMERR_INDEX);
    if(iKeyIndex<-1 || iKeyIndex>sg_iMaxDesKeyNum)
        return(HSMERR_INDEX);

    for(i=0; i<16; i++)
        sKey[i] = rand() % 256;
    if(iKeyIndex >= 0) {
        sg_aDesKey[iKeyIndex].ucOKFlag = 1;
        memcpy(sg_aDesKey[iKeyIndex].sKey, sKey, 16);
        _vGetTime(szDateTime);
	    sprintf(sg_aDesKey[iKeyIndex].szInf, "����ԿΪ�Զ�����, ʱ��:%s", szDateTime);
        iRet = iKeyFileSave();
    }
    if(pKey) {
        vDesEcb(TRI_ENCRYPT, 16, sKey, sg_aMasterKey[sg_iMasterKeyIndex].sKey, pKey);
        memset(szDateTime, 0, 8);
        _vDes(TRI_ENCRYPT, szDateTime, sKey, (char *)pKey+16);
    }
    return(iRet);
}

// ����RSA��Կ
// in  : iKeyIndex : ��Կ����(0-19), -1��ʾ���洢
//       iKeyLen   : ��Կ����, ��λ:�ֽ� 64-248, ����Ϊż��
//       lE        : ����ָ��, 3��65537
//       pKey      : LMK���ܺ�����Կ, NULL��ʾ�����
// out : pN        : ģ��
//       pKey      : LMK���ܺ�����Կ, NULL��ʾ�����
//       piOutLen  : ����ļ��ܺ���Կ����
// ret : 0         : �ɹ�
//       ����      : ʧ��
// Note: iKeyIndexΪ-1ʱpKey����ΪNULL
int iKeyGenRsaKey0(int iKeyIndex, int iKeyLen, long lE, void *pN, void *pKey, int *piOutLen)
{
#ifndef USE_KEY_POOL
	R_RSA_PROTO_KEY   ProtoKey;
#endif
	R_RSA_PUBLIC_KEY  PublicKey;
	R_RSA_PRIVATE_KEY PrivateKey;
	stRsaKey          RsaKey;
	char szDateTime[15];
	int  iRet;

    if(iKeyIndex==-1 && pKey==NULL)
        return(HSMERR_PARA);
    if(iKeyIndex<-1 || iKeyIndex>sg_iMaxRsaKeyNum)
        return(HSMERR_INDEX);
    if(iKeyLen<MIN_RSA_KEY_LEN || iKeyLen>MAX_RSA_KEY_LEN)
        return(HSMERR_KEY_LEN);
    if(iKeyLen % 2)
        return(HSMERR_KEY_LEN);
    if(lE!=3 && lE!=65537L)
        return(HSMERR_RSA_E);

#ifndef USE_KEY_POOL
	ProtoKey.bits = iKeyLen * 8;
	ProtoKey.useFermat4 = lE == 3 ? 0 : 1;
    iRet = R_GeneratePEMKeys(&PublicKey, &PrivateKey, &ProtoKey);
#else
    iRet = iRsaKeyPoolGetKey(iKeyLen, lE, &PublicKey, &PrivateKey);
#endif
    if(iRet)
        return(HSMERR_INTERNAL);

    RsaKey.ucOKFlag = 3; // D and CRT
    RsaKey.iKeyLen = iKeyLen;
    RsaKey.lE = lE;
    
	memcpy(RsaKey.sN, PrivateKey.modulus+sizeof(PrivateKey.modulus)-iKeyLen, iKeyLen);
	memcpy(RsaKey.sD, PrivateKey.exponent+sizeof(PrivateKey.exponent)-iKeyLen, iKeyLen);
	memcpy(RsaKey.sP, PrivateKey.prime[0]+sizeof(PrivateKey.prime[1])-iKeyLen/2, iKeyLen/2);
	memcpy(RsaKey.sQ, PrivateKey.prime[1]+sizeof(PrivateKey.prime[1])-iKeyLen/2, iKeyLen/2);
	memcpy(RsaKey.sDP, PrivateKey.primeExponent[0]+sizeof(PrivateKey.primeExponent[0])-iKeyLen/2, iKeyLen/2);
	memcpy(RsaKey.sDQ, PrivateKey.primeExponent[1]+sizeof(PrivateKey.primeExponent[1])-iKeyLen/2, iKeyLen/2);
	memcpy(RsaKey.sQINV, PrivateKey.coefficient+sizeof(PrivateKey.coefficient)-iKeyLen/2, iKeyLen/2);
    _vGetTime(szDateTime);
	sprintf(RsaKey.szInf, "����ԿΪ�Զ�����, ʱ��:%s", szDateTime);
    memset(RsaKey.sPassword, 0, sizeof(RsaKey.sPassword));

    if(iKeyIndex >= 0) {
        memcpy(&sg_aRsaKey[iKeyIndex], &RsaKey, sizeof(RsaKey));
        iRet = iKeyFileSave();
    }
    memcpy(pN, RsaKey.sN, iKeyLen);
    if(pKey)
        iPackRsaKey(&RsaKey, piOutLen, pKey);

    return(iRet);
}

// RSA˽Կ����
// in  : iKeyIndex : RSA��Կ����
//       pKey      : LMK���ܺ�RSA˽Կ, NULL��ʾʹ��������Կ
//       iLen      : �������ݳ���
//       pIn       : ��������
// out : pOut      : ������
// ret : 0         : �ɹ�
//       ����      : ʧ��
int iKeyRsaPrivateBlock0(int iKeyIndex, void *pKey, int iLen, void *pIn, void *pOut)
{
	R_RSA_PROTO_KEY   ProtoKey;
	R_RSA_PRIVATE_KEY PrivateKey;
	stRsaKey          RsaKey;
	unsigned short    usOutLen;
	int iRet;

    if(pKey) {
        iRet = iExtractRsaKey2(pKey, &RsaKey);
        if(iRet) {
            return(HSMERR_RSA_FORMAT);
        }
    } else {
        if(iKeyIndex<0 || iKeyIndex>sg_iMaxRsaKeyNum) {
            return(HSMERR_INDEX);
        }
        memcpy(&RsaKey, &sg_aRsaKey[iKeyIndex], sizeof(RsaKey));
        if(RsaKey.ucOKFlag == 0)
            return(HSMERR_NO_RSAKEY);
    }
    if(iLen != RsaKey.iKeyLen)
        return(HSMERR_DATAIN_LEN);

	ProtoKey.bits = RsaKey.iKeyLen * 8;
	ProtoKey.useFermat4 = RsaKey.lE==3 ? 0 : 1; // 0:3 1:65537
	if(RsaKey.ucOKFlag & 0x02) {
	    // support CRT
	    RSASetPrivateKeyCRT(&PrivateKey, 
	                        RsaKey.sN,
	                        RsaKey.sP,
	                        RsaKey.sQ,
	                        RsaKey.sDP,
	                        RsaKey.sDQ,
	                        RsaKey.sQINV,
                            &ProtoKey);
	} else {
	    // support D
	    RSASetPrivateKey(&PrivateKey,
	                        RsaKey.sN,
	                        RsaKey.sD,
                            &ProtoKey);
	}
	RSAPrivateBlock((unsigned char *)pOut, &usOutLen, (unsigned char *)pIn, (unsigned short)iLen, &PrivateKey);
    return(0);
}

// RSA��Կ����
// in  : iKeyIndex : RSA��Կ����
//       iLen      : �������ݳ���
//       pIn       : ��������
// out : pOut      : ������
// ret : 0         : �ɹ�
//       ����      : ʧ��
int iKeyRsaPublicBlock0(int iKeyIndex, int iLen, void *pIn, void *pOut)
{
	R_RSA_PROTO_KEY   ProtoKey;
	R_RSA_PUBLIC_KEY  PublicKey;
	stRsaKey          RsaKey;
	unsigned short    usOutLen;

    if(iKeyIndex<0 || iKeyIndex>sg_iMaxRsaKeyNum)
        return(HSMERR_INDEX);
    memcpy(&RsaKey, &sg_aRsaKey[iKeyIndex], sizeof(RsaKey));
    
    if(RsaKey.ucOKFlag == 0)
        return(HSMERR_NO_RSAKEY);
    if(iLen != RsaKey.iKeyLen)
        return(HSMERR_DATAIN_LEN);

	ProtoKey.bits = RsaKey.iKeyLen * 8;
	ProtoKey.useFermat4 = RsaKey.lE==3 ? 0 : 1; // 0:3 1:65537
	RSASetPublicKey(&PublicKey,
	                RsaKey.sN,
                    &ProtoKey);
    
	RSAPublicBlock((unsigned char *)pOut, &usOutLen, (unsigned char *)pIn, (unsigned short)iLen, &PublicKey);
    return(0);
}

// ��ȡRSA��Կ
// in  : iKeyIndex : RSA��Կ����
//       pKey      : LMK���ܺ�RSA˽Կ, NULL��ʾʹ��������Կ
// out : piLen     : RSA��Կ����, ��λ:�ֽ�
//       plE       : ����ָ��
//       pModulus  : ģ��
// ret : 0         : �ɹ�
//       ����      : ʧ��
int iKeyRsaGetInfo0(int iKeyIndex, void *pKey, int *piLen, long *plE, void *pModulus)
{
	stRsaKey          RsaKey;
	int iRet;

    if(pKey) {
        iRet = iExtractRsaKey2(pKey, &RsaKey);
        if(iRet) {
            return(HSMERR_RSA_FORMAT);
        }
    } else {
        if(iKeyIndex<0 || iKeyIndex>sg_iMaxRsaKeyNum)
            return(HSMERR_INDEX);
        memcpy(&RsaKey, &sg_aRsaKey[iKeyIndex], sizeof(RsaKey));
        if(RsaKey.ucOKFlag == 0) {
            return(HSMERR_NO_RSAKEY);
        }
    }

    *piLen = RsaKey.iKeyLen;
    *plE = RsaKey.lE;
    memcpy(pModulus, RsaKey.sN, *piLen);
    return(0);
}

// RSAֱ�ӹ�Կ����(���빫Կ)
// in  : iLen      : RSA��Կ����, ��λ:�ֽ�
//       lE        : RSA��Կָ��, 3����65537
//       pN        : ģ��
//       pIn       : ��������
// out : pOut      : �������
// ret : 0         : �ɹ�
//       ����      : ʧ��
int iKeyRsaPublicBlock20(int iLen, long lE, void *pN, void *pIn, void *pOut)
{
	R_RSA_PROTO_KEY   ProtoKey;
	R_RSA_PUBLIC_KEY  PublicKey;
	unsigned short    usOutLen;

    if(iLen<MIN_RSA_KEY_LEN || iLen>MAX_RSA_KEY_LEN)
        return(HSMERR_KEY_LEN);
    if(iLen % 2)
        return(HSMERR_KEY_LEN);
    if(lE!=3 && lE!=65537L)
        return(HSMERR_RSA_E);

	ProtoKey.bits = iLen * 8;
	ProtoKey.useFermat4 = lE==3 ? 0 : 1; // 0:3 1:65537
	RSASetPublicKey(&PublicKey, (unsigned char *)pN, &ProtoKey);
	RSAPublicBlock((unsigned char *)pOut, &usOutLen, (unsigned char *)pIn, (unsigned short)iLen, &PublicKey);
    return(0);
}

// ������Ƭ�ⲿ��֤����
// in  : iKey1Index : ��Կ����
//       pKey1      : LMK���ܺ���Կ, NULL��ʾʹ��������Կ
//       iDiv1Level : ��Կ��ɢ����(0-3)
//       pDiv1Data  : ��Կ��ɢ����[8*n]
//       pTermRand  : �ն������[8]
//       pCardData  : ��Ƭ����[28]
// out : pOut       : �ⲿ��֤���ļ�Mac[16]
// ret : KFR_OK     : �ɹ�
//       ����       : ʧ��
int iKeyCalExtAuth0(int iKey1Index, void *pKey1, int iDiv1Level, void *pDiv1Data, void *pTermRand, void *pCardData, void *pOut)
{
    unsigned char ucSessionKeyFlag = 1; // �Ƿ����������Կ��־(0:������������Կ 1:����������Կ)
    //QINGBO
    //unsigned char ucDivMode = 1;        // ��Ƭ��ɢģʽ(0:����ɢ 1:��ɢ)
	unsigned char ucSCPFlag;
	unsigned char sCardRand[8];
	unsigned char sCardCrypto[8];
	unsigned char sKey[16];
	unsigned char sSessionAuthKey[16], sSessionCMacKey[16];
	unsigned char sBuf[50], szBuf[100];
	unsigned char *psCardData;
	int   iRet;

    psCardData = (unsigned char *)pCardData;
    iRet = iDivKey(iKey1Index, pKey1, (uchar)iDiv1Level, pDiv1Data, sKey);
    if(iRet)
        return(iRet);
	ucSCPFlag = psCardData[11];
	// psCardData : KeyDivData[10]+KeyInfo[2]+CardChallenge[8]+CardCrypto[8]
    memcpy(sCardRand, psCardData+10+2, 8);
	memcpy(sCardCrypto, psCardData+10+2+8, 8);

	if(ucSessionKeyFlag) {
#if 1
		// 20140519, �Զ�����KMC��ɢ��ʽ
		sg_iGpDivMethod = 212; // �ȳ���212��ɢ��ʽ
		// cal session key:Auth Key �����ⲿ��֤
		iRet = iGpDivChannelKey(1/*1:S-AUTH(�ⲿ��֤)*/, sg_ucDivMode, pTermRand, pCardData, sKey, sSessionAuthKey);
		if(iRet)
			return(HSMERR_INTERNAL);
		// cal session key:Mac Key ����Mac����
		iRet = iGpDivChannelKey(2/*2:C-MAC(Command APDU Mac)*/, sg_ucDivMode, pTermRand, pCardData, sKey, sSessionCMacKey);
		if(iRet)
			return(HSMERR_INTERNAL);

		// check card crypto
		memcpy(sBuf, pTermRand, 8); // term rand
		memcpy(sBuf+8, sCardRand, 8); // card rand
		memset(szBuf, 0, 8); // IV
		iRet = iMac3DesCBC(sSessionAuthKey, sBuf, 16, szBuf);
		if(iRet)
			return(HSMERR_INTERNAL);
		if(memcmp(szBuf, sCardCrypto, 8) != 0) {
			// 212��ɢ��ʽ������У�����ݲ���ȷ, ������202��ɢ��ʽ
			sg_iGpDivMethod = 202; // �ȳ���212��ɢ��ʽ
			// cal session key:Auth Key �����ⲿ��֤
			iRet = iGpDivChannelKey(1/*1:S-AUTH(�ⲿ��֤)*/, sg_ucDivMode, pTermRand, pCardData, sKey, sSessionAuthKey);
			if(iRet)
				return(HSMERR_INTERNAL);
			// cal session key:Mac Key ����Mac����
			iRet = iGpDivChannelKey(2/*2:C-MAC(Command APDU Mac)*/, sg_ucDivMode, pTermRand, pCardData, sKey, sSessionCMacKey);
			if(iRet)
				return(HSMERR_INTERNAL);
		}
#else
		// ԭ�����Զ�����KMC��ʽ
		// cal session key:Auth Key �����ⲿ��֤
		iRet = iGpDivChannelKey(1/*1:S-AUTH(�ⲿ��֤)*/, ucDivMode, pTermRand, pCardData, sKey, sSessionAuthKey);
		if(iRet)
			return(HSMERR_INTERNAL);
		// cal session key:Mac Key ����Mac����
		iRet = iGpDivChannelKey(2/*2:C-MAC(Command APDU Mac)*/, ucDivMode, pTermRand, pCardData, sKey, sSessionCMacKey);
		if(iRet)
			return(HSMERR_INTERNAL);
#endif
	} else {
		// ������������Կ
		memcpy(sSessionAuthKey, sKey, 16);
		memcpy(sSessionCMacKey, sKey, 16);
	}

// check card crypto
	memcpy(sBuf, pTermRand, 8); // term rand
	memcpy(sBuf+8, sCardRand, 8); // card rand
	memset(szBuf, 0, 8); // IV
	iRet = iMac3DesCBC(sSessionAuthKey, sBuf, 16, szBuf);
	if(iRet)
		return(HSMERR_INTERNAL);
	if(memcmp(szBuf, sCardCrypto, 8) != 0)
	    return(HSMERR_CHECK);

// call external auth data
    memcpy(sBuf, sCardRand, 8); // card rand
	memcpy(sBuf+8, pTermRand, 8); // term rand
	memset(szBuf, 0, 8); // IV
    iRet = iMac3DesCBC(sSessionAuthKey, sBuf, 16, szBuf);
	if(iRet)
		return(HSMERR_INTERNAL);
	memcpy(sBuf, "\x84\x82\x00\x00\x10", 5);
	memcpy(sBuf+5, szBuf, 8);
	memset(szBuf+8, 0, 8); // IV
    if(ucSCPFlag == 0x02)
	    iRet = iMac3DesCBCRetail(sSessionCMacKey, sBuf, 5+8, szBuf+8);
	else
    	iRet = iMac3DesCBC(sSessionCMacKey, sBuf, 5+8, szBuf+8);
	if(iRet)
		return(HSMERR_INTERNAL);
    
	memcpy(pOut, szBuf, 16);
	return(0);
}

// �����޸�KMC����
// in  : iKey1Index : ԭ��Կ����
//       pKey1      : LMK���ܺ�ԭ��Կ, NULL��ʾʹ��������Կ
//       iDiv1Level : ԭ��Կ��ɢ����(0-3)
//       pDiv1Data  : ԭ��Կ��ɢ����[8*n]
//       pTermRand  : �ն������[8]
//       pCardData  : ��Ƭ����[28]
//       iKey2Index : ����Կ����
//       pKey2      : LMK���ܺ�����Կ, NULL��ʾʹ��������Կ
//       iDiv2Level : ����Կ��ɢ����(0-3)
//       pDiv2Data  : ����Կ��ɢ����[8*n]
// out : pOut       : ����AuthKey[16]+AuthKeyKcv[8]+����MacKey[16]+MacKeyKcv[8]+����EncKey[16]+EncKeyKcv[8]
// ret : KFR_OK     : �ɹ�
//       ����       : ʧ��
int iKeyCalChangeKmc0(int iKey1Index, void *pKey1, int iDiv1Level, void *pDiv1Data, void *pTermRand, void *pCardData, int iKey2Index, void *pKey2, int iDiv2Level, void *pDiv2Data, void *pOut)
{
    unsigned char ucSessionKeyFlag = 1; // �Ƿ����������Կ��־(0:������������Կ 1:����������Կ)
    //QINGBO
    //unsigned char ucDivMode = 1;        // ��Ƭ��ɢģʽ(0:����ɢ 1:��ɢ)
	unsigned char sKey[16], sDivKey[16];
	unsigned char sSessionEncKey[16];
	unsigned char sBuf[50];
	unsigned char *psOut;
	unsigned char *psCardData;
	int           iRet;

    psCardData = (unsigned char *)pCardData;
    iRet = iDivKey(iKey1Index, pKey1, (uchar)iDiv1Level, pDiv1Data, sKey);
    if(iRet)
        return(iRet);
	if(ucSessionKeyFlag) {
		// cal session key:Enc Key �������ݼ���
		iRet = iGpDivChannelKey(4/*4:Enc(����)*/, sg_ucDivMode, pTermRand, pCardData, sKey, sSessionEncKey);
		if(iRet)
			return(HSMERR_INTERNAL);
	} else {
		// ������������Կ
		memcpy(sSessionEncKey, sKey, 16);
	}
    iRet = iDivKey(iKey2Index, pKey2, (uchar)iDiv2Level, pDiv2Data, sKey);
    if(iRet)
        return(iRet);
        
    psOut = (unsigned char *)pOut;
    memset(sBuf, 0, 8);
    iRet = iGpDivKey(1/*1:S-AUTH(�ⲿ��֤)*/, pCardData, sKey, sDivKey);
    vDesEcb(TRI_ENCRYPT, 16, sDivKey, sSessionEncKey, psOut);
    _vDes(TRI_ENCRYPT, sBuf, sDivKey, psOut+16);

    iRet = iGpDivKey(2/*2:Mac*/, psCardData, sKey, sDivKey);
    vDesEcb(TRI_ENCRYPT, 16, sDivKey, sSessionEncKey, psOut+24);
    _vDes(TRI_ENCRYPT, sBuf, sDivKey, psOut+40);

    iRet = iGpDivKey(4/*4:Enc*/, psCardData, sKey, sDivKey);
    vDesEcb(TRI_ENCRYPT, 16, sDivKey, sSessionEncKey, psOut+48);
    _vDes(TRI_ENCRYPT, sBuf, sDivKey, psOut+64);

	return(0);
}

// ����IC��DES��Կ����(��ʱ����)
// in  : iKey1Index : ������Կ����
//       pKey1      : LMK���ܺ�������Կ, NULL��ʾʹ��������Կ
//       iDiv1Level : ������Կ��ɢ����(0-3)
//       pDiv1Data  : ������Կ��ɢ����[8*n]
//       pTermRand  : �ն������[8]
//       pCardData  : ��Ƭ����[28]
//       iKey2Index : Ӧ����Կ����
//       pKey2      : LMK���ܺ�Ӧ����Կ, NULL��ʾʹ��������Կ
//       iDiv2Level : Ӧ����Կ��ɢ����(0-3)
//       pDiv2Data  : Ӧ����Կ��ɢ����[8*n]
// out : pOut       : ����Key[16]+KeyKcv[8]
// ret : KFR_OK     : �ɹ�
//       ����       : ʧ��
int iKeyCalDesKey0(int iKey1Index, void *pKey1, int iDiv1Level, void *pDiv1Data, void *pTermRand, void *pCardData, int iKey2Index, void *pKey2, int iDiv2Level, void *pDiv2Data, void *pOut)
{
    unsigned char ucSessionKeyFlag = 1; // �Ƿ����������Կ��־(0:������������Կ 1:����������Կ)
    //QINGBO
    //unsigned char ucDivMode = 1;        // ��Ƭ��ɢģʽ(0:����ɢ 1:��ɢ)
	unsigned char sSessionDekKey[16];
	unsigned char sKek[16];    // ���ڼ��ܵ���Կ
	unsigned char sOutKey[16]; // �����ܵ���Կ
	unsigned char sBuf[50], szBuf[100];
	int   iRet;

    iRet = iDivKey(iKey1Index, pKey1, (uchar)iDiv1Level, pDiv1Data, sKek);
    if(iRet)
        return(iRet);
	if(ucSessionKeyFlag) {
    	// cal session key:ENC Key ���ڼ�����������
	    iRet = iGpDivChannelKey(4/*4:ENC(Sensitive Data Encryption)*/, sg_ucDivMode, pTermRand, pCardData, sKek, sSessionDekKey);
    	if(iRet)
	    	return(iRet);
	} else {
		// ������������Կ
		memcpy(sSessionDekKey, sKek, 16);
	}
    if(iDiv2Level<1 || iDiv2Level>3)
        return(HSMERR_DIV_NUM);
    iRet = iDivKey(iKey2Index, pKey2, (uchar)iDiv2Level, pDiv2Data, sOutKey);
    if(iRet)
        return(iRet);
    vDesEcb(TRI_ENCRYPT, 16, sOutKey, sSessionDekKey, pOut);
	memset(szBuf, 0, 8);
	_vDes(TRI_ENCRYPT, szBuf, sOutKey, sBuf);
	memset(sOutKey, 0, 16);
    memcpy((char *)pOut+16, sBuf, 8); // key check value

	return(0);
}

// ����IC��PIN����(��ʱ����)
// in  : iKey1Index : ������Կ����
//       pKey1      : LMK���ܺ�������Կ, NULL��ʾʹ��������Կ
//       iDiv1Level : ������Կ��ɢ����(0-3)
//       pDiv1Data  : ������Կ��ɢ����[8*n]
//       pTermRand  : �ն������[8]
//       pCardData  : ��Ƭ����[28]
//       pPinBlock  : �����������ݿ�
// out : pOut       : ���ĸ����������ݿ�[8]
// ret : KFR_OK     : �ɹ�
//       ����       : ʧ��
int iKeyCalPin0(int iKey1Index, void *pKey1, int iDiv1Level, void *pDiv1Data, void *pTermRand, void *pCardData, void *pPinBlock, void *pOut)
{
    unsigned char ucSessionKeyFlag = 1; // �Ƿ����������Կ��־(0:������������Կ 1:����������Կ)
    //QINGBO
    //unsigned char ucDivMode = 1;        // ��Ƭ��ɢģʽ(0:����ɢ 1:��ɢ)
	unsigned char sSessionDekKey[16];
	unsigned char sKek[16];    // ���ڼ��ܵ���Կ
	int   iRet;

    iRet = iDivKey(iKey1Index, pKey1, (uchar)iDiv1Level, pDiv1Data, sKek);
    if(iRet)
        return(iRet);
	if(ucSessionKeyFlag) {
    	// cal session key:ENC Key ���ڼ�����������
	    iRet = iGpDivChannelKey(4/*4:ENC(Sensitive Data Encryption)*/, sg_ucDivMode, pTermRand, pCardData, sKek, sSessionDekKey);
    	if(iRet)
	    	return(iRet);
	} else {
		// ������������Կ
		memcpy(sSessionDekKey, sKek, 16);
	}
	
	_vDes(TRI_ENCRYPT, pPinBlock, sSessionDekKey, pOut);
    return(0);
}

// ����IC��RSA��Կ����(��ʱ����)
// in  : iKey1Index : ������Կ����
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
int iKeyCalRsaKey0(int iKey1Index, void *pKey1, int iDiv1Level, void *pDiv1Data, void *pTermRand, void *pCardData, int iRsaKeyLen, long lE, void *pOut)
{
    uchar ucSessionKeyFlag = 1; // �Ƿ����������Կ��־(0:������������Կ 1:����������Կ)
    //QINGBO
    //unsigned char ucDivMode = 1;        // ��Ƭ��ɢģʽ(0:����ɢ 1:��ɢ)
	uchar sSessionDekKey[16];
	uchar sKek[16];    // ���ڼ��ܵ���Կ
	uchar sBuf[300];
#ifndef USE_KEY_POOL
	R_RSA_PROTO_KEY   ProtoKey;
#endif
	R_RSA_PUBLIC_KEY  PublicKey;
	R_RSA_PRIVATE_KEY PrivateKey;
	int   iItem1Len, iItem2Len;
	int   iRet;

    if(iRsaKeyLen%2 || iRsaKeyLen<MIN_RSA_KEY_LEN || iRsaKeyLen>MAX_RSA_KEY_LEN)
        return(HSMERR_KEY_LEN);
    if(lE!=3 && lE!=65537L)
        return(HSMERR_RSA_E);

    iRet = iDivKey(iKey1Index, pKey1, (uchar)iDiv1Level, pDiv1Data, sKek);
    if(iRet)
        return(iRet);
	if(ucSessionKeyFlag) {
    	// cal session key:ENC Key ���ڼ�����������
	    iRet = iGpDivChannelKey(4/*4:ENC(Sensitive Data Encryption)*/, sg_ucDivMode, pTermRand, pCardData, sKek, sSessionDekKey);
    	if(iRet)
	    	return(iRet);
	} else {
		// ������������Կ
		memcpy(sSessionDekKey, sKek, 16);
	}
        
    // ����RSA��Կ
#ifndef USE_KEY_POOL
	ProtoKey.bits = iRsaKeyLen * 8;
	ProtoKey.useFermat4 = lE == 3 ? 0 : 1;
    iRet = R_GeneratePEMKeys(&PublicKey, &PrivateKey, &ProtoKey);
#else
    iRet = iRsaKeyPoolGetKey(iRsaKeyLen, lE, &PublicKey, &PrivateKey);
#endif
    if(iRet)
        return(HSMERR_INTERNAL);
    
	memcpy(pOut, PrivateKey.modulus+sizeof(PrivateKey.modulus)-iRsaKeyLen, iRsaKeyLen);
    iItem1Len = (iRsaKeyLen+1+7)/8*8; // D����
    memset(sBuf, 0, sizeof(sBuf));
	memcpy(sBuf, PrivateKey.exponent+sizeof(PrivateKey.exponent)-iRsaKeyLen, iRsaKeyLen);
	sBuf[iRsaKeyLen] = 0x80;
    vDesEcb(TRI_ENCRYPT, iItem1Len, sBuf, sSessionDekKey, (char *)pOut+iRsaKeyLen);

    iItem2Len = (iRsaKeyLen/2+1+7)/8*8; // ��Կ����(p��q��)����
    memset(sBuf, 0, sizeof(sBuf));
	sBuf[iRsaKeyLen/2] = 0x80;
	memcpy(sBuf, PrivateKey.prime[0]+sizeof(PrivateKey.prime[0])-iRsaKeyLen/2, iRsaKeyLen/2);
    vDesEcb(TRI_ENCRYPT, iItem2Len, sBuf, sSessionDekKey, (char *)pOut+iRsaKeyLen+iItem1Len+iItem2Len*0);
	memcpy(sBuf, PrivateKey.prime[1]+sizeof(PrivateKey.prime[1])-iRsaKeyLen/2, iRsaKeyLen/2);
    vDesEcb(TRI_ENCRYPT, iItem2Len, sBuf, sSessionDekKey, (char *)pOut+iRsaKeyLen+iItem1Len+iItem2Len*1);
	memcpy(sBuf, PrivateKey.primeExponent[0]+sizeof(PrivateKey.primeExponent[0])-iRsaKeyLen/2, iRsaKeyLen/2);
    vDesEcb(TRI_ENCRYPT, iItem2Len, sBuf, sSessionDekKey, (char *)pOut+iRsaKeyLen+iItem1Len+iItem2Len*2);
	memcpy(sBuf, PrivateKey.primeExponent[1]+sizeof(PrivateKey.primeExponent[1])-iRsaKeyLen/2, iRsaKeyLen/2);
    vDesEcb(TRI_ENCRYPT, iItem2Len, sBuf, sSessionDekKey, (char *)pOut+iRsaKeyLen+iItem1Len+iItem2Len*3);
	memcpy(sBuf, PrivateKey.coefficient+sizeof(PrivateKey.coefficient)-iRsaKeyLen/2, iRsaKeyLen/2);
    vDesEcb(TRI_ENCRYPT, iItem2Len, sBuf, sSessionDekKey, (char *)pOut+iRsaKeyLen+iItem1Len+iItem2Len*4);
    	
	return(0);
}

// ��֤ARQC��TC, ����ARPC
// in  : iKey1Index     : ��Կ����
//       pKey1          : LMK���ܺ���Կ, NULL��ʾʹ��������Կ
//       iDiv1Level     : ��Կ��ɢ����(0-3)
//       pDiv1Data      : ��Կ��ɢ����[8*n]
//       pATC           : �������к�[2]
//       iTransDataLen  : �������ݳ���(�������Ϊ0, ����֤ARQC, ֻ����ARPC)
//       pTransData     : ��������
//       pARQC          : ARQC��TC
//       pARC           : ����Ӧ����, NULL��ʾֻ��֤ARQC��TC
// out : pARPC          : ARPC
// ret : KFR_OK         : �ɹ�
//       ����           : ʧ��
int iKeyVerifyAc0(int iKey1Index, void *pKey1, int iDiv1Level, void *pDiv1Data, void *pATC, int iTransDataLen, void *pTransData, void *pARQC, void *pARC, void *pARPC)
{
    char  sBuf[64];
	char  sDivKey[16];
	char  sSessionKey[16];
	int   iRet;

    if(iDiv1Level<1 || iDiv1Level>3)
        return(HSMERR_DIV_NUM);
    iRet = iDivKey(iKey1Index, pKey1, (uchar)iDiv1Level, pDiv1Data, sDivKey);
    if(iRet)
        return(iRet);

    // cal session key
	memset(sBuf, 0, 8);
	memcpy(sBuf+6, pATC, 2);
	_vDes(TRI_ENCRYPT, sBuf, sDivKey, sSessionKey);
	vXor(sBuf+6, "\xFF\xFF", 2);
	_vDes(TRI_ENCRYPT, sBuf, sDivKey, sSessionKey+8);

    if(iTransDataLen) {
// cal mac
    	memset(sBuf, 0, 8);
        iRet = iMac3DesCBCRetail(sSessionKey, pTransData, (uchar)iTransDataLen, sBuf);
    	if(iRet)
	    	return(HSMERR_INTERNAL);
// compare ARQC
    	if(memcmp(sBuf, pARQC, 8) != 0)
	        return(HSMERR_AC);
	}

    if(pARC) {
        // ����Ӧ����, ����ARPC
	    memcpy(sBuf, pARQC, 8);
        vXor(sBuf, pARC, 2);
	    _vDes(TRI_ENCRYPT, sBuf, sSessionKey, pARPC);
    }
	return(0);
}

// �ù�����Կ��������(�ű�)
// in  : iKey1Index    : ��Կ����
//       pKey1         : LMK���ܺ���Կ, NULL��ʾʹ��������Կ
//       iDiv1Level    : ��Կ��ɢ����(0-3)
//       pDiv1Data     : ��Կ��ɢ����[8*n]
//       pATC          : �������к�[2]
//       iPinFlag      : ������־, 0:pData����ͨ���� 1:pData�������
//       iDataLen      : ���ݳ���
//       pData         : ����
// out : piOutLen      : ������ݳ���
//       pOutData      : �������
// ret : KFR_OK        : �ɹ�
//       ����          : ʧ��
int iKeyScriptEncData0(int iKey1Index, void *pKey1, int iDiv1Level, void *pDiv1Data, void *pATC, int iPinFlag, int iDataLen, void *pData, int *piOutLen, void *pOutData)
{
    unsigned char sBuf[256];
	char  sDivKey[16];
	char  sSessionKey[16];
	int   iRet;

    if(iDiv1Level<1 || iDiv1Level>3)
        return(HSMERR_DIV_NUM);
    iRet = iDivKey(iKey1Index, pKey1, (uchar)iDiv1Level, pDiv1Data, sDivKey);
    if(iRet)
        return(iRet);

    // cal session key
	memset(sBuf, 0, 8);
	memcpy(sBuf+6, pATC, 2);
	_vDes(TRI_ENCRYPT, sBuf, sDivKey, sSessionKey);
	vXor(sBuf+6, "\xFF\xFF", 2);
	_vDes(TRI_ENCRYPT, sBuf, sDivKey, sSessionKey+8);

    // encrypt data
    memset(sBuf, 0, sizeof(sBuf));
    sBuf[0] = iDataLen;
    memcpy(sBuf+1, pData, iDataLen);
    sBuf[1+iDataLen] = 0x80;
    if(iPinFlag && iDataLen==8)
        vXor(sBuf+1+4, sDivKey+4, 4);
    vDesEcb(TRI_ENCRYPT, (iDataLen+2+7)/8*8, sBuf, sSessionKey, pOutData);
    *piOutLen = (iDataLen+2+7)/8*8;
	return(0);
}

// �ù�����Կ����Mac(�ű�)
// in  : iKey1Index    : ��Կ����
//       pKey1         : LMK���ܺ���Կ, NULL��ʾʹ��������Կ
//       iDiv1Level    : ��Կ��ɢ����(0-3)
//       pDiv1Data     : ��Կ��ɢ����[8*n]
//       pATC          : �������к�[2]
//       iDataLen      : ���ݳ���
//       pData         : ����
// out : pMac          : Mac[8]
// ret : KFR_OK        : �ɹ�
//       ����          : ʧ��
int iKeyScriptMac0(int iKey1Index, void *pKey1, int iDiv1Level, void *pDiv1Data, void *pATC, int iDataLen, void *pData, void *pMac)
{
    char  sBuf[64];
	char  sDivKey[16];
	char  sSessionKey[16];
	char  sMac[8];
	int   iRet;

    if(iDiv1Level<1 || iDiv1Level>3)
        return(HSMERR_DIV_NUM);
    iRet = iDivKey(iKey1Index, pKey1, (uchar)iDiv1Level, pDiv1Data, sDivKey);
    if(iRet)
        return(iRet);

    // cal session key
	memset(sBuf, 0, 8);
	memcpy(sBuf+6, pATC, 2);
	_vDes(TRI_ENCRYPT, sBuf, sDivKey, sSessionKey);
	vXor(sBuf+6, "\xFF\xFF", 2);
	_vDes(TRI_ENCRYPT, sBuf, sDivKey, sSessionKey+8);

    // cal mac
	memset(sMac, 0, 8);
    iRet = iMac3DesCBCRetail(sSessionKey, pData, (uchar)iDataLen, sMac);
    if(iRet)
        return(HSMERR_INTERNAL);
    memcpy(pMac, sMac, 8);
	return(0);
}

// ����������Կת��(����׼���ļ�����)
// in  : iKey1Index : ������Կ����
//       pKey1      : LMK���ܺ�������Կ, NULL��ʾʹ��������Կ
//       iDiv1Level : ������Կ��ɢ����(0-3)
//       pDiv1Data  : ������Կ��ɢ����[8*n]
//       pTermRand  : �ն������[8]
//       pCardData  : ��Ƭ����[28]
//       iKey2Index : Ӧ����Կ����
//       pKey2      : LMK���ܺ�Ӧ����Կ, NULL��ʾʹ��������Կ
//       iDiv2Level : Ӧ����Կ��ɢ����(0-3)
//       pDiv2Data  : Ӧ����Կ��ɢ����[8*n]
//       iDataLen   : ԭ�������ݳ���, ����Ϊ8�ı���
//       pData      : ԭ��������
// out : pOut       : ת���ܺ�����, ���ȵ���ԭ�������ݳ���
// ret : KFR_OK     : �ɹ�
//       ����       : ʧ��
// Note: ����ԭΪӦ����Կ����, ��������Կ�Ĺ�����Կת����
int iKeyReEncData0(int iKey1Index, void *pKey1, int iDiv1Level, void *pDiv1Data, void *pTermRand, void *pCardData, int iKey2Index, void *pKey2, int iDiv2Level, void *pDiv2Data, int iDataLen, void *pData, void *pOut)
{
    unsigned char ucSessionKeyFlag = 1; // �Ƿ����������Կ��־(0:������������Կ 1:����������Կ)
    //QINGBO
    //unsigned char ucDivMode = 1;        // ��Ƭ��ɢģʽ(0:����ɢ 1:��ɢ)

	unsigned char sKey[16];
	unsigned char sSessionEncKey[16];
	unsigned char sBuf[256];
	int   iRet;

    iRet = iDivKey(iKey1Index, pKey1, (uchar)iDiv1Level, pDiv1Data, sKey);
    if(iRet)
        return(iRet);
	if(ucSessionKeyFlag) {
		// cal session key:Enc Key �������ݼ���
		iRet = iGpDivChannelKey(4/*4:Enc(����)*/, sg_ucDivMode, pTermRand, pCardData, sKey, sSessionEncKey);
		if(iRet)
			return(HSMERR_INTERNAL);
	} else {
		// ������������Կ
		memcpy(sSessionEncKey, sKey, 16);
	}
    iRet = iDivKey(iKey2Index, pKey2, (uchar)iDiv2Level, pDiv2Data, sKey);
    if(iRet)
        return(iRet);

    vDesEcb(TRI_DECRYPT, iDataLen, pData, sKey, sBuf);
    vDesEcb(TRI_ENCRYPT, iDataLen, sBuf, sSessionEncKey, pOut);

	return(0);
}

// ����������Կת��(����׼���ļ�����)
// in  : iKey1Index : ������Կ����
//       pKey1      : LMK���ܺ�������Կ, NULL��ʾʹ��������Կ
//       iDiv1Level : ������Կ��ɢ����(0-3)
//       pDiv1Data  : ������Կ��ɢ����[8*n]
//       pTermRand  : �ն������[8]
//       pCardData  : ��Ƭ����[28]
//       iKey2Index : Ӧ����Կ����
//       pKey2      : LMK���ܺ�Ӧ����Կ, NULL��ʾʹ��������Կ
//       iDiv2Level : Ӧ����Կ��ɢ����(0-3)
//       pDiv2Data  : Ӧ����Կ��ɢ����[8*n]
//       pPinBlock  : ԭ���ܺ������[8]
// out : pOut       : ת���ܺ������[8]
// ret : KFR_OK     : �ɹ�
//       ����       : ʧ��
// Note: ����ԭΪӦ����Կ����, ��������Կ�Ĺ�����Կת����
int iKeyReEncPin0(int iKey1Index, void *pKey1, int iDiv1Level, void *pDiv1Data, void *pTermRand, void *pCardData, int iKey2Index, void *pKey2, int iDiv2Level, void *pDiv2Data, void *pPinBlock, void *pOut)
{
    unsigned char ucSessionKeyFlag = 1; // �Ƿ����������Կ��־(0:������������Կ 1:����������Կ)
    //QINGBO
    //unsigned char ucDivMode = 1;        // ��Ƭ��ɢģʽ(0:����ɢ 1:��ɢ)
	unsigned char sKey[16];
	unsigned char sSessionEncKey[16];
	int   iPinLen;
	unsigned char sPinBlock[8];
	unsigned char sBuf[8], szBuf[16];
	int   iRet;

    iRet = iDivKey(iKey1Index, pKey1, (uchar)iDiv1Level, pDiv1Data, sKey);
    if(iRet)
        return(iRet);
	if(ucSessionKeyFlag) {
		// cal session key:Enc Key �������ݼ���
		iRet = iGpDivChannelKey(4/*4:Enc(����)*/, sg_ucDivMode, pTermRand, pCardData, sKey, sSessionEncKey);
		if(iRet)
			return(HSMERR_INTERNAL);
	} else {
		// ������������Կ
		memcpy(sSessionEncKey, sKey, 16);
	}
    iRet = iDivKey(iKey2Index, pKey2, (uchar)iDiv2Level, pDiv2Data, sKey);
    if(iRet)
        return(iRet);

    _vDes(TRI_DECRYPT, pPinBlock, sKey, sBuf);
    if((sBuf[0]&0xF0)!=0x10 && (sBuf[0]&0xF0)!=0x20)
        return(HSMERR_PIN_BLOCK); // ֻ֧�ָ�ʽ1���ʽ2
    iPinLen = sBuf[0] & 0x0F;
    if(iPinLen<4 || iPinLen>12)
        return(HSMERR_PIN_BLOCK);
    sPinBlock[0] = 0x20; // �̶�ת���ɸ�ʽ2
    sPinBlock[0] |= iPinLen;    
    vOneTwo(sBuf+1, 7, szBuf);
    memset(szBuf+iPinLen, 'F', 14-iPinLen);
    vTwoOne(szBuf, 14, sPinBlock+1);

    _vDes(TRI_ENCRYPT, sPinBlock, sSessionEncKey, pOut);
	return(0);
}

// ������ʱ��Կ(����׼��)
// in  : iKey1Index : Ӧ����Կ����
//       pKey1      : LMK���ܺ�Ӧ����Կ, NULL��ʾʹ��������Կ
//       iDiv1Level : Ӧ����Կ��ɢ����(0-3)
//       pDiv1Data  : Ӧ����Կ��ɢ����[8*n]
// out : pOut       : ��Ӧ����Կ���ܺ�������ʱ��Կ[16]+cks[8]
// ret : KFR_OK     : �ɹ�
//       ����       : ʧ��
int iKeyDpGenDesKey0(int iKey1Index, void *pKey1, int iDiv1Level, void *pDiv1Data, void *pOut)
{
	unsigned char sKey[16];
	unsigned char sTmpKey[16];
	int i;
	int iRet;

    iRet = iDivKey(iKey1Index, pKey1, (uchar)iDiv1Level, pDiv1Data, sKey);
    if(iRet)
        return(iRet);
    for(i=0; i<16; i++)
        sTmpKey[i] = rand() % 256;
    vDesEcb(TRI_ENCRYPT, 16, sTmpKey, sKey, pOut);
    memset(sKey, 0, 8);
    _vDes(TRI_ENCRYPT, sKey, sTmpKey, (char *)pOut+16);
    return(0);
}

// Mac����(����׼��)
// in  : iKey1Index  : Ӧ����Կ����
//       pKey1       : LMK���ܺ�Ӧ����Կ, NULL��ʾʹ��������Կ
//       iDiv1Level  : Ӧ����Կ��ɢ����(0-3)
//       pDiv1Data   : Ӧ����Կ��ɢ����[8*n]
//       pMacKey     : ��Ӧ����Կ���ܺ��Mac��Կ
//       pIv         : ��ʼ����[8]
//       iDataLen    : ���ݳ���
//       pData       : ����
//       iBlockFlag  : ���ݿ��־, 1:��һ����м�� 2:Ψһ������һ��
// out : pMac        : Mac[8]
// ret : KFR_OK      : �ɹ�
//       ����        : ʧ��
// Note: ���ݳ��Ȳ���8�ı��������Զ���0
int iKeyDpCalMac0(int iKey1Index, void *pKey1, int iDiv1Level, void *pDiv1Data, void *pMacKey, void *pIv, int iDataLen, void *pData, void *pMac, int iBlockFlag)
{
	unsigned char sKey[16];
	unsigned char sTmpKey[16];
	int i;
	int iRet;

    if(iDataLen % 8)
        return(HSMERR_DATAIN_LEN);
    iRet = iDivKey(iKey1Index, pKey1, (uchar)iDiv1Level, pDiv1Data, sKey);
    if(iRet)
        return(iRet);
    vDesEcb(TRI_DECRYPT, 16, pMacKey, sKey, sTmpKey);
    memcpy(pMac, pIv, 8);
    for(i=0; i<iDataLen; i+=8) {
		vXor(pMac, (char *)pData+i, iDataLen-i>8?8:iDataLen-i);
		_vDes(ENCRYPT, pMac, sTmpKey, pMac);
		if(iBlockFlag==2 && (i+8==iDataLen)) {
		    // ���һ���
    		_vDes(DECRYPT, pMac, sTmpKey+8, pMac);
    		_vDes(ENCRYPT, pMac, sTmpKey, pMac);
		}
    }
    return(0);
}
// Mac����(����׼��)
// ����iKeyDpCalMac0()ֻʵ����cps�淶Ҫ���Mac�㷨, ������֧��ȫ����ʿͨ���ܻ�Mac�㷨
// in  : iKey1Index  : Mac��Կ����
//       pKey1       : LMK���ܺ�Mac��Կ, NULL��ʾʹ��������Կ
//       iDiv1Level  : Mac��Կ��ɢ����(0-3)
//       pDiv1Data   : Mac��Կ��ɢ����[8*n]
//       pMacKey     : ��Mac��Կ���ܺ����ʱMac��Կ, NULL��ʾֱ��ʹ��Mac��Կ����
//       iAlgoFlag   : �㷨, 1:ANSI99(full) 2:ANSI919(retail) 3:XOR 4:PBOC3DES
//                     �㷨1��2��3�����ݳ��Ȳ���8�ı���ʱ, ��0x00��8�ı���
//                     �㷨4�Ȳ�0x80, ֮��������Ȳ���8�ı���ʱ, ��0x00��8�ı���
//       pIv         : ��ʼ����[8]
//       iDataLen    : ���ݳ���
//       pData       : ����
//       iBlockFlag  : ���ݿ��־, 1:��һ����м�� 2:Ψһ������һ��
// out : pMac        : Mac[8]
// ret : KFR_OK      : �ɹ�
//       ����        : ʧ��
int iKeyDpCalMacFull0(int iKey1Index, void *pKey1, int iDiv1Level, void *pDiv1Data, void *pMacKey, int iDiv2Level, void *pDiv2Data, int iAlgoFlag, void *pIv, int iDataLen, void *pData, void *pMac, int iBlockFlag)
{
	unsigned char sKey[16];
	unsigned char sTmpKey[16];
    unsigned char sTmpData[4096+20]; // Ϊͳһ�㷨, ���øû�����, ���ڵõ������β��������
	int i;
	int iRet;

    iRet = iDivKey(iKey1Index, pKey1, (uchar)iDiv1Level, pDiv1Data, sKey);
    if(iRet)
        return(iRet);
    memcpy(sTmpKey, sKey, 16);
    if(pMacKey) {
        vDesEcb(TRI_DECRYPT, 16, pMacKey, sTmpKey, sKey);
        iRet = iDivKey2(sKey, (uchar)iDiv2Level, pDiv2Data, sTmpKey);
        if(iRet)
            return(iRet);
    }

    if(iBlockFlag==1 && iDataLen%8)
        return(HSMERR_DATAIN_LEN); // �������һ��ʱ, ���ݳ��ȱ���Ϊ8�ı���
    if(iAlgoFlag<1 || iAlgoFlag>4)
        return(HSMERR_MODE);
    if(iDataLen > 4096)
        return(HSMERR_DATAIN_LEN); // ���ݳ���

    memset(sTmpData, 0, sizeof(sTmpData));
    memcpy(sTmpData, pData, iDataLen);
    if(iAlgoFlag==4 && iBlockFlag==2)
        sTmpData[iDataLen++] = 0x80; // ��������һ�鲢��Ϊ�㷨4, ����0x80
    iDataLen = (iDataLen+7) / 8 * 8;

    memcpy(pMac, pIv, 8);
    switch(iAlgoFlag) {
    case 1: // ANSI99(full)
        for(i=0; i<iDataLen; i+=8) {
	    	vXor(pMac, (char *)sTmpData+i, 8);
		    _vDes(TRI_ENCRYPT, pMac, sTmpKey, pMac);
        }
        break;
    case 2: // ANSI919(retail)
    case 4: // PBOC3DES
        for(i=0; i<iDataLen; i+=8) {
	    	vXor(pMac, (char *)sTmpData+i, 8);
		    _vDes(ENCRYPT, pMac, sTmpKey, pMac);
    		if(iBlockFlag==2 && (i+8==iDataLen)) {
	    	    // ���һ���
    	    	_vDes(DECRYPT, pMac, sTmpKey+8, pMac);
    		    _vDes(ENCRYPT, pMac, sTmpKey, pMac);
		    }
        }
        break;
    case 3: // XOR
        for(i=0; i<iDataLen; i+=8)
	    	vXor(pMac, (char *)sTmpData+i, 8);
		if(iBlockFlag == 2)
		    _vDes(TRI_ENCRYPT, pMac, sTmpKey, pMac); // ���һ���
        break;
    }
    return(0);
}

// ��������׼��DES��Կ����(����׼��)
// in  : iKey1Index  : TK��Կ����
//       pKey1       : LMK���ܺ�Tk��Կ, NULL��ʾʹ��������Կ
//       iDiv1Level  : TK��Կ��ɢ����(0-3)
//       pDiv1Data   : TK��Կ��ɢ����[8*n]
//       iKey2Index  : Ӧ����Կ����
//       pKey2       : LMK���ܺ�Ӧ����Կ, NULL��ʾʹ��������Կ
//       iDiv2Level  : Ӧ����Կ��ɢ����(0-3)
//       pDiv2Data   : Ӧ����Կ��ɢ����[8*n]
// out : pOut        : OutKey[16]+Cks[8]
// ret : KFR_OK     : �ɹ�
//       ����       : ʧ��
int iKeyDpCalDesKey0(int iKey1Index, void *pKey1, int iDiv1Level, void *pDiv1Data, int iKey2Index, void *pKey2, int iDiv2Level, void *pDiv2Data, void *pOut)
{
    unsigned char ucSessionKeyFlag = 1; // �Ƿ����������Կ��־(0:������������Կ 1:����������Կ)
	unsigned char sKek[16];    // ���ڼ��ܵ���Կ
	unsigned char sOutKey[16]; // �����ܵ���Կ
	unsigned char szBuf[100];
	int   iRet;

    if(iDiv2Level<1 || iDiv2Level>3)
        return(HSMERR_DIV_NUM);
    iRet = iDivKey(iKey1Index, pKey1, (uchar)iDiv1Level, pDiv1Data, sKek);
    if(iRet)
        return(iRet);
    iRet = iDivKey(iKey2Index, pKey2, (uchar)iDiv2Level, pDiv2Data, sOutKey);
    if(iRet)
        return(iRet);

    vDesEcb(TRI_ENCRYPT, 16, sOutKey, sKek, pOut);
	memset(szBuf, 0, 8);
	_vDes(TRI_ENCRYPT, szBuf, sOutKey, (char *)pOut+16);
	memset(sOutKey, 0, 16);

	return(0);
}

// ��������׼��RSA��Կ����(����׼��)
// in  : iKey1Index  : TK��Կ����
//       pKey1       : LMK���ܺ�TK��Կ, NULL��ʾʹ��������Կ
//       iDiv1Level  : TK��Կ��ɢ����(0-3)
//       pDiv1Data   : TK��Կ��ɢ����[8*n]
//       iRsaKeyLen  : RSA��Կ����, ��λ:�ֽ�, 64-248, ����Ϊż��
//       lE          : ����ָ��ֵ, 3����65537
// out : pOut        : RSA��Կ��Կ����[n]+D����[n1]+P����[n2]+Q����[n2]+dP����[n2]+dQ����[n2]+qInv����[n2]
//                     n = iRsaKeyLen, n1 = (iRsaKeyLen+8)/8*8, n2 = (iRsaKeyLen/2+8)/8*8
// ret : KFR_OK      : �ɹ�
//       ����        : ʧ��
int iKeyDpCalRsaKey0(int iKey1Index, void *pKey1, int iDiv1Level, void *pDiv1Data, int iRsaKeyLen, long lE, void *pOut)
{
	unsigned char sKek[16];    // ���ڼ��ܵ���Կ
	unsigned char sBuf[300];
#ifndef USE_KEY_POOL
	R_RSA_PROTO_KEY   ProtoKey;
#endif
	R_RSA_PUBLIC_KEY  PublicKey;
	R_RSA_PRIVATE_KEY PrivateKey;
	int   iItem1Len, iItem2Len;
	int   iRet;

    if(iRsaKeyLen<MIN_RSA_KEY_LEN || iRsaKeyLen>MAX_RSA_KEY_LEN)
        return(HSMERR_KEY_LEN);
    if(lE!=3 && lE!=65537L)
        return(HSMERR_RSA_E);
    iRet = iDivKey(iKey1Index, pKey1, (uchar)iDiv1Level, pDiv1Data, sKek);
    if(iRet)
        return(iRet);
        
    // ����RSA��Կ
#ifndef USE_KEY_POOL
    ProtoKey.bits = iRsaKeyLen * 8;
	ProtoKey.useFermat4 = lE == 3 ? 0 : 1;
    iRet = R_GeneratePEMKeys(&PublicKey, &PrivateKey, &ProtoKey);
#else
    iRet = iRsaKeyPoolGetKey(iRsaKeyLen, lE, &PublicKey, &PrivateKey);
#endif
    if(iRet)
        return(HSMERR_INTERNAL);
    
	memcpy(pOut, PrivateKey.modulus+sizeof(PrivateKey.modulus)-iRsaKeyLen, iRsaKeyLen);
    iItem1Len = (iRsaKeyLen+1+7)/8*8; // D����
    memset(sBuf, 0, sizeof(sBuf));
	memcpy(sBuf, PrivateKey.exponent+sizeof(PrivateKey.exponent)-iRsaKeyLen, iRsaKeyLen);
	sBuf[iRsaKeyLen] = 0x80;
    vDesEcb(TRI_ENCRYPT, iItem1Len, sBuf, sKek, (char *)pOut+iRsaKeyLen);

    iItem2Len = (iRsaKeyLen/2+1+7)/8*8; // ��Կ����(p��q��)����
    memset(sBuf, 0, sizeof(sBuf));
	sBuf[iRsaKeyLen/2] = 0x80;
	memcpy(sBuf, PrivateKey.prime[0]+sizeof(PrivateKey.prime[0])-iRsaKeyLen/2, iRsaKeyLen/2);
    vDesEcb(TRI_ENCRYPT, iItem2Len, sBuf, sKek, (char *)pOut+iRsaKeyLen+iItem1Len+iItem2Len*0);
	memcpy(sBuf, PrivateKey.prime[1]+sizeof(PrivateKey.prime[1])-iRsaKeyLen/2, iRsaKeyLen/2);
    vDesEcb(TRI_ENCRYPT, iItem2Len, sBuf, sKek, (char *)pOut+iRsaKeyLen+iItem1Len+iItem2Len*1);
	memcpy(sBuf, PrivateKey.primeExponent[0]+sizeof(PrivateKey.primeExponent[0])-iRsaKeyLen/2, iRsaKeyLen/2);
    vDesEcb(TRI_ENCRYPT, iItem2Len, sBuf, sKek, (char *)pOut+iRsaKeyLen+iItem1Len+iItem2Len*2);
	memcpy(sBuf, PrivateKey.primeExponent[1]+sizeof(PrivateKey.primeExponent[1])-iRsaKeyLen/2, iRsaKeyLen/2);
    vDesEcb(TRI_ENCRYPT, iItem2Len, sBuf, sKek, (char *)pOut+iRsaKeyLen+iItem1Len+iItem2Len*3);
	memcpy(sBuf, PrivateKey.coefficient+sizeof(PrivateKey.coefficient)-iRsaKeyLen/2, iRsaKeyLen/2);
    vDesEcb(TRI_ENCRYPT, iItem2Len, sBuf, sKek, (char *)pOut+iRsaKeyLen+iItem1Len+iItem2Len*4);
    	
	return(0);
}

// ���ݼӽ���
// in  : iKey1Index  : �ӽ�����Կ����
//       pKey1       : LMK���ܺ�ӽ�����Կ, NULL��ʾʹ��������Կ
//       iDiv1Level  : �ӽ�����Կ��ɢ����(0-3)
//       pDiv1Data   : �ӽ�����Կ��ɢ����[8*n]
//       pKey2       : ���ӽ�����Կ���ܺ����Կ, NULL��ʾֱ��ʹ�üӽ�����Կ����
//       iDiv2Level  : ���ܺ���Կ��ɢ����(0-3)
//       pDiv2Data   : ���ܺ���Կ��ɢ����[8*n]
//       iEncDecFlag : �ӽ��ܱ�־, 1:���� 0:����
//       iPadFlag    : ���ģʽ, ����ʱ��Ч, 1:80+00(������8�ı���ʱ�����) 2:80+00(�������), 3:00(������8�ı���ʱ�����), 4:00(�������)
//       pIv         : ��ʼ����[8], NULL��ʾΪECBģʽ, ����ΪCBCģʽ
//       iDataLen    : ���ݳ���
//       pData       : ����
// out : piOutLen    : �������
//       pOutData    : ���
// ret : KFR_OK      : �ɹ�
//       ����        : ʧ��
int iKeyEncDec0(int iKey1Index, void *pKey1, int iDiv1Level, void *pDiv1Data, void *pKey2, int iDiv2Level, void *pDiv2Data, int iEncDecFlag, int iPadFlag, void *pIv, int iDataLen, void *pData, int *piOutLen, void *pOutData)
{
	unsigned char sKey[16];
	unsigned char sTmpKey[16];
    unsigned char sTmpData[4096+20]; // Ϊͳһ�㷨, ���øû�����, ���ڵõ������β��������
	int iRet;

    iRet = iDivKey(iKey1Index, pKey1, (uchar)iDiv1Level, pDiv1Data, sKey);
    if(iRet)
        return(iRet);
    memcpy(sTmpKey, sKey, 16);
    if(pKey2) {
        vDesEcb(TRI_DECRYPT, 16, pKey2, sTmpKey, sKey);
        iRet = iDivKey2(sKey, (uchar)iDiv2Level, pDiv2Data, sTmpKey);
        if(iRet)
            return(iRet);
    }
    if(iEncDecFlag!=1 && iEncDecFlag!=0)
        return(HSMERR_MODE);
    if(iPadFlag<1 || iPadFlag>4)
        return(HSMERR_MODE);
    if(iDataLen > 4096)
        return(HSMERR_DATAIN_LEN); // ���ݳ���
    if(iEncDecFlag==0 && iDataLen%8)
        return(HSMERR_DATAIN_LEN);

    memset(sTmpData, 0, sizeof(sTmpData));
    memcpy(sTmpData, pData, iDataLen);
    if(iEncDecFlag == 1) {
        // ����
        if(iPadFlag==1 && iDataLen%8)
            sTmpData[iDataLen++] = 0x80;
        if(iPadFlag == 2)
            sTmpData[iDataLen++] = 0x80;
        if(iPadFlag == 4)
            sTmpData[iDataLen++] = 0x00;
        iDataLen = (iDataLen+7) / 8 * 8;
    }
    *piOutLen = iDataLen;
    if(pIv == NULL) {
        // pIv==NULL��ʾʹ��ECBģʽ
        if(iEncDecFlag == 1)
            vDesEcb(TRI_ENCRYPT, iDataLen, sTmpData, sTmpKey, pOutData);
        else
            vDesEcb(TRI_DECRYPT, iDataLen, sTmpData, sTmpKey, pOutData);
    } else {
        // ʹ��CBCģʽ
        if(iEncDecFlag == 1)
            vDesCbc(TRI_ENCRYPT, pIv, iDataLen, sTmpData, sTmpKey, pOutData);
        else
            vDesCbc(TRI_DECRYPT, pIv, iDataLen, sTmpData, sTmpKey, pOutData);
    }

    return(0);
}

// ECBģʽ�ӽ�������
// in  : iKey1Index  : ��Կ����
//       pKey1       : LMK���ܺ���Կ, NULL��ʾʹ��������Կ
//       iDiv1Level  : ��Կ��ɢ����(0-3)
//       pDiv1Data   : ��Կ��ɢ����[8*n]
//       iFlag       : �ӽ��ܱ�־ 0:���� 1:����
//       iDataLen    : Ҫ�ӽ��ܵ����ݳ���, ����Ϊ8�ı���
//       pData       : Ҫ�ӽ��ܵ�����
// out : pOut        : �ӽ��ܺ������
// ret : KFR_OK     : �ɹ�
//       ����       : ʧ��
int iKeyEncDecEcb0(int iKey1Index, void *pKey1, int iDiv1Level, void *pDiv1Data, int iFlag, int iDataLen, void *pData, void *pOut)
{
    char sKey[16];
    int  iRet;
    if(iDataLen % 8)
        return(HSMERR_DATAIN_LEN);
    iRet = iDivKey(iKey1Index, pKey1, (uchar)iDiv1Level, pDiv1Data, sKey);
    if(iRet)
        return(iRet);
    if(iFlag)
        vDesEcb(TRI_ENCRYPT, iDataLen, pData, sKey, pOut);
    else
        vDesEcb(TRI_DECRYPT, iDataLen, pData, sKey, pOut);
    return(0);
}

// ͨ�ü���������Կת��
// ��1�Ĳ���Ϊ������Կ, ��2�Ĳ���Ϊ������Կ
// in  : iKey1Index  : ��Կ����
//       pKey1       : ���ܺ���Կ, NULL��ʾʹ��������Կ
//       iDiv1Level  : ��Կ��ɢ����(0-3)
//       pDiv1Data   : ��Կ��ɢ����[8*n]
//       iKey1IndexP : ������Կ����, -1��ʾ��ʹ�ñ�����Կ
//       pKey1P      : ���ܺ󱣻���Կ, NULL��ʾʹ��������Կ
//       iDiv1LevelP : ������Կ��ɢ����(0-3)
//       pDiv1DataP  : ������Կ��ɢ����[8*n]
//       pIv1        : Iv, NULL��ʾ��ECBģʽ
//       iKey2Index  : ��Կ����
//       pKey2       : ���ܺ���Կ, NULL��ʾʹ��������Կ
//       iDiv2Level  : ��Կ��ɢ����(0-3)
//       pDiv2Data   : ��Կ��ɢ����[8*n]
//       iKey2IndexP : ������Կ����, -1��ʾ��ʹ�ñ�����Կ
//       pKey2P      : ���ܺ󱣻���Կ, NULL��ʾʹ��������Կ
//       iDiv2LevelP : ������Կ��ɢ����(0-3)
//       pDiv2DataP  : ������Կ��ɢ����[8*n]
//       pIv2        : Iv, NULL��ʾ��ECBģʽ
//       iDataLen    : ԭ�������ݳ���, ����Ϊ8�ı���
//       pData       : ԭ��������
// out : pOut        : ת���ܺ�����, ���ȵ���ԭ�������ݳ���
// ret : KFR_OK      : �ɹ�
//       ����        : ʧ��
int iKeyDecEncData0(int iKey1Index, void *pKey1, int iDiv1Level, void *pDiv1Data, int iKey1IndexP, void *pKey1P, int iDiv1LevelP, void *pDiv1DataP, void *pIv1,
                    int iKey2Index, void *pKey2, int iDiv2Level, void *pDiv2Data, int iKey2IndexP, void *pKey2P, int iDiv2LevelP, void *pDiv2DataP, void *pIv2,
                    int iDataLen, void *pData, void *pOut)
{
	unsigned char sKey1[16], sKey2[16]; // ������Կ������܄1�7
    unsigned char sKey[16], sTmp[16];
	unsigned char sBuf[2020];
    int   i;
	int   iRet;

    if(iDataLen % 8)
        return(HSMERR_DATAIN_LEN);
    for(i=0; i<2; i++) {
        if(pKey1) {
            // Ӧ����Կ��LMK�򱣻���Կ����
            if(iKey1IndexP == -1) {
                // Ӧ����Կ��LMK����
                iRet = iDivKey(iKey1Index, pKey1, (uchar)iDiv1Level, pDiv1Data, sKey);
                if(iRet)
                    return(iRet);
            } else {
                // Ӧ����Կ��������Կ����
                iRet = iDivKey(iKey1IndexP, pKey1P, (uchar)iDiv1LevelP, pDiv1DataP, sTmp); // sTmpΪ������Կ
                if(iRet)
                    return(iRet);
                vDesEcb(TRI_DECRYPT, 16, pKey1, sTmp, sKey); // sKeyΪӦ����Կ
                vDesEcb(TRI_ENCRYPT, 16, sKey, sg_aMasterKey[sg_iMasterKeyIndex].sKey, sTmp); // sTmpΪLMK���ܺ�Ӧ����Կ
                iRet = iDivKey(iKey1Index, sTmp, (uchar)iDiv1Level, pDiv1Data, sKey); // sKeyΪ��ɢ��Ӧ����Կ
                if(iRet)
                    return(iRet);    
            }
        } else {
            // Ӧ����ԿΪ������Կ
            iRet = iDivKey(iKey1Index, NULL, (uchar)iDiv1Level, pDiv1Data, sKey);
            if(iRet)
                return(iRet);
        }
        if(i == 0) {
            memcpy(sKey1, sKey, 16);
            iKey1Index = iKey2Index;
            pKey1 = pKey2;
            iDiv1Level = iDiv2Level;
            pDiv1Data = pDiv2Data;
            iKey1IndexP = iKey2IndexP;
            pKey1P = pKey2P;
            iDiv1LevelP = iDiv2LevelP;
            pDiv1DataP = pDiv2DataP;
        } else {
            memcpy(sKey2, sKey, 16);
        }
    }
    // sKey1Ϊ������Կ, sKey2Ϊ������Կ
    if(iDataLen > sizeof(sBuf)-20)
        return(HSMERR_INTERNAL);
    // ����
    if(pIv1)
        vDesCbc(TRI_DECRYPT, pIv1, iDataLen, pData, sKey1, sBuf);
    else
        vDesEcb(TRI_DECRYPT, iDataLen, pData, sKey1, sBuf);
    // ����
    if(pIv2)
        vDesCbc(TRI_ENCRYPT, pIv2, iDataLen, sBuf, sKey2, pOut);
    else
        vDesEcb(TRI_ENCRYPT, iDataLen, sBuf, sKey2, pOut);
	return(0);
}

// ���ķ�ʽ����ĳλ��DES��Կ
// ret : 0 : OK
// Note: δ��������
int iHsmSetDesKey(int iIndex, void *pKey)
{
    uchar szDateTime[15];
    if(iIndex > sg_iMaxDesKeyNum)
        return(1);
    memcpy(sg_aDesKey[iIndex].sKey, pKey, 16);
    _vGetTime(szDateTime);
    sprintf(sg_aDesKey[iIndex].szInf, "����ԿΪ��������, ʱ��:%s", szDateTime);
    sg_aDesKey[iIndex].ucOKFlag = 1;
    return(0);
}

// QINGBO
// ���÷�ɢģʽ����Ƭ��ɢģʽ(0:����ɢ 1:��ɢ)
void vKeyCalSetDivMode(int iDivMode)
{
    sg_ucDivMode = iDivMode;
}
// �޸�KMC��Կֵ
void vHsmChangeKmcKey(uchar *psNewKey)
{
    memcpy(sg_aDesKey[1].sKey, psNewKey, 16);
}

