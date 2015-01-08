/**************************************
File name     : Common.c
Function      : EMV无关的一些通用功能函数
Author        : Yu Jun
First edition : Apr 9th, 2012
Modified      : 
**************************************/
/*
模块详细描述:
.	随机数生成函数vGetRand()原理
    随机数初始化时用传入随机数种子, 每次随机数扰动, 都用同一函数vRandShuffle(ulong ulRand)实现
	静态全局变量sg_Sha1Context与sg_ulLastX用于保存随机数上下文
	最终传出的随机数为初始化数据及后续所有扰动的SHA1值
	使用SHA1值可以确保产生的随机数能通过随机数性能分析
*/
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "VposFace.h"
#include "Sha.h"
#include "Common.h"

// 用于伪随机数发生
static SHA1_CTX sg_Sha1Context;    // SHA1上下文
static ulong	sg_ulLastX;        // X(n-1)

// 测试某字串某Byte某Bit真值
// in  : psStr     : 需要测试的字串
//       iPosition : 要测试的位号，0-7表示第0字节最高位到最低位，8-15表示第1字节最高位到最低位，以此类推
// out : 0         : 该位为0
//       1         : 该位为1
int iTestStrBit(uchar *psStr, int iPosition)
{
    uchar ucByte;
    ucByte = psStr[iPosition/8];
    iPosition %= 8;
    ucByte >>= (7-iPosition);
    if(ucByte & 0x01)
        return(1);
    return(0);
}

// 另一种方式测试某字串某Byte某Bit真值
// in  : psStr     : 需要测试的字串
//       iByteNo   : 要测试的字节下标, 0开始
//       iBitNo    : 要测试的字节的位号(为兼容emv规范描述，最高位为8，最低位为1)
// out : 0         : 该位为0
//       1         : 该位为1
// Note: 如果iBitNo不在1-8范围内，返回0
int iTestStrBit2(uchar *psStr, int iByteNo, int iBitNo)
{
    uchar ucByte;
    if(iBitNo<1 || iBitNo>8)
        return(0);
    ucByte = psStr[iByteNo];
    ucByte >>= (iBitNo-1);
    if(ucByte & 0x01)
        return(1);
    return(0);
}

// 设置某字串某Byte某Bit真值
// in  : psStr     : 需要设置的字串
//       iPosition : 要设置的的位号，0-7表示第0字节最高位到最低位，8-15表示第1字节最高位到最低位，以此类推
//       iValue    : 要设置的值，0或1
// out : psStr     : 设置好的字串
void vSetStrBit(uchar *psStr, int iPosition, int iValue)
{
    uchar *p;
    uchar ucTmp;
    p = psStr + iPosition/8;
    iPosition %= 8;
    ucTmp = 1 << (7-iPosition);
    if(iValue)
        *p |= ucTmp; // 设置1
    else
        *p &= ~ucTmp; // 设置0
}

// 另一种方式设置某字串某Byte某Bit真值
// in  : psStr     : 需要设置的字串
//       iByteNo   : 要测试的字节下标, 0开始
//       iBitNo    : 要测试的字节的位号(为兼容emv规范描述，最高位为8，最低位为1)
//       iValue    : 要设置的值，0或1
// out : psStr     : 设置好的字串
// Note: 如果iBitNo不在1-8范围内，不做设置
void vSetStrBit2(uchar *psStr, int iByteNo, int iBitNo, int iValue)
{
    uchar *p;
    uchar ucTmp;
    if(iBitNo<1 || iBitNo>8)
        return;
    p = psStr + iByteNo;
    ucTmp = 1 << (iBitNo-1); // 将1设置到对应位置
    if(iValue)
        *p |= ucTmp; // 设置1
    else
        *p &= ~ucTmp; // 设置0
}

// 测试一个字串是否全为0
// in  : psStr : 要测试的字串
//       iLen  : 字串长度
// ret : 0     : 字串所有字节全为0
//       1     : 字串至少有1字节不为0
int iTestStrZero(uchar *psStr, int iLen)
{
    while(iLen-- > 0)
        if(*psStr++ != 0)
            return(1); // 至少有1字节不为0
    return(0);
}

