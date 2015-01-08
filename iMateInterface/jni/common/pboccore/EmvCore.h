/**************************************
File name     : EmvCore.h
Function      : EMV/pboc2.0借贷记核心接口
Author        : Yu Jun
First edition : Mar 31st, 2012
Modified      : Sep 5th, 2013
				增加iHxSetAmount()函数, 可以实现GAC与GPO金额不同
                Apr 2nd, 2014
					iHxSetAmount()函数限制在读记录后, 处理限制前调用
				Apr 16th, 2014
				    iHxGetCardNativeData()与iHxGetData()函数在遇到未知Tag时按b型处理
				Apr 21st, 2014
				    更改iHxCoreInit()接口, 增加一个参数iAppType
					用于表明是送检程序还是实际应用程序
**************************************/
#ifndef _EMVCORE_H
#define _EMVCORE_H

// EMV核心调用类型
#define HXCORE_CALLBACK				1  // 指明使用回调接口
#define HXCORE_NOT_CALLBACK         2  // 指明使用非回调接口

// EMV核心返回码
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
#define HXEMV_TRY_OTHER_INTERFACE	28 // 尝试其它通信界面
#define HXEMV_CARD_TRY_AGAIN		29 // 要求重新提交卡片(非接)
#define HXEMV_EXPIRED_APP			30 // 过期卡(非接)
#define HXEMV_FDDA_FAIL				31 // fDDA失败(非接)

// 交易类型定义
// 低位为交易码, 即Tag9C内容
#define TRANS_TYPE_SALE				0x0000 // 消费
#define TRANS_TYPE_GOODS			0x0100 // 商品
#define TRANS_TYPE_SERVICES			0x0200 // 服务
#define TRANS_TYPE_CASH				0x0001 // 取现
#define TRANS_TYPE_RETURN			0x0020 // 退货
#define TRANS_TYPE_DEPOSIT			0x0021 // 存款
#define TRANS_TYPE_AVAILABLE_INQ	0x0030 // 可用余额查询
#define TRANS_TYPE_BALANCE_INQ		0x0031 // 余额查询
#define TRANS_TYPE_TRANSFER			0x0040 // 转账
#define TRANS_TYPE_PAYMENT			0x0050 // 支付
#define TRANS_TYPE_FIXEDACC_LOAD    0x0060 // 指定账户现金充值
#define TRANS_TYPE_NOFIXEDACC_LOAD  0x0062 // 非指定账户现金充值
#define TRANS_TYPE_CASH_LOAD        0x0063 // 现金充值

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

// 信息接受者类型
#define HXMSG_LOCAL_LANG  0 // 本地语言
#define HXMSG_OPERATOR    1 // 操作员
#define HXMSG_CARDHOLDER  2 // 持卡人
#define HXMSG_SMART       3 // 智能选择, 有人值守->操作员 无人值守->持卡人

// 参数管理
#define HXEMV_PARA_INIT				0x00 // 初始化
#define HXEMV_PARA_ADD				0x01 // 增加一个项目
#define HXEMV_PARA_DEL				0x02 // 删除一个项目

// 终端支持的应用列表结构声明
typedef struct {
    uchar ucAidLen;                     // AID长度
    uchar sAid[16];                     // AID
    uchar ucASI;                        // 应用选择指示器, 0:部分名字匹配，1:全部名字匹配
	// 以上参数为AID专用, 以下参数为与终端通用参数公共部分
	uchar ucTermAppVerExistFlag;        // TermAppVer存在标志, 0:无 1:存在
	uchar sTermAppVer[2];               // T9F09, 终端应用版本号
    int   iDefaultDDOLLen;              // TDFxx, Default DDOL长度,-1表示无
    uchar sDefaultDDOL[252];            // TDFxx, Default DDOL(TAG_DFXX_DefaultDDOL)
    int   iDefaultTDOLLen;              // TDFxx, Default TDOL长度,-1表示无
    uchar sDefaultTDOL[252];            // TDFxx, Default TDOL(TAG_DFXX_DefaultTDOL)
    int   iMaxTargetPercentage;         // TDFxx, Maximum Target Percentage to be used for Biased Random Selection，-1:无此数据(TAG_DFXX_MaxTargetPercentage)
    int   iTargetPercentage;            // TDFxx, Target Percentage to be used for Random Selection，-1:无此数据(TAG_DFXX_TargetPercentage)
	uchar ucFloorLimitExistFlag;        // FloorLimit存在标志, 0:无 1:存在
	ulong ulFloorLimit;                 // T9F1B, Terminal Floor Limit
	uchar ucThresholdExistFlag;         // Threshold存在标志, 0:无 1:存在
	ulong ulThresholdValue;             // TDFxx, Threshold Value for Biased Random Selection
	uchar ucTacDefaultExistFlag;        // TacDefault存在标志, 0:无 1:存在
    uchar sTacDefault[5];               // TDFxx, TAC-Default(TAG_DFXX_TACDefault)
	uchar ucTacDenialExistFlag;         // TacDenial存在标志, 0:无 1:存在
    uchar sTacDenial[5];                // TDFxx, TAC-Denial(TAG_DFXX_TACDenial)
	uchar ucTacOnlineExistFlag;         // TacOnline存在标志, 0:无 1:存在
    uchar sTacOnline[5];                // TDFxx, TAC-Online(TAG_DFXX_TACOnline)
    signed char cForcedOnlineSupport;   // TDFxx, 1:支持 0:不支持(TAG_DFXX_ForceOnlineSupport), -1表示无
    signed char cForcedAcceptanceSupport;     // TDFxx, 1:支持 0:不支持(TAG_DFXX_ForceAcceptSupport), -1表示无
    signed char cOnlinePinSupport;      // TDFxx, 1:支持 0:不支持(TAG_DFXX_OnlinePINSupport), -1表示无
    uchar ucTermRiskDataLen;            // T9F1D, Length of Terminal Risk Management Data，0:无此数据
    uchar sTermRiskData[8];             // T9F1D, Terminal Risk Management Data
    signed char cECashSupport;          // TDFxx, 1:支持电子现金 0:不支持电子现金(TAG_DFXX_ECTermSupportIndicator), -1表示无
										//        注:GPO需要T9F7A来指示, 交易开始后在判断符合电子现金支付条件后后, 设定T9F7A的值
    uchar szTermECashTransLimit[12+1];  // T9F7B, 终端电子现金交易限额, 空表示无
} stTermAid;

