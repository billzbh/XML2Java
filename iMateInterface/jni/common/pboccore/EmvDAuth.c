/**************************************
File name     : EmvDAuth.c
Function      : Emv脱机数据认证模块
Author        : Yu Jun
First edition : Apr 19th, 2012
Modified      : May 28th, 2013
				    增加fDDA认证函数iEmvFdda()
**************************************/
/*
模块详细描述:
	支持国际算法(RSA)的SDA、DDA、CDA
.	准备公钥
	根据RID及CA公钥索引找到CA公钥        -> sg_CaPubKey
	验证发卡行公钥证书, 解析出发卡行公钥 -> sg_IssuerPubKey
	验证IC卡公钥证书, 解析出IC卡公钥	 -> sg_IcPubKey
.	记录脱机认证记录数据
		sg_iOfflineRecDataErrFlag; // 脱机认证记录非法标志 0:记录正确 1:记录非法
		sg_iOfflineRecDataLen;     // 记录数据总长度
		sg_sOfflineRecData[];      // 需脱机认证的所有记录数据
	用于脱机数据认证
.	脱机数据认证
	SDA、DDA、CDA
	fDDA
*/
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "VposFace.h"
#include "Sha.h"
#include "RsaRef.h"
#include "Common.h"
#include "EmvCmd.h"
#include "TlvFunc.h"
#include "EmvFunc.h"
#include "EmvData.h"
#include "EmvCore.h"
#include "TagDef.h"
#include "EmvDAuth.h"
#include "EmvIO.h"
#include "EmvTrace.h"

// SDA需要认证的应用记录数据
static int   sg_iOfflineRecDataErrFlag; // 脱机认证记录非法标志 0:记录正确 1:记录非法
static int   sg_iOfflineRecDataLen;     // 记录数据总长度
static uchar sg_sOfflineRecData[4096];  // 需脱机认证的所有记录数据

static R_RSA_PUBLIC_KEY sg_CaPubKey;	// CA公钥
static R_RSA_PUBLIC_KEY sg_IssuerPubKey;// 发卡行公钥
static R_RSA_PUBLIC_KEY sg_IcPubKey;	// IC卡公钥

// 脱机数据认证初始化
// ret : EMV_DAUTH_OK : OK
int iEmvDAuthInit(void)
{
	sg_iOfflineRecDataErrFlag = 0;
    sg_iOfflineRecDataLen = 0;
	memset(&sg_CaPubKey, 0, sizeof(sg_CaPubKey));
	memset(&sg_IssuerPubKey, 0, sizeof(sg_IssuerPubKey));
	memset(&sg_IcPubKey, 0, sizeof(sg_IcPubKey));
	return(0);
}

// 增加一条SDA脱机认证记录数据
// in  : ucRecDataLen          : 记录数据长度
//       psRecData             : 记录数据
// ret : EMV_DAUTH_OK		   : OK
//       EMV_DAUTH_LACK_MEMORY : 预留存储空间不足
int iEmvSDADataAdd(uchar ucRecDataLen, uchar *psRecData)
{
    ASSERT(sg_iOfflineRecDataLen+ucRecDataLen <= sizeof(sg_sOfflineRecData));
    if(sg_iOfflineRecDataLen+ucRecDataLen > sizeof(sg_sOfflineRecData))
        return(1);
    memcpy(sg_sOfflineRecData+sg_iOfflineRecDataLen, psRecData, ucRecDataLen);
    sg_iOfflineRecDataLen += ucRecDataLen;
    return(0);
}

// 报告SDA记录数据错误, 脱机认证记录不是T70 Tlv对象
// ret : EMV_DAUTH_OK		   : OK
int iEmvSDADataErr(void)
{
	sg_iOfflineRecDataErrFlag = 1; // 标记脱机认证记录非法
	return(EMV_DAUTH_OK);
}

// 判断SDA是否已经失败
// ret : EMV_DAUTH_OK		   : OK
//       EMV_DAUTH_FAIL		   : SDA已经失败
int iEmvSDADataErrTest(void)
{
	if(sg_iOfflineRecDataErrFlag == 1)
		return(EMV_DAUTH_FAIL);
	return(EMV_DAUTH_OK);
}

// 搜索CA公钥
// ret : EMV_DAUTH_OK				 : OK
//       EMV_DAUTH_DATA_MISSING		 : 数据缺失
//       EMV_DAUTH_DATA_ILLEGAL		 : 数据非法
//       EMV_DAUTH_NATIVE			 : 内部错误
//       EMV_DAUTH_INDEX_NOT_SUPPORT : CA公钥索引不支持
//       EMV_DAUTH_FAIL              : 导致脱机数据认证失败的其它错误
static int iEmvSearchCaPubKey(void)
{
	int   iRet;
	int   i;
	uchar *pucCaPubKeyIndex; // T8F(Certification Authority Public Key Index)
	uchar *psRid;			 // Rid
	uchar *psTransDate;		 // 交易日期
	uchar szTransDate[9];    // 8位交易日期
	R_RSA_PROTO_KEY ProtoKey;

	if(sg_CaPubKey.bits)
		return(EMV_DAUTH_OK); // 已经搜索到了CA公钥
	if(sg_iOfflineRecDataErrFlag)
		return(EMV_DAUTH_FAIL); // 脱机认证数据格式错误, 直接返回认证失败
	iRet = iTlvGetObjValue(gl_sTlvDbCard, TAG_8F_CAPubKeyIndex, &pucCaPubKeyIndex);
	if(iRet < 0) {
		EMV_TRACE_ERR_MSG
		return(EMV_DAUTH_DATA_MISSING); // 数据缺失
	}
	if(iRet != 1) {
		EMV_TRACE_ERR_MSG
		return(EMV_DAUTH_DATA_ILLEGAL); // 数据非法
	}
	iRet = iTlvGetObjValue(gl_sTlvDbCard, TAG_4F_AID, &psRid);
	ASSERT(iRet >= 5)
	if(iRet < 5) {
		EMV_TRACE_ERR_MSG
		return(EMV_DAUTH_NATIVE); // T4F必须存在
	}
	iRet = iTlvGetObjValue(gl_sTlvDbTermVar, TAG_9A_TransDate, &psTransDate);
	ASSERT(iRet == 3);
	if(iRet != 3) {
		EMV_TRACE_ERR_MSG
		return(EMV_DAUTH_NATIVE); // T9A必须存在
	}
	if(psTransDate[0] < 0x50)
		strcpy(szTransDate, "20");
	else
		strcpy(szTransDate, "19");
	vOneTwo0(psTransDate, 3, szTransDate+2);

	for(i=0; i<gl_iTermSupportedCaPublicKeyNum; i++) {
		if(gl_aCAPublicKeyList[i].ucIndex == *pucCaPubKeyIndex) {
			if(memcmp(psRid, gl_aCAPublicKeyList[i].sRid, 5) == 0) {
				// CA公钥索引值与RIX都匹配, 找到CA公钥
				if(strcmp(szTransDate, gl_aCAPublicKeyList[i].szExpireDate) > 0) {
					EMV_TRACE_ERR_MSG
					return(EMV_DAUTH_INDEX_NOT_SUPPORT); // 如果CA公钥过期,当做没找到
				}
				break; // 找到
			}
		}
	}
	if(i >= gl_iTermSupportedCaPublicKeyNum) {
		EMV_TRACE_ERR_MSG
		return(EMV_DAUTH_INDEX_NOT_SUPPORT); // CA公钥没找到
	}

	// 找到CA公钥
	if(gl_aCAPublicKeyList[i].lE == 3)
		ProtoKey.useFermat4 = 0; // 3
	else if(gl_aCAPublicKeyList[i].lE == 65537L)
		ProtoKey.useFermat4 = 1; // 65537
	else {
	    ASSERT(0);
		EMV_TRACE_ERR_MSG
		return(EMV_DAUTH_NATIVE);
	}
	ProtoKey.bits = gl_aCAPublicKeyList[i].ucKeyLen * 8; // CA公钥长度
	RSASetPublicKey(&sg_CaPubKey, gl_aCAPublicKeyList[i].sKey, &ProtoKey);

	return(EMV_DAUTH_OK);
}

