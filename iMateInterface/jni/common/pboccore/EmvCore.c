/**************************************
File name     : EmvCore.c
Function      : EMV/pboc2.0����Ǻ��Ľӿ�
Author        : Yu Jun
First edition : Mar 31st, 2012
Modified      : Sep 5th, 2013
				����iHxSetAmount()����, ����ʵ��GAC��GPO��ͬ
                Apr 2nd, 2014
					iHxSetAmount()���������ڶ���¼��, ��������ǰ����
				Apr 16th, 2014
				    iHxGetCardNativeData()��iHxGetData()����������δ֪Tagʱ��b�ʹ���
				Apr 21st, 2014
				    ����iHxCoreInit()�ӿ�, ����һ������iAppType
					���ڱ������ͼ������ʵ��Ӧ�ó���
                Apr 21st, 2014
					���ӱ���gl_iAllowAllTransFlag���ڱ����Ƿ�֧�����н���
				Apr 22nd, 2014
					iHxGetCardNativeData()��iHxGetData()����Bin��������ʱ��β��������һ��'\0'
					���뺯��˵�������Ҳ���Ҫ, ����Bin��������ʱ������β��'\0'
				May 19th, 2014
				    iHxReadLog()��iHxReadLoadLog()�ڴ���Ƿ���¼��ʱ����HXEMV_NO_LOG��Ϊ����HXEMV_NO_RECORD
**************************************/
/*
ģ����ϸ����:
	���Ľӿ�
.	EMV��������
.	���ù淶ʵ�ֲ�ʵ��EMV���Ľӿ�
	���ڻص���ʽ, ���ûص��ӿ���ʾ��Ϣ
	���ڷǻص��ӿ�, ������ʾ��Ϣ�����Ӧ�ò�
.	EMV������ȡ
*/
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "VposFace.h"
#include "Arith.h"
#include "Common.h"
#include "EmvCmd.h"
#include "TagAttr.h"
#include "TlvFunc.h"
#include "Iso4217.h"
#include "EmvTrace.h"
#include "EmvData.h"
#include "EmvCore.h"
#include "EmvFunc.h"
#include "EmvCvm.h"
#include "EmvCvm2.h"
#include "EmvDAuth.h"
#include "EmvMsg.h"
#include "EmvPara.h"
#include "EmvSele.h"
#include "EmvIo.h"
#include "EmvModul.h"
#include "PbocCtls.h"
#include "TagDef.h"

static int   sg_iAidGetFlag;		 // AID�б��ȡ��־, 1:AID�б��Ѿ���ȡ 0:AID�б��б���δ��ȡ
static uchar sg_szSelectedLang[3];   // �ֿ���ѡ�������, ��ֹ���Ҫ��ֿ���ѡ������
static int   sg_iCardSlot = 100;	 // �Ӵ�������
static int   sg_iCtlsCardSlot = 100; // �ǽӿ�����
static int   sg_iCurrSlot = 0;		 // ��ǰ�������, ���һ�μ�⵽��Ƭ���ڵĿ�����

// ��ȡ������Ϣ
// out : pszCoreName    : ��������, ������40�ֽ�
//       pszCoreDesc    : ��������, ������60�ֽ��ַ���
//       pszCoreVer     : ���İ汾��, ������10�ֽ��ַ���, ��"1.00"
//       pszReleaseDate : ���ķ�������, YYYY/MM/DD
// ret : HXEMV_OK       : OK
int iHxCoreInfo(uchar *pszCoreName, uchar *pszCoreDesc, uchar *pszCoreVer, uchar *pszReleaseDate)
{
	strcpy(pszCoreName, "HXY5_EMV_PBOC30DC_ECASH_QPBOC"); // ��������
    strcpy(pszCoreDesc, "EMV & PBOC3.0 D/C & eCash & qPboc"); // ��������
    strcpy(pszCoreVer, "3.20"); // �汾
    strcpy(pszReleaseDate, "2014/08/14"); // ��������
    return(HXEMV_OK);
}

// trace tlvdb
static void vTraceTlvDb(void)
{
	uchar *psTlvDb;
	int   i, j, iLen;
	uchar *psTlvObj;
	uchar szBuf[600];

	vTraceWriteTxtCr(TRACE_TLV_DB, "==Tlv Database==");
	for(i=0; i<4; i++) {
		switch(i) {
		case 0:
			psTlvDb = gl_sTlvDbTermFixed;
			vTraceWriteTxtCr(TRACE_TLV_DB, "TlvDbTermFixed:");
			break;
		case 1:
			psTlvDb = gl_sTlvDbTermVar;
			vTraceWriteTxtCr(TRACE_TLV_DB, "TlvDbTermVar:");
			break;
		case 2:
			psTlvDb = gl_sTlvDbCard;
			vTraceWriteTxtCr(TRACE_TLV_DB, "TlvDbCard:");
			break;
		case 3:
			psTlvDb = gl_sTlvDbIssuer;
			vTraceWriteTxtCr(TRACE_TLV_DB, "TlvDbIssuer:");
			break;
		}
		for(j=0; ; j++) {
			iLen = iTlvGetObjByIndex(psTlvDb, j, &psTlvObj);
			if(iLen == TLV_ERR_NOT_FOUND)
				break;
			if(iLen < 0)
				break;
			vOneTwo0(psTlvObj, iLen, szBuf);
			vTraceWriteTxtCr(TRACE_TLV_DB, "..%s // %s", szBuf, psTagAttrGetDesc(psTlvObj));
		} // for(j=0; ; j++
	} // for(i=0; i<4; i++
}

// ��ʼ������
// in  : iCallBackFlag      : ָ�����ĵ��÷�ʽ
//                            HXCORE_CALLBACK     : ָ��ʹ�ûص��ӿ�
//                            HXCORE_NOT_CALLBACK : ָ��ʹ�÷ǻص��ӿ�
//		 iAppType           : Ӧ�ó����־, 0:�ͼ����(�ϸ����ع淶) 1:ʵ��Ӧ�ó���
//							      ����: 1. �ͼ����ֻ֧��ͨ��L2���Ľ���, Ӧ�ó���֧�����н���
//       ulRandSeed         : ��������ӣ����������ڲ������������
//       pszDefaultLanguage : ȱʡ����, ��ǰ֧��zh:���� en:Ӣ��
// ret : HXEMV_OK           : OK
//       HXEMV_PARA         : ��������
//       HXEMV_CORE         : �ڲ�����
// Note: �ڵ����κ������ӿ�ǰ�������ȳ�ʼ������
int iHxCoreInit(int iCallBackFlag, int iAppType, ulong ulRandSeed, uchar *pszDefaultLanguage)
{
	int iRet;

	if(iCallBackFlag!=HXCORE_CALLBACK && iCallBackFlag!=HXCORE_NOT_CALLBACK)
		return(HXEMV_PARA);
	gl_iCoreCallFlag = iCallBackFlag;

	iRet = iEmvMsgTableInit(pszDefaultLanguage); // ��ʼ����������Ϣ�ṹ, ���Է���ֵ, ������Բ�֧��, ��ʹ��ȱʡ����

	// ��ʼ��Tlv���ݿ�
	iTlvSetBuffer(gl_sTlvDbTermFixed, sizeof(gl_sTlvDbTermFixed));
	iTlvSetBuffer(gl_sTlvDbTermVar, sizeof(gl_sTlvDbTermVar));
	iTlvSetBuffer(gl_sTlvDbIssuer, sizeof(gl_sTlvDbIssuer));
	iTlvSetBuffer(gl_sTlvDbCard, sizeof(gl_sTlvDbCard));

	gl_iTermSupportedAidNum = 0; // �ն˵�ǰ֧�ֵ�Aid�б���Ŀ
	gl_iTermSupportedCaPublicKeyNum = 0; // �ն˵�ǰ֧�ֵ�CA��Կ��Ŀ
	gl_iCardAppNum = 0; // �ն��뿨Ƭͬʱ֧�ֵ�Ӧ����Ŀ

	gl_iAppType = iAppType;   // Ӧ������

    vRandShuffle(ulRandSeed); // ���������������
	vRandShuffle((ulong)_ucGetRand() * (ulong)_ucGetRand()); // ������Ŷ�

	iTraceSet(TRACE_OFF, 0, 0, 0, NULL);

	gl_iCoreStatus = EMV_STATUS_INIT; // ���ĳ�ʼ�����
	return(HXEMV_OK);
}

// �����ն˲���
// in  : pTermParam        : �ն˲���
// out : pszErrTag         : ������ó���, �����Tagֵ
// ret : HXEMV_OK          : OK
//       HXEMV_PARA        : ��������
//       HXEMV_CORE        : �ڲ�����
//       HXEMV_FLOW_ERROR  : EMV���̴���
int iHxSetTermParam(stTermParam *pTermParam, uchar *pszErrTag)
{
	uchar szDateTime[15];
	int iRet;

	_vGetTime(szDateTime);
	vTraceWriteTxtCr(TRACE_ALWAYS, "===...===...=== %4.4s/%2.2s/%2.2s %2.2s:%2.2s:%2.2s ===...===...===",
		             szDateTime, szDateTime+4, szDateTime+6, szDateTime+8, szDateTime+10, szDateTime+12); // ���ز����Ǻ��ĳ�ʼ������õĵ�һ������, ������һ��

	vTraceWriteTxtCr(TRACE_ALWAYS, "iHxSetTermParam()");
	vTraceWriteTxtCr(TRACE_TAG_DESC, "ucTermType=%02X", (int)pTermParam->ucTermType);
	vTraceWriteBinCr(TRACE_TAG_DESC, "sTermCapability=", pTermParam->sTermCapability, 3);
	vTraceWriteBinCr(TRACE_TAG_DESC, "sAdditionalTermCapability=", pTermParam->sAdditionalTermCapability, 5);
	vTraceWriteTxtCr(TRACE_TAG_DESC, "ucReaderCapability=%02X", (int)pTermParam->ucReaderCapability);
	vTraceWriteTxtCr(TRACE_TAG_DESC, "ucVoiceReferralSupport=%02X", (int)pTermParam->ucVoiceReferralSupport);
	vTraceWriteTxtCr(TRACE_TAG_DESC, "ucPinBypassBehavior=%02X", (int)pTermParam->ucPinBypassBehavior);
	vTraceWriteTxtCr(TRACE_TAG_DESC, "szMerchantId=%s", pTermParam->szMerchantId);
	vTraceWriteTxtCr(TRACE_TAG_DESC, "szTermId=%s", pTermParam->szTermId);
	vTraceWriteTxtCr(TRACE_TAG_DESC, "szIFDSerialNo=%s", pTermParam->szIFDSerialNo);
	vTraceWriteTxtCr(TRACE_TAG_DESC, "szMerchantNameLocation=%s", pTermParam->szMerchantNameLocation);
	vTraceWriteTxtCr(TRACE_TAG_DESC, "iMerchantCategoryCode=%d", pTermParam->iMerchantCategoryCode);
	vTraceWriteTxtCr(TRACE_TAG_DESC, "iTermCountryCode=%d", pTermParam->iTermCountryCode);
	vTraceWriteTxtCr(TRACE_TAG_DESC, "szAcquirerId=%s", pTermParam->szAcquirerId);
	vTraceWriteTxtCr(TRACE_TAG_DESC, "ucTransLogSupport=%02X", (int)pTermParam->ucTransLogSupport);
	vTraceWriteTxtCr(TRACE_TAG_DESC, "ucBlackListSupport=%02X", (int)pTermParam->ucBlackListSupport);
	vTraceWriteTxtCr(TRACE_TAG_DESC, "ucSeparateSaleSupport=%02X", (int)pTermParam->ucSeparateSaleSupport);
	vTraceWriteTxtCr(TRACE_TAG_DESC, "ucAppConfirmSupport=%02X", (int)pTermParam->ucAppConfirmSupport);
	vTraceWriteTxtCr(TRACE_TAG_DESC, "szLocalLanguage=%s", pTermParam->szLocalLanguage);
	vTraceWriteTxtCr(TRACE_TAG_DESC, "szPinpadLanguage=%s", pTermParam->szPinpadLanguage);
	vTraceWriteBinCr(TRACE_TAG_DESC, "sTermCtlsCapability=", pTermParam->sTermCtlsCapability, 4);
	vTraceWriteTxtCr(TRACE_TAG_DESC, "szTermCtlsAmountLimit=%s", pTermParam->szTermCtlsAmountLimit);
	vTraceWriteTxtCr(TRACE_TAG_DESC, "szTermCtlsOfflineAmountLimit=%s", pTermParam->szTermCtlsOfflineAmountLimit);
	vTraceWriteTxtCr(TRACE_TAG_DESC, "szTermCtlsCvmLimit=%s", pTermParam->szTermCtlsCvmLimit);

	if(pTermParam->AidCommonPara.ucTermAppVerExistFlag)
		vTraceWriteBinCr(TRACE_TAG_DESC, "sTermAppVer=", pTermParam->AidCommonPara.sTermAppVer, 2);
	if(pTermParam->AidCommonPara.iDefaultDDOLLen >= 0)
		vTraceWriteBinCr(TRACE_TAG_DESC, "sDefaultDDOL=", pTermParam->AidCommonPara.sDefaultDDOL, pTermParam->AidCommonPara.iDefaultDDOLLen);
	if(pTermParam->AidCommonPara.iDefaultTDOLLen >= 0)
		vTraceWriteBinCr(TRACE_TAG_DESC, "sDefaultTDOL=", pTermParam->AidCommonPara.sDefaultTDOL, pTermParam->AidCommonPara.iDefaultTDOLLen);
	if(pTermParam->AidCommonPara.iMaxTargetPercentage >= 0)
		vTraceWriteTxtCr(TRACE_TAG_DESC, "iMaxTargetPercentage=%d", pTermParam->AidCommonPara.iMaxTargetPercentage);
	if(pTermParam->AidCommonPara.iTargetPercentage >= 0)
		vTraceWriteTxtCr(TRACE_TAG_DESC, "iTargetPercentage=%d", pTermParam->AidCommonPara.iTargetPercentage);
	if(pTermParam->AidCommonPara.ucFloorLimitExistFlag)
		vTraceWriteTxtCr(TRACE_TAG_DESC, "ulFloorLimit=%lu", pTermParam->AidCommonPara.ulFloorLimit);
	if(pTermParam->AidCommonPara.ucThresholdExistFlag)
		vTraceWriteTxtCr(TRACE_TAG_DESC, "ulThresholdValue=%lu", pTermParam->AidCommonPara.ulThresholdValue);
	if(pTermParam->AidCommonPara.ucTacDefaultExistFlag)
		vTraceWriteBinCr(TRACE_TAG_DESC, "sTacDefault=", pTermParam->AidCommonPara.sTacDefault, 5);
	if(pTermParam->AidCommonPara.ucTacDenialExistFlag)
		vTraceWriteBinCr(TRACE_TAG_DESC, "sTacDenial=", pTermParam->AidCommonPara.sTacDenial, 5);
	if(pTermParam->AidCommonPara.ucTacOnlineExistFlag)
		vTraceWriteBinCr(TRACE_TAG_DESC, "sTacOnline=", pTermParam->AidCommonPara.sTacOnline, 5);
	if(pTermParam->AidCommonPara.cForcedOnlineSupport >= 0)
		vTraceWriteTxtCr(TRACE_TAG_DESC, "cForcedOnlineSupport=%02X", (int)pTermParam->AidCommonPara.cForcedOnlineSupport);
	if(pTermParam->AidCommonPara.cForcedAcceptanceSupport >= 0)
		vTraceWriteTxtCr(TRACE_TAG_DESC, "cForcedAcceptanceSupport=%02X", (int)pTermParam->AidCommonPara.cForcedAcceptanceSupport);
	if(pTermParam->AidCommonPara.cOnlinePinSupport >= 0)
		vTraceWriteTxtCr(TRACE_TAG_DESC, "cOnlinePinSupport=%02X", (int)pTermParam->AidCommonPara.cOnlinePinSupport);
	if(pTermParam->AidCommonPara.cECashSupport >= 0)
		vTraceWriteTxtCr(TRACE_TAG_DESC, "cECashSupport=%02X", (int)pTermParam->AidCommonPara.cECashSupport);
	if(pTermParam->AidCommonPara.szTermECashTransLimit[0])
		vTraceWriteTxtCr(TRACE_TAG_DESC, "szTermECashTransLimit=%s", pTermParam->AidCommonPara.szTermECashTransLimit);

	if(gl_iCoreStatus != EMV_STATUS_INIT)
		return(HXEMV_FLOW_ERROR); // ֻ���ں��ĳ�ʼ��������һ��

	iRet = iEmvSetTermParam(pTermParam, pszErrTag);
	if(iRet == 0)
	    gl_iCoreStatus = EMV_STATUS_SET_PARA; // �Ѿ�������EMV����
	if(iRet)
		vTraceWriteTxtCr(TRACE_PROC, "(%d)iHxSetTermParam(%s)", iRet, pszErrTag);
	return(iRet);
}

// װ���ն�֧�ֵ�Aid
// in  : pTermAid          : �ն�֧�ֵ�Aid
//       iFlag             : ����, HXEMV_PARA_INIT:��ʼ�� HXEMV_PARA_ADD:���� HXEMV_PARA_DEL:ɾ��
// out : pszErrTag         : ������ó���, �����Tagֵ
// ret : HXEMV_OK          : OK
//       HXEMV_LACK_MEMORY : �洢�ռ䲻��
//       HXEMV_PARA        : ��������
//       HXEMV_FLOW_ERROR  : EMV���̴���
// Note: ɾ��ʱֻ����AID����
int iHxLoadTermAid(stTermAid *pTermAid, int iFlag, uchar *pszErrTag)
{
	int iRet;

	iRet = iEmvLoadTermAid(pTermAid, iFlag, pszErrTag);
	vTraceWriteTxtCr(TRACE_PROC, "(%d)iHxLoadTermAid(%d)", iRet, iFlag);
	if(iFlag != HXEMV_PARA_INIT) {
		vTraceWriteBinCr(TRACE_TAG_DESC, "sAid=", pTermAid->sAid, pTermAid->ucAidLen);
		vTraceWriteTxtCr(TRACE_TAG_DESC, "ucASI=%02X", (int)pTermAid->ucASI);
		// �ݲ�������������
	}
	return(iRet);
}

// װ��CA��Կ
// in  : pCAPublicKey      : CA��Կ
//       iFlag             : ����, HXEMV_PARA_INIT:��ʼ�� HXEMV_PARA_ADD:���� HXEMV_PARA_DEL:ɾ��
// ret : HXEMV_OK          : OK
//       HXEMV_LACK_MEMORY : �洢�ռ䲻��
//       HXEMV_PARA        : ��������
//       HXEMV_FLOW_ERROR  : EMV���̴���
// Note: ɾ��ʱֻ����RID��index����
int iHxLoadCAPublicKey(stCAPublicKey *pCAPublicKey, int iFlag)
{
	int iRet;
	iRet = iEmvLoadCAPublicKey(pCAPublicKey, iFlag);
	vTraceWriteTxtCr(TRACE_PROC, "(%d)iHxLoadCAPublicKey(%d)", iRet, iFlag);
	if(iFlag != HXEMV_PARA_INIT) {
		uchar szBuf[50];
		vTraceWriteBinCr(TRACE_TAG_DESC, "sRid=", pCAPublicKey->sRid, 5);
		vTraceWriteTxtCr(TRACE_TAG_DESC, "ucIndex=%02X", (int)pCAPublicKey->ucIndex);
		vOneTwo0(pCAPublicKey->sKey, 5, szBuf);
		strcat(szBuf, "...");
		vOneTwo0(pCAPublicKey->sKey+pCAPublicKey->ucKeyLen-3, 3, szBuf+13);
		vTraceWriteTxtCr(TRACE_TAG_DESC, "sKey(%d)=%s", (int)pCAPublicKey->ucKeyLen, szBuf);
		vTraceWriteTxtCr(TRACE_TAG_DESC, "lE=%ld", pCAPublicKey->lE);
		vTraceWriteTxtCr(TRACE_TAG_DESC, "szExpireDate=%s", pCAPublicKey->szExpireDate);
	}
	return(iRet);
}

// ����IC��������
// in  : iSlotNo : �����ţ�VPOS�淶
// ret : HXEMV_OK       : OK
//       HXEMV_NO_SLOT  : ��֧�ִ˿���
int iHxSetIccSlot(int iSlotNo)
{
	uint uiRet, iRet;
	uiRet = uiEmvCmdSetSlot(iSlotNo);
	if(uiRet)
		iRet = HXEMV_NO_SLOT;
	else {
		iRet = HXEMV_OK;
		sg_iCardSlot = iSlotNo;
	}
	vTraceWriteTxtCr(TRACE_PROC, "(%d)iHxSetIccSlot(%d)", iRet, iSlotNo);
	return(iRet);
}

// ���÷ǽ�IC��������
// in  : iSlotNo : �����ţ�VPOS�淶
// ret : HXEMV_OK       : OK
//       HXEMV_NO_SLOT  : ��֧�ִ˿���
int iHxSetCtlsIccSlot(int iSlotNo)
{
	uint uiRet, iRet;
	uiRet = uiEmvCmdSetSlot(iSlotNo);
	if(uiRet)
		iRet = HXEMV_NO_SLOT;
	else {
		iRet = HXEMV_OK;
		sg_iCtlsCardSlot = iSlotNo;
	}
	vTraceWriteTxtCr(TRACE_PROC, "(%d)iHxSetCtlsIccSlot(%d)", iRet, iSlotNo);
	return(iRet);
}

// ���׳�ʼ��, ÿ���½��׿�ʼ����һ��
// ret : HXEMV_OK          : OK
//       HXEMV_FLOW_ERROR  : EMV���̴���
int iHxTransInit(void)
{
	uchar sBuf[5];
	uchar szDateTime[15];
	ulong ulRandShuffle;

	vTraceWriteTxtCr(TRACE_PROC, "iHxTransInit()");
	_vGetTime(szDateTime);

	vTraceWriteTxtCr(TRACE_ALWAYS, "---...---...--- EMV���׿�ʼ(%4.2s/%2.2s/%2.2s %2.2s:%2.2s:%2.2s) ---...---...---",
		             szDateTime, szDateTime+4, szDateTime+6, szDateTime+8, szDateTime+10, szDateTime+12);

	// ��ʼ��Tlv���ݿ�
	iTlvSetBuffer(gl_sTlvDbTermVar, sizeof(gl_sTlvDbTermVar));
	iTlvSetBuffer(gl_sTlvDbIssuer, sizeof(gl_sTlvDbIssuer));
	iTlvSetBuffer(gl_sTlvDbCard, sizeof(gl_sTlvDbCard));

	// ��ʼ��TSI��TVR
	memset(sBuf, 0, 5);
	iTlvMakeAddObj(gl_sTlvDbTermVar, TAG_9B_TSI, 2, sBuf, TLV_CONFLICT_REPLACE); // TSI
	iTlvMakeAddObj(gl_sTlvDbTermVar, TAG_95_TVR, 5, sBuf, TLV_CONFLICT_REPLACE); // TVR

	sg_iAidGetFlag = 0; // 0��ʾӦ���б���δ��ȡ

	sg_szSelectedLang[0] = 0;  // �ֿ���ѡ������Գ�ʼ��

	iEmvGpoInit(); // GPO���ݳ�ʼ��
	iEmvIOInit();

	// �ѻ�������֤ģ��
	iEmvDAuthInit();
	vRandShuffle((ulong)_ucGetRand() * (ulong)_ucGetRand()); // ������Ŷ�
	_vSetTimer(&ulRandShuffle, 0);
	vRandShuffle(ulRandShuffle); // ǿ��������Ŷ�, ���õ�ǰtickcount

	// �ǻص���ʽ�ֿ�����֤ģ��
	vCvm2Init();

	gl_iCoreStatus = EMV_STATUS_TRANS_INIT;

	return(HXEMV_OK);
}

// �ǽӴ�������Ԥ����
// in  : pszAmount					: ���׽��, ascii�봮, ��ҪС����, Ĭ��ΪȱʡС����λ��, ����:RMB1.00��ʾΪ"100"
//       uiCurrencyCode				: ���Ҵ���
// ret : HXEMV_OK					: OK
//		 HXEMV_TRY_OTHER_INTERFACE	: ����ܾ�������������ֹ, ��������ͨ�Ž���
//									  Ӧ��ʾEMVMSG_TERMINATE��Ϣ, ����ն�֧������ͨ�Ž���, ��ʾ�û���������ͨ�Ž���
//       HXEMV_CORE					: �ڲ�����
// Note: ÿ�ʷǽӴ�����ǰ��������Ԥ����, ֻ�гɹ������Ԥ����ſ��Խ��зǽӽ���
//       Ӧ�ò�Ҫȷ������������Ԥ��������һ��
//       �ο�: JR/T0025.12��2013, 6.2, p9
int iHxCtlsPreProc(uchar *pszAmount, uint uiCurrencyCode)
{
	int iRet;
	iRet = iPbocCtlsPreProc(pszAmount, uiCurrencyCode);
	vTraceWriteTxtCr(TRACE_PROC, "(%d)iHxCtlsPreProc(%s, %u)", iRet, pszAmount, uiCurrencyCode);
	return(iRet);
}

// ��⿨Ƭ
// ret : HXEMV_OK          : OK
//       HXEMV_NO_CARD     : ��Ƭ������
int iHxTestCard(void)
{
	uint uiRet;
	uiEmvCmdSetSlot(sg_iCardSlot);
	uiRet = uiEmvTestCard();
	if(uiRet == 0)
		return(HXEMV_NO_CARD);
	sg_iCurrSlot = sg_iCardSlot; // ��⵽�Ӵ���, ����ǰ������趨Ϊ�Ӵ�����
	gl_iCardType = EMV_CONTACT_CARD; // ���ÿ�����Ϊ�Ӵ���
	return(HXEMV_OK);
}

// ���ǽӿ�Ƭ
// ret : HXEMV_OK          : OK
//       HXEMV_NO_CARD     : ��Ƭ������
int iHxTestCtlsCard(void)
{
	uint uiRet;
	uiEmvCmdSetSlot(sg_iCtlsCardSlot);
	uiRet = uiEmvTestCard();
	if(uiRet == 0)
		return(HXEMV_NO_CARD);
	sg_iCurrSlot = sg_iCtlsCardSlot; // ��⵽�ǽӿ�, ����ǰ������趨Ϊ�ǽӿ���
	gl_iCardType = EMV_CONTACTLESS_CARD; // ���ÿ�����Ϊ�ǽӿ�
	return(HXEMV_OK);
}