// 终端参数结构声明
typedef struct {
    uchar ucTermType;                   // T9F35, 终端类型
    uchar sTermCapability[3];           // T9F33, 终端能力
    uchar sAdditionalTermCapability[5]; // T9F40, 终端能力扩展
    uchar ucReaderCapability;           // TDFxx, 1:只支持磁卡 2:只支持IC卡 3:支持组合式Mag/IC读卡器 4:支持分离式Mag/IC读卡器(TAG_DFXX_ReaderCapability)
    uchar ucVoiceReferralSupport;       // TDFxx, 1:支持 2:不支持,缺省approve 3:不支持,缺省decline(TAG_DFXX_VoiceReferralSupport)
	uchar ucPinBypassBehavior;          // TDFxx, PIN bypass特性 0:每次bypass只表示该次bypass 1:一次bypass,后续都认为bypass
    uchar szMerchantId[15+1];           // T9F16, Merchant Identifier
    uchar szTermId[8+1];                // T9F1C, Terminal Identification
    uchar szIFDSerialNo[8+1];           // T9F1E, IFD Serial Number
    uchar szMerchantNameLocation[254+1];// T9F4E, Merchant Name and Location
    int   iMerchantCategoryCode;		// T9F15, -1:无此数据 0-9999:有效数据
    int   iTermCountryCode;             // T9F1A, 终端国家代码
	int   iTermCurrencyCode;            // T5F2A, 终端交易货币代码
    uchar szAcquirerId[11+1];           // T9F01, Acquirer Identifier

    uchar ucTransLogSupport;            // TDFxx, 1:终端支持交易流水记录 0:不支持(TAG_DFXX_TransLogSupport)
    uchar ucBlackListSupport;           // TDFxx, 1:终端支持黑名单检查 0:不支持(TAG_DFXX_BlacklistSupport)
    uchar ucSeparateSaleSupport;        // TDFxx, 1:支持分开消费检查 0:不支持(TAG_DFXX_SeparateSaleSupport)
	uchar ucAppConfirmSupport;          // TDFxx, 1:支持应用确认 0:不支持应用确认(TAG_DFXX_AppConfirmSupport)
	uchar szLocalLanguage[3];           // TDFxx, ISO-639-1, 本地语言(TAG_DFXX_LocalLanguage)
	uchar szPinpadLanguage[32+1];       // TDFxx, ISO-639-1, pinpad支持的语言(TAG_DFXX_PinpadLanguage), 由语言决定字符集

	// 以下4个参数为非接专有参数
	uchar sTermCtlsCapability[4];		// TDFxx, 终端非接能力, T9F66 Mark位, 最终的T9F66值要与该参数做与操作
	uchar szTermCtlsAmountLimit[12+1];	// TDFxx, 终端非接交易限额, 空表示无
	uchar szTermCtlsOfflineAmountLimit[12+1];	// TDFxx, 终端非接脱机交易限额, 空表示无
	uchar szTermCtlsCvmLimit[12+1];		// TDFxx, 终端非接CVM限额, 空表示无

	stTermAid AidCommonPara;			// 终端通用参数与AID相关参数公共部分
} stTermParam;

// 终端支持的CA公钥结构声明
typedef struct {
    uchar ucKeyLen;                     // 公钥模数长度，字节为单位
    uchar sKey[248];                    // 公钥模数
    long  lE;                           // 公钥指数，3或65537
    uchar sRid[5];                      // 该公钥所属的RID
    uchar ucIndex;                      // 公钥索引
    uchar szExpireDate[8+1];            // 有效期，YYYYMMDD
	uchar sHashCode[20];                // 密钥hash值
} stCAPublicKey;

// 终端与卡片都支持的应用列表结构声明
typedef struct {
    uchar ucAdfNameLen;					// ADF名字长度
    uchar sAdfName[16];                 // ADF名字
    uchar szLabel[16+1];                // 应用标签
    int   iPriority;                    // 应用优先级, -1表示该应用没有提供
    uchar szLanguage[8+1];              // 语言指示, 应用如没提供则置为空串
    int   iIssuerCodeTableIndex;        // 字符代码表, -1表示该应用没有提供
    uchar szPreferredName[16+1];        // 应用首选名称, 应用如没提供则置为空串
} stADFInfo;

// 获取核心信息
// out : pszCoreName    : 核心名字, 不超过40字节
//       pszCoreDesc    : 核心描述, 不超过60字节字符串
//       pszCoreVer     : 核心版本号, 不超过10字节字符串, 如"1.00"
//       pszReleaseDate : 核心发布日期, YYYY/MM/DD
// ret : HXEMV_OK       : OK
int iHxCoreInfo(uchar *pszCoreName, uchar *pszCoreDesc, uchar *pszCoreVer, uchar *pszReleaseDate);

// 初始化核心
// in  : iCallBackFlag      : 指明核心调用方式
//                            HXCORE_CALLBACK     : 指明使用回调接口
//                            HXCORE_NOT_CALLBACK : 指明使用非回调接口
//		 iAppType           : 应用程序标志, 0:送检程序(严格遵守规范) 1:实际应用程序
//							      区别: 1. 送检程序只支持通过L2检测的交易, 应用程序支持所有交易
//       ulRandSeed         : 随机数种子，启动核心内部随机数发生器
//       pszDefaultLanguage : 缺省语言, 当前支持zh:中文 en:英文
// ret : HXEMV_OK           : OK
//       HXEMV_PARA         : 参数错误
//       HXEMV_CORE         : 内部错误
// Note: 在调用任何其它接口前必须且先初始化核心
int iHxCoreInit(int iCallBackFlag, int iAppType, ulong ulRandSeed, uchar *pszDefaultLanguage);

// 设置终端参数
// in  : pTermParam        : 终端参数
// out : pszErrTag         : 如果设置出错, 出错的Tag值
// ret : HXEMV_OK          : OK
//       HXEMV_PARA        : 参数错误
//       HXEMV_CORE        : 内部错误
//       HXEMV_FLOW_ERROR  : EMV流程错误
int iHxSetTermParam(stTermParam *pTermParam, uchar *pszErrTag);

