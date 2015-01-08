/**************************************
File name     : EmvCvm.c
Function      : Emv持卡人认证模块(回调接口)
Author        : Yu Jun
First edition : Apr 25th, 2012
Modified      : Aug 14th, 2014
				    修正iEmvDoCardHolderVerify(), 程序判断核心是否支持任一种脱机密码认证方法时未判断脱机明文PIN验证
					修正iCvmCheckCondition(), 原来只判断了CVM条件码, 未判断终端支持情况
**************************************/
/*
模块详细描述:
	回调方式持卡人认证
.	支持的持卡人认证方法
	脱机明文密码、联机密文密码、持卡人签字、NO CVM REQUIRED、FAIL CVM、持卡人证件
.	支持Bypass PIN
.	支持金额确认
	输入密码认为金额已经得到持卡人确认
	如果持卡人认证结束前金额还没得到确认, 会要求持卡人确认金额
	sg_iConfirmAmountFlag用于记录金额确认标志
.	EC联机密码支持
	如果判断为EC联机交易, 且没有输入过联机密码, 要求输入联机密码
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "VposFace.h"
#include "Arith.h"
#include "Common.h"
#include "EmvCmd.h"
#include "TlvFunc.h"
#include "EmvFunc.h"
#include "TagAttr.h"
#include "EmvData.h"
#include "EmvCore.h"
#include "EmvMsg.h"
#include "EmvIo.h"
#include "EmvModul.h"
#include "TagDef.h"

static int sg_iOnlinePinBypassFlag;  // 联机密码bypass标志, 用于记录联机密码曾经被bypass过, 0:没有bypass 1:bypass过
static int sg_iOfflinePinBypassFlag; // 脱机密码bypass标志, 用于记录脱机密码曾经被bypass过, 0:没有bypass 1:bypass过
static int sg_iOfflinePinBlockFlag;  // 脱机密码已锁标志, 0:未知 1:已锁
static int sg_iConfirmAmountFlag;    // 金额确认标志, 用于记录金额是否被确认过, 0:没有 1:有

// 验证脱机明文密码
// ret : 0               : OK
//       -1              : bypass or 验证失败
//       HXEMV_CANCEL    : 用户取消
//       HXEMV_TIMEOUT   : 超时
//       HXEMV_CARD_OP   : 卡操作错
//       HXEMV_CARD_SW   : 非法卡状态字
//       HXEMV_TERMINATE : 满足拒绝条件，交易终止
//       HXEMV_CORE      : 内部错误
// Note: refer to Emv2008 book3, figure11, P108, 流程图X
static int iCvmVerifyOfflinePlainPin(void)
{
	int   iRet;
	uint  uiRet;
	uchar ucLen, *p, *psPan;
	uchar szPan[21];
	uchar szPin[12+1];
	uchar sBuf[16], sBuf2[8];
	int   iPinTryCounter = -1; // >=0:重试次数 <0:没有读出

	if(sg_iOnlinePinBypassFlag || sg_iOfflinePinBypassFlag) {
		iRet = iTlvGetObjValue(gl_sTlvDbTermFixed, TAG_DFXX_PinBypassBehavior, &p); // PIN bypass特性 0:每次bypass只表示该次bypass 1:一次bypass,后续都认为bypass
		ASSERT(iRet == 1);
		if(iRet != 1)
			return(HXEMV_CORE);
		if(*p == 1)
			return(-1); // 密码被bypass过, 认为后续都bypass
	}
	if(sg_iOfflinePinBypassFlag)
		return(-1); // 脱机密码被bypass过, 认为后续脱机码输入都bypass
	if(sg_iOfflinePinBlockFlag)
		return(-1); // 脱机密码已锁

	uiRet = uiEmvCmdGetData(TAG_9F17_PINTryCounter, &ucLen, sBuf);
	if(uiRet == 1)
		return(HXEMV_CARD_OP);
	if(uiRet == 0) {
		iRet = iTlvCheckTlvObject(sBuf);
		if(iRet == ucLen) {
			iRet = iTlvValue(sBuf, &p);
			if(iRet == 1) {// 读出了剩余密码尝试次数
				iPinTryCounter = *p;
				if(iPinTryCounter == 0) {
					// 密码尝试次数已到
					iEmvSetItemBit(EMV_ITEM_TVR, TVR_18_EXCEEDS_PIN_TRY_LIMIT, 1);
					sg_iOfflinePinBlockFlag = 1; // 脱机密码已锁
					return(-1); // 验证失败
				}
			}
		}
	}

	while(1) {
		iRet = iTlvGetObjValue(gl_sTlvDbCard, TAG_5A_PAN, &psPan);
		ASSERT(iRet>0 && iRet<=10);
		if(iRet<0 || iRet>10)
			return(HXEMV_CORE); // PAN必须存在
		vOneTwo0(psPan, iRet, szPan);
		vTrimTailF(szPan);
		sg_iConfirmAmountFlag = 1; // 输入密码同时作为金额确认
		if(iPinTryCounter == 1)
			iEmvIOPinpadDispMsg(EMVMSG_LAST_PIN, EMVIO_NEED_NO_CONFIRM); // 最后一次密码输入
		if(iEmvTermEnvironment() == TERM_ENV_ATTENDED)
			iRet = iEmvIOGetPin(EMVIO_PLAIN_PIN, 1, szPin, szPan); // attended, 允许bypass
		else
			iRet = iEmvIOGetPin(EMVIO_PLAIN_PIN, 0, szPin, szPan); // 非attended, 不允许bypass
		switch(iRet) {
		case EMVIO_OK: // OK
			break;
		case EMVIO_CANCEL: // 取消
			return(HXEMV_CANCEL);
		case EMVIO_BYPASS: // bypass，略过
			iEmvSetItemBit(EMV_ITEM_TVR, TVR_20_PIN_NOT_ENTERED, 1);
			sg_iOfflinePinBypassFlag = 1; // 脱机密码bypass
			return(-1);
		case EMVIO_TIMEOUT: // 超时
			return(HXEMV_TIMEOUT);
		default:
			iEmvSetItemBit(EMV_ITEM_TVR, TVR_19_NO_PINPAD, 1);
			return(-1);
		} // switch
		// 密码输入完毕

		// make plain pin block
		memset(sBuf, 'F', 16);
		sBuf[0] = '2';
		sBuf[1] = strlen(szPin);
		memcpy(sBuf+2, szPin, strlen(szPin));
		vTwoOne(sBuf, 16, sBuf2);
		uiRet = uiEmvCmdVerifyPin(0x80/*P2:Plain Pin*/, 8, sBuf2);
		if(uiRet == 1)
			return(HXEMV_CARD_OP);
		if(uiRet && (uiRet&0xFFF0)!=0x63C0 && uiRet!=0x6983 && uiRet!=0x6984)
			return(HXEMV_CARD_SW);
		if((uiRet&0xFFF0)==0x63C0 && (uiRet&0x000F)>0) {
			// 密码验证失败, 还有剩余验证次数
			iEmvIOPinpadDispMsg(EMVMSG_0A_INCORRECT_PIN, EMVIO_NEED_NO_CONFIRM);
			iEmvIOPinpadDispMsg(EMVMSG_13_TRY_AGAIN, EMVIO_NEED_NO_CONFIRM);
			iPinTryCounter = uiRet & 0x000F;
			continue;
		}
		if(uiRet == 0)
			break; // 验证成功
		sg_iOfflinePinBlockFlag = 1; // 脱机密码已锁
		iEmvIOPinpadDispMsg(EMVMSG_0A_INCORRECT_PIN, EMVIO_NEED_NO_CONFIRM);
		iEmvSetItemBit(EMV_ITEM_TVR, TVR_18_EXCEEDS_PIN_TRY_LIMIT, 1);
		return(-1);
	} // while(1
	
	iEmvIOPinpadDispMsg(EMVMSG_0D_PIN_OK, EMVIO_NEED_NO_CONFIRM);
	return(0); // 验证成功
}

