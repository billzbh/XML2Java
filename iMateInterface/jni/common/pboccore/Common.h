/**************************************
File name     : Common.h
Function      : һЩͨ�ù��ܺ���
Author        : Yu Jun
First edition : Apr 9th, 2012
Modified      : 
**************************************/
#ifndef _COMMON_H
#define _COMMON_H

// ����ĳ�ִ�ĳByteĳBit��ֵ
// in  : psStr     : ��Ҫ���Ե��ִ�
//       iPosition : Ҫ���Ե�λ�ţ�0-7��ʾ��0�ֽ����λ�����λ��8-15��ʾ��1�ֽ����λ�����λ���Դ�����
// out : 0         : ��λΪ0
//       1         : ��λΪ1
int iTestStrBit(uchar *psStr, int iPosition);

// ��һ�ַ�ʽ����ĳ�ִ�ĳByteĳBit��ֵ
// in  : psStr     : ��Ҫ���Ե��ִ�
//       iByteNo   : Ҫ���Ե��ֽ��±�, 0��ʼ
//       iBitNo    : Ҫ���Ե��ֽڵ�λ��(Ϊ����emv�淶���������λΪ8�����λΪ1)
// out : 0         : ��λΪ0
//       1         : ��λΪ1
// Note: ���iBitNo����1-8��Χ�ڣ�����0
int iTestStrBit2(uchar *psStr, int iByteNo, int iBitNo);

// ����ĳ�ִ�ĳByteĳBit��ֵ
// in  : psStr     : ��Ҫ���õ��ִ�
//       iPosition : Ҫ���õĵ�λ�ţ�0-7��ʾ��0�ֽ����λ�����λ��8-15��ʾ��1�ֽ����λ�����λ���Դ�����
//       iValue    : Ҫ���õ�ֵ��0��1
// out : psStr     : ���úõ��ִ�
void vSetStrBit(uchar *psStr, int iPosition, int iValue);

// ��һ�ַ�ʽ����ĳ�ִ�ĳByteĳBit��ֵ
// in  : psStr     : ��Ҫ���õ��ִ�
//       iByteNo   : Ҫ���Ե��ֽ��±�, 0��ʼ
//       iBitNo    : Ҫ���Ե��ֽڵ�λ��(Ϊ����emv�淶���������λΪ8�����λΪ1)
//       iValue    : Ҫ���õ�ֵ��0��1
// out : psStr     : ���úõ��ִ�
// Note: ���iBitNo����1-8��Χ�ڣ���������
void vSetStrBit2(uchar *psStr, int iByteNo, int iBitNo, int iValue);

// ����һ���ִ��Ƿ�ȫΪ0
// in  : psStr : Ҫ���Ե��ִ�
//       iLen  : �ִ�����
// ret : 0     : �ִ������ֽ�ȫΪ0
//       1     : �ִ�������1�ֽڲ�Ϊ0
int iTestStrZero(uchar *psStr, int iLen);

// ����markλ�Ƿ�����Ϊ0��
// in  : psStr       : Ҫ���Ե�����
//       psMark      : mark��, Ҫ���Ե�λ����Ϊ1������λ����Ϊ0
//       iMarkLen    : mark�ֽ���
// ret : 0           : OK��markλ��Ϊ0
//       1           : ����1λmarkλΪ1
int iTestStrZeroWithMark(uchar *psStr, uchar *psMark, int iMarkLen);

// �����Ƿ���10�������ִ�
// in  : psStr       : Ҫ���ԵĴ�
//       iLen        : ���Գ���
// ret : 0           : OK, ����ʮ�����ַ�0x30-0x39
//       1           : ������ʮ�����ַ�
int iTestStrDecimal(uchar *psStr, int iLen);

// �����Ƿ���16�������ִ�
// in  : psStr       : Ҫ���ԵĴ�
//       iLen        : ���Գ���
// ret : 0           : OK, ����16�����ַ�0x30-0x39 0x41-0x46 0x61-0x66
//       1           : ������16�����ַ�
int iTestStrHexdecimal(uchar *psStr, int iLen);

