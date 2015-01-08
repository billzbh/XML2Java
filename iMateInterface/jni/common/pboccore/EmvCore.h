/**************************************
File name     : EmvCore.h
Function      : EMV/pboc2.0����Ǻ��Ľӿ�
Author        : Yu Jun
First edition : Mar 31st, 2012
Modified      : Sep 5th, 2013
				����iHxSetAmount()����, ����ʵ��GAC��GPO��ͬ
                Apr 2nd, 2014
					iHxSetAmount()���������ڶ���¼��, ��������ǰ����
				Apr 16th, 2014
				    iHxGetCardNativeData()��iHxGetData()����������δ֪Tagʱ��b�ʹ���
				Apr 21st, 2014
				    ����iHxCoreInit()�ӿ�, ����һ������iAppType
					���ڱ������ͼ������ʵ��Ӧ�ó���
**************************************/
#ifndef _EMVCORE_H
#define _EMVCORE_H

// EMV���ĵ�������
#define HXCORE_CALLBACK				1  // ָ��ʹ�ûص��ӿ�
#define HXCORE_NOT_CALLBACK         2  // ָ��ʹ�÷ǻص��ӿ�

// EMV���ķ�����
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
#define HXEMV_TRY_OTHER_INTERFACE	28 // ��������ͨ�Ž���
#define HXEMV_CARD_TRY_AGAIN		29 // Ҫ�������ύ��Ƭ(�ǽ�)
#define HXEMV_EXPIRED_APP			30 // ���ڿ�(�ǽ�)
#define HXEMV_FDDA_FAIL				31 // fDDAʧ��(�ǽ�)

// �������Ͷ���
// ��λΪ������, ��Tag9C����
#define TRANS_TYPE_SALE				0x0000 // ����
#define TRANS_TYPE_GOODS			0x0100 // ��Ʒ
#define TRANS_TYPE_SERVICES			0x0200 // ����
#define TRANS_TYPE_CASH				0x0001 // ȡ��
#define TRANS_TYPE_RETURN			0x0020 // �˻�
#define TRANS_TYPE_DEPOSIT			0x0021 // ���
#define TRANS_TYPE_AVAILABLE_INQ	0x0030 // ��������ѯ
#define TRANS_TYPE_BALANCE_INQ		0x0031 // ����ѯ
#define TRANS_TYPE_TRANSFER			0x0040 // ת��
#define TRANS_TYPE_PAYMENT			0x0050 // ֧��
#define TRANS_TYPE_FIXEDACC_LOAD    0x0060 // ָ���˻��ֽ��ֵ
#define TRANS_TYPE_NOFIXEDACC_LOAD  0x0062 // ��ָ���˻��ֽ��ֵ
#define TRANS_TYPE_CASH_LOAD        0x0063 // �ֽ��ֵ

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

// ��Ϣ����������
#define HXMSG_LOCAL_LANG  0 // ��������
#define HXMSG_OPERATOR    1 // ����Ա
#define HXMSG_CARDHOLDER  2 // �ֿ���
#define HXMSG_SMART       3 // ����ѡ��, ����ֵ��->����Ա ����ֵ��->�ֿ���

// ��������
#define HXEMV_PARA_INIT				0x00 // ��ʼ��
#define HXEMV_PARA_ADD				0x01 // ����һ����Ŀ
#define HXEMV_PARA_DEL				0x02 // ɾ��һ����Ŀ

// �ն�֧�ֵ�Ӧ���б�ṹ����
typedef struct {
    uchar ucAidLen;                     // AID����
    uchar sAid[16];                     // AID
    uchar ucASI;                        // Ӧ��ѡ��ָʾ��, 0:��������ƥ�䣬1:ȫ������ƥ��
	// ���ϲ���ΪAIDר��, ���²���Ϊ���ն�ͨ�ò�����������
	uchar ucTermAppVerExistFlag;        // TermAppVer���ڱ�־, 0:�� 1:����
	uchar sTermAppVer[2];               // T9F09, �ն�Ӧ�ð汾��
    int   iDefaultDDOLLen;              // TDFxx, Default DDOL����,-1��ʾ��
    uchar sDefaultDDOL[252];            // TDFxx, Default DDOL(TAG_DFXX_DefaultDDOL)
    int   iDefaultTDOLLen;              // TDFxx, Default TDOL����,-1��ʾ��
    uchar sDefaultTDOL[252];            // TDFxx, Default TDOL(TAG_DFXX_DefaultTDOL)
    int   iMaxTargetPercentage;         // TDFxx, Maximum Target Percentage to be used for Biased Random Selection��-1:�޴�����(TAG_DFXX_MaxTargetPercentage)
    int   iTargetPercentage;            // TDFxx, Target Percentage to be used for Random Selection��-1:�޴�����(TAG_DFXX_TargetPercentage)
	uchar ucFloorLimitExistFlag;        // FloorLimit���ڱ�־, 0:�� 1:����
	ulong ulFloorLimit;                 // T9F1B, Terminal Floor Limit
	uchar ucThresholdExistFlag;         // Threshold���ڱ�־, 0:�� 1:����
	ulong ulThresholdValue;             // TDFxx, Threshold Value for Biased Random Selection
	uchar ucTacDefaultExistFlag;        // TacDefault���ڱ�־, 0:�� 1:����
    uchar sTacDefault[5];               // TDFxx, TAC-Default(TAG_DFXX_TACDefault)
	uchar ucTacDenialExistFlag;         // TacDenial���ڱ�־, 0:�� 1:����
    uchar sTacDenial[5];                // TDFxx, TAC-Denial(TAG_DFXX_TACDenial)
	uchar ucTacOnlineExistFlag;         // TacOnline���ڱ�־, 0:�� 1:����
    uchar sTacOnline[5];                // TDFxx, TAC-Online(TAG_DFXX_TACOnline)
    signed char cForcedOnlineSupport;   // TDFxx, 1:֧�� 0:��֧��(TAG_DFXX_ForceOnlineSupport), -1��ʾ��
    signed char cForcedAcceptanceSupport;     // TDFxx, 1:֧�� 0:��֧��(TAG_DFXX_ForceAcceptSupport), -1��ʾ��
    signed char cOnlinePinSupport;      // TDFxx, 1:֧�� 0:��֧��(TAG_DFXX_OnlinePINSupport), -1��ʾ��
    uchar ucTermRiskDataLen;            // T9F1D, Length of Terminal Risk Management Data��0:�޴�����
    uchar sTermRiskData[8];             // T9F1D, Terminal Risk Management Data
    signed char cECashSupport;          // TDFxx, 1:֧�ֵ����ֽ� 0:��֧�ֵ����ֽ�(TAG_DFXX_ECTermSupportIndicator), -1��ʾ��
										//        ע:GPO��ҪT9F7A��ָʾ, ���׿�ʼ�����жϷ��ϵ����ֽ�֧���������, �趨T9F7A��ֵ
    uchar szTermECashTransLimit[12+1];  // T9F7B, �ն˵����ֽ����޶�, �ձ�ʾ��
} stTermAid;

