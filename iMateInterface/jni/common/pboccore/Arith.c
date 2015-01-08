/**************************************
File name     : arith.c
Function      : ��������
Author        : Yu Jun
First edition : Apr 10th, 2012
Note          : ΪEMV���Ľ��������Ʊ�ģ�飬����EMV�淶��Ƶ��������ݶ�����, ģ�鲢������Ч������
Modified      : 
**************************************/
/*
ģ����ϸ����:
	����: EMV�淶Ҫ��Ľ�������ָ�ʽ, 12N����4B��, �����Ҫ��ʵ��12N����������10����/16����ת��, �ʱ�д�˱�ģ��
.	��ģ��ʵ�������ⳤ���������(2-16����,֧�ָ���Ľ�����ʵ������,��ͬ)���ļӼ��˳����㼰������ƵĽ���ת��
.	�������ĺ�����iAdd() iSub() iMult() iDiv() iBaseConvert()
	ʵ��: �����ڲ���ʾΪuchar�����飬��λ��ǰ����ָ��ӳ��ȱ�ʾ
		  ����Ԫ��Ϊ�趨�Ľ��Ƶ�λ����16���Ƶ�'F'��ʾΪ0x0F. 10����'27'��ʾΪ"\x07\x02"
	      ����ǰ�Ƚ�����Ŀɶ����ִ�ת��Ϊ�ڲ���ʾ, ����󽫽����ת���ɿɶ����ִ����
.	�ⲿ�ӿں���ʹ�÷�����
	������ʾΪ�ɶ��ַ�������10���Ƶ�135��ʾΪ"135"��16���Ƶ�23F1��ʾΪ"23F1"��"23f1"
	�����11-16������10-15���ô�дA-F��Сдa-f��ʾ
	���������Ϊ11-16���ƣ�10-15��ʾΪ��д��ĸA-F
.	��MAX_DIGIT��ʾ֧�ֵ����λ�������Ϊ���㷢����������ɼӴ��䶨��, ��ò�����32767
	ע��: MAX_DIGITΪ��ʾ���н������ݵ���󳤶�, �����Ҫ����2��������, ��ע��ú궨��Ĵ�С�Ƿ���ܲ���
*/
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "VposFace.h"
#include "Arith.h"

#define MAX_DIGIT       128

// A = B
static void vAssign(uchar *psA, uchar *psB, uint uiLen)
{
	memcpy(psA, psB, uiLen);
}

// �Ƚ�A��B
// ret :  1 : A > B
//        0 : A == B
//       -1 : A < B
static int iComp(uchar *psA, uchar *psB, uint uiLen)
{
	int i;
	if(uiLen == 0)
		return(0);
	for(i=uiLen-1; i>=0; i--) {
		if(psA[i] > psB[i])
			return(1);
		if(psA[i] < psB[i])
			return(-1);
	}
	return(0);
}

// A = 0
static void vSetZero(uchar *psA, uint uiLen)
{
	memset(psA, 0, uiLen);
}

// �ж�psA�Ƿ�Ϊ0
// in  : psA   : ����
//       uiLen : ��������
// ret : 0     : ����0
//       1     : psA == 0
static int iIsZero(uchar *psA, uint uiLen)
{
	int i;
	for(i=0; i<(int)uiLen; i++)
		if(*psA++)
			return(0);
	return(1);
}