// 解析出发卡行公钥
// ret : EMV_DAUTH_OK				 : OK
//       EMV_DAUTH_DATA_MISSING		 : 数据缺失
//       EMV_DAUTH_DATA_ILLEGAL		 : 数据非法
//       EMV_DAUTH_NATIVE			 : 内部错误
//       EMV_DAUTH_INDEX_NOT_SUPPORT : CA公钥索引不支持
//       EMV_DAUTH_FAIL              : 导致脱机数据认证失败的其它错误
int iEmvGetIssuerPubKey(void)
{
	// need T90(Issuer Public Key Certificate)
	//      T92(Issuer Public Key Remainder)
	//      T9F32(Issuer Public Key Exponent)
	//      T5A(PAN)
	int    iRet;
	uchar  *p;
	ushort uiOutLen;
	uchar  sBuf[256], szBuf[512];
	uchar  sHashCode[20];
	uchar  *psT90Value, *psT92Value, *psT9F32Value, *psT5AValue;
	int    iT90ValueLen, iT92ValueLen, iT9F32ValueLen, iT5AValueLen;
	int    i;
	R_RSA_PROTO_KEY ProtoKey;
	uchar *pucCaPubKeyIndex; // T8F(Certification Authority Public Key Index)
	uchar *psRid;			 // Rid

	if(sg_IssuerPubKey.bits)
		return(EMV_DAUTH_OK);    // 已经解析出了发卡行公钥
    iRet = iEmvSearchCaPubKey(); // 首先搜索CA公钥
	if(iRet != EMV_DAUTH_OK)
		return(iRet);

	// 取所需数据
	iT90ValueLen = iTlvGetObjValue(gl_sTlvDbCard, TAG_90_IssuerPubKeyCert, &psT90Value);
	if(iT90ValueLen < 0) {
		EMV_TRACE_ERR_MSG
		return(EMV_DAUTH_DATA_MISSING);
	}
	iT92ValueLen = iTlvGetObjValue(gl_sTlvDbCard, TAG_92_IssuerPubKeyRem, &psT92Value);
	if(iT92ValueLen < 0)
		iT92ValueLen = 0; // have no remainder
	iT9F32ValueLen = iTlvGetObjValue(gl_sTlvDbCard, TAG_9F32_IssuerPubKeyExp, &psT9F32Value);
	if(iT9F32ValueLen < 0) {
		EMV_TRACE_ERR_MSG
		return(EMV_DAUTH_DATA_MISSING);
	}
	iT5AValueLen = iTlvGetObjValue(gl_sTlvDbCard, TAG_5A_PAN, &psT5AValue);
	if(iT5AValueLen < 0) {
	    ASSERT(iT5AValueLen > 0)
		EMV_TRACE_ERR_MSG
		return(EMV_DAUTH_NATIVE); // T5A为必备项, 不可能缺失
	}

	// 1.check length
	if(iT90ValueLen*8 != sg_CaPubKey.bits) {
		EMV_TRACE_ERR_MSG
		return(EMV_DAUTH_FAIL); // Issuer Certificate长度与CA公钥模数长度不同
	}
	// 2.recover Issuer public key & check trailer '0xBC'
	// sBuf : recovered Issuer Public Key data
	// 1  : Header '0x6A'
	// 1  : Certificate Format, '0x02'
	// 4  : Left 3-8 digits from PAN, padded with HEX 'F's
	// 2  : Expire Date (MMYY)
	// 3  : Certificate Serial Number
	// 1  : Hash Algo
	// 1  : Issuer Public Key Algo
	// 1  : Issuer Public Key Length
	// 1  : Issuer Public Key Exponent Length
	// NCA-36 : Issuer Public Key or leftmost Digits of the Issuer Public Key
	// 20 : Hash Result
	// 1  : Recovered Data Trailer, '0xBC'
    iRet = RSAPublicBlock(sBuf, &uiOutLen, psT90Value, (ushort)iT90ValueLen, &sg_CaPubKey);
	if(iRet) {
		EMV_TRACE_ERR_MSG
		return(EMV_DAUTH_FAIL);
	}
	if(sBuf[iT90ValueLen-1] != 0xbc) {
		EMV_TRACE_ERR_MSG
		return(EMV_DAUTH_FAIL);
	}
	// 3.Check header
	if(sBuf[0] != 0x6A) {
		EMV_TRACE_ERR_MSG
		return(EMV_DAUTH_FAIL);
	}
	// 4.check certificate format
	if(sBuf[1] != 0x02) {
		EMV_TRACE_ERR_MSG
		return(EMV_DAUTH_FAIL);
	}
/****注意.案例2CT.037.00要求如果缺失了余项, 需要设置TVR data missing位,由于Hash值如果不正确则会过早退出公钥获取, 以至于不能设置TVR data missing位, 故在此增加一判断以设置TVR data missing项****/
	if(sBuf[13]>iT90ValueLen-36 && iT92ValueLen==0) {
		// 发卡行公钥余项应该存在但不存在
		EMV_TRACE_ERR_MSG
		return(EMV_DAUTH_DATA_MISSING); // 数据缺失
	}
	if(sBuf[13]>iT90ValueLen-36 && iT92ValueLen!=sBuf[13]-iT90ValueLen+36) {
		// 发卡行公钥余项长度超长或不足
		EMV_TRACE_ERR_MSG
		return(EMV_DAUTH_DATA_ILLEGAL); // 数据非法
	}
/********/
    // 5.6. cal hash
	if(sBuf[11] != 0x01) {
		EMV_TRACE_ERR_MSG
		return(EMV_DAUTH_FAIL); // 0x01 indicate SHA-1
	}
	vSHA1Init();
	vSHA1Update(sBuf+1, (ushort)(iT90ValueLen-1-20-1));
	vSHA1Update(psT92Value, (ushort)iT92ValueLen);
	if(iT9F32ValueLen==1 && memcmp(psT9F32Value, "\x03", 1)==0) {
		vSHA1Update("\x03", 1);
	} else if(iT9F32ValueLen==3 && memcmp(psT9F32Value, "\x01\x00\x01", 3)==0) {
		vSHA1Update("\x01\x00\x01", 3);
	} else {
		EMV_TRACE_ERR_MSG
		return(EMV_DAUTH_FAIL); // exponent error
	}
    vSHA1Result(sHashCode);
    // 7.compare hash result
	if(memcmp(sBuf+iT90ValueLen-1-20, sHashCode, 20) != 0) {
		EMV_TRACE_ERR_MSG
		return(EMV_DAUTH_FAIL);
	}
	// 8.verify part of the PAN
	vOneTwo0(sBuf+2, 4, szBuf); // from certificate
	vOneTwo0(psT5AValue, 4, szBuf+10); // from PAN
	for(i=0; i<8; i++) {
		if(szBuf[i] == 'F')
			szBuf[i] = 0;
	}
	if(strlen(szBuf) < 3)
		return(EMV_DAUTH_FAIL); // must be greater or equal to 3
	if(memcmp(szBuf, szBuf+10, strlen(szBuf)) != 0) {
		EMV_TRACE_ERR_MSG
		return(EMV_DAUTH_FAIL); // left 3-8 digits of PAN not correct
	}
	// 9.check expire date
	iRet = iTlvGetObjValue(gl_sTlvDbTermVar, TAG_9A_TransDate, &p); // YYMMDD
    ASSERT(iRet == 3);
	if(iRet != 3) {
		EMV_TRACE_ERR_MSG
		return(EMV_DAUTH_NATIVE); // T9A必须存在
	}
	memset(szBuf, 0, sizeof(szBuf));
	vOneTwo0(p, 2, szBuf);
	strcat((char *)szBuf, "01");
	vOneTwo(sBuf+6+1, 1, szBuf+50); // YY
	vOneTwo0(sBuf+6, 1, szBuf+50+2); // MM
	strcat((char *)szBuf+50, "01");
	if(iCompDate6(szBuf, szBuf+50) > 0) {
		EMV_TRACE_ERR_MSG
		return(EMV_DAUTH_FAIL); // certificate expired
	}
	// 10.check RID, CA public key index, certificate serial no, 
	iRet = iTlvGetObjValue(gl_sTlvDbCard, TAG_8F_CAPubKeyIndex, &pucCaPubKeyIndex);
	if(iRet < 0) {
		EMV_TRACE_ERR_MSG
		return(EMV_DAUTH_DATA_MISSING); // 数据缺失
	}
	if(iRet != 1) {
		EMV_TRACE_ERR_MSG
		return(EMV_DAUTH_DATA_ILLEGAL); // 数据非法
	}
	iRet = iTlvGetObjValue(gl_sTlvDbCard, TAG_4F_AID, &psRid);
	ASSERT(iRet >= 5);
	if(iRet < 5) {
		EMV_TRACE_ERR_MSG
		return(EMV_DAUTH_NATIVE); // T4F必须存在
	}
	iRet = iEmvIOCRLCheck(sBuf+8);
	ASSERT(iRet != EMVIO_ERROR);
	if(iRet == EMVIO_BLACK) {
		EMV_TRACE_ERR_MSG
		return(EMV_DAUTH_FAIL); // 证书已经被回收
	}
	if(iRet != EMVIO_OK) {
		EMV_TRACE_ERR_MSG
		return(EMV_DAUTH_NATIVE);
	}

	// 11.check issuer public key algo
	if(sBuf[12] != 0x01) {
		EMV_TRACE_ERR_MSG
		return(EMV_DAUTH_FAIL); // 0x01 indicate RSA
	}
	// 12.done
	// 成功解析出发卡行公钥
	if(sBuf[13]>iT90ValueLen-36 && iT92ValueLen==0) {
		// 发卡行公钥余项应该存在但不存在
		EMV_TRACE_ERR_MSG
		return(EMV_DAUTH_DATA_MISSING); // 数据缺失
	}
	if(sBuf[13]>iT90ValueLen-36 && iT92ValueLen!=sBuf[13]-iT90ValueLen+36) {
		// 发卡行公钥余项长度超长或不足
		EMV_TRACE_ERR_MSG
		return(EMV_DAUTH_DATA_ILLEGAL); // 数据非法
	}
	if(sBuf[14] == 1)
		ProtoKey.useFermat4 = 0; // 3
	else if(sBuf[14] == 3)
		ProtoKey.useFermat4 = 1; // 65537
	else {
		EMV_TRACE_ERR_MSG
		return(EMV_DAUTH_FAIL);
	}
	ProtoKey.bits = sBuf[13] * 8;
	memcpy(szBuf, sBuf+15, iT90ValueLen-36); // the left most issuer public key
	memcpy(szBuf+iT90ValueLen-36, psT92Value, iT92ValueLen); // issuer public key remainder
	RSASetPublicKey(&sg_IssuerPubKey, szBuf, &ProtoKey);
    
	return(EMV_DAUTH_OK);
}

