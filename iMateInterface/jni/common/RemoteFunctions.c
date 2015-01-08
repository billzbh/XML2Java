#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "RemoteFunctions.h"
#include "RemoteCall.h"

//检测IC卡是否插入
uint _uiRcTestCard(uint uiReader)
{
	uint uiDataLen, uiRet;

	vWriteLogTxt("_uiRcTestCard");
    
	char *psFunCall = malloc(200);
	uiDataLen = sprintf(psFunCall, "%u(%u)",FUNC__uiTestCard, uiReader);
    
	uiRet = iDoRemoteFunc((uchar*)psFunCall, uiDataLen, 1);
	free(psFunCall);
	if (uiRet) {
		vWriteLogTxt("_uiRcTestCard iDoRemoteFunc ret = %d", uiRet);
		return 0;
	}
	return iGetParaInt(0);
}

//IC卡复位
uint _uiRcResetCard(uint uiReader, uchar *pusResetData)
{
	uint uiDataLen, uiRet;

	char *psFunCall = malloc(200);
	uiDataLen = sprintf(psFunCall, "%u(%u,b000)",FUNC__uiResetCard, uiReader);

	uiRet = iDoRemoteFunc((uchar*)psFunCall, uiDataLen, 2);
	free(psFunCall);
	if (uiRet) {
		vWriteLogTxt("_uiRcResetCard iDoRemoteFunc ret = %d", uiRet);
		return 0;
	}
	uiRet = iGetParaInt(0);
	if (uiRet > 0) {
		memcpy(pusResetData, psGetParaBuf(3), uiRet);
	}
	return uiRet;
}

//IC卡下电
uint _uiRcCloseCard(uint uiReader)
{
	uint uiDataLen, uiRet;

	char *psFunCall = malloc(20);
	uiDataLen = sprintf(psFunCall, "%u(%u)",FUNC__uiCloseCard, uiReader);

	uiRet = iDoRemoteFunc((uchar*)psFunCall, uiDataLen, 1);
	free(psFunCall);
	if (uiRet) {
		vWriteLogTxt("_uiRcCloseCard iDoRemoteFunc ret = %d", uiRet);
		return 1;
	}
	uiRet = iGetParaInt(0);

	return uiRet;
}

//IC卡APDU
//返回码：
//          0         : 成功
//          1         : 失败
//          9         : iMate不支持远程调用
uint _uiRcExchangeApduEx(uint uiReader, uchar ucCardType, uchar *psApduIn, uint uiInLen, uchar *psApduOut, uint *puiOutLen)
{
	uint uiDataLen;
    int iRet;

	char *psFunCall = malloc(512);
	uiDataLen = sprintf(psFunCall, "%u(%u,%u,b%03u",FUNC__uiExchangeApduEx, uiReader, ucCardType, uiInLen);
	memcpy(psFunCall + uiDataLen, psApduIn, uiInLen);
	uiDataLen += uiInLen;
	uiDataLen += sprintf(psFunCall + uiDataLen, ",%u,b000,0)", uiInLen);

	iRet = iDoRemoteFunc((uchar*)psFunCall, uiDataLen, 8);
	free(psFunCall);
    if (iRet == -2)
        return 9;
	if (iRet) {
		vWriteLogTxt("_uiRcExchangeApdu iDoRemoteFunc ret = %d", iRet);
		return 1;
	}
	iRet = iGetParaInt(0);
	if (iRet != 0)
		return 1;

	uiDataLen = iGetParaInt(7);

	if (uiDataLen < 2)
		return 1;
	*puiOutLen = uiDataLen;
	memcpy(psApduOut, psGetParaBuf(6), uiDataLen);

	return (0);
}

uint _uiRcMagReset(void)
{
	uint uiDataLen, uiRet;

	char *psFunCall = malloc(200);
	uiDataLen = sprintf(psFunCall, "%u()",FUNC__uiMagReset);

	uiRet = iDoRemoteFunc((uchar*)psFunCall, uiDataLen, 1);
	free(psFunCall);
	if (uiRet) {
		vWriteLogTxt("_uiRcMagReset iDoRemoteFunc ret = %d", uiRet);
		return 1;
	}
	return iGetParaInt(0);
}

// 8.2. 检测是否有磁卡刷过
// 返    回：0 : 无卡
//      1 : 有磁卡刷过
uint _uiRcMagTest(void)
{
	uint uiDataLen, uiRet;
    
	char *psFunCall = malloc(200);
	uiDataLen = sprintf(psFunCall, "%u()",FUNC__uiMagTest);
    
	uiRet = iDoRemoteFunc((uchar*)psFunCall, uiDataLen, 1);
	free(psFunCall);
	if (uiRet) {
		vWriteLogTxt("_uiRcMagTest iDoRemoteFunc ret = %d", uiRet);
		return 0;
	}
	return iGetParaInt(0);
}

// 8.3. 读磁道信息
// 输入参数：uiTrackNo : 磁道号，1-3
// 输出参数：pszBuffer : 磁道信息
// 返    回：0 : 读取信息正确
//           1 : 没有数据
//           2 : 检测到错误
uint _uiRcMagGet(uint uiTrackNo, uchar *pszBuffer)
{
	uint uiDataLen, uiRet;

	char *psFunCall = malloc(200);
	uiDataLen = sprintf(psFunCall, "%u(%u,s000)",FUNC__uiMagGet, uiTrackNo);

	uiRet = iDoRemoteFunc((uchar*)psFunCall, uiDataLen, 1);
	free(psFunCall);
	if (uiRet) {
		vWriteLogTxt("_uiRcMagGet iDoRemoteFunc ret = %d", uiRet);
		return 1;
	}
	uiRet = iGetParaInt(0);
	if (uiRet == 0) {
		strcpy((char*)pszBuffer, (char*)psGetParaBuf(3));
	}
	return uiRet;
}

void _vRcBuzzer(void)
{
	uint uiDataLen, uiRet;

	char *psFunCall = malloc(20);
	uiDataLen = sprintf(psFunCall, "%u()",FUNC__vBuzzer);
	uiRet = iDoRemoteFunc((uchar*)psFunCall, uiDataLen, 1);
	if (uiRet) {
		vWriteLogTxt("_vRcBuzzer iDoRemoteFunc ret = %d", uiRet);
	}
}

// 9.1. 设置当前串口号
// 输入参数 ：ucPortNo : 新串口号, 从串口1开始
// 返    回：原先的串口号
uchar _ucRcAsyncSetPort(uchar ucPortNo)
{
	uint uiDataLen, uiRet;

	char *psFunCall = malloc(20);
	uiDataLen = sprintf(psFunCall, "%u(%u)",FUNC__ucAsyncSetPort, ucPortNo);

	uiRet = iDoRemoteFunc((uchar*)psFunCall, uiDataLen, 1);
	free(psFunCall);
	if (uiRet) {
		vWriteLogTxt("_ucRcAsyncSetPort iDoRemoteFunc ret = %d", uiRet);
		return ucPortNo;
	}
	return iGetParaInt(0);
}

// 9.2. 打开当前串口
// 输入参数：ulBaud   : 波特率
//           ucParity : 奇偶校验标志，COMM_EVEN、COMM_ODD、COMM_NONE
//           ucBits   : 数据位长度
//           ucStop   : 停止位长度
// 返    回：0        : 成功
//           1        : 参数错误
//           2        : 失败
uchar _ucRcAsyncOpen(ulong ulBaud, uchar ucParity, uchar ucBits, uchar ucStop)
{
	uint uiDataLen, uiRet;

	char *psFunCall = malloc(100);
	uiDataLen = sprintf(psFunCall, "%u(%lu,%u,%u,%u)",FUNC__ucAsyncOpen, ulBaud, ucParity, ucBits, ucStop);

	uiRet = iDoRemoteFunc((uchar*)psFunCall, uiDataLen, 1);
	free(psFunCall);
	if (uiRet) {
		vWriteLogTxt("_ucRcAsyncOpen iDoRemoteFunc ret = %d", uiRet);
		return 2;
	}
	return iGetParaInt(0);
}

// 9.3. 关闭当前串口
// 返    回：0        : 成功
//           1        : 失败
uchar _ucRcAsyncClose(void)
{
	uint uiDataLen, uiRet;

	char *psFunCall = malloc(20);
	uiDataLen = sprintf(psFunCall, "%u()",FUNC__ucAsyncClose);

	uiRet = iDoRemoteFunc((uchar*)psFunCall, uiDataLen, 1);
	free(psFunCall);
	if (uiRet) {
		vWriteLogTxt("_ucRcAsyncClose iDoRemoteFunc ret = %d", uiRet);
		return 1;
	}
	return iGetParaInt(0);
}

// 9.4. 复位当前串口，清空接收缓冲区
// 返    回：0        : 成功
//           1        : 失败
uchar _ucRcAsyncReset(void)
{
	uint uiDataLen, uiRet;

	char *psFunCall = malloc(20);
	uiDataLen = sprintf(psFunCall, "%u()",FUNC__ucAsyncReset);

	uiRet = iDoRemoteFunc((uchar*)psFunCall, uiDataLen, 1);
	free(psFunCall);
	if (uiRet) {
		vWriteLogTxt("_ucRcAsyncReset iDoRemoteFunc ret = %d", uiRet);
		return 1;
	}
	return iGetParaInt(0);
}

// 9.5. 发送一个字节
// 输入参数：ucChar : 要发送的字符
// 返    回：0      : 成功
//           1      : 失败
uchar _ucRcAsyncSend(uchar ucChar)
{
	uint uiDataLen, uiRet;

	char *psFunCall = malloc(20);
	uiDataLen = sprintf(psFunCall, "%u(%u)",FUNC__ucAsyncSend, ucChar);

	uiRet = iDoRemoteFunc((uchar*)psFunCall, uiDataLen, 1);
	free(psFunCall);
	if (uiRet) {
		vWriteLogTxt("_ucRcAsyncSend iDoRemoteFunc ret = %d", uiRet);
		return 1;
	}
	return iGetParaInt(0);
}

// 9.6. 检测有没有字符收到
// 返    回：0 : 没有字符收到
//           1 : 有字符收到
uchar _ucRcAsyncTest(void)
{
	uint uiDataLen, uiRet;

	char *psFunCall = malloc(20);
	uiDataLen = sprintf(psFunCall, "%u()",FUNC__ucAsyncTest);

	uiRet = iDoRemoteFunc((uchar*)psFunCall, uiDataLen, 1);
	free(psFunCall);
	if (uiRet) {
		vWriteLogTxt("_ucRcAsyncTest iDoRemoteFunc ret = %d", uiRet);
		return 0;
	}
	return iGetParaInt(0);
}

// 9.7. 接收一个字符，没有则一直等待
// 返    回：接收到的字符
uchar _ucRcAsyncGet(void)
{
	uint uiDataLen, uiRet;

	char *psFunCall = malloc(20);
	uiDataLen = sprintf(psFunCall, "%u()",FUNC__ucAsyncGet);

	uiRet = iDoRemoteFunc((uchar*)psFunCall, uiDataLen, 1);
	free(psFunCall);
	if (uiRet) {
		vWriteLogTxt("_ucRcAsyncGet iDoRemoteFunc ret = %d", uiRet);
		return 1;
	}
	return iGetParaInt(0);
}

// 9.8.	发送一串数据
// 输入参数：pBuf  : 要发送的字符
//           uiLen : 数据长度
// 返    回：0     : 成功
//           1     : 失败
uchar _ucRcAsyncSendBuf(void *pBuf, uint uiLen)
{
	uint uiDataLen, uiRet;

	char *psFunCall = malloc(1024);
	uiDataLen = sprintf(psFunCall, "%u(b%03u",FUNC__ucAsyncSendBuf, uiLen);
	memcpy(psFunCall + uiDataLen, pBuf, uiLen);
	uiDataLen += uiLen;
	uiDataLen += sprintf(psFunCall + uiDataLen, ",%u)", uiLen);

	uiRet = iDoRemoteFunc((uchar*)psFunCall, uiDataLen, 1);
	free(psFunCall);
	if (uiRet) {
		vWriteLogTxt("_ucRcAsyncSendBuf iDoRemoteFunc ret = %d", uiRet);
		return 1;
	}
	return iGetParaInt(0);
}

