/**************************************
File name     : EmvMsg.c
Function      : EMV/pboc2.0������֧����Ϣ��Դ��
Author        : Yu Jun
First edition : Apr 6th, 2012
Modified      : Apr 21st, 2014
					��Ϊlinux��strlwr()����, ���ӶԴ˺�����֧��
**************************************/
/*
ģ����ϸ����:
	EMV������Ҫ�Ķ�������Ϣ��Դ, Ӧ�ò���Ϣ�������ﶨ��
.	��Ϣ����EMVbook4�ж���ı�׼��Ϣ���ͼ��Զ������Ϣ����
.	ģ�����Pinpad������Ļ������֧���������ܲ�ͬ, ����ṩ�����Pinpad����Ϣ��ȡ����
	(�����������豸���ܲ�ͬ�����, �߲�δ���������, ������Pinpad֧�ֵ�����Ϊ׼)
.	ģ���ṩ��ƥ��ֿ������Ժ���, �߲����ʹ��ƥ�������, Ҳ���������趨����
	����趨�����Բ�֧��, ��ģ����Ϊʹ��ȱʡ����
*/
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "VposFace.h"
#include "EmvMsg.h"

#define SUPORTED_LANGUAGE_NUM       2   // ֧��2������
#define DEFAULT_LANGUAGE            1   // ��ƥ������ʱʹ�õ�ȱʡ����(sg_aszLanguageTable[]�±�)

static int  sg_iCurrentLanguage;        // ��ǰָ��������(sg_aszLanguageTable[]�±�)
static char sg_szNull[1];               // �մ�

// ֧�ֵ�����, ISO-639-1, Ҫ��Сд
static uchar sg_aszLanguageTable[SUPORTED_LANGUAGE_NUM][3] = {"en", "zh"}; // ֧��Ӣ�ġ�����

typedef struct {
    int   iMsgType;     // ��Ϣ����
    uchar *apszMsg[SUPORTED_LANGUAGE_NUM]; // ��Ϣ����(c����sprintf��ʽ��)
} stMsg; // ��������Ϣ�ṹ��Ԫ

