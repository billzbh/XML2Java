package com.hxsmart.imateinterface.pbocsdk;

import com.hxsmart.imateinterface.BluetoothThread;

/*
 * 深圳华信智能科技有限公司研发的Pboc3.0借贷记/EMV2004核心已经通过了BCTC的Level 2认证, 
 * 但底层接口比较专业化了一些, 不适合高层客户调用, 本文档描述的客户接口更适合于不过分关心
 * Pboc3.0借贷记/EMV2004 Level 2认证细节的终端或系统调用.
 * 
 * EMV核心支持的功能:
 * 1、应用选择
 * 		PSE法、AID列表法
 * 2、脱机数据认证
 * 		SDA、DDA、CDA
 * 3、终端风险管理
 * 		限额检查、随机检查、频度检查
 * 		客户接口提供可进行发卡行证书检查的数据以方便高层程序检查
 * 		高层程序需自行做异常文件检查、分开销售检查, 客户接口提供对异常文件检查、分开销售检查的后续处理
 * 4、持卡人认证
 * 		脱机PIN明文验证、联机PIN输入、签字、无需CVM、CVM失败
 * 5、联机方式
 * 		纯脱机、纯联机、脱机并有联机功能
 * 6、交易记录查询
 * 7、ECash支持
 */

public class PbocApi {
	public static final int HXEMV_OK 					= 0;  // OK
	public static final int HXEMV_NA					= 1;  // 不可用
	public static final int HXEMV_PARA					= 2;  // 参数错误
	public static final int HXEMV_LACK_MEMORY			= 3;  // 存储空间不足
	public static final int HXEMV_CORE					= 4;  // 内部错误
	public static final int HXEMV_NO_SLOT				= 5;  // 不支持的卡座
	public static final int HXEMV_NO_CARD       		= 6;  // 卡片不存在
	public static final int HXEMV_CANCEL				= 7; // 用户取消
	public static final int HXEMV_TIMEOUT				= 8;  // 超时
	public static final int HXEMV_NO_APP				= 9;  // 无支持的应用
	public static final int HXEMV_AUTO_SELECT   		= 10; // 获取的应用可自动选择
	public static final int HXEMV_CARD_REMOVED			= 11; // 卡被取走
	public static final int HXEMV_CARD_OP				= 12; // 卡操作错
	public static final int HXEMV_CARD_SW				= 13; // 非法卡指令状态字
	public static final int HXEMV_NO_DATA				= 14; // 无数据
	public static final int HXEMV_NO_RECORD				= 15; // 无记录
	public static final int HXEMV_NO_LOG				= 16; // 卡片不支持交易流水记录
	public static final int HXEMV_TERMINATE				= 17; // 满足拒绝条件，交易终止
	public static final int HXEMV_USE_MAG				= 18; // 请使用磁条卡
	public static final int HXEMV_RESELECT				= 19; // 需要重新选择应用
	public static final int HXEMV_NOT_SUPPORTED 		= 20; // 不支持
	public static final int HXEMV_DENIAL				= 21; // 交易拒绝
	public static final int HXEMV_DENIAL_ADVICE			= 22; // 交易拒绝, 有Advice
	public static final int HXEMV_NOT_ALLOWED			= 23; // 服务不允许
	public static final int HXEMV_TRANS_NOT_ALLOWED		= 24; // 交易不允许
	public static final int HXEMV_FLOW_ERROR			= 25; // EMV流程错误
	public static final int HXEMV_CALLBACK_METHOD 		= 26; // 回调与非回调核心接口调用错误
	public static final int HXEMV_NOT_ACCEPTED  		= 27; // 不接受

	// 持卡人认证方法
	public static final int HXCVM_PLAIN_PIN            	= 0x01; // 脱机明文密码认证
	public static final int HXCVM_CIPHERED_OFFLINE_PIN 	= 0x02; // 脱机密文密码认证(现不支持)
	public static final int HXCVM_CIPHERED_ONLINE_PIN  	= 0x03; // 联机密文密码认证
	public static final int HXCVM_HOLDER_ID            	= 0x04; // 持卡人证件认证
	public static final int HXCVM_CONFIRM_AMOUNT      	= 0x10; // 非持卡人验证,仅用于要求确认金额
	
