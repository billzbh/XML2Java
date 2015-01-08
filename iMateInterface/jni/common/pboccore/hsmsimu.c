/**************************************
File name     : HSMSIMU.C
Function      : 仿真加密机
Author        : Yu Jun
First edition : Mar 18th, 2011
Modified      : 
Note          : 20110323，由多进程方式改成多线程方式，增加了变量KeyMutex做密钥更新控制，
                          由于时间关系，只做了密钥更新时的互斥控制，使用密钥时没有做互斥控制，
                          在用软加密机做测试时可以这样，如果以后要用该仿真加密机做正式用途，
                          需要全面审核密钥使用时的互斥控制。
                20110706, 重写密钥处理接口, 支持外部传入密钥运算
				20140519, 计算外部认证密文时自动尝试212 202分散方式, 并标记到变量sg_iGpDivMethod中
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
static unsigned char sg_ucDivMode = 1; // 卡片分散模式(0:不分散 1:分散)

static int sg_iMasterKeyIndex;      // 用来加密密钥的主密钥索引
static int sg_iGpDivMethod = 212;   // 用于标记卡片KMC分散方式, 212:Gp212分散方式 202:Gp202分散方式

typedef struct {
    char szInf[40+1];               // 密钥说明
    char ucOKFlag;                  // 密钥可用标志，0:密钥不可用 1:OK
    char sKey[16];                  // 密钥
} stDesKey;
typedef struct {
    char szInf[40+1];               // 密钥说明
    char ucOKFlag;                  // 密钥可用标志，0:密钥不可用 1:D值可用 2:CRT可用 3:D值和CRT都可用
    char sPassword[8];              // 口令
    int  iKeyLen;                   // 密钥长度，字节为单位，必须为偶数
    long lE;                        // E，3或65537
    char sN[MAX_RSA_KEY_LEN];       // N
    char sD[MAX_RSA_KEY_LEN];       // D
    char sP[MAX_RSA_KEY_LEN/2];     // P
    char sQ[MAX_RSA_KEY_LEN/2];     // Q
    char sDP[MAX_RSA_KEY_LEN/2];    // dP
    char sDQ[MAX_RSA_KEY_LEN/2];    // dQ
    char sQINV[MAX_RSA_KEY_LEN/2];  // qInv
} stRsaKey;

typedef struct {
    int  iIndex;                    // 扫描的行的密钥索引号
    char *pszName;                  // 扫描的行的名字(去除了index***_部分)，例如"key"、"inf"、"n"、"e"...
    char *pszContent;               // 扫描的行的内容
} stRow; // 一行内容的结构，例如【index012_e = 65537】，解析后,stRow.iIndex=12 stRow.psName指向"e" stRow.psContent指向"65537"

static stDesKey sg_aMasterKey[3];       // 主密钥存储区
static stDesKey sg_aDesKey[1000];       // DES密钥存储区
static stRsaKey sg_aRsaKey[20];         // RSA密钥存储区
static int      sg_iMaxMasterKeyNum;    // 最多主密钥个数
static int      sg_iMaxDesKeyNum;       // 最多des密钥个数
static int      sg_iMaxRsaKeyNum;       // 最多rsa密钥个数
static char     sg_szKeyFileName[256];  // 密钥文件名字


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

// 奇偶重整密钥
// in  : iFlag : 奇偶标志, 0:偶重整 1:奇重整
//       pKey  : 密钥
// out : pKey  : 重整后密钥
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

// 解析一行数据
// in  : pszRow : 一行内容
// out : pRow   : 解析好的内容
// ret : 0      : OK
//       1      : 解析错误
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

// ECB模式加解密数据
// in  : iFlag     : ENCRYPT or DECRYPT or TRI_ENCRYPT or TRI_DECRYPT
//       iLen      : 长度, 必须为8的倍数
//       pInData   : 输入数据
//       pKey      : 密钥
// out : pOutData  : 输出数据，长度等于输入数据长度
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
// CBC模式加解密数据
// in  : iFlag     : ENCRYPT or DECRYPT or TRI_ENCRYPT or TRI_DECRYPT
//       pIv       : IV
//       iLen      : 长度, 必须为8的倍数
//       pInData   : 输入数据
//       pKey      : 密钥
// out : pOutData  : 输出数据，长度等于输入数据长度
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

// 从密钥文件读取密钥
// ret : 0 : OK
//       其它       : 参考hsmsimu.h错误码
static int iKeyFileLoad(void)
{
    FILE *fp;
    char     szKeyBuf[1024], *p;
    enum     {SecNone, SecMasterKey, SecDesKey, SecRsaKey} eCurSection; // 正在扫描的密钥区
    stRow    Row;   // 当前行内容
    int      i;
    int      iRet;
    
    if(sg_szKeyFileName[0] == 0)
        return(0);

    memset(sg_aMasterKey, 0, sizeof(sg_aMasterKey));
    memset(sg_aDesKey, 0, sizeof(sg_aDesKey));
    memset(sg_aRsaKey, 0, sizeof(sg_aRsaKey));
    fp = fopen(sg_szKeyFileName, "rt");
    if(fp == NULL) {
        return(0); // 打开文件失败，认为密钥为空，不报错
    }
    
    eCurSection = SecNone;
    for(;;) {
        memset(szKeyBuf, 0, sizeof(szKeyBuf));
        p = fgets(szKeyBuf, sizeof(szKeyBuf), fp);
        if(p == NULL)
            break;
        if(szKeyBuf[0] == '#')
            continue; // 第一个字节为‘#’表示注释
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

    // 检测RSA密钥区密钥完整性
    memset(szKeyBuf, 0, sizeof(szKeyBuf)); // 用于比较，看RSA密钥是否为全0
    for(i=0; i<sg_iMaxRsaKeyNum; i++) {
        sg_aRsaKey[i].ucOKFlag = 0x03;  // 密钥可用标志，0:密钥不可用 1:D值可用 2:CRT可用 3:D值和CRT都可用
                                     // 先假设RSA密钥完全可用
        if(sg_aRsaKey[i].iKeyLen==0 || sg_aRsaKey[i].lE==0) {
            sg_aRsaKey[i].ucOKFlag = 0; // 无长度或E值，不可用
            continue;
        }
        if(memcmp(sg_aRsaKey[i].sN, szKeyBuf, MAX_RSA_KEY_LEN) == 0) {
            sg_aRsaKey[i].ucOKFlag = 0; // 无N值，不可用
            continue;
        }
        if(memcmp(sg_aRsaKey[i].sD, szKeyBuf, MAX_RSA_KEY_LEN) == 0) {
            sg_aRsaKey[i].ucOKFlag &= 0xFE; // 无D值
            continue;
        }
        if(memcmp(sg_aRsaKey[i].sP, szKeyBuf, MAX_RSA_KEY_LEN/2)==0 ||
                memcmp(sg_aRsaKey[i].sQ, szKeyBuf, MAX_RSA_KEY_LEN/2)==0 ||
                memcmp(sg_aRsaKey[i].sDP, szKeyBuf, MAX_RSA_KEY_LEN/2)==0 ||
                memcmp(sg_aRsaKey[i].sDQ, szKeyBuf, MAX_RSA_KEY_LEN/2)==0 ||
                memcmp(sg_aRsaKey[i].sQINV, szKeyBuf, MAX_RSA_KEY_LEN/2)==0) {
            sg_aRsaKey[i].ucOKFlag &= 0xFD; // 无CRT值
            continue;
        }
    } // for(i=0; i<sg_iMaxRsaKeyNum; i++) {

    return(0);
}
#if 1
// 设置新密钥(双倍长左右部分不同,奇校验)
// ret : 0 : OK
//       其它       : 参考hsmsimu.h错误码
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
// 设置旧密钥
// ret : 0 : OK
//       其它       : 参考hsmsimu.h错误码
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

// 存储密钥到文件
// ret : 0 : OK
//       其它       : 参考hsmsimu.h错误码
static int iKeyFileSave(void)
{
    FILE *fp;
    char szKeyBuf[1024];
    int  i;
    
    if(sg_szKeyFileName[0] == 0)
        return(0);
    
    fp = fopen(sg_szKeyFileName, "wt");
    if(fp == NULL) {
        return(HSMERR_INTERNAL); // 打开文件失败
    }

    // 写说明文字
    fprintf(fp, "# 本文件为仿真卫士通加密机密钥文件\n");
    fprintf(fp, "# 密钥分三个区，‘[master key]’区为主密钥区，‘[des key]’区为应用密钥区，‘[rsa key]’区为RSA密钥区\n");
    fprintf(fp, "# 索引号用3位十进制数表示，主密钥000-002，应用密钥为000-255，rsa密钥为000-019\n");
    fprintf(fp, "# 主密钥索引0-2分别对应卫士通加密机LMK、主密钥一、主密钥二\n");
    fprintf(fp, "# index***_inf为密钥说明，说明文字最长不超过40字节\n");
    fprintf(fp, "# 主密钥与应用密钥固定为双倍长密钥\n");
    fprintf(fp, "# rsa密钥中的index***_len用十进制字节数表示，取值64-256\n");
    fprintf(fp, "# rsa密钥中的index***_e用十进制表示，取值3或65537\n");
    fprintf(fp, "\n");

    // 写master key
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

    // 写des key
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

    // 写rsa key
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
            // D可用
            vOneTwo0(sg_aRsaKey[i].sD, sg_aRsaKey[i].iKeyLen, szKeyBuf);
            fprintf(fp, "index%03d_d    = %s\n", i, szKeyBuf);
        }
        if(sg_aRsaKey[i].ucOKFlag & 0x02) {
            // CRT可用
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

// 发卡方分散密钥
// in  : iKeyIndex  : 密钥索引
//       pKey       : LMK保护的密钥, 如果不是NULL, 表示用此密钥
//       ucDivLevel : 分散次数 0-3
//       pDivData   : 分散数据, 长度=ucDivLevel*8
// out : pDivKey    : 分散后密钥
// ret : 0          : OK
//       其它       : 参考hsmsimu.h错误码
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

	// 分散
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
    vReformKey(1, psDivKey); // 将密钥重整为奇校验
	return(0);
}
// 分散密钥
// in  : pKey       : 密钥
//       ucDivLevel : 分散次数 0-3
//       pDivData   : 分散数据, 长度=ucDivLevel*8
// out : pDivKey    : 分散后密钥
// ret : 0          : OK
//       其它       : 参考hsmsimu.h错误码
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
    vReformKey(1, psDivKey); // 将密钥重整为奇校验
	return(0);
}

// 卡片分散密钥, 产生KMC密钥分量
// div keys(GP2.02规范)
// in  : ucKeyType    : 分散类型 1:S-AUTH(外部认证) 2:C-MAC(Command APDU Mac) 3:R-MAC(Response APDU Mac) 4:ENC(Sensitive Data Encryption)
//       pCardData    : 卡片Initialize指令返回值(KeyDivData[10]+KeyInfoData[2]+CardRand[8]+CardCrypto[8])
//       pMKey        : 主密钥 [16]
// out : pKey         : 密钥分量 [16]
// ret : 0            : OK
//       其它       : 参考hsmsimu.h错误码
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
		// GCP212, 不是202就认为是212
		memcpy(sDivData, psCardData+4, 6); //Chip Id (CSN) 4 bytes + XX 2 bytes
		memcpy(sDivData+8, sDivData, 6);  // repeat
	}

	sDivData[6] = 0xf0;
	sDivData[14] = 0x0f;
	switch(ucKeyType) {
	case 1: // 1:S-AUTH(外部认证)
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
    vReformKey(1, psKey); // 将密钥重整为奇校验
	return(0);
}

// 卡片分散密钥, 产生过程密钥
// in  : ucKeyType    : 分散类型 1:S-AUTH(外部认证) 2:C-MAC(Command APDU Mac) 3:R-MAC(Response APDU Mac) 4:ENC(Sensitive Data Encryption)
//       ucDivMode    : 分散标志, 0:不分散  1:分散
//       pTermRand    : 终端随机数 [8]
//       pCardData    : 卡片Initialize指令返回值(KeyDivData[10]+KeyInfoData[2]+CardRand[8]+CardCrypto[8])
//       pMKey        : 主密钥 [16]
// out : pSessionKey  : 过程密钥 [16]
// ret : 0            : OK
//       其它       : 参考hsmsimu.h错误码
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
			// Div Mode为0, 用如下同一算法计算过程密钥
        	memcpy( sRand, sCardRand+4, 4);
			memcpy( sRand+4, psTermRand, 4);
			_vDes(TRI_ENCRYPT, sRand, sDivKey, psSessionKey);
			memcpy( sRand, sCardRand, 4);
			memcpy( sRand+4, psTermRand+4, 4);
			_vDes(TRI_ENCRYPT, sRand, sDivKey, psSessionKey+8);
		} else {
			// Div Mode为1, 分别计算过程密钥
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
		case 1: // 1:S-AUTH(外部认证)
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
    vReformKey(1, psSessionKey); // 将密钥重整为奇校验
	return(0);
}

// 计算Mac, Single DES plus final Triple Des
// in  : pKey     : 密钥
//       pMsg     : 信息
//       ucLength : 信息长度
// out : pMac     : Mac [8]
// ret : 0        : OK
//       其它     : 参考hsmsimu.h错误码
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

// 计算Mac, Full Triple Des
// in  : pKey     : 密钥
//       pMsg     : 信息
//       ucLength : 信息长度
// out : pMac     : Mac [8]
// ret : 0        : OK
//       其它     : 参考hsmsimu.h错误码
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

// 加密机初始化
// in  : pszKeyFileName : 密钥文件名称, NULL表示使用模块内部密钥
// ret : 0 : OK
//       其它       : 参考hsmsimu.h错误码
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

// 加密RSA密钥
// in  : pRsaKey  : RSA密钥
// out : piOutLen : 加密后密钥长度
//       pOutData : 加密后密钥
// ret : 0        : OK
// Note: 先打包成DER格式, 然后加密输出
static int iPackRsaKey(stRsaKey *pRsaKey, int *piOutLen, void *pOutData)
{
    uchar sDerBuf[1280];
    int  i;
    char *p;
    int  iDerLen;
    int  iPatchFlag;
    int  iLen;
    // DER格式, 补0x80, 再补至8的倍数, LMK以ECB模式加密
    // DER编码规则：
    //     大体同BER编码规则
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
    //     SEQUENCE的Tag为0x30，其它所有内容的Tag都为0x02
    //     注意，对于INTEGER来说，最高位表示符号位，所以在表示最高位为1的模数等大整数时，
    //           要在前面补一个字节的0x00，以说明此数为正数，TLV的长度域也要反映这一字节
    // DER打包
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
            iPatchFlag = 1; // 大于或等于0x80, 需要前补0
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
    
    // 添加Tag30
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
    // DER打包结束, sDerBuf, iDerLen
    
    // 补0x80
    memcpy(sDerBuf+iDerLen, "\x80\x00\x00\x00\x00\x00\x00\x00", 8);
    iDerLen = (iDerLen+8) / 8 * 8;

    // 加密输出
    // deskey, EncryptedKey[16]+cks[8]
    vDesEcb(TRI_ENCRYPT, iDerLen, sDerBuf, sg_aMasterKey[sg_iMasterKeyIndex].sKey, pOutData);
    *piOutLen = iDerLen;
    return(0);
}

// 用LMK加解密密钥
// in  : iFlag : 加解密标志 0:解密 1:加密
//       pIn   : 源数据[16]
// out : pOut  : 目标数据[16]
// 提供给高层用于实现某些特殊指令
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

// 密钥备份
// in  : iKeyType  : 1:DES密钥 2:RSA密钥
//       iKeyIndex : 密钥索引
// out : piOutLen  : 备份数据长度
//       pOutData  : 备份数据, DES:LMK以ECB模式加密[16]+cks[8]
//                             RSA:DER格式, 补0x80, 再补至8的倍数, LMK以ECB模式加密
// ret : 0         : 成功
//       其它      : 参考hsmsimu.h错误码
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

// 解出RSA密钥
// in  : iInLen  : 加密后DER格式RSA密钥长度
//       pInData : 加密后DER格式RSA密钥
// out : pRsaKey : 解出的密钥
// ret : 0       : OK
//       1       : 解析错误
static int iExtractRsaKey(int iInLen, void *pInData, stRsaKey *pRsaKey)
{
    int  iKeyLen, iLen;
    char sDerBuf[1280];
    int  i;
    char *p;
    int  iDerLen;  // DER格式RSA密钥长度
    char *pDest;   // 密钥分量指针
    int  iDestLen; // 密钥分量长度

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
    // 现在, sDerBuf为DER Tag30的内容, iDerLen为DER Tag30的内容长度
    memset(pRsaKey, 0, sizeof(*pRsaKey));
    p = sDerBuf;
    for(i=0; i<9; i++) {
        switch(i) {
        case 0: // Version
            if(memcmp(p, "\x02\x01\x00", 3) != 0) {
                // 必须是版本0
                return(1);
            }
            p += 3;
            continue;
        case 1: // N
            if(p[1] & 0x80)
                iKeyLen = ulStrToLong(p+2, p[1]&0x7F);
            else
                iKeyLen = p[1];
            iKeyLen --; // iKeyLen为RSA模数长度,第一字节一定是大于等于0x80的, 去除前面补的0
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
        
        // 取出DER密钥分量
        if(p[0] != 0x02) {
            // Tag错误
            return(1);
        }
        if(p[1] & 0x80) {
            iLen = ulStrToLong(p+2, p[1]&0x7F);
            p += 2 + (p[1]&0x7F);
        } else {
            iLen = p[1];
            p += 2;
        }
        // p指向DER Tlv对象值, iLen为DER Tlv对象值长度
        if(p+iLen > sDerBuf+iDerLen) {
            // 溢出
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
        
        // 保存密钥分量
        if(iLen > iDestLen) {
            // Tlv对象长度大于密钥分量长度, 前面是补充了0
            if(iLen-1!=iDestLen || p[0]!=0)
                return(1); // 只允许补一个0
            memcpy(pDest, p+1, iDestLen);
        } else {
            // Tlv对象长度小于或等于密钥分量长度, 全部拷贝至目的地
            memcpy(pDest+(iDestLen-iLen), p, iLen);
        }
        p += iLen;
    } // for(
    pRsaKey->ucOKFlag = 3;
    
    return(0);
}

// 解出RSA密钥
// in  : pInData : 加密后DER格式RSA密钥
// out : pRsaKey : 解出的密钥
// ret : 0       : OK
//       1       : 解析错误
static int iExtractRsaKey2(void *pInData, stRsaKey *pRsaKey)
{
    char sDerBuf[8];
    int  iDerLen;
    vDesEcb(TRI_DECRYPT, 8, pInData, sg_aMasterKey[sg_iMasterKeyIndex].sKey, sDerBuf); // 解出前8字节
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

// 密钥恢复
// in  : iKeyType  : 1:DES密钥 2:RSA密钥
//       iKeyIndex : 密钥索引
//       iInLen    : 备份数据长度
//       pInData   : 备份数据
// ret : 0         : 成功
//       其它      : 失败
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
// 卫士通加密机没有做判断
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
	    sprintf(sg_aDesKey[iKeyIndex].szInf, "该密钥为备份恢复, 时间:%s", szDateTime);
        sg_aDesKey[iKeyIndex].ucOKFlag = 1;
    } else {
        // RSA密钥为加密后DER格式
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
	    sprintf(sg_aRsaKey[iKeyIndex].szInf, "该密钥为备份恢复, 时间:%s", szDateTime);
    }

    iRet = iKeyFileSave();

    return(iRet);
}

// 密钥删除
// in  : iKeyType  : 1:RSA密钥 2:DES密钥
//       iKeyIndex : 密钥索引
// ret : 0         : 成功
//       其它      : 失败
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

// 生成DES密钥
// in  : iKeyIndex : 密钥索引(0-999), -1表示不存储
//       pKey      : LMK加密后输出, NULL表示不输出
// out : pKey      : LMK加密后新密钥[16]+cks[8], NULL表示不输出
// ret : 0         : 成功
//       其它      : 失败
// Note: iKeyIndex为-1时pKey不能为NULL
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
	    sprintf(sg_aDesKey[iKeyIndex].szInf, "该密钥为自动生成, 时间:%s", szDateTime);
        iRet = iKeyFileSave();
    }
    if(pKey) {
        vDesEcb(TRI_ENCRYPT, 16, sKey, sg_aMasterKey[sg_iMasterKeyIndex].sKey, pKey);
        memset(szDateTime, 0, 8);
        _vDes(TRI_ENCRYPT, szDateTime, sKey, (char *)pKey+16);
    }
    return(iRet);
}

// 生成RSA密钥
// in  : iKeyIndex : 密钥索引(0-19), -1表示不存储
//       iKeyLen   : 密钥长度, 单位:字节 64-248, 必须为偶数
//       lE        : 公共指数, 3或65537
//       pKey      : LMK加密后新密钥, NULL表示不输出
// out : pN        : 模数
//       pKey      : LMK加密后新密钥, NULL表示不输出
//       piOutLen  : 输出的加密后密钥长度
// ret : 0         : 成功
//       其它      : 失败
// Note: iKeyIndex为-1时pKey不能为NULL
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
	sprintf(RsaKey.szInf, "该密钥为自动生成, 时间:%s", szDateTime);
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

// RSA私钥运算
// in  : iKeyIndex : RSA密钥索引
//       pKey      : LMK加密后RSA私钥, NULL表示使用索引密钥
//       iLen      : 运算数据长度
//       pIn       : 运算数据
// out : pOut      : 运算结果
// ret : 0         : 成功
//       其它      : 失败
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

// RSA公钥运算
// in  : iKeyIndex : RSA密钥索引
//       iLen      : 运算数据长度
//       pIn       : 运算数据
// out : pOut      : 运算结果
// ret : 0         : 成功
//       其它      : 失败
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

// 获取RSA公钥
// in  : iKeyIndex : RSA密钥索引
//       pKey      : LMK加密后RSA私钥, NULL表示使用索引密钥
// out : piLen     : RSA密钥长度, 单位:字节
//       plE       : 公共指数
//       pModulus  : 模数
// ret : 0         : 成功
//       其它      : 失败
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

// RSA直接公钥运算(传入公钥)
// in  : iLen      : RSA公钥长度, 单位:字节
//       lE        : RSA公钥指数, 3或者65537
//       pN        : 模数
//       pIn       : 输入数据
// out : pOut      : 输出数据
// ret : 0         : 成功
//       其它      : 失败
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

// 产生卡片外部认证密文
// in  : iKey1Index : 密钥索引
//       pKey1      : LMK加密后密钥, NULL表示使用索引密钥
//       iDiv1Level : 密钥分散次数(0-3)
//       pDiv1Data  : 密钥分散数据[8*n]
//       pTermRand  : 终端随机数[8]
//       pCardData  : 卡片数据[28]
// out : pOut       : 外部认证密文及Mac[16]
// ret : KFR_OK     : 成功
//       其它       : 失败
int iKeyCalExtAuth0(int iKey1Index, void *pKey1, int iDiv1Level, void *pDiv1Data, void *pTermRand, void *pCardData, void *pOut)
{
    unsigned char ucSessionKeyFlag = 1; // 是否产生过程密钥标志(0:不产生过程密钥 1:产生过程密钥)
    //QINGBO
    //unsigned char ucDivMode = 1;        // 卡片分散模式(0:不分散 1:分散)
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
		// 20140519, 自动尝试KMC分散方式
		sg_iGpDivMethod = 212; // 先尝试212分散方式
		// cal session key:Auth Key 用于外部认证
		iRet = iGpDivChannelKey(1/*1:S-AUTH(外部认证)*/, sg_ucDivMode, pTermRand, pCardData, sKey, sSessionAuthKey);
		if(iRet)
			return(HSMERR_INTERNAL);
		// cal session key:Mac Key 用于Mac计算
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
			// 212分散方式产生的校验数据不正确, 尝试用202分散方式
			sg_iGpDivMethod = 202; // 先尝试212分散方式
			// cal session key:Auth Key 用于外部认证
			iRet = iGpDivChannelKey(1/*1:S-AUTH(外部认证)*/, sg_ucDivMode, pTermRand, pCardData, sKey, sSessionAuthKey);
			if(iRet)
				return(HSMERR_INTERNAL);
			// cal session key:Mac Key 用于Mac计算
			iRet = iGpDivChannelKey(2/*2:C-MAC(Command APDU Mac)*/, sg_ucDivMode, pTermRand, pCardData, sKey, sSessionCMacKey);
			if(iRet)
				return(HSMERR_INTERNAL);
		}
