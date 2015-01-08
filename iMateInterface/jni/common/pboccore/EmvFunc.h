/**************************************
File name     : EmvFunc.h
Function      : Emv������ص�ͨ�ú���
Author        : Yu Jun
First edition : Apr 14th, 2012
Modified      : Apr 1st, 2014
					����iEmvTestIfIsAtm()����
**************************************/
#ifndef _EMVFUNC_H
#define _EMVFUNC_H

// �ն����л���
#define TERM_ENV_ATTENDED				1	// ����ֵ��
#define TERM_ENV_UNATTENDED				2	// ����ֵ��

// �ն�ͨѶ����
#define TERM_COM_ONLINE_ONLY			1	// �����ն�
#define TERM_COM_BOTH					2	// ���ѻ������������ն�
#define TERM_COM_OFFLINE_ONLY			3	// �ѻ��ն�

// �������ʶ����
#define EMV_ITEM_AIP					0	// AIP
#define EMV_ITEM_AUC					1	// AUC
#define EMV_ITEM_TVR					2	// TVR
#define EMV_ITEM_TSI					3	// TSI
#define EMV_ITEM_TERM_CAP				4	// �ն�����λ����(Terminal Capability)
#define EMV_ITEM_TERM_CAP_ADD			5	// �ն�������չλ����(Additional Terminal Capability)

