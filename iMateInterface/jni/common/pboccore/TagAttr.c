/**************************************
File name     : TAGATTR.C
Function      : Implement EMV2004 standard Tag Attributes obtain
Author        : Yu Jun
First edition : Mar 4th, 2003
Note          : Refer EMV2004 Specification Book3 Part IV, Annex B
                                                  Part IV, Annex A
                �й����ڼ��ɵ�·��IC�����淶��1����13���֣��ϲ��棩.pdf
ģ�����      : ȡ��EMV�淶���Զ����TAG��������
                EMV Specifications - Last update: 2009-02-03
Update        : 2012.3.28
					Tag���Գ��ȷ�Χ��ʵ�ʱ��泤�ȸ�Ϊ��ָ�����������ͼ���ĳ���
					��ˣ�ĳһN��CN�ͳ��ȷ�Χ����趨Ϊ3-7,��ʵ����Ҫ����ĳ���Ϊ2-4�ֽ�
**************************************/
/*
ģ����ϸ����:
	Tag�������ݿ�
.	�������ݿ��е�ucRange1��ucRange2�ǰ��������Ͷ���ĺϷ���̳�����Ϸ���󳤶�
	����N�ͻ�CN��, ʵ�ʴ洢ֻ��Ҫ�������ֵ���ȵ�һ��, ����3-8, ��ʵ�ʱ���ĳ���Ϊ2-4�ֽ�
	��������, ʵ�ʴ洢��Ϊ������ֵ��ʾ�ĳ���
*/
# include <stdio.h>
# include <string.h>
# include <ctype.h>
# include "TagAttr.h"

//#define NEED_DEFINE_ISS_TAG // �����Ҫ���巢����Tag, ��Ҫ����ú�, ����ڱ��뻷������

struct stTagAttr {
    unsigned int   uiTag;     // tag
    unsigned char  ucAttr;    // attribute
    unsigned char  ucFrom;    // origin
    unsigned char  ucRange1;  // least length, ��ָ�����������ͼ���ĳ���
    unsigned char  ucRange2;  // most length, ��ָ�����������ͼ���ĳ���
	unsigned char  *pszDesc;  // ˵��, �64�ֽ�
};

