#ifndef __PUB_H_
#define __PUB_H_

#include "unsigned.h"

// 14.4 将二进制源串分解成双倍长度可读的16进制串
// In       : psIn     : 源串
//            iLength  : 源串长度
// Out      : psOut    : 目标串
void vOneTwo(const uchar *psIn, int iLength, uchar *psOut);

// 14.5 将二进制源串分解成双倍长度可读的16进制串, 并在末尾添'\0'
// In       : psIn     : 源串
//            iLength  : 源串长度
// Out      : pszOut   : 目标串
void vOneTwo0(const uchar *psIn, int iLength, uchar *pszOut);

// 14.6 将可读的16进制表示串压缩成其一半长度的二进制串
// In       : psIn     : 源串
//            iLength  : 源串长度
// Out      : psOut    : 目标串
// Attention: 源串必须为合法的十六进制表示，大小写均可
//            长度如果为奇数，函数会靠近到比它大一位的偶数
void vTwoOne(const uchar *psIn, int iLength, uchar *psOut);

#endif