// �رտ�Ƭ
// ret : HXEMV_OK          : OK
int iHxCloseCard(void)
{
	vTraceWriteTxtCr(TRACE_PROC, "iHxCloseCard()");
	uiEmvCloseCard();
	return(HXEMV_OK);
}

// ǿ�������趨,�趨TVRǿ������λ
// in  : iFlag             : �趨TVRǿ������λ��־ 0:���趨 1:�趨
// ret : HXEMV_OK          : OK
//       HXEMV_FLOW_ERROR  : EMV���̴���
// Note: ������ȷ����������Ҫ��TAC/IAC-Onlineǿ������λ������
//       �淶�涨:����������ֵ��,���ڽ��׿�ʼ��ʱ�����ǿ����������(emv2008 book4 6.5.3, P56)
//       �����Ĳ�֧�ֲ���Ա�����ǿ������, �ο�������ƾ��, ����ǿ������λ��ǿ���趨,
//       Ϊ�˽������ܱ�Ҫ���趨ǿ������λ, ���ṩ�˺���, ������������Ƿ�������ֵ��
int iHxSetForceOnlineFlag(int iFlag)
{
	int   iRet;
	uchar *p;

	vTraceWriteTxtCr(TRACE_PROC, "iHxSetForceOnlineFlag(%d)", iFlag);
	iRet = iTlvGetObjValue(gl_sTlvDbTermVar, TAG_DFXX_ForceOnlineSupport, &p);
	if(iRet != 1)
		return(HXEMV_OK);
	if(*p == 0)
		return(HXEMV_OK); // �ն˲�֧��
	// ����TVRǿ��������ʶλ
	iEmvSetItemBit(EMV_ITEM_TVR, TVR_28_FORCED_ONLINE, iFlag);
	return(HXEMV_OK);
}

// ��ȡ��Ƭ�ڲ�����
// in  : psTag             : ���ݱ�ǩ, ��:"\x9F\x79":�����ֽ����
//       piOutTlvDataLen   : psOutTlvData��������С
//       piOutDataLen      : psOutData��������С
// out : piOutTlvDataLen   : ���������ݳ���, ԭʼ��ʽ, ����Tag��Length��Value
//       psOutTlvData      : ����������, ����Tag��Length��Value
//       piOutDataLen      : ���������ݳ���, ������ʽ
//       psOutData         : ����������(N������:ת���ɿɶ����ִ�, CNѹ��������:ȥ��β��'F'������ִ�, A|AN|ANS|B|δ֪����:ԭ������),��B���ⷵ��ֵ�����ǿ�Ƽ�һ����β'\0', �ý������������ڷ��صĳ���֮��
//                           ע��, ����N3����������, ���ص����ݳ���ΪN4, ע����ջ���������Ҫ����
// ret : HXEMV_OK          : ��ȡ�ɹ�
//       HXEMV_NO_DATA     : �޴�����
//       HXEMV_LACK_MEMORY : ����������
//       HXEMV_CARD_REMOVED: ����ȡ��
//       HXEMV_CARD_OP     : ��������
//       HXEMV_CARD_SW     : �Ƿ���״̬��
//       HXEMV_FLOW_ERROR  : EMV���̴���
//       HXEMV_CORE        : �ڲ�����
// note: piOutTlvDataLen��psOutTlvData��һ������NULL���򲻻᷵�����������
//       piOutDataLen��psOutData��һ������NULL, �����᷵�����������
//       ����һ��������, ��ŵ�Tlv���ݿ���, �ٴ�ʹ�ÿ�����iHxGetData()��ȡ
//       ��������ʾ������Ϣ
int iHxGetCardNativeData(uchar *psTag, int *piOutTlvDataLen, uchar *psOutTlvData, int *piOutDataLen, uchar *psOutData)
{
	int   iRet;
	uint  uiRet;
	int   iTagLen;
	uchar ucTlvObjLen, sTlvObj[256];
	uchar szBuf[512];
    int   iValueLen;
    int   iValueType;
	uchar *psValue;

	iTagLen = iTlvTagLen(psTag);
	if(iTagLen == 0)
		iTagLen = 2; // ���Tag�Ƿ�, ��Ϊ��2�ֽ�Tag
	{
		int iOutTlvDataLen, iOutDataLen;
		if(piOutTlvDataLen && psOutTlvData)
			iOutTlvDataLen = *piOutTlvDataLen;
		else
			iOutTlvDataLen = -1;
		if(piOutDataLen && psOutData)
			iOutDataLen = *piOutDataLen;
		else
			iOutDataLen = -1;
		vOneTwo0(psTag, iTagLen, szBuf);
		vTraceWriteTxtCr(TRACE_PROC, "iHxGetCardNativeData(%s,%d,%d)", szBuf, iOutTlvDataLen, iOutDataLen);
	}

	if(gl_iCoreStatus < EMV_STATUS_APP_SELECT) {
		EMV_TRACE_ERR_MSG
		return(HXEMV_FLOW_ERROR); // Ӧ��ѡ��֮����Զ���Ƭ�ڲ�����
	}

	iTagLen = iTlvTagLen(psTag);
	if(iTagLen <= 0)
		return(HXEMV_NO_DATA); // ���Tag�Ƿ�, ��Ϊ�޴�����
	uiRet = uiEmvCmdGetData(psTag, &ucTlvObjLen, sTlvObj);
	if(uiRet) {
		EMV_TRACE_ERR_MSG_RET(uiRet)
	}
	if(uiRet == 1) {
		if(uiEmvTestCard() == 0) {
			return(HXEMV_CARD_REMOVED); // ����ȡ��
		} else {
			return(HXEMV_CARD_OP); // ��Ƭ��������
		}
	}
	if(uiRet==0x6A81 || uiRet==0x6A88)
		return(HXEMV_NO_DATA);
	if(uiRet)
		return(HXEMV_CARD_SW); // �Ƿ���״̬��
	if(ucTlvObjLen < 2)
		return(HXEMV_NO_DATA); // ��ʽ��,��Ϊ������
	if(memcmp(sTlvObj, psTag, 2) != 0)
		return(HXEMV_NO_DATA); // ����������Tag����, ��Ϊ������

	// �ɹ�����������
	iRet = iTlvCheckTlvObject(sTlvObj);
	if(iRet<0 || iRet!=(int)ucTlvObjLen) {
		EMV_TRACE_ERR_MSG_RET(iRet)
		return(HXEMV_NO_DATA); // �����ʽ��, ��Ϊ�޴�����
	}
	if(iRet > (int)ucTlvObjLen) {
		EMV_TRACE_ERR_MSG_RET(iRet)
		return(HXEMV_NO_DATA); // �����ʽ��, ��Ϊ�޴�����
	}

	// ���浽gl_sTlvDbCard��
	iRet = iTlvAddObj(gl_sTlvDbCard, sTlvObj, TLV_CONFLICT_REPLACE);
	if(iRet < 0) {
		EMV_TRACE_ERR_MSG_RET(iRet)
		return(HXEMV_CORE);
	}

	if(piOutTlvDataLen && psOutTlvData) {
		if(*piOutTlvDataLen < (int)ucTlvObjLen) {
			EMV_TRACE_ERR_MSG
			return(HXEMV_LACK_MEMORY);
		}
		*piOutTlvDataLen = (int)ucTlvObjLen;
		memcpy(psOutTlvData, sTlvObj, *piOutTlvDataLen);
		vTraceWriteBinCr(TRACE_ALWAYS, "Tlv -->", psOutTlvData, *piOutTlvDataLen);
	}

	if(piOutDataLen==NULL || psOutData==NULL)
		return(HXEMV_OK);
    // ����
	iValueLen = iTlvValue(sTlvObj, &psValue);
	iValueType = (int)uiTagAttrGetType(sTlvObj);
    switch(iValueType) {
    case TAG_ATTR_N:
		if(*piOutDataLen <= iValueLen*2)
			return(HXEMV_LACK_MEMORY);
        vOneTwo0(psValue, iValueLen, psOutData);
        *piOutDataLen = iValueLen*2;
        break;
    case TAG_ATTR_CN:
        vOneTwo0(psValue, iValueLen, szBuf);
		vTrimTailF(szBuf);
		if(*piOutDataLen <= (int)strlen(szBuf))
			return(HXEMV_LACK_MEMORY);
		strcpy(psOutData, szBuf);
        *piOutDataLen = strlen((char *)psOutData);
        break;
    case TAG_ATTR_A:
    case TAG_ATTR_AN:
    case TAG_ATTR_ANS:
		if(*piOutDataLen <= iValueLen)
			return(HXEMV_LACK_MEMORY);
        memcpy(psOutData, psValue, iValueLen);
        psOutData[iValueLen] = 0;
        *piOutDataLen = strlen((char *)psOutData);
        break;
    case TAG_ATTR_B:
    default: // δ֪Tag��b�ʹ���
		if(*piOutDataLen < iValueLen)
			return(HXEMV_LACK_MEMORY);
        memcpy(psOutData, psValue, iValueLen);
        *piOutDataLen = iValueLen;
        break;
    }
	if(iValueType==TAG_ATTR_N || iValueType==TAG_ATTR_CN || iValueType==TAG_ATTR_A || iValueType==TAG_ATTR_AN || iValueType==TAG_ATTR_ANS) {
		vTraceWriteTxtCr(TRACE_ALWAYS, "%s", psOutData);
	} else {
		vTraceWriteBinCr(TRACE_ALWAYS, "NTlv -->", psOutData, *piOutDataLen);
	}

	return(HXEMV_OK);
}

// ��ȡӦ������
// in  : psTag             : ���ݱ�ǩ, ��:"\x82":Aip
//       piOutTlvDataLen   : psOutTlvData��������С
//       piOutDataLen      : psOutData��������С
// out : piOutTlvDataLen   : ���������ݳ���, ԭʼ��ʽ, ����Tag��Length��Value
//       psOutTlvData      : ����������, ����Tag��Length��Value
//       piOutDataLen      : ���������ݳ���, ������ʽ
//       psOutData         : ����������(N������:ת���ɿɶ����ִ�, CNѹ��������:ȥ��β��'F'������ִ�, A|AN|ANS|B|δ֪����:ԭ������),��B���ⷵ��ֵ�����ǿ�Ƽ�һ����β'\0', �ý������������ڷ��صĳ���֮��
//                           ע��, ����N3����������, ���ص����ݳ���ΪN4, ע����ջ���������Ҫ����
// ret : HXEMV_OK          : ��ȡ�ɹ�
//       HXEMV_NO_DATA     : �޴�����
//       HXEMV_LACK_MEMORY : ����������
//       HXEMV_FLOW_ERROR  : EMV���̴���
// note: piOutTlvDataLen��psOutTlvData��һ������NULL���򲻻᷵�����������
//       piOutDataLen��psOutData��һ������NULL, �����᷵�����������
int iHxGetData(uchar *psTag, int *piOutTlvDataLen, uchar *psOutTlvData, int *piOutDataLen, uchar *psOutData)
{
	int   i;
	int   iRet;
	uchar *psTlvDb;
	int   iTagLen;
	uchar ucTlvObjLen, sTlvObj[256];
	uchar szBuf[512];
	uchar *psTlvObj;
    int   iValueLen;
    int   iValueType;
	uchar *psValue;

	iTagLen = iTlvTagLen(psTag);
	if(iTagLen == 0)
		iTagLen = 2; // ���Tag�Ƿ�, ��Ϊ��2�ֽ�Tag
	{
		int iOutTlvDataLen, iOutDataLen;
		if(piOutTlvDataLen && psOutTlvData)
			iOutTlvDataLen = *piOutTlvDataLen;
		else
			iOutTlvDataLen = -1;
		if(piOutDataLen && psOutData)
			iOutDataLen = *piOutDataLen;
		else
			iOutDataLen = -1;
		vOneTwo0(psTag, iTagLen, szBuf);
		vTraceWriteTxtCr(TRACE_PROC, "iHxGetData(%s,%d,%d)", szBuf, iOutTlvDataLen, iOutDataLen);
	}

	iTagLen = iTlvTagLen(psTag);
	if(iTagLen <= 0)
		return(HXEMV_NO_DATA); // ���Tag�Ƿ�, ��Ϊ�޴�����
    for(i=0; i<4; i++) {
        if(i == 0)
            psTlvDb = gl_sTlvDbTermFixed; // TLV���ݿ�, �ն˹̶�����
        else if(i == 1)
            psTlvDb = gl_sTlvDbTermVar;  // TLV���ݿ�, �ն˿ɱ�����
        else if(i == 2)
            psTlvDb = gl_sTlvDbIssuer;  // TLV���ݿ�, ����������
        else
            psTlvDb = gl_sTlvDbCard; // TLV���ݿ�, ��Ƭ����
    	iRet = iTlvGetObj(psTlvDb, psTag, &psTlvObj);
        if(iRet > 0)
            break;
    }
    if(i >= 4)
		return(HXEMV_NO_DATA);
	ucTlvObjLen = (uchar)iRet;
	memcpy(sTlvObj, psTlvObj, iRet);

	// �ɹ�����������
	iRet = iTlvCheckTlvObject(sTlvObj);
	if(iRet < 0)
		return(HXEMV_NO_DATA); // �����ʽ��, ��Ϊ�޴�����
	if(iRet > (int)ucTlvObjLen)
		return(HXEMV_NO_DATA); // �����ʽ��, ��Ϊ�޴�����

	if(piOutTlvDataLen && psOutTlvData) {
		if(*piOutTlvDataLen < (int)ucTlvObjLen)
			return(HXEMV_LACK_MEMORY);
		*piOutTlvDataLen = (int)ucTlvObjLen;
		memcpy(psOutTlvData, sTlvObj, *piOutTlvDataLen);
		vTraceWriteBinCr(TRACE_ALWAYS, "Tlv -->", psOutTlvData, *piOutTlvDataLen);
	}
	if(piOutDataLen==NULL || psOutData==NULL)
		return(HXEMV_OK);

    // ����
	iValueLen = iTlvValue(sTlvObj, &psValue);
	iValueType = (int)uiTagAttrGetType(sTlvObj);
    switch(iValueType) {
    case TAG_ATTR_N:
		if(*piOutDataLen <= iValueLen*2)
			return(HXEMV_LACK_MEMORY);
        vOneTwo0(psValue, iValueLen, psOutData);
        *piOutDataLen = iValueLen*2;
        break;
    case TAG_ATTR_CN:
        vOneTwo0(psValue, iValueLen, szBuf);
		vTrimTailF(szBuf);
		if(*piOutDataLen <= (int)strlen(szBuf))
			return(HXEMV_LACK_MEMORY);
		strcpy(psOutData, szBuf);
        *piOutDataLen = strlen((char *)psOutData);
        break;
    case TAG_ATTR_A:
    case TAG_ATTR_AN:
    case TAG_ATTR_ANS:
		if(*piOutDataLen <= iValueLen)
			return(HXEMV_LACK_MEMORY);
        memcpy(psOutData, psValue, iValueLen);
        psOutData[iValueLen] = 0;
        *piOutDataLen = strlen((char *)psOutData);
        break;
    case TAG_ATTR_B:
    default: // δ֪Tag��b�ʹ���
		if(*piOutDataLen < iValueLen)
			return(HXEMV_LACK_MEMORY);
        memcpy(psOutData, psValue, iValueLen);
        *piOutDataLen = iValueLen;
        break;
    }
	if(iValueType==TAG_ATTR_N || iValueType==TAG_ATTR_CN || iValueType==TAG_ATTR_A || iValueType==TAG_ATTR_AN || iValueType==TAG_ATTR_ANS) {
		vTraceWriteTxtCr(TRACE_ALWAYS, "%s", psOutData);
	} else {
		vTraceWriteBinCr(TRACE_ALWAYS, "NTlv -->", psOutData, *piOutDataLen);
	}
	return(HXEMV_OK);
}

// ����Ƭ������ˮ֧����Ϣ
// out : piMaxRecNum       : ��ཻ����ˮ��¼����
// ret : HXEMV_OK          : ��ȡ�ɹ�
//       HXEMV_NO_LOG      : ��Ƭ��֧�ֽ�����ˮ��¼
//       HXEMV_FLOW_ERROR  : EMV���̴���
int iHxGetLogInfo(int *piMaxRecNum)
{
	int   iRet;
	uchar *p;

	vTraceWriteTxtCr(TRACE_PROC, "iHxGetLogInfo()");
	if(gl_iCoreStatus < EMV_STATUS_APP_SELECT)
		return(HXEMV_FLOW_ERROR); // Ӧ��ѡ��֮����Զ���Ƭ������־

	iRet = iTlvGetObjValue(gl_sTlvDbCard, TAG_9F4D_LogEntry, &p);
	if(iRet != 2)
		return(HXEMV_NO_LOG);
	if(p[0]<10 || p[0]>30)
		return(HXEMV_NO_LOG); // SFI�������11-30֮��, ��Ϊ��֧�ֽ��׼�¼
	if(p[1] == 0)
		return(HXEMV_NO_LOG); // ����׼�¼�������Ϊ0, ��Ϊ��֧�ֽ��׼�¼
	*piMaxRecNum = (int)p[1];
	vTraceWriteTxtCr(TRACE_PROC, "(0)iHxGetLogInfo(%d)", *piMaxRecNum);
	return(HXEMV_OK);
}

// ����Ƭ������ˮ
// in  : iLogNo            : ������ˮ��¼��, �����һ����¼��Ϊ1
//       piLogLen          : psLog��������С
// out : piLogLen          : ������ˮ��¼����
//       psLog             : ��¼����(IC��ԭ��ʽ���)
// ret : HXEMV_OK          : ��ȡ�ɹ�
//       HXEMV_NO_RECORD   : �޴˼�¼
//       HXEMV_LACK_MEMORY : ����������
//       HXEMV_CARD_REMOVED: ����ȡ��
//       HXEMV_CARD_OP     : ��������
//       HXEMV_CARD_SW     : �Ƿ���ָ��״̬��
//       HXEMV_NO_LOG      : ��Ƭ��֧�ֽ�����ˮ��¼
//       HXEMV_FLOW_ERROR  : EMV���̴���
// note: ��������ʾ����
int iHxReadLog(int iLogNo, int *piLogLen, uchar *psLog)
{
	int   iRet;
	uint  uiRet;
	uchar *p;
	uchar ucLogLen, sLog[256];

	vTraceWriteTxtCr(TRACE_PROC, "iHxReadLog(%d,%d)", iLogNo, *piLogLen);
	if(gl_iCoreStatus < EMV_STATUS_APP_SELECT)
		return(HXEMV_FLOW_ERROR); // Ӧ��ѡ��֮����Զ���Ƭ������־

	iRet = iTlvGetObjValue(gl_sTlvDbCard, TAG_9F4D_LogEntry, &p);
	if(iRet != 2)
		return(HXEMV_NO_LOG);
	if(p[0]<10 || p[0]>30)
		return(HXEMV_NO_LOG); // SFI�������11-30֮��, ��Ϊ��֧�ֽ��׼�¼
	if(iLogNo<1 || iLogNo>(int)p[1])
		return(HXEMV_NO_RECORD);
	uiRet = uiEmvCmdRdRec(p[0], (uchar)iLogNo, &ucLogLen, sLog);
	if(uiRet == 0x6A83)
		return(HXEMV_NO_RECORD);
	if(uiRet == 1) {
		if(uiEmvTestCard() == 0)
			return(HXEMV_CARD_REMOVED); // ����ȡ��
		else
			return(HXEMV_CARD_OP); // ��Ƭ��������
	}
	if(uiRet)
		return(HXEMV_CARD_SW);
	if((int)ucLogLen > *piLogLen)
		return(HXEMV_LACK_MEMORY);

	memcpy(psLog, sLog, ucLogLen);
	*piLogLen = (int)ucLogLen;
	return(HXEMV_OK);
}

// ����ƬȦ�潻����ˮ֧����Ϣ
// out : piMaxRecNum       : ��ཻ����ˮ��¼����
// ret : HXEMV_OK          : ��ȡ�ɹ�
//       HXEMV_NO_LOG      : ��Ƭ��֧��Ȧ����ˮ��¼
//       HXEMV_FLOW_ERROR  : EMV���̴���
int iHxGetLoadLogInfo(int *piMaxRecNum)
{
	int   iRet;
	uchar *p;

	vTraceWriteTxtCr(TRACE_PROC, "iHxGetLoadLogInfo()");
	if(gl_iCoreStatus < EMV_STATUS_APP_SELECT)
		return(HXEMV_FLOW_ERROR); // Ӧ��ѡ��֮����Զ���Ƭ������־

	iRet = iTlvGetObjValue(gl_sTlvDbCard, TAG_DF4D_LoadLogEntry, &p);
	if(iRet != 2)
		return(HXEMV_NO_LOG);
	if(p[0]<10 || p[0]>30)
		return(HXEMV_NO_LOG); // SFI�������11-30֮��, ��Ϊ��֧�ֽ��׼�¼
	if(p[1] == 0)
		return(HXEMV_NO_LOG); // ����׼�¼�������Ϊ0, ��Ϊ��֧�ֽ��׼�¼
	*piMaxRecNum = (int)p[1];
	vTraceWriteTxtCr(TRACE_PROC, "(0)iHxGetLoadLogInfo(%d)", *piMaxRecNum);
	return(HXEMV_OK);
}

// ��Ȧ�潻����ˮ
// in  : iLogNo            : ������ˮ��¼��, �����һ����¼��Ϊ1, Ҫȫ��������¼��Ϊ0
//       piLogLen          : psLog��������С
// out : piLogLen          : ������ˮ��¼����
//       psLog             : ��¼����(IC��ԭ��ʽ���)
// ret : HXEMV_OK          : ��ȡ�ɹ�
//       HXEMV_NO_RECORD   : �޴˼�¼
//       HXEMV_LACK_MEMORY : ����������
//       HXEMV_CARD_REMOVED: ����ȡ��
//       HXEMV_CARD_OP     : ��������
//       HXEMV_CARD_SW     : �Ƿ���ָ��״̬��
//       HXEMV_NO_LOG      : ��Ƭ��֧�ֽ�����ˮ��¼
//       HXEMV_FLOW_ERROR  : EMV���̴���
// note: ��������ʾ����
int iHxReadLoadLog(int iLogNo, int *piLogLen, uchar *psLog)
{
	int   iRet;
	uint  uiRet;
	uchar *p;
	uchar ucLogLen, sLog[256];

	vTraceWriteTxtCr(TRACE_PROC, "iHxReadLoadLog(%d,%d)", iLogNo, *piLogLen);
	if(gl_iCoreStatus < EMV_STATUS_APP_SELECT)
		return(HXEMV_FLOW_ERROR); // Ӧ��ѡ��֮����Զ���Ƭ������־

	iRet = iTlvGetObjValue(gl_sTlvDbCard, TAG_DF4D_LoadLogEntry, &p);
	if(iRet != 2)
		return(HXEMV_NO_LOG);
	if(p[0]<10 || p[0]>30)
		return(HXEMV_NO_LOG); // SFI�������11-30֮��, ��Ϊ��֧�ֽ��׼�¼
	if(iLogNo<1 || iLogNo>(int)p[1])
		return(HXEMV_NO_RECORD);
	uiRet = uiEmvCmdRdRec(p[0], (uchar)iLogNo, &ucLogLen, sLog);
	if(uiRet == 0x6A83)
		return(HXEMV_NO_RECORD);
	if(uiRet == 1) {
		if(uiEmvTestCard() == 0)
			return(HXEMV_CARD_REMOVED); // ����ȡ��
		else
			return(HXEMV_CARD_OP); // ��Ƭ��������
	}
	if(uiRet)
		return(HXEMV_CARD_SW);
	if((int)ucLogLen > *piLogLen)
		return(HXEMV_LACK_MEMORY);

	memcpy(psLog, sLog, ucLogLen);
	*piLogLen = (int)ucLogLen;
	return(HXEMV_OK);
}

// �ǽӴ���GPO�������
// out : piTransRoute               : �����ߵ�·�� 0:�Ӵ�Pboc(��׼DC��EC) 1:�ǽӴ�Pboc(��׼DC��EC) 2:qPboc���� 3:qPboc�ѻ� 4:qPboc�ܾ�
//       piSignFlag					: ��Ҫǩ�ֱ�־(0:����Ҫǩ��, 1:��Ҫǩ��), �����ߵ���qPboc����ʱ����Ч
//       piNeedOnlinePin            : ��Ҫ���������־(0:����Ҫ��������, 1:��Ҫ��������), �����ߵ���qPboc��������ʱ����Ч
// ret : HXEMV_OK					: OK, ����piTransRoute������������
//       HXEMV_TERMINATE			: ����ܾ�������������ֹ
//		 HXEMV_TRY_OTHER_INTERFACE	: ��������ͨ�Ž���
//       HXEMV_CORE					: �ڲ�����
// Note: GPO�ɹ���ſ��Ե���
// �ο� JR/T0025.12��2013, 7.8 P40
int iHxCtlsGpoAnalyse(int *piTransRoute, int *piSignFlag, int *piNeedOnlinePin)
{
	int iRet;
	iRet = iPbocCtlsGpoAnalyse(piTransRoute, piSignFlag, piNeedOnlinePin);
	vTraceWriteTxtCr(TRACE_PROC, "(%d)iHxCtlsGpoAnalyse(%d,%d,%d)", iRet, *piTransRoute, *piSignFlag, *piNeedOnlinePin);
	return(iRet);
}

// ��������´�����
// in  : pszAmount         : ���
//       pszAmountOther    : �������, ע��, �ݲ�֧��, ���Ļ���Ը�ֵ
// ret : HXEMV_OK          : ���óɹ�
//       HXEMV_FLOW_ERROR  : EMV���̴���
//       HXEMV_PARA        : ��������
// note: GPO��GAC�����ܻ᲻ͬ, �����趨���׽��
//       ����¼��, ��������ǰ�ɵ���
int iHxSetAmount(uchar *pszAmount, uchar *pszAmountOther)
{
	int iRet;

	vTraceWriteTxtCr(TRACE_PROC, "iHxSetAmount(%s,%s)", pszAmount, pszAmountOther);
	if(gl_iCoreStatus < EMV_STATUS_READ_REC)
		return(HXEMV_FLOW_ERROR); // ����¼֮������������趨���
	if(gl_iCoreStatus >= EMV_STATUS_PROC_RISTRICTIONS)
		return(HXEMV_FLOW_ERROR); // ��������֮�����������趨���
	iRet = iEmvIOSetTransAmount(pszAmount, pszAmountOther);
	if(iRet != HXEMV_OK)
		return(HXEMV_PARA);
	iRet = iEmvSaveAmount(pszAmount);
	if(iRet != HXEMV_OK)
		return(HXEMV_PARA);
	return(HXEMV_OK);
}