// 解析出IC卡公钥
// ret : EMV_DAUTH_OK				 : OK
//       EMV_DAUTH_DATA_MISSING		 : 数据缺失
//       EMV_DAUTH_DATA_ILLEGAL		 : 数据非法
//       EMV_DAUTH_NATIVE			 : 内部错误
//       EMV_DAUTH_INDEX_NOT_SUPPORT : CA公钥索引不支持
//       EMV_DAUTH_FAIL              : 导致脱机数据认证失败的其它错误
int iEmvGetIcPubKey(void)
{
 	// need T9F46(ICC Public Key Certificate)
	//      T9F48(ICC Public Key Remainder)
 	//      T9F47(ICC Public Key Exponent)
	//      T5A(PAN)
	int    iRet;
	uchar  *p;
	ushort uiOutLen;
	uchar  sBuf[256], szBuf[512];
	uchar  sHashCode[20];
	uchar  *psT9F46Value, *psT9F47Value, *psT9F48Value, *psT5AValue;
	int    iT9F46ValueLen, iT9F47ValueLen, iT9F48ValueLen, iT5AValueLen;
	uchar  *psT9F4AValue;
	int    iT9F4AValueLen;
	R_RSA_PROTO_KEY ProtoKey;

	if(sg_IcPubKey.bits)
		return(EMV_DAUTH_OK);    // 已经解析出了IC卡公钥
    iRet = iEmvGetIssuerPubKey(); // 首先解析发卡行公钥
	if(iRet != EMV_DAUTH_OK)
		return(iRet);

	// 取所需数据
	iT9F46ValueLen = iTlvGetObjValue(gl_sTlvDbCard, TAG_9F46_ICCPubKeyCert, &psT9F46Value);
	if(iT9F46ValueLen < 0) {
		EMV_TRACE_ERR_MSG
		return(EMV_DAUTH_DATA_MISSING);
	}
	iT9F48ValueLen = iTlvGetObjValue(gl_sTlvDbCard, TAG_9F48_ICCPubKeyRem, &psT9F48Value);
	if(iT9F48ValueLen < 0)
		iT9F48ValueLen = 0; // have no remainder
	iT9F47ValueLen = iTlvGetObjValue(gl_sTlvDbCard, TAG_9F47_ICCPubKeyExp, &psT9F47Value);
	if(iT9F47ValueLen < 0) {
		EMV_TRACE_ERR_MSG
		return(EMV_DAUTH_DATA_MISSING);
	}
	iT5AValueLen = iTlvGetObjValue(gl_sTlvDbCard, TAG_5A_PAN, &psT5AValue);
	ASSERT(iT5AValueLen > 0);
	if(iT5AValueLen < 0) {
		EMV_TRACE_ERR_MSG
		return(EMV_DAUTH_NATIVE); // T5A为必备项, 不可能缺失
	}

	// 1.check length
	if(iT9F46ValueLen*8 != sg_IssuerPubKey.bits) {
		EMV_TRACE_ERR_MSG
		return(EMV_DAUTH_FAIL); // Icc Certificate长度与发卡方公钥模数长度不同
	}
	// 2.recover Icc public key & check trailer '0xBC'
	// sBuf : recovered Icc Public Key data
	// 1  : Header '0x6A'
	// 1  : Certificate Format, '0x04'
	// 10 : PAN, padded with HEX 'F's
	// 2  : Expire Date (MMYY)
	// 3  : Certificate Serial Number
	// 1  : Hash Algo
	// 1  : Icc Public Key Algo
	// 1  : Icc Public Key Length
	// 1  : Icc Public Key Exponent Length
	// NI-42 : Icc Public Key or leftmost Digits of the Icc Public Key
	// 20 : Hash Result
	// 1  : Recovered Data Trailer, '0xBC'
    iRet = RSAPublicBlock(sBuf, &uiOutLen, psT9F46Value, (ushort)iT9F46ValueLen, &sg_IssuerPubKey);
	if(iRet) {
		EMV_TRACE_ERR_MSG
		return(EMV_DAUTH_FAIL);
	}
	if(sBuf[iT9F46ValueLen-1] != 0xbc) {
		EMV_TRACE_ERR_MSG
		return(EMV_DAUTH_FAIL);
	}
	// 3.Check header
	if(sBuf[0] != 0x6A) {
		EMV_TRACE_ERR_MSG
		return(EMV_DAUTH_FAIL);
	}
	// 4.check certificate format
	if(sBuf[1] != 0x04) {
		EMV_TRACE_ERR_MSG
		return(EMV_DAUTH_FAIL);
	}
/****注意.案例2CT.037.00要求如果缺失了余项, 需要设置TVR data missing位,由于Hash值如果不正确则会过早退出公钥获取, 以至于不能设置TVR data missing位, 故在此增加一判断以设置TVR data missing项****/
	if(sBuf[19]>iT9F46ValueLen-42 && iT9F48ValueLen==0) {
		// IC卡公钥余项应该存在但不存在
		EMV_TRACE_ERR_MSG
		return(EMV_DAUTH_DATA_MISSING); // 数据缺失
	}
	if(sBuf[19]>iT9F46ValueLen-42 && iT9F48ValueLen!=sBuf[19]-iT9F46ValueLen+42) {
		// IC卡公钥余项长度超长或不足
		EMV_TRACE_ERR_MSG
		return(EMV_DAUTH_DATA_ILLEGAL); // 数据非法
	}
/********/
    // 5.6. cal hash
	if(sBuf[17] != 0x01) {
		EMV_TRACE_ERR_MSG
		return(EMV_DAUTH_FAIL); // 0x01 indicate SHA-1
	}
	vSHA1Init();
	vSHA1Update(sBuf+1, (ushort)(iT9F46ValueLen-1-20-1));
	vSHA1Update(psT9F48Value, (ushort)iT9F48ValueLen);
	if(iT9F47ValueLen==1 && memcmp(psT9F47Value, "\x03", 1)==0) {
		vSHA1Update("\x03", 1);
	} else if(iT9F47ValueLen==3 && memcmp(psT9F47Value, "\x01\x00\x01", 3)==0) {
		vSHA1Update("\x01\x00\x01", 3);
	} else {
		EMV_TRACE_ERR_MSG
		return(EMV_DAUTH_FAIL); // exponent error
	}

	vSHA1Update(sg_sOfflineRecData, (ushort)sg_iOfflineRecDataLen); // 需脱机认证的所有记录数据

	// search T9F4A (Static Data Authentication Tag List)
	iT9F4AValueLen = iTlvGetObjValue(gl_sTlvDbCard, TAG_9F4A_SDATagList, &psT9F4AValue);
	if(iT9F4AValueLen > 0) {
		// 存在T9F4A (Static Data Authentication Tag List)
		if(memcmp(psT9F4AValue, TAG_82_AIP, 1) != 0) {
			EMV_TRACE_ERR_MSG
			return(EMV_DAUTH_FAIL); // if T9F4A exist, it must only contain T82(AIP)
		}
		iRet = iTlvGetObjValue(gl_sTlvDbCard, TAG_82_AIP, &p); // get AIP
		ASSERT(iRet == 2);
		if(iRet != 2) {
			EMV_TRACE_ERR_MSG
			return(EMV_DAUTH_NATIVE); // AIP不可能缺失或出错
		}
		vSHA1Update(p, 2);
	}
    vSHA1Result(sHashCode);
    // 7.compare hash result
	if(memcmp(sBuf+iT9F46ValueLen-1-20, sHashCode, 20) != 0) {
		EMV_TRACE_ERR_MSG
		return(EMV_DAUTH_FAIL);
	}
	// 8.verify PAN
	vOneTwo0(sBuf+2, 10, szBuf); // from certificate
	vOneTwo0(psT5AValue, iT5AValueLen, szBuf+50); // from PAN
	strcat(szBuf+50, "FFFFFFFFFFFFFFFFFFFF");
	if(memcmp(szBuf, szBuf+50, 20) != 0) {
		EMV_TRACE_ERR_MSG
		return(EMV_DAUTH_FAIL); // PAN not correct
	}
	// 9.check expire date
	iRet = iTlvGetObjValue(gl_sTlvDbTermVar, TAG_9A_TransDate, &p); // YYMMDD
	ASSERT(iRet == 3);
	if(iRet != 3) {
		EMV_TRACE_ERR_MSG
		return(EMV_DAUTH_NATIVE); // T9A必须存在
	}
	memset(szBuf, 0, sizeof(szBuf));
	vOneTwo0(p, 2, szBuf);
	strcat((char *)szBuf, "01");
	vOneTwo(sBuf+12+1, 1, szBuf+50); // YY
	vOneTwo0(sBuf+12, 1, szBuf+50+2); // MM
	strcat((char *)szBuf+50, "01");
	if(iCompDate6(szBuf, szBuf+50) > 0) {
		EMV_TRACE_ERR_MSG
		return(EMV_DAUTH_FAIL); // certificate expired
	}
	// 10.checi icc public key algo
	if(sBuf[18] != 0x01) {
		EMV_TRACE_ERR_MSG
		return(EMV_DAUTH_FAIL); // 0x01 indicate RSA
	}
	// 11.done
	// 成功解析出IC卡公钥
	if(sBuf[19]>iT9F46ValueLen-42 && iT9F48ValueLen==0) {
		// IC卡公钥余项应该存在但不存在
		EMV_TRACE_ERR_MSG
		return(EMV_DAUTH_DATA_MISSING); // 数据缺失
	}
	if(sBuf[19]>iT9F46ValueLen-42 && iT9F48ValueLen!=sBuf[19]-iT9F46ValueLen+42) {
		// IC卡公钥余项长度超长或不足
		EMV_TRACE_ERR_MSG
		return(EMV_DAUTH_DATA_ILLEGAL); // 数据非法
	}

	ProtoKey.bits = sBuf[19] * 8;
	if(sBuf[20] == 1)
		ProtoKey.useFermat4 = 0; // 3
	else
		ProtoKey.useFermat4 = 1; // 65537
	memcpy(szBuf, sBuf+21, iT9F46ValueLen-42); // left most icc public key
	memcpy(szBuf+iT9F46ValueLen-42, psT9F48Value, iT9F48ValueLen); // icc public key remainder
	RSASetPublicKey(&sg_IcPubKey, szBuf, &ProtoKey);
    
	return(EMV_DAUTH_OK);
}

