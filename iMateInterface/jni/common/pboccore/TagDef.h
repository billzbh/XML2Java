/**************************************
File name     : TagDef.h
Function      : define emv2004 tag value
Author        : Yu Jun
First edition : Mar 26th, 2012
Note          : Refer EMV2004 Specification Book3 Part IV, Annex B
                                                  Part IV, Annex A
                Refer <�й����ڼ��ɵ�·��IC�����淶��1����13���֣��ϲ��棩.pdf>
�������      : EMV�淶�����Tag�����ʽʽ��Ϊ TAG_NNnn_DESC
                    TAG�ͱ�ʾΪTAG, NNnnΪ2��4�ֽ�Tagֵ, DESCΪTag����
					����TAG_4F_AID
					    TAG_5F20_HolderName
				��ҵ�Զ����Tag�����ʽʽ��Ϊ TAG_DFnn_ScriptResult
				    TAG�ͱ�ʾΪTAG, DFnnΪ4�ֽ�Tagֵ, DESCΪTag����
					����TAG_DF31_ScriptResult
				��EMV�����Զ���Tag�����ʽΪ TAG_DFXX_DESC
				    TAG�ͱ�ʾΪTAG, DFXX�ͱ�ʾΪDFXX, DESCΪTag����
					����TAG_DFXX_TACDefault
					֮����Tagֵ���þ�����ֵ, ��Ϊ�˷����Զ���Tag�����ĸ���Tagֵ
�Զ���Tag˵�� : ��EMV�����Զ���Tagʹ��2�ֽ�Tag, ��1�ֽ�ΪDF, �ڶ��ֽ�ȡֵ��ΧΪ80-FE
                    ���ڹ�����ҵ�Զ���Tag��ȫ�޷�׽��, Ϊ�˱����ͻ, �����Զ���Tag���õ�2�ֽڸ�λΪ1�ı���
					������TAG��BER_TLV����Ϊ3�ֽ�����Tag��ǩ, ��˹�����ҵ������ʹ��
					������ֻ֧��2�ֽ�Tag��ǩ, ��2�ֽڸ�λΪ1�����ͳ�Tag��ǩ��չλ, ���ں����Զ���Tag��ǩ
**************************************/
#ifndef _TAGDEF_H
#define _TAGDEF_H