#else
		// 原来不自动尝试KMC方式
		// cal session key:Auth Key 用于外部认证
		iRet = iGpDivChannelKey(1/*1:S-AUTH(外部认证)*/, ucDivMode, pTermRand, pCardData, sKey, sSessionAuthKey);
		if(iRet)
			return(HSMERR_INTERNAL);
		// cal session key:Mac Key 用于Mac计算
		iRet = iGpDivChannelKey(2/*2:C-MAC(Command APDU Mac)*/, ucDivMode, pTermRand, pCardData, sKey, sSessionCMacKey);
		if(iRet)
			return(HSMERR_INTERNAL);
#endif
	} else {
		// 不产生过程密钥
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

// 生成修改KMC密文
// in  : iKey1Index : 原密钥索引
//       pKey1      : LMK加密后原密钥, NULL表示使用索引密钥
//       iDiv1Level : 原密钥分散次数(0-3)
//       pDiv1Data  : 原密钥分散数据[8*n]
//       pTermRand  : 终端随机数[8]
//       pCardData  : 卡片数据[28]
//       iKey2Index : 新密钥索引
//       pKey2      : LMK加密后新密钥, NULL表示使用索引密钥
//       iDiv2Level : 新密钥分散次数(0-3)
//       pDiv2Data  : 新密钥分散数据[8*n]
// out : pOut       : 密文AuthKey[16]+AuthKeyKcv[8]+密文MacKey[16]+MacKeyKcv[8]+密文EncKey[16]+EncKeyKcv[8]
// ret : KFR_OK     : 成功
//       其它       : 失败
int iKeyCalChangeKmc0(int iKey1Index, void *pKey1, int iDiv1Level, void *pDiv1Data, void *pTermRand, void *pCardData, int iKey2Index, void *pKey2, int iDiv2Level, void *pDiv2Data, void *pOut)
{
    unsigned char ucSessionKeyFlag = 1; // 是否产生过程密钥标志(0:不产生过程密钥 1:产生过程密钥)
    //QINGBO
    //unsigned char ucDivMode = 1;        // 卡片分散模式(0:不分散 1:分散)
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
		// cal session key:Enc Key 用于数据加密
		iRet = iGpDivChannelKey(4/*4:Enc(加密)*/, sg_ucDivMode, pTermRand, pCardData, sKey, sSessionEncKey);
		if(iRet)
			return(HSMERR_INTERNAL);
	} else {
		// 不产生过程密钥
		memcpy(sSessionEncKey, sKey, 16);
	}
    iRet = iDivKey(iKey2Index, pKey2, (uchar)iDiv2Level, pDiv2Data, sKey);
    if(iRet)
        return(iRet);
        
    psOut = (unsigned char *)pOut;
    memset(sBuf, 0, 8);
    iRet = iGpDivKey(1/*1:S-AUTH(外部认证)*/, pCardData, sKey, sDivKey);
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

// 产生IC卡DES密钥密文(即时发卡)
// in  : iKey1Index : 主控密钥索引
//       pKey1      : LMK加密后主控密钥, NULL表示使用索引密钥
//       iDiv1Level : 主控密钥分散次数(0-3)
//       pDiv1Data  : 主控密钥分散数据[8*n]
//       pTermRand  : 终端随机数[8]
//       pCardData  : 卡片数据[28]
//       iKey2Index : 应用密钥索引
//       pKey2      : LMK加密后应用密钥, NULL表示使用索引密钥
//       iDiv2Level : 应用密钥分散次数(0-3)
//       pDiv2Data  : 应用密钥分散数据[8*n]
// out : pOut       : 密文Key[16]+KeyKcv[8]
// ret : KFR_OK     : 成功
//       其它       : 失败
int iKeyCalDesKey0(int iKey1Index, void *pKey1, int iDiv1Level, void *pDiv1Data, void *pTermRand, void *pCardData, int iKey2Index, void *pKey2, int iDiv2Level, void *pDiv2Data, void *pOut)
{
    unsigned char ucSessionKeyFlag = 1; // 是否产生过程密钥标志(0:不产生过程密钥 1:产生过程密钥)
    //QINGBO
    //unsigned char ucDivMode = 1;        // 卡片分散模式(0:不分散 1:分散)
	unsigned char sSessionDekKey[16];
	unsigned char sKek[16];    // 用于加密的密钥
	unsigned char sOutKey[16]; // 被加密的密钥
	unsigned char sBuf[50], szBuf[100];
	int   iRet;

    iRet = iDivKey(iKey1Index, pKey1, (uchar)iDiv1Level, pDiv1Data, sKek);
    if(iRet)
        return(iRet);
	if(ucSessionKeyFlag) {
    	// cal session key:ENC Key 用于加密敏感数据
	    iRet = iGpDivChannelKey(4/*4:ENC(Sensitive Data Encryption)*/, sg_ucDivMode, pTermRand, pCardData, sKek, sSessionDekKey);
    	if(iRet)
	    	return(iRet);
	} else {
		// 不产生过程密钥
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

// 产生IC卡PIN密文(即时发卡)
// in  : iKey1Index : 主控密钥索引
//       pKey1      : LMK加密后主控密钥, NULL表示使用索引密钥
//       iDiv1Level : 主控密钥分散次数(0-3)
//       pDiv1Data  : 主控密钥分散数据[8*n]
//       pTermRand  : 终端随机数[8]
//       pCardData  : 卡片数据[28]
//       pPinBlock  : 个人密码数据块
// out : pOut       : 密文个人密码数据块[8]
// ret : KFR_OK     : 成功
//       其它       : 失败
int iKeyCalPin0(int iKey1Index, void *pKey1, int iDiv1Level, void *pDiv1Data, void *pTermRand, void *pCardData, void *pPinBlock, void *pOut)
{
    unsigned char ucSessionKeyFlag = 1; // 是否产生过程密钥标志(0:不产生过程密钥 1:产生过程密钥)
    //QINGBO
    //unsigned char ucDivMode = 1;        // 卡片分散模式(0:不分散 1:分散)
	unsigned char sSessionDekKey[16];
	unsigned char sKek[16];    // 用于加密的密钥
	int   iRet;

    iRet = iDivKey(iKey1Index, pKey1, (uchar)iDiv1Level, pDiv1Data, sKek);
    if(iRet)
        return(iRet);
	if(ucSessionKeyFlag) {
    	// cal session key:ENC Key 用于加密敏感数据
	    iRet = iGpDivChannelKey(4/*4:ENC(Sensitive Data Encryption)*/, sg_ucDivMode, pTermRand, pCardData, sKek, sSessionDekKey);
    	if(iRet)
	    	return(iRet);
	} else {
		// 不产生过程密钥
		memcpy(sSessionDekKey, sKek, 16);
	}
	
	_vDes(TRI_ENCRYPT, pPinBlock, sSessionDekKey, pOut);
    return(0);
}

// 生成IC卡RSA密钥密文(即时发卡)
// in  : iKey1Index : 主控密钥索引
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
int iKeyCalRsaKey0(int iKey1Index, void *pKey1, int iDiv1Level, void *pDiv1Data, void *pTermRand, void *pCardData, int iRsaKeyLen, long lE, void *pOut)
{
    uchar ucSessionKeyFlag = 1; // 是否产生过程密钥标志(0:不产生过程密钥 1:产生过程密钥)
    //QINGBO
    //unsigned char ucDivMode = 1;        // 卡片分散模式(0:不分散 1:分散)
	uchar sSessionDekKey[16];
	uchar sKek[16];    // 用于加密的密钥
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
    	// cal session key:ENC Key 用于加密敏感数据
	    iRet = iGpDivChannelKey(4/*4:ENC(Sensitive Data Encryption)*/, sg_ucDivMode, pTermRand, pCardData, sKek, sSessionDekKey);
    	if(iRet)
	    	return(iRet);
	} else {
		// 不产生过程密钥
		memcpy(sSessionDekKey, sKek, 16);
	}
        
    // 生成RSA密钥
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
    iItem1Len = (iRsaKeyLen+1+7)/8*8; // D长度
    memset(sBuf, 0, sizeof(sBuf));
	memcpy(sBuf, PrivateKey.exponent+sizeof(PrivateKey.exponent)-iRsaKeyLen, iRsaKeyLen);
	sBuf[iRsaKeyLen] = 0x80;
    vDesEcb(TRI_ENCRYPT, iItem1Len, sBuf, sSessionDekKey, (char *)pOut+iRsaKeyLen);

    iItem2Len = (iRsaKeyLen/2+1+7)/8*8; // 密钥分量(p、q等)长度
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

// 验证ARQC、TC, 产生ARPC
// in  : iKey1Index     : 密钥索引
//       pKey1          : LMK加密后密钥, NULL表示使用索引密钥
//       iDiv1Level     : 密钥分散次数(0-3)
//       pDiv1Data      : 密钥分散数据[8*n]
//       pATC           : 交易序列号[2]
//       iTransDataLen  : 交易数据长度(如果长度为0, 则不验证ARQC, 只计算ARPC)
//       pTransData     : 交易数据
//       pARQC          : ARQC或TC
//       pARC           : 交易应答码, NULL表示只验证ARQC或TC
// out : pARPC          : ARPC
// ret : KFR_OK         : 成功
//       其它           : 失败
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
        // 存在应答码, 计算ARPC
	    memcpy(sBuf, pARQC, 8);
        vXor(sBuf, pARC, 2);
	    _vDes(TRI_ENCRYPT, sBuf, sSessionKey, pARPC);
    }
	return(0);
}

// 用过程密钥加密数据(脚本)
// in  : iKey1Index    : 密钥索引
//       pKey1         : LMK加密后密钥, NULL表示使用索引密钥
//       iDiv1Level    : 密钥分散次数(0-3)
//       pDiv1Data     : 密钥分散数据[8*n]
//       pATC          : 交易序列号[2]
//       iPinFlag      : 密码块标志, 0:pData是普通数据 1:pData是密码块
//       iDataLen      : 数据长度
//       pData         : 数据
// out : piOutLen      : 输出数据长度
//       pOutData      : 输出数据
// ret : KFR_OK        : 成功
//       其它          : 失败
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

// 用过程密钥计算Mac(脚本)
// in  : iKey1Index    : 密钥索引
//       pKey1         : LMK加密后密钥, NULL表示使用索引密钥
//       iDiv1Level    : 密钥分散次数(0-3)
//       pDiv1Data     : 密钥分散数据[8*n]
//       pATC          : 交易序列号[2]
//       iDataLen      : 数据长度
//       pData         : 数据
// out : pMac          : Mac[8]
// ret : KFR_OK        : 成功
//       其它          : 失败
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

// 加密数据密钥转换(数据准备文件发卡)
// in  : iKey1Index : 主控密钥索引
//       pKey1      : LMK加密后主控密钥, NULL表示使用索引密钥
//       iDiv1Level : 主控密钥分散次数(0-3)
//       pDiv1Data  : 主控密钥分散数据[8*n]
//       pTermRand  : 终端随机数[8]
//       pCardData  : 卡片数据[28]
//       iKey2Index : 应用密钥索引
//       pKey2      : LMK加密后应用密钥, NULL表示使用索引密钥
//       iDiv2Level : 应用密钥分散次数(0-3)
//       pDiv2Data  : 应用密钥分散数据[8*n]
//       iDataLen   : 原加密数据长度, 必须为8的倍数
//       pData      : 原加密数据
// out : pOut       : 转加密后数据, 长度等于原加密数据长度
// ret : KFR_OK     : 成功
//       其它       : 失败
// Note: 数据原为应用密钥加密, 用主控密钥的过程密钥转加密
int iKeyReEncData0(int iKey1Index, void *pKey1, int iDiv1Level, void *pDiv1Data, void *pTermRand, void *pCardData, int iKey2Index, void *pKey2, int iDiv2Level, void *pDiv2Data, int iDataLen, void *pData, void *pOut)
{
    unsigned char ucSessionKeyFlag = 1; // 是否产生过程密钥标志(0:不产生过程密钥 1:产生过程密钥)
    //QINGBO
    //unsigned char ucDivMode = 1;        // 卡片分散模式(0:不分散 1:分散)

	unsigned char sKey[16];
	unsigned char sSessionEncKey[16];
	unsigned char sBuf[256];
	int   iRet;

    iRet = iDivKey(iKey1Index, pKey1, (uchar)iDiv1Level, pDiv1Data, sKey);
    if(iRet)
        return(iRet);
	if(ucSessionKeyFlag) {
		// cal session key:Enc Key 用于数据加密
		iRet = iGpDivChannelKey(4/*4:Enc(加密)*/, sg_ucDivMode, pTermRand, pCardData, sKey, sSessionEncKey);
		if(iRet)
			return(HSMERR_INTERNAL);
	} else {
		// 不产生过程密钥
		memcpy(sSessionEncKey, sKey, 16);
	}
    iRet = iDivKey(iKey2Index, pKey2, (uchar)iDiv2Level, pDiv2Data, sKey);
    if(iRet)
        return(iRet);

    vDesEcb(TRI_DECRYPT, iDataLen, pData, sKey, sBuf);
    vDesEcb(TRI_ENCRYPT, iDataLen, sBuf, sSessionEncKey, pOut);

	return(0);
}

// 加密密码密钥转换(数据准备文件发卡)
// in  : iKey1Index : 主控密钥索引
//       pKey1      : LMK加密后主控密钥, NULL表示使用索引密钥
//       iDiv1Level : 主控密钥分散次数(0-3)
//       pDiv1Data  : 主控密钥分散数据[8*n]
//       pTermRand  : 终端随机数[8]
//       pCardData  : 卡片数据[28]
//       iKey2Index : 应用密钥索引
//       pKey2      : LMK加密后应用密钥, NULL表示使用索引密钥
//       iDiv2Level : 应用密钥分散次数(0-3)
//       pDiv2Data  : 应用密钥分散数据[8*n]
//       pPinBlock  : 原加密后密码块[8]
// out : pOut       : 转加密后密码块[8]
// ret : KFR_OK     : 成功
//       其它       : 失败
// Note: 数据原为应用密钥加密, 用主控密钥的过程密钥转加密
int iKeyReEncPin0(int iKey1Index, void *pKey1, int iDiv1Level, void *pDiv1Data, void *pTermRand, void *pCardData, int iKey2Index, void *pKey2, int iDiv2Level, void *pDiv2Data, void *pPinBlock, void *pOut)
{
    unsigned char ucSessionKeyFlag = 1; // 是否产生过程密钥标志(0:不产生过程密钥 1:产生过程密钥)
    //QINGBO
    //unsigned char ucDivMode = 1;        // 卡片分散模式(0:不分散 1:分散)
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
		// cal session key:Enc Key 用于数据加密
		iRet = iGpDivChannelKey(4/*4:Enc(加密)*/, sg_ucDivMode, pTermRand, pCardData, sKey, sSessionEncKey);
		if(iRet)
			return(HSMERR_INTERNAL);
	} else {
		// 不产生过程密钥
		memcpy(sSessionEncKey, sKey, 16);
	}
    iRet = iDivKey(iKey2Index, pKey2, (uchar)iDiv2Level, pDiv2Data, sKey);
    if(iRet)
        return(iRet);

    _vDes(TRI_DECRYPT, pPinBlock, sKey, sBuf);
    if((sBuf[0]&0xF0)!=0x10 && (sBuf[0]&0xF0)!=0x20)
        return(HSMERR_PIN_BLOCK); // 只支持格式1与格式2
    iPinLen = sBuf[0] & 0x0F;
    if(iPinLen<4 || iPinLen>12)
        return(HSMERR_PIN_BLOCK);
    sPinBlock[0] = 0x20; // 固定转换成格式2
    sPinBlock[0] |= iPinLen;    
    vOneTwo(sBuf+1, 7, szBuf);
    memset(szBuf+iPinLen, 'F', 14-iPinLen);
    vTwoOne(szBuf, 14, sPinBlock+1);

    _vDes(TRI_ENCRYPT, sPinBlock, sSessionEncKey, pOut);
	return(0);
}