//// ����Ϊ�ص���ʽAPI
//// �ص���ʽAPI  *** ��ʼ ***

// ��λ��Ƭ
// ret : HXEMV_OK          : OK
//       HXEMV_FLOW_ERROR  : EMV���̴���
//       HXEMV_CARD_REMOVED: ����ȡ��
//       HXEMV_CARD_OP     : ��Ƭ��λ����
//		 HXEMV_CALLBACK_METHOD : û���Իص���ʽ��ʼ������
int iHxResetCard(void)
{
	uint uiRet;

	vTraceWriteTxtCr(TRACE_PROC, "iHxResetCard()");
	if(gl_iCoreCallFlag != HXCORE_CALLBACK)
		return(HXEMV_CALLBACK_METHOD); // û���Իص���ʽ��ʼ������
	if(gl_iCoreStatus != EMV_STATUS_TRANS_INIT)
		return(HXEMV_FLOW_ERROR); // ���׳�ʼ����ſ��Ը�λ��Ƭ

	iEmvIOShowMsg(EMVMSG_0E_PLEASE_WAIT);

	uiRet = uiEmvCmdResetCard();
	if(uiRet != 0) {
		if(uiEmvTestCard() == 0) {
			iEmvIODispMsg(EMVMSG_0F_PROCESSING_ERROR, EMVIO_NEED_NO_CONFIRM);
			return(HXEMV_CARD_REMOVED); // ����ȡ��
		} else {
			iEmvIODispMsg(EMVMSG_06_CARD_ERROR, EMVIO_NEED_NO_CONFIRM);
			if(iEmvTestItemBit(EMV_ITEM_TERM_CAP, TERM_CAP_01_MAG_STRIPE)) {
				// ���ڴſ�������
				iEmvIODispMsg(EMVMSG_12_USE_MAG_STRIPE, EMVIO_NEED_NO_CONFIRM);
			}
			return(HXEMV_CARD_OP); // ��Ƭ�������󣬽�����ֹ
		}
	}

	gl_iCoreStatus = EMV_STATUS_RESET;
	return(HXEMV_OK);
}

// ��ȡ֧�ֵ�Ӧ��
// in  : iIgnoreBlock	   : !0:����Ӧ��������־, 0:������Ӧ�ò��ᱻѡ��
// ret : HXEMV_OK          : OK
//       HXEMV_NO_APP      : ��֧�ֵ�Ӧ��
//       HXEMV_CARD_REMOVED: ����ȡ��
//       HXEMV_CARD_OP     : ��������
//       HXEMV_CARD_SW     : ��״̬�ַǷ�
//       HXEMV_CANCEL      : �û�ȡ��
//       HXEMV_TIMEOUT     : ��ʱ
//       HXEMV_TERMINATE   : ����ܾ�������������ֹ
//       HXEMV_FLOW_ERROR  : EMV���̴���
//       HXEMV_CORE        : �ڲ�����
//		 HXEMV_CALLBACK_METHOD : û���Իص���ʽ��ʼ������
// �ǽӿ��ܷ�����
//		 HXEMV_TRY_OTHER_INTERFACE	: ��������ͨ�Ž���
//		 HXEMV_CARD_TRY_AGAIN		: Ҫ�������ύ��Ƭ(�ǽ�)
// Note: ���øú�����ɻ�ȡ����ƥ�������(TAG_DFXX_LanguageUsed)
int iHxGetSupportedApp(int iIgnoreBlock)
{
	int   iRet;

	vTraceWriteTxtCr(TRACE_PROC, "iHxGetSupportApp(%d)", iIgnoreBlock);
	if(gl_iCoreCallFlag != HXCORE_CALLBACK)
		return(HXEMV_CALLBACK_METHOD); // û���Իص���ʽ��ʼ������
	if(gl_iCoreStatus != EMV_STATUS_RESET)
		return(HXEMV_FLOW_ERROR); // ��Ƭ��λ��ſ��Ի�ȡ֧�ֵ�Ӧ��

	iEmvIOShowMsg(EMVMSG_0E_PLEASE_WAIT); // newadd

	// ������֧ͬ�ֵ�aid�б�
	iRet = iSelGetAids(iIgnoreBlock);
	EMV_TRACE_PROC_RET(iRet)
	if(iRet == SEL_ERR_CARD) {
		if(gl_iCardType == EMV_CONTACTLESS_CARD) {
			// �ǽӿ�, ��ʧЧ����, �ο�JR/T0025.12��2013ͼ4, P11
			iEmvIODispMsg(EMVMSG_13_TRY_AGAIN, EMVIO_NEED_NO_CONFIRM);
			return(HXEMV_CARD_TRY_AGAIN);
		}

		if(uiEmvTestCard() == 0) {
			iEmvIODispMsg(EMVMSG_0F_PROCESSING_ERROR, EMVIO_NEED_NO_CONFIRM);
			return(HXEMV_CARD_REMOVED); // ����ȡ��
		} else {
			iEmvIODispMsg(EMVMSG_06_CARD_ERROR, EMVIO_NEED_NO_CONFIRM);
			return(HXEMV_CARD_OP); // ��Ƭ�������󣬽�����ֹ
		}
	}
	if(iRet == SEL_ERR_CARD_SW) {
		if(gl_iCardType == EMV_CONTACTLESS_CARD) {
			// �ǽӿ�, �ο�JR/T0025.12��2013ͼ4, P11
			iEmvIODispMsg(EMVMSG_TERMINATE, EMVIO_NEED_NO_CONFIRM);
			return(HXEMV_TRY_OTHER_INTERFACE);
		}
		// Refer to EMV2008 book3 6.3.5, P47, ��ʶ���SW��Ҫ��ֹ����
		iEmvIODispMsg(EMVMSG_TERMINATE, EMVIO_NEED_NO_CONFIRM);
		return(HXEMV_CARD_SW); // ��״̬�ַǷ���������ֹ
	}

	if(iRet == SEL_ERR_CARD_BLOCKED) {
		iEmvIODispMsg(EMVMSG_TERMINATE, EMVIO_NEED_NO_CONFIRM);
		return(HXEMV_TERMINATE); // ��Ƭ������������ֹ
	}
	if(iRet == SEL_ERR_DATA_ERROR) {
		if(gl_iCardType == EMV_CONTACTLESS_CARD) {
			// �ǽӿ�, �ο�JR/T0025.12��2013 6.4.1, P12
			iEmvIODispMsg(EMVMSG_TERMINATE, EMVIO_NEED_NO_CONFIRM);
			return(HXEMV_TRY_OTHER_INTERFACE);
		}
		iEmvIODispMsg(EMVMSG_TERMINATE, EMVIO_NEED_NO_CONFIRM);
		return(HXEMV_TERMINATE); // Ӧ��������ݷǷ���������ֹ
	}
	if(iRet == SEL_ERR_DATA_ABSENT) {
		if(gl_iCardType == EMV_CONTACTLESS_CARD) {
			// �ǽӿ�, �ο�JR/T0025.12��2013 6.4.1, P12
			iEmvIODispMsg(EMVMSG_TERMINATE, EMVIO_NEED_NO_CONFIRM);
			return(HXEMV_TRY_OTHER_INTERFACE);
		}
		iEmvIODispMsg(EMVMSG_TERMINATE, EMVIO_NEED_NO_CONFIRM);
		return(HXEMV_TERMINATE); // ��Ҫ���ݲ����ڣ�������ֹ
	}
	if(iRet < 0) {
		iEmvIODispMsg(EMVMSG_NATIVE_ERR, EMVIO_NEED_NO_CONFIRM);
		return(HXEMV_CORE); // �ڴ治�㣬��Ϊ�ڲ����󷵻�
	}
	if(iRet == 0) {
		iEmvIODispMsg(EMVMSG_TERMINATE, EMVIO_NEED_NO_CONFIRM);
		return(HXEMV_NO_APP); // ��֧�ֵ�Ӧ��
	}

	gl_iCoreStatus = EMV_STATUS_GET_SUPPORTED_APP;

	return(HXEMV_OK);
}

// Ӧ��ѡ��
// in  : iIgnoreBlock	   : !0:����Ӧ��������־, 0:������Ӧ�ò��ᱻѡ��
// ret : HXEMV_OK          : OK
//       HXEMV_CARD_REMOVED: ����ȡ��
//       HXEMV_CARD_OP     : ��������
//       HXEMV_NO_APP	   : û��ѡ��Ӧ��
//       HXEMV_TERMINATE   : ����ܾ�������������ֹ
//       HXEMV_CANCEL      : �û�ȡ��
//       HXEMV_TIMEOUT     : ��ʱ
//       HXEMV_FLOW_ERROR  : EMV���̴���
//       HXEMV_CORE        : �ڲ�����
//		 HXEMV_CALLBACK_METHOD : û���Իص���ʽ��ʼ������
// �ǽӿ��ܷ�����
//		 HXEMV_TRY_OTHER_INTERFACE	: ��������ͨ�Ž���
//		 HXEMV_CARD_TRY_AGAIN		: Ҫ�������ύ��Ƭ(�ǽ�)
int iHxAppSelect(int iIgnoreBlock)
{
	int iRet;
	uchar *psPdol;
	int   iPdolLen;

	vTraceWriteTxtCr(TRACE_PROC, "iHxAppSelect(%d)", iIgnoreBlock);
	if(gl_iCoreCallFlag != HXCORE_CALLBACK)
		return(HXEMV_CALLBACK_METHOD); // û���Իص���ʽ��ʼ������
	if(gl_iCoreStatus != EMV_STATUS_GET_SUPPORTED_APP)
		return(HXEMV_FLOW_ERROR); // ��ȡ��ƬӦ���б��ſ���ѡ��Ӧ��

	iEmvIOShowMsg(EMVMSG_0E_PLEASE_WAIT); // newadd

	iRet = iSelFinalSelect(iIgnoreBlock);
	EMV_TRACE_PROC_RET(iRet)
	if(iRet == SEL_ERR_CARD) {
		if(gl_iCardType == EMV_CONTACTLESS_CARD) {
			// �ǽӿ�, ��ʧЧ����, �ο�JR/T0025.12��2013ͼ4, P11
			iEmvIODispMsg(EMVMSG_13_TRY_AGAIN, EMVIO_NEED_NO_CONFIRM);
			return(HXEMV_CARD_TRY_AGAIN);
		}
		if(uiEmvTestCard() == 0) {
			iEmvIODispMsg(EMVMSG_0F_PROCESSING_ERROR, EMVIO_NEED_NO_CONFIRM);
			return(HXEMV_CARD_REMOVED); // ����ȡ��
		} else {
			iEmvIODispMsg(EMVMSG_06_CARD_ERROR, EMVIO_NEED_NO_CONFIRM);
			return(HXEMV_CARD_OP); // ��Ƭ�������󣬽�����ֹ
		}
	}
	if(iRet == SEL_ERR_NO_APP) {
		if(gl_iCardType == EMV_CONTACTLESS_CARD) {
			// �ǽӿ�, �ο�JR/T0025.12��2013ͼ4, P11
			iEmvIODispMsg(EMVMSG_TERMINATE, EMVIO_NEED_NO_CONFIRM);
			return(HXEMV_TRY_OTHER_INTERFACE);
		}
		// refer to emv2008 book4 11.3, P91 wlfxy
//		iEmvIODispMsg(EMVMSG_0C_NOT_ACCEPTED, EMVIO_NEED_NO_CONFIRM);
		iEmvIODispMsg(EMVMSG_TERMINATE, EMVIO_NEED_NO_CONFIRM);
		return(HXEMV_NO_APP);
	}
	if(iRet == SEL_ERR_DATA_ERROR) {
		iEmvIODispMsg(EMVMSG_TERMINATE, EMVIO_NEED_NO_CONFIRM);
		return(HXEMV_TERMINATE);
	}
	if(iRet == SEL_ERR_CANCEL) {
		iEmvIODispMsg(EMVMSG_CANCEL, EMVIO_NEED_NO_CONFIRM);
		return(HXEMV_CANCEL);
	}
	if(iRet == SEL_ERR_TIMEOUT) {
		iEmvIODispMsg(EMVMSG_TIMEOUT, EMVIO_NEED_NO_CONFIRM);
		return(HXEMV_TIMEOUT);
	}
	if(iRet && iRet!=SEL_ERR_APP_BLOCKED) {
		iEmvIODispMsg(EMVMSG_NATIVE_ERR, EMVIO_NEED_NO_CONFIRM);
		return(HXEMV_CORE);
	}

	if(gl_iCardType == EMV_CONTACTLESS_CARD) {
		// �ǽӿ�
		iPdolLen = iTlvGetObjValue(gl_sTlvDbCard, TAG_9F38_PDOL, &psPdol);
		if(iPdolLen <= 0) {
			// �ǽӿ���PDOL, �ο�JR/T0025.12��2013ͼ4, P11
			iEmvIODispMsg(EMVMSG_TERMINATE, EMVIO_NEED_NO_CONFIRM);
			return(HXEMV_TRY_OTHER_INTERFACE);
		}
		iRet = iTlvSearchDOLTag(psPdol, iPdolLen, TAG_9F66_ContactlessSupport, NULL);
		if(iRet <= 0) {
			// �ǽӿ�PDOL��9F66, �ο�JR/T0025.12��2013ͼ4, P11
			iEmvIODispMsg(EMVMSG_TERMINATE, EMVIO_NEED_NO_CONFIRM);
			return(HXEMV_TRY_OTHER_INTERFACE);
		}
	}

	gl_iCoreStatus = EMV_STATUS_APP_SELECT;
	return(HXEMV_OK);
}

// ���óֿ�������
// in  : pszLanguage  : �ֿ������Դ���, ����:"zh"
//                      �������NULL, ��ʾʹ��EMV����ѡ�񷽷�
// ret : HXEMV_OK     : OK
//       HXEMV_FLOW_ERROR  : EMV���̴���
//		 HXEMV_CALLBACK_METHOD : û���Իص���ʽ��ʼ������
//       HXEMV_PARA   : ��������
//       HXEMV_CORE   : �ڲ�����
// Note: �����֧�ִ��������, ����ʹ�ñ�������
int iHxSetCardHolderLang(uchar *pszLanguage)
{
	int   iRet;
	uchar szLanguagePreferred[9];
	uchar szFinalLanguage[3];
	uchar *psValue, szPinpadLanguage[33];

	vTraceWriteTxtCr(TRACE_PROC, "iHxSetCardHolderLang(%s)", pszLanguage);
	if(gl_iCoreCallFlag != HXCORE_CALLBACK)
		return(HXEMV_CALLBACK_METHOD); // û���Իص���ʽ��ʼ������
	if(gl_iCoreStatus < EMV_STATUS_APP_SELECT)
		return(HXEMV_FLOW_ERROR); // ѡ��Ӧ�ú�ſ���

	if(pszLanguage) {
		if(strlen(pszLanguage) == 2) {
			// ��ƥ������Դ������TLV���ݿ�(TAG_DFXX_LanguageUsed)
			iRet = iTlvMakeAddObj(gl_sTlvDbTermVar, TAG_DFXX_LanguageUsed, 2, pszLanguage, TLV_CONFLICT_REPLACE);
			if(iRet < 0) {
				iEmvIODispMsg(EMVMSG_NATIVE_ERR, EMVIO_NEED_NO_CONFIRM);
				return(HXEMV_CORE);
			}
			return(HXEMV_OK);
		}
	}

	// emv����ѡ��
	iRet = iTlvGetObjValue(gl_sTlvDbCard, TAG_5F2D_LanguagePrefer, &psValue);
	if(iRet>0 && iRet<=8)
		vMemcpy0(szLanguagePreferred, psValue, iRet);
	else
		szLanguagePreferred[0] = 0;

	// ��ȡ����ƥ�������
	iRet = iEmvMsgTableGetFitLanguage(szLanguagePreferred, szFinalLanguage);
	// ʹ��ƥ�������, Ҳ����ǿ��ʹ����������
    iEmvMsgTableSetLanguage(szFinalLanguage);
	// ��ƥ������Դ������TLV���ݿ�(TAG_DFXX_LanguageUsed)
	iRet = iTlvMakeAddObj(gl_sTlvDbTermVar, TAG_DFXX_LanguageUsed, 2, szFinalLanguage, TLV_CONFLICT_REPLACE);
	if(iRet < 0) {
		iEmvIODispMsg(EMVMSG_NATIVE_ERR, EMVIO_NEED_NO_CONFIRM);
		return(HXEMV_CORE);
	}

	// ��ȡPinpadʹ�õ�����
	iRet = iTlvGetObjValue(gl_sTlvDbTermFixed, TAG_DFXX_PinpadLanguage, &psValue);
	if(iRet < 0) {
		iEmvIODispMsg(EMVMSG_NATIVE_ERR, EMVIO_NEED_NO_CONFIRM);
		return(HXEMV_CORE);
	}
	memcpy(szPinpadLanguage, psValue, iRet);
	szPinpadLanguage[iRet] = 0;
	iRet = iEmvMsgTableGetPinpadLanguage(szLanguagePreferred, szPinpadLanguage, szFinalLanguage);
	if(iRet==1/*1:��ƥ��*/ && strlen(szPinpadLanguage)>2) {
		// ��ƥ���pinpad���� ���� pinpad֧�ֶ���1������, Ҫ��ֿ���ѡ��
		if(iEmvTestItemBit(EMV_ITEM_TERM_CAP_ADD, TERM_CAP_ADD_16_NUMERIC_KEYS) ||
				iEmvTestItemBit(EMV_ITEM_TERM_CAP_ADD, TERM_CAP_ADD_17_ALPHA_KEYS) ||
				iEmvTestItemBit(EMV_ITEM_TERM_CAP_ADD, TERM_CAP_ADD_19_FUNC_KEYS)) {
			// �ն�֧�ְ���, Ҫ��ѡ������
			iRet = iEmvIOSelectLang(szFinalLanguage, NULL, NULL);
			if(iRet == EMVIO_CANCEL) {
				iEmvIODispMsg(EMVMSG_CANCEL, EMVIO_NEED_NO_CONFIRM);
				return(HXEMV_CANCEL);
			}
			if(iRet == EMVIO_TIMEOUT) {
				iEmvIODispMsg(EMVMSG_TIMEOUT, EMVIO_NEED_NO_CONFIRM);
				return(HXEMV_TIMEOUT);
			}
			if(iRet != EMVIO_OK) {
				iEmvIODispMsg(EMVMSG_NATIVE_ERR, EMVIO_NEED_NO_CONFIRM);
				return(HXEMV_CORE);
			}
		} // if(iEmvTestItemBit(EMV_ITEM_TERM_CAP, TERM_CAP_00_MANUAL_KEY_ENTRY)) {	
    } // if(iRet==1 && strlen(szPinpadLanguage)>2) {

	// ��ƥ������Դ������TLV���ݿ�(TAG_DFXX_LanguageUsed)
	iRet = iTlvMakeAddObj(gl_sTlvDbTermVar, TAG_DFXX_LanguageUsed, 2, szFinalLanguage, TLV_CONFLICT_REPLACE);
	if(iRet < 0) {
		iEmvIODispMsg(EMVMSG_NATIVE_ERR, EMVIO_NEED_NO_CONFIRM);
		return(HXEMV_CORE);
	}

	return(HXEMV_OK);
}