#define TAG_42_IIN                          "\x42"      // Issuer Identification Number (IIN)
#define TAG_4F_AID                          "\x4F"      // Application Identifier(AID)
#define TAG_50_AppLabel                     "\x50"      // Application label
#define TAG_57_Track2EquData                "\x57"      // Track 2 Equivalent Data
#define TAG_5A_PAN                          "\x5A"      // Application Primary Account Number(PAN)
#define TAG_5F20_HolderName                 "\x5F\x20"  // Cardholder Name
#define TAG_5F24_AppExpDate                 "\x5F\x24"  // Application Expiration Date
#define TAG_5F25_AppEffectiveDate           "\x5F\x25"  // Application Effective Date
#define TAG_5F28_IssuerCountryCode          "\x5F\x28"  // Issuer Country Code
#define TAG_5F2A_TransCurrencyCode          "\x5F\x2A"  // Transaction Currency Code
#define TAG_5F2D_LanguagePrefer             "\x5F\x2D"  // Language Preference
#define TAG_5F30_ServiceCode                "\x5F\x30"  // Service Code
#define TAG_5F34_PANSeqNo                   "\x5F\x34"  // Application Primary Account Number Sequence Number
#define TAG_5F36_TransCurrencyExp           "\x5F\x36"  // Transaction Currency Exponent
#define TAG_5F50_IssuerURL                  "\x5F\x50"  // Issuer URL
#define TAG_5F53_IBAN                       "\x5F\x53"  // International Bank Account Number (IBAN)
#define TAG_5F54_BIC                        "\x5F\x54"  // Bank Identifier Code (BIC)
#define TAG_5F55_IssuerCountryCodeA2        "\x5F\x55"  // Issuer Country Code (alpha2 format)
#define TAG_5F56_IssuerCountryCodeA3        "\x5F\x56"  // Issuer Country Code (alpha3 format)
#define TAG_5F57_AccountType                "\x5F\x57"  // Account Type
#define TAG_61_AppTemplate                  "\x61"      // Application Template
#define TAG_6F_FCITemplate                  "\x6F"      // File Control Infomation Template
#define TAG_70_AEFDataTemplate              "\x70"      // Application Elementary File Data Template
#define TAG_71_ScriptTemplate1              "\x71"      // Issuer Script Template 1
#define TAG_72_ScriptTemplate2              "\x72"      // Issuer Script Template 2
#define TAG_73_DirDiscTemplate              "\x73"      // Directory Discretionary Template
#define TAG_77_RspMsgTemplateFmt2           "\x77"      // Response Message Template Format 2
#define TAG_80_RspMsgTemplateFmt1           "\x80"      // Response Message Template Format 1
#define TAG_81_AmountB                      "\x81"      // Amount, Authorised(Binary)
#define TAG_82_AIP                          "\x82"      // Application Interchange Profile
#define TAG_83_CmdTemplate                  "\x83"      // Command Template
#define TAG_84_DFName                       "\x84"      // Dedicated File(DF) Name
#define TAG_86_IssuerScriptCmd              "\x86"      // Issuer Script command  ???? �ĵ��涨���261 ????
#define TAG_87_AppPriorIndicator            "\x87"      // Application Priority Indicator
#define TAG_88_SFI                          "\x88"      // Short File Identifier
#define TAG_89_AuthCode                     "\x89"      // Authorisation Code
#define TAG_8A_AuthRspCode                  "\x8A"      // Authorisation Response Code
#define TAG_8C_CDOL1                        "\x8C"      // Card Risk Management Data Object List 1(CDOL1)
#define TAG_8D_CDOL2                        "\x8D"      // Card Risk Management Data Object List 2(CDOL2)
#define TAG_8E_CVMList                      "\x8E"      // Cardholder Verification Method(CVM) List
#define TAG_8F_CAPubKeyIndex                "\x8F"      // Certification Authority Public Key Index
#define TAG_90_IssuerPubKeyCert             "\x90"      // Issuer Public Key Certificate
#define TAG_91_IssuerAuthData               "\x91"      // Issuer Authentication Data
#define TAG_92_IssuerPubKeyRem              "\x92"      // Issuer Public Key Remainder
#define TAG_93_SSAD                         "\x93"      // Signed Static Application Data
#define TAG_94_AFL                          "\x94"      // Application File Locator(AFL)
#define TAG_95_TVR                          "\x95"      // Terminal Verification Results
#define TAG_97_TDOL                         "\x97"      // Transaction Certificate Data Object List(TDOL)
#define TAG_98_TCHashValue                  "\x98"      // Transaction Certificate(TC) Hash Value
#define TAG_99_TransPINData                 "\x99"      // Transaction Personal Identification Number(PIN) Data
#define TAG_9A_TransDate                    "\x9A"      // Transaction Date
#define TAG_9B_TSI                          "\x9B"      // Transaction Status Information
#define TAG_9C_TransType                    "\x9C"      // Transaction Type
#define TAG_9D_DDFName                      "\x9D"      // Directory Definition File(DDF) Name
#define TAG_9F01_AcquirerId                 "\x9F\x01"  // Acquirer Identifier
#define TAG_9F02_AmountN                    "\x9F\x02"  // Amount, Authorised(Numeric)
#define TAG_9F03_AmountOtherN               "\x9F\x03"  // Amount, Other(Numeric)
#define TAG_9F04_AmountOtherB               "\x9F\x04"  // Amount, Other(Binary)
#define TAG_9F05_AppDiscData                "\x9F\x05"  // Application Discretionary Data
#define TAG_9F06_AID                        "\x9F\x06"  // Application Identifier(AID)
#define TAG_9F07_AUC                        "\x9F\x07"  // Application Usage Control
#define TAG_9F08_AppVerCard                 "\x9F\x08"  // Application Version Number, Card
#define TAG_9F09_AppVerTerm                 "\x9F\x09"  // Application Version Number, Terminal
#define TAG_9F0B_HolderNameExt              "\x9F\x0B"  // Cardholder Name Extended
#define TAG_9F0D_IACDefault                 "\x9F\x0D"  // Issuer Action Code - Default
#define TAG_9F0E_IACDenial                  "\x9F\x0E"  // Issuer Action Code - Denial
#define TAG_9F0F_IACOnline                  "\x9F\x0F"  // Issuer Action Code - online
#define TAG_9F10_IssuerAppData              "\x9F\x10"  // Issuer Application Data
#define TAG_9F11_IssuerCodeTableIndex       "\x9F\x11"  // Issuer Code Table Index
#define TAG_9F12_AppPrefName                "\x9F\x12"  // Application Preferred Name
#define TAG_9F13_LastOnlineATC              "\x9F\x13"  // Last Online Application Transaction Counter(ATC) Register
#define TAG_9F14_LCOL                       "\x9F\x14"  // Lower Consecutive Offline Limit
#define TAG_9F15_MerchantCategoryCode       "\x9F\x15"  // Merchant Category Code
#define TAG_9F16_MerchantId                 "\x9F\x16"  // Merchant Identifier
#define TAG_9F17_PINTryCounter              "\x9F\x17"  // Personal Identification Number(PIN) Try Counter
#define TAG_9F18_ScriptId                   "\x9F\x18"  // Issuer Script Identifier
#define TAG_9F1A_TermCountryCode            "\x9F\x1A"  // Terminal Country Code
#define TAG_9F1B_TermFloorLimit             "\x9F\x1B"  // Terminal Floor Limit
#define TAG_9F1C_TermId                     "\x9F\x1C"  // Terminal Identification
#define TAG_9F1D_TermRistManaData           "\x9F\x1D"  // Terminal Risk Management Data
#define TAG_9F1E_IFDSerNo                   "\x9F\x1E"  // Interface Device(IFD) Serial Number
#define TAG_9F1F_Track1DiscData             "\x9F\x1F"  // Track 1 Discretionary Data
#define TAG_9F20_Track2DiscData             "\x9F\x20"  // Track 2 Discretionary Data
#define TAG_9F21_TransTime                  "\x9F\x21"  // Transaction Time
#define TAG_9F22_CAPubKeyIndex              "\x9F\x22"  // Certification Authority Public Key Index
#define TAG_9F23_UCOL                       "\x9F\x23"  // Upper Consecutive Offline Limit
#define TAG_9F26_AC                         "\x9F\x26"  // Application Cryptogram
#define TAG_9F27_CID                        "\x9F\x27"  // Cryptogram information Data
#define TAG_9F2D_ICCPinEncPubKeyCert        "\x9F\x2D"  // ICC PIN Encipherment Public Key Certificate
#define TAG_9F2E_ICCPinEncPubKeyExp         "\x9F\x2E"  // ICC PIN Encipherment Public Key Exponent
#define TAG_9F2F_ICCPinEncPubKeyRem         "\x9F\x2F"  // ICC PIN Encipherment Public Key Remainder
#define TAG_9F32_IssuerPubKeyExp            "\x9F\x32"  // Issuer Public Key Exponent
#define TAG_9F33_TermCapability             "\x9F\x33"  // Terminal Capabilities
#define TAG_9F34_CVMResult                  "\x9F\x34"  // Cardholder Verification Method(CVM) Results
#define TAG_9F35_TermType                   "\x9F\x35"  // Terminal Type
#define TAG_9F36_ATC                        "\x9F\x36"  // Application Transaction Counter(ATC)
#define TAG_9F37_TermRand                   "\x9F\x37"  // Unpredictable Number
#define TAG_9F38_PDOL                       "\x9F\x38"  // Processing Options Data Object List(PDOL)
#define TAG_9F39_POSEntryMode               "\x9F\x39"  // Point-of-Service(POS) Entry Mode
#define TAG_9F3A_AmountRefCurrency          "\x9F\x3A"  // Amount, Reference Currency
#define TAG_9F3B_AppRefCurrency             "\x9F\x3B"  // Appilcation Reference Currency
#define TAG_9F3C_TransRefCurrency           "\x9F\x3C"  // Transaction Reference Currency
#define TAG_9F3D_TransRefCurrencyExp        "\x9F\x3D"  // Transaction Reference Currency Exponent
#define TAG_9F40_TermCapabilityExt          "\x9F\x40"  // Additional Terminal Capabilities
#define TAG_9F41_TransSeqCounter            "\x9F\x41"  // Transaction Sequence Counter
#define TAG_9F42_AppCurrencyCode            "\x9F\x42"  // Application Currency Code
#define TAG_9F43_AppRefCurrencyExp          "\x9F\x43"  // Application Reference Currency Exponent
#define TAG_9F44_AppCurrencyExp             "\x9F\x44"  // Application Curerncy Exponent
#define TAG_9F45_DAC                        "\x9F\x45"  // Data Authentication Code
#define TAG_9F46_ICCPubKeyCert              "\x9F\x46"  // ICC Public Key Certificate
#define TAG_9F47_ICCPubKeyExp               "\x9F\x47"  // ICC Public Key Exponent
#define TAG_9F48_ICCPubKeyRem               "\x9F\x48"  // ICC Public Key Remainder
#define TAG_9F49_DDOL                       "\x9F\x49"  // Dynamic Data Object List(DDOL)
#define TAG_9F4A_SDATagList                 "\x9F\x4A"  // Static Data Authentication Tag List
#define TAG_9F4B_SDAData                    "\x9F\x4B"  // Signed Dynamic Application Data
#define TAG_9F4C_ICCDynNumber               "\x9F\x4C"  // ICC Dynamic Number
#define TAG_9F4D_LogEntry                   "\x9F\x4D"  // Log Entry
#define TAG_9F4E_MerchantName               "\x9F\x4E"  // Merchant Name and Location
#define TAG_9F4F_LogFmt_I                   "\x9F\x4F"  // Log Format
#define TAG_9F51_AppCurrencyCode_I			"\x9F\x51"  // Application Currency Code(Pboc2.0 ���ڲ�����)
#define TAG_9F5A_IssuerURL2                 "\x9F\x5A"  // Issuer URL2
#define TAG_9F5D_OfflineAmount				"\x9F\x5D"	// qPBOC: Available Offline spending Amount
#define TAG_9F61_HolderId                   "\x9F\x61"  // pboc2.0 Holder Id
#define TAG_9F62_HolderIdType               "\x9F\x62"  // pboc2.0 Holder Id Type
#define TAG_9F63_CardProducId               "\x9F\x63"  // pboc2.0 ����Ʒ��ʶ��Ϣ
#define TAG_9F66_ContactlessSupport			"\x9F\x66"	// pboc2.0 contactless support flag
#define TAG_9F69_CardAuthRelatedData		"\x9F\x69"	// Card Authentication Related Data
#define TAG_9F6C_CardTransQualifiers		"\x9F\x6C"  // Card Transaction Qualifiers
#define TAG_9F6D_ECResetThreshold_I         "\x9F\x6D"  // EC Reset Threshold
#define TAG_9F74_ECIssuerAuthCode           "\x9F\x74"  // EC Issuer Authorization Code
#define TAG_9F77_ECBalanceLimit_I           "\x9F\x77"  // EC Balance Limit
#define TAG_9F78_ECSingleTransLimit_I       "\x9F\x78"  // EC Single Transaction Limit
#define TAG_9F79_ECBalance_I                "\x9F\x79"  // EC Balance
#define TAG_9F7A_ECTermSupportIndicator     "\x9F\x7A"  // EC Terminal Support Indicator
#define TAG_9F7B_ECTermTransLimit           "\x9F\x7B"  // EC Terminal Transaction Limit
#define TAG_A5_FCIProprietaryTemplate       "\xA5"      // File Control Information(FCI) Proprietary Template
#define TAG_BF0C_FCIIssuerDiscData          "\xBF\x0C"  // File Control Information(FCI) Issuer Discretionary Data