// 验证持卡人证件
// ret : 0               : OK
//       -1              : bypass or 验证失败
//       HXEMV_CANCEL    : 用户取消
//       HXEMV_TIMEOUT   : 超时
//       HXEMV_CORE      : 内部错误
// 2013.2.19新增
static int iCvmVerifyHolderId(void)
{
	int   iRet;
	uchar *pucHolderIdType, *psHolderId;  // 持卡人证件类型, 持卡人证件号码
	uchar szHolderId[41]; // 持卡人证件号码

	iRet = iTlvGetObjValue(gl_sTlvDbCard, TAG_9F62_HolderIdType, &pucHolderIdType);
	if(iRet <= 0) {
		// 数据缺失
		iEmvSetItemBit(EMV_ITEM_TVR, TVR_02_ICC_DATA_MISSING, 1); // data missing
		return(-1); // 认为验证失败
	}
	iRet = iTlvGetObjValue(gl_sTlvDbCard, TAG_9F61_HolderId, &psHolderId);
	if(iRet <= 0) {
		// 数据缺失
		iEmvSetItemBit(EMV_ITEM_TVR, TVR_02_ICC_DATA_MISSING, 1); // data missing
		return(-1); // 认为验证失败
	}
	vMemcpy0(szHolderId, psHolderId, iRet);

	iRet = iEmvIOVerifyHolderId(1/*允许bypass*/, *pucHolderIdType, szHolderId);
	switch(iRet) {
	case EMVIO_OK: // OK
		break;
	case EMVIO_CANCEL: // 取消
		return(HXEMV_CANCEL);
	case EMVIO_BYPASS: // bypass，略过
		return(-1);
	case EMVIO_TIMEOUT: // 超时
		return(HXEMV_TIMEOUT);
	default:
		ASSERT(0);
		return(HXEMV_CORE);
	} // switch
	
	return(0); // 验证成功
}