// 9.9.	接收指定长度的一串数据
// 输入参数：pBuf      : 要发送的数据
//           uiLen     : 数据长度
//           uiTimeOut : 以秒为单位的超时时间
// 返    回：0         : 成功
//           1         : 超时
uchar _ucRcAsyncGetBuf(void *pBuf, uint uiLen, uint uiTimeOut)
{
	uint uiDataLen, uiRet;

	char *psFunCall = malloc(100);
	uiDataLen = sprintf(psFunCall, "%u(b000,%u,%u)",FUNC__ucAsyncGetBuf, uiLen, uiTimeOut);

	uiRet = iDoRemoteFunc((uchar*)psFunCall, uiDataLen, uiTimeOut + 1);
	free(psFunCall);
	if (uiRet) {
		vWriteLogTxt("_ucRcAsyncGetBuf iDoRemoteFunc ret = %d", uiRet);
		return 1;
	}
	memcpy(pBuf, psGetParaBuf(2), uiLen);
	return iGetParaInt(0);
}

// 从iMate XMem中读取数据
// 输入参数：pBuf        : 读取数据缓冲区
//          uiOffset    : 数据偏移量
//          uiLen       : 数据长度
// 返    回：0           : 成功
//           1          : 失败
uchar _ucRcXMemRead(void *pBuf, uint uiOffset, uint uiLen)
{
	uint uiDataLen, uiRet;

	char *psFunCall = malloc(100+uiLen);
	uiDataLen = sprintf(psFunCall, "%u(b000,%u,%u)",FUNC__uiXMemRead, uiOffset, uiLen);

	uiRet = iDoRemoteFunc((uchar*)psFunCall, uiDataLen, 2);
	free(psFunCall);
	if (uiRet) {
		vWriteLogTxt("_ucRcXMemRead iDoRemoteFunc ret = %d", uiRet);
		return 1;
	}
    if (iGetParaInt(0) == 0)
        memcpy(pBuf, psGetParaBuf(2), uiLen);
	return iGetParaInt(0);
}

// 向iMate XMem中写数据
// 输入参数：pBuf        : 数据缓冲区
//          uiOffset    : 数据偏移量
//          uiLen       : 数据长度
// 返    回：0           : 成功
//           1          : 失败
uchar _ucRcXMemWrite(void *pBuf, uint uiOffset, uint uiLen)
{
	uint uiDataLen, uiRet;

	char *psFunCall = malloc(100 + uiLen);
	uiDataLen = sprintf(psFunCall, "%u(b%03u", FUNC__uiXMemWrite, uiLen);
    memcpy(psFunCall + uiDataLen, pBuf, uiLen);
    uiDataLen += uiLen;
    uiDataLen += sprintf(psFunCall + uiDataLen, ",%u,%u)", uiOffset, uiLen);

	uiRet = iDoRemoteFunc((uchar*)psFunCall, uiDataLen, 2);
	free(psFunCall);
	if (uiRet) {
		vWriteLogTxt("_ucRcXMemWrite iDoRemoteFunc ret = %d", uiRet);
		return 1;
	}
	return iGetParaInt(0);
}

// 从iMate XMem系统保留区中读取数据
// 输入参数：pBuf        : 读取数据缓冲区
//          uiOffset    : 数据偏移量
//          uiLen       : 数据长度
// 返    回：0           : 成功
//           1          : 失败
uchar _ucRcXMemReadReserved(void *pBuf, uint uiOffset, uint uiLen)
{
	uint uiDataLen, uiRet;

	char *psFunCall = malloc(100+uiLen);
	uiDataLen = sprintf(psFunCall, "%u(b000,%u,%u)",FUNC__uiXMemReadReserved, uiOffset, uiLen);

	uiRet = iDoRemoteFunc((uchar*)psFunCall, uiDataLen, 2);
	free(psFunCall);
	if (uiRet) {
		vWriteLogTxt("_ucRcXMemReadReserved iDoRemoteFunc ret = %d", uiRet);
		return 1;
	}
    if (iGetParaInt(0) == 0)
        memcpy(pBuf, psGetParaBuf(2), uiLen);
	return iGetParaInt(0);
}

// 向iMate XMem系统保留区中写数据
// 输入参数：pBuf        : 数据缓冲区
//          uiOffset    : 数据偏移量
//          uiLen       : 数据长度
// 返    回：0           : 成功
//           1          : 失败
uchar _ucRcXMemWriteReserved(void *pBuf, uint uiOffset, uint uiLen)
{
	uint uiDataLen, uiRet;

	char *psFunCall = malloc(100 + uiLen);
	uiDataLen = sprintf(psFunCall, "%u(b%03u", FUNC__uiXMemWriteReserved, uiLen);
    memcpy(psFunCall + uiDataLen, pBuf, uiLen);
    uiDataLen += uiLen;
    uiDataLen += sprintf(psFunCall + uiDataLen, ",%u,%u)", uiOffset, uiLen);

	uiRet = iDoRemoteFunc((uchar*)psFunCall, uiDataLen, 2);
	free(psFunCall);
	if (uiRet) {
		vWriteLogTxt("_ucRcXMemWriteReserved iDoRemoteFunc ret = %d", uiRet);
		return 1;
	}
	return iGetParaInt(0);
}

void _vRcSetLed(uint uiLedNo, uchar ucOnOff)
{
	uint uiDataLen, uiRet;

	vWriteLogTxt("_vRcSetLed");

	char *psFunCall = malloc(200);
	uiDataLen = sprintf(psFunCall, "%u(%u,%u)",FUNC_vSetLed, uiLedNo, ucOnOff);

	uiRet = iDoRemoteFunc((uchar*)psFunCall, uiDataLen, 1);
	free(psFunCall);
	if (uiRet) {
		vWriteLogTxt("_vRcSetLed iDoRemoteFunc ret = %d", uiRet);
	}
}

void _vRcSetUart(uint uiPortNo)
{
	uint uiDataLen, uiRet;

	vWriteLogTxt("_vRcSetUart");

	char *psFunCall = malloc(200);
	uiDataLen = sprintf(psFunCall, "%u(%u)",FUNC_vSetUart, uiPortNo);

	uiRet = iDoRemoteFunc((uchar*)psFunCall, uiDataLen, 1);
	free(psFunCall);
	if (uiRet) {
		vWriteLogTxt("_vRcSetUart iDoRemoteFunc ret = %d", uiRet);
	}
}

// 检测射频卡
// 输出参数：psSerialNo : 返回卡片系列号
// 返    回：>0         : 成功, 卡片系列号字节数
//           0          : 失败
uint _uiRcMifCard(uchar *psSerialNo)
{
	uint uiDataLen, uiRet;
    
	char *psFunCall = malloc(20);
	uiDataLen = sprintf(psFunCall, "%u(b000)",FUNC_uiMifCard);
    
	uiRet = iDoRemoteFunc((uchar*)psFunCall, uiDataLen, 1);
	free(psFunCall);
	if (uiRet) {
		vWriteLogTxt("_uiRcMifCard iDoRemoteFunc ret = %d", uiRet);
		return 0;
	}
	uiRet = iGetParaInt(0);
	if (uiRet > 0)
		memcpy(psSerialNo, psGetParaBuf(2), uiRet);
    
	return uiRet;
}

// MIF CPU卡激活
// 返    回：0          : 成功
//           其它       : 失败
uint _uiRcMifActive(void)
{
	uint uiDataLen, uiRet;

	char *psFunCall = malloc(20);
	uiDataLen = sprintf(psFunCall, "%u()",FUNC_uiMifActive);
	uiRet = iDoRemoteFunc((uchar*)psFunCall, uiDataLen, 1);
	free(psFunCall);
	if (uiRet) {
		vWriteLogTxt("_uiRcMifActive iDoRemoteFunc ret = %d", uiRet);
		return 1;
	}
	return iGetParaInt(0);
}

// 关闭射频信号
// 返    回：0			；成功
//   		 其它		：失败
uint _uiRcMifClose(void)
{
	uint uiDataLen, uiRet;

	char *psFunCall = malloc(20);
	uiDataLen = sprintf(psFunCall, "%u()",FUNC_uiMifClose);
	uiRet = iDoRemoteFunc((uchar*)psFunCall, uiDataLen, 1);
	free(psFunCall);
	if (uiRet) {
		vWriteLogTxt("_uiRcMifClose iDoRemoteFunc ret = %d", uiRet);
		return 1;
	}
	return iGetParaInt(0);
}

// MIF移除
// 返    回：0          : 移除
//           其它       : 未移除
uint _uiRcMifRemoved(void)
{
	uint uiDataLen, uiRet;

	char *psFunCall = malloc(20);
	uiDataLen = sprintf(psFunCall, "%u()",FUNC_uiMifRemoved);
	uiRet = iDoRemoteFunc((uchar*)psFunCall, uiDataLen, 1);
	free(psFunCall);
	if (uiRet) {
		vWriteLogTxt("_uiRcMifRemoved iDoRemoteFunc ret = %d", uiRet);
		return 1;
	}
	return iGetParaInt(0);
}

// M1卡扇区认证
// 输入参数：  ucSecNo	：扇区号
//			 ucKeyAB	：密钥类型，0x00：A密码，0x04: B密码
//			 psKey		: 6字节的密钥
// 返    回：0          : 成功
//           其它       : 失败
uint _uiRcMifAuth(uchar ucSecNo, uchar ucKeyAB, uchar *psKey)
{
	uint uiDataLen, uiRet;

	char *psFunCall = malloc(100);
	uiDataLen = sprintf(psFunCall, "%u(%u,%u,b006",FUNC_uiMifAuth, ucSecNo, ucKeyAB);
	memcpy(psFunCall + uiDataLen, psKey, 6);
	uiDataLen += 6;
	uiDataLen += sprintf(psFunCall + uiDataLen, ")");

	uiRet = iDoRemoteFunc((uchar*)psFunCall, uiDataLen, 1);
	free(psFunCall);
	if (uiRet) {
		vWriteLogTxt("_uiRcMifAuth iDoRemoteFunc ret = %d", uiRet);
		return 1;
	}
	return iGetParaInt(0);
}

// M1卡读数据块
// 输入参数：  ucSecNo	：扇区号
//			 ucBlock	: 块号
// 输出参数：psData		：16字节的数据
// 返    回：0          : 成功
//           其它       : 失败
uint _uiRcMifReadBlock(uchar ucSecNo, uchar ucBlock, uchar *psData)
{
	uint uiDataLen, uiRet;

	char *psFunCall = malloc(100);
	uiDataLen = sprintf(psFunCall, "%u(%u,%u,b000)",FUNC_uiMifReadBlock, ucSecNo, ucBlock);
	uiRet = iDoRemoteFunc((uchar*)psFunCall, uiDataLen, 1);
	free(psFunCall);
	if (uiRet) {
		vWriteLogTxt("_uiRcMifReadBlock iDoRemoteFunc ret = %d", uiRet);
		return 1;
	}
	uiRet = iGetParaInt(0);
	if (uiRet == 0)
		memcpy(psData, psGetParaBuf(4), 16);

	return uiRet;
}

// M1卡写数据块
// 输入参数：  ucSecNo	：扇区号
//			 ucBlock	: 块号
//			 psData		：16字节的数据
// 返    回：0          : 成功
//           其它       : 失败
uint _uiRcMifWriteBlock(uchar ucSecNo, uchar ucBlock, uchar *psData)
{
	uint uiDataLen, uiRet;

	char *psFunCall = malloc(100);
	uiDataLen = sprintf(psFunCall, "%u(%u,%u,b016",FUNC_uiMifWriteBlock, ucSecNo, ucBlock);
	memcpy(psFunCall + uiDataLen, psData, 16);
	uiDataLen += 16;
	uiDataLen += sprintf(psFunCall + uiDataLen, ")");

	uiRet = iDoRemoteFunc((uchar*)psFunCall, uiDataLen, 1);
	free(psFunCall);
	if (uiRet) {
		vWriteLogTxt("_uiRcMifWriteBlock iDoRemoteFunc ret = %d", uiRet);
		return 1;
	}
	return iGetParaInt(0);
}


