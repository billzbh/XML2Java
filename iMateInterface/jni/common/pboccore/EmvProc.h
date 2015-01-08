/**************************************
File name     : EmvProc.h
Function      : Pboc3.0�����/EMV2004�ͻ��ӿ�
Author        : Yu Jun
First edition : Mar 11th, 2014
Modified      : Mar 28th, 2014
				ȥ��iHxEmvSetAmount(uchar *pszAmount, uchar *pszAmountOther)�����е�pszAmountOther����
**************************************/
#ifndef _EMVPROC_H
#define _EMVPROC_H

// EMV�ͻ��ӿڷ�����
#define HXEMV_OK					0  // OK
#define HXEMV_NA					1  // ������
#define HXEMV_PARA					2  // ��������
#define HXEMV_LACK_MEMORY			3  // �洢�ռ䲻��
#define HXEMV_CORE					4  // �ڲ�����
#define HXEMV_NO_SLOT				5  // ��֧�ֵĿ���
#define HXEMV_NO_CARD               6  // ��Ƭ������
#define HXEMV_CANCEL				7  // �û�ȡ��
#define HXEMV_TIMEOUT				8  // ��ʱ
#define HXEMV_NO_APP				9  // ��֧�ֵ�Ӧ��
#define HXEMV_AUTO_SELECT           10 // ��ȡ��Ӧ�ÿ��Զ�ѡ��
#define HXEMV_CARD_REMOVED			11 // ����ȡ��
#define HXEMV_CARD_OP				12 // ��������
#define HXEMV_CARD_SW				13 // �Ƿ���ָ��״̬��
#define HXEMV_NO_DATA				14 // ������
#define HXEMV_NO_RECORD				15 // �޼�¼
#define HXEMV_NO_LOG				16 // ��Ƭ��֧�ֽ�����ˮ��¼
#define HXEMV_TERMINATE				17 // ����ܾ�������������ֹ
#define HXEMV_USE_MAG				18 // ��ʹ�ô�����
#define HXEMV_RESELECT				19 // ��Ҫ����ѡ��Ӧ��
#define HXEMV_NOT_SUPPORTED         20 // ��֧��
#define HXEMV_DENIAL				21 // ���׾ܾ�
#define HXEMV_DENIAL_ADVICE			22 // ���׾ܾ�, ��Advice
#define HXEMV_NOT_ALLOWED			23 // ��������
#define HXEMV_TRANS_NOT_ALLOWED		24 // ���ײ�����
#define HXEMV_FLOW_ERROR			25 // EMV���̴���
#define HXEMV_CALLBACK_METHOD       26 // �ص���ǻص����Ľӿڵ��ô���
#define HXEMV_NOT_ACCEPTED          27 // ������

// �ֿ�����֤����
#define HXCVM_PLAIN_PIN             0x01 // �ѻ�����������֤
#define HXCVM_CIPHERED_OFFLINE_PIN  0x02 // �ѻ�����������֤(�ֲ�֧��)
#define HXCVM_CIPHERED_ONLINE_PIN   0x03 // ��������������֤
#define HXCVM_HOLDER_ID             0x04 // �ֿ���֤����֤
#define HXCVM_CONFIRM_AMOUNT        0x10 // �ǳֿ�����֤,������Ҫ��ȷ�Ͻ��
// �ֿ�����֤��������ʽ
#define HXCVM_PROC_OK               0x00 // �����������
#define HXCVM_BYPASS                0x01 // Ҫ�����������֤֤��ʱѡ����Bypass
#define HXCVM_FAIL                  0x02 // ֤����֤û��ͨ��
#define HXCVM_CANCEL                0x03 // ��ȡ��
#define HXCVM_TIMEOUT               0x04 // ��ʱ

// GAC��ƬӦ��
#define GAC_ACTION_TC				0x00 // ��׼(����TC)
#define GAC_ACTION_AAC				0x01 // �ܾ�(����AAC)
#define GAC_ACTION_AAC_ADVICE		0x02 // �ܾ�(����AAC,��Advice)
#define GAC_ACTION_ARQC				0x03 // Ҫ������(����ARQC)