// 输入联机密码
// ret : 0              : OK
//       HXEMV_NA       : bypass
//       HXEMV_CANCEL   : 用户取消
//       HXEMV_TIMEOUT  : 超时
//       HXEMV_CORE     : 内部错误
// Note: refer to Emv2008 book3, figure10, P107, 流程图U
int iCvmEnterOnlinePin(void)
{
	int   iRet;
	uchar *p;
	uchar sPin[8], *psPan;
	uchar szPan[21];

	if(sg_iOnlinePinBypassFlag || sg_iOfflinePinBypassFlag) {
		iRet = iTlvGetObjValue(gl_sTlvDbTermFixed, TAG_DFXX_PinBypassBehavior, &p); // PIN bypass特性 0:每次bypass只表示该次bypass 1:一次bypass,后续都认为bypass
		ASSERT(iRet == 1);
		if(iRet != 1)
			return(HXEMV_CORE);
		if(*p == 1)
			return(HXEMV_NA); // 密码被bypass过, 认为后续都bypass
	}
	if(sg_iOnlinePinBypassFlag)
		return(HXEMV_NA); // 联机密码被bypass过, 认为后续联机密码输入都bypass

	iRet = iTlvGetObjValue(gl_sTlvDbCard, TAG_5A_PAN, &psPan);
	ASSERT(iRet>0 && iRet<=10);
	if(iRet<0 || iRet>10)
		return(HXEMV_CORE); // PAN必须存在
	vOneTwo0(psPan, iRet, szPan);
	vTrimTailF(szPan);
	sg_iConfirmAmountFlag = 1; // 输入密码同时作为金额确认
	if(iEmvTermEnvironment() == TERM_ENV_ATTENDED)
		iRet = iEmvIOGetPin(EMVIO_CIPHERED_PIN, 1, sPin, szPan); // attended, 允许bypass
	else
		iRet = iEmvIOGetPin(EMVIO_CIPHERED_PIN, 0, sPin, szPan); // 非attended, 不允许bypass
	switch(iRet) {
	case EMVIO_OK: // OK
		break;
	case EMVIO_CANCEL: // 取消
		return(HXEMV_CANCEL);
	case EMVIO_BYPASS: // bypass，略过
		iEmvSetItemBit(EMV_ITEM_TVR, TVR_20_PIN_NOT_ENTERED, 1);
		sg_iOnlinePinBypassFlag = 1; // 联机密码bypass
		return(HXEMV_NA);
	case EMVIO_TIMEOUT: // 超时
		return(HXEMV_TIMEOUT);
	default:
		iEmvSetItemBit(EMV_ITEM_TVR, TVR_19_NO_PINPAD, 1);
		return(HXEMV_NA);
	} // switch
	// 密码输入完毕

	iRet = iTlvMakeAddObj(gl_sTlvDbTermVar, TAG_DFXX_EncOnlinePIN, 8, sPin, TLV_CONFLICT_REPLACE);
	ASSERT(iRet > 0);
	if(iRet < 0)
		return(HXEMV_CORE);
	return(0); // 输入完成
}

