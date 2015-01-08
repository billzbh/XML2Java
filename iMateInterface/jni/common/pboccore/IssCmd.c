/**************************************
File name     : ISSCMD.C
Function      : Gemalto pboc2.0 Dual Pboc 卡发卡指令
Author        : Yu Jun
First edition : Jun 29th, 2009
Note          : immigrate from EMVCMD.C
Modified      : Aug 13th, 2013
					增加uiIssGetStatus()函数
                May 6th, 2014
                    修改uiEmvCmdExchangeApdu(), 抛弃APDU结构, 完全使用新增接口函数_uiDoApdu()实现
**************************************/
# include <stdio.h>
# include <string.h>
# include <stdarg.h>
# include "VposFace.h"
# include "isscmd.h"

static int      sg_iCardSlotNo = 0; // 卡座号

static int  sg_iLogFlag = 0;
static char sg_szLogFile[100] = "log.txt";
static FILE *fp = NULL;
static void vWriteLogHex(char *pszTitle, void *pLog, int iLength)
{
    unsigned char *psLog;
    char sBuf[1000];

	if(sg_iLogFlag == 0)
		return;
	if(fp == NULL) {
		fp = fopen(sg_szLogFile, "a+");
	    if(fp == NULL)
			return;
	}
    if(pszTitle) {
        sprintf(sBuf, "%s(%3d) : ", pszTitle, iLength);
		fwrite(sBuf, 1, strlen(sBuf), fp);
    }
    psLog = (unsigned char *)pLog;
	vOneTwo0(psLog, (ushort)iLength, sBuf);
	fwrite(sBuf, 1, iLength*2, fp);
	fwrite("\n", 1, 1, fp);
	fflush(fp);
}
static void vWriteLogTxt(char *pszFormat, ...)
{
    va_list args;

	if(sg_iLogFlag == 0)
		return;
	if(fp == NULL) {
		fp = fopen(sg_szLogFile, "a+");
	    if(fp == NULL)
			return;
	}
    va_start(args, pszFormat);
    vfprintf(fp, pszFormat, args);
	fwrite("\n", 1, 1, fp);
	fflush(fp);
    va_end(args);
}

// 设置APDU日志
// in  : iLogFlag  : 0:不记日志 1:记录日志
//       iEmptyLog : 清除日志标志, 0:不清楚 1:清除
void vIssSetLog(int iLogFlag, int iEmptyLog)
{
	sg_iLogFlag = iLogFlag;
	if(iEmptyLog) {
		if(fp) {
			fclose(fp);
			fp = NULL;
		}
		fp = fopen(sg_szLogFile, "wb");
		if(fp) {
			fclose(fp);
			fp = NULL;
		}
	}
}


// emv card slot select
// in  : uiSlotId   : virtual card slot id
// ret : 0          : OK
//       1          : 不支持此卡座
uint uiIssCmdSetSlot(uint uiSlotId)
{
	if(uiSlotId > 9)
		return(1); // 0-3:用户卡 4-7:Sam卡 8:非接触卡
	sg_iCardSlotNo = uiSlotId;
	return(0);
}

// test if card has been inserted
// ret : 0 : card not inserted
//       1 : card inserted
uint uiIssTestCard(void)
{
	if(_uiTestCard(sg_iCardSlotNo))
		return(1);
	return(0);
}

/*
// emv card reset
// ret : 0 : OK
//       1 : reset error
uint uiIssCmdResetCard(void)
{
	uint  uiRet;
	uchar sResetData[100];

	uiRet = _uiResetCard(sg_iCardSlotNo, sResetData);
	if(uiRet > 0) {
		vWriteLogHex("Reset  ", sResetData, uiRet);
		return(0);
	} 
	vWriteLogTxt("Reset  : error");
	return(1);
}
 */

// QINGBO
// emv card reset
// ret : >0 : OK
//       0  : reset error
uint uiIssCmdResetCard(uchar *psAtr)
{
    uint  uiRet;
    uchar sResetData[100];
    
    uiRet = _uiResetCard(sg_iCardSlotNo, sResetData);
    if(uiRet > 0) {
        memcpy(psAtr, sResetData, uiRet);
        vWriteLogHex("Reset  ", sResetData, uiRet);
        return(uiRet);
    }
    vWriteLogTxt("Reset  : error");
    return(0);
}

