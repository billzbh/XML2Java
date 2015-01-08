/**************************************
File name     : EmvMsg.c
Function      : EMV/pboc2.0多语言支持信息资源表
Author        : Yu Jun
First edition : Apr 6th, 2012
Modified      : Apr 21st, 2014
					因为linux无strlwr()函数, 增加对此函数的支持
**************************************/
/*
模块详细描述:
	EMV核心需要的多语言信息资源, 应用层信息不在这里定义
.	信息包括EMVbook4中定义的标准信息类型及自定义的信息类型
.	模块假设Pinpad与主屏幕的语言支持能力可能不同, 因此提供了针对Pinpad的信息获取函数
	(对于这两种设备可能不同的情况, 高层未作过多测试, 建议以Pinpad支持的语言为准)
.	模块提供了匹配持卡人语言函数, 高层可以使用匹配的语言, 也可以自行设定语言
	如果设定的语言不支持, 本模块认为使用缺省语言
*/
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "VposFace.h"
#include "EmvMsg.h"

#define SUPORTED_LANGUAGE_NUM       2   // 支持2种语言
#define DEFAULT_LANGUAGE            1   // 无匹配语言时使用的缺省语言(sg_aszLanguageTable[]下标)

static int  sg_iCurrentLanguage;        // 当前指定的语言(sg_aszLanguageTable[]下标)
static char sg_szNull[1];               // 空串

// 支持的语言, ISO-639-1, 要用小写
static uchar sg_aszLanguageTable[SUPORTED_LANGUAGE_NUM][3] = {"en", "zh"}; // 支持英文、中文

typedef struct {
    int   iMsgType;     // 信息类型
    uchar *apszMsg[SUPORTED_LANGUAGE_NUM]; // 信息数据(c语言sprintf格式串)
} stMsg; // 多语言信息结构单元