	// 持卡人认证方法处理方式
	public static final int HXCVM_PROC_OK              	= 0x00; // 正常处理完毕
	public static final int HXCVM_BYPASS               	= 0x01; // 要求输密码或验证证件时选择了Bypass
	public static final int HXCVM_FAIL                 	= 0x02; // 证件验证没有通过
	public static final int HXCVM_CANCEL               	= 0x03; // 被取消
	public static final int HXCVM_TIMEOUT              	= 0x04; // 超时

	// GAC卡片应答
	public static final int GAC_ACTION_TC				= 0x00; // 批准(生成TC)
	public static final int GAC_ACTION_AAC				= 0x01; // 拒绝(生成AAC)
	public static final int GAC_ACTION_AAC_ADVICE		= 0x02; // 拒绝(生成AAC,有Advice)
	public static final int GAC_ACTION_ARQC				= 0x03; // 要求联机(生成ARQC)
	
	// 交易类型定义
	// 低位为交易码, 即Tag9C内容, 8583 Field02处理码前2位
	public static final int TR_SALE						= 0x0000; // 消费
	public static final int TR_FIXEDACC_LOAD    		= 0x0060; // 指定账户现金充值
	public static final int TR_EC_BALANCE_INQ			= 0x0131; // 电子现金余额查询
	public static final int TR_LOG_INQ					= 0x0138; // 交易日志查询
	public static final int TR_LOAD_LOG_INQ				= 0x0238; // 圈存日志查询
	public static final int TR_RELOAD_PIN				= 0x0198; // 重装密码

	//	 初始化核心
	//	 ret : HXEMV_OK       : OK
	//	       HXEMV_PARA     : 参数错误
	//		   HXEMV_NA		  : 不可用
	//	       HXEMV_CORE     : 内部错误
	//	 Note: 在调用任何其它接口前必须且先初始化核心
	public int iHxEmvInit() {				
		return iHxEmvInitNative();		
	}
	
	//	 初始化核心
	//	 ret : HXEMV_OK       : OK
	//	       HXEMV_PARA     : 参数错误
	//	       HXEMV_CORE     : 内部错误
	//	 Note: 在调用任何其它接口前必须且先初始化核心
	private native int iHxEmvInitNative();
	
	//	 获取核心信息
	//	 out : emvInfo    		 : 核心信息
	//	 ret : HXEMV_OK       	 : OK
	//	       HXEMV_PARA        : 参数错误
	public native int iHxEmvInfo(HxEmvInfo emvInfo);

	//	 设置终端参数
	//	 in  : TermParam      	 : 终端参数
	//	 ret : HXEMV_OK          : OK
	//	       HXEMV_PARA        : 参数错误
	//	       HXEMV_CORE        : 内部错误
	//	       HXEMV_FLOW_ERROR  : EMV流程错误
	public native int iHxEmvSetParam(HxTermParam termParam);
	
	//	 装载终端支持的Aid
	//	 in  : termAids       	 : 终端支持的Aid数组
	//	 ret : HXEMV_OK          : OK
	//	       HXEMV_LACK_MEMORY : 存储空间不足
	//	       HXEMV_PARA        : 参数错误
	//	       HXEMV_FLOW_ERROR  : EMV流程错误
	public native int iHxEmvLoadAid(HxTermAid[] termAids);

	//	 装载CA公钥
	//	 in  : caPublicKeys   	 : CA公钥
	//	 ret : HXEMV_OK          : OK
	//	       HXEMV_LACK_MEMORY : 存储空间不足
	//	       HXEMV_PARA        : 参数错误
	//	       HXEMV_FLOW_ERROR  : EMV流程错误
	public native int iHxEmvLoadCaPublicKey(HxCaPublicKey[] caPublicKeys);

	//	 交易初始化
	//	 in  : flag              : =0, 保留以后使用
	//	 ret : HXEMV_OK          : OK
	//	       HXEMV_FLOW_ERROR  : EMV流程错误
	//	       HXEMV_NO_CARD     : 卡片不存在
	//	       HXEMV_CARD_OP     : 卡片复位错误
	public native int iHxEmvTransInit(int flag);

