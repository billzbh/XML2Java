/**************************************
File name     : EmvPara.c
Function      : EMV/pboc2.0借贷记参数管理
Author        : Yu Jun
First edition : Mar 31st, 2012
Modified      : Apr 21st, 2014
					因为linux无strlwr()函数, 增加对此函数的支持
**************************************/
/*
模块详细描述:
	提供终端参数设置、CA公钥参数设置、AID参数设置
.	设置参数时会检查参数合法性
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

// linux无此函数, 增加支持, 有些编译器不支持同名, 因此在后面添加'2'以示区别
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

// 将二进制表示的tag转换为ascii表示的tag
static void vMakeTagStr(uchar *pszBinTag, uchar *pszAscTag)
{
	vOneTwo0(pszBinTag, strlen(pszBinTag), pszAscTag);
}

// 求出pszLangs1与pszLangs2同时支持的语言
// in  : pszLangs1 : 语言组1, 例如:"enzh"
//       pszLangs2 : 语言组2, 例如:"fren"
// out : pszLangs1 : 交集, 例子的结果:"en"
static void vGetCommonLanguage(uchar *pszLangs1, uchar *pszLangs2)
{
	int i;
	ASSERT(strlen(pszLangs1)%2 == 0);
	if(strlen(pszLangs1)%2) {
		pszLangs1[0] = 0; // 如果语言组1格式错, 认为没有交集
		return;
	}
	while(strlen(pszLangs1)) {
		for(i=0; i<(int)strlen(pszLangs2); i+=2) {
			if(memcmp(pszLangs1, pszLangs2+i, 2)==0)
				break;
		}
		if(i < (int)strlen(pszLangs2)) {
			// 当前语言有匹配
			pszLangs1 += 2;
			continue;
		}
		// 当前语言没有匹配, 删除
		memmove(pszLangs1, pszLangs1+2, strlen(pszLangs1+2)+1);
	}
}

// 设置终端参数
// in  : pTermParam        : 终端参数
// out : pszErrTag         : 如果设置出错, 出错的Tag值
// ret : HXEMV_OK          : OK
//       HXEMV_PARA        : 参数错误, 错误Tag见pszErrTag
//       HXEMV_CORE        : 内部错误
//       HXEMV_FLOW_ERROR  : EMV流程错误
int iEmvSetTermParam(stTermParam *pTermParam, uchar *pszErrTag)
{
   	int   iRet;
	uchar szBuf[256], sBuf[256];

	// T9F35, 终端类型
	if(!((pTermParam->ucTermType>=0x11 && pTermParam->ucTermType<=0x16) || (pTermParam->ucTermType>=0x21 && pTermParam->ucTermType<=0x26))) {
		vMakeTagStr(TAG_9F35_TermType, pszErrTag);
		return(HXEMV_PARA); // 只支持终端类型'11'-'16', '21'-'26'
	}
	iRet = iTlvMakeAddObj(gl_sTlvDbTermFixed, TAG_9F35_TermType, 1, &pTermParam->ucTermType, TLV_CONFLICT_ERR);
   	ASSERT(iRet >= 0);
	if(iRet < 0)
		return(HXEMV_CORE);

	// T9F33, 终端能力
	if(iTestStrZeroWithMark(pTermParam->sTermCapability, "\x1F\x06\x17", 3) != 0) { // modified 2013.2.19, 支持证件验证, "\x1F\x07\x17"->"\x1F\x06\x17"
		vMakeTagStr(TAG_9F33_TermCapability, pszErrTag);
		return(HXEMV_PARA); // RFU位必须为0
	}
	if(iTestStrZeroWithMark(pTermParam->sTermCapability, "\x00\x10\x20", 3) != 0) {
		vMakeTagStr(TAG_9F33_TermCapability, pszErrTag);
		return(HXEMV_PARA); // 核心不支持脱机密文验证PIN、Card Capture
	}
	iRet = iTlvMakeAddObj(gl_sTlvDbTermFixed, TAG_9F33_TermCapability, 3, pTermParam->sTermCapability, TLV_CONFLICT_ERR);
    ASSERT(iRet >= 0);
	if(iRet < 0)
		return(HXEMV_CORE);
	if(iEmvTermCommunication() != TERM_COM_ONLINE_ONLY) {
		// 不是联机终端
		if(iTestStrBit(pTermParam->sTermCapability, TERM_CAP_16_SDA) != 1) {
			vMakeTagStr(TAG_9F33_TermCapability, pszErrTag);
			return(HXEMV_PARA); // 非联机终端必须支持SDA
		}
		if(iTestStrBit(pTermParam->sTermCapability, TERM_CAP_17_DDA) != 1) {
			vMakeTagStr(TAG_9F33_TermCapability, pszErrTag);
			return(HXEMV_PARA); // 非联机终端必须支持DDA
		}
	}
	
	// T9F40, 终端能力扩展
	if(iTestStrZeroWithMark(pTermParam->sAdditionalTermCapability, "\x00\x3F\x0F\x0C\x00", 5) != 0) {
		vMakeTagStr(TAG_9F40_TermCapabilityExt, pszErrTag);
		return(HXEMV_PARA); // RFU位必须为0
	}
	if(iTestStrZeroWithMark(pTermParam->sAdditionalTermCapability, "\x10\x00\x00\x03\xFF", 5) != 0) {
		vMakeTagStr(TAG_9F40_TermCapabilityExt, pszErrTag);
		return(HXEMV_PARA); // 核心不支持cashback、Code table1-10
	}
	iRet = iTlvMakeAddObj(gl_sTlvDbTermFixed, TAG_9F40_TermCapabilityExt, 5, pTermParam->sAdditionalTermCapability, TLV_CONFLICT_ERR);
	if(iRet < 0) {
        ASSERT(0);
		return(HXEMV_CORE);
	}

	// TDFxx, 终端非接支持能力
	if((pTermParam->sTermCtlsCapability[0]&0x60) == 0x00) {
		vMakeTagStr(TAG_DFXX_TermCtlsCapMask, pszErrTag);
		return(HXEMV_PARA); // 非接pboc与非接qPboc至少要支持一种
	}
	iRet = iTlvMakeAddObj(gl_sTlvDbTermFixed, TAG_DFXX_TermCtlsCapMask, 4, pTermParam->sTermCtlsCapability, TLV_CONFLICT_ERR);
	if(iRet < 0) {
        ASSERT(0);
		return(HXEMV_CORE);
	}

	// TDFxx, 1:只支持磁卡 2:只支持IC卡 3:支持组合式Mag/IC读卡器 4:支持分离式Mag/IC读卡器(TAG_DFXX_ReaderCapability)
	if(pTermParam->ucReaderCapability<1 || pTermParam->ucReaderCapability > 4) {
		vMakeTagStr(TAG_DFXX_ReaderCapability, pszErrTag);
		return(HXEMV_PARA);
	}
	iRet = iTlvMakeAddObj(gl_sTlvDbTermFixed, TAG_DFXX_ReaderCapability, 1, &pTermParam->ucReaderCapability, TLV_CONFLICT_ERR);
    ASSERT(iRet >= 0);
	if(iRet < 0)
		return(HXEMV_CORE);

	// TDFxx, 1:支持 2:不支持,缺省approve 3:不支持,缺省decline(TAG_DFXX_VoiceReferralSupport)
	if(pTermParam->ucVoiceReferralSupport<1 || pTermParam->ucVoiceReferralSupport > 3) {
		vMakeTagStr(TAG_DFXX_VoiceReferralSupport, pszErrTag);
		return(HXEMV_PARA);
	}
	iRet = iTlvMakeAddObj(gl_sTlvDbTermFixed, TAG_DFXX_VoiceReferralSupport, 1, &pTermParam->ucVoiceReferralSupport, TLV_CONFLICT_ERR);
	if(iRet < 0) {
        ASSERT(0);
		return(HXEMV_CORE);
	}

    // TDFxx, PIN bypass特性 0:每次bypass只表示该次bypass 1:一次bypass,后续都认为bypass
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

	// T9F15, -1:无此数据 0-9999:有效数据
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

	// T9F1A, 终端国家代码
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

	// T5F2A, 终端交易货币代码
	if(pTermParam->iTermCurrencyCode<0 || pTermParam->iTermCurrencyCode>999) {
		vMakeTagStr(TAG_5F2A_TransCurrencyCode, pszErrTag);
		return(HXEMV_PARA);
	}
#if 0
	// modified 20130121, 为支持多货币, 在gpo时使用传入, 放入gl_sTlvDbTermVar数据库
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
		return(HXEMV_PARA); // 必须是十进制字符串表示
	}
	strcpy(szBuf, "0");
	if(strlen(pTermParam->szAcquirerId) % 2)
		strcat(szBuf, pTermParam->szAcquirerId); // 如果长度为奇数，左补'0'
	else
		strcpy(szBuf, pTermParam->szAcquirerId);
	vTwoOne(szBuf, strlen(szBuf), sBuf);
	iRet = iTlvMakeAddObj(gl_sTlvDbTermFixed, TAG_9F01_AcquirerId, strlen(szBuf)/2, sBuf, TLV_CONFLICT_ERR);
	if(iRet < 0) {
        ASSERT(0);
		return(HXEMV_CORE);
	}

	// TDFxx, 1:终端支持交易流水记录 0:不支持(TAG_DFXX_TransLogSupport)
	if(pTermParam->ucTransLogSupport > 1) {
		vMakeTagStr(TAG_DFXX_TransLogSupport, pszErrTag);
		return(HXEMV_PARA);
	}
	iRet = iTlvMakeAddObj(gl_sTlvDbTermFixed, TAG_DFXX_TransLogSupport, 1, &pTermParam->ucTransLogSupport, TLV_CONFLICT_ERR);
	if(iRet < 0) {
        ASSERT(0);
		return(HXEMV_CORE);
	}

	// TDFxx, 1:终端支持黑名单检查 0:不支持(TAG_DFXX_BlacklistSupport)
	if(pTermParam->ucBlackListSupport > 1) {
		vMakeTagStr(TAG_DFXX_BlacklistSupport, pszErrTag);
		return(HXEMV_PARA);
	}
	iRet = iTlvMakeAddObj(gl_sTlvDbTermFixed, TAG_DFXX_BlacklistSupport, 1, &pTermParam->ucBlackListSupport, TLV_CONFLICT_ERR);
	if(iRet < 0) {
        ASSERT(0);
		return(HXEMV_CORE);
	}

	// TDFxx, 1:支持分开消费检查 0:不支持(TAG_DFXX_SeparateSaleSupport)
	if(pTermParam->ucSeparateSaleSupport > 1) {
		vMakeTagStr(TAG_DFXX_SeparateSaleSupport, pszErrTag);
		return(HXEMV_PARA);
	}
	iRet = iTlvMakeAddObj(gl_sTlvDbTermFixed, TAG_DFXX_SeparateSaleSupport, 1, &pTermParam->ucSeparateSaleSupport, TLV_CONFLICT_ERR);
	if(iRet < 0) {
        ASSERT(0);
		return(HXEMV_CORE);
	}

	// TDFxx, 1:支持应用确认 0:不支持应用确认(TAG_DFXX_AppConfirmSupport)
	if(pTermParam->ucAppConfirmSupport > 1) {
		vMakeTagStr(TAG_DFXX_AppConfirmSupport, pszErrTag);
		return(HXEMV_PARA);
	}
	iRet = iTlvMakeAddObj(gl_sTlvDbTermFixed, TAG_DFXX_AppConfirmSupport, 1, &pTermParam->ucAppConfirmSupport, TLV_CONFLICT_ERR);
	if(iRet < 0) {
        ASSERT(0);
		return(HXEMV_CORE);
	}

	// TDFxx, ISO-639-1, 本地语言(TAG_DFXX_LocalLanguage)
	if(strlen(pTermParam->szLocalLanguage) != 2) {
		vMakeTagStr(TAG_DFXX_LocalLanguage, pszErrTag);
		return(HXEMV_PARA);
	}
	iRet = iTlvMakeAddObj(gl_sTlvDbTermFixed, TAG_DFXX_LocalLanguage, 2, pTermParam->szLocalLanguage, TLV_CONFLICT_ERR);
	if(iRet < 0) {
        ASSERT(0);
		return(HXEMV_CORE);
	}

	// TDFxx, ISO-639-1, pinpad 语言(TAG_DFXX_PinpadLanguage)
	if(strlen(pTermParam->szPinpadLanguage) == 0)
        strcpy(szBuf, "en"); // 如果没有配置密码键盘使用语言, 缺省认为支持英语
    else
        strcpy(szBuf, pTermParam->szPinpadLanguage);
	if(strlen(szBuf)%2 || strlen(szBuf)>32) {
		vMakeTagStr(TAG_DFXX_PinpadLanguage, pszErrTag);
		return(HXEMV_PARA);
	}
	// 求出终端与pinpad同时支持的语言
	iRet = iEmvMsgTableSupportedLanguage(sBuf, NULL);
	if(iRet) {
		ASSERT(0);
		return(HXEMV_CORE);
	}
	strlwr2(szBuf);
	strlwr2(sBuf);
	vGetCommonLanguage(szBuf, sBuf); // szBuf=pinpad语言 sBuf=终端语言
	if(strlen(szBuf) == 0)
		strcpy(szBuf, "en"); // 如果没有匹配, 认为支持英语
	iRet = iTlvMakeAddObj(gl_sTlvDbTermFixed, TAG_DFXX_PinpadLanguage, strlen(szBuf), szBuf, TLV_CONFLICT_ERR);
	if(iRet < 0) {
        ASSERT(0);
		return(HXEMV_CORE);
	}
	// TAG_DFXX_TermCtlsAmountLimit, 终端非接交易限额
	if(strlen(pTermParam->szTermCtlsAmountLimit)) {
		if(strlen(pTermParam->szTermCtlsAmountLimit) > 12) {
			vMakeTagStr(TAG_DFXX_TermCtlsAmountLimit, pszErrTag);
			return(HXEMV_PARA);
		}
		if(iTestStrDecimal(pTermParam->szTermCtlsAmountLimit, strlen(pTermParam->szTermCtlsAmountLimit)) != 0) {
			vMakeTagStr(TAG_DFXX_TermCtlsAmountLimit, pszErrTag);
			return(HXEMV_PARA); // 必须是十进制字符串表示
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
	// TAG_DFXX_TermCtlsOfflineAmountLimit, 终端非接脱机交易限额
	if(strlen(pTermParam->szTermCtlsOfflineAmountLimit)) {
		if(strlen(pTermParam->szTermCtlsOfflineAmountLimit) > 12) {
			vMakeTagStr(TAG_DFXX_TermCtlsOfflineAmountLimit, pszErrTag);
			return(HXEMV_PARA);
		}
		if(iTestStrDecimal(pTermParam->szTermCtlsOfflineAmountLimit, strlen(pTermParam->szTermCtlsOfflineAmountLimit)) != 0) {
			vMakeTagStr(TAG_DFXX_TermCtlsOfflineAmountLimit, pszErrTag);
			return(HXEMV_PARA); // 必须是十进制字符串表示
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
	// TAG_DFXX_TermCtlsCvmLimit, 终端非接CVM限额
	if(strlen(pTermParam->szTermCtlsCvmLimit)) {
		if(strlen(pTermParam->szTermCtlsCvmLimit) > 12) {
			vMakeTagStr(TAG_DFXX_TermCtlsCvmLimit, pszErrTag);
			return(HXEMV_PARA);
		}
		if(iTestStrDecimal(pTermParam->szTermCtlsCvmLimit, strlen(pTermParam->szTermCtlsCvmLimit)) != 0) {
			vMakeTagStr(TAG_DFXX_TermCtlsCvmLimit, pszErrTag);
			return(HXEMV_PARA); // 必须是十进制字符串表示
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

	// 以下参数为终端通用参数与AID相关参数公共部分
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

	// TDFxx, Maximum Target Percentage to be used for Biased Random Selection，-1:无此数据(TAG_DFXX_MaxTargetPercentage)
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

	// TDFxx, Target Percentage to be used for Random Selection，-1:无此数据(TAG_DFXX_TargetPercentage)
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
		// iMaxTergetPercentage与iTargetPercentage同时存在, 判断合法性
		if(pTermParam->AidCommonPara.iMaxTargetPercentage < pTermParam->AidCommonPara.iTargetPercentage) {
			vMakeTagStr(TAG_DFXX_MaxTargetPercentage, pszErrTag);
			vMakeTagStr(TAG_DFXX_TargetPercentage, pszErrTag+strlen(pszErrTag));
			return(HXEMV_PARA); // iMaxTergetPercentage至少要与iTargetPercentage一样大
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
		// ulThresholdValue与ulFloorLimit同时存在, 合法性检查
		if(pTermParam->AidCommonPara.ulThresholdValue >= pTermParam->AidCommonPara.ulFloorLimit) {
			vMakeTagStr(TAG_9F1B_TermFloorLimit, pszErrTag);
			vMakeTagStr(TAG_DFXX_RandomSelThreshold, pszErrTag+strlen(pszErrTag));
			return(HXEMV_PARA); // ulThresholdValue必须小于ulFloorLimit
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

	// TDFxx, 1:支持 0:不支持(TAG_DFXX_ForceOnlineSupport)
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

	// TDFxx, 1:支持 0:不支持(TAG_DFXX_ForceAcceptSupport)
	if(pTermParam->AidCommonPara.cForcedAcceptanceSupport >= 0) {
		if(pTermParam->AidCommonPara.cForcedAcceptanceSupport > 0) { // 由于核心不支持, 不要设定为1
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

	// TDFxx, 1:支持电子现金 0:不支持电子现金(TAG_DFXX_ECTermSupportIndicator)
	// 注: 注:GPO需要T9F7A来指示, 交易开始后在判断符合电子现金支付条件后后, 设定T9F7A的值
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

	// T9F7B, 终端电子现金交易限额
	if(strlen(pTermParam->AidCommonPara.szTermECashTransLimit)) {
		if(strlen(pTermParam->AidCommonPara.szTermECashTransLimit) > 12) {
			vMakeTagStr(TAG_9F7B_ECTermTransLimit, pszErrTag);
			return(HXEMV_PARA);
		}
		if(iTestStrDecimal(pTermParam->AidCommonPara.szTermECashTransLimit, strlen(pTermParam->AidCommonPara.szTermECashTransLimit)) != 0) {
			vMakeTagStr(TAG_9F7B_ECTermTransLimit, pszErrTag);
			return(HXEMV_PARA); // 必须是十进制字符串表示
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

	// TDFxx, 1:支持联机PIN 0:不支持联机PIN(TAG_DFXX_OnlinePINSupport)
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

// 装载终端支持的Aid
// in  : pTermAid          : 终端支持的Aid
//       iFlag             : 动作, HXEMV_PARA_INIT:初始化 HXEMV_PARA_ADD:增加 HXEMV_PARA_DEL:删除
// out : pszErrTag         : 如果设置出错, 出错的Tag值
// ret : HXEMV_OK          : OK
//       HXEMV_LACK_MEMORY : 存储空间不足
//       HXEMV_PARA        : 参数错误, 错误Tag见pszErrTag
//       HXEMV_FLOW_ERROR  : EMV流程错误
// Note: 删除时只传入AID即可
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
		gl_iTermSupportedAidNum = 0; // 终端当前支持的Aid列表数目
		break;
	case HXEMV_PARA_ADD:
		if(gl_iTermSupportedAidNum >= sizeof(gl_aTermSupportedAidList)/sizeof(gl_aTermSupportedAidList[0])) {
            ASSERT(0);
			return(HXEMV_LACK_MEMORY);
		}

		// 参数合法性检查
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
	    		return(HXEMV_PARA); // iMaxTergetPercentage至少要与iTargetPercentage一样大
		    }
		}
		// 由于终端参数优先, 综合终端参数检查MaxTargetPercentage与TargetPercentage合法性
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
	    		return(HXEMV_PARA); // iMaxTergetPercentage至少要与iTargetPercentage一样大
		    }
		}

		if(pTermAid->ucThresholdExistFlag && pTermAid->ucFloorLimitExistFlag) {
			if(pTermAid->ulThresholdValue >= pTermAid->ulFloorLimit) {
				vMakeTagStr(TAG_DFXX_RandomSelThreshold, pszErrTag);
				vMakeTagStr(TAG_9F1B_TermFloorLimit, pszErrTag+strlen(pszErrTag));
				return(HXEMV_PARA); // Threshold必须小于FloorLimit
			}
		}
		// 由于终端参数优先, 综合终端参数检查ThresholdValue与FloorLimit合法性
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
				return(HXEMV_PARA); // Threshold必须小于FloorLimit
			}
		}
		// FloorLimit为必须参数, 或者终端或者AID, 至少有一处需要设置
		if(!ucFloorLimitExistFlag) {
			vMakeTagStr(TAG_9F1B_TermFloorLimit, pszErrTag);
			return(HXEMV_PARA); // FloorLimit必须存在
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
				return(HXEMV_PARA); // 必须是十进制字符串表示
			}
		}
		if(pTermAid->cOnlinePinSupport > 1) {
			vMakeTagStr(TAG_DFXX_OnlinePINSupport, pszErrTag);
			return(HXEMV_PARA);
		}

		// 保存参数
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

// 装载CA公钥
// in  : pCAPublicKey      : CA公钥
//       iFlag             : 动作, HXEMV_PARA_INIT:初始化 HXEMV_PARA_ADD:增加 HXEMV_PARA_DEL:删除
// ret : HXEMV_OK          : OK
//       HXEMV_LACK_MEMORY : 存储空间不足
//       HXEMV_PARA        : 参数错误
//       HXEMV_FLOW_ERROR  : EMV流程错误
// Note: 删除时只传入RID、index即可
int iEmvLoadCAPublicKey(stCAPublicKey *pCAPublicKey, int iFlag)
{
	int i;
	switch(iFlag) {
	case HXEMV_PARA_INIT:
		gl_iTermSupportedCaPublicKeyNum = 0; // 终端当前支持的Aid列表数目
		break;
	case HXEMV_PARA_ADD:
		if(pCAPublicKey->ucKeyLen == 0)
			break;
		if(gl_iTermSupportedCaPublicKeyNum >= sizeof(gl_aCAPublicKeyList)/sizeof(gl_aCAPublicKeyList[0])) {
		    ASSERT(0);
			return(HXEMV_LACK_MEMORY);
		}

		// 参数合法性检查
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
		
		// 保存参数
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