// emv card close
// ret : 0 : OK
uint uiIssCloseCard(void)
{
	vWriteLogTxt("Closed ");
	_uiCloseCard(sg_iCardSlotNo);
	return(0);
}

// emv card apdu
// in  : uiInLen   : apdu 指令长度
//       psApduIn  : 遵照emv规范case1-case4
// out : puiOutLen : apdu 应答长度(去除了尾部的SW[2])
//       psApduOut : apdu应答(去除了尾部的SW[2])
// ret : 0         : OK
//       1         : communication error between card and reader
//       other     : returned by card
uint uiIssCmdExchangeApdu(uint uiInLen, uchar *psApduIn, uint *puiOutLen, uchar *psApduOut)
{
    uint  uiRet;
    uint  uiOutLen;
    uchar sApduOut[260];

    uiOutLen = 0;
    *puiOutLen = 0;

	vWriteLogHex("ApduIn ", psApduIn, uiInLen);
    uiRet = _uiDoApdu(sg_iCardSlotNo, uiInLen, psApduIn, &uiOutLen, sApduOut, APDU_EMV);
    if(uiRet) {
		vWriteLogTxt("ApduOut : error");
        return(uiRet);
	}
    if(uiOutLen < 2) {
		vWriteLogTxt("ApduOut : error");
        return(1);
	}
	vWriteLogHex("ApduOut", sApduOut, uiOutLen);

    *puiOutLen = uiOutLen - 2;
    memcpy(psApduOut, sApduOut, *puiOutLen);
    if(memcmp(sApduOut+*puiOutLen, "\x90\x00", 2) != 0)
        uiRet = ulStrToLong(sApduOut+*puiOutLen, 2);
    return(uiRet);
}

// Install
// in  : ucClass  : C-APDU class, 80 or 84
//       ucP1     : P1
//       ucLength : length of install data
//       psIn     : install data
// ret : 0        : OK
//       1        : communication error between card and reader
//       other    : returned by card
// note: refer to Perso*.pdf 3.1
uint uiIssCmdInstall(uchar ucClass, uchar ucP1, uchar ucLength, uchar *psIn)
{
    uint  uiRet;
    uint  uiInLen, uiOutLen;
    uchar sApduIn[262], sApduOut[260];

    uiInLen = 0;
    sApduIn[uiInLen++] = ucClass;
    sApduIn[uiInLen++] = 0xE6;
    sApduIn[uiInLen++] = ucP1;
    sApduIn[uiInLen++] = 0x00;
    sApduIn[uiInLen++] = ucLength;
    memcpy(sApduIn+uiInLen, psIn, ucLength);
    uiInLen += ucLength;
    sApduIn[uiInLen++] = 0;
    uiRet = uiIssCmdExchangeApdu(uiInLen, sApduIn, &uiOutLen, sApduOut);
	return(uiRet);
}

// Initialize
// in  : ucKeyVer   : Key Set Version
//       ucKeyIndex : Key Index
//       psHostRand : Host Challenge [8]
// out : psOut      : Out Data [28]
// ret : 0          : OK
//       1          : communication error between card and reader
//       0x8001     : length out error
//       other      : returned by card
// note: refer to proso*.pdf 6.2.1.1
uint uiIssCmdInitialize(uchar ucKeyVer, uchar ucKeyIndex, uchar *psHostRand, uchar *psOut)
{
    uint  uiRet;
    uint  uiInLen, uiOutLen;
    uchar sApduIn[262], sApduOut[260];

    uiInLen = 0;
    sApduIn[uiInLen++] = 0x80;
    sApduIn[uiInLen++] = 0x50;
    sApduIn[uiInLen++] = ucKeyVer;
    sApduIn[uiInLen++] = ucKeyIndex;
    sApduIn[uiInLen++] = 8;
    memcpy(sApduIn+uiInLen, psHostRand, 8);
    uiInLen += 8;
    sApduIn[uiInLen++] = 0x00;
    uiRet = uiIssCmdExchangeApdu(uiInLen, sApduIn, &uiOutLen, sApduOut);
	if(uiRet != 0)
		return(uiRet);
	if(uiOutLen != 28)
		return(0x8001);
	memcpy(psOut, sApduOut, 28);
    return(0);
}