// M1钱包加值
// 输入参数：  ucSecNo	：扇区号
//			 ucBlock	: 块号
//			 ulValue	：值
// 返    回：0          : 成功
//           其它       : 失败
uint _uiRcMifIncrement(uchar ucSecNo,uchar ucBlock,ulong ulValue)
{
	uint uiDataLen, uiRet;

	char *psFunCall = malloc(50);
	uiDataLen = sprintf(psFunCall, "%u(%u,%u,%lu)",FUNC_uiMifIncrement, ucSecNo, ucBlock, ulValue);
	uiRet = iDoRemoteFunc((uchar*)psFunCall, uiDataLen, 1);
	free(psFunCall);
	if (uiRet) {
		vWriteLogTxt("_uiRcMifIncrement iDoRemoteFunc ret = %d", uiRet);
		return 1;
	}
	return iGetParaInt(0);
}

// M1钱包减值
// 输入参数：  ucSecNo	：扇区号
//			 ucBlock	: 块号
//			 ulValue	：值
// 返    回：0          : 成功
//           其它       : 失败
uint _uiRcMifDecrement(uchar ucSecNo,uchar ucBlock,ulong ulValue)
{
	uint uiDataLen, uiRet;

	char *psFunCall = malloc(50);
	uiDataLen = sprintf(psFunCall, "%u(%u,%u,%lu)",FUNC_uiMifDecrement, ucSecNo, ucBlock, ulValue);
	uiRet = iDoRemoteFunc((uchar*)psFunCall, uiDataLen, 1);
	free(psFunCall);
	if (uiRet) {
		vWriteLogTxt("_uiRcMifDecrement iDoRemoteFunc ret = %d", uiRet);
		return 1;
	}
	return iGetParaInt(0);
}

// M1卡块拷贝
// 输入参数： ucSrcSecNo	：源扇区号
//			 ucSrcBlock	: 源块号
//			 ucDesSecNo	: 目的扇区号
//			 ucDesBlock	: 目的块号
// 返    回：0          : 成功
//           其它       : 失败
uint _uiRcMifCopy(uchar ucSrcSecNo, uchar ucSrcBlock, uchar ucDesSecNo, uchar ucDesBlock)
{
	uint uiDataLen, uiRet;

	char *psFunCall = malloc(50);
	uiDataLen = sprintf(psFunCall, "%u(%u,%u,%u,%u)",FUNC_uiMifCopy, ucSrcSecNo, ucSrcBlock, ucDesSecNo, ucDesBlock);
	uiRet = iDoRemoteFunc((uchar*)psFunCall, uiDataLen, 1);
	free(psFunCall);
	if (uiRet) {
		vWriteLogTxt("_uiRcMifCopy iDoRemoteFunc ret = %d", uiRet);
		return 1;
	}
	return iGetParaInt(0);
}

// MIF CPU 卡 APDU
// 输入参数：psApduIn	：apdu命令串
//			 uiInLen	: apdu命令串长度
//			 psApduOut	: apdu返回串
//			 puiOutLen	: apdu返回串长度
// 返    回：0          : 成功
//           其它       : 失败
uint _uiRcMifApdu(uchar *psApduIn, uint uiInLen, uchar *psApduOut, uint *puiOutLen)
{
	uint uiDataLen, uiRet;

	char *psFunCall = malloc(1024);
	uiDataLen = sprintf(psFunCall, "%u(b%03u", FUNC_uiMifApdu, uiInLen);
	memcpy(psFunCall + uiDataLen, psApduIn, uiInLen);
	uiDataLen += uiInLen;
	uiDataLen += sprintf(psFunCall + uiDataLen, ",%u,b000,0)", uiInLen);

	uiRet = iDoRemoteFunc((uchar*)psFunCall, uiDataLen, 1);
	free(psFunCall);
	if (uiRet) {
		vWriteLogTxt("_uiRcMifApdu iDoRemoteFunc ret = %d", uiRet);
		return 1;
	}
	uiRet = iGetParaInt(0);
	if (uiRet == 0) {
		*puiOutLen = iGetParaInt(5);
		memcpy(psApduOut, psGetParaBuf(4), *puiOutLen);
	}
	return uiRet;

}

uint _uiRcSetMemoryCardCode(uchar ucCardType, uchar ucOffset, uchar ucLen, uchar *sCardCode)
{
	uint uiDataLen, uiRet;

	char *psFunCall = malloc(200);
	uiDataLen = sprintf(psFunCall, "%u(%u,%u,%u,b%03u",FUNC_uiSetMemoryCardCode, ucCardType, ucOffset, ucLen, ucLen);
	memcpy(psFunCall + uiDataLen, sCardCode, ucLen);
	uiDataLen += ucLen;
	psFunCall[uiDataLen++] = ')';
	uiRet = iDoRemoteFunc((uchar*)psFunCall, uiDataLen, 1);
	free(psFunCall);
	if (uiRet) {
		vWriteLogTxt("_uiRcSetMemoryCardCode iDoRemoteFunc ret = %d", uiRet);
		return 1;
	}
	return iGetParaInt(0);
}

// 功能：检测存储卡类型(4442\102\1604\1608)
// 输入：ucSlot		卡座号 0~8
// 输出: psResetData 复位数据 < 100bytes
// 返回: 0xff		不识别的卡
//		其它
//					SLE4442_TYPE		0x01
//					AT102_TYPE			0x02
//					AT1608_TYPE			0x04
//					AT1604_TYPE			0x05
//					SLE4428_TYPE		0x06
//					AT24Cxx_TYPE		0x10
uint _uiRcTestMemoryCardType(uchar *psResetData)
{
	uint uiDataLen, uiRet;

	char *psFunCall = malloc(200);
	uiDataLen = sprintf(psFunCall, "%u(b000)",FUNC_uiTestMemoryCardType);
	uiRet = iDoRemoteFunc((uchar*)psFunCall, uiDataLen, 2);
	free(psFunCall);
	if (uiRet) {
		vWriteLogTxt("_uiRcTestMemoryCardType iDoRemoteFunc ret = %d", uiRet);
		return 1;
	}
	uiRet = iGetParaInt(0);
	if (uiRet != 0xff) {
		uint uiLen = psGetParaBuf(2)[0];
		memcpy(psResetData, psGetParaBuf(2), uiLen + 1);
	}
	return uiRet;
}

//检测指定存储卡类型
//返回:
//输入: ucCardType 指定的卡类型，
//					SLE4442_TYPE		0x01
//					AT102_TYPE			0x02
//					AT1608_TYPE			0x04
//					AT1604_TYPE1		0x05
//					AT24Cxx_TYPE		0x10
//输出: sResetData 复位数据 < 100bytes
//返回：0x00  	   卡类型正确
//		0xff  	   不识别的卡
uint _uiRcTestMemorySpecifiedType(uchar ucCardType,uchar *sResetData)
{
	uint uiDataLen, uiRet;

	char *psFunCall = malloc(30);
	uiDataLen = sprintf(psFunCall, "%u(%u,b000)",FUNC_uiTestMemorySpecifiedType, ucCardType);
	uiRet = iDoRemoteFunc((uchar*)psFunCall, uiDataLen, 2);
	free(psFunCall);
	if (uiRet) {
		vWriteLogTxt("_uiRcTestMemorySpecifiedType iDoRemoteFunc ret = %d", uiRet);
		return 1;
	}
	uiRet = iGetParaInt(0);
	if (uiRet != 0xff) {
		uint uiLen = psGetParaBuf(3)[0];
		memcpy(sResetData, psGetParaBuf(3), uiLen + 1);
	}
	return uiRet;
}

// 设置读卡电压
// 输入参数： uiVoltage 	:	CARD_VOLTAGE_1V8		0
//							CARD_VOLTAGE_3V0		1
//							CARD_VOLTAGE_5V0		2
void _vRcSetCardVoltage(uint uiVoltage)
{
	uint uiDataLen, uiRet;

	char *psFunCall = malloc(50);
	uiDataLen = sprintf(psFunCall, "%u(%u)",FUNC_vSetCardVoltage, uiVoltage);
	uiRet = iDoRemoteFunc((uchar*)psFunCall, uiDataLen, 1);
	free(psFunCall);
	if (uiRet) {
		vWriteLogTxt("vSetCardVoltage iDoRemoteFunc ret = %d", uiRet);
	}
}

// 存储卡函数实现
//memory card 4442
void _vRcSLE4442_Open(void)
{
	uint uiDataLen, uiRet;

	char *psFunCall = malloc(50);

	uiDataLen = sprintf(psFunCall, "%u()",FUNC_SLE4442_Open);
	uiRet = iDoRemoteFunc((uchar*)psFunCall, uiDataLen, 1);
	free(psFunCall);
	if (uiRet) {
		vWriteLogTxt("_SLE4442_Open iDoRemoteFunc ret = %d", uiRet);
	}
}
void _vRcSLE4442_Close(void)
{
	uint uiDataLen, uiRet;

	char *psFunCall = malloc(50);
	uiDataLen = sprintf(psFunCall, "%u()",FUNC_SLE4442_Close);
	uiRet = iDoRemoteFunc((uchar*)psFunCall, uiDataLen, 1);
	free(psFunCall);
	if (uiRet) {
		vWriteLogTxt("_SLE4442_Close iDoRemoteFunc ret = %d", uiRet);
	}
}
//返回0成功
uchar _ucRcSLE4442_ChkCode(uchar *sSecurityCode)
{
	uint uiDataLen, uiRet;

	char *psFunCall = malloc(50);
	uiDataLen = sprintf(psFunCall, "%u(b003",FUNC_SLE4442_ChkCode);
	memcpy(psFunCall + uiDataLen, sSecurityCode, 3);
	uiDataLen += 3;
	uiDataLen += sprintf(psFunCall + uiDataLen,")");
	uiRet = iDoRemoteFunc((uchar*)psFunCall, uiDataLen, 1);
	free(psFunCall);
	if (uiRet) {
		vWriteLogTxt("_SLE4442_ChkCode iDoRemoteFunc ret = %d", uiRet);
		return 1;
	}
	return iGetParaInt(0);
}

uchar _ucRcSLE4442_ChkCodeEx(uchar *sSecurityCode)
{
	uint uiDataLen, uiRet;

	char *psFunCall = malloc(50);
	uiDataLen = sprintf(psFunCall, "%u(b008",FUNC_SLE4442_ChkCodeEx);
	memcpy(psFunCall + uiDataLen, sSecurityCode, 8);
	uiDataLen += 8;
	uiDataLen += sprintf(psFunCall + uiDataLen,")");
	uiRet = iDoRemoteFunc((uchar*)psFunCall, uiDataLen, 2);
	free(psFunCall);
	if (uiRet) {
		vWriteLogTxt("_SLE4442_ChkCodeEx iDoRemoteFunc ret = %d", uiRet);
		return 1;
	}
	return iGetParaInt(0);
}

void _vRcSLE4442_Read(uchar Addr, int DataLen, uchar* DataBuff)
{
	uint uiDataLen, uiRet;

	char *psFunCall = malloc(50);
	uiDataLen = sprintf(psFunCall, "%u(%u,%u,b000)",FUNC_SLE4442_Read, Addr, DataLen);
	uiRet = iDoRemoteFunc((uchar*)psFunCall, uiDataLen, 5);
	free(psFunCall);
	if (uiRet) {
		vWriteLogTxt("_SLE4442_Read iDoRemoteFunc ret = %d", uiRet);
		return;
	}
	memcpy(DataBuff, psGetParaBuf(4), DataLen);
}

void _vRcSLE4442_Write(uchar Addr,int DataLen,uchar* DataBuff)
{
	uint uiDataLen, uiRet;

	char *psFunCall = malloc(512);
	uiDataLen = sprintf(psFunCall, "%u(%u,%u,b%03u",FUNC_SLE4442_Write, Addr, DataLen, DataLen);
	memcpy(psFunCall + uiDataLen, DataBuff, DataLen);
	uiDataLen += DataLen;
	uiDataLen += sprintf(psFunCall + uiDataLen, ")");

	uiRet = iDoRemoteFunc((uchar*)psFunCall, uiDataLen, 5);
	free(psFunCall);
	if (uiRet) {
		vWriteLogTxt("_SLE4442_Write iDoRemoteFunc ret = %d", uiRet);
	}
}

void _vRcSLE4442_PrRead(uchar Addr, int DataLen, uchar* DataBuff)
{
	uint uiDataLen, uiRet;

	char *psFunCall = malloc(50);
	uiDataLen = sprintf(psFunCall, "%u(%u,%u,b000)",FUNC_SLE4442_PrRead, Addr, DataLen);
	uiRet = iDoRemoteFunc((uchar*)psFunCall, uiDataLen, 1);
	free(psFunCall);
	if (uiRet) {
		vWriteLogTxt("_SLE4442_PrRead iDoRemoteFunc ret = %d", uiRet);
		return;
	}
	memcpy(DataBuff, psGetParaBuf(4), DataLen);
}