// ������λ
// in  : psA
//       uiLen   : ���鳤��
//       iShift  : ��λ��־
//                 >0:��������(�൱�ڳ��Խ�����)
//                 <0:��������(�൱�ڳ��Խ�����)
//                  0:����λ
// ret : ���iShiftΪ1��-1, �����Ƴ�ȥ������Ԫֵ
//       �����������0
// Note: �Ƴ����Ŀ�λ��0
static uchar ucShift(uchar *psA, uint uiLen, int iShift)
{
	uchar ucRet;
    int   i;
	if(iShift==0 || uiLen==0)
		return(0); // iShift==0������λ, uiLen==0������λ
	if(iShift>=(int)uiLen || -iShift>=(int)uiLen) {
		// ��λ��̫����Ϊ������Ч���ֶ����Ƴ����飬���������㼴��
		if(iShift == 1)
			ucRet = psA[uiLen-1];
		else if(iShift == -1)
			ucRet = psA[0];
		else
			ucRet = 0;
        vSetZero(psA, uiLen);
		return(ucRet);
	}
	// ׼���÷���ֵ
	if(iShift == 1)
		ucRet = psA[uiLen-1];
	else if(iShift == -1)
		ucRet = psA[0];
	else
		ucRet = 0;
	// ��λ
    if(iShift > 0) {
        // ��������(�൱�ڳ���Base)
        for(i=uiLen-1; i>=0; i--) {
			if(i >= iShift)
				psA[i] = psA[i-iShift];
			else
				psA[i] = 0;
		}
    } else {
        // ��������(�൱�ڳ���Base)
		iShift = -iShift; // ��iShift����
		for(i=0; i<(int)uiLen; i++) {
			if(i+iShift < (int)uiLen)
				psA[i] = psA[i+iShift];
			else
				psA[i] = 0;
		}
    }
	return(ucRet);
}

// ������� A = A + B
// ABC��ʾ : ��λ��ǰ, ÿ��Ԫ��ʾһλx��������
// in  : psA, psB
//       uiLen   : λ��
//       uiBase  : ��������2-16
// out : piA
// ret : 0|1     : carry
//       -1      : uiBase��֧��
static int iAdd(uchar *psA, uchar *psB, uint uiLen, uint uiBase)
{
    int i;
    int iCarry;
	if(uiBase<2 || uiBase>16)
		return(-1);
    for(i=0, iCarry=0; i<(int)uiLen; i++, psA++, psB++) {
        *psA += *psB + iCarry;
        iCarry = *psA / uiBase;
        *psA %= uiBase;
    }
    return(iCarry);
}

// ������� A = A - B
// ABC��ʾ : ��λ��ǰ, ÿ��Ԫ��ʾһλx��������
// in  : psA, psB
//       uiLen   : λ�����������0
//       uiBase  : ��������2-16
// out : psA
// ret : 0|1     : borrow
//       -1      : uiBase��֧��
static int iSub(uchar *psA, uchar *psB, uint uiLen, uint uiBase)
{
    int i;
	int iT;
    int iBorrow;
	if(uiBase<2 || uiBase>16)
		return(-1);
    for(i=0, iBorrow=0; i<(int)uiLen; i++, psA++, psB++) {
		iT = (int)(*psA) - (int)(*psB) - iBorrow;
        *psA -= *psB + iBorrow;
        if(iT < 0) {
            iBorrow = 1;
            *psA = iT + uiBase;
        } else {
            iBorrow = 0;
			*psA = iT;
		}
    }
    return(iBorrow);
}

// �������Ե�λ�� A = A * ucMultiple
// A��λ��ǰ, ÿ��Ԫ��ʾһλ����, ucMultipleΪ1λx��������
// in  : psA
//       ucMultiple : ����, <=16
//       uiLen      : A��λ��
//       uiBase     : ��������2-16
// out : psA        : λ��ΪuiLen
// ret : >0         : Carry, ���ܴ���1
//       -1         : uiBase��֧�ֻ����̫��
static int iMultDigit(uchar *psA, uchar ucMultiple, uint uiLen, uint uiBase)
{
    int i;
    int iCarry;
	if(uiBase<2 || uiBase>16 || ucMultiple>=16)
		return(-1);
    for(i=0, iCarry=0; i<(int)uiLen; i++, psA++) {
        *psA = *psA * ucMultiple + iCarry;
        iCarry = *psA / uiBase;
        *psA %= uiBase;
    }
    return(iCarry);
}