// 信息表，语言顺序要与sg_aszLanguageTable定义的一致
// 规范定义终端显示字数为至少16字节，如果执行核心的终端显示字符只有16，需要确保信息占用字节数不超过16
static stMsg aMsg[] = {
// 标准信息
//   ID								en						zh
	{EMVMSG_00_NULL,				"",						""},
	{EMVMSG_01_AMOUNT,				"(%s)",					"(%s)"},
	{EMVMSG_02_AMOUNT_OK,			"(%s) OK?",				"(%s) 确认?"}, // 注意字节数，如果终端显示字符数太少，需要调整该信息
	{EMVMSG_03_APPROVED,			"APPROVED",				"批准"},
	{EMVMSG_04_CALL_YOUR_BANK,		"CALL YOUR BANK",		"请与你的银行联系"},
	{EMVMSG_05_CANCEL_OR_ENTER,		"CANCEL OR ENTER",		"取消或确认"}, // 该信息仅限于持卡人使用，如果面向操作员，请用EMVMSG_CANCEL_OR_ENTER
	{EMVMSG_06_CARD_ERROR,			"CARD ERROR",			"卡错误"},
	{EMVMSG_07_DECLINED,			"DECLINED",				"拒绝"},
	{EMVMSG_08_ENTER_AMOUNT,		"ENTER AMOUNT",			"输入金额"},
	{EMVMSG_09_ENTER_PIN,			"ENTER PIN",			"输入个人密码"},
	{EMVMSG_0A_INCORRECT_PIN,		"INCORRECT PIN",		"密码不正确"},
	{EMVMSG_0B_INSERT_CARD,			"INSERT CARD",			"请插卡"},
	{EMVMSG_0C_NOT_ACCEPTED,		"NOT ACCEPTED",			"不接受"},
	{EMVMSG_0D_PIN_OK,				"PIN OK",				"密码正确"},
	{EMVMSG_0E_PLEASE_WAIT,			"PLEASE WAIT",			"请等待"},
	{EMVMSG_0F_PROCESSING_ERROR,	"PROCESSING ERROR",		"处理错误"},
	{EMVMSG_10_REMOVE_CARD,			"REMOVE CARD",			"请取出卡"},
	{EMVMSG_11_USE_CHIP_REAEDER,	"USE CHIP READER",		"请用IC卡读卡器"},
	{EMVMSG_12_USE_MAG_STRIPE,		"USE MAG STRIPE",		"请用磁卡"},
	{EMVMSG_13_TRY_AGAIN,			"TRY AGAIN",			"重试一次"},

// 自定义信息
//   ID								en						zh
    {EMVMSG_CLEAR_SCR,				"",						""}, // 清屏
	{EMVMSG_NATIVE_ERR,             "NATIVE ERROR",         "内部错误"},   // 核心内部错误
	{EMVMSG_APPCALL_ERR,            "APPCALL ERROR",        "应用层错误"}, // 应用层错误
	{EMVMSG_SELECT_LANG,			"CARDHOLDER SELECT",    "选择持卡人语言"},
	{EMVMSG_SELECT_APP,             "PLEASE SELECT",        "请选择应用"},
	{EMVMSG_ENTER_PIN_OFFLINE,      "ENTER OFFLINE PIN",    "输入个人脱机密码"}, // over 16 bytes
	{EMVMSG_ENTER_PIN_ONLINE,       "ENTER ONLINE PIN",     "输入个人联机密码"}, // over 16 bytes
	{EMVMSG_LAST_PIN,				"LAST PIN TRY",			"最后一次密码输入"},

	{EMVMSG_SERVICE_NOT_ALLOWED,    "NOT ALLOWED",          "服务不允许"},
	{EMVMSG_TRANS_NOT_ALLOWED,      "TRANS NOT SUPPORTED",  "交易不支持"},
	{EMVMSG_CANCEL,                 "CANCELED",             "取消"},
	{EMVMSG_TIMEOUT,                "TIMEOUT",              "超时"},
	{EMVMSG_TERMINATE,				"TERMINATE",            "终止交易"},
	{EMVMSG_TRY_OTHER_INTERFACE,	"TRY OTHER INTERFACE",	"尝试其它通信界面"},
	{EMVMSG_EXPIRED_APP,			"EXPIRED APPLICATION",	"卡片过有效期,交易失败"},

	// 以下信息为pboc2.0证件验证专用
	{EMVMSG_VERIFY_HOLDER_ID,		"VERIFY HOLDER ID",		"请查验持卡人证件"},
	{EMVMSG_HOLDER_ID_TYPE,			"HOLDER ID TYPE",		"持卡人证件类型"},
	{EMVMSG_IDENTIFICATION_CARD,	"IDENTIFICATION CARD",  "身份证"},
	{EMVMSG_CERTIFICATE_OF_OFFICERS,"CERTIFICATE OF OFFICERS",	"军官证"},
	{EMVMSG_PASSPORT,				"PASSPORT",				"护照"},
	{EMVMSG_ARRIVAL_CARD,			"ARRIVAL CARD",			"入境证"},
	{EMVMSG_TEMPORARY_IDENTITY_CARD,"TEMPORARY IDENTITY CARD",	"临时身份证"},
	{EMVMSG_OTHER,					"OTHER",				"其它"},
	{EMVMSG_HOLDER_ID_NO,			"HOLDER ID NO.",		"持卡人证件号码"},

// 动态信息
	{EMVMSG_AMOUNT_CONFIRM,         "AMOUNT OK?",           "金额正确?"}, // Msg 02 不足以显示大金额, 用此消息号
	{EMVMSG_VAR,                    "",                     ""},
};

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

// 按信息类型排序, iMsgType从小到大
// 为了使编排信息时可以自由摆放位置，增加了排序功能
// ret : 0 : OK
static int iMsgSort(void)
{
    int i, j;
    int iMsgNum;
    stMsg MsgTmp;

    iMsgNum = sizeof(aMsg)/sizeof(aMsg[0]);
    for(i=0; i<iMsgNum; i++) {
        for(j=i+1; j<iMsgNum; j++) {
            if(aMsg[j].iMsgType < aMsg[i].iMsgType) {
                memcpy(&MsgTmp, &aMsg[i], sizeof(MsgTmp));
                memcpy(&aMsg[i], &aMsg[j], sizeof(MsgTmp));
                memcpy(&aMsg[j], &MsgTmp, sizeof(MsgTmp));
            }
        }
    }
	return(0);
}

// 获取支持的语言
// out : pszLanguages : 支持的语言, 例如:"enzh"表示支持英文、中文
//       pszDescs     : 以'='结尾的支持的语言描述, 例如:"ENGLISH=中文="描述了英文、中文
// ret : 0            : OK
//       1            : 配置错误
// 如果改变了语言配置, 要手工修改本函数以返回正确的配置
int iEmvMsgTableSupportedLanguage(uchar *pszLanguages, uchar *pszDescs)
{
	ASSERT(SUPORTED_LANGUAGE_NUM==2 && strcmp(sg_aszLanguageTable[0],"en")==0 && strcmp(sg_aszLanguageTable[0],"zh")==0);
	if(SUPORTED_LANGUAGE_NUM==2 && strcmp(sg_aszLanguageTable[0],"en")==0 && strcmp(sg_aszLanguageTable[1],"zh")==0) {
		if(pszLanguages)
			strcpy(pszLanguages, "enzh");
		if(pszDescs)
			strcpy(pszDescs, "ENGLISH=中文=");
		return(0);
	}
	return(1);
}