// 测试mark位是否都设置为0了
// in  : psStr       : 要测试的数据
//       psMark      : mark串, 要测试的位设置为1，其它位设置为0
//       iMarkLen    : mark字节数
// ret : 0           : OK，mark位都为0
//       1           : 至少1位mark位为1
int iTestStrZeroWithMark(uchar *psStr, uchar *psMark, int iMarkLen)
{
    uchar sBuf[100]; // 100字节缓冲区足够了
    int   iRet;
    if(iMarkLen > sizeof(sBuf))
        return(1); // 如果缓冲区不足，返回1，以引起注意
    memcpy(sBuf, psStr, iMarkLen);
    vAnd(sBuf, psMark, iMarkLen);
    iRet = iTestStrZero(sBuf, iMarkLen);
    return(iRet);
}

// 测试是否是10进制数字串
// in  : psStr       : 要测试的串
//       iLen        : 测试长度
// ret : 0           : OK, 都是十进制字符0x30-0x39
//       1           : 不都是十进制字符
int iTestStrDecimal(uchar *psStr, int iLen)
{
	int i;
	for(i=0; i<iLen; i++)
		if(!isdigit(psStr[i]))
			return(1);
	return(0);
}

// 测试是否是16进制数字串
// in  : psStr       : 要测试的串
//       iLen        : 测试长度
// ret : 0           : OK, 都是16进制字符0x30-0x39 0x41-0x46 0x61-0x66
//       1           : 不都是16进制字符
int iTestStrHexdecimal(uchar *psStr, int iLen)
{
	int i;
	for(i=0; i<iLen; i++)
		if(!isxdigit(psStr[i]))
			return(1);
	return(0);
}

// 测试是否是合法的8位日期
// in  : pszDate : YYYYMMDD格式日期
// ret : 0       : 合法
//       1       : 不合法
int iTestIfValidDate8(uchar *pszDate)
{
	int iYear, iMonth, iDay;
	int aiDaysPerMonth[13];

	if(strlen(pszDate) != 8)
		return(1);
	if(iTestStrDecimal(pszDate, 8) != 0)
		return(1);

	if(strcmp(pszDate, "20491231") > 0)
		return(1);
	if(strcmp(pszDate, "19500101") < 0)
		return(1);

	iYear = (int)ulA2L(pszDate, 4);
	iMonth = (int)ulA2L(pszDate+4, 2);
	iDay = (int)ulA2L(pszDate+6, 2);
	
	aiDaysPerMonth[1]=aiDaysPerMonth[3]=aiDaysPerMonth[5]=aiDaysPerMonth[7]=aiDaysPerMonth[8]=aiDaysPerMonth[10]=aiDaysPerMonth[12]=31;
	aiDaysPerMonth[4]=aiDaysPerMonth[6]=aiDaysPerMonth[9]=aiDaysPerMonth[11] = 30;
	if(iYear%400==0 || iYear%4==0 && iYear%100!=0)
		aiDaysPerMonth[2] = 29;
	else
		aiDaysPerMonth[2] = 28;

	if(iMonth<1 || iMonth>12)
		return(1);
	if(iDay<1 || iDay>aiDaysPerMonth[iMonth])
		return(1);
	return(0);
}

// 测试是否是合法的6位日期
// in  : pszDate : YYMMDD格式日期
// ret : 0       : 合法
//       1       : 不合法
int iTestIfValidDate6(uchar *pszDate)
{
	uchar szDate[9];

	if(strlen(pszDate) != 6)
		return(1);
	if(iTestStrDecimal(pszDate, 6) != 0)
		return(1);

	if(memcmp(pszDate, "49", 2) > 0)
		strcpy(szDate, "19");
	else
		strcpy(szDate, "20");
	strcat(szDate, pszDate);
	return(iTestIfValidDate8(szDate));
}

