/**************************************
File name     : PbocCtls.c
Function      : 非接卡核心处理模块
Author        : Yu Jun
First edition : May 23rd, 2013
Modified      : 
**************************************/
/*
模块详细描述:
	Pboc标准非接触卡核心处理模块
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "VposFace.h"
#include "Iso4217.h"
#include "Arith.h"
#include "Sha.h"
#include "Common.h"
#include "EmvCmd.h"
#include "TlvFunc.h"
#include "EmvFunc.h"
#include "TagAttr.h"
#include "EmvData.h"
#include "EmvCore.h"
#include "EmvMsg.h"
#include "EmvPara.h"
#include "EmvSele.h"
#include "EmvDAuth.h"
#include "TagDef.h"
#include "EmvTrace.h"

static uchar sg_sCtlsAttr[4];		 // 最近一次非接交易预处理后得出的终端交易属性
static uchar sg_szAmount[13] = {0};	 // 预处理传入的授权金额

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
int iPbocCtlsPreProc(uchar *pszAmount, uint uiCurrencyCode)
{
	uchar *psTlvObjValue;
	int   iRet;
	uchar szBuf[100];

	if(strlen(pszAmount) > 12)
		return(HXEMV_CORE);
	strcpy(sg_szAmount, pszAmount);

	// T9F66格式: B1, b7-支持非接pboc b6-支持qPboc b5-支持接触pboc b4-仅支持脱机 b3-支持联机Pin b2-支持签名
	//            B2, b8-要求联机密文 b7-要求CVM
	//            B3, 预留
	//            B4, b8-支持'01'版本fDDA
	memcpy(sg_sCtlsAttr, "\x7E\x00\x00\x80", 4); // 核心支持的功能表示位(B1、B4), B2要根据条件判断是否置位
	// 1.如果终端配置为支持状态检查，并且授权金额为一个货币单位（这是状态检查要求的），则终
	//   端用终端交易属性字节2中的第8位表示需要联机应用密文。支持状态检查应是一可配置的选
	//   项，在实施时应打开才能操作。这种检查的缺省行为为关闭

	//   构造一个货币单位
	iIso4217SearchDigitCode(uiCurrencyCode, szBuf/*ignore ret value*/, &iRet); // 取小数点位置
	strcpy(szBuf, "1");
	while(iRet--)
		strcat(szBuf, "0");
	if(0/*暂不支持状态检查,如果支持,用检查标志替换*/ && iStrNumCompare(pszAmount, szBuf, 10)==0)
		vSetStrBit2(sg_sCtlsAttr, 1/*B2*/, 8/*b8*/, 1); // 设置要求联机位
	// 2.如果授权金额为零，除非终端支持qPBOC扩展应用，具有联机能力的终端应在终端交易属性字
	//   节2的第8位表示要求联机应用密文
	if(iStrNumCompare(pszAmount, "0", 10)) {
		// 授权金额 == 0, 不支持qPboc扩展
		if(iEmvTermCommunication() != TERM_COM_OFFLINE_ONLY)
			vSetStrBit2(sg_sCtlsAttr, 1/*B2*/, 8/*b8*/, 1); // 有联机能力, 设置要求联机位
	}
	// 3.如果授权金额为零，除非终端支持qPBOC扩展应用，仅支持脱机的终端应终止交易，提示持卡
	//   人使用另一种界面（如果存在）
	if(iStrNumCompare(pszAmount, "0", 10)) {
		// 授权金额 == 0, 不支持qPboc扩展
		if(iEmvTermCommunication() == TERM_COM_OFFLINE_ONLY)
			return(HXEMV_TRY_OTHER_INTERFACE); // 应用层自行决定后续界面
	}
	// 4.如果授权金额大于或等于终端非接触交易限额（如果存在），则终端应提示持卡人采用另一种界面方式
	iRet = iTlvGetObjValue(gl_sTlvDbTermFixed, TAG_DFXX_TermCtlsAmountLimit, &psTlvObjValue);
	if(iRet > 0) {
		// 存在非接触交易限额
		vOneTwo0(psTlvObjValue, iRet, szBuf);
		if(iStrNumCompare(pszAmount, szBuf, 10) >= 0)
			return(HXEMV_TRY_OTHER_INTERFACE); // 应用层自行决定后续界面
	}
	// 5.如果授权金额大于或等于终端执行CVM限额（如果存在），则终端应在终端交易属性中表示
	//   要求CVM（第2字节第7位）以及支持的CVM种类。本部分当前版本支持联机PIN（第1
	//   字节第3位）和签名（第1字节第2位）
	iRet = iTlvGetObjValue(gl_sTlvDbTermFixed, TAG_DFXX_TermCtlsCvmLimit, &psTlvObjValue);
	if(iRet > 0) {
		// 存在非接触交易限额
		vOneTwo0(psTlvObjValue, iRet, szBuf);
		if(iStrNumCompare(pszAmount, szBuf, 10) >= 0)
			vSetStrBit2(sg_sCtlsAttr, 1/*B2*/, 7/*b7*/, 1); // 要求CVM
	}
	// 6.如果授权金额（标签“9F02”）大于非接触终端脱机最低限额或（如果非接触终端脱机最低
	//   限额不存在）可用的终端最低限额（标签“9F1B”），则终端应在终端交易属性第2字节第8
	//   位表示需要联机应用密文
	iRet = iTlvGetObjValue(gl_sTlvDbTermFixed, TAG_DFXX_TermCtlsOfflineAmountLimit, &psTlvObjValue);
	if(iRet <= 0) {
		// 无非接脱机最低限额, 尝试终端限额
		iRet = iTlvGetObjValue(gl_sTlvDbTermFixed, TAG_9F1B_TermFloorLimit, &psTlvObjValue);
		if(iRet <= 0) {
			// 注:按规范,Floor Limit为aid相关参数, 此时拿不到, 因此非接脱机最低限额就必须存在, 否则终止非接交易
			return(HXEMV_TRY_OTHER_INTERFACE); // 应用层自行决定后续界面
		}
	}
	// 存在非接脱机交易限额(或者是终端限额)
	vOneTwo0(psTlvObjValue, 6, szBuf);
	if(iStrNumCompare(pszAmount, szBuf, 10) > 0)
		vSetStrBit2(sg_sCtlsAttr, 1/*B2*/, 8/*b7*/, 1); // 要求联机

	// 获取终端非接能力屏蔽位, 产生非接终端交易属性
	iRet = iTlvGetObjValue(gl_sTlvDbTermFixed, TAG_DFXX_TermCtlsCapMask, &psTlvObjValue);
	if(iRet != 4)
		return(HXEMV_CORE); // 非接能力屏蔽位必须存在
	vAnd(sg_sCtlsAttr, psTlvObjValue, 4);
	return(HXEMV_OK);
}