// �ն�֧�ֵ�Ӧ���б�ṹ����
typedef struct {
    unsigned char ucAidLen;						// AID����
    unsigned char sAid[16];						// AID
	unsigned char ucASI;						// Ӧ��ѡ��ָʾ��, 0:��������ƥ�䣬1:ȫ������ƥ��
    signed char   cOnlinePinSupport;            // 1:��Aid֧���������� 0:��Aid��֧����������, -1��ʾ��

	// ���ϲ���ΪAIDר��, ���²���Ϊ���ն�ͨ�ò�����������
	unsigned char sTermAppVer[2];               // T9F09, �ն�Ӧ�ð汾��, "\xFF\xFF"��ʾ������
	unsigned long ulFloorLimit;					// T9F1B, �ն��޶�, ��λΪ��, 0xFFFFFFFF��ʾ������
    int			  iMaxTargetPercentage;         // ���ѡ�����ٷֱȣ�-1:������
    int			  iTargetPercentage;            // ���ѡ��Ŀ��ٷֱȣ�-1:������
	unsigned long ulThresholdValue;             // ���ѡ����ֵ, 0xFFFFFFFF��ʾ������
    unsigned char ucECashSupport;				// 1:֧�ֵ����ֽ� 0:��֧�ֵ����ֽ�, 0xFF��ʾ������
	unsigned char szTermECashTransLimit[12+1];  // T9F7B, �ն˵����ֽ����޶�, �ձ�ʾ������
	unsigned char ucTacDefaultExistFlag;        // 1:TacDefault����, 0:TacDefault������
    unsigned char sTacDefault[5];				// TAC-Default, �ο�TVR�ṹ
	unsigned char ucTacDenialExistFlag;         // 1:TacDenial����, 0:TacDenial������
    unsigned char sTacDenial[5];				// TAC-Denial, �ο�TVR�ṹ
	unsigned char ucTacOnlineExistFlag;         // 1:TacOnline����, 0:TacOnline������
    unsigned char sTacOnline[5];				// TAC-Online, �ο�TVR�ṹ
    int           iDefaultDDOLLen;              // Default DDOL����,-1��ʾ��
    unsigned char sDefaultDDOL[252];            // Default DDOL(TAG_DFXX_DefaultDDOL)
    int           iDefaultTDOLLen;              // Default TDOL����,-1��ʾ��
    unsigned char sDefaultTDOL[252];            // Default TDOL(TAG_DFXX_DefaultTDOL)
} stHxTermAid;

// �ն˲����ṹ����
typedef struct {
    unsigned char ucTermType;                   // T9F35, �ն�����, ��:0x21
    unsigned char sTermCapability[3];           // T9F33, �ն�����
    unsigned char sAdditionalTermCapability[5]; // T9F40, �ն�������չ
    unsigned char szMerchantId[15+1];           // T9F16, �̻���
    unsigned char szTermId[8+1];                // T9F1C, �ն˺�
    unsigned char szMerchantNameLocation[254+1];// T9F4E, �̻����ֵ�ַ, 0-254
    unsigned int  uiTermCountryCode;            // T9F1A, �ն˹��Ҵ���, 156=�й�
    unsigned char szAcquirerId[11+1];           // T9F01, �յ��б�ʶ��, 6-11
    int           iMerchantCategoryCode;		// T9F15, -1:�޴����� 0-9999:��Ч����
	unsigned char ucPinBypassBehavior;          // PIN bypass���� 0:ÿ��bypassֻ��ʾ�ô�bypass 1:һ��bypass,��������Ϊbypass
	unsigned char ucAppConfirmSupport;          // 1:֧��Ӧ��ȷ�� 0:��֧��Ӧ��ȷ��(TAG_DFXX_AppConfirmSupport)

	stHxTermAid   AidCommonPara;				// �ն�ͨ�ò�����AID��ز�����������
} stHxTermParam;

// �ն�֧�ֵ�CA��Կ�ṹ����
typedef struct {
    unsigned char ucKeyLen;                     // ��Կģ�����ȣ��ֽ�Ϊ��λ
    unsigned char sKey[248];                    // ��Կģ��
    long		  lE;                           // ��Կָ����3��65537
    unsigned char sRid[5];                      // �ù�Կ������RID
    unsigned char ucIndex;                      // ��Կ����
} stHxCaPublicKey;