// 比较6位日期
// in  : pszDate1 : 日期1, YYMMDD
//       pszDate2 : 日期2, YYMMDD
// ret : <0       : pszDate1早于pszDate2
//       0        : pszDate1等于pszDate2
//       >0       : pszDate2晚于pszDate2
// Note: YY说明, 00-49:2000-2049 50-99:1950-1999
int iCompDate6(uchar *pszDate1, uchar *pszDate2)
{
	uchar szDate1[8], szDate2[8];
	memcpy(szDate1+2, pszDate1, 6);
	if(memcmp(pszDate1, "49", 2) <= 0)
		memcpy(szDate1, "20", 2);
	else
		memcpy(szDate1, "19", 2);
	memcpy(szDate2+2, pszDate2, 6);
	if(memcmp(pszDate2, "49", 2) <= 0)
		memcpy(szDate2, "20", 2);
	else
		memcpy(szDate2, "19", 2);
	return(memcmp(szDate1, szDate2, 8));
}

// 比较3位二进制表示日期
// in  : psDate1 : 日期1, YYMMDD
//       psDate2 : 日期2, YYMMDD
// ret : <0      : pszDate1早于pszDate2
//       0       : pszDate1等于pszDate2
//       >0      : pszDate2晚于pszDate2
// Note: YY说明, 00-49:2000-2049 50-99:1950-1999
int iCompDate3(uchar *psDate1, uchar *psDate2)
{
	uchar szDate1[7], szDate2[7];
	vOneTwo0(psDate1, 3, szDate1);
	vOneTwo0(psDate2, 3, szDate2);
	return(iCompDate6(szDate1, szDate2));
}

// 测试是否是合法的6位时间
// in  : pszTime : hhmmss格式时间
// ret : 0       : 合法
//       1       : 不合法
int iTestIfValidTime(uchar *pszTime)
{
	if(strlen(pszTime) != 6)
		return(1);
	if(iTestStrDecimal(pszTime, 6) != 0)
		return(1);

	if(memcmp(pszTime, "23", 2) > 0)
		return(1);
	if(memcmp(pszTime+2, "59", 2) > 0)
		return(1);
	if(memcmp(pszTime+4, "59", 2) > 0)
		return(1);
	return(0);
}

// 测试是否是合法日期时间
// in  : pszDateTime : 日期时间(YYYYMMDDhhmmss)
// ret : 0           : 合法
//       1           : 不合法
int iTestIfValidDateTime(uchar *pszDateTime)
{
	uchar szBuf[10];
	vMemcpy0(szBuf, pszDateTime, 8);
	if(iTestIfValidDate8(szBuf))
		return(1);
	vMemcpy0(szBuf, pszDateTime+8, 6);
	return(iTestIfValidTime(szBuf));
}

// 提供随机数扰动数据，可用于初始化随机数发生器
void vRandShuffle(ulong ulRand)
{
    uchar sRand[4];
    sg_ulLastX |= ulRand; // 扰动sg_ulLastX
    vLongToStr(ulRand, 4, sRand);
	SHA1Update(&sg_Sha1Context, sRand, 4); // 扰动sg_Sha1Context
}

// 获取随机数
// in  : iRandLen : 要求以字节计的随机数长度, 0-20, 超出部分不会生成随机数
// out : pRand    : 伪随机数
void vGetRand(void *pRand, int iRandLen)
{
	SHA1_CTX context;
    uchar sRand[100];
	int   i;
	sg_ulLastX = (7141*sg_ulLastX + 54773) % 259200;
    vLongToStr(sg_ulLastX, 4, sRand);
	for(i=4; i<8; i++)
		sRand[i] = _ucGetRand();
	SHA1Update(&sg_Sha1Context, sRand, 100); // sRand后92字节未赋值区域用来增加随机性
	memcpy(&context, &sg_Sha1Context, sizeof(context));
    SHA1Final(sRand, &context);
	if(iRandLen < 0)
		iRandLen = 0;
	if(iRandLen > 20)
		iRandLen = 20;
	memcpy(pRand, sRand, iRandLen);
}

// 去除字符串尾部空格
void vTrimTailSpace(uchar *pszStr)
{
	int i;
	for(i=strlen(pszStr)-1; i=0; i--) {
		if(pszStr[i]!=' ' && pszStr[i]!='\t')
			break;
		pszStr[i] = 0;
	}
}

