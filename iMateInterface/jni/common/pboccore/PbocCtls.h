/**************************************
File name     : PbocCtls.h
Function      : 非接卡核心处理模块
Author        : Yu Jun
First edition : May 23rd, 2013
Modified      : 
**************************************/
#ifndef _PBOCCTLS_H
#define _PBOCCTLS_H

// 非接触卡交易预处理
// in  : pszAmount					: 交易金额, ascii码串, 不要小数点, 默认为缺省小数点位置, 例如:RMB1.00表示为"100"
//       uiCurrencyCode				: 货币代码
// ret : HXEMV_OK					: OK
//		 HXEMV_TRY_OTHER_INTERFACE	: 满足拒绝条件，交易终止, 尝试其它通信界面
//									  应显示EMVMSG_TERMINATE信息, 如果终端支持其它通信界面, 提示用户尝试其它通信界面
//       HXEMV_CORE					: 内部错误
// Note: 每笔非接触交易前必须先做预处理, 只有成功完成了预处理才可以进行非接交易
//       应用层要确保交易数据与预处理数据一致
//       预处理结果为终端交易属性, 保存在sg_sCtlsAttr[4]
//       参考: JR/T0025.12—2013, 6.2, p9
int iPbocCtlsPreProc(uchar *pszAmount, uint uiCurrencyCode);

// 获取终端交易属性
// ret : 9F66值, 获取前需先做预处理
uchar *psPbocCtlsGet9F66(void);

// 获取预处理传入的金额
// ret : 预处理传入的金额
uchar *pszPbocCtlsGetAmount(void);

// 获取交易路径
// ret : 交易走的路径, 0:接触Pboc(标准DC或EC) 1:非接触Pboc(标准DC或EC) 2:qPboc联机 3:qPboc脱机 4:qPboc拒绝
// Note : 非接GPO分析后获取此值才有意义
int iPbocCtlsGetRoute(void);

// 非接卡GPO数据分析
// out : piTransRoute               : 交易走的路径 0:接触Pboc(标准DC或EC) 1:非接触Pboc(标准DC或EC) 2:qPboc联机 3:qPboc脱机 4:qPboc拒绝
//       piSignFlag					: 需要签字标志(0:不需要签字, 1:需要签字), 仅当走的是qPboc流程时才有效
//       piNeedOnlinePin            : 需要联机密码标志(0:不需要联机密码, 1:需要联机密码), 仅当走的是qPboc联机流程时才有效
// ret : HXEMV_OK					: OK, 根据piTransRoute决定后续流程
//       HXEMV_TERMINATE			: 满足拒绝条件，交易终止
//		 HXEMV_TRY_OTHER_INTERFACE	: 尝试其它通信界面
//       HXEMV_CORE					: 内部错误
// Note: GPO成功后才可以调用
// 参考 JR/T0025.12—2013, 7.8 P40
int iPbocCtlsGpoAnalyse(int *piTransRoute, int *piSignFlag, int *piNeedOnlinePin);

#endif