// GPO
// in  : pszTransTime      : ����ʱ�䣬YYYYMMDDhhmmss
//       ulTTC             : �ն˽������, 1-999999
//       uiTransType       : ��������, BCD��ʽ(0xAABB, BBΪ��GB/T 15150 ����Ĵ�����ǰ2 λ��ʾ�Ľ��ڽ�������, AA����������Ʒ/����)
//       uiCurrencyCode    : ���Ҵ���
// ret : HXEMV_OK          : OK
//       HXEMV_CARD_REMOVED: ����ȡ��
//       HXEMV_CARD_OP     : ��������
//       HXEMV_CARD_SW     : �Ƿ���״̬��
//       HXEMV_TERMINATE   : ����ܾ�������������ֹ
//       HXEMV_NO_APP      : ��֧�ֵ�Ӧ��
//       HXEMV_RESELECT    : ��Ҫ����ѡ��Ӧ��
//       HXEMV_PARA        : ��������
//       HXEMV_FLOW_ERROR  : EMV���̴���
//       HXEMV_TRANS_NOT_ALLOWED : ���ײ�֧��
//       HXEMV_CORE        : �ڲ�����
//		 HXEMV_CALLBACK_METHOD : û���Իص���ʽ��ʼ������
//       HXEMV_DENIAL	   : qPboc�ܾ�
// �ǽӿ��ܷ�����
//		 HXEMV_TRY_OTHER_INTERFACE	: ��������ͨ�Ž���
//		 HXEMV_CARD_TRY_AGAIN		: Ҫ�������ύ��Ƭ(�ǽ�)
// Note: �������HXEMV_RESELECT����Ҫ����ִ��iHxAppSelect()
int iHxGPO(uchar *pszTransTime, ulong ulTTC, uint uiTransType, uint uiCurrencyCode)
{
	uchar ucTransType; // ��������, T9Cֵ
	int   iRet;
	int   iDecimalPos;
	uchar sBuf[100];

	vTraceWriteTxtCr(TRACE_PROC, "iHxGPO(%s,%lu,%04X,%u)", pszTransTime, ulTTC, uiTransType, uiCurrencyCode);
	if(gl_iCoreCallFlag != HXCORE_CALLBACK)
		return(HXEMV_CALLBACK_METHOD); // û���Իص���ʽ��ʼ������
	if(gl_iCoreStatus != EMV_STATUS_APP_SELECT)
		return(HXEMV_FLOW_ERROR); // ѡ��Ӧ�ú�ſ���GPO

	iEmvIOShowMsg(EMVMSG_0E_PLEASE_WAIT); // newadd

	ucTransType = uiTransType & 0xFF; // ȡ��T9Cֵ
	// �����Ϸ��Լ��
	iRet = iTestIfValidDateTime(pszTransTime);
	if(iRet || ulTTC<1 || ulTTC>999999L) {
		iEmvIODispMsg(EMVMSG_NATIVE_ERR, EMVIO_NEED_NO_CONFIRM);
		return(HXEMV_PARA);
	}
	switch(uiTransType) {
	case TRANS_TYPE_SALE:			// ����(��Ʒ�ͷ���)
		if(!iEmvTestItemBit(EMV_ITEM_TERM_CAP_ADD, TERM_CAP_ADD_01_GOODS)
				&& !iEmvTestItemBit(EMV_ITEM_TERM_CAP_ADD, TERM_CAP_ADD_02_SERVICES)) {
			iEmvIODispMsg(EMVMSG_TRANS_NOT_ALLOWED, EMVIO_NEED_NO_CONFIRM);
			return(HXEMV_TRANS_NOT_ALLOWED); // �����֧����Ʒ������, ��֧�����ѽ���
		}
		break;
	case TRANS_TYPE_GOODS:			// ��Ʒ
		if(!iEmvTestItemBit(EMV_ITEM_TERM_CAP_ADD, TERM_CAP_ADD_01_GOODS)) {
			iEmvIODispMsg(EMVMSG_TRANS_NOT_ALLOWED, EMVIO_NEED_NO_CONFIRM);
			return(HXEMV_TRANS_NOT_ALLOWED); // �����֧����ƷӦ��, ��֧����Ʒ����
		}
		break;
	case TRANS_TYPE_SERVICES:		// ����
		if(!iEmvTestItemBit(EMV_ITEM_TERM_CAP_ADD, TERM_CAP_ADD_02_SERVICES)) {
			iEmvIODispMsg(EMVMSG_TRANS_NOT_ALLOWED, EMVIO_NEED_NO_CONFIRM);
			return(HXEMV_TRANS_NOT_ALLOWED); // �����֧�ַ���Ӧ��, ��֧�ַ�����
		}
		break;
	case TRANS_TYPE_PAYMENT:		// ֧��
		if(!iEmvTestItemBit(EMV_ITEM_TERM_CAP_ADD, TERM_CAP_ADD_06_PAYMENT)) {
			iEmvIODispMsg(EMVMSG_TRANS_NOT_ALLOWED, EMVIO_NEED_NO_CONFIRM);
			return(HXEMV_TRANS_NOT_ALLOWED); // �����֧��֧��Ӧ��, ��֧��֧������
		}
		break;
	case TRANS_TYPE_CASH:			// ȡ��
		if(!iEmvTestItemBit(EMV_ITEM_TERM_CAP_ADD, TERM_CAP_ADD_00_CASH)) {
			iEmvIODispMsg(EMVMSG_TRANS_NOT_ALLOWED, EMVIO_NEED_NO_CONFIRM);
			return(HXEMV_TRANS_NOT_ALLOWED); // �����֧���ֽ�Ӧ��, ��֧��ȡ�ֽ���
		}
		break;
	case TRANS_TYPE_RETURN:			// �˻�
		break; // �����ն�������չ��û�ж��˻��Ķ���, ��Ϊ֧��
	case TRANS_TYPE_DEPOSIT:		// ���
		if(!iEmvTestItemBit(EMV_ITEM_TERM_CAP_ADD, TERM_CAP_ADD_08_CASH_DEPOSIT)) {
			iEmvIODispMsg(EMVMSG_TRANS_NOT_ALLOWED, EMVIO_NEED_NO_CONFIRM);
			return(HXEMV_TRANS_NOT_ALLOWED); // �����֧�ִ��Ӧ��, ��֧�ִ���
		}
		break;
	case TRANS_TYPE_AVAILABLE_INQ:	// ��������ѯ
	case TRANS_TYPE_BALANCE_INQ:	// ����ѯ
		if(!iEmvTestItemBit(EMV_ITEM_TERM_CAP_ADD, TERM_CAP_ADD_04_INQUIRY)) {
			iEmvIODispMsg(EMVMSG_TRANS_NOT_ALLOWED, EMVIO_NEED_NO_CONFIRM);
			return(HXEMV_TRANS_NOT_ALLOWED); // �����֧�ִ��Ӧ��, ��֧�ִ���
		}
		break;
	case TRANS_TYPE_TRANSFER:		// ת��
		if(!iEmvTestItemBit(EMV_ITEM_TERM_CAP_ADD, TERM_CAP_ADD_05_TRANSFER)) {
			iEmvIODispMsg(EMVMSG_TRANS_NOT_ALLOWED, EMVIO_NEED_NO_CONFIRM);
			return(HXEMV_TRANS_NOT_ALLOWED); // �����֧��ת��Ӧ��, ��֧��ת�˽���
		}
		break;
	default:
		if(gl_iAppType != 0/*0:�ͼ����*/)
			break; // ���ͼ����, �������н���
		iEmvIODispMsg(EMVMSG_TRANS_NOT_ALLOWED, EMVIO_NEED_NO_CONFIRM);
		return(HXEMV_TRANS_NOT_ALLOWED); // ��ʶ��Ľ�������
	}

	if(uiCurrencyCode>999) {
		iEmvIODispMsg(EMVMSG_NATIVE_ERR, EMVIO_NEED_NO_CONFIRM);
		return(HXEMV_PARA);
	}

	// ���������ڡ�����ʱ�䡢�ն˽�����ˮ�š��������͡����Ҵ��롢����ָ����PosEntryMode����tlv���ݿ�
	vTwoOne(pszTransTime+2, 6, sBuf); // ��������
	iRet = iTlvMakeAddObj(gl_sTlvDbTermVar, TAG_9A_TransDate, 3, sBuf, TLV_CONFLICT_REPLACE);
	if(iRet < 0) {
		EMV_TRACE_ERR_MSG_RET(iRet)
		iEmvIODispMsg(EMVMSG_NATIVE_ERR, EMVIO_NEED_NO_CONFIRM);
		return(HXEMV_CORE);
	}
	vTwoOne(pszTransTime+8, 6, sBuf); // ����ʱ��
	iRet = iTlvMakeAddObj(gl_sTlvDbTermVar, TAG_9F21_TransTime, 3, sBuf, TLV_CONFLICT_REPLACE);
	if(iRet < 0) {
		EMV_TRACE_ERR_MSG_RET(iRet)
		iEmvIODispMsg(EMVMSG_NATIVE_ERR, EMVIO_NEED_NO_CONFIRM);
		return(HXEMV_CORE);
	}
	sprintf(sBuf, "%06lu", ulTTC);
	vTwoOne(sBuf, 6, sBuf+10); // T9F41����N4-N8�ֽ�, ������ѭ8583Э��,����N6
	iRet = iTlvMakeAddObj(gl_sTlvDbTermVar, TAG_9F41_TransSeqCounter, 3, sBuf+10, TLV_CONFLICT_REPLACE);
	if(iRet < 0) {
		EMV_TRACE_ERR_MSG_RET(iRet)
		iEmvIODispMsg(EMVMSG_NATIVE_ERR, EMVIO_NEED_NO_CONFIRM);
		return(HXEMV_CORE);
	}
	
	vOneTwo0(&ucTransType, 1, sBuf); // ��������Ϊn2��
	if(iTestStrDecimal(sBuf, 2) != 0) {
		EMV_TRACE_ERR_MSG
		iEmvIODispMsg(EMVMSG_NATIVE_ERR, EMVIO_NEED_NO_CONFIRM);
		return(HXEMV_PARA);
	}
	iRet = iTlvMakeAddObj(gl_sTlvDbTermVar, TAG_9C_TransType, 1, &ucTransType, TLV_CONFLICT_REPLACE);
	if(iRet < 0) {
		EMV_TRACE_ERR_MSG_RET(iRet)
		iEmvIODispMsg(EMVMSG_NATIVE_ERR, EMVIO_NEED_NO_CONFIRM);
		return(HXEMV_CORE);
	}
	vLongToStr(uiTransType, 2, sBuf);
	iRet = iTlvMakeAddObj(gl_sTlvDbTermVar, TAG_DFXX_TransType, 2, sBuf, TLV_CONFLICT_REPLACE);
	if(iRet < 0) {
		EMV_TRACE_ERR_MSG_RET(iRet)
		iEmvIODispMsg(EMVMSG_NATIVE_ERR, EMVIO_NEED_NO_CONFIRM);
		return(HXEMV_CORE);
	}

	sprintf(sBuf, "%04lu", uiCurrencyCode); // ���׻��Ҵ���
	vTwoOne(sBuf, 4, sBuf+10);
	iRet = iTlvMakeAddObj(gl_sTlvDbTermVar, TAG_5F2A_TransCurrencyCode, 2, sBuf+10, TLV_CONFLICT_REPLACE);
	if(iRet < 0) {
		EMV_TRACE_ERR_MSG_RET(iRet)
		iEmvIODispMsg(EMVMSG_NATIVE_ERR, EMVIO_NEED_NO_CONFIRM);
		return(HXEMV_CORE);
	}

	// ���׻���ָ��
   	iIso4217SearchDigitCode(uiCurrencyCode, sBuf, &iDecimalPos);
	vL2Bcd((ulong)iDecimalPos, 1, sBuf);
	iRet = iTlvMakeAddObj(gl_sTlvDbTermVar, TAG_5F36_TransCurrencyExp, 1, sBuf, TLV_CONFLICT_REPLACE);
	if(iRet < 0) {
		EMV_TRACE_ERR_MSG_RET(iRet)
		iEmvIODispMsg(EMVMSG_NATIVE_ERR, EMVIO_NEED_NO_CONFIRM);
		return(HXEMV_CORE);
	}

    // Pos Entry Mode (�ǽӿ����ڲ�֪���ߵ��Ǳ�׼pboc����qPboc, ���Ҵ����ֵ, ��ȷ��������)
	iRet = iTlvMakeAddObj(gl_sTlvDbTermVar, TAG_9F39_POSEntryMode, 1, "\x05", TLV_CONFLICT_REPLACE); // pos entry mode, "05":ic card
	if(iRet < 0) {
		EMV_TRACE_ERR_MSG_RET(iRet)
		iEmvIODispMsg(EMVMSG_NATIVE_ERR, EMVIO_NEED_NO_CONFIRM);
		return(HXEMV_CORE);
	}

	if(gl_iCardType == EMV_CONTACTLESS_CARD) {
		// ����Ƿǽӿ�, ��T9F66����tlv���ݿ�
		iRet = iTlvMakeAddObj(gl_sTlvDbTermVar, TAG_9F66_ContactlessSupport, 4, psPbocCtlsGet9F66(), TLV_CONFLICT_REPLACE);
		if(iRet < 0) {
			EMV_TRACE_ERR_MSG_RET(iRet)
			iEmvIODispMsg(EMVMSG_NATIVE_ERR, EMVIO_NEED_NO_CONFIRM);
			return(HXEMV_CORE);
		}
	}

	// GPO
	iRet = iEmvGpo();
	vTraceWriteTxtCr(TRACE_PROC, "(%d)iEmvGpo()", iRet);
	if(iRet == HXEMV_RESELECT) {
		iEmvIODispMsg(EMVMSG_0C_NOT_ACCEPTED, EMVIO_NEED_NO_CONFIRM);
		gl_iCoreStatus = EMV_STATUS_GET_SUPPORTED_APP; // ��Ҫ����ѡ��Ӧ��
		if(gl_iCardAppNum <= 0) {
			iEmvIODispMsg(EMVMSG_TERMINATE, EMVIO_NEED_NO_CONFIRM);
			return(HXEMV_NO_APP); // ���û��APP��ѡ, ������APP
		}
		return(HXEMV_RESELECT);
	}
	if(iRet == HXEMV_CANCEL) {
		iEmvIODispMsg(EMVMSG_CANCEL, EMVIO_NEED_NO_CONFIRM);
		return(iRet);
	}
	if(iRet == HXEMV_TIMEOUT) {
		iEmvIODispMsg(EMVMSG_TIMEOUT, EMVIO_NEED_NO_CONFIRM);
		return(iRet);
	}
	if(iRet == HXEMV_CARD_OP) {
		if(gl_iCardType == EMV_CONTACTLESS_CARD) {
			// �ǽӿ�, ��ʧЧ����, �ο�JR/T0025.12��2013 6.5.2, P14
			iEmvIODispMsg(EMVMSG_13_TRY_AGAIN, EMVIO_NEED_NO_CONFIRM);
			return(HXEMV_CARD_TRY_AGAIN);
		}
		if(uiEmvTestCard() == 0) {
			iEmvIODispMsg(EMVMSG_0F_PROCESSING_ERROR, EMVIO_NEED_NO_CONFIRM);
			return(HXEMV_CARD_REMOVED); // ����ȡ��
		} else {
			iEmvIODispMsg(EMVMSG_06_CARD_ERROR, EMVIO_NEED_NO_CONFIRM);
			return(HXEMV_CARD_OP); // ��Ƭ�������󣬽�����ֹ
		}
	}
	if(iRet == HXEMV_CARD_SW) {
		if(gl_iCardType == EMV_CONTACTLESS_CARD) {
			// �ǽӿ�, �ο�JR/T0025.12��2013 6.5.3, P14
			iEmvIODispMsg(EMVMSG_TERMINATE, EMVIO_NEED_NO_CONFIRM);
			return(HXEMV_TRY_OTHER_INTERFACE);
		}
		// Refer to EMV2008 book3 6.3.5, P47, ��ʶ���SW��Ҫ��ֹ����
		iEmvIODispMsg(EMVMSG_TERMINATE, EMVIO_NEED_NO_CONFIRM);
		return(HXEMV_CARD_SW);
	}
	if(iRet == HXEMV_TERMINATE) {
		iEmvIODispMsg(EMVMSG_TERMINATE, EMVIO_NEED_NO_CONFIRM);
		if(gl_iCardType == EMV_CONTACTLESS_CARD)
			return(HXEMV_TRY_OTHER_INTERFACE); // �ǽӿ�, ��ΪӦ��������
		return(HXEMV_TERMINATE);
	}
	if(iRet != HXEMV_OK) {
		iEmvIODispMsg(EMVMSG_NATIVE_ERR, EMVIO_NEED_NO_CONFIRM);
		return(iRet);
	}

	iEmvIOShowMsg(EMVMSG_0E_PLEASE_WAIT);
	gl_iCoreStatus = EMV_STATUS_GPO;
	return(HXEMV_OK);
}

// ��Ӧ�ü�¼
// ret : HXEMV_OK          : OK
//       HXEMV_CARD_REMOVED: ����ȡ��
//       HXEMV_CARD_OP     : ��������
//       HXEMV_CARD_SW     : �Ƿ���ָ��״̬��
//       HXEMV_TERMINATE   : ����ܾ�������������ֹ
//       HXEMV_FLOW_ERROR  : EMV���̴���
//       HXEMV_CORE        : �ڲ�����
//		 HXEMV_CALLBACK_METHOD : û���Իص���ʽ��ʼ������
// �ǽӿ��ܷ�����
//		 HXEMV_TRY_OTHER_INTERFACE	: ��������ͨ�Ž���
int iHxReadRecord(void)
{
	int iRet;

	vTraceWriteTxtCr(TRACE_PROC, "iHxReadRecord()");
	if(gl_iCoreCallFlag != HXCORE_CALLBACK)
		return(HXEMV_CALLBACK_METHOD); // û���Իص���ʽ��ʼ������
	if(gl_iCoreStatus != EMV_STATUS_GPO)
		return(HXEMV_FLOW_ERROR); // GPO��ſ��Զ���¼
	iRet = iPbocCtlsGetRoute(); // �����ߵ�·��, 0:�Ӵ�Pboc(��׼DC��EC) 1:�ǽӴ�Pboc(��׼DC��EC) 2:qPboc���� 3:qPboc�ѻ� 4:qPboc�ܾ�
	if(iRet==2 || iRet==4) {
		EMV_TRACE_ERR_MSG
		return(HXEMV_FLOW_ERROR); // qPboc������qPboc�ܾ������Զ���¼
	}

	iEmvIOShowMsg(EMVMSG_0E_PLEASE_WAIT); // ֮ǰĳЩѡ��ȿ��ܻ�����ȴ���Ϣ, �ڴ�����ʾһ��

	iRet = iEmvReadRecord();
	vTraceWriteTxtCr(TRACE_PROC, "(%d)iEmvReadRecord()", iRet);
	if(iRet == HXEMV_CARD_OP) {
		if(gl_iCardType == EMV_CONTACTLESS_CARD) {
			// �ǽӿ�, ��ʧЧ����, �ο�JR/T0025.12��2013 6.5.2, P14
			iEmvIODispMsg(EMVMSG_13_TRY_AGAIN, EMVIO_NEED_NO_CONFIRM);
			return(HXEMV_CARD_TRY_AGAIN);
		}
		if(uiEmvTestCard() == 0) {
			iEmvIODispMsg(EMVMSG_0F_PROCESSING_ERROR, EMVIO_NEED_NO_CONFIRM);
			return(HXEMV_CARD_REMOVED); // ����ȡ��
		} else {
			iEmvIODispMsg(EMVMSG_06_CARD_ERROR, EMVIO_NEED_NO_CONFIRM);
			return(HXEMV_CARD_OP); // ��Ƭ�������󣬽�����ֹ
		}
	}
	if(iRet == HXEMV_CARD_SW) {
		// Refer to EMV2008 book3 6.3.5, P47, ��ʶ���SW��Ҫ��ֹ����
		iEmvIODispMsg(EMVMSG_TERMINATE, EMVIO_NEED_NO_CONFIRM);
		return(HXEMV_CARD_SW);
	}
	if(iRet == HXEMV_TERMINATE) {
		iEmvIODispMsg(EMVMSG_TERMINATE, EMVIO_NEED_NO_CONFIRM);
		return(iRet);
	}
	if(iRet == HXEMV_EXPIRED_APP) {
		iEmvIODispMsg(EMVMSG_EXPIRED_APP, EMVIO_NEED_NO_CONFIRM);
		return(HXEMV_TERMINATE);
	}

	if(iRet) {
		iEmvIODispMsg(EMVMSG_NATIVE_ERR, EMVIO_NEED_NO_CONFIRM);
		return(HXEMV_CORE);
	}

	gl_iCoreStatus = EMV_STATUS_READ_REC;
	return(HXEMV_OK);
}

// SDA��DDA
// ret : HXEMV_OK            : OK
//       HXEMV_CARD_REMOVED  : ����ȡ��
//       HXEMV_CARD_OP       : ��������
//       HXEMV_CARD_SW       : �Ƿ���״̬��
//       HXEMV_TERMINATE     : ����ܾ�������������ֹ
//       HXEMV_DENIAL        : �ն���Ϊ�������Ϊ�ѻ��ܾ�
//       HXEMV_DENIAL_ADVICE : �ն���Ϊ�������Ϊ�ѻ��ܾ�, ��Advice
//       HXEMV_FLOW_ERROR    : EMV���̴���
//       HXEMV_CORE          : �ڲ�����
//		 HXEMV_CALLBACK_METHOD : û���Իص���ʽ��ʼ������
// �ǽӿ��ܷ�����
//		 HXEMV_FDDA_FAIL	 : fDDAʧ��(�ǽ�)
// Note: qPboc, FDDAʧ�ܺ�, ���Ĳ���ʾ�κ���Ϣ, Ӧ�ò���Ҫ���м��9F6C B1 b6b5, �Ծ����Ǿܾ����ǽ�������
int iHxOfflineDataAuth(void)
{
	int iRet;

	vTraceWriteTxtCr(TRACE_PROC, "iHxOfflineDataAuth()");
	if(gl_iCoreCallFlag != HXCORE_CALLBACK)
		return(HXEMV_CALLBACK_METHOD); // û���Իص���ʽ��ʼ������
	if(gl_iCoreStatus<EMV_STATUS_READ_REC || gl_iCoreStatus>=EMV_STATUS_GAC1)
		return(HXEMV_FLOW_ERROR); // ����¼ǰ��GAC���������ѻ�������֤

	iEmvIOShowMsg(EMVMSG_0E_PLEASE_WAIT); // newadd

	iRet = iEmvOfflineDataAuth(0); // �ص������̶�����0, �����й�Կ֤���Ƿ���CRL�б��ɻص�����ȷ��
	vTraceWriteTxtCr(TRACE_PROC, "(%d)iEmvOfflineDataAuth()", iRet);
    if(iRet != HXEMV_OK) {
		vTraceTlvDb();
    }
	if(iRet == HXEMV_FDDA_FAIL)
		return(HXEMV_FDDA_FAIL); // FDDAʧ��
	if(iRet == HXEMV_TERMINATE) {
		iEmvIODispMsg(EMVMSG_TERMINATE, EMVIO_NEED_NO_CONFIRM);
		return(HXEMV_TERMINATE);
	}
	if(iRet==HXEMV_DENIAL || iRet==HXEMV_DENIAL_ADVICE) {
		iEmvIODispMsg(EMVMSG_07_DECLINED, EMVIO_NEED_NO_CONFIRM);
		return(iRet);
	}
	if(iRet == HXEMV_CARD_OP) {
		if(uiEmvTestCard() == 0) {
			iEmvIODispMsg(EMVMSG_0F_PROCESSING_ERROR, EMVIO_NEED_NO_CONFIRM);
			return(HXEMV_CARD_REMOVED); // ����ȡ��
		} else {
			iEmvIODispMsg(EMVMSG_06_CARD_ERROR, EMVIO_NEED_NO_CONFIRM);
			return(HXEMV_CARD_OP); // ��Ƭ�������󣬽�����ֹ
		}
	}
	if(iRet == HXEMV_CARD_SW) {
		// Refer to EMV2008 book3 6.3.5, P47, ��ʶ���SW��Ҫ��ֹ����
		iEmvIODispMsg(EMVMSG_TERMINATE, EMVIO_NEED_NO_CONFIRM);
		return(HXEMV_CARD_SW);
	}
	if(iRet != HXEMV_OK) {
		iEmvIODispMsg(EMVMSG_NATIVE_ERR, EMVIO_NEED_NO_CONFIRM);
		return(HXEMV_CORE);
	}

	return(HXEMV_OK);
}

// ��������
// ret : HXEMV_OK            : OK
//       HXEMV_TERMINATE     : ����ܾ�������������ֹ
//       HXEMV_FLOW_ERROR    : EMV���̴���
//       HXEMV_CORE          : �ڲ�����
//       HXEMV_DENIAL        : �ն���Ϊ�������Ϊ�ѻ��ܾ�
//       HXEMV_DENIAL_ADVICE : �ն���Ϊ�������Ϊ�ѻ��ܾ�, ��Advice
//       HXEMV_CARD_REMOVED  : ����ȡ��
//       HXEMV_CARD_OP       : �ն���Ϊ�������Ϊ�ѻ��ܾ�, ����AACʱ��Ƭ����
//		 HXEMV_CALLBACK_METHOD : û���Իص���ʽ��ʼ������
int iHxProcRistrictions(void)
{
	int iRet;

	vTraceWriteTxtCr(TRACE_PROC, "iHxProcRistrictions()");
	if(gl_iCoreCallFlag != HXCORE_CALLBACK)
		return(HXEMV_CALLBACK_METHOD); // û���Իص���ʽ��ʼ������
	if(gl_iCoreStatus<EMV_STATUS_READ_REC || gl_iCoreStatus>=EMV_STATUS_GAC1)
		return(HXEMV_FLOW_ERROR); // ����¼ǰ��GAC����������������

	iEmvIOShowMsg(EMVMSG_0E_PLEASE_WAIT); // newadd

	iRet = iEmvProcRistrictions();
	vTraceWriteTxtCr(TRACE_PROC, "(%d)iEmvProcRistrictions()", iRet);
	if(iRet != HXEMV_OK)
		vTraceTlvDb();
	if(iRet == HXEMV_CARD_OP) {
		if(uiEmvTestCard() == 0) {
			iEmvIODispMsg(EMVMSG_0F_PROCESSING_ERROR, EMVIO_NEED_NO_CONFIRM);
			return(HXEMV_CARD_REMOVED); // ����ȡ��
		} else {
			iEmvIODispMsg(EMVMSG_06_CARD_ERROR, EMVIO_NEED_NO_CONFIRM);
			return(HXEMV_CARD_OP); // ��Ƭ�������󣬽�����ֹ
		}
	}
	if(iRet == HXEMV_TERMINATE) {
		iEmvIODispMsg(EMVMSG_TERMINATE, EMVIO_NEED_NO_CONFIRM);
		return(iRet);
	}
	if(iRet==HXEMV_DENIAL || iRet==HXEMV_DENIAL_ADVICE) {
		iEmvIODispMsg(EMVMSG_07_DECLINED, EMVIO_NEED_NO_CONFIRM);
		return(iRet);
	}
	if(iRet != HXEMV_OK) {
		iEmvIODispMsg(EMVMSG_NATIVE_ERR, EMVIO_NEED_NO_CONFIRM);
		return(HXEMV_CORE);
	}
	gl_iCoreStatus = EMV_STATUS_PROC_RISTRICTIONS;
	return(HXEMV_OK);
}

// �ն˷��չ���
// ret : HXEMV_OK            : OK
//       HXEMV_TERMINATE     : ����ܾ�������������ֹ
//       HXEMV_CORE          : ��������
//       HXEMV_CARD_REMOVED  : ����ȡ��
//       HXEMV_CARD_OP       : ��������
//       HXEMV_DENIAL        : �ն���Ϊ�������Ϊ�ѻ��ܾ�
//       HXEMV_DENIAL_ADVICE : �ն���Ϊ�������Ϊ�ѻ��ܾ�, ��Advice
//       HXEMV_CANCEL        : ��ȡ��
//       HXEMV_TIMEOUT       : ��ʱ
//       HXEMV_FLOW_ERROR    : EMV���̴���
//		 HXEMV_CALLBACK_METHOD : û���Իص���ʽ��ʼ������
int iHxTermRiskManage(void)
{
	int iRet;

	vTraceWriteTxtCr(TRACE_PROC, "iHxTermRiskManage()");
	if(gl_iCoreCallFlag != HXCORE_CALLBACK)
		return(HXEMV_CALLBACK_METHOD); // û���Իص���ʽ��ʼ������
	if(gl_iCoreStatus<EMV_STATUS_READ_REC || gl_iCoreStatus>=EMV_STATUS_GAC1)
		return(HXEMV_FLOW_ERROR); // ����¼ǰ��GAC���������ն˷��չ���

	iEmvIOShowMsg(EMVMSG_0E_PLEASE_WAIT); // newadd

	iRet = iEmvTermRiskManage();
	vTraceWriteTxtCr(TRACE_PROC, "(%d)iEmvTermRiskManage()", iRet);
	if(iRet != HXEMV_OK)
		vTraceTlvDb();
	if(iRet == HXEMV_CARD_OP) {
		if(uiEmvTestCard() == 0) {
			iEmvIODispMsg(EMVMSG_0F_PROCESSING_ERROR, EMVIO_NEED_NO_CONFIRM);
			return(HXEMV_CARD_REMOVED); // ����ȡ��
		} else {
			iEmvIODispMsg(EMVMSG_06_CARD_ERROR, EMVIO_NEED_NO_CONFIRM);
			return(HXEMV_CARD_OP); // ��Ƭ�������󣬽�����ֹ
		}
	}
	if(iRet == HXEMV_TERMINATE) {
		iEmvIODispMsg(EMVMSG_TERMINATE, EMVIO_NEED_NO_CONFIRM);
		return(iRet);
	}
	if(iRet==HXEMV_DENIAL || iRet==HXEMV_DENIAL_ADVICE) {
		iEmvIODispMsg(EMVMSG_07_DECLINED, EMVIO_NEED_NO_CONFIRM);
		return(iRet);
	}
	if(iRet == HXEMV_CANCEL) {
		iEmvIODispMsg(EMVMSG_CANCEL, EMVIO_NEED_NO_CONFIRM);
		return(iRet);
	}
	if(iRet == HXEMV_TIMEOUT) {
		iEmvIODispMsg(EMVMSG_TIMEOUT, EMVIO_NEED_NO_CONFIRM);
		return(iRet);
	}
	if(iRet != HXEMV_OK) {
		iEmvIODispMsg(EMVMSG_NATIVE_ERR, EMVIO_NEED_NO_CONFIRM);
		return(HXEMV_CORE);
	}
	iEmvIOShowMsg(EMVMSG_0E_PLEASE_WAIT);
	return(HXEMV_OK);
}

// �ֿ�����֤
// out : piNeedSignFlag      : ����ɹ���ɣ�������Ҫǩ�ֱ�־��0:����Ҫǩ�� 1:��Ҫǩ��
// ret : HXEMV_OK            : OK
//       HXEMV_CANCEL        : �û�ȡ��
//       HXEMV_TIMEOUT       : ��ʱ
//       HXEMV_CARD_REMOVED  : ����ȡ��
//       HXEMV_CARD_OP       : ��������
//       HXEMV_CARD_SW       : �Ƿ���״̬��
//       HXEMV_TERMINATE     : ����ܾ�������������ֹ
//       HXEMV_DENIAL        : �ն���Ϊ�������Ϊ�ѻ��ܾ�
//       HXEMV_DENIAL_ADVICE : �ն���Ϊ�������Ϊ�ѻ��ܾ�, ��Advice
//       HXEMV_FLOW_ERROR    : EMV���̴���
//       HXEMV_CORE          : �ڲ�����
//		 HXEMV_CALLBACK_METHOD : û���Իص���ʽ��ʼ������
int iHxCardHolderVerify(int *piNeedSignFlag)
{
	int   iRet;

	vTraceWriteTxtCr(TRACE_PROC, "iHxCardHolderVerify()");
	if(gl_iCoreCallFlag != HXCORE_CALLBACK)
		return(HXEMV_CALLBACK_METHOD); // û���Իص���ʽ��ʼ������
	if(gl_iCoreStatus<EMV_STATUS_READ_REC || gl_iCoreStatus>=EMV_STATUS_GAC1)
		return(HXEMV_FLOW_ERROR); // ����¼ǰ��GAC���������ֿ�����֤

	iEmvIOShowMsg(EMVMSG_0E_PLEASE_WAIT); // newadd

	iRet = iEmvModuleCardHolderVerify(piNeedSignFlag);
	vTraceWriteTxtCr(TRACE_PROC, "(%d)iEmvModuleCardHolderVerify(%d)", iRet, *piNeedSignFlag);
	if(iRet != HXEMV_OK)
		vTraceTlvDb();
	if(iRet == HXEMV_CORE) {
		iEmvIODispMsg(EMVMSG_NATIVE_ERR, EMVIO_NEED_NO_CONFIRM);
		return(HXEMV_CORE);
	}
	if(iRet == HXEMV_CARD_OP) {
		if(uiEmvTestCard() == 0) {
			iEmvIODispMsg(EMVMSG_0F_PROCESSING_ERROR, EMVIO_NEED_NO_CONFIRM);
			return(HXEMV_CARD_REMOVED); // ����ȡ��
		} else {
			iEmvIODispMsg(EMVMSG_06_CARD_ERROR, EMVIO_NEED_NO_CONFIRM);
			return(HXEMV_CARD_OP); // ��Ƭ�������󣬽�����ֹ
		}
	}
	if(iRet == HXEMV_CARD_SW) {
		// Refer to EMV2008 book3 6.3.5, P47, ��ʶ���SW��Ҫ��ֹ����
		iEmvIODispMsg(EMVMSG_TERMINATE, EMVIO_NEED_NO_CONFIRM);
		return(HXEMV_CARD_SW);
	}
	if(iRet == HXEMV_TERMINATE) {
		iEmvIODispMsg(EMVMSG_TERMINATE, EMVIO_NEED_NO_CONFIRM);
		return(iRet);
	}
	if(iRet==HXEMV_DENIAL || iRet==HXEMV_DENIAL_ADVICE) {
		iEmvIODispMsg(EMVMSG_07_DECLINED, EMVIO_NEED_NO_CONFIRM);
		return(iRet);
	}
	if(iRet == HXEMV_CANCEL) {
		iEmvIODispMsg(EMVMSG_CANCEL, EMVIO_NEED_NO_CONFIRM);
		return(iRet);
	}
	if(iRet == HXEMV_TIMEOUT) {
		iEmvIODispMsg(EMVMSG_TIMEOUT, EMVIO_NEED_NO_CONFIRM);
		return(iRet);
	}
	if(iRet == HXEMV_OK)
		iEmvIOShowMsg(EMVMSG_0E_PLEASE_WAIT);
	return(iRet);
}

