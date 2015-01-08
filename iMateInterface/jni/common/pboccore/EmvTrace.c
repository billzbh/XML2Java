/**************************************
File name     : EMVTRACE.C
Function      : 流程跟踪信息，用于debug
Author        : Yu Jun
First edition : Mar 27th, 2012
Modified      : 
**************************************/
/*
模块详细描述:
    提供给核心及高层应用写流程日志服务
.   使用本模块前必须先调用iTraceSet()设置参数, 该接口通知本模块使用什么设备输出日志及输出哪些日志
.   vTraceWrite...()接口传入的iTraceType表明该函数传入的数据跟踪类型, 只有iTraceSet()设定的日志才会被输出
    通讯信息、流程信息、错误信息会被强制输出
.	vTraceWriteStruct()可对特定跟踪日志进行格式化
	应用层一般没必要使用该接口
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include "VposFace.h"
#include "TagAttr.h"
#include "TlvFunc.h"
#include "EmvCmd.h"
#include "EmvTrace.h"

#define TRACE_MSG_CRLF  "\x0d\x0a"  // 换行串

static int sg_iTraceDev;        // 跟踪设备
static int sg_iTraceApdu;       // 跟踪apdu指令标志，0:不跟踪 1:跟踪
static int sg_iTraceComm;       // 跟踪通讯信息
static int sg_iTraceTagDesc;    // 跟踪tag明细标志，0:不跟踪 1:跟踪
static int sg_iTraceTlvDb;      // 跟踪TLV数据库标志，0:不跟踪 1:跟踪
static int sg_iTraceProc;       // 跟踪流程信息，0:不跟踪 1:跟踪
static int sg_iTraceErrMsg;     // 跟踪错误信息，0:不跟踪 1:跟踪

static void (*sg_pfvTraceCallBack)(char *pOut) = NULL; // Trace回调函数

// 输出跟踪信息到跟踪设备
// in  : iTraceType   : 跟踪信息类型
//       pszTraceInfo : 跟踪信息
static void vTraceOut(int iTraceType, uchar *pszTraceInfo)
{
    uchar ucOldPort, ucNewPort;
		
    // 如果没有设定跟踪该类型，直接返回
    switch(iTraceType) {
    case TRACE_ALWAYS:
        break;
    case TRACE_PROC:
        if(sg_iTraceProc == 0)
            return;
        break;
    case TRACE_APDU:
        if(sg_iTraceApdu == 0)
            return;
        break;
    case TRACE_COMM:
        if(sg_iTraceComm == 0)
            return;
        break;
    case TRACE_TAG_DESC:
        if(sg_iTraceTagDesc == 0)
            return;
        break;
    case TRACE_TLV_DB:
        if(sg_iTraceTlvDb == 0)
            return;
        break;
    case TRACE_ERR_MSG:
        if(sg_iTraceErrMsg == 0)
            return;
        break;
    default:
        return;
    }
    
    // 输出跟踪信息到特定的设备
    switch(sg_iTraceDev) {
    case TRACE_COM1:
    case TRACE_COM2:
    case TRACE_COM3:
    case TRACE_COM4:
    case TRACE_COM5:
    case TRACE_COM6:
    case TRACE_COM7:
    case TRACE_COM8:
        ucNewPort = sg_iTraceDev-TRACE_COM1+1;
        ucOldPort = _ucAsyncSetPort(ucNewPort);
        _ucAsyncSendBuf(pszTraceInfo, strlen(pszTraceInfo));
        _ucAsyncSetPort(ucOldPort); // 恢复端口
        break;
    case TRACE_PRINTER:
        _uiPrint(pszTraceInfo);
        break;
	case TRACE_CONSOLE:
		printf(pszTraceInfo);
		break;
	case TRACE_PTS0:
	case TRACE_PTS1:
	case TRACE_PTS2:
	case TRACE_PTS3:
	case TRACE_PTS4:
	case TRACE_PTS5:
	case TRACE_PTS6:
	case TRACE_PTS7:
#if((POS_TYPE&POS_TYPE_MASK)==POS_TYPE_YIN5 || POS_TYPE==POS_TYPE_LINUX)
		{
            uchar szBuf[20];
			FILE *p;
            sprintf(szBuf, "/dev/pts/%d", sg_iTraceDev-TRACE_PTS0);
			p = fopen(szBuf, "w");
			if(p) {
				fprintf(p, pszTraceInfo);
				fclose(p);
			}
		}
#endif
		break;
	case TRACE_CALLBACK:
		if(sg_pfvTraceCallBack) {
			sg_pfvTraceCallBack(pszTraceInfo);
		}
		break;
    default:
		break;
    }
}

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
int iTraceSet(int iTraceDev, int iTraceApdu, int iTraceTagDesc, int iTraceTlvDb, void (*pfvTraceOut)(char *))
{
    uchar ucOldPort, ucNewPort;
    uchar ucRet;

    sg_iTraceApdu = iTraceApdu;
	sg_iTraceComm = 1;   // 强制跟踪通讯信息
    sg_iTraceTagDesc = iTraceTagDesc;
    sg_iTraceTlvDb = iTraceTlvDb;
    sg_iTraceProc = 1;   // 强制跟踪流程信息
    sg_iTraceErrMsg = 1; // 强制跟踪错误信息
    
    switch(iTraceDev) {
    case TRACE_OFF:
        sg_iTraceDev = TRACE_OFF;
        break;
    case TRACE_COM1:
    case TRACE_COM2:
    case TRACE_COM3:
    case TRACE_COM4:
    case TRACE_COM5:
    case TRACE_COM6:
    case TRACE_COM7:
    case TRACE_COM8:
        ucNewPort = iTraceDev-TRACE_COM1+1;
        ucOldPort = _ucAsyncSetPort(ucNewPort);
        _ucAsyncClose();
        ucRet = _ucAsyncOpen(57600L, COMM_NONE, 8, 1);
        _ucAsyncSetPort(ucOldPort); // 恢复端口
        if(ucRet) {
            sg_iTraceDev = TRACE_OFF;
            return(2); // 设备报错
        }
        sg_iTraceDev = iTraceDev;
        break;
    case TRACE_CONSOLE:
#if((POS_TYPE&POS_TYPE_MASK)==POS_TYPE_YIN5 || POS_TYPE==POS_TYPE_LINUX)
		sg_iTraceDev = TRACE_CONSOLE;
		break;
#else
        sg_iTraceDev = TRACE_OFF;
        return(1); // 不支持
#endif
    case TRACE_AUX:
        sg_iTraceDev = TRACE_OFF;
        return(1); // 不支持
    case TRACE_PRINTER:
        if(_uiGetPCols() == 0) {
            sg_iTraceDev = TRACE_OFF;
            return(2); // 设备报错
        }
        sg_iTraceDev = TRACE_PRINTER;
        break;
	case TRACE_PTS0:
	case TRACE_PTS1:
	case TRACE_PTS2:
	case TRACE_PTS3:
	case TRACE_PTS4:
	case TRACE_PTS5:
	case TRACE_PTS6:
	case TRACE_PTS7:
#if((POS_TYPE&POS_TYPE_MASK)==POS_TYPE_YIN5 || POS_TYPE==POS_TYPE_LINUX)
        sg_iTraceDev = iTraceDev;
		break;
#else
        sg_iTraceDev = TRACE_OFF;
        return(1); // 不支持
#endif
		break;
	case TRACE_CALLBACK:
		sg_pfvTraceCallBack = pfvTraceOut;
        sg_iTraceDev = iTraceDev;
		break;
    default:
        sg_iTraceDev = TRACE_OFF;
        return(1); // 不支持
    }
    return(0);
}

// 取文件基本名
char *pszTraceGetFileBaseName(char *pszFileName)
{
    int i;
    int iLen;

    iLen = strlen((char*)pszFileName);
    for(i=iLen-1; i>=0; i--)
        if(pszFileName[i]=='\\' || pszFileName[i]=='/')
            break;
    i ++;
	return(pszFileName+i);
}

// 写txt跟踪数据
// in  : iTraceType : TRACE_ALWAYS|TRACE_PROC|TRACE_APDU|TRACE_TAG_DESC|TRACE_ERR_MSG|TRACE_TLV_DB
void vTraceWriteTxt(int iTraceType, char *pszFormat, ...)
{
    va_list args;
    char    szMsg[2048];
    
    va_start(args, pszFormat);
    vsprintf(szMsg, pszFormat, args);
    vTraceOut(iTraceType, szMsg);
}

// 写txt带回车跟踪数据
// in  : iTraceType : TRACE_ALWAYS|TRACE_PROC|TRACE_APDU|TRACE_TAG_DESC|TRACE_ERR_MSG|TRACE_TLV_DB
void vTraceWriteTxtCr(int iTraceType, char *pszFormat, ...)
{
    va_list args;
    char    szMsg[2048];
    
    va_start(args, pszFormat);
    vsprintf(szMsg, pszFormat, args);
    strcat(szMsg, TRACE_MSG_CRLF);
    vTraceOut(iTraceType, szMsg);
}

// 写bin跟踪数据
// in  : iTraceType : TRACE_ALWAYS|TRACE_PROC|TRACE_APDU|TRACE_TAG_DESC|TRACE_ERR_MSG|TRACE_TLV_DB
void vTraceWriteBin(int iTraceType, char *pszTitle, void *pData, int iLen)
{
	uchar szMsg[2048];
	if(iLen > 1000)
		iLen = 1000;
	if(pszTitle)
		strcpy(szMsg, pszTitle);
	else
		szMsg[0] = 0;
	vOneTwo0((uchar *)pData, iLen, szMsg+strlen(szMsg));
	vTraceOut(iTraceType, szMsg);
}

// 写bin带回车跟踪数据
// in  : iTraceType : TRACE_ALWAYS|TRACE_PROC|TRACE_APDU|TRACE_TAG_DESC|TRACE_ERR_MSG|TRACE_TLV_DB
void vTraceWriteBinCr(int iTraceType, char *pszTitle, void *pData, int iLen)
{
	uchar szMsg[2048];
	if(iLen > 1000)
		iLen = 1000;
	if(pszTitle)
		strcpy(szMsg, pszTitle);
	else
		szMsg[0] = 0;
	vOneTwo0((uchar *)pData, iLen, szMsg+strlen(szMsg));
    strcat(szMsg, TRACE_MSG_CRLF);
	vTraceOut(iTraceType, szMsg);
}

// 写上次apdu跟踪数据
// in  : pszFormat  : 提示信息
void vTraceWriteLastApdu(char *pszFormat, ...)
{
	uchar szMsg[1024];
    va_list args;

	if(pszFormat) {
	    va_start(args, pszFormat);
        vsprintf(szMsg, pszFormat, args);
        vTraceWriteTxtCr(TRACE_APDU, "%s", szMsg);
	}
	if(gl_uiLastApduInLen == 0) {
		// 无数据
		vTraceWriteTxtCr(TRACE_APDU, "ApduIn :");
	    vTraceWriteTxtCr(TRACE_APDU, "ApduOut:");
		return;
	}
    if(gl_sLastApduIn[0]&0x01) {
		gl_sLastApduIn[0] &= 0xFE;
		vOneTwo0(gl_sLastApduIn, gl_uiLastApduInLen, szMsg);
        vTraceWriteTxt(TRACE_APDU, "ApduIn*:"); // 带*号表示该指令为批处理推送指令
	} else {
		vOneTwo0(gl_sLastApduIn, gl_uiLastApduInLen, szMsg);
        vTraceWriteTxt(TRACE_APDU, "ApduIn :");
	}
	vTraceWriteTxtCr(TRACE_APDU, szMsg);

	if(gl_uiLastApduOutLen == 0) {
	    vTraceWriteTxtCr(TRACE_APDU, "ApduOut:Error");
		return;
	}
	vOneTwo0(gl_sLastApduOut, gl_uiLastApduOutLen, szMsg);
    vTraceWriteTxtCr(TRACE_APDU, "ApduOut:%s", szMsg);
}

// 增加额外的信息
// in  : pszMsg   : 原跟踪数据
//       psTlvObj : Tlv对象
// out : pszMsg   : 处理后跟踪数据
static void vAddExtraInfo(uchar *pszMsg, uchar *psTlvObj)
{
	if(memcmp(psTlvObj, "\x50", 1)==0 || memcmp(psTlvObj, "\x5F\x20", 2)==0 ||
		        memcmp(psTlvObj, "\x5F\x2D", 2)==0 || memcmp(psTlvObj, "\x9F\x16", 2)==0 ||
				memcmp(psTlvObj, "\x9F\x1C", 2)==0 || memcmp(psTlvObj, "\x9F\x4E", 2)==0 ||
				memcmp(psTlvObj, "\x9F\x61", 2)==0) {
        *pszMsg++ = '[';
		memcpy(pszMsg, psTlvValue(psTlvObj), iTlvValueLen(psTlvObj));
		pszMsg += strlen(pszMsg);
		*pszMsg++ = ']';
	}
}

// 分解特定跟踪信息并输出
// in  : iTraceType : TRACE_ALWAYS|TRACE_PROC|TRACE_APDU|TRACE_TAG_DESC|TRACE_ERR_MSG|TRACE_TLV_DB|PARSE_FIELD55
//       iInfoType : 信息类型(PARSE_TLV_OBJECT)
//       psInfo    : 信息
//       iInfoLen  : 信息长度
//       iIndent   : 缩进位置
//       iRecursionLevel : 递归级别
// out : strLog    : 日志
// ret : 0         : OK
//       1         : 格式错误
static int iParseToTrace(int iTraceType, int iInfoType, uchar *psInfo, int iInfoLen, int iIndent, int iRecursionLevel)
{
    uchar szMsg[1024], *pszMsg;
	int   iTabLen = 4; // 每一层缩进字节数
	int   iRet;
	int   iValueLen;
	uchar *psTlvObjValue, *psTlvObj;
	int   iIndex, iTlvObjLen;
	uchar *p;
	int   i;

    szMsg[0] = 0;
    
	switch(iInfoType) {
	case PARSE_DOL:
		// DOL格式
		while(iInfoLen > 0) {
			int iTagLen;
			iTagLen = iTlvTagLen(psInfo);
			if(iTagLen <= 0) {
                vTraceWriteTxtCr(iTraceType, "%*sFormat error", iIndent, "");
				return(1);
			}
			if(iInfoLen < iTagLen+1) {
                vTraceWriteTxtCr(iTraceType, "%*sFormat error", iIndent, "");
				return(1);
			}
			pszMsg = szMsg;
			sprintf(pszMsg, "%*s", iIndent, "");
			pszMsg += strlen(pszMsg);
			vOneTwo0(psInfo, iTagLen, pszMsg);
			pszMsg += strlen(pszMsg);
			*pszMsg++ = ' ';
			vOneTwo0(psInfo+iTagLen, 1, pszMsg);
			pszMsg += 2;
			sprintf(pszMsg, " // %s", psTagAttrGetDesc(psInfo));
			vTraceWriteTxtCr(iTraceType, szMsg);
			psInfo += iTagLen + 1;
			iInfoLen -= iTagLen + 1;
		}
		break;
	case PARSE_TLV_GPO80:
		// GPO返回的格式0x80数据
		// TLV对象，T80
		iRet = iTlvCheckTlvObject(psInfo);
		if(iRet<0 || iRet>iInfoLen) {
			vTraceWriteTxtCr(iTraceType, "%*sFormat error", iIndent, "");
			return(1);
		}
		iValueLen = iTlvValue(psInfo, &psTlvObjValue);
		if(iRet < 2) {
			vTraceWriteTxtCr(iTraceType, "%*sFormat error, less data length", iIndent, "");
			return(1); // need at least Aip[2]
		}

		// AIP
		pszMsg = szMsg;
		sprintf(pszMsg, "%*s", iIndent, "");
		pszMsg += strlen(pszMsg);
		vOneTwo0(psTlvObjValue, 2, pszMsg);
		pszMsg += strlen(pszMsg);
		strcpy(pszMsg, " // [Support:");
		pszMsg += strlen(pszMsg);		
		if(psTlvObjValue[0] & 0x40)
		    strcpy(pszMsg, " SDA");
		if(psTlvObjValue[0] & 0x20)
		    strcpy(pszMsg, " DDA");
		if(psTlvObjValue[0] & 0x10)
		    strcpy(pszMsg, " Card holder verification");
		if(psTlvObjValue[0] & 0x08)
		    strcpy(pszMsg, " Terminal risk management");
		if(psTlvObjValue[0] & 0x04)
		    strcpy(pszMsg, " Issuer auth");
		if(psTlvObjValue[0] & 0x01)
		    strcpy(pszMsg, " CDA");
		if(psTlvObjValue[1] & 0x80)
		    strcpy(pszMsg, " MSD");
		pszMsg += strlen(pszMsg);
		strcpy(pszMsg, "] AIP");
		vTraceWriteTxtCr(iTraceType, szMsg);

		// AFL
		pszMsg = szMsg;
		for(i=0; i<(iValueLen-2)/4; i++) {
			p = psTlvObjValue + 2 + i*4; // p指向AFL 四元组首字节
    		sprintf(pszMsg, "%*s", iIndent, "");
	    	pszMsg += strlen(pszMsg);
    		vOneTwo0(p, 4, pszMsg);
	    	pszMsg += strlen(pszMsg);
	    	strcpy(pszMsg, " // [");
	    	pszMsg += strlen(pszMsg);
	    	sprintf(pszMsg, "SFI:%d", p[0]>>3);
	    	pszMsg += strlen(pszMsg);
	    	sprintf(pszMsg, " First Record No.:%d", p[1]);
	    	pszMsg += strlen(pszMsg);
	    	sprintf(pszMsg, " Last Record No.:%d", p[2]);
	    	pszMsg += strlen(pszMsg);
			if(p[3]) {
    	    	sprintf(pszMsg, " Need to be signed:%d", p[3]);
	        	pszMsg += strlen(pszMsg);
			}
   	    	sprintf(pszMsg, "] // AFL Entry %d", i);
    		vTraceWriteTxtCr(iTraceType, szMsg);
		}
		break;

	case PARSE_TLV_GAC180:
		// GPO返回的格式0x80数据, Format 1 CID(T9F27)[1] + ATC(T9F36) + AC(T9F26)[8] + IAD(T9F10)(Option)[n]
		iRet = iTlvCheckTlvObject(psInfo);
		if(iRet<0 || iRet>iInfoLen) {
			vTraceWriteTxtCr(iTraceType, "%*sFormat error", iIndent, "");
			return(1);
		}
		iValueLen = iTlvValue(psInfo, &psTlvObjValue);
		if(iRet < 11) {
			vTraceWriteTxtCr(iTraceType, "%*sFormat error, less data length", iIndent, "");
			return(1); // need at least Aip[2]
		}

		// CID
		pszMsg = szMsg;
		sprintf(pszMsg, "%*s", iIndent, "");
		pszMsg += strlen(pszMsg);
		vOneTwo0(psTlvObjValue, 1, pszMsg);
		pszMsg += strlen(pszMsg);
		strcpy(pszMsg, " // CID");
		vTraceWriteTxtCr(iTraceType, "%s", szMsg);
		// ATC
		pszMsg = szMsg;
		sprintf(pszMsg, "%*s", iIndent, "");
		pszMsg += strlen(pszMsg);
		vOneTwo0(psTlvObjValue+1, 2, pszMsg);
		pszMsg += strlen(pszMsg);
		strcpy(pszMsg, " // ATC");
		vTraceWriteTxtCr(iTraceType, "%s", szMsg);
		// AC
		pszMsg = szMsg;
		sprintf(pszMsg, "%*s", iIndent, "");
		pszMsg += strlen(pszMsg);
		vOneTwo0(psTlvObjValue+3, 8, pszMsg);
		pszMsg += strlen(pszMsg);
		strcpy(pszMsg, " // AC");
		vTraceWriteTxtCr(iTraceType, "%s", szMsg);
		// IAD(T9F10)(option)
		if(iValueLen-11 > 0) {
    		pszMsg = szMsg;
	    	sprintf(pszMsg, "%*s", iIndent, "");
    		pszMsg += strlen(pszMsg);
    		vOneTwo0(psTlvObjValue+11, iValueLen-11, pszMsg);
    		pszMsg += strlen(pszMsg);
    		strcpy(pszMsg, " // IAD(T9F10)");
    		vTraceWriteTxtCr(iTraceType, "%s", szMsg);
		}
		break;
	case PARSE_TLV_DB:
		// TLV数据库，遍历输出
		iRet = iTlvGetObjByIndex(psInfo, 0, &psTlvObj);
		if(iRet <= 0) {
			vTraceWriteTxtCr(iTraceType, "%*sTlv DB is empy", iIndent, "");
			return(1);
		}
		for(i=0; ; i++) {
    		iRet = iTlvGetObjByIndex(psInfo, i, &psTlvObj);
    		if(iRet <= 0)
    		    break;
    		iRet = iParseToTrace(iTraceType, PARSE_TLV_OBJECT, psInfo, iInfoLen, iIndent, 0);
    		if(iRet)
    		    return(1);
		}
		break;
	case PARSE_TLV_OBJECT:
		// TLV对象，模板或基本TLV对象
		iRet = iTlvCheckTlvObject(psInfo);
		if(iRet<0 || iRet>iInfoLen) {
			vTraceWriteTxtCr(iTraceType, "%*sFormat error", iIndent, "");
			return(1);
		}

		if((psInfo[0]&0x20) == 0) {
			// 基本TLV对象
    		pszMsg = szMsg;
	    	sprintf(pszMsg, "%*s", iIndent, "");
    		pszMsg += strlen(pszMsg);    		
			// Tag
	    	vOneTwo0(psInfo, iTlvTagLen(psInfo), pszMsg);
    		pszMsg += strlen(pszMsg);
    		*pszMsg++ = ' ';
			// Length
	    	vOneTwo0(psInfo+iTlvTagLen(psInfo), iTlvLengthLen(psInfo), pszMsg);
    		pszMsg += strlen(pszMsg);
    		*pszMsg++ = ' ';
			// Value
	    	vOneTwo0(psTlvValue(psInfo), iTlvValueLen(psInfo), pszMsg);
    		pszMsg += strlen(pszMsg);
    		strcpy(pszMsg, " // ");
    		pszMsg += strlen(pszMsg);
			vAddExtraInfo(pszMsg, psInfo);
    		pszMsg += strlen(pszMsg);
    		strcpy(pszMsg, psTagAttrGetDesc(psInfo));
    		vTraceWriteTxtCr(iTraceType, "%s", szMsg);
			return(0);
		} else {
			// 模板对象
    		pszMsg = szMsg;
	    	sprintf(pszMsg, "%*s", iIndent, "");
    		pszMsg += strlen(pszMsg);    		
			// Tag
	    	vOneTwo0(psInfo, iTlvTagLen(psInfo), pszMsg);
    		pszMsg += strlen(pszMsg);
    		*pszMsg++ = ' ';
			// Length
	    	vOneTwo0(psInfo+iTlvTagLen(psInfo), iTlvLengthLen(psInfo), pszMsg);
    		pszMsg += strlen(pszMsg);
    		strcpy(pszMsg, " // ");
    		pszMsg += strlen(pszMsg);
    		strcpy(pszMsg, psTagAttrGetDesc(psInfo));
    		vTraceWriteTxtCr(iTraceType, "%s", szMsg);

			for(iIndex=0; ; iIndex++) {
				iTlvObjLen = iTlvSerachTemplate(psInfo, iInfoLen, iIndex, &psTlvObj);
				if(iTlvObjLen == TLV_ERR_NOT_FOUND)
					break; // 没有TLV对象了，正常退出
				if(iTlvObjLen <= 0) {
        			vTraceWriteTxtCr(iTraceType, "%*sParse error", iIndent+iTabLen, "");
					return(1);
				}
				// 搜索到一个对象，递归处理
				iRet = iParseToTrace(iTraceType, PARSE_TLV_OBJECT, psTlvObj, iTlvObjLen, iIndent+iTabLen, iRecursionLevel+1);
				if(iRet)
					return(1); // 出错了
			}
		}
		break;
	case PARSE_FIELD55:
		// 8583报文55域格式
		pszMsg = szMsg;
		while(iInfoLen > 0) {
			iTlvObjLen = iTlvCheckTlvObject(psInfo);
			if(iTlvObjLen<0 || iTlvObjLen>iInfoLen)
				return(1);
			memset(pszMsg, '-', iIndent);
			pszMsg += iIndent; // 缩进
			vOneTwo0(psInfo, iTlvObjLen, pszMsg);
			pszMsg += iTlvObjLen*2;
			strcpy(pszMsg, TRACE_MSG_CRLF);
			pszMsg += strlen(pszMsg);
			iInfoLen -= iTlvObjLen;
			psInfo += iTlvObjLen;
		}
		vTraceWriteTxt(iTraceType, szMsg);
		break;
	default:
		vTraceWriteTxtCr(iTraceType, "%*sUnknown info type", iIndent, "");
		return(1);
	}
	return(0);
}

// 写结构化跟踪数据
// in  : iTraceType : TRACE_ALWAYS|TRACE_PROC|TRACE_APDU|TRACE_TAG_DESC|TRACE_ERR_MSG|TRACE_TLV_DB
//       iInfoType : 信息类型, PARSE_TLV_OBJECT|PARSE_TLV_GPO80|PARSE_TLV_GAC180|PARSE_DOL|PARSE_TLV_DB|PARSE_FIELD55
//       psInfo    : 信息
//       iInfoLen  : 信息长度
//       iIndent   : 缩进位置
void vTraceWriteStruct(int iTraceType, int iInfoType, uchar *psInfo, int iInfoLen, int iIndent)
{
    iParseToTrace(iTraceType, iInfoType, psInfo, iInfoLen, iIndent, 0);
}