// external authentication
// in  : ucClass  : 0x80 or 0x84
//       psIn     : ciphered data
// ret : 0        : OK
//       1        : communication error between card and reader
//       other    : returned by card
// note: refer to perso*.pdf 6.2.1.2
//       暂只支持secure message方式
uint uiIssCmdExternalAuth(uchar ucClass, uchar *psIn)
{
    uint  uiRet;
    uint  uiInLen, uiOutLen;
    uchar sApduIn[262], sApduOut[260];

    uiInLen = 0;
    sApduIn[uiInLen++] = ucClass;
    sApduIn[uiInLen++] = 0x82;
    sApduIn[uiInLen++] = 0x00;
    sApduIn[uiInLen++] = 0x00;
	if(ucClass & 0x04)
		sApduIn[uiInLen++] = 16;
	else
		sApduIn[uiInLen++] = 8;
    memcpy(sApduIn+uiInLen, psIn, sApduIn[4]);
    uiInLen += sApduIn[4];
    sApduIn[uiInLen++] = 0;
    uiRet = uiIssCmdExchangeApdu(uiInLen, sApduIn, &uiOutLen, sApduOut);
	return(uiRet);
}

// put data
// in  : ucP1         : parameter 1
//       ucP2         : parameter 2
//       ucLengthIn   : data length send to card
//       psDataIn     : data send to the card
// ret : 0            : OK
//       1            : communication error between card and reader
//       other        : returned by card
// note: refer to perso*.pdf 6.2.1.3
uint uiIssCmdPutData(uchar ucP1, uchar ucP2, uchar ucLengthIn, uchar *psDataIn)
{
    uint  uiRet;
    uint  uiInLen, uiOutLen;
    uchar sApduIn[262], sApduOut[260];

    uiInLen = 0;
    sApduIn[uiInLen++] = 0x80;
    sApduIn[uiInLen++] = 0xDA;
    sApduIn[uiInLen++] = ucP1;
    sApduIn[uiInLen++] = ucP2;
    sApduIn[uiInLen++] = ucLengthIn;
    memcpy(sApduIn+uiInLen, psDataIn, ucLengthIn);
    uiInLen += ucLengthIn;
//    sApduIn[uiInLen++] = 0;
    uiRet = uiIssCmdExchangeApdu(uiInLen, sApduIn, &uiOutLen, sApduOut);
	return(uiRet);
}

// append record
// in  : ucP2         : parameter 2
//       ucLengthIn   : data length send to card
//       psDataIn     : data send to the card
// ret : 0            : OK
//       1            : communication error between card and reader
//       other        : returned by card
// note: refer to perso*.pdf 6.2.1.4
uint uiIssCmdAppendRecord(uchar ucP2, uchar ucLengthIn, uchar *psDataIn)
{
    uint  uiRet;
    uint  uiInLen, uiOutLen;
    uchar sApduIn[262], sApduOut[260];

    uiInLen = 0;
    sApduIn[uiInLen++] = 0x00;
    sApduIn[uiInLen++] = 0xE2;
    sApduIn[uiInLen++] = 0x00;
    sApduIn[uiInLen++] = ucP2;
    sApduIn[uiInLen++] = ucLengthIn;
    memcpy(sApduIn+uiInLen, psDataIn, ucLengthIn);
    uiInLen += ucLengthIn;
//    sApduIn[uiInLen++] = 0;
    uiRet = uiIssCmdExchangeApdu(uiInLen, sApduIn, &uiOutLen, sApduOut);
	return(uiRet);
}
// update record
// in  : ucP1         : parameter 1
//       ucP2         : parameter 2
//       ucLengthIn   : data length send to card
//       psDataIn     : data send to the card
// ret : 0            : OK
//       1            : communication error between card and reader
//       other        : returned by card
uint uiIssCmdUpdateRecord(uchar ucP1, uchar ucP2, uchar ucLengthIn, uchar *psDataIn)
{
    uint  uiRet;
    uint  uiInLen, uiOutLen;
    uchar sApduIn[262], sApduOut[260];

    uiInLen = 0;
    sApduIn[uiInLen++] = 0x00;
    sApduIn[uiInLen++] = 0xDC;
    sApduIn[uiInLen++] = ucP1;
    sApduIn[uiInLen++] = ucP2;
    sApduIn[uiInLen++] = ucLengthIn;
    memcpy(sApduIn+uiInLen, psDataIn, ucLengthIn);
    uiInLen += ucLengthIn;
//    sApduIn[uiInLen++] = 0;
    uiRet = uiIssCmdExchangeApdu(uiInLen, sApduIn, &uiOutLen, sApduOut);
	return(uiRet);
}

