/**************************************
File name     : JsbPboc.c
Function      : 江苏银行Pboc卡接口
Author        : Yu Jun
First edition : Apr 15th, 2014
Note          : 实现江苏银行提供的接口
Modified      :
**************************************/
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include "VposFace.h"
#include "EmvProc.h"
#include "JsbPboc.h"

//extern void vSetWriteLog(int iOnOff);
//extern void vWriteLogHex(char *pszTitle, void *pLog, int iLength);
//extern void vWriteLogTxt(char *pszFormat, ...);

/*******************************************************************************
接口说明:
. 公共参数
  每个函数的前4个参数为公共参数, 它们是char *pszComNo, int nTermType, char cBpNo, int nIcFlag, 在这里统一描述, 以后每个具体函数不再描述.
    pszComNo:	通讯参数, 华信接口不需要
    nTermType:	终端类型, 华信接口不需要
    cBpNo:		BP盒的转口, 华信接口不需要
    nIcFlag:		卡类型, 1:接触IC卡 2:非接触IC卡 3:自动判断
				只有函数ICC_GetIcInfo()和ICC_GetTranDetail()需要
				其它函数会沿用调用ICC_GetIcInfo()函数时确定的卡类型
*******************************************************************************/

int   sg_iRet;
static uchar sg_szAppData[4096];

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
// ret : 0				: 成功
//       <0             : 失败
// Note: 在调用任何其它接口前必须且先初始化核心
int ICC_InitEnv(int (*pfiTestCard)(void),
			    int (*pfiResetCard)(unsigned char *psAtr),
			    int (*pfiDoApdu)(int iApduInLen, unsigned char *psApduIn, int *piApduOutLen, unsigned char *psApduOut),
			    int (*pfiCloseCard)(void))
{
	static int iInitFlag = 0;
	stHxTermParam HxTermParam;
	stHxTermAid   aHxTermAid[5];

	if(iInitFlag)
		return(0);

	_vPosInit();

	sg_iRet = iHxEmvInit(pfiTestCard, pfiResetCard, pfiDoApdu, pfiCloseCard);
	if(sg_iRet)
		return(JSBPBOC_CORE_INIT);

	// 设置终端参数
	memset(&HxTermParam, 0, sizeof(HxTermParam));
	HxTermParam.ucTermType = 0x11;
	memcpy(HxTermParam.sTermCapability, "\xA0\x00\x00", 3);
	memcpy(HxTermParam.sAdditionalTermCapability, "\xEF\x80\xF0\xF0\x00", 5);
	strcpy(HxTermParam.szMerchantId, "123456789000001");
	strcpy(HxTermParam.szTermId, "12345601");
	HxTermParam.uiTermCountryCode = 156;
	strcpy(HxTermParam.szAcquirerId, "000000");
	HxTermParam.iMerchantCategoryCode = -1;
	HxTermParam.ucPinBypassBehavior = 0;
	HxTermParam.ucAppConfirmSupport = 1;

	memcpy(HxTermParam.AidCommonPara.sTermAppVer, "\x00\x20", 2);
	HxTermParam.AidCommonPara.ulFloorLimit = 0xFFFFFFFEL;
	HxTermParam.AidCommonPara.iMaxTargetPercentage = -1;
	HxTermParam.AidCommonPara.iTargetPercentage = -1;
	HxTermParam.AidCommonPara.ulThresholdValue = 0xFFFFFFFFL;
	HxTermParam.AidCommonPara.ucECashSupport = 0;
	strcpy(HxTermParam.AidCommonPara.szTermECashTransLimit, "999999999999");
	HxTermParam.AidCommonPara.ucTacDefaultExistFlag = 1;
	memcpy(HxTermParam.AidCommonPara.sTacDefault, "\xFF\xFF\xFF\xFF\xFF", 5);
	HxTermParam.AidCommonPara.ucTacDenialExistFlag = 1;
	memcpy(HxTermParam.AidCommonPara.sTacDenial, "\x00\x00\x00\x00\x00", 5);
	HxTermParam.AidCommonPara.ucTacOnlineExistFlag = 1;
	memcpy(HxTermParam.AidCommonPara.sTacOnline, "\xFF\xFF\xFF\xFF\xFF", 5);
	HxTermParam.AidCommonPara.iDefaultDDOLLen = 3;
	memcpy(HxTermParam.AidCommonPara.sDefaultDDOL, "\x9F\x37\x04", 3);
	HxTermParam.AidCommonPara.iDefaultTDOLLen = -1;
	sg_iRet = iHxEmvSetParam(&HxTermParam);
	if(sg_iRet)
		return(JSBPBOC_CORE_INIT);

	/*
	 * 	unsigned char sTermAppVer[2];               // T9F09, 终端应用版本号, "\xFF\xFF"表示不存在
	unsigned long ulFloorLimit;					// T9F1B, 终端限额, 单位为分, 0xFFFFFFFF表示不存在
    int			  iMaxTargetPercentage;         // 随机选择最大百分比，-1:不存在
    int			  iTargetPercentage;            // 随机选择目标百分比，-1:不存在
	unsigned long ulThresholdValue;             // 随机选择阈值, 0xFFFFFFFF表示不存在
    unsigned char ucECashSupport;				// 1:支持电子现金 0:不支持电子现金, 0xFF表示不存在
	unsigned char szTermECashTransLimit[12+1];  // T9F7B, 终端电子现金交易限额, 空表示不存在
	unsigned char ucTacDefaultExistFlag;        // 1:TacDefault存在, 0:TacDefault不存在
    unsigned char sTacDefault[5];				// TAC-Default, 参考TVR结构
	unsigned char ucTacDenialExistFlag;         // 1:TacDenial存在, 0:TacDenial不存在
    unsigned char sTacDenial[5];				// TAC-Denial, 参考TVR结构
	unsigned char ucTacOnlineExistFlag;         // 1:TacOnline存在, 0:TacOnline不存在
    unsigned char sTacOnline[5];				// TAC-Online, 参考TVR结构
    int           iDefaultDDOLLen;              // Default DDOL长度,-1表示无
    unsigned char sDefaultDDOL[252];            // Default DDOL(TAG_DFXX_DefaultDDOL)
    int           iDefaultTDOLLen;              // Default TDOL长度,-1表示无
    unsigned char sDefaultTDOL[252];            // Default TDOL(TAG_DFXX_DefaultTDOL)
	 */

	memset(&aHxTermAid[0], 0, sizeof(aHxTermAid));
	aHxTermAid[0].ucAidLen = 8;
	memcpy(aHxTermAid[0].sAid, "\xA0\x00\x00\x03\x33\x01\x01\x01", 8);
	aHxTermAid[0].ucASI = 1;
	aHxTermAid[0].cOnlinePinSupport = 1;
	memcpy(aHxTermAid[0].sTermAppVer, "\xFF\xFF", 2);
	aHxTermAid[0].ulFloorLimit = 0xFFFFFFFFL;
	aHxTermAid[0].iMaxTargetPercentage = -1;
	aHxTermAid[0].iTargetPercentage = -1;
	aHxTermAid[0].ulThresholdValue = 0xFFFFFFFFL;
	aHxTermAid[0].ucECashSupport = 0xFF;
	aHxTermAid[0].iDefaultDDOLLen = -1;
	aHxTermAid[0].iDefaultTDOLLen = -1;
	memcpy(&aHxTermAid[1], &aHxTermAid[0], sizeof(stHxTermAid));
	aHxTermAid[1].ucAidLen = 8;
	memcpy(aHxTermAid[0].sAid, "\xA0\x00\x00\x03\x33\x01\x01\x02", 8);
	memcpy(&aHxTermAid[2], &aHxTermAid[0], sizeof(stHxTermAid));
	aHxTermAid[2].ucAidLen = 8;
	memcpy(aHxTermAid[0].sAid, "\xA0\x00\x00\x03\x33\x01\x01\x03", 8);
	sg_iRet = iHxEmvLoadAid(&aHxTermAid[0], 3);
	if(sg_iRet)
		return(JSBPBOC_CORE_INIT);

	sg_szAppData[0] = 0;
	iInitFlag = 1;

	return(JSBPBOC_OK);
}