	//	 读取支持的应用
	//	 in  : ignoreBlock	   	 : !0:忽略应用锁定标志, 0:锁定的应用不会被选择
	//	 out : adfInfos       	 : 终端与卡片同时支持的Adf数组(以优先级排序)
	//							   需要预先创建maxAdfNum个HxAdfInfo对象: HxAdfInfo[] adfInfos = new HxAdfInfo[maxAdfNum];
	//							   其中maxAdfNum为可容纳的终端与卡片同时支持的Adf个数
	//	 ret : HXEMV_OK          : OK, 获取的应用必须确认
	//	       HXEMV_AUTO_SELECT : OK, 获取的应用可自动选择
	//	       HXEMV_NO_APP      : 无支持的应用
	//	       HXEMV_CARD_SW     : 卡状态字非法
	//	       HXEMV_CARD_OP     : 卡操作错
	//	       HXEMV_TERMINATE   : 满足拒绝条件，交易终止
	//	       HXEMV_FLOW_ERROR  : EMV流程错误
	//	       HXEMV_LACK_MEMORY : 存储空间不足
	//	       HXEMV_PARA        : 参数错误
	public native int iHxEmvGetSupportedApp(int ignoreBlock, HxAdfInfo[] adfInfos);

	//	 应用选择
	//	 in  : ignoreBlock  	   : !0:忽略应用锁定标志, 0:锁定的应用不会被选择
	//	       aid                 : AID ，length <= 32
	//	 ret : HXEMV_OK            : OK
	//	       HXEMV_CARD_REMOVED  : 卡被取走
	//	       HXEMV_CARD_OP       : 卡操作错
	//	       HXEMV_TERMINATE     : 满足拒绝条件，交易终止
	//	       HXEMV_FLOW_ERROR    : EMV流程错误
	//	       HXEMV_CORE          : 内部错误
	//	       HXEMV_RESELECT      : 需要重新选择应用
	//	       HXEMV_NOT_SUPPORTED : 选择了不支持的应用
	public native int iHxEmvAppSelect(int ignoreBlock, String aid);

	//	 读取卡片内部数据
	//	 in  : tag               : 数据标签, 例:0x9F79:电子现金余额
	//	 out : outTlvData        : 读出的HxTlvData对象，调用该方法之前需要预先创建
	//	 ret : HXEMV_OK          : 读取成功
	//	       HXEMV_NO_DATA     : 无此数据
	//	       HXEMV_LACK_MEMORY : 缓冲区不足
	//	       HXEMV_CARD_REMOVED: 卡被取走
	//	       HXEMV_CARD_OP     : 卡操作错
	//	       HXEMV_CARD_SW     : 非法卡状态字
	//	       HXEMV_FLOW_ERROR  : EMV流程错误
	//	       HXEMV_CORE        : 内部错误
	//	 note: 数据一旦被读出, 会放到Tlv数据库中, 再次使用可以用iHxEmvGetData()读取
	public native int iHxEmvGetCardNativeData(int tag, HxTlvData outTlvData);

	//	 读取应用数据
	//	 in  : tag               : 数据标签, 例:0x82
	//	 out : outTlvData        : 读出的HxTlvData对象，调用该方法之前需要预先创建对象
	//	 ret : HXEMV_OK          : 读取成功
	//	       HXEMV_NO_DATA     : 无此数据
	//	       HXEMV_LACK_MEMORY : 缓冲区不足
	//	       HXEMV_FLOW_ERROR  : EMV流程错误
	public native int iHxEmvGetData(int tag, HxTlvData outTlvData);
	
	// 读卡片交易流水支持信息
	// in  : flag               : 0:标准交易流水 1:圈存流水
	// out : maxRecNum          : 最多交易流水记录个数
	// ret : HXEMV_OK           : 读取成功
	//	     HXEMV_NO_LOG       : 卡片不支持交易流水记录
	//	     HXEMV_PARA         : 参数错误
	//	     HXEMV_FLOW_ERROR   : EMV流程错误
	public native int iHxEmvGetLogInfo(int flag, Integer maxRecNum);

	// 读卡片交易流水
	// in  : flag                : 0:标准交易流水 1:圈存流水
	//       logNo               : 交易流水记录号, 最近的一条记录号为1
	// out : cardLog             : 记录内容对象(IC卡原格式输出)，需要预先创建，需要预先创建byte数组，MaxLength = 100
	//                             需要预先创建 byte[] cardlog = new byte[100];
	//       logLen              : 日志数据的长度
	// ret : HXEMV_OK          	 : 读取成功
	//  	 HXEMV_NO_RECORD   	 : 无此记录
	//  	 HXEMV_LACK_MEMORY 	 : 缓冲区不足
	//  	 HXEMV_CARD_REMOVED	 : 卡被取走
	//  	 HXEMV_CARD_OP     	 : 卡操作错
	//  	 HXEMV_CARD_SW     	 : 非法卡指令状态字
	//  	 HXEMV_NO_LOG      	 : 卡片不支持交易流水记录
	//  	 HXEMV_PARA        	 : 参数错误
	// 	 	 HXEMV_FLOW_ERROR  	 : EMV流程错误
	public native int iHxEmvReadLog(int flag, int logNo, byte[] cardLog, Integer logLen);

