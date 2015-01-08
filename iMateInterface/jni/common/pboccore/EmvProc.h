/**************************************
File name     : EmvProc.h
Function      : Pboc3.0借贷记/EMV2004客户接口
Author        : Yu Jun
First edition : Mar 11th, 2014
Modified      : Mar 28th, 2014
				去除iHxEmvSetAmount(uchar *pszAmount, uchar *pszAmountOther)函数中的pszAmountOther参数
**************************************/
#ifndef _EMVPROC_H
#define _EMVPROC_H

// EMV客户接口返回码
#define HXEMV_OK					0  // OK
#define HXEMV_NA					1  // 不可用
#define HXEMV_PARA					2  // 参数错误
#define HXEMV_LACK_MEMORY			3  // 存储空间不足
#define HXEMV_CORE					4  // 内部错误
#define HXEMV_NO_SLOT				5  // 不支持的卡座
#define HXEMV_NO_CARD               6  // 卡片不存在
#define HXEMV_CANCEL				7  // 用户取消
#define HXEMV_TIMEOUT				8  // 超时
#define HXEMV_NO_APP				9  // 无支持的应用
#define HXEMV_AUTO_SELECT           10 // 获取的应用可自动选择
#define HXEMV_CARD_REMOVED			11 // 卡被取走
#define HXEMV_CARD_OP				12 // 卡操作错
#define HXEMV_CARD_SW				13 // 非法卡指令状态字
#define HXEMV_NO_DATA				14 // 无数据
#define HXEMV_NO_RECORD				15 // 无记录
#define HXEMV_NO_LOG				16 // 卡片不支持交易流水记录
#define HXEMV_TERMINATE				17 // 满足拒绝条件，交易终止
#define HXEMV_USE_MAG				18 // 请使用磁条卡
#define HXEMV_RESELECT				19 // 需要重新选择应用
#define HXEMV_NOT_SUPPORTED         20 // 不支持
#define HXEMV_DENIAL				21 // 交易拒绝
#define HXEMV_DENIAL_ADVICE			22 // 交易拒绝, 有Advice
#define HXEMV_NOT_ALLOWED			23 // 服务不允许
#define HXEMV_TRANS_NOT_ALLOWED		24 // 交易不允许
#define HXEMV_FLOW_ERROR			25 // EMV流程错误
#define HXEMV_CALLBACK_METHOD       26 // 回调与非回调核心接口调用错误
#define HXEMV_NOT_ACCEPTED          27 // 不接受

// 持卡人认证方法
#define HXCVM_PLAIN_PIN             0x01 // 脱机明文密码认证
#define HXCVM_CIPHERED_OFFLINE_PIN  0x02 // 脱机密文密码认证(现不支持)
#define HXCVM_CIPHERED_ONLINE_PIN   0x03 // 联机密文密码认证
#define HXCVM_HOLDER_ID             0x04 // 持卡人证件认证
#define HXCVM_CONFIRM_AMOUNT        0x10 // 非持卡人验证,仅用于要求确认金额
// 持卡人认证方法处理方式
#define HXCVM_PROC_OK               0x00 // 正常处理完毕
#define HXCVM_BYPASS                0x01 // 要求输密码或验证证件时选择了Bypass
#define HXCVM_FAIL                  0x02 // 证件验证没有通过
#define HXCVM_CANCEL                0x03 // 被取消
#define HXCVM_TIMEOUT               0x04 // 超时

// GAC卡片应答
#define GAC_ACTION_TC				0x00 // 批准(生成TC)
#define GAC_ACTION_AAC				0x01 // 拒绝(生成AAC)
#define GAC_ACTION_AAC_ADVICE		0x02 // 拒绝(生成AAC,有Advice)
#define GAC_ACTION_ARQC				0x03 // 要求联机(生成ARQC)

// 终端支持的应用列表结构声明
typedef struct {
    unsigned char ucAidLen;						// AID长度
    unsigned char sAid[16];						// AID
	unsigned char ucASI;						// 应用选择指示器, 0:部分名字匹配，1:全部名字匹配
    signed char   cOnlinePinSupport;            // 1:该Aid支持联机密码 0:该Aid不支持联机密码, -1表示无

	// 以上参数为AID专用, 以下参数为与终端通用参数公共部分
	unsigned char sTermAppVer[2];               // T9F09, 终端应用版本号, "\xFF\xFF"表示不存在
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
} stHxTermAid;

