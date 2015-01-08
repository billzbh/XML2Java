/**************************************
File name     : EmvPara.c
Function      : EMV/pboc2.0����ǲ�������
Author        : Yu Jun
First edition : Mar 31st, 2012
Modified      : Apr 21st, 2014
					��Ϊlinux��strlwr()����, ���ӶԴ˺�����֧��
**************************************/
/*
ģ����ϸ����:
	�ṩ�ն˲������á�CA��Կ�������á�AID��������
.	���ò���ʱ��������Ϸ���
*/
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "VposFace.h"
#include "Arith.h"
#include "Common.h"
#include "TlvFunc.h"
#include "TagDef.h"
#include "EmvCore.h"
#include "EmvMsg.h"
#include "EmvData.h"
#include "EmvFunc.h"

// linux�޴˺���, ����֧��, ��Щ��������֧��ͬ��, ����ں������'2'��ʾ����
static char *strlwr2(char *pszStr)
{
	char *p;
	p = pszStr;
	while(*p) {
		if(*p>='A' && *p<='Z')
			*p += 0x20;
		p ++;
	}
	return(pszStr);
}

// �������Ʊ�ʾ��tagת��Ϊascii��ʾ��tag
static void vMakeTagStr(uchar *pszBinTag, uchar *pszAscTag)
{
	vOneTwo0(pszBinTag, strlen(pszBinTag), pszAscTag);
}

// ���pszLangs1��pszLangs2ͬʱ֧�ֵ�����
// in  : pszLangs1 : ������1, ����:"enzh"
//       pszLangs2 : ������2, ����:"fren"
// out : pszLangs1 : ����, ���ӵĽ��:"en"
static void vGetCommonLanguage(uchar *pszLangs1, uchar *pszLangs2)
{
	int i;
	ASSERT(strlen(pszLangs1)%2 == 0);
	if(strlen(pszLangs1)%2) {
		pszLangs1[0] = 0; // ���������1��ʽ��, ��Ϊû�н���
		return;
	}
	while(strlen(pszLangs1)) {
		for(i=0; i<(int)strlen(pszLangs2); i+=2) {
			if(memcmp(pszLangs1, pszLangs2+i, 2)==0)
				break;
		}
		if(i < (int)strlen(pszLangs2)) {
			// ��ǰ������ƥ��
			pszLangs1 += 2;
			continue;
		}
		// ��ǰ����û��ƥ��, ɾ��
		memmove(pszLangs1, pszLangs1+2, strlen(pszLangs1+2)+1);
	}
}