// 产生临时密钥(数据准备)
// in  : iKey1Index : 应用密钥索引
//       pKey1      : LMK加密后应用密钥, NULL表示使用索引密钥
//       iDiv1Level : 应用密钥分散次数(0-3)
//       pDiv1Data  : 应用密钥分散数据[8*n]
// out : pOut       : 被应用密钥加密后的随机临时密钥[16]+cks[8]
// ret : KFR_OK     : 成功
//       其它       : 失败
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

// Mac计算(数据准备)
// in  : iKey1Index  : 应用密钥索引
//       pKey1       : LMK加密后应用密钥, NULL表示使用索引密钥
//       iDiv1Level  : 应用密钥分散次数(0-3)
//       pDiv1Data   : 应用密钥分散数据[8*n]
//       pMacKey     : 被应用密钥加密后的Mac密钥
//       pIv         : 初始向量[8]
//       iDataLen    : 数据长度
//       pData       : 数据
//       iBlockFlag  : 数据块标志, 1:第一块或中间块 2:唯一块或最后一块
// out : pMac        : Mac[8]
// ret : KFR_OK      : 成功
//       其它        : 失败
// Note: 数据长度不足8的倍数后面自动补0
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
		    // 最后一块儿
    		_vDes(DECRYPT, pMac, sTmpKey+8, pMac);
    		_vDes(ENCRYPT, pMac, sTmpKey, pMac);
		}
    }
    return(0);
}
// Mac计算(数据准备)
// 函数iKeyDpCalMac0()只实现了cps规范要求的Mac算法, 本函数支持全部卫士通加密机Mac算法
// in  : iKey1Index  : Mac密钥索引
//       pKey1       : LMK加密后Mac密钥, NULL表示使用索引密钥
//       iDiv1Level  : Mac密钥分散次数(0-3)
//       pDiv1Data   : Mac密钥分散数据[8*n]
//       pMacKey     : 被Mac密钥加密后的临时Mac密钥, NULL表示直接使用Mac密钥计算
//       iAlgoFlag   : 算法, 1:ANSI99(full) 2:ANSI919(retail) 3:XOR 4:PBOC3DES
//                     算法1、2、3在数据长度不是8的倍数时, 后补0x00至8的倍数
//                     算法4先补0x80, 之后如果长度不是8的倍数时, 后补0x00至8的倍数
//       pIv         : 初始向量[8]
//       iDataLen    : 数据长度
//       pData       : 数据
//       iBlockFlag  : 数据块标志, 1:第一块或中间块 2:唯一块或最后一块
// out : pMac        : Mac[8]
// ret : KFR_OK      : 成功
//       其它        : 失败
int iKeyDpCalMacFull0(int iKey1Index, void *pKey1, int iDiv1Level, void *pDiv1Data, void *pMacKey, int iDiv2Level, void *pDiv2Data, int iAlgoFlag, void *pIv, int iDataLen, void *pData, void *pMac, int iBlockFlag)
{
	unsigned char sKey[16];
	unsigned char sTmpKey[16];
    unsigned char sTmpData[4096+20]; // 为统一算法, 设置该缓冲区, 用于得到填充了尾部的数据
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
        return(HSMERR_DATAIN_LEN); // 不是最后一块时, 数据长度必须为8的倍数
    if(iAlgoFlag<1 || iAlgoFlag>4)
        return(HSMERR_MODE);
    if(iDataLen > 4096)
        return(HSMERR_DATAIN_LEN); // 数据超长

    memset(sTmpData, 0, sizeof(sTmpData));
    memcpy(sTmpData, pData, iDataLen);
    if(iAlgoFlag==4 && iBlockFlag==2)
        sTmpData[iDataLen++] = 0x80; // 如果是最后一块并且为算法4, 补充0x80
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
	    	    // 最后一块儿
    	    	_vDes(DECRYPT, pMac, sTmpKey+8, pMac);
    		    _vDes(ENCRYPT, pMac, sTmpKey, pMac);
		    }
        }
        break;
    case 3: // XOR
        for(i=0; i<iDataLen; i+=8)
	    	vXor(pMac, (char *)sTmpData+i, 8);
		if(iBlockFlag == 2)
		    _vDes(TRI_ENCRYPT, pMac, sTmpKey, pMac); // 最后一块儿
        break;
    }
    return(0);
}

