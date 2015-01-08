/**************************************
File name     : ISSDATA.H
Function      : 声明PBOC2.0 D/C发卡数据相关结构
Author        : Yu Jun
First edition : Jul 3rd, 2009
**************************************/
#ifndef _ISSDATA_H
#define _ISSDATA_H

// 密钥特性
typedef struct {
    int   iIssuerRsaKeyIndex;       // 发卡行RSA密钥索引
    int   iKmcIndex;                // IC卡厂家密钥索引
    int   iAppKeyBaseIndex;         // IC卡应用密钥基索引, 依次为Auth、Mac、Enc、Dcvv
} stKeyRef;

// 卡片特性
typedef struct {
	uchar ucKmcKeyIndex;			// KMC索引号
	uchar ucKmcKeyDivFlag;			// KMC在卡片中是否需要分散
	uchar ucKmcKeyVer;              // KMC版本号
	uchar ucCardManagerAidLen;      // Card Manager Aid长度
	uchar sCardManagerAid[16];		// Card Manager Aid
} stCardRef;

// PBOC实例特性
typedef struct {
	uchar ucMaxAppNum;                  // 最多允许应用个数
	uchar ucJavaPackageAidLen;			// Java Package Aid长度
	uchar sJavaPackageAid[16];			// Java Package Aid
	uchar ucJavaAppletAidLen;			// Java Applet Aid长度
	uchar sJavaAppletAid[16];			// Java Applet Aid
	uchar ucAidLen;						// 应用实例Aid长度
	uchar sAid[16];						// 应用实例Aid
	uchar ucApplicationPrivilegesLen;	// Application Privileges长度
	uchar sApplicationPrivileges[16];   // Application Privileges
} stInstanceRef;