// �ն˲����ṹ����
typedef struct {
    uchar ucTermType;                   // T9F35, �ն�����
    uchar sTermCapability[3];           // T9F33, �ն�����
    uchar sAdditionalTermCapability[5]; // T9F40, �ն�������չ
    uchar ucReaderCapability;           // TDFxx, 1:ֻ֧�ִſ� 2:ֻ֧��IC�� 3:֧�����ʽMag/IC������ 4:֧�ַ���ʽMag/IC������(TAG_DFXX_ReaderCapability)
    uchar ucVoiceReferralSupport;       // TDFxx, 1:֧�� 2:��֧��,ȱʡapprove 3:��֧��,ȱʡdecline(TAG_DFXX_VoiceReferralSupport)
	uchar ucPinBypassBehavior;          // TDFxx, PIN bypass���� 0:ÿ��bypassֻ��ʾ�ô�bypass 1:һ��bypass,��������Ϊbypass
    uchar szMerchantId[15+1];           // T9F16, Merchant Identifier
    uchar szTermId[8+1];                // T9F1C, Terminal Identification
    uchar szIFDSerialNo[8+1];           // T9F1E, IFD Serial Number
    uchar szMerchantNameLocation[254+1];// T9F4E, Merchant Name and Location
    int   iMerchantCategoryCode;		// T9F15, -1:�޴����� 0-9999:��Ч����
    int   iTermCountryCode;             // T9F1A, �ն˹��Ҵ���
	int   iTermCurrencyCode;            // T5F2A, �ն˽��׻��Ҵ���
    uchar szAcquirerId[11+1];           // T9F01, Acquirer Identifier

    uchar ucTransLogSupport;            // TDFxx, 1:�ն�֧�ֽ�����ˮ��¼ 0:��֧��(TAG_DFXX_TransLogSupport)
    uchar ucBlackListSupport;           // TDFxx, 1:�ն�֧�ֺ�������� 0:��֧��(TAG_DFXX_BlacklistSupport)
    uchar ucSeparateSaleSupport;        // TDFxx, 1:֧�ַֿ����Ѽ�� 0:��֧��(TAG_DFXX_SeparateSaleSupport)
	uchar ucAppConfirmSupport;          // TDFxx, 1:֧��Ӧ��ȷ�� 0:��֧��Ӧ��ȷ��(TAG_DFXX_AppConfirmSupport)
	uchar szLocalLanguage[3];           // TDFxx, ISO-639-1, ��������(TAG_DFXX_LocalLanguage)
	uchar szPinpadLanguage[32+1];       // TDFxx, ISO-639-1, pinpad֧�ֵ�����(TAG_DFXX_PinpadLanguage), �����Ծ����ַ���

	// ����4������Ϊ�ǽ�ר�в���
	uchar sTermCtlsCapability[4];		// TDFxx, �ն˷ǽ�����, T9F66 Markλ, ���յ�T9F66ֵҪ��ò����������
	uchar szTermCtlsAmountLimit[12+1];	// TDFxx, �ն˷ǽӽ����޶�, �ձ�ʾ��
	uchar szTermCtlsOfflineAmountLimit[12+1];	// TDFxx, �ն˷ǽ��ѻ������޶�, �ձ�ʾ��
	uchar szTermCtlsCvmLimit[12+1];		// TDFxx, �ն˷ǽ�CVM�޶�, �ձ�ʾ��

	stTermAid AidCommonPara;			// �ն�ͨ�ò�����AID��ز�����������
} stTermParam;

// �ն�֧�ֵ�CA��Կ�ṹ����
typedef struct {
    uchar ucKeyLen;                     // ��Կģ�����ȣ��ֽ�Ϊ��λ
    uchar sKey[248];                    // ��Կģ��
    long  lE;                           // ��Կָ����3��65537
    uchar sRid[5];                      // �ù�Կ������RID
    uchar ucIndex;                      // ��Կ����
    uchar szExpireDate[8+1];            // ��Ч�ڣ�YYYYMMDD
	uchar sHashCode[20];                // ��Կhashֵ
} stCAPublicKey;

// �ն��뿨Ƭ��֧�ֵ�Ӧ���б�ṹ����
typedef struct {
    uchar ucAdfNameLen;					// ADF���ֳ���
    uchar sAdfName[16];                 // ADF����
    uchar szLabel[16+1];                // Ӧ�ñ�ǩ
    int   iPriority;                    // Ӧ�����ȼ�, -1��ʾ��Ӧ��û���ṩ
    uchar szLanguage[8+1];              // ����ָʾ, Ӧ����û�ṩ����Ϊ�մ�
    int   iIssuerCodeTableIndex;        // �ַ������, -1��ʾ��Ӧ��û���ṩ
    uchar szPreferredName[16+1];        // Ӧ����ѡ����, Ӧ����û�ṩ����Ϊ�մ�
} stADFInfo;

// ��ȡ������Ϣ
// out : pszCoreName    : ��������, ������40�ֽ�
//       pszCoreDesc    : ��������, ������60�ֽ��ַ���
//       pszCoreVer     : ���İ汾��, ������10�ֽ��ַ���, ��"1.00"
//       pszReleaseDate : ���ķ�������, YYYY/MM/DD
// ret : HXEMV_OK       : OK
int iHxCoreInfo(uchar *pszCoreName, uchar *pszCoreDesc, uchar *pszCoreVer, uchar *pszReleaseDate);

// ��ʼ������
// in  : iCallBackFlag      : ָ�����ĵ��÷�ʽ
//                            HXCORE_CALLBACK     : ָ��ʹ�ûص��ӿ�
//                            HXCORE_NOT_CALLBACK : ָ��ʹ�÷ǻص��ӿ�
//		 iAppType           : Ӧ�ó����־, 0:�ͼ����(�ϸ����ع淶) 1:ʵ��Ӧ�ó���
//							      ����: 1. �ͼ����ֻ֧��ͨ��L2���Ľ���, Ӧ�ó���֧�����н���
//       ulRandSeed         : ��������ӣ����������ڲ������������
//       pszDefaultLanguage : ȱʡ����, ��ǰ֧��zh:���� en:Ӣ��
// ret : HXEMV_OK           : OK
//       HXEMV_PARA         : ��������
//       HXEMV_CORE         : �ڲ�����
// Note: �ڵ����κ������ӿ�ǰ�������ȳ�ʼ������
int iHxCoreInit(int iCallBackFlag, int iAppType, ulong ulRandSeed, uchar *pszDefaultLanguage);

// �����ն˲���
// in  : pTermParam        : �ն˲���
// out : pszErrTag         : ������ó���, �����Tagֵ
// ret : HXEMV_OK          : OK
//       HXEMV_PARA        : ��������
//       HXEMV_CORE        : �ڲ�����
//       HXEMV_FLOW_ERROR  : EMV���̴���
int iHxSetTermParam(stTermParam *pTermParam, uchar *pszErrTag);