void _vRcSLE4442_PrWrite(uchar Addr,int DataLen,uchar* DataBuff)
{
	uint uiDataLen, uiRet;

	char *psFunCall = malloc(512);
	uiDataLen = sprintf(psFunCall, "%u(%u,%u,b%03u",FUNC_SLE4442_PrWrite, Addr, DataLen, DataLen);
	memcpy(psFunCall + uiDataLen, DataBuff, DataLen);
	uiDataLen += DataLen;
	uiDataLen += sprintf(psFunCall + uiDataLen, ")");

	uiRet = iDoRemoteFunc((uchar*)psFunCall, uiDataLen, 1);
	free(psFunCall);
	if (uiRet) {
		vWriteLogTxt("_SLE4442_PrWrite iDoRemoteFunc ret = %d", uiRet);
	}
}

//memory card 4442
void _vRcSLE4428_Open(void)
{
	uint uiDataLen, uiRet;

	char *psFunCall = malloc(50);

	uiDataLen = sprintf(psFunCall, "%u()",FUNC_SLE4428_Open);
	uiRet = iDoRemoteFunc((uchar*)psFunCall, uiDataLen, 1);
	free(psFunCall);
	if (uiRet) {
		vWriteLogTxt("_SLE4428_Open iDoRemoteFunc ret = %d", uiRet);
	}
}
void _vRcSLE4428_Close(void)
{
	uint uiDataLen, uiRet;

	char *psFunCall = malloc(50);
	uiDataLen = sprintf(psFunCall, "%u()",FUNC_SLE4428_Close);
	uiRet = iDoRemoteFunc((uchar*)psFunCall, uiDataLen, 1);
	free(psFunCall);
	if (uiRet) {
		vWriteLogTxt("_SLE4428_Close iDoRemoteFunc ret = %d", uiRet);
	}
}
//返回0成功
uchar _ucRcSLE4428_ChkPSC(uchar *sSecurityCode)
{
	uint uiDataLen, uiRet;

	char *psFunCall = malloc(50);
	uiDataLen = sprintf(psFunCall, "%u(b002",FUNC_SLE4428_ChkPSC);
	memcpy(psFunCall + uiDataLen, sSecurityCode, 2);
	uiDataLen += 2;
	uiDataLen += sprintf(psFunCall + uiDataLen,")");
	uiRet = iDoRemoteFunc((uchar*)psFunCall, uiDataLen, 1);
	free(psFunCall);
	if (uiRet) {
		vWriteLogTxt("_SLE4428_ChkCode iDoRemoteFunc ret = %d", uiRet);
		return 1;
	}
	return iGetParaInt(0);
}

uchar _ucRcSLE4428_ChkPSCEx(uchar *sSecurityCode)
{
	uint uiDataLen, uiRet;

	char *psFunCall = malloc(50);
	uiDataLen = sprintf(psFunCall, "%u(b008",FUNC_SLE4428_ChkPSCEx);
	memcpy(psFunCall + uiDataLen, sSecurityCode, 8);
	uiDataLen += 8;
	uiDataLen += sprintf(psFunCall + uiDataLen,")");
	uiRet = iDoRemoteFunc((uchar*)psFunCall, uiDataLen, 2);
	free(psFunCall);
	if (uiRet) {
		vWriteLogTxt("_SLE4428_ChkCodeEx iDoRemoteFunc ret = %d", uiRet);
		return 1;
	}
	return iGetParaInt(0);
}

void _vRcSLE4428_UpdPSC(uchar *sSecurityCode)
{
	uint uiDataLen, uiRet;

	char *psFunCall = malloc(50);
	uiDataLen = sprintf(psFunCall, "%u(b002",FUNC_SLE4428_UpdPSC);
	memcpy(psFunCall + uiDataLen, sSecurityCode, 2);
	uiDataLen += 2;
	uiDataLen += sprintf(psFunCall + uiDataLen, ")");

	uiRet = iDoRemoteFunc((uchar*)psFunCall, uiDataLen, 1);
	free(psFunCall);
	if (uiRet) {
		vWriteLogTxt("_SLE4428_UpdPSC iDoRemoteFunc ret = %d", uiRet);
	}
}

void _vRcSLE4428_Read(uchar Addr, int DataLen, uchar* DataBuff)
{
	uint uiDataLen, uiRet;

	char *psFunCall = malloc(50);
	uiDataLen = sprintf(psFunCall, "%u(%u,%u,b000)",FUNC_SLE4428_Read, Addr, DataLen);
	uiRet = iDoRemoteFunc((uchar*)psFunCall, uiDataLen, 5);
	free(psFunCall);
	if (uiRet) {
		vWriteLogTxt("_SLE4428_Read iDoRemoteFunc ret = %d", uiRet);
		return;
	}
	memcpy(DataBuff, psGetParaBuf(4), DataLen);
}

void _vRcSLE4428_Write(uchar Addr,int DataLen,uchar* DataBuff)
{
	uint uiDataLen, uiRet;

	char *psFunCall = malloc(512);
	uiDataLen = sprintf(psFunCall, "%u(%u,%u,p%04u",FUNC_SLE4428_Write, Addr, DataLen, DataLen);
	memcpy(psFunCall + uiDataLen, DataBuff, DataLen);
	uiDataLen += DataLen;
	uiDataLen += sprintf(psFunCall + uiDataLen, ")");

	uiRet = iDoRemoteFunc((uchar*)psFunCall, uiDataLen, 8);
	free(psFunCall);
	if (uiRet) {
		vWriteLogTxt("_SLE4428_Write iDoRemoteFunc ret = %d", uiRet);
	}
}

void _vRcSLE4428_PrRead(uchar Addr, int DataLen, uchar* DataBuff)
{
	uint uiDataLen, uiRet;

	char *psFunCall = malloc(50);
	uiDataLen = sprintf(psFunCall, "%u(%u,%u,b000)",FUNC_SLE4428_PrRead, Addr, DataLen);
	uiRet = iDoRemoteFunc((uchar*)psFunCall, uiDataLen, 1);
	free(psFunCall);
	if (uiRet) {
		vWriteLogTxt("_SLE4428_PrRead iDoRemoteFunc ret = %d", uiRet);
		return;
	}
	memcpy(DataBuff, psGetParaBuf(4), DataLen);
}

void _vRcSLE4428_PrWrite(uchar Addr,int DataLen,uchar* DataBuff)
{
	uint uiDataLen, uiRet;

	char *psFunCall = malloc(512);
	uiDataLen = sprintf(psFunCall, "%u(%u,%u,b%03u",FUNC_SLE4428_PrWrite, Addr, DataLen, DataLen);
	memcpy(psFunCall + uiDataLen, DataBuff, DataLen);
	uiDataLen += DataLen;
	uiDataLen += sprintf(psFunCall + uiDataLen, ")");

	uiRet = iDoRemoteFunc((uchar*)psFunCall, uiDataLen, 2);
	free(psFunCall);
	if (uiRet) {
		vWriteLogTxt("_SLE4428_PrWrite iDoRemoteFunc ret = %d", uiRet);
	}
}

//memory card at102
void _vRcAT102_OpenCard(void)
{
	uint uiDataLen, uiRet;

	char *psFunCall = malloc(50);
	uiDataLen = sprintf(psFunCall, "%u()",FUNC_AT102_OpenCard);
	uiRet = iDoRemoteFunc((uchar*)psFunCall, uiDataLen, 1);
	free(psFunCall);
	if (uiRet) {
		vWriteLogTxt("_AT102_OpenCard iDoRemoteFunc ret = %d", uiRet);
	}
}
void _vRcAT102_CloseCard(void)
{
	uint uiDataLen, uiRet;

	char *psFunCall = malloc(50);
	uiDataLen = sprintf(psFunCall, "%u()",FUNC_AT102_CloseCard);
	uiRet = iDoRemoteFunc((uchar*)psFunCall, uiDataLen, 1);
	free(psFunCall);
	if (uiRet) {
		vWriteLogTxt("_AT102_CloseCard iDoRemoteFunc ret = %d", uiRet);
	}
}

uchar _ucRcAT102_ChkSecurityCode(uchar *psSC)
{
	uint uiDataLen, uiRet;

	char *psFunCall = malloc(50);
	uiDataLen = sprintf(psFunCall, "%u(b002",FUNC_AT102_ChkSecurityCode);
	memcpy(psFunCall + uiDataLen, psSC, 2);
	uiDataLen += 2;
	uiDataLen += sprintf(psFunCall + uiDataLen,")");
	uiRet = iDoRemoteFunc((uchar*)psFunCall, uiDataLen, 1);
	free(psFunCall);
	if (uiRet) {
		vWriteLogTxt("_AT102_ChkSecurityCode iDoRemoteFunc ret = %d", uiRet);
		return 1;
	}
	return iGetParaInt(0);
}

uchar _ucRcAT102_ChkSecurityCodeEx(uchar *psSC)
{
	uint uiDataLen, uiRet;

	char *psFunCall = malloc(50);
	uiDataLen = sprintf(psFunCall, "%u(b008",FUNC_AT102_ChkSecurityCodeEx);
	memcpy(psFunCall + uiDataLen, psSC, 8);
	uiDataLen += 8;
	uiDataLen += sprintf(psFunCall + uiDataLen,")");
	uiRet = iDoRemoteFunc((uchar*)psFunCall, uiDataLen, 1);
	free(psFunCall);
	if (uiRet) {
		vWriteLogTxt("_AT102_ChkSecurityCodeEx iDoRemoteFunc ret = %d", uiRet);
		return 1;
	}
	return iGetParaInt(0);
}

void _vRcAT102_ReadWords(uchar ucWordAddr, uchar ucWordNum, uchar *psDataBuf )
{
	uint uiDataLen, uiRet;

	char *psFunCall = malloc(50);
	uiDataLen = sprintf(psFunCall, "%u(%u,%u,b000)",FUNC_AT102_ReadWords, ucWordAddr, ucWordNum);
	uiRet = iDoRemoteFunc((uchar*)psFunCall, uiDataLen, 2);
	free(psFunCall);
	if (uiRet) {
		vWriteLogTxt("_AT102_ReadWords iDoRemoteFunc ret = %d", uiRet);
		return;
	}
	memcpy(psDataBuf, psGetParaBuf(4), ucWordNum * 2);
}

uchar _ucRcAT102_EraseNonApp(uchar ucWordAddr, uchar ucWordNum)
{
	uint uiDataLen, uiRet;

	char *psFunCall = malloc(512);
	uiDataLen = sprintf(psFunCall, "%u(%u,%u)",FUNC_AT102_EraseNonApp, ucWordAddr, ucWordNum);
	uiRet = iDoRemoteFunc((uchar*)psFunCall, uiDataLen, 1);
	free(psFunCall);
	if (uiRet) {
		vWriteLogTxt("_AT102_EraseNonApp iDoRemoteFunc ret = %d", uiRet);
		return 1;
	}
	return iGetParaInt(0);
}

uchar _ucRcAT102_EraseApp(uchar ucArea, uchar ucLimited, uchar *psEraseKey)
{
	uint uiDataLen, uiRet, iKeyLen;

	char *psFunCall = malloc(512);

	if (psEraseKey == NULL)
		uiDataLen = sprintf(psFunCall, "%u(%u,%u)", FUNC_AT102_EraseApp, ucArea, ucLimited);
	else {
		iKeyLen = 6;
		if (ucArea == 2)
			iKeyLen = 4;
		uiDataLen = sprintf(psFunCall, "%u(%u,%u,b%03u", FUNC_AT102_EraseApp, ucArea, ucLimited, iKeyLen);
		memcpy(psFunCall + uiDataLen, psEraseKey, iKeyLen);
		uiDataLen += iKeyLen;
		uiDataLen += sprintf(psFunCall + uiDataLen,")");
	}
	uiRet = iDoRemoteFunc((uchar*)psFunCall, uiDataLen, 1);
	free(psFunCall);
	if (uiRet) {
		vWriteLogTxt("_AT102_EraseApp iDoRemoteFunc ret = %d", uiRet);
		return 1;
	}
	return iGetParaInt(0);
}