// SDA数据认证
// ret : EMV_DAUTH_OK				 : OK
//       EMV_DAUTH_DATA_MISSING		 : 数据缺失
//       EMV_DAUTH_NATIVE			 : 内部错误
//       EMV_DAUTH_FAIL              : 导致脱机数据认证失败的其它错误
int iEmvSda(void)
{
	// need : T93(Signed Static Application Data)
	int    iRet;
	ushort uiOutLen;
	uchar  sBuf[256], szBuf[512];
	uchar  sHashCode[20];
	uchar  *psT93Value, *psT9F4AValue;
	int    iT93ValueLen, iT9F4AValueLen;
	uchar  *p;

	if(sg_iOfflineRecDataErrFlag) {
		EMV_TRACE_ERR_MSG
		return(EMV_DAUTH_FAIL); // 静态认证数据记录有误
	}
	if(sg_IssuerPubKey.bits == 0) {
		EMV_TRACE_ERR_MSG
		return(EMV_DAUTH_FAIL); // 发卡行公钥未被正确解析出来
	}

	// 取所需数据
	iT93ValueLen = iTlvGetObjValue(gl_sTlvDbCard, TAG_93_SSAD, &psT93Value);
	if(iT93ValueLen < 0) {
		EMV_TRACE_ERR_MSG
		return(EMV_DAUTH_DATA_MISSING);
	}

	// 1.check length
	if(iT93ValueLen*8 != sg_IssuerPubKey.bits) {
		EMV_TRACE_ERR_MSG
		return(EMV_DAUTH_FAIL);
	}
	// 2.recover Signed Static Application Data & check trailer '0xBC'
	// sBuf : recovered Signed Static Application Data
	// 1  : Header '0x6A'
	// 1  : Signed Data Format, '0x03'
	// 1  : Hash Algo
	// 2  : Data Authentication Code
	// NI-26 : Pad Pattern, '0xBB's
	// 20 : Hash Result
	// 1  : Recovered Data Trailer, '0xBC'
    iRet = RSAPublicBlock(sBuf, &uiOutLen, psT93Value, (ushort)iT93ValueLen, &sg_IssuerPubKey);
	if(iRet) {
		EMV_TRACE_ERR_MSG
		return(EMV_DAUTH_FAIL);
	}
	if(sBuf[iT93ValueLen-1] != 0xbc) {
		EMV_TRACE_ERR_MSG
		return(EMV_DAUTH_FAIL);
	}
	// 3.Check header
	if(sBuf[0] != 0x6A) {
		EMV_TRACE_ERR_MSG
		return(EMV_DAUTH_FAIL);
	}
	// 4.check signed data format
	if(sBuf[1] != 0x03) {
		EMV_TRACE_ERR_MSG
		return(EMV_DAUTH_FAIL);
	}
	// 5.6.cal hash
	if(sBuf[2] != 0x01) {
		EMV_TRACE_ERR_MSG
		return(EMV_DAUTH_FAIL); // 0x01 indicate SHA-1
	}
	vSHA1Init();
	vSHA1Update(sBuf+1, (ushort)(iT93ValueLen-1-20-1));
	vSHA1Update(sg_sOfflineRecData, (ushort)sg_iOfflineRecDataLen);

	// search T9F4A (Static Data Authentication Tag List)
	iT9F4AValueLen = iTlvGetObjValue(gl_sTlvDbCard, TAG_9F4A_SDATagList, &psT9F4AValue);
	if(iT9F4AValueLen > 0) {
		// 存在T9F4A (Static Data Authentication Tag List)
		if(memcmp(psT9F4AValue, TAG_82_AIP, 1) != 0) {
			EMV_TRACE_ERR_MSG
			return(EMV_DAUTH_FAIL); // if T9F4A exist, it must only contain T82(AIP)
		}
		iRet = iTlvGetObjValue(gl_sTlvDbCard, TAG_82_AIP, &p); // Get AIP
		if(iRet != 2) {
			EMV_TRACE_ERR_MSG
			return(EMV_DAUTH_NATIVE); // AIP不应该不存在
		}
		vSHA1Update(p, 2);
	}
    vSHA1Result(sHashCode);
    // 7.compare hash result
	if(memcmp(sBuf+iT93ValueLen-1-20, sHashCode, 20) != 0) {
		EMV_TRACE_ERR_MSG
		return(EMV_DAUTH_FAIL);
	}

	// done
	// store T9F45(Data Authentication Code)
    iTlvMakeObject(TAG_9F45_DAC, 2, sBuf+3, szBuf, sizeof(szBuf));
    iRet = iTlvAddObj(gl_sTlvDbCard, szBuf, TLV_CONFLICT_ERR);
	if(iRet < 0) {
		EMV_TRACE_ERR_MSG
		return(EMV_DAUTH_FAIL);
	}
	return(EMV_DAUTH_OK);
}