// װ���ն�֧�ֵ�Aid
// in  : pTermAid          : �ն�֧�ֵ�Aid
//       iFlag             : ����, HXEMV_PARA_INIT:��ʼ�� HXEMV_PARA_ADD:���� HXEMV_PARA_DEL:ɾ��
// out : pszErrTag         : ������ó���, �����Tagֵ
// ret : HXEMV_OK          : OK
//       HXEMV_LACK_MEMORY : �洢�ռ䲻��
//       HXEMV_PARA        : ��������
//       HXEMV_FLOW_ERROR  : EMV���̴���
// Note: ɾ��ʱֻ����AID����
int iHxLoadTermAid(stTermAid *pTermAid, int iFlag, uchar *pszErrTag);

// װ��CA��Կ
// in  : pCAPublicKey      : CA��Կ
//       iFlag             : ����, HXEMV_PARA_INIT:��ʼ�� HXEMV_PARA_ADD:���� HXEMV_PARA_DEL:ɾ��
// ret : HXEMV_OK          : OK
//       HXEMV_LACK_MEMORY : �洢�ռ䲻��
//       HXEMV_PARA        : ��������
//       HXEMV_FLOW_ERROR  : EMV���̴���
// Note: ɾ��ʱֻ����RID��index����
int iHxLoadCAPublicKey(stCAPublicKey *pCAPublicKey, int iFlag);

// ����IC��������
// in  : iSlotNo : �����ţ�VPOS�淶
// ret : HXEMV_OK       : OK
//       HXEMV_NO_SLOT  : ��֧�ִ˿���
int iHxSetIccSlot(int iSlotNo);

// ���÷ǽ�IC��������
// in  : iSlotNo : �����ţ�VPOS�淶
// ret : HXEMV_OK       : OK
//       HXEMV_NO_SLOT  : ��֧�ִ˿���
int iHxSetCtlsIccSlot(int iSlotNo);

// ���׳�ʼ��, ÿ���½��׿�ʼ����һ��
// ret : HXEMV_OK          : OK
//       HXEMV_FLOW_ERROR  : EMV���̴���
int iHxTransInit(void);

// �ǽӴ�������Ԥ����
// in  : pszAmount					: ���׽��, ascii�봮, ��ҪС����, Ĭ��ΪȱʡС����λ��, ����:RMB1.00��ʾΪ"100"
//       uiCurrencyCode				: ���Ҵ���
// ret : HXEMV_OK					: OK
//		 HXEMV_TRY_OTHER_INTERFACE	: ����ܾ�������������ֹ, ��������ͨ�Ž���
//									  Ӧ��ʾEMVMSG_TERMINATE��Ϣ, ����ն�֧������ͨ�Ž���, ��ʾ�û���������ͨ�Ž���
//       HXEMV_CORE					: �ڲ�����
// Note: ÿ�ʷǽӴ�����ǰ��������Ԥ����, ֻ�гɹ������Ԥ����ſ��Խ��зǽӽ���
//       Ӧ�ò�Ҫȷ������������Ԥ��������һ��
//       �ο�: JR/T0025.12��2013, 6.2, p9
int iHxCtlsPreProc(uchar *pszAmount, uint uiCurrencyCode);

// ��⿨Ƭ
// ret : HXEMV_OK          : OK
//       HXEMV_NO_CARD     : ��Ƭ������
int iHxTestCard(void);

// ���ǽӿ�Ƭ
// ret : HXEMV_OK          : OK
//       HXEMV_NO_CARD     : ��Ƭ������
int iHxTestCtlsCard(void);

// �رտ�Ƭ
// ret : HXEMV_OK          : OK
int iHxCloseCard(void);

// ǿ�������趨,�趨TVRǿ������λ
// in  : iFlag             : �趨TVRǿ������λ��־ 0:���趨 1:�趨
// ret : HXEMV_OK          : OK
//       HXEMV_FLOW_ERROR  : EMV���̴���
// Note: ������ȷ����������Ҫ��TAC/IAC-Onlineǿ������λ������
//       �淶�涨:����������ֵ��,���ڽ��׿�ʼ��ʱ�����ǿ����������(emv2008 book4 6.5.3, P56)
//       �����Ĳ�֧�ֲ���Ա�����ǿ������, �ο�������ƾ��, ����ǿ������λ��ǿ���趨, 
//       Ϊ�˽������ܱ�Ҫ���趨ǿ������λ, ���ṩ�˺���, ������������Ƿ�������ֵ��
int iHxSetForceOnlineFlag(int iFlag);

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
//       ����һ��������, ��ŵ�Tlv���ݿ���, �ٴ�ʹ�ÿ�����iHxGetData()��ȡ
//       ��������ʾ������Ϣ
int iHxGetCardNativeData(uchar *psTag, int *piOutTlvDataLen, uchar *psOutTlvData, int *piOutDataLen, uchar *psOutData);

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
int iHxGetData(uchar *psTag, int *piOutTlvDataLen, uchar *psOutTlvData, int *piOutDataLen, uchar *psOutData);

// ����Ƭ������ˮ֧����Ϣ
// out : piMaxRecNum       : ��ཻ����ˮ��¼����
// ret : HXEMV_OK          : ��ȡ�ɹ�
//       HXEMV_NO_LOG      : ��Ƭ��֧�ֽ�����ˮ��¼
//       HXEMV_FLOW_ERROR  : EMV���̴���
int iHxGetLogInfo(int *piMaxRecNum);

// ����Ƭ������ˮ
// in  : iLogNo            : ������ˮ��¼��, �����һ����¼��Ϊ1
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
//       HXEMV_FLOW_ERROR  : EMV���̴���
// note: ��������ʾ����
int iHxReadLog(int iLogNo, int *piLogLen, uchar *psLog);

// ����ƬȦ�潻����ˮ֧����Ϣ
// out : piMaxRecNum       : ��ཻ����ˮ��¼����
// ret : HXEMV_OK          : ��ȡ�ɹ�
//       HXEMV_NO_LOG      : ��Ƭ��֧��Ȧ����ˮ��¼
//       HXEMV_FLOW_ERROR  : EMV���̴���
int iHxGetLoadLogInfo(int *piMaxRecNum);

// ��Ȧ�潻����ˮ
// in  : iLogNo            : ������ˮ��¼��, �����һ����¼��Ϊ1, Ҫȫ��������¼��Ϊ0
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
//       HXEMV_FLOW_ERROR  : EMV���̴���
// note: ��������ʾ����
int iHxReadLoadLog(int iLogNo, int *piLogLen, uchar *psLog);

// �ǽӴ���GPO�������
// out : piTransRoute               : �����ߵ�·�� 0:�Ӵ�Pboc(��׼DC��EC) 1:�ǽӴ�Pboc(��׼DC��EC) 2:qPboc���� 3:qPboc�ѻ� 4:qPboc�ܾ�
//       piSignFlag					: ��Ҫǩ�ֱ�־(0:����Ҫǩ��, 1:��Ҫǩ��), �����ߵ���qPboc����ʱ����Ч
//       piNeedOnlinePin            : ��Ҫ���������־(0:����Ҫ��������, 1:��Ҫ��������), �����ߵ���qPboc��������ʱ����Ч
// ret : HXEMV_OK					: OK, ����piTransRoute������������
//       HXEMV_TERMINATE			: ����ܾ�������������ֹ
//		 HXEMV_TRY_OTHER_INTERFACE	: ��������ͨ�Ž���
//       HXEMV_CORE					: �ڲ�����
// Note: GPO�ɹ���ſ��Ե���
// �ο� JR/T0025.12��2013, 7.8 P40
int iHxCtlsGpoAnalyse(int *piTransRoute, int *piSignFlag, int *piNeedOnlinePin);

