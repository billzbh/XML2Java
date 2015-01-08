/**************************************
File name     : EmvProc.c
Function      : Pboc3.0�����/EMV2004�ͻ��ӿ�
Author        : Yu Jun
First edition : Mar 11th, 2014
Modified      : Mar 28th, 2014
				ȥ��iHxEmvSetAmount(uchar *pszAmount, uchar *pszAmountOther)�����е�pszAmountOther����
**************************************/
/*
ģ����ϸ����:
	Pboc3.0�����/EMV2004�ͻ��ӿ�
.	����Emv����, ���߲�ͻ���װһ��ӿ�
*/
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <malloc.h>
#include "VposFace.h"
#include "EmvCore.h"
#include "EmvMsg.h"
#include "EmvTrace.h"
#include "EmvProc.h"

// ��ȡ������Ϣ
// out : pszCoreName    : ��������, ������40�ֽ�
//       pszCoreDesc    : ��������, ������60�ֽ��ַ���
//       pszCoreVer     : ���İ汾��, ������10�ֽ��ַ���, ��"1.00"
//       pszReleaseDate : ���ķ�������, YYYY/MM/DD
//       pszCustomerDesc: �ͻ��ӿ�˵��, ������100�ֽ��ַ���
// ret : HXEMV_OK       : OK
int iHxEmvInfo(uchar *pszCoreName, uchar *pszCoreDesc, uchar *pszCoreVer, uchar *pszReleaseDate, uchar *pszCustomerDesc)
{
    iHxCoreInfo(pszCoreName, pszCoreDesc, pszCoreVer, pszReleaseDate);
	strcpy(pszCustomerDesc, "Pboc3.0DC/eCash/Emv�ͻ��ӿ�, �汾[3.10], ��������[2014/05/04]");
	return(HXEMV_OK);
}

// ��ʼ������
// ��ʼ��������Ҫ����IC���������ָ��
// in  : pfiTestCard	: ��⿨Ƭ�Ƿ����
//                        ret : 0 : ������
//                              1 : ����
//       pfiResetCard   : ��Ƭ��λ
//                        ret : <=0 : ��λ����
//                              >0  : ��λ�ɹ�, ����ֵΪATR����
//       pfiDoApdu      : ִ��APDUָ��
//                        in  : iApduInLen   : Apduָ���
//                              psApduIn     : Apduָ��, ��ʽ: Cla Ins P1 P2 Lc DataIn Le
//                        out : piApduOutLen : ApduӦ�𳤶�
//                              psApduOut    : ApduӦ��, ��ʽ: DataOut Sw1 Sw2
//                        ret : 0            : �������ɹ�
//                              1            : ��������
//       pfiCloseCard   : �رտ�Ƭ
//                        ret : ������
// ret : HXEMV_OK       : OK
//       HXEMV_PARA     : ��������
//       HXEMV_CORE     : �ڲ�����
// Note: �ڵ����κ������ӿ�ǰ�������ȳ�ʼ������
int iHxEmvInit(int (*pfiTestCard)(void),
			   int (*pfiResetCard)(uchar *psAtr),
			   int (*pfiDoApdu)(int iApduInLen, unsigned char *psApduIn, int *piApduOutLen, unsigned char *psApduOut),
			   int (*pfiCloseCard)(void))
{
	int  iRet;
	uint uiRet;

	_vPosInit();

	iRet = iHxCoreInit(HXCORE_NOT_CALLBACK, 1/*1:ʵ��Ӧ�ó���*/, time(NULL), "zh");
	if(iRet)
		return(iRet);

	uiRet = _uiSetCardCtrlFunc(0, // ��Ϊ0�ſ���
					           pfiTestCard, pfiResetCard, pfiDoApdu, pfiCloseCard);
	if(uiRet)
		return(HXEMV_PARA);

	iHxSetIccSlot(0);
	return(0);
}

// ���ø��ٻص�����
// ע��, �ú���������, �������ڲ�������
int iHxEmvTraceSet(void (*pfvTraceFunc)(char *))
{
	int iRet;
	iRet = iTraceSet(TRACE_CALLBACK, 1, 1, 1, pfvTraceFunc);
	return(iRet);
}

