/**************************************
File name     : EmvData.h
Function      : EMV/pboc2.0借贷记核心数据
Author        : Yu Jun
First edition : Mar 29th, 2012
Modified      : Apr 2nd, 2014
				增加EMV_STATUS_PROC_RISTRICTIONS宏定义
**************************************/
#ifndef _EMVDATA_H
#define _EMVDATA_H
#include "EmvCore.h"

// 核心状态定义
#define EMV_STATUS_INIT						     0  // 核心初始化完成
#define EMV_STATUS_SET_PARA					    10  // 终端参数设置完成
#define EMV_STATUS_TRANS_INIT				    20  // 交易初始化完成
#define EMV_STATUS_RESET						30  // 卡片复位完成
#define EMV_STATUS_GET_SUPPORTED_APP            40  // 获取支持的应用完成
#define EMV_STATUS_APP_SELECT				    50  // 应用选择完成
#define EMV_STATUS_GPO						    60  // GPO完成
#define EMV_STATUS_READ_REC					    70  // 读记录数据完成
#define EMV_STATUS_PROC_RISTRICTIONS			75  // 处理限制完成
#define EMV_STATUS_TERM_ACTION_ANALYSIS			80  // 终端风险管理完成
#define EMV_STATUS_GAC1							90  // GAC1完成
#define EMV_STATUS_GAC2							100 // GAC2完成

#define EMV_CONTACT_CARD						0	// 接触卡类型
#define EMV_CONTACTLESS_CARD					1	// 非接卡类型

extern int   gl_iCoreStatus;                        // 核心当前状态
extern int   gl_iCoreCallFlag;						// 核心调用方式, HXCORE_CALLBACK or HXCORE_NOT_CALLBACK
extern int   gl_iCardType;							// 当前操作的卡片类型, EMV_CONTACT_CARD	or EMV_CONTACTLESS_CARD
extern int   gl_iAppType;							// 应用程序标志, 0:送检程序(严格遵守规范) 1:应用程序(实际应用程序)
													//    区别: 1. 送检程序只支持通过L2检测的交易, 应用程序支持所有交易

// 核心Tlv数据库数据
extern uchar gl_sTlvDbTermFixed[4096];              // 终端参数，一次设置，不再改变
extern uchar gl_sTlvDbTermVar[4096];                // 终端控制的交易中间数据
extern uchar gl_sTlvDbIssuer[2048];                 // 发卡行数据
extern uchar gl_sTlvDbCard[10240];                  // 卡片数据

// 终端支持的Aid列表
extern int   gl_iTermSupportedAidNum;               // 终端当前支持的Aid列表数目
extern stTermAid gl_aTermSupportedAidList[100];     // 终端支持的Aid列表(占用100*18字节)
// 终端支持的CA公钥列表
extern int   gl_iTermSupportedCaPublicKeyNum;       // 终端当前支持的CA公钥数目
extern stCAPublicKey gl_aCAPublicKeyList[120];      // 终端当前支持的CA公钥列表(占用120*268字节)
// 卡片与终端同时支持的应用
extern int   gl_iCardAppNum;                        // 终端与卡片同时支持的应用数目
extern stADFInfo gl_aCardAppList[100];              // 终端与卡片同时支持的应用列表(占用100*68字节)

#endif
