/**************************************
File name     : EmvCvm.h
Function      : Emv�ֿ�����֤ģ��
Author        : Yu Jun
First edition : Apr 25th, 2012
Modified      : 
**************************************/
#ifndef _EMVCVM_H
#define _EMVCVM_H

// �ֿ�����֤
// out : piNeedSignFlag    : ����ɹ���ɣ�������Ҫǩ�ֱ�־��0:����Ҫǩ�� 1:��Ҫǩ��
// ret : HXEMV_OK          : OK
//       HXEMV_CANCEL      : �û�ȡ��
//       HXEMV_TIMEOUT     : ��ʱ
//       HXEMV_CARD_OP     : ��������
//       HXEMV_CARD_SW     : �Ƿ���״̬��
//       HXEMV_TERMINATE   : ����ܾ�������������ֹ
//       HXEMV_CORE        : �ڲ�����
// Note: refer to Emv2008 book3 10.5, P100
//                Emv2008 book4 6.3.4, P45
int iEmvCardHolderVerify(int *piNeedSignFlag);

// ������������
// ret : 0              : OK
//       HXEMV_NA       : bypass
//       HXEMV_CANCEL   : �û�ȡ��
//       HXEMV_TIMEOUT  : ��ʱ
//       HXEMV_CORE     : �ڲ�����
// Note: refer to Emv2008 book3, figure10, P107, ����ͼU
int iCvmEnterOnlinePin(void);

// ���ֿ�����֤���������Ƿ�����
// in  : psCvRule        : �ֿ�����֤����
//       psCvmList       : CVM List, ������ȡX��Y���
// ret : 0               : ����
//       HXEMV_NA        : ������
//       HXEMV_CORE      : �ڲ�����
// Note: EmvCvm2ģ��Ҳ��Ҫ�˽ӿ�
int iCvmCheckCondition(uchar *psCvRule, uchar *psCvmList);

#endif
