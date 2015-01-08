/**************************************
File name     : arith.h
Function      : 算术运算
Author        : Yu Jun
First edition : Apr 10th, 2012
Note          : 为EMV核心金额运算，设计本模块，不关心效率问题
                基本核心函数使用方法：iAdd() iSub() iMult() iDiv() iBaseConvert()
				    大数表示为uchar型数组，低位在前，由指针加长度表示
					数组元素为设定的进制单位，如16进制的'F'表示为0x0F
				外部接口函数使用方法：
				    大数表示为可读字符串，如10进制的135表示为"135"，16进制的23F1表示为"23F1"或"23f1"
					传入的11-16进制数10-15可用大写A-F或小写a-f表示
					计算结果如果为11-16进制，10-15表示为大写字母A-F
				宏MAX_DIGIT表示支持的最长数位，如果发生了溢出，可加大其定义
Modified      : 
**************************************/
#ifndef _ARITH_H
#define _ARITH_H

// A = B + C
// in  : uiSizeA    : pszA空间(不包含尾部0，要多留1字节)
//       uiBase     : 进制，2-16
//       ucFlllChar : 结果A前导填充字符(0:不补任何字符, 其它:前补字符,最常用的补位字符为'0')
// ret : 0          : OK
//       -1         : uiBase超出范围
//       -2         : 内部空间不足
//       -3         : 源数字串有数字超出进制范围
int iStrNumAdd(uchar *pszA, uint uiSizeA, uchar *pszB, uchar *pszC, uint uiBase, uchar ucFillChar);

// A = B - C
// in  : uiSizeA    : pszA空间(不包含尾部0，要多留1字节)
//       uiBase     : 进制，2-16
//       ucFlllChar : 结果A前导填充字符(0:不补任何字符, 其它:前补字符,最常用的补位字符为'0')
// ret : 0          : OK
//       1          : B < C
//       -1         : uiBase超出范围
//       -2         : 内部空间不足
//       -3         : 源数字串有数字超出进制范围
int iStrNumSub(uchar *pszA, uint uiSizeA, uchar *pszB, uchar *pszC, uint uiBase, uchar ucFillChar);

// A = B * C
// in  : uiSizeA    : pszA空间(不包含尾部0，要多留1字节)
//       uiBase     : 进制，2-16
//       ucFlllChar : 结果A前导填充字符(0:不补任何字符, 其它:前补字符,最常用的补位字符为'0')
// ret : 0          : OK
//       -1         : uiBase超出范围
//       -2         : 内部空间不足
//       -3         : 源数字串有数字超出进制范围
int iStrNumMult(uchar *pszA, uint uiSizeA, uchar *pszB, uchar *pszC, uint uiBase, uchar ucFillChar);

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
int iStrNumDiv(uchar *pszA, uint uiSizeA, uchar *pszR, uint uiSizeR, uchar *pszB, uchar *pszC, uint uiBase, uchar ucFillChar);

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
int iStrNumBaseConvert(uchar *pszSource, uint uiSourceBase, uchar *pszTarget, uint uiTargetSize, uint uiTargetBase, uchar ucFillChar);

// ret : 1  : A > B
//       0  : A == B
//       -1 : A < B
int iStrNumCompare(uchar *pszA, uchar *pszB, uint uiBase);

#endif
