/**************************************
File name     : arith.h
Function      : ��������
Author        : Yu Jun
First edition : Apr 10th, 2012
Note          : ΪEMV���Ľ�����㣬��Ʊ�ģ�飬������Ч������
                �������ĺ���ʹ�÷�����iAdd() iSub() iMult() iDiv() iBaseConvert()
				    ������ʾΪuchar�����飬��λ��ǰ����ָ��ӳ��ȱ�ʾ
					����Ԫ��Ϊ�趨�Ľ��Ƶ�λ����16���Ƶ�'F'��ʾΪ0x0F
				�ⲿ�ӿں���ʹ�÷�����
				    ������ʾΪ�ɶ��ַ�������10���Ƶ�135��ʾΪ"135"��16���Ƶ�23F1��ʾΪ"23F1"��"23f1"
					�����11-16������10-15���ô�дA-F��Сдa-f��ʾ
					���������Ϊ11-16���ƣ�10-15��ʾΪ��д��ĸA-F
				��MAX_DIGIT��ʾ֧�ֵ����λ�����������������ɼӴ��䶨��
Modified      : 
**************************************/
#ifndef _ARITH_H
#define _ARITH_H

// A = B + C
// in  : uiSizeA    : pszA�ռ�(������β��0��Ҫ����1�ֽ�)
//       uiBase     : ���ƣ�2-16
//       ucFlllChar : ���Aǰ������ַ�(0:�����κ��ַ�, ����:ǰ���ַ�,��õĲ�λ�ַ�Ϊ'0')
// ret : 0          : OK
//       -1         : uiBase������Χ
//       -2         : �ڲ��ռ䲻��
//       -3         : Դ���ִ������ֳ������Ʒ�Χ
int iStrNumAdd(uchar *pszA, uint uiSizeA, uchar *pszB, uchar *pszC, uint uiBase, uchar ucFillChar);

// A = B - C
// in  : uiSizeA    : pszA�ռ�(������β��0��Ҫ����1�ֽ�)
//       uiBase     : ���ƣ�2-16
//       ucFlllChar : ���Aǰ������ַ�(0:�����κ��ַ�, ����:ǰ���ַ�,��õĲ�λ�ַ�Ϊ'0')
// ret : 0          : OK
//       1          : B < C
//       -1         : uiBase������Χ
//       -2         : �ڲ��ռ䲻��
//       -3         : Դ���ִ������ֳ������Ʒ�Χ
int iStrNumSub(uchar *pszA, uint uiSizeA, uchar *pszB, uchar *pszC, uint uiBase, uchar ucFillChar);

// A = B * C
// in  : uiSizeA    : pszA�ռ�(������β��0��Ҫ����1�ֽ�)
//       uiBase     : ���ƣ�2-16
//       ucFlllChar : ���Aǰ������ַ�(0:�����κ��ַ�, ����:ǰ���ַ�,��õĲ�λ�ַ�Ϊ'0')
// ret : 0          : OK
//       -1         : uiBase������Χ
//       -2         : �ڲ��ռ䲻��
//       -3         : Դ���ִ������ֳ������Ʒ�Χ
int iStrNumMult(uchar *pszA, uint uiSizeA, uchar *pszB, uchar *pszC, uint uiBase, uchar ucFillChar);

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
int iStrNumDiv(uchar *pszA, uint uiSizeA, uchar *pszR, uint uiSizeR, uchar *pszB, uchar *pszC, uint uiBase, uchar ucFillChar);

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
int iStrNumBaseConvert(uchar *pszSource, uint uiSourceBase, uchar *pszTarget, uint uiTargetSize, uint uiTargetBase, uchar ucFillChar);

// ret : 1  : A > B
//       0  : A == B
//       -1 : A < B
int iStrNumCompare(uchar *pszA, uchar *pszB, uint uiBase);

#endif