// put key
// in  : ucP1         : P1
//       ucP2         : P2
//       ucLengthIn   : data length send to card
//       psDataIn     : data send to the card
// ret : 0            : OK
//       1            : communication error between card and reader
//       other        : returned by card
// note: refer to perso*.pdf 6.2.1.5
uint uiIssCmdPutKey(uchar ucP1, uchar ucP2, uchar ucLengthIn, uchar *psDataIn)
{
    uint  uiRet;
    uint  uiInLen, uiOutLen;
    uchar sApduIn[262], sApduOut[260];

    uiInLen = 0;
    sApduIn[uiInLen++] = 0x80;
    sApduIn[uiInLen++] = 0xD8;
    sApduIn[uiInLen++] = ucP1;
    sApduIn[uiInLen++] = ucP2;
    sApduIn[uiInLen++] = ucLengthIn;
    memcpy(sApduIn+uiInLen, psDataIn, ucLengthIn);
    uiInLen += ucLengthIn;
//    sApduIn[uiInLen++] = 0;
    uiRet = uiIssCmdExchangeApdu(uiInLen, sApduIn, &uiOutLen, sApduOut);
	return(uiRet);
}

// pin change
// in  : ucP2         : P2 for pin try limit
//       psDataIn     : enciphered pin data [8]
// ret : 0            : OK
//       1            : communication error between card and reader
//       other        : returned by card
// note: refer to perso*.pdf 6.2.1.6
uint uiIssCmdPinChange(uchar ucP2, uchar *psDataIn)
{
    uint  uiRet;
    uint  uiInLen, uiOutLen;
    uchar sApduIn[262], sApduOut[260];

    uiInLen = 0;
    sApduIn[uiInLen++] = 0x80;
    sApduIn[uiInLen++] = 0x24;
    sApduIn[uiInLen++] = 0x00;
    sApduIn[uiInLen++] = ucP2;
    sApduIn[uiInLen++] = 8;
    memcpy(sApduIn+uiInLen, psDataIn, 8);
    uiInLen += 8;
//    sApduIn[uiInLen++] = 0;
    uiRet = uiIssCmdExchangeApdu(uiInLen, sApduIn, &uiOutLen, sApduOut);
	return(uiRet);
}

// set status
// in  : ucP1         : P1, 0x40 : Security Domain Life Cycle State
//       ucP2         : P2, 0x0F : Status value for the Application: Personalized
// ret : 0            : OK
//       1            : communication error between card and reader
//       other        : returned by card
// note: refer to perso*.pdf 6.2.1.7
uint uiIssCmdSetStatus(uchar ucP1, uchar ucP2)
{
    uint  uiRet;
    uint  uiInLen, uiOutLen;
    uchar sApduIn[262], sApduOut[260];

    uiInLen = 0;
    sApduIn[uiInLen++] = 0x80;
    sApduIn[uiInLen++] = 0xF0;
    sApduIn[uiInLen++] = ucP1;
    sApduIn[uiInLen++] = ucP2;
    sApduIn[uiInLen++] = 0;
    uiRet = uiIssCmdExchangeApdu(uiInLen, sApduIn, &uiOutLen, sApduOut);
	return(uiRet);
}