// 装载终端支持的Aid
// in  : pTermAid          : 终端支持的Aid
//       iFlag             : 动作, HXEMV_PARA_INIT:初始化 HXEMV_PARA_ADD:增加 HXEMV_PARA_DEL:删除
// out : pszErrTag         : 如果设置出错, 出错的Tag值
// ret : HXEMV_OK          : OK
//       HXEMV_LACK_MEMORY : 存储空间不足
//       HXEMV_PARA        : 参数错误
//       HXEMV_FLOW_ERROR  : EMV流程错误
// Note: 删除时只传入AID即可
int iHxLoadTermAid(stTermAid *pTermAid, int iFlag, uchar *pszErrTag);

// 装载CA公钥
// in  : pCAPublicKey      : CA公钥
//       iFlag             : 动作, HXEMV_PARA_INIT:初始化 HXEMV_PARA_ADD:增加 HXEMV_PARA_DEL:删除
// ret : HXEMV_OK          : OK
//       HXEMV_LACK_MEMORY : 存储空间不足
//       HXEMV_PARA        : 参数错误
//       HXEMV_FLOW_ERROR  : EMV流程错误
// Note: 删除时只传入RID、index即可
int iHxLoadCAPublicKey(stCAPublicKey *pCAPublicKey, int iFlag);

// 设置IC卡卡座号
// in  : iSlotNo : 卡座号，VPOS规范
// ret : HXEMV_OK       : OK
//       HXEMV_NO_SLOT  : 不支持此卡座
int iHxSetIccSlot(int iSlotNo);

// 设置非接IC卡卡座号
// in  : iSlotNo : 卡座号，VPOS规范
// ret : HXEMV_OK       : OK
//       HXEMV_NO_SLOT  : 不支持此卡座
int iHxSetCtlsIccSlot(int iSlotNo);

// 交易初始化, 每笔新交易开始调用一次
// ret : HXEMV_OK          : OK
//       HXEMV_FLOW_ERROR  : EMV流程错误
int iHxTransInit(void);

// 非接触卡交易预处理
// in  : pszAmount					: 交易金额, ascii码串, 不要小数点, 默认为缺省小数点位置, 例如:RMB1.00表示为"100"
//       uiCurrencyCode				: 货币代码
// ret : HXEMV_OK					: OK
//		 HXEMV_TRY_OTHER_INTERFACE	: 满足拒绝条件，交易终止, 尝试其它通信界面
//									  应显示EMVMSG_TERMINATE信息, 如果终端支持其它通信界面, 提示用户尝试其它通信界面
//       HXEMV_CORE					: 内部错误
// Note: 每笔非接触交易前必须先做预处理, 只有成功完成了预处理才可以进行非接交易
//       应用层要确保交易数据与预处理数据一致
//       参考: JR/T0025.12—2013, 6.2, p9
int iHxCtlsPreProc(uchar *pszAmount, uint uiCurrencyCode);

// 检测卡片
// ret : HXEMV_OK          : OK
//       HXEMV_NO_CARD     : 卡片不存在
int iHxTestCard(void);

// 检测非接卡片
// ret : HXEMV_OK          : OK
//       HXEMV_NO_CARD     : 卡片不存在
int iHxTestCtlsCard(void);

// 关闭卡片
// ret : HXEMV_OK          : OK
int iHxCloseCard(void);

// 强制联机设定,设定TVR强制联机位
// in  : iFlag             : 设定TVR强制联机位标志 0:不设定 1:设定
// ret : HXEMV_OK          : OK
//       HXEMV_FLOW_ERROR  : EMV流程错误
// Note: 并不能确保联机，还要看TAC/IAC-Online强制联机位的设置
//       规范规定:必须是有人值守,且在交易开始的时候进行强制联机处理(emv2008 book4 6.5.3, P56)
//       本核心不支持操作员发起的强制联机, 参考过工行凭条, 其中强制联机位被强制设定, 
//       为了将来可能被要求设定强制联机位, 特提供此函数, 本函数不理会是否是有人值守
int iHxSetForceOnlineFlag(int iFlag);

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
//       数据一旦被读出, 会放到Tlv数据库中, 再次使用可以用iHxGetData()读取
//       出错不会显示错误信息
int iHxGetCardNativeData(uchar *psTag, int *piOutTlvDataLen, uchar *psOutTlvData, int *piOutDataLen, uchar *psOutData);

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
int iHxGetData(uchar *psTag, int *piOutTlvDataLen, uchar *psOutTlvData, int *piOutDataLen, uchar *psOutData);

// 读卡片交易流水支持信息
// out : piMaxRecNum       : 最多交易流水记录个数
// ret : HXEMV_OK          : 读取成功
//       HXEMV_NO_LOG      : 卡片不支持交易流水记录
//       HXEMV_FLOW_ERROR  : EMV流程错误
int iHxGetLogInfo(int *piMaxRecNum);

// 读卡片交易流水
// in  : iLogNo            : 交易流水记录号, 最近的一条记录号为1
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
//       HXEMV_FLOW_ERROR  : EMV流程错误
// note: 出错不会显示错误
int iHxReadLog(int iLogNo, int *piLogLen, uchar *psLog);

// 读卡片圈存交易流水支持信息
// out : piMaxRecNum       : 最多交易流水记录个数
// ret : HXEMV_OK          : 读取成功
//       HXEMV_NO_LOG      : 卡片不支持圈存流水记录
//       HXEMV_FLOW_ERROR  : EMV流程错误
int iHxGetLoadLogInfo(int *piMaxRecNum);

// 读圈存交易流水
// in  : iLogNo            : 交易流水记录号, 最近的一条记录号为1, 要全部读出记录号为0
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
//       HXEMV_FLOW_ERROR  : EMV流程错误
// note: 出错不会显示错误
int iHxReadLoadLog(int iLogNo, int *piLogLen, uchar *psLog);

// 非接触卡GPO结果分析
// out : piTransRoute               : 交易走的路径 0:接触Pboc(标准DC或EC) 1:非接触Pboc(标准DC或EC) 2:qPboc联机 3:qPboc脱机 4:qPboc拒绝
//       piSignFlag					: 需要签字标志(0:不需要签字, 1:需要签字), 仅当走的是qPboc流程时才有效
//       piNeedOnlinePin            : 需要联机密码标志(0:不需要联机密码, 1:需要联机密码), 仅当走的是qPboc联机流程时才有效
// ret : HXEMV_OK					: OK, 根据piTransRoute决定后续流程
//       HXEMV_TERMINATE			: 满足拒绝条件，交易终止
//		 HXEMV_TRY_OTHER_INTERFACE	: 尝试其它通信界面
//       HXEMV_CORE					: 内部错误
// Note: GPO成功后才可以调用
// 参考 JR/T0025.12—2013, 7.8 P40
int iHxCtlsGpoAnalyse(int *piTransRoute, int *piSignFlag, int *piNeedOnlinePin);