// ��������´�����
// in  : pszAmount         : ���
//       pszAmountOther    : �������
// ret : HXEMV_OK          : ���óɹ�
//       HXEMV_FLOW_ERROR  : EMV���̴���
//       HXEMV_PARA        : ��������
// note: GPO��GAC�����ܻ᲻ͬ, �����趨���׽��
//       ����¼��, ��������ǰ�ɵ���
int iHxSetAmount(uchar *pszAmount, uchar *pszAmountOther);

//// ����Ϊ�ص���ʽAPI
//// �ص���ʽAPI  *** ��ʼ ***

// ��λ��Ƭ
// ret : HXEMV_OK          : OK
//       HXEMV_FLOW_ERROR  : EMV���̴���
//       HXEMV_CARD_REMOVED: ����ȡ��
//       HXEMV_CARD_OP     : ��Ƭ��λ����
//		 HXEMV_CALLBACK_METHOD : û���Իص���ʽ��ʼ������
int iHxResetCard(void);

// ��ȡ֧�ֵ�Ӧ��
// in  : iIgnoreBlock	   : !0:����Ӧ��������־, 0:������Ӧ�ò��ᱻѡ��
// ret : HXEMV_OK          : OK
//       HXEMV_NO_APP      : ��֧�ֵ�Ӧ��
//       HXEMV_CARD_REMOVED: ����ȡ��
//       HXEMV_CARD_OP     : ��������
//       HXEMV_CARD_SW     : ��״̬�ַǷ�
//       HXEMV_CANCEL      : �û�ȡ��
//       HXEMV_TIMEOUT     : ��ʱ
//       HXEMV_TERMINATE   : ����ܾ�������������ֹ
//       HXEMV_FLOW_ERROR  : EMV���̴���
//       HXEMV_CORE        : �ڲ�����
//		 HXEMV_CALLBACK_METHOD : û���Իص���ʽ��ʼ������
// Note: ���øú�����ɻ�ȡ����ƥ�������(TAG_DFXX_LanguageUsed)
int iHxGetSupportedApp(int iIgnoreBlock);

// Ӧ��ѡ��
// in  : iIgnoreBlock	   : !0:����Ӧ��������־, 0:������Ӧ�ò��ᱻѡ��
// ret : HXEMV_OK          : OK
//       HXEMV_CARD_REMOVED: ����ȡ��
//       HXEMV_CARD_OP     : ��������
//       HXEMV_NO_APP	   : û��ѡ��Ӧ��
//       HXEMV_TERMINATE   : ����ܾ�������������ֹ
//       HXEMV_CANCEL      : �û�ȡ��
//       HXEMV_TIMEOUT     : ��ʱ
//       HXEMV_FLOW_ERROR  : EMV���̴���
//       HXEMV_CORE        : �ڲ�����
//		 HXEMV_CALLBACK_METHOD : û���Իص���ʽ��ʼ������
int iHxAppSelect(int iIgnoreBlock);

// ���óֿ�������
// in  : pszLanguage  : �ֿ������Դ���, ����:"zh"
//                      �������NULL, ��ʾʹ��EMV����ѡ�񷽷�
// ret : HXEMV_OK     : OK
//       HXEMV_FLOW_ERROR  : EMV���̴���
//		 HXEMV_CALLBACK_METHOD : û���Իص���ʽ��ʼ������
//       HXEMV_PARA   : ��������
//       HXEMV_CORE   : �ڲ�����
// Note: �����֧�ִ��������, ����ʹ�ñ�������
int iHxSetCardHolderLang(uchar *pszLanguage);

// GPO
// in  : pszTransTime      : ����ʱ�䣬YYYYMMDDhhmmss
//       ulTTC             : �ն˽������, 1-999999
//       uiTransType       : ��������, BCD��ʽ(0xAABB, BBΪ��GB/T 15150 ����Ĵ�����ǰ2 λ��ʾ�Ľ��ڽ�������, AA����������Ʒ/����)
//       uiCurrencyCode    : ���Ҵ���
// ret : HXEMV_OK          : OK
//       HXEMV_CARD_REMOVED: ����ȡ��
//       HXEMV_CARD_OP     : ��������
//       HXEMV_CARD_SW     : �Ƿ���״̬��
//       HXEMV_TERMINATE   : ����ܾ�������������ֹ
//       HXEMV_NO_APP      : ��֧�ֵ�Ӧ��
//       HXEMV_RESELECT    : ��Ҫ����ѡ��Ӧ��
//       HXEMV_PARA        : ��������
//       HXEMV_FLOW_ERROR  : EMV���̴���
//       HXEMV_TRANS_NOT_ALLOWED : ���ײ�֧��
//       HXEMV_CORE        : �ڲ�����
//		 HXEMV_CALLBACK_METHOD : û���Իص���ʽ��ʼ������
//       HXEMV_DENIAL	   : qPboc�ܾ�
// Note: �������HXEMV_RESELECT����Ҫ����ִ��iHxAppSelect()
int iHxGPO(uchar *pszTransTime, ulong ulTTC, uint uiTransType, uint uiCurrencyCode);

// ��Ӧ�ü�¼
// ret : HXEMV_OK          : OK
//       HXEMV_CARD_REMOVED: ����ȡ��
//       HXEMV_CARD_OP     : ��������
//       HXEMV_CARD_SW     : �Ƿ���ָ��״̬��
//       HXEMV_TERMINATE   : ����ܾ�������������ֹ
//       HXEMV_FLOW_ERROR  : EMV���̴���
//       HXEMV_CORE        : �ڲ�����
//		 HXEMV_CALLBACK_METHOD : û���Իص���ʽ��ʼ������
// �ǽӿ��ܷ�����
//		 HXEMV_TRY_OTHER_INTERFACE	: ��������ͨ�Ž���
int iHxReadRecord(void);

// SDA��DDA
// ret : HXEMV_OK            : OK
//       HXEMV_CARD_REMOVED  : ����ȡ��
//       HXEMV_CARD_OP       : ��������
//       HXEMV_CARD_SW       : �Ƿ���״̬��
//       HXEMV_TERMINATE     : ����ܾ�������������ֹ
//       HXEMV_DENIAL        : �ն���Ϊ�������Ϊ�ѻ��ܾ�
//       HXEMV_DENIAL_ADVICE : �ն���Ϊ�������Ϊ�ѻ��ܾ�, ��Advice
//       HXEMV_FLOW_ERROR    : EMV���̴���
//       HXEMV_CORE          : �ڲ�����
//		 HXEMV_CALLBACK_METHOD : û���Իص���ʽ��ʼ������
//		 HXEMV_FDDA_FAIL	 : fDDAʧ��(�ǽ�)
// Note: qPboc, FDDAʧ�ܺ�, ���Ĳ���ʾ�κ���Ϣ, Ӧ�ò���Ҫ���м��9F6C B1 b6b5, �Ծ����Ǿܾ����ǽ�������
int iHxOfflineDataAuth(void);

