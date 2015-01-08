/**************************************
File name     : EmvFunc.c
Function      : Emv��ص�ͨ�ú���
Author        : Yu Jun
First edition : Apr 14th, 2012
Modified      : Apr 1st, 2014
					����iEmvTestIfIsAtm()����
**************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "VposFace.h"
#include "Common.h"
#include "TagDef.h"
#include "Arith.h"
#include "Iso4217.h"
#include "EmvData.h"
#include "EmvCore.h"
#include "TlvFunc.h"
#include "EmvFunc.h"

// �����ն����л���
// ret : TERM_ENV_ATTENDED   : ����ֵ��
//       TERM_ENV_UNATTENDED : ����ֵ��
int iEmvTermEnvironment(void)
{
	int iRet;
	uchar *psTlvValue;

	iRet = iTlvGetObjValue(gl_sTlvDbTermFixed, TAG_9F35_TermType, &psTlvValue);
	if(iRet <= 0)
		return(TERM_ENV_UNATTENDED); // ���ı���ȷ��T9F35��ֵ���Ϸ�����
	if((*psTlvValue & 0x0F)>=0x04 && (*psTlvValue & 0x0F)<=0x06)
		return(TERM_ENV_UNATTENDED);
	return(TERM_ENV_ATTENDED);
}

// �����ն�ͨѶ����
// ret : TERM_COM_OFFLINE_ONLY : �ѻ��ն�
//       TERM_COM_ONLINE_ONLY  : �����ն�
//       TERM_COM_BOTH         : ���ѻ������������ն�
int iEmvTermCommunication(void)
{
	int iRet;
	uchar *psTlvValue;

	iRet = iTlvGetObjValue(gl_sTlvDbTermFixed, TAG_9F35_TermType, &psTlvValue); // �ն����ͱ������ã����Է���ֵ
	if(iRet <= 0)
		return(TERM_COM_ONLINE_ONLY); // ���ı���ȷ��T9F35��ֵ���Ϸ�����
	if((*psTlvValue & 0x0F)==0x01 || (*psTlvValue & 0x0F)==0x04)
		return(TERM_COM_ONLINE_ONLY);
	else if((*psTlvValue & 0x0F)==0x02 || (*psTlvValue & 0x0F)==0x05)
		return(TERM_COM_BOTH);
	return(TERM_COM_OFFLINE_ONLY); 
}

// ���������Ȩ���浽TLV���ݿ� gl_sTlvDbTermVar
// in  : pszAmount : ��Ȩ���
// ret : 0         : OK
//       1         : ����
int iEmvSaveAmount(uchar *pszAmount)
{
	int   iRet;
	uchar szAmount[13], sAmount[6];
	if(strlen(pszAmount) > 12)
		return(1);
	sprintf(szAmount, "%012s", pszAmount);
	vTwoOne(szAmount, 12, sAmount);
	// ����ΪT9F02 N��
	iRet = iTlvMakeAddObj(gl_sTlvDbTermVar, TAG_9F02_AmountN, 6, sAmount, TLV_CONFLICT_REPLACE);
	ASSERT(iRet >= 0);
	if(iRet < 0)
		return(1);
	iStrNumBaseConvert(pszAmount, 10, szAmount, 12, 16, '0');
	vTwoOne(szAmount+4, 8, sAmount); // ȡ����4�ֽ�
	// ����ΪT81 B��
	iRet = iTlvMakeAddObj(gl_sTlvDbTermVar, TAG_81_AmountB, 4, sAmount, TLV_CONFLICT_REPLACE);
	ASSERT(iRet >= 0);
	if(iRet < 0)
		return(1);
	return(0);
}

// ����EMV������ĳλ
// in  : iItem  : �������ʶ, EMV_ITEM_TVR �� EMV_ITEM_TSI
//       iBit   : λ��, refer to EmvFunc.h
//       iValue : 0 or 1
// ret : 0      : OK
// Note: ��������ʶ���λ��Խ��,�����κδ���,����0
//       ��������ڲ�����,����0
int iEmvSetItemBit(int iItem, int iBit, int iValue)
{
	int   iValueLen;
	uchar *psValue;

	if(iItem == EMV_ITEM_TVR)
		iValueLen = iTlvGetObjValue(gl_sTlvDbTermVar, TAG_95_TVR, &psValue);
	else if(iItem == EMV_ITEM_TSI)
		iValueLen = iTlvGetObjValue(gl_sTlvDbTermVar, TAG_9B_TSI, &psValue);
	else
		return(0);
	if(iValueLen < 0)
		return(0);

	if(iBit<0 || iBit>=iValueLen*8)
		return(0);
	vSetStrBit(psValue, iBit, iValue);
	return(0);
}

// ����EMV������ĳλΪ0
// in  : iItem : �������ʶ, EMV_ITEM_TVR �� EMV_ITEM_TSI
//       iBit  : λ��, refer to EmvFunc.h
// ret : 0     : OK
// Note: ��������ʶ���λ��Խ��,�����κδ���,����0
//       ��������ڲ�����,����0
int iEmvUnsetItemBit(int iItem, int iBit)
{
	int   iValueLen;
	uchar *psValue;

	if(iItem == EMV_ITEM_TVR)
		iValueLen = iTlvGetObjValue(gl_sTlvDbTermVar, TAG_95_TVR, &psValue);
	else if(iItem == EMV_ITEM_TSI)
		iValueLen = iTlvGetObjValue(gl_sTlvDbTermVar, TAG_9B_TSI, &psValue);
	else
		return(0);
	if(iValueLen < 0)
		return(0);

	if(iBit<0 || iBit>=iValueLen*8)
		return(0);
	vSetStrBit(psValue, iBit, 0);
	return(0);
}

// ����EMV������ĳλֵ
// in  : iItem : �������ʶ, EMV_ITEM_AIP �� EMV_ITEM_AUC �� EMV_ITEM_TERM_CAP �� EMV_ITEM_TERM_CAP_ADD �� EMV_ITEM_TVR
//       iBit  : λ��, refer to EmvFunc.h
// ret : 0     : ��λΪ0
//       1     : ��λΪ1
// Note: ��������ʶ���λ��Խ��,����0
//       ��������ڲ�����,����0
int iEmvTestItemBit(int iItem, int iBit)
{
	int   iValueLen;
	uchar *psValue;

	if(iItem == EMV_ITEM_AIP)
		iValueLen = iTlvGetObjValue(gl_sTlvDbCard, TAG_82_AIP, &psValue);
	else if(iItem == EMV_ITEM_AUC)
		iValueLen = iTlvGetObjValue(gl_sTlvDbCard, TAG_9F07_AUC, &psValue);
	else if(iItem == EMV_ITEM_TERM_CAP)
		iValueLen = iTlvGetObjValue(gl_sTlvDbTermFixed, TAG_9F33_TermCapability, &psValue);
	else if(iItem == EMV_ITEM_TERM_CAP_ADD)
		iValueLen = iTlvGetObjValue(gl_sTlvDbTermFixed, TAG_9F40_TermCapabilityExt, &psValue);
	else if(iItem == EMV_ITEM_TVR)
		iValueLen = iTlvGetObjValue(gl_sTlvDbTermVar, TAG_95_TVR, &psValue);
	else
		return(0);
	if(iValueLen < 0)
		return(0);

	if(iBit<0 || iBit>=iValueLen*8)
		return(0);
	return(iTestStrBit(psValue, iBit));
}

// �����Ƿ�ΪATM
// ret : 0 : ����ATM
//       1 : ��ATM
// Refer to Emv4.3 book4 Annex A1
int iEmvTestIfIsAtm(void)
{
	int iRet;
	uchar *psTlvValue;

	iRet = iTlvGetObjValue(gl_sTlvDbTermFixed, TAG_9F35_TermType, &psTlvValue);
	if(iRet <= 0)
		return(0); // ���ı���ȷ��T9F35��ֵ���Ϸ�����
	if(*psTlvValue!=0x14 && *psTlvValue!=0x15 && *psTlvValue!=0x16)
		return(0); // ATM����Ϊ14 15 16����
	iRet = iEmvTestItemBit(EMV_ITEM_TERM_CAP_ADD, TERM_CAP_ADD_00_CASH);
	if(iRet == 0)
		return(0); // �����ATM, ����֧���ֽ�
	return(1); // ��ATM
}

// out : pszAmountStr : ��ʽ������Ȩ���
// ret : 0            : ���Ϊ0
//       1            : ��Ϊ0
//       -1           : ����
// ��ʽ�����, ����123.46��Ԫ��ʽ���� "USD123.46"
int iMakeAmountStr(uchar *pszAmountStr)
{
	int   iRet;
	uchar *psTlvValue;
	int   iDigitalCurrencyCode;
	uchar szAlphaCurrencyCode[4];
	int   iCurrencyDecimalPosition;
	uchar szAmount[20];

	strcpy(pszAmountStr, "XXX0"); // ��ʼ��
	iRet = iTlvGetObjValue(gl_sTlvDbTermVar, TAG_5F2A_TransCurrencyCode, &psTlvValue);
	ASSERT(iRet == 2);
    if(iRet != 2) {
        ASSERT(0);
		return(-1);
    }
	vOneTwo0(psTlvValue, 2, szAlphaCurrencyCode); // ����szCurrencyCode�ݴ�һ��ʮ���ƻ��Ҵ��봮
	iDigitalCurrencyCode = atoi(szAlphaCurrencyCode);
	iRet = iIso4217SearchDigitCode(iDigitalCurrencyCode, szAlphaCurrencyCode, &iCurrencyDecimalPosition);
	if(iRet) {
		// û�ҵ����Ҵ���, ʹ��ȱʡֵ
//		strcpy(szAlphaCurrencyCode, "XXX");
		strcpy(szAlphaCurrencyCode, "   ");
		iCurrencyDecimalPosition = 0;
	}
	iRet = iTlvGetObjValue(gl_sTlvDbTermVar, TAG_9F02_AmountN, &psTlvValue);
	if(iRet == 6)
		vOneTwo0(psTlvValue, 6, szAmount);
	else
		strcpy(szAmount, "000000000000");
    if(memcmp(szAmount, "000000000000", 12) == 0)
        iRet = 0;
    else
        iRet = 1;
	vFormatAmount(szAmount, iCurrencyDecimalPosition); // ��ʽ�����
	sprintf(pszAmountStr, "%s%s", szAlphaCurrencyCode, szAmount);
    return(iRet);
}

// ��ȡEC���ױ�־
// ret : 0 : ����EC����
//       1 : ��EC����
//       2 : GPO����������ֽ�ʶ����, ����ǰ����δ��ECͨ��
int iEmvGetECTransFlag(void)
{
	uchar *p;
	int iRet;
	iRet = iTlvGetObjValue(gl_sTlvDbTermVar, TAG_DFXX_ECTransFlag, &p);
	if(iRet != 1)
		return(0);
	if(*p == 1)
		return(1);
	if(*p == 2)
		return(2);
	return(0);
}

// ����EC���ױ�־
// in  : iFlag : 0 : ����EC����
//               1 : ��EC����
//               2 : GPO����������ֽ�ʶ����, ����ǰ����δ��ECͨ��
// ret : 0         : OK
//       1         : error
int iEmvSetECTransFlag(int iFlag)
{
	uchar ucFlag;
	int   iRet;

	if(iFlag == 2)
		iFlag = 2;

	ucFlag = (uchar)iFlag;
	iRet = iTlvMakeAddObj(gl_sTlvDbTermVar, TAG_DFXX_ECTransFlag, 1, &ucFlag, TLV_CONFLICT_REPLACE);
	if(iRet < 0)
		return(1);
	return(0);
}