// ǰ�����/**/��Tag��ȷ���Ƿ�Ϊ����ͨ��Tag, ������Ҫʹ������
static const struct stTagAttr gl_aTagAttr[] = {
//    Tag     Attr          From             L   M    Description
	{ 0x4200, TAG_ATTR_N,   TAG_FROM_CARD,   6,  6  , "IIN"}, // Issuer Identification Number (IIN)
    { 0x4F00, TAG_ATTR_B,   TAG_FROM_CARD,   5 , 16 , "AID"}, // Application Identifier(AID)
    { 0x5000, TAG_ATTR_ANS, TAG_FROM_CARD,   1 , 16 , "Application label"}, // Application label
    { 0x5700, TAG_ATTR_B,   TAG_FROM_CARD,   0 , 19 , "Track2 equivalent data"}, // Track 2 Equivalent Data
    { 0x5A00, TAG_ATTR_CN,  TAG_FROM_CARD,   1 , 19 , "PAN"}, // Application Primary Account Number(PAN)
    { 0x5F20, TAG_ATTR_ANS, TAG_FROM_CARD,   2 , 26 , "Cardholder name"}, // Cardholder Name
    { 0x5F24, TAG_ATTR_N,   TAG_FROM_CARD,   6 , 6  , "Expire date"}, // Application Expiration Date
    { 0x5F25, TAG_ATTR_N,   TAG_FROM_CARD,   6 , 6  , "Effective date"}, // Application Effective Date
    { 0x5F28, TAG_ATTR_N,   TAG_FROM_CARD,   3 , 3  , "Issuer country code"}, // Issuer Country Code
    { 0x5F2A, TAG_ATTR_N,   TAG_FROM_TERM,   3 , 3  , "Transaction currency code"}, // Transaction Currency Code
    { 0x5F2D, TAG_ATTR_AN,  TAG_FROM_CARD,   2 , 8  , "Language preference"}, // Language Preference
    { 0x5F30, TAG_ATTR_N,   TAG_FROM_CARD,   3 , 3  , "Service code"}, // Service Code
    { 0x5F34, TAG_ATTR_N,   TAG_FROM_CARD,   2 , 2  , "PAN seqno"}, // Application Primary Account Number Sequence Number
    { 0x5F36, TAG_ATTR_N,   TAG_FROM_TERM,   1 , 1  , "Transaction Currency Exponent"}, // Transaction Currency Exponent
    { 0x5F50, TAG_ATTR_ANS, TAG_FROM_CARD,   0 , 255, "Issuer Script Identifier"}, // Issuer Script Identifier
    { 0x5F53, TAG_ATTR_B,   TAG_FROM_CARD,   0 , 34,  "IBAN"}, // International Bank Account Number (IBAN)
    { 0x5F54, TAG_ATTR_B,   TAG_FROM_CARD,   8 , 11,  "BIC"}, // Bank Identifier Code (BIC)
    { 0x5F55, TAG_ATTR_A,   TAG_FROM_CARD,   2 , 2,   "Issuer Country Code(a2)"}, // Issuer Country Code (alpha2 format)
    { 0x5F56, TAG_ATTR_A,   TAG_FROM_CARD,   3 , 3,   "Issuer Country Code(a3)"}, // Issuer Country Code (alpha3 format)
	{ 0x5F57, TAG_ATTR_N,   TAG_FROM_TERM,   2 , 2,   "Account Type"}, // Account Type
	{ 0x6100, TAG_ATTR_B,   TAG_FROM_CARD,   0 , 252, "Application Template"}, // Application Template
    { 0x6F00, TAG_ATTR_B,   TAG_FROM_CARD,   0 , 252, "FCI"}, // File Control Infomation Template
    { 0x7000, TAG_ATTR_B,   TAG_FROM_CARD,   0 , 252, "AEF data template"}, // Application Elementary File Data Template
    { 0x7100, TAG_ATTR_B,   TAG_FROM_ISSUER, 0 , 255, "Issuer Script Template 1"}, // Issuer Script Template 1
    { 0x7200, TAG_ATTR_B,   TAG_FROM_ISSUER, 0 , 255, "Issuer Script Template 2"}, // Issuer Script Template 2
    { 0x7300, TAG_ATTR_B,   TAG_FROM_CARD,   0 , 252, "Directory Discretionary Template"}, // Directory Discretionary Template
    { 0x7700, TAG_ATTR_B,   TAG_FROM_CARD,   0 , 255, "Response Message Template Format 2"}, // Response Message Template Format 2
    { 0x8000, TAG_ATTR_B,   TAG_FROM_CARD,   0 , 255, "Response Message Template Format 1"}, // Response Message Template Format 1
    { 0x8100, TAG_ATTR_B,   TAG_FROM_TERM,   4 , 4  , "Amount(B)"}, // Amount, Authorised(Binary)
    { 0x8200, TAG_ATTR_B,   TAG_FROM_CARD,   2 , 2  , "AIP"}, // Application Interchange Profile
    { 0x8300, TAG_ATTR_B,   TAG_FROM_TERM,   0 , 255, "Command Template"}, // Command Template
    { 0x8400, TAG_ATTR_B,   TAG_FROM_CARD,   5 , 16 , "DF name"}, // Dedicated File(DF) Name
    { 0x8600, TAG_ATTR_B,   TAG_FROM_ISSUER, 0 , 255, "Issuer Script command"}, // Issuer Script command // �ĵ��涨���261 ????
    { 0x8700, TAG_ATTR_B,   TAG_FROM_CARD,   1 , 1  , "Application priority indicator"}, // Application Priority Indicator
    { 0x8800, TAG_ATTR_B,   TAG_FROM_CARD,   1 , 1  , "SFI"}, // Short File Identifier
    { 0x8900, TAG_ATTR_AN,  TAG_FROM_ISSUER, 6 , 6  , "Authorisation Code"}, // Authorisation Code, �淶δ��������, ���ݾ���֧��ϵͳ��, �����ΪAN��
    { 0x8A00, TAG_ATTR_AN,  TAG_FROM_ISSUER, 2 , 2  , "Authorisation Response Code"}, // Authorisation Response Code(�淶����ΪIssuer/Terminal)
    { 0x8C00, TAG_ATTR_B,   TAG_FROM_CARD,   0 , 252, "CDOL1"}, // Card Risk Management Data Object List 1(CDOL1)
    { 0x8D00, TAG_ATTR_B,   TAG_FROM_CARD,   0 , 252, "CDOL2"}, // Card Risk Management Data Object List 2(CDOL2)
    { 0x8E00, TAG_ATTR_B,   TAG_FROM_CARD,   8,  252, "CVM list"}, // Cardholder Verification Method(CVM) List
    { 0x8F00, TAG_ATTR_B,   TAG_FROM_CARD,   1 , 1  , "CA public key index"}, // Certification Authority Public Key Index
    { 0x9000, TAG_ATTR_B,   TAG_FROM_CARD,   0 , 248, "Issuer public key cert"}, // Issuer Public Key Certificate
    { 0x9100, TAG_ATTR_B,   TAG_FROM_ISSUER, 8 , 16 , "Issuer Authentication Data"}, // Issuer Authentication Data
    { 0x9200, TAG_ATTR_B,   TAG_FROM_CARD,   0 , 252, "Issuer public key remainder"}, // Issuer Public Key Remainder
    { 0x9300, TAG_ATTR_B,   TAG_FROM_CARD,   0 , 248, "Signed Static Application Data"}, // Signed Static Application Data
    { 0x9400, TAG_ATTR_B,   TAG_FROM_CARD,   0 , 252, "AFL"}, // Application File Locator(AFL)
    { 0x9500, TAG_ATTR_B,   TAG_FROM_TERM,   5 , 5  , "TVR"}, // Terminal Verification Results
    { 0x9700, TAG_ATTR_B,   TAG_FROM_CARD,   0 , 252, "TDOL"}, // Transaction Certificate Data Object List(TDOL)
    { 0x9800, TAG_ATTR_B,   TAG_FROM_TERM,   20, 20 , "TC hash value"}, // Transaction Certificate(TC) Hash Value
    { 0x9900, TAG_ATTR_B,   TAG_FROM_TERM,   0 , 255, "Transaction PIN data"}, // Transaction Personal Identification Number(PIN) Data
    { 0x9A00, TAG_ATTR_N,   TAG_FROM_TERM,   6 , 6  , "Transaction date"}, // Transaction Date
    { 0x9B00, TAG_ATTR_B,   TAG_FROM_TERM,   2 , 2  , "TSI"}, // Transaction Status Information
    { 0x9C00, TAG_ATTR_N,   TAG_FROM_TERM,   2 , 2  , "Transaction type"}, // Transaction Type
    { 0x9D00, TAG_ATTR_B,   TAG_FROM_CARD,   5 , 16 , "DDF name"}, // Directory Definition File(DDF) Name
    { 0x9F01, TAG_ATTR_N,   TAG_FROM_TERM,   6 , 11 , "Acquirer Identifier"}, // Acquirer Identifier
    { 0x9F02, TAG_ATTR_N,   TAG_FROM_TERM,   12, 12 , "Amount(N)"}, // Amount, Authorised(Numeric)
    { 0x9F03, TAG_ATTR_N,   TAG_FROM_TERM,   12, 12 , "Amount, Other(N)"}, // Amount, Other(Numeric)
    { 0x9F04, TAG_ATTR_B,   TAG_FROM_TERM,   4 , 4  , "Amount, Other(B)"}, // Amount, Other(Binary)
    { 0x9F05, TAG_ATTR_B,   TAG_FROM_CARD,   1 , 32 , "Application Discretionary Data"}, // Application Discretionary Data
    { 0x9F06, TAG_ATTR_B,   TAG_FROM_TERM,   5 , 16 , "AID"}, // Application Identifier(AID)
    { 0x9F07, TAG_ATTR_B,   TAG_FROM_CARD,   2 , 2  , "AUC"}, // Application Usage Control
    { 0x9F08, TAG_ATTR_B,   TAG_FROM_CARD,   2 , 2  , "Card application version number"}, // Application Version Number, Card
    { 0x9F09, TAG_ATTR_B,   TAG_FROM_TERM,   2 , 2  , "Terminal application version number"}, // Application Version Number, Terminal
    { 0x9F0B, TAG_ATTR_ANS, TAG_FROM_CARD,   27, 45 , "Cardholder Name Extended"}, // Cardholder Name Extended
    { 0x9F0D, TAG_ATTR_B,   TAG_FROM_CARD,   5 , 5  , "IAC-default"}, // Issuer Action Code - Default
    { 0x9F0E, TAG_ATTR_B,   TAG_FROM_CARD,   5 , 5  , "IAC-denial"}, // Issuer Action Code - Denial
    { 0x9F0F, TAG_ATTR_B,   TAG_FROM_CARD,   5 , 5  , "IAC-online"}, // Issuer Action Code - online
    { 0x9F10, TAG_ATTR_B,   TAG_FROM_CARD,   0 , 32 , "Issuer application data"}, // Issuer Application Data
    { 0x9F11, TAG_ATTR_N,   TAG_FROM_CARD,   2 , 2  , "Issuer Code Table Index"}, // Issuer Code Table Index
    { 0x9F12, TAG_ATTR_ANS, TAG_FROM_CARD,   1 , 16 , "Application preferred name"}, // Application Preferred Name
    { 0x9F13, TAG_ATTR_B,   TAG_FROM_CARD,   2 , 2  , "Last ATC"}, // Last Online Application Transaction Counter(ATC) Register
    { 0x9F14, TAG_ATTR_B,   TAG_FROM_CARD,   1 , 1  , "Lower consecutive offline limit"}, // Lower Consecutive Offline Limit
    { 0x9F15, TAG_ATTR_N,   TAG_FROM_TERM,   4 , 4  , "Merchant Category Code"}, // Merchant Category Code
    { 0x9F16, TAG_ATTR_ANS, TAG_FROM_TERM,   15, 15 , "MID"}, // Merchant Identifier
    { 0x9F17, TAG_ATTR_B,   TAG_FROM_CARD,   1 , 1  , "PIN try counter"}, // Personal Identification Number(PIN) Try Counter
    { 0x9F18, TAG_ATTR_B,   TAG_FROM_ISSUER, 4 , 4  , "Issuer Script Identifier"}, // Issuer Script Identifier
    { 0x9F1A, TAG_ATTR_N,   TAG_FROM_TERM,   3 , 3  , "Terminal country code"}, // Terminal Country Code
    { 0x9F1B, TAG_ATTR_B,   TAG_FROM_TERM,   4 , 4  , "Terminal floor limit"}, // Terminal Floor Limit
    { 0x9F1C, TAG_ATTR_AN,  TAG_FROM_TERM,   8 , 8  , "TID"}, // Terminal Identification
    { 0x9F1D, TAG_ATTR_B,   TAG_FROM_TERM,   1 , 8  , "Terminal Risk Management Data"}, // Terminal Risk Management Data
    { 0x9F1E, TAG_ATTR_AN,  TAG_FROM_TERM,   8 , 8  , "IFD Serial Number"}, // Interface Device(IFD) Serial Number
    { 0x9F1F, TAG_ATTR_ANS, TAG_FROM_CARD,   0 , 252, "Track 1 Discretionary Data"}, // Track 1 Discretionary Data
    { 0x9F20, TAG_ATTR_CN,  TAG_FROM_CARD,   0 , 252, "Track 2 Discretionary Data"}, // Track 2 Discretionary Data
    { 0x9F21, TAG_ATTR_N,   TAG_FROM_TERM,   6 , 6  , "Transaction time"}, // Transaction Time
    { 0x9F22, TAG_ATTR_B,   TAG_FROM_TERM,   1 , 1  , "CA public key index"}, // Certification Authority Public Key Index
    { 0x9F23, TAG_ATTR_B,   TAG_FROM_CARD,   1 , 1  , "Upper consecutive offline limit"}, // Upper Consecutive Offline Limit
    { 0x9F26, TAG_ATTR_B,   TAG_FROM_CARD,   8 , 8  , "AC"}, // Application Cryptogram
    { 0x9F27, TAG_ATTR_B,   TAG_FROM_CARD,   1 , 1  , "CID"}, // Cryptogram information Data
    { 0x9F2D, TAG_ATTR_B,   TAG_FROM_CARD,   0 , 248, "ICC PIN Public Key Certificate"}, // ICC PIN Encipherment Public Key Certificate
    { 0x9F2E, TAG_ATTR_B,   TAG_FROM_CARD,   1 , 3  , "ICC PIN Public Key Exponent"}, // ICC PIN Encipherment Public Key Exponent
    { 0x9F2F, TAG_ATTR_B,   TAG_FROM_CARD,   0 , 255, "ICC PIN Public Key Remainder"}, // ICC PIN Encipherment Public Key Remainder
    { 0x9F32, TAG_ATTR_B,   TAG_FROM_CARD,   1 , 3  , "Issuer Public Key Exponent"}, // Issuer Public Key Exponent
    { 0x9F33, TAG_ATTR_B,   TAG_FROM_TERM,   3 , 3  , "Terminal Capabilities"}, // Terminal Capabilities
    { 0x9F34, TAG_ATTR_B,   TAG_FROM_TERM,   3 , 3  , "CVM result"}, // Cardholder Verification Method(CVM) Results
    { 0x9F35, TAG_ATTR_N,   TAG_FROM_TERM,   2 , 2  , "Terminal Type"}, // Terminal Type
    { 0x9F36, TAG_ATTR_B,   TAG_FROM_CARD,   2 , 2  , "ATC"}, // Application Transaction Counter(ATC)
    { 0x9F37, TAG_ATTR_B,   TAG_FROM_TERM,   4 , 4  , "RAND"}, // Unpredictable Number
    { 0x9F38, TAG_ATTR_B,   TAG_FROM_CARD,   0 , 255, "PDOL"}, // Processing Options Data Object List(PDOL)
    { 0x9F39, TAG_ATTR_N,   TAG_FROM_TERM,   2 , 2  , "POS Entry Mode"}, // Point-of-Service(POS) Entry Mode
    { 0x9F3A, TAG_ATTR_B,   TAG_FROM_TERM,   4 , 4  , "Amount, Reference Currency"}, // Amount, Reference Currency
    { 0x9F3B, TAG_ATTR_N,   TAG_FROM_CARD,   3 , 15 , "Appilcation Reference Currency"}, // Appilcation Reference Currency
    { 0x9F3C, TAG_ATTR_N,   TAG_FROM_TERM,   3 , 3  , "Transaction Reference Currency"}, // Transaction Reference Currency
    { 0x9F3D, TAG_ATTR_N,   TAG_FROM_TERM,   1 , 1  , "Transaction Reference Currency Exponent"}, // Transaction Reference Currency Exponent
    { 0x9F40, TAG_ATTR_B,   TAG_FROM_TERM,   5 , 5  , "Additional Terminal Capabilities"}, // Additional Terminal Capabilities
    { 0x9F41, TAG_ATTR_N,   TAG_FROM_TERM,   4 , 8  , "Transaction Sequence Counter"}, // Transaction Sequence Counter
    { 0x9F42, TAG_ATTR_N,   TAG_FROM_CARD,   3 , 3  , "Application currency code"}, // Application Currency Code
    { 0x9F43, TAG_ATTR_N,   TAG_FROM_CARD,   1 , 7  , "Application Reference Currency Exponent"}, // Application Reference Currency Exponent
    { 0x9F44, TAG_ATTR_N,   TAG_FROM_CARD,   1 , 1  , "Application Currency Exponent"}, // Application Curerncy Exponent
    { 0x9F45, TAG_ATTR_B,   TAG_FROM_CARD,   2 , 2  , "DAC"}, // Data Authentication Code
    { 0x9F46, TAG_ATTR_B,   TAG_FROM_CARD,   0 , 248, "Icc public key cert"}, // ICC Public Key Certificate
    { 0x9F47, TAG_ATTR_B,   TAG_FROM_CARD,   1 , 3  , "Icc public key exp"}, // ICC Public Key Exponent
    { 0x9F48, TAG_ATTR_B,   TAG_FROM_CARD,   0 , 252, "Icc public key remainder"}, // ICC Public Key Remainder
    { 0x9F49, TAG_ATTR_B,   TAG_FROM_CARD,   0 , 252, "DDOL"}, // Dynamic Data Object List(DDOL)
    { 0x9F4A, TAG_ATTR_B,   TAG_FROM_CARD,   0 , 252, "Static data Authentication tag list"}, // Static Data Authentication Tag List
    { 0x9F4B, TAG_ATTR_B,   TAG_FROM_CARD,   0 , 248, "Signed Dynamic Application Data"}, // Signed Dynamic Application Data
    { 0x9F4C, TAG_ATTR_B,   TAG_FROM_CARD,   2 , 8  , "ICC Dynamic Number"}, // ICC Dynamic Number
    { 0x9F4D, TAG_ATTR_B,   TAG_FROM_CARD,   2 , 2  , "Log Entry"}, // Log Entry
    { 0x9F4E, TAG_ATTR_ANS, TAG_FROM_TERM,   0 , 255, "Merchant Name and Location"}, // Merchant Name and Location
    { 0x9F4F, TAG_ATTR_B,   TAG_FROM_CARD,   0 , 255, "Log format"}, // Log Format

// Tag9F51-Tag9F7Bֻ��pboc2.0�淶�������������ױ���/**/ע�͵�TagΪ��Ƭ�ڲ����ݣ����Ĳ��ع��Ĵ���Tag
	{ 0x9F51, TAG_ATTR_N,   TAG_FROM_CARD,   3 , 3  , "Application currency code"}, // Application Currency Code
#ifdef NEED_DEFINE_ISS_TAG
/**/{ 0x9F52, TAG_ATTR_B,   TAG_FROM_CARD,   2 , 2  , "Application default action"}, // Application Default Action
/**/{ 0x9F53, TAG_ATTR_B,   TAG_FROM_CARD,   1 , 1  , "Transaction Category Code"}, // Transaction Category Code
/**/{ 0x9F54, TAG_ATTR_N,   TAG_FROM_CARD,   12, 12 , "Cumulative Total Transaction Amount Limit"}, // Cumulative Total Transaction Amount Limit
#endif
//Visa˽�����ݣ�Emv��pboc�淶���޴˶���	{ 0x9F55, TAG_ATTR_B,   TAG_FROM_CARD,   1 , 1  , "Geographic Indicator"}, // Geographic Indicator
#ifdef NEED_DEFINE_ISS_TAG
/**/{ 0x9F56, TAG_ATTR_B,   TAG_FROM_CARD,   1 , 1  , "Issuer Authentication Indicator"}, // Issuer Authentication Indicator
/**/{ 0x9F57, TAG_ATTR_N,   TAG_FROM_CARD,   3 , 3  , "Issuer country code"}, // Issuer Country Code
/**/{ 0x9F58, TAG_ATTR_B,   TAG_FROM_CARD,   1 , 1  , "LCOL"}, // Lower Consecutive Offline Limit (Proprietary)
/**/{ 0x9F59, TAG_ATTR_B,   TAG_FROM_CARD,   1 , 1  , "UCOL"}, // Upper Consecutive Offline Limit (Proprietary)
#endif
	{ 0x9F5A, TAG_ATTR_ANS, TAG_FROM_CARD,   0 , 255, "Issuer URL2"}, // Issuer URL2
#ifdef NEED_DEFINE_ISS_TAG
/**/{ 0x9F5C, TAG_ATTR_N,   TAG_FROM_CARD,   12, 12 , "Cumulative Total Transaction Amount Upper Limit"}, // Cumulative Total Transaction Amount Upper Limit
#endif
    { 0x9F5D, TAG_ATTR_N,   TAG_FROM_CARD,   12, 12 , "available offline amount"}, // qPBOC: Available Offline spending Amount
    { 0x9F61, TAG_ATTR_AN,  TAG_FROM_CARD,   1 , 40 , "holder id"}, // pboc2.0 Holder Id
    { 0x9F62, TAG_ATTR_B,   TAG_FROM_CARD,   1 , 1  , "holder id type"}, // pboc2.0 Holder Id Type
    { 0x9F63, TAG_ATTR_B,   TAG_FROM_CARD,   16, 16 , "pboc2.0 card product id"}, // pboc2.0 ����Ʒ��ʶ��Ϣ
	{ 0x9F66, TAG_ATTR_B,   TAG_FROM_TERM,	 4 , 4  , "contactless support"}, // pboc2.0 contactless support flag
#ifdef NEED_DEFINE_ISS_TAG
/**/{ 0x9F67, TAG_ATTR_B,   TAG_FROM_CARD,   1 , 1  , "qpboc MSD offset"}, // qPBOC: MSD offset
/**/{ 0x9F68, TAG_ATTR_B,   TAG_FROM_CARD,   4 , 4  , "qpboc card additional process"}, // qPBOC: Card Additional Processes
#endif 
   	{ 0x9F69, TAG_ATTR_B,	TAG_FROM_CARD,	 8 , 16 , "Card Authentication Related Data"}, // ��Ƭ��֤�������
#ifdef NEED_DEFINE_ISS_TAG
/**/{ 0x9F6B, TAG_ATTR_N,   TAG_FROM_CARD,	 6 , 6  , "qpboc card holder verification method limit"}, // ��Ƭ�ֿ�����֤�����޶�
#endif
	{ 0x9F6C, TAG_ATTR_B,   TAG_FROM_CARD,   2 , 2  , "Card Transaction Qualifiers"}, // �������豸ָ����ƬҪ����һ��CVM
    { 0x9F6D, TAG_ATTR_N,   TAG_FROM_CARD,   12, 12 , "EC Reset Threshold"}, // eCash: Reset Threshold
#ifdef NEED_DEFINE_ISS_TAG
/**/{ 0x9F72, TAG_ATTR_B,   TAG_FROM_CARD,   1 , 1  , "Consecutive Transaction Limit(international-country)"}, // Consecutive Transaction Limit (international - country)
/**/{ 0x9F73, TAG_ATTR_N,   TAG_FROM_CARD,   8 , 8  , "Currency Conversion Factor"}, // Currency Conversion Factor
#endif
	{ 0x9F74, TAG_ATTR_A,   TAG_FROM_CARD,   6 , 6  , "EC Issuer Authorization Code"}, // EC Issuer Authorization Code
#ifdef NEED_DEFINE_ISS_TAG
/**/{ 0x9F75, TAG_ATTR_N,   TAG_FROM_CARD,   12, 12 , "Total Transaction Amount Limit - Dual Currency"}, // Cumulative Total Transaction Amount Limit - Dual Currency
/**/{ 0x9F76, TAG_ATTR_N,   TAG_FROM_CARD,   3 , 3  , "Secondary Application Currency Code"}, // Secondary Application Currency Code
#endif
	{ 0x9F77, TAG_ATTR_N,   TAG_FROM_CARD,   12, 12 , "EC Balance Limit"}, // EC Balance Limit
	{ 0x9F78, TAG_ATTR_N,   TAG_FROM_CARD,   12, 12 , "EC Single Transaction Limit"}, // EC Single Transaction Limit
	{ 0x9F79, TAG_ATTR_N,   TAG_FROM_CARD,   12, 12 , "EC Balance"}, // EC Balance
    { 0x9F7A, TAG_ATTR_B,   TAG_FROM_TERM,   1 , 1  , "EC Terminal Support Indicator"}, // EC Terminal Support Indicator, �뿨Ƭ����
    { 0x9F7B, TAG_ATTR_N,   TAG_FROM_TERM,   12, 12 , "EC Terminal Transaction Limit"}, // EC Terminal Transaction Limit
// Tag9F51-Tag9F7Bֻ��pboc2.0�淶��������
#ifdef NEED_DEFINE_ISS_TAG
/**/{ 0x9F7F, TAG_ATTR_ANS, 0 , 255, ""}, // Card Product Life Cycle History File Identifiers
#endif

    { 0xA500, TAG_ATTR_B,   TAG_FROM_CARD,   0 , 255, "FCI template"}, // File Control Information(FCI) Proprietary Template
    { 0xBF0C, TAG_ATTR_B,   TAG_FROM_CARD,   0 , 222, "FCI Issuer Discretionary Data"}, // File Control Information(FCI) Issuer Discretionary Data

// Pboc2.0�Զ���Tag
	{ 0xDF31, TAG_ATTR_B,   TAG_FROM_TERM,   0 , 80,  "Script result"}, //  Script result, 5�ֽ�һ�飬���֧��16���ű���ʶ(71&72)
// Pboc3.0 EC�Զ���Tag
	{ 0xDF4D, TAG_ATTR_B,   TAG_FROM_CARD,   2 , 2,   "Load Log Entry"}, // Load Log Entry(Ȧ����־���)
	{ 0xDF4F, TAG_ATTR_B,   TAG_FROM_CARD,   0 , 255, "Load Log Format"}, // Load Log Format(Ȧ����־��ʽ)
	{ 0xDF71, TAG_ATTR_N,	TAG_FROM_CARD,   3 , 3,   "Application second currency code"}, // Application Second Currency Code(Pboc3.0 ˫�ҵ����ֽ��ڲ�����)
	{ 0xDF79, TAG_ATTR_N,	TAG_FROM_CARD,   12, 12,  "Second EC Balance"}, // EC Balance(�ڶ�����)

// �������Զ���Tag
// Ϊ��ֹ��Pboc2.0��Emv��ͻ, �Զ���Tagʹ�����ֽ�Tag, �ҵڶ��ֽڵ����λ��Ϊ1, ��Լ��������Tag���Ȳ��ܳ���2�ֽ�
    { 0xDF81, TAG_ATTR_B,   TAG_FROM_TERM,   0 , 252, "Default DDOL"}, // default ddol
    { 0xDF82, TAG_ATTR_B,   TAG_FROM_TERM,   0 , 252, "Default TDOL"}, // default tdol
    { 0xDF83, TAG_ATTR_B,   TAG_FROM_TERM,   8 , 8,   "Enciphered online PIN"}, // enciphered online pin
    { 0xDF84, TAG_ATTR_B,   TAG_FROM_TERM,   1,  1,   "Online PIN support"}, // online pin support flag, 1:֧�� 0:��֧��
    { 0xDF87, TAG_ATTR_B,   TAG_FROM_TERM,   1 , 1,   "Max target percentage"}, // Maximum Target Percentage to be used for Biased Random Selection
    { 0xDF88, TAG_ATTR_B,   TAG_FROM_TERM,   4 , 4,   "Threshold value"}, // Threshold Value for Biased Random Selection

    { 0xDF8A, TAG_ATTR_B,   TAG_FROM_TERM,   1 , 1,   "Target percentage"}, // Target Percentage to be Used for Random

    { 0xDF8C, TAG_ATTR_B,   TAG_FROM_TERM,   5 , 5,   "Tac-default"}, // TAC-default
    { 0xDF8D, TAG_ATTR_B,   TAG_FROM_TERM,   5 , 5,   "Tac-denial"}, // TAC-denial
    { 0xDF8E, TAG_ATTR_B,   TAG_FROM_TERM,   5 , 5,   "Tac-online"}, // TAC-online
	{ 0xDF90, TAG_ATTR_A,   TAG_FROM_TERM,   2 , 2,   "Local language"}, // ��������(����Ա), ISO-639-1
    { 0xDF91, TAG_ATTR_B,   TAG_FROM_TERM,   1 , 1,   "Reader capability"}, // 1:ֻ֧�ִſ� 2:ֻ֧��IC�� 3:֧�����ʽMag/IC������ 4:֧�ַ���ʽMag/IC������
    { 0xDF92, TAG_ATTR_B,   TAG_FROM_TERM,   1 , 1,   "Voice referral support"}, // 1:֧�� 2:��֧��,ȱʡapprove 3:��֧��,ȱʡdecline
    { 0xDF93, TAG_ATTR_B,   TAG_FROM_TERM,   1 , 1,   "Force online support"}, // 1:֧�� 0:��֧��
    { 0xDF94, TAG_ATTR_B,   TAG_FROM_TERM,   1 , 1,   "Force Accept support"}, // 1:֧�� 0:��֧��
    { 0xDF95, TAG_ATTR_B,   TAG_FROM_TERM,   1 , 1,   "Transaction log support"}, // 1:�ն�֧�ֽ�����ˮ��¼ 0:��֧��
    { 0xDF96, TAG_ATTR_B,   TAG_FROM_TERM,   1 , 1,   "Blacklist support"}, // 1:�ն�֧�ֺ�������� 0:��֧��
    { 0xDF97, TAG_ATTR_B,   TAG_FROM_TERM,   1 , 1,   "Separate sale support"}, // 1:֧�ַֿ����Ѽ�� 0:��֧��
    { 0xDF98, TAG_ATTR_B,   TAG_FROM_TERM,   1 , 1,   "Application confirm support"}, // 1:֧��Ӧ��ȷ�� 0:��֧��Ӧ��ȷ��
    { 0xDF99, TAG_ATTR_A,   TAG_FROM_TERM,   2 , 32,  "Pinpad language"}, // ISO-639-1, Pinpad֧�ֵ�����
    { 0xDF9A, TAG_ATTR_A,   TAG_FROM_TERM,   2 , 2,   "Language to be used"}, // ����ʹ�õ�����(�ֿ���)��iso-639-1
    { 0xDF9B, TAG_ATTR_B,   TAG_FROM_TERM,   1 , 1  , "EC Terminal Support Indicator"}, // EC Terminal Support Indicator, �ն�����
	{ 0xDF9C, TAG_ATTR_B,   TAG_FROM_TERM,   0 , 255, "Pdol data block"}, // PDOL���ݿ�, CDAʹ��
	{ 0xDF9D, TAG_ATTR_B,   TAG_FROM_TERM,   0 , 255, "Cdol1 data block"}, // CDOL1���ݿ�, CDAʹ��
	{ 0xDF9E, TAG_ATTR_B,   TAG_FROM_TERM,   0 , 255, "Cdol2 data block"}, // CDOL2���ݿ�, CDAʹ��
	{ 0xDF9F, TAG_ATTR_B,   TAG_FROM_TERM,   1 , 1,   "ASI"}, // Ӧ��ѡ��ָʾ 0:��������ƥ�䣬1:ȫ������ƥ��
	{ 0xDFA0, TAG_ATTR_B,   TAG_FROM_TERM,   32, 248, "CA public key modulus"}, // CA��Կģ��
	{ 0xDFA1, TAG_ATTR_B,   TAG_FROM_TERM,   1,  3,   "CA public key exponent"}, // CA��Կָ��
	{ 0xDFA2, TAG_ATTR_B,   TAG_FROM_TERM,   5,  5,   "CA public key RID"}, // CA��Կ����RID
	{ 0xDFA3, TAG_ATTR_B,   TAG_FROM_TERM,   1,  1,   "CA public key hash algo"}, // CA��ԿHash�㷨
	{ 0xDFA4, TAG_ATTR_B,   TAG_FROM_TERM,   1,  1,   "CA public key algo"}, // CA��Կ�㷨
	{ 0xDFA5, TAG_ATTR_N,   TAG_FROM_TERM,   8,  8,   "CA public key expire date"}, // CA��Կ��Ч��
	{ 0xDFA6, TAG_ATTR_B,   TAG_FROM_TERM,   20, 20,  "CA pub key hash value"}, // CA��ԿHashֵ
    { 0xDFA7, TAG_ATTR_B,   TAG_FROM_TERM,   1 , 1,   "Ramdom number for biased random select"}, // Ramdom number for biased random select
    { 0xDFA8, TAG_ATTR_B,   TAG_FROM_TERM,   1 , 1,   "PIN bypass behavior"}, // PIN bypass���� 0:ÿ��bypassֻ��ʾ�ô�bypass 1:һ��bypass,��������Ϊbypass
    { 0xDFAA, TAG_ATTR_B,   TAG_FROM_TERM,   1 , 1,   "EC transaction flag"}, // EC���ױ�־ 0:��EC���� 1:EC���� 2:GPO��Ƭ����EC��Ȩ��,��δ��ECͨ��
    { 0xDFAB, TAG_ATTR_N,   TAG_FROM_TERM,   4 , 4  , "Native transaction type"}, // �ڲ��ý�������, ��λ����T9Cֵ, ��λ����������Ʒ/���� IC/Mag
	{ 0xDFAC, TAG_ATTR_N,	TAG_FROM_TERM,   6,  6,   "EC Balance Original"}, // ����ǰEC���
	{ 0xDFE6, TAG_ATTR_B,	TAG_FROM_TERM,   4,  4,   "Terminal Ctls Capabilities Mask"}, // �ն˷ǽ���������λ, 9F66ӳ��, ��ʾ�ն���������Ӧλ��ʾ������
	{ 0xDFE7, TAG_ATTR_N,	TAG_FROM_TERM,   12, 12,  "Terminal Ctls Amount Limit"}, // �ն˷ǽӽ����޶�
	{ 0xDFE8, TAG_ATTR_N,	TAG_FROM_TERM,   12, 12,  "Terminal Ctls Offline Amount Limit"}, // �ն˷ǽ��ѻ������޶�
	{ 0xDFE9, TAG_ATTR_N,	TAG_FROM_TERM,   12, 12,  "Terminal Ctls CVM Limit"}, // �ն˷ǽ�CVM�޶�
	{ 0xDFEA, TAG_ATTR_B,	TAG_FROM_TERM,   1 , 1,   "Transaction Route"}, // �����ߵ�·��, 0:�Ӵ�Pboc(��׼DC��EC) 1:�ǽӴ�Pboc(��׼DC��EC) 2:qPboc���� 3:qPboc�ѻ� 4:qPboc�ܾ�
	{ 0xDFF1, TAG_ATTR_B,   TAG_FROM_ISSUER, 0,  255, "Issuer Script 71 Package"}, // 71�ű���, �ڴ�����71�ű�, ע��,��Ҫ�޸�Tagֵ, ����ΪDFF1
	{ 0xDFF2, TAG_ATTR_B,   TAG_FROM_ISSUER, 0,  255, "Issuer Script 72 Package"}, // 72�ű���, �ڴ�����72�ű�, ע��,��Ҫ�޸�Tagֵ, ����ΪDFF2
};

