#include <stdio.h>
#include <stdlib.h>
#include "pub.h"

// 14.4 将二进制源串分解成双倍长度可读的16进制串
// In       : psIn     : 源串
//            iLength  : 源串长度
// Out      : psOut    : 目标串
void vOneTwo(const uchar *psIn, int iLength, uchar *psOut)
{
    static const uchar aucHexToChar[17] = "0123456789ABCDEF";
    int iCounter;

    for(iCounter = 0; iCounter < iLength; iCounter++){
        psOut[2*iCounter] = aucHexToChar[((psIn[iCounter] >> 4)) & 0x0F];
        psOut[2*iCounter+1] = aucHexToChar[(psIn[iCounter] & 0x0F)];
    }
}

// 14.5 将二进制源串分解成双倍长度可读的16进制串, 并在末尾添'\0'
// In       : psIn     : 源串
//            iLength  : 源串长度
// Out      : pszOut   : 目标串
void vOneTwo0(const uchar *psIn, int iLength, uchar *pszOut)
{
    vOneTwo(psIn, iLength, pszOut);
	if(iLength < 0)
		iLength = 0;
    pszOut[2*iLength]=0;
}

// 14.6 将可读的16进制表示串压缩成其一半长度的二进制串
// In       : psIn     : 源串
//            iLength  : 源串长度
// Out      : psOut    : 目标串
// Attention: 源串必须为合法的十六进制表示，大小写均可
//            长度如果为奇数，函数会靠近到比它大一位的偶数
void vTwoOne(const uchar *psIn, int iLength, uchar *psOut)
{
    uchar ucTmp;
    int   i;

    for(i=0; i<iLength; i+=2) {
        ucTmp = psIn[i];
        if(ucTmp > '9')
            ucTmp = toupper(ucTmp) - 'A' + 0x0a;
        else
            ucTmp &= 0x0f;
        psOut[i/2] = ucTmp << 4;

        ucTmp = psIn[i+1];
        if(ucTmp > '9')
            ucTmp = toupper(ucTmp) - 'A' + 0x0a;
        else
            ucTmp &= 0x0f;
        psOut[i/2] += ucTmp;
    } // for(i=0; i<uiLength; i+=2) {
}