// 终端参数结构声明
typedef struct {
    unsigned char ucTermType;                   // T9F35, 终端类型, 例:0x21
    unsigned char sTermCapability[3];           // T9F33, 终端能力
    unsigned char sAdditionalTermCapability[5]; // T9F40, 终端能力扩展
    unsigned char szMerchantId[15+1];           // T9F16, 商户号
    unsigned char szTermId[8+1];                // T9F1C, 终端号
    unsigned char szMerchantNameLocation[254+1];// T9F4E, 商户名字地址, 0-254
    unsigned int  uiTermCountryCode;            // T9F1A, 终端国家代码, 156=中国
    unsigned char szAcquirerId[11+1];           // T9F01, 收单行标识符, 6-11
    int           iMerchantCategoryCode;		// T9F15, -1:无此数据 0-9999:有效数据
	unsigned char ucPinBypassBehavior;          // PIN bypass特性 0:每次bypass只表示该次bypass 1:一次bypass,后续都认为bypass
	unsigned char ucAppConfirmSupport;          // 1:支持应用确认 0:不支持应用确认(TAG_DFXX_AppConfirmSupport)

	stHxTermAid   AidCommonPara;				// 终端通用参数与AID相关参数公共部分
} stHxTermParam;

// 终端支持的CA公钥结构声明
typedef struct {
    unsigned char ucKeyLen;                     // 公钥模数长度，字节为单位
    unsigned char sKey[248];                    // 公钥模数
    long		  lE;                           // 公钥指数，3或65537
    unsigned char sRid[5];                      // 该公钥所属的RID
    unsigned char ucIndex;                      // 公钥索引
} stHxCaPublicKey;

// 终端与卡片都支持的应用列表结构声明
typedef struct {
    unsigned char ucAdfNameLen;					// ADF名字长度
    unsigned char sAdfName[16];                 // ADF名字
    unsigned char szLabel[16+1];                // 应用标签
    int		      iPriority;                    // 应用优先级, -1表示该应用没有提供
    unsigned char szLanguage[8+1];              // 语言指示, 应用如没提供则置为空串
    int			  iIssuerCodeTableIndex;        // 字符代码表, -1表示该应用没有提供
    unsigned char szPreferredName[16+1];        // 应用首选名称, 应用如没提供则置为空串
} stHxAdfInfo;

