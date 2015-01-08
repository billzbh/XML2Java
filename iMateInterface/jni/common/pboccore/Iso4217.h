#ifndef _ISO4217_H
#define _ISO4217_H

// �������Ҵ����
// in  : iDigitCode        : 1-999��ʾ�Ļ��Ҵ���
// out : pszAlphaCode      : 3��ĸ��ʾ�Ļ��Ҵ���
//       piDecimalPosition : С����λ��(0-3)
// ret : 0                 : OK
//       1                 : û�ҵ�
int iIso4217SearchDigitCode(int iDigitCode, uchar *pszAlphaCode, int *piDecimalPosition);

#endif