// �ն��뿨Ƭ��֧�ֵ�Ӧ���б�ṹ����
typedef struct {
    unsigned char ucAdfNameLen;					// ADF���ֳ���
    unsigned char sAdfName[16];                 // ADF����
    unsigned char szLabel[16+1];                // Ӧ�ñ�ǩ
    int		      iPriority;                    // Ӧ�����ȼ�, -1��ʾ��Ӧ��û���ṩ
    unsigned char szLanguage[8+1];              // ����ָʾ, Ӧ����û�ṩ����Ϊ�մ�
    int			  iIssuerCodeTableIndex;        // �ַ������, -1��ʾ��Ӧ��û���ṩ
    unsigned char szPreferredName[16+1];        // Ӧ����ѡ����, Ӧ����û�ṩ����Ϊ�մ�
} stHxAdfInfo;

#ifdef __cplusplus
extern "C" {
#endif

// ��ȡ������Ϣ
// out : pszCoreName    : ��������, ������40�ֽ�
//       pszCoreDesc    : ��������, ������60�ֽ��ַ���
//       pszCoreVer     : ���İ汾��, ������10�ֽ��ַ���, ��"1.00"
//       pszReleaseDate : ���ķ�������, YYYY/MM/DD
//       pszCustomerDesc: �ͻ��ӿ�˵��, ������100�ֽ��ַ���
// ret : HXEMV_OK       : OK
int iHxEmvInfo(unsigned char *pszCoreName, unsigned char *pszCoreDesc, unsigned char *pszCoreVer, unsigned char *pszReleaseDate, unsigned char *pszCustomerDesc);

// ��ʼ������
// ��ʼ��������Ҫ����IC���������ָ��
// in  : pfiTestCard	: ��⿨Ƭ�Ƿ����
//                        ret : 0 : ������
//                              1 : ����
//       pfiResetCard   : ��Ƭ��λ
//                        ret : <=0 : ��λ����
//                              >0  : ��λ�ɹ�, ����ֵΪATR����
//       pfiDoApdu      : ִ��APDUָ��
//                        in  : iApduInLen   : Apduָ���
//                              psApduIn     : Apduָ��, ��ʽ: Cla Ins P1 P2 Lc DataIn Le
//                        out : piApduOutLen : ApduӦ�𳤶�
//                              psApduOut    : ApduӦ��, ��ʽ: DataOut Sw1 Sw2
//                        ret : 0            : �������ɹ�
//                              1            : ��������
//       pfiCloseCard   : �رտ�Ƭ
//                        ret : ������
// ret : HXEMV_OK       : OK
//       HXEMV_PARA     : ��������
//       HXEMV_CORE     : �ڲ�����
// Note: �ڵ����κ������ӿ�ǰ�������ȳ�ʼ������
int iHxEmvInit(int (*pfiTestCard)(void),
			   int (*pfiResetCard)(unsigned char *psAtr),
			   int (*pfiDoApdu)(int iApduInLen, unsigned char *psApduIn, int *piApduOutLen, unsigned char *psApduOut),
			   int (*pfiCloseCard)(void));

// �رտ�Ƭ
// ret : HXEMV_OK       : OK
int iHxEmvCloseCard(void);

// �����ն˲���
// in  : pHxTermParam      : �ն˲���
// ret : HXEMV_OK          : OK
//       HXEMV_PARA        : ��������
//       HXEMV_CORE        : �ڲ�����
//       HXEMV_FLOW_ERROR  : EMV���̴���
int iHxEmvSetParam(stHxTermParam *pHxTermParam);

// װ���ն�֧�ֵ�Aid
// in  : paHxTermAid       : �ն�֧�ֵ�Aid����
//       iHxTermAidNum     : �ն�֧�ֵ�Aid����
// ret : HXEMV_OK          : OK
//       HXEMV_LACK_MEMORY : �洢�ռ䲻��
//       HXEMV_PARA        : ��������
//       HXEMV_FLOW_ERROR  : EMV���̴���
int iHxEmvLoadAid(stHxTermAid *paHxTermAid, int iHxTermAidNum);

// װ��CA��Կ
// in  : paHxCaPublicKey   : CA��Կ
//       iHxCaPublicKeyNum : CA��Կ����
// ret : HXEMV_OK          : OK
//       HXEMV_LACK_MEMORY : �洢�ռ䲻��
//       HXEMV_PARA        : ��������
//       HXEMV_FLOW_ERROR  : EMV���̴���
int iHxEmvLoadCaPublicKey(stHxCaPublicKey *paHxCaPublicKey, int iHxCaPublicKeyNum);

// ���׳�ʼ��
// in  : iFlag             : =0, �����Ժ�ʹ��
// ret : HXEMV_OK          : OK
//       HXEMV_FLOW_ERROR  : EMV���̴���
//       HXEMV_CARD_REMOVED: ����ȡ��
//       HXEMV_NO_CARD     : ��Ƭ������
//       HXEMV_CARD_OP     : ��Ƭ��λ����
int iHxEmvTransInit(int iFlag);

// ��ȡ֧�ֵ�Ӧ��
// in  : iIgnoreBlock	   : !0:����Ӧ��������־, 0:������Ӧ�ò��ᱻѡ��
//       piAdfNum          : �����ɵ��ն��뿨Ƭͬʱ֧�ֵ�Adf����
// out : paHxAdfInfo       : �ն��뿨Ƭͬʱ֧�ֵ�Adf�б�(�����ȼ�����)
//       piHxAdfNum        : �ն��뿨Ƭͬʱ֧�ֵ�Adf����
// ret : HXEMV_OK          : OK, ��ȡ��Ӧ�ñ���ȷ��
//       HXEMV_AUTO_SELECT : OK, ��ȡ��Ӧ�ÿ��Զ�ѡ��
//       HXEMV_NO_APP      : ��֧�ֵ�Ӧ��
//       HXEMV_CARD_REMOVED: ����ȡ��
//       HXEMV_CARD_SW     : ��״̬�ַǷ�
//       HXEMV_CARD_OP     : ��������
//       HXEMV_TERMINATE   : ����ܾ�������������ֹ
//       HXEMV_FLOW_ERROR  : EMV���̴���
//       HXEMV_LACK_MEMORY : �洢�ռ䲻��
//       HXEMV_PARA        : ��������
int iHxEmvGetSupportedApp(int iIgnoreBlock, stHxAdfInfo *paHxAdfInfo, int *piHxAdfNum);

// Ӧ��ѡ��
// in  : iIgnoreBlock	     : !0:����Ӧ��������־, 0:������Ӧ�ò��ᱻѡ��
//       iAidLen             : AID����
//       psAid               : AID
// ret : HXEMV_OK            : OK
//       HXEMV_CARD_REMOVED  : ����ȡ��
//       HXEMV_CARD_OP       : ��������
//       HXEMV_TERMINATE     : ����ܾ�������������ֹ
//       HXEMV_FLOW_ERROR    : EMV���̴���
//       HXEMV_CORE          : �ڲ�����
//       HXEMV_RESELECT      : ��Ҫ����ѡ��Ӧ��
//       HXEMV_NOT_SUPPORTED : ѡ���˲�֧�ֵ�Ӧ��
int iHxEmvAppSelect(int iIgnoreBlock, int iAidLen, unsigned char *psAid);

// ��ȡ��Ƭ�ڲ�����
// in  : psTag             : ���ݱ�ǩ, ��:"\x9F\x79":�����ֽ����
//       piOutTlvDataLen   : psOutTlvData��������С
//       piOutDataLen      : psOutData��������С
// out : piOutTlvDataLen   : ���������ݳ���, ԭʼ��ʽ, ����Tag��Length��Value
//       psOutTlvData      : ����������, ����Tag��Length��Value
//       piOutDataLen      : ���������ݳ���, ������ʽ
//       psOutData         : ����������(N������:ת���ɿɶ����ִ�, CNѹ��������:ȥ��β��'F'������ִ�, A|AN|ANS|B|δ֪����:ԭ������),��B���ⷵ��ֵ�����ǿ�Ƽ�һ����β'\0', �ý������������ڷ��صĳ���֮��
//                           ע��, ����N3����������, ���ص����ݳ���ΪN4, ע����ջ���������Ҫ����
// ret : HXEMV_OK          : ��ȡ�ɹ�
//       HXEMV_NO_DATA     : �޴�����
//       HXEMV_LACK_MEMORY : ����������
//       HXEMV_CARD_REMOVED: ����ȡ��
//       HXEMV_CARD_OP     : ��������
//       HXEMV_CARD_SW     : �Ƿ���״̬��
//       HXEMV_FLOW_ERROR  : EMV���̴���
//       HXEMV_CORE        : �ڲ�����
// note: piOutTlvDataLen��psOutTlvData��һ������NULL���򲻻᷵�����������
//       piOutDataLen��psOutData��һ������NULL, �����᷵�����������
//       ����һ��������, ��ŵ�Tlv���ݿ���, �ٴ�ʹ�ÿ�����iHxEmvGetData()��ȡ
int iHxEmvGetCardNativeData(unsigned char *psTag, int *piOutTlvDataLen, unsigned char *psOutTlvData, int *piOutDataLen, unsigned char *psOutData);

// ��ȡӦ������
// in  : psTag             : ���ݱ�ǩ, ��:"\x82":Aip
//       piOutTlvDataLen   : psOutTlvData��������С
//       piOutDataLen      : psOutData��������С
// out : piOutTlvDataLen   : ���������ݳ���, ԭʼ��ʽ, ����Tag��Length��Value
//       psOutTlvData      : ����������, ����Tag��Length��Value
//       piOutDataLen      : ���������ݳ���, ������ʽ
//       psOutData         : ����������(N������:ת���ɿɶ����ִ�, CNѹ��������:ȥ��β��'F'������ִ�, A|AN|ANS|B|δ֪����:ԭ������),��B���ⷵ��ֵ�����ǿ�Ƽ�һ����β'\0', �ý������������ڷ��صĳ���֮��
//                           ע��, ����N3����������, ���ص����ݳ���ΪN4, ע����ջ���������Ҫ����
// ret : HXEMV_OK          : ��ȡ�ɹ�
//       HXEMV_NO_DATA     : �޴�����
//       HXEMV_LACK_MEMORY : ����������
//       HXEMV_FLOW_ERROR  : EMV���̴���
// note: piOutTlvDataLen��psOutTlvData��һ������NULL���򲻻᷵�����������
//       piOutDataLen��psOutData��һ������NULL, �����᷵�����������
int iHxEmvGetData(unsigned char *psTag, int *piOutTlvDataLen, unsigned char *psOutTlvData, int *piOutDataLen, unsigned char *psOutData);

// ����Ƭ������ˮ֧����Ϣ
// in  : iFlag             : 0:��׼������ˮ 1:Ȧ����ˮ
// out : piMaxRecNum       : ��ཻ����ˮ��¼����
// ret : HXEMV_OK          : ��ȡ�ɹ�
//       HXEMV_NO_LOG      : ��Ƭ��֧�ֽ�����ˮ��¼
//       HXEMV_PARA        : ��������
//       HXEMV_FLOW_ERROR  : EMV���̴���
int iHxEmvGetLogInfo(int iFlag, int *piMaxRecNum);

// ����Ƭ������ˮ
// in  : iFlag             : 0:��׼������ˮ 1:Ȧ����ˮ
//       iLogNo            : ������ˮ��¼��, �����һ����¼��Ϊ1
//       piLogLen          : psLog��������С
// out : piLogLen          : ������ˮ��¼����
//       psLog             : ��¼����(IC��ԭ��ʽ���)
// ret : HXEMV_OK          : ��ȡ�ɹ�
//       HXEMV_NO_RECORD   : �޴˼�¼
//       HXEMV_LACK_MEMORY : ����������
//       HXEMV_CARD_REMOVED: ����ȡ��
//       HXEMV_CARD_OP     : ��������
//       HXEMV_CARD_SW     : �Ƿ���ָ��״̬��
//       HXEMV_NO_LOG      : ��Ƭ��֧�ֽ�����ˮ��¼
//       HXEMV_PARA        : ��������
//       HXEMV_FLOW_ERROR  : EMV���̴���
int iHxEmvReadLog(int iFlag, int iLogNo, int *piLogLen, unsigned char *psLog);

// GPO
// in  : pszTransTime      : ����ʱ�䣬YYYYMMDDhhmmss
//       ulTTC             : �ն˽������, 1-999999
//       ucTransType       : ��������,������ǰ2λ��ʾ�Ľ��ڽ�������
//                           �����ֽ��ֵ63��ʾΪ0x63
//       pszAmount         : ���׽��, ascii�봮, ��ҪС����, Ĭ��ΪȱʡС����λ��, ����:RMB1.00��ʾΪ"100"
//       uiCurrencyCode    : ���Ҵ���, 156=�����
// ret : HXEMV_OK          : OK
//       HXEMV_CARD_REMOVED: ����ȡ��
//       HXEMV_CARD_OP     : ��������
//       HXEMV_CARD_SW     : �Ƿ���״̬��
//       HXEMV_TERMINATE   : ����ܾ�������������ֹ
//       HXEMV_RESELECT    : ��Ҫ����ѡ��Ӧ��
//       HXEMV_TRANS_NOT_ALLOWED : ���ײ�֧��
//       HXEMV_PARA        : ��������
//       HXEMV_FLOW_ERROR  : EMV���̴���
//       HXEMV_CORE        : �ڲ�����
// Note: �������HXEMV_RESELECT����iHxEmvGetSupportedApp()��ʼ����ִ������
int iHxEmvGPO(unsigned char *pszTransTime, unsigned long ulTTC, unsigned char ucTransType, unsigned char *pszAmount, unsigned int uiCurrencyCode);

// ��������´�����
// in  : pszAmount         : ���
// ret : HXEMV_OK          : ���óɹ�
//       HXEMV_FLOW_ERROR  : EMV���̴���
//       HXEMV_PARA        : ��������
// note: GPO��GAC�����ܻ᲻ͬ, �����趨���׽��
//       GPO��, GAC1ǰ�ſ��Ե���
int iHxEmvSetAmount(unsigned char *pszAmount);

// ��Ӧ�ü�¼
// ret : HXEMV_OK          : OK
//       HXEMV_CARD_REMOVED: ����ȡ��
//       HXEMV_CARD_OP     : ��������
//       HXEMV_CARD_SW     : �Ƿ���ָ��״̬��
//       HXEMV_TERMINATE   : ����ܾ�������������ֹ
//       HXEMV_FLOW_ERROR  : EMV���̴���
//       HXEMV_CORE        : �ڲ�����
int iHxEmvReadRecord(void);

// ���ÿ�Ƭ���������ֿ����ѱ�־
// in  : iBlackFlag        : ���øÿ��Ƿ�Ϊ��������, 0:��Ϊ�������� 1:��������
//       iSeparateFlag     : ���øÿ��ۼ����ѳ��޶�, 0:û���޶� 1:���޶�
// ret : HXEMV_OK          : OK
//       HXEMV_CORE        : �ڲ�����
// Note: �������¼���ֿ�Ƭ�Ǻ����������ۼ����ѳ��޶�, ����Ҫ���ô˺��������˻���Ϣ
//       �����֧�ֻ��鲻�����ñ��, ����Ҫ���ñ��ӿ�
int iHxEmvSetPanFlag(int iBlackFlag, int iSeparateFlag);

// SDA��DDA
// out : piNeedCheckCrlFlag  : �Ƿ���Ҫ�жϷ����й�Կ֤��Ϊ��������־, 0:����Ҫ�ж� 1:��Ҫ�ж�
//       psRid               : RID[5], psRid+pucCaIndex+psCertSerNo�������ṩ��Ӧ�ò��жϷ����й�Կ֤���Ƿ��ں������б���. �κ�һ��ָ�봫��Ϊ�ն���ʾ�߲㲻��Ҫ
//       pucCaIndex          : CA��Կ����
//       psCertSerNo         : �����й�Կ֤�����к�[3]
// ret : HXEMV_OK            : OK
//       HXEMV_CARD_REMOVED  : ����ȡ��
//       HXEMV_CARD_OP       : ��������
//       HXEMV_CARD_SW       : �Ƿ���״̬��
//       HXEMV_TERMINATE     : ����ܾ�������������ֹ
//       HXEMV_DENIAL        : �ն���Ϊ�������Ϊ�ѻ��ܾ�
//       HXEMV_DENIAL_ADVICE : �ն���Ϊ�������Ϊ�ѻ��ܾ�, ��Advice
//       HXEMV_FLOW_ERROR    : EMV���̴���
//       HXEMV_CORE          : �ڲ�����
// Note: Ӧ�ò�֧�ַ����й�Կ֤����������ʱ, piNeedCheckCrlFlag��������, ������Ժ���
int iHxEmvOfflineDataAuth(int *piNeedCheckCrlFlag, unsigned char *psRid, unsigned char *pucCaIndex, unsigned char *psCertSerNo);

// ���÷����й�Կ֤��Ϊ������
// in  : iIssuerCertCrlFlag  : �Ƿ��ں������б��־, 0:���ں������б� 1:�ں������б�
// ret : HXEMV_OK            : OK
//       HXEMV_CARD_REMOVED  : ����ȡ��
//       HXEMV_CARD_OP       : ��������
//       HXEMV_CARD_SW       : �Ƿ���״̬��
//       HXEMV_TERMINATE     : ����ܾ�������������ֹ
//       HXEMV_DENIAL        : �ն���Ϊ�������Ϊ�ѻ��ܾ�
//       HXEMV_DENIAL_ADVICE : �ն���Ϊ�������Ϊ�ѻ��ܾ�, ��Advice
//       HXEMV_FLOW_ERROR    : EMV���̴���
//       HXEMV_CORE          : �ڲ�����
// Note: Ӧ�ò����iHxEmvOfflineDataAuth()��õ�Rid+CaIndex+CertSerNo
//       ���ݴ������жϷ����й�Կ֤���Ƿ�Ϊ������,ֻ���Ǻ�����,����Ҫ���ñ�����֪ͨ����
int iHxEmvSetIssuerCertCrl(int iIssuerCertCrlFlag);

// ��������
// ret : HXEMV_OK            : OK
//       HXEMV_TERMINATE     : ����ܾ�������������ֹ
//       HXEMV_FLOW_ERROR    : EMV���̴���
//       HXEMV_CORE          : �ڲ�����
//       HXEMV_CARD_REMOVED  : ����ȡ��
//       HXEMV_DENIAL        : �ն���Ϊ�������Ϊ�ѻ��ܾ�
//       HXEMV_DENIAL_ADVICE : �ն���Ϊ�������Ϊ�ѻ��ܾ�, ��Advice
//       HXEMV_CARD_OP       : �ն���Ϊ�������Ϊ�ѻ��ܾ�, ����AACʱ��Ƭ����
int iHxEmvProcRistrictions(void);

// �ն˷��չ���
// ret : HXEMV_OK            : OK
//       HXEMV_TERMINATE     : ����ܾ�������������ֹ
//       HXEMV_CORE          : ��������
//       HXEMV_CARD_REMOVED  : ����ȡ��
//       HXEMV_CARD_OP       : ��������
//       HXEMV_DENIAL        : �ն���Ϊ�������Ϊ�ѻ��ܾ�
//       HXEMV_DENIAL_ADVICE : �ն���Ϊ�������Ϊ�ѻ��ܾ�, ��Advice
//       HXEMV_FLOW_ERROR    : EMV���̴���
int iHxEmvTermRiskManage(void);

// ��ȡ�ֿ�����֤
// out : piCvm               : �ֿ�����֤����, ֧��HXCVM_PLAIN_PIN��HXCVM_CIPHERED_ONLINE_PIN��HXCVM_HOLDER_ID��HXCVM_CONFIRM_AMOUNT
//       piBypassFlag        : ����bypass��־, 0:������, 1:����
// ret : HXEMV_OK            : OK
//       HXEMV_NO_DATA       : ����������гֿ�����֤
//       HXEMV_CARD_REMOVED  : ����ȡ��
//       HXEMV_CARD_OP       : ��������
//       HXEMV_TERMINATE     : ����ܾ�������������ֹ
//       HXEMV_DENIAL        : �ն���Ϊ�������Ϊ�ѻ��ܾ�
//       HXEMV_DENIAL_ADVICE : �ն���Ϊ�������Ϊ�ѻ��ܾ�, ��Advice
//       HXEMV_FLOW_ERROR    : EMV���̴���
//       HXEMV_CORE          : �ڲ�����
// Note: ��iHxEmvDoCvmMethod()����, ������Ҫ��ε���, ֱ������HXEMV_NO_DATA
int iHxEmvGetCvmMethod(int *piCvm, int *piBypassFlag);

// ִ�гֿ�����֤
// in  : iCvmProc            : ��֤��������ʽ, HXCVM_PROC_OK or HXCVM_BYPASS or HXCVM_FAIL or HXCVM_CANCEL or HXCVM_TIMEOUT
//       psCvmData           : ���������, ���Ϊ��������, ����β��Ҫ��0
// out : piPrompt            : ������ʾ��Ϣ, 0:�޶�����ʾ��Ϣ 1:�����,������ 2:�����,�������� 3:�ѻ�������֤�ɹ�
// ret : HXEMV_OK            : OK, ��Ҫ�������гֿ�����֤, ��������iHxGetCvmMethod2(), Ȼ���ٵ��ñ�����
//       HXEMV_PARA		     : ��������
//       HXEMV_CANCEL        : �û�ȡ��
//       HXEMV_TIMEOUT       : ��ʱ
//       HXEMV_CARD_REMOVED  : ����ȡ��
//       HXEMV_CARD_OP       : ��������
//       HXEMV_CARD_SW       : �Ƿ���״̬��
//       HXEMV_TERMINATE     : ����ܾ�������������ֹ
//       HXEMV_DENIAL        : �ն���Ϊ�������Ϊ�ѻ��ܾ�
//       HXEMV_DENIAL_ADVICE : �ն���Ϊ�������Ϊ�ѻ��ܾ�, ��Advice
//       HXEMV_FLOW_ERROR    : EMV���̴���
//       HXEMV_CORE          : �ڲ�����
// Note: ִ�е�CVM���������һ��iHxEmvGetCvmMethod()��õ�CVM
int iHxEmvDoCvmMethod(int iCvmProc, unsigned char *psCvmData, int *piPrompt);

// ��ȡCVM��֤����ǩ�ֱ�־
// out : piNeedSignFlag    : ��ʾ��Ҫǩ�ֱ�־��0:����Ҫǩ�� 1:��Ҫǩ��
// ret : HXEMV_OK          : OK
int iHxEmvGetCvmSignFlag(int *piNeedSignFlag);

// �ն���Ϊ����
// ret : HXEMV_OK            : OK
//       HXEMV_CORE          : ��������
//       HXEMV_FLOW_ERROR    : EMV���̴���
//       HXEMV_CARD_REMOVED  : ����ȡ��
//       HXEMV_DENIAL        : �ն���Ϊ�������Ϊ�ѻ��ܾ�
//       HXEMV_DENIAL_ADVICE : �ն���Ϊ�������Ϊ�ѻ��ܾ�, ��Advice
int iHxEmvTermActionAnalysis(void);

// ��һ��GAC
// in  : ucForcedOnline    : 1:�趨ǿ��������־ 0:���趨ǿ��������־
// out : piCardAction      : ��Ƭִ�н��
//								GAC_ACTION_TC     : ��׼(����TC)
//								GAC_ACTION_AAC    : �ܾ�(����AAC)
//								GAC_ACTION_AAC_ADVICE : �ܾ�(����AAC,��Advice)
//								GAC_ACTION_ARQC   : Ҫ������(����ARQC)
// ret : HXEMV_OK          : OK
//       HXEMV_CORE        : ��������
//       HXEMV_CARD_REMOVED: ����ȡ��
//       HXEMV_CARD_OP     : ��������
//       HXEMV_CARD_SW     : �Ƿ�״̬��
//       HXEMV_TERMINATE   : ����ܾ�������������ֹ
//       HXEMV_NOT_ALLOWED : ��������
//       HXEMV_NOT_ACCEPTED: ������
//       HXEMV_FLOW_ERROR  : EMV���̴���
int iHxEmvGac1(unsigned char ucForcedOnline , int *piCardAction);

// �ڶ���GAC
// in  : pszArc            : Ӧ����[2], NULL��""��ʾ����ʧ��
//       pszAuthCode	       : ��Ȩ��[6], NULL��""��ʾ����Ȩ������
//       psOnlineData      : ������Ӧ����(55������),һ��TLV��ʽ����, NULL��ʾ��������Ӧ����
//       psOnlineDataLen   : ������Ӧ���ݳ���
// out : piCardAction      : ��Ƭִ�н��
//                           GAC_ACTION_TC     : ��׼(����TC)
//							     GAC_ACTION_AAC    : �ܾ�(����AAC)
//                           GAC_ACTION_AAC_ADVICE : �ܾ�(����AAC,��Advice)
// ret : HXEMV_OK          : OK
//       HXEMV_CORE        : ��������
//       HXEMV_CARD_REMOVED: ����ȡ��
//       HXEMV_CARD_OP     : ��������
//       HXEMV_CARD_SW     : �Ƿ���״̬��
//       HXEMV_TERMINATE   : ����ܾ�������������ֹ
//       HXEMV_NOT_ALLOWED : ��������
//       HXEMV_NOT_ACCEPTED: ������
//       HXEMV_FLOW_ERROR  : EMV���̴���
int iHxEmvGac2 (unsigned char *pszArc, unsigned char *pszAuthCode, unsigned char *psIssuerData, int iIssuerDataLen, int *piCardAction);

#ifdef __cplusplus
}
#endif

#endif