uchar _ucRcAT102_EraseAppEx(uchar ucArea, uchar ucLimited, uchar *psEraseKey)
{
	uint uiDataLen, uiRet, iKeyLen;

	char *psFunCall = malloc(512);

	if (psEraseKey == NULL)
		uiDataLen = sprintf(psFunCall, "%u(%u,%u)", FUNC_AT102_EraseApp, ucArea, ucLimited);
	else {
		uiDataLen = sprintf(psFunCall, "%u(%u,%u,b008", FUNC_AT102_EraseAppEx, ucArea, ucLimited);
		memcpy(psFunCall + uiDataLen, psEraseKey, 8);
		uiDataLen += 8;
		uiDataLen += sprintf(psFunCall + uiDataLen,")");
	}
	uiRet = iDoRemoteFunc((uchar*)psFunCall, uiDataLen, 2);
	free(psFunCall);
	if (uiRet) {
		vWriteLogTxt("_AT102_EraseAppEx iDoRemoteFunc ret = %d", uiRet);
		return 1;
	}
	return iGetParaInt(0);
}

uchar _ucRcAT102_WriteWords(uchar ucWordAddr, uchar ucWordNum, uchar *psDataBuf)
{
	uint uiDataLen, uiRet;

	char *psFunCall = malloc(512);
	uiDataLen = sprintf(psFunCall, "%u(%u,%u,b%03u",FUNC_AT102_WriteWords, ucWordAddr, ucWordNum, ucWordNum * 2);
	memcpy(psFunCall + uiDataLen, psDataBuf, ucWordNum * 2);
	uiDataLen += ucWordNum * 2;
	uiDataLen += sprintf(psFunCall + uiDataLen,")");
	uiRet = iDoRemoteFunc((uchar*)psFunCall, uiDataLen, 5);
	free(psFunCall);
	if (uiRet) {
		vWriteLogTxt("_AT102_WriteWords iDoRemoteFunc ret = %d", uiRet);
		return 1;
	}
	return iGetParaInt(0);
}

void _vRcAT102_ReadAZ(uchar ucArea, uchar *psAZ)
{
	uint uiDataLen, uiRet;

	char *psFunCall = malloc(50);
	uiDataLen = sprintf(psFunCall, "%u(%u,b000)",FUNC_AT102_ReadAZ, ucArea);
	uiRet = iDoRemoteFunc((uchar*)psFunCall, uiDataLen, 1);
	free(psFunCall);
	if (uiRet) {
		vWriteLogTxt("_AT102_ReadAZ iDoRemoteFunc ret = %d", uiRet);
		return;
	}
	memcpy(psAZ, psGetParaBuf(3), 64);
}

uchar _ucRcAT102_WriteAZ(uchar ucArea, uchar *psAZ)
{
	uint uiDataLen, uiRet;

	char *psFunCall = malloc(200);
	uiDataLen = sprintf(psFunCall, "%u(%u,b064",FUNC_AT102_WriteAZ, ucArea);
	memcpy(psFunCall + uiDataLen, psAZ, 64);
	uiDataLen += 64;
	uiDataLen += sprintf(psFunCall + uiDataLen,")");
	uiRet = iDoRemoteFunc((uchar*)psFunCall, uiDataLen, 5);
	free(psFunCall);
	if (uiRet) {
		vWriteLogTxt("_AT102_WriteAZ iDoRemoteFunc ret = %d", uiRet);
		return 1;
	}
	return iGetParaInt(0);
}

void _vRcAT102_ReadMTZ(uchar *psMTZ)
{
	uint uiDataLen, uiRet;

	char *psFunCall = malloc(50);
	uiDataLen = sprintf(psFunCall, "%u(b000)",FUNC_AT102_ReadMTZ);
	uiRet = iDoRemoteFunc((uchar*)psFunCall, uiDataLen, 1);
	free(psFunCall);
	if (uiRet) {
		vWriteLogTxt("_AT102_ReadMTZ iDoRemoteFunc ret = %d", uiRet);
		return;
	}
	memcpy(psMTZ, psGetParaBuf(2), 2);
}

uchar _ucRcAT102_UpdateMTZ(uchar *psMTZ)
{
	uint uiDataLen, uiRet;

	char *psFunCall = malloc(200);
	uiDataLen = sprintf(psFunCall, "%u(b002",FUNC_AT102_UpdateMTZ);
	memcpy(psFunCall + uiDataLen, psMTZ, 2);
	uiDataLen += 2;
	uiDataLen += sprintf(psFunCall + uiDataLen,")");
	uiRet = iDoRemoteFunc((uchar*)psFunCall, uiDataLen, 1);
	free(psFunCall);
	if (uiRet) {
		vWriteLogTxt("_AT102_UpdateMTZ iDoRemoteFunc ret = %d", uiRet);
		return 1;
	}
	return iGetParaInt(0);
}

//Memory card 1604
void _vRcAT1604_OpenCard(void)
{
	uint uiDataLen, uiRet;

	char *psFunCall = malloc(50);
	uiDataLen = sprintf(psFunCall, "%u()",FUNC_AT1604_OpenCard);
	uiRet = iDoRemoteFunc((uchar*)psFunCall, uiDataLen, 1);
	free(psFunCall);
	if (uiRet) {
		vWriteLogTxt("_AT1604_OpenCard iDoRemoteFunc ret = %d", uiRet);
	}
}
void _vRcAT1604_CloseCard(void)
{
	uint uiDataLen, uiRet;

	char *psFunCall = malloc(50);
	uiDataLen = sprintf(psFunCall, "%u()",FUNC_AT1604_CloseCard);
	uiRet = iDoRemoteFunc((uchar*)psFunCall, uiDataLen, 1);
	free(psFunCall);
	if (uiRet) {
		vWriteLogTxt("_AT1604_CloseCard iDoRemoteFunc ret = %d", uiRet);
	}
}
uchar _ucRcAT1604_ChkSC(uchar *SC)
{
	uint uiDataLen, uiRet;

	char *psFunCall = malloc(50);
	uiDataLen = sprintf(psFunCall, "%u(b002",FUNC_AT1604_ChkSC);
	memcpy(psFunCall + uiDataLen, SC, 2);
	uiDataLen += 2;
	uiDataLen += sprintf(psFunCall + uiDataLen,")");
	uiRet = iDoRemoteFunc((uchar*)psFunCall, uiDataLen, 1);
	free(psFunCall);
	if (uiRet) {
		vWriteLogTxt("_AT1604_ChkSC iDoRemoteFunc ret = %d", uiRet);
		return 1;
	}
	return iGetParaInt(0);
}
uchar _ucRcAT1604_ChkSCEx(uchar *SC)
{
	uint uiDataLen, uiRet;

	char *psFunCall = malloc(50);
	uiDataLen = sprintf(psFunCall, "%u(b008",FUNC_AT1604_ChkSCEx);
	memcpy(psFunCall + uiDataLen, SC, 8);
	uiDataLen += 8;
	uiDataLen += sprintf(psFunCall + uiDataLen,")");
	uiRet = iDoRemoteFunc((uchar*)psFunCall, uiDataLen, 2);
	free(psFunCall);
	if (uiRet) {
		vWriteLogTxt("_AT1604_ChkSCEx iDoRemoteFunc ret = %d", uiRet);
		return 1;
	}
	return iGetParaInt(0);
}
uchar _ucRcAT1604_ChkSCn(uchar Area,uchar *SCn)
{
	uint uiDataLen, uiRet;

	char *psFunCall = malloc(50);
	uiDataLen = sprintf(psFunCall, "%u(%u,b002",FUNC_AT1604_ChkSCn, Area);
	memcpy(psFunCall + uiDataLen, SCn, 2);
	uiDataLen += 2;
	uiDataLen += sprintf(psFunCall + uiDataLen,")");
	uiRet = iDoRemoteFunc((uchar*)psFunCall, uiDataLen, 1);
	free(psFunCall);
	if (uiRet) {
		vWriteLogTxt("_AT1604_ChkSCn iDoRemoteFunc ret = %d", uiRet);
		return 1;
	}
	return iGetParaInt(0);
}

uchar _ucRcAT1604_ChkSCnEx(uchar Area,uchar *SCn)
{
	uint uiDataLen, uiRet;

	char *psFunCall = malloc(50);
	uiDataLen = sprintf(psFunCall, "%u(%u,b008",FUNC_AT1604_ChkSCnEx, Area);
	memcpy(psFunCall + uiDataLen, SCn, 8);
	uiDataLen += 8;
	uiDataLen += sprintf(psFunCall + uiDataLen,")");
	uiRet = iDoRemoteFunc((uchar*)psFunCall, uiDataLen, 2);
	free(psFunCall);
	if (uiRet) {
		vWriteLogTxt("_AT1604_ChkSCnEx iDoRemoteFunc ret = %d", uiRet);
		return 1;
	}
	return iGetParaInt(0);
}
void _vRcAT1604_Read(uint Addr,uint Len,uchar *DataBuf)
{
	uint uiDataLen, uiRet;

	char *psFunCall = malloc(100);
	uiDataLen = sprintf(psFunCall, "%u(%u,%u,b000)",FUNC_AT1604_Read, Addr, Len);
	uiRet = iDoRemoteFunc((uchar*)psFunCall, uiDataLen, 5);
	free(psFunCall);
	if (uiRet) {
		vWriteLogTxt("_AT1604_Read iDoRemoteFunc ret = %d", uiRet);
		return;
	}
	memcpy(DataBuf, psGetParaBuf(4), Len);
}
void _vRcAT1604_ReadAZ(uchar Area,uint Addr,uint Len,uchar *DataBuf)
{
	uint uiDataLen, uiRet;

	char *psFunCall = malloc(100);
	uiDataLen = sprintf(psFunCall, "%u(%u,%u,%u,b000)",FUNC_AT1604_ReadAZ, Area, Addr, Len);
	uiRet = iDoRemoteFunc((uchar*)psFunCall, uiDataLen, 2);
	free(psFunCall);
	if (uiRet) {
		vWriteLogTxt("_AT1604_ReadAZ iDoRemoteFunc ret = %d", uiRet);
		return;
	}
	memcpy(DataBuf, psGetParaBuf(5), Len);
}
uchar _ucRcAT1604_Erase(uint Addr,uint Len)
{
	uint uiDataLen, uiRet;

	char *psFunCall = malloc(50);
	uiDataLen = sprintf(psFunCall, "%u(%u,%u)",FUNC_AT1604_Erase, Addr, Len);
	uiRet = iDoRemoteFunc((uchar*)psFunCall, uiDataLen, 2);
	free(psFunCall);
	if (uiRet) {
		vWriteLogTxt("_AT1604_Erase iDoRemoteFunc ret = %d", uiRet);
		return 1;
	}
	return iGetParaInt(0);
}
uchar _ucRcAT1604_EraseAZ(uchar Area,uint Addr,uint Len)
{
	uint uiDataLen, uiRet;

	char *psFunCall = malloc(50);
	uiDataLen = sprintf(psFunCall, "%u(%u,%u,%u)",FUNC_AT1604_EraseAZ, Area, Addr, Len);
	uiRet = iDoRemoteFunc((uchar*)psFunCall, uiDataLen, 2);
	free(psFunCall);
	if (uiRet) {
		vWriteLogTxt("_AT1604_EraseAZ iDoRemoteFunc ret = %d", uiRet);
		return 1;
	}
	return iGetParaInt(0);
}
uchar _ucRcAT1604_Write(uint Addr,uint Len,uchar *DataBuf)
{
	uint uiDataLen, uiRet;

	char *psFunCall = malloc(1024);
	uiDataLen = sprintf(psFunCall, "%u(%u,%u,p%04u",FUNC_AT1604_Write, Addr, Len, Len);
	memcpy(psFunCall + uiDataLen, DataBuf, Len);
	uiDataLen += Len;
	uiDataLen += sprintf(psFunCall + uiDataLen,")");
	uiRet = iDoRemoteFunc((uchar*)psFunCall, uiDataLen, 8);
	free(psFunCall);
	if (uiRet) {
		vWriteLogTxt("_AT1604_Write iDoRemoteFunc ret = %d", uiRet);
		return 1;
	}
	return iGetParaInt(0);
}
uchar _ucRcAT1604_WriteAZ(uchar Area,uint Addr,uint Len,uchar *DataBuf)
{
	uint uiDataLen, uiRet;

	char *psFunCall = malloc(1024);
	uiDataLen = sprintf(psFunCall, "%u(%u,%u,%u,b%03u",FUNC_AT1604_WriteAZ, Area, Addr, Len, Len);
	memcpy(psFunCall + uiDataLen, DataBuf, Len);
	uiDataLen += Len;
	uiDataLen += sprintf(psFunCall + uiDataLen,")");
	uiRet = iDoRemoteFunc((uchar*)psFunCall, uiDataLen, 5);
	free(psFunCall);
	if (uiRet) {
		vWriteLogTxt("_AT1604_WriteAZ iDoRemoteFunc ret = %d", uiRet);
		return 1;
	}
	return iGetParaInt(0);
}
uchar _ucRcAT1604_ChkEZn(uchar Area,uchar *EZn)
{
	uint uiDataLen, uiRet;

	char *psFunCall = malloc(50);
	uiDataLen = sprintf(psFunCall, "%u(%u,b002",FUNC_AT1604_ChkEZn, Area);
	memcpy(psFunCall + uiDataLen, EZn, 2);
	uiDataLen += 2;
	uiDataLen += sprintf(psFunCall + uiDataLen,")");
	uiRet = iDoRemoteFunc((uchar*)psFunCall, uiDataLen, 1);
	free(psFunCall);
	if (uiRet) {
		vWriteLogTxt("_AT1604_ChkEZn iDoRemoteFunc ret = %d", uiRet);
		return 1;
	}
	return iGetParaInt(0);
}
uchar _ucRcAT1604_ChkEZnEx(uchar Area,uchar *EZn)
{
	uint uiDataLen, uiRet;

	char *psFunCall = malloc(50);
	uiDataLen = sprintf(psFunCall, "%u(%u,b008",FUNC_AT1604_ChkEZnEx, Area);
	memcpy(psFunCall + uiDataLen, EZn, 8);
	uiDataLen += 8;
	uiDataLen += sprintf(psFunCall + uiDataLen,")");
	uiRet = iDoRemoteFunc((uchar*)psFunCall, uiDataLen, 1);
	free(psFunCall);
	if (uiRet) {
		vWriteLogTxt("_AT1604_ChkEZnEx iDoRemoteFunc ret = %d", uiRet);
		return 1;
	}
	return iGetParaInt(0);
}
void _vRcAT1604_ReadMTZ(uchar *MTZ)
{
	uint uiDataLen, uiRet;

	char *psFunCall = malloc(100);
	uiDataLen = sprintf(psFunCall, "%u(b000)",FUNC_AT1604_ReadMTZ);
	uiRet = iDoRemoteFunc((uchar*)psFunCall, uiDataLen, 1);
	free(psFunCall);
	if (uiRet) {
		vWriteLogTxt("_AT1604_ReadAZ iDoRemoteFunc ret = %d", uiRet);
		return;
	}
	memcpy(MTZ, psGetParaBuf(2), 2);
}