// AIPλ����
#define AIP_01_SDA_SUPPORTED            1	// SDA supported
#define AIP_02_DDA_SUPPORTED			2	// DDA supported
#define AIP_03_CVM_SUPPORTED			3	// cardholder verification is supported
#define AIP_04_TERM_RISK_IS_PERFORMED	4	// terminal risk management is to be performed
#define AIP_05_ISSUER_AUTH_SUPPORED		5	// issuer authentication is supported
#define AIP_07_CDA_SUPPORTED			7	// CDA supported
// AUCλ����
#define AUC_00_DOMESTIC_CASH			0	// valid for domestic cash transactions
#define AUC_01_INTERNATIONAL_CASH       1   // valid for international cash transactions
#define AUC_02_DOMESTIC_GOODS			2	// valid for domestic goods
#define AUC_03_INTERNATIONAL_GOODS		3	// valid for international goods
#define AUC_04_DOMESTIC_SERVICES		4	// valid for domestic services
#define AUC_05_INTERNATIONAL_SERVICES	5	// valid for international services
#define AUC_06_ATMS						6	// valid at ATMs
#define AUC_07_NO_ATMS					7	// valid at terminals other than ATMs
#define AUC_08_DOMESTIC_CASHBACK		8	// domestic cashback allowed
#define AUC_09_INTERNATIONAL_CASHBACK	9	// international cashback allowed
// TVRλ����
#define TVR_00_DATA_AUTH_NOT_PERFORMED  0   // offline data authentication was not performed
#define TVR_01_SDA_FAILED				1	// SDA failed
#define TVR_02_ICC_DATA_MISSING			2	// ICC data missing
#define TVR_03_BLACK_CARD				3	// card appears on terminal exception file
#define TVR_04_DDA_FAILED				4	// DDA failed
#define TVR_05_CDA_FAILED				5	// CDA failed
#define TVR_08_DIFFERENT_VER			8	// ICC and terminal have different application versions
#define TVR_09_EXPIRED					9	// expired application
#define TVR_10_NOT_EFFECTIVE			10	// application not yet effective
#define TVR_11_SERVICE_NOT_ALLOWED		11	// requested service not allowed for card product
#define TVR_12_NEW_CARD					12	// new card
#define TVR_16_CVM_NOT_SUCCESS			16	// cardholder verification was not successfull
#define TVR_17_UNRECOGNISED_CVM			17	// unrecognised CVM
#define TVR_18_EXCEEDS_PIN_TRY_LIMIT	18	// PIN try limit exceeded
#define TVR_19_NO_PINPAD				19	// PIN entry required and PIN pad not present or not working
#define TVR_20_PIN_NOT_ENTERED			20	// PIN entry required, PIN pad present, but PIN was not entered
#define TVR_21_ONLINE_PIN_ENTERED		21	// online PIN entered
#define TVR_24_EXCEEDS_FLOOR_LIMIT		24	// transaction exceeds floor limit
#define TVR_25_EXCEEDS_LOWER_LIMIT		25	// lower consecutive offline limit exceeded
#define TVR_26_EXCEEDS_UPPER_LIMIT		26	// upper consecutive offline limit exceeded
#define TVR_27_RANDOMLY_SELECTED		27	// transaction selected randomly for online processing
#define TVR_28_FORCED_ONLINE			28	// merchant forced transaction online
#define TVR_32_DEFAULT_TDOL_USED		32  // default TDOL used
#define TVR_33_ISSUER_AUTH_FAILED		33	// issuer authentication failed
#define TVR_34_SCRIPT_FAILED_BEFORE_AC	34  // script processing failed before final GAC
#define TVR_35_SCRIPT_FAILED_AFTER_AC	35  // script processing failed after final GAC
// TSIλ����
#define TSI_00_DATA_AUTH_PERFORMED		0	// offline data authentication was performed
#define TSI_01_CVM_PERFORMED			1	// cardholder verification was performed
#define TSI_02_CARD_RISK_PERFORMED		2	// card risk management was performed
#define TSI_03_ISSUER_AUTH_PERFORMED	3	// issuer authentication was performed
#define TSI_04_TERM_RISK_PERFORMED		4	// terminal risk management was performed
#define TSI_05_SCRIPT_PERFORMED			5	// script processing was performed
// �ն�����λ����(Terminal Capability)
#define TERM_CAP_00_MANUAL_KEY_ENTRY	0	// manual key entry
#define TERM_CAP_01_MAG_STRIPE			1	// magnetic stripe
#define TERM_CAP_02_IC_WITH_CONTACTS	2	// IC with contacts
#define TERM_CAP_08_PLAIN_PIN			8	// plaintext PIN for ICC verification
#define TERM_CAP_09_ENC_ONLINE_PIN		9	// enciphered PIN for online verification
#define TERM_CAP_10_SIGNATURE			10	// signature
#define TERM_CAP_11_ENC_OFFLINE_PIN		11	// enciphered PIN for offline verification
#define TERM_CAP_12_NO_CVM_REQUIRED		12	// no CVM required
#define TERM_CAP_15_HOLDER_ID			15	// �ֿ���֤����֤(Pboc2.0, 2013.2.19����)
#define TERM_CAP_16_SDA					16	// SDA
#define TERM_CAP_17_DDA					17	// DDA
#define TERM_CAP_18_CARD_CAPTURE		18	// card capture
#define TERM_CAP_20_CDA					20	// CDA
// �ն�������չλ����(Additional Terminal Capability)
#define TERM_CAP_ADD_00_CASH			0	// cash
#define TERM_CAP_ADD_01_GOODS			1	// goods
#define TERM_CAP_ADD_02_SERVICES		2	// services
#define TERM_CAP_ADD_03_CASHBACK		3	// cashback
#define TERM_CAP_ADD_04_INQUIRY			4	// inquiry
#define TERM_CAP_ADD_05_TRANSFER		5	// transfer
#define TERM_CAP_ADD_06_PAYMENT			6	// payment
#define TERM_CAP_ADD_07_ADMIN			7	// administrative
#define TERM_CAP_ADD_08_CASH_DEPOSIT	8	// cash deposit
#define TERM_CAP_ADD_16_NUMERIC_KEYS	16	// numeric keys
#define TERM_CAP_ADD_17_ALPHA_KEYS		17	// alphabetic and special characters keys
#define TERM_CAP_ADD_18_COMMAND_KEYS	18	// command keys
#define TERM_CAP_ADD_19_FUNC_KEYS		19	// function keys
#define TERM_CAP_ADD_24_PR_ATTENDANT	24	// print, attendant
#define TERM_CAP_ADD_25_PR_CARD_HOLDER	25	// print, cardholder
#define TERM_CAP_ADD_26_DISP_ATTENDANT	26	// display, attendant
#define TERM_CAP_ADD_27_DISP_CARD_HOLDER 27	// display, cardholder
#define TERM_CAP_ADD_30_CODE_TABLE10	30	// code table 10
#define TERM_CAP_ADD_31_CODE_TABLE9		31	// code table 9
#define TERM_CAP_ADD_31_CODE_TABLE8		32	// code table 8
#define TERM_CAP_ADD_31_CODE_TABLE7		33	// code table 7
#define TERM_CAP_ADD_31_CODE_TABLE6		34	// code table 6
#define TERM_CAP_ADD_31_CODE_TABLE5		35	// code table 5
#define TERM_CAP_ADD_31_CODE_TABLE4		36	// code table 4
#define TERM_CAP_ADD_31_CODE_TABLE3		37	// code table 3
#define TERM_CAP_ADD_31_CODE_TABLE2		38	// code table 2
#define TERM_CAP_ADD_31_CODE_TABLE1		39	// code table 1

