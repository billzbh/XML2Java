/**************************************
 File name     : PbocHigh.h
 Function      : Pboc3.0借贷记/EMV2004客户高层接口
 Author        : Yu Jun
 First edition : Jun 10th, 2014
 **************************************/
#ifndef _PBOCHIGH_H
#define _PBOCHIGH_H

// EMV客户高层接口返回码
#define HXPBOC_HIGH_OK				0  // OK
#define HXPBOC_HIGH_PARA			1  // 参数错误
#define HXPBOC_HIGH_NO_CARD			2  // 无卡
#define HXPBOC_HIGH_NO_APP			3  // 无支持的应用
#define HXPBOC_HIGH_CARD_IO			4  // 卡操作错
#define HXPBOC_HIGH_CARD_SW			5  // 非法卡指令状态字
#define HXPBOC_HIGH_DENIAL			6  // 交易被拒绝
#define HXPBOC_HIGH_TERMINATE		7  // 交易被终止
#define HXPBOC_HIGH_OTHER			8  // 其它错误

#ifdef __cplusplus
extern "C" {
#endif
	
	// 核心初始化
	// in  : pszMerchantId   : 商户号[15]
	//		 pszTermId       : 终端号[8]
	//		 pszMerchantName : 商户名字[40]
	//		 uiCountryCode   : 终端国家代码, 1-999
	//		 uiCurrencyCode  : 交易货币代码, 1-999
	int iHxPbocHighInitCore(char *pszMerchantId, char *pszTermId, char *pszMerchantName, unsigned int uiCountryCode, unsigned int uiCurrencyCode);
	
	// 交易初始化
	// in  : pszDateTime  : 交易日期时间[14], YYYYMMDDhhmmss
	//       ulAtc        : 终端交易流水号, 1-999999
	//       ucTransType  : 交易类型, 0x00 - 0xFF
	//       pszAmount    : 交易金额[12]
	// out : pszField55   : 组装好的55域内容, 十六进制可读格式, 预留513字节长度
	//       pszPan       : 主账号[19], 可读格式
	//       piPanSeqNo   : 主账号序列号, 0-99, -1表示不存在
	//       pszTrack2    : 二磁道等效数据[37], 3x格式, 长度为0表示不存在
	//       pszExtInfo   : 其它数据, 保留
	int iHxPbocHighInitTrans(char *pszDateTime, unsigned long ulAtc, unsigned char ucTransType, unsigned char *pszAmount,
							 char *pszField55, char *pszPan, int *piPanSeqNo, char *pszTrack2, char *pszExtInfo);
	
	// 完成交易
	// in  : pszIssuerData  : 后台数据, 十六进制可读格式
	//       iIssuerDataLen : 后台数据长度
	// out : pszField55     : 组装好的55域内容, 二进制格式, 预留513字节长度
	// Note: 除了返回HXPBOC_HIGH_OK外, 返回HXPBOC_HIGH_DENIAL也会返回脚本结果
	int iHxPbocHighDoTrans(char *pszIssuerData, char *pszField55);
	
	int iHxPbocHighReadInfoEx(char *szOutData,int infoType);

	char *szHxPbocHighGetTagValue(char *szTag);
	
#ifdef __cplusplus
}
#endif

#endif