// 传入或重新传入金额
// in  : pszAmount         : 金额
//       pszAmountOther    : 其它金额
// ret : HXEMV_OK          : 设置成功
//       HXEMV_FLOW_ERROR  : EMV流程错误
//       HXEMV_PARA        : 参数错误
// note: GPO后GAC金额可能会不同, 重新设定交易金额
//       读记录后, 处理限制前可调用
int iHxSetAmount(uchar *pszAmount, uchar *pszAmountOther);

//// 以下为回调方式API
//// 回调方式API  *** 开始 ***

// 复位卡片
// ret : HXEMV_OK          : OK
//       HXEMV_FLOW_ERROR  : EMV流程错误
//       HXEMV_CARD_REMOVED: 卡被取走
//       HXEMV_CARD_OP     : 卡片复位错误
//		 HXEMV_CALLBACK_METHOD : 没有以回调方式初始化核心
int iHxResetCard(void);

// 读取支持的应用
// in  : iIgnoreBlock	   : !0:忽略应用锁定标志, 0:锁定的应用不会被选择
// ret : HXEMV_OK          : OK
//       HXEMV_NO_APP      : 无支持的应用
//       HXEMV_CARD_REMOVED: 卡被取走
//       HXEMV_CARD_OP     : 卡操作错
//       HXEMV_CARD_SW     : 卡状态字非法
//       HXEMV_CANCEL      : 用户取消
//       HXEMV_TIMEOUT     : 超时
//       HXEMV_TERMINATE   : 满足拒绝条件，交易终止
//       HXEMV_FLOW_ERROR  : EMV流程错误
//       HXEMV_CORE        : 内部错误
//		 HXEMV_CALLBACK_METHOD : 没有以回调方式初始化核心
// Note: 调用该函数后可获取最终匹配的语言(TAG_DFXX_LanguageUsed)
int iHxGetSupportedApp(int iIgnoreBlock);

// 应用选择
// in  : iIgnoreBlock	   : !0:忽略应用锁定标志, 0:锁定的应用不会被选择
// ret : HXEMV_OK          : OK
//       HXEMV_CARD_REMOVED: 卡被取走
//       HXEMV_CARD_OP     : 卡操作错
//       HXEMV_NO_APP	   : 没有选中应用
//       HXEMV_TERMINATE   : 满足拒绝条件，交易终止
//       HXEMV_CANCEL      : 用户取消
//       HXEMV_TIMEOUT     : 超时
//       HXEMV_FLOW_ERROR  : EMV流程错误
//       HXEMV_CORE        : 内部错误
//		 HXEMV_CALLBACK_METHOD : 没有以回调方式初始化核心
int iHxAppSelect(int iIgnoreBlock);

// 设置持卡人语言
// in  : pszLanguage  : 持卡人语言代码, 例如:"zh"
//                      如果传入NULL, 表示使用EMV语言选择方法
// ret : HXEMV_OK     : OK
//       HXEMV_FLOW_ERROR  : EMV流程错误
//		 HXEMV_CALLBACK_METHOD : 没有以回调方式初始化核心
//       HXEMV_PARA   : 参数错误
//       HXEMV_CORE   : 内部错误
// Note: 如果不支持传入的语言, 将会使用本地语言
int iHxSetCardHolderLang(uchar *pszLanguage);

// GPO
// in  : pszTransTime      : 交易时间，YYYYMMDDhhmmss
//       ulTTC             : 终端交易序号, 1-999999
//       uiTransType       : 交易类型, BCD格式(0xAABB, BB为按GB/T 15150 定义的处理码前2 位表示的金融交易类型, AA用于区分商品/服务)
//       uiCurrencyCode    : 货币代码
// ret : HXEMV_OK          : OK
//       HXEMV_CARD_REMOVED: 卡被取走
//       HXEMV_CARD_OP     : 卡操作错
//       HXEMV_CARD_SW     : 非法卡状态字
//       HXEMV_TERMINATE   : 满足拒绝条件，交易终止
//       HXEMV_NO_APP      : 无支持的应用
//       HXEMV_RESELECT    : 需要重新选择应用
//       HXEMV_PARA        : 参数错误
//       HXEMV_FLOW_ERROR  : EMV流程错误
//       HXEMV_TRANS_NOT_ALLOWED : 交易不支持
//       HXEMV_CORE        : 内部错误
//		 HXEMV_CALLBACK_METHOD : 没有以回调方式初始化核心
//       HXEMV_DENIAL	   : qPboc拒绝
// Note: 如果返回HXEMV_RESELECT，需要重新执行iHxAppSelect()
int iHxGPO(uchar *pszTransTime, ulong ulTTC, uint uiTransType, uint uiCurrencyCode);

// 读应用记录
// ret : HXEMV_OK          : OK
//       HXEMV_CARD_REMOVED: 卡被取走
//       HXEMV_CARD_OP     : 卡操作错
//       HXEMV_CARD_SW     : 非法卡指令状态字
//       HXEMV_TERMINATE   : 满足拒绝条件，交易终止
//       HXEMV_FLOW_ERROR  : EMV流程错误
//       HXEMV_CORE        : 内部错误
//		 HXEMV_CALLBACK_METHOD : 没有以回调方式初始化核心
// 非接可能返回码
//		 HXEMV_TRY_OTHER_INTERFACE	: 尝试其它通信界面
int iHxReadRecord(void);

// SDA或DDA
// ret : HXEMV_OK            : OK
//       HXEMV_CARD_REMOVED  : 卡被取走
//       HXEMV_CARD_OP       : 卡操作错
//       HXEMV_CARD_SW       : 非法卡状态字
//       HXEMV_TERMINATE     : 满足拒绝条件，交易终止
//       HXEMV_DENIAL        : 终端行为分析结果为脱机拒绝
//       HXEMV_DENIAL_ADVICE : 终端行为分析结果为脱机拒绝, 有Advice
//       HXEMV_FLOW_ERROR    : EMV流程错误
//       HXEMV_CORE          : 内部错误
//		 HXEMV_CALLBACK_METHOD : 没有以回调方式初始化核心
//		 HXEMV_FDDA_FAIL	 : fDDA失败(非接)
// Note: qPboc, FDDA失败后, 核心不显示任何信息, 应用层需要自行检查9F6C B1 b6b5, 以决定是拒绝还是进行联机
int iHxOfflineDataAuth(void);

