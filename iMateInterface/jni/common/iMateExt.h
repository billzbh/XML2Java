#ifndef IMATEEXT_H
#define IMATEEXT_H

#include "unsigned.h"

// ����IC���������ͣ�0��оƬ��������1����Ƶ������
void vSetCardReaderType(int iCardReaderType);

int  iIMateTestCard(void);

// ��Ƭ��λ
// ret : <=0 : ��λ����
//       >0  : ��λ�ɹ�, ����ֵΪATR����
int  iIMateResetCard(uchar *psResetData);

// �رտ�Ƭ
// ret : ������
int  iIMateCloseCard(void);

// ִ��APDUָ��
// in  : iInLen   	: Apduָ���
// 		 pIn     	: Apduָ��, ��ʽ: Cla Ins P1 P2 Lc DataIn Le
// out : piOutLen 	: ApduӦ�𳤶�
//       pOut    	: ApduӦ��, ��ʽ: DataOut Sw1 Sw2
// ret : 0          : �������ɹ�
//       1          : ��������
int  iIMateExchangeApdu(int iInLen, uchar *pIn, int *piOutLen, uchar *pOut);

#endif
