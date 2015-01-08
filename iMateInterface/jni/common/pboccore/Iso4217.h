#ifndef _ISO4217_H
#define _ISO4217_H

// 搜索货币代码表
// in  : iDigitCode        : 1-999表示的货币代码
// out : pszAlphaCode      : 3字母表示的货币代码
//       piDecimalPosition : 小数点位置(0-3)
// ret : 0                 : OK
//       1                 : 没找到
int iIso4217SearchDigitCode(int iDigitCode, uchar *pszAlphaCode, int *piDecimalPosition);

#endif