// 处理限制
// ret : HXEMV_OK            : OK
//       HXEMV_TERMINATE     : 满足拒绝条件，交易终止
//       HXEMV_FLOW_ERROR    : EMV流程错误
//       HXEMV_CORE          : 内部错误
//       HXEMV_DENIAL        : 终端行为分析结果为脱机拒绝
//       HXEMV_DENIAL_ADVICE : 终端行为分析结果为脱机拒绝, 有Advice
//       HXEMV_CARD_REMOVED  : 卡被取走
//       HXEMV_CARD_OP       : 终端行为分析结果为脱机拒绝, 申请AAC时卡片出错
//		 HXEMV_CALLBACK_METHOD : 没有以回调方式初始化核心
int iHxProcRistrictions(void);

// 终端风险管理
// ret : HXEMV_OK            : OK
//       HXEMV_TERMINATE     : 满足拒绝条件，交易终止
//       HXEMV_CORE          : 其它错误
//       HXEMV_CARD_REMOVED  : 卡被取走
//       HXEMV_CARD_OP       : 卡操作错
//       HXEMV_DENIAL        : 终端行为分析结果为脱机拒绝
//       HXEMV_DENIAL_ADVICE : 终端行为分析结果为脱机拒绝, 有Advice
//       HXEMV_CANCEL        : 被取消
//       HXEMV_TIMEOUT       : 超时
//       HXEMV_FLOW_ERROR    : EMV流程错误
//		 HXEMV_CALLBACK_METHOD : 没有以回调方式初始化核心
int iHxTermRiskManage(void);

// 持卡人验证
// out : piNeedSignFlag      : 如果成功完成，返回需要签字标志，0:不需要签字 1:需要签字
// ret : HXEMV_OK            : OK
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
//		 HXEMV_CALLBACK_METHOD : 没有以回调方式初始化核心
int iHxCardHolderVerify(int *piNeedSignFlag);

// 终端行为分析
// ret : HXEMV_OK		     : OK
//       HXEMV_CORE          : 其它错误
//       HXEMV_DENIAL        : 终端行为分析结果为脱机拒绝
//       HXEMV_DENIAL_ADVICE : 终端行为分析结果为脱机拒绝, 有Advice
//       HXEMV_CARD_REMOVED  : 卡被取走
//       HXEMV_CARD_OP       : 终端行为分析结果为脱机拒绝, 申请AAC时卡片出错
//		 HXEMV_CALLBACK_METHOD : 没有以回调方式初始化核心
int iHxTermActionAnalysis(void);

// 第一次GAC
// out : piCardAction      : 卡片执行结果
//                           GAC_ACTION_TC     : 批准(生成TC)
//							 GAC_ACTION_AAC    : 拒绝(生成AAC)
//                           GAC_ACTION_AAC_ADVICE : 拒绝(生成AAC,有Advice)
//                           GAC_ACTION_ARQC   : 要求联机(生成ARQC)
// ret : HXEMV_OK          : OK
//       HXEMV_CORE        : 其它错误
//       HXEMV_CARD_REMOVED: 卡被取走
//       HXEMV_CARD_OP     : 卡操作错
//       HXEMV_CARD_SW     : 非法状态字
//       HXEMV_TERMINATE   : 满足拒绝条件，交易终止
//       HXEMV_NOT_ALLOWED : 服务不允许
//       HXEMV_NOT_ACCEPTED: 不接受
//       HXEMV_FLOW_ERROR  : EMV流程错误
//		 HXEMV_CALLBACK_METHOD : 没有以回调方式初始化核心
// Note: 某些情况下, 该函数会自动执行第二次Gac
//       如:CDA失败、仅脱机终端GAC1卡片返回ARQC
int iHx1stGAC(int *piCardAction);

// 第二次GAC
// in  : pszArc            : 应答码[2], NULL或""表示联机失败
//       pszAuthCode	   : 授权码[6], NULL或""表示无授权码数据
//       psOnlineData      : 联机响应数据(55域内容),一组TLV格式数据, NULL表示无联机响应数据
//       psOnlineDataLen   : 联机响应数据长度
// out : piCardAction      : 卡片执行结果
//                           GAC_ACTION_TC     : 批准(生成TC)
//							 GAC_ACTION_AAC    : 拒绝(生成AAC)
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
//		 HXEMV_CALLBACK_METHOD : 没有以回调方式初始化核心
int iHx2ndGAC(uchar *pszArc, uchar *pszAuthCode, uchar *psIssuerData, int iIssuerDataLen, int *piCardAction);

//// 以上为回调方式API
//// 回调方式API  *** 结束 ***

//// 以下为非回调方式API
//// 回调方式API  *** 开始 ***

// 复位卡片
// out : piErrMsgType      : 错误信息类型码
// ret : HXEMV_OK          : OK
//       HXEMV_FLOW_ERROR  : EMV流程错误
//       HXEMV_CARD_REMOVED: 卡被取走
//       HXEMV_CARD_OP     : 卡片复位错误
//		 HXEMV_CALLBACK_METHOD : 没有以非回调方式初始化核心
// Note: emv规范要求插卡后要显示请等待信息, 应用层可在调用本函数前先行显示EMVMSG_0E_PLEASE_WAIT,
int iHxResetCard2(int *piErrMsgType);

// 读取支持的应用
// in  : iIgnoreBlock	   : !0:忽略应用锁定标志, 0:锁定的应用不会被选择
//       piAdfNum          : 可容纳的终端与卡片同时支持的Adf个数
// out : paAdfInfo         : 终端与卡片同时支持的Adf列表(以优先级排序)
//       piAdfNum          : 终端与卡片同时支持的Adf个数
//       piMsgType         : 函数返回HXEMV_OK时, 返回 [应用选择提示] 信息类型码
//							 函数返回!HXEMV_OK时, 如果存在, 则为附加错误信息
//       piErrMsgType      : 错误信息类型码, 函数返回!HXEMV_OK并且!HXEMV_AUTO_SELECT时存在
// ret : HXEMV_OK          : OK, 获取的应用必须确认
//       HXEMV_AUTO_SELECT : OK, 获取的应用可自动选择
//       HXEMV_NO_APP      : 无支持的应用
//       HXEMV_CARD_REMOVED: 卡被取走
//       HXEMV_CARD_SW     : 卡状态字非法
//       HXEMV_CARD_OP     : 卡操作错
//       HXEMV_TERMINATE   : 满足拒绝条件，交易终止
//       HXEMV_FLOW_ERROR  : EMV流程错误
//       HXEMV_LACK_MEMORY : 存储空间不足
//		 HXEMV_CALLBACK_METHOD : 没有以非回调方式初始化核心
int iHxGetSupportedApp2(int iIgnoreBlock, stADFInfo *paAdfInfo, int *piAdfNum, int *piMsgType, int *piErrMsgType);