// Pboc3.0�Զ���Tag
#define TAG_DF31_ScriptResult               "\xDF\x31"  // Script result �㷢��̨�趨�ű�ִ�н��TagΪDF31
// Pboc3.0 EC�Զ���Tag
#define TAG_DF4D_LoadLogEntry               "\xDF\x4D"  // Load Log Entry(Ȧ����־���)
#define TAG_DF4F_LoadLogFmt_I               "\xDF\x4F"  // Load Log Format(Ȧ����־��ʽ)
#define TAG_DF71_AppCurrencyCode2_I			"\xDF\x71"  // Application Second Currency Code(Pboc3.0 ˫�ҵ����ֽ��ڲ�����)
#define TAG_DF79_ECBalance2_I				"\xDF\x79"  // EC Balance(�ڶ�����)

// �������Զ���Tag
// Ϊ��ֹ��Pboc2.0��Emv��ͻ, �Զ���Tagʹ�����ֽ�Tag, �ҵڶ��ֽڵ����λ��Ϊ1, ��Լ��������Tag���Ȳ��ܳ���2�ֽ�
#define TAG_DFXX_DefaultDDOL                "\xDF\x81"  // default ddol
#define TAG_DFXX_DefaultTDOL                "\xDF\x82"  // default tdol
#define TAG_DFXX_EncOnlinePIN               "\xDF\x83"  // enciphered online pin
#define TAG_DFXX_OnlinePINSupport           "\xDF\x84"  // online pin support, 1:֧�� 0:��֧��
#define TAG_DFXX_MaxTargetPercentage        "\xDF\x87"  // Maximum Target Percentage to be used for Biased Random Selection
#define TAG_DFXX_RandomSelThreshold         "\xDF\x88"  // Threshold Value for Biased Random Selection
#define TAG_DFXX_TargetPercentage           "\xDF\x8A"  // Target Percentage to be Used for Random
#define TAG_DFXX_TACDefault                 "\xDF\x8C"  // TAC-default
#define TAG_DFXX_TACDenial                  "\xDF\x8D"  // TAC-denial
#define TAG_DFXX_TACOnline                  "\xDF\x8E"  // TAC-online
#define TAG_DFXX_LocalLanguage				"\xDF\x90"  // ��������(����Ա), ISO-639-1
#define TAG_DFXX_ReaderCapability			"\xDF\x91"  // ���������� 1:ֻ֧�ִſ� 2:ֻ֧��IC�� 3:֧�����ʽMag/IC������ 4:֧�ַ���ʽMag/IC������
#define TAG_DFXX_VoiceReferralSupport       "\xDF\x92"  // Voice referral֧�� 1:֧�� 2:��֧��,ȱʡapprove 3:��֧��,ȱʡdecline
#define TAG_DFXX_ForceOnlineSupport         "\xDF\x93"  // ǿ������֧�� 1:֧�� 0:��֧��
#define TAG_DFXX_ForceAcceptSupport         "\xDF\x94"  // ǿ�ƽ���֧�� 1:֧�� 0:��֧��
#define TAG_DFXX_TransLogSupport			"\xDF\x95"  // 1:�ն�֧�ֽ�����ˮ��¼ 0:��֧��
#define TAG_DFXX_BlacklistSupport           "\xDF\x96"  // 1:�ն�֧�ֺ�������� 0:��֧��
#define TAG_DFXX_SeparateSaleSupport        "\xDF\x97"  // 1:֧�ַֿ����Ѽ�� 0:��֧��
#define TAG_DFXX_AppConfirmSupport          "\xDF\x98"	// 1:֧��Ӧ��ȷ�� 0:��֧��Ӧ��ȷ��
#define TAG_DFXX_PinpadLanguage             "\xDF\x99"  // ISO-639-1, Pinpad֧�ֵ����Ի����֧�ֳֿ��˵�����
#define TAG_DFXX_LanguageUsed               "\xDF\x9A"  // ����ʹ�õ�����(�ֿ���)��iso-639-1
#define TAG_DFXX_ECTermSupportIndicator     "\xDF\x9B"  // EC Terminal Support Indicator
#define TAG_DFXX_PdolDataBlock				"\xDF\x9C"	// PDOL���ݿ�, CDAʹ��
#define TAG_DFXX_Cdol1DataBlock				"\xDF\x9D"	// CDOL1���ݿ�, CDAʹ��
#define TAG_DFXX_Cdol2DataBlock				"\xDF\x9E"	// CDOL2���ݿ�, CDAʹ��
#define TAG_DFXX_ASI						"\xDF\x9F"  // Ӧ��ѡ��ָʾ
#define TAG_DFXX_KeyModulus					"\xDF\xA0"  // CA��Կģ��
#define TAG_DFXX_KeyExp						"\xDF\xA1"  // CA��Կָ��
#define TAG_DFXX_KeyRid						"\xDF\xA2"  // CA��Կ����RID
#define TAG_DFXX_KeyHashAlgo				"\xDF\xA3"  // CA��ԿHash�㷨
#define TAG_DFXX_KeyAlgo					"\xDF\xA4"  // CA��Կ�㷨
#define TAG_DFXX_KeyExpireDate				"\xDF\xA5"  // CA��Կ��Ч��
#define TAG_DFXX_KeyHashValue				"\xDF\xA6"  // CA��ԿHashֵ
#define TAG_DFXX_RandForSelect              "\xDF\xA7"  // ���ѡ�������ն����ɵ������
#define TAG_DFXX_PinBypassBehavior          "\xDF\xA8"  // PIN bypass���� 0:ÿ��bypassֻ��ʾ�ô�bypass 1:һ��bypass,��������Ϊbypass
#define TAG_DFXX_ECTransFlag                "\xDF\xAA"  // EC���ױ�־ 0:��EC���� 1:EC���� 2:GPO��Ƭ����EC��Ȩ��,��δ��ECͨ��
#define TAG_DFXX_TransType					"\xDF\xAB"  // ��������, ��λ����T9Cֵ, ��λ����������Ʒ/���� IC/Mag
#define TAG_DFXX_ECBalanceOriginal			"\xDF\xAC"	// ����ǰEC���
#define TAG_DFXX_TermCtlsCapMask			"\xDF\xE6"	// �ն˷ǽ���������λ, 9F66ӳ��, ��ʾ�ն���������Ӧλ��ʾ������
#define TAG_DFXX_TermCtlsAmountLimit		"\xDF\xE7"	// �ն˷ǽӽ����޶�
#define TAG_DFXX_TermCtlsOfflineAmountLimit	"\xDF\xE8"	// �ն˷ǽ��ѻ������޶�
#define TAG_DFXX_TermCtlsCvmLimit			"\xDF\xE9"	// �ն˷ǽ�CVM�޶�
#define TAG_DFXX_TransRoute					"\xDF\xEA"  // �����ߵ�·��, 0:�Ӵ�Pboc(��׼DC��EC) 1:�ǽӴ�Pboc(��׼DC��EC) 2:qPboc���� 3:qPboc�ѻ� 4:qPboc�ܾ�
#define TAG_DFXX_ScriptPackage71            "\xDF\xF1"  // 71�ű���, �ڴ�����71�ű�, ע��,��Ҫ�޸�Tagֵ, ����ΪDFF1
#define TAG_DFXX_ScriptPackage72            "\xDF\xF2"  // 72�ű���, �ڴ�����72�ű�, ע��,��Ҫ�޸�Tagֵ, ����ΪDFF2

#endif
