/**************************************
File name     : EMVTRACE.C
Function      : ���̸�����Ϣ������debug
Author        : Yu Jun
First edition : Mar 27th, 2012
Modified      : 
**************************************/
/*
ģ����ϸ����:
    �ṩ�����ļ��߲�Ӧ��д������־����
.   ʹ�ñ�ģ��ǰ�����ȵ���iTraceSet()���ò���, �ýӿ�֪ͨ��ģ��ʹ��ʲô�豸�����־�������Щ��־
.   vTraceWrite...()�ӿڴ����iTraceType�����ú�����������ݸ�������, ֻ��iTraceSet()�趨����־�Żᱻ���
    ͨѶ��Ϣ��������Ϣ��������Ϣ�ᱻǿ�����
.	vTraceWriteStruct()�ɶ��ض�������־���и�ʽ��
	Ӧ�ò�һ��û��Ҫʹ�øýӿ�
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

#define TRACE_MSG_CRLF  "\x0d\x0a"  // ���д�

static int sg_iTraceDev;        // �����豸
static int sg_iTraceApdu;       // ����apduָ���־��0:������ 1:����
static int sg_iTraceComm;       // ����ͨѶ��Ϣ
static int sg_iTraceTagDesc;    // ����tag��ϸ��־��0:������ 1:����
static int sg_iTraceTlvDb;      // ����TLV���ݿ��־��0:������ 1:����
static int sg_iTraceProc;       // ����������Ϣ��0:������ 1:����
static int sg_iTraceErrMsg;     // ���ٴ�����Ϣ��0:������ 1:����

static void (*sg_pfvTraceCallBack)(char *pOut) = NULL; // Trace�ص�����

// ���������Ϣ�������豸
// in  : iTraceType   : ������Ϣ����
//       pszTraceInfo : ������Ϣ
static void vTraceOut(int iTraceType, uchar *pszTraceInfo)
{
    uchar ucOldPort, ucNewPort;
		
    // ���û���趨���ٸ����ͣ�ֱ�ӷ���
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
    
    // ���������Ϣ���ض����豸
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
        _ucAsyncSetPort(ucOldPort); // �ָ��˿�
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

// ���ø����豸
// in  : iTraceDev     : �����豸��TRACE_COM1-TRACE_COM8 | TRACE_CONSOLE | TRACE_PRINTER | TRACE_PTS0-7 | TRACE_CALLBACK
//                       TRACE_OFF��ʾ���������ݸ���
//       iTraceApdu    : ����apdu��־��0:������ 1:����
//       iTraceTagDesc : ����tag��ϸ��־��0:������ 1:����
//       iTraceTlvDb   : ����Tlv���ݿ��־��0:������ 1:����
//       pfvTraceOut   : �ص����ٺ���ָ��, �����豸ΪTRACE_CALLBACKʱ��Ҫ
// ret : 0         : OK
//       1         : ��֧�ָ����豸
//       2         : �����豸����
// Note: ����apduָ�����ݼ�Tag��ϸ�������Ƚϴ󣬹ʿ��趨���ٻ򲻸��ٱ�־
int iTraceSet(int iTraceDev, int iTraceApdu, int iTraceTagDesc, int iTraceTlvDb, void (*pfvTraceOut)(char *))
{
    uchar ucOldPort, ucNewPort;
    uchar ucRet;

    sg_iTraceApdu = iTraceApdu;
	sg_iTraceComm = 1;   // ǿ�Ƹ���ͨѶ��Ϣ
    sg_iTraceTagDesc = iTraceTagDesc;
    sg_iTraceTlvDb = iTraceTlvDb;
    sg_iTraceProc = 1;   // ǿ�Ƹ���������Ϣ
    sg_iTraceErrMsg = 1; // ǿ�Ƹ��ٴ�����Ϣ
    
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
        _ucAsyncSetPort(ucOldPort); // �ָ��˿�
        if(ucRet) {
            sg_iTraceDev = TRACE_OFF;
            return(2); // �豸����
        }
        sg_iTraceDev = iTraceDev;
        break;
    case TRACE_CONSOLE:
#if((POS_TYPE&POS_TYPE_MASK)==POS_TYPE_YIN5 || POS_TYPE==POS_TYPE_LINUX)
		sg_iTraceDev = TRACE_CONSOLE;
		break;
#else
        sg_iTraceDev = TRACE_OFF;
        return(1); // ��֧��
#endif
    case TRACE_AUX:
        sg_iTraceDev = TRACE_OFF;
        return(1); // ��֧��
    case TRACE_PRINTER:
        if(_uiGetPCols() == 0) {
            sg_iTraceDev = TRACE_OFF;
            return(2); // �豸����
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
        return(1); // ��֧��
#endif
		break;
	case TRACE_CALLBACK:
		sg_pfvTraceCallBack = pfvTraceOut;
        sg_iTraceDev = iTraceDev;
		break;
    default:
        sg_iTraceDev = TRACE_OFF;
        return(1); // ��֧��
    }
    return(0);
}

// ȡ�ļ�������
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

// дtxt��������
// in  : iTraceType : TRACE_ALWAYS|TRACE_PROC|TRACE_APDU|TRACE_TAG_DESC|TRACE_ERR_MSG|TRACE_TLV_DB
void vTraceWriteTxt(int iTraceType, char *pszFormat, ...)
{
    va_list args;
    char    szMsg[2048];
    
    va_start(args, pszFormat);
    vsprintf(szMsg, pszFormat, args);
    vTraceOut(iTraceType, szMsg);
}

// дtxt���س���������
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

// дbin��������
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

// дbin���س���������
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

// д�ϴ�apdu��������
// in  : pszFormat  : ��ʾ��Ϣ
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
		// ������
		vTraceWriteTxtCr(TRACE_APDU, "ApduIn :");
	    vTraceWriteTxtCr(TRACE_APDU, "ApduOut:");
		return;
	}
    if(gl_sLastApduIn[0]&0x01) {
		gl_sLastApduIn[0] &= 0xFE;
		vOneTwo0(gl_sLastApduIn, gl_uiLastApduInLen, szMsg);
        vTraceWriteTxt(TRACE_APDU, "ApduIn*:"); // ��*�ű�ʾ��ָ��Ϊ����������ָ��
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

// ���Ӷ������Ϣ
// in  : pszMsg   : ԭ��������
//       psTlvObj : Tlv����
// out : pszMsg   : ������������
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

// �ֽ��ض�������Ϣ�����
// in  : iTraceType : TRACE_ALWAYS|TRACE_PROC|TRACE_APDU|TRACE_TAG_DESC|TRACE_ERR_MSG|TRACE_TLV_DB|PARSE_FIELD55
//       iInfoType : ��Ϣ����(PARSE_TLV_OBJECT)
//       psInfo    : ��Ϣ
//       iInfoLen  : ��Ϣ����
//       iIndent   : ����λ��
//       iRecursionLevel : �ݹ鼶��
// out : strLog    : ��־
// ret : 0         : OK
//       1         : ��ʽ����
static int iParseToTrace(int iTraceType, int iInfoType, uchar *psInfo, int iInfoLen, int iIndent, int iRecursionLevel)
{
    uchar szMsg[1024], *pszMsg;
	int   iTabLen = 4; // ÿһ�������ֽ���
	int   iRet;
	int   iValueLen;
	uchar *psTlvObjValue, *psTlvObj;
	int   iIndex, iTlvObjLen;
	uchar *p;
	int   i;

    szMsg[0] = 0;
    
	switch(iInfoType) {
	case PARSE_DOL:
		// DOL��ʽ
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
		// GPO���صĸ�ʽ0x80����
		// TLV����T80
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
			p = psTlvObjValue + 2 + i*4; // pָ��AFL ��Ԫ�����ֽ�
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
		// GPO���صĸ�ʽ0x80����, Format 1 CID(T9F27)[1] + ATC(T9F36) + AC(T9F26)[8] + IAD(T9F10)(Option)[n]
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
		// TLV���ݿ⣬�������
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
		// TLV����ģ������TLV����
		iRet = iTlvCheckTlvObject(psInfo);
		if(iRet<0 || iRet>iInfoLen) {
			vTraceWriteTxtCr(iTraceType, "%*sFormat error", iIndent, "");
			return(1);
		}

		if((psInfo[0]&0x20) == 0) {
			// ����TLV����
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
			// ģ�����
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
					break; // û��TLV�����ˣ������˳�
				if(iTlvObjLen <= 0) {
        			vTraceWriteTxtCr(iTraceType, "%*sParse error", iIndent+iTabLen, "");
					return(1);
				}
				// ������һ�����󣬵ݹ鴦��
				iRet = iParseToTrace(iTraceType, PARSE_TLV_OBJECT, psTlvObj, iTlvObjLen, iIndent+iTabLen, iRecursionLevel+1);
				if(iRet)
					return(1); // ������
			}
		}
		break;
	case PARSE_FIELD55:
		// 8583����55���ʽ
		pszMsg = szMsg;
		while(iInfoLen > 0) {
			iTlvObjLen = iTlvCheckTlvObject(psInfo);
			if(iTlvObjLen<0 || iTlvObjLen>iInfoLen)
				return(1);
			memset(pszMsg, '-', iIndent);
			pszMsg += iIndent; // ����
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

// д�ṹ����������
// in  : iTraceType : TRACE_ALWAYS|TRACE_PROC|TRACE_APDU|TRACE_TAG_DESC|TRACE_ERR_MSG|TRACE_TLV_DB
//       iInfoType : ��Ϣ����, PARSE_TLV_OBJECT|PARSE_TLV_GPO80|PARSE_TLV_GAC180|PARSE_DOL|PARSE_TLV_DB|PARSE_FIELD55
//       psInfo    : ��Ϣ
//       iInfoLen  : ��Ϣ����
//       iIndent   : ����λ��
void vTraceWriteStruct(int iTraceType, int iInfoType, uchar *psInfo, int iInfoLen, int iIndent)
{
    iParseToTrace(iTraceType, iInfoType, psInfo, iInfoLen, iIndent, 0);
}