// �����ն˲���
// in  : pTermParam        : �ն˲���
// out : pszErrTag         : ������ó���, �����Tagֵ
// ret : HXEMV_OK          : OK
//       HXEMV_PARA        : ��������, ����Tag��pszErrTag
//       HXEMV_CORE        : �ڲ�����
//       HXEMV_FLOW_ERROR  : EMV���̴���
int iEmvSetTermParam(stTermParam *pTermParam, uchar *pszErrTag)
{
   	int   iRet;
	uchar szBuf[256], sBuf[256];

	// T9F35, �ն�����
	if(!((pTermParam->ucTermType>=0x11 && pTermParam->ucTermType<=0x16) || (pTermParam->ucTermType>=0x21 && pTermParam->ucTermType<=0x26))) {
		vMakeTagStr(TAG_9F35_TermType, pszErrTag);
		return(HXEMV_PARA); // ֻ֧���ն�����'11'-'16', '21'-'26'
	}
	iRet = iTlvMakeAddObj(gl_sTlvDbTermFixed, TAG_9F35_TermType, 1, &pTermParam->ucTermType, TLV_CONFLICT_ERR);
   	ASSERT(iRet >= 0);
	if(iRet < 0)
		return(HXEMV_CORE);

	// T9F33, �ն�����
	if(iTestStrZeroWithMark(pTermParam->sTermCapability, "\x1F\x06\x17", 3) != 0) { // modified 2013.2.19, ֧��֤����֤, "\x1F\x07\x17"->"\x1F\x06\x17"
		vMakeTagStr(TAG_9F33_TermCapability, pszErrTag);
		return(HXEMV_PARA); // RFUλ����Ϊ0
	}
	if(iTestStrZeroWithMark(pTermParam->sTermCapability, "\x00\x10\x20", 3) != 0) {
		vMakeTagStr(TAG_9F33_TermCapability, pszErrTag);
		return(HXEMV_PARA); // ���Ĳ�֧���ѻ�������֤PIN��Card Capture
	}
	iRet = iTlvMakeAddObj(gl_sTlvDbTermFixed, TAG_9F33_TermCapability, 3, pTermParam->sTermCapability, TLV_CONFLICT_ERR);
    ASSERT(iRet >= 0);
	if(iRet < 0)
		return(HXEMV_CORE);
	if(iEmvTermCommunication() != TERM_COM_ONLINE_ONLY) {
		// ���������ն�
		if(iTestStrBit(pTermParam->sTermCapability, TERM_CAP_16_SDA) != 1) {
			vMakeTagStr(TAG_9F33_TermCapability, pszErrTag);
			return(HXEMV_PARA); // �������ն˱���֧��SDA
		}
		if(iTestStrBit(pTermParam->sTermCapability, TERM_CAP_17_DDA) != 1) {
			vMakeTagStr(TAG_9F33_TermCapability, pszErrTag);
			return(HXEMV_PARA); // �������ն˱���֧��DDA
		}
	}
	
	// T9F40, �ն�������չ
	if(iTestStrZeroWithMark(pTermParam->sAdditionalTermCapability, "\x00\x3F\x0F\x0C\x00", 5) != 0) {
		vMakeTagStr(TAG_9F40_TermCapabilityExt, pszErrTag);
		return(HXEMV_PARA); // RFUλ����Ϊ0
	}
	if(iTestStrZeroWithMark(pTermParam->sAdditionalTermCapability, "\x10\x00\x00\x03\xFF", 5) != 0) {
		vMakeTagStr(TAG_9F40_TermCapabilityExt, pszErrTag);
		return(HXEMV_PARA); // ���Ĳ�֧��cashback��Code table1-10
	}
	iRet = iTlvMakeAddObj(gl_sTlvDbTermFixed, TAG_9F40_TermCapabilityExt, 5, pTermParam->sAdditionalTermCapability, TLV_CONFLICT_ERR);
	if(iRet < 0) {
        ASSERT(0);
		return(HXEMV_CORE);
	}

	// TDFxx, �ն˷ǽ�֧������
	if((pTermParam->sTermCtlsCapability[0]&0x60) == 0x00) {
		vMakeTagStr(TAG_DFXX_TermCtlsCapMask, pszErrTag);
		return(HXEMV_PARA); // �ǽ�pboc��ǽ�qPboc����Ҫ֧��һ��
	}
	iRet = iTlvMakeAddObj(gl_sTlvDbTermFixed, TAG_DFXX_TermCtlsCapMask, 4, pTermParam->sTermCtlsCapability, TLV_CONFLICT_ERR);
	if(iRet < 0) {
        ASSERT(0);
		return(HXEMV_CORE);
	}

	// TDFxx, 1:ֻ֧�ִſ� 2:ֻ֧��IC�� 3:֧�����ʽMag/IC������ 4:֧�ַ���ʽMag/IC������(TAG_DFXX_ReaderCapability)
	if(pTermParam->ucReaderCapability<1 || pTermParam->ucReaderCapability > 4) {
		vMakeTagStr(TAG_DFXX_ReaderCapability, pszErrTag);
		return(HXEMV_PARA);
	}
	iRet = iTlvMakeAddObj(gl_sTlvDbTermFixed, TAG_DFXX_ReaderCapability, 1, &pTermParam->ucReaderCapability, TLV_CONFLICT_ERR);
    ASSERT(iRet >= 0);
	if(iRet < 0)
		return(HXEMV_CORE);

	// TDFxx, 1:֧�� 2:��֧��,ȱʡapprove 3:��֧��,ȱʡdecline(TAG_DFXX_VoiceReferralSupport)
	if(pTermParam->ucVoiceReferralSupport<1 || pTermParam->ucVoiceReferralSupport > 3) {
		vMakeTagStr(TAG_DFXX_VoiceReferralSupport, pszErrTag);
		return(HXEMV_PARA);
	}
	iRet = iTlvMakeAddObj(gl_sTlvDbTermFixed, TAG_DFXX_VoiceReferralSupport, 1, &pTermParam->ucVoiceReferralSupport, TLV_CONFLICT_ERR);
	if(iRet < 0) {
        ASSERT(0);
		return(HXEMV_CORE);
	}

    // TDFxx, PIN bypass���� 0:ÿ��bypassֻ��ʾ�ô�bypass 1:һ��bypass,��������Ϊbypass
	iRet = iTlvMakeAddObj(gl_sTlvDbTermFixed, TAG_DFXX_PinBypassBehavior, 1, &pTermParam->ucPinBypassBehavior, TLV_CONFLICT_ERR);
	if(iRet < 0) {
        ASSERT(0);
		return(HXEMV_CORE);
	}

	// T9F16, Merchant Identifier
	if(strlen(pTermParam->szMerchantId) != 15) {
		vMakeTagStr(TAG_9F16_MerchantId, pszErrTag);
		return(HXEMV_PARA);
	}
	iRet = iTlvMakeAddObj(gl_sTlvDbTermFixed, TAG_9F16_MerchantId, 15, pTermParam->szMerchantId, TLV_CONFLICT_ERR);
	if(iRet < 0) {
        ASSERT(0);
		return(HXEMV_CORE);
	}

	// T9F1C, Terminal Identification
	if(strlen(pTermParam->szTermId) != 8) {
		vMakeTagStr(TAG_9F1C_TermId, pszErrTag);
		return(HXEMV_PARA);
	}
	iRet = iTlvMakeAddObj(gl_sTlvDbTermFixed, TAG_9F1C_TermId, 8, pTermParam->szTermId, TLV_CONFLICT_ERR);
	if(iRet < 0) {
        ASSERT(0);
		return(HXEMV_CORE);
	}

	// T9F1E, IFD Serial Number
	if(strlen(pTermParam->szIFDSerialNo) != 8) {
		vMakeTagStr(TAG_9F1E_IFDSerNo, pszErrTag);
		return(HXEMV_PARA);
	}
	iRet = iTlvMakeAddObj(gl_sTlvDbTermFixed, TAG_9F1E_IFDSerNo, 8, pTermParam->szIFDSerialNo, TLV_CONFLICT_ERR);
	if(iRet < 0) {
        ASSERT(0);
		return(HXEMV_CORE);
	}

	// T9F4E, Merchant Name and Location
	if(strlen(pTermParam->szMerchantNameLocation)) {
		iRet = iTlvMakeAddObj(gl_sTlvDbTermFixed, TAG_9F4E_MerchantName, strlen(pTermParam->szMerchantNameLocation), pTermParam->szMerchantNameLocation, TLV_CONFLICT_ERR);
		if(iRet < 0) {
	        ASSERT(0);
			return(HXEMV_CORE);
		}
	}

	// T9F15, -1:�޴����� 0-9999:��Ч����
	if(pTermParam->iMerchantCategoryCode >= 0) {
		if(pTermParam->iMerchantCategoryCode > 9999) {
			vMakeTagStr(TAG_9F15_MerchantCategoryCode, pszErrTag);
			return(HXEMV_PARA);
		}
		sprintf(szBuf, "%04d", pTermParam->iMerchantCategoryCode);
		vTwoOne(szBuf, 4, sBuf);
		iRet = iTlvMakeAddObj(gl_sTlvDbTermFixed, TAG_9F15_MerchantCategoryCode, 2, sBuf, TLV_CONFLICT_ERR);
		if(iRet < 0) {
            ASSERT(0);
			return(HXEMV_CORE);
		}
	}

	// T9F1A, �ն˹��Ҵ���
	if(pTermParam->iTermCountryCode<0 || pTermParam->iTermCountryCode>999) {
		vMakeTagStr(TAG_9F1A_TermCountryCode, pszErrTag);
		return(HXEMV_PARA);
	}
	sprintf(szBuf, "%04d", pTermParam->iTermCountryCode);
	vTwoOne(szBuf, 4, sBuf);
	iRet = iTlvMakeAddObj(gl_sTlvDbTermFixed, TAG_9F1A_TermCountryCode, 2, sBuf, TLV_CONFLICT_ERR);
	if(iRet < 0) {
        ASSERT(0);
		return(HXEMV_CORE);
	}

	// T5F2A, �ն˽��׻��Ҵ���
	if(pTermParam->iTermCurrencyCode<0 || pTermParam->iTermCurrencyCode>999) {
		vMakeTagStr(TAG_5F2A_TransCurrencyCode, pszErrTag);
		return(HXEMV_PARA);
	}
#if 0
	// modified 20130121, Ϊ֧�ֶ����, ��gpoʱʹ�ô���, ����gl_sTlvDbTermVar���ݿ�
	sprintf(szBuf, "%04d", pTermParam->iTermCurrencyCode);
	vTwoOne(szBuf, 4, sBuf);
	iRet = iTlvMakeAddObj(gl_sTlvDbTermFixed, TAG_5F2A_TransCurrencyCode, 2, sBuf, TLV_CONFLICT_ERR);
	if(iRet < 0) {
        ASSERT(0);
		return(HXEMV_CORE);
	}
#endif

	// T9F01, Acquirer Identifier
	if(strlen(pTermParam->szAcquirerId)<6 || strlen(pTermParam->szAcquirerId)>11) {
		vMakeTagStr(TAG_9F01_AcquirerId, pszErrTag);
		return(HXEMV_PARA);
	}
	if(iTestStrDecimal(pTermParam->szAcquirerId, strlen(pTermParam->szAcquirerId)) != 0) {
		vMakeTagStr(TAG_9F01_AcquirerId, pszErrTag);
		return(HXEMV_PARA); // ������ʮ�����ַ�����ʾ
	}
	strcpy(szBuf, "0");
	if(strlen(pTermParam->szAcquirerId) % 2)
		strcat(szBuf, pTermParam->szAcquirerId); // �������Ϊ��������'0'
	else
		strcpy(szBuf, pTermParam->szAcquirerId);
	vTwoOne(szBuf, strlen(szBuf), sBuf);
	iRet = iTlvMakeAddObj(gl_sTlvDbTermFixed, TAG_9F01_AcquirerId, strlen(szBuf)/2, sBuf, TLV_CONFLICT_ERR);
	if(iRet < 0) {
        ASSERT(0);
		return(HXEMV_CORE);
	}

	// TDFxx, 1:�ն�֧�ֽ�����ˮ��¼ 0:��֧��(TAG_DFXX_TransLogSupport)
	if(pTermParam->ucTransLogSupport > 1) {
		vMakeTagStr(TAG_DFXX_TransLogSupport, pszErrTag);
		return(HXEMV_PARA);
	}
	iRet = iTlvMakeAddObj(gl_sTlvDbTermFixed, TAG_DFXX_TransLogSupport, 1, &pTermParam->ucTransLogSupport, TLV_CONFLICT_ERR);
	if(iRet < 0) {
        ASSERT(0);
		return(HXEMV_CORE);
	}

	// TDFxx, 1:�ն�֧�ֺ�������� 0:��֧��(TAG_DFXX_BlacklistSupport)
	if(pTermParam->ucBlackListSupport > 1) {
		vMakeTagStr(TAG_DFXX_BlacklistSupport, pszErrTag);
		return(HXEMV_PARA);
	}
	iRet = iTlvMakeAddObj(gl_sTlvDbTermFixed, TAG_DFXX_BlacklistSupport, 1, &pTermParam->ucBlackListSupport, TLV_CONFLICT_ERR);
	if(iRet < 0) {
        ASSERT(0);
		return(HXEMV_CORE);
	}

	// TDFxx, 1:֧�ַֿ����Ѽ�� 0:��֧��(TAG_DFXX_SeparateSaleSupport)
	if(pTermParam->ucSeparateSaleSupport > 1) {
		vMakeTagStr(TAG_DFXX_SeparateSaleSupport, pszErrTag);
		return(HXEMV_PARA);
	}
	iRet = iTlvMakeAddObj(gl_sTlvDbTermFixed, TAG_DFXX_SeparateSaleSupport, 1, &pTermParam->ucSeparateSaleSupport, TLV_CONFLICT_ERR);
	if(iRet < 0) {
        ASSERT(0);
		return(HXEMV_CORE);
	}

	// TDFxx, 1:֧��Ӧ��ȷ�� 0:��֧��Ӧ��ȷ��(TAG_DFXX_AppConfirmSupport)
	if(pTermParam->ucAppConfirmSupport > 1) {
		vMakeTagStr(TAG_DFXX_AppConfirmSupport, pszErrTag);
		return(HXEMV_PARA);
	}
	iRet = iTlvMakeAddObj(gl_sTlvDbTermFixed, TAG_DFXX_AppConfirmSupport, 1, &pTermParam->ucAppConfirmSupport, TLV_CONFLICT_ERR);
	if(iRet < 0) {
        ASSERT(0);
		return(HXEMV_CORE);
	}

	// TDFxx, ISO-639-1, ��������(TAG_DFXX_LocalLanguage)
	if(strlen(pTermParam->szLocalLanguage) != 2) {
		vMakeTagStr(TAG_DFXX_LocalLanguage, pszErrTag);
		return(HXEMV_PARA);
	}
	iRet = iTlvMakeAddObj(gl_sTlvDbTermFixed, TAG_DFXX_LocalLanguage, 2, pTermParam->szLocalLanguage, TLV_CONFLICT_ERR);
	if(iRet < 0) {
        ASSERT(0);
		return(HXEMV_CORE);
	}

	// TDFxx, ISO-639-1, pinpad ����(TAG_DFXX_PinpadLanguage)
	if(strlen(pTermParam->szPinpadLanguage) == 0)
        strcpy(szBuf, "en"); // ���û�������������ʹ������, ȱʡ��Ϊ֧��Ӣ��
    else
        strcpy(szBuf, pTermParam->szPinpadLanguage);
	if(strlen(szBuf)%2 || strlen(szBuf)>32) {
		vMakeTagStr(TAG_DFXX_PinpadLanguage, pszErrTag);
		return(HXEMV_PARA);
	}
	// ����ն���pinpadͬʱ֧�ֵ�����
	iRet = iEmvMsgTableSupportedLanguage(sBuf, NULL);
	if(iRet) {
		ASSERT(0);
		return(HXEMV_CORE);
	}
	strlwr2(szBuf);
	strlwr2(sBuf);
	vGetCommonLanguage(szBuf, sBuf); // szBuf=pinpad���� sBuf=�ն�����
	if(strlen(szBuf) == 0)
		strcpy(szBuf, "en"); // ���û��ƥ��, ��Ϊ֧��Ӣ��
	iRet = iTlvMakeAddObj(gl_sTlvDbTermFixed, TAG_DFXX_PinpadLanguage, strlen(szBuf), szBuf, TLV_CONFLICT_ERR);
	if(iRet < 0) {
        ASSERT(0);
		return(HXEMV_CORE);
	}
	// TAG_DFXX_TermCtlsAmountLimit, �ն˷ǽӽ����޶�
	if(strlen(pTermParam->szTermCtlsAmountLimit)) {
		if(strlen(pTermParam->szTermCtlsAmountLimit) > 12) {
			vMakeTagStr(TAG_DFXX_TermCtlsAmountLimit, pszErrTag);
			return(HXEMV_PARA);
		}
		if(iTestStrDecimal(pTermParam->szTermCtlsAmountLimit, strlen(pTermParam->szTermCtlsAmountLimit)) != 0) {
			vMakeTagStr(TAG_DFXX_TermCtlsAmountLimit, pszErrTag);
			return(HXEMV_PARA); // ������ʮ�����ַ�����ʾ
		}
		memset(szBuf, '0', 12);
		szBuf[12] = 0;
		memcpy(szBuf+12-strlen(pTermParam->szTermCtlsAmountLimit), pTermParam->szTermCtlsAmountLimit, strlen(pTermParam->szTermCtlsAmountLimit));
		vTwoOne(szBuf, 12, sBuf);
		iRet = iTlvMakeAddObj(gl_sTlvDbTermFixed, TAG_DFXX_TermCtlsAmountLimit, 6, sBuf, TLV_CONFLICT_ERR);
		if(iRet < 0) {
            ASSERT(0);
			return(HXEMV_CORE);
		}
	}
	// TAG_DFXX_TermCtlsOfflineAmountLimit, �ն˷ǽ��ѻ������޶�
	if(strlen(pTermParam->szTermCtlsOfflineAmountLimit)) {
		if(strlen(pTermParam->szTermCtlsOfflineAmountLimit) > 12) {
			vMakeTagStr(TAG_DFXX_TermCtlsOfflineAmountLimit, pszErrTag);
			return(HXEMV_PARA);
		}
		if(iTestStrDecimal(pTermParam->szTermCtlsOfflineAmountLimit, strlen(pTermParam->szTermCtlsOfflineAmountLimit)) != 0) {
			vMakeTagStr(TAG_DFXX_TermCtlsOfflineAmountLimit, pszErrTag);
			return(HXEMV_PARA); // ������ʮ�����ַ�����ʾ
		}
		memset(szBuf, '0', 12);
		szBuf[12] = 0;
		memcpy(szBuf+12-strlen(pTermParam->szTermCtlsOfflineAmountLimit), pTermParam->szTermCtlsOfflineAmountLimit, strlen(pTermParam->szTermCtlsOfflineAmountLimit));
		vTwoOne(szBuf, 12, sBuf);
		iRet = iTlvMakeAddObj(gl_sTlvDbTermFixed, TAG_DFXX_TermCtlsOfflineAmountLimit, 6, sBuf, TLV_CONFLICT_ERR);
		if(iRet < 0) {
            ASSERT(0);
			return(HXEMV_CORE);
		}
	}
	// TAG_DFXX_TermCtlsCvmLimit, �ն˷ǽ�CVM�޶�
	if(strlen(pTermParam->szTermCtlsCvmLimit)) {
		if(strlen(pTermParam->szTermCtlsCvmLimit) > 12) {
			vMakeTagStr(TAG_DFXX_TermCtlsCvmLimit, pszErrTag);
			return(HXEMV_PARA);
		}
		if(iTestStrDecimal(pTermParam->szTermCtlsCvmLimit, strlen(pTermParam->szTermCtlsCvmLimit)) != 0) {
			vMakeTagStr(TAG_DFXX_TermCtlsCvmLimit, pszErrTag);
			return(HXEMV_PARA); // ������ʮ�����ַ�����ʾ
		}
		memset(szBuf, '0', 12);
		szBuf[12] = 0;
		memcpy(szBuf+12-strlen(pTermParam->szTermCtlsCvmLimit), pTermParam->szTermCtlsCvmLimit, strlen(pTermParam->szTermCtlsCvmLimit));
		vTwoOne(szBuf, 12, sBuf);
		iRet = iTlvMakeAddObj(gl_sTlvDbTermFixed, TAG_DFXX_TermCtlsCvmLimit, 6, sBuf, TLV_CONFLICT_ERR);
		if(iRet < 0) {
            ASSERT(0);
			return(HXEMV_CORE);
		}
	}

	// ���²���Ϊ�ն�ͨ�ò�����AID��ز�����������
	// T9F09, Terminal Application Ver
	if(pTermParam->AidCommonPara.ucTermAppVerExistFlag) {
		iRet = iTlvMakeAddObj(gl_sTlvDbTermFixed, TAG_9F09_AppVerTerm, 2, pTermParam->AidCommonPara.sTermAppVer, TLV_CONFLICT_ERR);
		if(iRet < 0) {
            ASSERT(0);
			return(HXEMV_CORE);
		}
	}

	// TDFxx, Default DDOL
	if(pTermParam->AidCommonPara.iDefaultDDOLLen >= 0) {
		if(pTermParam->AidCommonPara.iDefaultDDOLLen > 252) {
			vMakeTagStr(TAG_DFXX_DefaultDDOL, pszErrTag);
			return(HXEMV_PARA);
		}
		iRet = iTlvMakeAddObj(gl_sTlvDbTermFixed, TAG_DFXX_DefaultDDOL, pTermParam->AidCommonPara.iDefaultDDOLLen, pTermParam->AidCommonPara.sDefaultDDOL, TLV_CONFLICT_ERR);
		if(iRet < 0) {
            ASSERT(0);
			return(HXEMV_CORE);
		}
	}

	// TDFxx, Default TDOL
	if(pTermParam->AidCommonPara.iDefaultTDOLLen >= 0) {
		if(pTermParam->AidCommonPara.iDefaultTDOLLen > 252) {
			vMakeTagStr(TAG_DFXX_DefaultTDOL, pszErrTag);
			return(HXEMV_PARA);
		}
		iRet = iTlvMakeAddObj(gl_sTlvDbTermFixed, TAG_DFXX_DefaultTDOL, pTermParam->AidCommonPara.iDefaultTDOLLen, pTermParam->AidCommonPara.sDefaultTDOL, TLV_CONFLICT_ERR);
		if(iRet < 0) {
            ASSERT(0);
			return(HXEMV_CORE);
		}
	}

	// TDFxx, Maximum Target Percentage to be used for Biased Random Selection��-1:�޴�����(TAG_DFXX_MaxTargetPercentage)
	if(pTermParam->AidCommonPara.iMaxTargetPercentage >= 0) {
		if(pTermParam->AidCommonPara.iMaxTargetPercentage > 99) {
			vMakeTagStr(TAG_DFXX_MaxTargetPercentage, pszErrTag);
			return(HXEMV_PARA);
		}
		sBuf[0] = (uchar)pTermParam->AidCommonPara.iMaxTargetPercentage;
		iRet = iTlvMakeAddObj(gl_sTlvDbTermFixed, TAG_DFXX_MaxTargetPercentage, 1, sBuf, TLV_CONFLICT_ERR);
    	ASSERT(iRet >= 0);
		if(iRet < 0)
			return(HXEMV_CORE);
	}

	// TDFxx, Target Percentage to be used for Random Selection��-1:�޴�����(TAG_DFXX_TargetPercentage)
	if(pTermParam->AidCommonPara.iTargetPercentage >= 0) {
		if(pTermParam->AidCommonPara.iTargetPercentage > 99) {
			vMakeTagStr(TAG_DFXX_TargetPercentage, pszErrTag);
			return(HXEMV_PARA);
		}
		sBuf[0] = (uchar)pTermParam->AidCommonPara.iTargetPercentage;
		iRet = iTlvMakeAddObj(gl_sTlvDbTermFixed, TAG_DFXX_TargetPercentage, 1, sBuf, TLV_CONFLICT_ERR);
    	ASSERT(iRet >= 0);
		if(iRet < 0)
			return(HXEMV_CORE);
	}
	if(pTermParam->AidCommonPara.iMaxTargetPercentage>=0 && pTermParam->AidCommonPara.iTargetPercentage>=0) {
		// iMaxTergetPercentage��iTargetPercentageͬʱ����, �жϺϷ���
		if(pTermParam->AidCommonPara.iMaxTargetPercentage < pTermParam->AidCommonPara.iTargetPercentage) {
			vMakeTagStr(TAG_DFXX_MaxTargetPercentage, pszErrTag);
			vMakeTagStr(TAG_DFXX_TargetPercentage, pszErrTag+strlen(pszErrTag));
			return(HXEMV_PARA); // iMaxTergetPercentage����Ҫ��iTargetPercentageһ����
		}
	}

	// T9F1B, Terminal Floor Limit
	if(pTermParam->AidCommonPara.ucFloorLimitExistFlag) {
		vLongToStr(pTermParam->AidCommonPara.ulFloorLimit, 4, sBuf);
		iRet = iTlvMakeAddObj(gl_sTlvDbTermFixed, TAG_9F1B_TermFloorLimit, 4, sBuf, TLV_CONFLICT_ERR);
    	ASSERT(iRet >= 0);
		if(iRet < 0)
			return(HXEMV_CORE);
	}

	// TDFxx, Threshold Value for Biased Random Selection
	if(pTermParam->AidCommonPara.ucThresholdExistFlag) {
		vLongToStr(pTermParam->AidCommonPara.ulThresholdValue, 4, sBuf);
		iRet = iTlvMakeAddObj(gl_sTlvDbTermFixed, TAG_DFXX_RandomSelThreshold, 4, sBuf, TLV_CONFLICT_ERR);
    	ASSERT(iRet >= 0);
		if(iRet < 0)
			return(HXEMV_CORE);
	}
	if(pTermParam->AidCommonPara.ucFloorLimitExistFlag && pTermParam->AidCommonPara.ucThresholdExistFlag) {
		// ulThresholdValue��ulFloorLimitͬʱ����, �Ϸ��Լ��
		if(pTermParam->AidCommonPara.ulThresholdValue >= pTermParam->AidCommonPara.ulFloorLimit) {
			vMakeTagStr(TAG_9F1B_TermFloorLimit, pszErrTag);
			vMakeTagStr(TAG_DFXX_RandomSelThreshold, pszErrTag+strlen(pszErrTag));
			return(HXEMV_PARA); // ulThresholdValue����С��ulFloorLimit
		}
	}

	// TDFxx TAC-default
	if(pTermParam->AidCommonPara.ucTacDefaultExistFlag) {
		iRet = iTlvMakeAddObj(gl_sTlvDbTermFixed, TAG_DFXX_TACDefault, 5, pTermParam->AidCommonPara.sTacDefault, TLV_CONFLICT_ERR);
    	ASSERT(iRet >= 0);
		if(iRet < 0)
			return(HXEMV_CORE);
	}

	// TDFxx TAC-denial
	if(pTermParam->AidCommonPara.ucTacDenialExistFlag) {
		iRet = iTlvMakeAddObj(gl_sTlvDbTermFixed, TAG_DFXX_TACDenial, 5, pTermParam->AidCommonPara.sTacDenial, TLV_CONFLICT_ERR);
    	ASSERT(iRet >= 0);
		if(iRet < 0)
			return(HXEMV_CORE);
	}

	// TDFxx TAC-online
	if(pTermParam->AidCommonPara.ucTacOnlineExistFlag) {
		iRet = iTlvMakeAddObj(gl_sTlvDbTermFixed, TAG_DFXX_TACOnline, 5, pTermParam->AidCommonPara.sTacOnline, TLV_CONFLICT_ERR);
    	ASSERT(iRet >= 0);
		if(iRet < 0)
			return(HXEMV_CORE);
	}

	// TDFxx, 1:֧�� 0:��֧��(TAG_DFXX_ForceOnlineSupport)
	if(pTermParam->AidCommonPara.cForcedOnlineSupport >= 0) {
		if(pTermParam->AidCommonPara.cForcedOnlineSupport > 1) {
			vMakeTagStr(TAG_DFXX_ForceOnlineSupport, pszErrTag);
			return(HXEMV_PARA);
		}
		iRet = iTlvMakeAddObj(gl_sTlvDbTermFixed, TAG_DFXX_ForceOnlineSupport, 1, &pTermParam->AidCommonPara.cForcedOnlineSupport, TLV_CONFLICT_ERR);
		if(iRet < 0) {
			ASSERT(0);
			return(HXEMV_CORE);
		}
	}

	// TDFxx, 1:֧�� 0:��֧��(TAG_DFXX_ForceAcceptSupport)
	if(pTermParam->AidCommonPara.cForcedAcceptanceSupport >= 0) {
		if(pTermParam->AidCommonPara.cForcedAcceptanceSupport > 0) { // ���ں��Ĳ�֧��, ��Ҫ�趨Ϊ1
			vMakeTagStr(TAG_DFXX_ForceAcceptSupport, pszErrTag);
			return(HXEMV_PARA);
		}
		iRet = iTlvMakeAddObj(gl_sTlvDbTermFixed, TAG_DFXX_ForceAcceptSupport, 1, &pTermParam->AidCommonPara.cForcedAcceptanceSupport, TLV_CONFLICT_ERR);
		if(iRet < 0) {
			ASSERT(0);
			return(HXEMV_CORE);
		}
	}

	// T9F1D, Terminal Risk Management Data
	if(pTermParam->AidCommonPara.ucTermRiskDataLen) {
		if(pTermParam->AidCommonPara.ucTermRiskDataLen>8) {
			vMakeTagStr(TAG_9F1D_TermRistManaData, pszErrTag);
			return(HXEMV_PARA);
		}
		iRet = iTlvMakeAddObj(gl_sTlvDbTermFixed, TAG_9F1D_TermRistManaData, pTermParam->AidCommonPara.ucTermRiskDataLen, pTermParam->AidCommonPara.sTermRiskData, TLV_CONFLICT_ERR);
		if(iRet < 0) {
            ASSERT(0);
			return(HXEMV_CORE);
		}
	}

	// TDFxx, 1:֧�ֵ����ֽ� 0:��֧�ֵ����ֽ�(TAG_DFXX_ECTermSupportIndicator)
	// ע: ע:GPO��ҪT9F7A��ָʾ, ���׿�ʼ�����жϷ��ϵ����ֽ�֧���������, �趨T9F7A��ֵ
	if(pTermParam->AidCommonPara.cECashSupport >= 0) {
		if(pTermParam->AidCommonPara.cECashSupport > 1) {
			vMakeTagStr(TAG_DFXX_ECTermSupportIndicator, pszErrTag);
			return(HXEMV_PARA);
		}
		iRet = iTlvMakeAddObj(gl_sTlvDbTermFixed, TAG_DFXX_ECTermSupportIndicator, 1, &pTermParam->AidCommonPara.cECashSupport, TLV_CONFLICT_ERR);
		if(iRet < 0) {
			ASSERT(0);
			return(HXEMV_CORE);
		}
	}

	// T9F7B, �ն˵����ֽ����޶�
	if(strlen(pTermParam->AidCommonPara.szTermECashTransLimit)) {
		if(strlen(pTermParam->AidCommonPara.szTermECashTransLimit) > 12) {
			vMakeTagStr(TAG_9F7B_ECTermTransLimit, pszErrTag);
			return(HXEMV_PARA);
		}
		if(iTestStrDecimal(pTermParam->AidCommonPara.szTermECashTransLimit, strlen(pTermParam->AidCommonPara.szTermECashTransLimit)) != 0) {
			vMakeTagStr(TAG_9F7B_ECTermTransLimit, pszErrTag);
			return(HXEMV_PARA); // ������ʮ�����ַ�����ʾ
		}
		memset(szBuf, '0', 12);
		szBuf[12] = 0;
		memcpy(szBuf+12-strlen(pTermParam->AidCommonPara.szTermECashTransLimit), pTermParam->AidCommonPara.szTermECashTransLimit, strlen(pTermParam->AidCommonPara.szTermECashTransLimit));
		vTwoOne(szBuf, 12, sBuf);
		iRet = iTlvMakeAddObj(gl_sTlvDbTermFixed, TAG_9F7B_ECTermTransLimit, 6, sBuf, TLV_CONFLICT_ERR);
		if(iRet < 0) {
            ASSERT(0);
			return(HXEMV_CORE);
		}
	}

	// TDFxx, 1:֧������PIN 0:��֧������PIN(TAG_DFXX_OnlinePINSupport)
	if(pTermParam->AidCommonPara.cOnlinePinSupport >= 0) {
		if(pTermParam->AidCommonPara.cOnlinePinSupport > 1) {
			vMakeTagStr(TAG_DFXX_OnlinePINSupport, pszErrTag);
			return(HXEMV_PARA);
		}
		iRet = iTlvMakeAddObj(gl_sTlvDbTermFixed, TAG_DFXX_OnlinePINSupport, 1, &pTermParam->AidCommonPara.cOnlinePinSupport, TLV_CONFLICT_ERR);
		if(iRet < 0) {
			ASSERT(0);
			return(HXEMV_CORE);
		}
	}

    return(HXEMV_OK);
}