// DDA数据认证
// ret : EMV_DAUTH_OK				 : OK
//       EMV_DAUTH_DATA_MISSING		 : 数据缺失
//       EMV_DAUTH_NATIVE			 : 内部错误
//       EMV_DAUTH_DATA_ILLEGAL      : 数据非法
//       EMV_DAUTH_FAIL              : 导致脱机数据认证失败的其它错误
//       EMV_DAUTH_CARD_IO			 : 卡片操作失败
//       EMV_DAUTH_CARD_SW			 : 非法卡状态字
int iEmvDda(void)
{
	// need : T9F49(DDOL from ICC)
	int    iRet;
	ushort uiOutLen;
	uchar  ucLenOut;
	uchar  sBuf[256], szBuf[512];
	uchar  sHashCode[20];
	uchar  *psT9F49Value;
	int    iT9F49ValueLen;
	uchar  sDDOLBody[256];  // 按DDOL打包后的内容
	int    iDDOLBodyLen;	// 按DDOL打包后的内容长度
	uchar  sRand[4];
	uchar  ucIccDynamicDataLen, sIccDynamicData[256];
	int    iSignedDynamicDataLen;
	uchar  sSignedDynamicData[248];
	uchar  *p;

	if(sg_IcPubKey.bits == 0) {
		EMV_TRACE_ERR_MSG
		return(EMV_DAUTH_FAIL); // IC卡公钥未被正确解析出来
	}

	// 取所需数据
	iT9F49ValueLen = iTlvGetObjValue(gl_sTlvDbCard, TAG_9F49_DDOL, &psT9F49Value); // DDOL
	if(iT9F49ValueLen <= 0) {
		// DDOL from ICC not found, use default DDOL
		iT9F49ValueLen = iTlvGetObjValue(gl_sTlvDbTermVar, TAG_DFXX_DefaultDDOL, &psT9F49Value); // DDOL
		if(iT9F49ValueLen < 0) {
			EMV_TRACE_ERR_MSG
			return(EMV_DAUTH_FAIL); // 卡片没提供DDOL且终端没提供缺省DDOL
		}
	}
	// search T9F37, 随机数
    iRet =  iTlvSearchDOLTag(psT9F49Value, iT9F49ValueLen, TAG_9F37_TermRand, NULL); // T9F37, term rand
    if(iRet <= 0) {
		EMV_TRACE_ERR_MSG
		return(EMV_DAUTH_FAIL); // DDOL中没有要求随机数
	}
	vGetRand(sRand, 4);
	iRet = iTlvMakeAddObj(gl_sTlvDbTermVar, TAG_9F37_TermRand, 4, sRand, TLV_CONFLICT_REPLACE);
	if(iRet < 0) {
		EMV_TRACE_ERR_MSG
		return(EMV_DAUTH_NATIVE);
	}

    // 打包DDOL
	iDDOLBodyLen = iTlvMakeDOLBlock(sDDOLBody, sizeof(sDDOLBody), psT9F49Value, iT9F49ValueLen,
          				            gl_sTlvDbTermFixed, gl_sTlvDbTermVar, gl_sTlvDbCard, NULL);
	if(iDDOLBodyLen < 0) {
		EMV_TRACE_ERR_MSG
		return(EMV_DAUTH_NATIVE);
	}

	// 执行内部认证指令
	iRet = uiEmvCmdInternalAuth((uchar)iDDOLBodyLen, sDDOLBody, &ucLenOut, sBuf);
	if(iRet == 1)
		return(EMV_DAUTH_CARD_IO); // 卡片操作失败
	if(iRet)
		return(EMV_DAUTH_CARD_SW); // 非法卡状态字


	iRet = iTlvCheckTlvObject(sBuf);
	if(iRet < 0)
		return(EMV_DAUTH_FAIL);
    if(iRet > (int)ucLenOut)
		return(EMV_DAUTH_DATA_ILLEGAL);

	if(sBuf[0] == 0x80) {
		// format 1
		iSignedDynamicDataLen = iTlvValue(sBuf, &p);
		if(iSignedDynamicDataLen <= 0) {
			EMV_TRACE_ERR_MSG
			return(EMV_DAUTH_DATA_ILLEGAL);
		}
		memcpy(sSignedDynamicData, p, iSignedDynamicDataLen);
	} else if(sBuf[0] == 0x77) {
		// format 2
        iTlvSetBuffer(szBuf, sizeof(szBuf)); // 用szBuf作临时TLV数据库
		iRet = iTlvBatchAddObj(0/*0:add only primitive TLV object*/, szBuf, sBuf, iRet, TLV_CONFLICT_ERR, 0/*0:长度为0对象不添加*/);
        if(iRet < 0) {
			EMV_TRACE_ERR_MSG
			return(EMV_DAUTH_DATA_ILLEGAL);
		}
		iSignedDynamicDataLen = iTlvGetObjValue(szBuf, TAG_9F4B_SDAData, &p);
		if(iSignedDynamicDataLen <= 0) {
			EMV_TRACE_ERR_MSG
			return(EMV_DAUTH_DATA_ILLEGAL); // Refer to SU54
		}
		memcpy(sSignedDynamicData, p, iSignedDynamicDataLen);
	} else {
		EMV_TRACE_ERR_MSG
		return(EMV_DAUTH_DATA_ILLEGAL);
	}

	// verify
	// 1.check length
	if(iSignedDynamicDataLen*8 != sg_IcPubKey.bits) {
		EMV_TRACE_ERR_MSG
//		return(EMV_DAUTH_DATA_ILLEGAL);
		return(EMV_DAUTH_FAIL); // 案例2CC.078.00_0, 长度不符, 认为DDA失败, 需要继续交易
	}
	// 2.recover signed dynamic application data & check trailer '0xBC'
	// sBuf : recovered signed dynamic application data
	// 1   : Header '0x6A'
	// 1   : Signed Dynamic Application Data format, '0x05'
	// 1   : Hash Algo
	// 1   : ICC Dynamic Data Length
	// Ldd : ICC Dynamic Data
	// Nic-Ldd-25 : Pad Pattern, '0xBB's
	// 20  : Hash Result
	// 1   : Trailer '0xBC'
    iRet = RSAPublicBlock(sBuf, &uiOutLen, sSignedDynamicData, (ushort)iSignedDynamicDataLen, &sg_IcPubKey);
	if(iRet) {
		EMV_TRACE_ERR_MSG
		return(EMV_DAUTH_FAIL);
	}
	if(sBuf[iSignedDynamicDataLen-1] != 0xbc) {
		EMV_TRACE_ERR_MSG
		return(EMV_DAUTH_FAIL);
	}
	ucIccDynamicDataLen = sBuf[3]; // Ldd
	memcpy(sIccDynamicData, sBuf+4, ucIccDynamicDataLen);

	// 3.Check header
	if(sBuf[0] != 0x6A) {
		EMV_TRACE_ERR_MSG
		return(EMV_DAUTH_FAIL);
	}
	// 4.check signed data format
	if(sBuf[1] != 0x05) {
		EMV_TRACE_ERR_MSG
		return(EMV_DAUTH_FAIL);
	}
	// 5.6.cal hash
	if(sBuf[2] != 0x01) {
		EMV_TRACE_ERR_MSG
		return(EMV_DAUTH_FAIL); // 0x01 indicate SHA-1
	}
	vSHA1Init();
	vSHA1Update(sBuf+1, (ushort)(iSignedDynamicDataLen-1-20-1));
	vSHA1Update(sDDOLBody, (ushort)iDDOLBodyLen);
    vSHA1Result(sHashCode);
    // 7.compare hash result
	if(memcmp(sBuf+iSignedDynamicDataLen-1-20, sHashCode, 20) != 0) {
		EMV_TRACE_ERR_MSG
		return(EMV_DAUTH_FAIL);
	}

    // done, store T9F4C(ICC Dynamic Number), T9F26(AC)
	// Left most of sIccDynamicData :
	// 1  : ICC Dynamic Number Length
	// 2-8: ICC Dynamic Number
	if(sIccDynamicData[0]<2 || sIccDynamicData[0]>8 || sIccDynamicData[0]+1>ucIccDynamicDataLen) {
		EMV_TRACE_ERR_MSG
		return(EMV_DAUTH_FAIL); // 如果IDD非法, 认为动态数据认证失败
	}
	iRet = iTlvMakeAddObj(gl_sTlvDbCard, TAG_9F4C_ICCDynNumber, sIccDynamicData[0], sIccDynamicData+1, TLV_CONFLICT_REPLACE);
	if(iRet < 0) {
		EMV_TRACE_ERR_MSG
		return(EMV_DAUTH_NATIVE);
	}
	return(EMV_DAUTH_OK);
}