// 检查持卡人验证方法条件是否满足
// in  : psCvRule        : 持卡人验证方法
//       psCvmList       : CVM List, 用于提取X、Y金额
// ret : 0               : 满足
//       HXEMV_NA        : 不满足
//       HXEMV_CORE      : 内部错误
int iCvmCheckCondition(uchar *psCvRule, uchar *psCvmList)
{
	int   iRet;
	uint  uiTransType;
	uchar *psAppCurrencyCode, *psTransCurrencyCode;
	uchar *psTransType, *pucOnlinePinSupport;
	uchar szAmountX[13], szAmountY[13]; // CVM list金额
	uchar *psAmount, szAmount[13]; // 交易授权金额
	uchar ucCurrencyMatchFlag; // 应用货币代码与交易货币代码匹配标志, 0:不匹配 1:匹配

	ucCurrencyMatchFlag = 0;
	// 判断应用货币代码(T9F42)与交易货币代码(T5F2A)是否相等
	iRet = iTlvGetObjValue(gl_sTlvDbTermVar, TAG_5F2A_TransCurrencyCode, &psTransCurrencyCode);
	ASSERT(iRet == 2);
	if(iRet != 2)
		return(HXEMV_CORE); // 交易货币代码必须存在
	iRet = iTlvGetObjValue(gl_sTlvDbCard, TAG_9F42_AppCurrencyCode, &psAppCurrencyCode);
	if(iRet == 2) {
		// 交易货币代码存在且格式正确
		if(memcmp(psTransCurrencyCode, psAppCurrencyCode, 2) == 0)
			ucCurrencyMatchFlag = 1; // 应用货币代码与交易货币代码匹配
	} else if(iRet > 0)
		return(HXEMV_CORE); // 应用货币代码格式错误
	if(ucCurrencyMatchFlag) {
		// 取出金额X与金额Y
		sprintf(szAmountX, "%lu", ulStrToLong(psCvmList, 4));
		sprintf(szAmountY, "%lu", ulStrToLong(psCvmList+4, 4));
	}

	iRet = iTlvGetObjValue(gl_sTlvDbTermVar, TAG_DFXX_TransType, &psTransType); // 取得交易类型
	ASSERT(iRet == 2);
	if(iRet != 2)
		return(HXEMV_CORE); // 交易类型必须存在
	uiTransType = ulStrToLong(psTransType, 2);
	iRet = iTlvGetObjValue(gl_sTlvDbTermVar, TAG_9F02_AmountN, &psAmount); // 取得授权金额
	ASSERT(iRet == 6);
	if(iRet != 6)
		return(HXEMV_CORE); // 授权金额必须存在
	vOneTwo0(psAmount, 6, szAmount);

	switch(psCvRule[1]) {
	// break  -- if terminal supports the CVM
	// return -- 处了"if terminal supports the CVM"之外的条件, 包括不识别的条件
	case 0: // always
		break; // 满足条件
	case 1: // if unattended cash
		if(uiTransType==TRANS_TYPE_CASH && iEmvTermEnvironment()==TERM_ENV_UNATTENDED)
			break; // 满足条件
		return(HXEMV_NA); // 不满足条件
	case 2: // if not unattended cash and not manual cash and not purchase with cashback
		if(uiTransType != TRANS_TYPE_CASH)
			break; // 满足条件
		return(HXEMV_NA); // 不满足条件
	case 3: // if terminal supports the CVM
		break; // break, 稍后再判断
	case 4: // if manual cash
		if(uiTransType==TRANS_TYPE_CASH && iEmvTermEnvironment()==TERM_ENV_ATTENDED)
			break; // 满足条件
		return(HXEMV_NA); // 不满足条件
	case 5: // if purchase with cashback
		return(HXEMV_NA); // 核心暂不支持cashback
	case 6: // if transaction is in the application currency and is under X value
		if(ucCurrencyMatchFlag == 0)
			return(HXEMV_NA); // 只有交易货币代码与应用货币代码匹配时才可比较金额
		if(iStrNumCompare(szAmount, szAmountX, 10) < 0)
			break; // 满足条件
		return(HXEMV_NA); // 不满足条件
	case 7: // if transaction is in the application currency and is over X value
		if(ucCurrencyMatchFlag == 0)
			return(HXEMV_NA); // 只有交易货币代码与应用货币代码匹配时才可比较金额
		if(iStrNumCompare(szAmount, szAmountX, 10) > 0)
			break; // 满足条件
		return(HXEMV_NA); // 不满足条件
	case 8: // if transaction is in the application currency and is under Y value
		if(ucCurrencyMatchFlag == 0)
			return(HXEMV_NA); // 只有交易货币代码与应用货币代码匹配时才可比较金额
		if(iStrNumCompare(szAmount, szAmountY, 10) < 0)
			break; // 满足条件
		return(HXEMV_NA); // 不满足条件
	case 9: // if transaction is in the application currency and is over Y value
		if(ucCurrencyMatchFlag == 0)
			return(HXEMV_NA); // 只有交易货币代码与应用货币代码匹配时才可比较金额
		if(iStrNumCompare(szAmount, szAmountY, 10) > 0)
			break; // 满足条件
		return(HXEMV_NA); // 不满足条件
	default:
		return(HXEMV_NA); // 不识别, 认为不满足条件
	} // switch(psCvRule[1]) {


	// if terminal supports the CVM
	switch(psCvRule[0] & 0x3F) { // b1-b6 of the first byte
	// break  -- 不支持认证方法
	// return(0) -- 支持此认证方法
	case 0: // Fail CVM processing
		return(0); // 支持该方法
	case 1: // Plaintext PIN verification performed by ICC
		if(iEmvTestItemBit(EMV_ITEM_TERM_CAP, TERM_CAP_08_PLAIN_PIN) == 0)
			break; // 不支持该方法
		return(0); // 支持该方法
	case 2: // Enciphered PIN verified online
		if(iEmvTestItemBit(EMV_ITEM_TERM_CAP, TERM_CAP_09_ENC_ONLINE_PIN) == 0)
			break; // 不支持该方法
    	iRet = iTlvGetObjValue(gl_sTlvDbTermVar, TAG_DFXX_OnlinePINSupport, &pucOnlinePinSupport); // 取得联机密码支持情况
        if(iRet == 1) {
            if(*pucOnlinePinSupport == 0)
                break; // AID不支持联机密码(如果AID中没有给出该值, 缺省认为支持联机密码)
        }
		return(0); // 支持该方法
	case 3: // Plaintext PIN verification performed by ICC and signature
		if(iEmvTestItemBit(EMV_ITEM_TERM_CAP, TERM_CAP_08_PLAIN_PIN) == 0)
			break; // 不支持该方法
		if(iEmvTestItemBit(EMV_ITEM_TERM_CAP, TERM_CAP_10_SIGNATURE) == 0)
			break; // 不支持该方法
		return(0); // 支持该方法
	case 4: // Enciphered PIN verification performed by ICC
		if(iEmvTestItemBit(EMV_ITEM_TERM_CAP, TERM_CAP_11_ENC_OFFLINE_PIN) == 0)
			break; // 不支持该方法
		return(0); // 支持该方法
	case 5: // Enciphered PIN verification performed by ICC and signature
		if(iEmvTestItemBit(EMV_ITEM_TERM_CAP, TERM_CAP_11_ENC_OFFLINE_PIN) == 0)
			break; // 不支持该方法
		if(iEmvTestItemBit(EMV_ITEM_TERM_CAP, TERM_CAP_10_SIGNATURE) == 0)
			break; // 不支持该方法
		return(0); // 支持该方法
	case 0x1E: // signature
		if(iEmvTestItemBit(EMV_ITEM_TERM_CAP, TERM_CAP_10_SIGNATURE) == 0)
			break; // 不支持该方法
		return(0); // 支持该方法
	case 0x1F: // No CVM required
		if(iEmvTestItemBit(EMV_ITEM_TERM_CAP, TERM_CAP_12_NO_CVM_REQUIRED) == 0)
			break; // 不支持该方法
		return(0); // 支持该方法
	case 0x20: // Pboc2.0 持卡人证件验证
		// 2013.2.19新增
		if(iEmvTestItemBit(EMV_ITEM_TERM_CAP, TERM_CAP_15_HOLDER_ID) == 0)
			break; // 不支持该方法
		if(iEmvTermEnvironment() != TERM_ENV_ATTENDED)
			break; // 无人值守终端不支持持卡人证件验证
		return(0); // 支持该方法
	default:
		break; // 不支持该方法
	} // switch(psCvRule[0] & 0x3F) { // b1-b6 of the first byte
	return(HXEMV_NA); // 不支持该方法
}