// װ���ն�֧�ֵ�Aid
// in  : pTermAid          : �ն�֧�ֵ�Aid
//       iFlag             : ����, HXEMV_PARA_INIT:��ʼ�� HXEMV_PARA_ADD:���� HXEMV_PARA_DEL:ɾ��
// out : pszErrTag         : ������ó���, �����Tagֵ
// ret : HXEMV_OK          : OK
//       HXEMV_LACK_MEMORY : �洢�ռ䲻��
//       HXEMV_PARA        : ��������, ����Tag��pszErrTag
//       HXEMV_FLOW_ERROR  : EMV���̴���
// Note: ɾ��ʱֻ����AID����
int iEmvLoadTermAid(stTermAid *pTermAid, int iFlag, uchar *pszErrTag)
{
	int i;
	int iRet;
	uchar *p;
	int   iMaxTargetPercentage, iTargetPercentage;
	ulong ulThresholdValue, ulFloorLimit;
	uchar ucThresholdValueExistFlag, ucFloorLimitExistFlag;

	switch(iFlag) {
	case HXEMV_PARA_INIT:
		gl_iTermSupportedAidNum = 0; // �ն˵�ǰ֧�ֵ�Aid�б���Ŀ
		break;
	case HXEMV_PARA_ADD:
		if(gl_iTermSupportedAidNum >= sizeof(gl_aTermSupportedAidList)/sizeof(gl_aTermSupportedAidList[0])) {
            ASSERT(0);
			return(HXEMV_LACK_MEMORY);
		}

		// �����Ϸ��Լ��
		if(pTermAid->ucAidLen<5 || pTermAid->ucAidLen>16) {
			vMakeTagStr(TAG_9F06_AID, pszErrTag);
			return(HXEMV_PARA);
		}
		if(pTermAid->ucASI > 1) {
			vMakeTagStr(TAG_DFXX_ASI, pszErrTag);
			return(HXEMV_PARA);
		}
		if(pTermAid->iDefaultDDOLLen > 252) {
			vMakeTagStr(TAG_DFXX_DefaultDDOL, pszErrTag);
			return(HXEMV_PARA);
		}
		if(pTermAid->iDefaultTDOLLen > 252) {
			vMakeTagStr(TAG_DFXX_DefaultTDOL, pszErrTag);
			return(HXEMV_PARA);
		}

		if(pTermAid->iMaxTargetPercentage > 99) {
			vMakeTagStr(TAG_DFXX_MaxTargetPercentage, pszErrTag);
			return(HXEMV_PARA);
		}
		if(pTermAid->iTargetPercentage > 99) {
			vMakeTagStr(TAG_DFXX_TargetPercentage, pszErrTag);
			return(HXEMV_PARA);
		}
		if(pTermAid->iMaxTargetPercentage>=0 && pTermAid->iTargetPercentage>=0) {
			if(pTermAid->iTargetPercentage > pTermAid->iMaxTargetPercentage) {
				vMakeTagStr(TAG_DFXX_MaxTargetPercentage, pszErrTag);
				vMakeTagStr(TAG_DFXX_TargetPercentage, pszErrTag+strlen(pszErrTag));
	    		return(HXEMV_PARA); // iMaxTergetPercentage����Ҫ��iTargetPercentageһ����
		    }
		}
		// �����ն˲�������, �ۺ��ն˲������MaxTargetPercentage��TargetPercentage�Ϸ���
		iRet = iTlvGetObjValue(gl_sTlvDbTermFixed, TAG_DFXX_MaxTargetPercentage, &p);
		if(iRet == 1)
			iMaxTargetPercentage = *p;
		else
			iMaxTargetPercentage = pTermAid->iMaxTargetPercentage;
		iRet = iTlvGetObjValue(gl_sTlvDbTermFixed, TAG_DFXX_TargetPercentage, &p);
		if(iRet == 1)
			iTargetPercentage = *p;
		else
			iTargetPercentage = pTermAid->iTargetPercentage;
		if(iMaxTargetPercentage>=0 && iTargetPercentage>=0) {
			if(iTargetPercentage > iMaxTargetPercentage) {
				vMakeTagStr(TAG_DFXX_MaxTargetPercentage, pszErrTag);
				vMakeTagStr(TAG_DFXX_TargetPercentage, pszErrTag+strlen(pszErrTag));
	    		return(HXEMV_PARA); // iMaxTergetPercentage����Ҫ��iTargetPercentageһ����
		    }
		}

		if(pTermAid->ucThresholdExistFlag && pTermAid->ucFloorLimitExistFlag) {
			if(pTermAid->ulThresholdValue >= pTermAid->ulFloorLimit) {
				vMakeTagStr(TAG_DFXX_RandomSelThreshold, pszErrTag);
				vMakeTagStr(TAG_9F1B_TermFloorLimit, pszErrTag+strlen(pszErrTag));
				return(HXEMV_PARA); // Threshold����С��FloorLimit
			}
		}
		// �����ն˲�������, �ۺ��ն˲������ThresholdValue��FloorLimit�Ϸ���
		iRet = iTlvGetObjValue(gl_sTlvDbTermFixed, TAG_DFXX_RandomSelThreshold, &p);
		if(iRet == 4) {
			ucThresholdValueExistFlag = 1;
			ulThresholdValue = ulStrToLong(p, 4);
		} else {
			ucThresholdValueExistFlag = pTermAid->ucThresholdExistFlag;
			ulThresholdValue = pTermAid->ulThresholdValue;
		}
		iRet = iTlvGetObjValue(gl_sTlvDbTermFixed, TAG_9F1B_TermFloorLimit, &p);
		if(iRet == 4) {
			ucFloorLimitExistFlag = 1;
			ulFloorLimit = ulStrToLong(p, 4);
		} else {
			ucFloorLimitExistFlag = pTermAid->ucFloorLimitExistFlag;
			ulFloorLimit = pTermAid->ulFloorLimit;
		}
		if(ucThresholdValueExistFlag && ucFloorLimitExistFlag) {
			if(ulThresholdValue >= ulFloorLimit) {
				vMakeTagStr(TAG_DFXX_RandomSelThreshold, pszErrTag);
				vMakeTagStr(TAG_9F1B_TermFloorLimit, pszErrTag+strlen(pszErrTag));
				return(HXEMV_PARA); // Threshold����С��FloorLimit
			}
		}
		// FloorLimitΪ�������, �����ն˻���AID, ������һ����Ҫ����
		if(!ucFloorLimitExistFlag) {
			vMakeTagStr(TAG_9F1B_TermFloorLimit, pszErrTag);
			return(HXEMV_PARA); // FloorLimit�������
		}

		if(pTermAid->cForcedOnlineSupport > 1) {
			vMakeTagStr(TAG_DFXX_ForceOnlineSupport, pszErrTag);
			return(HXEMV_PARA);
		}

		if(pTermAid->cForcedAcceptanceSupport > 1) {
			vMakeTagStr(TAG_DFXX_ForceAcceptSupport, pszErrTag);
			return(HXEMV_PARA);
		}

		if(pTermAid->cECashSupport > 1) {
			vMakeTagStr(TAG_DFXX_ECTermSupportIndicator, pszErrTag);
			return(HXEMV_PARA);
		}

		if(pTermAid->szTermECashTransLimit[0]) {
			if(strlen(pTermAid->szTermECashTransLimit) != 12) {
				vMakeTagStr(TAG_9F7B_ECTermTransLimit, pszErrTag);
				return(HXEMV_PARA);
			}
			if(iTestStrDecimal(pTermAid->szTermECashTransLimit, strlen(pTermAid->szTermECashTransLimit)) != 0) {
				vMakeTagStr(TAG_9F7B_ECTermTransLimit, pszErrTag);
				return(HXEMV_PARA); // ������ʮ�����ַ�����ʾ
			}
		}
		if(pTermAid->cOnlinePinSupport > 1) {
			vMakeTagStr(TAG_DFXX_OnlinePINSupport, pszErrTag);
			return(HXEMV_PARA);
		}

		// �������
		memcpy(&gl_aTermSupportedAidList[gl_iTermSupportedAidNum++], pTermAid, sizeof(stTermAid));
		break;
	case HXEMV_PARA_DEL:
		for(i=0; i<gl_iTermSupportedAidNum; i++) {
			if(gl_aTermSupportedAidList[i].ucAidLen != pTermAid->ucAidLen)
				continue;
			if(memcmp(gl_aTermSupportedAidList[i].sAid, pTermAid->sAid, pTermAid->ucAidLen) != 0)
				continue;
			// match, delete this AID
			if(i != gl_iTermSupportedAidNum-1)
				memcpy(&gl_aTermSupportedAidList[i], &gl_aTermSupportedAidList[gl_iTermSupportedAidNum-1], sizeof(stTermAid));
			gl_iTermSupportedAidNum--;
			break;
		}
		break;
	}
	return(HXEMV_OK);
}