// A = B * C
// ABC��ʾ : ��λ��ǰ, ÿ��Ԫ��ʾһλx��������
// in  : psB, psC : λ��ΪuiLen
//       uiLen    : B��C��λ��������С�ڵ���MAX_DIGIT
//       uiBase   : ��������2-16
// out : psA      : λ��ΪuiLen*2
// ret : 0        : OK
//       -1       : uiBase��֧��
//       -2       : ���
static int iMult(uchar *psA, uchar *psB, uchar *psC, uint uiLen, uint uiBase)
{
    int   i;
    uchar sT[MAX_DIGIT*2];
    
	if(uiBase<2 || uiBase>16)
		return(-1);
    if(uiLen > MAX_DIGIT)
        return(-2);
    
    vSetZero(psA, uiLen*2);
    for(i=0; i<(int)uiLen; i++) {
		memset(sT, 0, sizeof(sT));
		vAssign(sT, psB, uiLen);
		ucShift(sT, uiLen*2, i/*����iλ*/);
        iMultDigit(sT, psC[i], uiLen*2, uiBase);
        iAdd(psA, sT, uiLen*2, uiBase);
    }
    return(0);
}

// A = B / C
// R = B % C
// ABCR��ʾ : ��λ��ǰ, ÿ��Ԫ��ʾһλx��������
// in  : psB, psC
//       uiLen    : A��B��C��Rλ��������С�ڵ���MAX_DIGIT
//       uiBase   : ��������2-16
// out : psA��psR : λ��ΪuiLen
// ret : 0        : OK
//       -1       : uiBase��֧��
//       -2       : ���(��0���������)
//       -3       : C == 0
static int iDiv(uchar *psA, uchar *psR, uchar *psB, uchar *psC, uint uiLen, uint uiBase)
{
	int   iRet;
	uchar ucRet;
    int   i;
	int   iQuotientPos;  // �����ڵ�λ��
    uchar sB[MAX_DIGIT]; // B��ʱ����
	uchar sC[MAX_DIGIT]; // C��ʱ����
	uchar sT[MAX_DIGIT]; // ��ʱ����
	uchar sUnit[MAX_DIGIT]; // ��ʱ����, ���ڼ�����ʱ����һ����λ��ֵ
    
	if(uiBase<2 || uiBase>16)
		return(-1);
    if(uiLen > MAX_DIGIT)
        return(-2);
	if(uiLen == 0)
		return(-3);
	if(iIsZero(psC, uiLen))
		return(-3);

    vSetZero(psA, uiLen);     // A = 0
	vAssign(sB, psB, uiLen); // sB = B, sB���ּ�����ʣ��δ��ɵı�����
	while(iComp(sB, psC, uiLen) >= 0) {
		// ʣ��δ��ɵı����������ڳ�������Ҫ��������
		vAssign(sC, psC, uiLen); // sC = C, sC������ʱ����
		for(i=0; i<(int)uiLen; i++) {
			ucRet = ucShift(sC, uiLen, 1); // ����һλ
			if(ucRet)
				break;
			if(iComp(sB, sC, uiLen) < 0)
				break;
		}
		iQuotientPos = i; // �����ڵ�λ��
		vAssign(sUnit, psC, uiLen); // sUnit = C, sUnit������ʱ����
		ucShift(sUnit, uiLen, iQuotientPos); // ��sUnit������1�����㵥λ
		vAssign(sT, sUnit, uiLen); // sT������ʱ����, ����ֵΪ1����λ
		for(i=1; i<(int)uiBase; i++) {
			iRet = iAdd(sT, sUnit, uiLen, uiBase);
			if(iRet)
				break; // �������Ϊ�����1λ��=i
			if(iComp(sT, sB, uiLen) > 0)
				break; // �����1λ��=i
		}
		psA[iQuotientPos] = i; // ������
        iMultDigit(sUnit, (uchar)i, uiLen, uiBase); // �������
		iSub(sB, sUnit, uiLen, uiBase);
	}
	vAssign(psR, sB, uiLen);

    return(0);
}

