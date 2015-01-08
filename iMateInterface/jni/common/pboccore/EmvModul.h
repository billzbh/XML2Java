/**************************************
File name     : EmvModul.h
Function      : Emv核心处理模块
Author        : Yu Jun
First edition : Apr 18th, 2012
Modified      : 
**************************************/
#ifndef _EMVMODUL_H
#define _EMVMODUL_H

// GPO初始化
// ret : HXEMV_OK		: 成功
int iEmvGpoInit(void);

// GPO
// ret : HXEMV_OK		: 成功
//       HXEMV_CANCEL   : 用户取消
//       HXEMV_TIMEOUT  : 用户超时
//       HXEMV_CORE     : 其它错误
//       HXEMV_RESELECT : 需要重新选择应用
//       HXEMV_CARD_OP  : 卡操作错
//       HXEMV_CARD_SW  : 非法卡状态字
//       HXEMV_TERMINATE: 满足拒绝条件，交易终止
// Note: 非接卡返回HXEMV_OK时, 有可能交易路径指示交易被拒绝
int iEmvGpo(void);

// 读应用记录
// ret : HXEMV_OK          : OK
//       HXEMV_CARD_OP     : 卡操作错
//       HXEMV_TERMINATE   : 满足拒绝条件，交易终止
//       HXEMV_CORE        : 其它错误
int iEmvReadRecord(void);

// SDA或DDA
// in  : iInCrlFlag          : 发卡行证书在CRL列表之中, 1:发卡行证书在CRL列表 0:发卡行证书是否在CRL列表中未知
//                             此参数专门为非回调方式使用, 传入1时设置脱机数据认证失败位
// ret : HXEMV_OK            : OK
//       HXEMV_CARD_OP       : 卡操作错
//       HXEMV_CARD_SW       : 非法卡状态字
//       HXEMV_TERMINATE     : 满足拒绝条件，交易终止
//       HXEMV_DENIAL        : 终端行为分析结果为脱机拒绝
//       HXEMV_DENIAL_ADVICE : 终端行为分析结果为脱机拒绝, 有Advice
//       HXEMV_FLOW_ERROR    : EMV流程错误
//       HXEMV_CORE          : 内部错误
//		 HXEMV_FDDA_FAIL	 : fDDA失败(非接)
int iEmvOfflineDataAuth(int iInCrlFlag);

// 处理限制
// ret : HXEMV_OK            : OK
//       HXEMV_CORE          : 其它错误
//       HXEMV_DENIAL        : 终端行为分析结果为脱机拒绝
//       HXEMV_DENIAL_ADVICE : 终端行为分析结果为脱机拒绝, 有Advice
//       HXEMV_CARD_OP       : 终端行为分析结果为脱机拒绝, 申请AAC时卡片出错
//       HXEMV_TERMINATE     : 满足拒绝条件，交易终止
int iEmvProcRistrictions(void);

// 终端风险管理
// ret : HXEMV_OK            : OK
//       HXEMV_CORE          : 其它错误
//       HXEMV_CARD_OP       : 卡操作错
//       HXEMV_TERMINATE     : 满足拒绝条件，交易终止
//       HXEMV_DENIAL        : 终端行为分析结果为脱机拒绝
//       HXEMV_DENIAL_ADVICE : 终端行为分析结果为脱机拒绝, 有Advice
//       HXEMV_CANCEL        : 被取消
//       HXEMV_TIMEOUT       : 超时
// Note: 不关心AIP设置, 强制执行
int iEmvTermRiskManage(void);

// 持卡人验证
// out : piNeedSignFlag      : 如果成功完成，返回需要签字标志，0:不需要签字 1:需要签字
// ret : HXEMV_OK            : OK
//       HXEMV_CANCEL        : 用户取消
//       HXEMV_TIMEOUT       : 超时
//       HXEMV_CARD_OP       : 卡操作错
//       HXEMV_TERMINATE     : 满足拒绝条件，交易终止
//       HXEMV_DENIAL        : 终端行为分析结果为脱机拒绝
//       HXEMV_DENIAL_ADVICE : 终端行为分析结果为脱机拒绝, 有Advice
//       HXEMV_FLOW_ERROR    : EMV流程错误
//       HXEMV_CORE          : 内部错误
int iEmvModuleCardHolderVerify(int *piNeedSignFlag);