// 应用选择
// in  : iIgnoreBlock	   : !0:忽略应用锁定标志, 0:锁定的应用不会被选择
//       iAidLen             : AID长度
//       psAid               : AID
// out : piErrMsgType        : 错误信息类型码
// ret : HXEMV_OK            : OK
//       HXEMV_CARD_REMOVED  : 卡被取走
//       HXEMV_CARD_OP       : 卡操作错
//       HXEMV_TERMINATE     : 满足拒绝条件，交易终止
//       HXEMV_FLOW_ERROR    : EMV流程错误
//       HXEMV_CORE          : 内部错误
//       HXEMV_RESELECT      : 需要重新选择应用
//       HXEMV_NOT_SUPPORTED : 选择了不支持的应用
//		 HXEMV_CALLBACK_METHOD : 没有以非回调方式初始化核心
// Note: 可通过获取9F7A判断是否支持eCash
int iHxAppSelect2(int iIgnoreBlock, int iAidLen, uchar *psAid, int *piErrMsgType);

// 获取支持的语言以供持卡人选择
// out : pszLangs     : 支持的语言列表, 如果输出字符串长度为0, 表示不必持卡人自行选择
//       pszLangsDesc : '='号结尾的支持语言描述列表
//       piMsgType    : pszLangs表明需要持卡人选择时表示选择语言提示信息类型码
//       piErrMsgType : 错误信息类型码
// ret : HXEMV_OK     : OK
//       HXEMV_FLOW_ERROR  : EMV流程错误
//		 HXEMV_CALLBACK_METHOD : 没有以非回调方式初始化核心
//       HXEMV_CORE        : 内部错误
int iHxGetSupportedLang2(uchar *pszLangs, uchar *pszLangsDesc, int *piMsgType, int *piErrMsgType);

// 设置持卡人语言
// in  : pszLanguage  : 持卡人语言代码, 例如:"zh"
// out : piErrMsgType : 错误信息类型码
// ret : HXEMV_OK     : OK
//       HXEMV_FLOW_ERROR  : EMV流程错误
//		 HXEMV_CALLBACK_METHOD : 没有以非回调方式初始化核心
//       HXEMV_PARA   : 参数错误
//       HXEMV_CORE   : 内部错误
int iHxSetCardHolderLang2(uchar *pszLanguage, int *piErrMsgType);

// GPO
// in  : pszTransTime      : 交易时间，YYYYMMDDhhmmss
//       ulTTC             : 终端交易序号, 1-999999
//       uiTransType       : 交易类型, BCD格式(0xAABB, BB为按GB/T 15150 定义的处理码前2 位表示的金融交易类型, AA用于区分商品/服务)
//       pszAmount         : 交易金额, ascii码串, 不要小数点, 默认为缺省小数点位置, 例如:RMB1.00表示为"100"
//       uiCurrencyCode    : 货币代码
// out : piMsgType         : 信息类型, 额外显示信息, 函数返回HXEMV_RESELECT时附加此提示信息
//       piErrMsgType      : 错误信息类型码, 此信息比piMsgType优先
// ret : HXEMV_OK          : OK
//       HXEMV_CARD_REMOVED: 卡被取走
//       HXEMV_CARD_OP     : 卡操作错
//       HXEMV_CARD_SW     : 非法卡状态字
//       HXEMV_TERMINATE   : 满足拒绝条件，交易终止
//       HXEMV_RESELECT    : 需要重新选择应用
//       HXEMV_TRANS_NOT_ALLOWED : 交易不支持
//       HXEMV_PARA        : 参数错误
//       HXEMV_FLOW_ERROR  : EMV流程错误
//		 HXEMV_CALLBACK_METHOD : 没有以非回调方式初始化核心
//       HXEMV_CORE        : 内部错误
//       HXEMV_DENIAL	   : qPboc拒绝
// Note: 如果返回HXEMV_RESELECT，从iHxGetSupportedApp()开始重新执行流程
//       因为GPO失败后可能需要显示两条信息(不接受&重试一次), 因此最多可能返回了两条信息类型信息
int iHxGPO2(uchar *pszTransTime, ulong ulTTC, uint uiTransType, uchar *pszAmount, uint uiCurrencyCode, int *piMsgType, int *piErrMsgType);

// 读应用记录
// out : psRid+pszPan+piPanSeqNo这三项提供给应用层判断黑名单及分开消费情况
//		 psRid             : RID[5], NULL表示不需要传出
//       pszPan            : 账号
//       piPanSeqNo        : 账号序列号, -1表示无此内容, NULL表示不需要传出
//       piErrMsgType      : 错误信息类型码
// ret : HXEMV_OK          : OK
//       HXEMV_CARD_REMOVED: 卡被取走
//       HXEMV_CARD_OP     : 卡操作错
//       HXEMV_CARD_SW     : 非法卡指令状态字
//       HXEMV_TERMINATE   : 满足拒绝条件，交易终止
//       HXEMV_FLOW_ERROR  : EMV流程错误
//       HXEMV_CORE        : 内部错误
//		 HXEMV_CALLBACK_METHOD : 没有以非回调方式初始化核心
// 非接可能返回码
//		 HXEMV_TRY_OTHER_INTERFACE	: 尝试其它通信界面
int iHxReadRecord2(uchar *psRid, uchar *pszPan, int *piPanSeqNo, int *piErrMsgType);

// 设置卡片账户黑名单及分开消费信息
// in  : iBlackFlag        : 设置该卡是否为黑名单卡, 0:不为黑名单卡 1:黑名单卡
//       pszRecentAmount   : 最近一笔消费金额(用于分开消费检查)
// out : piErrMsgType      : 错误信息类型码
// ret : HXEMV_OK          : OK
//       HXEMV_CORE        : 内部错误
// Note: 如果读记录后发现卡片是黑名单卡或有最近消费记录, 则需要调用此函数设置账户信息
int iHxSetPanInfo2(int iBlackFlag, uchar *pszRecentAmount, int *piErrMsgType);