// 整理字符串
// in  : iFlag   : 标志, 0:去除头部'0' 1:去除尾部'F'
//     : pszData : 待整理字符串
// out : pszData : 整理好的字符串
static int iReformString(int iFlag, uchar *pszData)
{
	int   iLen, i;
	uchar szData[256];

	iLen = strlen((char *)pszData);
	switch(iFlag) {
	case 0: // 去除头部'0'
		for(i=0; i<iLen; i++)
			if(pszData[i] != '0')
				break;
		if(i >= iLen) {
			strcpy((char *)pszData, "0"); // 全是'0'
			break;
		}
		strcpy((char *)szData, (char *)pszData+i);
		strcpy((char *)pszData, (char *)szData);
		break;
	case 1: // 去除尾部'F'
		for(i=iLen-1; i>=0; i--) {
			if(pszData[i]!='F' && pszData[i]!='f')
				break;
			pszData[i] = 0;
		}
		break;
	default:
		break; // 不整理
	}
	return(strlen((char *)pszData));
}

// 读卡信息
// In  : pszTagList   : 表示需要返回的用户数据的标签列表
// Out : pnUsrInfoLen :	返回的用户数据长度
//       pszUserInfo  : 返回的用户数据, 缓冲区至少保留1024字节
//       pszAppData   :	IC卡数据, 包括Tag8C、Tag8D等, 用于生成55域, 缓冲区至少保留4096字节
//       pnIcType     : 上电成功的卡类型, 1:接触卡 2:非接卡
//						用于函数ICC_GenARQC()和ICC_CtlScriptData()传入
// Ret : 0            : 成功
//       <0           : 失败
int ICC_GetIcInfo(char *pszComNo, int nTermType, char cBpNo, int nIcFlag,
						char *pszTaglist,
						int *pnUsrInfoLen, char *pszUserInfo, char *pszAppData, int *pnIcType)
{
	int i;

	if(nIcFlag!=1 && nIcFlag!=3)
		return(JSBPBOC_CARD_TYPE);

	sg_iRet = iHxEmvTransInit(0);
	if(sg_iRet)
		return(JSBPBOC_TRANS_INIT);

	sg_szAppData[0] = 0;

	for(;;) {
		stHxAdfInfo aHxAdfInfo[5];
		int iHxAdfNum;
		iHxAdfNum = sizeof(aHxAdfInfo) / sizeof(aHxAdfInfo[0]);
		memset(&aHxAdfInfo[0], 0, sizeof(aHxAdfInfo));
		sg_iRet = iHxEmvGetSupportedApp(0/*应用锁定时不能选*/, &aHxAdfInfo[0], &iHxAdfNum);
		if(sg_iRet == HXEMV_NO_APP)
			return(JSBPBOC_NO_APP);
		if(sg_iRet == HXEMV_CARD_SW || sg_iRet == HXEMV_CARD_OP || sg_iRet == HXEMV_CARD_REMOVED)
			return(JSBPBOC_CARD_IO);
		if(sg_iRet && sg_iRet!=HXEMV_AUTO_SELECT)
			return(JSBPBOC_UNKNOWN);

		sg_iRet = iHxEmvAppSelect(0/*应用锁定时不能选*/, aHxAdfInfo[0].ucAdfNameLen, aHxAdfInfo[0].sAdfName);
		if(sg_iRet == HXEMV_CARD_SW || sg_iRet == HXEMV_CARD_OP || sg_iRet == HXEMV_CARD_REMOVED)
			return(JSBPBOC_CARD_IO);
		if(sg_iRet == HXEMV_RESELECT)
			continue;
		if(sg_iRet)
			return(JSBPBOC_UNKNOWN);

		sg_iRet = iHxEmvGPO("20140415000000", 1, 0x00, "0", 156);
		if(sg_iRet == HXEMV_CARD_SW || sg_iRet == HXEMV_CARD_OP || sg_iRet == HXEMV_CARD_REMOVED)
			return(JSBPBOC_CARD_IO);
		if(sg_iRet == HXEMV_RESELECT)
			continue;
		if(sg_iRet)
			return(JSBPBOC_UNKNOWN);
		break;
	} // for(;;

	sg_iRet = iHxEmvReadRecord();
	if(sg_iRet == HXEMV_CARD_SW || sg_iRet == HXEMV_CARD_OP || sg_iRet == HXEMV_CARD_REMOVED)
		return(JSBPBOC_CARD_IO);
	if(sg_iRet)
		return(JSBPBOC_UNKNOWN);

	sg_iRet = iHxEmvOfflineDataAuth(NULL, NULL, NULL, NULL);
	if(sg_iRet)
		return(JSBPBOC_UNKNOWN);

	sg_iRet = iHxEmvProcRistrictions();
	if(sg_iRet)
		return(JSBPBOC_UNKNOWN);

	sg_iRet = iHxEmvTermRiskManage();
	if(sg_iRet)
		return(JSBPBOC_UNKNOWN);

	sg_iRet = iHxEmvTermActionAnalysis();
	if(sg_iRet)
		return(JSBPBOC_UNKNOWN);

	// 流程走完, 组织接口数据
	*pnIcType = 1; // 由于接口只支持接触卡, 一定走的是接触通道
	*pnUsrInfoLen = 0;
	for(i=0; i<(int)strlen(pszTaglist); i++) {
		uchar sData[128], szData[256];
		int   iDataLen;
		switch(pszTaglist[i]) {
		case 'A': // 应用主账号
			iDataLen = sizeof(szData);
			sg_iRet = iHxEmvGetData("\x5A", NULL, NULL, &iDataLen, szData);
			if(sg_iRet)
				return(JSBPBOC_DATA_LOSS);
			break;
		case 'B': // 磁条2 等效数据
			iDataLen = sizeof(sData);
			sg_iRet = iHxEmvGetData("\x57", NULL, NULL, &iDataLen, sData);
			if(sg_iRet) {
				szData[0] = 0; // 磁道2等效数据不是必备数据
			}
			vOneTwoX0(sData, iDataLen, szData);
			iReformString(1/*1:去除尾部部'F'*/, szData);
			break;
		case 'C': // 电子现金余额
			iDataLen = sizeof(szData);
			sg_iRet = iHxEmvGetCardNativeData("\x9F\x79", NULL, NULL, &iDataLen, szData);
			if(sg_iRet == HXEMV_CARD_SW || sg_iRet == HXEMV_CARD_OP || sg_iRet == HXEMV_CARD_REMOVED)
				return(JSBPBOC_CARD_IO);
			if(sg_iRet) {
				szData[0] = 0;
			} else {
				iReformString(0/*0:去除头部'0'*/, szData);
			}
			break;
		case 'D': // 电子现金余额上限
			iDataLen = sizeof(szData);
			sg_iRet = iHxEmvGetCardNativeData("\x9F\x77", NULL, NULL, &iDataLen, szData);
			if(sg_iRet == HXEMV_CARD_SW || sg_iRet == HXEMV_CARD_OP || sg_iRet == HXEMV_CARD_REMOVED)
				return(JSBPBOC_CARD_IO);
			if(sg_iRet) {
				szData[0] = 0;
			} else {
				iReformString(0/*0:去除头部'0'*/, szData);
			}
			break;
		case 'E': // 应用失效日期
			iDataLen = sizeof(szData);
			sg_iRet = iHxEmvGetData("\x5F\x24", NULL, NULL, &iDataLen, szData);
			if(sg_iRet)
				return(JSBPBOC_DATA_LOSS);
			break;
		case 'F': // 电子现金单笔交易限额
			iDataLen = sizeof(szData);
			sg_iRet = iHxEmvGetCardNativeData("\x9F\x78", NULL, NULL, &iDataLen, szData);
			if(sg_iRet == HXEMV_CARD_SW || sg_iRet == HXEMV_CARD_OP || sg_iRet == HXEMV_CARD_REMOVED)
				return(JSBPBOC_CARD_IO);
			if(sg_iRet) {
				szData[0] = 0;
			} else {
				iReformString(0/*0:去除头部'0'*/, szData);
			}
			break;
		case 'G': // 应用主账号序列号
			iDataLen = sizeof(szData);
			sg_iRet = iHxEmvGetData("\x5F\x34", NULL, NULL, &iDataLen, szData);
			if(sg_iRet) {
				szData[0] = 0;
			} else {
				iReformString(0/*0:去除头部'0'*/, szData);
			}
			break;
		case 'H': // 应用交易计数器
			iDataLen = sizeof(sData);
			sg_iRet = iHxEmvGetCardNativeData("\x9F\x36", NULL, NULL, &iDataLen, sData);
			if(sg_iRet == HXEMV_CARD_SW || sg_iRet == HXEMV_CARD_OP || sg_iRet == HXEMV_CARD_REMOVED)
				return(JSBPBOC_CARD_IO);
			if(sg_iRet)
				return(JSBPBOC_DATA_LOSS);
			vOneTwo0(sData, 2, szData);
			break;
		case 'I': // 持卡人证件类型
			iDataLen = sizeof(sData);
			sg_iRet = iHxEmvGetData("\x9F\x62", NULL, NULL, &iDataLen, sData);
			if(sg_iRet) {
				szData[0] = 0;
			} else {
				sprintf(szData, "%d", sData[0]);
			}
			break;
		case 'J': // 持卡人证件号
			iDataLen = sizeof(szData);
			sg_iRet = iHxEmvGetData("\x9F\x61", NULL, NULL, &iDataLen, szData);
			if(sg_iRet) {
				szData[0] = 0;
			}
			break;
		case 'K': // 持卡人姓名
			iDataLen = sizeof(szData);
			sg_iRet = iHxEmvGetData("\x5F\x20", NULL, NULL, &iDataLen, szData);
			if(sg_iRet) {
				szData[0] = 0;
			}
			break;
		case 'L': // 持卡人姓名扩展
			iDataLen = sizeof(szData);
			sg_iRet = iHxEmvGetData("\x9F\x0B", NULL, NULL, &iDataLen, szData);
			if(sg_iRet) {
				szData[0] = 0;
			}
			break;
		default:
			return(JSBPBOC_TAG_UNKNOWN);
		}
		iDataLen = strlen((char *)szData);
		pszUserInfo[(*pnUsrInfoLen)++] = pszTaglist[i];
		sprintf(&pszUserInfo[*pnUsrInfoLen], "%03d", iDataLen);
		(*pnUsrInfoLen) += 3;
		memcpy(&pszUserInfo[*pnUsrInfoLen], szData, iDataLen);
		(*pnUsrInfoLen) += iDataLen;
	} // for(i=0; i<strlen(pszTaglist); i++) {

	{
		static unsigned short auiTag[] = {
			0x4F00, 0x5000, 0x5A00, 0x5F20, 0x5F24, 0x5F25, 0x5F28, 0x5F30,
			0x5F34, 0x5F53, 0x5F54, 0x8200, 0x8C00, 0x8D00, 0x8E00, 0x9F05,
			0x9F07, 0x9F08, 0x9F0B, 0x9F0D, 0x9F0E, 0x9F0F, 0x9F12, 0x9F13,
			0x9F14, 0x9F23, 0x9F36, 0x9F38, 0x9F42, 0x9F4D, 0x9F4F, 0x9F61,
			0x9F62, 0x9F63, 0x9F6D, 0x9F74, 0
		};
		uchar sOutBuf[2048];
		int   iOutLen;
		iOutLen = 0;
		for(i=0; ; i++) {
			uchar sTag[2], sTlvObj[270];
			int   iTlvObjLen;
			int   iRet;
			if(auiTag[i] == 0)
				break;
			vLongToStr((ulong)auiTag[i], 2, sTag);
			iTlvObjLen = sizeof(sTlvObj);
			iRet = iHxEmvGetData(sTag, &iTlvObjLen, sTlvObj, NULL, NULL);
			if(iRet) {
				// 没有读出数据, 判断是否为卡片内部数据
				if(auiTag[i]==0x9F13 || auiTag[i]==0x9F36 || auiTag[i]==0x9F4F || auiTag[i]==0x9F6D) {
					// 卡片内部数据
					iTlvObjLen = sizeof(sTlvObj);
					iRet = iHxEmvGetCardNativeData(sTag, &iTlvObjLen, sTlvObj, NULL, NULL);
				}
			}
			if(iRet)
				continue;
			memcpy(sOutBuf+iOutLen, sTlvObj, iTlvObjLen);
			iOutLen += iTlvObjLen;
		}
		vOneTwo0(sOutBuf, iOutLen, pszAppData);
		strcpy(sg_szAppData, pszAppData);
	}

	return(JSBPBOC_OK);
}