// CDA数据认证
// in  : ucGacFlag              : 1:第一次GAC 2:第二次GAC
//       psGacDataOut           : GAC指令返回数据
//       ucGacDataOutLen        : GAC指令返回数据长度
// ret : EMV_DAUTH_OK			: OK
//       EMV_DAUTH_DATA_MISSING	: 数据缺失
//       EMV_DAUTH_NATIVE		: 内部错误
//       EMV_DAUTH_FAIL         : 导致脱机数据认证失败的其它错误
// Note: 调用本函数后, gl_sTlvDbCard中得CID可能会改变(解出来的CID与明文CID不符, 会用解出来的CID替换明文CID)
int iEmvCda(uchar ucGacFlag, uchar *psGacDataOut, uchar ucGacDataOutLen)
{
	ushort uiOutLen;
	uchar  sBuf[256];
	uchar  sTmpTlvBuf[280];
	uchar  ucIccDynamicDataLen, sIccDynamicData[256];
	uchar  sHashCode[20];
	uchar  *psT9F4BValue; // T9F4B, Signed Dynamic Application Data
	int    iT9F4BValueLen;
	uchar  *p;
	int    iRet;
	int    i;

	// get data
	iT9F4BValueLen = iTlvGetObjValue(gl_sTlvDbCard, TAG_9F4B_SDAData, &psT9F4BValue);
	if(iT9F4BValueLen < 0) {
		EMV_TRACE_ERR_MSG
		return(EMV_DAUTH_DATA_MISSING);
	}
	
	// 1.Check length
	if(iT9F4BValueLen*8 != sg_IcPubKey.bits) {
		EMV_TRACE_ERR_MSG
		return(EMV_DAUTH_FAIL);
	}
	// 2.recover Signed Dynamic Application Data & check trailer '0xBC'
	// sBuf : recovered Signed Dynamic Application Data
	// 1  : Header '0x6A'
	// 1  : Signed Data Format, '0x05'
	// 1  : Hash Algo
	// 1  : ICC Dynamic Data Length(Ldd)
	// Ldd: ICC Dynamic Data
	// Nic-Ldd-25: Pad Pattern
	// 20 : Hash Result
	// 1  : Recovered Data Trailer, '0xBC'
    iRet = RSAPublicBlock(sBuf, &uiOutLen, psT9F4BValue, (ushort)iT9F4BValueLen, &sg_IcPubKey);
	if(iRet) {
		EMV_TRACE_ERR_MSG
		return(EMV_DAUTH_FAIL);
	}
	if(sBuf[iT9F4BValueLen-1] != 0xbc) {
		EMV_TRACE_ERR_MSG
		return(EMV_DAUTH_FAIL);
	}
	// 3.Check header
	if(sBuf[0] != 0x6A) {
		EMV_TRACE_ERR_MSG
		return(EMV_DAUTH_FAIL);
	}
	// 4.check Signed Data format
	if(sBuf[1] != 0x05) {
		EMV_TRACE_ERR_MSG
		return(EMV_DAUTH_FAIL);
	}
	// 5.retrieve ICC Dynamic Data
	// Left most of sIccDynamicData :
	// 1  : ICC Dynamic Number Length
	// 2-8: ICC Dynamic Number
	// 1  : Cryptogram Information Data
	// 8  : TC or ARQC
	// 20 : Transaction Data Hash Code
	ucIccDynamicDataLen = sBuf[3]; // Ldd
	memcpy(sIccDynamicData, sBuf+4, ucIccDynamicDataLen);
    // 6.check CID
    iRet = iTlvGetObjValue(gl_sTlvDbCard, TAG_9F27_CID, &p); // Get CID from TLV数据库
	if(iRet != 1) {
		EMV_TRACE_ERR_MSG
		return(EMV_DAUTH_FAIL);
	}
	if(*p != sIccDynamicData[1+sIccDynamicData[0]]) {
		// 解出来的CID与明文CID不符, 用解出来的CID替换明文CID
		iTlvMakeAddObj(gl_sTlvDbCard, TAG_9F27_CID, 1, &sIccDynamicData[1+sIccDynamicData[0]], TLV_CONFLICT_REPLACE);
		EMV_TRACE_ERR_MSG
		return(EMV_DAUTH_FAIL);
	}
	// 7.8.cal hash
	if(sBuf[2] != 0x01) {
		EMV_TRACE_ERR_MSG
		return(EMV_DAUTH_FAIL); // 0x01 indicate SHA-1
	}
	vSHA1Init();
	vSHA1Update(sBuf+1, (ushort)(iT9F4BValueLen-1-20-1));
    iTlvGetObjValue(gl_sTlvDbTermVar, TAG_9F37_TermRand, &p); // Get Term Rand
	vSHA1Update(p, (ushort)4);
    vSHA1Result(sHashCode);
    // 9.compare hash result
	if(memcmp(sBuf+iT9F4BValueLen-1-20, sHashCode, 20) != 0) {
		EMV_TRACE_PROC
		return(EMV_DAUTH_FAIL);
	}
	// 10.11. cal Transaction Data Hash Code
	vSHA1Init();
	iRet = iTlvGetObjValue(gl_sTlvDbTermVar, TAG_DFXX_PdolDataBlock, &p);
	if(iRet > 0)
		vSHA1Update(p, (ushort)iRet); // PDOL Data Block
	iRet = iTlvGetObjValue(gl_sTlvDbTermVar, TAG_DFXX_Cdol1DataBlock, &p);
	if(iRet <= 0) {
		EMV_TRACE_PROC
		return(EMV_DAUTH_NATIVE);
	}
	vSHA1Update(p, (ushort)iRet); // CDOL1 Data Block
	if(ucGacFlag == 2) {
		// 是第二次GAC
		iRet = iTlvGetObjValue(gl_sTlvDbTermVar, TAG_DFXX_Cdol2DataBlock, &p);
		if(iRet <= 0) {
			EMV_TRACE_PROC
			return(EMV_DAUTH_NATIVE);
		}
		vSHA1Update(p, (ushort)iRet); // CDOL2 Data Block
	}

	// 将GAC返回数据写入临时TLV数据库(去除T9F4B, Signed Dynamic Application Data), 用于CDA验证
	iTlvSetBuffer(sTmpTlvBuf,  sizeof(sTmpTlvBuf));    // 初始化临时TLV数据库
	if(psGacDataOut[0] != 0x77) {
		EMV_TRACE_PROC
		return(EMV_DAUTH_FAIL);
	}
    iRet = iTlvBatchAddObj(1/*not only primitive*/, sTmpTlvBuf, psGacDataOut, ucGacDataOutLen, TLV_CONFLICT_ERR, 1/*1:长度为0对象也添加*/); // 卡片返回数据
	if(iRet < 0) {
		EMV_TRACE_PROC
		return(EMV_DAUTH_FAIL);
	}
	iTlvDelObj(sTmpTlvBuf, TAG_9F4B_SDAData); // 从临时TLV数据库中去除Signed Dynamic Application Data
	for(i=0; ; i++) {
		iRet = iTlvGetObjByIndex(sTmpTlvBuf, (ushort)i, &p);
		if(iRet < 0)
			break;
    	vSHA1Update(p, (ushort)iRet);
	}
    vSHA1Result(sHashCode); // get Transaction Data Hash Code
	// 12.compare Transaction Data Hash Code
	if(memcmp(sIccDynamicData+1+sIccDynamicData[0]+1+8, sHashCode, 20) != 0) {
		EMV_TRACE_PROC
		return(EMV_DAUTH_FAIL);
	}

	// done, store T9F4C(ICC Dynamic Number), T9F26(AC)
	// Left most of sIccDynamicData :
	// 1  : ICC Dynamic Number Length
	// 2-8: ICC Dynamic Number
	// 1  : Cryptogram Information Data
	// 8  : TC or ARQC
	// 20 : Transaction Data Hash Code
	if(sIccDynamicData[0]<2 || sIccDynamicData[0]>8 || sIccDynamicData[0]+1>ucIccDynamicDataLen) {
		EMV_TRACE_PROC
		return(EMV_DAUTH_FAIL); // 如果IDD非法, 认为动态数据认证失败
	}
	iRet = iTlvMakeAddObj(gl_sTlvDbCard, TAG_9F4C_ICCDynNumber, sIccDynamicData[0], sIccDynamicData+1, TLV_CONFLICT_REPLACE);
	if(iRet < 0) {
		EMV_TRACE_PROC
		return(EMV_DAUTH_NATIVE);
	}
	iRet = iTlvMakeAddObj(gl_sTlvDbCard, TAG_9F26_AC, 8, sIccDynamicData+1+sIccDynamicData[0]+1, TLV_CONFLICT_REPLACE);
	if(iRet < 0) {
		EMV_TRACE_PROC
		return(EMV_DAUTH_NATIVE);
	}

	return(EMV_DAUTH_OK);
}