// �����ն˲���
// in  : pHxTermParam      : �ն˲���
// ret : HXEMV_OK          : OK
//       HXEMV_PARA        : ��������
//       HXEMV_CORE        : �ڲ�����
//       HXEMV_FLOW_ERROR  : EMV���̴���
int iHxEmvSetParam(stHxTermParam *pHxTermParam)
{
	stTermParam TermParam;
	uchar szErrTag[100];
	int   iRet;

	memset(&TermParam, 0, sizeof(TermParam));
	TermParam.ucTermType = pHxTermParam->ucTermType;
	memcpy(TermParam.sTermCapability, pHxTermParam->sTermCapability, 3);
	memcpy(TermParam.sAdditionalTermCapability, pHxTermParam->sAdditionalTermCapability, 5);
	TermParam.ucReaderCapability = 2;     // 2:ֻ֧��IC��
	TermParam.ucVoiceReferralSupport = 3; // 3:��֧��,ȱʡdecline
	TermParam.ucPinBypassBehavior = pHxTermParam->ucPinBypassBehavior; // 0:ÿ��bypassֻ��ʾ�ô�bypass
	memcpy(TermParam.szMerchantId, pHxTermParam->szMerchantId, 15);
	memcpy(TermParam.szTermId, pHxTermParam->szTermId, 8);
	memcpy(TermParam.szMerchantNameLocation, pHxTermParam->szMerchantNameLocation, sizeof(TermParam.szMerchantNameLocation)-1);
	TermParam.iMerchantCategoryCode = pHxTermParam->iMerchantCategoryCode;
	TermParam.iTermCountryCode = pHxTermParam->uiTermCountryCode;
	TermParam.iTermCurrencyCode = 156; // �ò�����GPOʱ����, �ն˲�����Ҫ�ò���
	memcpy(TermParam.szAcquirerId, pHxTermParam->szAcquirerId, sizeof(TermParam.szAcquirerId)-1);
	TermParam.ucTransLogSupport = 1;
	TermParam.ucBlackListSupport = 1;
	TermParam.ucSeparateSaleSupport = 1;
	TermParam.ucAppConfirmSupport = pHxTermParam->ucAppConfirmSupport;
	strcpy(TermParam.szLocalLanguage, "zh");
	strcpy(TermParam.szPinpadLanguage, "zhen");
	strcpy(TermParam.szIFDSerialNo, "00000000");

	// �ǽӲ���
	memcpy(TermParam.sTermCtlsCapability, "\x40\x00\x00\x00", 4); // TDFxx, �ն˷ǽ�����, T9F66 Markλ, ���յ�T9F66ֵҪ��ò����������

	// ����Ϊ��AID���ò���
	TermParam.AidCommonPara.cOnlinePinSupport = -1; // ���ն���ز���

	TermParam.AidCommonPara.iDefaultDDOLLen = pHxTermParam->AidCommonPara.iDefaultDDOLLen;
	if(TermParam.AidCommonPara.iDefaultDDOLLen >= 0) {
		if(TermParam.AidCommonPara.iDefaultDDOLLen > sizeof(TermParam.AidCommonPara.sDefaultDDOL))
			return(HXEMV_PARA);
		memcpy(TermParam.AidCommonPara.sDefaultDDOL, pHxTermParam->AidCommonPara.sDefaultDDOL, TermParam.AidCommonPara.iDefaultDDOLLen);
	} else {
		TermParam.AidCommonPara.iDefaultDDOLLen = -1;
	}

	TermParam.AidCommonPara.iDefaultTDOLLen = pHxTermParam->AidCommonPara.iDefaultTDOLLen;
	if(TermParam.AidCommonPara.iDefaultTDOLLen >= 0) {
		if(TermParam.AidCommonPara.iDefaultTDOLLen > sizeof(TermParam.AidCommonPara.sDefaultTDOL))
			return(HXEMV_PARA);
		memcpy(TermParam.AidCommonPara.sDefaultTDOL, pHxTermParam->AidCommonPara.sDefaultTDOL, TermParam.AidCommonPara.iDefaultTDOLLen);
	} else {
		TermParam.AidCommonPara.iDefaultTDOLLen = -1;
	}

	TermParam.AidCommonPara.ucTermRiskDataLen = 0;

	if(memcmp(pHxTermParam->AidCommonPara.sTermAppVer, "\xFF\xFF", 2) != 0) {
		TermParam.AidCommonPara.ucTermAppVerExistFlag = 1;
		memcpy(TermParam.AidCommonPara.sTermAppVer, pHxTermParam->AidCommonPara.sTermAppVer, 2);
	}
	if(pHxTermParam->AidCommonPara.ulFloorLimit != 0xFFFFFFFFL) {
		TermParam.AidCommonPara.ucFloorLimitExistFlag = 1;
		TermParam.AidCommonPara.ulFloorLimit = pHxTermParam->AidCommonPara.ulFloorLimit;
	}
	TermParam.AidCommonPara.iMaxTargetPercentage = pHxTermParam->AidCommonPara.iMaxTargetPercentage;
	TermParam.AidCommonPara.iTargetPercentage = pHxTermParam->AidCommonPara.iTargetPercentage;
	if(pHxTermParam->AidCommonPara.ulThresholdValue != 0xFFFFFFFFL) {
		TermParam.AidCommonPara.ucThresholdExistFlag = 1;
		TermParam.AidCommonPara.ulThresholdValue = pHxTermParam->AidCommonPara.ulThresholdValue;
	}
	if(pHxTermParam->AidCommonPara.ucECashSupport == 0xFF)
		TermParam.AidCommonPara.cECashSupport = -1;
	else
		TermParam.AidCommonPara.cECashSupport = pHxTermParam->AidCommonPara.ucECashSupport;
	memcpy(TermParam.AidCommonPara.szTermECashTransLimit, pHxTermParam->AidCommonPara.szTermECashTransLimit, 12);
	if(pHxTermParam->AidCommonPara.ucTacDefaultExistFlag) {
		TermParam.AidCommonPara.ucTacDefaultExistFlag = 1;
		memcpy(TermParam.AidCommonPara.sTacDefault, pHxTermParam->AidCommonPara.sTacDefault, 5);
	}
	if(pHxTermParam->AidCommonPara.ucTacDenialExistFlag) {
		TermParam.AidCommonPara.ucTacDenialExistFlag = 1;
		memcpy(TermParam.AidCommonPara.sTacDenial, pHxTermParam->AidCommonPara.sTacDenial, 5);
	}
	if(pHxTermParam->AidCommonPara.ucTacOnlineExistFlag) {
		TermParam.AidCommonPara.ucTacOnlineExistFlag = 1;
		memcpy(TermParam.AidCommonPara.sTacOnline, pHxTermParam->AidCommonPara.sTacOnline, 5);
	}

	iRet = iHxSetTermParam(&TermParam, szErrTag);
	return(iRet);
}

