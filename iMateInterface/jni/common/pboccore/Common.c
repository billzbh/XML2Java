/**************************************
File name     : Common.c
Function      : EMV�޹ص�һЩͨ�ù��ܺ���
Author        : Yu Jun
First edition : Apr 9th, 2012
Modified      : 
**************************************/
/*
ģ����ϸ����:
.	��������ɺ���vGetRand()ԭ��
    �������ʼ��ʱ�ô������������, ÿ��������Ŷ�, ����ͬһ����vRandShuffle(ulong ulRand)ʵ��
	��̬ȫ�ֱ���sg_Sha1Context��sg_ulLastX���ڱ��������������
	���մ����������Ϊ��ʼ�����ݼ����������Ŷ���SHA1ֵ
	ʹ��SHA1ֵ����ȷ���������������ͨ����������ܷ���
*/
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "VposFace.h"
#include "Sha.h"
#include "Common.h"

// ����α���������
static SHA1_CTX sg_Sha1Context;    // SHA1������
static ulong	sg_ulLastX;        // X(n-1)

// ����ĳ�ִ�ĳByteĳBit��ֵ
// in  : psStr     : ��Ҫ���Ե��ִ�
//       iPosition : Ҫ���Ե�λ�ţ�0-7��ʾ��0�ֽ����λ�����λ��8-15��ʾ��1�ֽ����λ�����λ���Դ�����
// out : 0         : ��λΪ0
//       1         : ��λΪ1
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

// ��һ�ַ�ʽ����ĳ�ִ�ĳByteĳBit��ֵ
// in  : psStr     : ��Ҫ���Ե��ִ�
//       iByteNo   : Ҫ���Ե��ֽ��±�, 0��ʼ
//       iBitNo    : Ҫ���Ե��ֽڵ�λ��(Ϊ����emv�淶���������λΪ8�����λΪ1)
// out : 0         : ��λΪ0
//       1         : ��λΪ1
// Note: ���iBitNo����1-8��Χ�ڣ�����0
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

// ����ĳ�ִ�ĳByteĳBit��ֵ
// in  : psStr     : ��Ҫ���õ��ִ�
//       iPosition : Ҫ���õĵ�λ�ţ�0-7��ʾ��0�ֽ����λ�����λ��8-15��ʾ��1�ֽ����λ�����λ���Դ�����
//       iValue    : Ҫ���õ�ֵ��0��1
// out : psStr     : ���úõ��ִ�
void vSetStrBit(uchar *psStr, int iPosition, int iValue)
{
    uchar *p;
    uchar ucTmp;
    p = psStr + iPosition/8;
    iPosition %= 8;
    ucTmp = 1 << (7-iPosition);
    if(iValue)
        *p |= ucTmp; // ����1
    else
        *p &= ~ucTmp; // ����0
}

// ��һ�ַ�ʽ����ĳ�ִ�ĳByteĳBit��ֵ
// in  : psStr     : ��Ҫ���õ��ִ�
//       iByteNo   : Ҫ���Ե��ֽ��±�, 0��ʼ
//       iBitNo    : Ҫ���Ե��ֽڵ�λ��(Ϊ����emv�淶���������λΪ8�����λΪ1)
//       iValue    : Ҫ���õ�ֵ��0��1
// out : psStr     : ���úõ��ִ�
// Note: ���iBitNo����1-8��Χ�ڣ���������
void vSetStrBit2(uchar *psStr, int iByteNo, int iBitNo, int iValue)
{
    uchar *p;
    uchar ucTmp;
    if(iBitNo<1 || iBitNo>8)
        return;
    p = psStr + iByteNo;
    ucTmp = 1 << (iBitNo-1); // ��1���õ���Ӧλ��
    if(iValue)
        *p |= ucTmp; // ����1
    else
        *p &= ~ucTmp; // ����0
}

// ����һ���ִ��Ƿ�ȫΪ0
// in  : psStr : Ҫ���Ե��ִ�
//       iLen  : �ִ�����
// ret : 0     : �ִ������ֽ�ȫΪ0
//       1     : �ִ�������1�ֽڲ�Ϊ0
int iTestStrZero(uchar *psStr, int iLen)
{
    while(iLen-- > 0)
        if(*psStr++ != 0)
            return(1); // ������1�ֽڲ�Ϊ0
    return(0);
}

// ����markλ�Ƿ�����Ϊ0��
// in  : psStr       : Ҫ���Ե�����
//       psMark      : mark��, Ҫ���Ե�λ����Ϊ1������λ����Ϊ0
//       iMarkLen    : mark�ֽ���
// ret : 0           : OK��markλ��Ϊ0
//       1           : ����1λmarkλΪ1
int iTestStrZeroWithMark(uchar *psStr, uchar *psMark, int iMarkLen)
{
    uchar sBuf[100]; // 100�ֽڻ������㹻��
    int   iRet;
    if(iMarkLen > sizeof(sBuf))
        return(1); // ������������㣬����1��������ע��
    memcpy(sBuf, psStr, iMarkLen);
    vAnd(sBuf, psMark, iMarkLen);
    iRet = iTestStrZero(sBuf, iMarkLen);
    return(iRet);
}

// �����Ƿ���10�������ִ�
// in  : psStr       : Ҫ���ԵĴ�
//       iLen        : ���Գ���
// ret : 0           : OK, ����ʮ�����ַ�0x30-0x39
//       1           : ������ʮ�����ַ�
int iTestStrDecimal(uchar *psStr, int iLen)
{
	int i;
	for(i=0; i<iLen; i++)
		if(!isdigit(psStr[i]))
			return(1);
	return(0);
}