// function : search tag
// In  : psTag : tag
// Ret : >=0 : subscript of the tag
//       -1  : not found
static short _iTagLocate(unsigned char *psTag)
{
    unsigned int uiTag;
    int          i, i1, i2;
    
    uiTag = (unsigned short)psTag[0] << 8;
    if((uiTag & 0x1f00) == 0x1f00)
        uiTag += psTag[1]; // this tag include 2 bytes
    
    i1 = 0;
    i2 = sizeof(gl_aTagAttr) / sizeof(gl_aTagAttr[0]) - 1;
    while(i1 <= i2) {
        i = (i2 - i1) / 2 + i1;
        if(gl_aTagAttr[i].uiTag == uiTag)
            return(i);
        if(gl_aTagAttr[i].uiTag > uiTag)
            i2 = i-1;
        else
            i1 = i+1;
    }
    return(-1);
}

// function : get attribute of the Tag
// In  : psTag : tag
// Ret : TAG_ATTR_N or TAG_ATTR_CN or TAG_ATTR_B or TAG_ATTR_A or TAG_ATTR_AN or TAG_ATTR_ANS
//       or TAG_ATTR_UNKNOWN
unsigned int uiTagAttrGetType(unsigned char *psTag)
{
    int iSubscript;
    
    iSubscript = _iTagLocate(psTag);
    if(iSubscript < 0)
        return(TAG_ATTR_UNKNOWN);
    return((unsigned int)gl_aTagAttr[iSubscript].ucAttr);
}

