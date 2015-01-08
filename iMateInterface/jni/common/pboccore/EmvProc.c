/**************************************
File name     : EmvProc.c
Function      : Pboc3.0借贷记/EMV2004客户接口
Author        : Yu Jun
First edition : Mar 11th, 2014
Modified      : Mar 28th, 2014
				去除iHxEmvSetAmount(uchar *pszAmount, uchar *pszAmountOther)函数中的pszAmountOther参数
**************************************/
/*
模块详细描述:
	Pboc3.0借贷记/EMV2004客户接口
.	调用Emv核心, 给高层客户包装一层接口
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

// 获取核心信息
// out : pszCoreName    : 核心名字, 不超过40字节
//       pszCoreDesc    : 核心描述, 不超过60字节字符串
//       pszCoreVer     : 核心版本号, 不超过10字节字符串, 如"1.00"
//       pszReleaseDate : 核心发布日期, YYYY/MM/DD
//       pszCustomerDesc: 客户接口说明, 不超过100字节字符串
// ret : HXEMV_OK       : OK
int iHxEmvInfo(uchar *pszCoreName, uchar *pszCoreDesc, uchar *pszCoreVer, uchar *pszReleaseDate, uchar *pszCustomerDesc)
{
    iHxCoreInfo(pszCoreName, pszCoreDesc, pszCoreVer, pszReleaseDate);
	strcpy(pszCustomerDesc, "Pboc3.0DC/eCash/Emv客户接口, 版本[3.10], 发行日期[2014/05/04]");
	return(HXEMV_OK);
}

// 初始化核心
// 初始化核心需要传入IC卡操作相关指令
// in  : pfiTestCard	: 检测卡片是否存在
//                        ret : 0 : 不存在
//                              1 : 存在
//       pfiResetCard   : 卡片复位
//                        ret : <=0 : 复位错误
//                              >0  : 复位成功, 返回值为ATR长度
//       pfiDoApdu      : 执行APDU指令
//                        in  : iApduInLen   : Apdu指令长度
//                              psApduIn     : Apdu指令, 格式: Cla Ins P1 P2 Lc DataIn Le
//                        out : piApduOutLen : Apdu应答长度
//                              psApduOut    : Apdu应答, 格式: DataOut Sw1 Sw2
//                        ret : 0            : 卡操作成功
//                              1            : 卡操作错
//       pfiCloseCard   : 关闭卡片
//                        ret : 不关心
// ret : HXEMV_OK       : OK
//       HXEMV_PARA     : 参数错误
//       HXEMV_CORE     : 内部错误
// Note: 在调用任何其它接口前必须且先初始化核心
int iHxEmvInit(int (*pfiTestCard)(void),
			   int (*pfiResetCard)(uchar *psAtr),
			   int (*pfiDoApdu)(int iApduInLen, unsigned char *psApduIn, int *piApduOutLen, unsigned char *psApduOut),
			   int (*pfiCloseCard)(void))
{
	int  iRet;
	uint uiRet;

	_vPosInit();

	iRet = iHxCoreInit(HXCORE_NOT_CALLBACK, 1/*1:实际应用程序*/, time(NULL), "zh");
	if(iRet)
		return(iRet);

	uiRet = _uiSetCardCtrlFunc(0, // 设为0号卡座
					           pfiTestCard, pfiResetCard, pfiDoApdu, pfiCloseCard);
	if(uiRet)
		return(HXEMV_PARA);

	iHxSetIccSlot(0);
	return(0);
}

// 设置跟踪回调函数
// 注意, 该函数不对外, 仅用于内部测试用
int iHxEmvTraceSet(void (*pfvTraceFunc)(char *))
{
	int iRet;
	iRet = iTraceSet(TRACE_CALLBACK, 1, 1, 1, pfvTraceFunc);
	return(iRet);
}