// ����ת����2-16�����Ƽ�ת��
// in  : psS     : Դ����
//       uiSBase : Դ���ݽ���
//       uiSLen  : Դ���ݳ���
//       uiTBase : Ŀ�����ݽ���
//       uiTLen  : Ŀ�����ݻ���������
// out : psT     : Ŀ������
// ret : 0       : OK
//       -1      : Base�Ƿ�
//       -2      : ���
static int iBaseConvert(uchar *psS, uint uiSBase, uint uiSLen, uchar *psT, uint uiTBase, uint uiTLen)
{   
	int   i, j;
	uint  uiTmp;
    uchar sTBaseInSBase[MAX_DIGIT]; // ��uiSBase�����Ľ��Ʊ�ʾ��uiTbase������
	uchar sS[MAX_DIGIT];
	uchar sTmp1[MAX_DIGIT], sTmp2[MAX_DIGIT];

	if(uiSBase<2 || uiSBase>16 || uiTBase<2 || uiTBase>16)
		return(-1);
	if(uiSLen > MAX_DIGIT)
		return(-2);
    // ���쳤��ΪuiSLen��sTBaseInSBase
    vSetZero(sTBaseInSBase, uiSLen);
	uiTmp = uiTBase; // ��uiTmp����uiTBase��������
    for(i=0; i<5; i++) { // ��2���Ʊ�ʾ16, 5λ��Ҳ�㹻��
		sTBaseInSBase[i] = uiTmp % uiSBase;
		uiTmp /= uiSBase;
	}
	// ת��
    vSetZero(sS, sizeof(sS));
    vAssign(sS, psS, uiSLen); // sS���ڼ�¼Դ����psS�ڼ�������е��м�����
	if(uiSLen < 5)
		uiSLen = 5; // ҪuiSLen���ٳ���Ϊ5����ֹĳЩʱ��Դ���ݳ���̫�̵�����
	for(i=0; i<(int)uiTLen; i++) {
		iDiv(sTmp1, sTmp2, sS, sTBaseInSBase, uiSLen, uiSBase); // sTmp1=�� sTmp2=����
		for(j=4, psT[i]=0; j>=0; j--) {
			psT[i] *= uiSBase;
			psT[i] += sTmp2[j];
		}
		vAssign(sS, sTmp1, uiSLen);
	}
	if(!iIsZero(sS, uiSLen))
		return(-2); // ����Ŀ�껺��������������
	return(0);
}

// ���ɶ����ִ�ת���ɴ�������
// in  : pszSource    : uiBase��ʾ��Դ���ִ�
//       uiTargetSize : ���������С
//       uiBase       : Դ���ִ�����, 2-16
// out : psTarget     : ���ɵĴ�������
//       puiValidLen  : ��Ч���ֳ���, NULL��ʾ����Ҫ����
// ret : 0            : OK
//       -1           : uiBase������Χ
//       -2           : Ŀ���������ռ䲻��
//       -3           : Դ���ִ������ֳ������Ʒ�Χ
static int iEncode(char *pszSource, uchar *psTarget, uint uiTargetSize, uint uiBase, uint *puiValidLen)
{
	int  i, j;
	int  iDigit;
	int  iLen; // Դ���ִ���Ч���ֳ���

	if(uiBase<2 || uiBase>16)
		return(-1);

	// passǰ��'0'
	while(*pszSource=='0' && *pszSource!=0)
		pszSource++;
	iLen = strlen(pszSource);
	if(iLen > (int)uiTargetSize)
		return(-2);

	// ת��
    vSetZero(psTarget, uiTargetSize);
	// i:Դ�±� j:Ŀ���±�
	for(i=iLen-1, j=0; i>=0; i--, j++) {
		if(pszSource[i]>='0' && pszSource[i]<='9')
			iDigit = pszSource[i] - '0';
		else if(pszSource[i]>='A' && pszSource[i]<='F')
			iDigit = pszSource[i] - 'A' + 10;
		else if(pszSource[i]>='a' && pszSource[i]<='f')
			iDigit = pszSource[i] - 'a' + 10;
		else
			return(-3); // ����16�������ڽ�����
		if(iDigit >= (int)uiBase)
			return(-3); // ����uiBase���Ʊ�ʾ��Χ��
		psTarget[j] = iDigit;
	}
	if(puiValidLen)
		*puiValidLen = iLen; // ���Ҫ�󷵻���Ч���ֳ��ȣ�������Ч���ֳ���
	return(0);
}