// ��Ϣ������˳��Ҫ��sg_aszLanguageTable�����һ��
// �淶�����ն���ʾ����Ϊ����16�ֽڣ����ִ�к��ĵ��ն���ʾ�ַ�ֻ��16����Ҫȷ����Ϣռ���ֽ���������16
static stMsg aMsg[] = {
// ��׼��Ϣ
//   ID								en						zh
	{EMVMSG_00_NULL,				"",						""},
	{EMVMSG_01_AMOUNT,				"(%s)",					"(%s)"},
	{EMVMSG_02_AMOUNT_OK,			"(%s) OK?",				"(%s) ȷ��?"}, // ע���ֽ���������ն���ʾ�ַ���̫�٣���Ҫ��������Ϣ
	{EMVMSG_03_APPROVED,			"APPROVED",				"��׼"},
	{EMVMSG_04_CALL_YOUR_BANK,		"CALL YOUR BANK",		"�������������ϵ"},
	{EMVMSG_05_CANCEL_OR_ENTER,		"CANCEL OR ENTER",		"ȡ����ȷ��"}, // ����Ϣ�����ڳֿ���ʹ�ã�����������Ա������EMVMSG_CANCEL_OR_ENTER
	{EMVMSG_06_CARD_ERROR,			"CARD ERROR",			"������"},
	{EMVMSG_07_DECLINED,			"DECLINED",				"�ܾ�"},
	{EMVMSG_08_ENTER_AMOUNT,		"ENTER AMOUNT",			"������"},
	{EMVMSG_09_ENTER_PIN,			"ENTER PIN",			"�����������"},
	{EMVMSG_0A_INCORRECT_PIN,		"INCORRECT PIN",		"���벻��ȷ"},
	{EMVMSG_0B_INSERT_CARD,			"INSERT CARD",			"��忨"},
	{EMVMSG_0C_NOT_ACCEPTED,		"NOT ACCEPTED",			"������"},
	{EMVMSG_0D_PIN_OK,				"PIN OK",				"������ȷ"},
	{EMVMSG_0E_PLEASE_WAIT,			"PLEASE WAIT",			"��ȴ�"},
	{EMVMSG_0F_PROCESSING_ERROR,	"PROCESSING ERROR",		"�������"},
	{EMVMSG_10_REMOVE_CARD,			"REMOVE CARD",			"��ȡ����"},
	{EMVMSG_11_USE_CHIP_REAEDER,	"USE CHIP READER",		"����IC��������"},
	{EMVMSG_12_USE_MAG_STRIPE,		"USE MAG STRIPE",		"���ôſ�"},
	{EMVMSG_13_TRY_AGAIN,			"TRY AGAIN",			"����һ��"},

// �Զ�����Ϣ
//   ID								en						zh
    {EMVMSG_CLEAR_SCR,				"",						""}, // ����
	{EMVMSG_NATIVE_ERR,             "NATIVE ERROR",         "�ڲ�����"},   // �����ڲ�����
	{EMVMSG_APPCALL_ERR,            "APPCALL ERROR",        "Ӧ�ò����"}, // Ӧ�ò����
	{EMVMSG_SELECT_LANG,			"CARDHOLDER SELECT",    "ѡ��ֿ�������"},
	{EMVMSG_SELECT_APP,             "PLEASE SELECT",        "��ѡ��Ӧ��"},
	{EMVMSG_ENTER_PIN_OFFLINE,      "ENTER OFFLINE PIN",    "��������ѻ�����"}, // over 16 bytes
	{EMVMSG_ENTER_PIN_ONLINE,       "ENTER ONLINE PIN",     "���������������"}, // over 16 bytes
	{EMVMSG_LAST_PIN,				"LAST PIN TRY",			"���һ����������"},

	{EMVMSG_SERVICE_NOT_ALLOWED,    "NOT ALLOWED",          "��������"},
	{EMVMSG_TRANS_NOT_ALLOWED,      "TRANS NOT SUPPORTED",  "���ײ�֧��"},
	{EMVMSG_CANCEL,                 "CANCELED",             "ȡ��"},
	{EMVMSG_TIMEOUT,                "TIMEOUT",              "��ʱ"},
	{EMVMSG_TERMINATE,				"TERMINATE",            "��ֹ����"},
	{EMVMSG_TRY_OTHER_INTERFACE,	"TRY OTHER INTERFACE",	"��������ͨ�Ž���"},
	{EMVMSG_EXPIRED_APP,			"EXPIRED APPLICATION",	"��Ƭ����Ч��,����ʧ��"},

	// ������ϢΪpboc2.0֤����֤ר��
	{EMVMSG_VERIFY_HOLDER_ID,		"VERIFY HOLDER ID",		"�����ֿ���֤��"},
	{EMVMSG_HOLDER_ID_TYPE,			"HOLDER ID TYPE",		"�ֿ���֤������"},
	{EMVMSG_IDENTIFICATION_CARD,	"IDENTIFICATION CARD",  "���֤"},
	{EMVMSG_CERTIFICATE_OF_OFFICERS,"CERTIFICATE OF OFFICERS",	"����֤"},
	{EMVMSG_PASSPORT,				"PASSPORT",				"����"},
	{EMVMSG_ARRIVAL_CARD,			"ARRIVAL CARD",			"�뾳֤"},
	{EMVMSG_TEMPORARY_IDENTITY_CARD,"TEMPORARY IDENTITY CARD",	"��ʱ���֤"},
	{EMVMSG_OTHER,					"OTHER",				"����"},
	{EMVMSG_HOLDER_ID_NO,			"HOLDER ID NO.",		"�ֿ���֤������"},

// ��̬��Ϣ
	{EMVMSG_AMOUNT_CONFIRM,         "AMOUNT OK?",           "�����ȷ?"}, // Msg 02 ��������ʾ����, �ô���Ϣ��
	{EMVMSG_VAR,                    "",                     ""},
};

// linux�޴˺���, ����֧��, ��Щ��������֧��ͬ��, ����ں������'2'��ʾ����
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

// ����Ϣ��������, iMsgType��С����
// Ϊ��ʹ������Ϣʱ�������ɰڷ�λ�ã�������������
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

// ��ȡ֧�ֵ�����
// out : pszLanguages : ֧�ֵ�����, ����:"enzh"��ʾ֧��Ӣ�ġ�����
//       pszDescs     : ��'='��β��֧�ֵ���������, ����:"ENGLISH=����="������Ӣ�ġ�����
// ret : 0            : OK
//       1            : ���ô���
// ����ı�����������, Ҫ�ֹ��޸ı������Է�����ȷ������
int iEmvMsgTableSupportedLanguage(uchar *pszLanguages, uchar *pszDescs)
{
	ASSERT(SUPORTED_LANGUAGE_NUM==2 && strcmp(sg_aszLanguageTable[0],"en")==0 && strcmp(sg_aszLanguageTable[0],"zh")==0);
	if(SUPORTED_LANGUAGE_NUM==2 && strcmp(sg_aszLanguageTable[0],"en")==0 && strcmp(sg_aszLanguageTable[1],"zh")==0) {
		if(pszLanguages)
			strcpy(pszLanguages, "enzh");
		if(pszDescs)
			strcpy(pszDescs, "ENGLISH=����=");
		return(0);
	}
	return(1);
}

