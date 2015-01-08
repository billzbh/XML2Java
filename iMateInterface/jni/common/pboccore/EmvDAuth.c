/**************************************
File name     : EmvDAuth.c
Function      : Emv�ѻ�������֤ģ��
Author        : Yu Jun
First edition : Apr 19th, 2012
Modified      : May 28th, 2013
				    ����fDDA��֤����iEmvFdda()
**************************************/
/*
ģ����ϸ����:
	֧�ֹ����㷨(RSA)��SDA��DDA��CDA
.	׼����Կ
	����RID��CA��Կ�����ҵ�CA��Կ        -> sg_CaPubKey
	��֤�����й�Կ֤��, �����������й�Կ -> sg_IssuerPubKey
	��֤IC����Կ֤��, ������IC����Կ	 -> sg_IcPubKey
.	��¼�ѻ���֤��¼����
		sg_iOfflineRecDataErrFlag; // �ѻ���֤��¼�Ƿ���־ 0:��¼��ȷ 1:��¼�Ƿ�
		sg_iOfflineRecDataLen;     // ��¼�����ܳ���
		sg_sOfflineRecData[];      // ���ѻ���֤�����м�¼����
	�����ѻ�������֤
.	�ѻ�������֤
	SDA��DDA��CDA
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

// SDA��Ҫ��֤��Ӧ�ü�¼����
static int   sg_iOfflineRecDataErrFlag; // �ѻ���֤��¼�Ƿ���־ 0:��¼��ȷ 1:��¼�Ƿ�
static int   sg_iOfflineRecDataLen;     // ��¼�����ܳ���
static uchar sg_sOfflineRecData[4096];  // ���ѻ���֤�����м�¼����

static R_RSA_PUBLIC_KEY sg_CaPubKey;	// CA��Կ
static R_RSA_PUBLIC_KEY sg_IssuerPubKey;// �����й�Կ
static R_RSA_PUBLIC_KEY sg_IcPubKey;	// IC����Կ

// �ѻ�������֤��ʼ��
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

// ����һ��SDA�ѻ���֤��¼����
// in  : ucRecDataLen          : ��¼���ݳ���
//       psRecData             : ��¼����
// ret : EMV_DAUTH_OK		   : OK
//       EMV_DAUTH_LACK_MEMORY : Ԥ���洢�ռ䲻��
int iEmvSDADataAdd(uchar ucRecDataLen, uchar *psRecData)
{
    ASSERT(sg_iOfflineRecDataLen+ucRecDataLen <= sizeof(sg_sOfflineRecData));
    if(sg_iOfflineRecDataLen+ucRecDataLen > sizeof(sg_sOfflineRecData))
        return(1);
    memcpy(sg_sOfflineRecData+sg_iOfflineRecDataLen, psRecData, ucRecDataLen);
    sg_iOfflineRecDataLen += ucRecDataLen;
    return(0);
}

// ����SDA��¼���ݴ���, �ѻ���֤��¼����T70 Tlv����
// ret : EMV_DAUTH_OK		   : OK
int iEmvSDADataErr(void)
{
	sg_iOfflineRecDataErrFlag = 1; // ����ѻ���֤��¼�Ƿ�
	return(EMV_DAUTH_OK);
}

// �ж�SDA�Ƿ��Ѿ�ʧ��
// ret : EMV_DAUTH_OK		   : OK
//       EMV_DAUTH_FAIL		   : SDA�Ѿ�ʧ��
int iEmvSDADataErrTest(void)
{
	if(sg_iOfflineRecDataErrFlag == 1)
		return(EMV_DAUTH_FAIL);
	return(EMV_DAUTH_OK);
}

// ����CA��Կ
// ret : EMV_DAUTH_OK				 : OK
//       EMV_DAUTH_DATA_MISSING		 : ����ȱʧ
//       EMV_DAUTH_DATA_ILLEGAL		 : ���ݷǷ�
//       EMV_DAUTH_NATIVE			 : �ڲ�����
//       EMV_DAUTH_INDEX_NOT_SUPPORT : CA��Կ������֧��
//       EMV_DAUTH_FAIL              : �����ѻ�������֤ʧ�ܵ���������
static int iEmvSearchCaPubKey(void)
{
	int   iRet;
	int   i;
	uchar *pucCaPubKeyIndex; // T8F(Certification Authority Public Key Index)
	uchar *psRid;			 // Rid
	uchar *psTransDate;		 // ��������
	uchar szTransDate[9];    // 8λ��������
	R_RSA_PROTO_KEY ProtoKey;

	if(sg_CaPubKey.bits)
		return(EMV_DAUTH_OK); // �Ѿ���������CA��Կ
	if(sg_iOfflineRecDataErrFlag)
		return(EMV_DAUTH_FAIL); // �ѻ���֤���ݸ�ʽ����, ֱ�ӷ�����֤ʧ��
	iRet = iTlvGetObjValue(gl_sTlvDbCard, TAG_8F_CAPubKeyIndex, &pucCaPubKeyIndex);
	if(iRet < 0) {
		EMV_TRACE_ERR_MSG
		return(EMV_DAUTH_DATA_MISSING); // ����ȱʧ
	}
	if(iRet != 1) {
		EMV_TRACE_ERR_MSG
		return(EMV_DAUTH_DATA_ILLEGAL); // ���ݷǷ�
	}
	iRet = iTlvGetObjValue(gl_sTlvDbCard, TAG_4F_AID, &psRid);
	ASSERT(iRet >= 5)
	if(iRet < 5) {
		EMV_TRACE_ERR_MSG
		return(EMV_DAUTH_NATIVE); // T4F�������
	}
	iRet = iTlvGetObjValue(gl_sTlvDbTermVar, TAG_9A_TransDate, &psTransDate);
	ASSERT(iRet == 3);
	if(iRet != 3) {
		EMV_TRACE_ERR_MSG
		return(EMV_DAUTH_NATIVE); // T9A�������
	}
	if(psTransDate[0] < 0x50)
		strcpy(szTransDate, "20");
	else
		strcpy(szTransDate, "19");
	vOneTwo0(psTransDate, 3, szTransDate+2);

	for(i=0; i<gl_iTermSupportedCaPublicKeyNum; i++) {
		if(gl_aCAPublicKeyList[i].ucIndex == *pucCaPubKeyIndex) {
			if(memcmp(psRid, gl_aCAPublicKeyList[i].sRid, 5) == 0) {
				// CA��Կ����ֵ��RIX��ƥ��, �ҵ�CA��Կ
				if(strcmp(szTransDate, gl_aCAPublicKeyList[i].szExpireDate) > 0) {
					EMV_TRACE_ERR_MSG
					return(EMV_DAUTH_INDEX_NOT_SUPPORT); // ���CA��Կ����,����û�ҵ�
				}
				break; // �ҵ�
			}
		}
	}
	if(i >= gl_iTermSupportedCaPublicKeyNum) {
		EMV_TRACE_ERR_MSG
		return(EMV_DAUTH_INDEX_NOT_SUPPORT); // CA��Կû�ҵ�
	}

	// �ҵ�CA��Կ
	if(gl_aCAPublicKeyList[i].lE == 3)
		ProtoKey.useFermat4 = 0; // 3
	else if(gl_aCAPublicKeyList[i].lE == 65537L)
		ProtoKey.useFermat4 = 1; // 65537
	else {
	    ASSERT(0);
		EMV_TRACE_ERR_MSG
		return(EMV_DAUTH_NATIVE);
	}
	ProtoKey.bits = gl_aCAPublicKeyList[i].ucKeyLen * 8; // CA��Կ����
	RSASetPublicKey(&sg_CaPubKey, gl_aCAPublicKeyList[i].sKey, &ProtoKey);

	return(EMV_DAUTH_OK);
}

// �����������й�Կ
// ret : EMV_DAUTH_OK				 : OK
//       EMV_DAUTH_DATA_MISSING		 : ����ȱʧ
//       EMV_DAUTH_DATA_ILLEGAL		 : ���ݷǷ�
//       EMV_DAUTH_NATIVE			 : �ڲ�����
//       EMV_DAUTH_INDEX_NOT_SUPPORT : CA��Կ������֧��
//       EMV_DAUTH_FAIL              : �����ѻ�������֤ʧ�ܵ���������
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
		return(EMV_DAUTH_OK);    // �Ѿ��������˷����й�Կ
    iRet = iEmvSearchCaPubKey(); // ��������CA��Կ
	if(iRet != EMV_DAUTH_OK)
		return(iRet);

	// ȡ��������
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
		return(EMV_DAUTH_NATIVE); // T5AΪ�ر���, ������ȱʧ
	}

	// 1.check length
	if(iT90ValueLen*8 != sg_CaPubKey.bits) {
		EMV_TRACE_ERR_MSG
		return(EMV_DAUTH_FAIL); // Issuer Certificate������CA��Կģ�����Ȳ�ͬ
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
/****ע��.����2CT.037.00Ҫ�����ȱʧ������, ��Ҫ����TVR data missingλ,����Hashֵ�������ȷ�������˳���Կ��ȡ, �����ڲ�������TVR data missingλ, ���ڴ�����һ�ж�������TVR data missing��****/
	if(sBuf[13]>iT90ValueLen-36 && iT92ValueLen==0) {
		// �����й�Կ����Ӧ�ô��ڵ�������
		EMV_TRACE_ERR_MSG
		return(EMV_DAUTH_DATA_MISSING); // ����ȱʧ
	}
	if(sBuf[13]>iT90ValueLen-36 && iT92ValueLen!=sBuf[13]-iT90ValueLen+36) {
		// �����й�Կ����ȳ�������
		EMV_TRACE_ERR_MSG
		return(EMV_DAUTH_DATA_ILLEGAL); // ���ݷǷ�
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
		return(EMV_DAUTH_NATIVE); // T9A�������
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
		return(EMV_DAUTH_DATA_MISSING); // ����ȱʧ
	}
	if(iRet != 1) {
		EMV_TRACE_ERR_MSG
		return(EMV_DAUTH_DATA_ILLEGAL); // ���ݷǷ�
	}
	iRet = iTlvGetObjValue(gl_sTlvDbCard, TAG_4F_AID, &psRid);
	ASSERT(iRet >= 5);
	if(iRet < 5) {
		EMV_TRACE_ERR_MSG
		return(EMV_DAUTH_NATIVE); // T4F�������
	}
	iRet = iEmvIOCRLCheck(sBuf+8);
	ASSERT(iRet != EMVIO_ERROR);
	if(iRet == EMVIO_BLACK) {
		EMV_TRACE_ERR_MSG
		return(EMV_DAUTH_FAIL); // ֤���Ѿ�������
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
	// �ɹ������������й�Կ
	if(sBuf[13]>iT90ValueLen-36 && iT92ValueLen==0) {
		// �����й�Կ����Ӧ�ô��ڵ�������
		EMV_TRACE_ERR_MSG
		return(EMV_DAUTH_DATA_MISSING); // ����ȱʧ
	}
	if(sBuf[13]>iT90ValueLen-36 && iT92ValueLen!=sBuf[13]-iT90ValueLen+36) {
		// �����й�Կ����ȳ�������
		EMV_TRACE_ERR_MSG
		return(EMV_DAUTH_DATA_ILLEGAL); // ���ݷǷ�
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

// ������IC����Կ
// ret : EMV_DAUTH_OK				 : OK
//       EMV_DAUTH_DATA_MISSING		 : ����ȱʧ
//       EMV_DAUTH_DATA_ILLEGAL		 : ���ݷǷ�
//       EMV_DAUTH_NATIVE			 : �ڲ�����
//       EMV_DAUTH_INDEX_NOT_SUPPORT : CA��Կ������֧��
//       EMV_DAUTH_FAIL              : �����ѻ�������֤ʧ�ܵ���������
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
		return(EMV_DAUTH_OK);    // �Ѿ���������IC����Կ
    iRet = iEmvGetIssuerPubKey(); // ���Ƚ��������й�Կ
	if(iRet != EMV_DAUTH_OK)
		return(iRet);

	// ȡ��������
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
		return(EMV_DAUTH_NATIVE); // T5AΪ�ر���, ������ȱʧ
	}

	// 1.check length
	if(iT9F46ValueLen*8 != sg_IssuerPubKey.bits) {
		EMV_TRACE_ERR_MSG
		return(EMV_DAUTH_FAIL); // Icc Certificate�����뷢������Կģ�����Ȳ�ͬ
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
/****ע��.����2CT.037.00Ҫ�����ȱʧ������, ��Ҫ����TVR data missingλ,����Hashֵ�������ȷ�������˳���Կ��ȡ, �����ڲ�������TVR data missingλ, ���ڴ�����һ�ж�������TVR data missing��****/
	if(sBuf[19]>iT9F46ValueLen-42 && iT9F48ValueLen==0) {
		// IC����Կ����Ӧ�ô��ڵ�������
		EMV_TRACE_ERR_MSG
		return(EMV_DAUTH_DATA_MISSING); // ����ȱʧ
	}
	if(sBuf[19]>iT9F46ValueLen-42 && iT9F48ValueLen!=sBuf[19]-iT9F46ValueLen+42) {
		// IC����Կ����ȳ�������
		EMV_TRACE_ERR_MSG
		return(EMV_DAUTH_DATA_ILLEGAL); // ���ݷǷ�
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

	vSHA1Update(sg_sOfflineRecData, (ushort)sg_iOfflineRecDataLen); // ���ѻ���֤�����м�¼����

	// search T9F4A (Static Data Authentication Tag List)
	iT9F4AValueLen = iTlvGetObjValue(gl_sTlvDbCard, TAG_9F4A_SDATagList, &psT9F4AValue);
	if(iT9F4AValueLen > 0) {
		// ����T9F4A (Static Data Authentication Tag List)
		if(memcmp(psT9F4AValue, TAG_82_AIP, 1) != 0) {
			EMV_TRACE_ERR_MSG
			return(EMV_DAUTH_FAIL); // if T9F4A exist, it must only contain T82(AIP)
		}
		iRet = iTlvGetObjValue(gl_sTlvDbCard, TAG_82_AIP, &p); // get AIP
		ASSERT(iRet == 2);
		if(iRet != 2) {
			EMV_TRACE_ERR_MSG
			return(EMV_DAUTH_NATIVE); // AIP������ȱʧ�����
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
		return(EMV_DAUTH_NATIVE); // T9A�������
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
	// �ɹ�������IC����Կ
	if(sBuf[19]>iT9F46ValueLen-42 && iT9F48ValueLen==0) {
		// IC����Կ����Ӧ�ô��ڵ�������
		EMV_TRACE_ERR_MSG
		return(EMV_DAUTH_DATA_MISSING); // ����ȱʧ
	}
	if(sBuf[19]>iT9F46ValueLen-42 && iT9F48ValueLen!=sBuf[19]-iT9F46ValueLen+42) {
		// IC����Կ����ȳ�������
		EMV_TRACE_ERR_MSG
		return(EMV_DAUTH_DATA_ILLEGAL); // ���ݷǷ�
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

// SDA������֤
// ret : EMV_DAUTH_OK				 : OK
//       EMV_DAUTH_DATA_MISSING		 : ����ȱʧ
//       EMV_DAUTH_NATIVE			 : �ڲ�����
//       EMV_DAUTH_FAIL              : �����ѻ�������֤ʧ�ܵ���������
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
		return(EMV_DAUTH_FAIL); // ��̬��֤���ݼ�¼����
	}
	if(sg_IssuerPubKey.bits == 0) {
		EMV_TRACE_ERR_MSG
		return(EMV_DAUTH_FAIL); // �����й�Կδ����ȷ��������
	}

	// ȡ��������
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
		// ����T9F4A (Static Data Authentication Tag List)
		if(memcmp(psT9F4AValue, TAG_82_AIP, 1) != 0) {
			EMV_TRACE_ERR_MSG
			return(EMV_DAUTH_FAIL); // if T9F4A exist, it must only contain T82(AIP)
		}
		iRet = iTlvGetObjValue(gl_sTlvDbCard, TAG_82_AIP, &p); // Get AIP
		if(iRet != 2) {
			EMV_TRACE_ERR_MSG
			return(EMV_DAUTH_NATIVE); // AIP��Ӧ�ò�����
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

// DDA������֤
// ret : EMV_DAUTH_OK				 : OK
//       EMV_DAUTH_DATA_MISSING		 : ����ȱʧ
//       EMV_DAUTH_NATIVE			 : �ڲ�����
//       EMV_DAUTH_DATA_ILLEGAL      : ���ݷǷ�
//       EMV_DAUTH_FAIL              : �����ѻ�������֤ʧ�ܵ���������
//       EMV_DAUTH_CARD_IO			 : ��Ƭ����ʧ��
//       EMV_DAUTH_CARD_SW			 : �Ƿ���״̬��
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
	uchar  sDDOLBody[256];  // ��DDOL����������
	int    iDDOLBodyLen;	// ��DDOL���������ݳ���
	uchar  sRand[4];
	uchar  ucIccDynamicDataLen, sIccDynamicData[256];
	int    iSignedDynamicDataLen;
	uchar  sSignedDynamicData[248];
	uchar  *p;

	if(sg_IcPubKey.bits == 0) {
		EMV_TRACE_ERR_MSG
		return(EMV_DAUTH_FAIL); // IC����Կδ����ȷ��������
	}

	// ȡ��������
	iT9F49ValueLen = iTlvGetObjValue(gl_sTlvDbCard, TAG_9F49_DDOL, &psT9F49Value); // DDOL
	if(iT9F49ValueLen <= 0) {
		// DDOL from ICC not found, use default DDOL
		iT9F49ValueLen = iTlvGetObjValue(gl_sTlvDbTermVar, TAG_DFXX_DefaultDDOL, &psT9F49Value); // DDOL
		if(iT9F49ValueLen < 0) {
			EMV_TRACE_ERR_MSG
			return(EMV_DAUTH_FAIL); // ��Ƭû�ṩDDOL���ն�û�ṩȱʡDDOL
		}
	}
	// search T9F37, �����
    iRet =  iTlvSearchDOLTag(psT9F49Value, iT9F49ValueLen, TAG_9F37_TermRand, NULL); // T9F37, term rand
    if(iRet <= 0) {
		EMV_TRACE_ERR_MSG
		return(EMV_DAUTH_FAIL); // DDOL��û��Ҫ�������
	}
	vGetRand(sRand, 4);
	iRet = iTlvMakeAddObj(gl_sTlvDbTermVar, TAG_9F37_TermRand, 4, sRand, TLV_CONFLICT_REPLACE);
	if(iRet < 0) {
		EMV_TRACE_ERR_MSG
		return(EMV_DAUTH_NATIVE);
	}

    // ���DDOL
	iDDOLBodyLen = iTlvMakeDOLBlock(sDDOLBody, sizeof(sDDOLBody), psT9F49Value, iT9F49ValueLen,
          				            gl_sTlvDbTermFixed, gl_sTlvDbTermVar, gl_sTlvDbCard, NULL);
	if(iDDOLBodyLen < 0) {
		EMV_TRACE_ERR_MSG
		return(EMV_DAUTH_NATIVE);
	}

	// ִ���ڲ���ָ֤��
	iRet = uiEmvCmdInternalAuth((uchar)iDDOLBodyLen, sDDOLBody, &ucLenOut, sBuf);
	if(iRet == 1)
		return(EMV_DAUTH_CARD_IO); // ��Ƭ����ʧ��
	if(iRet)
		return(EMV_DAUTH_CARD_SW); // �Ƿ���״̬��


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
        iTlvSetBuffer(szBuf, sizeof(szBuf)); // ��szBuf����ʱTLV���ݿ�
		iRet = iTlvBatchAddObj(0/*0:add only primitive TLV object*/, szBuf, sBuf, iRet, TLV_CONFLICT_ERR, 0/*0:����Ϊ0�������*/);
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
		return(EMV_DAUTH_FAIL); // ����2CC.078.00_0, ���Ȳ���, ��ΪDDAʧ��, ��Ҫ��������
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
		return(EMV_DAUTH_FAIL); // ���IDD�Ƿ�, ��Ϊ��̬������֤ʧ��
	}
	iRet = iTlvMakeAddObj(gl_sTlvDbCard, TAG_9F4C_ICCDynNumber, sIccDynamicData[0], sIccDynamicData+1, TLV_CONFLICT_REPLACE);
	if(iRet < 0) {
		EMV_TRACE_ERR_MSG
		return(EMV_DAUTH_NATIVE);
	}
	return(EMV_DAUTH_OK);
}

// CDA������֤
// in  : ucGacFlag              : 1:��һ��GAC 2:�ڶ���GAC
//       psGacDataOut           : GACָ�������
//       ucGacDataOutLen        : GACָ������ݳ���
// ret : EMV_DAUTH_OK			: OK
//       EMV_DAUTH_DATA_MISSING	: ����ȱʧ
//       EMV_DAUTH_NATIVE		: �ڲ�����
//       EMV_DAUTH_FAIL         : �����ѻ�������֤ʧ�ܵ���������
// Note: ���ñ�������, gl_sTlvDbCard�е�CID���ܻ�ı�(�������CID������CID����, ���ý������CID�滻����CID)
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
    iRet = iTlvGetObjValue(gl_sTlvDbCard, TAG_9F27_CID, &p); // Get CID from TLV���ݿ�
	if(iRet != 1) {
		EMV_TRACE_ERR_MSG
		return(EMV_DAUTH_FAIL);
	}
	if(*p != sIccDynamicData[1+sIccDynamicData[0]]) {
		// �������CID������CID����, �ý������CID�滻����CID
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
		// �ǵڶ���GAC
		iRet = iTlvGetObjValue(gl_sTlvDbTermVar, TAG_DFXX_Cdol2DataBlock, &p);
		if(iRet <= 0) {
			EMV_TRACE_PROC
			return(EMV_DAUTH_NATIVE);
		}
		vSHA1Update(p, (ushort)iRet); // CDOL2 Data Block
	}

	// ��GAC��������д����ʱTLV���ݿ�(ȥ��T9F4B, Signed Dynamic Application Data), ����CDA��֤
	iTlvSetBuffer(sTmpTlvBuf,  sizeof(sTmpTlvBuf));    // ��ʼ����ʱTLV���ݿ�
	if(psGacDataOut[0] != 0x77) {
		EMV_TRACE_PROC
		return(EMV_DAUTH_FAIL);
	}
    iRet = iTlvBatchAddObj(1/*not only primitive*/, sTmpTlvBuf, psGacDataOut, ucGacDataOutLen, TLV_CONFLICT_ERR, 1/*1:����Ϊ0����Ҳ���*/); // ��Ƭ��������
	if(iRet < 0) {
		EMV_TRACE_PROC
		return(EMV_DAUTH_FAIL);
	}
	iTlvDelObj(sTmpTlvBuf, TAG_9F4B_SDAData); // ����ʱTLV���ݿ���ȥ��Signed Dynamic Application Data
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
		return(EMV_DAUTH_FAIL); // ���IDD�Ƿ�, ��Ϊ��̬������֤ʧ��
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

// FDDA������֤
// ret : EMV_DAUTH_OK				 : OK
//       EMV_DAUTH_DATA_MISSING		 : ����ȱʧ
//       EMV_DAUTH_NATIVE			 : �ڲ�����
//       EMV_DAUTH_DATA_ILLEGAL      : ���ݷǷ�
//       EMV_DAUTH_FAIL              : �����ѻ�������֤ʧ�ܵ���������
// Note: �ο�JR/T0025.12��2013 ��¼B, P47
//       FDDAʧ�ܺ�, Ӧ�ò���Ҫ���м��9F6C B1 b6b5, �Ծ����Ǿܾ����ǽ�������
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
		return(EMV_DAUTH_FAIL); // IC����Կδ����ȷ��������
	}

	// ��ȡǩ�ֺ�̬����
	iSignedDynamicDataLen = iTlvGetObjValue(gl_sTlvDbCard, TAG_9F4B_SDAData, &p);
	if(iSignedDynamicDataLen < 0) {
		EMV_TRACE_PROC
		return(EMV_DAUTH_FAIL);
	}
	memcpy(sSignedDynamicData, p, iSignedDynamicDataLen);

	// ��ȡ��Ƭ��֤�������, T9F69
	iCardAuthRelateDataLen = iTlvGetObjValue(gl_sTlvDbCard, TAG_9F69_CardAuthRelatedData, &p);
	if(iCardAuthRelateDataLen > 0)
		memcpy(sCardAuthRelateData, p, iCardAuthRelateDataLen);
	ucFddaVer = 0; // ����fdda�汾��Ϊ00
	if(iCardAuthRelateDataLen > 0) {
		if(sCardAuthRelateData[0] == 1)
			ucFddaVer = 1; // ����fdda�汾��Ϊ01
		else if(sCardAuthRelateData[0]) {
			EMV_TRACE_PROC
			return(EMV_DAUTH_FAIL); // ��ʶ���fdda�汾
		}
	}
	vTraceWriteTxtCr(TRACE_PROC, "FDDA-%02d", (int)ucFddaVer);

	if(!iEmvTestItemBit(EMV_ITEM_AIP, AIP_02_DDA_SUPPORTED)) {
		EMV_TRACE_PROC
		return(EMV_DAUTH_FAIL); // AIPָ����֧��DDA
	}
	if(ucFddaVer == 0x00) {
		iRet = iTlvGetObjValue(gl_sTlvDbTermVar, TAG_9F66_ContactlessSupport, &p);
		if(iRet == 4) {
			if((p[3]&0x80) == 0x80) {
				// �ն˱���֧��fDDA-01, �жϿ�Ƭ�汾
				iRet = iTlvGetObjValue(gl_sTlvDbCard, TAG_9F08_AppVerCard, &p);
				if(iRet == 2) {
//    				if(memcmp(p, "\x00\x30", 2) == 0)
					if(memcmp(p, "\x00\x30", 2) >= 0) { // PBOC����03, 3.0�����ϰ汾����֧��fdda-01
						EMV_TRACE_PROC
	    				return(EMV_DAUTH_FAIL); // 3.0�淶��, ����֧��01�汾fdda
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

	// �ն������
	iRet = iTlvGetObjValue(gl_sTlvDbTermVar, TAG_9F37_TermRand, &p);
	if(iRet != 4) {
		EMV_TRACE_PROC
		return(EMV_DAUTH_FAIL);
	}
	vSHA1Update(p, 4);
	if(ucFddaVer == 0x01) {
		// FDDA�汾01
		// ��Ȩ���
		iRet = iTlvGetObjValue(gl_sTlvDbTermVar, TAG_9F02_AmountN, &p);
		if(iRet != 6) {
			EMV_TRACE_PROC
			return(EMV_DAUTH_FAIL);
		}
		vSHA1Update(p, 6);
		// ���Ҵ���
		iRet = iTlvGetObjValue(gl_sTlvDbTermVar, TAG_5F2A_TransCurrencyCode, &p);
		if(iRet != 2) {
			EMV_TRACE_PROC
			return(EMV_DAUTH_FAIL);
		}
		vSHA1Update(p, 2);
		// ��Ƭ��֤�������, T9F69
		vSHA1Update(sCardAuthRelateData, (ushort)iCardAuthRelateDataLen);
		// Ӧ�ý��׼�����
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