uchar _ucRcAT1604_UpdMTZ(uchar *MTZ)
{
	uint uiDataLen, uiRet;

	char *psFunCall = malloc(50);
	uiDataLen = sprintf(psFunCall, "%u(b002",FUNC_AT1604_UpdMTZ);
	memcpy(psFunCall + uiDataLen, MTZ, 2);
	uiDataLen += 2;
	uiDataLen += sprintf(psFunCall + uiDataLen,")");
	uiRet = iDoRemoteFunc((uchar*)psFunCall, uiDataLen, 1);
	free(psFunCall);
	if (uiRet) {
		vWriteLogTxt("_AT1604_UpdMTZ iDoRemoteFunc ret = %d", uiRet);
		return 1;
	}
	return iGetParaInt(0);
}

//Memory card 1608
void _vRcAT1608_OpenCard(uchar *RstData)
{
	uint uiDataLen, uiRet;

	char *psFunCall = malloc(50);
	uiDataLen = sprintf(psFunCall, "%u(b000)",FUNC_AT1608_OpenCard);
	uiRet = iDoRemoteFunc((uchar*)psFunCall, uiDataLen, 1);
	free(psFunCall);
	if (uiRet) {
		vWriteLogTxt("_AT1608_OpenCard iDoRemoteFunc ret = %d", uiRet);
		return;
	}
	memcpy(RstData, psGetParaBuf(2) + 1, 4);
}
void _vRcAT1608_CloseCard(void)
{
	uint uiDataLen, uiRet;

	char *psFunCall = malloc(50);
	uiDataLen = sprintf(psFunCall, "%u()",FUNC_AT1608_CloseCard);
	uiRet = iDoRemoteFunc((uchar*)psFunCall, uiDataLen, 1);
	free(psFunCall);
	if (uiRet) {
		vWriteLogTxt("_AT1608_CloseCard iDoRemoteFunc ret = %d", uiRet);
	}
}

uchar _ucRcAT1608_ReadFuse(uchar *Fuse)
{
	uint uiDataLen, uiRet;

	char *psFunCall = malloc(100);
	uiDataLen = sprintf(psFunCall, "%u(b000)",FUNC_AT1608_ReadFuse);
	uiRet = iDoRemoteFunc((uchar*)psFunCall, uiDataLen, 1);
	free(psFunCall);
	if (uiRet) {
		vWriteLogTxt("_ucRcAT1608_ReadFuse iDoRemoteFunc ret = %d", uiRet);
		return 1;
	}
	uiRet = iGetParaInt(0);
	if (uiRet == 0)
		memcpy(Fuse, psGetParaBuf(2), 1);
	return uiRet;
}

uchar _ucRcAT1608_WriteFuse(void)
{
	uint uiDataLen, uiRet;

	char *psFunCall = malloc(100);
	uiDataLen = sprintf(psFunCall, "%u()",FUNC_AT1608_WriteFuse);
	uiRet = iDoRemoteFunc((uchar*)psFunCall, uiDataLen, 1);
	free(psFunCall);
	if (uiRet) {
		vWriteLogTxt("_ucRcAT1608_WriteFuse iDoRemoteFunc ret = %d", uiRet);
		return 1;
	}
	uiRet = iGetParaInt(0);

	return uiRet;
}

uchar _ucRcAT1608_VerifyPassword(uchar Index,uchar *Password)
{
	uint uiDataLen, uiRet;

	char *psFunCall = malloc(50);
	uiDataLen = sprintf(psFunCall, "%u(%u,b003",FUNC_AT1608_VerifyPassword, Index);
	memcpy(psFunCall + uiDataLen, Password, 3);
	uiDataLen += 3;
	uiDataLen += sprintf(psFunCall + uiDataLen,")");
	uiRet = iDoRemoteFunc((uchar*)psFunCall, uiDataLen, 1);
	free(psFunCall);
	if (uiRet) {
		vWriteLogTxt("_AT1608_VerifyPassword iDoRemoteFunc ret = %d", uiRet);
		return 1;
	}
	return iGetParaInt(0);
}
uchar _ucRcAT1608_VerifyPasswordEx(uchar Index,uchar *Password)
{
	uint uiDataLen, uiRet;

	char *psFunCall = malloc(50);
	uiDataLen = sprintf(psFunCall, "%u(%u,b008",FUNC_AT1608_VerifyPasswordEx, Index);
	memcpy(psFunCall + uiDataLen, Password, 8);
	uiDataLen += 8;
	uiDataLen += sprintf(psFunCall + uiDataLen,")");
	uiRet = iDoRemoteFunc((uchar*)psFunCall, uiDataLen, 2);
	free(psFunCall);
	if (uiRet) {
		vWriteLogTxt("_AT1608_VerifyPasswordEx iDoRemoteFunc ret = %d", uiRet);
		return 1;
	}
	return iGetParaInt(0);
}
uchar _ucRcAT1608_Read(uchar Level,uchar Addr,uint Len,uchar *DataBuf)
{
	uint uiDataLen, uiRet;

	char *psFunCall = malloc(100);
	uiDataLen = sprintf(psFunCall, "%u(%u,%u,%u,b000)",FUNC_AT1608_Read, Level, Addr, Len);
	uiRet = iDoRemoteFunc((uchar*)psFunCall, uiDataLen, 5);
	free(psFunCall);
	if (uiRet) {
		vWriteLogTxt("_AT1608_Read iDoRemoteFunc ret = %d", uiRet);
		return 1;
	}
	uiRet = iGetParaInt(0);
	if (uiRet == 0)
		memcpy(DataBuf, psGetParaBuf(5), Len);
	return uiRet;
}

uchar _ucRcAT1608_Write(uchar Level,uchar Addr,uint Len,uchar *DataBuf)
{
	uint uiDataLen, uiRet;

	char *psFunCall = malloc(1024);
	uiDataLen = sprintf(psFunCall, "%u(%u,%u,%u,p%04u",FUNC_AT1608_Write, Level, Addr, Len, Len);
	memcpy(psFunCall + uiDataLen, DataBuf, Len);
	uiDataLen += Len;
	uiDataLen += sprintf(psFunCall + uiDataLen,")");
	uiRet = iDoRemoteFunc((uchar*)psFunCall, uiDataLen, 8);
	free(psFunCall);
	if (uiRet) {
		vWriteLogTxt("_AT1608_Write iDoRemoteFunc ret = %d", uiRet);
		return 1;
	}
	return iGetParaInt(0);
}

uchar _ucRcAT1608_Auth(uchar *Gc)
{
	uint uiDataLen, uiRet;

	char *psFunCall = malloc(100);
	uiDataLen = sprintf(psFunCall, "%u(b008",FUNC_AT1608_Auth);
	memcpy(psFunCall + uiDataLen, Gc,8);
	uiDataLen += 8;
	uiDataLen += sprintf(psFunCall + uiDataLen,")");
	uiRet = iDoRemoteFunc((uchar*)psFunCall, uiDataLen, 1);
	free(psFunCall);
	if (uiRet) {
		vWriteLogTxt("_AT1608_Auth iDoRemoteFunc ret = %d", uiRet);
		return 1;
	}
	return iGetParaInt(0);
}

uchar _ucRcAT1608_AuthEx(uchar *Gc)
{
	uint uiDataLen, uiRet;

	char *psFunCall = malloc(100);
	uiDataLen = sprintf(psFunCall, "%u(b016",FUNC_AT1608_AuthEx);
	memcpy(psFunCall + uiDataLen, Gc,16);
	uiDataLen += 16;
	uiDataLen += sprintf(psFunCall + uiDataLen,")");
	uiRet = iDoRemoteFunc((uchar*)psFunCall, uiDataLen, 2);
	free(psFunCall);
	if (uiRet) {
		vWriteLogTxt("_AT1608_AuthEx iDoRemoteFunc ret = %d", uiRet);
		return 1;
	}
	return iGetParaInt(0);
}

uchar _ucRcAT1608_SetAZ(uchar AZ)
{
	uint uiDataLen, uiRet;

	char *psFunCall = malloc(100);
	uiDataLen = sprintf(psFunCall, "%u(%u)",FUNC_AT1608_SetAZ, AZ);
	uiRet = iDoRemoteFunc((uchar*)psFunCall, uiDataLen, 1);
	free(psFunCall);
	if (uiRet) {
		vWriteLogTxt("_AT1608_SetAZ iDoRemoteFunc ret = %d", uiRet);
		return 1;
	}
	return iGetParaInt(0);
}

void _vRcAT24Cxx_OpenCard(void)
{
	uint uiDataLen, uiRet;

	char *psFunCall = malloc(50);

	uiDataLen = sprintf(psFunCall, "%u()",FUNC_AT24Cxx_OpenCard);
	uiRet = iDoRemoteFunc((uchar*)psFunCall, uiDataLen, 1);
	free(psFunCall);
	if (uiRet) {
		vWriteLogTxt("_AT24Cxx_OpenCard iDoRemoteFunc ret = %d", uiRet);
	}
}
void _vRcAT24Cxx_CloseCard(void)
{
	uint uiDataLen, uiRet;

	char *psFunCall = malloc(50);
	uiDataLen = sprintf(psFunCall, "%u()",FUNC_AT24Cxx_CloseCard);
	uiRet = iDoRemoteFunc((uchar*)psFunCall, uiDataLen, 1);
	free(psFunCall);
	if (uiRet) {
		vWriteLogTxt("_AT24Cxx_CloseCard iDoRemoteFunc ret = %d", uiRet);
	}
}

