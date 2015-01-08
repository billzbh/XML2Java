/**************************************
File name     : EmvCvm2.c
Function      : Emv�ֿ�����֤ģ��(�ǻص��ӿ�)
Author        : Yu Jun
First edition : Apr 25th, 2012
Modified      : Jan 24th, 2013
                    �޸�Ϊ�ǻص���ʽ�ӿ�, Դ��EmvCvm.c
**************************************/
#ifndef _EMVCVM2_H
#define _EMVCVM2_H

// CVM2�����ʼ��, emv���׳�ʼ��ʱ����
void vCvm2Init(void);

// ��ȡ�ֿ�����֤
// out : piCvm               : �ֿ�����֤����, ֧��HXCVM_PLAIN_PIN��HXCVM_CIPHERED_ONLINE_PIN��HXCVM_HOLDER_ID��HXCVM_CONFIRM_AMOUNT
//       piBypassFlag        : ����bypass��־, 0:������, 1:����
//       piMsgType           : ��ʾ��Ϣ��(�����֤����Ϊ֤����֤, ����֤��������)
//       piMsgType2          : ������ʾ��Ϣ, �����Ϊ0, ����ʾ����Ϣ
//       pszAmountStr        : ��ʾ�ø�ʽ�����(HXCVM_HOLDER_ID��֤��������Ҫ)
// ret : HXEMV_OK            : OK
//       HXEMV_NO_DATA       : ����������гֿ�����֤
//       HXEMV_CARD_OP       : ��������
//       HXEMV_TERMINATE     : ����ܾ�������������ֹ
//       HXEMV_DENIAL        : �ն���Ϊ�������Ϊ�ѻ��ܾ�
//       HXEMV_DENIAL_ADVICE : �ն���Ϊ�������Ϊ�ѻ��ܾ�, ��Advice
//       HXEMV_FLOW_ERROR    : EMV���̴���
//       HXEMV_CORE          : �ڲ�����
//		 HXEMV_CALLBACK_METHOD : û���Էǻص���ʽ��ʼ������
// Note: ����ֿ�����֤����ΪHXCVM_HOLDER_ID, ��piMsgType&pszAmountStr��������, Ӧ�ò�Ӧ������ȡ��ʾ��֯��ʾ��Ϣ
int iCvm2GetMethod(int *piCvm, int *piBypassFlag, int *piMsgType, int *piMsgType2, uchar *pszAmountStr);

// ִ�гֿ�����֤
// in  : psCvmProc           : ��֤��������ʽ, HXCVM_PROC_OK or HXCVM_BYPASS or HXCVM_FAIL or HXCVM_CANCEL or HXCVM_TIMEOUT
//       psCvmData           : ���������, ���Ϊ��������, ����β��Ҫ��0
// out : piMsgType           : ����HXEMV_OKʱʹ��
//                             �����Ϊ0, ����Ҫ��ʾ��������Ϣ
//       piMsgType2          : ����HXEMV_OKʱʹ��
//                             �����Ϊ0, ����Ҫ��ʾ��������Ϣ
// ret : HXEMV_OK            : OK, ��Ҫ�������гֿ�����֤, ��������iHxGetCvmMethod(), Ȼ���ٵ��ñ�����
//       HXEMV_PARA		     : ��������
//       HXEMV_CANCEL        : �û�ȡ��
//       HXEMV_TIMEOUT       : ��ʱ
//       HXEMV_CARD_OP       : ��������
//       HXEMV_CARD_SW       : �Ƿ���״̬��
//       HXEMV_TERMINATE     : ����ܾ�������������ֹ
//       HXEMV_DENIAL        : �ն���Ϊ�������Ϊ�ѻ��ܾ�
//       HXEMV_DENIAL_ADVICE : �ն���Ϊ�������Ϊ�ѻ��ܾ�, ��Advice
//       HXEMV_FLOW_ERROR    : EMV���̴���
//       HXEMV_CORE          : �ڲ�����
//		 HXEMV_CALLBACK_METHOD : û���Էǻص���ʽ��ʼ������
// Note: ִ�е�CVM���������һ��iHxGetCvmMethod2()��õ�CVM
int iCvm2DoMethod(int iCvmProc, uchar *psCvmData, int *piMsgType, int *piMsgType2);

// ��ȡCVM��֤�����費��Ҫǩ��
// out : piNeedSignFlag    : �������HXEMV_OK����ʾ��Ҫǩ�ֱ�־��0:����Ҫǩ�� 1:��Ҫǩ��
// ret : HXEMV_OK          : OK
int iCvm2GetSignFlag(int *piNeedSignFlag);

#endif