// 执行持卡人验证方法
// in  : psCvmResult     : 下标0-1表示持卡人验证方法
// out : psCvmResult     : 下标2表示验证结果
//       piNeedSignFlag  : 如果成功完成，返回需要签字标志，0:不需要签字 1:需要签字
// ret : 0               : 验证成功
//       -1              : 不识别验证方法
//       -2              : 识别验证方法, 不支持验证方法
//       -3              : 识别验证方法, 支持验证方法, 验证失败
//       HXEMV_CANCEL    : 用户取消
//       HXEMV_TIMEOUT   : 超时
//       HXEMV_CARD_OP   : 卡操作错
//       HXEMV_CARD_SW   : 非法卡状态字
//       HXEMV_TERMINATE : 满足拒绝条件，交易终止
//       HXEMV_CORE      : 内部错误
static int iCvmDoVerify(uchar *psCvmResult, int *piNeedSignFlag)
{
	int   iRet;
	int   iCvmRecognisedFlag; // CVM识别标志, 0:不识别 1:识别
	int   iCvmSupportFlag; // CVM支持标志, 0:不支持 1:支持
	int   iCvmPassFlag; // CVM通过标志, 0:没通过 1:通过

	iCvmSupportFlag = 0; // 首先假定不支持验证方法
	*piNeedSignFlag = 0; // 首先假定不需要签字
	iCvmRecognisedFlag = 1; // 首先假定识别该CVM
	iCvmPassFlag = 0; // 先假设CVM没有通过
	switch(psCvmResult[0] & 0x3F) {
	case 0: // Fail CVM processing
		iCvmSupportFlag = 1; // CVM支持标志, 0:不支持 1:支持
		psCvmResult[2] = 1;
		break;
	case 1: // Plaintext PIN verification performed by ICC
		if(iEmvTestItemBit(EMV_ITEM_TERM_CAP, TERM_CAP_08_PLAIN_PIN) == 0)
			break; // 不支持该方法
		iCvmSupportFlag = 1; // CVM支持标志, 0:不支持 1:支持
		iRet = iCvmVerifyOfflinePlainPin();
		if(iRet == 0) {
			// 验证成功
			iCvmPassFlag = 1;
			psCvmResult[2] = 2; // 0:Unknown 1:Failed 2:Successfull
			break;
		}
		if(iRet == -1) {
			// 验证失败
			psCvmResult[2] = 1; // 0:Unknown 1:Failed 2:Successfull
			break;
		}
		return(iRet); // 其它返回码
	case 3: // Plaintext PIN verification performed by ICC and signature
		if(iEmvTestItemBit(EMV_ITEM_TERM_CAP, TERM_CAP_08_PLAIN_PIN) == 0)
			break; // 不支持该方法
		// 需要附加签字
		if(iEmvTestItemBit(EMV_ITEM_TERM_CAP, TERM_CAP_10_SIGNATURE) == 0)
			break; // 不支持该方法
		*piNeedSignFlag = 1; // 需要签字

		iCvmSupportFlag = 1; // CVM支持标志, 0:不支持 1:支持
		iRet = iCvmVerifyOfflinePlainPin();
		if(iRet == 0) {
			// 验证成功
			iCvmPassFlag = 1;
			psCvmResult[2] = 0; // 0:Unknown 1:Failed 2:Successfull
			break;
		}
		if(iRet == -1) {
			// 验证失败
			psCvmResult[2] = 1; // 0:Unknown 1:Failed 2:Successfull
			break;
		}
		return(iRet); // 其它返回码
	case 2: // Enciphered PIN verified online
		if(iEmvTestItemBit(EMV_ITEM_TERM_CAP, TERM_CAP_09_ENC_ONLINE_PIN) == 0)
			break; // 不支持该方法
		iCvmSupportFlag = 1; // CVM支持标志, 0:不支持 1:支持
		iRet = iCvmEnterOnlinePin();
		if(iRet == 0) {
			// 输入PIN
			iCvmPassFlag = 1;
			psCvmResult[2] = 0; // 0:Unknown 1:Failed 2:Successfull
			iEmvSetItemBit(EMV_ITEM_TVR, TVR_21_ONLINE_PIN_ENTERED, 1);
			break;
		}
		if(iRet == HXEMV_NA) {
			// 验证失败
			psCvmResult[2] = 1; // 0:Unknown 1:Failed 2:Successfull
			break;
		}
		return(iRet); // 其它返回码
		break;
	case 4: // Enciphered PIN verification performed by ICC
		if(iEmvTestItemBit(EMV_ITEM_TERM_CAP, TERM_CAP_11_ENC_OFFLINE_PIN) == 0)
			break; // 不支持该方法
		break; // 没有实现, 即使终端性能指示位表示支持脱机密文密码认证, 也无能为力:(
	case 5: // Enciphered PIN verification performed by ICC and signature
		if(iEmvTestItemBit(EMV_ITEM_TERM_CAP, TERM_CAP_11_ENC_OFFLINE_PIN) == 0)
			break; // 不支持该方法
		// 需要附加签字
		if(iEmvTestItemBit(EMV_ITEM_TERM_CAP, TERM_CAP_10_SIGNATURE) == 0)
			break; // 不支持该方法
		*piNeedSignFlag = 1; // 需要签字
		break; // 没有实现, 即使终端性能指示位表示支持脱机密文密码认证, 也无能为力:(
	case 0x1E: // signature
		if(iEmvTestItemBit(EMV_ITEM_TERM_CAP, TERM_CAP_10_SIGNATURE) == 0)
			break; // 不支持该方法
		iCvmSupportFlag = 1; // CVM支持标志, 0:不支持 1:支持
		*piNeedSignFlag = 1; // 需要签字
		psCvmResult[2] = 0; // 0:Unknown 1:Failed 2:Successfull
		iCvmPassFlag = 1; // CVM通过标志, 0:没通过 1:通过
		break;
	case 0x1F: // No CVM required
		if(iEmvTestItemBit(EMV_ITEM_TERM_CAP, TERM_CAP_12_NO_CVM_REQUIRED) == 0)
			break; // 不支持该方法
		// 支持No CVM required
		iCvmSupportFlag = 1; // CVM支持标志, 0:不支持 1:支持
		psCvmResult[2] = 2; // 0:Unknown 1:Failed 2:Successfull
		iCvmPassFlag = 1; // CVM通过标志, 0:没通过 1:通过
		break;
	case 0x20: // Pboc2.0 持卡人证件验证
		if(iEmvTestItemBit(EMV_ITEM_TERM_CAP, TERM_CAP_15_HOLDER_ID) == 0)
			break; // 不支持该方法
		if(iEmvTermEnvironment() != TERM_ENV_ATTENDED)
			break; // 无人值守终端不支持持卡人证件验证
		iCvmSupportFlag = 1; // CVM支持标志, 0:不支持 1:支持
		iRet = iCvmVerifyHolderId();
		if(iRet == 0) {
			// 验证成功
			iCvmPassFlag = 1;
			psCvmResult[2] = 2; // 0:Unknown 1:Failed 2:Successfull
			break;
		}
		if(iRet == -1) {
			// 验证失败
			psCvmResult[2] = 1; // 0:Unknown 1:Failed 2:Successfull
			break;
		}
		return(iRet); // 其它返回码
		break;
	default:
		iCvmRecognisedFlag = 0;
		break;
	}
	if(iCvmRecognisedFlag == 0) {
		// unrecognised CVM
		iEmvSetItemBit(EMV_ITEM_TVR, TVR_17_UNRECOGNISED_CVM, 1);
		return(-1); // 不识别验证方法
	}
	if(iCvmSupportFlag == 0) {
		// unsupported CVM
		return(-2); // 不支持验证方法
	}
	if(iCvmPassFlag == 0) {
		// 验证失败
		return(-3);
	}
	return(0); // 验证成功
}