// 获取终端交易属性
// ret : 9F66值, 获取前需先做预处理
uchar *psPbocCtlsGet9F66(void)
{
	return(sg_sCtlsAttr);
}

// 获取预处理传入的金额
// ret : 预处理传入的金额
uchar *pszPbocCtlsGetAmount(void)
{
	return(sg_szAmount);
}

// 获取交易路径
// ret : 交易走的路径, 0:接触Pboc(标准DC或EC) 1:非接触Pboc(标准DC或EC) 2:qPboc联机 3:qPboc脱机 4:qPboc拒绝
// Note : 非接GPO分析后获取此值才有意义
int iPbocCtlsGetRoute(void)
{
	int   iRet;
	uchar *p;
	iRet = iTlvGetObjValue(gl_sTlvDbTermVar, TAG_DFXX_TransRoute, &p);
	if(iRet <= 0)
		return(0); // 无此数据, 假设走的是接触pboc
	return(*p);
}

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
int iPbocCtlsGpoAnalyse(int *piTransRoute, int *piSignFlag, int *piNeedOnlinePin)
{
	uchar *ps9F6CValue;
	int   i9F6CLen;

	*piSignFlag = 0;
	*piNeedOnlinePin = 0;

	*piTransRoute = iPbocCtlsGetRoute(); // 0:接触Pboc(标准DC或EC) 1:非接触Pboc(标准DC或EC) 2:qPboc联机 3:qPboc脱机 4:qPboc拒绝
	if(*piTransRoute==0 || *piTransRoute==1)
		return(HXEMV_OK); // pboc走pboc流程

	i9F6CLen = iTlvGetObjValue(gl_sTlvDbCard, TAG_9F6C_CardTransQualifiers, &ps9F6CValue);

	if(*piTransRoute == 2) {
		// qPboc联机, 参考JR/T0025.12—2013, 7.8.3 P41
		if(i9F6CLen == 2) {
			// 成功读出了9F6C
			if(iTestStrBit2(ps9F6CValue, 0, 8))
				*piNeedOnlinePin = 1; // 需要联机PIN
			if(iTestStrBit2(ps9F6CValue, 0, 7))
				*piSignFlag = 1; // 需要签名
		} else {
			// 没有成功读出9F6C
			if(iEmvTestItemBit(EMV_ITEM_TERM_CAP, TERM_CAP_10_SIGNATURE))
				*piSignFlag = 1; // 需要签名, refer JR/T0025.12—2013, 7.6 p21
		}
		return(HXEMV_OK); // 走qPboc联机流程
	}

	if(*piTransRoute == 3) {
		// qPboc脱机, 参考JR/T0025.12—2013, 7.8.2 P41
		if(i9F6CLen == 2) {
			// 成功读出了9F6C
			if(iTestStrBit2(ps9F6CValue, 0, 7))
				*piSignFlag = 1; // 需要签名
		} else {
			// 没有成功读出9F6C
			if(iEmvTestItemBit(EMV_ITEM_TERM_CAP, TERM_CAP_10_SIGNATURE))
				*piSignFlag = 1; // 需要签名, refer JR/T0025.12—2013, 7.6 p21
		}
		return(HXEMV_OK); // 走qPboc联机流程
	}

	if(*piTransRoute == 4) {
		return(HXEMV_OK); // 拒绝
	}

	ASSERT(0);
	return(HXEMV_CORE);
}