// clear file
// in  : ucAidLen : length of aid
//       psAid    : aid
// ret : 0        : OK
//       1            : communication error between card and reader
//       other        : returned by card
// note: refer to java card SRC from XiaFei (PageDelete.cpp)
uint uiIssCmdClearFile(uchar ucAidLen, uchar *psAid)
{
    uint  uiRet;
    uint  uiInLen, uiOutLen;
    uchar sApduIn[262], sApduOut[260];

    uiInLen = 0;
    sApduIn[uiInLen++] = 0x80;
    sApduIn[uiInLen++] = 0xE4;
    sApduIn[uiInLen++] = 0x00;
    sApduIn[uiInLen++] = 0x00;
    sApduIn[uiInLen++] = 2+ucAidLen;
    sApduIn[uiInLen++] = 0x4F;
    sApduIn[uiInLen++] = ucAidLen;
    memcpy(sApduIn+uiInLen, psAid, ucAidLen);
    uiInLen += ucAidLen;
    uiRet = uiIssCmdExchangeApdu(uiInLen, sApduIn, &uiOutLen, sApduOut);
	return(uiRet);
}

// store data
// in  : ucP1         : P1
//       ucP2         : P2
//       ucLengthIn   : data length send to card
//       psDataIn     : data send to the card
// ret : 0            : OK
//       1            : communication error between card and reader
//       other        : returned by card
// note: refer to
uint uiIssCmdStoreData(uchar ucP1, uchar ucP2, uchar ucLengthIn, uchar *psDataIn)
{
    uint  uiRet;
    uint  uiInLen, uiOutLen;
    uchar sApduIn[262], sApduOut[260];

    uiInLen = 0;
    sApduIn[uiInLen++] = 0x80;
    sApduIn[uiInLen++] = 0xE2;
    sApduIn[uiInLen++] = ucP1;
    sApduIn[uiInLen++] = ucP2;
    sApduIn[uiInLen++] = ucLengthIn;
    memcpy(sApduIn+uiInLen, psDataIn, ucLengthIn);
    uiInLen += ucLengthIn;
    uiRet = uiIssCmdExchangeApdu(uiInLen, sApduIn, &uiOutLen, sApduOut);
	return(uiRet);
}

// get status
// in  : ucP1         : P1
//       ucP2         : P2
//       ucLengthIn   : data length send to card
//       psDataIn     : data send to the card
// ret : 0            : OK
//       1            : communication error between card and reader
//       other        : returned by card
// note: refer to
uint uiIssGetStatus(uchar ucP1, uchar ucP2, uchar ucLengthIn, uchar *psDataIn, uchar *pucLengthOut, uchar *psDataOut)
{
    uint  uiRet;
    uint  uiInLen, uiOutLen;
    uchar sApduIn[262], sApduOut[260];

    uiInLen = 0;
    sApduIn[uiInLen++] = 0x80;
    sApduIn[uiInLen++] = 0xF2;
    sApduIn[uiInLen++] = ucP1;
    sApduIn[uiInLen++] = ucP2;
    sApduIn[uiInLen++] = ucLengthIn;
    memcpy(sApduIn+uiInLen, psDataIn, ucLengthIn);
    uiInLen += ucLengthIn;
    sApduIn[uiInLen++] = 0x00;
    uiRet = uiIssCmdExchangeApdu(uiInLen, sApduIn, &uiOutLen, sApduOut);
	if(uiRet != 0)
		return(uiRet);
	*pucLengthOut = uiOutLen;
	memcpy(psDataOut, sApduOut, uiOutLen);
    return(0);
}

