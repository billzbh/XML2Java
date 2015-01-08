/**************************************
File name     : EmvCvm2.c
Function      : Emv持卡人认证模块(非回调接口)
Author        : Yu Jun
First edition : Apr 25th, 2012
Modified      : Jan 24th, 2013
                    修改为非回调方式接口, 源自EmvCvm.c
Modified      : Mar 20th, 2014
                    修正验证方法失败后未设置TVR持卡人验证失败位Bug
                    修正密码Bypass后未设置TVR持卡人未输入密码位Bug
                    修正联机密码输入后未保存到TLV数据库中Bug
				Mar 31st, 2014
					及早判断拒绝位, 造成一些案例不能通过, Cvm模块不再判断拒绝位
					(修正, 20140422, 根据应用类型决定是否及早判断拒绝位)
				Apr 4th, 2014
					修正iCvm2GetMethod()中因为CVM列表完结还没通过时, CVM Result未赋值Bug
					修正iCvm2DoMethod()中脱机Pin+签字时应该设置CVMResult为未知而设置为成功Bug
				Apr 22nd, 2014
					针对类似2cj.002.05 0a案例, 在已知需要拒绝时但案例还要求继续时, 根据应用类型决定是继续走流程还是拒绝
				Aug 14th, 2014
				    修正iCvm2GetMethod(), 程序判断核心是否支持任一种脱机密码认证方法时未判断脱机明文PIN验证
					修正iCvm2GetMethod(), 在持卡人验证阶段, 不做早期拒绝位检查, 因为可能造成某些关键TVR位没有置位
					修正iCvm2DoMethod(), 在持卡人验证阶段, 不做早期拒绝位检查, 因为可能造成某些关键TVR位没有置位
**************************************/
/*
模块详细描述:
	非回调方式持卡人认证
.	支持的持卡人认证方法
	脱机明文密码、联机密文密码、持卡人签字、NO CVM REQUIRED、FAIL CVM、持卡人证件
.	支持Bypass PIN
.	支持金额确认
	输入密码认为金额已经得到持卡人确认
	如果持卡人认证结束前金额还没得到确认, 会要求持卡人确认金额
	sg_iConfirmAmountFlag用于记录金额确认标志
.	EC联机密码支持
	如果判断为EC联机交易, 且没有输入过联机密码, 要求输入联机密码
.	非回调方式CVM设计思想
	读入卡片CVM列表, 从中选出满足条件的CVM, 保存到变量sg_asCvmList
	应用层进行CVM时, 从sg_asCvmList中逐一选择, 变量sg_iCvmProcIndex记录处理到的位置
	如果持卡人认证结束(成功/失败/处理完所有CVM), 转入附加处理
.	支持附加处理
	EC联机交易需要联机密码, 如果正常CVM不是联机密码, 则EC联机交易会要求输入联机密码
	金额确认, 如果之前没有任何CVM进行过持卡人金额确认, 则要求持卡人确认金额
	变量sg_iECExtProcStatus用于记录附加处理状态
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
#include "EmvCvm.h"
#include "EmvCvm2.h"

static uchar sg_asCvmList[250];		 // 满足条件的CVM列表(金额1[4]+金额2[4]+Item1[2]+Item2[2]+...)
static int   sg_iCvmListLen;         // CVM列表长度
static int   sg_iCvmProcIndex;       // 下一个要处理的CVM在CVM列表中的偏移量
                                     // -1表示尚未初始化

static int sg_iOnlinePinBypassFlag;  // 联机密码bypass标志, 用于记录联机密码曾经被bypass过, 0:没有bypass 1:bypass过
static int sg_iOfflinePinBypassFlag; // 脱机密码bypass标志, 用于记录脱机密码曾经被bypass过, 0:没有bypass 1:bypass过
static int sg_iOfflinePinBlockFlag;  // 脱机密码已锁标志, 0:未知 1:已锁
static int sg_iConfirmAmountFlag;    // 金额确认标志, 用于记录金额是否被确认过, 0:没有 1:有

static int sg_iCvmOKFlag;            // 持卡人验证成功标志, 0:未成功 1:成功
static int sg_iCvmSupportFlag;       // 存在支持的CVM标志, 0:无支持的CVM 1:有支持的CVM
static int sg_iCvmNeedSignFlag;      // 要求签字标志, 0:不要求签字 1:要求签字
static int sg_iECExtProcStatus;      // 附加处理标志状态, 0:未开始
                                     //                   1:正在处理EC联机密码附加处理 2:EC联机密码附加处理完毕
                                     //                   3:正在处理金额确认附加处理 4:金额确认附加处理完毕
                                     //                   10:附加处理完毕

static int sg_iPinTryLeft;			 // 剩余脱机密码尝试次数, 0:密码已经被锁 >0:剩余尝试次数 -1:未设置 -2:不能获取

// CVM2处理初始化, emv交易初始化时调用
void vCvm2Init(void)
{
	sg_iCvmProcIndex = -1; // 尚未初始化
	sg_iPinTryLeft = -1; // 设置剩余脱机密码尝试次数为未设置
}

// 准备CVM数据, 选出满足条件的CVM项
// ret : HXEMV_OK          : OK
//       HXEMV_TERMINATE   : CVM list格式错误, 需要终止
//       HXEMV_NA		   : 卡片不支持
//       HXEMV_CORE        : 内部错误
static int iCvm2PrepareCvmList(void)
{
	int   iRet;
	uchar ucCvmListLen, *psCvmList;
	int   i;

	// 初始化环境
	sg_iConfirmAmountFlag = 0;    // 金额没有被确认过
	sg_iOfflinePinBypassFlag = 0; // 清除脱机密码bypass标记
	sg_iOfflinePinBlockFlag = 0;  // 清除脱机密码锁定标记
	sg_iOnlinePinBypassFlag = 0;  // 清除联机密码bypass标记

	sg_iCvmListLen = 0;           // 初始化CVM列表长度, 每个项目占用2字节
    sg_iCvmProcIndex = 0;         // 下一个要处理的CVM在CVM列表中的下标, 0开始, 依次表示为2、4、6、...

	sg_iCvmOKFlag = 0;            // 持卡人验证成功标志, 0:未成功 1:成功
	sg_iCvmSupportFlag = 0;       // 存在支持的CVM标志, 0:无支持的CVM 1:有支持的CVM
	sg_iCvmNeedSignFlag = 0;      // 初始值为不要求签字
	sg_iECExtProcStatus = 0;      // 附加处理标志状态, 0:未开始

	if(iEmvTestItemBit(EMV_ITEM_AIP, AIP_03_CVM_SUPPORTED) == 0)
		return(HXEMV_NA); // AIP指示不支持持卡人验证
	iRet = iTlvGetObjValue(gl_sTlvDbCard, TAG_8E_CVMList, &psCvmList);
	if(iRet<=0 || iRet==8) {
		// 无CVM list(长度为8表示只有金额X与金额Y, 无CVM rule)
		iEmvSetItemBit(EMV_ITEM_TVR, TVR_02_ICC_DATA_MISSING, 1);
		return(HXEMV_NA); 
	}
	if(iRet<8 || iRet%2)
		return(HXEMV_TERMINATE); // CVM list格式错误
	ucCvmListLen = (uchar)iRet;

	// 提取出满足条件的持卡人验证方法
	for(i=8; i<(int)ucCvmListLen; i+=2) {
		// 判断条件码
		iRet = iCvmCheckCondition(psCvmList+i, psCvmList);
		if(iRet!=0 && iRet!=HXEMV_NA)
			return(iRet); // 出现错误, 返回
		if(iRet == HXEMV_NA)
			continue; // 条件不满足,继续下一个验证方法
		// 满足条件, 添加到列表
		memcpy(&sg_asCvmList[sg_iCvmListLen], psCvmList+i, 2);
		sg_iCvmListLen += 2;
	} // for(i=8; i<(int)ucCvmListLen; i+=2) {

	return(HXEMV_OK);
}

// 执行持卡人验证方法
// in  : psCvmResult     : 下标0-1表示持卡人验证方法
// out : psCvmResult     : 下标2表示验证结果
//       piNeedSignFlag  : 如果成功完成，返回需要签字标志，0:不需要签字 1:需要签字
// ret : 0               : 验证成功
//       -1              : 不识别验证方法
//       -2              : 识别验证方法, 不支持验证方法
//       -3              : 识别验证方法, 支持验证方法, 验证失败
//       -4              : 识别验证方法, 支持验证方法, 需要交互
//       HXEMV_CARD_OP   : 卡操作错
//       HXEMV_TERMINATE : 满足拒绝条件，交易终止
//       HXEMV_CORE      : 内部错误
static int iCvm2DoVerify(uchar *psCvmResult, int *piNeedSignFlag)
{
	int   iRet;
	uint  uiRet;
	uchar ucLen, sBuf[30], *p;
	int   iCvmRecognisedFlag; // CVM识别标志, 0:不识别 1:识别
	int   iCvmSupportFlag; // CVM支持标志, 0:不支持 1:支持
	int   iCvmPassFlag; // CVM通过标志, 0:没通过 1:通过
	int   iCvmNeedInteraction; // 需要交互标志, 0:不需要 1:需要

	iCvmSupportFlag = 0; // 首先假定不支持验证方法
	*piNeedSignFlag = 0; // 首先假定不需要签字
	iCvmRecognisedFlag = 1; // 首先假定识别该CVM
	iCvmPassFlag = 0; // 先假设CVM没有通过
	iCvmNeedInteraction = 0; // 先假设不需要交互
	switch(psCvmResult[0] & 0x3F) {
	case 0: // Fail CVM processing
		iCvmSupportFlag = 1; // CVM支持标志, 0:不支持 1:支持
		psCvmResult[2] = 1;
		break;
	case 1: // Plaintext PIN verification performed by ICC
	case 3: // Plaintext PIN verification performed by ICC and signature
		if(iEmvTestItemBit(EMV_ITEM_TERM_CAP, TERM_CAP_08_PLAIN_PIN) == 0)
			break; // 不支持该方法
		if((psCvmResult[0]&0x3F) == 3) {
			// 需要附加签字
			if(iEmvTestItemBit(EMV_ITEM_TERM_CAP, TERM_CAP_10_SIGNATURE) == 0)
				break; // 不支持该方法
			*piNeedSignFlag = 1; // 需要签字
		}
		iCvmSupportFlag = 1; // CVM支持标志, 0:不支持 1:支持
		if(sg_iOfflinePinBypassFlag || sg_iOfflinePinBlockFlag) {
			// 脱机密码bypass || 密码已锁
			psCvmResult[2] = 1; // 0:Unknown 1:Failed 2:Successfull
			break;
		}

		if(sg_iPinTryLeft == -1) {
			// sg_iPinTryLeft(0:密码已经被锁 >0:剩余尝试次数 -1:未设置 -2:不能获取)
			// 只有在sg_iPinTryLeft处于未设置状态才需要判断脱机密码是否被锁
			sg_iPinTryLeft = -2; // 先假定剩余密码尝试次数不能获取
			uiRet = uiEmvCmdGetData(TAG_9F17_PINTryCounter, &ucLen, sBuf);
			if(uiRet == 1)
				return(HXEMV_CARD_OP);
			if(uiRet == 0) {
				iRet = iTlvCheckTlvObject(sBuf);
				if(iRet == ucLen) {
					iRet = iTlvValue(sBuf, &p);
					if(iRet == 1) {// 读出了剩余密码尝试次数
						sg_iPinTryLeft = *p;
						if(*p == 0) {
							// 密码尝试次数已到
							iEmvSetItemBit(EMV_ITEM_TVR, TVR_18_EXCEEDS_PIN_TRY_LIMIT, 1);
							sg_iOfflinePinBlockFlag = 1; // 脱机密码已锁
							psCvmResult[2] = 1; // 0:Unknown 1:Failed 2:Successfull
							break;
						}
					}
				}
			}
		} // if(sg_iPinTryLeft == -1) {
		iCvmNeedInteraction = 1; // 需要交互
		break;
	case 2: // Enciphered PIN verified online
		if(iEmvTestItemBit(EMV_ITEM_TERM_CAP, TERM_CAP_09_ENC_ONLINE_PIN) == 0)
			break; // 不支持该方法
		iCvmSupportFlag = 1; // CVM支持标志, 0:不支持 1:支持
		if(sg_iOnlinePinBypassFlag) {
			// 联机密码bypass
			psCvmResult[2] = 1; // 0:Unknown 1:Failed 2:Successfull
			break;
		}
		iCvmNeedInteraction = 1; // 需要交互
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
		iCvmSupportFlag = 1; // CVM支持标志, 0:不支持 1:支持
		// 确保持卡人证件认证数据完备
		iRet = iTlvGetObjValue(gl_sTlvDbCard, TAG_9F62_HolderIdType, &p);
		if(iRet <= 0) {
			// 数据缺失
			iEmvSetItemBit(EMV_ITEM_TVR, TVR_02_ICC_DATA_MISSING, 1); // data missing
			break; // 认为验证失败
		}
		if(*p > 5)
			break; // 目前证件类型只支持0-5, 如果越界, 认为认证失败
		iRet = iTlvGetObjValue(gl_sTlvDbCard, TAG_9F61_HolderId, &p);
		if(iRet <= 0) {
			// 数据缺失
			iEmvSetItemBit(EMV_ITEM_TVR, TVR_02_ICC_DATA_MISSING, 1); // data missing
			break; // 认为验证失败
		}
		iCvmNeedInteraction = 1; // 需要交互
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
	if(iCvmNeedInteraction == 1) {
		return(-4); // 需要交互
	}
	if(iCvmPassFlag == 0) {
		// 验证失败
		return(-3);
	}
	return(0); // 验证成功
}

// 准备交互数据
// in  : psCvm             : CVM方法
// out : piCvm             : 持卡人认证方法, 支持HXCVM_PLAIN_PIN、HXCVM_CIPHERED_ONLINE_PIN、HXCVM_HOLDER_ID、HXCVM_CONFIRM_AMOUNT
//       piBypassFlag      : 允许bypass标志, 0:不允许, 1:允许
//       piMsgType         : 提示信息码(如果验证方法为证件验证, 返回证件类型码)
//       piMsgType2        : 附加提示信息, 如果不为0, 先显示该信息
//       pszAmountStr      : 提示用格式化后金额串(HXCVM_HOLDER_ID验证方法不需要)
// ret : HXEMV_OK          : OK
//       HXEMV_CORE        : 内部错误
static int iCvm2PrepareInteractData(uchar *psCvm, int *piCvm, int *piBypassFlag, int *piMsgType, int *piMsgType2, uchar *pszAmountStr)
{
	int   iRet;
	uchar *pucHolderIdType;

	*piMsgType2 = 0;

	switch(psCvm[0] & 0x3F) {
	case 1: // Plaintext PIN verification performed by ICC
	case 3: // Plaintext PIN verification performed by ICC and signature
		*piCvm = HXCVM_PLAIN_PIN;
		if(iEmvTermEnvironment() == TERM_ENV_ATTENDED)
			*piBypassFlag = 1; // 允许bypass
		else
			*piBypassFlag = 0; // 不允许bypass
		*piMsgType = EMVMSG_ENTER_PIN_OFFLINE;
		if(sg_iPinTryLeft == 1)
			*piMsgType2 = EMVMSG_LAST_PIN;// 还剩余最后一次机会
		iRet = iMakeAmountStr(pszAmountStr);
		if(iRet <= 0)
			pszAmountStr[0] = 0; // 出错或金额为0, 不显示金额
		break;
	case 2: // Enciphered PIN verified online
		*piCvm = HXCVM_CIPHERED_ONLINE_PIN;
		if(iEmvTermEnvironment() == TERM_ENV_ATTENDED)
			*piBypassFlag = 1; // 允许bypass
		else
			*piBypassFlag = 0; // 不允许bypass
		*piMsgType = EMVMSG_ENTER_PIN_ONLINE;
		iRet = iMakeAmountStr(pszAmountStr);
		if(iRet <= 0)
			pszAmountStr[0] = 0; // 出错或金额为0, 不显示金额
		break;
	case 0x20: // Pboc2.0 持卡人证件验证
		*piCvm = HXCVM_HOLDER_ID;
		*piBypassFlag = 1;   // 允许bypass
		// 由于持卡人证件验证要求返回信息较多(提示、证件类型提示、证件类型、证件号码提示、证件号码)
		// 因此对于证件验证, 只返回证件类型的代码信息, 其它信息由应用层自行提取
		iRet = iTlvGetObjValue(gl_sTlvDbCard, TAG_9F62_HolderIdType, &pucHolderIdType);
		ASSERT(iRet == 1);
		if(iRet != 1)
			return(HXEMV_CORE); // 读记录时验证过数据合法性, 获取持卡人验证方法时验证过数据存在, 不应该此时返回不合法
		if(*pucHolderIdType == 0)
			*piMsgType = EMVMSG_IDENTIFICATION_CARD; // "身份证"
		else if(*pucHolderIdType == 1)
			*piMsgType = EMVMSG_CERTIFICATE_OF_OFFICERS; // "军官证"
		else if(*pucHolderIdType == 2)
			*piMsgType = EMVMSG_PASSPORT; // "护照"
		else if(*pucHolderIdType == 3)
			*piMsgType = EMVMSG_ARRIVAL_CARD; // "入境证"
		else if(*pucHolderIdType == 4)
			*piMsgType = EMVMSG_TEMPORARY_IDENTITY_CARD; // "临时身份证"
		else if(*pucHolderIdType == 5)
			*piMsgType = EMVMSG_OTHER; // "其它"
		else {
			ASSERT(0);
			return(HXEMV_CORE);
		}
		pszAmountStr[0] = 0; // 
		break;
	default:
		ASSERT(0);
		return(HXEMV_CORE);
	}
	return(HXEMV_OK);
}

// 获取持卡人验证
// out : piCvm               : 持卡人认证方法, 支持HXCVM_PLAIN_PIN、HXCVM_CIPHERED_ONLINE_PIN、HXCVM_HOLDER_ID、HXCVM_CONFIRM_AMOUNT
//       piBypassFlag        : 允许bypass标志, 0:不允许, 1:允许
//       piMsgType           : 提示信息码(如果验证方法为证件验证, 返回证件类型码)
//       piMsgType2          : 附加提示信息, 如果不为0, 先显示该信息
//       pszAmountStr        : 提示用格式化后金额串(HXCVM_HOLDER_ID验证方法不需要)
// ret : HXEMV_OK            : OK
//       HXEMV_NO_DATA       : 无需继续进行持卡人验证
//       HXEMV_CARD_OP       : 卡操作错
//       HXEMV_TERMINATE     : 满足拒绝条件，交易终止
//       HXEMV_DENIAL        : 终端行为分析结果为脱机拒绝
//       HXEMV_DENIAL_ADVICE : 终端行为分析结果为脱机拒绝, 有Advice
//       HXEMV_FLOW_ERROR    : EMV流程错误
//       HXEMV_CORE          : 内部错误
//		 HXEMV_CALLBACK_METHOD : 没有以非回调方式初始化核心
// Note: 如果持卡人验证方法为HXCVM_HOLDER_ID, 则piMsgType&pszAmountStr都不可用, 应用层应自行提取显示组织显示信息
int iCvm2GetMethod(int *piCvm, int *piBypassFlag, int *piMsgType, int *piMsgType2, uchar *pszAmountStr)
{
	static uchar sCvmResult[3];
	int   iRet;
	int   iNeedSignFlag;

	*piMsgType2 = 0;

	// 本次交易第一次调用, 准备环境
	if(sg_iCvmProcIndex == -1) {
		iRet = iCvm2PrepareCvmList();
		if(iRet == HXEMV_TERMINATE)
			return(iRet);
		if(iRet!=HXEMV_OK && iRet!=HXEMV_NA)
			return(HXEMV_CORE);
		if(iRet == HXEMV_NA) {
			iTlvMakeAddObj(gl_sTlvDbTermVar, TAG_9F34_CVMResult, 3, "\x3F\x00\x00", TLV_CONFLICT_REPLACE);
			goto label_extra_proc;
		}
	}

	// 判断是否已经进入附加处理阶段, 如果已经进入, 直接转到附加处理
	if(sg_iECExtProcStatus != 0)
		goto label_extra_proc;

	for(; (sg_iCvmListLen-sg_iCvmProcIndex)>=2; sg_iCvmProcIndex+=2) {
		iEmvSetItemBit(EMV_ITEM_TSI, TSI_01_CVM_PERFORMED, 1); // 持卡人验证已执行
		memcpy(sCvmResult, &sg_asCvmList[sg_iCvmProcIndex], 2); // get CVRule
		sCvmResult[2] = 0x01; // 1:failed, assume failed first
		iRet = iCvm2DoVerify(sCvmResult, &iNeedSignFlag);
		// ret : 0               : 验证成功
		//       -1              : 不识别验证方法
		//       -2              : 识别验证方法, 不支持验证方法
		//       -3              : 识别验证方法, 支持验证方法, 验证失败
		//       -4              : 识别验证方法, 支持验证方法, 需要交互
		//       HXEMV_CARD_OP   : 卡操作错
		//       HXEMV_TERMINATE : 满足拒绝条件，交易终止
		//       HXEMV_CORE      : 内部错误
		if(iRet == 0) {
			// 成功
			sg_iCvmOKFlag = 1;
			sg_iCvmSupportFlag = 1;
			sg_iCvmNeedSignFlag = iNeedSignFlag;
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
			sg_iCvmSupportFlag = 1;
		} else if(iRet == -4) {
			// 需要交互
			sg_iCvmSupportFlag = 1;
			iRet = iCvm2PrepareInteractData(sCvmResult, piCvm, piBypassFlag, piMsgType, piMsgType2, pszAmountStr);
			if(iRet != HXEMV_OK)
				return(HXEMV_CORE);
#if 0
// 20140814
// 在持卡人验证阶段, 不做此类早期检查, 因为可能造成某些关键TVR位没有置位
			if(gl_iAppType != 0/*0:送检程序*/) {
				// 20140422, 针对非送检程序, 尽早判断是否可以拒绝
				//           但对送检程序, 这样做一些案例过不去, 比如2cj.002.05 0a, 因此对于送检程序则不过早判断
				iRet = iEmvIfDenial();
				if(iRet != HXEMV_OK)
					return(iRet); // 如果已经拒绝, 及早通知
			}
