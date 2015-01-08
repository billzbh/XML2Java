/**************************************
File name     : EmvDAuth.h
Function      : Emv脱机数据认证模块
Author        : Yu Jun
First edition : Apr 19th, 2012
Modified      : 
**************************************/
#ifndef _EMVDAUTH_H
#define _EMVDAUTH_H

#define EMV_DAUTH_OK					0 // OK
#define EMV_DAUTH_DATA_MISSING			1 // 数据缺失
#define EMV_DAUTH_DATA_ILLEGAL			2 // 数据非法
#define EMV_DAUTH_NATIVE				3 // 内部错误
#define EMV_DAUTH_INDEX_NOT_SUPPORT		4 // CA公钥索引不支持
#define EMV_DAUTH_FAIL					5 // 导致脱机数据认证失败的其它错误
#define EMV_DAUTH_CARD_IO				6 // 卡片操作失败
#define EMV_DAUTH_CARD_SW				7 // 非法卡状态字

// 脱机数据认证初始化
// ret : EMV_DAUTH_OK : OK
int iEmvDAuthInit(void);

// 增加一条SDA脱机认证记录数据
// in  : ucRecDataLen          : 记录数据长度
//       psRecData             : 记录数据
// ret : EMV_DAUTH_OK		   : OK
//       EMV_DAUTH_LACK_MEMORY : 预留存储空间不足
int iEmvSDADataAdd(uchar ucRecDataLen, uchar *psRecData);

// 报告SDA记录数据错误, 脱机认证记录不是T70 Tlv对象
// ret : EMV_DAUTH_OK		   : OK
int iEmvSDADataErr(void);

// 判断SDA是否已经失败
// ret : EMV_DAUTH_OK		   : OK
//       EMV_DAUTH_FAIL		   : SDA已经失败
int iEmvSDADataErrTest(void);

// 解析出发卡行公钥
// ret : EMV_DAUTH_OK				 : OK
//       EMV_DAUTH_DATA_MISSING		 : 数据缺失
//       EMV_DAUTH_DATA_ILLEGAL		 : 数据非法
//       EMV_DAUTH_NATIVE			 : 内部错误
//       EMV_DAUTH_INDEX_NOT_SUPPORT : CA公钥索引不支持
//       EMV_DAUTH_FAIL              : 导致脱机数据认证失败的其它错误
int iEmvGetIssuerPubKey(void);

// 解析出IC卡公钥
// ret : EMV_DAUTH_OK				 : OK
//       EMV_DAUTH_DATA_MISSING		 : 数据缺失
//       EMV_DAUTH_DATA_ILLEGAL		 : 数据非法
//       EMV_DAUTH_NATIVE			 : 内部错误
//       EMV_DAUTH_INDEX_NOT_SUPPORT : CA公钥索引不支持
//       EMV_DAUTH_FAIL              : 导致脱机数据认证失败的其它错误
int iEmvGetIcPubKey(void);

// SDA数据认证
// ret : EMV_DAUTH_OK				 : OK
//       EMV_DAUTH_DATA_MISSING		 : 数据缺失
//       EMV_DAUTH_NATIVE			 : 内部错误
//       EMV_DAUTH_FAIL              : 导致脱机数据认证失败的其它错误
int iEmvSda(void);

// DDA数据认证
// ret : EMV_DAUTH_OK				 : OK
//       EMV_DAUTH_DATA_MISSING		 : 数据缺失
//       EMV_DAUTH_NATIVE			 : 内部错误
//       EMV_DAUTH_DATA_ILLEGAL      : 数据非法
//       EMV_DAUTH_FAIL              : 导致脱机数据认证失败的其它错误
//       EMV_DAUTH_CARD_IO			 : 卡片操作失败
//       EMV_DAUTH_CARD_SW			 : 非法卡状态字
int iEmvDda(void);

// CDA数据认证
// in  : ucGacFlag              : 1:第一次GAC 2:第二次GAC
//       psGacDataOut           : GAC指令返回数据
//       ucGacDataOutLen        : GAC指令返回数据长度
// ret : EMV_DAUTH_OK			: OK
//       EMV_DAUTH_DATA_MISSING	: 数据缺失
//       EMV_DAUTH_NATIVE		: 内部错误
//       EMV_DAUTH_FAIL         : 导致脱机数据认证失败的其它错误
int iEmvCda(uchar ucGacFlag, uchar *psGacDataOut, uchar ucGacDataOutLen);

// FDDA数据认证
// ret : EMV_DAUTH_OK				 : OK
//       EMV_DAUTH_DATA_MISSING		 : 数据缺失
//       EMV_DAUTH_NATIVE			 : 内部错误
//       EMV_DAUTH_DATA_ILLEGAL      : 数据非法
//       EMV_DAUTH_FAIL              : 导致脱机数据认证失败的其它错误
// Note: FDDA失败后, 应用层需要自行检查9F6C B1 b6b5, 以决定是拒绝还是进行联机
int iEmvFdda(void);

#endif