// 获取Pboc应用版本, 必须在ICC_GetIcInfo之后执行
// Out : pszPbocVer   :	返回的PBOC版本号，"0002"标识PBOC2.0，"0003"标识PBOC3.0， 缓冲区大小 >= 5bytes
// Ret : 0            : 成功
//       !=0          : 失败
int ICC_GetIcPbocVersion(char *pszPbocVer)
{
    uchar sOutData[10];
    int iOutDataLen, retCode;

    iOutDataLen = sizeof(sOutData);
    retCode = iHxEmvGetData("\x9f\x08", NULL, NULL, &iOutDataLen, sOutData);
    if (retCode)
        return retCode;

    vOneTwo0(sOutData, 2, pszPbocVer);

    return 0;
}


// 为江苏银行定制的TLV数据库替换函数
// 在EmvCore.c模块中
extern int iHxReplaceTlvDb_Jsb(uchar *pszAmount, uchar *pszAmountOther, int iCurrencyCode,
						uchar *pszTransDate, uchar ucTransType, uchar *pszTransTime, int iCountryCode, uchar *pszNodeName);
static int iReplaceCoreData(char *pszTxData)
{
	uchar szAmount[13], szAmountOther[13], szTransDate[7], ucTransType, szTransTime[7], szNodeName[256];
	int   iCurrencyCode, iCountryCode;

	szAmount[0] = 0;
	szAmountOther[0] = 0;
	szTransDate[0] = 0;
	ucTransType = 0xFF;
	szTransTime[0] = 0;
	szNodeName[0] = 0;
	iCurrencyCode = -1;
	iCountryCode = -1;
	while(*pszTxData) {
		int   iTxDataLen;
		uchar *p;
		iTxDataLen = ulA2L(pszTxData+1, 3);
		p = pszTxData;
		pszTxData += 4 + iTxDataLen;
		switch(p[0]) {
		case 'P': // 授权金额
			if(iTxDataLen > 12)
				return(JSBPBOC_PARAM);
			vMemcpy0(szAmount, p+4, iTxDataLen);
			break;
		case 'Q': // 其它金额
			if(iTxDataLen > 12)
				return(JSBPBOC_PARAM);
			vMemcpy0(szAmountOther, p+4, iTxDataLen);
			break;
		case 'R': // 货币代码
			iCurrencyCode = ulA2L(p+4, iTxDataLen);
			break;
		case 'S': // 交易日期 YYMMDD
			if(iTxDataLen != 6)
				return(JSBPBOC_PARAM);
			vMemcpy0(szTransDate, p+4, iTxDataLen);
			break;
		case 'T': // 交易类型
			if(iTxDataLen == 0)
				break;
			if(iTxDataLen > 2)
				return(JSBPBOC_PARAM);
			if(iTxDataLen == 1)
				ucTransType = p[4] - 0x30;
			else
				vTwoOne(p+4, 2, &ucTransType);
			break;
		case 'U': // 交易时间 hhmmss
			if(iTxDataLen != 6)
				return(JSBPBOC_PARAM);
			vMemcpy0(szTransTime, p+4, iTxDataLen);
			break;
		case 'V': // 国家代码
			iCountryCode = ulA2L(p+4, iTxDataLen);
			break;
		case 'W': // 商户名称
			if(iTxDataLen > 250)
				return(JSBPBOC_PARAM);
			vMemcpy0(szNodeName, p+4, iTxDataLen);
			break;
		default:
			return(JSBPBOC_TAG_UNKNOWN);
		} // switch(*pszTxData) {
	} // while(*pszTxData

	sg_iRet = iHxReplaceTlvDb_Jsb(szAmount, szAmountOther, iCurrencyCode,
						       szTransDate, ucTransType, szTransTime, iCountryCode, szNodeName);
	if(sg_iRet)
		return(JSBPBOC_PARAM);

	return(0);
}