// ��������
// ret : HXEMV_OK            : OK
//       HXEMV_TERMINATE     : ����ܾ�������������ֹ
//       HXEMV_FLOW_ERROR    : EMV���̴���
//       HXEMV_CORE          : �ڲ�����
//       HXEMV_DENIAL        : �ն���Ϊ�������Ϊ�ѻ��ܾ�
//       HXEMV_DENIAL_ADVICE : �ն���Ϊ�������Ϊ�ѻ��ܾ�, ��Advice
//       HXEMV_CARD_REMOVED  : ����ȡ��
//       HXEMV_CARD_OP       : �ն���Ϊ�������Ϊ�ѻ��ܾ�, ����AACʱ��Ƭ����
//		 HXEMV_CALLBACK_METHOD : û���Իص���ʽ��ʼ������
int iHxProcRistrictions(void);

// �ն˷��չ���
// ret : HXEMV_OK            : OK
//       HXEMV_TERMINATE     : ����ܾ�������������ֹ
//       HXEMV_CORE          : ��������
//       HXEMV_CARD_REMOVED  : ����ȡ��
//       HXEMV_CARD_OP       : ��������
//       HXEMV_DENIAL        : �ն���Ϊ�������Ϊ�ѻ��ܾ�
//       HXEMV_DENIAL_ADVICE : �ն���Ϊ�������Ϊ�ѻ��ܾ�, ��Advice
//       HXEMV_CANCEL        : ��ȡ��
//       HXEMV_TIMEOUT       : ��ʱ
//       HXEMV_FLOW_ERROR    : EMV���̴���
//		 HXEMV_CALLBACK_METHOD : û���Իص���ʽ��ʼ������
int iHxTermRiskManage(void);

// �ֿ�����֤
// out : piNeedSignFlag      : ����ɹ���ɣ�������Ҫǩ�ֱ�־��0:����Ҫǩ�� 1:��Ҫǩ��
// ret : HXEMV_OK            : OK
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
//		 HXEMV_CALLBACK_METHOD : û���Իص���ʽ��ʼ������
int iHxCardHolderVerify(int *piNeedSignFlag);

// �ն���Ϊ����
// ret : HXEMV_OK		     : OK
//       HXEMV_CORE          : ��������
//       HXEMV_DENIAL        : �ն���Ϊ�������Ϊ�ѻ��ܾ�
//       HXEMV_DENIAL_ADVICE : �ն���Ϊ�������Ϊ�ѻ��ܾ�, ��Advice
//       HXEMV_CARD_REMOVED  : ����ȡ��
//       HXEMV_CARD_OP       : �ն���Ϊ�������Ϊ�ѻ��ܾ�, ����AACʱ��Ƭ����
//		 HXEMV_CALLBACK_METHOD : û���Իص���ʽ��ʼ������
int iHxTermActionAnalysis(void);

// ��һ��GAC
// out : piCardAction      : ��Ƭִ�н��
//                           GAC_ACTION_TC     : ��׼(����TC)
//							 GAC_ACTION_AAC    : �ܾ�(����AAC)
//                           GAC_ACTION_AAC_ADVICE : �ܾ�(����AAC,��Advice)
//                           GAC_ACTION_ARQC   : Ҫ������(����ARQC)
// ret : HXEMV_OK          : OK
//       HXEMV_CORE        : ��������
//       HXEMV_CARD_REMOVED: ����ȡ��
//       HXEMV_CARD_OP     : ��������
//       HXEMV_CARD_SW     : �Ƿ�״̬��
//       HXEMV_TERMINATE   : ����ܾ�������������ֹ
//       HXEMV_NOT_ALLOWED : ��������
//       HXEMV_NOT_ACCEPTED: ������
//       HXEMV_FLOW_ERROR  : EMV���̴���
//		 HXEMV_CALLBACK_METHOD : û���Իص���ʽ��ʼ������
// Note: ĳЩ�����, �ú������Զ�ִ�еڶ���Gac
//       ��:CDAʧ�ܡ����ѻ��ն�GAC1��Ƭ����ARQC
int iHx1stGAC(int *piCardAction);

// �ڶ���GAC
// in  : pszArc            : Ӧ����[2], NULL��""��ʾ����ʧ��
//       pszAuthCode	   : ��Ȩ��[6], NULL��""��ʾ����Ȩ������
//       psOnlineData      : ������Ӧ����(55������),һ��TLV��ʽ����, NULL��ʾ��������Ӧ����
//       psOnlineDataLen   : ������Ӧ���ݳ���
// out : piCardAction      : ��Ƭִ�н��
//                           GAC_ACTION_TC     : ��׼(����TC)
//							 GAC_ACTION_AAC    : �ܾ�(����AAC)
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
//		 HXEMV_CALLBACK_METHOD : û���Իص���ʽ��ʼ������
int iHx2ndGAC(uchar *pszArc, uchar *pszAuthCode, uchar *psIssuerData, int iIssuerDataLen, int *piCardAction);

//// ����Ϊ�ص���ʽAPI
//// �ص���ʽAPI  *** ���� ***

//// ����Ϊ�ǻص���ʽAPI
//// �ص���ʽAPI  *** ��ʼ ***

// ��λ��Ƭ
// out : piErrMsgType      : ������Ϣ������
// ret : HXEMV_OK          : OK
//       HXEMV_FLOW_ERROR  : EMV���̴���
//       HXEMV_CARD_REMOVED: ����ȡ��
//       HXEMV_CARD_OP     : ��Ƭ��λ����
//		 HXEMV_CALLBACK_METHOD : û���Էǻص���ʽ��ʼ������
// Note: emv�淶Ҫ��忨��Ҫ��ʾ��ȴ���Ϣ, Ӧ�ò���ڵ��ñ�����ǰ������ʾEMVMSG_0E_PLEASE_WAIT,
int iHxResetCard2(int *piErrMsgType);

