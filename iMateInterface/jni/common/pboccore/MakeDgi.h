/**************************************
File name     : MAKEDGI.H
Function      : ����PBOC2.0 D/C����������ؽṹ
Author        : Yu Jun
First edition : Feb 21st, 2011
**************************************/
#ifndef _MAKEDGI_H
#define _MAKEDGI_H

extern long gl_alPbocDgis[]; // pboc2.0Ӧ�����ݷ��鼯��

// ����Pboc2.0����ǿ�Ӧ�����ݷ���
// in  : lDgi      : ���ݷ����ʶ
// out : psDgiData : ���ݷ�������
// ret : >0        : OK, ֵΪ���ݷ������ݳ���
//       0         : ����ʶ�����ݷ���
//       -1        : ��������
int iMakePbocDgi(long lDgi, uchar *psDgiData);

# endif