// 生成数据准备DES密钥密文(数据准备)
// in  : iKey1Index  : TK密钥索引
//       pKey1       : LMK加密后Tk密钥, NULL表示使用索引密钥
//       iDiv1Level  : TK密钥分散次数(0-3)
//       pDiv1Data   : TK密钥分散数据[8*n]
//       iKey2Index  : 应用密钥索引
//       pKey2       : LMK加密后应用密钥, NULL表示使用索引密钥
//       iDiv2Level  : 应用密钥分散次数(0-3)
//       pDiv2Data   : 应用密钥分散数据[8*n]
// out : pOut        : OutKey[16]+Cks[8]
// ret : KFR_OK     : 成功
//       其它       : 失败
int iKeyDpCalDesKey0(int iKey1Index, void *pKey1, int iDiv1Level, void *pDiv1Data, int iKey2Index, void *pKey2, int iDiv2Level, void *pDiv2Data, void *pOut)
{
    unsigned char ucSessionKeyFlag = 1; // 是否产生过程密钥标志(0:不产生过程密钥 1:产生过程密钥)
	unsigned char sKek[16];    // 用于加密的密钥
	unsigned char sOutKey[16]; // 被加密的密钥
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

// 生成数据准备RSA密钥密文(数据准备)
// in  : iKey1Index  : TK密钥索引
//       pKey1       : LMK加密后TK密钥, NULL表示使用索引密钥
//       iDiv1Level  : TK密钥分散次数(0-3)
//       pDiv1Data   : TK密钥分散数据[8*n]
//       iRsaKeyLen  : RSA密钥长度, 单位:字节, 64-248, 必须为偶数
//       lE          : 公共指数值, 3或者65537
// out : pOut        : RSA密钥公钥明文[n]+D密文[n1]+P密文[n2]+Q密文[n2]+dP密文[n2]+dQ密文[n2]+qInv密文[n2]
//                     n = iRsaKeyLen, n1 = (iRsaKeyLen+8)/8*8, n2 = (iRsaKeyLen/2+8)/8*8
// ret : KFR_OK      : 成功
//       其它        : 失败
int iKeyDpCalRsaKey0(int iKey1Index, void *pKey1, int iDiv1Level, void *pDiv1Data, int iRsaKeyLen, long lE, void *pOut)
{
	unsigned char sKek[16];    // 用于加密的密钥
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
        
    // 生成RSA密钥
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
    iItem1Len = (iRsaKeyLen+1+7)/8*8; // D长度
    memset(sBuf, 0, sizeof(sBuf));
	memcpy(sBuf, PrivateKey.exponent+sizeof(PrivateKey.exponent)-iRsaKeyLen, iRsaKeyLen);
	sBuf[iRsaKeyLen] = 0x80;
    vDesEcb(TRI_ENCRYPT, iItem1Len, sBuf, sKek, (char *)pOut+iRsaKeyLen);

    iItem2Len = (iRsaKeyLen/2+1+7)/8*8; // 密钥分量(p、q等)长度
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

// 数据加解密
// in  : iKey1Index  : 加解密密钥索引
//       pKey1       : LMK加密后加解密密钥, NULL表示使用索引密钥
//       iDiv1Level  : 加解密密钥分散次数(0-3)
//       pDiv1Data   : 加解密密钥分散数据[8*n]
//       pKey2       : 被加解密密钥加密后的密钥, NULL表示直接使用加解密密钥计算
//       iDiv2Level  : 加密后密钥分散次数(0-3)
//       pDiv2Data   : 加密后密钥分散数据[8*n]
//       iEncDecFlag : 加解密标志, 1:加密 0:解密
//       iPadFlag    : 填充模式, 解密时无效, 1:80+00(长度是8的倍数时不填充) 2:80+00(总是填充), 3:00(长度是8的倍数时不填充), 4:00(总是填充)
//       pIv         : 初始向量[8], NULL表示为ECB模式, 否则为CBC模式
//       iDataLen    : 数据长度
//       pData       : 数据
// out : piOutLen    : 结果长度
//       pOutData    : 结果
// ret : KFR_OK      : 成功
//       其它        : 失败
int iKeyEncDec0(int iKey1Index, void *pKey1, int iDiv1Level, void *pDiv1Data, void *pKey2, int iDiv2Level, void *pDiv2Data, int iEncDecFlag, int iPadFlag, void *pIv, int iDataLen, void *pData, int *piOutLen, void *pOutData)
{
	unsigned char sKey[16];
	unsigned char sTmpKey[16];
    unsigned char sTmpData[4096+20]; // 为统一算法, 设置该缓冲区, 用于得到填充了尾部的数据
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
        return(HSMERR_DATAIN_LEN); // 数据超长
    if(iEncDecFlag==0 && iDataLen%8)
        return(HSMERR_DATAIN_LEN);

    memset(sTmpData, 0, sizeof(sTmpData));
    memcpy(sTmpData, pData, iDataLen);
    if(iEncDecFlag == 1) {
        // 加密
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
        // pIv==NULL表示使用ECB模式
        if(iEncDecFlag == 1)
            vDesEcb(TRI_ENCRYPT, iDataLen, sTmpData, sTmpKey, pOutData);
        else
            vDesEcb(TRI_DECRYPT, iDataLen, sTmpData, sTmpKey, pOutData);
    } else {
        // 使用CBC模式
        if(iEncDecFlag == 1)
            vDesCbc(TRI_ENCRYPT, pIv, iDataLen, sTmpData, sTmpKey, pOutData);
        else
            vDesCbc(TRI_DECRYPT, pIv, iDataLen, sTmpData, sTmpKey, pOutData);
    }

    return(0);
}

// ECB模式加解密数据
// in  : iKey1Index  : 密钥索引
//       pKey1       : LMK加密后密钥, NULL表示使用索引密钥
//       iDiv1Level  : 密钥分散次数(0-3)
//       pDiv1Data   : 密钥分散数据[8*n]
//       iFlag       : 加解密标志 0:解密 1:加密
//       iDataLen    : 要加解密的数据长度, 必须为8的倍数
//       pData       : 要加解密的数据
// out : pOut        : 加解密后的数据
// ret : KFR_OK     : 成功
//       其它       : 失败
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

// 通用加密数据密钥转换
// 带1的参数为解密密钥, 带2的参数为加密密钥
// in  : iKey1Index  : 密钥索引
//       pKey1       : 加密后密钥, NULL表示使用索引密钥
//       iDiv1Level  : 密钥分散次数(0-3)
//       pDiv1Data   : 密钥分散数据[8*n]
//       iKey1IndexP : 保护密钥索引, -1表示不使用保护密钥
//       pKey1P      : 加密后保护密钥, NULL表示使用索引密钥
//       iDiv1LevelP : 保护密钥分散次数(0-3)
//       pDiv1DataP  : 保护密钥分散数据[8*n]
//       pIv1        : Iv, NULL表示用ECB模式
//       iKey2Index  : 密钥索引
//       pKey2       : 加密后密钥, NULL表示使用索引密钥
//       iDiv2Level  : 密钥分散次数(0-3)
//       pDiv2Data   : 密钥分散数据[8*n]
//       iKey2IndexP : 保护密钥索引, -1表示不使用保护密钥
//       pKey2P      : 加密后保护密钥, NULL表示使用索引密钥
//       iDiv2LevelP : 保护密钥分散次数(0-3)
//       pDiv2DataP  : 保护密钥分散数据[8*n]
//       pIv2        : Iv, NULL表示用ECB模式
//       iDataLen    : 原加密数据长度, 必须为8的倍数
//       pData       : 原加密数据
// out : pOut        : 转加密后数据, 长度等于原加密数据长度
// ret : KFR_OK      : 成功
//       其它        : 失败
int iKeyDecEncData0(int iKey1Index, void *pKey1, int iDiv1Level, void *pDiv1Data, int iKey1IndexP, void *pKey1P, int iDiv1LevelP, void *pDiv1DataP, void *pIv1,
                    int iKey2Index, void *pKey2, int iDiv2Level, void *pDiv2Data, int iKey2IndexP, void *pKey2P, int iDiv2LevelP, void *pDiv2DataP, void *pIv2,
                    int iDataLen, void *pData, void *pOut)
{
	unsigned char sKey1[16], sKey2[16]; // 解密密钥与加密密�
    unsigned char sKey[16], sTmp[16];
	unsigned char sBuf[2020];
    int   i;
	int   iRet;

    if(iDataLen % 8)
        return(HSMERR_DATAIN_LEN);
    for(i=0; i<2; i++) {
        if(pKey1) {
            // 应用密钥被LMK或保护密钥保护
            if(iKey1IndexP == -1) {
                // 应用密钥被LMK保护
                iRet = iDivKey(iKey1Index, pKey1, (uchar)iDiv1Level, pDiv1Data, sKey);
                if(iRet)
                    return(iRet);
            } else {
                // 应用密钥被保护密钥保护
                iRet = iDivKey(iKey1IndexP, pKey1P, (uchar)iDiv1LevelP, pDiv1DataP, sTmp); // sTmp为保护密钥
                if(iRet)
                    return(iRet);
                vDesEcb(TRI_DECRYPT, 16, pKey1, sTmp, sKey); // sKey为应用密钥
                vDesEcb(TRI_ENCRYPT, 16, sKey, sg_aMasterKey[sg_iMasterKeyIndex].sKey, sTmp); // sTmp为LMK加密后应用密钥
                iRet = iDivKey(iKey1Index, sTmp, (uchar)iDiv1Level, pDiv1Data, sKey); // sKey为分散后应用密钥
                if(iRet)
                    return(iRet);    
            }
        } else {
            // 应用密钥为索引密钥
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
    // sKey1为解密密钥, sKey2为加密密钥
    if(iDataLen > sizeof(sBuf)-20)
        return(HSMERR_INTERNAL);
    // 解密
    if(pIv1)
        vDesCbc(TRI_DECRYPT, pIv1, iDataLen, pData, sKey1, sBuf);
    else
        vDesEcb(TRI_DECRYPT, iDataLen, pData, sKey1, sBuf);
    // 加密
    if(pIv2)
        vDesCbc(TRI_ENCRYPT, pIv2, iDataLen, sBuf, sKey2, pOut);
    else
        vDesEcb(TRI_ENCRYPT, iDataLen, sBuf, sKey2, pOut);
	return(0);
}

// 明文方式设置某位置DES密钥
// ret : 0 : OK
// Note: 未公开函数
int iHsmSetDesKey(int iIndex, void *pKey)
{
    uchar szDateTime[15];
    if(iIndex > sg_iMaxDesKeyNum)
        return(1);
    memcpy(sg_aDesKey[iIndex].sKey, pKey, 16);
    _vGetTime(szDateTime);
    sprintf(sg_aDesKey[iIndex].szInf, "该密钥为明文设置, 时间:%s", szDateTime);
    sg_aDesKey[iIndex].ucOKFlag = 1;
    return(0);
}

// QINGBO
// 设置分散模式：卡片分散模式(0:不分散 1:分散)
void vKeyCalSetDivMode(int iDivMode)
{
    sg_ucDivMode = iDivMode;
}
// 修改KMC密钥值
void vHsmChangeKmcKey(uchar *psNewKey)
{
    memcpy(sg_aDesKey[1].sKey, psNewKey, 16);
}