#ifdef __cplusplus
extern "C" {
#endif

// 获取核心信息
// out : pszCoreName    : 核心名字, 不超过40字节
//       pszCoreDesc    : 核心描述, 不超过60字节字符串
//       pszCoreVer     : 核心版本号, 不超过10字节字符串, 如"1.00"
//       pszReleaseDate : 核心发布日期, YYYY/MM/DD
//       pszCustomerDesc: 客户接口说明, 不超过100字节字符串
// ret : HXEMV_OK       : OK
int iHxEmvInfo(unsigned char *pszCoreName, unsigned char *pszCoreDesc, unsigned char *pszCoreVer, unsigned char *pszReleaseDate, unsigned char *pszCustomerDesc);

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
// ret : HXEMV_OK       : OK
//       HXEMV_PARA     : 参数错误
//       HXEMV_CORE     : 内部错误
// Note: 在调用任何其它接口前必须且先初始化核心
int iHxEmvInit(int (*pfiTestCard)(void),
			   int (*pfiResetCard)(unsigned char *psAtr),
			   int (*pfiDoApdu)(int iApduInLen, unsigned char *psApduIn, int *piApduOutLen, unsigned char *psApduOut),
			   int (*pfiCloseCard)(void));

// 关闭卡片
// ret : HXEMV_OK       : OK
int iHxEmvCloseCard(void);

// 设置终端参数
// in  : pHxTermParam      : 终端参数
// ret : HXEMV_OK          : OK
//       HXEMV_PARA        : 参数错误
//       HXEMV_CORE        : 内部错误
//       HXEMV_FLOW_ERROR  : EMV流程错误
int iHxEmvSetParam(stHxTermParam *pHxTermParam);

// 装载终端支持的Aid
// in  : paHxTermAid       : 终端支持的Aid数组
//       iHxTermAidNum     : 终端支持的Aid个数
// ret : HXEMV_OK          : OK
//       HXEMV_LACK_MEMORY : 存储空间不足
//       HXEMV_PARA        : 参数错误
//       HXEMV_FLOW_ERROR  : EMV流程错误
int iHxEmvLoadAid(stHxTermAid *paHxTermAid, int iHxTermAidNum);

// 装载CA公钥
// in  : paHxCaPublicKey   : CA公钥
//       iHxCaPublicKeyNum : CA公钥个数
// ret : HXEMV_OK          : OK
//       HXEMV_LACK_MEMORY : 存储空间不足
//       HXEMV_PARA        : 参数错误
//       HXEMV_FLOW_ERROR  : EMV流程错误
int iHxEmvLoadCaPublicKey(stHxCaPublicKey *paHxCaPublicKey, int iHxCaPublicKeyNum);

// 交易初始化
// in  : iFlag             : =0, 保留以后使用
// ret : HXEMV_OK          : OK
//       HXEMV_FLOW_ERROR  : EMV流程错误
//       HXEMV_CARD_REMOVED: 卡被取走
//       HXEMV_NO_CARD     : 卡片不存在
//       HXEMV_CARD_OP     : 卡片复位错误
int iHxEmvTransInit(int iFlag);

// 读取支持的应用
// in  : iIgnoreBlock	   : !0:忽略应用锁定标志, 0:锁定的应用不会被选择
//       piAdfNum          : 可容纳的终端与卡片同时支持的Adf个数
// out : paHxAdfInfo       : 终端与卡片同时支持的Adf列表(以优先级排序)
//       piHxAdfNum        : 终端与卡片同时支持的Adf个数
// ret : HXEMV_OK          : OK, 获取的应用必须确认
//       HXEMV_AUTO_SELECT : OK, 获取的应用可自动选择
//       HXEMV_NO_APP      : 无支持的应用
//       HXEMV_CARD_REMOVED: 卡被取走
//       HXEMV_CARD_SW     : 卡状态字非法
//       HXEMV_CARD_OP     : 卡操作错
//       HXEMV_TERMINATE   : 满足拒绝条件，交易终止
//       HXEMV_FLOW_ERROR  : EMV流程错误
//       HXEMV_LACK_MEMORY : 存储空间不足
//       HXEMV_PARA        : 参数错误
int iHxEmvGetSupportedApp(int iIgnoreBlock, stHxAdfInfo *paHxAdfInfo, int *piHxAdfNum);

// 应用选择
// in  : iIgnoreBlock	     : !0:忽略应用锁定标志, 0:锁定的应用不会被选择
//       iAidLen             : AID长度
//       psAid               : AID
// ret : HXEMV_OK            : OK
//       HXEMV_CARD_REMOVED  : 卡被取走
//       HXEMV_CARD_OP       : 卡操作错
//       HXEMV_TERMINATE     : 满足拒绝条件，交易终止
//       HXEMV_FLOW_ERROR    : EMV流程错误
//       HXEMV_CORE          : 内部错误
//       HXEMV_RESELECT      : 需要重新选择应用
//       HXEMV_NOT_SUPPORTED : 选择了不支持的应用
int iHxEmvAppSelect(int iIgnoreBlock, int iAidLen, unsigned char *psAid);

// 读取卡片内部数据
// in  : psTag             : 数据标签, 例:"\x9F\x79":电子现金余额
//       piOutTlvDataLen   : psOutTlvData缓冲区大小
//       piOutDataLen      : psOutData缓冲区大小
// out : piOutTlvDataLen   : 读出的数据长度, 原始格式, 包括Tag、Length、Value
//       psOutTlvData      : 读出的数据, 包括Tag、Length、Value
//       piOutDataLen      : 读出的数据长度, 解码后格式
//       psOutData         : 读出的数据(N数字型:转换成可读数字串, CN压缩数字型:去除尾部'F'后的数字串, A|AN|ANS|B|未知类型:原样返回),除B型外返回值后面会强制加一个结尾'\0', 该结束符不包括在返回的长度之内
//                           注意, 类似N3这样的数据, 返回的内容长度为N4, 注意接收缓冲区长度要给足
// ret : HXEMV_OK          : 读取成功
//       HXEMV_NO_DATA     : 无此数据
//       HXEMV_LACK_MEMORY : 缓冲区不足
//       HXEMV_CARD_REMOVED: 卡被取走
//       HXEMV_CARD_OP     : 卡操作错
//       HXEMV_CARD_SW     : 非法卡状态字
//       HXEMV_FLOW_ERROR  : EMV流程错误
//       HXEMV_CORE        : 内部错误
// note: piOutTlvDataLen或psOutTlvData任一个传入NULL，则不会返回这两个结果
//       piOutDataLen或psOutData任一个传入NULL, 将不会返回这两个结果
//       数据一旦被读出, 会放到Tlv数据库中, 再次使用可以用iHxEmvGetData()读取
int iHxEmvGetCardNativeData(unsigned char *psTag, int *piOutTlvDataLen, unsigned char *psOutTlvData, int *piOutDataLen, unsigned char *psOutData);

// 读取应用数据
// in  : psTag             : 数据标签, 例:"\x82":Aip
//       piOutTlvDataLen   : psOutTlvData缓冲区大小
//       piOutDataLen      : psOutData缓冲区大小
// out : piOutTlvDataLen   : 读出的数据长度, 原始格式, 包括Tag、Length、Value
//       psOutTlvData      : 读出的数据, 包括Tag、Length、Value
//       piOutDataLen      : 读出的数据长度, 解码后格式
//       psOutData         : 读出的数据(N数字型:转换成可读数字串, CN压缩数字型:去除尾部'F'后的数字串, A|AN|ANS|B|未知类型:原样返回),除B型外返回值后面会强制加一个结尾'\0', 该结束符不包括在返回的长度之内
//                           注意, 类似N3这样的数据, 返回的内容长度为N4, 注意接收缓冲区长度要给足
// ret : HXEMV_OK          : 读取成功
//       HXEMV_NO_DATA     : 无此数据
//       HXEMV_LACK_MEMORY : 缓冲区不足
//       HXEMV_FLOW_ERROR  : EMV流程错误
// note: piOutTlvDataLen或psOutTlvData任一个传入NULL，则不会返回这两个结果
//       piOutDataLen或psOutData任一个传入NULL, 将不会返回这两个结果
int iHxEmvGetData(unsigned char *psTag, int *piOutTlvDataLen, unsigned char *psOutTlvData, int *piOutDataLen, unsigned char *psOutData);

// 读卡片交易流水支持信息
// in  : iFlag             : 0:标准交易流水 1:圈存流水
// out : piMaxRecNum       : 最多交易流水记录个数
// ret : HXEMV_OK          : 读取成功
//       HXEMV_NO_LOG      : 卡片不支持交易流水记录
//       HXEMV_PARA        : 参数错误
//       HXEMV_FLOW_ERROR  : EMV流程错误
int iHxEmvGetLogInfo(int iFlag, int *piMaxRecNum);

// 读卡片交易流水
// in  : iFlag             : 0:标准交易流水 1:圈存流水
//       iLogNo            : 交易流水记录号, 最近的一条记录号为1
//       piLogLen          : psLog缓冲区大小
// out : piLogLen          : 交易流水记录长度
//       psLog             : 记录内容(IC卡原格式输出)
// ret : HXEMV_OK          : 读取成功
//       HXEMV_NO_RECORD   : 无此记录
//       HXEMV_LACK_MEMORY : 缓冲区不足
//       HXEMV_CARD_REMOVED: 卡被取走
//       HXEMV_CARD_OP     : 卡操作错
//       HXEMV_CARD_SW     : 非法卡指令状态字
//       HXEMV_NO_LOG      : 卡片不支持交易流水记录
//       HXEMV_PARA        : 参数错误
//       HXEMV_FLOW_ERROR  : EMV流程错误
int iHxEmvReadLog(int iFlag, int iLogNo, int *piLogLen, unsigned char *psLog);

// GPO
// in  : pszTransTime      : 交易时间，YYYYMMDDhhmmss
//       ulTTC             : 终端交易序号, 1-999999
//       ucTransType       : 交易类型,处理码前2位表示的金融交易类型
//                           比如现金充值63表示为0x63
//       pszAmount         : 交易金额, ascii码串, 不要小数点, 默认为缺省小数点位置, 例如:RMB1.00表示为"100"
//       uiCurrencyCode    : 货币代码, 156=人民币
// ret : HXEMV_OK          : OK
//       HXEMV_CARD_REMOVED: 卡被取走
//       HXEMV_CARD_OP     : 卡操作错
//       HXEMV_CARD_SW     : 非法卡状态字
//       HXEMV_TERMINATE   : 满足拒绝条件，交易终止
//       HXEMV_RESELECT    : 需要重新选择应用
//       HXEMV_TRANS_NOT_ALLOWED : 交易不支持
//       HXEMV_PARA        : 参数错误
//       HXEMV_FLOW_ERROR  : EMV流程错误
//       HXEMV_CORE        : 内部错误
// Note: 如果返回HXEMV_RESELECT，从iHxEmvGetSupportedApp()开始重新执行流程
int iHxEmvGPO(unsigned char *pszTransTime, unsigned long ulTTC, unsigned char ucTransType, unsigned char *pszAmount, unsigned int uiCurrencyCode);

// 传入或重新传入金额
// in  : pszAmount         : 金额
// ret : HXEMV_OK          : 设置成功
//       HXEMV_FLOW_ERROR  : EMV流程错误
//       HXEMV_PARA        : 参数错误
// note: GPO后GAC金额可能会不同, 重新设定交易金额
//       GPO后, GAC1前才可以调用
int iHxEmvSetAmount(unsigned char *pszAmount);

// 读应用记录
// ret : HXEMV_OK          : OK
//       HXEMV_CARD_REMOVED: 卡被取走
//       HXEMV_CARD_OP     : 卡操作错
//       HXEMV_CARD_SW     : 非法卡指令状态字
//       HXEMV_TERMINATE   : 满足拒绝条件，交易终止
//       HXEMV_FLOW_ERROR  : EMV流程错误
//       HXEMV_CORE        : 内部错误
int iHxEmvReadRecord(void);

// 设置卡片黑名单及分开消费标志
// in  : iBlackFlag        : 设置该卡是否为黑名单卡, 0:不为黑名单卡 1:黑名单卡
//       iSeparateFlag     : 设置该卡累计消费超限额, 0:没超限额 1:超限额
// ret : HXEMV_OK          : OK
//       HXEMV_CORE        : 内部错误
// Note: 如果读记录后发现卡片是黑名单卡或累计消费超限额, 则需要调用此函数设置账户信息
//       如果不支持或检查不许设置标记, 则不需要调用本接口
int iHxEmvSetPanFlag(int iBlackFlag, int iSeparateFlag);

// SDA或DDA
// out : piNeedCheckCrlFlag  : 是否需要判断发卡行公钥证书为黑名单标志, 0:不需要判断 1:需要判断
//       psRid               : RID[5], psRid+pucCaIndex+psCertSerNo这三项提供给应用层判断发卡行公钥证书是否在黑名单列表中. 任何一个指针传入为空都表示高层不需要
//       pucCaIndex          : CA公钥索引
//       psCertSerNo         : 发卡行公钥证书序列号[3]
// ret : HXEMV_OK            : OK
//       HXEMV_CARD_REMOVED  : 卡被取走
//       HXEMV_CARD_OP       : 卡操作错
//       HXEMV_CARD_SW       : 非法卡状态字
//       HXEMV_TERMINATE     : 满足拒绝条件，交易终止
//       HXEMV_DENIAL        : 终端行为分析结果为脱机拒绝
//       HXEMV_DENIAL_ADVICE : 终端行为分析结果为脱机拒绝, 有Advice
//       HXEMV_FLOW_ERROR    : EMV流程错误
//       HXEMV_CORE          : 内部错误
// Note: 应用层支持发卡行公钥证书黑名单检查时, piNeedCheckCrlFlag才有意义, 否则可以忽略
int iHxEmvOfflineDataAuth(int *piNeedCheckCrlFlag, unsigned char *psRid, unsigned char *pucCaIndex, unsigned char *psCertSerNo);

// 设置发卡行公钥证书为黑名单
// in  : iIssuerCertCrlFlag  : 是否在黑名单列表标志, 0:不在黑名单列表 1:在黑名单列表
// ret : HXEMV_OK            : OK
//       HXEMV_CARD_REMOVED  : 卡被取走
//       HXEMV_CARD_OP       : 卡操作错
//       HXEMV_CARD_SW       : 非法卡状态字
//       HXEMV_TERMINATE     : 满足拒绝条件，交易终止
//       HXEMV_DENIAL        : 终端行为分析结果为脱机拒绝
//       HXEMV_DENIAL_ADVICE : 终端行为分析结果为脱机拒绝, 有Advice
//       HXEMV_FLOW_ERROR    : EMV流程错误
//       HXEMV_CORE          : 内部错误
// Note: 应用层调用iHxEmvOfflineDataAuth()后得到Rid+CaIndex+CertSerNo
//       根据此数据判断发卡行公钥证书是否为黑名单,只有是黑名单,才需要调用本函数通知核心
int iHxEmvSetIssuerCertCrl(int iIssuerCertCrlFlag);

// 处理限制
// ret : HXEMV_OK            : OK
//       HXEMV_TERMINATE     : 满足拒绝条件，交易终止
//       HXEMV_FLOW_ERROR    : EMV流程错误
//       HXEMV_CORE          : 内部错误
//       HXEMV_CARD_REMOVED  : 卡被取走
//       HXEMV_DENIAL        : 终端行为分析结果为脱机拒绝
//       HXEMV_DENIAL_ADVICE : 终端行为分析结果为脱机拒绝, 有Advice
//       HXEMV_CARD_OP       : 终端行为分析结果为脱机拒绝, 申请AAC时卡片出错
int iHxEmvProcRistrictions(void);

// 终端风险管理
// ret : HXEMV_OK            : OK
//       HXEMV_TERMINATE     : 满足拒绝条件，交易终止
//       HXEMV_CORE          : 其它错误
//       HXEMV_CARD_REMOVED  : 卡被取走
//       HXEMV_CARD_OP       : 卡操作错
//       HXEMV_DENIAL        : 终端行为分析结果为脱机拒绝
//       HXEMV_DENIAL_ADVICE : 终端行为分析结果为脱机拒绝, 有Advice
//       HXEMV_FLOW_ERROR    : EMV流程错误
int iHxEmvTermRiskManage(void);

// 获取持卡人验证
// out : piCvm               : 持卡人认证方法, 支持HXCVM_PLAIN_PIN、HXCVM_CIPHERED_ONLINE_PIN、HXCVM_HOLDER_ID、HXCVM_CONFIRM_AMOUNT
//       piBypassFlag        : 允许bypass标志, 0:不允许, 1:允许
// ret : HXEMV_OK            : OK
//       HXEMV_NO_DATA       : 无需继续进行持卡人验证
//       HXEMV_CARD_REMOVED  : 卡被取走
//       HXEMV_CARD_OP       : 卡操作错
//       HXEMV_TERMINATE     : 满足拒绝条件，交易终止
//       HXEMV_DENIAL        : 终端行为分析结果为脱机拒绝
//       HXEMV_DENIAL_ADVICE : 终端行为分析结果为脱机拒绝, 有Advice
//       HXEMV_FLOW_ERROR    : EMV流程错误
//       HXEMV_CORE          : 内部错误
// Note: 与iHxEmvDoCvmMethod()搭配, 可能需要多次调用, 直到返回HXEMV_NO_DATA
int iHxEmvGetCvmMethod(int *piCvm, int *piBypassFlag);

// 执行持卡人验证
// in  : iCvmProc            : 认证方法处理方式, HXCVM_PROC_OK or HXCVM_BYPASS or HXCVM_FAIL or HXCVM_CANCEL or HXCVM_TIMEOUT
//       psCvmData           : 输入的密码, 如果为明文密码, 密码尾部要补0
// out : piPrompt            : 额外提示信息, 0:无额外提示信息 1:密码错,可重试 2:密码错,密码已锁 3:脱机密码验证成功
// ret : HXEMV_OK            : OK, 需要继续进行持卡人验证, 继续调用iHxGetCvmMethod2(), 然后再调用本函数
//       HXEMV_PARA		     : 参数错误
//       HXEMV_CANCEL        : 用户取消
//       HXEMV_TIMEOUT       : 超时
//       HXEMV_CARD_REMOVED  : 卡被取走
//       HXEMV_CARD_OP       : 卡操作错
//       HXEMV_CARD_SW       : 非法卡状态字
//       HXEMV_TERMINATE     : 满足拒绝条件，交易终止
//       HXEMV_DENIAL        : 终端行为分析结果为脱机拒绝
//       HXEMV_DENIAL_ADVICE : 终端行为分析结果为脱机拒绝, 有Advice
//       HXEMV_FLOW_ERROR    : EMV流程错误
//       HXEMV_CORE          : 内部错误
// Note: 执行的CVM必须是最近一次iHxEmvGetCvmMethod()获得的CVM
int iHxEmvDoCvmMethod(int iCvmProc, unsigned char *psCvmData, int *piPrompt);

// 获取CVM认证方法签字标志
// out : piNeedSignFlag    : 表示需要签字标志，0:不需要签字 1:需要签字
// ret : HXEMV_OK          : OK
int iHxEmvGetCvmSignFlag(int *piNeedSignFlag);

// 终端行为分析
// ret : HXEMV_OK            : OK
//       HXEMV_CORE          : 其它错误
//       HXEMV_FLOW_ERROR    : EMV流程错误
//       HXEMV_CARD_REMOVED  : 卡被取走
//       HXEMV_DENIAL        : 终端行为分析结果为脱机拒绝
//       HXEMV_DENIAL_ADVICE : 终端行为分析结果为脱机拒绝, 有Advice
int iHxEmvTermActionAnalysis(void);

// 第一次GAC
// in  : ucForcedOnline    : 1:设定强制联机标志 0:不设定强制联机标志
// out : piCardAction      : 卡片执行结果
//								GAC_ACTION_TC     : 批准(生成TC)
//								GAC_ACTION_AAC    : 拒绝(生成AAC)
//								GAC_ACTION_AAC_ADVICE : 拒绝(生成AAC,有Advice)
//								GAC_ACTION_ARQC   : 要求联机(生成ARQC)
// ret : HXEMV_OK          : OK
//       HXEMV_CORE        : 其它错误
//       HXEMV_CARD_REMOVED: 卡被取走
//       HXEMV_CARD_OP     : 卡操作错
//       HXEMV_CARD_SW     : 非法状态字
//       HXEMV_TERMINATE   : 满足拒绝条件，交易终止
//       HXEMV_NOT_ALLOWED : 服务不允许
//       HXEMV_NOT_ACCEPTED: 不接受
//       HXEMV_FLOW_ERROR  : EMV流程错误
int iHxEmvGac1(unsigned char ucForcedOnline , int *piCardAction);

// 第二次GAC
// in  : pszArc            : 应答码[2], NULL或""表示联机失败
//       pszAuthCode	       : 授权码[6], NULL或""表示无授权码数据
//       psOnlineData      : 联机响应数据(55域内容),一组TLV格式数据, NULL表示无联机响应数据
//       psOnlineDataLen   : 联机响应数据长度
// out : piCardAction      : 卡片执行结果
//                           GAC_ACTION_TC     : 批准(生成TC)
//							     GAC_ACTION_AAC    : 拒绝(生成AAC)
//                           GAC_ACTION_AAC_ADVICE : 拒绝(生成AAC,有Advice)
// ret : HXEMV_OK          : OK
//       HXEMV_CORE        : 其它错误
//       HXEMV_CARD_REMOVED: 卡被取走
//       HXEMV_CARD_OP     : 卡操作错
//       HXEMV_CARD_SW     : 非法卡状态字
//       HXEMV_TERMINATE   : 满足拒绝条件，交易终止
//       HXEMV_NOT_ALLOWED : 服务不允许
//       HXEMV_NOT_ACCEPTED: 不接受
//       HXEMV_FLOW_ERROR  : EMV流程错误
int iHxEmvGac2 (unsigned char *pszArc, unsigned char *pszAuthCode, unsigned char *psIssuerData, int iIssuerDataLen, int *piCardAction);

#ifdef __cplusplus
}
#endif

#endif