// SDA或DDA
// out : piNeedCheckCrlFlag  : 是否需要判断发卡行公钥证书为黑名单标志, 0:不需要判断 1:需要判断
//       psRid               : RID[5], psRid+pucCaIndex+psCertSerNo这三项提供给应用层判断发卡行公钥证书是否在黑名单列表中
//       pucCaIndex          : CA公钥索引
//       psCertSerNo         : 发卡行公钥证书序列号[3]
//       piErrMsgType        : 错误信息类型码
// ret : HXEMV_OK            : OK
//       HXEMV_CARD_REMOVED  : 卡被取走
//       HXEMV_CARD_OP       : 卡操作错
//       HXEMV_CARD_SW       : 非法卡状态字
//       HXEMV_TERMINATE     : 满足拒绝条件，交易终止
//       HXEMV_DENIAL        : 终端行为分析结果为脱机拒绝
//       HXEMV_DENIAL_ADVICE : 终端行为分析结果为脱机拒绝, 有Advice
//       HXEMV_FLOW_ERROR    : EMV流程错误
//       HXEMV_CORE          : 内部错误
//		 HXEMV_CALLBACK_METHOD : 没有以非回调方式初始化核心
//		 HXEMV_FDDA_FAIL	 : fDDA失败(非接)
// Note: 应用层支持发卡行公钥证书黑名单检查是, piNeedCheckCrlFlag才有意义, 否则可以忽略
// Note: qPboc, FDDA失败后, piErrMsgType不可用, 应用层需要自行检查9F6C B1 b6b5, 以决定是拒绝还是进行联机
int iHxOfflineDataAuth2(int *piNeedCheckCrlFlag, uchar *psRid, uchar *pucCaIndex, uchar *psCertSerNo, int *piErrMsgType);

// 设置发卡行公钥证书为黑名单
// out : piErrMsgType        : 错误信息类型码, 0表示无需要显示的信息
// ret : HXEMV_OK            : OK
//       HXEMV_CARD_REMOVED  : 卡被取走
//       HXEMV_CARD_OP       : 卡操作错
//       HXEMV_CARD_SW       : 非法卡状态字
//       HXEMV_TERMINATE     : 满足拒绝条件，交易终止
//       HXEMV_DENIAL        : 终端行为分析结果为脱机拒绝
//       HXEMV_DENIAL_ADVICE : 终端行为分析结果为脱机拒绝, 有Advice
//       HXEMV_FLOW_ERROR    : EMV流程错误
//       HXEMV_CORE          : 内部错误
//		 HXEMV_CALLBACK_METHOD : 没有以非回调方式初始化核心
// Note: 应用层调用iHxOfflineDataAuth2()后得到Rid+CaIndex+CertSerNo
//       根据此数据判断发卡行公钥证书是否为黑名单,只有是黑名单,才需要调用本函数通知核心
int iHxSetIssuerCertCrl2(int *piErrMsgType);

// 处理限制
// out : piErrMsgType        : 错误信息类型码
// ret : HXEMV_OK            : OK
//       HXEMV_TERMINATE     : 满足拒绝条件，交易终止
//       HXEMV_FLOW_ERROR    : EMV流程错误
//       HXEMV_CORE          : 内部错误
//       HXEMV_DENIAL        : 终端行为分析结果为脱机拒绝
//       HXEMV_DENIAL_ADVICE : 终端行为分析结果为脱机拒绝, 有Advice
//       HXEMV_CARD_REMOVED  : 卡被取走
//       HXEMV_CARD_OP       : 终端行为分析结果为脱机拒绝, 申请AAC时卡片出错
//		 HXEMV_CALLBACK_METHOD : 没有以非回调方式初始化核心
int iHxProcRistrictions2(int *piErrMsgType);

// 终端风险管理
// out : piErrMsgType        : 错误信息类型码
// ret : HXEMV_OK            : OK
//       HXEMV_TERMINATE     : 满足拒绝条件，交易终止
//       HXEMV_CORE          : 其它错误
//       HXEMV_CARD_REMOVED  : 卡被取走
//       HXEMV_CARD_OP       : 卡操作错
//       HXEMV_DENIAL        : 终端行为分析结果为脱机拒绝
//       HXEMV_DENIAL_ADVICE : 终端行为分析结果为脱机拒绝, 有Advice
//       HXEMV_FLOW_ERROR    : EMV流程错误
//		 HXEMV_CALLBACK_METHOD : 没有以非回调方式初始化核心
int iHxTermRiskManage2(int *piErrMsgType);

// 获取持卡人验证
// out : piCvm               : 持卡人认证方法, 支持HXCVM_PLAIN_PIN、HXCVM_CIPHERED_ONLINE_PIN、HXCVM_HOLDER_ID、HXCVM_CONFIRM_AMOUNT
//       piBypassFlag        : 允许bypass标志, 0:不允许, 1:允许
//       piMsgType           : 提示信息码(如果验证方法为证件验证, 返回证件类型码)
//       piMsgType2          : 附加提示信息, 如果不为0, 则需要先显示本信息
//       pszAmountStr        : 提示用格式化后金额串(HXCVM_HOLDER_ID验证方法不需要)
//       piErrMsgType        : 错误信息类型码, 函数返回!HXEMV_OK并且!HXEMV_NO_DATA时存在
// ret : HXEMV_OK            : OK
//       HXEMV_NO_DATA       : 无需继续进行持卡人验证
//       HXEMV_CARD_REMOVED  : 卡被取走
//       HXEMV_CARD_OP       : 卡操作错
//       HXEMV_TERMINATE     : 满足拒绝条件，交易终止
//       HXEMV_DENIAL        : 终端行为分析结果为脱机拒绝
//       HXEMV_DENIAL_ADVICE : 终端行为分析结果为脱机拒绝, 有Advice
//       HXEMV_FLOW_ERROR    : EMV流程错误
//       HXEMV_CORE          : 内部错误
//		 HXEMV_CALLBACK_METHOD : 没有以非回调方式初始化核心
// Note: 与iHxDoCvmMethod2()搭配, 可能需要多次调用, 知道返回HXEMV_NO_DATA
//       由于返回的信息较多, 对于支持HXCVM_HOLDER_ID验证方法, piMsgType返回的是证件类型码
//       此时其它信息使用固定信息码, 描述如下:
//           EMVMSG_VERIFY_HOLDER_ID		"请查验持卡人证件"
//			 EMVMSG_HOLDER_ID_TYPE			"持卡人证件类型"
//			 EMVMSG_HOLDER_ID_NO			"持卡人证件号码"
//       证件号码提取Tag为:TAG_9F61_HolderId
int iHxGetCvmMethod2(int *piCvm, int *piBypassFlag, int *piMsgType, int *piMsgType2, uchar *pszAmountStr, int *piErrMsgType);