	//	 GPO
	//	 in  : transTime         : 交易时间，YYYYMMDDhhmmss
	//	       ttc               : 终端交易序号, 1-999999
	//	       transType         : 交易类型,处理码前2位表示的金融交易类型
	//	                           比如现金充值63表示为0x63
	//	       amount            : 交易金额, ascii码串, 不要小数点, 默认为缺省小数点位置, 例如:RMB1.00表示为"100"
	//	       currencyCode      : 货币代码, 156=人民币
	//	 ret : HXEMV_OK          : OK
	//	       HXEMV_CARD_OP     : 卡操作错
	//	       HXEMV_CARD_SW     : 非法卡状态字
	//	       HXEMV_TERMINATE   : 满足拒绝条件，交易终止
	//	       HXEMV_RESELECT    : 需要重新选择应用
	//	       HXEMV_TRANS_NOT_ALLOWED : 交易不支持
	//	       HXEMV_PARA        : 参数错误
	//	       HXEMV_FLOW_ERROR  : EMV流程错误
	//	       HXEMV_CORE        : 内部错误
	//	 Note: 如果返回HXEMV_RESELECT，从iHxEmvGetSupportedApp()开始重新执行流程
	public native int iHxEmvGPO(String transTime, long ttc, int transType, String amount, int currencyCode);

	//	 传入或重新传入金额
	//	 in  : amount            : 金额
	//	 ret : HXEMV_OK          : 设置成功
	//	       HXEMV_FLOW_ERROR  : EMV流程错误
	//	       HXEMV_PARA        : 参数错误
	//	 note: GPO后GAC金额可能会不同, 重新设定交易金额
	//	       GPO后, GAC1前才可以调用
	public native int iHxEmvSetAmount(String amount);

	//	 读应用记录
	//	 ret : HXEMV_OK          : OK
	//	       HXEMV_CARD_OP     : 卡操作错
	//	       HXEMV_CARD_SW     : 非法卡指令状态字
	//	       HXEMV_TERMINATE   : 满足拒绝条件，交易终止
	//	       HXEMV_FLOW_ERROR  : EMV流程错误
	//	       HXEMV_CORE        : 内部错误
	public native int iHxEmvReadRecord();

	//	 设置卡片黑名单及分开消费标志
	//	 in  : blackFlag        : 设置该卡是否为黑名单卡, 0:不为黑名单卡 1:黑名单卡
	//	       separateFlag     : 设置该卡累计消费超限额, 0:没超限额 1:超限额
	//	 ret : HXEMV_OK          : OK
	//	       HXEMV_CORE        : 内部错误
	//	 Note: 如果读记录后发现卡片是黑名单卡或累计消费超限额, 则需要调用此函数设置账户信息
	//	       如果不支持或检查不许设置标记, 则不需要调用本接口
	public native int iHxEmvSetPanFlag(int blackFlag, int separateFlag);

	//	 SDA或DDA
	//	 out : offlineAuthData     : SDA或DDA返回的数据对象
	//                               需要预先创建 HxOfflineAuthData offlineAuthData = new HxOfflineAuthData();
	//	 ret : HXEMV_OK            : OK
	//	       HXEMV_CARD_OP       : 卡操作错
	//	       HXEMV_CARD_SW       : 非法卡状态字
	//	       HXEMV_TERMINATE     : 满足拒绝条件，交易终止
	//	       HXEMV_DENIAL        : 终端行为分析结果为脱机拒绝
	//	       HXEMV_DENIAL_ADVICE : 终端行为分析结果为脱机拒绝, 有Advice
	//	       HXEMV_FLOW_ERROR    : EMV流程错误
	//	       HXEMV_CORE          : 内部错误
	//	 Note: 应用层支持发卡行公钥证书黑名单检查时, piNeedCheckCrlFlag才有意义, 否则可以忽略
	public native int iHxEmvOfflineDataAuth(HxOfflineAuthData offlineAuthData);

