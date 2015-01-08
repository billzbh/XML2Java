/**************************************
File name     : Common.h
Function      : 一些通用功能函数
Author        : Yu Jun
First edition : Apr 9th, 2012
Modified      : 
**************************************/
#ifndef _COMMON_H
#define _COMMON_H

// 测试某字串某Byte某Bit真值
// in  : psStr     : 需要测试的字串
//       iPosition : 要测试的位号，0-7表示第0字节最高位到最低位，8-15表示第1字节最高位到最低位，以此类推
// out : 0         : 该位为0
//       1         : 该位为1
int iTestStrBit(uchar *psStr, int iPosition);

// 另一种方式测试某字串某Byte某Bit真值
// in  : psStr     : 需要测试的字串
//       iByteNo   : 要测试的字节下标, 0开始
//       iBitNo    : 要测试的字节的位号(为兼容emv规范描述，最高位为8，最低位为1)
// out : 0         : 该位为0
//       1         : 该位为1
// Note: 如果iBitNo不在1-8范围内，返回0
int iTestStrBit2(uchar *psStr, int iByteNo, int iBitNo);

// 设置某字串某Byte某Bit真值
// in  : psStr     : 需要设置的字串
//       iPosition : 要设置的的位号，0-7表示第0字节最高位到最低位，8-15表示第1字节最高位到最低位，以此类推
//       iValue    : 要设置的值，0或1
// out : psStr     : 设置好的字串
void vSetStrBit(uchar *psStr, int iPosition, int iValue);

// 另一种方式设置某字串某Byte某Bit真值
// in  : psStr     : 需要设置的字串
//       iByteNo   : 要测试的字节下标, 0开始
//       iBitNo    : 要测试的字节的位号(为兼容emv规范描述，最高位为8，最低位为1)
//       iValue    : 要设置的值，0或1
// out : psStr     : 设置好的字串
// Note: 如果iBitNo不在1-8范围内，不做设置
void vSetStrBit2(uchar *psStr, int iByteNo, int iBitNo, int iValue);

// 测试一个字串是否全为0
// in  : psStr : 要测试的字串
//       iLen  : 字串长度
// ret : 0     : 字串所有字节全为0
//       1     : 字串至少有1字节不为0
int iTestStrZero(uchar *psStr, int iLen);

// 测试mark位是否都设置为0了
// in  : psStr       : 要测试的数据
//       psMark      : mark串, 要测试的位设置为1，其它位设置为0
//       iMarkLen    : mark字节数
// ret : 0           : OK，mark位都为0
//       1           : 至少1位mark位为1
int iTestStrZeroWithMark(uchar *psStr, uchar *psMark, int iMarkLen);

// 测试是否是10进制数字串
// in  : psStr       : 要测试的串
//       iLen        : 测试长度
// ret : 0           : OK, 都是十进制字符0x30-0x39
//       1           : 不都是十进制字符
int iTestStrDecimal(uchar *psStr, int iLen);

// 测试是否是16进制数字串
// in  : psStr       : 要测试的串
//       iLen        : 测试长度
// ret : 0           : OK, 都是16进制字符0x30-0x39 0x41-0x46 0x61-0x66
//       1           : 不都是16进制字符
int iTestStrHexdecimal(uchar *psStr, int iLen);

// 测试是否是合法的8位日期
// in  : pszDate : YYYYMMDD格式日期
// ret : 0       : 合法
//       1       : 不合法
int iTestIfValidDate8(uchar *pszDate);

// 测试是否是合法的6位日期
// in  : pszDate : YYMMDD格式日期
// ret : 0       : 合法
//       1       : 不合法
int iTestIfValidDate6(uchar *pszDate);

// 比较6位日期
// in  : pszDate1 : 日期1, YYMMDD
//       pszDate2 : 日期2, YYMMDD
// ret : <0       : pszDate1早于pszDate2
//       0        : pszDate1等于pszDate2
//       >0       : pszDate2晚于pszDate2
// Note: YY说明, 00-49:2000-2049 50-99:1950-1999
int iCompDate6(uchar *pszDate1, uchar *pszDate2);

// 比较3位二进制表示日期
// in  : psDate1 : 日期1, YYMMDD
//       psDate2 : 日期2, YYMMDD
// ret : <0      : pszDate1早于pszDate2
//       0       : pszDate1等于pszDate2
//       >0      : pszDate2晚于pszDate2
// Note: YY说明, 00-49:2000-2049 50-99:1950-1999
int iCompDate3(uchar *psDate1, uchar *psDate2);

// 测试是否是合法的6位时间
// in  : pszTime : hhmmss格式时间
// ret : 0       : 合法
//       1       : 不合法
int iTestIfValidTime(uchar *pszTime);

// 测试是否是合法日期时间
// in  : pszDateTime : 日期时间(YYYYMMDDhhmmss)
// ret : 0           : 合法
//       1           : 不合法
int iTestIfValidDateTime(uchar *pszDateTime);

// 提供随机数扰动数据，可用于初始化随机数发生器
void vRandShuffle(ulong ulRand);

// 获取随机数
// in  : iRandLen : 要求以字节计的随机数长度, 0-20, 超出部分不会生成随机数
// out : pRand    : 伪随机数
void vGetRand(void *pRand, int iRandLen);

// 去除字符串尾部空格
void vTrimTailSpace(uchar *pszStr);

// 去除CN类型数据尾部'F'
void vTrimTailF(uchar *pszCN);

// 去除N类型头部'0'
void vTrimHead0(uchar *pszN);

// 将一串数字金额转变为"*9.99"格式
// iDecimalPos : 小数点位置
void vFormatAmount(uchar *pszAmount, int iDecimalPos);

// 获取2磁道卡号
// in  : pszTrack2      : Track2 data
// out : pszPan         : 卡号
// ret : 0 : OK
//       1 : 二磁道解析错误
// Note: Pan+'='+ExpireDate[4]+ServiceCode[3]+...
int iGetTrack2Pan(uchar *pszTrack2, uchar *pszPan);

// 获取2磁道有效期
// in  : pszTrack2      : Track2 data
// out : pszExpDate     : 有效期(YYMM)
// ret : 0 : OK
//       1 : 二磁道解析错误
// Note: Pan+'='+ExpireDate[4]+ServiceCode[3]+...
int iGetTrack2ExpDate(uchar *pszTrack2, uchar *pszExpDate);

// 获取2磁道服务码
// in  : pszTrack2      : Track2 data
// out : pszServiceCode : 服务码
// ret : 0 : OK
//       1 : 二磁道解析错误
// Note: Pan+'='+ExpireDate[4]+ServiceCode[3]+...
int iGetTrack2ServiceCode(uchar *pszTrack2, uchar *pszServiceCode);

#endif