// �ն���Ϊ����
// ret : HXEMV_OK		     : OK
//       HXEMV_CORE          : ��������
//       HXEMV_DENIAL        : �ն���Ϊ�������Ϊ�ѻ��ܾ�
//       HXEMV_DENIAL_ADVICE : �ն���Ϊ�������Ϊ�ѻ��ܾ�, ��Advice
//       HXEMV_CARD_REMOVED  : ����ȡ��
//       HXEMV_CARD_OP       : �ն���Ϊ�������Ϊ�ѻ��ܾ�, ����AACʱ��Ƭ����
//		 HXEMV_CALLBACK_METHOD : û���Իص���ʽ��ʼ������
int iHxTermActionAnalysis(void)
{
	int iRet;

	vTraceWriteTxtCr(TRACE_PROC, "iHxTermActionAnalysis()");
	if(gl_iCoreCallFlag != HXCORE_CALLBACK)
		return(HXEMV_CALLBACK_METHOD); // û���Իص���ʽ��ʼ������
	if(gl_iCoreStatus<EMV_STATUS_READ_REC || gl_iCoreStatus>=EMV_STATUS_GAC1)
		return(HXEMV_FLOW_ERROR); // ����¼ǰ��GAC���������ն���Ϊ����

	iEmvIOShowMsg(EMVMSG_0E_PLEASE_WAIT); // newadd

	iRet = iEmvTermActionAnalysis();
	vTraceWriteTxtCr(TRACE_PROC, "(%d)iEmvTermActionAnalysis()", iRet);
	if(iRet != HXEMV_OK)
		vTraceTlvDb();
	if(iRet == HXEMV_TERMINATE) {
		iEmvIODispMsg(EMVMSG_TERMINATE, EMVIO_NEED_NO_CONFIRM);
		return(HXEMV_TERMINATE);
	}
	if(iRet==HXEMV_DENIAL || iRet==HXEMV_DENIAL_ADVICE) {
		iEmvIODispMsg(EMVMSG_07_DECLINED, EMVIO_NEED_NO_CONFIRM);
		return(iRet);
	}
	if(iRet == HXEMV_CARD_OP) {
		if(uiEmvTestCard() == 0) {
			iEmvIODispMsg(EMVMSG_0F_PROCESSING_ERROR, EMVIO_NEED_NO_CONFIRM);
			return(HXEMV_CARD_REMOVED); // ����ȡ��
		} else {
			iEmvIODispMsg(EMVMSG_06_CARD_ERROR, EMVIO_NEED_NO_CONFIRM);
			return(HXEMV_CARD_OP); // ��Ƭ�������󣬽�����ֹ
		}
	}
	if(iRet) {
		iEmvIODispMsg(EMVMSG_NATIVE_ERR, EMVIO_NEED_NO_CONFIRM);
		return(iRet);
	}

	gl_iCoreStatus = EMV_STATUS_TERM_ACTION_ANALYSIS;

	return(HXEMV_OK);
}

// ��һ��GAC
// out : piCardAction      : ��Ƭִ�н��
//                           GAC_ACTION_TC     : ��׼(����TC)
//							 GAC_ACTION_AAC    : �ܾ�(����AAC)
//                           GAC_ACTION_AAC_ADVICE : �ܾ�(����AAC,��Advice)
//                           GAC_ACTION_ARQC   : Ҫ������(����ARQC)
// ret : HXEMV_OK          : OK
//       HXEMV_CORE        : ��������
//       HXEMV_CARD_REMOVED: ����ȡ��
//       HXEMV_CARD_OP     : ��������
//       HXEMV_CARD_SW     : �Ƿ�״̬��
//       HXEMV_TERMINATE   : ����ܾ�������������ֹ
//       HXEMV_NOT_ACCEPTED: ������
//       HXEMV_NOT_ALLOWED : ��������
//       HXEMV_FLOW_ERROR  : EMV���̴���
//		 HXEMV_CALLBACK_METHOD : û���Իص���ʽ��ʼ������
// Note: ĳЩ�����, �ú������Զ�ִ�еڶ���Gac
//       ��:CDAʧ�ܡ����ѻ��ն�GAC1��Ƭ����ARQC
int iHx1stGAC(int *piCardAction)
{
	int iRet;

	vTraceWriteTxtCr(TRACE_PROC, "iHx1stGAC()");
	if(gl_iCoreCallFlag != HXCORE_CALLBACK)
		return(HXEMV_CALLBACK_METHOD); // û���Իص���ʽ��ʼ������
	if(gl_iCoreStatus != EMV_STATUS_TERM_ACTION_ANALYSIS)
		return(HXEMV_FLOW_ERROR); // �ն���Ϊ������ſ�����GAC1

	iEmvIOShowMsg(EMVMSG_0E_PLEASE_WAIT); // newadd

	iRet = iEmvGAC1(piCardAction);
	vTraceWriteTxtCr(TRACE_PROC, "(%d)iEmvGAC1(%d)", iRet, *piCardAction);
	if(iRet!=HXEMV_OK || *piCardAction!=GAC_ACTION_ARQC) {
		// �����Ƭ���ط�ARQC(�������ARQC, ��GAC2��trace), TRACE
		vTraceTlvDb();
	}
	if(iRet == HXEMV_TERMINATE) {
		iEmvIODispMsg(EMVMSG_TERMINATE, EMVIO_NEED_NO_CONFIRM);
		return(HXEMV_TERMINATE);
	}
	if(iRet == HXEMV_NOT_ALLOWED) {
		iEmvIODispMsg(EMVMSG_SERVICE_NOT_ALLOWED, EMVIO_NEED_NO_CONFIRM);
		return(HXEMV_NOT_ALLOWED);
	}
	if(iRet == HXEMV_NOT_ACCEPTED) {
		iEmvIODispMsg(EMVMSG_0C_NOT_ACCEPTED, EMVIO_NEED_NO_CONFIRM);
		return(HXEMV_NOT_ACCEPTED);
	}
	if(iRet == HXEMV_CARD_OP) {
		if(uiEmvTestCard() == 0) {
			iEmvIODispMsg(EMVMSG_0F_PROCESSING_ERROR, EMVIO_NEED_NO_CONFIRM);
			return(HXEMV_CARD_REMOVED); // ����ȡ��
		} else {
			iEmvIODispMsg(EMVMSG_06_CARD_ERROR, EMVIO_NEED_NO_CONFIRM);
			return(HXEMV_CARD_OP); // ��Ƭ�������󣬽�����ֹ
		}
	}
	if(iRet == HXEMV_CARD_SW) {
		// Refer to EMV2008 book3 6.3.5, P47, ��ʶ���SW��Ҫ��ֹ����
		iEmvIODispMsg(EMVMSG_TERMINATE, EMVIO_NEED_NO_CONFIRM);
		return(HXEMV_CARD_SW);
	}
	if(iRet) {
		iEmvIODispMsg(EMVMSG_NATIVE_ERR, EMVIO_NEED_NO_CONFIRM);
		return(HXEMV_CORE);
	}

	gl_iCoreStatus = EMV_STATUS_GAC1;

	// GAC1�ɹ�ִ��
	if(*piCardAction == GAC_ACTION_TC) {
		// ��Ƭ�ѻ������˽���
		iEmvIODispMsg(EMVMSG_03_APPROVED, EMVIO_NEED_NO_CONFIRM);
		return(HXEMV_OK);
	}
	if(*piCardAction==GAC_ACTION_AAC || *piCardAction==GAC_ACTION_AAC_ADVICE) {
		// ��Ƭ�ܾ��˽���
		iEmvIODispMsg(EMVMSG_07_DECLINED, EMVIO_NEED_NO_CONFIRM);
		return(HXEMV_OK);
	}

	return(HXEMV_OK); // ��Ƭ��������
}

// �ڶ���GAC
// in  : pszArc            : Ӧ����[2], NULL��""��ʾ����ʧ��
//       pszAuthCode	   : ��Ȩ��[6], NULL��""��ʾ����Ȩ������
//       psOnlineData      : ������Ӧ����(55������),һ��TLV��ʽ����, NULL��ʾ��������Ӧ����
//       psOnlineDataLen   : ������Ӧ���ݳ���
// out : piCardAction      : ��Ƭִ�н��
//                           GAC_ACTION_TC     : ��׼(����TC)
//							 GAC_ACTION_AAC    : �ܾ�(����AAC)
//                           GAC_ACTION_AAC_ADVICE : �ܾ�(����AAC,��Advice)
// ret : HXEMV_OK          : OK
//       HXEMV_CORE        : ��������
//       HXEMV_CARD_REMOVED: ����ȡ��
//       HXEMV_CARD_OP     : ��������
//       HXEMV_CARD_SW     : �Ƿ���״̬��
//       HXEMV_TERMINATE   : ����ܾ�������������ֹ
//       HXEMV_NOT_ALLOWED : ��������
//       HXEMV_NOT_ACCEPTED: ������
//       HXEMV_FLOW_ERROR  : EMV���̴���
//		 HXEMV_CALLBACK_METHOD : û���Իص���ʽ��ʼ������
int iHx2ndGAC(uchar *pszArc, uchar *pszAuthCode, uchar *psIssuerData, int iIssuerDataLen, int *piCardAction)
{
	int iRet;

	if(pszArc)
		vTraceWriteTxtCr(TRACE_PROC, "iHx2ndGAC(%s)", pszArc);
	else
		vTraceWriteTxtCr(TRACE_PROC, "iHx2ndGAC(NULL)");
	if(gl_iCoreCallFlag != HXCORE_CALLBACK)
		return(HXEMV_CALLBACK_METHOD); // û���Իص���ʽ��ʼ������
	if(gl_iCoreStatus != EMV_STATUS_GAC1)
		return(HXEMV_FLOW_ERROR); // GAC1��ſ�����GAC2

	iEmvIOShowMsg(EMVMSG_0E_PLEASE_WAIT); // newadd

	iRet = iEmvGAC2(pszArc, pszAuthCode, psIssuerData, iIssuerDataLen, piCardAction);
	vTraceWriteTxtCr(TRACE_PROC, "(%d)iEmvGAC2(%d)", iRet, *piCardAction);
	vTraceTlvDb();
	if(iRet == HXEMV_TERMINATE) {
		iEmvIODispMsg(EMVMSG_TERMINATE, EMVIO_NEED_NO_CONFIRM);
		return(HXEMV_TERMINATE);
	}
	if(iRet == HXEMV_NOT_ALLOWED) {
		iEmvIODispMsg(EMVMSG_SERVICE_NOT_ALLOWED, EMVIO_NEED_NO_CONFIRM);
		return(HXEMV_NOT_ALLOWED);
	}
	if(iRet == HXEMV_NOT_ACCEPTED) {
		iEmvIODispMsg(EMVMSG_0C_NOT_ACCEPTED, EMVIO_NEED_NO_CONFIRM);
		return(HXEMV_NOT_ACCEPTED);
	}
	if(iRet == HXEMV_CARD_OP) {
		if(uiEmvTestCard() == 0) {
			iEmvIODispMsg(EMVMSG_0F_PROCESSING_ERROR, EMVIO_NEED_NO_CONFIRM);
			return(HXEMV_CARD_REMOVED); // ����ȡ��
		} else {
			iEmvIODispMsg(EMVMSG_06_CARD_ERROR, EMVIO_NEED_NO_CONFIRM);
			return(HXEMV_CARD_OP); // ��Ƭ�������󣬽�����ֹ
		}
	}
	if(iRet == HXEMV_CARD_SW) {
		// Refer to EMV2008 book3 6.3.5, P47, ��ʶ���SW��Ҫ��ֹ����
		iEmvIODispMsg(EMVMSG_TERMINATE, EMVIO_NEED_NO_CONFIRM);
		return(HXEMV_CARD_SW);
	}
	if(iRet) {
		iEmvIODispMsg(EMVMSG_NATIVE_ERR, EMVIO_NEED_NO_CONFIRM);
		return(HXEMV_CORE);
	}

	gl_iCoreStatus = EMV_STATUS_GAC2;

	// GAC2�ɹ�ִ��
	if(*piCardAction == GAC_ACTION_TC) {
		// ��Ƭ�ѻ������˽���
		iEmvIODispMsg(EMVMSG_03_APPROVED, EMVIO_NEED_NO_CONFIRM);
		return(HXEMV_OK);
	}
	// ��Ƭ�ܾ��˽���
	iEmvIODispMsg(EMVMSG_07_DECLINED, EMVIO_NEED_NO_CONFIRM);
	return(HXEMV_OK);
}

//// ����Ϊ�ص���ʽAPI
//// �ص���ʽAPI  *** ���� ***

//// ����Ϊ�ǻص���ʽAPI
//// �ǻص���ʽAPI  *** ��ʼ ***

// ��λ��Ƭ
// out : piErrMsgType      : ������Ϣ������
// ret : HXEMV_OK          : OK
//       HXEMV_FLOW_ERROR  : EMV���̴���
//       HXEMV_CARD_REMOVED: ����ȡ��
//       HXEMV_CARD_OP     : ��Ƭ��λ����
//		 HXEMV_CALLBACK_METHOD : û���Էǻص���ʽ��ʼ������
// Note: emv�淶Ҫ��忨��Ҫ��ʾ��ȴ���Ϣ, Ӧ�ò���ڵ��ñ�����ǰ������ʾEMVMSG_0E_PLEASE_WAIT,
int iHxResetCard2(int *piErrMsgType)
{
	uint uiRet;

	vTraceWriteTxtCr(TRACE_PROC, "iHxResetCard2()");
	*piErrMsgType = EMVMSG_APPCALL_ERR; // ��ȱʡֵ
	if(gl_iCoreCallFlag != HXCORE_NOT_CALLBACK)
		return(HXEMV_CALLBACK_METHOD); // û���Էǻص���ʽ��ʼ������
	if(gl_iCoreStatus != EMV_STATUS_TRANS_INIT)
		return(HXEMV_FLOW_ERROR); // ���׳�ʼ����ſ��Ը�λ��Ƭ

	uiRet = uiEmvCmdResetCard();
	if(uiRet != 0) {
		if(uiEmvTestCard() == 0) {
			*piErrMsgType = EMVMSG_0F_PROCESSING_ERROR;
			return(HXEMV_CARD_REMOVED); // ����ȡ��
		} else {
			*piErrMsgType = EMVMSG_06_CARD_ERROR;
			if(iEmvTestItemBit(EMV_ITEM_TERM_CAP, TERM_CAP_01_MAG_STRIPE)) {
				// ���ڴſ�������
				*piErrMsgType = EMVMSG_12_USE_MAG_STRIPE;
			}
			return(HXEMV_CARD_OP); // ��Ƭ�������󣬽�����ֹ
		}
	}

	gl_iCoreStatus = EMV_STATUS_RESET;
	*piErrMsgType = EMVMSG_00_NULL;
	return(HXEMV_OK);
}

// ��ȡ֧�ֵ�Ӧ��
// in  : iIgnoreBlock	   : !0:����Ӧ��������־, 0:������Ӧ�ò��ᱻѡ��
//       piAdfNum          : �����ɵ��ն��뿨Ƭͬʱ֧�ֵ�Adf����
// out : paAdfInfo         : �ն��뿨Ƭͬʱ֧�ֵ�Adf�б�(�����ȼ�����)
//       piAdfNum          : �ն��뿨Ƭͬʱ֧�ֵ�Adf����
//       piMsgType         : ��������HXEMV_OKʱ, ���� [Ӧ��ѡ����ʾ] ��Ϣ������
//							 ��������!HXEMV_OKʱ, �������, ��Ϊ���Ӵ�����Ϣ
//       piErrMsgType      : ������Ϣ������, ��������!HXEMV_OK����!HXEMV_AUTO_SELECTʱ����
// ret : HXEMV_OK          : OK, ��ȡ��Ӧ�ñ���ȷ��
//       HXEMV_AUTO_SELECT : OK, ��ȡ��Ӧ�ÿ��Զ�ѡ��
//       HXEMV_NO_APP      : ��֧�ֵ�Ӧ��
//       HXEMV_CARD_REMOVED: ����ȡ��
//       HXEMV_CARD_SW     : ��״̬�ַǷ�
//       HXEMV_CARD_OP     : ��������
//       HXEMV_TERMINATE   : ����ܾ�������������ֹ
//       HXEMV_FLOW_ERROR  : EMV���̴���
//       HXEMV_LACK_MEMORY : �洢�ռ䲻��
//		 HXEMV_CALLBACK_METHOD : û���Էǻص���ʽ��ʼ������
// �ǽӿ��ܷ�����
//		 HXEMV_TRY_OTHER_INTERFACE	: ��������ͨ�Ž���
//		 HXEMV_CARD_TRY_AGAIN		: Ҫ�������ύ��Ƭ(�ǽ�)
int iHxGetSupportedApp2(int iIgnoreBlock, stADFInfo *paAdfInfo, int *piAdfNum, int *piMsgType, int *piErrMsgType)
{
	int   iRet;
	uchar szFinalLanguage[3];
	uchar *psValue, szPinpadLanguage[21];
	uchar *p, ucAppConfirmSupport;        // TDFxx, 1:֧��Ӧ��ȷ�� 0:��֧��Ӧ��ȷ��(TAG_DFXX_AppConfirmSupport)
	int   iFirstCallFlag;                 // ��ǰ���׵�һ�α�������־, 0:���ǵ�һ�� 1:�ǵ�һ��

	vTraceWriteTxtCr(TRACE_PROC, "iHxGetSupportedApp2(%d,%d)", iIgnoreBlock, *piAdfNum);
	*piMsgType = EMVMSG_00_NULL;
	*piErrMsgType = EMVMSG_APPCALL_ERR; // ��ȱʡֵ
	if(gl_iCoreCallFlag != HXCORE_NOT_CALLBACK)
		return(HXEMV_CALLBACK_METHOD); // û���Էǻص���ʽ��ʼ������
	if(gl_iCoreStatus<EMV_STATUS_RESET || gl_iCoreStatus>=EMV_STATUS_GPO)
		return(HXEMV_FLOW_ERROR); // ��Ƭ��λ��GPO�ɹ�֮ǰ���Ի�ȡ֧�ֵ�Ӧ��

	iFirstCallFlag = 0; // �ȼ��費�ǵ�һ�ε���
	if(sg_iAidGetFlag == 0) {
	    // ��ʾӦ���б���δ��ȡ, ������֧ͬ�ֵ�aid�б�
		iFirstCallFlag = 1; // ȷ���ǵ�һ�ε���
		iRet = iSelGetAids(iIgnoreBlock);
		vTraceWriteTxtCr(TRACE_PROC, "(%d)iSelGetAids(%d)", iRet, iIgnoreBlock);

		if(iRet == SEL_ERR_CARD) {
			if(gl_iCardType == EMV_CONTACTLESS_CARD) {
				// �ǽӿ�, ��ʧЧ����, �ο�JR/T0025.12��2013ͼ4, P11
				*piErrMsgType = EMVMSG_13_TRY_AGAIN;
				return(HXEMV_CARD_TRY_AGAIN);
			}
			if(uiEmvTestCard() == 0) {
				*piErrMsgType = EMVMSG_0F_PROCESSING_ERROR;
				return(HXEMV_CARD_REMOVED); // ����ȡ��
			} else {
				*piErrMsgType = EMVMSG_06_CARD_ERROR;
				return(HXEMV_CARD_OP); // ��Ƭ�������󣬽�����ֹ
			}
		}
		if(iRet == SEL_ERR_CARD_SW) {
			if(gl_iCardType == EMV_CONTACTLESS_CARD) {
				// �ǽӿ�, �ο�JR/T0025.12��2013ͼ4, P11
				*piErrMsgType = EMVMSG_TERMINATE;
				return(HXEMV_TRY_OTHER_INTERFACE);
			}
			// Refer to EMV2008 book3 6.3.5, P47, ��ʶ���SW��Ҫ��ֹ����
			*piErrMsgType = EMVMSG_TERMINATE;
			return(HXEMV_CARD_SW); // ��״̬�ַǷ���������ֹ
		}

		if(iRet == SEL_ERR_CARD_BLOCKED) {
			*piErrMsgType = EMVMSG_TERMINATE;
			return(HXEMV_TERMINATE); // ��Ƭ������������ֹ
		}
		if(iRet == SEL_ERR_DATA_ERROR) {
			*piErrMsgType = EMVMSG_TERMINATE;
			return(HXEMV_TERMINATE); // Ӧ��������ݷǷ���������ֹ
		}
		if(iRet == SEL_ERR_DATA_ABSENT) {
			*piErrMsgType = EMVMSG_TERMINATE;
			return(HXEMV_TERMINATE); // ��Ҫ���ݲ����ڣ�������ֹ
		}
		if(iRet < 0) {
			*piErrMsgType = EMVMSG_NATIVE_ERR;
			return(HXEMV_CORE); // �ڴ治�㣬��Ϊ�ڲ����󷵻�
		}
		if(iRet == 0) {
			// refer to emv2008 book4 11.3, P91 wlfxy
			*piErrMsgType = EMVMSG_0C_NOT_ACCEPTED;
			*piMsgType = EMVMSG_TERMINATE;
			return(HXEMV_NO_APP); // ��֧�ֵ�Ӧ��
		}
		sg_iAidGetFlag = 1; // AID�б��ȡ���
	}
	if(gl_iCardAppNum == 0) {
		*piErrMsgType = EMVMSG_TERMINATE; // ��֧�ֵ�Ӧ��
		return(HXEMV_NO_APP);
	}
	if(*piAdfNum < gl_iCardAppNum) {
		*piErrMsgType = EMVMSG_NATIVE_ERR;
		return(HXEMV_LACK_MEMORY);
	}
	*piAdfNum = gl_iCardAppNum;  // ������ص�AID�������ڻ�������С, ������ȷ�ĸ���, ���򷵻ػ����������������

	// ��ȡ�ն�Ӧ��ȷ��֧�����
	iRet = iTlvGetObjValue(gl_sTlvDbTermFixed, TAG_DFXX_AppConfirmSupport, &p); // ��ȡ�ն�Ӧ��ѡ��֧�����
	if(iRet <= 0)
		ucAppConfirmSupport = 1; // �����ȡ���ɹ�, �����ն�֧��ȷ��
	else
		ucAppConfirmSupport = *p;

	// ��ȡ����ƥ�������
	iRet = iEmvMsgTableGetFitLanguage(gl_aCardAppList[0].szLanguage, szFinalLanguage);
	// ʹ��ƥ�������, Ҳ����ǿ��ʹ����������
    iEmvMsgTableSetLanguage(szFinalLanguage);
	// ��ƥ������Դ������TLV���ݿ�(TAG_DFXX_LanguageUsed)
	iRet = iTlvMakeAddObj(gl_sTlvDbTermVar, TAG_DFXX_LanguageUsed, 2, szFinalLanguage, TLV_CONFLICT_REPLACE);
	if(iRet < 0) {
		EMV_TRACE_ERR_MSG
		*piErrMsgType = EMVMSG_NATIVE_ERR;
		return(HXEMV_CORE);
	}

	// ��ȡPinpadʹ�õ�����
	iRet = iTlvGetObjValue(gl_sTlvDbTermFixed, TAG_DFXX_PinpadLanguage, &psValue);
	if(iRet < 0) {
		EMV_TRACE_ERR_MSG
		*piErrMsgType = EMVMSG_NATIVE_ERR;
		return(HXEMV_CORE);
	}
	memcpy(szPinpadLanguage, psValue, iRet);
	szPinpadLanguage[iRet] = 0;
	iRet = iEmvMsgTableGetPinpadLanguage(gl_aCardAppList[0].szLanguage, szPinpadLanguage, szFinalLanguage);
	// ��ƥ������Դ������TLV���ݿ�(TAG_DFXX_LanguageUsed)
	iRet = iTlvMakeAddObj(gl_sTlvDbTermVar, TAG_DFXX_LanguageUsed, 2, szFinalLanguage, TLV_CONFLICT_REPLACE);
	if(iRet < 0) {
		EMV_TRACE_ERR_MSG
		*piErrMsgType = EMVMSG_NATIVE_ERR;
		return(HXEMV_CORE);
	}

	memcpy(&paAdfInfo[0], &gl_aCardAppList[0], sizeof(stADFInfo)*(*piAdfNum));
	gl_iCoreStatus = EMV_STATUS_GET_SUPPORTED_APP;
	*piMsgType = EMVMSG_SELECT_APP;
	*piErrMsgType = EMVMSG_00_NULL;

	if(ucAppConfirmSupport==0 || gl_iCardType==EMV_CONTACTLESS_CARD) // 0:�ն˲�֧��Ӧ��ȷ�� �� ��ƬΪ�ǽӿ�
		return(HXEMV_AUTO_SELECT); // ��֧��Ӧ��ȷ��, ���ؿ��Զ�ѡ��

	// �ն�֧��Ӧ��ȷ��
	if(iFirstCallFlag && *piAdfNum==1) {
		// ��һ�ε��û�ȡӦ�� && ֻ��һ����ѡӦ��
		if(paAdfInfo[0].iPriority>=0 && (paAdfInfo[0].iPriority&0x80) == 0)
			return(HXEMV_AUTO_SELECT); // ��Ӧ�ò���Ҫȷ�ϣ����ؿ��Զ�ѡ��
	}

	return(HXEMV_OK); // ��Ҫȷ��Ӧ��
}