// �����ն����л���
// ret : TERM_ENV_ATTENDED     : ����ֵ��
//       TERM_ENV_UNATTENDED   : ����ֵ��
int iEmvTermEnvironment(void);

// �����ն�ͨѶ����
// ret : TERM_COM_OFFLINE_ONLY : �ѻ��ն�
//       TERM_COM_ONLINE_ONLY  : �����ն�
//       TERM_COM_BOTH         : ���ѻ������������ն�
int iEmvTermCommunication(void);

// ���������Ȩ���浽TLV���ݿ� gl_sTlvDbTermVar
// in  : pszAmount : ��Ȩ���
// ret : 0         : OK
//       1         : ����
int iEmvSaveAmount(uchar *pszAmount);

// ����EMV������ĳλ
// in  : iItem  : �������ʶ, EMV_ITEM_TVR �� EMV_ITEM_TSI
//       iBit   : λ��, refer to EmvFunc.h
//       iValue : 0 or 1
// ret : 0      : OK
// Note: ��������ʶ���λ��Խ��,�����κδ���,����0
//       ��������ڲ�����,����0
int iEmvSetItemBit(int iItem, int iBit, int iValue);

// ����EMV������ĳλֵ
// in  : iItem : �������ʶ, EMV_ITEM_AIP �� EMV_ITEM_AUC �� EMV_ITEM_TERM_CAP �� EMV_ITEM_TERM_CAP_ADD �� EMV_ITEM_TVR
//       iBit  : λ��, refer to EmvFunc.h
// ret : 0     : ��λΪ0
//       1     : ��λΪ1
// Note: ��������ʶ���λ��Խ��,����0
//       ��������ڲ�����,����0
int iEmvTestItemBit(int iItem, int iBit);

// �����Ƿ�ΪATM
// ret : 0 : ����ATM
//       1 : ��ATM
// Refer to Emv4.3 book4 Annex A1
int iEmvTestIfIsAtm(void);

// out : pszAmountStr : ��ʽ������Ȩ���
// ret : 0            : ���Ϊ0
//       1            : ��Ϊ0
//       -1           : ����
// ��ʽ�����, ����123.46��Ԫ��ʽ���� "USD123.46"
int iMakeAmountStr(uchar *pszAmountStr);

// ��ȡEC���ױ�־
// ret : 0 : ����EC����
//       1 : ��EC����
//       2 : GPO����������ֽ�ʶ����, ����ǰ����δ��ECͨ��
int iEmvGetECTransFlag(void);

// ����EC���ױ�־
// in  : iFlag : 0 : ����EC����
//               1 : ��EC����
//               2 : GPO����������ֽ�ʶ����, ����ǰ����δ��ECͨ��
// ret : 0         : OK
//       1         : error
int iEmvSetECTransFlag(int iFlag);

#endif
