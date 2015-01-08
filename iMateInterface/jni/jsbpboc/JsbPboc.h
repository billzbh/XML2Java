/**************************************
File name     : JsbPboc.h
Function      : 江苏银行Pboc卡接口
Author        : Yu Jun
First edition : Apr 15th, 2014
Modified      :
**************************************/
/*******************************************************************************
接口说明:
. 公共参数
  每个函数的前4个参数为公共参数, 它们是char *pszComNo, int nTermType, char cBpNo, int nIcFlag, 在这里统一描述, 以后每个具体函数不再描述.
    pszComNo:	通讯参数, 华信接口不需要
    nTermType:	终端类型, 华信接口不需要
    cBpNo:		BP盒的转口, 华信接口不需要
    nIcFlag:		卡类型, 1:接触IC卡 2:非接触IC卡 3:自动判断
				只有函数ICC_GetIcInfo()和ICC_GetTranDetail()需要
				其它函数会沿用调用ICC_GetIcInfo()函数时确定的卡类型
*******************************************************************************/

#ifndef _JSBPBOC_H
#define _JSBPBOC_H

#ifdef __cplusplus
extern "C" {
#endif

#define JSBPBOC_OK				0   // OK
#define JSBPBOC_CORE_INIT		-1  // 核心初始化错误
#define JSBPBOC_CARD_TYPE		-2  // 不支持的卡片类型
#define JSBPBOC_TRANS_INIT		-3  // 交易初始化错误
#define JSBPBOC_NO_APP			-4  // 无支持的应用
#define JSBPBOC_CARD_IO			-5  // 卡操作错
#define JSBPBOC_TAG_UNKNOWN		-6  // 不识别的标签
#define JSBPBOC_DATA_LOSS       -7  // 必备数据不存在
#define JSBPBOC_PARAM			-8  // 参数错误
#define JSBPBOC_APP_DATA		-9  // 传入的应用数据与传出的应用数据不符
#define JSBPBOC_GEN_ARQC		-10 // 卡片未生成ARQC
#define JSBPBOC_LOG_FORMAT		-11 // 日志格式不支持
#define JSBPBOC_UNKNOWN			-99 // 未知错误

// 初始化函数
int ICC_InitEnv(int (*pfiTestCard)(void),
			    int (*pfiResetCard)(unsigned char *psAtr),
			    int (*pfiDoApdu)(int iApduInLen, unsigned char *psApduIn, int *piApduOutLen, unsigned char *psApduOut),
			    int (*pfiCloseCard)(void));

// 读卡信息
// In  : pszTagList   : 表示需要返回的用户数据的标签列表
// Out : pnUsrInfoLen :	返回的用户数据长度
//       pszUserInfo  : 返回的用户数据, 缓冲区至少保留1024字节
//       pszAppData   :	IC卡数据, 包括Tag8C、Tag8D等, 用于生成55域, 缓冲区至少保留4096字节
//       pnIcType     : 上电成功的卡类型, 1:接触卡 2:非接卡
//						用于函数ICC_GenARQC()和ICC_CtlScriptData()传入
// Ret : 0            : 成功
//       <0           : 失败
int ICC_GetIcInfo(char *pszComNo, int nTermType, char cBpNo, int nIcFlag,
						char *pszTaglist,
						int *pnUsrInfoLen, char *pszUserInfo, char *pszAppData, int *pnIcType);

// 获取Pboc应用版本, 必须在ICC_GetIcInfo之后执行
// Out : pszPbocVer   :	返回的PBOC版本号，"0002"标识PBOC2.0，"0003"标识PBOC3.0
// Ret : 0            : 成功
//       <0           : 失败
int ICC_GetIcPbocVersion(char *pszPbocVer);

// 生成ARQC
// In  : pszTxData    : 交易数据, 标签加长度内容格式
//       pszAppData   : ICC_GetIcInfo函数返回的应用数据, 必须保持与原数据相同
// Out : pnARQCLen    : 返回的ARQC长度
//       pszARQC      : 返回的ARQC, TLV格式, 转换为16进制可读形式
// Ret : 0            : 成功
//       <0           : 失败
int ICC_GenARQC(char *pszComNo, int nTermType, char cBpNo, int nIcFlag,
						char *pszTxData, char *pszAppData, int *pnARQCLen, char *pszARQC);

// 交易结束处理
// In  : pszTxData    : 交易数据, 标签加长度内容格式
//       pszAppData   : ICC_GetIcInfo函数返回的应用数据, 必须保持与原数据相同
//       pszARPC      : 发卡行数据, TLV格式, 转换为16进制可读形式
// Out : pnTCLen      : 返回的TC长度
//       pszTC        : 返回的TC, TLV格式, 转换为16进制可读形式
// Ret : 0            : 成功
//       <0           : 失败
int ICC_CtlScriptData(char *pszComNo, int nTermType, char cBpNo, int nIcFlag,
						char *pszTxData, char *pszAppData, char *pszARPC,
						int *pnTCLen, char *pszTC, char *pszScriptResult);

// 读取交易明细
// Out : pnTxDetailLen:	交易明细长度
//       TxDetail     :	交易明细, 格式为
//                      明细条数(2字节十进制)+每条明细的长度(3字节十进制) + 明细1+明细2+...
// Ret : 0            : 成功
//       <0           : 失败
// Note: 最多10条记录
int ICC_GetTranDetail(char *pszComNo, int nTermType, char BpNo, int nIcFlag,
						int *pnTxDetailLen, char *TxDetail);

#ifdef __cplusplus
}
#endif

#endif
