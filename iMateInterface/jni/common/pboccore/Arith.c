/**************************************
File name     : arith.c
Function      : 算术运算
Author        : Yu Jun
First edition : Apr 10th, 2012
Note          : 为EMV核心金额运算设计本模块，由于EMV规范设计的运算数据都不大, 模块并不关心效率问题
Modified      : 
**************************************/
/*
模块详细描述:
	背景: EMV规范要求的金额有两种格式, 12N型与4B型, 因此需要能实现12N数字运算与10进制/16进制转换, 故编写了本模块
.	本模块实现了任意长度任意进制(2-16进制,支持更多的进制无实际意义,后同)数的加减乘除运算及任意进制的进制转换
.	基本核心函数：iAdd() iSub() iMult() iDiv() iBaseConvert()
	实现: 大数内部表示为uchar型数组，低位在前，由指针加长度表示
		  数组元素为设定的进制单位，如16进制的'F'表示为0x0F. 10进制'27'表示为"\x07\x02"
	      计算前先将传入的可读数字串转换为内部表示, 计算后将结果再转换成可读数字串输出
.	外部接口函数使用方法：
	大数表示为可读字符串，如10进制的135表示为"135"，16进制的23F1表示为"23F1"或"23f1"
	传入的11-16进制数10-15可用大写A-F或小写a-f表示
	计算结果如果为11-16进制，10-15表示为大写字母A-F
.	宏MAX_DIGIT表示支持的最长数位，如果因为不足发生了溢出，可加大其定义, 最好不大于32767
	注意: MAX_DIGIT为表示所有进制数据的最大长度, 如果需要进行2进制运算, 需注意该宏定义的大小是否可能不足
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

// 比较A、B
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

// 判断psA是否为0
// in  : psA   : 大数
//       uiLen : 大数长度
// ret : 0     : 不是0
//       1     : psA == 0
static int iIsZero(uchar *psA, uint uiLen)
{
	int i;
	for(i=0; i<(int)uiLen; i++)
		if(*psA++)
			return(0);
	return(1);
}

// 数组移位
// in  : psA
//       uiLen   : 数组长度
//       iShift  : 移位标志
//                 >0:数组右移(相当于乘以进制数)
//                 <0:数组左移(相当于除以进制数)
//                  0:不移位
// ret : 如果iShift为1或-1, 返回移出去的数据元值
//       其它情况返回0
// Note: 移出来的空位填0
static uchar ucShift(uchar *psA, uint uiLen, int iShift)
{
	uchar ucRet;
    int   i;
	if(iShift==0 || uiLen==0)
		return(0); // iShift==0不用移位, uiLen==0不用移位
	if(iShift>=(int)uiLen || -iShift>=(int)uiLen) {
		// 移位数太大，认为所有有效数字都会移出数组，将数组清零即可
		if(iShift == 1)
			ucRet = psA[uiLen-1];
		else if(iShift == -1)
			ucRet = psA[0];
		else
			ucRet = 0;
        vSetZero(psA, uiLen);
		return(ucRet);
	}
	// 准备好返回值
	if(iShift == 1)
		ucRet = psA[uiLen-1];
	else if(iShift == -1)
		ucRet = psA[0];
	else
		ucRet = 0;
	// 移位
    if(iShift > 0) {
        // 数组右移(相当于乘以Base)
        for(i=uiLen-1; i>=0; i--) {
			if(i >= iShift)
				psA[i] = psA[i-iShift];
			else
				psA[i] = 0;
		}
    } else {
        // 数组左移(相当于除以Base)
		iShift = -iShift; // 将iShift扶正
		for(i=0; i<(int)uiLen; i++) {
			if(i+iShift < (int)uiLen)
				psA[i] = psA[i+iShift];
			else
				psA[i] = 0;
		}
    }
	return(ucRet);
}

// 两数相加 A = A + B
// ABC表示 : 低位在前, 每单元表示一位x进制数字
// in  : psA, psB
//       uiLen   : 位数
//       uiBase  : 进制数，2-16
// out : piA
// ret : 0|1     : carry
//       -1      : uiBase不支持
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

// 两数相减 A = A - B
// ABC表示 : 低位在前, 每单元表示一位x进制数字
// in  : psA, psB
//       uiLen   : 位数，必须大于0
//       uiBase  : 进制数，2-16
// out : psA
// ret : 0|1     : borrow
//       -1      : uiBase不支持
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

// 大数乘以单位数 A = A * ucMultiple
// A低位在前, 每单元表示一位数字, ucMultiple为1位x进制数字
// in  : psA
//       ucMultiple : 乘数, <=16
//       uiLen      : A的位数
//       uiBase     : 进制数，2-16
// out : psA        : 位数为uiLen
// ret : >0         : Carry, 可能大于1
//       -1         : uiBase不支持或乘数太大
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
// ABC表示 : 低位在前, 每单元表示一位x进制数字
// in  : psB, psC : 位数为uiLen
//       uiLen    : B、C的位数，必须小于等于MAX_DIGIT
//       uiBase   : 进制数，2-16
// out : psA      : 位数为uiLen*2
// ret : 0        : OK
//       -1       : uiBase不支持
//       -2       : 溢出
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
		ucShift(sT, uiLen*2, i/*右移i位*/);
        iMultDigit(sT, psC[i], uiLen*2, uiBase);
        iAdd(psA, sT, uiLen*2, uiBase);
    }
    return(0);
}

