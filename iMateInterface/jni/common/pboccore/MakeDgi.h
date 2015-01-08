/**************************************
File name     : MAKEDGI.H
Function      : 定义PBOC2.0 D/C发卡数据相关结构
Author        : Yu Jun
First edition : Feb 21st, 2011
**************************************/
#ifndef _MAKEDGI_H
#define _MAKEDGI_H

extern long gl_alPbocDgis[]; // pboc2.0应用数据分组集合

// 构造Pboc2.0借贷记卡应用数据分组
// in  : lDgi      : 数据分组标识
// out : psDgiData : 数据分组数据
// ret : >0        : OK, 值为数据分组数据长度
//       0         : 不认识的数据分组
//       -1        : 其它错误
int iMakePbocDgi(long lDgi, uchar *psDgiData);

# endif