// ��ȡ֧�ֵ�Ӧ��
// in  : iIgnoreBlock	   : !0:����Ӧ��������־, 0:������Ӧ�ò��ᱻѡ��
//       piAdfNum          : �����ɵ��ն��뿨Ƭͬʱ֧�ֵ�Adf����
// out : paAdfInfo         : �ն��뿨Ƭͬʱ֧�ֵ�Adf�б�(�����ȼ�����)
//       piAdfNum          : �ն��뿨Ƭͬʱ֧�ֵ�Adf����
//       piMsgType         : ��������HXEMV_OKʱ, ���� [Ӧ��ѡ����ʾ] ��Ϣ������
//							 ��������!HXEMV_OKʱ, �������, ��Ϊ���Ӵ�����Ϣ
//       piErrMsgType      : ������Ϣ������, ��������!HXEMV_OK����!HXEMV_AUTO_SELECTʱ����
// ret : HXEMV_OK          : OK, ��ȡ��Ӧ�ñ���ȷ��
//       HXEMV_AUTO_SELECT : OK, ��ȡ��Ӧ�ÿ��Զ�ѡ��
//       HXEMV_NO_APP      : ��֧�ֵ�Ӧ��
//       HXEMV_CARD_REMOVED: ����ȡ��
//       HXEMV_CARD_SW     : ��״̬�ַǷ�
//       HXEMV_CARD_OP     : ��������
//       HXEMV_TERMINATE   : ����ܾ�������������ֹ
//       HXEMV_FLOW_ERROR  : EMV���̴���
//       HXEMV_LACK_MEMORY : �洢�ռ䲻��
//		 HXEMV_CALLBACK_METHOD : û���Էǻص���ʽ��ʼ������
int iHxGetSupportedApp2(int iIgnoreBlock, stADFInfo *paAdfInfo, int *piAdfNum, int *piMsgType, int *piErrMsgType);

// Ӧ��ѡ��
// in  : iIgnoreBlock	   : !0:����Ӧ��������־, 0:������Ӧ�ò��ᱻѡ��
//       iAidLen             : AID����
//       psAid               : AID
// out : piErrMsgType        : ������Ϣ������
// ret : HXEMV_OK            : OK
//       HXEMV_CARD_REMOVED  : ����ȡ��
//       HXEMV_CARD_OP       : ��������
//       HXEMV_TERMINATE     : ����ܾ�������������ֹ
//       HXEMV_FLOW_ERROR    : EMV���̴���
//       HXEMV_CORE          : �ڲ�����
//       HXEMV_RESELECT      : ��Ҫ����ѡ��Ӧ��
//       HXEMV_NOT_SUPPORTED : ѡ���˲�֧�ֵ�Ӧ��
//		 HXEMV_CALLBACK_METHOD : û���Էǻص���ʽ��ʼ������
// Note: ��ͨ����ȡ9F7A�ж��Ƿ�֧��eCash
int iHxAppSelect2(int iIgnoreBlock, int iAidLen, uchar *psAid, int *piErrMsgType);

// ��ȡ֧�ֵ������Թ��ֿ���ѡ��
// out : pszLangs     : ֧�ֵ������б�, �������ַ�������Ϊ0, ��ʾ���سֿ�������ѡ��
//       pszLangsDesc : '='�Ž�β��֧�����������б�
//       piMsgType    : pszLangs������Ҫ�ֿ���ѡ��ʱ��ʾѡ��������ʾ��Ϣ������
//       piErrMsgType : ������Ϣ������
// ret : HXEMV_OK     : OK
//       HXEMV_FLOW_ERROR  : EMV���̴���
//		 HXEMV_CALLBACK_METHOD : û���Էǻص���ʽ��ʼ������
//       HXEMV_CORE        : �ڲ�����
int iHxGetSupportedLang2(uchar *pszLangs, uchar *pszLangsDesc, int *piMsgType, int *piErrMsgType);

// ���óֿ�������
// in  : pszLanguage  : �ֿ������Դ���, ����:"zh"
// out : piErrMsgType : ������Ϣ������
// ret : HXEMV_OK     : OK
//       HXEMV_FLOW_ERROR  : EMV���̴���
//		 HXEMV_CALLBACK_METHOD : û���Էǻص���ʽ��ʼ������
//       HXEMV_PARA   : ��������
//       HXEMV_CORE   : �ڲ�����
int iHxSetCardHolderLang2(uchar *pszLanguage, int *piErrMsgType);

// GPO
// in  : pszTransTime      : ����ʱ�䣬YYYYMMDDhhmmss
//       ulTTC             : �ն˽������, 1-999999
//       uiTransType       : ��������, BCD��ʽ(0xAABB, BBΪ��GB/T 15150 ����Ĵ�����ǰ2 λ��ʾ�Ľ��ڽ�������, AA����������Ʒ/����)
//       pszAmount         : ���׽��, ascii�봮, ��ҪС����, Ĭ��ΪȱʡС����λ��, ����:RMB1.00��ʾΪ"100"
//       uiCurrencyCode    : ���Ҵ���
// out : piMsgType         : ��Ϣ����, ������ʾ��Ϣ, ��������HXEMV_RESELECTʱ���Ӵ���ʾ��Ϣ
//       piErrMsgType      : ������Ϣ������, ����Ϣ��piMsgType����
// ret : HXEMV_OK          : OK
//       HXEMV_CARD_REMOVED: ����ȡ��
//       HXEMV_CARD_OP     : ��������
//       HXEMV_CARD_SW     : �Ƿ���״̬��
//       HXEMV_TERMINATE   : ����ܾ�������������ֹ
//       HXEMV_RESELECT    : ��Ҫ����ѡ��Ӧ��
//       HXEMV_TRANS_NOT_ALLOWED : ���ײ�֧��
//       HXEMV_PARA        : ��������
//       HXEMV_FLOW_ERROR  : EMV���̴���
//		 HXEMV_CALLBACK_METHOD : û���Էǻص���ʽ��ʼ������
//       HXEMV_CORE        : �ڲ�����
//       HXEMV_DENIAL	   : qPboc�ܾ�
// Note: �������HXEMV_RESELECT����iHxGetSupportedApp()��ʼ����ִ������
//       ��ΪGPOʧ�ܺ������Ҫ��ʾ������Ϣ(������&����һ��), ��������ܷ�����������Ϣ������Ϣ
int iHxGPO2(uchar *pszTransTime, ulong ulTTC, uint uiTransType, uchar *pszAmount, uint uiCurrencyCode, int *piMsgType, int *piErrMsgType);

// ��Ӧ�ü�¼
// out : psRid+pszPan+piPanSeqNo�������ṩ��Ӧ�ò��жϺ��������ֿ��������
//		 psRid             : RID[5], NULL��ʾ����Ҫ����
//       pszPan            : �˺�
//       piPanSeqNo        : �˺����к�, -1��ʾ�޴�����, NULL��ʾ����Ҫ����
//       piErrMsgType      : ������Ϣ������
// ret : HXEMV_OK          : OK
//       HXEMV_CARD_REMOVED: ����ȡ��
//       HXEMV_CARD_OP     : ��������
//       HXEMV_CARD_SW     : �Ƿ���ָ��״̬��
//       HXEMV_TERMINATE   : ����ܾ�������������ֹ
//       HXEMV_FLOW_ERROR  : EMV���̴���
//       HXEMV_CORE        : �ڲ�����
//		 HXEMV_CALLBACK_METHOD : û���Էǻص���ʽ��ʼ������
// �ǽӿ��ܷ�����
//		 HXEMV_TRY_OTHER_INTERFACE	: ��������ͨ�Ž���
int iHxReadRecord2(uchar *psRid, uchar *pszPan, int *piPanSeqNo, int *piErrMsgType);