// 执行持卡人认证
// in  : iCvmProc            : 认证方法处理方式, HXCVM_PROC_OK or HXCVM_BYPASS or HXCVM_FAIL or HXCVM_CANCEL or HXCVM_TIMEOUT
//       psCvmData           : 输入的密码, 如果为明文密码, 密码尾部要补0
// out : piMsgType           : 返回HXEMV_OK时使用
//                             如果不为0, 则需要显示本类型信息
//       piMsgType2          : 返回HXEMV_OK时使用
//                             如果不为0, 则需要显示本类型信息
//       piErrMsgType        : 错误信息类型码
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
//		 HXEMV_CALLBACK_METHOD : 没有以非回调方式初始化核心
// Note: 执行的CVM必须是最近一次iHxGetCvmMethod2()获得的CVM
int iHxDoCvmMethod2(int iCvmProc, uchar *psCvmData, int *piMsgType, int *piMsgType2, int *piErrMsgType);

// 获取CVM认证方法签字标志
// out : piNeedSignFlag    : 表示需要签字标志，0:不需要签字 1:需要签字
// ret : HXEMV_OK          : OK
int iHxGetCvmSignFlag2(int *piNeedSignFlag);

// 终端行为分析
// out : piErrMsgType        : 错误信息类型码
// ret : HXEMV_OK            : OK
//       HXEMV_CORE          : 其它错误
//       HXEMV_FLOW_ERROR    : EMV流程错误
//       HXEMV_CARD_REMOVED  : 卡被取走
//       HXEMV_CARD_OP       : 卡操作错
//       HXEMV_DENIAL        : 终端行为分析结果为脱机拒绝
//       HXEMV_DENIAL_ADVICE : 终端行为分析结果为脱机拒绝, 有Advice
//		 HXEMV_CALLBACK_METHOD : 没有以非回调方式初始化核心
int iHxTermActionAnalysis2(int *piErrMsgType);

// 第一次GAC
// out : piCardAction      : 卡片执行结果
//                           GAC_ACTION_TC     : 批准(生成TC)
//							 GAC_ACTION_AAC    : 拒绝(生成AAC)
//                           GAC_ACTION_AAC_ADVICE : 拒绝(生成AAC,有Advice)
//                           GAC_ACTION_ARQC   : 要求联机(生成ARQC)
//       piMsgType         : 提示信息码, 函数返回HXEMV_OK时使用
//       piErrMsgType      : 错误信息类型码
// ret : HXEMV_OK          : OK
//       HXEMV_CORE        : 其它错误
//       HXEMV_CARD_REMOVED: 卡被取走
//       HXEMV_CARD_OP     : 卡操作错
//       HXEMV_CARD_SW     : 非法状态字
//       HXEMV_TERMINATE   : 满足拒绝条件，交易终止
//       HXEMV_NOT_ACCEPTED: 不接受
//       HXEMV_NOT_ALLOWED : 服务不允许
//       HXEMV_FLOW_ERROR  : EMV流程错误
// Note: 某些情况下, 该函数会自动调用iHx2ndGAC()
//       如:CDA失败、仅脱机终端GAC1卡片返回ARQC...
//		 HXEMV_CALLBACK_METHOD : 没有以非回调方式初始化核心
int iHx1stGAC2(int *piCardAction, int *piMsgType, int *piErrMsgType);

// 第二次GAC
// in  : pszArc            : 应答码[2], NULL或""表示联机失败
//       pszAuthCode	   : 授权码[6], NULL或""表示无授权码数据
//       psOnlineData      : 联机响应数据(55域内容),一组TLV格式数据, NULL表示无联机响应数据
//       psOnlineDataLen   : 联机响应数据长度
// out : piCardAction      : 卡片执行结果
//                           GAC_ACTION_TC     : 批准(生成TC)
//							 GAC_ACTION_AAC    : 拒绝(生成AAC)
//                           GAC_ACTION_AAC_ADVICE : 拒绝(生成AAC,有Advice)
//       piMsgType         : 提示信息码, 函数返回HXEMV_OK时使用
//       piErrMsgType      : 错误信息类型码
// ret : HXEMV_OK          : OK
//       HXEMV_CORE        : 其它错误
//       HXEMV_CARD_REMOVED: 卡被取走
//       HXEMV_CARD_OP     : 卡操作错
//       HXEMV_CARD_SW     : 非法卡状态字
//       HXEMV_TERMINATE   : 满足拒绝条件，交易终止
//       HXEMV_NOT_ALLOWED : 服务不允许
//       HXEMV_NOT_ACCEPTED: 不接受
//       HXEMV_FLOW_ERROR  : EMV流程错误
//		 HXEMV_CALLBACK_METHOD : 没有以非回调方式初始化核心
int iHx2ndGAC2(uchar *pszArc, uchar *pszAuthCode, uchar *psIssuerData, int iIssuerDataLen, int *piCardAction, int *piMsgType, int *piErrMsgType);

// 获取提示信息内容
// in  : iMsgType    : 信息类型, 参考EmvMsg.h
//       iMsgFor     : 信息接受者类型(HXMSG_SMART or HXMSG_OPERATOR or HXMSG_CARDHOLDER or HXMSG_LOCAL_LANG)
// out : pszMsg      : 按接受者语言返回的信息内容
//       pszLanguage : 使用的语言代码，ISO-639-1，小写. 传入NULL表示不需要返回
// ret : 返回值等于pszMsg
// Note: 如果信息类型不支持, pszMsg[0]会被赋值为0
uchar *pszHxGetMsg2(int iMsgType, int iMsgFor, uchar *pszMsg, uchar *pszLanguage);

//// 以上为非回调方式API
//// 回调方式API  *** 结束 ***

#endif