// 以下为EMV规范指令
// emv card read record file
// in  : ucSFI      : short file identifier
//       ucRecordNo : record number, starting from 1
// out : psBuf      : data buffer
//       pucLength  : length read
// ret : 0          : OK
//       1          : communication error between card and reader
//       other      : returned by card
// note: refer to EMV2004 book3 Part II, 6.5.11
uint uiIssCmdRdRec(uchar ucSFI, uchar ucRecordNo, uchar *pucLength, uchar *psBuf)
{
    uint  uiRet;
	uchar ucSW1;
    uint  uiInLen, uiOutLen;
    uchar sApduIn[262], sApduOut[260];

    uiInLen = 0;
    sApduIn[uiInLen++] = 0x00;
    sApduIn[uiInLen++] = 0xb2;
    sApduIn[uiInLen++] = ucRecordNo;
    sApduIn[uiInLen++] = (ucSFI<<3)|0x04; // 0x04 : p1 is a record number
    sApduIn[uiInLen++] = 0;
    uiRet = uiIssCmdExchangeApdu(uiInLen, sApduIn, &uiOutLen, sApduOut);
	ucSW1 = uiRet>>8;
	if(uiRet!=1 && (ucSW1==0x00 || ucSW1==0x62 || ucSW1==0x63)) {
	    *pucLength = uiOutLen;
		memcpy(psBuf, sApduOut, uiOutLen);
	}
	return(uiRet);
}

// emv card select file
// in  : ucP2      : parameter 2, 0x00 : first occurrence, 0x02 : next occurrence
//       ucAidLen  : length of the AID
//       psAid     : AID
// out : pucFciLen : FCI length, NULL means don't get FCI info
//       psFci     : file control infomation, NULL means don't get FCI info
// ret : 0         : OK
//       1         : communication error between card and reader
//       other     : returned by card
// note: refer to EMV2004 book1 Part III, 11.3
uint uiIssCmdSelect(uchar ucP2, uchar ucAidLen, uchar *psAid, uchar *pucFciLen, uchar *psFci)
{
    uint  uiRet;
	uchar ucSW1;
    uint  uiInLen, uiOutLen;
    uchar sApduIn[262], sApduOut[260];

    uiInLen = 0;
    sApduIn[uiInLen++] = 0x00;
    sApduIn[uiInLen++] = 0xa4;
    sApduIn[uiInLen++] = 0x04;
    sApduIn[uiInLen++] = ucP2;
    sApduIn[uiInLen++] = ucAidLen;
    memcpy(sApduIn+uiInLen, psAid, ucAidLen);
    uiInLen += ucAidLen;
    if(ucAidLen != 0)
        sApduIn[uiInLen++] = 0;
    uiRet = uiIssCmdExchangeApdu(uiInLen, sApduIn, &uiOutLen, sApduOut);
	ucSW1 = uiRet>>8;
	if(uiRet!=1 && (ucSW1==0x00 || ucSW1==0x62 || ucSW1==0x63)) {
		if(pucFciLen && psFci) {
		    *pucFciLen = uiOutLen;
		    memcpy(psFci, sApduOut, uiOutLen);
		}
	}
	return(uiRet);
}

// emv card external authentication
// in  : ucLength : length of data
//       psIn     : ciphered data
// ret : 0        : OK
//       1        : communication error between card and reader
//       other    : returned by card
// note: refer to EMV2004 book3 Part II, 6.5.4
uint uiIssCmdExternalAuth_Emv(uchar ucLength, uchar *psIn)
{
    uint  uiRet;
    uint  uiInLen, uiOutLen;
    uchar sApduIn[262], sApduOut[260];

    uiInLen = 0;
    sApduIn[uiInLen++] = 0x00;
    sApduIn[uiInLen++] = 0x82;
    sApduIn[uiInLen++] = 0x00;
    sApduIn[uiInLen++] = 0x00;
    sApduIn[uiInLen++] = ucLength;
    memcpy(sApduIn+uiInLen, psIn, ucLength);
    uiInLen += ucLength;
    uiRet = uiIssCmdExchangeApdu(uiInLen, sApduIn, &uiOutLen, sApduOut);
    return(uiRet);
}