// 生成输出数据
static int iGenOutData(uchar *psOutData)
{
	static unsigned short auiTag[] = {
		0x9F26, // 应用密文 （8字节）。
		0x9F27, // 密文信息数据 （1字节）
		0x9F10, // 发卡行应用数据 （最多32字节）：
		0x9F37, // 随机数(4字节)
		0x9F36, // ATC （2字节）
		0x9500, // 终端验证结果 （5字节）
		0x9A00, // 交易日期（3字节)
		0x9C00, // 交易类型 （1字节）
		0x9F02, // 授权金额 （6字节）
		0x5F2A, // 交易货币代码 （2字节）
		0x8200, // 应用交互特征 （2字节）
		0x9F1A, // 终端国家代码 （2字节）
		0x9F03, // 其它金额 （6字节）
		0x9F33, // 终端性能 （3字节）
		0x9F13, // 上次联机应用交易计数器寄存器 （1字节）
		0x5A00, // 应用主账号（PAN） （最多10字节）
		0x5F34, // 应用PAN序列号 （1字节）

		0xDF31,
		0
	};
	int   i;
	int   iOutLen;
	iOutLen = 0;
	for(i=0; ; i++) {
		uchar sTag[2], sTlvObj[270];
		int   iTlvObjLen;
		int   iRet;
		if(auiTag[i] == 0)
			break;
		vLongToStr((ulong)auiTag[i], 2, sTag);
		iTlvObjLen = sizeof(sTlvObj);
		iRet = iHxEmvGetData(sTag, &iTlvObjLen, sTlvObj, NULL, NULL);
		if(iRet)
			continue;
		memcpy(psOutData+iOutLen, sTlvObj, iTlvObjLen);
		iOutLen += iTlvObjLen;
	}
	return(iOutLen);
}