// ����������ת���ɿɶ����ִ�
// in  : psSource     : Դ��������
//       uiSourceSize : Դ���������С
//       uiTargetSize : Ŀ�����ִ��ռ��С(������β��0��Ԥ���Ļ�����Ӧ���ٶ�1�ֽ�)
//       uiBase       : Դ�����������, 2-16
//       ucFillChar   : Ŀ�����ִ�ǰ���ַ�(0:�����κ��ַ�, ����:ǰ���ַ�,��õĲ�λ�ַ�Ϊ'0')
// out : pszTarget    : Ŀ�����ִ�(��uiBase���Ʊ�ʾ)
// ret : 0            : OK
//       -1           : uiBase������Χ
//       -2           : Ŀ�����ִ��ռ䲻��
//       -3           : Դ���������������ֳ������Ʒ�Χ
static int iDecode(uchar *psSource, uint uiSourceSize, char *pszTarget, uint uiTargetSize, uint uiBase, uchar ucFillChar)
{
	int  i, j;
	int  iLen; // Դ����������Ч���ֳ���

	if(uiBase<2 || uiBase>16)
		return(-1);

	// ���Դ����������Ч���ֳ���
	for(i=(int)uiSourceSize-1; i>0; i--)
		if(psSource[i])
			break;
	iLen = i + 1;
	if(iLen > (int)uiTargetSize)
		return(-2);

	// Ŀ����0, ׼����װ���ִ�
	memset(pszTarget, 0, uiTargetSize+1); // uiTargetSize����������0�����Ҫһ����0
	// �����Ҫ���λ�ַ�
	if(ucFillChar) {
		while(iLen < (int)uiTargetSize) {
			*pszTarget++ = ucFillChar;
			uiTargetSize --;
		}
	}

	// ת��
	// i:Դ�±� j:Ŀ���±�
	for(i=iLen-1, j=0; i>=0; i--, j++) {
		if(psSource[i] >= uiBase)
			return(2);
		if(psSource[i] < 10)
			pszTarget[j] = psSource[i] + '0';
		else
			pszTarget[j] = psSource[i] - 10 + 'A';
	}
	return(0);
}

// A = B + C
// in  : uiSizeA    : pszA�ռ�(������β��0��Ҫ����1�ֽ�)
//       uiBase     : ���ƣ�2-16
//       ucFlllChar : ���Aǰ������ַ�(0:�����κ��ַ�, ����:ǰ���ַ�,��õĲ�λ�ַ�Ϊ'0')
// ret : 0          : OK
//       -1         : uiBase������Χ
//       -2         : �ڲ��ռ䲻��
//       -3         : Դ���ִ������ֳ������Ʒ�Χ
int iStrNumAdd(uchar *pszA, uint uiSizeA, uchar *pszB, uchar *pszC, uint uiBase, uchar ucFillChar)
{
	int   iRet;
	int   iCarry;
    uchar sB[MAX_DIGIT], sC[MAX_DIGIT];

	iRet = iEncode(pszB, sB, MAX_DIGIT, uiBase, NULL);
	if(iRet)
		return(iRet);
	iRet = iEncode(pszC, sC, MAX_DIGIT, uiBase, NULL);
	if(iRet)
		return(iRet);
	iCarry = iAdd(sB, sC, MAX_DIGIT, uiBase);
	if(iCarry < 0)
		return(iCarry);
	if(iCarry > 0)
		return(-2); // �н�λ��Ϊ�ڲ��ռ䲻��
	iRet = iDecode(sB, MAX_DIGIT, pszA, uiSizeA, uiBase, ucFillChar);
	if(iRet)
		return(iRet);
	return(0);
}

