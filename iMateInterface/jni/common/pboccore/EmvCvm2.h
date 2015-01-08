/**************************************
File name     : EmvCvm2.c
Function      : Emv持卡人认证模块(非回调接口)
Author        : Yu Jun
First edition : Apr 25th, 2012
Modified      : Jan 24th, 2013
                    修改为非回调方式接口, 源自EmvCvm.c
**************************************/
#ifndef _EMVCVM2_H
#define _EMVCVM2_H

// CVM2处理初始化, emv交易初始化时调用
void vCvm2Init(void);

// 获取持卡人验证
// out : piCvm               : 持卡人认证方法, 支持HXCVM_PLAIN_PIN、HXCVM_CIPHERED_ONLINE_PIN、HXCVM_HOLDER_ID、HXCVM_CONFIRM_AMOUNT
//       piBypassFlag        : 允许bypass标志, 0:不允许, 1:允许
//       piMsgType           : 提示信息码(如果验证方法为证件验证, 返回证件类型码)
//       piMsgType2          : 附加提示信息, 如果不为0, 先显示该信息
//       pszAmountStr        : 提示用格式化后金额串(HXCVM_HOLDER_ID验证方法不需要)
// ret : HXEMV_OK            : OK
//       HXEMV_NO_DATA       : 无需继续进行持卡人验证
//       HXEMV_CARD_OP       : 卡操作错
//       HXEMV_TERMINATE     : 满足拒绝条件，交易终止
//       HXEMV_DENIAL        : 终端行为分析结果为脱机拒绝
//       HXEMV_DENIAL_ADVICE : 终端行为分析结果为脱机拒绝, 有Advice
//       HXEMV_FLOW_ERROR    : EMV流程错误
//       HXEMV_CORE          : 内部错误
//		 HXEMV_CALLBACK_METHOD : 没有以非回调方式初始化核心
// Note: 如果持卡人验证方法为HXCVM_HOLDER_ID, 则piMsgType&pszAmountStr都不可用, 应用层应自行提取显示组织显示信息
int iCvm2GetMethod(int *piCvm, int *piBypassFlag, int *piMsgType, int *piMsgType2, uchar *pszAmountStr);

// 执行持卡人认证
// in  : psCvmProc           : 认证方法处理方式, HXCVM_PROC_OK or HXCVM_BYPASS or HXCVM_FAIL or HXCVM_CANCEL or HXCVM_TIMEOUT
//       psCvmData           : 输入的密码, 如果为明文密码, 密码尾部要补0
// out : piMsgType           : 返回HXEMV_OK时使用
//                             如果不为0, 则需要显示本类型信息
//       piMsgType2          : 返回HXEMV_OK时使用
//                             如果不为0, 则需要显示本类型信息
// ret : HXEMV_OK            : OK, 需要继续进行持卡人验证, 继续调用iHxGetCvmMethod(), 然后再调用本函数
//       HXEMV_PARA		     : 参数错误
//       HXEMV_CANCEL        : 用户取消
//       HXEMV_TIMEOUT       : 超时
//       HXEMV_CARD_OP       : 卡操作错
//       HXEMV_CARD_SW       : 非法卡状态字
//       HXEMV_TERMINATE     : 满足拒绝条件，交易终止
//       HXEMV_DENIAL        : 终端行为分析结果为脱机拒绝
//       HXEMV_DENIAL_ADVICE : 终端行为分析结果为脱机拒绝, 有Advice
//       HXEMV_FLOW_ERROR    : EMV流程错误
//       HXEMV_CORE          : 内部错误
//		 HXEMV_CALLBACK_METHOD : 没有以非回调方式初始化核心
// Note: 执行的CVM必须是最近一次iHxGetCvmMethod2()获得的CVM
int iCvm2DoMethod(int iCvmProc, uchar *psCvmData, int *piMsgType, int *piMsgType2);

// 获取CVM认证方法需不需要签字
// out : piNeedSignFlag    : 如果返回HXEMV_OK，表示需要签字标志，0:不需要签字 1:需要签字
// ret : HXEMV_OK          : OK
int iCvm2GetSignFlag(int *piNeedSignFlag);

#endif