// PBOC D/C 应用数据
typedef struct {
    uchar ucTrack2EquDataLen;           // 二磁道等效数据长度
    uchar sTrack2EquData[19];           // 二磁道等效数据, T57
    uchar szHolderName[46];             // 持卡人姓名, T5F20|T9F0B (<=26:T5F20 >26:T9F0B)
    uchar szHolderId[40+1];             // 持卡人证件号码, T9F61
    uchar ucHolderIdType;               // 持卡人证件类型, T9F62
                                        // 00：身份证 01：军官证 02：护照 03：入境证 04：临时身份证 05：其它
	uchar ucIssuerPkCertificateLen;     // 发卡行公钥证书长度
	uchar sIssuerPkCertificate[248];    // 发卡行公钥证书, T90
	uchar ucIssuerPkExponentLen;        // 发卡行公钥公共指数长度
	uchar sIssuerPkExponent[3];         // 发卡行公钥公共指数, T9F32
	uchar ucIssuerPkRemainderLen;       // 发卡行公钥剩余长度
	uchar sIssuerPkRemainder[36];       // 发卡行公钥剩余, T92
	uchar ucCaPublicKeyIndex;           // CA公钥索引, T8F

    // 标准DC数据
	uchar ucDcSignedStaticDataLen;      // 标准DC签名的应用数据长度
	uchar sDcSignedStaticData[248];     // 标准DC签名的应用数据, T93
    uchar ucDcIcCertificateLen;         // 标准DC的IC卡公钥证书长度
    uchar sDcIcCertificate[248];        // 标准DC的IC卡公钥证书, T9F46
	uchar ucDcCVMListLen;
	uchar sDcCVMList[252];				// Cardholder Verification Method (CVM) List, T8E
	uchar sDcIACDefault[5];				// IAC - default, T9F0D
	uchar sDcIACDenial[5];				// IAC - denial, T9F0E
	uchar sDcIACOnline[5];				// IAC - online, T9F0F
    // ECash数据
	uchar ucECashSignedStaticDataLen;   // ECash签名的应用数据长度
	uchar sECashSignedStaticData[248];  // ECash签名的应用数据, T93
    uchar ucECashIcCertificateLen;      // ECash的IC卡公钥证书长度
    uchar sECashIcCertificate[248];     // ECash的IC卡公钥证书, T9F46
	uchar ucECashCVMListLen;
	uchar sECashCVMList[252];			// Cardholder Verification Method (CVM) List, T8E
	uchar sECashIACDefault[5];			// IAC - default, T9F0D
	uchar sECashIACDenial[5];			// IAC - denial, T9F0E
	uchar sECashIACOnline[5];			// IAC - online, T9F0F
    ////////////

    uchar ucIcPublicKeyELen;            // IC卡公钥指数长度
    uchar sIcPublicKeyE[3];             // IC卡公钥指数, T9F47
    uchar ucIcPublicKeyRemainderLen;    // IC卡公钥余项长度
    uchar sIcPublicKeyRemainder[42];    // IC卡公钥余项, T9F48
    uchar ucDDOLLen;                    // DDOL长度
    uchar sDDOL[64];                    // DDOL, T9F49

	uchar sPAN[10];                     // 个人账号, T5A
	uchar ucPANSerialNo;                // 个人账号序列号(0xFF表示无此项), T5F34
	uchar sExpireDate[3];               // 有效期, T5F24
	uchar sEffectDate[3];               // 启用日期("\xFF\xFF\xFF"表示无此项), T5F25
 	uchar sIssuerCountryCode[2];        // 发卡行国家代码, T5F28
    uchar sUsageControl[2];             // 应用用途控制(AUC), T9F07
    
	uchar ucCDOL1Len;
	uchar sCDOL1[252];	    			// CDOL1, T8C
	uchar ucCDOL2Len;
	uchar sCDOL2[252];			    	// CDOL2, T8D
	uchar sServiceCode[2];              // 服务码, T5F30
	uchar sAppVerNo[2];					// 应用版本号, T9F08
	uchar ucSDATagListLen;              // SDA标签列表长度
	uchar sSDATagList[64];				// SDA标签列表, T9F4A
	uchar sDataAuthCode[2];             // Data Authentication Code, T9F45
    uchar sPinCounterRef[2];            // 密码数据, [0]:最大次数 [1]:剩余次数

    uchar ucLowerConsecutiveLimit;      // 连续交易下限(LCOL), T9F58
 	uchar ucUpperConsecutiveLimit;      // 连续交易上限(UCOL), T9F59
    uchar ucConsecutiveTransLimit;      // 连续脱机交易限制数(国际-货币), T9F53
	uchar sCTTAL[6];                    // 累计脱机交易金额限制(CTTAL), T9F54
	uchar sCTTAUL[6];                   // 累计脱机交易金额上限(CTTAUL), T9F5C
    uchar ucLogFormatLen;               // 日志格式长度
    uchar sLogFormat[128];              // 日志格式, T9F4F
    uchar ucLoadLogFormatLen;           // 圈存日志格式长度
    uchar sLoadLogFormat[128];          // 圈存日志格式, TDF4F

    // ECash专有数据
	uchar sECashIssuerAuthCode[6];      // 电子现金发卡行授权码, T9F74
	uchar sECashBalanceLimit[6];        // eCash: Balance Limit, T9F77
	uchar sECashSingleTransLimit[6];    // eCash: Single Transaction Limit, T9F78
	uchar sECashBalance[6];             // eCash: Balance, T9F79
	uchar sECashResetThreshold[6];      // eCash: Reset Threshold, T9F6D

    // QPboc专有数据
	uchar sQPbocAdditionalProcess[4];   // qPBOC: Card Additional Processes, T9F68
	uchar sQPbocTransQualifiers[2];     // qPBOC: Card Transaction Qualifiers (CTQ), T9F6C
	uchar sQPbocSpendingAmount[6];      // qPBOC: Available Offline spending Amount, T9F5D
	uchar sQPbocCvmLimitAmount[6];      // qPBOC: Card CVM Limit, T9F6B

	uchar sAppCurrencyCode[2];          // Application Currency Code, T9F51
    uchar sAppDefaultAction[2];         // Application Default Action (ADA), T9F52
	uchar ucIssuerAuthIndicator;        // Issuer Authentication Indicator, T9F56

    // 密码密钥相关数据
    uchar sDesKeyAuth[16+3];            // Kmc过程密钥加密后的Auth密钥及校验位
    uchar sDesKeyMac[16+3];             // Kmc过程密钥加密后的Mac密钥及校验位
    uchar sDesKeyEnc[16+3];             // Kmc过程密钥加密后的Enc密钥及校验位
    uchar sRsaKeyD[256];
    uchar sRsaKeyP[128];
    uchar sRsaKeyQ[128];
    uchar sRsaKeyDp[128];
    uchar sRsaKeyDq[128];
    uchar sRsaKeyQinv[128];
    uchar sPinBlock[8];
    ///

    // GPO相应数据
    uchar sDcAip[2];                    // 标准DC AIP, T82
    uchar ucDcAflLen;                   // 标准DC AFL长度
    uchar sDcAfl[64];                   // 标准DC AFL, T94
	uchar ucDcFciIssuerAppDataLen;      // 标准DC发卡行专有数据(IAD)长度
	uchar sDcFciIssuerAppData[32];      // 标准DC发卡行专有数据(IAD), T9F10 (长度为0表示不存在该项)
	                                    // eCash同用此项目
    uchar sECashAip[2];                 // eCash AIP, T82
    uchar ucECashAflLen;                // eCash AFL长度
    uchar sECashAfl[64];                // eCash AFL, T94
    uchar sQpbocAip[2];                 // qPboc AIP, T82
    uchar ucQpbocAflLen;                // qPboc AFL长度
    uchar sQpbocAfl[64];                // qPboc AFL, T94
	uchar ucQpbocFciIssuerAppDataLen;   // qPboc发卡行专有数据(IAD)长度
	uchar sQpbocFciIssuerAppData[32];   // qPboc发卡行专有数据(IAD), T9F10 (长度为0表示不存在该项)
	                                    // eCash同用此项目
    uchar sMsdAip[2];                   // MSD AIP, T82
    uchar ucMsdAflLen;                  // MSD AFL长度
    uchar sMsdAfl[64];                  // MSD AFL, T94

    // 接触交易FCI相关数据, TA5
    uchar szAppLabel[17];               // 应用标签, T50, 非接FCI也用此项
    uchar ucPriority;                   // 应用优先指示符, T87, 非接FCI也用此项
    uchar ucContactPdolLen;             // 处理选项数据对象列表长度
    uchar sContactPdol[64];             // 处理选项数据对象列表, T9F38
    uchar szLanguage[9];                // 语言优选项, T5F2D, 非接FCI也用此项
    uchar ucFciPrivateLen;              // FCI发卡行自定义数据长度, 非接FCI也用此项
    uchar sFciPrivate[64];              // FCI发卡行自定义数据, TBF0C, 非接FCI也用此项
    uchar ucContactFciLen;              // 接触交易FCI长度
    uchar sContactFci[128];             // 接触交易FCI
    // 非接触交易FCI相关数据, TA5
    uchar ucCtLessPdolLen;              // 处理选项数据对象列表长度
    uchar sCtLessPdol[64];              // 处理选项数据对象列表, T9F38
    uchar ucCtLessFciLen;               // 非接触交易FCI长度
    uchar sCtLessFci[128];              // 非接触交易FCI

	uchar ucIccPkLen;                   // ICC公钥长度
	uchar szPin[13];                    // 个人密码, 4-12
	uchar ucECashExistFlag;             // eCash存在标志
	uchar ucQpbocExistFlag;             // qPboc存在标志
} stAppRef;

#ifdef __cplusplus
extern "C" {
#endif

extern stKeyRef         gl_KeyRef;      // 密钥特性
extern stCardRef		gl_CardRef;		// 卡片特性参数
extern stInstanceRef	gl_InstanceRef;	// PBOC D/C 实例特性参数
extern stAppRef		    gl_AppRef;      // PBOC D/C 应用参数

// 初始化发卡参数
// in  : iAppVer : 应用版本号, 0x20:2.0 0x30:3.0
void vIssDataInit(int iAppVer);

// 完成发卡数据准备
// in  : psTermRand : 终端随机数
//       psCardData : IC卡初始化数据
// out : pszErrMsg  : 错误信息
// ret : 0          : OK
//       1          : refer pszErrMsg
int iCompleteIssData(uchar *psTermRand, uchar *psCardData, uchar *pszErrMsg);

#ifdef __cplusplus
}
#endif

#endif