// װ���ն�֧�ֵ�Aid
// in  : paHxTermAid       : �ն�֧�ֵ�Aid����
//       iHxTermAidNum     : �ն�֧�ֵ�Aid����
// ret : HXEMV_OK          : OK
//       HXEMV_LACK_MEMORY : �洢�ռ䲻��
//       HXEMV_PARA        : ��������
//       HXEMV_FLOW_ERROR  : EMV���̴���
int iHxEmvLoadAid(stHxTermAid *paHxTermAid, int iHxTermAidNum)
{
	stTermAid TermAid;
	uchar szErrTag[100];
	int   i, iRet;
	
	if(iHxTermAidNum < 1)
		return(HXEMV_PARA);
	memset(&TermAid, 0, sizeof(TermAid));
	iRet = iHxLoadTermAid(&TermAid, HXEMV_PARA_INIT, szErrTag);
	if(iRet)
		return(iRet);
	for(i=0; i<iHxTermAidNum; i++) {
		memset(&TermAid, 0, sizeof(TermAid));
		TermAid.ucAidLen = paHxTermAid[i].ucAidLen;
		if(TermAid.ucAidLen<5 || TermAid.ucAidLen>16)
			return(HXEMV_PARA);
		memcpy(TermAid.sAid, paHxTermAid[i].sAid, TermAid.ucAidLen);
		TermAid.ucASI = paHxTermAid[i].ucASI;

		// ����Ϊ���ն˹��ò���
		TermAid.cForcedOnlineSupport = -1;
		TermAid.cForcedAcceptanceSupport = -1;
		TermAid.ucTermRiskDataLen = 0;

		TermAid.iDefaultDDOLLen = paHxTermAid[i].iDefaultDDOLLen;
		if(TermAid.iDefaultDDOLLen >= 0) {
			if(TermAid.iDefaultDDOLLen>sizeof(TermAid.sDefaultDDOL) || TermAid.iDefaultDDOLLen>sizeof(paHxTermAid[i].sDefaultDDOL))
				return(HXEMV_PARA);
			memcpy(TermAid.sDefaultDDOL, paHxTermAid[i].sDefaultDDOL, TermAid.iDefaultDDOLLen);
		} else {
			TermAid.iDefaultDDOLLen = -1;
		}

		TermAid.iDefaultTDOLLen = paHxTermAid[i].iDefaultTDOLLen;
		if(TermAid.iDefaultTDOLLen >= 0) {
			if(TermAid.iDefaultTDOLLen>sizeof(TermAid.sDefaultTDOL) || TermAid.iDefaultTDOLLen>sizeof(paHxTermAid[i].sDefaultTDOL))
				return(HXEMV_PARA);
			memcpy(TermAid.sDefaultTDOL, paHxTermAid[i].sDefaultTDOL, TermAid.iDefaultTDOLLen);
		} else {
			TermAid.iDefaultTDOLLen = -1;
		}

		if(paHxTermAid[i].cOnlinePinSupport == 0)
			TermAid.cOnlinePinSupport = 0;
		else
			TermAid.cOnlinePinSupport = 1; // �������Ϊȱʡֵ, Ҳ��Ϊ֧��

		if(memcmp(paHxTermAid[i].sTermAppVer, "\xFF\xFF", 2) != 0) {
			TermAid.ucTermAppVerExistFlag = 1;
			memcpy(TermAid.sTermAppVer, paHxTermAid[i].sTermAppVer, 2);
		}
		if(paHxTermAid[i].ulFloorLimit != 0xFFFFFFFFL) {
			TermAid.ucFloorLimitExistFlag = 1;
			TermAid.ulFloorLimit = paHxTermAid[i].ulFloorLimit;
		}
		TermAid.iMaxTargetPercentage = paHxTermAid[i].iMaxTargetPercentage;
		TermAid.iTargetPercentage = paHxTermAid[i].iTargetPercentage;
		if(paHxTermAid[i].ulThresholdValue != 0xFFFFFFFFL) {
			TermAid.ucThresholdExistFlag = 1;
			TermAid.ulThresholdValue = paHxTermAid[i].ulThresholdValue;
		}
		if(paHxTermAid[i].ucECashSupport == 0xFF)
			TermAid.cECashSupport = -1;
		else
			TermAid.cECashSupport = paHxTermAid[i].ucECashSupport;
		memcpy(TermAid.szTermECashTransLimit, paHxTermAid[i].szTermECashTransLimit, 12);
		if(paHxTermAid[i].ucTacDefaultExistFlag) {
			TermAid.ucTacDefaultExistFlag = 1;
			memcpy(TermAid.sTacDefault, paHxTermAid[i].sTacDefault, 5);
		}
		if(paHxTermAid[i].ucTacDenialExistFlag) {
			TermAid.ucTacDenialExistFlag = 1;
			memcpy(TermAid.sTacDenial, paHxTermAid[i].sTacDenial, 5);
		}
		if(paHxTermAid[i].ucTacOnlineExistFlag) {
			TermAid.ucTacOnlineExistFlag = 1;
			memcpy(TermAid.sTacOnline, paHxTermAid[i].sTacOnline, 5);
		}

		iRet = iHxLoadTermAid(&TermAid, HXEMV_PARA_ADD, szErrTag);
		if(iRet)
			return(iRet);
	}
	return(HXEMV_OK);
}