// 判断是否需要输入EC需要的联机密码
// ret : HXEMV_OK		   : 需要
//       HXEMV_NA          : 不需要
int iEmvIfNeedECOnlinePin(void);

// 判断是否需要拒绝交易
// ret : HXEMV_OK		     : 不需要
//       HXEMV_CARD_OP       : 卡操作错
//       HXEMV_TERMINATE     : 满足拒绝条件，交易终止
//       HXEMV_DENIAL        : 终端行为分析结果为脱机拒绝
//       HXEMV_DENIAL_ADVICE : 终端行为分析结果为脱机拒绝, 有Advice
//       HXEMV_CORE          : 内部错误
int iEmvIfDenial(void);

// 终端行为分析
// ret : HXEMV_OK		     : OK
//       HXEMV_CORE          : 其它错误
//       HXEMV_DENIAL        : 终端行为分析结果为脱机拒绝
//       HXEMV_DENIAL_ADVICE : 终端行为分析结果为脱机拒绝, 有Advice
//       HXEMV_CARD_OP       : 终端行为分析结果为脱机拒绝, 申请AAC时卡片出错
//       HXEMV_TERMINATE     : 满足拒绝条件，交易终止
//       HXEMV_CANCEL        : 用户取消
//       HXEMV_TIMEOUT       : 超时
// Note: 该函数可在iHxReadRecord()后iHxGAC1()之前任意调用, 判断为脱机拒绝后, 会自动GAC申请AAC, 此时高层可结束EMV流程
int iEmvTermActionAnalysis(void);

// 第一次GAC
// out : piCardAction      : 卡片执行结果
//                           GAC_ACTION_TC     : 批准(生成TC)
//							 GAC_ACTION_AAC    : 拒绝(生成AAC)
//                           GAC_ACTION_AAC_ADVICE : 拒绝(生成AAC,有Advice)
//                           GAC_ACTION_ARQC   : 要求联机(生成ARQC)
// ret : HXEMV_OK          : OK
//       HXEMV_CORE        : 其它错误
//       HXEMV_CARD_OP     : 卡操作错
//       HXEMV_CARD_SW     : 非法卡状态字
//       HXEMV_TERMINATE   : 满足拒绝条件，交易终止
//       HXEMV_NOT_ALLOWED : 服务不允许
// Note: 某些情况下, 该函数会自动调用iEmvGAC2()
//       如:CDA失败、仅脱机终端GAC1卡片返回ARQC...
int iEmvGAC1(int *piCardAction);

// 第二次GAC
// in  : pszArc            : 应答码[2], NULL或""表示联机失败
//       pszAuthCode	   : 授权码[6], NULL或""表示无授权码数据
//       psIssuerData      : 联机响应数据(55域内容),一组TLV格式数据, NULL表示无联机响应数据
//       iIssuerDataLen    : 联机响应数据长度
// out : piCardAction      : 卡片执行结果
//                           GAC_ACTION_TC     : 批准(生成TC)
//							 GAC_ACTION_AAC    : 拒绝(生成AAC)
//                           GAC_ACTION_AAC_ADVICE : 拒绝(生成AAC,有Advice)
//                           GAC_ACTION_ARQC   : 要求联机(生成ARQC)
// ret : HXEMV_OK          : OK
//       HXEMV_CORE        : 其它错误
//       HXEMV_CARD_OP     : 卡操作错
//       HXEMV_CARD_SW     : 非法卡状态字
//       HXEMV_TERMINATE   : 满足拒绝条件，交易终止
//       HXEMV_NOT_ALLOWED : 服务不允许
int iEmvGAC2(uchar *pszArc, uchar *pszAuthCode, uchar *psIssuerData, int iIssuerDataLen, int *piCardAction);

#endif