// �����Ƿ��ǺϷ���8λ����
// in  : pszDate : YYYYMMDD��ʽ����
// ret : 0       : �Ϸ�
//       1       : ���Ϸ�
int iTestIfValidDate8(uchar *pszDate);

// �����Ƿ��ǺϷ���6λ����
// in  : pszDate : YYMMDD��ʽ����
// ret : 0       : �Ϸ�
//       1       : ���Ϸ�
int iTestIfValidDate6(uchar *pszDate);

// �Ƚ�6λ����
// in  : pszDate1 : ����1, YYMMDD
//       pszDate2 : ����2, YYMMDD
// ret : <0       : pszDate1����pszDate2
//       0        : pszDate1����pszDate2
//       >0       : pszDate2����pszDate2
// Note: YY˵��, 00-49:2000-2049 50-99:1950-1999
int iCompDate6(uchar *pszDate1, uchar *pszDate2);

// �Ƚ�3λ�����Ʊ�ʾ����
// in  : psDate1 : ����1, YYMMDD
//       psDate2 : ����2, YYMMDD
// ret : <0      : pszDate1����pszDate2
//       0       : pszDate1����pszDate2
//       >0      : pszDate2����pszDate2
// Note: YY˵��, 00-49:2000-2049 50-99:1950-1999
int iCompDate3(uchar *psDate1, uchar *psDate2);

// �����Ƿ��ǺϷ���6λʱ��
// in  : pszTime : hhmmss��ʽʱ��
// ret : 0       : �Ϸ�
//       1       : ���Ϸ�
int iTestIfValidTime(uchar *pszTime);

// �����Ƿ��ǺϷ�����ʱ��
// in  : pszDateTime : ����ʱ��(YYYYMMDDhhmmss)
// ret : 0           : �Ϸ�
//       1           : ���Ϸ�
int iTestIfValidDateTime(uchar *pszDateTime);

// �ṩ������Ŷ����ݣ������ڳ�ʼ�������������
void vRandShuffle(ulong ulRand);

// ��ȡ�����
// in  : iRandLen : Ҫ�����ֽڼƵ����������, 0-20, �������ֲ������������
// out : pRand    : α�����
void vGetRand(void *pRand, int iRandLen);

// ȥ���ַ���β���ո�
void vTrimTailSpace(uchar *pszStr);

// ȥ��CN��������β��'F'
void vTrimTailF(uchar *pszCN);

// ȥ��N����ͷ��'0'
void vTrimHead0(uchar *pszN);

// ��һ�����ֽ��ת��Ϊ"*9.99"��ʽ
// iDecimalPos : С����λ��
void vFormatAmount(uchar *pszAmount, int iDecimalPos);

// ��ȡ2�ŵ�����
// in  : pszTrack2      : Track2 data
// out : pszPan         : ����
// ret : 0 : OK
//       1 : ���ŵ���������
// Note: Pan+'='+ExpireDate[4]+ServiceCode[3]+...
int iGetTrack2Pan(uchar *pszTrack2, uchar *pszPan);

// ��ȡ2�ŵ���Ч��
// in  : pszTrack2      : Track2 data
// out : pszExpDate     : ��Ч��(YYMM)
// ret : 0 : OK
//       1 : ���ŵ���������
// Note: Pan+'='+ExpireDate[4]+ServiceCode[3]+...
int iGetTrack2ExpDate(uchar *pszTrack2, uchar *pszExpDate);

// ��ȡ2�ŵ�������
// in  : pszTrack2      : Track2 data
// out : pszServiceCode : ������
// ret : 0 : OK
//       1 : ���ŵ���������
// Note: Pan+'='+ExpireDate[4]+ServiceCode[3]+...
int iGetTrack2ServiceCode(uchar *pszTrack2, uchar *pszServiceCode);

#endif