// װ��CA��Կ
// in  : paHxCaPublicKey   : CA��Կ
//       iHxCaPublicKeyNum : CA��Կ����
// ret : HXEMV_OK          : OK
//       HXEMV_LACK_MEMORY : �洢�ռ䲻��
//       HXEMV_PARA        : ��������
//       HXEMV_FLOW_ERROR  : EMV���̴���
int iHxEmvLoadCaPublicKey(stHxCaPublicKey *paHxCaPublicKey, int iHxCaPublicKeyNum)
{
	stCAPublicKey CAPublicKey;
	int   i, iRet;
	
	if(iHxCaPublicKeyNum < 1)
		return(HXEMV_PARA);
	memset(&CAPublicKey, 0, sizeof(CAPublicKey));
	iRet = iHxLoadCAPublicKey(&CAPublicKey, HXEMV_PARA_INIT);
	if(iRet)
		return(iRet);
	for(i=0; i<iHxCaPublicKeyNum; i++) {
		memset(&CAPublicKey, 0, sizeof(CAPublicKey));
		CAPublicKey.ucKeyLen = paHxCaPublicKey[i].ucKeyLen;
		if(CAPublicKey.ucKeyLen > 248)
			return(HXEMV_PARA);
		memcpy(CAPublicKey.sKey, paHxCaPublicKey[i].sKey, CAPublicKey.ucKeyLen);
		CAPublicKey.lE = paHxCaPublicKey[i].lE;
		memcpy(CAPublicKey.sRid, paHxCaPublicKey[i].sRid, 5);
		CAPublicKey.ucIndex = paHxCaPublicKey[i].ucIndex;
		strcpy(CAPublicKey.szExpireDate, "20491231");
		memset(CAPublicKey.sHashCode, '0', 20);
		iRet = iHxLoadCAPublicKey(&CAPublicKey, HXEMV_PARA_ADD);
		if(iRet)
			return(iRet);
	}
	return(HXEMV_OK);
}

// ���׳�ʼ��
// in  : iFlag             : =0, �����Ժ�ʹ��
// ret : HXEMV_OK          : OK
//       HXEMV_FLOW_ERROR  : EMV���̴���
//       HXEMV_CARD_REMOVED: ����ȡ��
//       HXEMV_NO_CARD     : ��Ƭ������
//       HXEMV_CARD_OP     : ��Ƭ��λ����
int iHxEmvTransInit(int iFlag)
{
	int iRet, iErrMsgType;
	iRet = iHxTransInit();
	if(iRet)
		return(iRet);
	iRet = iHxTestCard();
	if(iRet)
		return(iRet);
	iRet = iHxResetCard2(&iErrMsgType);
	return(iRet);
}

// �رտ�Ƭ
// ret : HXEMV_OK       : OK
int iHxEmvCloseCard(void)
{
	iHxCloseCard();
	return(HXEMV_OK);
}

// ��ȡ֧�ֵ�Ӧ��
// in  : iIgnoreBlock	   : !0:����Ӧ��������־, 0:������Ӧ�ò��ᱻѡ��
//       piAdfNum          : �����ɵ��ն��뿨Ƭͬʱ֧�ֵ�Adf����
// out : paHxAdfInfo       : �ն��뿨Ƭͬʱ֧�ֵ�Adf�б�(�����ȼ�����)
//       piHxAdfNum        : �ն��뿨Ƭͬʱ֧�ֵ�Adf����
// ret : HXEMV_OK          : OK, ��ȡ��Ӧ�ñ���ȷ��
//       HXEMV_AUTO_SELECT : OK, ��ȡ��Ӧ�ÿ��Զ�ѡ��
//       HXEMV_NO_APP      : ��֧�ֵ�Ӧ��
//       HXEMV_CARD_REMOVED: ����ȡ��
//       HXEMV_CARD_SW     : ��״̬�ַǷ�
//       HXEMV_CARD_OP     : ��������
//       HXEMV_TERMINATE   : ����ܾ�������������ֹ
//       HXEMV_FLOW_ERROR  : EMV���̴���
//       HXEMV_LACK_MEMORY : �洢�ռ䲻��
//       HXEMV_PARA        : ��������
int iHxEmvGetSupportedApp(int iIgnoreBlock, stHxAdfInfo *paHxAdfInfo, int *piHxAdfNum)
{
	int  iRet;
	stADFInfo *paAdfInfo;
	int  iMsgType, iErrMsgType;
	int  i;

	if(*piHxAdfNum<1 || *piHxAdfNum>100)
		return(HXEMV_PARA);
	paAdfInfo = malloc(*piHxAdfNum * sizeof(stADFInfo));
	if(paAdfInfo == NULL)
		return(HXEMV_LACK_MEMORY);
	iRet = iHxGetSupportedApp2(iIgnoreBlock, paAdfInfo, piHxAdfNum, &iMsgType, &iErrMsgType);
	if(iRet!=HXEMV_OK && iRet!=HXEMV_AUTO_SELECT) {
		free(paAdfInfo);
		return(iRet);
	}
	for(i=0; i<*piHxAdfNum; i++) {
		memset(&paHxAdfInfo[i], 0, sizeof(stHxAdfInfo));
		paHxAdfInfo[i].ucAdfNameLen = paAdfInfo[i].ucAdfNameLen;
		memcpy(paHxAdfInfo[i].sAdfName, paAdfInfo[i].sAdfName, paHxAdfInfo[i].ucAdfNameLen);
		strcpy(paHxAdfInfo[i].szLabel, paAdfInfo[i].szLabel);
		paHxAdfInfo[i].iPriority = paAdfInfo[i].iPriority;
		strcpy(paHxAdfInfo[i].szLanguage, paAdfInfo[i].szLanguage);
		paHxAdfInfo[i].iIssuerCodeTableIndex = paAdfInfo[i].iIssuerCodeTableIndex;
		strcpy(paHxAdfInfo[i].szPreferredName, paAdfInfo[i].szPreferredName);
	}
	free(paAdfInfo);
	return(iRet);
}