#endif
			return(HXEMV_OK);
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
	} // for(; (sg_iCvmListLen-sg_iCvmProcIndex)>=2; sg_iCvmProcIndex+=2) {

	// EMV认证结束
	sg_iECExtProcStatus = 1; // 不再做后续的CVM了, 准备处理EC联机密码附加处理
	// 持卡人验证过程结束
	if(sg_iCvmOKFlag == 0) {
		// 验证失败, refer to Emv2008 book3, Figure 12, 流程Y
		if(sg_iCvmSupportFlag == 0) {
			// 没有支持的持卡人验证方法
			memcpy(sCvmResult, "\x3F\x00\x01", 3);
		}
		iEmvSetItemBit(EMV_ITEM_TVR, TVR_16_CVM_NOT_SUCCESS, 1); // 持卡人验证失败
		iEmvSetItemBit(EMV_ITEM_TSI, TSI_01_CVM_PERFORMED, 1); // 持卡人验证已执行
	}
	iTlvMakeAddObj(gl_sTlvDbTermVar, TAG_9F34_CVMResult, 3, sCvmResult, TLV_CONFLICT_REPLACE);
	sg_iECExtProcStatus = 1; // 不再做后续的CVM了, 准备处理EC联机密码附加处理
	// 转入附加处理

label_extra_proc:
	// sg_asCvmList列表中无后续验证方法可用了, 此时附加检查两项可能需要的处理, 1:EC联机密码 2:金额确认
	// 附加处理状态sg_iECExtProcStatus定义 0:未开始
    //                                     1:正在处理EC联机密码附加处理 2:EC联机密码附加处理完毕
    //                                     3:正在处理金额确认附加处理 4:金额确认附加处理完毕
    //                                     10:附加处理完毕
	if(sg_iECExtProcStatus==0 || sg_iECExtProcStatus==1) {
		// 需要进行EC联机密码附加处理
		if(iEmvIfNeedECOnlinePin()==HXEMV_OK && sg_iOnlinePinBypassFlag==0) {
			// 需要输入EC联机密码 并且 联机密码没有被bypass过
			// 要求输入联机密码
			*piCvm = HXCVM_CIPHERED_ONLINE_PIN;
			if(iEmvTermEnvironment() == TERM_ENV_ATTENDED)
				*piBypassFlag = 1; // 允许bypass
			else
				*piBypassFlag = 0; // 不允许bypass
			*piMsgType = EMVMSG_ENTER_PIN_ONLINE;
			iRet = iMakeAmountStr(pszAmountStr);
			if(iRet <= 0)
				pszAmountStr[0] = 0; // 出错或金额为0, 不显示金额
			sg_iECExtProcStatus = 1; // 1:正在处理EC联机密码附加处理
			return(HXEMV_OK);
		} else {
			// 不要求输入EC联机密码 或者 联机密码被bypass过, 不再要求输入联机密码
			sg_iECExtProcStatus = 2; // 转到--> 2:EC联机密码附加处理完毕
		}
	}

	if(sg_iECExtProcStatus==2 || sg_iECExtProcStatus==3) {
		// 需要进行金额确认附加处理
		if(sg_iConfirmAmountFlag == 0) {
			// 金额没有被确认过, 进行金额确认附加处理
			*piCvm = HXCVM_CONFIRM_AMOUNT;
			*piBypassFlag = 1; // 允许bypass
			*piMsgType = EMVMSG_AMOUNT_CONFIRM;
			iRet = iMakeAmountStr(pszAmountStr);
			ASSERT(iRet >= 0);
			if(iRet < 0)
				pszAmountStr[0] = 0; // 出错, 不显示金额
			sg_iECExtProcStatus = 3; // 3:正在处理金额确认附加处理
			return(HXEMV_OK);
		} else {
			// 金额被确认过, 不再要求金额确认
			sg_iECExtProcStatus = 4; // 转到--> 4:金额确认附加处理完毕
		}
	}

	if(sg_iECExtProcStatus >= 4) {
		// 如果还有其它附加处理, 在此添加
		sg_iECExtProcStatus =10;
	}

	if(sg_iECExtProcStatus != 10) {
		ASSERT(0);
		return(HXEMV_CORE);
	}

	if(gl_iAppType != 0/*0:送检程序*/) {
		// 20140422, 针对非送检程序, 尽早判断是否可以拒绝
		//           但对送检程序, 这样做一些案例过不去, 比如2cj.002.05 0a, 因此对于送检程序则不过早判断
		iRet = iEmvIfDenial();
		if(iRet != HXEMV_OK)
			return(iRet); // 如果已经拒绝, 及早通知
	}

	return(HXEMV_NO_DATA);
}