// �����Ƿ���16�������ִ�
// in  : psStr       : Ҫ���ԵĴ�
//       iLen        : ���Գ���
// ret : 0           : OK, ����16�����ַ�0x30-0x39 0x41-0x46 0x61-0x66
//       1           : ������16�����ַ�
int iTestStrHexdecimal(uchar *psStr, int iLen)
{
	int i;
	for(i=0; i<iLen; i++)
		if(!isxdigit(psStr[i]))
			return(1);
	return(0);
}

// �����Ƿ��ǺϷ���8λ����
// in  : pszDate : YYYYMMDD��ʽ����
// ret : 0       : �Ϸ�
//       1       : ���Ϸ�
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

// �����Ƿ��ǺϷ���6λ����
// in  : pszDate : YYMMDD��ʽ����
// ret : 0       : �Ϸ�
//       1       : ���Ϸ�
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

// �Ƚ�6λ����
// in  : pszDate1 : ����1, YYMMDD
//       pszDate2 : ����2, YYMMDD
// ret : <0       : pszDate1����pszDate2
//       0        : pszDate1����pszDate2
//       >0       : pszDate2����pszDate2
// Note: YY˵��, 00-49:2000-2049 50-99:1950-1999
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

// �Ƚ�3λ�����Ʊ�ʾ����
// in  : psDate1 : ����1, YYMMDD
//       psDate2 : ����2, YYMMDD
// ret : <0      : pszDate1����pszDate2
//       0       : pszDate1����pszDate2
//       >0      : pszDate2����pszDate2
// Note: YY˵��, 00-49:2000-2049 50-99:1950-1999
int iCompDate3(uchar *psDate1, uchar *psDate2)
{
	uchar szDate1[7], szDate2[7];
	vOneTwo0(psDate1, 3, szDate1);
	vOneTwo0(psDate2, 3, szDate2);
	return(iCompDate6(szDate1, szDate2));
}

// �����Ƿ��ǺϷ���6λʱ��
// in  : pszTime : hhmmss��ʽʱ��
// ret : 0       : �Ϸ�
//       1       : ���Ϸ�
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

// �����Ƿ��ǺϷ�����ʱ��
// in  : pszDateTime : ����ʱ��(YYYYMMDDhhmmss)
// ret : 0           : �Ϸ�
//       1           : ���Ϸ�
int iTestIfValidDateTime(uchar *pszDateTime)
{
	uchar szBuf[10];
	vMemcpy0(szBuf, pszDateTime, 8);
	if(iTestIfValidDate8(szBuf))
		return(1);
	vMemcpy0(szBuf, pszDateTime+8, 6);
	return(iTestIfValidTime(szBuf));
}

// �ṩ������Ŷ����ݣ������ڳ�ʼ�������������
void vRandShuffle(ulong ulRand)
{
    uchar sRand[4];
    sg_ulLastX |= ulRand; // �Ŷ�sg_ulLastX
    vLongToStr(ulRand, 4, sRand);
	SHA1Update(&sg_Sha1Context, sRand, 4); // �Ŷ�sg_Sha1Context
}

// ��ȡ�����
// in  : iRandLen : Ҫ�����ֽڼƵ����������, 0-20, �������ֲ������������
// out : pRand    : α�����
void vGetRand(void *pRand, int iRandLen)
{
	SHA1_CTX context;
    uchar sRand[100];
	int   i;
	sg_ulLastX = (7141*sg_ulLastX + 54773) % 259200;
    vLongToStr(sg_ulLastX, 4, sRand);
	for(i=4; i<8; i++)
		sRand[i] = _ucGetRand();
	SHA1Update(&sg_Sha1Context, sRand, 100); // sRand��92�ֽ�δ��ֵ�����������������
	memcpy(&context, &sg_Sha1Context, sizeof(context));
    SHA1Final(sRand, &context);
	if(iRandLen < 0)
		iRandLen = 0;
	if(iRandLen > 20)
		iRandLen = 20;
	memcpy(pRand, sRand, iRandLen);
}

// ȥ���ַ���β���ո�
void vTrimTailSpace(uchar *pszStr)
{
	int i;
	for(i=strlen(pszStr)-1; i=0; i--) {
		if(pszStr[i]!=' ' && pszStr[i]!='\t')
			break;
		pszStr[i] = 0;
	}
}

// ȥ��CN��������β��'F'
void vTrimTailF(uchar *pszCN)
{
	int i;
	for(i=0; i<(int)strlen((char *)pszCN); i++)
		if(pszCN[i] == 'F')
			pszCN[i] = 0;
}

// ȥ��N����ͷ��'0'
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

// ��һ�����ֽ��ת��Ϊ"*9.99"��ʽ
// iDecimalPos : С����λ��
void vFormatAmount(uchar *pszAmount, int iDecimalPos)
{
	uchar szBuf[20], szBuf2[20];
	int   iLen;
	if(iDecimalPos<0 || iDecimalPos>3)
		iDecimalPos = 0;
	strcpy(szBuf, pszAmount);
	vTrimHead0(szBuf);
	if(iDecimalPos == 0) {
		// ��С����
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

// ��ȡ2�ŵ�����
// in  : pszTrack2      : Track2 data
// out : pszPan         : ����
// ret : 0 : OK
//       1 : ���ŵ���������
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

// ��ȡ2�ŵ���Ч��
// in  : pszTrack2      : Track2 data
// out : pszExpDate     : ��Ч��(YYMM)
// ret : 0 : OK
//       1 : ���ŵ���������
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

// ��ȡ2�ŵ�������
// in  : pszTrack2      : Track2 data
// out : pszServiceCode : ������
// ret : 0 : OK
//       1 : ���ŵ���������
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