// 生成ARQC
// In  : pszTxData    : 交易数据, 标签加长度内容格式
//       pszAppData   : ICC_GetIcInfo函数返回的应用数据, 必须保持与原数据相同
// Out : pnARQCLen    : 返回的ARQC长度
//       pszARQC      : 返回的ARQC, TLV格式, 转换为16进制可读形式
// Ret : 0            : 成功
//       <0           : 失败
int ICC_GenARQC(char *pszComNo, int nTermType, char cBpNo, int nIcFlag,
						char *pszTxData, char *pszAppData, int *pnARQCLen, char *pszARQC)
{
	uchar sOutData[2048];
	int   iOutLen;
	int   iCardAction;
	int   iRet;

	if(nIcFlag!=1 && nIcFlag!=2)
		return(JSBPBOC_CARD_TYPE);
	/*
	if(strcmp(pszAppData, sg_szAppData) != 0) {
		vSetWriteLog(1);
		vWriteLogTxt("ICC_GenARQC ret = %d", JSBPBOC_APP_DATA);
		vWriteLogTxt("pszAppData(%d)   = [%s]", strlen(pszAppData), pszAppData);
		vWriteLogTxt("sg_szAppData(%d) = [%s]", strlen(sg_szAppData), sg_szAppData);
		vSetWriteLog(0);
		return(JSBPBOC_APP_DATA);
	}
	*/

	iRet = iReplaceCoreData(pszTxData);
	if(iRet)
		return(iRet);

	// GAC1
	sg_iRet = iHxEmvGac1(1/*1:强制联机*/ , &iCardAction);
	if(sg_iRet == HXEMV_CARD_SW || sg_iRet == HXEMV_CARD_OP || sg_iRet == HXEMV_CARD_REMOVED)
		return(JSBPBOC_CARD_IO);
	if(sg_iRet)
		return(JSBPBOC_UNKNOWN);
	if(iCardAction != GAC_ACTION_ARQC)
		return(JSBPBOC_GEN_ARQC);

	// 产生输出数据
	iOutLen = iGenOutData(sOutData);
	vOneTwo0(sOutData, iOutLen, pszARQC);
	*pnARQCLen = iOutLen * 2;

	return(0);
}