// 去除CN类型数据尾部'F'
void vTrimTailF(uchar *pszCN)
{
	int i;
	for(i=0; i<(int)strlen((char *)pszCN); i++)
		if(pszCN[i] == 'F')
			pszCN[i] = 0;
}

// 去除N类型头部'0'
void vTrimHead0(uchar *pszN)
{
	if(iTestStrZero(pszN, strlen(pszN)) == 0) {
		strcpy(pszN, "0");
		return;
	}
	if(strlen(pszN) == 0) {
		strcpy(pszN, "0");
		return;
	}
	while(pszN[0] == '0') {
		memmove(pszN, pszN+1, strlen(pszN));
	}
	if(strlen(pszN) == 0)
		strcpy(pszN, "0");
}

// 将一串数字金额转变为"*9.99"格式
// iDecimalPos : 小数点位置
void vFormatAmount(uchar *pszAmount, int iDecimalPos)
{
	uchar szBuf[20], szBuf2[20];
	int   iLen;
	if(iDecimalPos<0 || iDecimalPos>3)
		iDecimalPos = 0;
	strcpy(szBuf, pszAmount);
	vTrimHead0(szBuf);
	if(iDecimalPos == 0) {
		// 无小数点
		strcpy(pszAmount, szBuf);
		return;
	}
	while(strlen(szBuf) <= (uint)iDecimalPos) {
		szBuf2[0] = '0';
		strcpy((char *)szBuf2+1, (char *)szBuf);
		strcpy((char *)szBuf, (char *)szBuf2);
	}
	iLen = strlen(szBuf);
	memcpy(pszAmount, szBuf, iLen-iDecimalPos);
	pszAmount[iLen-iDecimalPos] = '.';
	memcpy(pszAmount+iLen-iDecimalPos+1, szBuf+iLen-iDecimalPos, iDecimalPos);
	pszAmount[iLen+1] = 0;
}

// 获取2磁道卡号
// in  : pszTrack2      : Track2 data
// out : pszPan         : 卡号
// ret : 0 : OK
//       1 : 二磁道解析错误
// Note: Pan+'='+ExpireDate[4]+ServiceCode[3]+...
int iGetTrack2Pan(uchar *pszTrack2, uchar *pszPan)
{
	int i;
	for(i=0; i<19; i++) {
		if(pszTrack2[i] == '=')
			break;
		if(pszTrack2[i] == 0)
			break;
	}
	if(i==19 || pszTrack2[i]==0)
		return(1);
	if(i < 12)
		return(1);
	vMemcpy0(pszPan, pszTrack2, i);
	return(0);
}

// 获取2磁道有效期
// in  : pszTrack2      : Track2 data
// out : pszExpDate     : 有效期(YYMM)
// ret : 0 : OK
//       1 : 二磁道解析错误
// Note: Pan+'='+ExpireDate[4]+ServiceCode[3]+...
int iGetTrack2ExpDate(uchar *pszTrack2, uchar *pszExpDate)
{
	int i;
	for(i=0; i<19; i++) {
		if(pszTrack2[i] == '=')
			break;
		if(pszTrack2[i] == 0)
			break;
	}
	if(i==19 || pszTrack2[i]==0)
		return(1);
	if(strlen(pszTrack2+i) < 8)
		return(1);
	vMemcpy0(pszExpDate, pszTrack2+i+1, 4);
	return(0);
}

// 获取2磁道服务码
// in  : pszTrack2      : Track2 data
// out : pszServiceCode : 服务码
// ret : 0 : OK
//       1 : 二磁道解析错误
// Note: Pan+'='+ExpireDate[4]+ServiceCode[3]+...
int iGetTrack2ServiceCode(uchar *pszTrack2, uchar *pszServiceCode)
{
	int i;
	for(i=0; i<19; i++) {
		if(pszTrack2[i] == '=')
			break;
		if(pszTrack2[i] == 0)
			break;
	}
	if(i==19 || pszTrack2[i]==0)
		return(1);
	if(strlen(pszTrack2+i) < 8)
		return(1);
	vMemcpy0(pszServiceCode, pszTrack2+i+1+4, 3);
	return(0);
}