// A = B / C
// R = B % C
// ABCR表示 : 低位在前, 每单元表示一位x进制数字
// in  : psB, psC
//       uiLen    : A、B、C、R位数，必须小于等于MAX_DIGIT
//       uiBase   : 进制数，2-16
// out : psA、psR : 位数为uiLen
// ret : 0        : OK
//       -1       : uiBase不支持
//       -2       : 溢出(被0除当做溢出)
//       -3       : C == 0
static int iDiv(uchar *psA, uchar *psR, uchar *psB, uchar *psC, uint uiLen, uint uiBase)
{
	int   iRet;
	uchar ucRet;
    int   i;
	int   iQuotientPos;  // 商所在的位置
    uchar sB[MAX_DIGIT]; // B临时变量
	uchar sC[MAX_DIGIT]; // C临时变量
	uchar sT[MAX_DIGIT]; // 临时变量
	uchar sUnit[MAX_DIGIT]; // 临时变量, 用于计算商时保存一个单位的值
    
	if(uiBase<2 || uiBase>16)
		return(-1);
    if(uiLen > MAX_DIGIT)
        return(-2);
	if(uiLen == 0)
		return(-3);
	if(iIsZero(psC, uiLen))
		return(-3);

    vSetZero(psA, uiLen);     // A = 0
	vAssign(sB, psB, uiLen); // sB = B, sB保持计算中剩余未完成的被除数
	while(iComp(sB, psC, uiLen) >= 0) {
		// 剩余未完成的被除数还大于除数，需要继续求商
		vAssign(sC, psC, uiLen); // sC = C, sC用于临时计算
		for(i=0; i<(int)uiLen; i++) {
			ucRet = ucShift(sC, uiLen, 1); // 右移一位
			if(ucRet)
				break;
			if(iComp(sB, sC, uiLen) < 0)
				break;
		}
		iQuotientPos = i; // 商所在的位置
		vAssign(sUnit, psC, uiLen); // sUnit = C, sUnit用于临时计算
		ucShift(sUnit, uiLen, iQuotientPos); // 将sUnit调整到1个计算单位
		vAssign(sT, sUnit, uiLen); // sT用于临时计算, 赋初值为1个单位
		for(i=1; i<(int)uiBase; i++) {
			iRet = iAdd(sT, sUnit, uiLen, uiBase);
			if(iRet)
				break; // 溢出，认为计算出1位商=i
			if(iComp(sT, sB, uiLen) > 0)
				break; // 计算出1位商=i
		}
		psA[iQuotientPos] = i; // 保存商
        iMultDigit(sUnit, (uchar)i, uiLen, uiBase); // 用于求差
		iSub(sB, sUnit, uiLen, uiBase);
	}
	vAssign(psR, sB, uiLen);

    return(0);
}