// ���ÿ�Ƭ�˻����������ֿ�������Ϣ
// in  : iBlackFlag        : ���øÿ��Ƿ�Ϊ��������, 0:��Ϊ�������� 1:��������
//       pszRecentAmount   : ���һ�����ѽ��(���ڷֿ����Ѽ��)
// out : piErrMsgType      : ������Ϣ������
// ret : HXEMV_OK          : OK
//       HXEMV_CORE        : �ڲ�����
// Note: �������¼���ֿ�Ƭ�Ǻ�����������������Ѽ�¼, ����Ҫ���ô˺��������˻���Ϣ
int iHxSetPanInfo2(int iBlackFlag, uchar *pszRecentAmount, int *piErrMsgType);

// SDA��DDA
// out : piNeedCheckCrlFlag  : �Ƿ���Ҫ�жϷ����й�Կ֤��Ϊ��������־, 0:����Ҫ�ж� 1:��Ҫ�ж�
//       psRid               : RID[5], psRid+pucCaIndex+psCertSerNo�������ṩ��Ӧ�ò��жϷ����й�Կ֤���Ƿ��ں������б���
//       pucCaIndex          : CA��Կ����
//       psCertSerNo         : �����й�Կ֤�����к�[3]
//       piErrMsgType        : ������Ϣ������
// ret : HXEMV_OK            : OK
//       HXEMV_CARD_REMOVED  : ����ȡ��
//       HXEMV_CARD_OP       : ��������
//       HXEMV_CARD_SW       : �Ƿ���״̬��
//       HXEMV_TERMINATE     : ����ܾ�������������ֹ
//       HXEMV_DENIAL        : �ն���Ϊ�������Ϊ�ѻ��ܾ�
//       HXEMV_DENIAL_ADVICE : �ն���Ϊ�������Ϊ�ѻ��ܾ�, ��Advice
//       HXEMV_FLOW_ERROR    : EMV���̴���
//       HXEMV_CORE          : �ڲ�����
//		 HXEMV_CALLBACK_METHOD : û���Էǻص���ʽ��ʼ������
//		 HXEMV_FDDA_FAIL	 : fDDAʧ��(�ǽ�)
// Note: Ӧ�ò�֧�ַ����й�Կ֤������������, piNeedCheckCrlFlag��������, ������Ժ���
// Note: qPboc, FDDAʧ�ܺ�, piErrMsgType������, Ӧ�ò���Ҫ���м��9F6C B1 b6b5, �Ծ����Ǿܾ����ǽ�������
int iHxOfflineDataAuth2(int *piNeedCheckCrlFlag, uchar *psRid, uchar *pucCaIndex, uchar *psCertSerNo, int *piErrMsgType);

// ���÷����й�Կ֤��Ϊ������
// out : piErrMsgType        : ������Ϣ������, 0��ʾ����Ҫ��ʾ����Ϣ
// ret : HXEMV_OK            : OK
//       HXEMV_CARD_REMOVED  : ����ȡ��
//       HXEMV_CARD_OP       : ��������
//       HXEMV_CARD_SW       : �Ƿ���״̬��
//       HXEMV_TERMINATE     : ����ܾ�������������ֹ
//       HXEMV_DENIAL        : �ն���Ϊ�������Ϊ�ѻ��ܾ�
//       HXEMV_DENIAL_ADVICE : �ն���Ϊ�������Ϊ�ѻ��ܾ�, ��Advice
//       HXEMV_FLOW_ERROR    : EMV���̴���
//       HXEMV_CORE          : �ڲ�����
//		 HXEMV_CALLBACK_METHOD : û���Էǻص���ʽ��ʼ������
// Note: Ӧ�ò����iHxOfflineDataAuth2()��õ�Rid+CaIndex+CertSerNo
//       ���ݴ������жϷ����й�Կ֤���Ƿ�Ϊ������,ֻ���Ǻ�����,����Ҫ���ñ�����֪ͨ����
int iHxSetIssuerCertCrl2(int *piErrMsgType);

// ��������
// out : piErrMsgType        : ������Ϣ������
// ret : HXEMV_OK            : OK
//       HXEMV_TERMINATE     : ����ܾ�������������ֹ
//       HXEMV_FLOW_ERROR    : EMV���̴���
//       HXEMV_CORE          : �ڲ�����
//       HXEMV_DENIAL        : �ն���Ϊ�������Ϊ�ѻ��ܾ�
//       HXEMV_DENIAL_ADVICE : �ն���Ϊ�������Ϊ�ѻ��ܾ�, ��Advice
//       HXEMV_CARD_REMOVED  : ����ȡ��
//       HXEMV_CARD_OP       : �ն���Ϊ�������Ϊ�ѻ��ܾ�, ����AACʱ��Ƭ����
//		 HXEMV_CALLBACK_METHOD : û���Էǻص���ʽ��ʼ������
int iHxProcRistrictions2(int *piErrMsgType);

// �ն˷��չ���
// out : piErrMsgType        : ������Ϣ������
// ret : HXEMV_OK            : OK
//       HXEMV_TERMINATE     : ����ܾ�������������ֹ
//       HXEMV_CORE          : ��������
//       HXEMV_CARD_REMOVED  : ����ȡ��
//       HXEMV_CARD_OP       : ��������
//       HXEMV_DENIAL        : �ն���Ϊ�������Ϊ�ѻ��ܾ�
//       HXEMV_DENIAL_ADVICE : �ն���Ϊ�������Ϊ�ѻ��ܾ�, ��Advice
//       HXEMV_FLOW_ERROR    : EMV���̴���
//		 HXEMV_CALLBACK_METHOD : û���Էǻص���ʽ��ʼ������
int iHxTermRiskManage2(int *piErrMsgType);

// ��ȡ�ֿ�����֤
// out : piCvm               : �ֿ�����֤����, ֧��HXCVM_PLAIN_PIN��HXCVM_CIPHERED_ONLINE_PIN��HXCVM_HOLDER_ID��HXCVM_CONFIRM_AMOUNT
//       piBypassFlag        : ����bypass��־, 0:������, 1:����
//       piMsgType           : ��ʾ��Ϣ��(�����֤����Ϊ֤����֤, ����֤��������)
//       piMsgType2          : ������ʾ��Ϣ, �����Ϊ0, ����Ҫ����ʾ����Ϣ
//       pszAmountStr        : ��ʾ�ø�ʽ�����(HXCVM_HOLDER_ID��֤��������Ҫ)
//       piErrMsgType        : ������Ϣ������, ��������!HXEMV_OK����!HXEMV_NO_DATAʱ����
// ret : HXEMV_OK            : OK
//       HXEMV_NO_DATA       : ����������гֿ�����֤
//       HXEMV_CARD_REMOVED  : ����ȡ��
//       HXEMV_CARD_OP       : ��������
//       HXEMV_TERMINATE     : ����ܾ�������������ֹ
//       HXEMV_DENIAL        : �ն���Ϊ�������Ϊ�ѻ��ܾ�
//       HXEMV_DENIAL_ADVICE : �ն���Ϊ�������Ϊ�ѻ��ܾ�, ��Advice
//       HXEMV_FLOW_ERROR    : EMV���̴���
//       HXEMV_CORE          : �ڲ�����
//		 HXEMV_CALLBACK_METHOD : û���Էǻص���ʽ��ʼ������
// Note: ��iHxDoCvmMethod2()����, ������Ҫ��ε���, ֪������HXEMV_NO_DATA
//       ���ڷ��ص���Ϣ�϶�, ����֧��HXCVM_HOLDER_ID��֤����, piMsgType���ص���֤��������
//       ��ʱ������Ϣʹ�ù̶���Ϣ��, ��������:
//           EMVMSG_VERIFY_HOLDER_ID		"�����ֿ���֤��"
//			 EMVMSG_HOLDER_ID_TYPE			"�ֿ���֤������"
//			 EMVMSG_HOLDER_ID_NO			"�ֿ���֤������"
//       ֤��������ȡTagΪ:TAG_9F61_HolderId
int iHxGetCvmMethod2(int *piCvm, int *piBypassFlag, int *piMsgType, int *piMsgType2, uchar *pszAmountStr, int *piErrMsgType);