// Ӧ��ѡ��
// in  : iIgnoreBlock	     : !0:����Ӧ��������־, 0:������Ӧ�ò��ᱻѡ��
//       iAidLen             : AID����
//       psAid               : AID
// ret : HXEMV_OK            : OK
//       HXEMV_CARD_REMOVED  : ����ȡ��
//       HXEMV_CARD_OP       : ��������
//       HXEMV_TERMINATE     : ����ܾ�������������ֹ
//       HXEMV_FLOW_ERROR    : EMV���̴���
//       HXEMV_CORE          : �ڲ�����
//       HXEMV_RESELECT      : ��Ҫ����ѡ��Ӧ��
//       HXEMV_NOT_SUPPORTED : ѡ���˲�֧�ֵ�Ӧ��
int iHxEmvAppSelect(int iIgnoreBlock, int iAidLen, uchar *psAid)
{
	int iRet;
	int iErrMsgType;

	iRet = iHxAppSelect2(iIgnoreBlock, iAidLen, psAid, &iErrMsgType);
	return(iRet);
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
//       ����һ��������, ��ŵ�Tlv���ݿ���, �ٴ�ʹ�ÿ�����iHxEmvGetData()��ȡ
int iHxEmvGetCardNativeData(uchar *psTag, int *piOutTlvDataLen, uchar *psOutTlvData, int *piOutDataLen, uchar *psOutData)
{
	int iRet;
	iRet = iHxGetCardNativeData(psTag, piOutTlvDataLen, psOutTlvData, piOutDataLen, psOutData);
	return(iRet);
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
int iHxEmvGetData(uchar *psTag, int *piOutTlvDataLen, uchar *psOutTlvData, int *piOutDataLen, uchar *psOutData)
{
	int iRet;
	iRet = iHxGetData(psTag, piOutTlvDataLen, psOutTlvData, piOutDataLen, psOutData);
	return(iRet);
}

// ����Ƭ������ˮ֧����Ϣ
// in  : iFlag             : 0:��׼������ˮ 1:Ȧ����ˮ
// out : piMaxRecNum       : ��ཻ����ˮ��¼����
// ret : HXEMV_OK          : ��ȡ�ɹ�
//       HXEMV_NO_LOG      : ��Ƭ��֧�ֽ�����ˮ��¼
//       HXEMV_PARA        : ��������
//       HXEMV_FLOW_ERROR  : EMV���̴���
int iHxEmvGetLogInfo(int iFlag, int *piMaxRecNum)
{
	int iRet;
	switch(iFlag) {
	case 0:
		iRet = iHxGetLogInfo(piMaxRecNum);
		break;
	case 1:
		iRet = iHxGetLoadLogInfo(piMaxRecNum);
		break;
	default:
		return(HXEMV_PARA);
	}
	return(iRet);
}

// ����Ƭ������ˮ
// in  : iFlag             : 0:��׼������ˮ 1:Ȧ����ˮ
//       iLogNo            : ������ˮ��¼��, �����һ����¼��Ϊ1
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
//       HXEMV_PARA        : ��������
//       HXEMV_FLOW_ERROR  : EMV���̴���
int iHxEmvReadLog(int iFlag, int iLogNo, int *piLogLen, uchar *psLog)
{
	int iRet;
	switch(iFlag) {
	case 0:
		iRet = iHxReadLog(iLogNo, piLogLen, psLog);
		break;
	case 1:
		iRet = iHxReadLoadLog(iLogNo, piLogLen, psLog);
		break;
	default:
		return(HXEMV_PARA);
	}
	return(iRet);
}

// GPO
// in  : pszTransTime      : ����ʱ�䣬YYYYMMDDhhmmss
//       ulTTC             : �ն˽������, 1-999999
//       ucTransType       : ��������,������ǰ2λ��ʾ�Ľ��ڽ�������
//                           �����ֽ��ֵ63��ʾΪ0x63
//       pszAmount         : ���׽��, ascii�봮, ��ҪС����, Ĭ��ΪȱʡС����λ��, ����:RMB1.00��ʾΪ"100"
//       uiCurrencyCode    : ���Ҵ���, 156=�����
// ret : HXEMV_OK          : OK
//       HXEMV_CARD_REMOVED: ����ȡ��
//       HXEMV_CARD_OP     : ��������
//       HXEMV_CARD_SW     : �Ƿ���״̬��
//       HXEMV_TERMINATE   : ����ܾ�������������ֹ
//       HXEMV_RESELECT    : ��Ҫ����ѡ��Ӧ��
//       HXEMV_TRANS_NOT_ALLOWED : ���ײ�֧��
//       HXEMV_PARA        : ��������
//       HXEMV_FLOW_ERROR  : EMV���̴���
//       HXEMV_CORE        : �ڲ�����
// �ǽӿ��ܷ�����
//       HXEMV_DENIAL	   : qPboc�ܾ�
//		 HXEMV_TRY_OTHER_INTERFACE	: ��������ͨ�Ž���
//		 HXEMV_CARD_TRY_AGAIN		: Ҫ�������ύ��Ƭ(�ǽ�)
// Note: �������HXEMV_RESELECT����iHxEmvGetSupportedApp()��ʼ����ִ������
int iHxEmvGPO(uchar *pszTransTime, ulong ulTTC, uchar ucTransType, uchar *pszAmount, uint uiCurrencyCode)
{
	int iRet;
	int iMsgType, iErrMsgType;

	iRet = iHxGPO2(pszTransTime, ulTTC, (uint)ucTransType, pszAmount, uiCurrencyCode, &iMsgType, &iErrMsgType);
	return(iRet);
}

// ��������´�����
// in  : pszAmount         : ���
// ret : HXEMV_OK          : ���óɹ�
//       HXEMV_FLOW_ERROR  : EMV���̴���
//       HXEMV_PARA        : ��������
// note: GPO��GAC�����ܻ᲻ͬ, �����趨���׽��
//       ����¼��, ��������ǰ�ɵ���
int iHxEmvSetAmount(uchar *pszAmount)
{
	int iRet;

	iRet = iHxSetAmount(pszAmount, "0");
	return(iRet);
}

// ��Ӧ�ü�¼
// ret : HXEMV_OK          : OK
//       HXEMV_CARD_REMOVED: ����ȡ��
//       HXEMV_CARD_OP     : ��������
//       HXEMV_CARD_SW     : �Ƿ���ָ��״̬��
//       HXEMV_TERMINATE   : ����ܾ�������������ֹ
//       HXEMV_FLOW_ERROR  : EMV���̴���
//       HXEMV_CORE        : �ڲ�����
// �ǽӿ��ܷ�����
//		 HXEMV_TRY_OTHER_INTERFACE	: ��������ͨ�Ž���
int iHxEmvReadRecord(void)
{
	int iRet;
	int iErrMsgType;

	iRet = iHxReadRecord2(NULL, NULL, NULL, &iErrMsgType);
	return(iRet);
}

// �����˻����������ֿ�������Ϣ
// in  : iBlackFlag        : ���øÿ��Ƿ�Ϊ��������, 0:��Ϊ�������� 1:��������
//       iSeparateFlag     : ���øÿ��ۼ����ѳ��޶�, 0:û���޶� 1:���޶�
// ret : HXEMV_OK          : OK
//       HXEMV_CORE        : �ڲ�����
// Note: �������¼���ֿ�Ƭ�Ǻ����������ۼ����ѳ��޶�, ����Ҫ���ô˺��������˻���Ϣ
//       �����֧�ֻ��鲻�����ñ��, ����Ҫ���ñ��ӿ�
int iHxEmvSetPanFlag(int iBlackFlag, int iSeparateFlag)
{
	int   iRet;
	uchar szRecentAmount[13];
	int iErrMsgType;
	
	if(iSeparateFlag)
		strcpy(szRecentAmount, "999999999999"); // ͨ�������ֶ�ʹ�������÷ֿ����ѳ��޶�
	else
		strcpy(szRecentAmount, "000000000000");
	iRet = iHxSetPanInfo2(iBlackFlag, szRecentAmount, &iErrMsgType);
	return(iRet);
}

// SDA��DDA
// out : piNeedCheckCrlFlag  : �Ƿ���Ҫ�жϷ����й�Կ֤��Ϊ��������־, 0:����Ҫ�ж� 1:��Ҫ�ж�
//       psRid               : RID[5], psRid+pucCaIndex+psCertSerNo�������ṩ��Ӧ�ò��жϷ����й�Կ֤���Ƿ��ں������б���. �κ�һ��ָ�봫��Ϊ�ն���ʾ�߲㲻��Ҫ
//       pucCaIndex          : CA��Կ����
//       psCertSerNo         : �����й�Կ֤�����к�[3]
// ret : HXEMV_OK            : OK
//       HXEMV_CARD_REMOVED  : ����ȡ��
//       HXEMV_CARD_OP       : ��������
//       HXEMV_CARD_SW       : �Ƿ���״̬��
//       HXEMV_TERMINATE     : ����ܾ�������������ֹ
//       HXEMV_DENIAL        : �ն���Ϊ�������Ϊ�ѻ��ܾ�
//       HXEMV_DENIAL_ADVICE : �ն���Ϊ�������Ϊ�ѻ��ܾ�, ��Advice
//       HXEMV_FLOW_ERROR    : EMV���̴���
//       HXEMV_CORE          : �ڲ�����
// �ǽӿ��ܷ�����
//		 HXEMV_FDDA_FAIL	 : fDDAʧ��(�ǽ�)
// Note: Ӧ�ò�֧�ַ����й�Կ֤����������ʱ, piNeedCheckCrlFlag��������, ������Ժ���
//       ����һ��ָ�봫��Ϊ�ձ�ʾ����CRL���
int iHxEmvOfflineDataAuth(int *piNeedCheckCrlFlag, uchar *psRid, uchar *pucCaIndex, uchar *psCertSerNo)
{
	int   iRet;
	uchar sRid[5], ucCaIndex, sCertSerNo[3];
	int   iNeedCheckCrlFlag;
	int   iErrMsgType;

	iRet = iHxOfflineDataAuth2(&iNeedCheckCrlFlag, sRid, &ucCaIndex, sCertSerNo, &iErrMsgType);
	if(iRet)
		return(iRet);
	if(piNeedCheckCrlFlag && psRid && pucCaIndex && psCertSerNo) {
		// �߲�Ҫ�󴫻�CRL�����Ϣ
		*piNeedCheckCrlFlag = iNeedCheckCrlFlag;
		if(iNeedCheckCrlFlag) {
			memcpy(psRid, sRid, 5);
			*pucCaIndex = ucCaIndex;
			memcpy(psCertSerNo, sCertSerNo, 3);
		}
	}
	return(HXEMV_OK);
}

// ���÷����й�Կ֤��Ϊ������
// in  : iIssuerCertCrlFlag  : �Ƿ��ں������б��־, 0:���ں������б� 1:�ں������б�
// ret : HXEMV_OK            : OK
//       HXEMV_CARD_REMOVED  : ����ȡ��
//       HXEMV_CARD_OP       : ��������
//       HXEMV_CARD_SW       : �Ƿ���״̬��
//       HXEMV_TERMINATE     : ����ܾ�������������ֹ
//       HXEMV_DENIAL        : �ն���Ϊ�������Ϊ�ѻ��ܾ�
//       HXEMV_DENIAL_ADVICE : �ն���Ϊ�������Ϊ�ѻ��ܾ�, ��Advice
//       HXEMV_FLOW_ERROR    : EMV���̴���
//       HXEMV_CORE          : �ڲ�����
// Note: Ӧ�ò����iHxEmvOfflineDataAuth()��õ�Rid+CaIndex+CertSerNo
//       ���ݴ������жϷ����й�Կ֤���Ƿ�Ϊ������,ֻ���Ǻ�����,����Ҫ���ñ�����֪ͨ����
int iHxEmvSetIssuerCertCrl(int iIssuerCertCrlFlag)
{
	int   iRet;
	int   iErrMsgType;

	iRet = HXEMV_OK;
	if(iIssuerCertCrlFlag)
		iRet = iHxSetIssuerCertCrl2(&iErrMsgType);
	return(iRet);
}

// ��������
// ret : HXEMV_OK            : OK
//       HXEMV_TERMINATE     : ����ܾ�������������ֹ
//       HXEMV_FLOW_ERROR    : EMV���̴���
//       HXEMV_CORE          : �ڲ�����
//       HXEMV_CARD_REMOVED  : ����ȡ��
//       HXEMV_DENIAL        : �ն���Ϊ�������Ϊ�ѻ��ܾ�
//       HXEMV_DENIAL_ADVICE : �ն���Ϊ�������Ϊ�ѻ��ܾ�, ��Advice
//       HXEMV_CARD_OP       : �ն���Ϊ�������Ϊ�ѻ��ܾ�, ����AACʱ��Ƭ����
int iHxEmvProcRistrictions(void)
{
	int   iRet;
	int   iErrMsgType;

	iRet = iHxProcRistrictions2(&iErrMsgType);
	return(iRet);
}

// �ն˷��չ���
// ret : HXEMV_OK            : OK
//       HXEMV_TERMINATE     : ����ܾ�������������ֹ
//       HXEMV_CORE          : ��������
//       HXEMV_CARD_REMOVED  : ����ȡ��
//       HXEMV_CARD_OP       : ��������
//       HXEMV_DENIAL        : �ն���Ϊ�������Ϊ�ѻ��ܾ�
//       HXEMV_DENIAL_ADVICE : �ն���Ϊ�������Ϊ�ѻ��ܾ�, ��Advice
//       HXEMV_FLOW_ERROR    : EMV���̴���
int iHxEmvTermRiskManage(void)
{
	int   iRet;
	int   iErrMsgType;

	iRet = iHxTermRiskManage2(&iErrMsgType);
	return(iRet);
}

// ��ȡ�ֿ�����֤
// out : piCvm               : �ֿ�����֤����, ֧��HXCVM_PLAIN_PIN��HXCVM_CIPHERED_ONLINE_PIN��HXCVM_HOLDER_ID��HXCVM_CONFIRM_AMOUNT
//       piBypassFlag        : ����bypass��־, 0:������, 1:����
// ret : HXEMV_OK            : OK
//       HXEMV_NO_DATA       : ����������гֿ�����֤
//       HXEMV_CARD_REMOVED  : ����ȡ��
//       HXEMV_CARD_OP       : ��������
//       HXEMV_TERMINATE     : ����ܾ�������������ֹ
//       HXEMV_DENIAL        : �ն���Ϊ�������Ϊ�ѻ��ܾ�
//       HXEMV_DENIAL_ADVICE : �ն���Ϊ�������Ϊ�ѻ��ܾ�, ��Advice
//       HXEMV_FLOW_ERROR    : EMV���̴���
//       HXEMV_CORE          : �ڲ�����
// Note: ��iHxEmvDoCvmMethod()����, ������Ҫ��ε���, ֱ������HXEMV_NO_DATA
int iHxEmvGetCvmMethod(int *piCvm, int *piBypassFlag)
{
	int   iRet;
	int   iMsgType, iMsgType2, iErrMsgType;
	uchar szAmountStr[50];

	iRet = iHxGetCvmMethod2(piCvm, piBypassFlag, &iMsgType, &iMsgType2, szAmountStr, &iErrMsgType);
	return(iRet);
}

// ִ�гֿ�����֤
// in  : iCvmProc            : ��֤��������ʽ, HXCVM_PROC_OK or HXCVM_BYPASS or HXCVM_FAIL or HXCVM_CANCEL or HXCVM_TIMEOUT
//       psCvmData           : ���������, ���Ϊ��������, ����β��Ҫ��0
// out : piPrompt            : ������ʾ��Ϣ, 0:�޶�����ʾ��Ϣ 1:�����,������ 2:�����,�������� 3:�ѻ�������֤�ɹ�
// ret : HXEMV_OK            : OK, ��Ҫ�������гֿ�����֤, ��������iHxGetCvmMethod2(), Ȼ���ٵ��ñ�����
//       HXEMV_PARA		     : ��������
//       HXEMV_CARD_REMOVED  : ����ȡ��
//       HXEMV_CARD_OP       : ��������
//       HXEMV_CARD_SW       : �Ƿ���״̬��
//       HXEMV_TERMINATE     : ����ܾ�������������ֹ
//       HXEMV_DENIAL        : �ն���Ϊ�������Ϊ�ѻ��ܾ�
//       HXEMV_DENIAL_ADVICE : �ն���Ϊ�������Ϊ�ѻ��ܾ�, ��Advice
//       HXEMV_FLOW_ERROR    : EMV���̴���
//       HXEMV_CORE          : �ڲ�����
// Note: ִ�е�CVM���������һ��iHxEmvGetCvmMethod()��õ�CVM
int iHxEmvDoCvmMethod(int iCvmProc, uchar *psCvmData, int *piPrompt)
{
	int   iRet;
	int   iMsgType, iMsgType2, iErrMsgType;

	*piPrompt = 0; // ����û�ж�����Ϣ��ʾ
	iRet = iHxDoCvmMethod2(iCvmProc, psCvmData, &iMsgType, &iMsgType2, &iErrMsgType);
	if(iMsgType == EMVMSG_0A_INCORRECT_PIN) {
		// �����, ��Ҫ������ʾ
		if(iMsgType2 == EMVMSG_13_TRY_AGAIN)
			*piPrompt = 1; // �����, ������
		else
			*piPrompt = 2; // �����, ��������
	} else if(iMsgType == EMVMSG_0D_PIN_OK) {
		*piPrompt = 3;
	}
	return(iRet);
}

// ��ȡCVM��֤����ǩ�ֱ�־
// out : piNeedSignFlag    : ��ʾ��Ҫǩ�ֱ�־��0:����Ҫǩ�� 1:��Ҫǩ��
// ret : HXEMV_OK          : OK
int iHxEmvGetCvmSignFlag(int *piNeedSignFlag)
{
	int iRet;

	iRet = iHxGetCvmSignFlag2(piNeedSignFlag);
	return(iRet);
}

// �ն���Ϊ����
// ret : HXEMV_OK            : OK
//       HXEMV_CORE          : ��������
//       HXEMV_FLOW_ERROR    : EMV���̴���
//       HXEMV_CARD_REMOVED  : ����ȡ��
//       HXEMV_DENIAL        : �ն���Ϊ�������Ϊ�ѻ��ܾ�
//       HXEMV_DENIAL_ADVICE : �ն���Ϊ�������Ϊ�ѻ��ܾ�, ��Advice
int iHxEmvTermActionAnalysis(void)
{
	int iRet;
	int iErrMsgType;

	iRet = iHxTermActionAnalysis2(&iErrMsgType);
	return(iRet);
}

// ��һ��GAC
// in  : ucForcedOnline    : 1:�趨ǿ��������־ 0:���趨ǿ��������־
// out : piCardAction      : ��Ƭִ�н��
//								GAC_ACTION_TC     : ��׼(����TC)
//								GAC_ACTION_AAC    : �ܾ�(����AAC)
//								GAC_ACTION_AAC_ADVICE : �ܾ�(����AAC,��Advice)
//								GAC_ACTION_ARQC   : Ҫ������(����ARQC)
// ret : HXEMV_OK          : OK
//       HXEMV_CORE        : ��������
//       HXEMV_CARD_REMOVED: ����ȡ��
//       HXEMV_CARD_OP     : ��������
//       HXEMV_CARD_SW     : �Ƿ�״̬��
//       HXEMV_TERMINATE   : ����ܾ�������������ֹ
//       HXEMV_NOT_ALLOWED : ��������
//       HXEMV_NOT_ACCEPTED: ������
//       HXEMV_FLOW_ERROR  : EMV���̴���
int iHxEmvGac1(uchar ucForcedOnline , int *piCardAction)
{
	int iRet;
	int iMsgType, iErrMsgType;
	int iFlag;

	if(ucForcedOnline)
		iFlag = 1;
	else
		iFlag = 0;
	iHxSetForceOnlineFlag(iFlag);
	iRet = iHx1stGAC2(piCardAction, &iMsgType, &iErrMsgType);
	return(iRet);
}

// �ڶ���GAC
// in  : pszArc            : Ӧ����[2], NULL��""��ʾ����ʧ��
//       pszAuthCode	   : ��Ȩ��[6], NULL��""��ʾ����Ȩ������
//       psOnlineData      : ������Ӧ����(55������),һ��TLV��ʽ����, NULL��ʾ��������Ӧ����
//       psOnlineDataLen   : ������Ӧ���ݳ���
// out : piCardAction      : ��Ƭִ�н��
//                           GAC_ACTION_TC     : ��׼(����TC)
//							     GAC_ACTION_AAC    : �ܾ�(����AAC)
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
int iHxEmvGac2 (uchar *pszArc, uchar *pszAuthCode, uchar *psIssuerData, int iIssuerDataLen, int *piCardAction)
{
	int iRet;
	int iMsgType, iErrMsgType;

	iRet = iHx2ndGAC2(pszArc, pszAuthCode, psIssuerData, iIssuerDataLen, piCardAction, &iMsgType, &iErrMsgType);
	return(iRet);
}