// 进制转换，2-16各进制间转换
// in  : psS     : 源数据
//       uiSBase : 源数据进制
//       uiSLen  : 源数据长度
//       uiTBase : 目标数据进制
//       uiTLen  : 目标数据缓冲区长度
// out : psT     : 目标数据
// ret : 0       : OK
//       -1      : Base非法
//       -2      : 溢出
static int iBaseConvert(uchar *psS, uint uiSBase, uint uiSLen, uchar *psT, uint uiTBase, uint uiTLen)
{   
	int   i, j;
	uint  uiTmp;
    uchar sTBaseInSBase[MAX_DIGIT]; // 用uiSBase标明的进制表示的uiTbase进制数
	uchar sS[MAX_DIGIT];
	uchar sTmp1[MAX_DIGIT], sTmp2[MAX_DIGIT];

	if(uiSBase<2 || uiSBase>16 || uiTBase<2 || uiTBase>16)
		return(-1);
	if(uiSLen > MAX_DIGIT)
		return(-2);
    // 构造长度为uiSLen的sTBaseInSBase
    vSetZero(sTBaseInSBase, uiSLen);
	uiTmp = uiTBase; // 用uiTmp保存uiTBase用于运算
    for(i=0; i<5; i++) { // 用2进制表示16, 5位数也足够了
		sTBaseInSBase[i] = uiTmp % uiSBase;
		uiTmp /= uiSBase;
	}
	// 转换
    vSetZero(sS, sizeof(sS));
    vAssign(sS, psS, uiSLen); // sS用于记录源数据psS在计算过程中的中间数据
	if(uiSLen < 5)
		uiSLen = 5; // 要uiSLen至少长度为5，防止某些时候源数据长度太短的问题
	for(i=0; i<(int)uiTLen; i++) {
		iDiv(sTmp1, sTmp2, sS, sTBaseInSBase, uiSLen, uiSBase); // sTmp1=商 sTmp2=余数
		for(j=4, psT[i]=0; j>=0; j--) {
			psT[i] *= uiSBase;
			psT[i] += sTmp2[j];
		}
		vAssign(sS, sTmp1, uiSLen);
	}
	if(!iIsZero(sS, uiSLen))
		return(-2); // 填完目标缓冲区后还有余项，溢出
	return(0);
}

// 将可读数字串转换成大数数组
// in  : pszSource    : uiBase表示的源数字串
//       uiTargetSize : 大数数组大小
//       uiBase       : 源数字串进制, 2-16
// out : psTarget     : 生成的大数数组
//       puiValidLen  : 有效数字长度, NULL表示不需要返回
// ret : 0            : OK
//       -1           : uiBase超出范围
//       -2           : 目标大数数组空间不足
//       -3           : 源数字串有数字超出进制范围
static int iEncode(char *pszSource, uchar *psTarget, uint uiTargetSize, uint uiBase, uint *puiValidLen)
{
	int  i, j;
	int  iDigit;
	int  iLen; // 源数字串有效数字长度

	if(uiBase<2 || uiBase>16)
		return(-1);

	// pass前导'0'
	while(*pszSource=='0' && *pszSource!=0)
		pszSource++;
	iLen = strlen(pszSource);
	if(iLen > (int)uiTargetSize)
		return(-2);

	// 转换
    vSetZero(psTarget, uiTargetSize);
	// i:源下标 j:目标下标
	for(i=iLen-1, j=0; i>=0; i--, j++) {
		if(pszSource[i]>='0' && pszSource[i]<='9')
			iDigit = pszSource[i] - '0';
		else if(pszSource[i]>='A' && pszSource[i]<='F')
			iDigit = pszSource[i] - 'A' + 10;
		else if(pszSource[i]>='a' && pszSource[i]<='f')
			iDigit = pszSource[i] - 'a' + 10;
		else
			return(-3); // 不是16进制以内进制数
		if(iDigit >= (int)uiBase)
			return(-3); // 超出uiBase进制表示范围了
		psTarget[j] = iDigit;
	}
	if(puiValidLen)
		*puiValidLen = iLen; // 如果要求返回有效数字长度，返回有效数字长度
	return(0);
}

