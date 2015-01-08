/**************************************
File name     : EmvCvm.h
Function      : Emv持卡人验证模块
Author        : Yu Jun
First edition : Apr 25th, 2012
Modified      : 
**************************************/
#ifndef _EMVCVM_H
#define _EMVCVM_H

// 持卡人验证
// out : piNeedSignFlag    : 如果成功完成，返回需要签字标志，0:不需要签字 1:需要签字
// ret : HXEMV_OK          : OK
//       HXEMV_CANCEL      : 用户取消
//       HXEMV_TIMEOUT     : 超时
//       HXEMV_CARD_OP     : 卡操作错
//       HXEMV_CARD_SW     : 非法卡状态字
//       HXEMV_TERMINATE   : 满足拒绝条件，交易终止
//       HXEMV_CORE        : 内部错误
// Note: refer to Emv2008 book3 10.5, P100
//                Emv2008 book4 6.3.4, P45
int iEmvCardHolderVerify(int *piNeedSignFlag);

// 输入联机密码
// ret : 0              : OK
//       HXEMV_NA       : bypass
//       HXEMV_CANCEL   : 用户取消
//       HXEMV_TIMEOUT  : 超时
//       HXEMV_CORE     : 内部错误
// Note: refer to Emv2008 book3, figure10, P107, 流程图U
int iCvmEnterOnlinePin(void);

// 检查持卡人验证方法条件是否满足
// in  : psCvRule        : 持卡人验证方法
//       psCvmList       : CVM List, 用于提取X、Y金额
// ret : 0               : 满足
//       HXEMV_NA        : 不满足
//       HXEMV_CORE      : 内部错误
// Note: EmvCvm2模块也需要此接口
int iCvmCheckCondition(uchar *psCvRule, uchar *psCvmList);

#endif
