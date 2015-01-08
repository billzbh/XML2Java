#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "RemoteFunctions.h"
#include "iMateExt.h"

static uchar sg_sResetData[100], sg_ucResetDataLen = 0;
static uchar sg_ucCardReaderType = 0; //0：芯片读卡器；1：射频读卡器

extern int syncCommon(unsigned char *sendData, int sendLength, unsigned char *receivedData, int *receivedLength, int timeout);

// 设置IC读卡器类型，0：芯片读卡器；1：射频读卡器
void vSetCardReaderType(int iCardReaderType)
{
    sg_ucCardReaderType = (uchar)iCardReaderType;
}

void vSetCardResetData(uchar *psCardResetData, uint uiLen)
{
    memcpy(sg_sResetData, psCardResetData, uiLen);
    sg_ucResetDataLen = uiLen;
}

int uiExchangeApduDirect(unsigned int uiSlot, unsigned char *psIn, int iInLen, unsigned char* psOut, int *piOutLen)
{
    uchar sRequestDataBuffer[300], sResponseDataBuffer[300];
    int iRequestDataLength, iResponseDataLength;

    memcpy(sRequestDataBuffer,"\x62\x02",2);
    sRequestDataBuffer[2] = uiSlot;
    sRequestDataBuffer[3] = 0; //normal:0, pboc:1

    memcpy(sRequestDataBuffer+4,psIn,iInLen);
    iRequestDataLength = iInLen+4;

    int ret = syncCommon(sRequestDataBuffer, iRequestDataLength, sResponseDataBuffer, &iResponseDataLength, 8);

    if (ret || iResponseDataLength == 0) {
        return (1);
    }
    if (sResponseDataBuffer[0]) {
        return (ret);
    }

    memcpy(psOut, sResponseDataBuffer+1, iResponseDataLength-1);
    *piOutLen = iResponseDataLength-1;

    return (0);
}

// 检测卡片是否存在
// ret : 	0 : 不存在
// 			1 : 存在
int  iIMateTestCard(void)
{
    uchar sSerialNumbers[10];
    if (sg_ucCardReaderType == 1) {
        return _uiRcMifCard(sSerialNumbers);
    }

    if (iGetRemoteCallMode())
        return _uiRcTestCard(0);

    uchar sRequestDataBuffer[10], sResponseDataBuffer[200];
    int iRequestDataLength, iResponseDataLength;

    memcpy(sRequestDataBuffer,"\x62\x01",2);
    sRequestDataBuffer[2] = 0;
    sRequestDataBuffer[3] = 0; //timeout

    int ret = syncCommon(sRequestDataBuffer, 4, sResponseDataBuffer, &iResponseDataLength, 2);

    if (ret || iResponseDataLength == 0) {
        return (0);
    }
    if (sResponseDataBuffer[0]) {
        return (0);
    }
    vSetCardResetData(sResponseDataBuffer + 1, iResponseDataLength - 1);

    return 1;
}

// 卡片复位
// ret : <=0 : 复位错误
//       >0  : 复位成功, 返回值为ATR长度
int  iIMateResetCard(uchar *psResetData)
{
    if (sg_ucCardReaderType == 1) {
        int iRet = _uiRcMifActive();
        if (iRet)
            return 0;
        memcpy(psResetData, "\x3B\x8E\x80\x01\x80\x31\x80\x66\xB0\x84\x0C\x01\x6E\x01\x83\x00\x90\x00\x1D", 19);
        return 19;
    }
    if (iGetRemoteCallMode())
        return _uiRcResetCard(0, psResetData);
    memcpy(psResetData, sg_sResetData, sg_ucResetDataLen);
    return sg_ucResetDataLen;
}

// 关闭卡片
// ret : 不关心
int  iIMateCloseCard(void)
{
    if (sg_ucCardReaderType == 1) {
        return _uiRcMifClose();
    }
    //直接返回下电成功
    if (iGetRemoteCallMode())
        return _uiRcCloseCard(0);
    return 0;
}

// 执行APDU指令
// in  : iInLen   	: Apdu指令长度
// 		 pIn     	: Apdu指令, 格式: Cla Ins P1 P2 Lc DataIn Le
// out : piOutLen 	: Apdu应答长度
//       pOut    	: Apdu应答, 格式: DataOut Sw1 Sw2
// ret : 0          : 卡操作成功
//       1          : 卡操作错
int  iIMateExchangeApdu(int iInLen, uchar *pIn, int *piOutLen, uchar *pOut)
{
	uint uiRet;

	//vSetWriteLog(1);
	vWriteLogHex("ApduIn:", pIn, iInLen);

    if (sg_ucCardReaderType == 1) {
        uiRet = _uiRcMifApdu(pIn, (uint)iInLen, pOut, (uint*)piOutLen);
        if (uiRet == 0) {
        	vWriteLogHex("ApduOut:", pOut, *piOutLen);
        }
        else
        	vWriteLogTxt("ApduRet = %d", uiRet);
        return uiRet;
    }
    if (iGetRemoteCallMode()) {
        uiRet = _uiRcExchangeApduEx(0, 0/*Normal card*/, pIn, (uint)iInLen, pOut, (uint*)piOutLen);
        if (uiRet == 0) {
        	vWriteLogHex("ApduOut:", pOut, *piOutLen);
        }
        else
        	vWriteLogTxt("ApduRet = %d", uiRet);
        return uiRet;
    }
    else {
        uiRet = uiExchangeApduDirect(0, pIn, iInLen, pOut, piOutLen);
        if (uiRet == 0) {
        	vWriteLogHex("ApduOut:", pOut, *piOutLen);
        }
        else
        	vWriteLogTxt("ApduRet = %d", uiRet);
        return uiRet;
    }
}