// Ӧ��ѡ��
// in  : iIgnoreBlock	   : !0:����Ӧ��������־, 0:������Ӧ�ò��ᱻѡ��
//       iAidLen             : AID����
//       psAid               : AID
// out : piErrMsgType        : ������Ϣ������
// ret : HXEMV_OK            : OK
//       HXEMV_CARD_REMOVED  : ����ȡ��
//       HXEMV_CARD_OP       : ��������
//       HXEMV_TERMINATE     : ����ܾ�������������ֹ
//       HXEMV_FLOW_ERROR    : EMV���̴���
//       HXEMV_CORE          : �ڲ�����
//       HXEMV_RESELECT      : ��Ҫ����ѡ��Ӧ��
//       HXEMV_NOT_SUPPORTED : ѡ���˲�֧�ֵ�Ӧ��
//		 HXEMV_CALLBACK_METHOD : û���Էǻص���ʽ��ʼ������
// �ǽӿ��ܷ�����
//		 HXEMV_TRY_OTHER_INTERFACE	: ��������ͨ�Ž���
//		 HXEMV_CARD_TRY_AGAIN		: Ҫ�������ύ��Ƭ(�ǽ�)
// Note: ��ͨ����ȡ9F7A�ж��Ƿ�֧��eCash
int iHxAppSelect2(int iIgnoreBlock, int iAidLen, uchar *psAid, int *piErrMsgType)
{
	uchar *psPdol;
	int   iPdolLen;
	int   i;
	int   iRet;
	uchar *pucAppConfirmSupport;          // TDFxx, 1:֧��Ӧ��ȷ�� 0:��֧��Ӧ��ȷ��(TAG_DFXX_AppConfirmSupport)
	
	{
		uchar szAid[33];
		vOneTwo0(psAid, iAidLen, szAid);
		vTraceWriteTxtCr(TRACE_PROC, "iHxAppSelect2(%d,%s)", iAidLen, szAid);
	}

	*piErrMsgType = EMVMSG_APPCALL_ERR; // ��ȱʡֵ
	if(gl_iCoreCallFlag != HXCORE_NOT_CALLBACK)
		return(HXEMV_CALLBACK_METHOD); // û���Էǻص���ʽ��ʼ������
	if(gl_iCoreStatus<EMV_STATUS_GET_SUPPORTED_APP || gl_iCoreStatus>=EMV_STATUS_GPO)
		return(HXEMV_FLOW_ERROR); // ��ȡ��֧�ֵ�Ӧ���б�֮��GPO�ɹ�֮ǰ����ѡ��Ӧ��

	// ��ȡ�ն�Ӧ��ѡ��֧�����
	iRet = iTlvGetObjValue(gl_sTlvDbTermFixed, TAG_DFXX_AppConfirmSupport, &pucAppConfirmSupport);
	if(iRet <= 0) {
		EMV_TRACE_ERR_MSG
		return(SEL_ERR_OTHER);
	}

	// ���ѡ���Ӧ���Ƿ���֧�ֵ�Ӧ�÷�Χ֮��
	for(i=0; i<gl_iCardAppNum; i++) {
		if(iAidLen == gl_aCardAppList[i].ucAdfNameLen) {
			if(memcmp(psAid, gl_aCardAppList[i].sAdfName, iAidLen) == 0)
				break;
		}
	}
	if(i >= gl_iCardAppNum) {
		EMV_TRACE_ERR_MSG
		*piErrMsgType = EMVMSG_APPCALL_ERR;
		return(HXEMV_NOT_SUPPORTED); // Ӧ�ò����, ѡ���˲�֧�ֵ�Ӧ��
	}

	iRet = iSelFinalSelect2(iIgnoreBlock, iAidLen, psAid);
	vTraceWriteTxtCr(TRACE_PROC, "(%d)iSelFinalSelect2()", iRet);

	if(iRet == SEL_ERR_CARD) {
		if(gl_iCardType == EMV_CONTACTLESS_CARD) {
			// �ǽӿ�, ��ʧЧ����, �ο�JR/T0025.12��2013ͼ4, P11
			*piErrMsgType = EMVMSG_13_TRY_AGAIN;
			return(HXEMV_CARD_TRY_AGAIN);
		}
		if(uiEmvTestCard() == 0) {
			*piErrMsgType = EMVMSG_0F_PROCESSING_ERROR;
			return(HXEMV_CARD_REMOVED); // ����ȡ��
		} else {
			*piErrMsgType = EMVMSG_06_CARD_ERROR;
			return(HXEMV_CARD_OP); // ��Ƭ�������󣬽�����ֹ
		}
	}
	if(iRet == SEL_ERR_NO_APP) {
		if(gl_iCardType == EMV_CONTACTLESS_CARD) {
			// �ǽӿ�, �ο�JR/T0025.12��2013ͼ4, P11
			*piErrMsgType = EMVMSG_TERMINATE;
			return(HXEMV_TRY_OTHER_INTERFACE);
		}
		*piErrMsgType = EMVMSG_APPCALL_ERR;
		return(HXEMV_NOT_SUPPORTED); // Ӧ�ò����, ѡ���˲�֧�ֵ�Ӧ��
	}
	if(iRet == SEL_ERR_NEED_RESELECT) {
		if(gl_iCardType == EMV_CONTACTLESS_CARD) {
			// �ǽӿ�, �ο�JR/T0025.12��2013 6.4.1, P12
			// ���ڷǽӿ�, Ҫ��SEL_ERR_NEED_RESELECT����Ҫ��ֹ����, ��������������
			*piErrMsgType = EMVMSG_TERMINATE;
			return(HXEMV_TRY_OTHER_INTERFACE);
		}
		if(gl_iCardAppNum > 0 ) {
			if(*pucAppConfirmSupport)
				*piErrMsgType = EMVMSG_13_TRY_AGAIN; // �ն�֧��Ӧ��ȷ��ʱ����Ҫ��ʾ"Try Again"
			else
				*piErrMsgType = EMVMSG_00_NULL;
			return(HXEMV_RESELECT); // ѡ�е�Ӧ��������, ��Ҫ����ѡ��
		} else {
			*piErrMsgType = EMVMSG_TERMINATE;
			return(HXEMV_TERMINATE); // ѡ�е�Ӧ��������, ���޿�ѡӦ����
		}
	}
	if(iRet) {
		*piErrMsgType = EMVMSG_NATIVE_ERR;
		return(HXEMV_CORE);
	}

	if(gl_iCardType == EMV_CONTACTLESS_CARD) {
		// �ǽӿ�
		iPdolLen = iTlvGetObjValue(gl_sTlvDbCard, TAG_9F38_PDOL, &psPdol);
		if(iPdolLen <= 0) {
			// �ǽӿ���PDOL, �ο�JR/T0025.12��2013ͼ4, P11
			*piErrMsgType = EMVMSG_TERMINATE;
			return(HXEMV_TRY_OTHER_INTERFACE);
		}
		iRet = iTlvSearchDOLTag(psPdol, iPdolLen, TAG_9F66_ContactlessSupport, NULL);
		if(iRet <= 0) {
			// �ǽӿ�PDOL��9F66, �ο�JR/T0025.12��2013ͼ4, P11
			*piErrMsgType = EMVMSG_TERMINATE;
			return(HXEMV_TRY_OTHER_INTERFACE);
		}
	}

	*piErrMsgType = EMVMSG_00_NULL;
	gl_iCoreStatus = EMV_STATUS_APP_SELECT;
	return(HXEMV_OK);
}

// ��ȡ֧�ֵ������Թ��ֿ���ѡ��
// out : pszLangs     : ֧�ֵ������б�, �������ַ�������Ϊ0, ��ʾ���سֿ�������ѡ��
//       pszLangsDesc : '='�Ž�β��֧�����������б�
//       piMsgType    : ��������HXEMV_OK����pszLangs������Ҫ�ֿ���ѡ��ʱ��ʾѡ��������ʾ��Ϣ������
//       piErrMsgType : ������Ϣ������
// ret : HXEMV_OK     : OK
//       HXEMV_FLOW_ERROR  : EMV���̴���
//		 HXEMV_CALLBACK_METHOD : û���Էǻص���ʽ��ʼ������
//       HXEMV_CORE        : �ڲ�����
int iHxGetSupportedLang2(uchar *pszLangs, uchar *pszLangsDesc, int *piMsgType, int *piErrMsgType)
{
	int   iRet;
	uchar szFinalLanguage[3];
	uchar *psValue;
	uchar szCardPreferredLang[9];

	vTraceWriteTxtCr(TRACE_PROC, "iHxGetSupportedLang2()");
	*piMsgType = EMVMSG_00_NULL;
	*piErrMsgType = EMVMSG_APPCALL_ERR; // ��ȱʡֵ
	if(gl_iCoreCallFlag != HXCORE_NOT_CALLBACK)
		return(HXEMV_CALLBACK_METHOD); // û���Էǻص���ʽ��ʼ������
	if(gl_iCoreStatus<EMV_STATUS_APP_SELECT || gl_iCoreStatus>=EMV_STATUS_GPO)
		return(HXEMV_FLOW_ERROR); // Ӧ��ѡ��֮��GPO֮ǰ�������óֿ�������

	if(sg_szSelectedLang[0]) {
		// �ֿ����Ѿ�ѡ�������
		iHxSetCardHolderLang2(sg_szSelectedLang, piErrMsgType);
		*piErrMsgType = EMVMSG_00_NULL;
		pszLangs[0] = 0; // ������ѡ��
		return(HXEMV_OK);
	}

	// ��ȡ��ƬӦ��֧�ֵ�����
	szCardPreferredLang[0] = 0;
	iRet = iTlvGetObjValue(gl_sTlvDbCard, TAG_5F2D_LanguagePrefer, &psValue);
	if(iRet>=0 && iRet<=8 && iRet%2==0)
		vMemcpy0(szCardPreferredLang, psValue, iRet);
	// ��ȡ�ն�֧�ֵ�����
	iRet = iEmvIOSelectLang(NULL, pszLangs, pszLangsDesc);
	if(iRet != HXEMV_OK) {
		*piErrMsgType = EMVMSG_NATIVE_ERR;
		return(HXEMV_CORE);
	}
	// ����Ƿ���ƥ��
	iRet = iEmvMsgTableGetPinpadLanguage(szCardPreferredLang, pszLangs, szFinalLanguage);
	if(iRet==0 || strlen(pszLangs)<=2) {
		// ��ƥ�� or �ն˲�֧�ֶ�����, ����Ҫ�ֿ���ѡ��
		pszLangs[0] = 0;
		iHxSetCardHolderLang2(szFinalLanguage, piErrMsgType);
		*piErrMsgType = EMVMSG_00_NULL;
		return(HXEMV_OK);
	}

	// ��ƥ��, ��Ҫ�ֿ���ѡ��
	*piErrMsgType = EMVMSG_00_NULL;
	*piMsgType = EMVMSG_SELECT_LANG;
	return(HXEMV_OK);
}

// ���óֿ�������
// in  : pszLanguage  : �ֿ������Դ���, ����:"zh"
// out : piErrMsgType : ������Ϣ������
// ret : HXEMV_OK     : OK
//       HXEMV_FLOW_ERROR  : EMV���̴���
//		 HXEMV_CALLBACK_METHOD : û���Էǻص���ʽ��ʼ������
//       HXEMV_PARA   : ��������
//       HXEMV_CORE   : �ڲ�����
// Note: �����֧�ִ��������, ����ʹ�ñ�������
int iHxSetCardHolderLang2(uchar *pszLanguage, int *piErrMsgType)
{
	int   iRet;
	int   i;
	uchar *psValue;

	vTraceWriteTxtCr(TRACE_PROC, "iHxSetCardHolderLang2(%s)", pszLanguage);
	*piErrMsgType = EMVMSG_APPCALL_ERR; // ��ȱʡֵ
	if(gl_iCoreCallFlag != HXCORE_NOT_CALLBACK)
		return(HXEMV_CALLBACK_METHOD); // û���Էǻص���ʽ��ʼ������
	if(gl_iCoreStatus<EMV_STATUS_GET_SUPPORTED_APP || gl_iCoreStatus>EMV_STATUS_APP_SELECT)
		return(HXEMV_FLOW_ERROR); // ��ȡ��֧�ֵ�Ӧ���б�֮��Ӧ��ѡ��֮ǰ�������óֿ�������

	// �жϲ����Ϸ���
	if(strlen(pszLanguage) != 2) {
		*piErrMsgType = EMVMSG_APPCALL_ERR;
		return(HXEMV_PARA);
	}
	iRet = iTlvGetObjValue(gl_sTlvDbTermFixed, TAG_DFXX_PinpadLanguage, &psValue);
	if(iRet < 0) {
		*piErrMsgType = EMVMSG_NATIVE_ERR;
		return(HXEMV_CORE);
	}
	for(i=0; i<iRet; i+=2) {
		if(memcmp(pszLanguage, psValue+i, 2) == 0)
			break;
	}
	if(i >= iRet) {
		*piErrMsgType = EMVMSG_APPCALL_ERR;
		return(HXEMV_PARA); // ��������Բ��ǿ��õ�����
	}

	strcpy(sg_szSelectedLang, pszLanguage); // ����
	// ��������
	iRet = iTlvMakeAddObj(gl_sTlvDbTermVar, TAG_DFXX_LanguageUsed, 2, pszLanguage, TLV_CONFLICT_REPLACE);
	if(iRet < 0) {
		*piErrMsgType = EMVMSG_NATIVE_ERR;
		return(HXEMV_CORE);
	}

	// �ֿ���ѡ���pinpad����Ҳ����Ϊ�ն�ƥ������
	iRet = iTlvMakeAddObj(gl_sTlvDbTermVar, TAG_DFXX_LanguageUsed, 2, pszLanguage, TLV_CONFLICT_REPLACE);
	if(iRet < 0) {
		*piErrMsgType = EMVMSG_NATIVE_ERR;
		return(HXEMV_CORE);
	}
	*piErrMsgType = EMVMSG_00_NULL;
	return(HXEMV_OK);
}

// GPO
// in  : pszTransTime      : ����ʱ�䣬YYYYMMDDhhmmss
//       ulTTC             : �ն˽������, 1-999999
//       uiTransType       : ��������, BCD��ʽ(0xAABB, BBΪ��GB/T 15150 ����Ĵ�����ǰ2 λ��ʾ�Ľ��ڽ�������, AA����������Ʒ/����)
//       pszAmount         : ���׽��, ascii�봮, ��ҪС����, Ĭ��ΪȱʡС����λ��, ����:RMB1.00��ʾΪ"100"
//       uiCurrencyCode    : ���Ҵ���
// out : piMsgType         : ��Ϣ����, ������ʾ��Ϣ, ��������HXEMV_RESELECTʱ���Ӵ���ʾ��Ϣ
//       piErrMsgType      : ������Ϣ������, ����Ϣ��piMsgType����
// ret : HXEMV_OK          : OK
//       HXEMV_CARD_REMOVED: ����ȡ��
//       HXEMV_CARD_OP     : ��������
//       HXEMV_CARD_SW     : �Ƿ���״̬��
//       HXEMV_TERMINATE   : ����ܾ�������������ֹ
//       HXEMV_RESELECT    : ��Ҫ����ѡ��Ӧ��
//       HXEMV_TRANS_NOT_ALLOWED : ���ײ�֧��
//       HXEMV_PARA        : ��������
//       HXEMV_FLOW_ERROR  : EMV���̴���
//		 HXEMV_CALLBACK_METHOD : û���Էǻص���ʽ��ʼ������
//       HXEMV_CORE        : �ڲ�����
//       HXEMV_DENIAL	   : qPboc�ܾ�
// �ǽӿ��ܷ�����
//		 HXEMV_TRY_OTHER_INTERFACE	: ��������ͨ�Ž���
//		 HXEMV_CARD_TRY_AGAIN		: Ҫ�������ύ��Ƭ(�ǽ�)
// Note: �������HXEMV_RESELECT����iHxGetSupportedApp()��ʼ����ִ������
//       ��ΪGPOʧ�ܺ������Ҫ��ʾ������Ϣ(������&����һ��), ��������ܷ�����������Ϣ������Ϣ
int iHxGPO2(uchar *pszTransTime, ulong ulTTC, uint uiTransType, uchar *pszAmount, uint uiCurrencyCode, int *piMsgType, int *piErrMsgType)
{
	uchar ucTransType; // ��������, T9Cֵ
	int   iRet;
	uchar sBuf[100];
	uchar *pucAppConfirmSupport;          // TDFxx, 1:֧��Ӧ��ȷ�� 0:��֧��Ӧ��ȷ��(TAG_DFXX_AppConfirmSupport)

	vTraceWriteTxtCr(TRACE_PROC, "iHxGPO2(%s,%lu,%04X,%s,%u)", pszTransTime, ulTTC, uiTransType, pszAmount, uiCurrencyCode);
	*piMsgType = EMVMSG_00_NULL;
	*piErrMsgType = EMVMSG_APPCALL_ERR; // ��ȱʡֵ
	if(gl_iCoreCallFlag != HXCORE_NOT_CALLBACK)
		return(HXEMV_CALLBACK_METHOD); // û���Էǻص���ʽ��ʼ������
	if(gl_iCoreStatus != EMV_STATUS_APP_SELECT)
		return(HXEMV_FLOW_ERROR); // ѡ��Ӧ�ú�ſ���GPO

	// ��ȡ�ն�Ӧ��ѡ��֧�����
	iRet = iTlvGetObjValue(gl_sTlvDbTermFixed, TAG_DFXX_AppConfirmSupport, &pucAppConfirmSupport);
	if(iRet <= 0)
		return(SEL_ERR_OTHER);

	ucTransType = uiTransType & 0xFF; // ȡ��T9Cֵ
	// �����Ϸ��Լ��
	iRet = iTestIfValidDateTime(pszTransTime);
	if(iRet || ulTTC<1 || ulTTC>999999L) {
		*piErrMsgType = EMVMSG_APPCALL_ERR;
		return(HXEMV_PARA);
	}
	switch(uiTransType) {
	case TRANS_TYPE_SALE:			// ����(��Ʒ�ͷ���)
		if(!iEmvTestItemBit(EMV_ITEM_TERM_CAP_ADD, TERM_CAP_ADD_01_GOODS)
				&& !iEmvTestItemBit(EMV_ITEM_TERM_CAP_ADD, TERM_CAP_ADD_02_SERVICES)) {
			iEmvIODispMsg(EMVMSG_TRANS_NOT_ALLOWED, EMVIO_NEED_NO_CONFIRM);
			return(HXEMV_TRANS_NOT_ALLOWED); // �����֧����Ʒ������, ��֧�����ѽ���
		}
		break;
	case TRANS_TYPE_GOODS:			// ��Ʒ
		if(!iEmvTestItemBit(EMV_ITEM_TERM_CAP_ADD, TERM_CAP_ADD_01_GOODS)) {
			iEmvIODispMsg(EMVMSG_TRANS_NOT_ALLOWED, EMVIO_NEED_NO_CONFIRM);
			return(HXEMV_TRANS_NOT_ALLOWED); // �����֧����ƷӦ��, ��֧����Ʒ����
		}
		break;
	case TRANS_TYPE_SERVICES:		// ����
		if(!iEmvTestItemBit(EMV_ITEM_TERM_CAP_ADD, TERM_CAP_ADD_02_SERVICES)) {
			iEmvIODispMsg(EMVMSG_TRANS_NOT_ALLOWED, EMVIO_NEED_NO_CONFIRM);
			return(HXEMV_TRANS_NOT_ALLOWED); // �����֧�ַ���Ӧ��, ��֧�ַ�����
		}
		break;
	case TRANS_TYPE_PAYMENT:		// ֧��
		if(!iEmvTestItemBit(EMV_ITEM_TERM_CAP_ADD, TERM_CAP_ADD_06_PAYMENT)) {
			iEmvIODispMsg(EMVMSG_TRANS_NOT_ALLOWED, EMVIO_NEED_NO_CONFIRM);
			return(HXEMV_TRANS_NOT_ALLOWED); // �����֧��֧��Ӧ��, ��֧��֧������
		}
		break;
	case TRANS_TYPE_CASH:			// ȡ��
		if(!iEmvTestItemBit(EMV_ITEM_TERM_CAP_ADD, TERM_CAP_ADD_00_CASH)) {
			*piErrMsgType = EMVMSG_TRANS_NOT_ALLOWED;
			return(HXEMV_TRANS_NOT_ALLOWED); // �����֧���ֽ�Ӧ��, ��֧��ȡ�ֽ���
		}
		break;
	case TRANS_TYPE_RETURN:			// �˻�
		break; // �����ն�������չ��û�ж��˻��Ķ���, ��Ϊ֧��
	case TRANS_TYPE_DEPOSIT:		// ���
		if(!iEmvTestItemBit(EMV_ITEM_TERM_CAP_ADD, TERM_CAP_ADD_08_CASH_DEPOSIT)) {
			*piErrMsgType = EMVMSG_TRANS_NOT_ALLOWED;
			return(HXEMV_TRANS_NOT_ALLOWED); // �����֧�ִ��Ӧ��, ��֧�ִ���
		}
		break;
	case TRANS_TYPE_AVAILABLE_INQ:	// ��������ѯ
	case TRANS_TYPE_BALANCE_INQ:	// ����ѯ
		if(!iEmvTestItemBit(EMV_ITEM_TERM_CAP_ADD, TERM_CAP_ADD_04_INQUIRY)) {
			*piErrMsgType = EMVMSG_TRANS_NOT_ALLOWED;
			return(HXEMV_TRANS_NOT_ALLOWED); // �����֧�ִ��Ӧ��, ��֧�ִ���
		}
		break;
	case TRANS_TYPE_TRANSFER:		// ת��
		if(!iEmvTestItemBit(EMV_ITEM_TERM_CAP_ADD, TERM_CAP_ADD_05_TRANSFER)) {
			*piErrMsgType = EMVMSG_TRANS_NOT_ALLOWED;
			return(HXEMV_TRANS_NOT_ALLOWED); // �����֧��ת��Ӧ��, ��֧��ת�˽���
		}
		break;
	default:
		if(gl_iAppType != 0/*0:�ͼ����*/)
			break; // ���ͼ����, �������н���
		*piErrMsgType = EMVMSG_TRANS_NOT_ALLOWED;
		return(HXEMV_TRANS_NOT_ALLOWED); // ��ʶ��Ľ�������
	}

	if(uiCurrencyCode>999) {
		*piErrMsgType = EMVMSG_APPCALL_ERR;
		return(HXEMV_PARA);
	}

	// ���������ڡ�����ʱ�䡢�ն˽�����ˮ�š��������͡������Ҵ������tlv���ݿ�
	vTwoOne(pszTransTime+2, 6, sBuf); // ��������
	iRet = iTlvMakeAddObj(gl_sTlvDbTermVar, TAG_9A_TransDate, 3, sBuf, TLV_CONFLICT_REPLACE);
	if(iRet < 0) {
		*piErrMsgType = EMVMSG_NATIVE_ERR;
		return(HXEMV_CORE);
	}
	vTwoOne(pszTransTime+8, 6, sBuf); // ����ʱ��
	iRet = iTlvMakeAddObj(gl_sTlvDbTermVar, TAG_9F21_TransTime, 3, sBuf, TLV_CONFLICT_REPLACE);
	if(iRet < 0) {
		*piErrMsgType = EMVMSG_NATIVE_ERR;
		return(HXEMV_CORE);
	}
	sprintf(sBuf, "%06lu", ulTTC);
	vTwoOne(sBuf, 6, sBuf+10); // T9F41����N4-N8�ֽ�, ������ѭ8583Э��,����N6
	iRet = iTlvMakeAddObj(gl_sTlvDbTermVar, TAG_9F41_TransSeqCounter, 3, sBuf+10, TLV_CONFLICT_REPLACE);
	if(iRet < 0) {
		*piErrMsgType = EMVMSG_NATIVE_ERR;
		return(HXEMV_CORE);
	}
	
	vOneTwo0(&ucTransType, 1, sBuf); // ��������Ϊn2��
	if(iTestStrDecimal(sBuf, 2) != 0) {
		*piErrMsgType = EMVMSG_NATIVE_ERR;
		return(HXEMV_PARA);
	}
	iRet = iTlvMakeAddObj(gl_sTlvDbTermVar, TAG_9C_TransType, 1, &ucTransType, TLV_CONFLICT_REPLACE);
	if(iRet < 0) {
		*piErrMsgType = EMVMSG_NATIVE_ERR;
		return(HXEMV_CORE);
	}
	vLongToStr(uiTransType, 2, sBuf);
	iRet = iTlvMakeAddObj(gl_sTlvDbTermVar, TAG_DFXX_TransType, 2, sBuf, TLV_CONFLICT_REPLACE);
	if(iRet < 0) {
		iEmvIODispMsg(EMVMSG_NATIVE_ERR, EMVIO_NEED_NO_CONFIRM);
		return(HXEMV_CORE);
	}

	iEmvIOSetTransAmount(pszAmount, "0"); // Amount & AmountOther

	sprintf(sBuf, "%04lu", uiCurrencyCode);
	vTwoOne(sBuf, 4, sBuf+10);
	iRet = iTlvMakeAddObj(gl_sTlvDbTermVar, TAG_5F2A_TransCurrencyCode, 2, sBuf+10, TLV_CONFLICT_REPLACE);
	if(iRet < 0) {
		*piErrMsgType = EMVMSG_NATIVE_ERR;
		return(HXEMV_CORE);
	}

	iRet = iTlvMakeAddObj(gl_sTlvDbTermVar, TAG_9F39_POSEntryMode, 1, "\x05", TLV_CONFLICT_REPLACE); // pos entry mode, "05":ic card
	if(iRet < 0) {
		*piErrMsgType = EMVMSG_NATIVE_ERR;
		return(HXEMV_CORE);
	}

	if(gl_iCardType == EMV_CONTACTLESS_CARD) {
		// ����Ƿǽӿ�, ��T9F66����tlv���ݿ�
		iRet = iTlvMakeAddObj(gl_sTlvDbTermVar, TAG_9F66_ContactlessSupport, 4, psPbocCtlsGet9F66(), TLV_CONFLICT_REPLACE);
		if(iRet < 0) {
			iEmvIODispMsg(EMVMSG_NATIVE_ERR, EMVIO_NEED_NO_CONFIRM);
			return(HXEMV_CORE);
		}
    } else { //Add by Qingbo Jia
        // ������Ƿǽӿ�ģʽ
        iRet = iTlvMakeAddObj(gl_sTlvDbTermVar, TAG_9F66_ContactlessSupport, 4, "\x40\x00\x00\x00", TLV_CONFLICT_REPLACE);
        if(iRet < 0) {
            iEmvIODispMsg(EMVMSG_NATIVE_ERR, EMVIO_NEED_NO_CONFIRM);
            return(HXEMV_CORE);
        }
    }

	iRet = iEmvGpo();
	vTraceWriteTxtCr(TRACE_PROC, "(%d)iEmvGpo()", iRet);
	if(iRet == HXEMV_RESELECT) {
		*piErrMsgType = EMVMSG_0C_NOT_ACCEPTED;
		gl_iCoreStatus = EMV_STATUS_GET_SUPPORTED_APP; // ��Ҫ����ѡ��Ӧ��
		if(gl_iCardAppNum > 0) {
			if(*pucAppConfirmSupport)
				*piMsgType = EMVMSG_13_TRY_AGAIN; // �ն�֧��Ӧ��ȷ��ʱ����Ҫ��ʾ"Try Again"
			else
				*piMsgType = EMVMSG_00_NULL;
			return(HXEMV_RESELECT);
		} else {
			*piMsgType = EMVMSG_TERMINATE;
			return(HXEMV_TERMINATE);
		}
	}
	if(iRet == HXEMV_CARD_OP) {
		if(gl_iCardType == EMV_CONTACTLESS_CARD) {
			// �ǽӿ�, ��ʧЧ����, �ο�JR/T0025.12��2013 6.5.2, P14
			*piErrMsgType = EMVMSG_13_TRY_AGAIN;
			return(HXEMV_CARD_TRY_AGAIN);
		}
		if(uiEmvTestCard() == 0) {
			*piErrMsgType = EMVMSG_0F_PROCESSING_ERROR;
			return(HXEMV_CARD_REMOVED); // ����ȡ��
		} else {
			*piErrMsgType = EMVMSG_06_CARD_ERROR;
			return(HXEMV_CARD_OP); // ��Ƭ�������󣬽�����ֹ
		}
	}
	if(iRet == HXEMV_CARD_SW) {
		if(gl_iCardType == EMV_CONTACTLESS_CARD) {
			// �ǽӿ�, �ο�JR/T0025.12��2013 6.5.3, P14
			*piErrMsgType = EMVMSG_TERMINATE;
			return(HXEMV_TRY_OTHER_INTERFACE);
		}
		// Refer to EMV2008 book3 6.3.5, P47, ��ʶ���SW��Ҫ��ֹ����
		*piErrMsgType = EMVMSG_TERMINATE;
		return(HXEMV_CARD_SW);
	}
	if(iRet == HXEMV_TERMINATE) {
		*piErrMsgType = EMVMSG_TERMINATE;
		if(gl_iCardType == EMV_CONTACTLESS_CARD)
			return(HXEMV_TRY_OTHER_INTERFACE); // �ǽӿ�, ��ΪӦ��������
		return(HXEMV_TERMINATE);
	}
	if(iRet != HXEMV_OK) {
		*piErrMsgType = EMVMSG_NATIVE_ERR;
		return(iRet);
	}

	*piErrMsgType = EMVMSG_00_NULL;
	gl_iCoreStatus = EMV_STATUS_GPO;
	return(HXEMV_OK);
}