// 交易结束处理
// In  : pszTxData    : 交易数据, 标签加长度内容格式
//       pszAppData   : ICC_GetIcInfo函数返回的应用数据, 必须保持与原数据相同
//       pszARPC      : 发卡行数据, TLV格式, 转换为16进制可读形式
// Out : pnTCLen      : 返回的TC长度
//       pszTC        : 返回的TC, TLV格式, 转换为16进制可读形式
// Ret : 0            : 成功
//       <0           : 失败
int ICC_CtlScriptData(char *pszComNo, int nTermType, char cBpNo, int nIcFlag,
						char *pszTxData, char *pszAppData, char *pszARPC,
						int *pnTCLen, char *pszTC, char *pszScriptResult)
{
	uchar sOutData[2048];
	int   iOutLen;
	uchar sIssuerData[256];
	int   iIssuerDataLen;
	int   iCardAction;
	int   iRet;

	if(nIcFlag!=1 && nIcFlag!=2)
		return(JSBPBOC_CARD_TYPE);

	if(strcmp(pszAppData, sg_szAppData) != 0) {
		//vSetWriteLog(1);
		//vWriteLogTxt("ICC_CtlScriptData ret = %d", JSBPBOC_APP_DATA);
		//vWriteLogTxt("pszAppData(%d)   = [%s]", strlen(pszAppData), pszAppData);
		//vWriteLogTxt("sg_szAppData(%d) = [%s]", strlen(sg_szAppData), sg_szAppData);
		//vSetWriteLog(0);
		return(JSBPBOC_APP_DATA);
	}

	iIssuerDataLen = strlen(pszARPC);
	if(iIssuerDataLen>512 || iIssuerDataLen%2)
		return(JSBPBOC_PARAM);
	vTwoOne(pszARPC, iIssuerDataLen, sIssuerData);
	iIssuerDataLen /= 2;

	iRet = iReplaceCoreData(pszTxData);
	if(iRet)
		return(iRet);

	sg_iRet = iHxEmvGac2("00", NULL, sIssuerData, iIssuerDataLen, &iCardAction);
	if(sg_iRet == HXEMV_CARD_SW || sg_iRet == HXEMV_CARD_OP || sg_iRet == HXEMV_CARD_REMOVED)
		return(JSBPBOC_CARD_IO);
	if(sg_iRet)
		return(JSBPBOC_UNKNOWN);

	// 产生输出数据
	iOutLen = iGenOutData(sOutData);
	vOneTwo0(sOutData, iOutLen, pszTC);
	*pnTCLen = iOutLen * 2;

	iOutLen = sizeof(sOutData);
	iRet = iHxEmvGetData("\xDF\x31", &iOutLen, sOutData, NULL, NULL);
	if(iRet) {
		// 如果无脚本, 生成脚本未执行Tlv对象
		memcpy(sOutData, "\xDF\x31\x05\x00\x00\x00\x00\x00", 8);
		iOutLen = 8;
	}
	vOneTwo0(sOutData, iOutLen, pszScriptResult);

	return(0);
}