	//	 设置发卡行公钥证书为黑名单
	//	 in  : issuerCertCrlFlag   : 是否在黑名单列表标志, 0:不在黑名单列表 1:在黑名单列表
	//	 ret : HXEMV_OK            : OK
	//	       HXEMV_CARD_OP       : 卡操作错
	//	       HXEMV_CARD_SW       : 非法卡状态字
	//	       HXEMV_TERMINATE     : 满足拒绝条件，交易终止
	//	       HXEMV_DENIAL        : 终端行为分析结果为脱机拒绝
	//	       HXEMV_DENIAL_ADVICE : 终端行为分析结果为脱机拒绝, 有Advice
	//	       HXEMV_FLOW_ERROR    : EMV流程错误
	//	       HXEMV_CORE          : 内部错误
	//	 Note: 应用层调用iHxEmvOfflineDataAuth()后得到Rid+CaIndex+CertSerNo
	//	       根据此数据判断发卡行公钥证书是否为黑名单,只有是黑名单,才需要调用本函数通知核心
	public native int iHxEmvSetIssuerCertCrl(int issuerCertCrlFlag);

	//	 处理限制
	//	 ret : HXEMV_OK            : OK
	//	       HXEMV_TERMINATE     : 满足拒绝条件，交易终止
	//	       HXEMV_FLOW_ERROR    : EMV流程错误
	//	       HXEMV_CORE          : 内部错误
	//	       HXEMV_DENIAL        : 终端行为分析结果为脱机拒绝
	//	       HXEMV_DENIAL_ADVICE : 终端行为分析结果为脱机拒绝, 有Advice
	//	       HXEMV_CARD_OP       : 终端行为分析结果为脱机拒绝, 申请AAC时卡片出错
	public native int iHxEmvProcRistrictions();

	//	 终端风险管理
	//	 ret : HXEMV_OK            : OK
	//	       HXEMV_TERMINATE     : 满足拒绝条件，交易终止
	//	       HXEMV_CORE          : 其它错误
	//	       HXEMV_CARD_OP       : 卡操作错
	//	       HXEMV_DENIAL        : 终端行为分析结果为脱机拒绝
	//	       HXEMV_DENIAL_ADVICE : 终端行为分析结果为脱机拒绝, 有Advice
	//	       HXEMV_FLOW_ERROR    : EMV流程错误
	public native int iHxEmvTermRiskManage();

	//	 获取持卡人验证
	//	 out : cvm                 : 持卡人认证方法, 支持HXCVM_PLAIN_PIN、HXCVM_CIPHERED_ONLINE_PIN、HXCVM_HOLDER_ID、HXCVM_CONFIRM_AMOUNT
	//							     cvm必须预先创建: Integer cvm = new Integer(0);
	//	       bypassFlag          : 允许bypass标志, false:不允许, true:允许, bypassFlag必须预先创建：Boolean bypassFlag = new Boolean(false);
	//	 ret : HXEMV_OK            : OK
	//	       HXEMV_NO_DATA       : 无需继续进行持卡人验证
	//	       HXEMV_CARD_OP       : 卡操作错
	//	       HXEMV_TERMINATE     : 满足拒绝条件，交易终止
	//	       HXEMV_DENIAL        : 终端行为分析结果为脱机拒绝
	//	       HXEMV_DENIAL_ADVICE : 终端行为分析结果为脱机拒绝, 有Advice
	//	       HXEMV_FLOW_ERROR    : EMV流程错误
	//	       HXEMV_CORE          : 内部错误
	//	 Note: 与iHxEmvDoCvmMethod()搭配, 可能需要多次调用, 直到返回HXEMV_NO_DATA
	public native int iHxEmvGetCvmMethod(Integer cvm, Boolean bypassFlag);

	//	 执行持卡人验证
	//	 in  : cvmProc             : 认证方法处理方式, HXCVM_PROC_OK or HXCVM_BYPASS or HXCVM_FAIL or HXCVM_CANCEL or HXCVM_TIMEOUT
	//	       cvmData             : 输入的密码
	//	 out : prompt              : 额外提示信息, 0:无额外提示信息 1:密码错,可重试 2:密码错,密码已锁 3:脱机密码验证成功
	//                               prompt必须预先创建：Integer prompt = new Integer(0);
	//	 ret : HXEMV_OK            : OK, 需要继续进行持卡人验证, 继续调用iHxGetCvmMethod(), 然后再调用本函数
	//	       HXEMV_PARA		   : 参数错误
	//	       HXEMV_CANCEL        : 用户取消
	//	       HXEMV_TIMEOUT       : 超时
	//	       HXEMV_CARD_OP       : 卡操作错
	//	       HXEMV_CARD_SW       : 非法卡状态字
	//	       HXEMV_TERMINATE     : 满足拒绝条件，交易终止
	//	       HXEMV_DENIAL        : 终端行为分析结果为脱机拒绝
	//	       HXEMV_DENIAL_ADVICE : 终端行为分析结果为脱机拒绝, 有Advice
	//	       HXEMV_FLOW_ERROR    : EMV流程错误
	//	       HXEMV_CORE          : 内部错误
	//	 Note: 执行的CVM必须是最近一次iHxEmvGetCvmMethod()获得的CVM
	public native int iHxEmvDoCvmMethod(int cvmProc, byte[] vmData, Integer prompt);