// ִ�гֿ�����֤
// in  : iCvmProc            : ��֤��������ʽ, HXCVM_PROC_OK or HXCVM_BYPASS or HXCVM_FAIL or HXCVM_CANCEL or HXCVM_TIMEOUT
//       psCvmData           : ���������, ���Ϊ��������, ����β��Ҫ��0
// out : piMsgType           : ����HXEMV_OKʱʹ��
//                             �����Ϊ0, ����Ҫ��ʾ��������Ϣ
//       piMsgType2          : ����HXEMV_OKʱʹ��
//                             �����Ϊ0, ����Ҫ��ʾ��������Ϣ
//       piErrMsgType        : ������Ϣ������
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
//		 HXEMV_CALLBACK_METHOD : û���Էǻص���ʽ��ʼ������
// Note: ִ�е�CVM���������һ��iHxGetCvmMethod2()��õ�CVM
int iHxDoCvmMethod2(int iCvmProc, uchar *psCvmData, int *piMsgType, int *piMsgType2, int *piErrMsgType);

// ��ȡCVM��֤����ǩ�ֱ�־
// out : piNeedSignFlag    : ��ʾ��Ҫǩ�ֱ�־��0:����Ҫǩ�� 1:��Ҫǩ��
// ret : HXEMV_OK          : OK
int iHxGetCvmSignFlag2(int *piNeedSignFlag);

// �ն���Ϊ����
// out : piErrMsgType        : ������Ϣ������
// ret : HXEMV_OK            : OK
//       HXEMV_CORE          : ��������
//       HXEMV_FLOW_ERROR    : EMV���̴���
//       HXEMV_CARD_REMOVED  : ����ȡ��
//       HXEMV_CARD_OP       : ��������
//       HXEMV_DENIAL        : �ն���Ϊ�������Ϊ�ѻ��ܾ�
//       HXEMV_DENIAL_ADVICE : �ն���Ϊ�������Ϊ�ѻ��ܾ�, ��Advice
//		 HXEMV_CALLBACK_METHOD : û���Էǻص���ʽ��ʼ������
int iHxTermActionAnalysis2(int *piErrMsgType);

// ��һ��GAC
// out : piCardAction      : ��Ƭִ�н��
//                           GAC_ACTION_TC     : ��׼(����TC)
//							 GAC_ACTION_AAC    : �ܾ�(����AAC)
//                           GAC_ACTION_AAC_ADVICE : �ܾ�(����AAC,��Advice)
//                           GAC_ACTION_ARQC   : Ҫ������(����ARQC)
//       piMsgType         : ��ʾ��Ϣ��, ��������HXEMV_OKʱʹ��
//       piErrMsgType      : ������Ϣ������
// ret : HXEMV_OK          : OK
//       HXEMV_CORE        : ��������
//       HXEMV_CARD_REMOVED: ����ȡ��
//       HXEMV_CARD_OP     : ��������
//       HXEMV_CARD_SW     : �Ƿ�״̬��
//       HXEMV_TERMINATE   : ����ܾ�������������ֹ
//       HXEMV_NOT_ACCEPTED: ������
//       HXEMV_NOT_ALLOWED : ��������
//       HXEMV_FLOW_ERROR  : EMV���̴���
// Note: ĳЩ�����, �ú������Զ�����iHx2ndGAC()
//       ��:CDAʧ�ܡ����ѻ��ն�GAC1��Ƭ����ARQC...
//		 HXEMV_CALLBACK_METHOD : û���Էǻص���ʽ��ʼ������
int iHx1stGAC2(int *piCardAction, int *piMsgType, int *piErrMsgType);

// �ڶ���GAC
// in  : pszArc            : Ӧ����[2], NULL��""��ʾ����ʧ��
//       pszAuthCode	   : ��Ȩ��[6], NULL��""��ʾ����Ȩ������
//       psOnlineData      : ������Ӧ����(55������),һ��TLV��ʽ����, NULL��ʾ��������Ӧ����
//       psOnlineDataLen   : ������Ӧ���ݳ���
// out : piCardAction      : ��Ƭִ�н��
//                           GAC_ACTION_TC     : ��׼(����TC)
//							 GAC_ACTION_AAC    : �ܾ�(����AAC)
//                           GAC_ACTION_AAC_ADVICE : �ܾ�(����AAC,��Advice)
//       piMsgType         : ��ʾ��Ϣ��, ��������HXEMV_OKʱʹ��
//       piErrMsgType      : ������Ϣ������
// ret : HXEMV_OK          : OK
//       HXEMV_CORE        : ��������
//       HXEMV_CARD_REMOVED: ����ȡ��
//       HXEMV_CARD_OP     : ��������
//       HXEMV_CARD_SW     : �Ƿ���״̬��
//       HXEMV_TERMINATE   : ����ܾ�������������ֹ
//       HXEMV_NOT_ALLOWED : ��������
//       HXEMV_NOT_ACCEPTED: ������
//       HXEMV_FLOW_ERROR  : EMV���̴���
//		 HXEMV_CALLBACK_METHOD : û���Էǻص���ʽ��ʼ������
int iHx2ndGAC2(uchar *pszArc, uchar *pszAuthCode, uchar *psIssuerData, int iIssuerDataLen, int *piCardAction, int *piMsgType, int *piErrMsgType);

// ��ȡ��ʾ��Ϣ����
// in  : iMsgType    : ��Ϣ����, �ο�EmvMsg.h
//       iMsgFor     : ��Ϣ����������(HXMSG_SMART or HXMSG_OPERATOR or HXMSG_CARDHOLDER or HXMSG_LOCAL_LANG)
// out : pszMsg      : �����������Է��ص���Ϣ����
//       pszLanguage : ʹ�õ����Դ��룬ISO-639-1��Сд. ����NULL��ʾ����Ҫ����
// ret : ����ֵ����pszMsg
// Note: �����Ϣ���Ͳ�֧��, pszMsg[0]�ᱻ��ֵΪ0
uchar *pszHxGetMsg2(int iMsgType, int iMsgFor, uchar *pszMsg, uchar *pszLanguage);

//// ����Ϊ�ǻص���ʽAPI
//// �ص���ʽAPI  *** ���� ***

#endif