// function : get origin of the Tag
// In  : psTag : tag
// Ret : TAG_FROM_CARD or TAG_FROM_TERM or TAG_FROM_ISSUER
//       or TAG_FROM_UNKNOWN
unsigned int uiTagAttrGetFrom(unsigned char *psTag)
{
    int iSubscript;
    
    iSubscript = _iTagLocate(psTag);
    if(iSubscript < 0)
        return(TAG_FROM_UNKNOWN);
    return((unsigned int)gl_aTagAttr[iSubscript].ucFrom);
}

// function : get attribute of the Tag
// In  : psTag   : tag
// Out : piLeast : least length available, ��ָ�����������ͼ���ĳ���
//       piMost  : most length available, ��ָ�����������ͼ���ĳ���
// Ret : TAG_ATTR_N or TAG_ATTR_CN or TAG_ATTR_B or TAG_ATTR_A or TAG_ATTR_AN or TAG_ATTR_ANS
//       or TAG_ATTR_UNKNOWN
unsigned int uiTagAttrGetRange(unsigned char *psTag, int *piLeast, int *piMost)
{
    int iSubscript;
    
    iSubscript = _iTagLocate(psTag);
    if(iSubscript < 0)
        return(TAG_ATTR_UNKNOWN);
    *piLeast = (int)gl_aTagAttr[iSubscript].ucRange1;
    *piMost = (int)gl_aTagAttr[iSubscript].ucRange2;
    return((unsigned int)gl_aTagAttr[iSubscript].ucAttr);
}