// 设置终端参数
// in  : pHxTermParam      : 终端参数
// ret : HXEMV_OK          : OK
//       HXEMV_PARA        : 参数错误
//       HXEMV_CORE        : 内部错误
//       HXEMV_FLOW_ERROR  : EMV流程错误
int iHxEmvSetParam(stHxTermParam *pHxTermParam)
{
	stTermParam TermParam;
	uchar szErrTag[100];
	int   iRet;

	memset(&TermParam, 0, sizeof(TermParam));
	TermParam.ucTermType = pHxTermParam->ucTermType;
	memcpy(TermParam.sTermCapability, pHxTermParam->sTermCapability, 3);
	memcpy(TermParam.sAdditionalTermCapability, pHxTermParam->sAdditionalTermCapability, 5);
	TermParam.ucReaderCapability = 2;     // 2:只支持IC卡
	TermParam.ucVoiceReferralSupport = 3; // 3:不支持,缺省decline
	TermParam.ucPinBypassBehavior = pHxTermParam->ucPinBypassBehavior; // 0:每次bypass只表示该次bypass
	memcpy(TermParam.szMerchantId, pHxTermParam->szMerchantId, 15);
	memcpy(TermParam.szTermId, pHxTermParam->szTermId, 8);
	memcpy(TermParam.szMerchantNameLocation, pHxTermParam->szMerchantNameLocation, sizeof(TermParam.szMerchantNameLocation)-1);
	TermParam.iMerchantCategoryCode = pHxTermParam->iMerchantCategoryCode;
	TermParam.iTermCountryCode = pHxTermParam->uiTermCountryCode;
	TermParam.iTermCurrencyCode = 156; // 该参数在GPO时传入, 终端不再需要该参数
	memcpy(TermParam.szAcquirerId, pHxTermParam->szAcquirerId, sizeof(TermParam.szAcquirerId)-1);
	TermParam.ucTransLogSupport = 1;
	TermParam.ucBlackListSupport = 1;
	TermParam.ucSeparateSaleSupport = 1;
	TermParam.ucAppConfirmSupport = pHxTermParam->ucAppConfirmSupport;
	strcpy(TermParam.szLocalLanguage, "zh");
	strcpy(TermParam.szPinpadLanguage, "zhen");
	strcpy(TermParam.szIFDSerialNo, "00000000");

	// 非接参数
	memcpy(TermParam.sTermCtlsCapability, "\x40\x00\x00\x00", 4); // TDFxx, 终端非接能力, T9F66 Mark位, 最终的T9F66值要与该参数做与操作

	// 以下为与AID公用参数
	TermParam.AidCommonPara.cOnlinePinSupport = -1; // 非终端相关参数

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

// 装载终端支持的Aid
// in  : paHxTermAid       : 终端支持的Aid数组
//       iHxTermAidNum     : 终端支持的Aid个数
// ret : HXEMV_OK          : OK
//       HXEMV_LACK_MEMORY : 存储空间不足
//       HXEMV_PARA        : 参数错误
//       HXEMV_FLOW_ERROR  : EMV流程错误
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

		// 以下为与终端公用参数
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
			TermAid.cOnlinePinSupport = 1; // 如果设置为缺省值, 也认为支持

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

// 装载CA公钥
// in  : paHxCaPublicKey   : CA公钥
//       iHxCaPublicKeyNum : CA公钥个数
// ret : HXEMV_OK          : OK
//       HXEMV_LACK_MEMORY : 存储空间不足
//       HXEMV_PARA        : 参数错误
//       HXEMV_FLOW_ERROR  : EMV流程错误
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

// 交易初始化
// in  : iFlag             : =0, 保留以后使用
// ret : HXEMV_OK          : OK
//       HXEMV_FLOW_ERROR  : EMV流程错误
//       HXEMV_CARD_REMOVED: 卡被取走
//       HXEMV_NO_CARD     : 卡片不存在
//       HXEMV_CARD_OP     : 卡片复位错误
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

// 关闭卡片
// ret : HXEMV_OK       : OK
int iHxEmvCloseCard(void)
{
	iHxCloseCard();
	return(HXEMV_OK);
}

// 读取支持的应用
// in  : iIgnoreBlock	   : !0:忽略应用锁定标志, 0:锁定的应用不会被选择
//       piAdfNum          : 可容纳的终端与卡片同时支持的Adf个数
// out : paHxAdfInfo       : 终端与卡片同时支持的Adf列表(以优先级排序)
//       piHxAdfNum        : 终端与卡片同时支持的Adf个数
// ret : HXEMV_OK          : OK, 获取的应用必须确认
//       HXEMV_AUTO_SELECT : OK, 获取的应用可自动选择
//       HXEMV_NO_APP      : 无支持的应用
//       HXEMV_CARD_REMOVED: 卡被取走
//       HXEMV_CARD_SW     : 卡状态字非法
//       HXEMV_CARD_OP     : 卡操作错
//       HXEMV_TERMINATE   : 满足拒绝条件，交易终止
//       HXEMV_FLOW_ERROR  : EMV流程错误
//       HXEMV_LACK_MEMORY : 存储空间不足
//       HXEMV_PARA        : 参数错误
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

// 应用选择
// in  : iIgnoreBlock	     : !0:忽略应用锁定标志, 0:锁定的应用不会被选择
//       iAidLen             : AID长度
//       psAid               : AID
// ret : HXEMV_OK            : OK
//       HXEMV_CARD_REMOVED  : 卡被取走
//       HXEMV_CARD_OP       : 卡操作错
//       HXEMV_TERMINATE     : 满足拒绝条件，交易终止
//       HXEMV_FLOW_ERROR    : EMV流程错误
//       HXEMV_CORE          : 内部错误
//       HXEMV_RESELECT      : 需要重新选择应用
//       HXEMV_NOT_SUPPORTED : 选择了不支持的应用
int iHxEmvAppSelect(int iIgnoreBlock, int iAidLen, uchar *psAid)
{
	int iRet;
	int iErrMsgType;

	iRet = iHxAppSelect2(iIgnoreBlock, iAidLen, psAid, &iErrMsgType);
	return(iRet);
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
//       数据一旦被读出, 会放到Tlv数据库中, 再次使用可以用iHxEmvGetData()读取
int iHxEmvGetCardNativeData(uchar *psTag, int *piOutTlvDataLen, uchar *psOutTlvData, int *piOutDataLen, uchar *psOutData)
{
	int iRet;
	iRet = iHxGetCardNativeData(psTag, piOutTlvDataLen, psOutTlvData, piOutDataLen, psOutData);
	return(iRet);
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
int iHxEmvGetData(uchar *psTag, int *piOutTlvDataLen, uchar *psOutTlvData, int *piOutDataLen, uchar *psOutData)
{
	int iRet;
	iRet = iHxGetData(psTag, piOutTlvDataLen, psOutTlvData, piOutDataLen, psOutData);
	return(iRet);
}

// 读卡片交易流水支持信息
// in  : iFlag             : 0:标准交易流水 1:圈存流水
// out : piMaxRecNum       : 最多交易流水记录个数
// ret : HXEMV_OK          : 读取成功
//       HXEMV_NO_LOG      : 卡片不支持交易流水记录
//       HXEMV_PARA        : 参数错误
//       HXEMV_FLOW_ERROR  : EMV流程错误
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

// 读卡片交易流水
// in  : iFlag             : 0:标准交易流水 1:圈存流水
//       iLogNo            : 交易流水记录号, 最近的一条记录号为1
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
//       HXEMV_PARA        : 参数错误
//       HXEMV_FLOW_ERROR  : EMV流程错误
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
// in  : pszTransTime      : 交易时间，YYYYMMDDhhmmss
//       ulTTC             : 终端交易序号, 1-999999
//       ucTransType       : 交易类型,处理码前2位表示的金融交易类型
//                           比如现金充值63表示为0x63
//       pszAmount         : 交易金额, ascii码串, 不要小数点, 默认为缺省小数点位置, 例如:RMB1.00表示为"100"
//       uiCurrencyCode    : 货币代码, 156=人民币
// ret : HXEMV_OK          : OK
//       HXEMV_CARD_REMOVED: 卡被取走
//       HXEMV_CARD_OP     : 卡操作错
//       HXEMV_CARD_SW     : 非法卡状态字
//       HXEMV_TERMINATE   : 满足拒绝条件，交易终止
//       HXEMV_RESELECT    : 需要重新选择应用
//       HXEMV_TRANS_NOT_ALLOWED : 交易不支持
//       HXEMV_PARA        : 参数错误
//       HXEMV_FLOW_ERROR  : EMV流程错误
//       HXEMV_CORE        : 内部错误
// 非接可能返回码
//       HXEMV_DENIAL	   : qPboc拒绝
//		 HXEMV_TRY_OTHER_INTERFACE	: 尝试其它通信界面
//		 HXEMV_CARD_TRY_AGAIN		: 要求重新提交卡片(非接)
// Note: 如果返回HXEMV_RESELECT，从iHxEmvGetSupportedApp()开始重新执行流程
int iHxEmvGPO(uchar *pszTransTime, ulong ulTTC, uchar ucTransType, uchar *pszAmount, uint uiCurrencyCode)
{
	int iRet;
	int iMsgType, iErrMsgType;

	iRet = iHxGPO2(pszTransTime, ulTTC, (uint)ucTransType, pszAmount, uiCurrencyCode, &iMsgType, &iErrMsgType);
	return(iRet);
}

// 传入或重新传入金额
// in  : pszAmount         : 金额
// ret : HXEMV_OK          : 设置成功
//       HXEMV_FLOW_ERROR  : EMV流程错误
//       HXEMV_PARA        : 参数错误
// note: GPO后GAC金额可能会不同, 重新设定交易金额
//       读记录后, 处理限制前可调用
int iHxEmvSetAmount(uchar *pszAmount)
{
	int iRet;

	iRet = iHxSetAmount(pszAmount, "0");
	return(iRet);
}

// 读应用记录
// ret : HXEMV_OK          : OK
//       HXEMV_CARD_REMOVED: 卡被取走
//       HXEMV_CARD_OP     : 卡操作错
//       HXEMV_CARD_SW     : 非法卡指令状态字
//       HXEMV_TERMINATE   : 满足拒绝条件，交易终止
//       HXEMV_FLOW_ERROR  : EMV流程错误
//       HXEMV_CORE        : 内部错误
// 非接可能返回码
//		 HXEMV_TRY_OTHER_INTERFACE	: 尝试其它通信界面
int iHxEmvReadRecord(void)
{
	int iRet;
	int iErrMsgType;

	iRet = iHxReadRecord2(NULL, NULL, NULL, &iErrMsgType);
	return(iRet);
}

// 设置账户黑名单及分开消费信息
// in  : iBlackFlag        : 设置该卡是否为黑名单卡, 0:不为黑名单卡 1:黑名单卡
//       iSeparateFlag     : 设置该卡累计消费超限额, 0:没超限额 1:超限额
// ret : HXEMV_OK          : OK
//       HXEMV_CORE        : 内部错误
// Note: 如果读记录后发现卡片是黑名单卡或累计消费超限额, 则需要调用此函数设置账户信息
//       如果不支持或检查不许设置标记, 则不需要调用本接口
int iHxEmvSetPanFlag(int iBlackFlag, int iSeparateFlag)
{
	int   iRet;
	uchar szRecentAmount[13];
	int iErrMsgType;
	
	if(iSeparateFlag)
		strcpy(szRecentAmount, "999999999999"); // 通过这种手段使核心设置分开消费超限额
	else
		strcpy(szRecentAmount, "000000000000");
	iRet = iHxSetPanInfo2(iBlackFlag, szRecentAmount, &iErrMsgType);
	return(iRet);
}

// SDA或DDA
// out : piNeedCheckCrlFlag  : 是否需要判断发卡行公钥证书为黑名单标志, 0:不需要判断 1:需要判断
//       psRid               : RID[5], psRid+pucCaIndex+psCertSerNo这三项提供给应用层判断发卡行公钥证书是否在黑名单列表中. 任何一个指针传入为空都表示高层不需要
//       pucCaIndex          : CA公钥索引
//       psCertSerNo         : 发卡行公钥证书序列号[3]
// ret : HXEMV_OK            : OK
//       HXEMV_CARD_REMOVED  : 卡被取走
//       HXEMV_CARD_OP       : 卡操作错
//       HXEMV_CARD_SW       : 非法卡状态字
//       HXEMV_TERMINATE     : 满足拒绝条件，交易终止
//       HXEMV_DENIAL        : 终端行为分析结果为脱机拒绝
//       HXEMV_DENIAL_ADVICE : 终端行为分析结果为脱机拒绝, 有Advice
//       HXEMV_FLOW_ERROR    : EMV流程错误
//       HXEMV_CORE          : 内部错误
// 非接可能返回码
//		 HXEMV_FDDA_FAIL	 : fDDA失败(非接)
// Note: 应用层支持发卡行公钥证书黑名单检查时, piNeedCheckCrlFlag才有意义, 否则可以忽略
//       任意一个指针传入为空表示不做CRL检查
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
		// 高层要求传回CRL检查信息
		*piNeedCheckCrlFlag = iNeedCheckCrlFlag;
		if(iNeedCheckCrlFlag) {
			memcpy(psRid, sRid, 5);
			*pucCaIndex = ucCaIndex;
			memcpy(psCertSerNo, sCertSerNo, 3);
		}
	}
	return(HXEMV_OK);
}

// 设置发卡行公钥证书为黑名单
// in  : iIssuerCertCrlFlag  : 是否在黑名单列表标志, 0:不在黑名单列表 1:在黑名单列表
// ret : HXEMV_OK            : OK
//       HXEMV_CARD_REMOVED  : 卡被取走
//       HXEMV_CARD_OP       : 卡操作错
//       HXEMV_CARD_SW       : 非法卡状态字
//       HXEMV_TERMINATE     : 满足拒绝条件，交易终止
//       HXEMV_DENIAL        : 终端行为分析结果为脱机拒绝
//       HXEMV_DENIAL_ADVICE : 终端行为分析结果为脱机拒绝, 有Advice
//       HXEMV_FLOW_ERROR    : EMV流程错误
//       HXEMV_CORE          : 内部错误
// Note: 应用层调用iHxEmvOfflineDataAuth()后得到Rid+CaIndex+CertSerNo
//       根据此数据判断发卡行公钥证书是否为黑名单,只有是黑名单,才需要调用本函数通知核心
int iHxEmvSetIssuerCertCrl(int iIssuerCertCrlFlag)
{
	int   iRet;
	int   iErrMsgType;

	iRet = HXEMV_OK;
	if(iIssuerCertCrlFlag)
		iRet = iHxSetIssuerCertCrl2(&iErrMsgType);
	return(iRet);
}

// 处理限制
// ret : HXEMV_OK            : OK
//       HXEMV_TERMINATE     : 满足拒绝条件，交易终止
//       HXEMV_FLOW_ERROR    : EMV流程错误
//       HXEMV_CORE          : 内部错误
//       HXEMV_CARD_REMOVED  : 卡被取走
//       HXEMV_DENIAL        : 终端行为分析结果为脱机拒绝
//       HXEMV_DENIAL_ADVICE : 终端行为分析结果为脱机拒绝, 有Advice
//       HXEMV_CARD_OP       : 终端行为分析结果为脱机拒绝, 申请AAC时卡片出错
int iHxEmvProcRistrictions(void)
{
	int   iRet;
	int   iErrMsgType;

	iRet = iHxProcRistrictions2(&iErrMsgType);
	return(iRet);
}

// 终端风险管理
// ret : HXEMV_OK            : OK
//       HXEMV_TERMINATE     : 满足拒绝条件，交易终止
//       HXEMV_CORE          : 其它错误
//       HXEMV_CARD_REMOVED  : 卡被取走
//       HXEMV_CARD_OP       : 卡操作错
//       HXEMV_DENIAL        : 终端行为分析结果为脱机拒绝
//       HXEMV_DENIAL_ADVICE : 终端行为分析结果为脱机拒绝, 有Advice
//       HXEMV_FLOW_ERROR    : EMV流程错误
int iHxEmvTermRiskManage(void)
{
	int   iRet;
	int   iErrMsgType;

	iRet = iHxTermRiskManage2(&iErrMsgType);
	return(iRet);
}

// 获取持卡人验证
// out : piCvm               : 持卡人认证方法, 支持HXCVM_PLAIN_PIN、HXCVM_CIPHERED_ONLINE_PIN、HXCVM_HOLDER_ID、HXCVM_CONFIRM_AMOUNT
//       piBypassFlag        : 允许bypass标志, 0:不允许, 1:允许
// ret : HXEMV_OK            : OK
//       HXEMV_NO_DATA       : 无需继续进行持卡人验证
//       HXEMV_CARD_REMOVED  : 卡被取走
//       HXEMV_CARD_OP       : 卡操作错
//       HXEMV_TERMINATE     : 满足拒绝条件，交易终止
//       HXEMV_DENIAL        : 终端行为分析结果为脱机拒绝
//       HXEMV_DENIAL_ADVICE : 终端行为分析结果为脱机拒绝, 有Advice
//       HXEMV_FLOW_ERROR    : EMV流程错误
//       HXEMV_CORE          : 内部错误
// Note: 与iHxEmvDoCvmMethod()搭配, 可能需要多次调用, 直到返回HXEMV_NO_DATA
int iHxEmvGetCvmMethod(int *piCvm, int *piBypassFlag)
{
	int   iRet;
	int   iMsgType, iMsgType2, iErrMsgType;
	uchar szAmountStr[50];

	iRet = iHxGetCvmMethod2(piCvm, piBypassFlag, &iMsgType, &iMsgType2, szAmountStr, &iErrMsgType);
	return(iRet);
}

// 执行持卡人验证
// in  : iCvmProc            : 认证方法处理方式, HXCVM_PROC_OK or HXCVM_BYPASS or HXCVM_FAIL or HXCVM_CANCEL or HXCVM_TIMEOUT
//       psCvmData           : 输入的密码, 如果为明文密码, 密码尾部要补0
// out : piPrompt            : 额外提示信息, 0:无额外提示信息 1:密码错,可重试 2:密码错,密码已锁 3:脱机密码验证成功
// ret : HXEMV_OK            : OK, 需要继续进行持卡人验证, 继续调用iHxGetCvmMethod2(), 然后再调用本函数
//       HXEMV_PARA		     : 参数错误
//       HXEMV_CARD_REMOVED  : 卡被取走
//       HXEMV_CARD_OP       : 卡操作错
//       HXEMV_CARD_SW       : 非法卡状态字
//       HXEMV_TERMINATE     : 满足拒绝条件，交易终止
//       HXEMV_DENIAL        : 终端行为分析结果为脱机拒绝
//       HXEMV_DENIAL_ADVICE : 终端行为分析结果为脱机拒绝, 有Advice
//       HXEMV_FLOW_ERROR    : EMV流程错误
//       HXEMV_CORE          : 内部错误
// Note: 执行的CVM必须是最近一次iHxEmvGetCvmMethod()获得的CVM
int iHxEmvDoCvmMethod(int iCvmProc, uchar *psCvmData, int *piPrompt)
{
	int   iRet;
	int   iMsgType, iMsgType2, iErrMsgType;

	*piPrompt = 0; // 假设没有额外信息提示
	iRet = iHxDoCvmMethod2(iCvmProc, psCvmData, &iMsgType, &iMsgType2, &iErrMsgType);
	if(iMsgType == EMVMSG_0A_INCORRECT_PIN) {
		// 密码错, 需要额外提示
		if(iMsgType2 == EMVMSG_13_TRY_AGAIN)
			*piPrompt = 1; // 密码错, 可重试
		else
			*piPrompt = 2; // 密码错, 密码已锁
	} else if(iMsgType == EMVMSG_0D_PIN_OK) {
		*piPrompt = 3;
	}
	return(iRet);
}

// 获取CVM认证方法签字标志
// out : piNeedSignFlag    : 表示需要签字标志，0:不需要签字 1:需要签字
// ret : HXEMV_OK          : OK
int iHxEmvGetCvmSignFlag(int *piNeedSignFlag)
{
	int iRet;

	iRet = iHxGetCvmSignFlag2(piNeedSignFlag);
	return(iRet);
}

// 终端行为分析
// ret : HXEMV_OK            : OK
//       HXEMV_CORE          : 其它错误
//       HXEMV_FLOW_ERROR    : EMV流程错误
//       HXEMV_CARD_REMOVED  : 卡被取走
//       HXEMV_DENIAL        : 终端行为分析结果为脱机拒绝
//       HXEMV_DENIAL_ADVICE : 终端行为分析结果为脱机拒绝, 有Advice
int iHxEmvTermActionAnalysis(void)
{
	int iRet;
	int iErrMsgType;

	iRet = iHxTermActionAnalysis2(&iErrMsgType);
	return(iRet);
}

// 第一次GAC
// in  : ucForcedOnline    : 1:设定强制联机标志 0:不设定强制联机标志
// out : piCardAction      : 卡片执行结果
//								GAC_ACTION_TC     : 批准(生成TC)
//								GAC_ACTION_AAC    : 拒绝(生成AAC)
//								GAC_ACTION_AAC_ADVICE : 拒绝(生成AAC,有Advice)
//								GAC_ACTION_ARQC   : 要求联机(生成ARQC)
// ret : HXEMV_OK          : OK
//       HXEMV_CORE        : 其它错误
//       HXEMV_CARD_REMOVED: 卡被取走
//       HXEMV_CARD_OP     : 卡操作错
//       HXEMV_CARD_SW     : 非法状态字
//       HXEMV_TERMINATE   : 满足拒绝条件，交易终止
//       HXEMV_NOT_ALLOWED : 服务不允许
//       HXEMV_NOT_ACCEPTED: 不接受
//       HXEMV_FLOW_ERROR  : EMV流程错误
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

// 第二次GAC
// in  : pszArc            : 应答码[2], NULL或""表示联机失败
//       pszAuthCode	   : 授权码[6], NULL或""表示无授权码数据
//       psOnlineData      : 联机响应数据(55域内容),一组TLV格式数据, NULL表示无联机响应数据
//       psOnlineDataLen   : 联机响应数据长度
// out : piCardAction      : 卡片执行结果
//                           GAC_ACTION_TC     : 批准(生成TC)
//							     GAC_ACTION_AAC    : 拒绝(生成AAC)
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
int iHxEmvGac2 (uchar *pszArc, uchar *pszAuthCode, uchar *psIssuerData, int iIssuerDataLen, int *piCardAction)
{
	int iRet;
	int iMsgType, iErrMsgType;

	iRet = iHx2ndGAC2(pszArc, pszAuthCode, psIssuerData, iIssuerDataLen, piCardAction, &iMsgType, &iErrMsgType);
	return(iRet);
}