// ��Ӧ�ü�¼
// out : psRid+pszPan+piPanSeqNo�������ṩ��Ӧ�ò��жϺ��������ֿ��������
//		 psRid             : RID[5], NULL��ʾ����Ҫ����
//       pszPan            : �˺�
//       piPanSeqNo        : �˺����к�, -1��ʾ�޴�����, NULL��ʾ����Ҫ����
//       piErrMsgType      : ������Ϣ������
// ret : HXEMV_OK          : OK
//       HXEMV_CARD_REMOVED: ����ȡ��
//       HXEMV_CARD_OP     : ��������
//       HXEMV_CARD_SW     : �Ƿ���ָ��״̬��
//       HXEMV_TERMINATE   : ����ܾ�������������ֹ
//       HXEMV_FLOW_ERROR  : EMV���̴���
//       HXEMV_CORE        : �ڲ�����
//		 HXEMV_CALLBACK_METHOD : û���Էǻص���ʽ��ʼ������
// �ǽӿ��ܷ�����
//		 HXEMV_TRY_OTHER_INTERFACE	: ��������ͨ�Ž���
int iHxReadRecord2(uchar *psRid, uchar *pszPan, int *piPanSeqNo, int *piErrMsgType)
{
	int   i;
	int   iRet;
	uchar *p;

	vTraceWriteTxtCr(TRACE_PROC, "iHxReadRecord2()");
	*piErrMsgType = EMVMSG_APPCALL_ERR; // ��ȱʡֵ
	if(gl_iCoreCallFlag != HXCORE_NOT_CALLBACK)
		return(HXEMV_CALLBACK_METHOD); // û���Էǻص���ʽ��ʼ������
	if(gl_iCoreStatus != EMV_STATUS_GPO)
		return(HXEMV_FLOW_ERROR); // GPO��ſ��Զ���¼
	iRet = iPbocCtlsGetRoute(); // �����ߵ�·��, 0:�Ӵ�Pboc(��׼DC��EC) 1:�ǽӴ�Pboc(��׼DC��EC) 2:qPboc���� 3:qPboc�ѻ� 4:qPboc�ܾ�
	if(iRet==2 || iRet==4)
		return(HXEMV_FLOW_ERROR); // qPboc������qPboc�ܾ������Զ���¼

	iRet = iEmvReadRecord();
	vTraceWriteTxtCr(TRACE_PROC, "(%d)iEmvReadRecord()", iRet);
	if(iRet == HXEMV_CARD_OP) {
		if(gl_iCardType == EMV_CONTACTLESS_CARD) {
			// �ǽӿ�, ��ʧЧ����, �ο�JR/T0025.12��2013 6.5.2, P14
			*piErrMsgType = EMVMSG_13_TRY_AGAIN;
			return(HXEMV_CARD_TRY_AGAIN);
		}
		if(uiEmvTestCard() == 0) {
			*piErrMsgType = EMVMSG_0F_PROCESSING_ERROR;
			return(HXEMV_CARD_REMOVED); // ����ȡ��
		} else {
			*piErrMsgType = EMVMSG_06_CARD_ERROR;
			return(HXEMV_CARD_OP); // ��Ƭ�������󣬽�����ֹ
		}
	}
	if(iRet == HXEMV_CARD_SW) {
		// Refer to EMV2008 book3 6.3.5, P47, ��ʶ���SW��Ҫ��ֹ����
		*piErrMsgType = EMVMSG_TERMINATE;
		return(HXEMV_CARD_SW);
	}
	if(iRet == HXEMV_TERMINATE) {
		*piErrMsgType = EMVMSG_TERMINATE;
		return(iRet);
	}
	if(iRet == HXEMV_EXPIRED_APP) {
		*piErrMsgType = EMVMSG_EXPIRED_APP;
		return(HXEMV_TERMINATE);
	}
	if(iRet) {
		*piErrMsgType = EMVMSG_NATIVE_ERR;
		return(HXEMV_CORE);
	}

	// ��ȡ������Ϣ
	*piErrMsgType = EMVMSG_NATIVE_ERR;
	if(psRid) {
		iRet = iTlvGetObjValue(gl_sTlvDbCard, TAG_4F_AID, &p);
		if(iRet < 5)
			return(HXEMV_CORE);
		memcpy(psRid, p, 5);
	}
	if(pszPan) {
		iRet = iTlvGetObjValue(gl_sTlvDbCard, TAG_5A_PAN, &p);
		if(iRet<1 || iRet>10)
			return(HXEMV_CORE);
		vOneTwo0(p, iRet, pszPan);
		for(i=0; i<(int)strlen(pszPan); i++)
			if(pszPan[i] == 'F')
				pszPan[i] = 0;
	}
	if(piPanSeqNo) {
		iRet = iTlvGetObjValue(gl_sTlvDbCard, TAG_5F34_PANSeqNo, &p);
		if(iRet == 1)
			*piPanSeqNo = (int)(*p);
		else
			*piPanSeqNo = -1;
	}

	*piErrMsgType = EMVMSG_00_NULL;
	gl_iCoreStatus = EMV_STATUS_READ_REC;
	return(HXEMV_OK);
}

// ���ÿ�Ƭ�˻����������ֿ�������Ϣ
// in  : iBlackFlag        : ���øÿ��Ƿ�Ϊ��������, 0:��Ϊ�������� 1:��������
//       pszRecentAmount   : ���һ�����ѽ��(���ڷֿ����Ѽ��)
// out : piErrMsgType      : ������Ϣ������
// ret : HXEMV_OK          : OK
//       HXEMV_CORE        : �ڲ�����
// Note: �������¼���ֿ�Ƭ�Ǻ�����������������Ѽ�¼, ����Ҫ���ô˺��������˻���Ϣ
int iHxSetPanInfo2(int iBlackFlag, uchar *pszRecentAmount, int *piErrMsgType)
{
	int iRet;
	vTraceWriteTxtCr(TRACE_PROC, "iHxSetPanInfo2(%d,%s)", iBlackFlag, pszRecentAmount);
	iRet = iEmvIOSetPanInfo(iBlackFlag, pszRecentAmount);
	if(iRet != EMVIO_OK) {
		*piErrMsgType = EMVMSG_NATIVE_ERR;
		return(HXEMV_CORE);
	}
	*piErrMsgType = EMVMSG_00_NULL;
	return(HXEMV_OK);
}

// SDA��DDA
// out : piNeedCheckCrlFlag  : �Ƿ���Ҫ�жϷ����й�Կ֤��Ϊ��������־, 0:����Ҫ�ж� 1:��Ҫ�ж�
//       psRid               : RID[5], psRid+pucCaIndex+psCertSerNo�������ṩ��Ӧ�ò��жϷ����й�Կ֤���Ƿ��ں������б���
//       pucCaIndex          : CA��Կ����
//       psCertSerNo         : �����й�Կ֤�����к�[3]
//       piErrMsgType        : ������Ϣ������
// ret : HXEMV_OK            : OK
//       HXEMV_CARD_REMOVED  : ����ȡ��
//       HXEMV_CARD_OP       : ��������
//       HXEMV_CARD_SW       : �Ƿ���״̬��
//       HXEMV_TERMINATE     : ����ܾ�������������ֹ
//       HXEMV_DENIAL        : �ն���Ϊ�������Ϊ�ѻ��ܾ�
//       HXEMV_DENIAL_ADVICE : �ն���Ϊ�������Ϊ�ѻ��ܾ�, ��Advice
//       HXEMV_FLOW_ERROR    : EMV���̴���
//       HXEMV_CORE          : �ڲ�����
//		 HXEMV_CALLBACK_METHOD : û���Էǻص���ʽ��ʼ������
//		 HXEMV_FDDA_FAIL	 : fDDAʧ��(�ǽ�)
// Note: Ӧ�ò�֧�ַ����й�Կ֤������������, piNeedCheckCrlFlag��������, ������Ժ���
// Note: qPboc, FDDAʧ�ܺ�, piErrMsgType������, Ӧ�ò���Ҫ���м��9F6C B1 b6b5, �Ծ����Ǿܾ����ǽ�������
int iHxOfflineDataAuth2(int *piNeedCheckCrlFlag, uchar *psRid, uchar *pucCaIndex, uchar *psCertSerNo, int *piErrMsgType)
{
	int iRet;
	uchar *pucCaPubKeyIndex;
	uchar *psAid;

	vTraceWriteTxtCr(TRACE_PROC, "iHxOfflineDataAuth2()");
	*piErrMsgType = EMVMSG_APPCALL_ERR; // ��ȱʡֵ
	if(gl_iCoreCallFlag != HXCORE_NOT_CALLBACK)
		return(HXEMV_CALLBACK_METHOD); // û���Էǻص���ʽ��ʼ������
	if(gl_iCoreStatus<EMV_STATUS_READ_REC || gl_iCoreStatus>=EMV_STATUS_GAC1)
		return(HXEMV_FLOW_ERROR); // ����¼ǰ��GAC���������ѻ�������֤

	*piNeedCheckCrlFlag = 0; // ������Ϊ����Ҫ�ж�CRL
	iRet = iEmvOfflineDataAuth(0); // �ǻص��ӿڹ̶�����0, ������鷢���й�Կ֤���Ƿ���CRL�б���
	vTraceWriteTxtCr(TRACE_PROC, "(%d)iEmvOfflineDataAuth()", iRet);
    if(iRet != HXEMV_OK) {
		vTraceTlvDb();
    }
	if(iRet == HXEMV_FDDA_FAIL) {
		*piErrMsgType = EMVMSG_00_NULL;
		return(HXEMV_FDDA_FAIL); // FDDAʧ��
	}
	if(iRet == HXEMV_TERMINATE) {
		*piErrMsgType = EMVMSG_TERMINATE;
		return(HXEMV_TERMINATE);
	}
	if(iRet==HXEMV_DENIAL || iRet==HXEMV_DENIAL_ADVICE) {
		*piErrMsgType = EMVMSG_07_DECLINED;
		return(iRet);
	}
	if(iRet == HXEMV_CARD_OP) {
		if(uiEmvTestCard() == 0) {
			*piErrMsgType = EMVMSG_0F_PROCESSING_ERROR;
			return(HXEMV_CARD_REMOVED); // ����ȡ��
		} else {
			*piErrMsgType = EMVMSG_06_CARD_ERROR;
			return(HXEMV_CARD_OP); // ��Ƭ�������󣬽�����ֹ
		}
	}
	if(iRet == HXEMV_CARD_SW) {
		// Refer to EMV2008 book3 6.3.5, P47, ��ʶ���SW��Ҫ��ֹ����
		*piErrMsgType = EMVMSG_TERMINATE;
		return(HXEMV_CARD_SW);
	}
	if(iRet != HXEMV_OK) {
		*piErrMsgType = EMVMSG_NATIVE_ERR;
		return(HXEMV_CORE);
	}

	// �ѻ�������֤û�б���, ׼��Ӧ�ò���Ҫ��CRL�������
	*piErrMsgType = EMVMSG_00_NULL;
	iRet = iTlvGetObjValue(gl_sTlvDbCard, TAG_8F_CAPubKeyIndex, &pucCaPubKeyIndex);
	if(iRet != 1)
		return(EMVIO_OK); // �Ҳ���CA��Կ����, ����Ҫ���CRL
	*pucCaIndex = *pucCaPubKeyIndex;

	iRet = iTlvGetObjValue(gl_sTlvDbCard, TAG_4F_AID, &psAid);
	if(iRet < 5)
		return(EMVIO_OK);
	memcpy(psRid, psAid, 5);

	iEmvIOGetCertSerNo(psCertSerNo);

	*piNeedCheckCrlFlag = 1; // ������Ҫ�ж�CRL��־
	return(HXEMV_OK);
}

// ���÷����й�Կ֤��Ϊ������
// out : piErrMsgType        : ������Ϣ������, 0��ʾ����Ҫ��ʾ����Ϣ
// ret : HXEMV_OK            : OK
//       HXEMV_CARD_REMOVED  : ����ȡ��
//       HXEMV_CARD_OP       : ��������
//       HXEMV_CARD_SW       : �Ƿ���״̬��
//       HXEMV_TERMINATE     : ����ܾ�������������ֹ
//       HXEMV_DENIAL        : �ն���Ϊ�������Ϊ�ѻ��ܾ�
//       HXEMV_DENIAL_ADVICE : �ն���Ϊ�������Ϊ�ѻ��ܾ�, ��Advice
//       HXEMV_FLOW_ERROR    : EMV���̴���
//       HXEMV_CORE          : �ڲ�����
//		 HXEMV_CALLBACK_METHOD : û���Էǻص���ʽ��ʼ������
// Note: Ӧ�ò����iHxOfflineDataAuth2()��õ�Rid+CaIndex+CertSerNo
//       ���ݴ������жϷ����й�Կ֤���Ƿ�Ϊ������,ֻ���Ǻ�����,����Ҫ���ñ�����֪ͨ����
int iHxSetIssuerCertCrl2(int *piErrMsgType)
{
	int iRet;

	vTraceWriteTxtCr(TRACE_PROC, "iHxSetIssuerCertCrl2()");
	*piErrMsgType = EMVMSG_APPCALL_ERR; // ��ȱʡֵ
	if(gl_iCoreCallFlag != HXCORE_NOT_CALLBACK)
		return(HXEMV_CALLBACK_METHOD); // û���Էǻص���ʽ��ʼ������
	if(gl_iCoreStatus<EMV_STATUS_READ_REC || gl_iCoreStatus>=EMV_STATUS_GAC1)
		return(HXEMV_FLOW_ERROR); // ����¼ǰ��GAC���������ѻ�������֤

	iRet = iEmvOfflineDataAuth(1); // ����1, ���趨�����й�Կ֤������CRL�б���
    if(iRet != HXEMV_OK) {
		vTraceTlvDb();
    }
	if(iRet == HXEMV_TERMINATE) {
		*piErrMsgType = EMVMSG_TERMINATE;
		return(HXEMV_TERMINATE);
	}
	if(iRet==HXEMV_DENIAL || iRet==HXEMV_DENIAL_ADVICE) {
		*piErrMsgType = EMVMSG_07_DECLINED;
		return(iRet);
	}
	if(iRet == HXEMV_CARD_OP) {
		if(uiEmvTestCard() == 0) {
			*piErrMsgType = EMVMSG_0F_PROCESSING_ERROR;
			return(HXEMV_CARD_REMOVED); // ����ȡ��
		} else {
			*piErrMsgType = EMVMSG_06_CARD_ERROR;
			return(HXEMV_CARD_OP); // ��Ƭ�������󣬽�����ֹ
		}
	}
	if(iRet == HXEMV_CARD_SW) {
		// Refer to EMV2008 book3 6.3.5, P47, ��ʶ���SW��Ҫ��ֹ����
		*piErrMsgType = EMVMSG_TERMINATE;
		return(HXEMV_CARD_SW);
	}
	if(iRet != HXEMV_OK) {
		*piErrMsgType = EMVMSG_NATIVE_ERR;
		return(HXEMV_CORE);
	}

	*piErrMsgType = EMVMSG_00_NULL;
	return(HXEMV_OK);
}

// ��������
// out : piErrMsgType        : ������Ϣ������
// ret : HXEMV_OK            : OK
//       HXEMV_TERMINATE     : ����ܾ�������������ֹ
//       HXEMV_FLOW_ERROR    : EMV���̴���
//       HXEMV_CORE          : �ڲ�����
//       HXEMV_DENIAL        : �ն���Ϊ�������Ϊ�ѻ��ܾ�
//       HXEMV_DENIAL_ADVICE : �ն���Ϊ�������Ϊ�ѻ��ܾ�, ��Advice
//       HXEMV_CARD_REMOVED  : ����ȡ��
//       HXEMV_CARD_OP       : �ն���Ϊ�������Ϊ�ѻ��ܾ�, ����AACʱ��Ƭ����
//		 HXEMV_CALLBACK_METHOD : û���Էǻص���ʽ��ʼ������
int iHxProcRistrictions2(int *piErrMsgType)
{
	int iRet;

	vTraceWriteTxtCr(TRACE_PROC, "iHxProcRistrictions2()");
	*piErrMsgType = EMVMSG_APPCALL_ERR; // ��ȱʡֵ
	if(gl_iCoreCallFlag != HXCORE_NOT_CALLBACK)
		return(HXEMV_CALLBACK_METHOD); // û���Էǻص���ʽ��ʼ������
	if(gl_iCoreStatus<EMV_STATUS_READ_REC || gl_iCoreStatus>=EMV_STATUS_GAC1)
		return(HXEMV_FLOW_ERROR); // ����¼ǰ��GAC����������������

	iRet = iEmvProcRistrictions();
	vTraceWriteTxtCr(TRACE_PROC, "(%d)iEmvProcRistrictions()", iRet);
	if(iRet != HXEMV_OK)
		vTraceTlvDb();
	if(iRet == HXEMV_CARD_OP) {
		if(uiEmvTestCard() == 0) {
			*piErrMsgType = EMVMSG_0F_PROCESSING_ERROR;
			return(HXEMV_CARD_REMOVED); // ����ȡ��
		} else {
			*piErrMsgType = EMVMSG_06_CARD_ERROR;
			return(HXEMV_CARD_OP); // ��Ƭ�������󣬽�����ֹ
		}
	}
	if(iRet == HXEMV_TERMINATE) {
		*piErrMsgType = EMVMSG_TERMINATE;
		return(iRet);
	}
	if(iRet==HXEMV_DENIAL || iRet==HXEMV_DENIAL_ADVICE) {
		*piErrMsgType = EMVMSG_07_DECLINED;
		return(iRet);
	}
	if(iRet != HXEMV_OK) {
		*piErrMsgType = EMVMSG_NATIVE_ERR;
		return(HXEMV_CORE);
	}

	*piErrMsgType = EMVMSG_00_NULL;
	gl_iCoreStatus = EMV_STATUS_PROC_RISTRICTIONS;
	return(HXEMV_OK);
}

// �ն˷��չ���
// out : piErrMsgType        : ������Ϣ������
// ret : HXEMV_OK            : OK
//       HXEMV_TERMINATE     : ����ܾ�������������ֹ
//       HXEMV_CORE          : ��������
//       HXEMV_CARD_REMOVED  : ����ȡ��
//       HXEMV_CARD_OP       : ��������
//       HXEMV_DENIAL        : �ն���Ϊ�������Ϊ�ѻ��ܾ�
//       HXEMV_DENIAL_ADVICE : �ն���Ϊ�������Ϊ�ѻ��ܾ�, ��Advice
//       HXEMV_FLOW_ERROR    : EMV���̴���
//		 HXEMV_CALLBACK_METHOD : û���Էǻص���ʽ��ʼ������
int iHxTermRiskManage2(int *piErrMsgType)
{
	int iRet;

	vTraceWriteTxtCr(TRACE_PROC, "iHxTermRiskManage2()");
	*piErrMsgType = EMVMSG_APPCALL_ERR; // ��ȱʡֵ
	if(gl_iCoreCallFlag != HXCORE_NOT_CALLBACK)
		return(HXEMV_CALLBACK_METHOD); // û���Էǻص���ʽ��ʼ������
	if(gl_iCoreStatus<EMV_STATUS_READ_REC || gl_iCoreStatus>=EMV_STATUS_GAC1)
		return(HXEMV_FLOW_ERROR); // ����¼ǰ��GAC���������ն˷��չ���

	iRet = iEmvTermRiskManage();
	vTraceWriteTxtCr(TRACE_PROC, "(%d)iEmvTermRiskManage()", iRet);
	if(iRet != HXEMV_OK)
		vTraceTlvDb();
	if(iRet == HXEMV_CARD_OP) {
		if(uiEmvTestCard() == 0) {
			*piErrMsgType = EMVMSG_0F_PROCESSING_ERROR;
			return(HXEMV_CARD_REMOVED); // ����ȡ��
		} else {
			*piErrMsgType = EMVMSG_06_CARD_ERROR;
			return(HXEMV_CARD_OP); // ��Ƭ�������󣬽�����ֹ
		}
	}
	if(iRet == HXEMV_TERMINATE) {
		*piErrMsgType = EMVMSG_TERMINATE;
		return(iRet);
	}
	if(iRet==HXEMV_DENIAL || iRet==HXEMV_DENIAL_ADVICE) {
		*piErrMsgType = EMVMSG_07_DECLINED;
		return(iRet);
	}
	if(iRet != HXEMV_OK) {
		*piErrMsgType = EMVMSG_NATIVE_ERR;
		return(HXEMV_CORE);
	}
	*piErrMsgType = EMVMSG_00_NULL;
	return(HXEMV_OK);
}

// ��ȡ�ֿ�����֤
// out : piCvm               : �ֿ�����֤����, ֧��HXCVM_PLAIN_PIN��HXCVM_CIPHERED_ONLINE_PIN��HXCVM_HOLDER_ID��HXCVM_CONFIRM_AMOUNT
//       piBypassFlag        : ����bypass��־, 0:������, 1:����
//       piMsgType           : ��ʾ��Ϣ��(�����֤����Ϊ֤����֤, ����֤��������)
//       piMsgType2          : ������ʾ��Ϣ, �����Ϊ0, ����Ҫ����ʾ����Ϣ
//       pszAmountStr        : ��ʾ�ø�ʽ�����(HXCVM_HOLDER_ID��֤��������Ҫ)
//       piErrMsgType        : ������Ϣ������, ��������!HXEMV_OK����!HXEMV_NO_DATAʱ����
// ret : HXEMV_OK            : OK
//       HXEMV_NO_DATA       : ����������гֿ�����֤
//       HXEMV_CARD_REMOVED  : ����ȡ��
//       HXEMV_CARD_OP       : ��������
//       HXEMV_TERMINATE     : ����ܾ�������������ֹ
//       HXEMV_DENIAL        : �ն���Ϊ�������Ϊ�ѻ��ܾ�
//       HXEMV_DENIAL_ADVICE : �ն���Ϊ�������Ϊ�ѻ��ܾ�, ��Advice
//       HXEMV_FLOW_ERROR    : EMV���̴���
//       HXEMV_CORE          : �ڲ�����
//		 HXEMV_CALLBACK_METHOD : û���Էǻص���ʽ��ʼ������
// Note: ��iHxDoCvmMethod2()����, ������Ҫ��ε���, ֪������HXEMV_NO_DATA
//       ���ڷ��ص���Ϣ�϶�, ����֧��HXCVM_HOLDER_ID��֤����, piMsgType���ص���֤��������
//       ��ʱ������Ϣʹ�ù̶���Ϣ��, ��������:
//           EMVMSG_VERIFY_HOLDER_ID		"�����ֿ���֤��"
//			 EMVMSG_HOLDER_ID_TYPE			"�ֿ���֤������"
//			 EMVMSG_HOLDER_ID_NO			"�ֿ���֤������"
//       ֤��������ȡTagΪ:TAG_9F61_HolderId
int iHxGetCvmMethod2(int *piCvm, int *piBypassFlag, int *piMsgType, int *piMsgType2, uchar *pszAmountStr, int *piErrMsgType)
{
	int iRet;

	vTraceWriteTxtCr(TRACE_PROC, "iHxGetCvmMethod2()");
	*piMsgType = EMVMSG_00_NULL;
	*piMsgType2 = EMVMSG_00_NULL;
	*piErrMsgType = EMVMSG_APPCALL_ERR; // ��ȱʡֵ
	if(gl_iCoreCallFlag != HXCORE_NOT_CALLBACK)
		return(HXEMV_CALLBACK_METHOD); // û���Էǻص���ʽ��ʼ������
	if(gl_iCoreStatus<EMV_STATUS_READ_REC || gl_iCoreStatus>=EMV_STATUS_GAC1)
		return(HXEMV_FLOW_ERROR); // ����¼ǰ��GAC������CVM����

	iRet = iCvm2GetMethod(piCvm, piBypassFlag, piMsgType, piMsgType2, pszAmountStr);
	vTraceWriteTxtCr(TRACE_PROC, "(%d)iCvm2GetMethod(%d,%d)", iRet, *piCvm, *piBypassFlag);
	if(iRet == HXEMV_TERMINATE) {
		*piErrMsgType = EMVMSG_TERMINATE;
		return(HXEMV_TERMINATE);
	}
	if(iRet==HXEMV_DENIAL || iRet==HXEMV_DENIAL_ADVICE) {
		*piErrMsgType = EMVMSG_07_DECLINED;
		return(iRet);
	}
	if(iRet == HXEMV_CARD_OP) {
		if(uiEmvTestCard() == 0) {
			*piErrMsgType = EMVMSG_0F_PROCESSING_ERROR;
			return(HXEMV_CARD_REMOVED); // ����ȡ��
		} else {
			*piErrMsgType = EMVMSG_06_CARD_ERROR;
			return(HXEMV_CARD_OP); // ��Ƭ�������󣬽�����ֹ
		}
	}
	if(iRet!=HXEMV_OK && iRet!=HXEMV_NO_DATA) {
		*piErrMsgType = EMVMSG_NATIVE_ERR;
		return(HXEMV_CORE);
	}

	*piErrMsgType = EMVMSG_00_NULL;
	return(iRet);
}