// 执行持卡人认证
// in  : psCvmProc           : 认证方法处理方式, HXCVM_PROC_OK or HXCVM_BYPASS or HXCVM_FAIL or HXCVM_CANCEL or HXCVM_TIMEOUT
//       psCvmData           : 输入的密码, 如果为明文密码, 密码尾部要补0
// out : piMsgType           : 返回HXEMV_OK时使用
//                             如果不为0, 则需要显示本类型信息
//       piMsgType2          : 返回HXEMV_OK时使用
//                             如果不为0, 则需要显示本类型信息
// ret : HXEMV_OK            : OK, 需要继续进行持卡人验证, 继续调用iHxGetCvmMethod()
//       HXEMV_PARA  		   : 参数错误
//       HXEMV_CANCEL        : 用户取消
//       HXEMV_TIMEOUT       : 超时
//       HXEMV_CARD_OP       : 卡操作错
//       HXEMV_CARD_SW       : 非法卡状态字
//       HXEMV_TERMINATE     : 满足拒绝条件，交易终止
//       HXEMV_DENIAL        : 终端行为分析结果为脱机拒绝
//       HXEMV_DENIAL_ADVICE : 终端行为分析结果为脱机拒绝, 有Advice
//       HXEMV_FLOW_ERROR    : EMV流程错误
//       HXEMV_CORE          : 内部错误
//		 HXEMV_CALLBACK_METHOD : 没有以非回调方式初始化核心
// Note: 执行的CVM必须是最近一次iHxGetCvmMethod2()获得的CVM
int iCvm2DoMethod(int iCvmProc, uchar *psCvmData, int *piMsgType, int *piMsgType2)
{
	int   iRet;
	uint  uiRet;
	uchar *p;
	uchar szBuf[32], sBuf[16];
	uchar sCvmResult[3];

	*piMsgType = EMVMSG_00_NULL;
	*piMsgType2 = EMVMSG_00_NULL;
	if(sg_iCvmProcIndex==-1 || sg_iECExtProcStatus==10)
		return(HXEMV_FLOW_ERROR);

	if(iCvmProc == HXCVM_CANCEL)
		return(HXEMV_CANCEL);
	if(iCvmProc == HXCVM_TIMEOUT)
		return(HXEMV_TIMEOUT);

	if(sg_iECExtProcStatus == 0) {
		// 处于处理CVM列表项阶段
		memcpy(sCvmResult, &sg_asCvmList[sg_iCvmProcIndex], 2); // 当前处理的CVM
		sCvmResult[2] = 1; // 1:failed, assume failed first
		switch(sCvmResult[0] & 0x3F) {
		case 1: // Plaintext PIN verification performed by ICC
		case 3: // Plaintext PIN verification performed by ICC and signature
			sg_iConfirmAmountFlag = 1;
			if(iCvmProc == HXCVM_BYPASS) {
				iEmvSetItemBit(EMV_ITEM_TVR, TVR_20_PIN_NOT_ENTERED, 1); // add by yujun 20140320, 修正Bug
				                                                         // wlfxy, 如果CVM包含2处或以上密码验证, 后续密码验证持卡人没有bypass, 该位如何设置?
				sg_iOfflinePinBypassFlag = 1;
				// 检查pin bypass特性
				iRet = iTlvGetObjValue(gl_sTlvDbTermFixed, TAG_DFXX_PinBypassBehavior, &p); // PIN bypass特性 0:每次bypass只表示该次bypass 1:一次bypass,后续都认为bypass
				if(iRet == 1) {
					if(*p == 1) // 1:一次bypass,后续Pin都认为bypass
						sg_iOnlinePinBypassFlag = 1; // 同时设置联机Pin bypass标志
				}
			}
			if(iCvmProc == HXCVM_PROC_OK) {
				// 传入密码, 准备认证
				// make plain pin block
				if(strlen(psCvmData)<4 || strlen(psCvmData)>12) {
					ASSERT(0);
					return(HXEMV_PARA);
				}
				memset(szBuf, 'F', 16);
				szBuf[0] = '2';
				szBuf[1] = strlen(psCvmData);
				memcpy(szBuf+2, psCvmData, strlen(psCvmData));
				vTwoOne(szBuf, 16, sBuf);
				uiRet = uiEmvCmdVerifyPin(0x80/*P2:Plain Pin*/, 8, sBuf);
				if(uiRet == 1)
					return(HXEMV_CARD_OP);
				if(uiRet && (uiRet&0xFFF0)!=0x63C0 && uiRet!=0x6983 && uiRet!=0x6984)
					return(HXEMV_CARD_SW);
				if((uiRet&0xFFF0) == 0x63C0) {
					sg_iPinTryLeft = uiRet & 0x000F; // 保存剩余尝试次数
				}
				if((uiRet&0xFFF0)==0x63C0 && (uiRet&0x000F)>0) {
					// 密码验证失败, 还有剩余验证次数
					*piMsgType = EMVMSG_0A_INCORRECT_PIN;
					*piMsgType2 = EMVMSG_13_TRY_AGAIN;
					return(HXEMV_OK);
				}
				if(uiRet == 0) {
					*piMsgType = EMVMSG_0D_PIN_OK;
					sCvmResult[2] = 2; // ok
					sg_iCvmOKFlag = 1;
					sg_iECExtProcStatus = 1; // 不再做后续的CVM了, 准备处理EC联机密码附加处理
					if((sCvmResult[0]&0x3F) == 3) {
						sCvmResult[2] = 0; // 脱机密码认证成功, 但需要签字, 设置持卡人验证结果为未知
						sg_iCvmNeedSignFlag = 1; // 需要签字
					}
					iTlvMakeAddObj(gl_sTlvDbTermVar, TAG_9F34_CVMResult, 3, sCvmResult, TLV_CONFLICT_REPLACE);
					return(HXEMV_OK); // 继续进行附加处理
				}
				// 验证失败
				sg_iOfflinePinBlockFlag = 1; // 脱机密码已锁
				iEmvSetItemBit(EMV_ITEM_TVR, TVR_18_EXCEEDS_PIN_TRY_LIMIT, 1);
				*piMsgType = EMVMSG_0A_INCORRECT_PIN;
			}
			// 没传入密码 或 密码认证失败
			sCvmResult[2] = 1; // fail
			iTlvMakeAddObj(gl_sTlvDbTermVar, TAG_9F34_CVMResult, 3, sCvmResult, TLV_CONFLICT_REPLACE);
			if((sCvmResult[0]&0x40) == 0) {
				// b7指示如果该认证方法失败,认为持卡人认证失败,不必继续了
				iEmvSetItemBit(EMV_ITEM_TVR, TVR_16_CVM_NOT_SUCCESS, 1); // 持卡人验证失败, add by yujun 20140320, 修复Bug
				sg_iECExtProcStatus = 1; // 不再做后续的CVM了, 准备处理EC联机密码附加处理
			} else {
				sg_iCvmProcIndex += 2; // 过滤掉该CVM
			}
#if 0
// 20140814
// 在持卡人验证阶段, 不做此类早期检查, 因为可能造成某些关键TVR位没有置位
			if(gl_iAppType != 0/*0:送检程序*/) {
				// 20140422, 针对非送检程序, 尽早判断是否可以拒绝
				//           但对送检程序, 这样做一些案例过不去, 比如2cj.002.05 0a, 因此对于送检程序则不过早判断
				iRet = iEmvIfDenial();
				if(iRet != HXEMV_OK)
					return(iRet); // 如果已经拒绝, 及早通知
			}
#endif
			return(HXEMV_OK); // 继续下一验证方法 或 进行附加处理
		case 2: // Enciphered PIN verified online
			sg_iConfirmAmountFlag = 1;
			if(iCvmProc == HXCVM_BYPASS) {
				iEmvSetItemBit(EMV_ITEM_TVR, TVR_20_PIN_NOT_ENTERED, 1); // add by yujun 20140320, 修正Bug
				                                                         // wlfxy, 如果CVM包含2处或以上密码验证, 后续密码验证持卡人没有bypass, 该位如何设置?
				sg_iOnlinePinBypassFlag = 1;
				// 检查pin bypass特性
				iRet = iTlvGetObjValue(gl_sTlvDbTermFixed, TAG_DFXX_PinBypassBehavior, &p); // PIN bypass特性 0:每次bypass只表示该次bypass 1:一次bypass,后续都认为bypass
				if(iRet == 1) {
					if(*p == 1) // 1:一次bypass,后续Pin都认为bypass
						sg_iOfflinePinBypassFlag = 1; // 同时设置脱机Pin bypass标志
				}
			}
			if(iCvmProc == HXCVM_PROC_OK) {
				// 传入密码
				sCvmResult[2] = 0; // unknown
				iTlvMakeAddObj(gl_sTlvDbTermVar, TAG_9F34_CVMResult, 3, sCvmResult, TLV_CONFLICT_REPLACE);
				sg_iCvmOKFlag = 1;
				sg_iECExtProcStatus = 1; // 不再做后续的CVM了, 准备处理EC联机密码附加处理
				iEmvSetItemBit(EMV_ITEM_TVR, TVR_21_ONLINE_PIN_ENTERED, 1);
				iTlvMakeAddObj(gl_sTlvDbTermVar, TAG_DFXX_EncOnlinePIN, 8, psCvmData, TLV_CONFLICT_REPLACE);
				return(HXEMV_OK); // 继续进行附加处理
			}
			// 没传入密码或失败
			sCvmResult[2] = 1; // fail
			iTlvMakeAddObj(gl_sTlvDbTermVar, TAG_9F34_CVMResult, 3, sCvmResult, TLV_CONFLICT_REPLACE);
			if((sCvmResult[0]&0x40) == 0) {
				// b7指示如果该认证方法失败,认为持卡人认证失败,不必继续了
				iEmvSetItemBit(EMV_ITEM_TVR, TVR_16_CVM_NOT_SUCCESS, 1); // 持卡人验证失败, add by yujun 20140320, 修复Bug
				sg_iECExtProcStatus = 1; // 不再做后续的CVM了, 准备处理EC联机密码附加处理
			} else {
				sg_iCvmProcIndex += 2; // 过滤掉该CVM
			}
#if 0
// 20140814
// 在持卡人验证阶段, 不做此类早期检查, 因为可能造成某些关键TVR位没有置位
			if(gl_iAppType != 0/*0:送检程序*/) {
				// 20140422, 针对非送检程序, 尽早判断是否可以拒绝
				//           但对送检程序, 这样做一些案例过不去, 比如2cj.002.05 0a, 因此对于送检程序则不过早判断
				iRet = iEmvIfDenial();
				if(iRet != HXEMV_OK)
					return(iRet); // 如果已经拒绝, 及早通知
			}
#endif
			return(HXEMV_OK); // 继续下一验证方法 或 进行附加处理
		case 0x20: // Pboc2.0 持卡人证件验证
			if(iCvmProc == HXCVM_PROC_OK) {
				// 确认证件
				sCvmResult[2] = 2; // OK
				iTlvMakeAddObj(gl_sTlvDbTermVar, TAG_9F34_CVMResult, 3, sCvmResult, TLV_CONFLICT_REPLACE);
				sg_iCvmOKFlag = 1;
				sg_iECExtProcStatus = 1; // 不再做后续的CVM了, 准备处理EC联机密码附加处理
				return(HXEMV_OK); // 继续进行附加处理
			}
			// Bypass或失败
			sCvmResult[2] = 1; // fail
			iTlvMakeAddObj(gl_sTlvDbTermVar, TAG_9F34_CVMResult, 3, sCvmResult, TLV_CONFLICT_REPLACE);
			if((sCvmResult[0]&0x40) == 0) {
				// b7指示如果该认证方法失败,认为持卡人认证失败,不必继续了
				sg_iECExtProcStatus = 1; // 不再做后续的CVM了, 准备处理EC联机密码附加处理
				iEmvSetItemBit(EMV_ITEM_TVR, TVR_16_CVM_NOT_SUCCESS, 1); // 持卡人验证失败, add by yujun 20140320, 修复Bug
			} else {
				sg_iCvmProcIndex += 2; // 过滤掉该CVM
			}
#if 0
// 20140814
// 在持卡人验证阶段, 不做此类早期检查, 因为可能造成某些关键TVR位没有置位
			if(gl_iAppType != 0/*0:送检程序*/) {
				// 20140422, 针对非送检程序, 尽早判断是否可以拒绝
				//           但对送检程序, 这样做一些案例过不去, 比如2cj.002.05 0a, 因此对于送检程序则不过早判断
				iRet = iEmvIfDenial();
				if(iRet != HXEMV_OK)
					return(iRet); // 如果已经拒绝, 及早通知
			}
#endif
			return(HXEMV_OK); // 继续下一验证方法 或 进行附加处理
		default:
			ASSERT(0);
			return(HXEMV_CORE);
		} // switch(sCvmResult[0] & 0x3F) {
	} else {
		// 处于附加处理阶段
		// 附加处理状态sg_iECExtProcStatus定义 0:未开始
		//                                     1:正在处理EC联机密码附加处理 2:EC联机密码附加处理完毕
	    //                                     3:正在处理金额确认附加处理 4:金额确认附加处理完毕
		//                                     10:附加处理完毕
		switch(sg_iECExtProcStatus) {
		case 1: // 1:正在处理EC联机密码附加处理
			sg_iConfirmAmountFlag = 1;
			if(iCvmProc == HXCVM_PROC_OK) {
				iEmvSetItemBit(EMV_ITEM_TVR, TVR_21_ONLINE_PIN_ENTERED, 1);
				iTlvMakeAddObj(gl_sTlvDbTermVar, TAG_DFXX_EncOnlinePIN, 8, psCvmData, TLV_CONFLICT_REPLACE);
			}
			sg_iECExtProcStatus = 2; // 2:EC联机密码附加处理完毕
			return(HXEMV_OK); // 继续进行附加处理
		case 3: // 3:正在处理金额确认附加处理
			sg_iConfirmAmountFlag = 1;
			if(iCvmProc!=HXCVM_BYPASS && iCvmProc!=HXCVM_PROC_OK)
				return(HXEMV_CANCEL); // 除了bypass与确认外, 都认为是取消
			sg_iECExtProcStatus = 4; // 4:金额确认附加处理完毕
			return(HXEMV_OK); // 继续进行附加处理
		default:
			ASSERT(0);
			return(HXEMV_CORE);
		}
	}
#if 0
// 20140814
// 在持卡人验证阶段, 不做此类早期检查, 因为可能造成某些关键TVR位没有置位
	if(gl_iAppType != 0/*0:送检程序*/) {
		// 20140422, 针对非送检程序, 尽早判断是否可以拒绝
		//           但对送检程序, 这样做一些案例过不去, 比如2cj.002.05 0a, 因此对于送检程序则不过早判断
		iRet = iEmvIfDenial();
		if(iRet != HXEMV_OK)
			return(iRet); // 如果已经拒绝, 及早通知
	}
#endif
	return(HXEMV_OK);
}

// 获取CVM认证方法需不需要签字
// out : piNeedSignFlag    : 如果返回HXEMV_OK，表示需要签字标志，0:不需要签字 1:需要签字
// ret : HXEMV_OK          : OK
int iCvm2GetSignFlag(int *piNeedSignFlag)
{
	*piNeedSignFlag = sg_iCvmNeedSignFlag;
	return(HXEMV_OK);
}