uchar _ucRcAT24Cxx_Read(uchar* DataBuff, uchar Addr, int DataLen)
{
	uint uiDataLen, uiRet;

	char *psFunCall = malloc(50);
	uiDataLen = sprintf(psFunCall, "%u(b000,%u,%u)",FUNC_AT24Cxx_Read, Addr, DataLen);
	uiRet = iDoRemoteFunc((uchar*)psFunCall, uiDataLen, 5);
	free(psFunCall);
	if (uiRet) {
		vWriteLogTxt("_AT24Cxx_Read iDoRemoteFunc ret = %d", uiRet);
		return 1;
	}
	memcpy(DataBuff, psGetParaBuf(2), DataLen);
	return iGetParaInt(0);
}

uchar _ucRcAT24Cxx_Write(uchar* DataBuff, uchar Addr,int DataLen)
{
	uint uiDataLen, uiRet;

	char *psFunCall = malloc(512);
	uiDataLen = sprintf(psFunCall, "%u(b%03u",FUNC_AT24Cxx_Write, DataLen);
	memcpy(psFunCall + uiDataLen, DataBuff, DataLen);
	uiDataLen += DataLen;
	uiDataLen += sprintf(psFunCall + uiDataLen, ",%u,%u)",Addr, DataLen);
	uiRet = iDoRemoteFunc((uchar*)psFunCall, uiDataLen, 10);
	free(psFunCall);
	if (uiRet) {
		vWriteLogTxt("_AT24Cxx_Write iDoRemoteFunc ret = %d", uiRet);
		return 1;
	}
	return iGetParaInt(0);
}

uchar _ucRcAT24C32_Read(uchar* DataBuff, uchar Addr, int DataLen)
{
	uint uiDataLen, uiRet;

	char *psFunCall = malloc(50);
	uiDataLen = sprintf(psFunCall, "%u(b000,%u,%u)",FUNC_AT24C32_Read, Addr, DataLen);
	uiRet = iDoRemoteFunc((uchar*)psFunCall, uiDataLen, 5);
	free(psFunCall);
	if (uiRet) {
		vWriteLogTxt("_AT24C32_Read iDoRemoteFunc ret = %d", uiRet);
		return 1;
	}
	memcpy(DataBuff, psGetParaBuf(2), DataLen);
	return iGetParaInt(0);
}

uchar _ucRcAT24C32_Write(uchar* DataBuff, uchar Addr,int DataLen)
{
	uint uiDataLen, uiRet;

	char *psFunCall = malloc(512);
	uiDataLen = sprintf(psFunCall, "%u(b%03u",FUNC_AT24C32_Write, DataLen);
	memcpy(psFunCall + uiDataLen, DataBuff, DataLen);
	uiDataLen += DataLen;
	uiDataLen += sprintf(psFunCall + uiDataLen, ",%u,%u)",Addr, DataLen);
	uiRet = iDoRemoteFunc((uchar*)psFunCall, uiDataLen, 10);
	free(psFunCall);
	if (uiRet) {
		vWriteLogTxt("_AT24C32_Write iDoRemoteFunc ret = %d", uiRet);
		return 1;
	}
	return iGetParaInt(0);
}

uint _uiRcFingerprintOpen(void)
{
	uint uiDataLen, uiRet;
    
	char *psFunCall = malloc(100);
	uiDataLen = sprintf(psFunCall, "%u()",FUNC__uiFingerprintOpen);
	uiRet = iDoRemoteFunc((uchar*)psFunCall, uiDataLen, 5);
	free(psFunCall);
	if (uiRet) {
		vWriteLogTxt("_uiRcFingerprintOpen iDoRemoteFunc ret = %d", uiRet);
		return 99;
	}
	return iGetParaInt(0);
}

void _vRcFingerprintClose(void)
{
	uint uiDataLen;
    
	char *psFunCall = malloc(100);
	uiDataLen = sprintf(psFunCall, "%u()", FUNC__vFingerprintClose);
	iDoRemoteFunc((uchar*)psFunCall, uiDataLen, 1);
	free(psFunCall);
}
uint _uiRcFingerprintLink(void)
{
	uint uiDataLen, uiRet;
    
	char *psFunCall = malloc(100);
	uiDataLen = sprintf(psFunCall, "%u()",FUNC__uiFingerprintLink);
	uiRet = iDoRemoteFunc((uchar*)psFunCall, uiDataLen, 1);
	free(psFunCall);
	if (uiRet) {
		vWriteLogTxt("_uiRcFingerprintLink iDoRemoteFunc ret = %d", uiRet);
		return 99;
	}
	return iGetParaInt(0);
}
void _vRcFingerprintSend(uchar *psIn, uint uiInLen)
{
	uint uiDataLen;
    
	char *psFunCall = malloc(100);
	uiDataLen = sprintf(psFunCall, "%u(b%03u", FUNC__vFingerprintSend, uiInLen);
    memcpy(psFunCall + uiDataLen, psIn, uiInLen);
    uiDataLen += uiInLen;
    uiDataLen += sprintf(psFunCall + uiDataLen, ",%u)", uiInLen);
	iDoRemoteFunc((uchar*)psFunCall, uiDataLen, 1);
	free(psFunCall);
}
uint _uiRcFingerprintRecv(uchar *psOut, uint *puiOutLen, ulong ulTimeOutMs)
{
	uint uiDataLen, uiRet;
    
	char *psFunCall = malloc(100);
	uiDataLen = sprintf(psFunCall, "%u(b000,0,%lu)",FUNC__uiFingerprintRecv, ulTimeOutMs);
	uiRet = iDoRemoteFunc((uchar*)psFunCall, uiDataLen, 1 + (uint)(ulTimeOutMs/1000L));
	free(psFunCall);
	if (uiRet) {
		vWriteLogTxt("_uiRcFingerprintRecv iDoRemoteFunc ret = %d", uiRet);
		return 99;
	}
    if (iGetParaInt(0) == 0) {
        *puiOutLen = iGetParaInt(3);
        memcpy(psOut, psGetParaBuf(2), *puiOutLen);
    }
	return iGetParaInt(0);
}

int iRcTsFingerprintOpen(void)
{
	uint uiDataLen, uiRet;

	char *psFunCall = malloc(100);
	uiDataLen = sprintf(psFunCall, "%u()",FUNC_iTsFingerprintOpen);
	uiRet = iDoRemoteFunc((uchar*)psFunCall, uiDataLen, 5);
	free(psFunCall);
	if (uiRet) {
		vWriteLogTxt("iRcTsFingerprintOpen iDoRemoteFunc ret = %d", uiRet);
		return 99;
	}
	return iGetParaInt(0);
}

void vRcTsFingerprintClose(void)
{
	uint uiDataLen;

	char *psFunCall = malloc(100);
	uiDataLen = sprintf(psFunCall, "%u()", FUNC_vTsFingerprintClose);
	iDoRemoteFunc((uchar*)psFunCall, uiDataLen, 1);
	free(psFunCall);
}

void vRcTSFID_RUDLL_SetOverTime(int timeout)
{
	uint uiDataLen;

	char *psFunCall = malloc(100);
	uiDataLen = sprintf(psFunCall, "%u(%d)", FUNC_TSFID_RUDLL_SetOverTime, timeout);
	iDoRemoteFunc((uchar*)psFunCall, uiDataLen, 1);
	free(psFunCall);
}

int iRcTSFID_RUDLL_GetFinger(char fpflag[3],unsigned char fpminu1[200],unsigned char fpminu2[100])
{
	uint uiDataLen, uiRet;

	char *psFunCall = malloc(100);
	uiDataLen = sprintf(psFunCall, "%u(b003",FUNC_TSFID_RUDLL_GetFinger);
	memcpy(psFunCall + uiDataLen, fpflag, 3);
	uiDataLen += 3;
	uiDataLen += sprintf(psFunCall + uiDataLen, ",b000,b000)");

	uiRet = iDoRemoteFunc((uchar*)psFunCall, uiDataLen, 61);
	free(psFunCall);
	if (uiRet) {
		vWriteLogTxt("iRcTSFID_RUDLL_GetFinger iDoRemoteFunc ret = %d", uiRet);
		return 99;
	}
    if (iGetParaInt(0) == 0) {
    	memcpy(fpminu1, psGetParaBuf(3), 200);
    	memcpy(fpminu2, psGetParaBuf(4), 100);
    }
	return iGetParaInt(0);
}

int iRcTSFID_RUDLL_EnrollFinger(char fpflag[3],int order,unsigned char fpminu1[200],unsigned char fpminu2[100])
{
	uint uiDataLen, uiRet;

	char *psFunCall = malloc(100);
	uiDataLen = sprintf(psFunCall, "%u(b003",FUNC_TSFID_RUDLL_EnrollFinger);
	memcpy(psFunCall + uiDataLen, fpflag, 3);
	uiDataLen += 3;
	uiDataLen += sprintf(psFunCall + uiDataLen, ",%d,b000,b000)", order);

	uiRet = iDoRemoteFunc((uchar*)psFunCall, uiDataLen, 61);
	free(psFunCall);
	if (uiRet) {
		vWriteLogTxt("iRcTSFID_RUDLL_EnrollFinger iDoRemoteFunc ret = %d", uiRet);
		return 99;
	}
    if (iGetParaInt(0) == 0) {
    	memcpy(fpminu1, psGetParaBuf(4), 200);
    	memcpy(fpminu2, psGetParaBuf(5), 100);
    }
	return iGetParaInt(0);
}

int iRcTSFID_RUDLL_SetDeviceNo(char fpflag[3], unsigned char deviceno[12])
{
	uint uiDataLen, uiRet;

	char *psFunCall = malloc(100);
	uiDataLen = sprintf(psFunCall, "%u(b003", FUNC_TSFID_RUDLL_SetDeviceNo);
	memcpy(psFunCall + uiDataLen, fpflag, 3);
	uiDataLen += 3;
	uiDataLen += sprintf(psFunCall + uiDataLen, ",b012");
	memcpy(psFunCall + uiDataLen, deviceno, 12);
	uiDataLen += 12;
	uiDataLen += sprintf(psFunCall + uiDataLen, ")");
	uiRet = iDoRemoteFunc((uchar*)psFunCall, uiDataLen, 1);
	free(psFunCall);
	if (uiRet) {
		vWriteLogTxt("iRcTSFID_RUDLL_SetDeviceNo iDoRemoteFunc ret = %d", uiRet);
		return 99;
	}
	return iGetParaInt(0);
}
int iRcTSFID_RUDLL_GetDeviceNo(char fpflag[3],unsigned char deviceno[12])
{
	uint uiDataLen, uiRet;

	char *psFunCall = malloc(100);
	uiDataLen = sprintf(psFunCall, "%u(b003", FUNC_TSFID_RUDLL_GetDeviceNo);
	memcpy(psFunCall + uiDataLen, fpflag, 3);
	uiDataLen += 3;
	uiDataLen += sprintf(psFunCall + uiDataLen, ",b000)");
	uiRet = iDoRemoteFunc((uchar*)psFunCall, uiDataLen, 1);
	free(psFunCall);
	if (uiRet) {
		vWriteLogTxt("iRcTSFID_RUDLL_GetDeviceNo iDoRemoteFunc ret = %d", uiRet);
		return 99;
	}
	if (iGetParaInt(0) == 0) {
    	memcpy(deviceno, psGetParaBuf(3), 12);
	}
	return iGetParaInt(0);
}

void vRcTSFID_RUDLL_GetErrorMSG(int errorno,char msgptr[80])
{
	uint uiDataLen;

	char *psFunCall = malloc(100);
	uiDataLen = sprintf(psFunCall, "%u(%u,s000)", FUNC_TSFID_RUDLL_GetErrorMSG, errorno);
	iDoRemoteFunc((uchar*)psFunCall, uiDataLen, 1);
    strcpy(msgptr, psGetParaBuf(3));
}

int iRcTSFID_RUDLL_SetDeviceType(char fpflag[3],unsigned char devicetype)
{
	uint uiDataLen, uiRet;

	char *psFunCall = malloc(100);
	uiDataLen = sprintf(psFunCall, "%u(b003", FUNC_TSFID_RUDLL_SetDeviceType);
	memcpy(psFunCall + uiDataLen, fpflag, 3);
	uiDataLen += 3;
	uiDataLen += sprintf(psFunCall + uiDataLen, ",%u)", devicetype);
	uiRet = iDoRemoteFunc((uchar*)psFunCall, uiDataLen, 1);
	free(psFunCall);
	if (uiRet) {
		vWriteLogTxt("iRcTSFID_RUDLL_SetDeviceType iDoRemoteFunc ret = %d", uiRet);
		return 99;
	}
	return iGetParaInt(0);
}