// A = B - C
// in  : uiSizeA    : pszA�ռ�(������β��0��Ҫ����1�ֽ�)
//       uiBase     : ���ƣ�2-16
//       ucFlllChar : ���Aǰ������ַ�(0:�����κ��ַ�, ����:ǰ���ַ�,��õĲ�λ�ַ�Ϊ'0')
// ret : 0          : OK
//       1          : B < C
//       -1         : uiBase������Χ
//       -2         : �ڲ��ռ䲻��
//       -3         : Դ���ִ������ֳ������Ʒ�Χ
int iStrNumSub(uchar *pszA, uint uiSizeA, uchar *pszB, uchar *pszC, uint uiBase, uchar ucFillChar)
{
	int   iRet;
	int   iBorrow;
    uchar sB[MAX_DIGIT], sC[MAX_DIGIT];

	iRet = iEncode(pszB, sB, MAX_DIGIT, uiBase, NULL);
	if(iRet)
		return(iRet);
	iRet = iEncode(pszC, sC, MAX_DIGIT, uiBase, NULL);
	if(iRet)
		return(iRet);
	iBorrow = iSub(sB, sC, MAX_DIGIT, uiBase);
	if(iBorrow < 0)
		return(iBorrow);
	if(iBorrow > 0)
		return(1);
	iRet = iDecode(sB, MAX_DIGIT, pszA, uiSizeA, uiBase, ucFillChar);
	if(iRet)
		return(iRet);
	return(0);
}

// A = B * C
// in  : uiSizeA    : pszA�ռ�(������β��0��Ҫ����1�ֽ�)
//       uiBase     : ���ƣ�2-16
//       ucFlllChar : ���Aǰ������ַ�(0:�����κ��ַ�, ����:ǰ���ַ�,��õĲ�λ�ַ�Ϊ'0')
// ret : 0          : OK
//       -1         : uiBase������Χ
//       -2         : �ڲ��ռ䲻��
//       -3         : Դ���ִ������ֳ������Ʒ�Χ
int iStrNumMult(uchar *pszA, uint uiSizeA, uchar *pszB, uchar *pszC, uint uiBase, uchar ucFillChar)
{
	int   iRet;
	uint  uiBValidLen, uiCValidLen; // ��Ч���ֳ���
    uchar sA[MAX_DIGIT*2], sB[MAX_DIGIT], sC[MAX_DIGIT];

	iRet = iEncode(pszB, sB, MAX_DIGIT, uiBase, &uiBValidLen);
	if(iRet)
		return(iRet);
	iRet = iEncode(pszC, sC, MAX_DIGIT, uiBase, &uiCValidLen);
	if(iRet)
		return(iRet);
	vSetZero(sA, sizeof(sA));
	iRet = iMult(sA, sB, sC, (uiBValidLen>uiCValidLen?uiBValidLen:uiCValidLen), uiBase);
	if(iRet)
		return(iRet);
	iRet = iDecode(sA, MAX_DIGIT, pszA, uiSizeA, uiBase, ucFillChar);
	if(iRet)
		return(iRet);
	return(0);
}