// 读取交易明细
// Out : pnTxDetailLen:	交易明细长度
//       TxDetail     :	交易明细, 格式为
//                      明细条数(2字节十进制)+每条明细的长度(3字节十进制) + 明细1+明细2+...
// Ret : 0            : 成功
//       <0           : 失败
// Note: 最多10条记录
int ICC_GetTranDetail(char *pszComNo, int nTermType, char BpNo, int nIcFlag,
						int *pnTxDetailLen, char *TxDetail)
{
	int   i;
	int   iLogLen;
	uchar sLog[100], *p;
	uchar szBuf[21];
	int   iMaxRecNum;

	if(nIcFlag!=1 && nIcFlag!=3)
		return(JSBPBOC_CARD_TYPE);

	sg_iRet = iHxEmvTransInit(0);
	if(sg_iRet)
		return(JSBPBOC_TRANS_INIT);

	sg_szAppData[0] = 0;

	for(;;) {
		stHxAdfInfo aHxAdfInfo[5];
		int iHxAdfNum;
		iHxAdfNum = sizeof(aHxAdfInfo) / sizeof(aHxAdfInfo[0]);
		memset(&aHxAdfInfo[0], 0, sizeof(aHxAdfInfo));
		sg_iRet = iHxEmvGetSupportedApp(1/*应用锁定时能选*/, &aHxAdfInfo[0], &iHxAdfNum);
		if(sg_iRet == HXEMV_NO_APP)
			return(JSBPBOC_NO_APP);
		if(sg_iRet == HXEMV_CARD_SW || sg_iRet == HXEMV_CARD_OP || sg_iRet == HXEMV_CARD_REMOVED)
			return(JSBPBOC_CARD_IO);
		if(sg_iRet && sg_iRet!=HXEMV_AUTO_SELECT)
			return(JSBPBOC_UNKNOWN);

		sg_iRet = iHxEmvAppSelect(1/*应用锁定时能选*/, aHxAdfInfo[0].ucAdfNameLen, aHxAdfInfo[0].sAdfName);
		if(sg_iRet == HXEMV_CARD_SW || sg_iRet == HXEMV_CARD_OP || sg_iRet == HXEMV_CARD_REMOVED)
			return(JSBPBOC_CARD_IO);
		if(sg_iRet == HXEMV_RESELECT)
			continue;
		if(sg_iRet)
			return(JSBPBOC_UNKNOWN);
		break;
	} // for(;;

	sg_iRet = iHxEmvGetLogInfo(0/*0:标准交易流水*/, &iMaxRecNum);
	if(sg_iRet) {
		strcpy(TxDetail, "00106");
		*pnTxDetailLen = strlen(TxDetail);
		return(0);
	}
	if(iMaxRecNum > 10)
		iMaxRecNum = 10;

	iLogLen = sizeof(sLog);
	sg_iRet = iHxEmvGetCardNativeData("\x9F\x4F", NULL, NULL, &iLogLen, sLog);
	if(sg_iRet == HXEMV_CARD_SW || sg_iRet == HXEMV_CARD_OP || sg_iRet == HXEMV_CARD_REMOVED)
		return(JSBPBOC_CARD_IO);
	if(sg_iRet) {
		strcpy(TxDetail, "00106");
		*pnTxDetailLen = strlen(TxDetail);
		return(0);
	}
	if(iLogLen != 25)
		return(JSBPBOC_LOG_FORMAT);
	if(memcmp(sLog, "\x9A\x03\x9F\x21\x03\x9F\x02\x06\x9F\x03\x06\x9F\x1A\x02\x5F\x2A\x02\x9F\x4E\x14\x9C\x01\x9F\x36\x02", iLogLen) != 0)
		return(JSBPBOC_LOG_FORMAT);

	// 准备输出数据
	strcpy(TxDetail, "00106"); // 先填0, 最后更新为正确值
	*pnTxDetailLen = 5;
	p = TxDetail + 5;
	for(i=1; i<=iMaxRecNum; i++) {
		iLogLen = sizeof(sLog);
		sg_iRet = iHxEmvReadLog(0/*0:标准交易流水*/, i, &iLogLen, sLog);
		if(sg_iRet == HXEMV_CARD_SW || sg_iRet == HXEMV_CARD_OP || sg_iRet == HXEMV_CARD_REMOVED)
			return(JSBPBOC_CARD_IO);
		if(sg_iRet == HXEMV_NO_RECORD)
			break;
		if(sg_iRet)
			return(JSBPBOC_UNKNOWN);
		
		// 授权金额
		strcpy(p, "P012");
		vOneTwo0(sLog+6, 6, p+4);
		p += 16;
		// 其它金额
		strcpy(p, "Q012");
		vOneTwo0(sLog+12, 6, p+4);
		p += 16;
		// 交易货币代码
		strcpy(p, "R004");
		vOneTwo0(sLog+20, 2, p+4);
		p += 8;
		// 交易日期
		strcpy(p, "S006");
		vOneTwo0(sLog+0, 3, p+4);
		p += 10;
		// 交易类型
		strcpy(p, "T002");
		vOneTwo0(sLog+42, 1, p+4);
		p += 6;
		// 交易时间
		strcpy(p, "U006");
		vOneTwo0(sLog+3, 3, p+4);
		p += 10;
		// 终端国家代码
		strcpy(p, "V004");
		vOneTwo0(sLog+18, 2, p+4);
		p += 8;
		// 商户名称
		strcpy(p, "W020");
		vMemcpy0(szBuf, sLog+22, 20);
		memset(p+4, ' ', 20);
		memcpy(p+4, szBuf, strlen(szBuf));
		p += 24;
		// ATC
		strcpy(p, "X004");
		vOneTwo0(sLog+43, 2, p+4);
		p += 8;

		*pnTxDetailLen += 106;
	} // for(i=1

	// 重新填写记录个数
	sprintf(szBuf, "%02d", (int)((*pnTxDetailLen - 5) / 106));
	memcpy(TxDetail, szBuf, 2);

	return(0);
}