// 将大数数组转换成可读数字串
// in  : psSource     : 源大数数组
//       uiSourceSize : 源大数数组大小
//       uiTargetSize : 目标数字串空间大小(不包括尾部0，预留的缓冲区应该再多1字节)
//       uiBase       : 源大数数组进制, 2-16
//       ucFillChar   : 目标数字串前补字符(0:不补任何字符, 其它:前补字符,最常用的补位字符为'0')
// out : pszTarget    : 目标数字串(以uiBase进制表示)
// ret : 0            : OK
//       -1           : uiBase超出范围
//       -2           : 目标数字串空间不足
//       -3           : 源大数数组中有数字超出进制范围
static int iDecode(uchar *psSource, uint uiSourceSize, char *pszTarget, uint uiTargetSize, uint uiBase, uchar ucFillChar)
{
	int  i, j;
	int  iLen; // 源大数数组有效数字长度

	if(uiBase<2 || uiBase>16)
		return(-1);

	// 求出源大数数组有效数字长度
	for(i=(int)uiSourceSize-1; i>0; i--)
		if(psSource[i])
			break;
	iLen = i + 1;
	if(iLen > (int)uiTargetSize)
		return(-2);

	// 目标清0, 准备组装数字串
	memset(pszTarget, 0, uiTargetSize+1); // uiTargetSize不包括结束0，因此要一起清0
	// 如果需要，填补位字符
	if(ucFillChar) {
		while(iLen < (int)uiTargetSize) {
			*pszTarget++ = ucFillChar;
			uiTargetSize --;
		}
	}

	// 转换
	// i:源下标 j:目标下标
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
// in  : uiSizeA    : pszA空间(不包含尾部0，要多留1字节)
//       uiBase     : 进制，2-16
//       ucFlllChar : 结果A前导填充字符(0:不补任何字符, 其它:前补字符,最常用的补位字符为'0')
// ret : 0          : OK
//       -1         : uiBase超出范围
//       -2         : 内部空间不足
//       -3         : 源数字串有数字超出进制范围
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
		return(-2); // 有进位认为内部空间不足
	iRet = iDecode(sB, MAX_DIGIT, pszA, uiSizeA, uiBase, ucFillChar);
	if(iRet)
		return(iRet);
	return(0);
}

// A = B - C
// in  : uiSizeA    : pszA空间(不包含尾部0，要多留1字节)
//       uiBase     : 进制，2-16
//       ucFlllChar : 结果A前导填充字符(0:不补任何字符, 其它:前补字符,最常用的补位字符为'0')
// ret : 0          : OK
//       1          : B < C
//       -1         : uiBase超出范围
//       -2         : 内部空间不足
//       -3         : 源数字串有数字超出进制范围
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
// in  : uiSizeA    : pszA空间(不包含尾部0，要多留1字节)
//       uiBase     : 进制，2-16
//       ucFlllChar : 结果A前导填充字符(0:不补任何字符, 其它:前补字符,最常用的补位字符为'0')
// ret : 0          : OK
//       -1         : uiBase超出范围
//       -2         : 内部空间不足
//       -3         : 源数字串有数字超出进制范围
int iStrNumMult(uchar *pszA, uint uiSizeA, uchar *pszB, uchar *pszC, uint uiBase, uchar ucFillChar)
{
	int   iRet;
	uint  uiBValidLen, uiCValidLen; // 有效数字长度
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
// in  : pszA       : 商，如果传入NULL, 表示不返回商
//       uiSizeA    : pszA空间(不包含尾部0，要多留1字节)
//       pszR       : 余，如果传入NULL, 表示不返回余数
//       uiSizeR    : pszR空间(不包含尾部0，要多留1字节)
//       uiBase     : 进制，2-16
//       ucFlllChar : 结果A、R前导填充字符(0:不补任何字符, 其它:前补字符,最常用的补位字符为'0')
// ret : 0          : OK
//       1          : 除数为0
//       -1         : uiBase超出范围
//       -2         : 内部空间不足
//       -3         : 源数字串有数字超出进制范围
int iStrNumDiv(uchar *pszA, uint uiSizeA, uchar *pszR, uint uiSizeR, uchar *pszB, uchar *pszC, uint uiBase, uchar ucFillChar)
{
	int   iRet;
	uint  uiBValidLen, uiCValidLen; // 有效数字长度
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
	return(0); // A == B (如果出错, 认为相等, 高层应该确保各参数没有越界)
}

// 进制转换
// in  : pszSource    : 源数字串
//       uiSourceBase : 源数字串进制
//       uiTargetSize : 目标串空间大小(不包含尾部0，要多留1字节)
//       uiTargetBase : 目标串进制
//       ucFlllChar   : 结果pszTarget前导填充字符(0:不补任何字符, 其它:前补字符,最常用的补位字符为'0')
// ret : pszTarget    : 目标串
// ret : 0            : OK
//       -1           : Base超出范围
//       -2           : 内部空间不足
//       -3           : 源数字串有数字超出进制范围
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
