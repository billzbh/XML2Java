/**************************************
File name     : EmvCore.c
Function      : EMV/pboc2.0借贷记核心接口
Author        : Yu Jun
First edition : Mar 31st, 2012
Modified      : Sep 5th, 2013
				增加iHxSetAmount()函数, 可以实现GAC与GPO金额不同
                Apr 2nd, 2014
					iHxSetAmount()函数限制在读记录后, 处理限制前调用
				Apr 16th, 2014
				    iHxGetCardNativeData()与iHxGetData()函数在遇到未知Tag时按b型处理
				Apr 21st, 2014
				    更改iHxCoreInit()接口, 增加一个参数iAppType
					用于表明是送检程序还是实际应用程序
                Apr 21st, 2014
					增加变量gl_iAllowAllTransFlag用于表明是否支持所有交易
				Apr 22nd, 2014
					iHxGetCardNativeData()和iHxGetData()返回Bin类型数据时在尾部增加了一个'\0'
					这与函数说明不符且不必要, 修正Bin类型数据时不增加尾部'\0'
				May 19th, 2014
				    iHxReadLog()、iHxReadLoadLog()在传入非法记录号时返回HXEMV_NO_LOG改为返回HXEMV_NO_RECORD
**************************************/
/*
模块详细描述:
	核心接口
.	EMV参数设置
.	调用规范实现层实现EMV核心接口
	对于回调方式, 调用回调接口显示信息
	对于非回调接口, 返回显示信息代码给应用层
.	EMV数据提取
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

static int   sg_iAidGetFlag;		 // AID列表获取标志, 1:AID列表已经获取 0:AID列表列表尚未获取
static uchar sg_szSelectedLang[3];   // 持卡人选择的语言, 防止多次要求持卡人选择语言
static int   sg_iCardSlot = 100;	 // 接触卡座号
static int   sg_iCtlsCardSlot = 100; // 非接卡座号
static int   sg_iCurrSlot = 0;		 // 当前活动卡座号, 最后一次检测到卡片存在的卡座号

// 获取核心信息
// out : pszCoreName    : 核心名字, 不超过40字节
//       pszCoreDesc    : 核心描述, 不超过60字节字符串
//       pszCoreVer     : 核心版本号, 不超过10字节字符串, 如"1.00"
//       pszReleaseDate : 核心发布日期, YYYY/MM/DD
// ret : HXEMV_OK       : OK
int iHxCoreInfo(uchar *pszCoreName, uchar *pszCoreDesc, uchar *pszCoreVer, uchar *pszReleaseDate)
{
	strcpy(pszCoreName, "HXY5_EMV_PBOC30DC_ECASH_QPBOC"); // 核心名字
    strcpy(pszCoreDesc, "EMV & PBOC3.0 D/C & eCash & qPboc"); // 核心描述
    strcpy(pszCoreVer, "3.20"); // 版本
    strcpy(pszReleaseDate, "2014/08/14"); // 发布日期
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

// 初始化核心
// in  : iCallBackFlag      : 指明核心调用方式
//                            HXCORE_CALLBACK     : 指明使用回调接口
//                            HXCORE_NOT_CALLBACK : 指明使用非回调接口
//		 iAppType           : 应用程序标志, 0:送检程序(严格遵守规范) 1:实际应用程序
//							      区别: 1. 送检程序只支持通过L2检测的交易, 应用程序支持所有交易
//       ulRandSeed         : 随机数种子，启动核心内部随机数发生器
//       pszDefaultLanguage : 缺省语言, 当前支持zh:中文 en:英文
// ret : HXEMV_OK           : OK
//       HXEMV_PARA         : 参数错误
//       HXEMV_CORE         : 内部错误
// Note: 在调用任何其它接口前必须且先初始化核心
int iHxCoreInit(int iCallBackFlag, int iAppType, ulong ulRandSeed, uchar *pszDefaultLanguage)
{
	int iRet;

	if(iCallBackFlag!=HXCORE_CALLBACK && iCallBackFlag!=HXCORE_NOT_CALLBACK)
		return(HXEMV_PARA);
	gl_iCoreCallFlag = iCallBackFlag;

	iRet = iEmvMsgTableInit(pszDefaultLanguage); // 初始化多语言信息结构, 忽略返回值, 如果语言不支持, 会使用缺省语言

	// 初始化Tlv数据库
	iTlvSetBuffer(gl_sTlvDbTermFixed, sizeof(gl_sTlvDbTermFixed));
	iTlvSetBuffer(gl_sTlvDbTermVar, sizeof(gl_sTlvDbTermVar));
	iTlvSetBuffer(gl_sTlvDbIssuer, sizeof(gl_sTlvDbIssuer));
	iTlvSetBuffer(gl_sTlvDbCard, sizeof(gl_sTlvDbCard));

	gl_iTermSupportedAidNum = 0; // 终端当前支持的Aid列表数目
	gl_iTermSupportedCaPublicKeyNum = 0; // 终端当前支持的CA公钥数目
	gl_iCardAppNum = 0; // 终端与卡片同时支持的应用数目

	gl_iAppType = iAppType;   // 应用类型

    vRandShuffle(ulRandSeed); // 启动随机数发生器
	vRandShuffle((ulong)_ucGetRand() * (ulong)_ucGetRand()); // 随机数扰动

	iTraceSet(TRACE_OFF, 0, 0, 0, NULL);

	gl_iCoreStatus = EMV_STATUS_INIT; // 核心初始化完成
	return(HXEMV_OK);
}

// 设置终端参数
// in  : pTermParam        : 终端参数
// out : pszErrTag         : 如果设置出错, 出错的Tag值
// ret : HXEMV_OK          : OK
//       HXEMV_PARA        : 参数错误
//       HXEMV_CORE        : 内部错误
//       HXEMV_FLOW_ERROR  : EMV流程错误
int iHxSetTermParam(stTermParam *pTermParam, uchar *pszErrTag)
{
	uchar szDateTime[15];
	int iRet;

	_vGetTime(szDateTime);
	vTraceWriteTxtCr(TRACE_ALWAYS, "===...===...=== %4.4s/%2.2s/%2.2s %2.2s:%2.2s:%2.2s ===...===...===",
		             szDateTime, szDateTime+4, szDateTime+6, szDateTime+8, szDateTime+10, szDateTime+12); // 下载参数是核心初始化后调用的第一个函数, 特殊标记一下

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
		return(HXEMV_FLOW_ERROR); // 只能在核心初始化后设置一次

	iRet = iEmvSetTermParam(pTermParam, pszErrTag);
	if(iRet == 0)
	    gl_iCoreStatus = EMV_STATUS_SET_PARA; // 已经设置完EMV参数
	if(iRet)
		vTraceWriteTxtCr(TRACE_PROC, "(%d)iHxSetTermParam(%s)", iRet, pszErrTag);
	return(iRet);
}

// 装载终端支持的Aid
// in  : pTermAid          : 终端支持的Aid
//       iFlag             : 动作, HXEMV_PARA_INIT:初始化 HXEMV_PARA_ADD:增加 HXEMV_PARA_DEL:删除
// out : pszErrTag         : 如果设置出错, 出错的Tag值
// ret : HXEMV_OK          : OK
//       HXEMV_LACK_MEMORY : 存储空间不足
//       HXEMV_PARA        : 参数错误
//       HXEMV_FLOW_ERROR  : EMV流程错误
// Note: 删除时只传入AID即可
int iHxLoadTermAid(stTermAid *pTermAid, int iFlag, uchar *pszErrTag)
{
	int iRet;

	iRet = iEmvLoadTermAid(pTermAid, iFlag, pszErrTag);
	vTraceWriteTxtCr(TRACE_PROC, "(%d)iHxLoadTermAid(%d)", iRet, iFlag);
	if(iFlag != HXEMV_PARA_INIT) {
		vTraceWriteBinCr(TRACE_TAG_DESC, "sAid=", pTermAid->sAid, pTermAid->ucAidLen);
		vTraceWriteTxtCr(TRACE_TAG_DESC, "ucASI=%02X", (int)pTermAid->ucASI);
		// 暂不跟踪其它参数
	}
	return(iRet);
}

// 装载CA公钥
// in  : pCAPublicKey      : CA公钥
//       iFlag             : 动作, HXEMV_PARA_INIT:初始化 HXEMV_PARA_ADD:增加 HXEMV_PARA_DEL:删除
// ret : HXEMV_OK          : OK
//       HXEMV_LACK_MEMORY : 存储空间不足
//       HXEMV_PARA        : 参数错误
//       HXEMV_FLOW_ERROR  : EMV流程错误
// Note: 删除时只传入RID、index即可
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

// 设置IC卡卡座号
// in  : iSlotNo : 卡座号，VPOS规范
// ret : HXEMV_OK       : OK
//       HXEMV_NO_SLOT  : 不支持此卡座
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

// 设置非接IC卡卡座号
// in  : iSlotNo : 卡座号，VPOS规范
// ret : HXEMV_OK       : OK
//       HXEMV_NO_SLOT  : 不支持此卡座
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

// 交易初始化, 每笔新交易开始调用一次
// ret : HXEMV_OK          : OK
//       HXEMV_FLOW_ERROR  : EMV流程错误
int iHxTransInit(void)
{
	uchar sBuf[5];
	uchar szDateTime[15];
	ulong ulRandShuffle;

	vTraceWriteTxtCr(TRACE_PROC, "iHxTransInit()");
	_vGetTime(szDateTime);

	vTraceWriteTxtCr(TRACE_ALWAYS, "---...---...--- EMV交易开始(%4.2s/%2.2s/%2.2s %2.2s:%2.2s:%2.2s) ---...---...---",
		             szDateTime, szDateTime+4, szDateTime+6, szDateTime+8, szDateTime+10, szDateTime+12);

	// 初始化Tlv数据库
	iTlvSetBuffer(gl_sTlvDbTermVar, sizeof(gl_sTlvDbTermVar));
	iTlvSetBuffer(gl_sTlvDbIssuer, sizeof(gl_sTlvDbIssuer));
	iTlvSetBuffer(gl_sTlvDbCard, sizeof(gl_sTlvDbCard));

	// 初始化TSI、TVR
	memset(sBuf, 0, 5);
	iTlvMakeAddObj(gl_sTlvDbTermVar, TAG_9B_TSI, 2, sBuf, TLV_CONFLICT_REPLACE); // TSI
	iTlvMakeAddObj(gl_sTlvDbTermVar, TAG_95_TVR, 5, sBuf, TLV_CONFLICT_REPLACE); // TVR

	sg_iAidGetFlag = 0; // 0表示应用列表尚未获取

	sg_szSelectedLang[0] = 0;  // 持卡人选择的语言初始化

	iEmvGpoInit(); // GPO数据初始化
	iEmvIOInit();

	// 脱机数据认证模块
	iEmvDAuthInit();
	vRandShuffle((ulong)_ucGetRand() * (ulong)_ucGetRand()); // 随机数扰动
	_vSetTimer(&ulRandShuffle, 0);
	vRandShuffle(ulRandShuffle); // 强化随机数扰动, 利用当前tickcount

	// 非回调方式持卡人验证模块
	vCvm2Init();

	gl_iCoreStatus = EMV_STATUS_TRANS_INIT;

	return(HXEMV_OK);
}

// 非接触卡交易预处理
// in  : pszAmount					: 交易金额, ascii码串, 不要小数点, 默认为缺省小数点位置, 例如:RMB1.00表示为"100"
//       uiCurrencyCode				: 货币代码
// ret : HXEMV_OK					: OK
//		 HXEMV_TRY_OTHER_INTERFACE	: 满足拒绝条件，交易终止, 尝试其它通信界面
//									  应显示EMVMSG_TERMINATE信息, 如果终端支持其它通信界面, 提示用户尝试其它通信界面
//       HXEMV_CORE					: 内部错误
// Note: 每笔非接触交易前必须先做预处理, 只有成功完成了预处理才可以进行非接交易
//       应用层要确保交易数据与预处理数据一致
//       参考: JR/T0025.12—2013, 6.2, p9
int iHxCtlsPreProc(uchar *pszAmount, uint uiCurrencyCode)
{
	int iRet;
	iRet = iPbocCtlsPreProc(pszAmount, uiCurrencyCode);
	vTraceWriteTxtCr(TRACE_PROC, "(%d)iHxCtlsPreProc(%s, %u)", iRet, pszAmount, uiCurrencyCode);
	return(iRet);
}

// 检测卡片
// ret : HXEMV_OK          : OK
//       HXEMV_NO_CARD     : 卡片不存在
int iHxTestCard(void)
{
	uint uiRet;
	uiEmvCmdSetSlot(sg_iCardSlot);
	uiRet = uiEmvTestCard();
	if(uiRet == 0)
		return(HXEMV_NO_CARD);
	sg_iCurrSlot = sg_iCardSlot; // 检测到接触卡, 将当前活动卡座设定为接触卡座
	gl_iCardType = EMV_CONTACT_CARD; // 设置卡类型为接触卡
	return(HXEMV_OK);
}

// 检测非接卡片
// ret : HXEMV_OK          : OK
//       HXEMV_NO_CARD     : 卡片不存在
int iHxTestCtlsCard(void)
{
	uint uiRet;
	uiEmvCmdSetSlot(sg_iCtlsCardSlot);
	uiRet = uiEmvTestCard();
	if(uiRet == 0)
		return(HXEMV_NO_CARD);
	sg_iCurrSlot = sg_iCtlsCardSlot; // 检测到非接卡, 将当前活动卡座设定为非接卡座
	gl_iCardType = EMV_CONTACTLESS_CARD; // 设置卡类型为非接卡
	return(HXEMV_OK);
}

// 关闭卡片
// ret : HXEMV_OK          : OK
int iHxCloseCard(void)
{
	vTraceWriteTxtCr(TRACE_PROC, "iHxCloseCard()");
	uiEmvCloseCard();
	return(HXEMV_OK);
}

// 强制联机设定,设定TVR强制联机位
// in  : iFlag             : 设定TVR强制联机位标志 0:不设定 1:设定
// ret : HXEMV_OK          : OK
//       HXEMV_FLOW_ERROR  : EMV流程错误
// Note: 并不能确保联机，还要看TAC/IAC-Online强制联机位的设置
//       规范规定:必须是有人值守,且在交易开始的时候进行强制联机处理(emv2008 book4 6.5.3, P56)
//       本核心不支持操作员发起的强制联机, 参考过工行凭条, 其中强制联机位被强制设定,
//       为了将来可能被要求设定强制联机位, 特提供此函数, 本函数不理会是否是有人值守
int iHxSetForceOnlineFlag(int iFlag)
{
	int   iRet;
	uchar *p;

	vTraceWriteTxtCr(TRACE_PROC, "iHxSetForceOnlineFlag(%d)", iFlag);
	iRet = iTlvGetObjValue(gl_sTlvDbTermVar, TAG_DFXX_ForceOnlineSupport, &p);
	if(iRet != 1)
		return(HXEMV_OK);
	if(*p == 0)
		return(HXEMV_OK); // 终端不支持
	// 设置TVR强制联机标识位
	iEmvSetItemBit(EMV_ITEM_TVR, TVR_28_FORCED_ONLINE, iFlag);
	return(HXEMV_OK);
}

// 读取卡片内部数据
// in  : psTag             : 数据标签, 例:"\x9F\x79":电子现金余额
//       piOutTlvDataLen   : psOutTlvData缓冲区大小
//       piOutDataLen      : psOutData缓冲区大小
// out : piOutTlvDataLen   : 读出的数据长度, 原始格式, 包括Tag、Length、Value
//       psOutTlvData      : 读出的数据, 包括Tag、Length、Value
//       piOutDataLen      : 读出的数据长度, 解码后格式
//       psOutData         : 读出的数据(N数字型:转换成可读数字串, CN压缩数字型:去除尾部'F'后的数字串, A|AN|ANS|B|未知类型:原样返回),除B型外返回值后面会强制加一个结尾'\0', 该结束符不包括在返回的长度之内
//                           注意, 类似N3这样的数据, 返回的内容长度为N4, 注意接收缓冲区长度要给足
// ret : HXEMV_OK          : 读取成功
//       HXEMV_NO_DATA     : 无此数据
//       HXEMV_LACK_MEMORY : 缓冲区不足
//       HXEMV_CARD_REMOVED: 卡被取走
//       HXEMV_CARD_OP     : 卡操作错
//       HXEMV_CARD_SW     : 非法卡状态字
//       HXEMV_FLOW_ERROR  : EMV流程错误
//       HXEMV_CORE        : 内部错误
// note: piOutTlvDataLen或psOutTlvData任一个传入NULL，则不会返回这两个结果
//       piOutDataLen或psOutData任一个传入NULL, 将不会返回这两个结果
//       数据一旦被读出, 会放到Tlv数据库中, 再次使用可以用iHxGetData()读取
//       出错不会显示错误信息
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
		iTagLen = 2; // 如果Tag非法, 认为是2字节Tag
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
		return(HXEMV_FLOW_ERROR); // 应用选择之后可以读卡片内部数据
	}

	iTagLen = iTlvTagLen(psTag);
	if(iTagLen <= 0)
		return(HXEMV_NO_DATA); // 如果Tag非法, 认为无此数据
	uiRet = uiEmvCmdGetData(psTag, &ucTlvObjLen, sTlvObj);
	if(uiRet) {
		EMV_TRACE_ERR_MSG_RET(uiRet)
	}
	if(uiRet == 1) {
		if(uiEmvTestCard() == 0) {
			return(HXEMV_CARD_REMOVED); // 卡被取走
		} else {
			return(HXEMV_CARD_OP); // 卡片操作错误
		}
	}
	if(uiRet==0x6A81 || uiRet==0x6A88)
		return(HXEMV_NO_DATA);
	if(uiRet)
		return(HXEMV_CARD_SW); // 非法卡状态字
	if(ucTlvObjLen < 2)
		return(HXEMV_NO_DATA); // 格式错,认为无数据
	if(memcmp(sTlvObj, psTag, 2) != 0)
		return(HXEMV_NO_DATA); // 读出的数据Tag不符, 认为无数据

	// 成功读出了数据
	iRet = iTlvCheckTlvObject(sTlvObj);
	if(iRet<0 || iRet!=(int)ucTlvObjLen) {
		EMV_TRACE_ERR_MSG_RET(iRet)
		return(HXEMV_NO_DATA); // 如果格式错, 认为无此数据
	}
	if(iRet > (int)ucTlvObjLen) {
		EMV_TRACE_ERR_MSG_RET(iRet)
		return(HXEMV_NO_DATA); // 如果格式错, 认为无此数据
	}

	// 保存到gl_sTlvDbCard中
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
    // 解码
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
    default: // 未知Tag按b型处理
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

// 读取应用数据
// in  : psTag             : 数据标签, 例:"\x82":Aip
//       piOutTlvDataLen   : psOutTlvData缓冲区大小
//       piOutDataLen      : psOutData缓冲区大小
// out : piOutTlvDataLen   : 读出的数据长度, 原始格式, 包括Tag、Length、Value
//       psOutTlvData      : 读出的数据, 包括Tag、Length、Value
//       piOutDataLen      : 读出的数据长度, 解码后格式
//       psOutData         : 读出的数据(N数字型:转换成可读数字串, CN压缩数字型:去除尾部'F'后的数字串, A|AN|ANS|B|未知类型:原样返回),除B型外返回值后面会强制加一个结尾'\0', 该结束符不包括在返回的长度之内
//                           注意, 类似N3这样的数据, 返回的内容长度为N4, 注意接收缓冲区长度要给足
// ret : HXEMV_OK          : 读取成功
//       HXEMV_NO_DATA     : 无此数据
//       HXEMV_LACK_MEMORY : 缓冲区不足
//       HXEMV_FLOW_ERROR  : EMV流程错误
// note: piOutTlvDataLen或psOutTlvData任一个传入NULL，则不会返回这两个结果
//       piOutDataLen或psOutData任一个传入NULL, 将不会返回这两个结果
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
		iTagLen = 2; // 如果Tag非法, 认为是2字节Tag
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
		return(HXEMV_NO_DATA); // 如果Tag非法, 认为无此数据
    for(i=0; i<4; i++) {
        if(i == 0)
            psTlvDb = gl_sTlvDbTermFixed; // TLV数据库, 终端固定参数
        else if(i == 1)
            psTlvDb = gl_sTlvDbTermVar;  // TLV数据库, 终端可变数据
        else if(i == 2)
            psTlvDb = gl_sTlvDbIssuer;  // TLV数据库, 发卡行数据
        else
            psTlvDb = gl_sTlvDbCard; // TLV数据库, 卡片数据
    	iRet = iTlvGetObj(psTlvDb, psTag, &psTlvObj);
        if(iRet > 0)
            break;
    }
    if(i >= 4)
		return(HXEMV_NO_DATA);
	ucTlvObjLen = (uchar)iRet;
	memcpy(sTlvObj, psTlvObj, iRet);

	// 成功读出了数据
	iRet = iTlvCheckTlvObject(sTlvObj);
	if(iRet < 0)
		return(HXEMV_NO_DATA); // 如果格式错, 认为无此数据
	if(iRet > (int)ucTlvObjLen)
		return(HXEMV_NO_DATA); // 如果格式错, 认为无此数据

	if(piOutTlvDataLen && psOutTlvData) {
		if(*piOutTlvDataLen < (int)ucTlvObjLen)
			return(HXEMV_LACK_MEMORY);
		*piOutTlvDataLen = (int)ucTlvObjLen;
		memcpy(psOutTlvData, sTlvObj, *piOutTlvDataLen);
		vTraceWriteBinCr(TRACE_ALWAYS, "Tlv -->", psOutTlvData, *piOutTlvDataLen);
	}
	if(piOutDataLen==NULL || psOutData==NULL)
		return(HXEMV_OK);

    // 解码
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
    default: // 未知Tag按b型处理
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

// 读卡片交易流水支持信息
// out : piMaxRecNum       : 最多交易流水记录个数
// ret : HXEMV_OK          : 读取成功
//       HXEMV_NO_LOG      : 卡片不支持交易流水记录
//       HXEMV_FLOW_ERROR  : EMV流程错误
int iHxGetLogInfo(int *piMaxRecNum)
{
	int   iRet;
	uchar *p;

	vTraceWriteTxtCr(TRACE_PROC, "iHxGetLogInfo()");
	if(gl_iCoreStatus < EMV_STATUS_APP_SELECT)
		return(HXEMV_FLOW_ERROR); // 应用选择之后可以读卡片交易日志

	iRet = iTlvGetObjValue(gl_sTlvDbCard, TAG_9F4D_LogEntry, &p);
	if(iRet != 2)
		return(HXEMV_NO_LOG);
	if(p[0]<10 || p[0]>30)
		return(HXEMV_NO_LOG); // SFI如果不是11-30之间, 认为不支持交易记录
	if(p[1] == 0)
		return(HXEMV_NO_LOG); // 最大交易记录个数如果为0, 认为不支持交易记录
	*piMaxRecNum = (int)p[1];
	vTraceWriteTxtCr(TRACE_PROC, "(0)iHxGetLogInfo(%d)", *piMaxRecNum);
	return(HXEMV_OK);
}

// 读卡片交易流水
// in  : iLogNo            : 交易流水记录号, 最近的一条记录号为1
//       piLogLen          : psLog缓冲区大小
// out : piLogLen          : 交易流水记录长度
//       psLog             : 记录内容(IC卡原格式输出)
// ret : HXEMV_OK          : 读取成功
//       HXEMV_NO_RECORD   : 无此记录
//       HXEMV_LACK_MEMORY : 缓冲区不足
//       HXEMV_CARD_REMOVED: 卡被取走
//       HXEMV_CARD_OP     : 卡操作错
//       HXEMV_CARD_SW     : 非法卡指令状态字
//       HXEMV_NO_LOG      : 卡片不支持交易流水记录
//       HXEMV_FLOW_ERROR  : EMV流程错误
// note: 出错不会显示错误
int iHxReadLog(int iLogNo, int *piLogLen, uchar *psLog)
{
	int   iRet;
	uint  uiRet;
	uchar *p;
	uchar ucLogLen, sLog[256];

	vTraceWriteTxtCr(TRACE_PROC, "iHxReadLog(%d,%d)", iLogNo, *piLogLen);
	if(gl_iCoreStatus < EMV_STATUS_APP_SELECT)
		return(HXEMV_FLOW_ERROR); // 应用选择之后可以读卡片交易日志

	iRet = iTlvGetObjValue(gl_sTlvDbCard, TAG_9F4D_LogEntry, &p);
	if(iRet != 2)
		return(HXEMV_NO_LOG);
	if(p[0]<10 || p[0]>30)
		return(HXEMV_NO_LOG); // SFI如果不是11-30之间, 认为不支持交易记录
	if(iLogNo<1 || iLogNo>(int)p[1])
		return(HXEMV_NO_RECORD);
	uiRet = uiEmvCmdRdRec(p[0], (uchar)iLogNo, &ucLogLen, sLog);
	if(uiRet == 0x6A83)
		return(HXEMV_NO_RECORD);
	if(uiRet == 1) {
		if(uiEmvTestCard() == 0)
			return(HXEMV_CARD_REMOVED); // 卡被取走
		else
			return(HXEMV_CARD_OP); // 卡片操作错误
	}
	if(uiRet)
		return(HXEMV_CARD_SW);
	if((int)ucLogLen > *piLogLen)
		return(HXEMV_LACK_MEMORY);

	memcpy(psLog, sLog, ucLogLen);
	*piLogLen = (int)ucLogLen;
	return(HXEMV_OK);
}

// 读卡片圈存交易流水支持信息
// out : piMaxRecNum       : 最多交易流水记录个数
// ret : HXEMV_OK          : 读取成功
//       HXEMV_NO_LOG      : 卡片不支持圈存流水记录
//       HXEMV_FLOW_ERROR  : EMV流程错误
int iHxGetLoadLogInfo(int *piMaxRecNum)
{
	int   iRet;
	uchar *p;

	vTraceWriteTxtCr(TRACE_PROC, "iHxGetLoadLogInfo()");
	if(gl_iCoreStatus < EMV_STATUS_APP_SELECT)
		return(HXEMV_FLOW_ERROR); // 应用选择之后可以读卡片交易日志

	iRet = iTlvGetObjValue(gl_sTlvDbCard, TAG_DF4D_LoadLogEntry, &p);
	if(iRet != 2)
		return(HXEMV_NO_LOG);
	if(p[0]<10 || p[0]>30)
		return(HXEMV_NO_LOG); // SFI如果不是11-30之间, 认为不支持交易记录
	if(p[1] == 0)
		return(HXEMV_NO_LOG); // 最大交易记录个数如果为0, 认为不支持交易记录
	*piMaxRecNum = (int)p[1];
	vTraceWriteTxtCr(TRACE_PROC, "(0)iHxGetLoadLogInfo(%d)", *piMaxRecNum);
	return(HXEMV_OK);
}

// 读圈存交易流水
// in  : iLogNo            : 交易流水记录号, 最近的一条记录号为1, 要全部读出记录号为0
//       piLogLen          : psLog缓冲区大小
// out : piLogLen          : 交易流水记录长度
//       psLog             : 记录内容(IC卡原格式输出)
// ret : HXEMV_OK          : 读取成功
//       HXEMV_NO_RECORD   : 无此记录
//       HXEMV_LACK_MEMORY : 缓冲区不足
//       HXEMV_CARD_REMOVED: 卡被取走
//       HXEMV_CARD_OP     : 卡操作错
//       HXEMV_CARD_SW     : 非法卡指令状态字
//       HXEMV_NO_LOG      : 卡片不支持交易流水记录
//       HXEMV_FLOW_ERROR  : EMV流程错误
// note: 出错不会显示错误
int iHxReadLoadLog(int iLogNo, int *piLogLen, uchar *psLog)
{
	int   iRet;
	uint  uiRet;
	uchar *p;
	uchar ucLogLen, sLog[256];

	vTraceWriteTxtCr(TRACE_PROC, "iHxReadLoadLog(%d,%d)", iLogNo, *piLogLen);
	if(gl_iCoreStatus < EMV_STATUS_APP_SELECT)
		return(HXEMV_FLOW_ERROR); // 应用选择之后可以读卡片交易日志

	iRet = iTlvGetObjValue(gl_sTlvDbCard, TAG_DF4D_LoadLogEntry, &p);
	if(iRet != 2)
		return(HXEMV_NO_LOG);
	if(p[0]<10 || p[0]>30)
		return(HXEMV_NO_LOG); // SFI如果不是11-30之间, 认为不支持交易记录
	if(iLogNo<1 || iLogNo>(int)p[1])
		return(HXEMV_NO_RECORD);
	uiRet = uiEmvCmdRdRec(p[0], (uchar)iLogNo, &ucLogLen, sLog);
	if(uiRet == 0x6A83)
		return(HXEMV_NO_RECORD);
	if(uiRet == 1) {
		if(uiEmvTestCard() == 0)
			return(HXEMV_CARD_REMOVED); // 卡被取走
		else
			return(HXEMV_CARD_OP); // 卡片操作错误
	}
	if(uiRet)
		return(HXEMV_CARD_SW);
	if((int)ucLogLen > *piLogLen)
		return(HXEMV_LACK_MEMORY);

	memcpy(psLog, sLog, ucLogLen);
	*piLogLen = (int)ucLogLen;
	return(HXEMV_OK);
}

// 非接触卡GPO结果分析
// out : piTransRoute               : 交易走的路径 0:接触Pboc(标准DC或EC) 1:非接触Pboc(标准DC或EC) 2:qPboc联机 3:qPboc脱机 4:qPboc拒绝
//       piSignFlag					: 需要签字标志(0:不需要签字, 1:需要签字), 仅当走的是qPboc流程时才有效
//       piNeedOnlinePin            : 需要联机密码标志(0:不需要联机密码, 1:需要联机密码), 仅当走的是qPboc联机流程时才有效
// ret : HXEMV_OK					: OK, 根据piTransRoute决定后续流程
//       HXEMV_TERMINATE			: 满足拒绝条件，交易终止
//		 HXEMV_TRY_OTHER_INTERFACE	: 尝试其它通信界面
//       HXEMV_CORE					: 内部错误
// Note: GPO成功后才可以调用
// 参考 JR/T0025.12—2013, 7.8 P40
int iHxCtlsGpoAnalyse(int *piTransRoute, int *piSignFlag, int *piNeedOnlinePin)
{
	int iRet;
	iRet = iPbocCtlsGpoAnalyse(piTransRoute, piSignFlag, piNeedOnlinePin);
	vTraceWriteTxtCr(TRACE_PROC, "(%d)iHxCtlsGpoAnalyse(%d,%d,%d)", iRet, *piTransRoute, *piSignFlag, *piNeedOnlinePin);
	return(iRet);
}

// 传入或重新传入金额
// in  : pszAmount         : 金额
//       pszAmountOther    : 其它金额, 注意, 暂不支持, 核心会忽略该值
// ret : HXEMV_OK          : 设置成功
//       HXEMV_FLOW_ERROR  : EMV流程错误
//       HXEMV_PARA        : 参数错误
// note: GPO后GAC金额可能会不同, 重新设定交易金额
//       读记录后, 处理限制前可调用
int iHxSetAmount(uchar *pszAmount, uchar *pszAmountOther)
{
	int iRet;

	vTraceWriteTxtCr(TRACE_PROC, "iHxSetAmount(%s,%s)", pszAmount, pszAmountOther);
	if(gl_iCoreStatus < EMV_STATUS_READ_REC)
		return(HXEMV_FLOW_ERROR); // 读记录之后才允许重新设定金额
	if(gl_iCoreStatus >= EMV_STATUS_PROC_RISTRICTIONS)
		return(HXEMV_FLOW_ERROR); // 处理限制之后不允许重新设定金额
	iRet = iEmvIOSetTransAmount(pszAmount, pszAmountOther);
	if(iRet != HXEMV_OK)
		return(HXEMV_PARA);
	iRet = iEmvSaveAmount(pszAmount);
	if(iRet != HXEMV_OK)
		return(HXEMV_PARA);
	return(HXEMV_OK);
}

//// 以下为回调方式API
//// 回调方式API  *** 开始 ***

// 复位卡片
// ret : HXEMV_OK          : OK
//       HXEMV_FLOW_ERROR  : EMV流程错误
//       HXEMV_CARD_REMOVED: 卡被取走
//       HXEMV_CARD_OP     : 卡片复位错误
//		 HXEMV_CALLBACK_METHOD : 没有以回调方式初始化核心
int iHxResetCard(void)
{
	uint uiRet;

	vTraceWriteTxtCr(TRACE_PROC, "iHxResetCard()");
	if(gl_iCoreCallFlag != HXCORE_CALLBACK)
		return(HXEMV_CALLBACK_METHOD); // 没有以回调方式初始化核心
	if(gl_iCoreStatus != EMV_STATUS_TRANS_INIT)
		return(HXEMV_FLOW_ERROR); // 交易初始化后才可以复位卡片

	iEmvIOShowMsg(EMVMSG_0E_PLEASE_WAIT);

	uiRet = uiEmvCmdResetCard();
	if(uiRet != 0) {
		if(uiEmvTestCard() == 0) {
			iEmvIODispMsg(EMVMSG_0F_PROCESSING_ERROR, EMVIO_NEED_NO_CONFIRM);
			return(HXEMV_CARD_REMOVED); // 卡被取走
		} else {
			iEmvIODispMsg(EMVMSG_06_CARD_ERROR, EMVIO_NEED_NO_CONFIRM);
			if(iEmvTestItemBit(EMV_ITEM_TERM_CAP, TERM_CAP_01_MAG_STRIPE)) {
				// 存在磁卡读卡器
				iEmvIODispMsg(EMVMSG_12_USE_MAG_STRIPE, EMVIO_NEED_NO_CONFIRM);
			}
			return(HXEMV_CARD_OP); // 卡片操作错误，交易终止
		}
	}

	gl_iCoreStatus = EMV_STATUS_RESET;
	return(HXEMV_OK);
}

// 读取支持的应用
// in  : iIgnoreBlock	   : !0:忽略应用锁定标志, 0:锁定的应用不会被选择
// ret : HXEMV_OK          : OK
//       HXEMV_NO_APP      : 无支持的应用
//       HXEMV_CARD_REMOVED: 卡被取走
//       HXEMV_CARD_OP     : 卡操作错
//       HXEMV_CARD_SW     : 卡状态字非法
//       HXEMV_CANCEL      : 用户取消
//       HXEMV_TIMEOUT     : 超时
//       HXEMV_TERMINATE   : 满足拒绝条件，交易终止
//       HXEMV_FLOW_ERROR  : EMV流程错误
//       HXEMV_CORE        : 内部错误
//		 HXEMV_CALLBACK_METHOD : 没有以回调方式初始化核心
// 非接可能返回码
//		 HXEMV_TRY_OTHER_INTERFACE	: 尝试其它通信界面
//		 HXEMV_CARD_TRY_AGAIN		: 要求重新提交卡片(非接)
// Note: 调用该函数后可获取最终匹配的语言(TAG_DFXX_LanguageUsed)
int iHxGetSupportedApp(int iIgnoreBlock)
{
	int   iRet;

	vTraceWriteTxtCr(TRACE_PROC, "iHxGetSupportApp(%d)", iIgnoreBlock);
	if(gl_iCoreCallFlag != HXCORE_CALLBACK)
		return(HXEMV_CALLBACK_METHOD); // 没有以回调方式初始化核心
	if(gl_iCoreStatus != EMV_STATUS_RESET)
		return(HXEMV_FLOW_ERROR); // 卡片复位后才可以获取支持的应用

	iEmvIOShowMsg(EMVMSG_0E_PLEASE_WAIT); // newadd

	// 搜索共同支持的aid列表
	iRet = iSelGetAids(iIgnoreBlock);
	EMV_TRACE_PROC_RET(iRet)
	if(iRet == SEL_ERR_CARD) {
		if(gl_iCardType == EMV_CONTACTLESS_CARD) {
			// 非接卡, 走失效序列, 参考JR/T0025.12—2013图4, P11
			iEmvIODispMsg(EMVMSG_13_TRY_AGAIN, EMVIO_NEED_NO_CONFIRM);
			return(HXEMV_CARD_TRY_AGAIN);
		}

		if(uiEmvTestCard() == 0) {
			iEmvIODispMsg(EMVMSG_0F_PROCESSING_ERROR, EMVIO_NEED_NO_CONFIRM);
			return(HXEMV_CARD_REMOVED); // 卡被取走
		} else {
			iEmvIODispMsg(EMVMSG_06_CARD_ERROR, EMVIO_NEED_NO_CONFIRM);
			return(HXEMV_CARD_OP); // 卡片操作错误，交易终止
		}
	}
	if(iRet == SEL_ERR_CARD_SW) {
		if(gl_iCardType == EMV_CONTACTLESS_CARD) {
			// 非接卡, 参考JR/T0025.12—2013图4, P11
			iEmvIODispMsg(EMVMSG_TERMINATE, EMVIO_NEED_NO_CONFIRM);
			return(HXEMV_TRY_OTHER_INTERFACE);
		}
		// Refer to EMV2008 book3 6.3.5, P47, 不识别的SW需要终止交易
		iEmvIODispMsg(EMVMSG_TERMINATE, EMVIO_NEED_NO_CONFIRM);
		return(HXEMV_CARD_SW); // 卡状态字非法，交易终止
	}

	if(iRet == SEL_ERR_CARD_BLOCKED) {
		iEmvIODispMsg(EMVMSG_TERMINATE, EMVIO_NEED_NO_CONFIRM);
		return(HXEMV_TERMINATE); // 卡片被锁，交易终止
	}
	if(iRet == SEL_ERR_DATA_ERROR) {
		if(gl_iCardType == EMV_CONTACTLESS_CARD) {
			// 非接卡, 参考JR/T0025.12—2013 6.4.1, P12
			iEmvIODispMsg(EMVMSG_TERMINATE, EMVIO_NEED_NO_CONFIRM);
			return(HXEMV_TRY_OTHER_INTERFACE);
		}
		iEmvIODispMsg(EMVMSG_TERMINATE, EMVIO_NEED_NO_CONFIRM);
		return(HXEMV_TERMINATE); // 应用相关数据非法，交易终止
	}
	if(iRet == SEL_ERR_DATA_ABSENT) {
		if(gl_iCardType == EMV_CONTACTLESS_CARD) {
			// 非接卡, 参考JR/T0025.12—2013 6.4.1, P12
			iEmvIODispMsg(EMVMSG_TERMINATE, EMVIO_NEED_NO_CONFIRM);
			return(HXEMV_TRY_OTHER_INTERFACE);
		}
		iEmvIODispMsg(EMVMSG_TERMINATE, EMVIO_NEED_NO_CONFIRM);
		return(HXEMV_TERMINATE); // 必要数据不存在，交易终止
	}
	if(iRet < 0) {
		iEmvIODispMsg(EMVMSG_NATIVE_ERR, EMVIO_NEED_NO_CONFIRM);
		return(HXEMV_CORE); // 内存不足，作为内部错误返回
	}
	if(iRet == 0) {
		iEmvIODispMsg(EMVMSG_TERMINATE, EMVIO_NEED_NO_CONFIRM);
		return(HXEMV_NO_APP); // 无支持的应用
	}

	gl_iCoreStatus = EMV_STATUS_GET_SUPPORTED_APP;

	return(HXEMV_OK);
}

// 应用选择
// in  : iIgnoreBlock	   : !0:忽略应用锁定标志, 0:锁定的应用不会被选择
// ret : HXEMV_OK          : OK
//       HXEMV_CARD_REMOVED: 卡被取走
//       HXEMV_CARD_OP     : 卡操作错
//       HXEMV_NO_APP	   : 没有选中应用
//       HXEMV_TERMINATE   : 满足拒绝条件，交易终止
//       HXEMV_CANCEL      : 用户取消
//       HXEMV_TIMEOUT     : 超时
//       HXEMV_FLOW_ERROR  : EMV流程错误
//       HXEMV_CORE        : 内部错误
//		 HXEMV_CALLBACK_METHOD : 没有以回调方式初始化核心
// 非接可能返回码
//		 HXEMV_TRY_OTHER_INTERFACE	: 尝试其它通信界面
//		 HXEMV_CARD_TRY_AGAIN		: 要求重新提交卡片(非接)
int iHxAppSelect(int iIgnoreBlock)
{
	int iRet;
	uchar *psPdol;
	int   iPdolLen;

	vTraceWriteTxtCr(TRACE_PROC, "iHxAppSelect(%d)", iIgnoreBlock);
	if(gl_iCoreCallFlag != HXCORE_CALLBACK)
		return(HXEMV_CALLBACK_METHOD); // 没有以回调方式初始化核心
	if(gl_iCoreStatus != EMV_STATUS_GET_SUPPORTED_APP)
		return(HXEMV_FLOW_ERROR); // 获取卡片应用列表后才可以选择应用

	iEmvIOShowMsg(EMVMSG_0E_PLEASE_WAIT); // newadd

	iRet = iSelFinalSelect(iIgnoreBlock);
	EMV_TRACE_PROC_RET(iRet)
	if(iRet == SEL_ERR_CARD) {
		if(gl_iCardType == EMV_CONTACTLESS_CARD) {
			// 非接卡, 走失效序列, 参考JR/T0025.12—2013图4, P11
			iEmvIODispMsg(EMVMSG_13_TRY_AGAIN, EMVIO_NEED_NO_CONFIRM);
			return(HXEMV_CARD_TRY_AGAIN);
		}
		if(uiEmvTestCard() == 0) {
			iEmvIODispMsg(EMVMSG_0F_PROCESSING_ERROR, EMVIO_NEED_NO_CONFIRM);
			return(HXEMV_CARD_REMOVED); // 卡被取走
		} else {
			iEmvIODispMsg(EMVMSG_06_CARD_ERROR, EMVIO_NEED_NO_CONFIRM);
			return(HXEMV_CARD_OP); // 卡片操作错误，交易终止
		}
	}
	if(iRet == SEL_ERR_NO_APP) {
		if(gl_iCardType == EMV_CONTACTLESS_CARD) {
			// 非接卡, 参考JR/T0025.12—2013图4, P11
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
		// 非接卡
		iPdolLen = iTlvGetObjValue(gl_sTlvDbCard, TAG_9F38_PDOL, &psPdol);
		if(iPdolLen <= 0) {
			// 非接卡无PDOL, 参考JR/T0025.12—2013图4, P11
			iEmvIODispMsg(EMVMSG_TERMINATE, EMVIO_NEED_NO_CONFIRM);
			return(HXEMV_TRY_OTHER_INTERFACE);
		}
		iRet = iTlvSearchDOLTag(psPdol, iPdolLen, TAG_9F66_ContactlessSupport, NULL);
		if(iRet <= 0) {
			// 非接卡PDOL无9F66, 参考JR/T0025.12—2013图4, P11
			iEmvIODispMsg(EMVMSG_TERMINATE, EMVIO_NEED_NO_CONFIRM);
			return(HXEMV_TRY_OTHER_INTERFACE);
		}
	}

	gl_iCoreStatus = EMV_STATUS_APP_SELECT;
	return(HXEMV_OK);
}

// 设置持卡人语言
// in  : pszLanguage  : 持卡人语言代码, 例如:"zh"
//                      如果传入NULL, 表示使用EMV语言选择方法
// ret : HXEMV_OK     : OK
//       HXEMV_FLOW_ERROR  : EMV流程错误
//		 HXEMV_CALLBACK_METHOD : 没有以回调方式初始化核心
//       HXEMV_PARA   : 参数错误
//       HXEMV_CORE   : 内部错误
// Note: 如果不支持传入的语言, 将会使用本地语言
int iHxSetCardHolderLang(uchar *pszLanguage)
{
	int   iRet;
	uchar szLanguagePreferred[9];
	uchar szFinalLanguage[3];
	uchar *psValue, szPinpadLanguage[33];

	vTraceWriteTxtCr(TRACE_PROC, "iHxSetCardHolderLang(%s)", pszLanguage);
	if(gl_iCoreCallFlag != HXCORE_CALLBACK)
		return(HXEMV_CALLBACK_METHOD); // 没有以回调方式初始化核心
	if(gl_iCoreStatus < EMV_STATUS_APP_SELECT)
		return(HXEMV_FLOW_ERROR); // 选择应用后才可以

	if(pszLanguage) {
		if(strlen(pszLanguage) == 2) {
			// 将匹配的语言代码存入TLV数据库(TAG_DFXX_LanguageUsed)
			iRet = iTlvMakeAddObj(gl_sTlvDbTermVar, TAG_DFXX_LanguageUsed, 2, pszLanguage, TLV_CONFLICT_REPLACE);
			if(iRet < 0) {
				iEmvIODispMsg(EMVMSG_NATIVE_ERR, EMVIO_NEED_NO_CONFIRM);
				return(HXEMV_CORE);
			}
			return(HXEMV_OK);
		}
	}

	// emv语言选择
	iRet = iTlvGetObjValue(gl_sTlvDbCard, TAG_5F2D_LanguagePrefer, &psValue);
	if(iRet>0 && iRet<=8)
		vMemcpy0(szLanguagePreferred, psValue, iRet);
	else
		szLanguagePreferred[0] = 0;

	// 获取最终匹配的语言
	iRet = iEmvMsgTableGetFitLanguage(szLanguagePreferred, szFinalLanguage);
	// 使用匹配的语言, 也可以强制使用其它语言
    iEmvMsgTableSetLanguage(szFinalLanguage);
	// 将匹配的语言代码存入TLV数据库(TAG_DFXX_LanguageUsed)
	iRet = iTlvMakeAddObj(gl_sTlvDbTermVar, TAG_DFXX_LanguageUsed, 2, szFinalLanguage, TLV_CONFLICT_REPLACE);
	if(iRet < 0) {
		iEmvIODispMsg(EMVMSG_NATIVE_ERR, EMVIO_NEED_NO_CONFIRM);
		return(HXEMV_CORE);
	}

	// 获取Pinpad使用的语言
	iRet = iTlvGetObjValue(gl_sTlvDbTermFixed, TAG_DFXX_PinpadLanguage, &psValue);
	if(iRet < 0) {
		iEmvIODispMsg(EMVMSG_NATIVE_ERR, EMVIO_NEED_NO_CONFIRM);
		return(HXEMV_CORE);
	}
	memcpy(szPinpadLanguage, psValue, iRet);
	szPinpadLanguage[iRet] = 0;
	iRet = iEmvMsgTableGetPinpadLanguage(szLanguagePreferred, szPinpadLanguage, szFinalLanguage);
	if(iRet==1/*1:无匹配*/ && strlen(szPinpadLanguage)>2) {
		// 无匹配的pinpad语言 并且 pinpad支持多于1种语言, 要求持卡人选择
		if(iEmvTestItemBit(EMV_ITEM_TERM_CAP_ADD, TERM_CAP_ADD_16_NUMERIC_KEYS) ||
				iEmvTestItemBit(EMV_ITEM_TERM_CAP_ADD, TERM_CAP_ADD_17_ALPHA_KEYS) ||
				iEmvTestItemBit(EMV_ITEM_TERM_CAP_ADD, TERM_CAP_ADD_19_FUNC_KEYS)) {
			// 终端支持按键, 要求选择语言
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

	// 将匹配的语言代码存入TLV数据库(TAG_DFXX_LanguageUsed)
	iRet = iTlvMakeAddObj(gl_sTlvDbTermVar, TAG_DFXX_LanguageUsed, 2, szFinalLanguage, TLV_CONFLICT_REPLACE);
	if(iRet < 0) {
		iEmvIODispMsg(EMVMSG_NATIVE_ERR, EMVIO_NEED_NO_CONFIRM);
		return(HXEMV_CORE);
	}

	return(HXEMV_OK);
}

// GPO
// in  : pszTransTime      : 交易时间，YYYYMMDDhhmmss
//       ulTTC             : 终端交易序号, 1-999999
//       uiTransType       : 交易类型, BCD格式(0xAABB, BB为按GB/T 15150 定义的处理码前2 位表示的金融交易类型, AA用于区分商品/服务)
//       uiCurrencyCode    : 货币代码
// ret : HXEMV_OK          : OK
//       HXEMV_CARD_REMOVED: 卡被取走
//       HXEMV_CARD_OP     : 卡操作错
//       HXEMV_CARD_SW     : 非法卡状态字
//       HXEMV_TERMINATE   : 满足拒绝条件，交易终止
//       HXEMV_NO_APP      : 无支持的应用
//       HXEMV_RESELECT    : 需要重新选择应用
//       HXEMV_PARA        : 参数错误
//       HXEMV_FLOW_ERROR  : EMV流程错误
//       HXEMV_TRANS_NOT_ALLOWED : 交易不支持
//       HXEMV_CORE        : 内部错误
//		 HXEMV_CALLBACK_METHOD : 没有以回调方式初始化核心
//       HXEMV_DENIAL	   : qPboc拒绝
// 非接可能返回码
//		 HXEMV_TRY_OTHER_INTERFACE	: 尝试其它通信界面
//		 HXEMV_CARD_TRY_AGAIN		: 要求重新提交卡片(非接)
// Note: 如果返回HXEMV_RESELECT，需要重新执行iHxAppSelect()
int iHxGPO(uchar *pszTransTime, ulong ulTTC, uint uiTransType, uint uiCurrencyCode)
{
	uchar ucTransType; // 交易类型, T9C值
	int   iRet;
	int   iDecimalPos;
	uchar sBuf[100];

	vTraceWriteTxtCr(TRACE_PROC, "iHxGPO(%s,%lu,%04X,%u)", pszTransTime, ulTTC, uiTransType, uiCurrencyCode);
	if(gl_iCoreCallFlag != HXCORE_CALLBACK)
		return(HXEMV_CALLBACK_METHOD); // 没有以回调方式初始化核心
	if(gl_iCoreStatus != EMV_STATUS_APP_SELECT)
		return(HXEMV_FLOW_ERROR); // 选择应用后才可以GPO

	iEmvIOShowMsg(EMVMSG_0E_PLEASE_WAIT); // newadd

	ucTransType = uiTransType & 0xFF; // 取得T9C值
	// 参数合法性检查
	iRet = iTestIfValidDateTime(pszTransTime);
	if(iRet || ulTTC<1 || ulTTC>999999L) {
		iEmvIODispMsg(EMVMSG_NATIVE_ERR, EMVIO_NEED_NO_CONFIRM);
		return(HXEMV_PARA);
	}
	switch(uiTransType) {
	case TRANS_TYPE_SALE:			// 消费(商品和服务)
		if(!iEmvTestItemBit(EMV_ITEM_TERM_CAP_ADD, TERM_CAP_ADD_01_GOODS)
				&& !iEmvTestItemBit(EMV_ITEM_TERM_CAP_ADD, TERM_CAP_ADD_02_SERVICES)) {
			iEmvIODispMsg(EMVMSG_TRANS_NOT_ALLOWED, EMVIO_NEED_NO_CONFIRM);
			return(HXEMV_TRANS_NOT_ALLOWED); // 如果不支持商品、服务, 则不支持消费交易
		}
		break;
	case TRANS_TYPE_GOODS:			// 商品
		if(!iEmvTestItemBit(EMV_ITEM_TERM_CAP_ADD, TERM_CAP_ADD_01_GOODS)) {
			iEmvIODispMsg(EMVMSG_TRANS_NOT_ALLOWED, EMVIO_NEED_NO_CONFIRM);
			return(HXEMV_TRANS_NOT_ALLOWED); // 如果不支持商品应用, 则不支持商品交易
		}
		break;
	case TRANS_TYPE_SERVICES:		// 服务
		if(!iEmvTestItemBit(EMV_ITEM_TERM_CAP_ADD, TERM_CAP_ADD_02_SERVICES)) {
			iEmvIODispMsg(EMVMSG_TRANS_NOT_ALLOWED, EMVIO_NEED_NO_CONFIRM);
			return(HXEMV_TRANS_NOT_ALLOWED); // 如果不支持服务应用, 则不支持服务交易
		}
		break;
	case TRANS_TYPE_PAYMENT:		// 支付
		if(!iEmvTestItemBit(EMV_ITEM_TERM_CAP_ADD, TERM_CAP_ADD_06_PAYMENT)) {
			iEmvIODispMsg(EMVMSG_TRANS_NOT_ALLOWED, EMVIO_NEED_NO_CONFIRM);
			return(HXEMV_TRANS_NOT_ALLOWED); // 如果不支持支付应用, 则不支持支付交易
		}
		break;
	case TRANS_TYPE_CASH:			// 取现
		if(!iEmvTestItemBit(EMV_ITEM_TERM_CAP_ADD, TERM_CAP_ADD_00_CASH)) {
			iEmvIODispMsg(EMVMSG_TRANS_NOT_ALLOWED, EMVIO_NEED_NO_CONFIRM);
			return(HXEMV_TRANS_NOT_ALLOWED); // 如果不支持现金应用, 则不支持取现交易
		}
		break;
	case TRANS_TYPE_RETURN:			// 退货
		break; // 由于终端性能扩展中没有对退货的定义, 认为支持
	case TRANS_TYPE_DEPOSIT:		// 存款
		if(!iEmvTestItemBit(EMV_ITEM_TERM_CAP_ADD, TERM_CAP_ADD_08_CASH_DEPOSIT)) {
			iEmvIODispMsg(EMVMSG_TRANS_NOT_ALLOWED, EMVIO_NEED_NO_CONFIRM);
			return(HXEMV_TRANS_NOT_ALLOWED); // 如果不支持存款应用, 则不支持存款交易
		}
		break;
	case TRANS_TYPE_AVAILABLE_INQ:	// 可用余额查询
	case TRANS_TYPE_BALANCE_INQ:	// 余额查询
		if(!iEmvTestItemBit(EMV_ITEM_TERM_CAP_ADD, TERM_CAP_ADD_04_INQUIRY)) {
			iEmvIODispMsg(EMVMSG_TRANS_NOT_ALLOWED, EMVIO_NEED_NO_CONFIRM);
			return(HXEMV_TRANS_NOT_ALLOWED); // 如果不支持存款应用, 则不支持存款交易
		}
		break;
	case TRANS_TYPE_TRANSFER:		// 转账
		if(!iEmvTestItemBit(EMV_ITEM_TERM_CAP_ADD, TERM_CAP_ADD_05_TRANSFER)) {
			iEmvIODispMsg(EMVMSG_TRANS_NOT_ALLOWED, EMVIO_NEED_NO_CONFIRM);
			return(HXEMV_TRANS_NOT_ALLOWED); // 如果不支持转账应用, 则不支持转账交易
		}
		break;
	default:
		if(gl_iAppType != 0/*0:送检程序*/)
			break; // 非送检程序, 允许所有交易
		iEmvIODispMsg(EMVMSG_TRANS_NOT_ALLOWED, EMVIO_NEED_NO_CONFIRM);
		return(HXEMV_TRANS_NOT_ALLOWED); // 不识别的交易类型
	}

	if(uiCurrencyCode>999) {
		iEmvIODispMsg(EMVMSG_NATIVE_ERR, EMVIO_NEED_NO_CONFIRM);
		return(HXEMV_PARA);
	}

	// 将交易日期、交易时间、终端交易流水号、交易类型、货币代码、货币指数、PosEntryMode存入tlv数据库
	vTwoOne(pszTransTime+2, 6, sBuf); // 交易日期
	iRet = iTlvMakeAddObj(gl_sTlvDbTermVar, TAG_9A_TransDate, 3, sBuf, TLV_CONFLICT_REPLACE);
	if(iRet < 0) {
		EMV_TRACE_ERR_MSG_RET(iRet)
		iEmvIODispMsg(EMVMSG_NATIVE_ERR, EMVIO_NEED_NO_CONFIRM);
		return(HXEMV_CORE);
	}
	vTwoOne(pszTransTime+8, 6, sBuf); // 交易时间
	iRet = iTlvMakeAddObj(gl_sTlvDbTermVar, TAG_9F21_TransTime, 3, sBuf, TLV_CONFLICT_REPLACE);
	if(iRet < 0) {
		EMV_TRACE_ERR_MSG_RET(iRet)
		iEmvIODispMsg(EMVMSG_NATIVE_ERR, EMVIO_NEED_NO_CONFIRM);
		return(HXEMV_CORE);
	}
	sprintf(sBuf, "%06lu", ulTTC);
	vTwoOne(sBuf, 6, sBuf+10); // T9F41允许N4-N8字节, 这里遵循8583协议,采用N6
	iRet = iTlvMakeAddObj(gl_sTlvDbTermVar, TAG_9F41_TransSeqCounter, 3, sBuf+10, TLV_CONFLICT_REPLACE);
	if(iRet < 0) {
		EMV_TRACE_ERR_MSG_RET(iRet)
		iEmvIODispMsg(EMVMSG_NATIVE_ERR, EMVIO_NEED_NO_CONFIRM);
		return(HXEMV_CORE);
	}
	
	vOneTwo0(&ucTransType, 1, sBuf); // 交易类型为n2型
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

	sprintf(sBuf, "%04lu", uiCurrencyCode); // 交易货币代码
	vTwoOne(sBuf, 4, sBuf+10);
	iRet = iTlvMakeAddObj(gl_sTlvDbTermVar, TAG_5F2A_TransCurrencyCode, 2, sBuf+10, TLV_CONFLICT_REPLACE);
	if(iRet < 0) {
		EMV_TRACE_ERR_MSG_RET(iRet)
		iEmvIODispMsg(EMVMSG_NATIVE_ERR, EMVIO_NEED_NO_CONFIRM);
		return(HXEMV_CORE);
	}

	// 交易货币指数
   	iIso4217SearchDigitCode(uiCurrencyCode, sBuf, &iDecimalPos);
	vL2Bcd((ulong)iDecimalPos, 1, sBuf);
	iRet = iTlvMakeAddObj(gl_sTlvDbTermVar, TAG_5F36_TransCurrencyExp, 1, sBuf, TLV_CONFLICT_REPLACE);
	if(iRet < 0) {
		EMV_TRACE_ERR_MSG_RET(iRet)
		iEmvIODispMsg(EMVMSG_NATIVE_ERR, EMVIO_NEED_NO_CONFIRM);
		return(HXEMV_CORE);
	}

    // Pos Entry Mode (非接卡由于不知道走的是标准pboc还是qPboc, 暂且传入此值, 待确定后修正)
	iRet = iTlvMakeAddObj(gl_sTlvDbTermVar, TAG_9F39_POSEntryMode, 1, "\x05", TLV_CONFLICT_REPLACE); // pos entry mode, "05":ic card
	if(iRet < 0) {
		EMV_TRACE_ERR_MSG_RET(iRet)
		iEmvIODispMsg(EMVMSG_NATIVE_ERR, EMVIO_NEED_NO_CONFIRM);
		return(HXEMV_CORE);
	}

	if(gl_iCardType == EMV_CONTACTLESS_CARD) {
		// 如果是非接卡, 将T9F66存入tlv数据库
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
		gl_iCoreStatus = EMV_STATUS_GET_SUPPORTED_APP; // 需要重新选择应用
		if(gl_iCardAppNum <= 0) {
			iEmvIODispMsg(EMVMSG_TERMINATE, EMVIO_NEED_NO_CONFIRM);
			return(HXEMV_NO_APP); // 如果没有APP可选, 返回无APP
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
			// 非接卡, 走失效序列, 参考JR/T0025.12—2013 6.5.2, P14
			iEmvIODispMsg(EMVMSG_13_TRY_AGAIN, EMVIO_NEED_NO_CONFIRM);
			return(HXEMV_CARD_TRY_AGAIN);
		}
		if(uiEmvTestCard() == 0) {
			iEmvIODispMsg(EMVMSG_0F_PROCESSING_ERROR, EMVIO_NEED_NO_CONFIRM);
			return(HXEMV_CARD_REMOVED); // 卡被取走
		} else {
			iEmvIODispMsg(EMVMSG_06_CARD_ERROR, EMVIO_NEED_NO_CONFIRM);
			return(HXEMV_CARD_OP); // 卡片操作错误，交易终止
		}
	}
	if(iRet == HXEMV_CARD_SW) {
		if(gl_iCardType == EMV_CONTACTLESS_CARD) {
			// 非接卡, 参考JR/T0025.12—2013 6.5.3, P14
			iEmvIODispMsg(EMVMSG_TERMINATE, EMVIO_NEED_NO_CONFIRM);
			return(HXEMV_TRY_OTHER_INTERFACE);
		}
		// Refer to EMV2008 book3 6.3.5, P47, 不识别的SW需要终止交易
		iEmvIODispMsg(EMVMSG_TERMINATE, EMVIO_NEED_NO_CONFIRM);
		return(HXEMV_CARD_SW);
	}
	if(iRet == HXEMV_TERMINATE) {
		iEmvIODispMsg(EMVMSG_TERMINATE, EMVIO_NEED_NO_CONFIRM);
		if(gl_iCardType == EMV_CONTACTLESS_CARD)
			return(HXEMV_TRY_OTHER_INTERFACE); // 非接卡, 认为应该这样做
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

// 读应用记录
// ret : HXEMV_OK          : OK
//       HXEMV_CARD_REMOVED: 卡被取走
//       HXEMV_CARD_OP     : 卡操作错
//       HXEMV_CARD_SW     : 非法卡指令状态字
//       HXEMV_TERMINATE   : 满足拒绝条件，交易终止
//       HXEMV_FLOW_ERROR  : EMV流程错误
//       HXEMV_CORE        : 内部错误
//		 HXEMV_CALLBACK_METHOD : 没有以回调方式初始化核心
// 非接可能返回码
//		 HXEMV_TRY_OTHER_INTERFACE	: 尝试其它通信界面
int iHxReadRecord(void)
{
	int iRet;

	vTraceWriteTxtCr(TRACE_PROC, "iHxReadRecord()");
	if(gl_iCoreCallFlag != HXCORE_CALLBACK)
		return(HXEMV_CALLBACK_METHOD); // 没有以回调方式初始化核心
	if(gl_iCoreStatus != EMV_STATUS_GPO)
		return(HXEMV_FLOW_ERROR); // GPO后才可以读记录
	iRet = iPbocCtlsGetRoute(); // 交易走的路径, 0:接触Pboc(标准DC或EC) 1:非接触Pboc(标准DC或EC) 2:qPboc联机 3:qPboc脱机 4:qPboc拒绝
	if(iRet==2 || iRet==4) {
		EMV_TRACE_ERR_MSG
		return(HXEMV_FLOW_ERROR); // qPboc联机或qPboc拒绝不可以读记录
	}

	iEmvIOShowMsg(EMVMSG_0E_PLEASE_WAIT); // 之前某些选择等可能会清楚等待信息, 在此再显示一次

	iRet = iEmvReadRecord();
	vTraceWriteTxtCr(TRACE_PROC, "(%d)iEmvReadRecord()", iRet);
	if(iRet == HXEMV_CARD_OP) {
		if(gl_iCardType == EMV_CONTACTLESS_CARD) {
			// 非接卡, 走失效序列, 参考JR/T0025.12—2013 6.5.2, P14
			iEmvIODispMsg(EMVMSG_13_TRY_AGAIN, EMVIO_NEED_NO_CONFIRM);
			return(HXEMV_CARD_TRY_AGAIN);
		}
		if(uiEmvTestCard() == 0) {
			iEmvIODispMsg(EMVMSG_0F_PROCESSING_ERROR, EMVIO_NEED_NO_CONFIRM);
			return(HXEMV_CARD_REMOVED); // 卡被取走
		} else {
			iEmvIODispMsg(EMVMSG_06_CARD_ERROR, EMVIO_NEED_NO_CONFIRM);
			return(HXEMV_CARD_OP); // 卡片操作错误，交易终止
		}
	}
	if(iRet == HXEMV_CARD_SW) {
		// Refer to EMV2008 book3 6.3.5, P47, 不识别的SW需要终止交易
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

// SDA或DDA
// ret : HXEMV_OK            : OK
//       HXEMV_CARD_REMOVED  : 卡被取走
//       HXEMV_CARD_OP       : 卡操作错
//       HXEMV_CARD_SW       : 非法卡状态字
//       HXEMV_TERMINATE     : 满足拒绝条件，交易终止
//       HXEMV_DENIAL        : 终端行为分析结果为脱机拒绝
//       HXEMV_DENIAL_ADVICE : 终端行为分析结果为脱机拒绝, 有Advice
//       HXEMV_FLOW_ERROR    : EMV流程错误
//       HXEMV_CORE          : 内部错误
//		 HXEMV_CALLBACK_METHOD : 没有以回调方式初始化核心
// 非接可能返回码
//		 HXEMV_FDDA_FAIL	 : fDDA失败(非接)
// Note: qPboc, FDDA失败后, 核心不显示任何信息, 应用层需要自行检查9F6C B1 b6b5, 以决定是拒绝还是进行联机
int iHxOfflineDataAuth(void)
{
	int iRet;

	vTraceWriteTxtCr(TRACE_PROC, "iHxOfflineDataAuth()");
	if(gl_iCoreCallFlag != HXCORE_CALLBACK)
		return(HXEMV_CALLBACK_METHOD); // 没有以回调方式初始化核心
	if(gl_iCoreStatus<EMV_STATUS_READ_REC || gl_iCoreStatus>=EMV_STATUS_GAC1)
		return(HXEMV_FLOW_ERROR); // 读记录前、GAC后不允许做脱机数据认证

	iEmvIOShowMsg(EMVMSG_0E_PLEASE_WAIT); // newadd

	iRet = iEmvOfflineDataAuth(0); // 回调函数固定传入0, 发卡行公钥证书是否在CRL列表由回调函数确定
	vTraceWriteTxtCr(TRACE_PROC, "(%d)iEmvOfflineDataAuth()", iRet);
    if(iRet != HXEMV_OK) {
		vTraceTlvDb();
    }
	if(iRet == HXEMV_FDDA_FAIL)
		return(HXEMV_FDDA_FAIL); // FDDA失败
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
			return(HXEMV_CARD_REMOVED); // 卡被取走
		} else {
			iEmvIODispMsg(EMVMSG_06_CARD_ERROR, EMVIO_NEED_NO_CONFIRM);
			return(HXEMV_CARD_OP); // 卡片操作错误，交易终止
		}
	}
	if(iRet == HXEMV_CARD_SW) {
		// Refer to EMV2008 book3 6.3.5, P47, 不识别的SW需要终止交易
		iEmvIODispMsg(EMVMSG_TERMINATE, EMVIO_NEED_NO_CONFIRM);
		return(HXEMV_CARD_SW);
	}
	if(iRet != HXEMV_OK) {
		iEmvIODispMsg(EMVMSG_NATIVE_ERR, EMVIO_NEED_NO_CONFIRM);
		return(HXEMV_CORE);
	}

	return(HXEMV_OK);
}

// 处理限制
// ret : HXEMV_OK            : OK
//       HXEMV_TERMINATE     : 满足拒绝条件，交易终止
//       HXEMV_FLOW_ERROR    : EMV流程错误
//       HXEMV_CORE          : 内部错误
//       HXEMV_DENIAL        : 终端行为分析结果为脱机拒绝
//       HXEMV_DENIAL_ADVICE : 终端行为分析结果为脱机拒绝, 有Advice
//       HXEMV_CARD_REMOVED  : 卡被取走
//       HXEMV_CARD_OP       : 终端行为分析结果为脱机拒绝, 申请AAC时卡片出错
//		 HXEMV_CALLBACK_METHOD : 没有以回调方式初始化核心
int iHxProcRistrictions(void)
{
	int iRet;

	vTraceWriteTxtCr(TRACE_PROC, "iHxProcRistrictions()");
	if(gl_iCoreCallFlag != HXCORE_CALLBACK)
		return(HXEMV_CALLBACK_METHOD); // 没有以回调方式初始化核心
	if(gl_iCoreStatus<EMV_STATUS_READ_REC || gl_iCoreStatus>=EMV_STATUS_GAC1)
		return(HXEMV_FLOW_ERROR); // 读记录前、GAC后不允许做处理限制

	iEmvIOShowMsg(EMVMSG_0E_PLEASE_WAIT); // newadd

	iRet = iEmvProcRistrictions();
	vTraceWriteTxtCr(TRACE_PROC, "(%d)iEmvProcRistrictions()", iRet);
	if(iRet != HXEMV_OK)
		vTraceTlvDb();
	if(iRet == HXEMV_CARD_OP) {
		if(uiEmvTestCard() == 0) {
			iEmvIODispMsg(EMVMSG_0F_PROCESSING_ERROR, EMVIO_NEED_NO_CONFIRM);
			return(HXEMV_CARD_REMOVED); // 卡被取走
		} else {
			iEmvIODispMsg(EMVMSG_06_CARD_ERROR, EMVIO_NEED_NO_CONFIRM);
			return(HXEMV_CARD_OP); // 卡片操作错误，交易终止
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

// 终端风险管理
// ret : HXEMV_OK            : OK
//       HXEMV_TERMINATE     : 满足拒绝条件，交易终止
//       HXEMV_CORE          : 其它错误
//       HXEMV_CARD_REMOVED  : 卡被取走
//       HXEMV_CARD_OP       : 卡操作错
//       HXEMV_DENIAL        : 终端行为分析结果为脱机拒绝
//       HXEMV_DENIAL_ADVICE : 终端行为分析结果为脱机拒绝, 有Advice
//       HXEMV_CANCEL        : 被取消
//       HXEMV_TIMEOUT       : 超时
//       HXEMV_FLOW_ERROR    : EMV流程错误
//		 HXEMV_CALLBACK_METHOD : 没有以回调方式初始化核心
int iHxTermRiskManage(void)
{
	int iRet;

	vTraceWriteTxtCr(TRACE_PROC, "iHxTermRiskManage()");
	if(gl_iCoreCallFlag != HXCORE_CALLBACK)
		return(HXEMV_CALLBACK_METHOD); // 没有以回调方式初始化核心
	if(gl_iCoreStatus<EMV_STATUS_READ_REC || gl_iCoreStatus>=EMV_STATUS_GAC1)
		return(HXEMV_FLOW_ERROR); // 读记录前、GAC后不允许做终端风险管理

	iEmvIOShowMsg(EMVMSG_0E_PLEASE_WAIT); // newadd

	iRet = iEmvTermRiskManage();
	vTraceWriteTxtCr(TRACE_PROC, "(%d)iEmvTermRiskManage()", iRet);
	if(iRet != HXEMV_OK)
		vTraceTlvDb();
	if(iRet == HXEMV_CARD_OP) {
		if(uiEmvTestCard() == 0) {
			iEmvIODispMsg(EMVMSG_0F_PROCESSING_ERROR, EMVIO_NEED_NO_CONFIRM);
			return(HXEMV_CARD_REMOVED); // 卡被取走
		} else {
			iEmvIODispMsg(EMVMSG_06_CARD_ERROR, EMVIO_NEED_NO_CONFIRM);
			return(HXEMV_CARD_OP); // 卡片操作错误，交易终止
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

// 持卡人验证
// out : piNeedSignFlag      : 如果成功完成，返回需要签字标志，0:不需要签字 1:需要签字
// ret : HXEMV_OK            : OK
//       HXEMV_CANCEL        : 用户取消
//       HXEMV_TIMEOUT       : 超时
//       HXEMV_CARD_REMOVED  : 卡被取走
//       HXEMV_CARD_OP       : 卡操作错
//       HXEMV_CARD_SW       : 非法卡状态字
//       HXEMV_TERMINATE     : 满足拒绝条件，交易终止
//       HXEMV_DENIAL        : 终端行为分析结果为脱机拒绝
//       HXEMV_DENIAL_ADVICE : 终端行为分析结果为脱机拒绝, 有Advice
//       HXEMV_FLOW_ERROR    : EMV流程错误
//       HXEMV_CORE          : 内部错误
//		 HXEMV_CALLBACK_METHOD : 没有以回调方式初始化核心
int iHxCardHolderVerify(int *piNeedSignFlag)
{
	int   iRet;

	vTraceWriteTxtCr(TRACE_PROC, "iHxCardHolderVerify()");
	if(gl_iCoreCallFlag != HXCORE_CALLBACK)
		return(HXEMV_CALLBACK_METHOD); // 没有以回调方式初始化核心
	if(gl_iCoreStatus<EMV_STATUS_READ_REC || gl_iCoreStatus>=EMV_STATUS_GAC1)
		return(HXEMV_FLOW_ERROR); // 读记录前、GAC后不允许做持卡人认证

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
			return(HXEMV_CARD_REMOVED); // 卡被取走
		} else {
			iEmvIODispMsg(EMVMSG_06_CARD_ERROR, EMVIO_NEED_NO_CONFIRM);
			return(HXEMV_CARD_OP); // 卡片操作错误，交易终止
		}
	}
	if(iRet == HXEMV_CARD_SW) {
		// Refer to EMV2008 book3 6.3.5, P47, 不识别的SW需要终止交易
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

// 终端行为分析
// ret : HXEMV_OK		     : OK
//       HXEMV_CORE          : 其它错误
//       HXEMV_DENIAL        : 终端行为分析结果为脱机拒绝
//       HXEMV_DENIAL_ADVICE : 终端行为分析结果为脱机拒绝, 有Advice
//       HXEMV_CARD_REMOVED  : 卡被取走
//       HXEMV_CARD_OP       : 终端行为分析结果为脱机拒绝, 申请AAC时卡片出错
//		 HXEMV_CALLBACK_METHOD : 没有以回调方式初始化核心
int iHxTermActionAnalysis(void)
{
	int iRet;

	vTraceWriteTxtCr(TRACE_PROC, "iHxTermActionAnalysis()");
	if(gl_iCoreCallFlag != HXCORE_CALLBACK)
		return(HXEMV_CALLBACK_METHOD); // 没有以回调方式初始化核心
	if(gl_iCoreStatus<EMV_STATUS_READ_REC || gl_iCoreStatus>=EMV_STATUS_GAC1)
		return(HXEMV_FLOW_ERROR); // 读记录前、GAC后不允许做终端行为分析

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
			return(HXEMV_CARD_REMOVED); // 卡被取走
		} else {
			iEmvIODispMsg(EMVMSG_06_CARD_ERROR, EMVIO_NEED_NO_CONFIRM);
			return(HXEMV_CARD_OP); // 卡片操作错误，交易终止
		}
	}
	if(iRet) {
		iEmvIODispMsg(EMVMSG_NATIVE_ERR, EMVIO_NEED_NO_CONFIRM);
		return(iRet);
	}

	gl_iCoreStatus = EMV_STATUS_TERM_ACTION_ANALYSIS;

	return(HXEMV_OK);
}

// 第一次GAC
// out : piCardAction      : 卡片执行结果
//                           GAC_ACTION_TC     : 批准(生成TC)
//							 GAC_ACTION_AAC    : 拒绝(生成AAC)
//                           GAC_ACTION_AAC_ADVICE : 拒绝(生成AAC,有Advice)
//                           GAC_ACTION_ARQC   : 要求联机(生成ARQC)
// ret : HXEMV_OK          : OK
//       HXEMV_CORE        : 其它错误
//       HXEMV_CARD_REMOVED: 卡被取走
//       HXEMV_CARD_OP     : 卡操作错
//       HXEMV_CARD_SW     : 非法状态字
//       HXEMV_TERMINATE   : 满足拒绝条件，交易终止
//       HXEMV_NOT_ACCEPTED: 不接受
//       HXEMV_NOT_ALLOWED : 服务不允许
//       HXEMV_FLOW_ERROR  : EMV流程错误
//		 HXEMV_CALLBACK_METHOD : 没有以回调方式初始化核心
// Note: 某些情况下, 该函数会自动执行第二次Gac
//       如:CDA失败、仅脱机终端GAC1卡片返回ARQC
int iHx1stGAC(int *piCardAction)
{
	int iRet;

	vTraceWriteTxtCr(TRACE_PROC, "iHx1stGAC()");
	if(gl_iCoreCallFlag != HXCORE_CALLBACK)
		return(HXEMV_CALLBACK_METHOD); // 没有以回调方式初始化核心
	if(gl_iCoreStatus != EMV_STATUS_TERM_ACTION_ANALYSIS)
		return(HXEMV_FLOW_ERROR); // 终端行为分析后才可以做GAC1

	iEmvIOShowMsg(EMVMSG_0E_PLEASE_WAIT); // newadd

	iRet = iEmvGAC1(piCardAction);
	vTraceWriteTxtCr(TRACE_PROC, "(%d)iEmvGAC1(%d)", iRet, *piCardAction);
	if(iRet!=HXEMV_OK || *piCardAction!=GAC_ACTION_ARQC) {
		// 出错或卡片返回非ARQC(如果返回ARQC, 则GAC2后trace), TRACE
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
			return(HXEMV_CARD_REMOVED); // 卡被取走
		} else {
			iEmvIODispMsg(EMVMSG_06_CARD_ERROR, EMVIO_NEED_NO_CONFIRM);
			return(HXEMV_CARD_OP); // 卡片操作错误，交易终止
		}
	}
	if(iRet == HXEMV_CARD_SW) {
		// Refer to EMV2008 book3 6.3.5, P47, 不识别的SW需要终止交易
		iEmvIODispMsg(EMVMSG_TERMINATE, EMVIO_NEED_NO_CONFIRM);
		return(HXEMV_CARD_SW);
	}
	if(iRet) {
		iEmvIODispMsg(EMVMSG_NATIVE_ERR, EMVIO_NEED_NO_CONFIRM);
		return(HXEMV_CORE);
	}

	gl_iCoreStatus = EMV_STATUS_GAC1;

	// GAC1成功执行
	if(*piCardAction == GAC_ACTION_TC) {
		// 卡片脱机接受了交易
		iEmvIODispMsg(EMVMSG_03_APPROVED, EMVIO_NEED_NO_CONFIRM);
		return(HXEMV_OK);
	}
	if(*piCardAction==GAC_ACTION_AAC || *piCardAction==GAC_ACTION_AAC_ADVICE) {
		// 卡片拒绝了交易
		iEmvIODispMsg(EMVMSG_07_DECLINED, EMVIO_NEED_NO_CONFIRM);
		return(HXEMV_OK);
	}

	return(HXEMV_OK); // 卡片请求联机
}

// 第二次GAC
// in  : pszArc            : 应答码[2], NULL或""表示联机失败
//       pszAuthCode	   : 授权码[6], NULL或""表示无授权码数据
//       psOnlineData      : 联机响应数据(55域内容),一组TLV格式数据, NULL表示无联机响应数据
//       psOnlineDataLen   : 联机响应数据长度
// out : piCardAction      : 卡片执行结果
//                           GAC_ACTION_TC     : 批准(生成TC)
//							 GAC_ACTION_AAC    : 拒绝(生成AAC)
//                           GAC_ACTION_AAC_ADVICE : 拒绝(生成AAC,有Advice)
// ret : HXEMV_OK          : OK
//       HXEMV_CORE        : 其它错误
//       HXEMV_CARD_REMOVED: 卡被取走
//       HXEMV_CARD_OP     : 卡操作错
//       HXEMV_CARD_SW     : 非法卡状态字
//       HXEMV_TERMINATE   : 满足拒绝条件，交易终止
//       HXEMV_NOT_ALLOWED : 服务不允许
//       HXEMV_NOT_ACCEPTED: 不接受
//       HXEMV_FLOW_ERROR  : EMV流程错误
//		 HXEMV_CALLBACK_METHOD : 没有以回调方式初始化核心
int iHx2ndGAC(uchar *pszArc, uchar *pszAuthCode, uchar *psIssuerData, int iIssuerDataLen, int *piCardAction)
{
	int iRet;

	if(pszArc)
		vTraceWriteTxtCr(TRACE_PROC, "iHx2ndGAC(%s)", pszArc);
	else
		vTraceWriteTxtCr(TRACE_PROC, "iHx2ndGAC(NULL)");
	if(gl_iCoreCallFlag != HXCORE_CALLBACK)
		return(HXEMV_CALLBACK_METHOD); // 没有以回调方式初始化核心
	if(gl_iCoreStatus != EMV_STATUS_GAC1)
		return(HXEMV_FLOW_ERROR); // GAC1后才可以做GAC2

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
			return(HXEMV_CARD_REMOVED); // 卡被取走
		} else {
			iEmvIODispMsg(EMVMSG_06_CARD_ERROR, EMVIO_NEED_NO_CONFIRM);
			return(HXEMV_CARD_OP); // 卡片操作错误，交易终止
		}
	}
	if(iRet == HXEMV_CARD_SW) {
		// Refer to EMV2008 book3 6.3.5, P47, 不识别的SW需要终止交易
		iEmvIODispMsg(EMVMSG_TERMINATE, EMVIO_NEED_NO_CONFIRM);
		return(HXEMV_CARD_SW);
	}
	if(iRet) {
		iEmvIODispMsg(EMVMSG_NATIVE_ERR, EMVIO_NEED_NO_CONFIRM);
		return(HXEMV_CORE);
	}

	gl_iCoreStatus = EMV_STATUS_GAC2;

	// GAC2成功执行
	if(*piCardAction == GAC_ACTION_TC) {
		// 卡片脱机接受了交易
		iEmvIODispMsg(EMVMSG_03_APPROVED, EMVIO_NEED_NO_CONFIRM);
		return(HXEMV_OK);
	}
	// 卡片拒绝了交易
	iEmvIODispMsg(EMVMSG_07_DECLINED, EMVIO_NEED_NO_CONFIRM);
	return(HXEMV_OK);
}

//// 以上为回调方式API
//// 回调方式API  *** 结束 ***

//// 以下为非回调方式API
//// 非回调方式API  *** 开始 ***

// 复位卡片
// out : piErrMsgType      : 错误信息类型码
// ret : HXEMV_OK          : OK
//       HXEMV_FLOW_ERROR  : EMV流程错误
//       HXEMV_CARD_REMOVED: 卡被取走
//       HXEMV_CARD_OP     : 卡片复位错误
//		 HXEMV_CALLBACK_METHOD : 没有以非回调方式初始化核心
// Note: emv规范要求插卡后要显示请等待信息, 应用层可在调用本函数前先行显示EMVMSG_0E_PLEASE_WAIT,
int iHxResetCard2(int *piErrMsgType)
{
	uint uiRet;

	vTraceWriteTxtCr(TRACE_PROC, "iHxResetCard2()");
	*piErrMsgType = EMVMSG_APPCALL_ERR; // 赋缺省值
	if(gl_iCoreCallFlag != HXCORE_NOT_CALLBACK)
		return(HXEMV_CALLBACK_METHOD); // 没有以非回调方式初始化核心
	if(gl_iCoreStatus != EMV_STATUS_TRANS_INIT)
		return(HXEMV_FLOW_ERROR); // 交易初始化后才可以复位卡片

	uiRet = uiEmvCmdResetCard();
	if(uiRet != 0) {
		if(uiEmvTestCard() == 0) {
			*piErrMsgType = EMVMSG_0F_PROCESSING_ERROR;
			return(HXEMV_CARD_REMOVED); // 卡被取走
		} else {
			*piErrMsgType = EMVMSG_06_CARD_ERROR;
			if(iEmvTestItemBit(EMV_ITEM_TERM_CAP, TERM_CAP_01_MAG_STRIPE)) {
				// 存在磁卡读卡器
				*piErrMsgType = EMVMSG_12_USE_MAG_STRIPE;
			}
			return(HXEMV_CARD_OP); // 卡片操作错误，交易终止
		}
	}

	gl_iCoreStatus = EMV_STATUS_RESET;
	*piErrMsgType = EMVMSG_00_NULL;
	return(HXEMV_OK);
}

// 读取支持的应用
// in  : iIgnoreBlock	   : !0:忽略应用锁定标志, 0:锁定的应用不会被选择
//       piAdfNum          : 可容纳的终端与卡片同时支持的Adf个数
// out : paAdfInfo         : 终端与卡片同时支持的Adf列表(以优先级排序)
//       piAdfNum          : 终端与卡片同时支持的Adf个数
//       piMsgType         : 函数返回HXEMV_OK时, 返回 [应用选择提示] 信息类型码
//							 函数返回!HXEMV_OK时, 如果存在, 则为附加错误信息
//       piErrMsgType      : 错误信息类型码, 函数返回!HXEMV_OK并且!HXEMV_AUTO_SELECT时存在
// ret : HXEMV_OK          : OK, 获取的应用必须确认
//       HXEMV_AUTO_SELECT : OK, 获取的应用可自动选择
//       HXEMV_NO_APP      : 无支持的应用
//       HXEMV_CARD_REMOVED: 卡被取走
//       HXEMV_CARD_SW     : 卡状态字非法
//       HXEMV_CARD_OP     : 卡操作错
//       HXEMV_TERMINATE   : 满足拒绝条件，交易终止
//       HXEMV_FLOW_ERROR  : EMV流程错误
//       HXEMV_LACK_MEMORY : 存储空间不足
//		 HXEMV_CALLBACK_METHOD : 没有以非回调方式初始化核心
// 非接可能返回码
//		 HXEMV_TRY_OTHER_INTERFACE	: 尝试其它通信界面
//		 HXEMV_CARD_TRY_AGAIN		: 要求重新提交卡片(非接)
int iHxGetSupportedApp2(int iIgnoreBlock, stADFInfo *paAdfInfo, int *piAdfNum, int *piMsgType, int *piErrMsgType)
{
	int   iRet;
	uchar szFinalLanguage[3];
	uchar *psValue, szPinpadLanguage[21];
	uchar *p, ucAppConfirmSupport;        // TDFxx, 1:支持应用确认 0:不支持应用确认(TAG_DFXX_AppConfirmSupport)
	int   iFirstCallFlag;                 // 当前交易第一次本函数标志, 0:不是第一次 1:是第一次

	vTraceWriteTxtCr(TRACE_PROC, "iHxGetSupportedApp2(%d,%d)", iIgnoreBlock, *piAdfNum);
	*piMsgType = EMVMSG_00_NULL;
	*piErrMsgType = EMVMSG_APPCALL_ERR; // 赋缺省值
	if(gl_iCoreCallFlag != HXCORE_NOT_CALLBACK)
		return(HXEMV_CALLBACK_METHOD); // 没有以非回调方式初始化核心
	if(gl_iCoreStatus<EMV_STATUS_RESET || gl_iCoreStatus>=EMV_STATUS_GPO)
		return(HXEMV_FLOW_ERROR); // 卡片复位后、GPO成功之前可以获取支持的应用

	iFirstCallFlag = 0; // 先假设不是第一次调用
	if(sg_iAidGetFlag == 0) {
	    // 表示应用列表尚未获取, 搜索共同支持的aid列表
		iFirstCallFlag = 1; // 确定是第一次调用
		iRet = iSelGetAids(iIgnoreBlock);
		vTraceWriteTxtCr(TRACE_PROC, "(%d)iSelGetAids(%d)", iRet, iIgnoreBlock);

		if(iRet == SEL_ERR_CARD) {
			if(gl_iCardType == EMV_CONTACTLESS_CARD) {
				// 非接卡, 走失效序列, 参考JR/T0025.12—2013图4, P11
				*piErrMsgType = EMVMSG_13_TRY_AGAIN;
				return(HXEMV_CARD_TRY_AGAIN);
			}
			if(uiEmvTestCard() == 0) {
				*piErrMsgType = EMVMSG_0F_PROCESSING_ERROR;
				return(HXEMV_CARD_REMOVED); // 卡被取走
			} else {
				*piErrMsgType = EMVMSG_06_CARD_ERROR;
				return(HXEMV_CARD_OP); // 卡片操作错误，交易终止
			}
		}
		if(iRet == SEL_ERR_CARD_SW) {
			if(gl_iCardType == EMV_CONTACTLESS_CARD) {
				// 非接卡, 参考JR/T0025.12—2013图4, P11
				*piErrMsgType = EMVMSG_TERMINATE;
				return(HXEMV_TRY_OTHER_INTERFACE);
			}
			// Refer to EMV2008 book3 6.3.5, P47, 不识别的SW需要终止交易
			*piErrMsgType = EMVMSG_TERMINATE;
			return(HXEMV_CARD_SW); // 卡状态字非法，交易终止
		}

		if(iRet == SEL_ERR_CARD_BLOCKED) {
			*piErrMsgType = EMVMSG_TERMINATE;
			return(HXEMV_TERMINATE); // 卡片被锁，交易终止
		}
		if(iRet == SEL_ERR_DATA_ERROR) {
			*piErrMsgType = EMVMSG_TERMINATE;
			return(HXEMV_TERMINATE); // 应用相关数据非法，交易终止
		}
		if(iRet == SEL_ERR_DATA_ABSENT) {
			*piErrMsgType = EMVMSG_TERMINATE;
			return(HXEMV_TERMINATE); // 必要数据不存在，交易终止
		}
		if(iRet < 0) {
			*piErrMsgType = EMVMSG_NATIVE_ERR;
			return(HXEMV_CORE); // 内存不足，作为内部错误返回
		}
		if(iRet == 0) {
			// refer to emv2008 book4 11.3, P91 wlfxy
			*piErrMsgType = EMVMSG_0C_NOT_ACCEPTED;
			*piMsgType = EMVMSG_TERMINATE;
			return(HXEMV_NO_APP); // 无支持的应用
		}
		sg_iAidGetFlag = 1; // AID列表获取完毕
	}
	if(gl_iCardAppNum == 0) {
		*piErrMsgType = EMVMSG_TERMINATE; // 无支持的应用
		return(HXEMV_NO_APP);
	}
	if(*piAdfNum < gl_iCardAppNum) {
		*piErrMsgType = EMVMSG_NATIVE_ERR;
		return(HXEMV_LACK_MEMORY);
	}
	*piAdfNum = gl_iCardAppNum;  // 如果返回的AID个数少于缓冲区大小, 返回正确的个数, 否则返回缓冲区容许的最大个数

	// 获取终端应用确认支持情况
	iRet = iTlvGetObjValue(gl_sTlvDbTermFixed, TAG_DFXX_AppConfirmSupport, &p); // 获取终端应用选择支持情况
	if(iRet <= 0)
		ucAppConfirmSupport = 1; // 如果获取不成功, 假设终端支持确认
	else
		ucAppConfirmSupport = *p;

	// 获取最终匹配的语言
	iRet = iEmvMsgTableGetFitLanguage(gl_aCardAppList[0].szLanguage, szFinalLanguage);
	// 使用匹配的语言, 也可以强制使用其它语言
    iEmvMsgTableSetLanguage(szFinalLanguage);
	// 将匹配的语言代码存入TLV数据库(TAG_DFXX_LanguageUsed)
	iRet = iTlvMakeAddObj(gl_sTlvDbTermVar, TAG_DFXX_LanguageUsed, 2, szFinalLanguage, TLV_CONFLICT_REPLACE);
	if(iRet < 0) {
		EMV_TRACE_ERR_MSG
		*piErrMsgType = EMVMSG_NATIVE_ERR;
		return(HXEMV_CORE);
	}

	// 获取Pinpad使用的语言
	iRet = iTlvGetObjValue(gl_sTlvDbTermFixed, TAG_DFXX_PinpadLanguage, &psValue);
	if(iRet < 0) {
		EMV_TRACE_ERR_MSG
		*piErrMsgType = EMVMSG_NATIVE_ERR;
		return(HXEMV_CORE);
	}
	memcpy(szPinpadLanguage, psValue, iRet);
	szPinpadLanguage[iRet] = 0;
	iRet = iEmvMsgTableGetPinpadLanguage(gl_aCardAppList[0].szLanguage, szPinpadLanguage, szFinalLanguage);
	// 将匹配的语言代码存入TLV数据库(TAG_DFXX_LanguageUsed)
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

	if(ucAppConfirmSupport==0 || gl_iCardType==EMV_CONTACTLESS_CARD) // 0:终端不支持应用确认 或 卡片为非接卡
		return(HXEMV_AUTO_SELECT); // 不支持应用确认, 返回可自动选择

	// 终端支持应用确认
	if(iFirstCallFlag && *piAdfNum==1) {
		// 第一次调用获取应用 && 只有一个候选应用
		if(paAdfInfo[0].iPriority>=0 && (paAdfInfo[0].iPriority&0x80) == 0)
			return(HXEMV_AUTO_SELECT); // 该应用不需要确认，返回可自动选择
	}

	return(HXEMV_OK); // 需要确认应用
}

// 应用选择
// in  : iIgnoreBlock	   : !0:忽略应用锁定标志, 0:锁定的应用不会被选择
//       iAidLen             : AID长度
//       psAid               : AID
// out : piErrMsgType        : 错误信息类型码
// ret : HXEMV_OK            : OK
//       HXEMV_CARD_REMOVED  : 卡被取走
//       HXEMV_CARD_OP       : 卡操作错
//       HXEMV_TERMINATE     : 满足拒绝条件，交易终止
//       HXEMV_FLOW_ERROR    : EMV流程错误
//       HXEMV_CORE          : 内部错误
//       HXEMV_RESELECT      : 需要重新选择应用
//       HXEMV_NOT_SUPPORTED : 选择了不支持的应用
//		 HXEMV_CALLBACK_METHOD : 没有以非回调方式初始化核心
// 非接可能返回码
//		 HXEMV_TRY_OTHER_INTERFACE	: 尝试其它通信界面
//		 HXEMV_CARD_TRY_AGAIN		: 要求重新提交卡片(非接)
// Note: 可通过获取9F7A判断是否支持eCash
int iHxAppSelect2(int iIgnoreBlock, int iAidLen, uchar *psAid, int *piErrMsgType)
{
	uchar *psPdol;
	int   iPdolLen;
	int   i;
	int   iRet;
	uchar *pucAppConfirmSupport;          // TDFxx, 1:支持应用确认 0:不支持应用确认(TAG_DFXX_AppConfirmSupport)
	
	{
		uchar szAid[33];
		vOneTwo0(psAid, iAidLen, szAid);
		vTraceWriteTxtCr(TRACE_PROC, "iHxAppSelect2(%d,%s)", iAidLen, szAid);
	}

	*piErrMsgType = EMVMSG_APPCALL_ERR; // 赋缺省值
	if(gl_iCoreCallFlag != HXCORE_NOT_CALLBACK)
		return(HXEMV_CALLBACK_METHOD); // 没有以非回调方式初始化核心
	if(gl_iCoreStatus<EMV_STATUS_GET_SUPPORTED_APP || gl_iCoreStatus>=EMV_STATUS_GPO)
		return(HXEMV_FLOW_ERROR); // 获取了支持的应用列表之后、GPO成功之前可以选择应用

	// 获取终端应用选择支持情况
	iRet = iTlvGetObjValue(gl_sTlvDbTermFixed, TAG_DFXX_AppConfirmSupport, &pucAppConfirmSupport);
	if(iRet <= 0) {
		EMV_TRACE_ERR_MSG
		return(SEL_ERR_OTHER);
	}

	// 检查选择的应用是否在支持的应用范围之内
	for(i=0; i<gl_iCardAppNum; i++) {
		if(iAidLen == gl_aCardAppList[i].ucAdfNameLen) {
			if(memcmp(psAid, gl_aCardAppList[i].sAdfName, iAidLen) == 0)
				break;
		}
	}
	if(i >= gl_iCardAppNum) {
		EMV_TRACE_ERR_MSG
		*piErrMsgType = EMVMSG_APPCALL_ERR;
		return(HXEMV_NOT_SUPPORTED); // 应用层出错, 选择了不支持的应用
	}

	iRet = iSelFinalSelect2(iIgnoreBlock, iAidLen, psAid);
	vTraceWriteTxtCr(TRACE_PROC, "(%d)iSelFinalSelect2()", iRet);

	if(iRet == SEL_ERR_CARD) {
		if(gl_iCardType == EMV_CONTACTLESS_CARD) {
			// 非接卡, 走失效序列, 参考JR/T0025.12—2013图4, P11
			*piErrMsgType = EMVMSG_13_TRY_AGAIN;
			return(HXEMV_CARD_TRY_AGAIN);
		}
		if(uiEmvTestCard() == 0) {
			*piErrMsgType = EMVMSG_0F_PROCESSING_ERROR;
			return(HXEMV_CARD_REMOVED); // 卡被取走
		} else {
			*piErrMsgType = EMVMSG_06_CARD_ERROR;
			return(HXEMV_CARD_OP); // 卡片操作错误，交易终止
		}
	}
	if(iRet == SEL_ERR_NO_APP) {
		if(gl_iCardType == EMV_CONTACTLESS_CARD) {
			// 非接卡, 参考JR/T0025.12—2013图4, P11
			*piErrMsgType = EMVMSG_TERMINATE;
			return(HXEMV_TRY_OTHER_INTERFACE);
		}
		*piErrMsgType = EMVMSG_APPCALL_ERR;
		return(HXEMV_NOT_SUPPORTED); // 应用层出错, 选择了不支持的应用
	}
	if(iRet == SEL_ERR_NEED_RESELECT) {
		if(gl_iCardType == EMV_CONTACTLESS_CARD) {
			// 非接卡, 参考JR/T0025.12—2013 6.4.1, P12
			// 对于非接卡, 要求SEL_ERR_NEED_RESELECT即需要终止交易, 并尝试其它界面
			*piErrMsgType = EMVMSG_TERMINATE;
			return(HXEMV_TRY_OTHER_INTERFACE);
		}
		if(gl_iCardAppNum > 0 ) {
			if(*pucAppConfirmSupport)
				*piErrMsgType = EMVMSG_13_TRY_AGAIN; // 终端支持应用确认时才需要显示"Try Again"
			else
				*piErrMsgType = EMVMSG_00_NULL;
			return(HXEMV_RESELECT); // 选中的应用有问题, 需要重新选择
		} else {
			*piErrMsgType = EMVMSG_TERMINATE;
			return(HXEMV_TERMINATE); // 选中的应用有问题, 再无可选应用了
		}
	}
	if(iRet) {
		*piErrMsgType = EMVMSG_NATIVE_ERR;
		return(HXEMV_CORE);
	}

	if(gl_iCardType == EMV_CONTACTLESS_CARD) {
		// 非接卡
		iPdolLen = iTlvGetObjValue(gl_sTlvDbCard, TAG_9F38_PDOL, &psPdol);
		if(iPdolLen <= 0) {
			// 非接卡无PDOL, 参考JR/T0025.12—2013图4, P11
			*piErrMsgType = EMVMSG_TERMINATE;
			return(HXEMV_TRY_OTHER_INTERFACE);
		}
		iRet = iTlvSearchDOLTag(psPdol, iPdolLen, TAG_9F66_ContactlessSupport, NULL);
		if(iRet <= 0) {
			// 非接卡PDOL无9F66, 参考JR/T0025.12—2013图4, P11
			*piErrMsgType = EMVMSG_TERMINATE;
			return(HXEMV_TRY_OTHER_INTERFACE);
		}
	}

	*piErrMsgType = EMVMSG_00_NULL;
	gl_iCoreStatus = EMV_STATUS_APP_SELECT;
	return(HXEMV_OK);
}

// 获取支持的语言以供持卡人选择
// out : pszLangs     : 支持的语言列表, 如果输出字符串长度为0, 表示不必持卡人自行选择
//       pszLangsDesc : '='号结尾的支持语言描述列表
//       piMsgType    : 函数返回HXEMV_OK并且pszLangs表明需要持卡人选择时表示选择语言提示信息类型码
//       piErrMsgType : 错误信息类型码
// ret : HXEMV_OK     : OK
//       HXEMV_FLOW_ERROR  : EMV流程错误
//		 HXEMV_CALLBACK_METHOD : 没有以非回调方式初始化核心
//       HXEMV_CORE        : 内部错误
int iHxGetSupportedLang2(uchar *pszLangs, uchar *pszLangsDesc, int *piMsgType, int *piErrMsgType)
{
	int   iRet;
	uchar szFinalLanguage[3];
	uchar *psValue;
	uchar szCardPreferredLang[9];

	vTraceWriteTxtCr(TRACE_PROC, "iHxGetSupportedLang2()");
	*piMsgType = EMVMSG_00_NULL;
	*piErrMsgType = EMVMSG_APPCALL_ERR; // 赋缺省值
	if(gl_iCoreCallFlag != HXCORE_NOT_CALLBACK)
		return(HXEMV_CALLBACK_METHOD); // 没有以非回调方式初始化核心
	if(gl_iCoreStatus<EMV_STATUS_APP_SELECT || gl_iCoreStatus>=EMV_STATUS_GPO)
		return(HXEMV_FLOW_ERROR); // 应用选择之后、GPO之前可以设置持卡人语言

	if(sg_szSelectedLang[0]) {
		// 持卡人已经选择过语言
		iHxSetCardHolderLang2(sg_szSelectedLang, piErrMsgType);
		*piErrMsgType = EMVMSG_00_NULL;
		pszLangs[0] = 0; // 不必再选择
		return(HXEMV_OK);
	}

	// 获取卡片应用支持的语言
	szCardPreferredLang[0] = 0;
	iRet = iTlvGetObjValue(gl_sTlvDbCard, TAG_5F2D_LanguagePrefer, &psValue);
	if(iRet>=0 && iRet<=8 && iRet%2==0)
		vMemcpy0(szCardPreferredLang, psValue, iRet);
	// 获取终端支持的语言
	iRet = iEmvIOSelectLang(NULL, pszLangs, pszLangsDesc);
	if(iRet != HXEMV_OK) {
		*piErrMsgType = EMVMSG_NATIVE_ERR;
		return(HXEMV_CORE);
	}
	// 检查是否有匹配
	iRet = iEmvMsgTableGetPinpadLanguage(szCardPreferredLang, pszLangs, szFinalLanguage);
	if(iRet==0 || strlen(pszLangs)<=2) {
		// 有匹配 or 终端不支持多语言, 不需要持卡人选择
		pszLangs[0] = 0;
		iHxSetCardHolderLang2(szFinalLanguage, piErrMsgType);
		*piErrMsgType = EMVMSG_00_NULL;
		return(HXEMV_OK);
	}

	// 无匹配, 需要持卡人选择
	*piErrMsgType = EMVMSG_00_NULL;
	*piMsgType = EMVMSG_SELECT_LANG;
	return(HXEMV_OK);
}

// 设置持卡人语言
// in  : pszLanguage  : 持卡人语言代码, 例如:"zh"
// out : piErrMsgType : 错误信息类型码
// ret : HXEMV_OK     : OK
//       HXEMV_FLOW_ERROR  : EMV流程错误
//		 HXEMV_CALLBACK_METHOD : 没有以非回调方式初始化核心
//       HXEMV_PARA   : 参数错误
//       HXEMV_CORE   : 内部错误
// Note: 如果不支持传入的语言, 将会使用本地语言
int iHxSetCardHolderLang2(uchar *pszLanguage, int *piErrMsgType)
{
	int   iRet;
	int   i;
	uchar *psValue;

	vTraceWriteTxtCr(TRACE_PROC, "iHxSetCardHolderLang2(%s)", pszLanguage);
	*piErrMsgType = EMVMSG_APPCALL_ERR; // 赋缺省值
	if(gl_iCoreCallFlag != HXCORE_NOT_CALLBACK)
		return(HXEMV_CALLBACK_METHOD); // 没有以非回调方式初始化核心
	if(gl_iCoreStatus<EMV_STATUS_GET_SUPPORTED_APP || gl_iCoreStatus>EMV_STATUS_APP_SELECT)
		return(HXEMV_FLOW_ERROR); // 获取了支持的应用列表之后、应用选择之前可以设置持卡人语言

	// 判断参数合法性
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
		return(HXEMV_PARA); // 传入的语言不是可用的语言
	}

	strcpy(sg_szSelectedLang, pszLanguage); // 保存
	// 设置语言
	iRet = iTlvMakeAddObj(gl_sTlvDbTermVar, TAG_DFXX_LanguageUsed, 2, pszLanguage, TLV_CONFLICT_REPLACE);
	if(iRet < 0) {
		*piErrMsgType = EMVMSG_NATIVE_ERR;
		return(HXEMV_CORE);
	}

	// 持卡人选择的pinpad语言也将作为终端匹配语言
	iRet = iTlvMakeAddObj(gl_sTlvDbTermVar, TAG_DFXX_LanguageUsed, 2, pszLanguage, TLV_CONFLICT_REPLACE);
	if(iRet < 0) {
		*piErrMsgType = EMVMSG_NATIVE_ERR;
		return(HXEMV_CORE);
	}
	*piErrMsgType = EMVMSG_00_NULL;
	return(HXEMV_OK);
}

// GPO
// in  : pszTransTime      : 交易时间，YYYYMMDDhhmmss
//       ulTTC             : 终端交易序号, 1-999999
//       uiTransType       : 交易类型, BCD格式(0xAABB, BB为按GB/T 15150 定义的处理码前2 位表示的金融交易类型, AA用于区分商品/服务)
//       pszAmount         : 交易金额, ascii码串, 不要小数点, 默认为缺省小数点位置, 例如:RMB1.00表示为"100"
//       uiCurrencyCode    : 货币代码
// out : piMsgType         : 信息类型, 额外显示信息, 函数返回HXEMV_RESELECT时附加此提示信息
//       piErrMsgType      : 错误信息类型码, 此信息比piMsgType优先
// ret : HXEMV_OK          : OK
//       HXEMV_CARD_REMOVED: 卡被取走
//       HXEMV_CARD_OP     : 卡操作错
//       HXEMV_CARD_SW     : 非法卡状态字
//       HXEMV_TERMINATE   : 满足拒绝条件，交易终止
//       HXEMV_RESELECT    : 需要重新选择应用
//       HXEMV_TRANS_NOT_ALLOWED : 交易不支持
//       HXEMV_PARA        : 参数错误
//       HXEMV_FLOW_ERROR  : EMV流程错误
//		 HXEMV_CALLBACK_METHOD : 没有以非回调方式初始化核心
//       HXEMV_CORE        : 内部错误
//       HXEMV_DENIAL	   : qPboc拒绝
// 非接可能返回码
//		 HXEMV_TRY_OTHER_INTERFACE	: 尝试其它通信界面
//		 HXEMV_CARD_TRY_AGAIN		: 要求重新提交卡片(非接)
// Note: 如果返回HXEMV_RESELECT，从iHxGetSupportedApp()开始重新执行流程
//       因为GPO失败后可能需要显示两条信息(不接受&重试一次), 因此最多可能返回了两条信息类型信息
int iHxGPO2(uchar *pszTransTime, ulong ulTTC, uint uiTransType, uchar *pszAmount, uint uiCurrencyCode, int *piMsgType, int *piErrMsgType)
{
	uchar ucTransType; // 交易类型, T9C值
	int   iRet;
	uchar sBuf[100];
	uchar *pucAppConfirmSupport;          // TDFxx, 1:支持应用确认 0:不支持应用确认(TAG_DFXX_AppConfirmSupport)

	vTraceWriteTxtCr(TRACE_PROC, "iHxGPO2(%s,%lu,%04X,%s,%u)", pszTransTime, ulTTC, uiTransType, pszAmount, uiCurrencyCode);
	*piMsgType = EMVMSG_00_NULL;
	*piErrMsgType = EMVMSG_APPCALL_ERR; // 赋缺省值
	if(gl_iCoreCallFlag != HXCORE_NOT_CALLBACK)
		return(HXEMV_CALLBACK_METHOD); // 没有以非回调方式初始化核心
	if(gl_iCoreStatus != EMV_STATUS_APP_SELECT)
		return(HXEMV_FLOW_ERROR); // 选择应用后才可以GPO

	// 获取终端应用选择支持情况
	iRet = iTlvGetObjValue(gl_sTlvDbTermFixed, TAG_DFXX_AppConfirmSupport, &pucAppConfirmSupport);
	if(iRet <= 0)
		return(SEL_ERR_OTHER);

	ucTransType = uiTransType & 0xFF; // 取得T9C值
	// 参数合法性检查
	iRet = iTestIfValidDateTime(pszTransTime);
	if(iRet || ulTTC<1 || ulTTC>999999L) {
		*piErrMsgType = EMVMSG_APPCALL_ERR;
		return(HXEMV_PARA);
	}
	switch(uiTransType) {
	case TRANS_TYPE_SALE:			// 消费(商品和服务)
		if(!iEmvTestItemBit(EMV_ITEM_TERM_CAP_ADD, TERM_CAP_ADD_01_GOODS)
				&& !iEmvTestItemBit(EMV_ITEM_TERM_CAP_ADD, TERM_CAP_ADD_02_SERVICES)) {
			iEmvIODispMsg(EMVMSG_TRANS_NOT_ALLOWED, EMVIO_NEED_NO_CONFIRM);
			return(HXEMV_TRANS_NOT_ALLOWED); // 如果不支持商品、服务, 则不支持消费交易
		}
		break;
	case TRANS_TYPE_GOODS:			// 商品
		if(!iEmvTestItemBit(EMV_ITEM_TERM_CAP_ADD, TERM_CAP_ADD_01_GOODS)) {
			iEmvIODispMsg(EMVMSG_TRANS_NOT_ALLOWED, EMVIO_NEED_NO_CONFIRM);
			return(HXEMV_TRANS_NOT_ALLOWED); // 如果不支持商品应用, 则不支持商品交易
		}
		break;
	case TRANS_TYPE_SERVICES:		// 服务
		if(!iEmvTestItemBit(EMV_ITEM_TERM_CAP_ADD, TERM_CAP_ADD_02_SERVICES)) {
			iEmvIODispMsg(EMVMSG_TRANS_NOT_ALLOWED, EMVIO_NEED_NO_CONFIRM);
			return(HXEMV_TRANS_NOT_ALLOWED); // 如果不支持服务应用, 则不支持服务交易
		}
		break;
	case TRANS_TYPE_PAYMENT:		// 支付
		if(!iEmvTestItemBit(EMV_ITEM_TERM_CAP_ADD, TERM_CAP_ADD_06_PAYMENT)) {
			iEmvIODispMsg(EMVMSG_TRANS_NOT_ALLOWED, EMVIO_NEED_NO_CONFIRM);
			return(HXEMV_TRANS_NOT_ALLOWED); // 如果不支持支付应用, 则不支持支付交易
		}
		break;
	case TRANS_TYPE_CASH:			// 取现
		if(!iEmvTestItemBit(EMV_ITEM_TERM_CAP_ADD, TERM_CAP_ADD_00_CASH)) {
			*piErrMsgType = EMVMSG_TRANS_NOT_ALLOWED;
			return(HXEMV_TRANS_NOT_ALLOWED); // 如果不支持现金应用, 则不支持取现交易
		}
		break;
	case TRANS_TYPE_RETURN:			// 退货
		break; // 由于终端性能扩展中没有对退货的定义, 认为支持
	case TRANS_TYPE_DEPOSIT:		// 存款
		if(!iEmvTestItemBit(EMV_ITEM_TERM_CAP_ADD, TERM_CAP_ADD_08_CASH_DEPOSIT)) {
			*piErrMsgType = EMVMSG_TRANS_NOT_ALLOWED;
			return(HXEMV_TRANS_NOT_ALLOWED); // 如果不支持存款应用, 则不支持存款交易
		}
		break;
	case TRANS_TYPE_AVAILABLE_INQ:	// 可用余额查询
	case TRANS_TYPE_BALANCE_INQ:	// 余额查询
		if(!iEmvTestItemBit(EMV_ITEM_TERM_CAP_ADD, TERM_CAP_ADD_04_INQUIRY)) {
			*piErrMsgType = EMVMSG_TRANS_NOT_ALLOWED;
			return(HXEMV_TRANS_NOT_ALLOWED); // 如果不支持存款应用, 则不支持存款交易
		}
		break;
	case TRANS_TYPE_TRANSFER:		// 转账
		if(!iEmvTestItemBit(EMV_ITEM_TERM_CAP_ADD, TERM_CAP_ADD_05_TRANSFER)) {
			*piErrMsgType = EMVMSG_TRANS_NOT_ALLOWED;
			return(HXEMV_TRANS_NOT_ALLOWED); // 如果不支持转账应用, 则不支持转账交易
		}
		break;
	default:
		if(gl_iAppType != 0/*0:送检程序*/)
			break; // 非送检程序, 允许所有交易
		*piErrMsgType = EMVMSG_TRANS_NOT_ALLOWED;
		return(HXEMV_TRANS_NOT_ALLOWED); // 不识别的交易类型
	}

	if(uiCurrencyCode>999) {
		*piErrMsgType = EMVMSG_APPCALL_ERR;
		return(HXEMV_PARA);
	}

	// 将交易日期、交易时间、终端交易流水号、交易类型、金额、货币代码存入tlv数据库
	vTwoOne(pszTransTime+2, 6, sBuf); // 交易日期
	iRet = iTlvMakeAddObj(gl_sTlvDbTermVar, TAG_9A_TransDate, 3, sBuf, TLV_CONFLICT_REPLACE);
	if(iRet < 0) {
		*piErrMsgType = EMVMSG_NATIVE_ERR;
		return(HXEMV_CORE);
	}
	vTwoOne(pszTransTime+8, 6, sBuf); // 交易时间
	iRet = iTlvMakeAddObj(gl_sTlvDbTermVar, TAG_9F21_TransTime, 3, sBuf, TLV_CONFLICT_REPLACE);
	if(iRet < 0) {
		*piErrMsgType = EMVMSG_NATIVE_ERR;
		return(HXEMV_CORE);
	}
	sprintf(sBuf, "%06lu", ulTTC);
	vTwoOne(sBuf, 6, sBuf+10); // T9F41允许N4-N8字节, 这里遵循8583协议,采用N6
	iRet = iTlvMakeAddObj(gl_sTlvDbTermVar, TAG_9F41_TransSeqCounter, 3, sBuf+10, TLV_CONFLICT_REPLACE);
	if(iRet < 0) {
		*piErrMsgType = EMVMSG_NATIVE_ERR;
		return(HXEMV_CORE);
	}
	
	vOneTwo0(&ucTransType, 1, sBuf); // 交易类型为n2型
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
		// 如果是非接卡, 将T9F66存入tlv数据库
		iRet = iTlvMakeAddObj(gl_sTlvDbTermVar, TAG_9F66_ContactlessSupport, 4, psPbocCtlsGet9F66(), TLV_CONFLICT_REPLACE);
		if(iRet < 0) {
			iEmvIODispMsg(EMVMSG_NATIVE_ERR, EMVIO_NEED_NO_CONFIRM);
			return(HXEMV_CORE);
		}
    } else { //Add by Qingbo Jia
        // 如果不是非接卡模式
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
		gl_iCoreStatus = EMV_STATUS_GET_SUPPORTED_APP; // 需要重新选择应用
		if(gl_iCardAppNum > 0) {
			if(*pucAppConfirmSupport)
				*piMsgType = EMVMSG_13_TRY_AGAIN; // 终端支持应用确认时才需要显示"Try Again"
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
			// 非接卡, 走失效序列, 参考JR/T0025.12—2013 6.5.2, P14
			*piErrMsgType = EMVMSG_13_TRY_AGAIN;
			return(HXEMV_CARD_TRY_AGAIN);
		}
		if(uiEmvTestCard() == 0) {
			*piErrMsgType = EMVMSG_0F_PROCESSING_ERROR;
			return(HXEMV_CARD_REMOVED); // 卡被取走
		} else {
			*piErrMsgType = EMVMSG_06_CARD_ERROR;
			return(HXEMV_CARD_OP); // 卡片操作错误，交易终止
		}
	}
	if(iRet == HXEMV_CARD_SW) {
		if(gl_iCardType == EMV_CONTACTLESS_CARD) {
			// 非接卡, 参考JR/T0025.12—2013 6.5.3, P14
			*piErrMsgType = EMVMSG_TERMINATE;
			return(HXEMV_TRY_OTHER_INTERFACE);
		}
		// Refer to EMV2008 book3 6.3.5, P47, 不识别的SW需要终止交易
		*piErrMsgType = EMVMSG_TERMINATE;
		return(HXEMV_CARD_SW);
	}
	if(iRet == HXEMV_TERMINATE) {
		*piErrMsgType = EMVMSG_TERMINATE;
		if(gl_iCardType == EMV_CONTACTLESS_CARD)
			return(HXEMV_TRY_OTHER_INTERFACE); // 非接卡, 认为应该这样做
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

// 读应用记录
// out : psRid+pszPan+piPanSeqNo这三项提供给应用层判断黑名单及分开消费情况
//		 psRid             : RID[5], NULL表示不需要传出
//       pszPan            : 账号
//       piPanSeqNo        : 账号序列号, -1表示无此内容, NULL表示不需要传出
//       piErrMsgType      : 错误信息类型码
// ret : HXEMV_OK          : OK
//       HXEMV_CARD_REMOVED: 卡被取走
//       HXEMV_CARD_OP     : 卡操作错
//       HXEMV_CARD_SW     : 非法卡指令状态字
//       HXEMV_TERMINATE   : 满足拒绝条件，交易终止
//       HXEMV_FLOW_ERROR  : EMV流程错误
//       HXEMV_CORE        : 内部错误
//		 HXEMV_CALLBACK_METHOD : 没有以非回调方式初始化核心
// 非接可能返回码
//		 HXEMV_TRY_OTHER_INTERFACE	: 尝试其它通信界面
int iHxReadRecord2(uchar *psRid, uchar *pszPan, int *piPanSeqNo, int *piErrMsgType)
{
	int   i;
	int   iRet;
	uchar *p;

	vTraceWriteTxtCr(TRACE_PROC, "iHxReadRecord2()");
	*piErrMsgType = EMVMSG_APPCALL_ERR; // 赋缺省值
	if(gl_iCoreCallFlag != HXCORE_NOT_CALLBACK)
		return(HXEMV_CALLBACK_METHOD); // 没有以非回调方式初始化核心
	if(gl_iCoreStatus != EMV_STATUS_GPO)
		return(HXEMV_FLOW_ERROR); // GPO后才可以读记录
	iRet = iPbocCtlsGetRoute(); // 交易走的路径, 0:接触Pboc(标准DC或EC) 1:非接触Pboc(标准DC或EC) 2:qPboc联机 3:qPboc脱机 4:qPboc拒绝
	if(iRet==2 || iRet==4)
		return(HXEMV_FLOW_ERROR); // qPboc联机或qPboc拒绝不可以读记录

	iRet = iEmvReadRecord();
	vTraceWriteTxtCr(TRACE_PROC, "(%d)iEmvReadRecord()", iRet);
	if(iRet == HXEMV_CARD_OP) {
		if(gl_iCardType == EMV_CONTACTLESS_CARD) {
			// 非接卡, 走失效序列, 参考JR/T0025.12—2013 6.5.2, P14
			*piErrMsgType = EMVMSG_13_TRY_AGAIN;
			return(HXEMV_CARD_TRY_AGAIN);
		}
		if(uiEmvTestCard() == 0) {
			*piErrMsgType = EMVMSG_0F_PROCESSING_ERROR;
			return(HXEMV_CARD_REMOVED); // 卡被取走
		} else {
			*piErrMsgType = EMVMSG_06_CARD_ERROR;
			return(HXEMV_CARD_OP); // 卡片操作错误，交易终止
		}
	}
	if(iRet == HXEMV_CARD_SW) {
		// Refer to EMV2008 book3 6.3.5, P47, 不识别的SW需要终止交易
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

	// 获取所需信息
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

// 设置卡片账户黑名单及分开消费信息
// in  : iBlackFlag        : 设置该卡是否为黑名单卡, 0:不为黑名单卡 1:黑名单卡
//       pszRecentAmount   : 最近一笔消费金额(用于分开消费检查)
// out : piErrMsgType      : 错误信息类型码
// ret : HXEMV_OK          : OK
//       HXEMV_CORE        : 内部错误
// Note: 如果读记录后发现卡片是黑名单卡或有最近消费记录, 则需要调用此函数设置账户信息
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

// SDA或DDA
// out : piNeedCheckCrlFlag  : 是否需要判断发卡行公钥证书为黑名单标志, 0:不需要判断 1:需要判断
//       psRid               : RID[5], psRid+pucCaIndex+psCertSerNo这三项提供给应用层判断发卡行公钥证书是否在黑名单列表中
//       pucCaIndex          : CA公钥索引
//       psCertSerNo         : 发卡行公钥证书序列号[3]
//       piErrMsgType        : 错误信息类型码
// ret : HXEMV_OK            : OK
//       HXEMV_CARD_REMOVED  : 卡被取走
//       HXEMV_CARD_OP       : 卡操作错
//       HXEMV_CARD_SW       : 非法卡状态字
//       HXEMV_TERMINATE     : 满足拒绝条件，交易终止
//       HXEMV_DENIAL        : 终端行为分析结果为脱机拒绝
//       HXEMV_DENIAL_ADVICE : 终端行为分析结果为脱机拒绝, 有Advice
//       HXEMV_FLOW_ERROR    : EMV流程错误
//       HXEMV_CORE          : 内部错误
//		 HXEMV_CALLBACK_METHOD : 没有以非回调方式初始化核心
//		 HXEMV_FDDA_FAIL	 : fDDA失败(非接)
// Note: 应用层支持发卡行公钥证书黑名单检查是, piNeedCheckCrlFlag才有意义, 否则可以忽略
// Note: qPboc, FDDA失败后, piErrMsgType不可用, 应用层需要自行检查9F6C B1 b6b5, 以决定是拒绝还是进行联机
int iHxOfflineDataAuth2(int *piNeedCheckCrlFlag, uchar *psRid, uchar *pucCaIndex, uchar *psCertSerNo, int *piErrMsgType)
{
	int iRet;
	uchar *pucCaPubKeyIndex;
	uchar *psAid;

	vTraceWriteTxtCr(TRACE_PROC, "iHxOfflineDataAuth2()");
	*piErrMsgType = EMVMSG_APPCALL_ERR; // 赋缺省值
	if(gl_iCoreCallFlag != HXCORE_NOT_CALLBACK)
		return(HXEMV_CALLBACK_METHOD); // 没有以非回调方式初始化核心
	if(gl_iCoreStatus<EMV_STATUS_READ_REC || gl_iCoreStatus>=EMV_STATUS_GAC1)
		return(HXEMV_FLOW_ERROR); // 读记录前、GAC后不允许做脱机数据认证

	*piNeedCheckCrlFlag = 0; // 先设置为不需要判断CRL
	iRet = iEmvOfflineDataAuth(0); // 非回调接口固定传入0, 即不检查发卡行公钥证书是否在CRL列表中
	vTraceWriteTxtCr(TRACE_PROC, "(%d)iEmvOfflineDataAuth()", iRet);
    if(iRet != HXEMV_OK) {
		vTraceTlvDb();
    }
	if(iRet == HXEMV_FDDA_FAIL) {
		*piErrMsgType = EMVMSG_00_NULL;
		return(HXEMV_FDDA_FAIL); // FDDA失败
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
			return(HXEMV_CARD_REMOVED); // 卡被取走
		} else {
			*piErrMsgType = EMVMSG_06_CARD_ERROR;
			return(HXEMV_CARD_OP); // 卡片操作错误，交易终止
		}
	}
	if(iRet == HXEMV_CARD_SW) {
		// Refer to EMV2008 book3 6.3.5, P47, 不识别的SW需要终止交易
		*piErrMsgType = EMVMSG_TERMINATE;
		return(HXEMV_CARD_SW);
	}
	if(iRet != HXEMV_OK) {
		*piErrMsgType = EMVMSG_NATIVE_ERR;
		return(HXEMV_CORE);
	}

	// 脱机数据认证没有报错, 准备应用层需要的CRL检查数据
	*piErrMsgType = EMVMSG_00_NULL;
	iRet = iTlvGetObjValue(gl_sTlvDbCard, TAG_8F_CAPubKeyIndex, &pucCaPubKeyIndex);
	if(iRet != 1)
		return(EMVIO_OK); // 找不到CA公钥索引, 不需要检查CRL
	*pucCaIndex = *pucCaPubKeyIndex;

	iRet = iTlvGetObjValue(gl_sTlvDbCard, TAG_4F_AID, &psAid);
	if(iRet < 5)
		return(EMVIO_OK);
	memcpy(psRid, psAid, 5);

	iEmvIOGetCertSerNo(psCertSerNo);

	*piNeedCheckCrlFlag = 1; // 设置需要判断CRL标志
	return(HXEMV_OK);
}

// 设置发卡行公钥证书为黑名单
// out : piErrMsgType        : 错误信息类型码, 0表示无需要显示的信息
// ret : HXEMV_OK            : OK
//       HXEMV_CARD_REMOVED  : 卡被取走
//       HXEMV_CARD_OP       : 卡操作错
//       HXEMV_CARD_SW       : 非法卡状态字
//       HXEMV_TERMINATE     : 满足拒绝条件，交易终止
//       HXEMV_DENIAL        : 终端行为分析结果为脱机拒绝
//       HXEMV_DENIAL_ADVICE : 终端行为分析结果为脱机拒绝, 有Advice
//       HXEMV_FLOW_ERROR    : EMV流程错误
//       HXEMV_CORE          : 内部错误
//		 HXEMV_CALLBACK_METHOD : 没有以非回调方式初始化核心
// Note: 应用层调用iHxOfflineDataAuth2()后得到Rid+CaIndex+CertSerNo
//       根据此数据判断发卡行公钥证书是否为黑名单,只有是黑名单,才需要调用本函数通知核心
int iHxSetIssuerCertCrl2(int *piErrMsgType)
{
	int iRet;

	vTraceWriteTxtCr(TRACE_PROC, "iHxSetIssuerCertCrl2()");
	*piErrMsgType = EMVMSG_APPCALL_ERR; // 赋缺省值
	if(gl_iCoreCallFlag != HXCORE_NOT_CALLBACK)
		return(HXEMV_CALLBACK_METHOD); // 没有以非回调方式初始化核心
	if(gl_iCoreStatus<EMV_STATUS_READ_REC || gl_iCoreStatus>=EMV_STATUS_GAC1)
		return(HXEMV_FLOW_ERROR); // 读记录前、GAC后不允许做脱机数据认证

	iRet = iEmvOfflineDataAuth(1); // 传入1, 即设定发卡行公钥证书是在CRL列表中
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
			return(HXEMV_CARD_REMOVED); // 卡被取走
		} else {
			*piErrMsgType = EMVMSG_06_CARD_ERROR;
			return(HXEMV_CARD_OP); // 卡片操作错误，交易终止
		}
	}
	if(iRet == HXEMV_CARD_SW) {
		// Refer to EMV2008 book3 6.3.5, P47, 不识别的SW需要终止交易
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

// 处理限制
// out : piErrMsgType        : 错误信息类型码
// ret : HXEMV_OK            : OK
//       HXEMV_TERMINATE     : 满足拒绝条件，交易终止
//       HXEMV_FLOW_ERROR    : EMV流程错误
//       HXEMV_CORE          : 内部错误
//       HXEMV_DENIAL        : 终端行为分析结果为脱机拒绝
//       HXEMV_DENIAL_ADVICE : 终端行为分析结果为脱机拒绝, 有Advice
//       HXEMV_CARD_REMOVED  : 卡被取走
//       HXEMV_CARD_OP       : 终端行为分析结果为脱机拒绝, 申请AAC时卡片出错
//		 HXEMV_CALLBACK_METHOD : 没有以非回调方式初始化核心
int iHxProcRistrictions2(int *piErrMsgType)
{
	int iRet;

	vTraceWriteTxtCr(TRACE_PROC, "iHxProcRistrictions2()");
	*piErrMsgType = EMVMSG_APPCALL_ERR; // 赋缺省值
	if(gl_iCoreCallFlag != HXCORE_NOT_CALLBACK)
		return(HXEMV_CALLBACK_METHOD); // 没有以非回调方式初始化核心
	if(gl_iCoreStatus<EMV_STATUS_READ_REC || gl_iCoreStatus>=EMV_STATUS_GAC1)
		return(HXEMV_FLOW_ERROR); // 读记录前、GAC后不允许做处理限制

	iRet = iEmvProcRistrictions();
	vTraceWriteTxtCr(TRACE_PROC, "(%d)iEmvProcRistrictions()", iRet);
	if(iRet != HXEMV_OK)
		vTraceTlvDb();
	if(iRet == HXEMV_CARD_OP) {
		if(uiEmvTestCard() == 0) {
			*piErrMsgType = EMVMSG_0F_PROCESSING_ERROR;
			return(HXEMV_CARD_REMOVED); // 卡被取走
		} else {
			*piErrMsgType = EMVMSG_06_CARD_ERROR;
			return(HXEMV_CARD_OP); // 卡片操作错误，交易终止
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

// 终端风险管理
// out : piErrMsgType        : 错误信息类型码
// ret : HXEMV_OK            : OK
//       HXEMV_TERMINATE     : 满足拒绝条件，交易终止
//       HXEMV_CORE          : 其它错误
//       HXEMV_CARD_REMOVED  : 卡被取走
//       HXEMV_CARD_OP       : 卡操作错
//       HXEMV_DENIAL        : 终端行为分析结果为脱机拒绝
//       HXEMV_DENIAL_ADVICE : 终端行为分析结果为脱机拒绝, 有Advice
//       HXEMV_FLOW_ERROR    : EMV流程错误
//		 HXEMV_CALLBACK_METHOD : 没有以非回调方式初始化核心
int iHxTermRiskManage2(int *piErrMsgType)
{
	int iRet;

	vTraceWriteTxtCr(TRACE_PROC, "iHxTermRiskManage2()");
	*piErrMsgType = EMVMSG_APPCALL_ERR; // 赋缺省值
	if(gl_iCoreCallFlag != HXCORE_NOT_CALLBACK)
		return(HXEMV_CALLBACK_METHOD); // 没有以非回调方式初始化核心
	if(gl_iCoreStatus<EMV_STATUS_READ_REC || gl_iCoreStatus>=EMV_STATUS_GAC1)
		return(HXEMV_FLOW_ERROR); // 读记录前、GAC后不允许做终端风险管理

	iRet = iEmvTermRiskManage();
	vTraceWriteTxtCr(TRACE_PROC, "(%d)iEmvTermRiskManage()", iRet);
	if(iRet != HXEMV_OK)
		vTraceTlvDb();
	if(iRet == HXEMV_CARD_OP) {
		if(uiEmvTestCard() == 0) {
			*piErrMsgType = EMVMSG_0F_PROCESSING_ERROR;
			return(HXEMV_CARD_REMOVED); // 卡被取走
		} else {
			*piErrMsgType = EMVMSG_06_CARD_ERROR;
			return(HXEMV_CARD_OP); // 卡片操作错误，交易终止
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

// 获取持卡人验证
// out : piCvm               : 持卡人认证方法, 支持HXCVM_PLAIN_PIN、HXCVM_CIPHERED_ONLINE_PIN、HXCVM_HOLDER_ID、HXCVM_CONFIRM_AMOUNT
//       piBypassFlag        : 允许bypass标志, 0:不允许, 1:允许
//       piMsgType           : 提示信息码(如果验证方法为证件验证, 返回证件类型码)
//       piMsgType2          : 附加提示信息, 如果不为0, 则需要先显示本信息
//       pszAmountStr        : 提示用格式化后金额串(HXCVM_HOLDER_ID验证方法不需要)
//       piErrMsgType        : 错误信息类型码, 函数返回!HXEMV_OK并且!HXEMV_NO_DATA时存在
// ret : HXEMV_OK            : OK
//       HXEMV_NO_DATA       : 无需继续进行持卡人验证
//       HXEMV_CARD_REMOVED  : 卡被取走
//       HXEMV_CARD_OP       : 卡操作错
//       HXEMV_TERMINATE     : 满足拒绝条件，交易终止
//       HXEMV_DENIAL        : 终端行为分析结果为脱机拒绝
//       HXEMV_DENIAL_ADVICE : 终端行为分析结果为脱机拒绝, 有Advice
//       HXEMV_FLOW_ERROR    : EMV流程错误
//       HXEMV_CORE          : 内部错误
//		 HXEMV_CALLBACK_METHOD : 没有以非回调方式初始化核心
// Note: 与iHxDoCvmMethod2()搭配, 可能需要多次调用, 知道返回HXEMV_NO_DATA
//       由于返回的信息较多, 对于支持HXCVM_HOLDER_ID验证方法, piMsgType返回的是证件类型码
//       此时其它信息使用固定信息码, 描述如下:
//           EMVMSG_VERIFY_HOLDER_ID		"请查验持卡人证件"
//			 EMVMSG_HOLDER_ID_TYPE			"持卡人证件类型"
//			 EMVMSG_HOLDER_ID_NO			"持卡人证件号码"
//       证件号码提取Tag为:TAG_9F61_HolderId
int iHxGetCvmMethod2(int *piCvm, int *piBypassFlag, int *piMsgType, int *piMsgType2, uchar *pszAmountStr, int *piErrMsgType)
{
	int iRet;

	vTraceWriteTxtCr(TRACE_PROC, "iHxGetCvmMethod2()");
	*piMsgType = EMVMSG_00_NULL;
	*piMsgType2 = EMVMSG_00_NULL;
	*piErrMsgType = EMVMSG_APPCALL_ERR; // 赋缺省值
	if(gl_iCoreCallFlag != HXCORE_NOT_CALLBACK)
		return(HXEMV_CALLBACK_METHOD); // 没有以非回调方式初始化核心
	if(gl_iCoreStatus<EMV_STATUS_READ_REC || gl_iCoreStatus>=EMV_STATUS_GAC1)
		return(HXEMV_FLOW_ERROR); // 读记录前、GAC后不允许CVM处理

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
			return(HXEMV_CARD_REMOVED); // 卡被取走
		} else {
			*piErrMsgType = EMVMSG_06_CARD_ERROR;
			return(HXEMV_CARD_OP); // 卡片操作错误，交易终止
		}
	}
	if(iRet!=HXEMV_OK && iRet!=HXEMV_NO_DATA) {
		*piErrMsgType = EMVMSG_NATIVE_ERR;
		return(HXEMV_CORE);
	}

	*piErrMsgType = EMVMSG_00_NULL;
	return(iRet);
}

// 执行持卡人认证
// in  : iCvmProc            : 认证方法处理方式, HXCVM_PROC_OK or HXCVM_BYPASS or HXCVM_FAIL or HXCVM_CANCEL or HXCVM_TIMEOUT
//       psCvmData           : 输入的密码, 如果为明文密码, 密码尾部要补0
// out : piMsgType           : 返回HXEMV_OK时使用
//                             如果不为0, 则需要显示本类型信息
//       piMsgType2          : 返回HXEMV_OK时使用
//                             如果不为0, 则需要显示本类型信息
//       piErrMsgType        : 错误信息类型码
// ret : HXEMV_OK            : OK, 需要继续进行持卡人验证, 继续调用iHxGetCvmMethod2(), 然后再调用本函数
//       HXEMV_PARA		     : 参数错误
//       HXEMV_CANCEL        : 用户取消
//       HXEMV_TIMEOUT       : 超时
//       HXEMV_CARD_REMOVED  : 卡被取走
//       HXEMV_CARD_OP       : 卡操作错
//       HXEMV_CARD_SW       : 非法卡状态字
//       HXEMV_TERMINATE     : 满足拒绝条件，交易终止
//       HXEMV_DENIAL        : 终端行为分析结果为脱机拒绝
//       HXEMV_DENIAL_ADVICE : 终端行为分析结果为脱机拒绝, 有Advice
//       HXEMV_FLOW_ERROR    : EMV流程错误
//       HXEMV_CORE          : 内部错误
//		 HXEMV_CALLBACK_METHOD : 没有以非回调方式初始化核心
// Note: 执行的CVM必须是最近一次iHxGetCvmMethod2()获得的CVM
int iHxDoCvmMethod2(int iCvmProc, uchar *psCvmData, int *piMsgType, int *piMsgType2, int *piErrMsgType)
{
	int iRet;

	vTraceWriteTxtCr(TRACE_PROC, "iHxDoCvmMethod2(%d)", iCvmProc);
	*piMsgType = EMVMSG_00_NULL;
	*piMsgType2 = EMVMSG_00_NULL;
	if(gl_iCoreCallFlag != HXCORE_NOT_CALLBACK)
		return(HXEMV_CALLBACK_METHOD); // 没有以非回调方式初始化核心
	if(gl_iCoreStatus<EMV_STATUS_READ_REC || gl_iCoreStatus>=EMV_STATUS_GAC1)
		return(HXEMV_FLOW_ERROR); // 读记录前、GAC后不允许CVM处理

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
			return(HXEMV_CARD_REMOVED); // 卡被取走
		} else {
			*piErrMsgType = EMVMSG_06_CARD_ERROR;
			return(HXEMV_CARD_OP); // 卡片操作错误，交易终止
		}
	}
	if(iRet == HXEMV_CARD_SW) {
		// Refer to EMV2008 book3 6.3.5, P47, 不识别的SW需要终止交易
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

// 获取CVM认证方法签字标志
// out : piNeedSignFlag    : 表示需要签字标志，0:不需要签字 1:需要签字
// ret : HXEMV_OK          : OK
int iHxGetCvmSignFlag2(int *piNeedSignFlag)
{
	iCvm2GetSignFlag(piNeedSignFlag);
	vTraceWriteTxtCr(TRACE_PROC, "(0)iHxGetCvmSignFlag2(%d)", *piNeedSignFlag);
	return(HXEMV_OK);
}

// 终端行为分析
// out : piErrMsgType        : 错误信息类型码
// ret : HXEMV_OK            : OK
//       HXEMV_CORE          : 其它错误
//       HXEMV_FLOW_ERROR    : EMV流程错误
//       HXEMV_CARD_REMOVED  : 卡被取走
//       HXEMV_CARD_OP       : 卡操作错
//       HXEMV_DENIAL        : 终端行为分析结果为脱机拒绝
//       HXEMV_DENIAL_ADVICE : 终端行为分析结果为脱机拒绝, 有Advice
//		 HXEMV_CALLBACK_METHOD : 没有以非回调方式初始化核心
int iHxTermActionAnalysis2(int *piErrMsgType)
{
	int iRet;

	vTraceWriteTxtCr(TRACE_PROC, "iHxTermActionAnalysis2()");
	*piErrMsgType = EMVMSG_APPCALL_ERR; // 赋缺省值
	if(gl_iCoreCallFlag != HXCORE_NOT_CALLBACK)
		return(HXEMV_CALLBACK_METHOD); // 没有以非回调方式初始化核心
	if(gl_iCoreStatus<EMV_STATUS_READ_REC || gl_iCoreStatus>=EMV_STATUS_GAC1)
		return(HXEMV_FLOW_ERROR); // 读记录前、GAC后不允许做终端行为分析

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
			return(HXEMV_CARD_REMOVED); // 卡被取走
		} else {
			*piErrMsgType = EMVMSG_06_CARD_ERROR;
			return(HXEMV_CARD_OP); // 卡片操作错误，交易终止
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

// 第一次GAC
// out : piCardAction      : 卡片执行结果
//                           GAC_ACTION_TC     : 批准(生成TC)
//							 GAC_ACTION_AAC    : 拒绝(生成AAC)
//                           GAC_ACTION_AAC_ADVICE : 拒绝(生成AAC,有Advice)
//                           GAC_ACTION_ARQC   : 要求联机(生成ARQC)
//       piMsgType         : 提示信息码, 函数返回HXEMV_OK时使用
//       piErrMsgType      : 错误信息类型码
// ret : HXEMV_OK          : OK
//       HXEMV_CORE        : 其它错误
//       HXEMV_CARD_REMOVED: 卡被取走
//       HXEMV_CARD_OP     : 卡操作错
//       HXEMV_CARD_SW     : 非法状态字
//       HXEMV_TERMINATE   : 满足拒绝条件，交易终止
//       HXEMV_NOT_ACCEPTED: 不接受
//       HXEMV_NOT_ALLOWED : 服务不允许
//       HXEMV_FLOW_ERROR  : EMV流程错误
// Note: 某些情况下, 该函数会自动调用iHx2ndGAC()
//       如:CDA失败、仅脱机终端GAC1卡片返回ARQC...
//		 HXEMV_CALLBACK_METHOD : 没有以非回调方式初始化核心
int iHx1stGAC2(int *piCardAction, int *piMsgType, int *piErrMsgType)
{
	int iRet;

	vTraceWriteTxtCr(TRACE_PROC, "iHx1stGAC2()");
	*piMsgType = EMVMSG_00_NULL;
	*piErrMsgType = EMVMSG_APPCALL_ERR; // 赋缺省值
	if(gl_iCoreCallFlag != HXCORE_NOT_CALLBACK)
		return(HXEMV_CALLBACK_METHOD); // 没有以非回调方式初始化核心
	if(gl_iCoreStatus != EMV_STATUS_TERM_ACTION_ANALYSIS)
		return(HXEMV_FLOW_ERROR); // 终端行为分析后才可以做GAC1

	iRet = iEmvGAC1(piCardAction);
	vTraceWriteTxtCr(TRACE_PROC, "(%d)iEmvGAC1(%d)", iRet, *piCardAction);
	if(iRet!=HXEMV_OK || *piCardAction!=GAC_ACTION_ARQC) {
		// 出错或卡片返回非ARQC(如果返回ARQC, 则GAC2后trace), TRACE
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
			return(HXEMV_CARD_REMOVED); // 卡被取走
		} else {
			*piErrMsgType = EMVMSG_06_CARD_ERROR;
			return(HXEMV_CARD_OP); // 卡片操作错误，交易终止
		}
	}
	if(iRet == HXEMV_CARD_SW) {
		// Refer to EMV2008 book3 6.3.5, P47, 不识别的SW需要终止交易
		*piErrMsgType = EMVMSG_TERMINATE;
		return(HXEMV_CARD_SW);
	}
	if(iRet) {
		*piErrMsgType = EMVMSG_NATIVE_ERR;
		return(HXEMV_CORE);
	}

	gl_iCoreStatus = EMV_STATUS_GAC1;

	// GAC1成功执行
	*piErrMsgType = EMVMSG_00_NULL;
	if(*piCardAction == GAC_ACTION_TC) {
		// 卡片脱机接受了交易
		*piMsgType = EMVMSG_03_APPROVED;
		return(HXEMV_OK);
	}
	if(*piCardAction==GAC_ACTION_AAC || *piCardAction==GAC_ACTION_AAC_ADVICE) {
		// 卡片拒绝了交易
		*piMsgType = EMVMSG_07_DECLINED;
		return(HXEMV_OK);
	}

	return(HXEMV_OK); // 卡片请求联机, 高层自行显示联机信息
}

// 第二次GAC
// in  : pszArc            : 应答码[2], NULL或""表示联机失败
//       pszAuthCode	   : 授权码[6], NULL或""表示无授权码数据
//       psOnlineData      : 联机响应数据(55域内容),一组TLV格式数据, NULL表示无联机响应数据
//       psOnlineDataLen   : 联机响应数据长度
// out : piCardAction      : 卡片执行结果
//                           GAC_ACTION_TC     : 批准(生成TC)
//							 GAC_ACTION_AAC    : 拒绝(生成AAC)
//                           GAC_ACTION_AAC_ADVICE : 拒绝(生成AAC,有Advice)
//       piMsgType         : 提示信息码, 函数返回HXEMV_OK时使用
//       piErrMsgType      : 错误信息类型码
// ret : HXEMV_OK          : OK
//       HXEMV_CORE        : 其它错误
//       HXEMV_CARD_REMOVED: 卡被取走
//       HXEMV_CARD_OP     : 卡操作错
//       HXEMV_CARD_SW     : 非法卡状态字
//       HXEMV_TERMINATE   : 满足拒绝条件，交易终止
//       HXEMV_NOT_ALLOWED : 服务不允许
//       HXEMV_NOT_ACCEPTED: 不接受
//       HXEMV_FLOW_ERROR  : EMV流程错误
//		 HXEMV_CALLBACK_METHOD : 没有以非回调方式初始化核心
int iHx2ndGAC2(uchar *pszArc, uchar *pszAuthCode, uchar *psIssuerData, int iIssuerDataLen, int *piCardAction, int *piMsgType, int *piErrMsgType)
{
	int iRet;

	if(pszArc)
		vTraceWriteTxtCr(TRACE_PROC, "iHx2ndGAC2(%s)", pszArc);
	else
		vTraceWriteTxtCr(TRACE_PROC, "iHx2ndGAC2(NULL)");
	*piMsgType = EMVMSG_00_NULL;
	*piErrMsgType = EMVMSG_APPCALL_ERR; // 赋缺省值
	if(gl_iCoreCallFlag != HXCORE_NOT_CALLBACK)
		return(HXEMV_CALLBACK_METHOD); // 没有以非回调方式初始化核心
	if(gl_iCoreStatus != EMV_STATUS_GAC1)
		return(HXEMV_FLOW_ERROR); // GAC1后才可以做GAC2

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
			return(HXEMV_CARD_REMOVED); // 卡被取走
		} else {
			*piErrMsgType = EMVMSG_06_CARD_ERROR;
			return(HXEMV_CARD_OP); // 卡片操作错误，交易终止
		}
	}
	if(iRet == HXEMV_CARD_SW) {
		// Refer to EMV2008 book3 6.3.5, P47, 不识别的SW需要终止交易
		*piErrMsgType = EMVMSG_TERMINATE;
		return(HXEMV_CARD_SW);
	}
	if(iRet) {
		*piErrMsgType = EMVMSG_NATIVE_ERR;
		return(HXEMV_CORE);
	}

	gl_iCoreStatus = EMV_STATUS_GAC2;

	// GAC2成功执行
	*piErrMsgType = EMVMSG_00_NULL;
	if(*piCardAction == GAC_ACTION_TC) {
		// 卡片脱机接受了交易
		*piMsgType = EMVMSG_03_APPROVED;
		return(HXEMV_OK);
	}
	// 卡片拒绝了交易
	*piMsgType = EMVMSG_07_DECLINED;
	return(HXEMV_OK);
}

// 获取提示信息内容
// in  : iMsgType    : 信息类型, 参考EmvMsg.h
//       iMsgFor     : 信息接受者类型(HXMSG_SMART or HXMSG_OPERATOR or HXMSG_CARDHOLDER or HXMSG_LOCAL_LANG)
// out : pszMsg      : 按接受者语言返回的信息内容
//       pszLanguage : 使用的语言代码，ISO-639-1，小写. 传入NULL表示不需要返回
// ret : 返回值等于pszMsg
// Note: 如果信息类型不支持, pszMsg[0]会被赋值为0
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

//// 以上为非回调方式API
//// 非回调方式API  *** 结束 ***

/********
以下为核心接口之外的定制接口
********/

// 该函数为江苏银行使用
// 江苏银行提供的接口是在GAC1时给出部分关键数据, 因此需要在GAC前调整TLV数据库
// in  : pszAmount      : 授权金额, 空串表示不替换
//       pszAmountOther : 其它金额, 空串表示不替换
//       iCurrencyCode  : 货币代码, <0表示不替换
//       pszTransDate   : 交易日期(YYMMDD), 空串表示不替换
//       ucTransType    : 交易类型, 0xFF表示不替换
//       pszTransTime   : 交易时间(hhmmss), 空串表示不替换
//       iCountryCode   : 国家代码, <0表示不替换
//       pszNodeName    : 商户名称
// ret : HXEMV_OK       : OK
//       HXEMV_PARA     : 数据非法
//       HXEMV_CORE     : 内部错误
int iHxReplaceTlvDb_Jsb(uchar *pszAmount, uchar *pszAmountOther, int iCurrencyCode,
						uchar *pszTransDate, uchar ucTransType, uchar *pszTransTime, int iCountryCode, uchar *pszNodeName)
{
	int   iRet;
	uchar szBuf[100], sBuf[50];

	vTraceWriteTxtCr(TRACE_PROC, "iHxReplaceTlvDb_Jsb(%s,%s,%d,%s,%02X,%s,%d,%s)",
		                          pszAmount, pszAmountOther, iCurrencyCode, pszTransDate,
								  (uint)ucTransType, pszTransTime, iCountryCode, pszNodeName);

	// 授权金额
	if(strlen(pszAmount)) {
		if(strlen(pszAmount) > 12)
			return(HXEMV_PARA);
		sprintf(szBuf, "%012s", pszAmount);
		vTwoOne(szBuf, 12, sBuf);
		// 保存为T9F02 N型
		iRet = iTlvMakeAddObj(gl_sTlvDbTermVar, TAG_9F02_AmountN, 6, sBuf, TLV_CONFLICT_REPLACE);
		if(iRet < 0)
			return(HXEMV_CORE);
	}
	// 其它金额
	if(strlen(pszAmountOther)) {
		if(strlen(pszAmountOther) > 12)
			return(HXEMV_PARA);
		sprintf(szBuf, "%012s", pszAmountOther);
		vTwoOne(szBuf, 12, sBuf);
		// 保存为T9F03 N型
		iRet = iTlvMakeAddObj(gl_sTlvDbTermVar, TAG_9F03_AmountOtherN, 6, sBuf, TLV_CONFLICT_REPLACE);
		if(iRet < 0)
			return(HXEMV_CORE);
	}
	// 货币代码
	if(iCurrencyCode >= 0) {
		sprintf(szBuf, "%04u", iCurrencyCode); // 交易货币代码
		vTwoOne(szBuf, 4, sBuf);
		iRet = iTlvMakeAddObj(gl_sTlvDbTermVar, TAG_5F2A_TransCurrencyCode, 2, sBuf, TLV_CONFLICT_REPLACE);
		if(iRet < 0)
			return(HXEMV_CORE);
	}
	// 交易日期
	if(strlen(pszTransDate) > 0) {
		if(strlen(pszTransDate) != 6)
			return(HXEMV_PARA);
		vTwoOne(pszTransDate, 6, sBuf);
		iRet = iTlvMakeAddObj(gl_sTlvDbTermVar, TAG_9A_TransDate, 3, sBuf, TLV_CONFLICT_REPLACE);
		if(iRet < 0)
			return(HXEMV_CORE);
	}
	// 交易类型
	if(ucTransType != 0xFF) {
		iRet = iTlvMakeAddObj(gl_sTlvDbTermVar, TAG_9C_TransType, 1, &ucTransType, TLV_CONFLICT_REPLACE);
		if(iRet < 0)
			return(HXEMV_CORE);
	}
	// 交易时间
	if(strlen(pszTransTime) > 0) {
		if(strlen(pszTransTime) != 6)
			return(HXEMV_PARA);
		vTwoOne(pszTransTime, 6, sBuf);
		iRet = iTlvMakeAddObj(gl_sTlvDbTermVar, TAG_9F21_TransTime, 3, sBuf, TLV_CONFLICT_REPLACE);
		if(iRet < 0)
			return(HXEMV_CORE);
	}
	// 国家代码
	if(iCountryCode >= 0) {
		sprintf(szBuf, "%04u", iCountryCode); // 国家代码
		vTwoOne(szBuf, 4, sBuf);
		iRet = iTlvMakeAddObj(gl_sTlvDbTermFixed, TAG_9F1A_TermCountryCode, 2, sBuf, TLV_CONFLICT_REPLACE);
		if(iRet < 0)
			return(HXEMV_CORE);
	}
	// 商户名称
	if(strlen(pszNodeName) > 250)
		return(HXEMV_PARA);
	iRet = iTlvMakeAddObj(gl_sTlvDbTermFixed, TAG_9F4E_MerchantName, strlen(pszNodeName), pszNodeName, TLV_CONFLICT_REPLACE);
	if(iRet < 0)
		return(HXEMV_CORE);

	return(HXEMV_OK);
}