// function : get description of the Tag
// In  : psTag : tag
// Ret : description or ""
unsigned char *psTagAttrGetDesc(unsigned char *psTag)
{
    int iSubscript;
    
    iSubscript = _iTagLocate(psTag);
    if(iSubscript < 0)
        return("UNKNOWN Tag");
    return((char *)gl_aTagAttr[iSubscript].pszDesc);
}

# if 0
void vTwoOne(const unsigned char *psIn, unsigned short uiLength, unsigned char *psOut)
{
    unsigned char  ucTmp;
    unsigned short i;

    for(i=0; i<uiLength; i+=2) {
        ucTmp = psIn[i];
        if(ucTmp > '9')
            ucTmp = toupper(ucTmp) - 'A' + 0x0a;
        else
            ucTmp &= 0x0f;
        psOut[i/2] = ucTmp << 4;

        ucTmp = psIn[i+1];
        if(ucTmp > '9')
            ucTmp = toupper(ucTmp) - 'A' + 0x0a;
        else
            ucTmp &= 0x0f;
        psOut[i/2] += ucTmp;
    } // for(i=0; i<uiLength; i+=2) {
}
void main(int argc, char *argv[])
{
    unsigned char  sTag[2];
    unsigned int   uiTagAttr;
    unsigned int   uiTagFrom;    
    unsigned char  szBuf1[50], szBuf2[50], szBuf3[70];
    short          iLeast, iMost;
    
    if(argc != 2) {
        printf("need input tag\n");
        return;
    }
    if(strlen(argv[1])!=2 && strlen(argv[1])!=4) {
        printf("a tag can be only 2 or 4 bytes in hexadecimal\n");
        return;
    }

    vTwoOne(argv[1], 4, sTag);
    uiTagAttr = uiTagAttrGetType(sTag);
    switch(uiTagAttr) {
        case TAG_ATTR_N :
            strcpy(szBuf1, "NUMERIC");
            break;
        case TAG_ATTR_CN :
            strcpy(szBuf1, "COMPACTED NUMERIC");
            break;
        case TAG_ATTR_B :
            strcpy(szBuf1, "BINARY");
            break;
        case TAG_ATTR_A :
            strcpy(szBuf1, "ALPHA");
            break;
        case TAG_ATTR_AN :
            strcpy(szBuf1, "ALPHA-NUMERIC");
            break;
        case TAG_ATTR_ANS :
            strcpy(szBuf1, "ALPHA-NUMERIC-SPECIAL");
            break;
        default :
            strcpy(szBuf1, "UNKNOWN");
            break;
    }

    uiTagFrom = uiTagAttrGetFrom(sTag);
    switch(uiTagFrom) {
        case TAG_FROM_CARD :
            strcpy(szBuf2, "FROM CARD");
            break;
        case TAG_FROM_TERM :
            strcpy(szBuf2, "FROM TERMINAL");
            break;
        case TAG_FROM_ISSUER :
            strcpy(szBuf2, "FROM ISSUER");
            break;
        default :
            strcpy(szBuf2, "FROM UNKNOWN");
            break;
    }
    
    strcpy(szBuf3, psTagAttrGetDesc(sTag));

    uiTagAttr = uiTagAttrGetRange(&iLeast, &iMost);

    printf("The attribute of tag %s is %s\n", argv[1], szBuf1);
    if(uiTagAttr != TAG_ATTR_UNKNOWN) {
        printf("The tag is %s\n", argv[1], szBuf2);
        printf("Tag desc : [%s]", szBuf3);
        printf("The least length available is %d\n", (int)iLeast);
        printf("The most length available is %d\n", (int)iMost);
    }
}
# endif