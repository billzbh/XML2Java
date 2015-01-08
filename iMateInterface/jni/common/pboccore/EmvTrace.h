/**************************************
File name     : EMVTRACE.H
Function      : 流程跟踪信息，用于debug
Author        : Yu Jun
First edition : Mar 27th, 2012
Modified      : 
**************************************/
#ifndef _EMVTRACE_H
#define _EMVTRACE_H

// 跟踪数据输出设备
#define TRACE_OFF           0   // 关闭跟踪
#define TRACE_COM1          1   // com1
#define TRACE_COM2          2   // com2
#define TRACE_COM3          3   // com3
#define TRACE_COM4          4   // com4
#define TRACE_COM5          5   // com5
#define TRACE_COM6          6   // com6
#define TRACE_COM7          7   // com7
#define TRACE_COM8          8   // com8
#define TRACE_CONSOLE       9   // 标准输出
#define TRACE_AUX           10  // 辅助输出
#define TRACE_PRINTER       11  // 打印机输出
#define TRACE_PTS0          12  // PTS0
#define TRACE_PTS1          13  // PTS0
#define TRACE_PTS2          14  // PTS0
#define TRACE_PTS3          15  // PTS0
#define TRACE_PTS4          16  // PTS0
#define TRACE_PTS5          17  // PTS0
#define TRACE_PTS6          18  // PTS0
#define TRACE_PTS7          19  // PTS0
#define TRACE_CALLBACK		20  // 回调函数

// 跟踪数据类型
#define TRACE_ALWAYS        0
#define TRACE_PROC          1
#define TRACE_APDU          2
#define TRACE_COMM          3
#define TRACE_TAG_DESC      4
#define TRACE_TLV_DB        5
#define TRACE_ERR_MSG       6

// 结构信息类型
#define PARSE_TLV_OBJECT    1  // TLV对象结构
#define PARSE_TLV_GPO80     2  // GPO，Tag80格式返回的结构
#define PARSE_TLV_GAC180    3  // GAC, Tag80格式返回的结构
#define PARSE_DOL           4  // DOL结构
#define PARSE_TLV_DB        5  // TLV对象数据库结构
#define PARSE_FIELD55       6  // 8583报文55域格式
#define INDENT_APDUOUT	    10 // APDUOUT的缩进位置

#define EMV_TRACE_PROC              vTraceWriteTxtCr(TRACE_PROC, "%s Line:%d", pszTraceGetFileBaseName(__FILE__), (int)__LINE__);
#define EMV_TRACE_PROC_RET(iRet)    vTraceWriteTxtCr(TRACE_PROC, "%s Line:%d (%d)", pszTraceGetFileBaseName(__FILE__), (int)__LINE__, (int)iRet);
#define EMV_TRACE_ERR_MSG           vTraceWriteTxtCr(TRACE_ERR_MSG, "%s Line:%d", pszTraceGetFileBaseName(__FILE__), (int)__LINE__);
#define EMV_TRACE_ERR_MSG_RET(iRet) vTraceWriteTxtCr(TRACE_ERR_MSG, "%s Line:%d (%d)", pszTraceGetFileBaseName(__FILE__), (int)__LINE__, (int)iRet);

// 设置跟踪设备
// in  : iTraceDev     : 跟踪设备，TRACE_COM1-TRACE_COM8 | TRACE_CONSOLE | TRACE_PRINTER | TRACE_PTS0-7 | TRACE_CALLBACK
//                       TRACE_OFF表示不进行数据跟踪
//       iTraceApdu    : 跟踪apdu标志，0:不跟踪 1:跟踪
//       iTraceTagDesc : 跟踪tag明细标志，0:不跟踪 1:跟踪
//       iTraceTlvDb   : 跟踪Tlv数据库标志，0:不跟踪 1:跟踪
//       pfvTraceOut   : 回调跟踪函数指针, 跟踪设备为TRACE_CALLBACK时需要
// ret : 0         : OK
//       1         : 不支持跟踪设备
//       2         : 跟踪设备报错
// Note: 由于apdu指令数据及Tag明细数据量比较大，故可设定跟踪或不跟踪标志
int iTraceSet(int iTraceDev, int iTraceApdu, int iTraceTagDesc, int iTraceTlvDb, void (*pfvTraceOut)(char *));

// 取文件基本名
char *pszTraceGetFileBaseName(char *pszFileName);

// 写txt跟踪数据
// in  : iTraceType : TRACE_ALWAYS|TRACE_PROC|TRACE_APDU|TRACE_TAG_DESC|TRACE_ERR_MSG|TRACE_TLV_DB
void vTraceWriteTxt(int iTraceType, char *pszFormat, ...);

// 写txt带回车跟踪数据
// in  : iTraceType : TRACE_ALWAYS|TRACE_PROC|TRACE_APDU|TRACE_TAG_DESC|TRACE_ERR_MSG|TRACE_TLV_DB
void vTraceWriteTxtCr(int iTraceType, char *pszFormat, ...);

// 写bin跟踪数据
// in  : iTraceType : TRACE_ALWAYS|TRACE_PROC|TRACE_APDU|TRACE_TAG_DESC|TRACE_ERR_MSG|TRACE_TLV_DB
void vTraceWriteBin(int iTraceType, char *pszTitle, void *pData, int iLen);

// 写bin带回车跟踪数据
// in  : iTraceType : TRACE_ALWAYS|TRACE_PROC|TRACE_APDU|TRACE_TAG_DESC|TRACE_ERR_MSG|TRACE_TLV_DB
void vTraceWriteBinCr(int iTraceType, char *pszTitle, void *pData, int iLen);

// 写上次apdu跟踪数据
// in  : pszFormat  : 提示信息
void vTraceWriteLastApdu(char *pszFormat, ...);

// 写结构化跟踪数据
// in  : iTraceType : TRACE_ALWAYS|TRACE_PROC|TRACE_APDU|TRACE_TAG_DESC|TRACE_ERR_MSG|TRACE_TLV_DB
//       iInfoType : 信息类型, PARSE_TLV_OBJECT|PARSE_TLV_GPO80|PARSE_TLV_GAC180|PARSE_DOL|PARSE_FIELD55
//       psInfo    : 信息
//       iInfoLen  : 信息长度
//       iIndent   : 缩进位置
void vTraceWriteStruct(int iTraceType, int iInfoType, uchar *psInfo, int iInfoLen, int iIndent);

#endif
