/**************************************
File name     : EmvIO.c
Function      : EMV/pboc2.0�������ģ��
                �����ն����͡����ܾ�����������(����Ա|�ֿ���)
				���лص�����(EmvFace.h����)��ͨ����ģ�����
Author        : Yu Jun
First edition : Apr 13th, 2012
Modified      : Jan 21st, 2013
	                ���ݺ��ĵ��÷�ʽ(����gl_iCoreCallFlag)�����Ƿ���ûص�����
					ֻ�к��ĵ��÷�ʽΪHXCORE_CALLBACKʱ�ŵ��ûص�����
**************************************/
/*
ģ����ϸ����:
	EMV�������ⲿ�������м��
.	ֻ���Իص���ʽ��ʼ������, ��ģ��Ż����EmvFace.h�������Ļص�����
.	�ص���ʽʱ, ��ģ������ն�����, ������ʾ����, ��ȷ����������
.	��¼�ֿ���ѡ�������
		sg_szSelectedLang[3];  // �ֿ���ѡ�������, ���GPO����ѡӦ��, ������Ҫ��ֿ���ѡ����
.	�ǻص���ʽʱ, ��¼���Ľӿڴ�����˺���Ϣ, �����������ֿ�����ʱ��Ҫ
		sg_iBlackFlag;         // ��������־, 0:���Ǻ����� 1:�Ǻ�����
		sg_szRecentAmount[13]; // �����ǽ��
.	�ǻص���ʽʱ, ��¼������֤�����кŹ����Ļ�ȡ���жϸ�֤���Ƿ��ѱ��ջ�
		sg_sCertSerNo[3];      // �ǻص���ʽ��ͨ����ģ�鱣�淢���й�Կ֤�����к��Թ�Ӧ�ò��ȡ
.	�ǻص���ʽʱ, ����GPO����Ľ��
		sg_szAmount[13];       // �ǻص���ʽ����ͨ����ģ���ȡ���, ����GPO����Ľ��
		sg_szAmountOther[13];  // �ǻص���ʽ����ͨ����ģ���ȡ�������, ����GPO������������
	****** ע��:δ����GAC��GPO��ͬ��� ******
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "VposFace.h"
#include "Common.h"
#include "TlvFunc.h"
#include "Iso4217.h"
#include "EmvFunc.h"
#include "EmvTrace.h"
#include "TagDef.h"
#include "EmvMsg.h"
#include "EmvFace.h"
#include "EmvIo.h"
#include "EmvData.h"
#include "EmvCore.h"

static uchar sg_szAmount[13];       // �ǻص���ʽ����ͨ����ģ���ȡ���, ����GPO����Ľ��
static uchar sg_szAmountOther[13];  // �ǻص���ʽ����ͨ����ģ���ȡ�������, ����GPO������������

static uchar sg_iBlackFlag;         // ��������־, 0:���Ǻ����� 1:�Ǻ�����
static uchar sg_szRecentAmount[13]; // �����ǽ��

static uchar sg_szSelectedLang[3];  // �ֿ���ѡ�������, ���GPO����ѡӦ��, ������Ҫ��ֿ���ѡ����

static uchar sg_sCertSerNo[3];      // �ǻص���ʽ��ͨ����ģ�鱣�淢���й�Կ֤�����к��Թ�Ӧ�ò��ȡ

// ��ʼ���������ģ��
int iEmvIOInit(void)
{
	strcpy(sg_szAmount, "0");
	strcpy(sg_szAmountOther, "0");
	sg_iBlackFlag = 0;
	strcpy(sg_szRecentAmount, "0");
	memset(sg_sCertSerNo, 0xFF, sizeof(sg_sCertSerNo));
	sg_szSelectedLang[0] = 0;	
	return(EMVIO_OK);
}

// ���ý��׽��
// in  : pszAmount      : ���
//       pszAmountOther : �������
// ret : EMVIO_OK       : OK
//       EMVIO_ERROR    : ��������
int iEmvIOSetTransAmount(uchar *pszAmount, uchar *pszAmountOther)
{
	if(strlen((char *)pszAmount) > 12)
		return(EMVIO_ERROR);
	strcpy(sg_szAmount, pszAmount);
	if(strlen((char *)pszAmountOther) > 12)
		return(EMVIO_ERROR);
	strcpy(sg_szAmountOther, pszAmountOther);
	return(EMVIO_OK);
}

// �����˺���Ϣ
// in  : iBlackFlag      : ����������־, 0:���Ǻ����� 1:������
//       pszRecentAmount : �����ǽ��
// ret : EMVIO_OK        : OK
//       EMVIO_ERROR     : ��������
int iEmvIOSetPanInfo(int iBlackFlag, uchar *pszRecentAmount)
{
	if(iBlackFlag)
		sg_iBlackFlag = 1;
	else
		sg_iBlackFlag = 0;
	if(strlen((char *)pszRecentAmount) > 12)
		return(EMVIO_ERROR);
	strcpy(sg_szRecentAmount, pszRecentAmount);
	return(EMVIO_OK);
}

// ��ȡ�����й�Կ֤�����к�
// out : psCertSerNo    : �����й�Կ֤�����к�
// ret : EMVIO_OK       : OK
//       EMVIO_ERROR    : ��������
int iEmvIOGetCertSerNo(uchar *psCertSerNo)
{
	memcpy(psCertSerNo, sg_sCertSerNo, 3);
	return(EMVIO_OK);
}

// ѡ������
// out : pszSelectedLang : ѡ�е����Դ���, ����:"zh"
//       pszLangs        : ��ѡ�������б�, ����:"frzhen"
//       pszLangsDesc    : ��ѡ�����������б�, '='�Ž�β, ����:"FRANCE=����=ENGLISH="
// ret : EMVIO_OK        : OK
//       EMVIO_CANCEL    : ȡ��
//       EMVIO_TIMEOUT   : ��ʱ
//       EMVIO_ERROR     : ��������
// Note: ��֧�ֲ���Աѡ������, ��ѡ������Ϊѡ��ֿ�������
//       ����attended�ն�, Ҫ��pinpad֧��������ѡ
//       ����unattended�ն�, ҲҪ��pinpad֧��������ѡ, ���unattended�ն˱�������pinpad���Ե�ͬ���ն�֧������
int iEmvIOSelectLang(uchar *pszSelectedLang, uchar *pszLangs, uchar *pszLangsDesc)
{
    int   iRet;
	uchar szTermLangs[33], szTermLangsDesc[320];
	uchar szPinpadLangs[33], szPinpadLangsDesc[320];
	uchar *psTlvValue, *p;
	int   iTermEnv; // attended or unattended
	uchar szLocalLang[3];
	int   i, j, k;

	if(sg_szSelectedLang[0]) {
		// ���ν���ѡ���(�ǻص��ӿڲ�������sg_szSelectedLang, ��˲������)
		if(pszSelectedLang)
			strcpy(pszSelectedLang, sg_szSelectedLang);
		return(EMVIO_OK);
	}

	// ��ȡ��������
	iRet = iTlvGetObjValue(gl_sTlvDbTermFixed, TAG_DFXX_LocalLanguage, &psTlvValue); // attended�豸����pinpad������ѡ
	ASSERT(iRet == 2);
	if(iRet != 2)
		return(EMVIO_ERROR);
	memcpy(szLocalLang, psTlvValue, 2);
	szLocalLang[2] = 0;

	// ��ȡ�ն�֧�ֵ�����
	iRet = iEmvMsgTableSupportedLanguage(szTermLangs, szTermLangsDesc);
	ASSERT(iRet = 0);
	if(iRet != 0)
		return(EMVIO_ERROR);

	iTermEnv = iEmvTermEnvironment();
	if(iTermEnv == TERM_ENV_UNATTENDED) {
		// unattended�ն�ʹ���ն�����
		if(pszLangs)
			strcpy(pszLangs, szTermLangs);
		if(pszLangsDesc)
			strcpy(pszLangsDesc, szTermLangsDesc);
		if(gl_iCoreCallFlag != HXCORE_CALLBACK)
			return(EMVIO_OK); // ���ĵ��÷�ʽΪ�ǻص���ʽ, ��ȡ��ѡ���б��ɷ���
		iRet = iHxFaceSelectLang(szTermLangs, szTermLangsDesc, pszSelectedLang, pszEmvMsgTableGetInfo(EMVMSG_SELECT_LANG, szLocalLang), szLocalLang);
	} else {
		// attended�ն���Ҫʹ��pinpad����
		iRet = iTlvGetObjValue(gl_sTlvDbTermFixed, TAG_DFXX_PinpadLanguage, &psTlvValue); // attended�豸����pinpad������ѡ
		ASSERT(iRet>0 && (iRet%2)==0);
		if(iRet<=0 || iRet%2)
			return(EMVIO_ERROR);
		memcpy(szPinpadLangs, psTlvValue, iRet);
		szPinpadLangs[iRet] = 0;
		// ȡ��pinpad��������
		szPinpadLangsDesc[0] = 0;
		for(i=0; i<(int)strlen(szPinpadLangs); i+=2) {
			for(j=0; j<(int)strlen(szTermLangs); j+=2) {
				if(memcmp(szPinpadLangs+i, szTermLangs+j, 2) == 0)
					break;
			}
			ASSERT(j < (int)strlen(szTermLangs));
			if(j >= (int)strlen(szTermLangs))
				return(EMVIO_ERROR); // pinpad���Ա��뱻�ն�֧��, �������߼���������
			// ��ȡj������������(jÿ2�ֽ�����һ������)
			for(p=szTermLangsDesc,k=0; k<j/2; k++) {
				p = strchr(p, '=');
				ASSERT(p);
				if(p == NULL)
					return(EMVFACE_ERROR); // ���������ִ���ʽ����, '='�Ÿ�������
				p ++;
			}
			j = strlen(szPinpadLangsDesc);
			k = 0;
			do {
				szPinpadLangsDesc[j++] = p[k];
			} while(p[k++] != '=');
			szPinpadLangsDesc[j] = 0;
		}
		if(pszLangs)
			strcpy(pszLangs, szPinpadLangs);
		if(pszLangsDesc)
			strcpy(pszLangsDesc, szPinpadLangsDesc);
		if(gl_iCoreCallFlag != HXCORE_CALLBACK)
			return(EMVIO_OK); // ���ĵ��÷�ʽΪ�ǻص���ʽ, ��ȡ��ѡ���б��ɷ���
		iRet = iHxFaceSelectLang(szPinpadLangs, szPinpadLangsDesc, pszSelectedLang, pszEmvMsgTableGetInfo(EMVMSG_SELECT_LANG, szLocalLang), szLocalLang);
	}

	if(iRet == EMVFACE_OK) {
		vMemcpy0(sg_szSelectedLang, pszSelectedLang, 2); // ����ѡ�������
		return(EMVIO_OK);
	}
	if(iRet == EMVFACE_CANCEL)
		return(EMVIO_CANCEL);
	if(iRet == EMVFACE_TIMEOUT)
		return(EMVIO_TIMEOUT);
	return(EMVIO_ERROR);
}

// ѡ��Ӧ��
// in  : pAppList      : ��Ƭ���ն˶�֧�ֵ�Ӧ���б�
//       iAppNum       : ��Ƭ���ն˶�֧�ֵ�Ӧ�ø���
// out : piAppNo       : ѡ�е�Ӧ����Ӧ���б��е��±�
// ret : EMVIO_OK      : OK
//       EMVIO_CANCEL  : ȡ��
//       EMVIO_TIMEOUT : ��ʱ
//       EMVIO_ERROR   : ��������
// Note: Ӧ��ѡ��unattended�ն�ʹ�óֿ������ԣ�attended�ն�ʹ�ñ�������
int iEmvIOSelectApp(stADFInfo *pAppList, int iAppNum, int *piAppNo)
{
    int   iRet;
	uchar *psTlvValue;
	int   iTermEnv; // attended or unattended
	uchar szLanguage[3];

	if(gl_iCoreCallFlag != HXCORE_CALLBACK)
		return(EMVIO_OK); // ���ĵ��÷�ʽΪ�ǻص���ʽ

	iTermEnv = iEmvTermEnvironment();
	if(iTermEnv == TERM_ENV_UNATTENDED)
		iRet = iTlvGetObjValue(gl_sTlvDbTermVar, TAG_DFXX_LanguageUsed, &psTlvValue); // unattended�豸, ʹ�óֿ�������
	else
		iRet = iTlvGetObjValue(gl_sTlvDbTermFixed, TAG_DFXX_LocalLanguage, &psTlvValue); // attended�豸��ʹ�ñ�������
	ASSERT(iRet == 2);
	if(iRet != 2)
        return(EMVIO_ERROR);
	memcpy(szLanguage, psTlvValue, 2);
	szLanguage[2] = 0;

	iRet = iHxFaceSelectApp(pAppList, iAppNum, piAppNo, pszEmvMsgTableGetInfo(EMVMSG_SELECT_APP, szLanguage), szLanguage);
	if(iRet == EMVFACE_OK)
		return(EMVIO_OK);
	if(iRet == EMVFACE_CANCEL)
		return(EMVIO_CANCEL);
	if(iRet == EMVFACE_TIMEOUT)
		return(EMVIO_TIMEOUT);
	return(EMVIO_ERROR);
}

// ȷ��Ӧ��
// in  : pApp          : Ӧ��
// ret : EMVIO_OK      : OK
//       EMVIO_CANCEL  : ȡ��
//       EMVIO_TIMEOUT : ��ʱ
//       EMVIO_ERROR   : ��������
int iEmvIOConfirmApp(stADFInfo *pApp)
{
    int   iRet;
	uchar *psTlvValue;
	int   iTermEnv; // attended or unattended
	uchar szLanguage[3];

	if(gl_iCoreCallFlag != HXCORE_CALLBACK)
		return(EMVIO_OK); // ���ĵ��÷�ʽΪ�ǻص���ʽ

	iTermEnv = iEmvTermEnvironment();
	if(iTermEnv == TERM_ENV_UNATTENDED)
		iRet = iTlvGetObjValue(gl_sTlvDbTermVar, TAG_DFXX_LanguageUsed, &psTlvValue); // unattended�豸, ʹ�óֿ�������
	else
		iRet = iTlvGetObjValue(gl_sTlvDbTermFixed, TAG_DFXX_LocalLanguage, &psTlvValue); // attended�豸��ʹ�ñ�������
	ASSERT(iRet == 2);
	if(iRet != 2)
        return(EMVIO_ERROR);
	memcpy(szLanguage, psTlvValue, 2);
	szLanguage[2] = 0;

	iRet = iHxFaceConfirmApp(pApp, pszEmvMsgTableGetInfo(EMVMSG_05_CANCEL_OR_ENTER, szLanguage), szLanguage);
	if(iRet == EMVFACE_OK)
		return(EMVIO_OK);
	if(iRet == EMVFACE_CANCEL)
		return(EMVIO_CANCEL);
	if(iRet == EMVFACE_TIMEOUT)
		return(EMVIO_TIMEOUT);
	return(EMVIO_ERROR);
}

// ��ȡ���
// in  : iAmountType   : ������ͣ�EMVIO_AMOUNT:Amount
//                                 EMVIO_AMOUNT_OTHER:Amount other
// out : pszAmount     : ���[12]
// ret : EMVIO_OK      : OK
//       EMVIO_CANCEL  : ȡ��
//       EMVIO_TIMEOUT : ��ʱ
//       EMVIO_ERROR   : ��������
int iEmvIOGetAmount(int iAmountType, uchar *pszAmount)
{
    int   iRet;
	uchar *psTlvValue;
	int   iTermEnv; // attended or unattended
	uchar szLanguage[3];
	int   iDigitalCurrencyCode;
	uchar szAlphaCurrencyCode[5];
	int   iCurrencyDecimalPosition;

	if(gl_iCoreCallFlag != HXCORE_CALLBACK) {
		if(iAmountType == EMVIO_AMOUNT)
			strcpy(pszAmount, sg_szAmount);
		else if(iAmountType == EMVIO_AMOUNT_OTHER)
			strcpy(pszAmount, sg_szAmountOther);
		else
			return(EMVIO_ERROR);
		return(EMVIO_OK);
	}

	iTermEnv = iEmvTermEnvironment();
	if(iTermEnv == TERM_ENV_UNATTENDED) {
		iRet = iTlvGetObjValue(gl_sTlvDbTermVar, TAG_DFXX_LanguageUsed, &psTlvValue); // unattended�豸, ʹ�óֿ�������
	} else {
		iRet = iTlvGetObjValue(gl_sTlvDbTermFixed, TAG_DFXX_LocalLanguage, &psTlvValue); // attended�豸��ʹ�ñ�������
	}
	ASSERT(iRet == 2);
	if(iRet != 2)
        return(EMVIO_ERROR);
	memcpy(szLanguage, psTlvValue, 2);
	szLanguage[2] = 0;

	iRet = iTlvGetObjValue(gl_sTlvDbTermVar, TAG_5F2A_TransCurrencyCode, &psTlvValue);
	if(iRet != 2)
		return(EMVIO_ERROR);
	vOneTwo0(psTlvValue, 2, szAlphaCurrencyCode); // ����szCurrencyCode�ݴ�һ��ʮ���ƻ��Ҵ��봮
	iDigitalCurrencyCode = atoi(szAlphaCurrencyCode);
	iRet = iIso4217SearchDigitCode(iDigitalCurrencyCode, szAlphaCurrencyCode, &iCurrencyDecimalPosition);
	if(iRet) {
		// û�ҵ����Ҵ���, ʹ��ȱʡֵ
		strcpy(szAlphaCurrencyCode, "XXX");
		iCurrencyDecimalPosition = 0;
	}

	iRet = iHxFaceGetAmount(iAmountType, pszAmount, pszEmvMsgTableGetInfo(EMVMSG_08_ENTER_AMOUNT, szLanguage), szAlphaCurrencyCode, iCurrencyDecimalPosition, szLanguage);
	if(iRet == EMVFACE_OK)
		return(EMVIO_OK);
	if(iRet == EMVFACE_CANCEL)
		return(EMVIO_CANCEL);
	if(iRet == EMVFACE_TIMEOUT)
		return(EMVIO_TIMEOUT);
	return(EMVIO_ERROR);
}

// ��ȡ����
// in  : iPinType      : ��������, EMVIO_PLAIN_PIN:����PIN
//                                 EMVIO_CIPHERED_PIN:����PIN
//       iBypassFlag   : ����Bypass���, 0:������bypass 1:����bypass
//       pszPan        : �˺�
// out : psPin         : 4-12λ��������('\0'��β���ַ���)��8�ֽ�����PIN
// ret : EMVIO_OK      : OK
//       EMVIO_CANCEL  : ȡ��
//       EMVIO_BYPASS  : bypass���Թ�
//       EMVIO_TIMEOUT : ��ʱ
//       EMVIO_ERROR   : ��������
int iEmvIOGetPin(int iPinType, int iBypassFlag, uchar *psPin, uchar *pszPan)
{
    int   iRet;
    int   iZeroFlag;
	uchar *psTlvValue;
	uchar szLanguage[3];
	uchar szAmountStr[20];

	if(gl_iCoreCallFlag != HXCORE_CALLBACK)
		return(EMVIO_OK); // ���ĵ��÷�ʽΪ�ǻص���ʽ

	iRet = iTlvGetObjValue(gl_sTlvDbTermVar, TAG_DFXX_LanguageUsed, &psTlvValue);
	ASSERT(iRet == 2);
	if(iRet != 2)
        return(EMVIO_ERROR);
	memcpy(szLanguage, psTlvValue, 2);
	szLanguage[2] = 0;

	// form amount string
	iZeroFlag = iMakeAmountStr(szAmountStr);
    if(iZeroFlag <= 0) {
        // �������Ϊ0, ����ʾ���
        szAmountStr[0] = 0;
    }

	if(iPinType == EMVIO_PLAIN_PIN)
		iRet = iHxFaceGetPin(iPinType, iBypassFlag, psPin, pszPan, szAmountStr, pszEmvMsgTableGetInfo(EMVMSG_ENTER_PIN_OFFLINE, szLanguage), szLanguage);
	else
		iRet = iHxFaceGetPin(iPinType, iBypassFlag, psPin, pszPan, szAmountStr, pszEmvMsgTableGetInfo(EMVMSG_ENTER_PIN_ONLINE, szLanguage), szLanguage);
	if(iRet == EMVFACE_OK)
		return(EMVIO_OK);
	if(iRet == EMVFACE_CANCEL)
		return(EMVIO_CANCEL);
	if(iRet == EMVFACE_TIMEOUT)
		return(EMVIO_TIMEOUT);
	if(iRet == EMVFACE_BYPASS)
		return(EMVIO_BYPASS);
	return(EMVIO_ERROR);
}

// ��֤�ֿ������֤��
// in  : iBypassFlag    : ����Bypass���, 0:������bypass 1:����bypass
//       ucHolderIdType : ֤������
//       szHolderId     : ֤������
// ret : EMVIO_OK       : OK
//       EMVIO_CANCEL   : ȡ��
//       EMVIO_BYPASS   : bypass���Թ���֤������
//       EMVIO_TIMEOUT  : ��ʱ
//       EMVIO_ERROR    : ��������
int iEmvIOVerifyHolderId(int iBypassFlag, uchar ucHolderIdType, uchar *pszHolderId)
{
    int   iRet;
	uchar *psTlvValue;
	uchar szLanguage[3];
	uchar szHolderIdType[40];

	if(gl_iCoreCallFlag != HXCORE_CALLBACK)
		return(EMVIO_OK); // ���ĵ��÷�ʽΪ�ǻص���ʽ

	iRet = iTlvGetObjValue(gl_sTlvDbTermFixed, TAG_DFXX_LocalLanguage, &psTlvValue); // ʹ�ñ�������
	ASSERT(iRet == 2);
	if(iRet != 2)
        return(EMVIO_ERROR);
	memcpy(szLanguage, psTlvValue, 2);
	szLanguage[2] = 0;

	switch(ucHolderIdType) {
	case 0: // ���֤
		strcpy(szHolderIdType, pszEmvMsgTableGetInfo(EMVMSG_IDENTIFICATION_CARD, szLanguage));
		break;
	case 1: // ����֤
		strcpy(szHolderIdType, pszEmvMsgTableGetInfo(EMVMSG_CERTIFICATE_OF_OFFICERS, szLanguage));
		break;
	case 2: // ����
		strcpy(szHolderIdType, pszEmvMsgTableGetInfo(EMVMSG_PASSPORT, szLanguage));
		break;
	case 3: // �뾳֤
		strcpy(szHolderIdType, pszEmvMsgTableGetInfo(EMVMSG_ARRIVAL_CARD, szLanguage));
		break;
	case 4: // ��ʱ���֤
		strcpy(szHolderIdType, pszEmvMsgTableGetInfo(EMVMSG_TEMPORARY_IDENTITY_CARD, szLanguage));
		break;
	case 5: // ����
		strcpy(szHolderIdType, pszEmvMsgTableGetInfo(EMVMSG_OTHER, szLanguage));
		break;
	default:
		return(EMVIO_BYPASS); // ��ʶ���֤������, ��Ϊʧ��
	}

	iRet = iHxFaceVerifyHolderId(iBypassFlag,
		                         pszEmvMsgTableGetInfo(EMVMSG_VERIFY_HOLDER_ID, szLanguage), // "�����ֿ���֤��"
		                         pszEmvMsgTableGetInfo(EMVMSG_HOLDER_ID_TYPE, szLanguage),   // "�ֿ���֤������"
		                         szHolderIdType, // ��ֿ���֤������
		                         pszEmvMsgTableGetInfo(EMVMSG_HOLDER_ID_NO, szLanguage), // "�ֿ���֤������"
		                         pszHolderId,    // "�ֿ���֤������"
								 szLanguage);
	if(iRet == EMVFACE_OK)
		return(EMVIO_OK);
	if(iRet == EMVFACE_CANCEL)
		return(EMVIO_CANCEL);
	if(iRet == EMVFACE_TIMEOUT)
		return(EMVIO_TIMEOUT);
	if(iRet==EMVFACE_BYPASS || iRet==EMVFACE_FAIL)
		return(EMVIO_BYPASS);
	return(EMVIO_ERROR);
}

// ��ʾ��Ϣ
// in  : iMsgType      : ��Ϣ���ͣ���EmvMsg.h
//       iConfirmFlag  : ȷ�ϱ�־ EMVIO_NEED_NO_CONFIRM : ��Ҫ��ȷ��
//                                EMVIO_NEED_CONFIRM    : Ҫ��ȷ��
// ret : EMVIO_OK      : OK, ���Ҫ��ȷ�ϣ��Ѿ�ȷ��
//       EMVIO_CANCEL  : ȡ��
//       EMVIO_TIMEOUT : ��ʱ
int iEmvIODispMsg(int iMsgType, int iConfirmFlag)
{
    int   iRet;
	uchar *psTlvValue;
	int   iTermEnv; // attended or unattended
	uchar szLanguage[3];
	uchar szAmountStr[20];
	int   iConfirmPara;

	if(gl_iCoreCallFlag != HXCORE_CALLBACK)
		return(EMVIO_OK); // ���ĵ��÷�ʽΪ�ǻص���ʽ

	iTermEnv = iEmvTermEnvironment();
	if(iTermEnv == TERM_ENV_UNATTENDED)
		iRet = iTlvGetObjValue(gl_sTlvDbTermVar, TAG_DFXX_LanguageUsed, &psTlvValue); // unattended�豸, ʹ�óֿ�������
	else
		iRet = iTlvGetObjValue(gl_sTlvDbTermFixed, TAG_DFXX_LocalLanguage, &psTlvValue); // attended�豸��ʹ�ñ�������
	ASSERT(iRet == 2);
	if(iRet != 2)
        return(EMVIO_ERROR);
	memcpy(szLanguage, psTlvValue, 2);
	szLanguage[2] = 0;

	if(iConfirmFlag == EMVIO_NEED_CONFIRM)
		iConfirmPara = EMVFACE_NEED_CONFIRM;
	else
	    iConfirmPara = EMVFACE_NEED_NO_CONFIRM;

	if(iMsgType < EMVMSG_VAR) {
		// �̶���Ϣ
		iRet = iHxFaceDispMsg(iMsgType, iConfirmPara, NULL, pszEmvMsgTableGetInfo(iMsgType, szLanguage), szLanguage);
	} else {
		//iMsgType >= EMVMSG_VAR ������̬��Ϣ
		switch(iMsgType) {
		case EMVMSG_AMOUNT_CONFIRM:
			iMakeAmountStr(szAmountStr);
			iRet = iHxFaceDispMsg(iMsgType, iConfirmPara, pszEmvMsgTableGetInfo(iMsgType, szLanguage), szAmountStr, szLanguage);
			break;
		default:
			iRet = EMVFACE_OK;
			break; // ֻ֧��EMVMSG_AMOUNT_CONFIRM
		}
	}
	
	if(iRet == EMVFACE_OK)
		return(EMVIO_OK);
	if(iRet == EMVFACE_CANCEL)
		return(EMVIO_CANCEL);
	return(EMVIO_TIMEOUT);
}

// ��ʾ��Ϣ, ��Ϣ������Ļ��, �����˳�
// in  : iMsgType      : ��Ϣ���ͣ���EmvMsg.h
//                       iMsgType==-1��ʾ���֮ǰ��ʾ����Ϣ
// ret : EMVIO_OK      : OK
// Note: iEmvIODispMsg()�������������ʾ������
int iEmvIOShowMsg(int iMsgType)
{
    int   iRet;
	uchar *psTlvValue;
	int   iTermEnv; // attended or unattended
	uchar szLanguage[3];
	uchar szAmountStr[20];

	if(gl_iCoreCallFlag != HXCORE_CALLBACK)
		return(EMVIO_OK); // ���ĵ��÷�ʽΪ�ǻص���ʽ

	if(iMsgType == -1) {
		iHxFaceShowMsg(iMsgType, NULL, NULL, "");
		return(EMVIO_OK);
	}

	iTermEnv = iEmvTermEnvironment();
	if(iTermEnv == TERM_ENV_UNATTENDED)
		iRet = iTlvGetObjValue(gl_sTlvDbTermVar, TAG_DFXX_LanguageUsed, &psTlvValue); // unattended�豸, ʹ�óֿ�������
	else
		iRet = iTlvGetObjValue(gl_sTlvDbTermFixed, TAG_DFXX_LocalLanguage, &psTlvValue); // attended�豸��ʹ�ñ�������
	ASSERT(iRet == 2);
	if(iRet != 2)
        return(EMVIO_ERROR);
	memcpy(szLanguage, psTlvValue, 2);
	szLanguage[2] = 0;

	if(iMsgType < EMVMSG_VAR) {
		// �̶���Ϣ
		iRet = iHxFaceShowMsg(iMsgType, NULL, pszEmvMsgTableGetInfo(iMsgType, szLanguage), szLanguage);
	} else {
		//iMsgType >= EMVMSG_VAR ������̬��Ϣ
		switch(iMsgType) {
		case EMVMSG_AMOUNT_CONFIRM:
			iMakeAmountStr(szAmountStr);
			iRet = iHxFaceShowMsg(iMsgType, pszEmvMsgTableGetInfo(iMsgType, szLanguage), szAmountStr, szLanguage);
			break;
		default:
			iRet = EMVFACE_OK;
			break; // ֻ֧��EMVMSG_AMOUNT_CONFIRM
		}
	}

	return(EMVIO_OK);
}

// ���������ʾ��Ϣ
// in  : iMsgType      : ��Ϣ���ͣ���EmvMsg.h
//       iConfirmFlag  : ȷ�ϱ�־ EMVIO_NEED_NO_CONFIRM : ��Ҫ��ȷ��
//                                EMVIO_NEED_CONFIRM    : Ҫ��ȷ��
// ret : EMVIO_OK      : OK, ���Ҫ��ȷ�ϣ��Ѿ�ȷ��
//       EMVIO_CANCEL  : ȡ��
//       EMVIO_TIMEOUT : ��ʱ
// Note: ����unattended�ն˻�attended�ն˵���Pinpad������Ϣ����ʾ��Ψһ����ʾ����
int iEmvIOPinpadDispMsg(int iMsgType, int iConfirmFlag)
{
    int   iRet;
    int   iZeroFlag;
	uchar *psTlvValue;
	uchar szLanguage[3];
	uchar szAmountStr[20];
	int   iConfirmPara;

	if(gl_iCoreCallFlag != HXCORE_CALLBACK)
		return(EMVIO_OK); // ���ĵ��÷�ʽΪ�ǻص���ʽ

	iRet = iTlvGetObjValue(gl_sTlvDbTermVar, TAG_DFXX_LanguageUsed, &psTlvValue);
	ASSERT(iRet == 2);
	if(iRet != 2)
        return(EMVIO_ERROR);
	memcpy(szLanguage, psTlvValue, 2);
	szLanguage[2] = 0;

	if(iConfirmFlag == EMVIO_NEED_CONFIRM)
		iConfirmPara = EMVFACE_NEED_CONFIRM;
	else
	    iConfirmPara = EMVFACE_NEED_NO_CONFIRM;

	if(iMsgType < EMVMSG_VAR) {
		// �̶���Ϣ
		iRet = iHxFacePinpadDispMsg(iMsgType, iConfirmPara, NULL, pszEmvMsgTableGetInfo(iMsgType, szLanguage), szLanguage);
	} else {
		//iMsgType >= EMVMSG_VAR ������̬��Ϣ
		switch(iMsgType) {
		case EMVMSG_AMOUNT_CONFIRM:
			iZeroFlag = iMakeAmountStr(szAmountStr);
            if(iZeroFlag <= 0) {
                // �������Ϊ0, ����ʾ���
                szAmountStr[0] = 0;
            }
			iRet = iHxFacePinpadDispMsg(iMsgType, iConfirmPara, pszEmvMsgTableGetInfo(iMsgType, szLanguage), szAmountStr, szLanguage);
			break;
		default:
			iRet = EMVFACE_OK;
			break; // ֻ֧��EMVMSG_AMOUNT_CONFIRM
		}
	}
	if(iRet == EMVFACE_OK)
		return(EMVIO_OK);
	if(iRet == EMVFACE_CANCEL)
		return(EMVIO_CANCEL);
	return(EMVIO_TIMEOUT);
}

// ֤������б��ѯ
// in  : psCertSerNo     : ֤�����к�[3]
// ret : EMVIO_OK        : ����֤������б���
//       EMVIO_BLACK     : ��֤������б���
//       EMVIO_ERROR     : ��������
int iEmvIOCRLCheck(uchar *psCertSerNo)
{
	int   iRet;
	uchar *pucCaPubKeyIndex;
	uchar *psRid;

	if(gl_iCoreCallFlag != HXCORE_CALLBACK) {
		memcpy(sg_sCertSerNo, psCertSerNo, 3); // ���淢����֤�����к�
		return(EMVIO_OK); // ���ĵ��÷�ʽΪ�ǻص���ʽ
	}

	iRet = iTlvGetObjValue(gl_sTlvDbCard, TAG_8F_CAPubKeyIndex, &pucCaPubKeyIndex);
	if(iRet != 1)
		return(EMVIO_OK);
	iRet = iTlvGetObjValue(gl_sTlvDbCard, TAG_4F_AID, &psRid);
	if(iRet < 5)
		return(EMVIO_OK);
	iRet = 	iHxFaceCRLCheck(psRid, *pucCaPubKeyIndex, psCertSerNo);
	if(iRet == EMVFACE_BLACK)
		return(EMVIO_BLACK);
	if(iRet != EMVFACE_OK)
		return(EMVIO_ERROR);
	return(EMVIO_OK);
}

// ��������ѯ
// ret : EMVIO_OK        : ���Ǻ�������
//       EMVIO_BLACK     : ��������
//       EMVIO_ERROR     : ��������
int iEmvIOBlackCardCheck(void)
{
	int   iRet;
	uchar *p;
	uchar szPan[21], sRid[5];
	int   iPanSeqNo;

	if(gl_iCoreCallFlag != HXCORE_CALLBACK) {
		// ���ĵ��÷�ʽΪ�ǻص���ʽ
		if(sg_iBlackFlag)
			return(EMVIO_BLACK);
		return(EMVIO_OK);
	}

	if(gl_iCoreCallFlag != HXCORE_CALLBACK)
		return(EMVIO_OK); // ���ĵ��÷�ʽΪ�ǻص���ʽ

	iRet = iTlvGetObjValue(gl_sTlvDbCard, TAG_5A_PAN, &p);
	if(iRet<=0 || iRet>10)
		return(EMVIO_OK);
	vOneTwo0(p, iRet, szPan);
    vTrimTailF(szPan);
	iRet = iTlvGetObjValue(gl_sTlvDbCard, TAG_5F34_PANSeqNo, &p);
	if(iRet == 1)
		iPanSeqNo = (int)*p;
	else
		iPanSeqNo = -1;
	iRet = iTlvGetObjValue(gl_sTlvDbCard, TAG_4F_AID, &p);
	if(iRet < 5)
		return(EMVIO_OK); // AID�������
	memcpy(sRid, p, 5);

	iRet = iHxFaceBlackCardCheck(sRid, szPan, iPanSeqNo);
	if(iRet == EMVFACE_ERROR)
		return(EMVIO_ERROR);
	if(iRet == EMVFACE_BLACK)
		return(EMVIO_BLACK);
	if(iRet != EMVFACE_OK)
		return(EMVIO_ERROR);
	return(EMVIO_OK);
}

// �ֿ����Ѳ�ѯ
// out : pszAmount       : ���˻���ʷ��¼���(�鲻������"0"), 
// ret : EMVIO_OK        : �ɹ�
//       EMVIO_ERROR     : ��������
int iEmvIOSeparateSaleCheck(uchar *pszAmount)
{
	int iRet;
	uchar *p;
	uchar *psRid, szPan[21];
	int   iPanSeqNo;

	if(gl_iCoreCallFlag != HXCORE_CALLBACK) {
		// ���ĵ��÷�ʽΪ�ǻص���ʽ
		strcpy(pszAmount, sg_szRecentAmount);
		return(EMVIO_OK);
	}

	strcpy((char *)pszAmount, "0"); // ����0
	// ȡRID
	iRet = iTlvGetObjValue(gl_sTlvDbCard, TAG_4F_AID, &psRid);
	if(iRet <= 0)
		return(EMVIO_OK); // aid�������
	// ȡPan
	iRet = iTlvGetObjValue(gl_sTlvDbCard, TAG_5A_PAN, &p);
	if(iRet<=0 || iRet>10)
		return(EMVIO_OK); // Pan�������
	vOneTwo0(p, iRet, szPan);
	vTrimTailF(szPan);
	// ȡPan SeqNo
	iRet = iTlvGetObjValue(gl_sTlvDbCard, TAG_5F34_PANSeqNo, &p);
	if(iRet <= 0)
		iPanSeqNo = -1;
	else
		iPanSeqNo = (int)(*p);
	iRet = iHxFaceSeparateSaleCheck(psRid, szPan, iPanSeqNo, pszAmount);
	if(iRet != EMVFACE_OK)
		return(EMVIO_ERROR);
	return(EMVIO_OK);
}