	// 获取CVM认证方法签字标志
	// out : needSignFlag      : 表示需要签字标志，0:不需要签字 1:需要签字
	//							 需要预先创建 Boolean needSignFlag = false;
	// ret : HXEMV_OK          : OK
	public native int iHxEmvGetCvmSignFlag(Boolean needSignFlag);

	//	 终端行为分析
	//	 ret : HXEMV_OK            : OK
	//	       HXEMV_CORE          : 其它错误
	//	       HXEMV_FLOW_ERROR    : EMV流程错误
	//	       HXEMV_DENIAL        : 终端行为分析结果为脱机拒绝
	//	       HXEMV_DENIAL_ADVICE : 终端行为分析结果为脱机拒绝, 有Advice
	public native int iHxEmvTermActionAnalysis();

	//	 第一次GAC
	//	 in  : forcedOnline      : 1:设定强制联机标志 0:不设定强制联机标志
	//	 out : cardAction        : 卡片执行结果, cardAction必须预先创建：Integer cardAction = new Integer(0);
	//								GAC_ACTION_TC     : 批准(生成TC)
	//								GAC_ACTION_AAC    : 拒绝(生成AAC)
	//								GAC_ACTION_AAC_ADVICE : 拒绝(生成AAC,有Advice)
	//								GAC_ACTION_ARQC   : 要求联机(生成ARQC)
	//	 ret : HXEMV_OK          : OK
	//	       HXEMV_CORE        : 其它错误
	//	       HXEMV_CARD_OP     : 卡操作错
	//	       HXEMV_CARD_SW     : 非法状态字
	//	       HXEMV_TERMINATE   : 满足拒绝条件，交易终止
	//	       HXEMV_NOT_ALLOWED : 服务不允许
	//	       HXEMV_NOT_ACCEPTED: 不接受
	//	       HXEMV_FLOW_ERROR  : EMV流程错误
	public native int iHxEmvGac1(int forcedOnline, Integer cardAction);

	//	 第二次GAC
	//	 in  : arc               : 应答码[2+1], NULL或""表示联机失败
	//	       authCode	         : 授权码[6+1], NULL或""表示无授权码数据
	//	       onlineData        : 联机响应数据(55域内容),一组TLV格式数据, NULL表示无联机响应数据
	//	       onlineDataLen     : 联机响应数据长度
	//	 out : cardAction        : 卡片执行结果, cardAction必须预先创建：Integer cardAction = new Integer(0);：
	//         						GAC_ACTION_TC     : 批准(生成TC)
	//	       						GAC_ACTION_AAC    : 拒绝(生成AAC)
	//         						GAC_ACTION_AAC_ADVICE : 拒绝(生成AAC,有Advice)
	//	 ret : HXEMV_OK          : OK
	//	       HXEMV_CORE        : 其它错误
	//	       HXEMV_CARD_OP     : 卡操作错
	//	       HXEMV_CARD_SW     : 非法卡状态字
	//	       HXEMV_TERMINATE   : 满足拒绝条件，交易终止
	//	       HXEMV_NOT_ALLOWED : 服务不允许
	//	       HXEMV_NOT_ACCEPTED: 不接受
	//	       HXEMV_FLOW_ERROR  : EMV流程错误
	public native int iHxEmvGac2 (String arc, String authCode, byte[] issuerData, int issuerDataLen, Integer cardAction);

	/**
	 * 蓝牙数据交换接口
	 * @param dataIn	输入数据缓冲
	 * @param inlength	输入数据长度	
	 * @param apduOut	输出数据缓冲
	 * @return	0		失败
	 * 			其他		成功，返回数据长度
	 */
	public int sendReceiveCallBack(byte[] dataIn, int inlength, byte[] dataOut, int timeOut) {	
	    return BluetoothThread.getInstance().sendReceive(dataIn, inlength, dataOut, timeOut);
	}	
	
	static {
		System.loadLibrary("pbocapi");
	}
}
