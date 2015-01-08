/**************************************
File name     : EmvIO.c
Function      : EMV/pboc2.0输入输出模块
                根据终端类型、性能决定交互对象(操作员|持卡人)
				所有回调函数(EmvFace.h声明)都通过此模块控制
Author        : Yu Jun
First edition : Apr 13th, 2012
Modified      : Jan 21st, 2013
	                根据核心调用方式(变量gl_iCoreCallFlag)决定是否调用回调函数
					只有核心调用方式为HXCORE_CALLBACK时才调用回调函数
**************************************/
/*
模块详细描述:
	EMV核心与外部交互的中间层
.	只有以回调方式初始化核心, 本模块才会调用EmvFace.h中声明的回调函数
.	回调方式时, 本模块根据终端类型, 决定显示对象, 以确定交互语言
.	记录持卡人选择的语言
		sg_szSelectedLang[3];  // 持卡人选择的语言, 如果GPO后重选应用, 不必再要求持卡人选语言
.	非回调方式时, 记录核心接口传入的账号信息, 检查黑名单及分开消费时需要
		sg_iBlackFlag;         // 黑名单标志, 0:不是黑名单 1:是黑名单
		sg_szRecentAmount[13]; // 最近借记金额
.	非回调方式时, 记录发卡行证书序列号供核心获取以判断该证书是否已被收回
		sg_sCertSerNo[3];      // 非回调方式会通过本模块保存发卡行公钥证书序列号以供应用层获取
.	非回调方式时, 保存GPO传入的金额
		sg_szAmount[13];       // 非回调方式还会通过本模块读取金额, 保存GPO传入的金额
		sg_szAmountOther[13];  // 非回调方式还会通过本模块读取其它金额, 保存GPO传入的其它金额
	****** 注意:未测试GAC与GPO金额不同情况 ******
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

static uchar sg_szAmount[13];       // 非回调方式还会通过本模块读取金额, 保存GPO传入的金额
static uchar sg_szAmountOther[13];  // 非回调方式还会通过本模块读取其它金额, 保存GPO传入的其它金额

static uchar sg_iBlackFlag;         // 黑名单标志, 0:不是黑名单 1:是黑名单
static uchar sg_szRecentAmount[13]; // 最近借记金额

static uchar sg_szSelectedLang[3];  // 持卡人选择的语言, 如果GPO后重选应用, 不必再要求持卡人选语言

static uchar sg_sCertSerNo[3];      // 非回调方式会通过本模块保存发卡行公钥证书序列号以供应用层获取

// 初始化输入输出模块
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

// 设置交易金额
// in  : pszAmount      : 金额
//       pszAmountOther : 其它金额
// ret : EMVIO_OK       : OK
//       EMVIO_ERROR    : 其它错误
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

// 设置账号信息
// in  : iBlackFlag      : 黑名单卡标志, 0:不是黑名单 1:黑名单
//       pszRecentAmount : 最近借记金额
// ret : EMVIO_OK        : OK
//       EMVIO_ERROR     : 其它错误
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

// 获取发卡行公钥证书序列号
// out : psCertSerNo    : 发卡行公钥证书序列号
// ret : EMVIO_OK       : OK
//       EMVIO_ERROR    : 其它错误
int iEmvIOGetCertSerNo(uchar *psCertSerNo)
{
	memcpy(psCertSerNo, sg_sCertSerNo, 3);
	return(EMVIO_OK);
}

// 选择语言
// out : pszSelectedLang : 选中的语言代码, 例如:"zh"
//       pszLangs        : 可选的语言列表, 例如:"frzhen"
//       pszLangsDesc    : 可选的语言描述列表, '='号结尾, 例如:"FRANCE=中文=ENGLISH="
// ret : EMVIO_OK        : OK
//       EMVIO_CANCEL    : 取消
//       EMVIO_TIMEOUT   : 超时
//       EMVIO_ERROR     : 其它错误
// Note: 不支持操作员选择语言, 此选择语言为选择持卡人语言
//       对于attended终端, 要从pinpad支持语言中选
//       对于unattended终端, 也要从pinpad支持语言中选, 因此unattended终端必须配置pinpad语言等同于终端支持语言
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
		// 本次交易选择过(非回调接口不会设置sg_szSelectedLang, 因此不会进入)
		if(pszSelectedLang)
			strcpy(pszSelectedLang, sg_szSelectedLang);
		return(EMVIO_OK);
	}

	// 获取本地语言
	iRet = iTlvGetObjValue(gl_sTlvDbTermFixed, TAG_DFXX_LocalLanguage, &psTlvValue); // attended设备，从pinpad语言中选
	ASSERT(iRet == 2);
	if(iRet != 2)
		return(EMVIO_ERROR);
	memcpy(szLocalLang, psTlvValue, 2);
	szLocalLang[2] = 0;

	// 获取终端支持的语言
	iRet = iEmvMsgTableSupportedLanguage(szTermLangs, szTermLangsDesc);
	ASSERT(iRet = 0);
	if(iRet != 0)
		return(EMVIO_ERROR);

	iTermEnv = iEmvTermEnvironment();
	if(iTermEnv == TERM_ENV_UNATTENDED) {
		// unattended终端使用终端语言
		if(pszLangs)
			strcpy(pszLangs, szTermLangs);
		if(pszLangsDesc)
			strcpy(pszLangsDesc, szTermLangsDesc);
		if(gl_iCoreCallFlag != HXCORE_CALLBACK)
			return(EMVIO_OK); // 核心调用方式为非回调方式, 获取可选择列表即可返回
		iRet = iHxFaceSelectLang(szTermLangs, szTermLangsDesc, pszSelectedLang, pszEmvMsgTableGetInfo(EMVMSG_SELECT_LANG, szLocalLang), szLocalLang);
	} else {
		// attended终端需要使用pinpad语言
		iRet = iTlvGetObjValue(gl_sTlvDbTermFixed, TAG_DFXX_PinpadLanguage, &psTlvValue); // attended设备，从pinpad语言中选
		ASSERT(iRet>0 && (iRet%2)==0);
		if(iRet<=0 || iRet%2)
			return(EMVIO_ERROR);
		memcpy(szPinpadLangs, psTlvValue, iRet);
		szPinpadLangs[iRet] = 0;
		// 取得pinpad语言描述
		szPinpadLangsDesc[0] = 0;
		for(i=0; i<(int)strlen(szPinpadLangs); i+=2) {
			for(j=0; j<(int)strlen(szTermLangs); j+=2) {
				if(memcmp(szPinpadLangs+i, szTermLangs+j, 2) == 0)
					break;
			}
			ASSERT(j < (int)strlen(szTermLangs));
			if(j >= (int)strlen(szTermLangs))
				return(EMVIO_ERROR); // pinpad语言必须被终端支持, 否则是逻辑出了问题
			// 获取j处的语言描述(j每2字节描述一种语言)
			for(p=szTermLangsDesc,k=0; k<j/2; k++) {
				p = strchr(p, '=');
				ASSERT(p);
				if(p == NULL)
					return(EMVFACE_ERROR); // 语言描述字串格式错误, '='号个数不足
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
			return(EMVIO_OK); // 核心调用方式为非回调方式, 获取可选择列表即可返回
		iRet = iHxFaceSelectLang(szPinpadLangs, szPinpadLangsDesc, pszSelectedLang, pszEmvMsgTableGetInfo(EMVMSG_SELECT_LANG, szLocalLang), szLocalLang);
	}

	if(iRet == EMVFACE_OK) {
		vMemcpy0(sg_szSelectedLang, pszSelectedLang, 2); // 保存选择的语言
		return(EMVIO_OK);
	}
	if(iRet == EMVFACE_CANCEL)
		return(EMVIO_CANCEL);
	if(iRet == EMVFACE_TIMEOUT)
		return(EMVIO_TIMEOUT);
	return(EMVIO_ERROR);
}

// 选择应用
// in  : pAppList      : 卡片与终端都支持的应用列表
//       iAppNum       : 卡片与终端都支持的应用个数
// out : piAppNo       : 选中的应用在应用列表中得下标
// ret : EMVIO_OK      : OK
//       EMVIO_CANCEL  : 取消
//       EMVIO_TIMEOUT : 超时
//       EMVIO_ERROR   : 其它错误
// Note: 应用选择，unattended终端使用持卡人语言，attended终端使用本地语言
int iEmvIOSelectApp(stADFInfo *pAppList, int iAppNum, int *piAppNo)
{
    int   iRet;
	uchar *psTlvValue;
	int   iTermEnv; // attended or unattended
	uchar szLanguage[3];

	if(gl_iCoreCallFlag != HXCORE_CALLBACK)
		return(EMVIO_OK); // 核心调用方式为非回调方式

	iTermEnv = iEmvTermEnvironment();
	if(iTermEnv == TERM_ENV_UNATTENDED)
		iRet = iTlvGetObjValue(gl_sTlvDbTermVar, TAG_DFXX_LanguageUsed, &psTlvValue); // unattended设备, 使用持卡人语言
	else
		iRet = iTlvGetObjValue(gl_sTlvDbTermFixed, TAG_DFXX_LocalLanguage, &psTlvValue); // attended设备，使用本地语言
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

// 确认应用
// in  : pApp          : 应用
// ret : EMVIO_OK      : OK
//       EMVIO_CANCEL  : 取消
//       EMVIO_TIMEOUT : 超时
//       EMVIO_ERROR   : 其它错误
int iEmvIOConfirmApp(stADFInfo *pApp)
{
    int   iRet;
	uchar *psTlvValue;
	int   iTermEnv; // attended or unattended
	uchar szLanguage[3];

	if(gl_iCoreCallFlag != HXCORE_CALLBACK)
		return(EMVIO_OK); // 核心调用方式为非回调方式

	iTermEnv = iEmvTermEnvironment();
	if(iTermEnv == TERM_ENV_UNATTENDED)
		iRet = iTlvGetObjValue(gl_sTlvDbTermVar, TAG_DFXX_LanguageUsed, &psTlvValue); // unattended设备, 使用持卡人语言
	else
		iRet = iTlvGetObjValue(gl_sTlvDbTermFixed, TAG_DFXX_LocalLanguage, &psTlvValue); // attended设备，使用本地语言
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

// 读取金额
// in  : iAmountType   : 金额类型，EMVIO_AMOUNT:Amount
//                                 EMVIO_AMOUNT_OTHER:Amount other
// out : pszAmount     : 金额[12]
// ret : EMVIO_OK      : OK
//       EMVIO_CANCEL  : 取消
//       EMVIO_TIMEOUT : 超时
//       EMVIO_ERROR   : 其它错误
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
		iRet = iTlvGetObjValue(gl_sTlvDbTermVar, TAG_DFXX_LanguageUsed, &psTlvValue); // unattended设备, 使用持卡人语言
	} else {
		iRet = iTlvGetObjValue(gl_sTlvDbTermFixed, TAG_DFXX_LocalLanguage, &psTlvValue); // attended设备，使用本地语言
	}
	ASSERT(iRet == 2);
	if(iRet != 2)
        return(EMVIO_ERROR);
	memcpy(szLanguage, psTlvValue, 2);
	szLanguage[2] = 0;

	iRet = iTlvGetObjValue(gl_sTlvDbTermVar, TAG_5F2A_TransCurrencyCode, &psTlvValue);
	if(iRet != 2)
		return(EMVIO_ERROR);
	vOneTwo0(psTlvValue, 2, szAlphaCurrencyCode); // 借用szCurrencyCode暂存一下十进制货币代码串
	iDigitalCurrencyCode = atoi(szAlphaCurrencyCode);
	iRet = iIso4217SearchDigitCode(iDigitalCurrencyCode, szAlphaCurrencyCode, &iCurrencyDecimalPosition);
	if(iRet) {
		// 没找到货币代码, 使用缺省值
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

// 读取密码
// in  : iPinType      : 密码类型, EMVIO_PLAIN_PIN:明文PIN
//                                 EMVIO_CIPHERED_PIN:密文PIN
//       iBypassFlag   : 允许Bypass标记, 0:不允许bypass 1:允许bypass
//       pszPan        : 账号
// out : psPin         : 4-12位明文密码('\0'结尾的字符串)或8字节密文PIN
// ret : EMVIO_OK      : OK
//       EMVIO_CANCEL  : 取消
//       EMVIO_BYPASS  : bypass，略过
//       EMVIO_TIMEOUT : 超时
//       EMVIO_ERROR   : 其它错误
int iEmvIOGetPin(int iPinType, int iBypassFlag, uchar *psPin, uchar *pszPan)
{
    int   iRet;
    int   iZeroFlag;
	uchar *psTlvValue;
	uchar szLanguage[3];
	uchar szAmountStr[20];

	if(gl_iCoreCallFlag != HXCORE_CALLBACK)
		return(EMVIO_OK); // 核心调用方式为非回调方式

	iRet = iTlvGetObjValue(gl_sTlvDbTermVar, TAG_DFXX_LanguageUsed, &psTlvValue);
	ASSERT(iRet == 2);
	if(iRet != 2)
        return(EMVIO_ERROR);
	memcpy(szLanguage, psTlvValue, 2);
	szLanguage[2] = 0;

	// form amount string
	iZeroFlag = iMakeAmountStr(szAmountStr);
    if(iZeroFlag <= 0) {
        // 出错或金额为0, 不显示金额
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

// 验证持卡人身份证件
// in  : iBypassFlag    : 允许Bypass标记, 0:不允许bypass 1:允许bypass
//       ucHolderIdType : 证件类型
//       szHolderId     : 证件号码
// ret : EMVIO_OK       : OK
//       EMVIO_CANCEL   : 取消
//       EMVIO_BYPASS   : bypass，略过或证件不符
//       EMVIO_TIMEOUT  : 超时
//       EMVIO_ERROR    : 其它错误
int iEmvIOVerifyHolderId(int iBypassFlag, uchar ucHolderIdType, uchar *pszHolderId)
{
    int   iRet;
	uchar *psTlvValue;
	uchar szLanguage[3];
	uchar szHolderIdType[40];

	if(gl_iCoreCallFlag != HXCORE_CALLBACK)
		return(EMVIO_OK); // 核心调用方式为非回调方式

	iRet = iTlvGetObjValue(gl_sTlvDbTermFixed, TAG_DFXX_LocalLanguage, &psTlvValue); // 使用本地语言
	ASSERT(iRet == 2);
	if(iRet != 2)
        return(EMVIO_ERROR);
	memcpy(szLanguage, psTlvValue, 2);
	szLanguage[2] = 0;

	switch(ucHolderIdType) {
	case 0: // 身份证
		strcpy(szHolderIdType, pszEmvMsgTableGetInfo(EMVMSG_IDENTIFICATION_CARD, szLanguage));
		break;
	case 1: // 军官证
		strcpy(szHolderIdType, pszEmvMsgTableGetInfo(EMVMSG_CERTIFICATE_OF_OFFICERS, szLanguage));
		break;
	case 2: // 护照
		strcpy(szHolderIdType, pszEmvMsgTableGetInfo(EMVMSG_PASSPORT, szLanguage));
		break;
	case 3: // 入境证
		strcpy(szHolderIdType, pszEmvMsgTableGetInfo(EMVMSG_ARRIVAL_CARD, szLanguage));
		break;
	case 4: // 临时身份证
		strcpy(szHolderIdType, pszEmvMsgTableGetInfo(EMVMSG_TEMPORARY_IDENTITY_CARD, szLanguage));
		break;
	case 5: // 其它
		strcpy(szHolderIdType, pszEmvMsgTableGetInfo(EMVMSG_OTHER, szLanguage));
		break;
	default:
		return(EMVIO_BYPASS); // 不识别的证件类型, 认为失败
	}

	iRet = iHxFaceVerifyHolderId(iBypassFlag,
		                         pszEmvMsgTableGetInfo(EMVMSG_VERIFY_HOLDER_ID, szLanguage), // "请查验持卡人证件"
		                         pszEmvMsgTableGetInfo(EMVMSG_HOLDER_ID_TYPE, szLanguage),   // "持卡人证件类型"
		                         szHolderIdType, // 验持卡人证件类型
		                         pszEmvMsgTableGetInfo(EMVMSG_HOLDER_ID_NO, szLanguage), // "持卡人证件号码"
		                         pszHolderId,    // "持卡人证件号码"
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

// 显示信息
// in  : iMsgType      : 消息类型，见EmvMsg.h
//       iConfirmFlag  : 确认标志 EMVIO_NEED_NO_CONFIRM : 不要求确认
//                                EMVIO_NEED_CONFIRM    : 要求确认
// ret : EMVIO_OK      : OK, 如果要求确认，已经确认
//       EMVIO_CANCEL  : 取消
//       EMVIO_TIMEOUT : 超时
int iEmvIODispMsg(int iMsgType, int iConfirmFlag)
{
    int   iRet;
	uchar *psTlvValue;
	int   iTermEnv; // attended or unattended
	uchar szLanguage[3];
	uchar szAmountStr[20];
	int   iConfirmPara;

	if(gl_iCoreCallFlag != HXCORE_CALLBACK)
		return(EMVIO_OK); // 核心调用方式为非回调方式

	iTermEnv = iEmvTermEnvironment();
	if(iTermEnv == TERM_ENV_UNATTENDED)
		iRet = iTlvGetObjValue(gl_sTlvDbTermVar, TAG_DFXX_LanguageUsed, &psTlvValue); // unattended设备, 使用持卡人语言
	else
		iRet = iTlvGetObjValue(gl_sTlvDbTermFixed, TAG_DFXX_LocalLanguage, &psTlvValue); // attended设备，使用本地语言
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
		// 固定信息
		iRet = iHxFaceDispMsg(iMsgType, iConfirmPara, NULL, pszEmvMsgTableGetInfo(iMsgType, szLanguage), szLanguage);
	} else {
		//iMsgType >= EMVMSG_VAR 包含动态信息
		switch(iMsgType) {
		case EMVMSG_AMOUNT_CONFIRM:
			iMakeAmountStr(szAmountStr);
			iRet = iHxFaceDispMsg(iMsgType, iConfirmPara, pszEmvMsgTableGetInfo(iMsgType, szLanguage), szAmountStr, szLanguage);
			break;
		default:
			iRet = EMVFACE_OK;
			break; // 只支持EMVMSG_AMOUNT_CONFIRM
		}
	}
	
	if(iRet == EMVFACE_OK)
		return(EMVIO_OK);
	if(iRet == EMVFACE_CANCEL)
		return(EMVIO_CANCEL);
	return(EMVIO_TIMEOUT);
}

// 显示信息, 信息留在屏幕上, 立即退出
// in  : iMsgType      : 消息类型，见EmvMsg.h
//                       iMsgType==-1表示清除之前显示的信息
// ret : EMVIO_OK      : OK
// Note: iEmvIODispMsg()会清除本函数显示的内容
int iEmvIOShowMsg(int iMsgType)
{
    int   iRet;
	uchar *psTlvValue;
	int   iTermEnv; // attended or unattended
	uchar szLanguage[3];
	uchar szAmountStr[20];

	if(gl_iCoreCallFlag != HXCORE_CALLBACK)
		return(EMVIO_OK); // 核心调用方式为非回调方式

	if(iMsgType == -1) {
		iHxFaceShowMsg(iMsgType, NULL, NULL, "");
		return(EMVIO_OK);
	}

	iTermEnv = iEmvTermEnvironment();
	if(iTermEnv == TERM_ENV_UNATTENDED)
		iRet = iTlvGetObjValue(gl_sTlvDbTermVar, TAG_DFXX_LanguageUsed, &psTlvValue); // unattended设备, 使用持卡人语言
	else
		iRet = iTlvGetObjValue(gl_sTlvDbTermFixed, TAG_DFXX_LocalLanguage, &psTlvValue); // attended设备，使用本地语言
	ASSERT(iRet == 2);
	if(iRet != 2)
        return(EMVIO_ERROR);
	memcpy(szLanguage, psTlvValue, 2);
	szLanguage[2] = 0;

	if(iMsgType < EMVMSG_VAR) {
		// 固定信息
		iRet = iHxFaceShowMsg(iMsgType, NULL, pszEmvMsgTableGetInfo(iMsgType, szLanguage), szLanguage);
	} else {
		//iMsgType >= EMVMSG_VAR 包含动态信息
		switch(iMsgType) {
		case EMVMSG_AMOUNT_CONFIRM:
			iMakeAmountStr(szAmountStr);
			iRet = iHxFaceShowMsg(iMsgType, pszEmvMsgTableGetInfo(iMsgType, szLanguage), szAmountStr, szLanguage);
			break;
		default:
			iRet = EMVFACE_OK;
			break; // 只支持EMVMSG_AMOUNT_CONFIRM
		}
	}

	return(EMVIO_OK);
}

// 密码键盘显示信息
// in  : iMsgType      : 消息类型，见EmvMsg.h
//       iConfirmFlag  : 确认标志 EMVIO_NEED_NO_CONFIRM : 不要求确认
//                                EMVIO_NEED_CONFIRM    : 要求确认
// ret : EMVIO_OK      : OK, 如果要求确认，已经确认
//       EMVIO_CANCEL  : 取消
//       EMVIO_TIMEOUT : 超时
// Note: 对于unattended终端或attended终端但无Pinpad，该信息会显示到唯一的显示器上
int iEmvIOPinpadDispMsg(int iMsgType, int iConfirmFlag)
{
    int   iRet;
    int   iZeroFlag;
	uchar *psTlvValue;
	uchar szLanguage[3];
	uchar szAmountStr[20];
	int   iConfirmPara;

	if(gl_iCoreCallFlag != HXCORE_CALLBACK)
		return(EMVIO_OK); // 核心调用方式为非回调方式

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
		// 固定信息
		iRet = iHxFacePinpadDispMsg(iMsgType, iConfirmPara, NULL, pszEmvMsgTableGetInfo(iMsgType, szLanguage), szLanguage);
	} else {
		//iMsgType >= EMVMSG_VAR 包含动态信息
		switch(iMsgType) {
		case EMVMSG_AMOUNT_CONFIRM:
			iZeroFlag = iMakeAmountStr(szAmountStr);
            if(iZeroFlag <= 0) {
                // 出错或金额为0, 不显示金额
                szAmountStr[0] = 0;
            }
			iRet = iHxFacePinpadDispMsg(iMsgType, iConfirmPara, pszEmvMsgTableGetInfo(iMsgType, szLanguage), szAmountStr, szLanguage);
			break;
		default:
			iRet = EMVFACE_OK;
			break; // 只支持EMVMSG_AMOUNT_CONFIRM
		}
	}
	if(iRet == EMVFACE_OK)
		return(EMVIO_OK);
	if(iRet == EMVFACE_CANCEL)
		return(EMVIO_CANCEL);
	return(EMVIO_TIMEOUT);
}

// 证书回收列表查询
// in  : psCertSerNo     : 证书序列号[3]
// ret : EMVIO_OK        : 不在证书回收列表中
//       EMVIO_BLACK     : 在证书回收列表中
//       EMVIO_ERROR     : 其它错误
int iEmvIOCRLCheck(uchar *psCertSerNo)
{
	int   iRet;
	uchar *pucCaPubKeyIndex;
	uchar *psRid;

	if(gl_iCoreCallFlag != HXCORE_CALLBACK) {
		memcpy(sg_sCertSerNo, psCertSerNo, 3); // 保存发卡行证书序列号
		return(EMVIO_OK); // 核心调用方式为非回调方式
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

// 黑名单查询
// ret : EMVIO_OK        : 不是黑名单卡
//       EMVIO_BLACK     : 黑名单卡
//       EMVIO_ERROR     : 其它错误
int iEmvIOBlackCardCheck(void)
{
	int   iRet;
	uchar *p;
	uchar szPan[21], sRid[5];
	int   iPanSeqNo;

	if(gl_iCoreCallFlag != HXCORE_CALLBACK) {
		// 核心调用方式为非回调方式
		if(sg_iBlackFlag)
			return(EMVIO_BLACK);
		return(EMVIO_OK);
	}

	if(gl_iCoreCallFlag != HXCORE_CALLBACK)
		return(EMVIO_OK); // 核心调用方式为非回调方式

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
		return(EMVIO_OK); // AID必须存在
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

// 分开消费查询
// out : pszAmount       : 该账户历史记录金额(查不到传出"0"), 
// ret : EMVIO_OK        : 成功
//       EMVIO_ERROR     : 其它错误
int iEmvIOSeparateSaleCheck(uchar *pszAmount)
{
	int iRet;
	uchar *p;
	uchar *psRid, szPan[21];
	int   iPanSeqNo;

	if(gl_iCoreCallFlag != HXCORE_CALLBACK) {
		// 核心调用方式为非回调方式
		strcpy(pszAmount, sg_szRecentAmount);
		return(EMVIO_OK);
	}

	strcpy((char *)pszAmount, "0"); // 先填0
	// 取RID
	iRet = iTlvGetObjValue(gl_sTlvDbCard, TAG_4F_AID, &psRid);
	if(iRet <= 0)
		return(EMVIO_OK); // aid必须存在
	// 取Pan
	iRet = iTlvGetObjValue(gl_sTlvDbCard, TAG_5A_PAN, &p);
	if(iRet<=0 || iRet>10)
		return(EMVIO_OK); // Pan必须存在
	vOneTwo0(p, iRet, szPan);
	vTrimTailF(szPan);
	// 取Pan SeqNo
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