// emv card generate AC
// in  : ucP1         : parameter 1
//       ucLengthIn   : data length send to card
//       psDataIn     : data send to the card
// out : pucLengthOut : data length received from card
//       psDataOut    : data received from card
// ret : 0            : OK
//       1            : communication error between card and reader
//       other        : returned by card
// note: refer to EMV2004 book3 Part II, 6.5.5
uint uiIssCmdGenerateAC(uchar ucP1, uchar ucLengthIn, uchar *psDataIn, uchar *pucLengthOut, uchar *psDataOut)
{
    uint  uiRet;
	uchar ucSW1;
    uint  uiInLen, uiOutLen;
    uchar sApduIn[262], sApduOut[260];

    uiInLen = 0;
    sApduIn[uiInLen++] = 0x80;
    sApduIn[uiInLen++] = 0xae;
    sApduIn[uiInLen++] = ucP1;
    sApduIn[uiInLen++] = 0x00;
    sApduIn[uiInLen++] = ucLengthIn;
    memcpy(sApduIn+uiInLen, psDataIn, ucLengthIn);
    uiInLen += ucLengthIn;
    sApduIn[uiInLen++] = 0;
    uiRet = uiIssCmdExchangeApdu(uiInLen, sApduIn, &uiOutLen, sApduOut);
	ucSW1 = uiRet>>8;
	if(uiRet!=1 && (ucSW1==0x00 || ucSW1==0x62 || ucSW1==0x63)) {
	    *pucLengthOut = uiOutLen;
		memcpy(psDataOut, sApduOut, uiOutLen);
	}
	return(uiRet);
}

// emv card get 8-byte random number from pboc card
// out : pucLength: 长度
//       psOut    : random number from pboc card
// ret : 0        : OK
//       1        : communication error between card and reader
//       other    : returned by card
// note: refer to EMV2004 book3 Part II, 6.5.6
uint uiIssCmdGetChallenge(uchar *pucLength, uchar *psOut)
{
    uint  uiRet;
	uchar ucSW1;
    uint  uiInLen, uiOutLen;
    uchar sApduIn[262], sApduOut[260];

    uiInLen = 0;
    sApduIn[uiInLen++] = 0x00;
    sApduIn[uiInLen++] = 0x84;
    sApduIn[uiInLen++] = 0x00;
    sApduIn[uiInLen++] = 0x00;
    sApduIn[uiInLen++] = 0;
    uiRet = uiIssCmdExchangeApdu(uiInLen, sApduIn, &uiOutLen, sApduOut);
	ucSW1 = uiRet>>8;
	if(uiRet!=1 && (ucSW1==0x00 || ucSW1==0x62 || ucSW1==0x63)) {
	    *pucLength = uiOutLen;
		memcpy(psOut, sApduOut, uiOutLen);
	}
	return(uiRet);
}

// emv card get data
// in  : psTag     : tag, can only be "\x9f\x36" or "\x9f\x13" or "\x9f\x17" or "\x9f\x4f"
// out : pucLength : length of data
//       psData    : data
// ret : 0         : OK
//       1         : communication error between card and reader
//       other     : returned by card
// note: refer to EMV2004 book3 Part II, 6.5.7
uint uiIssCmdGetData(uchar *psTag, uchar *pucLength, uchar *psData)
{
    uint  uiRet;
	uchar ucSW1;
    uint  uiInLen, uiOutLen;
    uchar sApduIn[262], sApduOut[260];

    uiInLen = 0;
    sApduIn[uiInLen++] = 0x80;
    sApduIn[uiInLen++] = 0xca;
    sApduIn[uiInLen++] = psTag[0];
    sApduIn[uiInLen++] = psTag[1];
    sApduIn[uiInLen++] = 0;
    uiRet = uiIssCmdExchangeApdu(uiInLen, sApduIn, &uiOutLen, sApduOut);
	ucSW1 = uiRet>>8;
	if(uiRet!=1 && (ucSW1==0x00 || ucSW1==0x62 || ucSW1==0x63)) {
	    *pucLength = uiOutLen;
		memcpy(psData, sApduOut, uiOutLen);
	}
	return(uiRet);
}