// װ��CA��Կ
// in  : pCAPublicKey      : CA��Կ
//       iFlag             : ����, HXEMV_PARA_INIT:��ʼ�� HXEMV_PARA_ADD:���� HXEMV_PARA_DEL:ɾ��
// ret : HXEMV_OK          : OK
//       HXEMV_LACK_MEMORY : �洢�ռ䲻��
//       HXEMV_PARA        : ��������
//       HXEMV_FLOW_ERROR  : EMV���̴���
// Note: ɾ��ʱֻ����RID��index����
int iEmvLoadCAPublicKey(stCAPublicKey *pCAPublicKey, int iFlag)
{
	int i;
	switch(iFlag) {
	case HXEMV_PARA_INIT:
		gl_iTermSupportedCaPublicKeyNum = 0; // �ն˵�ǰ֧�ֵ�Aid�б���Ŀ
		break;
	case HXEMV_PARA_ADD:
		if(pCAPublicKey->ucKeyLen == 0)
			break;
		if(gl_iTermSupportedCaPublicKeyNum >= sizeof(gl_aCAPublicKeyList)/sizeof(gl_aCAPublicKeyList[0])) {
		    ASSERT(0);
			return(HXEMV_LACK_MEMORY);
		}

		// �����Ϸ��Լ��
		if(pCAPublicKey->ucKeyLen > 248) {
		    ASSERT(0);
			return(HXEMV_PARA);
		}
		if(pCAPublicKey->lE!=3L && pCAPublicKey->lE!=65537L) {
		    ASSERT(0);
			return(HXEMV_PARA);
		}
		if(iTestIfValidDate8(pCAPublicKey->szExpireDate) != 0) {
		    ASSERT(0);
			return(HXEMV_PARA);
		}
		
		// �������
		memcpy(&gl_aCAPublicKeyList[gl_iTermSupportedCaPublicKeyNum++], pCAPublicKey, sizeof(stCAPublicKey));
		break;
	case HXEMV_PARA_DEL:
		for(i=0; i<gl_iTermSupportedCaPublicKeyNum; i++) {
			if(gl_aCAPublicKeyList[i].ucIndex != pCAPublicKey->ucIndex)
				continue;
			if(memcmp(gl_aCAPublicKeyList[i].sRid, pCAPublicKey->sRid, 5) != 0)
				continue;
			// match, delete this CA public key
			if(i != gl_iTermSupportedCaPublicKeyNum-1)
				memcpy(&gl_aCAPublicKeyList[i], &gl_aCAPublicKeyList[gl_iTermSupportedCaPublicKeyNum-1], sizeof(stCAPublicKey));
			gl_iTermSupportedCaPublicKeyNum--;
			break;
		}
		break;
	}
	return(HXEMV_OK);
}