// 持卡人验证
// out : piNeedSignFlag    : 如果成功完成，返回需要签字标志，0:不需要签字 1:需要签字
// ret : HXEMV_OK          : OK
//       HXEMV_CANCEL      : 用户取消
//       HXEMV_TIMEOUT     : 超时
//       HXEMV_CARD_OP     : 卡操作错
//       HXEMV_CARD_SW     : 非法卡状态字
//       HXEMV_TERMINATE   : 满足拒绝条件，交易终止
//       HXEMV_CORE        : 内部错误
// Note: refer to Emv2008 book3 10.5, P100
//                Emv2008 book4 6.3.4, P45
static int iEmvDoCardHolderVerify(int *piNeedSignFlag)
{
	int   iRet;
	uchar ucCvmListLen, *psCvmList;
	uchar sCvmResult[3];
	int   iCvmOKFlag; // 持卡人验证成功标志, 0:未成功 1:成功
	int   iCvmSupportFlag; // 持卡人验证方法支持标志, 0:没有支持的验证方法 1:至少支持一种验证方法
	int   i;

	if(iEmvTestItemBit(EMV_ITEM_AIP, AIP_03_CVM_SUPPORTED) == 0) {
		// AIP指示不支持持卡人验证
		iTlvMakeAddObj(gl_sTlvDbTermVar, TAG_9F34_CVMResult, 3, "\x3F\x00\x00", TLV_CONFLICT_REPLACE);
		return(HXEMV_OK);
	}
	iRet = iTlvGetObjValue(gl_sTlvDbCard, TAG_8E_CVMList, &psCvmList);
	if(iRet<=0 || iRet==8) {
		// 无CVM list(长度为8表示只有金额X与金额Y, 无CVM rule)
		iEmvSetItemBit(EMV_ITEM_TVR, TVR_02_ICC_DATA_MISSING, 1);
		iTlvMakeAddObj(gl_sTlvDbTermVar, TAG_9F34_CVMResult, 3, "\x3F\x00\x00", TLV_CONFLICT_REPLACE);
		return(HXEMV_OK); 
	}
	if(iRet<8 || iRet%2)
		return(HXEMV_TERMINATE); // CVM list格式错误
	ucCvmListLen = (uchar)iRet;

	// 执行持卡人验证
	iCvmOKFlag = 0; // 首先置持卡人验证未成功
	iCvmSupportFlag = 0; // 首先假设没有支持的验证方法
	sg_iOfflinePinBypassFlag = 0; // 清除脱机密码bypass标记
	sg_iOfflinePinBlockFlag = 0;  // 清除脱机密码锁定标记

	sg_iOnlinePinBypassFlag = 0; // 清除联机密码bypass标记
	for(i=8; i<(int)ucCvmListLen; i+=2) {
		memcpy(sCvmResult, psCvmList+i, 2); // get CVRule
		sCvmResult[2] = 0x01; // 1:failed, assume failed first
		iRet = iCvmCheckCondition(sCvmResult, psCvmList);
		if(iRet!=0 && iRet!=HXEMV_NA)
			return(iRet); // 出现错误, 返回
		if(iRet == HXEMV_NA)
			continue; // 条件不满足,继续下一个验证方法
		iRet = iCvmDoVerify(sCvmResult, piNeedSignFlag);
		// ret : 0               : 验证成功
		//       -1              : 不识别验证方法
		//       -2              : 识别验证方法, 不支持验证方法
		//       -3              : 识别验证方法, 支持验证方法, 验证失败
		//       HXEMV_CANCEL    : 用户取消
		//       HXEMV_TIMEOUT   : 超时
		//       HXEMV_CARD_OP   : 卡操作错
		//       HXEMV_TERMINATE : 满足拒绝条件，交易终止
		//       HXEMV_CORE      : 内部错误
		if(iRet == 0) {
			// 成功
			iCvmSupportFlag = 1;
			iCvmOKFlag = 1;
			break;
		} else if(iRet == -1) {
			// 不识别验证方法
			if((sCvmResult[0]&0x40) == 0)
				break; // b7指示如果该认证方法失败,认为持卡人认证失败,不必继续了
			else
				continue; // b7指示如果该认证方法失败,继续下一认证方法
		} else if(iRet == -2) {
			// 识别验证方法, 不支持验证方法
			// refer to Emv2008 book3 Figure 9, 流程T
			if((sCvmResult[0]&0x3F) == 2) {
				// online pin
				iEmvSetItemBit(EMV_ITEM_TVR, TVR_19_NO_PINPAD, 1);
			} else if((sCvmResult[0]&0x3F)==1 || (sCvmResult[0]&0x3F)==3 || (sCvmResult[0]&0x3F)==4 || (sCvmResult[0]&0x3F)==5) {
				// offline pin required
				if(iEmvTestItemBit(EMV_ITEM_TERM_CAP, TERM_CAP_08_PLAIN_PIN)==0
						&& iEmvTestItemBit(EMV_ITEM_TERM_CAP, TERM_CAP_11_ENC_OFFLINE_PIN)==0
						&& iEmvTestItemBit(EMV_ITEM_TERM_CAP, TERM_CAP_08_PLAIN_PIN)==0) {
					// 终端不支持任何一种脱机密码认证方法
					iEmvSetItemBit(EMV_ITEM_TVR, TVR_19_NO_PINPAD, 1);
				}
			}
		} else if(iRet == -3) {
			// iRet == -3, 识别验证方法, 支持验证方法, 验证失败
			iCvmSupportFlag = 1;
		} else {
			// 其它错误, EMV流程不能继续下去
			return(iRet);
		}
		// iRet == -2, 识别验证方法, 不支持验证方法
		// or
		// iRet == -3, 识别验证方法, 支持验证方法, 验证失败
		// 根据标志判断是否继续下一验证方法
		if((sCvmResult[0]&0x3F) == 0x00)
			break; // Fail CVM, 认证失败, 不理会b7, 不必继续了
		if((sCvmResult[0]&0x40) == 0)
			break; // b7指示如果该认证方法失败,认为持卡人认证失败,不必继续了
	} // for(i=8; i<iT8EValueLen; i+=2

	// 持卡人验证过程结束
	if(iCvmOKFlag == 0) {
		// 验证失败, refer to Emv2008 book3, Figure 12, 流程Y
		if(iCvmSupportFlag == 0) {
			// 没有支持的持卡人验证方法
			memcpy(sCvmResult, "\x3F\x00\x01", 3);
		}
		iEmvSetItemBit(EMV_ITEM_TVR, TVR_16_CVM_NOT_SUCCESS, 1); // 持卡人验证失败
	}
	iTlvMakeAddObj(gl_sTlvDbTermVar, TAG_9F34_CVMResult, 3, sCvmResult, TLV_CONFLICT_REPLACE);
	iEmvSetItemBit(EMV_ITEM_TSI, TSI_01_CVM_PERFORMED, 1); // 持卡人验证已执行
	return(HXEMV_OK);
}