// emv card get processing options
// in  : ucLengthIn   : data length send to card
//       psDataIn     : data send to the card
// out : pucLengthOut : data length received from card
//       psDataOut    : data received from card
// ret : 0            : OK
//       1            : communication error between card and reader
//       other        : returned by card
// note: refer to EMV2004 book3 Part II, 6.5.8
uint uiIssCmdGetOptions(uchar ucLengthIn, uchar *psDataIn, uchar *pucLengthOut, uchar *psDataOut)
{
    uint  uiRet;
	uchar ucSW1;
    uint  uiInLen, uiOutLen;
    uchar sApduIn[262], sApduOut[260];

    uiInLen = 0;
    sApduIn[uiInLen++] = 0x80;
    sApduIn[uiInLen++] = 0xa8;
    sApduIn[uiInLen++] = 0x00;
    sApduIn[uiInLen++] = 0x00;
    sApduIn[uiInLen++] = ucLengthIn;
    memcpy(sApduIn+uiInLen, psDataIn, ucLengthIn);
    uiInLen += ucLengthIn;
    sApduIn[uiInLen++] = 0;
    uiRet = uiIssCmdExchangeApdu(uiInLen, sApduIn, &uiOutLen, sApduOut);
	ucSW1 = uiRet>>8;
	if(uiRet!=1 && (ucSW1==0x00 || ucSW1==0x62 || ucSW1==0x63)) {
	    *pucLengthOut = uiOutLen;
		memcpy(psDataOut, sApduOut, uiOutLen);
	}
	return(uiRet);
}

// emv card internal authentication
// in  : ucLengthIn   : data length send to card
//       psIn         : data send to the card
// out : pucLengthOut : data length received from the card
//       psOut        : data received from the card
// ret : 0            : OK
//       1            : communication error between card and reader
//       other        : returned by card
// note: refer to EMV2004 book3 Part II, 6.5.9
uint uiIssCmdInternalAuth(uchar ucLengthIn, uchar *psIn, uchar *pucLengthOut, uchar *psOut)
{
    uint  uiRet;
	uchar ucSW1;
    uint  uiInLen, uiOutLen;
    uchar sApduIn[262], sApduOut[260];

    uiInLen = 0;
    sApduIn[uiInLen++] = 0x00;
    sApduIn[uiInLen++] = 0x88;
    sApduIn[uiInLen++] = 0x00;
    sApduIn[uiInLen++] = 0x00;
    sApduIn[uiInLen++] = ucLengthIn;
    memcpy(sApduIn+uiInLen, psIn, ucLengthIn);
    uiInLen += ucLengthIn;
    sApduIn[uiInLen++] = 0;
    uiRet = uiIssCmdExchangeApdu(uiInLen, sApduIn, &uiOutLen, sApduOut);
	ucSW1 = uiRet>>8;
	if(uiRet!=1 && (ucSW1==0x00 || ucSW1==0x62 || ucSW1==0x63)) {
	    *pucLengthOut = uiOutLen;
		memcpy(psOut, sApduOut, uiOutLen);
	}
	return(uiRet);
}

// emv card verify user pin
// in  : ucP2      : parameter 2
//       ucLength  : length of pin relevant data
//       psPinData : pin data
// ret : 0         : OK
//       1         : communication error between card and reader
//       other     : returned by card
// note: refer to EMV2004 book3 Part II, 6.5.12
uint uiIssCmdVerifyPin(uchar ucP2, uchar ucLength, uchar *psPinData)
{
    uint  uiRet;
    uint  uiInLen, uiOutLen;
    uchar sApduIn[262], sApduOut[260];

    uiInLen = 0;
    sApduIn[uiInLen++] = 0x00;
    sApduIn[uiInLen++] = 0x20;
    sApduIn[uiInLen++] = 0x00;
    sApduIn[uiInLen++] = ucP2;
    sApduIn[uiInLen++] = ucLength;
    memcpy(sApduIn+uiInLen, psPinData, ucLength);
    uiInLen += ucLength;
    uiRet = uiIssCmdExchangeApdu(uiInLen, sApduIn, &uiOutLen, sApduOut);
    return(uiRet);
}