// ִ�гֿ�����֤
// in  : iCvmProc            : ��֤��������ʽ, HXCVM_PROC_OK or HXCVM_BYPASS or HXCVM_FAIL or HXCVM_CANCEL or HXCVM_TIMEOUT
//       psCvmData           : ���������, ���Ϊ��������, ����β��Ҫ��0
// out : piMsgType           : ����HXEMV_OKʱʹ��
//                             �����Ϊ0, ����Ҫ��ʾ��������Ϣ
//       piMsgType2          : ����HXEMV_OKʱʹ��
//                             �����Ϊ0, ����Ҫ��ʾ��������Ϣ
//       piErrMsgType        : ������Ϣ������
// ret : HXEMV_OK            : OK, ��Ҫ�������гֿ�����֤, ��������iHxGetCvmMethod2(), Ȼ���ٵ��ñ�����
//       HXEMV_PARA		     : ��������
//       HXEMV_CANCEL        : �û�ȡ��
//       HXEMV_TIMEOUT       : ��ʱ
//       HXEMV_CARD_REMOVED  : ����ȡ��
//       HXEMV_CARD_OP       : ��������
//       HXEMV_CARD_SW       : �Ƿ���״̬��
//       HXEMV_TERMINATE     : ����ܾ�������������ֹ
//       HXEMV_DENIAL        : �ն���Ϊ�������Ϊ�ѻ��ܾ�
//       HXEMV_DENIAL_ADVICE : �ն���Ϊ�������Ϊ�ѻ��ܾ�, ��Advice
//       HXEMV_FLOW_ERROR    : EMV���̴���
//       HXEMV_CORE          : �ڲ�����
//		 HXEMV_CALLBACK_METHOD : û���Էǻص���ʽ��ʼ������
// Note: ִ�е�CVM���������һ��iHxGetCvmMethod2()��õ�CVM
int iHxDoCvmMethod2(int iCvmProc, uchar *psCvmData, int *piMsgType, int *piMsgType2, int *piErrMsgType)
{
	int iRet;

	vTraceWriteTxtCr(TRACE_PROC, "iHxDoCvmMethod2(%d)", iCvmProc);
	*piMsgType = EMVMSG_00_NULL;
	*piMsgType2 = EMVMSG_00_NULL;
	if(gl_iCoreCallFlag != HXCORE_NOT_CALLBACK)
		return(HXEMV_CALLBACK_METHOD); // û���Էǻص���ʽ��ʼ������
	if(gl_iCoreStatus<EMV_STATUS_READ_REC || gl_iCoreStatus>=EMV_STATUS_GAC1)
		return(HXEMV_FLOW_ERROR); // ����¼ǰ��GAC������CVM����

	iRet = iCvm2DoMethod(iCvmProc, psCvmData, piMsgType, piMsgType2);
	vTraceWriteTxtCr(TRACE_PROC, "(%d)iCvm2DoMethod()", iRet);
	if(iRet == HXEMV_FLOW_ERROR) {
		*piErrMsgType = EMVMSG_APPCALL_ERR;
		return(HXEMV_FLOW_ERROR);
	}
	if(iRet == HXEMV_PARA) {
		*piErrMsgType = EMVMSG_APPCALL_ERR;
		return(HXEMV_PARA);
	}
	if(iRet == HXEMV_CANCEL) {
		*piErrMsgType = EMVMSG_CANCEL;
		return(HXEMV_CANCEL);
	}
	if(iRet == HXEMV_TIMEOUT) {
		*piErrMsgType = EMVMSG_TIMEOUT;
		return(HXEMV_TIMEOUT);
	}
	if(iRet == HXEMV_TERMINATE) {
		*piErrMsgType = EMVMSG_TERMINATE;
		return(HXEMV_TERMINATE);
	}
	if(iRet==HXEMV_DENIAL || iRet==HXEMV_DENIAL_ADVICE) {
		*piErrMsgType = EMVMSG_07_DECLINED;
		return(iRet);
	}
	if(iRet == HXEMV_CARD_OP) {
		if(uiEmvTestCard() == 0) {
			*piErrMsgType = EMVMSG_0F_PROCESSING_ERROR;
			return(HXEMV_CARD_REMOVED); // ����ȡ��
		} else {
			*piErrMsgType = EMVMSG_06_CARD_ERROR;
			return(HXEMV_CARD_OP); // ��Ƭ�������󣬽�����ֹ
		}
	}
	if(iRet == HXEMV_CARD_SW) {
		// Refer to EMV2008 book3 6.3.5, P47, ��ʶ���SW��Ҫ��ֹ����
		*piErrMsgType = EMVMSG_TERMINATE;
		return(HXEMV_CARD_SW);
	}
	if(iRet != HXEMV_OK) {
		*piErrMsgType = EMVMSG_NATIVE_ERR;
		return(HXEMV_CORE);
	}

	*piErrMsgType = EMVMSG_00_NULL;
	return(HXEMV_OK);
}

// ��ȡCVM��֤����ǩ�ֱ�־
// out : piNeedSignFlag    : ��ʾ��Ҫǩ�ֱ�־��0:����Ҫǩ�� 1:��Ҫǩ��
// ret : HXEMV_OK          : OK
int iHxGetCvmSignFlag2(int *piNeedSignFlag)
{
	iCvm2GetSignFlag(piNeedSignFlag);
	vTraceWriteTxtCr(TRACE_PROC, "(0)iHxGetCvmSignFlag2(%d)", *piNeedSignFlag);
	return(HXEMV_OK);
}

// �ն���Ϊ����
// out : piErrMsgType        : ������Ϣ������
// ret : HXEMV_OK            : OK
//       HXEMV_CORE          : ��������
//       HXEMV_FLOW_ERROR    : EMV���̴���
//       HXEMV_CARD_REMOVED  : ����ȡ��
//       HXEMV_CARD_OP       : ��������
//       HXEMV_DENIAL        : �ն���Ϊ�������Ϊ�ѻ��ܾ�
//       HXEMV_DENIAL_ADVICE : �ն���Ϊ�������Ϊ�ѻ��ܾ�, ��Advice
//		 HXEMV_CALLBACK_METHOD : û���Էǻص���ʽ��ʼ������
int iHxTermActionAnalysis2(int *piErrMsgType)
{
	int iRet;

	vTraceWriteTxtCr(TRACE_PROC, "iHxTermActionAnalysis2()");
	*piErrMsgType = EMVMSG_APPCALL_ERR; // ��ȱʡֵ
	if(gl_iCoreCallFlag != HXCORE_NOT_CALLBACK)
		return(HXEMV_CALLBACK_METHOD); // û���Էǻص���ʽ��ʼ������
	if(gl_iCoreStatus<EMV_STATUS_READ_REC || gl_iCoreStatus>=EMV_STATUS_GAC1)
		return(HXEMV_FLOW_ERROR); // ����¼ǰ��GAC���������ն���Ϊ����

	iRet = iEmvTermActionAnalysis();
	vTraceWriteTxtCr(TRACE_PROC, "(%d)iEmvTermActionAnalysis()", iRet);
	if(iRet != HXEMV_OK)
		vTraceTlvDb();
	if(iRet == HXEMV_TERMINATE) {
		*piErrMsgType = EMVMSG_TERMINATE;
		return(HXEMV_TERMINATE);
	}
	if(iRet==HXEMV_DENIAL || iRet==HXEMV_DENIAL_ADVICE) {
		*piErrMsgType = EMVMSG_07_DECLINED;
		return(iRet);
	}
	if(iRet == HXEMV_CARD_OP) {
		if(uiEmvTestCard() == 0) {
			*piErrMsgType = EMVMSG_0F_PROCESSING_ERROR;
			return(HXEMV_CARD_REMOVED); // ����ȡ��
		} else {
			*piErrMsgType = EMVMSG_06_CARD_ERROR;
			return(HXEMV_CARD_OP); // ��Ƭ�������󣬽�����ֹ
		}
	}
	if(iRet) {
		*piErrMsgType = EMVMSG_NATIVE_ERR;
		return(iRet);
	}

	*piErrMsgType = EMVMSG_00_NULL;
	gl_iCoreStatus = EMV_STATUS_TERM_ACTION_ANALYSIS;
	return(HXEMV_OK);
}

// ��һ��GAC
// out : piCardAction      : ��Ƭִ�н��
//                           GAC_ACTION_TC     : ��׼(����TC)
//							 GAC_ACTION_AAC    : �ܾ�(����AAC)
//                           GAC_ACTION_AAC_ADVICE : �ܾ�(����AAC,��Advice)
//                           GAC_ACTION_ARQC   : Ҫ������(����ARQC)
//       piMsgType         : ��ʾ��Ϣ��, ��������HXEMV_OKʱʹ��
//       piErrMsgType      : ������Ϣ������
// ret : HXEMV_OK          : OK
//       HXEMV_CORE        : ��������
//       HXEMV_CARD_REMOVED: ����ȡ��
//       HXEMV_CARD_OP     : ��������
//       HXEMV_CARD_SW     : �Ƿ�״̬��
//       HXEMV_TERMINATE   : ����ܾ�������������ֹ
//       HXEMV_NOT_ACCEPTED: ������
//       HXEMV_NOT_ALLOWED : ��������
//       HXEMV_FLOW_ERROR  : EMV���̴���
// Note: ĳЩ�����, �ú������Զ�����iHx2ndGAC()
//       ��:CDAʧ�ܡ����ѻ��ն�GAC1��Ƭ����ARQC...
//		 HXEMV_CALLBACK_METHOD : û���Էǻص���ʽ��ʼ������
int iHx1stGAC2(int *piCardAction, int *piMsgType, int *piErrMsgType)
{
	int iRet;

	vTraceWriteTxtCr(TRACE_PROC, "iHx1stGAC2()");
	*piMsgType = EMVMSG_00_NULL;
	*piErrMsgType = EMVMSG_APPCALL_ERR; // ��ȱʡֵ
	if(gl_iCoreCallFlag != HXCORE_NOT_CALLBACK)
		return(HXEMV_CALLBACK_METHOD); // û���Էǻص���ʽ��ʼ������
	if(gl_iCoreStatus != EMV_STATUS_TERM_ACTION_ANALYSIS)
		return(HXEMV_FLOW_ERROR); // �ն���Ϊ������ſ�����GAC1

	iRet = iEmvGAC1(piCardAction);
	vTraceWriteTxtCr(TRACE_PROC, "(%d)iEmvGAC1(%d)", iRet, *piCardAction);
	if(iRet!=HXEMV_OK || *piCardAction!=GAC_ACTION_ARQC) {
		// �����Ƭ���ط�ARQC(�������ARQC, ��GAC2��trace), TRACE
		vTraceTlvDb();
	}
	if(iRet == HXEMV_TERMINATE) {
		*piErrMsgType = EMVMSG_TERMINATE;
		return(HXEMV_TERMINATE);
	}
	if(iRet == HXEMV_NOT_ALLOWED) {
		*piErrMsgType = EMVMSG_SERVICE_NOT_ALLOWED;
		return(HXEMV_NOT_ALLOWED);
	}
	if(iRet == HXEMV_NOT_ACCEPTED) {
		*piErrMsgType = EMVMSG_0C_NOT_ACCEPTED;
		return(HXEMV_NOT_ACCEPTED);
	}
	if(iRet == HXEMV_CARD_OP) {
		if(uiEmvTestCard() == 0) {
			*piErrMsgType = EMVMSG_0F_PROCESSING_ERROR;
			return(HXEMV_CARD_REMOVED); // ����ȡ��
		} else {
			*piErrMsgType = EMVMSG_06_CARD_ERROR;
			return(HXEMV_CARD_OP); // ��Ƭ�������󣬽�����ֹ
		}
	}
	if(iRet == HXEMV_CARD_SW) {
		// Refer to EMV2008 book3 6.3.5, P47, ��ʶ���SW��Ҫ��ֹ����
		*piErrMsgType = EMVMSG_TERMINATE;
		return(HXEMV_CARD_SW);
	}
	if(iRet) {
		*piErrMsgType = EMVMSG_NATIVE_ERR;
		return(HXEMV_CORE);
	}

	gl_iCoreStatus = EMV_STATUS_GAC1;

	// GAC1�ɹ�ִ��
	*piErrMsgType = EMVMSG_00_NULL;
	if(*piCardAction == GAC_ACTION_TC) {
		// ��Ƭ�ѻ������˽���
		*piMsgType = EMVMSG_03_APPROVED;
		return(HXEMV_OK);
	}
	if(*piCardAction==GAC_ACTION_AAC || *piCardAction==GAC_ACTION_AAC_ADVICE) {
		// ��Ƭ�ܾ��˽���
		*piMsgType = EMVMSG_07_DECLINED;
		return(HXEMV_OK);
	}

	return(HXEMV_OK); // ��Ƭ��������, �߲�������ʾ������Ϣ
}

// �ڶ���GAC
// in  : pszArc            : Ӧ����[2], NULL��""��ʾ����ʧ��
//       pszAuthCode	   : ��Ȩ��[6], NULL��""��ʾ����Ȩ������
//       psOnlineData      : ������Ӧ����(55������),һ��TLV��ʽ����, NULL��ʾ��������Ӧ����
//       psOnlineDataLen   : ������Ӧ���ݳ���
// out : piCardAction      : ��Ƭִ�н��
//                           GAC_ACTION_TC     : ��׼(����TC)
//							 GAC_ACTION_AAC    : �ܾ�(����AAC)
//                           GAC_ACTION_AAC_ADVICE : �ܾ�(����AAC,��Advice)
//       piMsgType         : ��ʾ��Ϣ��, ��������HXEMV_OKʱʹ��
//       piErrMsgType      : ������Ϣ������
// ret : HXEMV_OK          : OK
//       HXEMV_CORE        : ��������
//       HXEMV_CARD_REMOVED: ����ȡ��
//       HXEMV_CARD_OP     : ��������
//       HXEMV_CARD_SW     : �Ƿ���״̬��
//       HXEMV_TERMINATE   : ����ܾ�������������ֹ
//       HXEMV_NOT_ALLOWED : ��������
//       HXEMV_NOT_ACCEPTED: ������
//       HXEMV_FLOW_ERROR  : EMV���̴���
//		 HXEMV_CALLBACK_METHOD : û���Էǻص���ʽ��ʼ������
int iHx2ndGAC2(uchar *pszArc, uchar *pszAuthCode, uchar *psIssuerData, int iIssuerDataLen, int *piCardAction, int *piMsgType, int *piErrMsgType)
{
	int iRet;

	if(pszArc)
		vTraceWriteTxtCr(TRACE_PROC, "iHx2ndGAC2(%s)", pszArc);
	else
		vTraceWriteTxtCr(TRACE_PROC, "iHx2ndGAC2(NULL)");
	*piMsgType = EMVMSG_00_NULL;
	*piErrMsgType = EMVMSG_APPCALL_ERR; // ��ȱʡֵ
	if(gl_iCoreCallFlag != HXCORE_NOT_CALLBACK)
		return(HXEMV_CALLBACK_METHOD); // û���Էǻص���ʽ��ʼ������
	if(gl_iCoreStatus != EMV_STATUS_GAC1)
		return(HXEMV_FLOW_ERROR); // GAC1��ſ�����GAC2

	iRet = iEmvGAC2(pszArc, pszAuthCode, psIssuerData, iIssuerDataLen, piCardAction);
	vTraceWriteTxtCr(TRACE_PROC, "(%d)iEmvGAC2(%d)", iRet, *piCardAction);
	vTraceTlvDb();
	if(iRet == HXEMV_TERMINATE) {
		*piErrMsgType = EMVMSG_TERMINATE;
		return(HXEMV_TERMINATE);
	}
	if(iRet == HXEMV_NOT_ALLOWED) {
		*piErrMsgType = EMVMSG_SERVICE_NOT_ALLOWED;
		return(HXEMV_NOT_ALLOWED);
	}
	if(iRet == HXEMV_NOT_ACCEPTED) {
		*piErrMsgType = EMVMSG_0C_NOT_ACCEPTED;
		return(HXEMV_NOT_ACCEPTED);
	}
	if(iRet == HXEMV_CARD_OP) {
		if(uiEmvTestCard() == 0) {
			*piErrMsgType = EMVMSG_0F_PROCESSING_ERROR;
			return(HXEMV_CARD_REMOVED); // ����ȡ��
		} else {
			*piErrMsgType = EMVMSG_06_CARD_ERROR;
			return(HXEMV_CARD_OP); // ��Ƭ�������󣬽�����ֹ
		}
	}
	if(iRet == HXEMV_CARD_SW) {
		// Refer to EMV2008 book3 6.3.5, P47, ��ʶ���SW��Ҫ��ֹ����
		*piErrMsgType = EMVMSG_TERMINATE;
		return(HXEMV_CARD_SW);
	}
	if(iRet) {
		*piErrMsgType = EMVMSG_NATIVE_ERR;
		return(HXEMV_CORE);
	}

	gl_iCoreStatus = EMV_STATUS_GAC2;

	// GAC2�ɹ�ִ��
	*piErrMsgType = EMVMSG_00_NULL;
	if(*piCardAction == GAC_ACTION_TC) {
		// ��Ƭ�ѻ������˽���
		*piMsgType = EMVMSG_03_APPROVED;
		return(HXEMV_OK);
	}
	// ��Ƭ�ܾ��˽���
	*piMsgType = EMVMSG_07_DECLINED;
	return(HXEMV_OK);
}

// ��ȡ��ʾ��Ϣ����
// in  : iMsgType    : ��Ϣ����, �ο�EmvMsg.h
//       iMsgFor     : ��Ϣ����������(HXMSG_SMART or HXMSG_OPERATOR or HXMSG_CARDHOLDER or HXMSG_LOCAL_LANG)
// out : pszMsg      : �����������Է��ص���Ϣ����
//       pszLanguage : ʹ�õ����Դ��룬ISO-639-1��Сд. ����NULL��ʾ����Ҫ����
// ret : ����ֵ����pszMsg
// Note: �����Ϣ���Ͳ�֧��, pszMsg[0]�ᱻ��ֵΪ0
uchar *pszHxGetMsg2(int iMsgType, int iMsgFor, uchar *pszMsg, uchar *pszLanguage)
{
    uchar szLanguageCode[3];
	int   iTermEnv;
    int   iLen;
    int   iRet;
    
    iLen = 3;
    switch(iMsgFor) {
    case HXMSG_SMART:
			iTermEnv = iEmvTermEnvironment();
			if(iTermEnv == TERM_ENV_ATTENDED)
		        iRet = iHxGetData(TAG_DFXX_LocalLanguage, NULL, NULL, &iLen, szLanguageCode);
			else
		        iRet = iHxGetData(TAG_DFXX_LanguageUsed, NULL, NULL, &iLen, szLanguageCode);
        break;
    case HXMSG_CARDHOLDER:
        iRet = iHxGetData(TAG_DFXX_LanguageUsed, NULL, NULL, &iLen, szLanguageCode);
        break;
    case HXMSG_OPERATOR:
    case HXMSG_LOCAL_LANG:
    default:
        iRet = iHxGetData(TAG_DFXX_LocalLanguage, NULL, NULL, &iLen, szLanguageCode);
        break;
    }
    if(iRet!=HXEMV_OK || iLen!=2) {
		strcpy(pszMsg, pszEmvMsgTableGetInfo(iMsgType, NULL));
		if(pszLanguage)
			pszLanguage[0] = 0;
	} else {
		strcpy(pszMsg, pszEmvMsgTableGetInfo(iMsgType, szLanguageCode));
		if(pszLanguage)
			strcpy(pszLanguage, szLanguageCode);
	}

	return(pszMsg);
}

//// ����Ϊ�ǻص���ʽAPI
//// �ǻص���ʽAPI  *** ���� ***

/********
����Ϊ���Ľӿ�֮��Ķ��ƽӿ�
********/

// �ú���Ϊ��������ʹ��
// ���������ṩ�Ľӿ�����GAC1ʱ�������ֹؼ�����, �����Ҫ��GACǰ����TLV���ݿ�
// in  : pszAmount      : ��Ȩ���, �մ���ʾ���滻
//       pszAmountOther : �������, �մ���ʾ���滻
//       iCurrencyCode  : ���Ҵ���, <0��ʾ���滻
//       pszTransDate   : ��������(YYMMDD), �մ���ʾ���滻
//       ucTransType    : ��������, 0xFF��ʾ���滻
//       pszTransTime   : ����ʱ��(hhmmss), �մ���ʾ���滻
//       iCountryCode   : ���Ҵ���, <0��ʾ���滻
//       pszNodeName    : �̻�����
// ret : HXEMV_OK       : OK
//       HXEMV_PARA     : ���ݷǷ�
//       HXEMV_CORE     : �ڲ�����
int iHxReplaceTlvDb_Jsb(uchar *pszAmount, uchar *pszAmountOther, int iCurrencyCode,
						uchar *pszTransDate, uchar ucTransType, uchar *pszTransTime, int iCountryCode, uchar *pszNodeName)
{
	int   iRet;
	uchar szBuf[100], sBuf[50];

	vTraceWriteTxtCr(TRACE_PROC, "iHxReplaceTlvDb_Jsb(%s,%s,%d,%s,%02X,%s,%d,%s)",
		                          pszAmount, pszAmountOther, iCurrencyCode, pszTransDate,
								  (uint)ucTransType, pszTransTime, iCountryCode, pszNodeName);

	// ��Ȩ���
	if(strlen(pszAmount)) {
		if(strlen(pszAmount) > 12)
			return(HXEMV_PARA);
		sprintf(szBuf, "%012s", pszAmount);
		vTwoOne(szBuf, 12, sBuf);
		// ����ΪT9F02 N��
		iRet = iTlvMakeAddObj(gl_sTlvDbTermVar, TAG_9F02_AmountN, 6, sBuf, TLV_CONFLICT_REPLACE);
		if(iRet < 0)
			return(HXEMV_CORE);
	}
	// �������
	if(strlen(pszAmountOther)) {
		if(strlen(pszAmountOther) > 12)
			return(HXEMV_PARA);
		sprintf(szBuf, "%012s", pszAmountOther);
		vTwoOne(szBuf, 12, sBuf);
		// ����ΪT9F03 N��
		iRet = iTlvMakeAddObj(gl_sTlvDbTermVar, TAG_9F03_AmountOtherN, 6, sBuf, TLV_CONFLICT_REPLACE);
		if(iRet < 0)
			return(HXEMV_CORE);
	}
	// ���Ҵ���
	if(iCurrencyCode >= 0) {
		sprintf(szBuf, "%04u", iCurrencyCode); // ���׻��Ҵ���
		vTwoOne(szBuf, 4, sBuf);
		iRet = iTlvMakeAddObj(gl_sTlvDbTermVar, TAG_5F2A_TransCurrencyCode, 2, sBuf, TLV_CONFLICT_REPLACE);
		if(iRet < 0)
			return(HXEMV_CORE);
	}
	// ��������
	if(strlen(pszTransDate) > 0) {
		if(strlen(pszTransDate) != 6)
			return(HXEMV_PARA);
		vTwoOne(pszTransDate, 6, sBuf);
		iRet = iTlvMakeAddObj(gl_sTlvDbTermVar, TAG_9A_TransDate, 3, sBuf, TLV_CONFLICT_REPLACE);
		if(iRet < 0)
			return(HXEMV_CORE);
	}
	// ��������
	if(ucTransType != 0xFF) {
		iRet = iTlvMakeAddObj(gl_sTlvDbTermVar, TAG_9C_TransType, 1, &ucTransType, TLV_CONFLICT_REPLACE);
		if(iRet < 0)
			return(HXEMV_CORE);
	}
	// ����ʱ��
	if(strlen(pszTransTime) > 0) {
		if(strlen(pszTransTime) != 6)
			return(HXEMV_PARA);
		vTwoOne(pszTransTime, 6, sBuf);
		iRet = iTlvMakeAddObj(gl_sTlvDbTermVar, TAG_9F21_TransTime, 3, sBuf, TLV_CONFLICT_REPLACE);
		if(iRet < 0)
			return(HXEMV_CORE);
	}
	// ���Ҵ���
	if(iCountryCode >= 0) {
		sprintf(szBuf, "%04u", iCountryCode); // ���Ҵ���
		vTwoOne(szBuf, 4, sBuf);
		iRet = iTlvMakeAddObj(gl_sTlvDbTermFixed, TAG_9F1A_TermCountryCode, 2, sBuf, TLV_CONFLICT_REPLACE);
		if(iRet < 0)
			return(HXEMV_CORE);
	}
	// �̻�����
	if(strlen(pszNodeName) > 250)
		return(HXEMV_PARA);
	iRet = iTlvMakeAddObj(gl_sTlvDbTermFixed, TAG_9F4E_MerchantName, strlen(pszNodeName), pszNodeName, TLV_CONFLICT_REPLACE);
	if(iRet < 0)
		return(HXEMV_CORE);

	return(HXEMV_OK);
}