// 持卡人验证
// out : piNeedSignFlag    : 如果成功完成，返回需要签字标志，0:不需要签字 1:需要签字
// ret : HXEMV_OK          : OK
//       HXEMV_CANCEL      : 用户取消
//       HXEMV_TIMEOUT     : 超时
//       HXEMV_CARD_OP     : 卡操作错
//       HXEMV_CARD_SW     : 非法卡状态字
//       HXEMV_TERMINATE   : 满足拒绝条件，交易终止
//       HXEMV_CORE        : 内部错误
// Note: refer to Emv2008 book3 10.5, P100
//                Emv2008 book4 6.3.4, P45
int iEmvCardHolderVerify(int *piNeedSignFlag)
{
	uint  uiTransType;
	int iRet;
	uchar *psTransType;

	sg_iConfirmAmountFlag = 0; // 首先假设金额没有被确认过
	iRet = iEmvDoCardHolderVerify(piNeedSignFlag);
	if(iRet)
		return(iRet); // 出错, 直接返回

	// 判断是否还需要输入EC需要的联机密码
	if(iEmvIfNeedECOnlinePin() == HXEMV_OK) {
		// 需要输入EC联机密码
		iRet = iCvmEnterOnlinePin();
		if(iRet==HXEMV_CANCEL || iRet==HXEMV_TIMEOUT || iRet==HXEMV_CORE)
			return(iRet);
	}

	if(sg_iConfirmAmountFlag)
		return(HXEMV_OK); // 金额已经被确认过, 返回
	// 确认金额
	iRet = iTlvGetObjValue(gl_sTlvDbTermVar, TAG_DFXX_TransType, &psTransType); // 取得交易类型
	ASSERT(iRet == 2);
	if(iRet != 2)
		return(HXEMV_CORE); // 交易类型必须存在
	uiTransType = ulStrToLong(psTransType, 2);
	if(uiTransType==TRANS_TYPE_AVAILABLE_INQ || uiTransType==TRANS_TYPE_BALANCE_INQ)
		return(HXEMV_OK); // 如果是余额查询, 不用确认金额
	iRet = iEmvIOPinpadDispMsg(EMVMSG_AMOUNT_CONFIRM, EMVIO_NEED_CONFIRM);
	if(iRet == EMVIO_CANCEL)
		return(HXEMV_CANCEL);
	if(iRet == EMVIO_TIMEOUT)
		return(HXEMV_TIMEOUT);
	ASSERT(iRet == 0);
	if(iRet)
		return(HXEMV_CORE);
	return(HXEMV_OK);
}