// 初始化多语言支持信息表, 在EMV核心初始化时调用
// in  : pszDefaultLanguage : 缺省语言, ISO-639-1, 2字节语言表示
// ret : 0 : OK
//       1 : 语言不支持, 此时会使用内定缺省语言
int iEmvMsgTableInit(uchar *pszDefaultLanguage)
{
	uchar szDefaultLanguage[3];
	int   iSub; // 制定语言的下标
	int   iLanguageSupportFlag;

	sg_iCurrentLanguage = -1;
	iLanguageSupportFlag = 0; // assume not support
	if(pszDefaultLanguage) {
		if(strlen(pszDefaultLanguage) == 2) {
			strcpy(szDefaultLanguage, pszDefaultLanguage);
			strlwr2(szDefaultLanguage);
			for(iSub=0; iSub<SUPORTED_LANGUAGE_NUM; iSub++) {
				if(strcmp(szDefaultLanguage, sg_aszLanguageTable[iSub]) == 0)
					break;
			}
			if(iSub < SUPORTED_LANGUAGE_NUM) {
				sg_iCurrentLanguage = iSub;
				iLanguageSupportFlag = 1;
			}
		}
	}
	if(sg_iCurrentLanguage == -1) {
		// 没有找到指定语言, 使用内定缺省语言
		if(DEFAULT_LANGUAGE<0 || DEFAULT_LANGUAGE>=SUPORTED_LANGUAGE_NUM)
			sg_iCurrentLanguage = 0;
		else
		    sg_iCurrentLanguage = DEFAULT_LANGUAGE;
	}
    sg_szNull[0] = 0;
    iMsgSort();
	if(!iLanguageSupportFlag)
		return(1);
	return(0);
}

// 获取匹配语言
// in  : pszCardPreferredLanguage : 持卡人偏好语言列表
// out : pszFinalLanguage         : 匹配语言[2]
// ret : 0 : OK, 找到匹配语言
//       1 : 没有匹配的语言，返回的匹配语言实为缺省语言
int iEmvMsgTableGetFitLanguage(uchar *pszCardPreferredLanguage, uchar *pszFinalLanguage)
{
	int i;
	uchar szCardPreferredLanguage[9];
	uchar *p;

	memcpy(szCardPreferredLanguage, pszCardPreferredLanguage, 8);
	szCardPreferredLanguage[8] = 0;
	strlwr2(szCardPreferredLanguage);
	for(p=szCardPreferredLanguage; *p; p+=2) {
		for(i=0; i<SUPORTED_LANGUAGE_NUM; i++) {
			if(memcmp(p, sg_aszLanguageTable[i], 2) == 0) {
				// 找到匹配语言
				memcpy(pszFinalLanguage, p, 2);
				pszFinalLanguage[2] = 0;
				return(0);
			}
		}
	}
	// 没找到匹配语言，使用缺省语言
	if(DEFAULT_LANGUAGE<0 || DEFAULT_LANGUAGE>=SUPORTED_LANGUAGE_NUM)
		strcpy(pszFinalLanguage, sg_aszLanguageTable[0]);
	else
		strcpy(pszFinalLanguage, sg_aszLanguageTable[DEFAULT_LANGUAGE]);
	return(1);
}