// ��ʼ��������֧����Ϣ��, ��EMV���ĳ�ʼ��ʱ����
// in  : pszDefaultLanguage : ȱʡ����, ISO-639-1, 2�ֽ����Ա�ʾ
// ret : 0 : OK
//       1 : ���Բ�֧��, ��ʱ��ʹ���ڶ�ȱʡ����
int iEmvMsgTableInit(uchar *pszDefaultLanguage)
{
	uchar szDefaultLanguage[3];
	int   iSub; // �ƶ����Ե��±�
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
		// û���ҵ�ָ������, ʹ���ڶ�ȱʡ����
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

// ��ȡƥ������
// in  : pszCardPreferredLanguage : �ֿ���ƫ�������б�
// out : pszFinalLanguage         : ƥ������[2]
// ret : 0 : OK, �ҵ�ƥ������
//       1 : û��ƥ������ԣ����ص�ƥ������ʵΪȱʡ����
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
				// �ҵ�ƥ������
				memcpy(pszFinalLanguage, p, 2);
				pszFinalLanguage[2] = 0;
				return(0);
			}
		}
	}
	// û�ҵ�ƥ�����ԣ�ʹ��ȱʡ����
	if(DEFAULT_LANGUAGE<0 || DEFAULT_LANGUAGE>=SUPORTED_LANGUAGE_NUM)
		strcpy(pszFinalLanguage, sg_aszLanguageTable[0]);
	else
		strcpy(pszFinalLanguage, sg_aszLanguageTable[DEFAULT_LANGUAGE]);
	return(1);
}

// ��ȡPinpadƥ������
// in  : pszCardPreferredLanguage : �ֿ���ƫ�������б�
//       pszPinpadLanguage        : Pinpad֧�ֵ�����
// out : pszFinalLanguage         : ƥ������[2]
// ret : 0 : OK, �ҵ�ƥ������
//       1 : û��ƥ������ԣ����ص�ƥ������ʵΪȱʡ����
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
			continue; // pinpad��֧��
		for(j=0; j<SUPORTED_LANGUAGE_NUM; j++) {
			if(memcmp(szCardPreferredLanguage+i, sg_aszLanguageTable[j], 2) == 0) {
				// �ҵ�ƥ������
				memcpy(pszFinalLanguage, szCardPreferredLanguage+i, 2);
				pszFinalLanguage[2] = 0;
				return(0);
			}
		}
	}
	// û�ҵ�ƥ�����ԣ�ʹ��ȱʡ����
	if(DEFAULT_LANGUAGE<0 || DEFAULT_LANGUAGE>=SUPORTED_LANGUAGE_NUM)
		strcpy(pszFinalLanguage, sg_aszLanguageTable[0]);
	else
		strcpy(pszFinalLanguage, sg_aszLanguageTable[DEFAULT_LANGUAGE]);
	return(1);
}

// ��������
// in  : psLanguageCode : ISO-639-1��׼��2�ֽ����Դ���
// ret : 0              : OK
//       1              : ���Բ�֧��
// Note: �����֧��ָ�������ԣ����趨Ϊȱʡ����
int iEmvMsgTableSetLanguage(uchar *psLanguageCode)
{
    int i;
    uchar szLanguageCode[3];
    
    memcpy(szLanguageCode, psLanguageCode, 2);
    szLanguageCode[2] = 0;
    strlwr2(szLanguageCode);
    sg_iCurrentLanguage = 0; // ���趨Ϊȱʡ����
    for(i=0; i<SUPORTED_LANGUAGE_NUM; i++) {
        if(strcmp(szLanguageCode, sg_aszLanguageTable[i]) == 0) {
            sg_iCurrentLanguage = i; // ֧�ָ�����
            return(0);
        }
    }
    return(1); // ��֧�ָ�����
}

// ������Ϣ����
// in  : iMsgType : ��Ϣ����
// ret : >=0      : �±�
//       -1       : û�ҵ�
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

// ��ȡĳ��Ϣ���ͱ�ʾ����Ϣ
// in  : iMsgType : ��Ϣ����
//       psLanguageCode : ISO-639-1��׼��2�ֽ����Դ���
//           NULL��ʾʹ��iEmvMsgTableSetLanguage()�趨������
// ret : ��Ϣ, ���û�ж�Ӧ��Ϣ�����ؿ��ַ���
char *pszEmvMsgTableGetInfo(int iMsgType, uchar *psLanguageCode)
{
	int iSub; // ��Ϣ�±�
	int i;
	uchar szLanguageCode[3];

	iSub = 	iMsgTableSearch(iMsgType);
	if(iSub < 0)
	    return(sg_szNull); // û�ҵ����ؿ��ַ���
	if(psLanguageCode == NULL) {
		// ʹ��֮ǰ�趨������
	    return(aMsg[iSub].apszMsg[sg_iCurrentLanguage]);
	}

	// ʹ�ñ���ָ��������
	vMemcpy0(szLanguageCode, psLanguageCode, 2);
	strlwr2(szLanguageCode);
	for(i=0; i<SUPORTED_LANGUAGE_NUM; i++) {
		if(strcmp(szLanguageCode, sg_aszLanguageTable[i]) == 0) {
			// �ҵ�ָ������
		    return(aMsg[iSub].apszMsg[i]);
		}
	}
	// û�ҵ�ָ������, ʹ��֮ǰ�趨������
    return(aMsg[iSub].apszMsg[sg_iCurrentLanguage]);
}
