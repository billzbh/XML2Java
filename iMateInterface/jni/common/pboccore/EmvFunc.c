/**************************************
File name     : EmvFunc.c
Function      : Emv相关的通用函数
Author        : Yu Jun
First edition : Apr 14th, 2012
Modified      : Apr 1st, 2014
					增加iEmvTestIfIsAtm()函数
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

// 返回终端运行环境
// ret : TERM_ENV_ATTENDED   : 有人值守
//       TERM_ENV_UNATTENDED : 无人值守
int iEmvTermEnvironment(void)
{
	int iRet;
	uchar *psTlvValue;

	iRet = iTlvGetObjValue(gl_sTlvDbTermFixed, TAG_9F35_TermType, &psTlvValue);
	if(iRet <= 0)
		return(TERM_ENV_UNATTENDED); // 核心必须确保T9F35的值被合法配置
	if((*psTlvValue & 0x0F)>=0x04 && (*psTlvValue & 0x0F)<=0x06)
		return(TERM_ENV_UNATTENDED);
	return(TERM_ENV_ATTENDED);
}

// 返回终端通讯能力
// ret : TERM_COM_OFFLINE_ONLY : 脱机终端
//       TERM_COM_ONLINE_ONLY  : 联机终端
//       TERM_COM_BOTH         : 有脱机能力的联机终端
int iEmvTermCommunication(void)
{
	int iRet;
	uchar *psTlvValue;

	iRet = iTlvGetObjValue(gl_sTlvDbTermFixed, TAG_9F35_TermType, &psTlvValue); // 终端类型必须配置，忽略返回值
	if(iRet <= 0)
		return(TERM_COM_ONLINE_ONLY); // 核心必须确保T9F35的值被合法配置
	if((*psTlvValue & 0x0F)==0x01 || (*psTlvValue & 0x0F)==0x04)
		return(TERM_COM_ONLINE_ONLY);
	else if((*psTlvValue & 0x0F)==0x02 || (*psTlvValue & 0x0F)==0x05)
		return(TERM_COM_BOTH);
	return(TERM_COM_OFFLINE_ONLY); 
}

// 将输入的授权金额保存到TLV数据库 gl_sTlvDbTermVar
// in  : pszAmount : 授权金额
// ret : 0         : OK
//       1         : 出错
int iEmvSaveAmount(uchar *pszAmount)
{
	int   iRet;
	uchar szAmount[13], sAmount[6];
	if(strlen(pszAmount) > 12)
		return(1);
	sprintf(szAmount, "%012s", pszAmount);
	vTwoOne(szAmount, 12, sAmount);
	// 保存为T9F02 N型
	iRet = iTlvMakeAddObj(gl_sTlvDbTermVar, TAG_9F02_AmountN, 6, sAmount, TLV_CONFLICT_REPLACE);
	ASSERT(iRet >= 0);
	if(iRet < 0)
		return(1);
	iStrNumBaseConvert(pszAmount, 10, szAmount, 12, 16, '0');
	vTwoOne(szAmount+4, 8, sAmount); // 取右面4字节
	// 保存为T81 B型
	iRet = iTlvMakeAddObj(gl_sTlvDbTermVar, TAG_81_AmountB, 4, sAmount, TLV_CONFLICT_REPLACE);
	ASSERT(iRet >= 0);
	if(iRet < 0)
		return(1);
	return(0);
}

// 设置EMV数据项某位
// in  : iItem  : 数据项标识, EMV_ITEM_TVR 或 EMV_ITEM_TSI
//       iBit   : 位号, refer to EmvFunc.h
//       iValue : 0 or 1
// ret : 0      : OK
// Note: 如果数据项不识别或位号越界,不做任何处理,返回0
//       如果发生内部错误,返回0
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

// 设置EMV数据项某位为0
// in  : iItem : 数据项标识, EMV_ITEM_TVR 或 EMV_ITEM_TSI
//       iBit  : 位号, refer to EmvFunc.h
// ret : 0     : OK
// Note: 如果数据项不识别或位号越界,不做任何处理,返回0
//       如果发生内部错误,返回0
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

// 测试EMV数据项某位值
// in  : iItem : 数据项标识, EMV_ITEM_AIP 或 EMV_ITEM_AUC 或 EMV_ITEM_TERM_CAP 或 EMV_ITEM_TERM_CAP_ADD 或 EMV_ITEM_TVR
//       iBit  : 位号, refer to EmvFunc.h
// ret : 0     : 该位为0
//       1     : 该位为1
// Note: 如果数据项不识别或位号越界,返回0
//       如果发生内部错误,返回0
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

// 测试是否为ATM
// ret : 0 : 不是ATM
//       1 : 是ATM
// Refer to Emv4.3 book4 Annex A1
int iEmvTestIfIsAtm(void)
{
	int iRet;
	uchar *psTlvValue;

	iRet = iTlvGetObjValue(gl_sTlvDbTermFixed, TAG_9F35_TermType, &psTlvValue);
	if(iRet <= 0)
		return(0); // 核心必须确保T9F35的值被合法配置
	if(*psTlvValue!=0x14 && *psTlvValue!=0x15 && *psTlvValue!=0x16)
		return(0); // ATM必须为14 15 16类型
	iRet = iEmvTestItemBit(EMV_ITEM_TERM_CAP_ADD, TERM_CAP_ADD_00_CASH);
	if(iRet == 0)
		return(0); // 如果是ATM, 必须支持现金
	return(1); // 是ATM
}

// out : pszAmountStr : 格式化后授权金额
// ret : 0            : 金额为0
//       1            : 金额不为0
//       -1           : 出错
// 格式化金额, 例如123.46美元格式化成 "USD123.46"
int iMakeAmountStr(uchar *pszAmountStr)
{
	int   iRet;
	uchar *psTlvValue;
	int   iDigitalCurrencyCode;
	uchar szAlphaCurrencyCode[4];
	int   iCurrencyDecimalPosition;
	uchar szAmount[20];

	strcpy(pszAmountStr, "XXX0"); // 初始化
	iRet = iTlvGetObjValue(gl_sTlvDbTermVar, TAG_5F2A_TransCurrencyCode, &psTlvValue);
	ASSERT(iRet == 2);
    if(iRet != 2) {
        ASSERT(0);
		return(-1);
    }
	vOneTwo0(psTlvValue, 2, szAlphaCurrencyCode); // 借用szCurrencyCode暂存一下十进制货币代码串
	iDigitalCurrencyCode = atoi(szAlphaCurrencyCode);
	iRet = iIso4217SearchDigitCode(iDigitalCurrencyCode, szAlphaCurrencyCode, &iCurrencyDecimalPosition);
	if(iRet) {
		// 没找到货币代码, 使用缺省值
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
	vFormatAmount(szAmount, iCurrencyDecimalPosition); // 格式化金额
	sprintf(pszAmountStr, "%s%s", szAlphaCurrencyCode, szAmount);
    return(iRet);
}

// 读取EC交易标志
// ret : 0 : 不是EC交易
//       1 : 是EC交易
//       2 : GPO后读出电子现金识别码, 但当前交易未走EC通道
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

// 设置EC交易标志
// in  : iFlag : 0 : 不是EC交易
//               1 : 是EC交易
//               2 : GPO后读出电子现金识别码, 但当前交易未走EC通道
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