// 获取Pinpad匹配语言
// in  : pszCardPreferredLanguage : 持卡人偏好语言列表
//       pszPinpadLanguage        : Pinpad支持的语言
// out : pszFinalLanguage         : 匹配语言[2]
// ret : 0 : OK, 找到匹配语言
//       1 : 没有匹配的语言，返回的匹配语言实为缺省语言
int iEmvMsgTableGetPinpadLanguage(uchar *pszCardPreferredLanguage, uchar *pszPinpadLanguage, uchar *pszFinalLanguage)
{
	int i, j;
	uchar szCardPreferredLanguage[9];
	uchar szPinpadLanguage[9];

	memcpy(szCardPreferredLanguage, pszCardPreferredLanguage, 8);
	szCardPreferredLanguage[8] = 0;
	strlwr2(szCardPreferredLanguage);
	memcpy(szPinpadLanguage, pszPinpadLanguage, 8);
	szPinpadLanguage[8] = 0;
	strlwr2(szPinpadLanguage);
	for(i=0; i<(int)strlen((char *)szCardPreferredLanguage); i+=2) {
		for(j=0; j<(int)strlen(szPinpadLanguage); j+=2)
			if(memcmp(szCardPreferredLanguage+i, szPinpadLanguage+j, 2) == 0)
				break;
		if(j >= (int)strlen(szPinpadLanguage))
			continue; // pinpad不支持
		for(j=0; j<SUPORTED_LANGUAGE_NUM; j++) {
			if(memcmp(szCardPreferredLanguage+i, sg_aszLanguageTable[j], 2) == 0) {
				// 找到匹配语言
				memcpy(pszFinalLanguage, szCardPreferredLanguage+i, 2);
				pszFinalLanguage[2] = 0;
				return(0);
			}
		}
	}
	// 没找到匹配语言，使用缺省语言
	if(DEFAULT_LANGUAGE<0 || DEFAULT_LANGUAGE>=SUPORTED_LANGUAGE_NUM)
		strcpy(pszFinalLanguage, sg_aszLanguageTable[0]);
	else
		strcpy(pszFinalLanguage, sg_aszLanguageTable[DEFAULT_LANGUAGE]);
	return(1);
}

// 设置语言
// in  : psLanguageCode : ISO-639-1标准的2字节语言代码
// ret : 0              : OK
//       1              : 语言不支持
// Note: 如果不支持指定的语言，会设定为缺省语言
int iEmvMsgTableSetLanguage(uchar *psLanguageCode)
{
    int i;
    uchar szLanguageCode[3];
    
    memcpy(szLanguageCode, psLanguageCode, 2);
    szLanguageCode[2] = 0;
    strlwr2(szLanguageCode);
    sg_iCurrentLanguage = 0; // 先设定为缺省语言
    for(i=0; i<SUPORTED_LANGUAGE_NUM; i++) {
        if(strcmp(szLanguageCode, sg_aszLanguageTable[i]) == 0) {
            sg_iCurrentLanguage = i; // 支持该语言
            return(0);
        }
    }
    return(1); // 不支持该语言
}

// 搜索信息类型
// in  : iMsgType : 信息类型
// ret : >=0      : 下标
//       -1       : 没找到
static int iMsgTableSearch(int iMsgType)
{
    int iMin, iMax;
    int i;
    int iMsgNum;
    
    iMsgNum = sizeof(aMsg)/sizeof(aMsg[0]);
    iMin = 0;
    iMax = iMsgNum-1;
    while(iMin <= iMax) {
        i = (iMax-iMin)/2 + iMin;
        if(aMsg[i].iMsgType == iMsgType)
            return(i);
        if(aMsg[i].iMsgType < iMsgType)
            iMin = i+1;
        else
            iMax = i-1;
    }
	ASSERT(0);
    return(-1);
}

// 读取某信息类型表示的信息
// in  : iMsgType : 信息类型
//       psLanguageCode : ISO-639-1标准的2字节语言代码
//           NULL表示使用iEmvMsgTableSetLanguage()设定的语言
// ret : 信息, 如果没有对应信息，返回空字符串
char *pszEmvMsgTableGetInfo(int iMsgType, uchar *psLanguageCode)
{
	int iSub; // 消息下标
	int i;
	uchar szLanguageCode[3];

	iSub = 	iMsgTableSearch(iMsgType);
	if(iSub < 0)
	    return(sg_szNull); // 没找到返回空字符串
	if(psLanguageCode == NULL) {
		// 使用之前设定的语言
	    return(aMsg[iSub].apszMsg[sg_iCurrentLanguage]);
	}

	// 使用本次指定的语言
	vMemcpy0(szLanguageCode, psLanguageCode, 2);
	strlwr2(szLanguageCode);
	for(i=0; i<SUPORTED_LANGUAGE_NUM; i++) {
		if(strcmp(szLanguageCode, sg_aszLanguageTable[i]) == 0) {
			// 找到指定语言
		    return(aMsg[iSub].apszMsg[i]);
		}
	}
	// 没找到指定语言, 使用之前设定的语言
    return(aMsg[iSub].apszMsg[sg_iCurrentLanguage]);
}