// A = B / C
// R = B % C
// in  : pszA       : �̣��������NULL, ��ʾ��������
//       uiSizeA    : pszA�ռ�(������β��0��Ҫ����1�ֽ�)
//       pszR       : �࣬�������NULL, ��ʾ����������
//       uiSizeR    : pszR�ռ�(������β��0��Ҫ����1�ֽ�)
//       uiBase     : ���ƣ�2-16
//       ucFlllChar : ���A��Rǰ������ַ�(0:�����κ��ַ�, ����:ǰ���ַ�,��õĲ�λ�ַ�Ϊ'0')
// ret : 0          : OK
//       1          : ����Ϊ0
//       -1         : uiBase������Χ
//       -2         : �ڲ��ռ䲻��
//       -3         : Դ���ִ������ֳ������Ʒ�Χ
int iStrNumDiv(uchar *pszA, uint uiSizeA, uchar *pszR, uint uiSizeR, uchar *pszB, uchar *pszC, uint uiBase, uchar ucFillChar)
{
	int   iRet;
	uint  uiBValidLen, uiCValidLen; // ��Ч���ֳ���
    uchar sA[MAX_DIGIT], sR[MAX_DIGIT], sB[MAX_DIGIT], sC[MAX_DIGIT];

	iRet = iEncode(pszB, sB, MAX_DIGIT, uiBase, &uiBValidLen);
	if(iRet)
		return(iRet);
	iRet = iEncode(pszC, sC, MAX_DIGIT, uiBase, &uiCValidLen);
	if(iRet)
		return(iRet);
	vSetZero(sA, sizeof(sA));
	vSetZero(sR, sizeof(sR));
	iRet = iDiv(sA, sR, sB, sC, (uiBValidLen>uiCValidLen?uiBValidLen:uiCValidLen), uiBase);
	if(iRet == -3)
		return(1);
	if(iRet)
		return(iRet);
	if(pszA) {
		iRet = iDecode(sA, MAX_DIGIT, pszA, uiSizeA, uiBase, ucFillChar);
		if(iRet)
			return(iRet);
	}
	if(pszR) {
		iRet = iDecode(sR, MAX_DIGIT, pszR, uiSizeR, uiBase, ucFillChar);
		if(iRet)
			return(iRet);
	}
	return(0);
}

// ret : 1  : A > B
//       0  : A == B
//       -1 : A < B
int iStrNumCompare(uchar *pszA, uchar *pszB, uint uiBase)
{
	uchar sT[MAX_DIGIT+1];
	int   iRet;
	iRet = iStrNumSub(sT, MAX_DIGIT, pszA, pszB, uiBase, 0);
	if(iRet == 1)
		return(-1); // A < B
	if(strcmp(sT, "0") != 0)
		return(1); // A > B
	return(0); // A == B (�������, ��Ϊ���, �߲�Ӧ��ȷ��������û��Խ��)
}

// ����ת��
// in  : pszSource    : Դ���ִ�
//       uiSourceBase : Դ���ִ�����
//       uiTargetSize : Ŀ�괮�ռ��С(������β��0��Ҫ����1�ֽ�)
//       uiTargetBase : Ŀ�괮����
//       ucFlllChar   : ���pszTargetǰ������ַ�(0:�����κ��ַ�, ����:ǰ���ַ�,��õĲ�λ�ַ�Ϊ'0')
// ret : pszTarget    : Ŀ�괮
// ret : 0            : OK
//       -1           : Base������Χ
//       -2           : �ڲ��ռ䲻��
//       -3           : Դ���ִ������ֳ������Ʒ�Χ
int iStrNumBaseConvert(uchar *pszSource, uint uiSourceBase, uchar *pszTarget, uint uiTargetSize, uint uiTargetBase, uchar ucFillChar)
{
	int   iRet;
    uchar sS[MAX_DIGIT], sT[MAX_DIGIT];

	iRet = iEncode(pszSource, sS, MAX_DIGIT, uiSourceBase, NULL);
	if(iRet)
		return(iRet);
	iRet = iBaseConvert(sS, uiSourceBase, MAX_DIGIT, sT, uiTargetBase, MAX_DIGIT);
	if(iRet)
		return(iRet);
	iRet = iDecode(sT, MAX_DIGIT, pszTarget, uiTargetSize, uiTargetBase, ucFillChar);
	if(iRet)
		return(iRet);
	return(0);
}