// FDDA数据认证
// ret : EMV_DAUTH_OK				 : OK
//       EMV_DAUTH_DATA_MISSING		 : 数据缺失
//       EMV_DAUTH_NATIVE			 : 内部错误
//       EMV_DAUTH_DATA_ILLEGAL      : 数据非法
//       EMV_DAUTH_FAIL              : 导致脱机数据认证失败的其它错误
// Note: 参考JR/T0025.12—2013 附录B, P47
//       FDDA失败后, 应用层需要自行检查9F6C B1 b6b5, 以决定是拒绝还是进行联机
int iEmvFdda(void)
{
	int    iRet;
	ushort uiOutLen;
	uchar  sBuf[256];
	uchar  sHashCode[20];
	int    iSignedDynamicDataLen;
	uchar  sSignedDynamicData[248];
	int    iCardAuthRelateDataLen;
	uchar  sCardAuthRelateData[16];
	uchar  ucFddaVer; // 00 0r 01
	uchar  *p;

	if(sg_IcPubKey.bits == 0) {
		EMV_TRACE_PROC
		return(EMV_DAUTH_FAIL); // IC卡公钥未被正确解析出来
	}

	// 获取签字后动态数据
	iSignedDynamicDataLen = iTlvGetObjValue(gl_sTlvDbCard, TAG_9F4B_SDAData, &p);
	if(iSignedDynamicDataLen < 0) {
		EMV_TRACE_PROC
		return(EMV_DAUTH_FAIL);
	}
	memcpy(sSignedDynamicData, p, iSignedDynamicDataLen);

	// 获取卡片认证相关数据, T9F69
	iCardAuthRelateDataLen = iTlvGetObjValue(gl_sTlvDbCard, TAG_9F69_CardAuthRelatedData, &p);
	if(iCardAuthRelateDataLen > 0)
		memcpy(sCardAuthRelateData, p, iCardAuthRelateDataLen);
	ucFddaVer = 0; // 假设fdda版本号为00
	if(iCardAuthRelateDataLen > 0) {
		if(sCardAuthRelateData[0] == 1)
			ucFddaVer = 1; // 设置fdda版本号为01
		else if(sCardAuthRelateData[0]) {
			EMV_TRACE_PROC
			return(EMV_DAUTH_FAIL); // 不识别的fdda版本
		}
	}
	vTraceWriteTxtCr(TRACE_PROC, "FDDA-%02d", (int)ucFddaVer);

	if(!iEmvTestItemBit(EMV_ITEM_AIP, AIP_02_DDA_SUPPORTED)) {
		EMV_TRACE_PROC
		return(EMV_DAUTH_FAIL); // AIP指明不支持DDA
	}
	if(ucFddaVer == 0x00) {
		iRet = iTlvGetObjValue(gl_sTlvDbTermVar, TAG_9F66_ContactlessSupport, &p);
		if(iRet == 4) {
			if((p[3]&0x80) == 0x80) {
				// 终端表明支持fDDA-01, 判断卡片版本
				iRet = iTlvGetObjValue(gl_sTlvDbCard, TAG_9F08_AppVerCard, &p);
				if(iRet == 2) {
//    				if(memcmp(p, "\x00\x30", 2) == 0)
					if(memcmp(p, "\x00\x30", 2) >= 0) { // PBOC公告03, 3.0或以上版本必须支持fdda-01
						EMV_TRACE_PROC
	    				return(EMV_DAUTH_FAIL); // 3.0规范卡, 但不支持01版本fdda
					}
				}
			}
		}
	}

	// verify
	// 1.check length
	if(iSignedDynamicDataLen*8 != sg_IcPubKey.bits) {
		EMV_TRACE_PROC
		return(EMV_DAUTH_FAIL);
	}
	// 2.recover signed dynamic application data & check trailer '0xBC'
	// sBuf : recovered signed dynamic application data
	// 1   : Header '0x6A'
	// 1   : Signed Dynamic Application Data format, '0x05'
	// 1   : Hash Algo
	// 1   : ICC Dynamic Data Length
	// Ldd : ICC Dynamic Data
	// Nic-Ldd-25 : Pad Pattern, '0xBB's
	// 20  : Hash Result
	// 1   : Trailer '0xBC'
    iRet = RSAPublicBlock(sBuf, &uiOutLen, sSignedDynamicData, (ushort)iSignedDynamicDataLen, &sg_IcPubKey);
	if(iRet) {
		EMV_TRACE_PROC
		return(EMV_DAUTH_FAIL);
	}
	if(sBuf[iSignedDynamicDataLen-1] != 0xbc) {
		EMV_TRACE_PROC
		return(EMV_DAUTH_FAIL);
	}
	// 3.Check header
	if(sBuf[0] != 0x6A) {
		EMV_TRACE_PROC
		return(EMV_DAUTH_FAIL);
	}
	// 4.check signed data format
	if(sBuf[1] != 0x05) {
		EMV_TRACE_PROC
		return(EMV_DAUTH_FAIL);
	}
	// 5.6.cal hash
	if(sBuf[2] != 0x01) {
		EMV_TRACE_PROC
		return(EMV_DAUTH_FAIL); // 0x01 indicate SHA-1
	}
	vSHA1Init();
	vSHA1Update(sBuf+1, (ushort)(iSignedDynamicDataLen-1-20-1));

	// 终端随机数
	iRet = iTlvGetObjValue(gl_sTlvDbTermVar, TAG_9F37_TermRand, &p);
	if(iRet != 4) {
		EMV_TRACE_PROC
		return(EMV_DAUTH_FAIL);
	}
	vSHA1Update(p, 4);
	if(ucFddaVer == 0x01) {
		// FDDA版本01
		// 授权金额
		iRet = iTlvGetObjValue(gl_sTlvDbTermVar, TAG_9F02_AmountN, &p);
		if(iRet != 6) {
			EMV_TRACE_PROC
			return(EMV_DAUTH_FAIL);
		}
		vSHA1Update(p, 6);
		// 货币代码
		iRet = iTlvGetObjValue(gl_sTlvDbTermVar, TAG_5F2A_TransCurrencyCode, &p);
		if(iRet != 2) {
			EMV_TRACE_PROC
			return(EMV_DAUTH_FAIL);
		}
		vSHA1Update(p, 2);
		// 卡片认证相关数据, T9F69
		vSHA1Update(sCardAuthRelateData, (ushort)iCardAuthRelateDataLen);
		// 应用交易计数器
		iRet = iTlvGetObjValue(gl_sTlvDbCard, TAG_9F36_ATC, &p);
		if(iRet != 2) {
			EMV_TRACE_PROC
			return(EMV_DAUTH_FAIL);
		}
//		vSHA1Update(p, 2); // wlfxy gemalto card ???
	}

    vSHA1Result(sHashCode);
    // 7.compare hash result
	if(memcmp(sBuf+iSignedDynamicDataLen-1-20, sHashCode, 20) != 0) {
		EMV_TRACE_PROC
		return(EMV_DAUTH_FAIL);
	}

	// done
    return(EMV_DAUTH_OK);
}