int iRcTSFID_RUDLL_GetDeviceType(char fpflag[3],unsigned char *devicetype)
{
	uint uiDataLen, uiRet;

	char *psFunCall = malloc(100);
	uiDataLen = sprintf(psFunCall, "%u(b003", FUNC_TSFID_RUDLL_GetDeviceType);
	memcpy(psFunCall + uiDataLen, fpflag, 3);
	uiDataLen += 3;
	uiDataLen += sprintf(psFunCall + uiDataLen, ",0)");
	uiRet = iDoRemoteFunc((uchar*)psFunCall, uiDataLen, 1);
	free(psFunCall);
	if (uiRet) {
		vWriteLogTxt("iRcTSFID_RUDLL_GetDeviceType iDoRemoteFunc ret = %d", uiRet);
		return 99;
	}
	if (iGetParaInt(0) == 0)
		*devicetype = iGetParaInt(3);
	return iGetParaInt(0);
}

int iRcTSFID_RUDLL_GetDeviceInfo(char fpflag[3], char firmver[10], char deviceinfo[10])
{
	uint uiDataLen, uiRet;

	char *psFunCall = malloc(100);
	uiDataLen = sprintf(psFunCall, "%u(b003", FUNC_TSFID_RUDLL_GetDeviceInfo);
	memcpy(psFunCall + uiDataLen, fpflag, 3);
	uiDataLen += 3;
	uiDataLen += sprintf(psFunCall + uiDataLen, ",b000,b000)");
	uiRet = iDoRemoteFunc((uchar*)psFunCall, uiDataLen, 1);
	free(psFunCall);
	if (uiRet) {
		vWriteLogTxt("iRcTSFID_RUDLL_GetDeviceInfo iDoRemoteFunc ret = %d", uiRet);
		return 99;
	}
	if (iGetParaInt(0) == 0) {
    	memcpy(firmver, psGetParaBuf(3), 10);
    	memcpy(deviceinfo, psGetParaBuf(4), 10);
	}
	return iGetParaInt(0);
}


// TF ICC functions
uint _uiRcSD_Init(void)
{
	uint uiDataLen, uiRet;
    
	char *psFunCall = malloc(100);
	uiDataLen = sprintf(psFunCall, "%u()",FUNC_SD_Init);
	uiRet = iDoRemoteFunc((uchar*)psFunCall, uiDataLen, 1);
	free(psFunCall);
	if (uiRet) {
		vWriteLogTxt("_uiRcSD_Init iDoRemoteFunc ret = %d", uiRet);
		return 99;
	}
	return iGetParaInt(0);
}

void _vRcSD_DeInit(void)
{
	uint uiDataLen;
    
	char *psFunCall = malloc(100);
	uiDataLen = sprintf(psFunCall, "%u()",FUNC_SD_DeInit);
	iDoRemoteFunc((uchar*)psFunCall, uiDataLen, 1);
	free(psFunCall);
}

uint _uiRcSDSCConnectDev(void)
{
	uint uiDataLen, uiRet;
    
	char *psFunCall = malloc(100);
	uiDataLen = sprintf(psFunCall, "%u()",FUNC_SDSCConnectDev);
	uiRet = iDoRemoteFunc((uchar*)psFunCall, uiDataLen, 5);
	free(psFunCall);
	if (uiRet) {
		vWriteLogTxt("_uiRcSDSCConnectDev iDoRemoteFunc ret = %d", uiRet);
		return 99;
	}
	return iGetParaInt(0);
}
uint _uiRcSDSCDisconnectDev(void)
{
	uint uiDataLen, uiRet;
    
	char *psFunCall = malloc(100);
	uiDataLen = sprintf(psFunCall, "%u()",FUNC_SDSCDisconnectDev);
	uiRet = iDoRemoteFunc((uchar*)psFunCall, uiDataLen, 1);
	free(psFunCall);
	if (uiRet) {
		vWriteLogTxt("_uiRcSDSCDisconnectDev iDoRemoteFunc ret = %d", uiRet);
		return 99;
	}
	return iGetParaInt(0);
}

uint _uiRcSDSCGetFirmwareVer(uchar *psFirmwareVer, uint *puiFirmwareVerLen)
{
	uint uiDataLen, uiRet;
    
	char *psFunCall = malloc(100);
	uiDataLen = sprintf(psFunCall, "%u(b000, %u)",FUNC_SDSCGetFirmwareVer, *puiFirmwareVerLen);
	uiRet = iDoRemoteFunc((uchar*)psFunCall, uiDataLen, 1);
	free(psFunCall);
	if (uiRet) {
		vWriteLogTxt("_uiRcSDSCGetFirmwareVer iDoRemoteFunc ret = %d", uiRet);
		return 99;
	}
    if (iGetParaInt(0) == 0) {
        memcpy(psFirmwareVer, psGetParaBuf(2), iGetParaInt(3));
        //psFirmwareVer[iGetParaInt(3)] = 0;
        *puiFirmwareVerLen = iGetParaInt(3);
    }
	return iGetParaInt(0);
}
uint _uiRcSDSCResetCard(uchar *psAtr, uint *puiAtrLen)
{
	uint uiDataLen, uiRet;
    
	char *psFunCall = malloc(100);
	uiDataLen = sprintf(psFunCall, "%u(b000, 0)",FUNC_SDSCResetCard);
	uiRet = iDoRemoteFunc((uchar*)psFunCall, uiDataLen, 5);
	free(psFunCall);
	if (uiRet) {
		vWriteLogTxt("_uiRcSDSCResetCard iDoRemoteFunc ret = %d", uiRet);
		return 99;
	}
    if (iGetParaInt(0) == 0) {
        memcpy(psAtr, psGetParaBuf(2), iGetParaInt(3));
        *puiAtrLen = iGetParaInt(3);
    }
	return iGetParaInt(0);
}

uint _uiRcSDSCResetController(uint uiSCPowerMode)
{
	uint uiDataLen, uiRet;
    
	char *psFunCall = malloc(100);
	uiDataLen = sprintf(psFunCall, "%u(%u)",FUNC_SDSCResetController, uiSCPowerMode);
	uiRet = iDoRemoteFunc((uchar*)psFunCall, uiDataLen, 5);
	free(psFunCall);
	if (uiRet) {
		vWriteLogTxt("_uiRcSDSCResetController iDoRemoteFunc ret = %d", uiRet);
		return 99;
	}
	return iGetParaInt(0);
}

uint _uiRcSDSCTransmit(uchar *psCommand, uint uiCommandLen, uint uiTimeOutMode, uchar *psOutData, uint *puiOutDataLen, uint *puiCosState)
{
	uint uiDataLen;
    int iRet;
    unsigned int uiTimeout;
    
	//vSetWriteLog(1);

	char *psFunCall = malloc(1024);
	uiDataLen = sprintf(psFunCall, "%u(b%03u",FUNC_SDSCTransmit, uiCommandLen);
	memcpy(psFunCall + uiDataLen, psCommand, uiCommandLen);
	uiDataLen += uiCommandLen;
	uiDataLen += sprintf(psFunCall + uiDataLen, ",%u,%u,b000,%u,0)", uiCommandLen, uiTimeOutMode, *puiOutDataLen);
    
	uiTimeout = 5;
	if (uiTimeOutMode)
		uiTimeout = 46;
	iRet = iDoRemoteFunc((uchar*)psFunCall, uiDataLen, uiTimeout);
	free(psFunCall);
    if (iRet == -2)
        return 9;
	if (iRet) {
		vWriteLogTxt("_uiRcSDSCTransmit iDoRemoteFunc ret = %d", iRet);
		return 1;
	}
	iRet = iGetParaInt(0);

	vWriteLogTxt("_uiRcSDSCTransmit iRet = %d", iRet);
	if (iRet != 0)
		return 1;
    
	uiDataLen = iGetParaInt(6);
	//if (uiDataLen < 2)
	//	return 1;
	//vWriteLogTxt("_uiRcSDSCTransmit uiDataLen = %d", uiDataLen);

	*puiOutDataLen = uiDataLen;
    *puiCosState = iGetParaInt(7);
	memcpy(psOutData, psGetParaBuf(5), uiDataLen);

	vWriteLogTxt("_uiRcSDSCTransmit success!!");
    
	return (0);
}
uint _uiRcSDSCTransmitEx(uchar *psCommand, uint uiCommandLen, uint uiTimeOutMode, uchar *psOutData, uint *puiOutDataLen)
{
	uint uiDataLen;
    int iRet;
    unsigned int uiTimeout;
    
	char *psFunCall = malloc(1024);
	uiDataLen = sprintf(psFunCall, "%u(b%03u",FUNC_SDSCTransmitEx, uiCommandLen);
	memcpy(psFunCall + uiDataLen, psCommand, uiCommandLen);
	uiDataLen += uiCommandLen;
	uiDataLen += sprintf(psFunCall + uiDataLen, ",%u,%u,b000,%u)", uiCommandLen, uiTimeOutMode, *puiOutDataLen);
    
	uiTimeout = 5;
	if (uiTimeOutMode)
		uiTimeout = 46;
	iRet = iDoRemoteFunc((uchar*)psFunCall, uiDataLen, uiTimeout);
	free(psFunCall);
    if (iRet == -2)
        return 9;
	if (iRet) {
		vWriteLogTxt("_uiRcSDSCTransmitEx iDoRemoteFunc ret = %d", iRet);
		return 1;
	}
	iRet = iGetParaInt(0);
	if (iRet != 0)
		return 1;
    
	uiDataLen = iGetParaInt(6);
    
	if (uiDataLen < 2)
		return 1;
	*puiOutDataLen = uiDataLen;
	memcpy(psOutData, psGetParaBuf(5), uiDataLen);
    
	return (0);
}
uint _uiRcSDSCGetSDKVersion(char *pszVersion, uint *puiVersionLen)
{
	uint uiDataLen, uiRet;
    
	char *psFunCall = malloc(100);
	uiDataLen = sprintf(psFunCall, "%u(b000, %u)",FUNC_SDSCGetSDKVersion, *puiVersionLen);
	uiRet = iDoRemoteFunc((uchar*)psFunCall, uiDataLen, 1);
	free(psFunCall);
	if (uiRet) {
		vWriteLogTxt("_uiRcSDSCGetSDKVersion iDoRemoteFunc ret = %d", uiRet);
		return 99;
	}
    if (iGetParaInt(0) == 0) {
        memcpy(pszVersion, psGetParaBuf(2), iGetParaInt(3));
        pszVersion[iGetParaInt(3)] = 0;
        *puiVersionLen = iGetParaInt(3);
        vWriteLogHex("_uiRcSDSCGetSDKVersion:", pszVersion, *puiVersionLen);
    }
	return iGetParaInt(0);
    
}
uint _uiRcSDSCGetSCIOType(uint *puiSCIOType)
{
	uint uiDataLen, uiRet;
    
	char *psFunCall = malloc(100);
	uiDataLen = sprintf(psFunCall, "%u(0)",FUNC_SDSCGetSCIOType);
	uiRet = iDoRemoteFunc((uchar*)psFunCall, uiDataLen, 1);
	free(psFunCall);
	if (uiRet) {
		vWriteLogTxt("_uiRcSDSCGetSCIOType iDoRemoteFunc ret = %d", uiRet);
		return 99;
	}
    if (iGetParaInt(0) == 0) {
        *puiSCIOType = iGetParaInt(2);
    }
	return iGetParaInt(0);
}

uint uiGenCommKey(uchar ucMasterKeyId, uchar *psRandom)
{
	uint uiDataLen, uiRet;

	char *psFunCall = malloc(100);
	uiDataLen = sprintf(psFunCall, "%u(%u, b008",FUNC_uiGenCommKey, ucMasterKeyId);
	memcpy(psFunCall + uiDataLen, psRandom, 8);
	uiDataLen += 8;
	uiDataLen += sprintf(psFunCall + uiDataLen, ")");

	uiRet = iDoRemoteFunc((uchar*)psFunCall, uiDataLen, 5);
	free(psFunCall);
	if (uiRet) {
		vWriteLogTxt("uiGenCommKey iDoRemoteFunc ret = %d", uiRet);
		return 99;
	}
	return iGetParaInt(0);
}




