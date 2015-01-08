/**************************************
File name     : EmvData.c
Function      : EMV/pboc2.0借贷记核心数据
Author        : Yu Jun
First edition : Mar 29th, 2012
Modified      : 
**************************************/
/*
模块详细描述:
	EMV核心数据库
.	终端参数、支持的所有CA公钥、支持的所有应用都保存在这里
	这些参数需要终端有足够的内存空间, 好在现在的终端内存都不是大问题
.	保存终端与卡片匹配的应用列表
.	保存4种类型的TLV数据库
.	大概所需要的内存空间
			   4096
			   4096
			   2048
	          10240
	 100*18 =  1800
	120*268 = 32160
	100*68  =  6800    +
   ----------------------
			     61K字节 
*/
#include "VposFace.h"
#include "EmvCore.h"

int   gl_iCoreStatus;                       // 核心当前状态

int   gl_iCoreCallFlag;                     // 核心调用方式, HXCORE_CALLBACK      : 指明使用回调接口
											//               HXCORE_NOT_CALLBACK  : 指明使用非回调接口
int   gl_iCardType;							// 当前操作的卡片类型, 核心根据此全局变量来确定操作的卡类型
											//				 EMV_CONTACT_CARD	  : 接触卡
											//				 EMV_CONTACTLESS_CARD : 非接卡
int   gl_iAppType;							// 应用程序标志, 0:送检程序(严格遵守规范) 1:应用程序(实际应用程序)
											//    区别: 1. 送检程序只支持通过L2检测的交易, 应用程序支持所有交易


// 核心Tlv数据库数据
uchar gl_sTlvDbTermFixed[4096];             // 终端参数，一次设置，不再改变
uchar gl_sTlvDbTermVar[4096];               // 终端控制的交易中间数据
uchar gl_sTlvDbIssuer[2048];                // 发卡行数据
uchar gl_sTlvDbCard[10240];                 // 卡片数据

// 终端支持的Aid列表
int   gl_iTermSupportedAidNum;              // 终端当前支持的Aid列表数目
stTermAid gl_aTermSupportedAidList[100];    // 终端支持的Aid列表(占用100*18字节)
// 终端支持的CA公钥列表
int   gl_iTermSupportedCaPublicKeyNum;      // 终端当前支持的CA公钥数目
stCAPublicKey gl_aCAPublicKeyList[120];     // 终端当前支持的CA公钥列表(占用120*268字节)
// 卡片与终端同时支持的应用
int   gl_iCardAppNum;                       // 终端与卡片同时支持的应用数目
stADFInfo gl_aCardAppList[100];             // 终端与卡片同时支持的应用列表(占用100*68字节), cAdfNameLen==0表示列表尾部
