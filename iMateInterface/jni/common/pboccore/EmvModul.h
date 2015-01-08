/**************************************
File name     : EmvModul.h
Function      : Emv���Ĵ���ģ��
Author        : Yu Jun
First edition : Apr 18th, 2012
Modified      : 
**************************************/
#ifndef _EMVMODUL_H
#define _EMVMODUL_H

// GPO��ʼ��
// ret : HXEMV_OK		: �ɹ�
int iEmvGpoInit(void);

// GPO
// ret : HXEMV_OK		: �ɹ�
//       HXEMV_CANCEL   : �û�ȡ��
//       HXEMV_TIMEOUT  : �û���ʱ
//       HXEMV_CORE     : ��������
//       HXEMV_RESELECT : ��Ҫ����ѡ��Ӧ��
//       HXEMV_CARD_OP  : ��������
//       HXEMV_CARD_SW  : �Ƿ���״̬��
//       HXEMV_TERMINATE: ����ܾ�������������ֹ
// Note: �ǽӿ�����HXEMV_OKʱ, �п��ܽ���·��ָʾ���ױ��ܾ�
int iEmvGpo(void);

// ��Ӧ�ü�¼
// ret : HXEMV_OK          : OK
//       HXEMV_CARD_OP     : ��������
//       HXEMV_TERMINATE   : ����ܾ�������������ֹ
//       HXEMV_CORE        : ��������
int iEmvReadRecord(void);

// SDA��DDA
// in  : iInCrlFlag          : ������֤����CRL�б�֮��, 1:������֤����CRL�б� 0:������֤���Ƿ���CRL�б���δ֪
//                             �˲���ר��Ϊ�ǻص���ʽʹ��, ����1ʱ�����ѻ�������֤ʧ��λ
// ret : HXEMV_OK            : OK
//       HXEMV_CARD_OP       : ��������
//       HXEMV_CARD_SW       : �Ƿ���״̬��
//       HXEMV_TERMINATE     : ����ܾ�������������ֹ
//       HXEMV_DENIAL        : �ն���Ϊ�������Ϊ�ѻ��ܾ�
//       HXEMV_DENIAL_ADVICE : �ն���Ϊ�������Ϊ�ѻ��ܾ�, ��Advice
//       HXEMV_FLOW_ERROR    : EMV���̴���
//       HXEMV_CORE          : �ڲ�����
//		 HXEMV_FDDA_FAIL	 : fDDAʧ��(�ǽ�)
int iEmvOfflineDataAuth(int iInCrlFlag);

// ��������
// ret : HXEMV_OK            : OK
//       HXEMV_CORE          : ��������
//       HXEMV_DENIAL        : �ն���Ϊ�������Ϊ�ѻ��ܾ�
//       HXEMV_DENIAL_ADVICE : �ն���Ϊ�������Ϊ�ѻ��ܾ�, ��Advice
//       HXEMV_CARD_OP       : �ն���Ϊ�������Ϊ�ѻ��ܾ�, ����AACʱ��Ƭ����
//       HXEMV_TERMINATE     : ����ܾ�������������ֹ
int iEmvProcRistrictions(void);

// �ն˷��չ���
// ret : HXEMV_OK            : OK
//       HXEMV_CORE          : ��������
//       HXEMV_CARD_OP       : ��������
//       HXEMV_TERMINATE     : ����ܾ�������������ֹ
//       HXEMV_DENIAL        : �ն���Ϊ�������Ϊ�ѻ��ܾ�
//       HXEMV_DENIAL_ADVICE : �ն���Ϊ�������Ϊ�ѻ��ܾ�, ��Advice
//       HXEMV_CANCEL        : ��ȡ��
//       HXEMV_TIMEOUT       : ��ʱ
// Note: ������AIP����, ǿ��ִ��
int iEmvTermRiskManage(void);

// �ֿ�����֤
// out : piNeedSignFlag      : ����ɹ���ɣ�������Ҫǩ�ֱ�־��0:����Ҫǩ�� 1:��Ҫǩ��
// ret : HXEMV_OK            : OK
//       HXEMV_CANCEL        : �û�ȡ��
//       HXEMV_TIMEOUT       : ��ʱ
//       HXEMV_CARD_OP       : ��������
//       HXEMV_TERMINATE     : ����ܾ�������������ֹ
//       HXEMV_DENIAL        : �ն���Ϊ�������Ϊ�ѻ��ܾ�
//       HXEMV_DENIAL_ADVICE : �ն���Ϊ�������Ϊ�ѻ��ܾ�, ��Advice
//       HXEMV_FLOW_ERROR    : EMV���̴���
//       HXEMV_CORE          : �ڲ�����
int iEmvModuleCardHolderVerify(int *piNeedSignFlag);

// �ж��Ƿ���Ҫ����EC��Ҫ����������
// ret : HXEMV_OK		   : ��Ҫ
//       HXEMV_NA          : ����Ҫ
int iEmvIfNeedECOnlinePin(void);

// �ж��Ƿ���Ҫ�ܾ�����
// ret : HXEMV_OK		     : ����Ҫ
//       HXEMV_CARD_OP       : ��������
//       HXEMV_TERMINATE     : ����ܾ�������������ֹ
//       HXEMV_DENIAL        : �ն���Ϊ�������Ϊ�ѻ��ܾ�
//       HXEMV_DENIAL_ADVICE : �ն���Ϊ�������Ϊ�ѻ��ܾ�, ��Advice
//       HXEMV_CORE          : �ڲ�����
int iEmvIfDenial(void);

// �ն���Ϊ����
// ret : HXEMV_OK		     : OK
//       HXEMV_CORE          : ��������
//       HXEMV_DENIAL        : �ն���Ϊ�������Ϊ�ѻ��ܾ�
//       HXEMV_DENIAL_ADVICE : �ն���Ϊ�������Ϊ�ѻ��ܾ�, ��Advice
//       HXEMV_CARD_OP       : �ն���Ϊ�������Ϊ�ѻ��ܾ�, ����AACʱ��Ƭ����
//       HXEMV_TERMINATE     : ����ܾ�������������ֹ
//       HXEMV_CANCEL        : �û�ȡ��
//       HXEMV_TIMEOUT       : ��ʱ
// Note: �ú�������iHxReadRecord()��iHxGAC1()֮ǰ�������, �ж�Ϊ�ѻ��ܾ���, ���Զ�GAC����AAC, ��ʱ�߲�ɽ���EMV����
int iEmvTermActionAnalysis(void);

// ��һ��GAC
// out : piCardAction      : ��Ƭִ�н��
//                           GAC_ACTION_TC     : ��׼(����TC)
//							 GAC_ACTION_AAC    : �ܾ�(����AAC)
//                           GAC_ACTION_AAC_ADVICE : �ܾ�(����AAC,��Advice)
//                           GAC_ACTION_ARQC   : Ҫ������(����ARQC)
// ret : HXEMV_OK          : OK
//       HXEMV_CORE        : ��������
//       HXEMV_CARD_OP     : ��������
//       HXEMV_CARD_SW     : �Ƿ���״̬��
//       HXEMV_TERMINATE   : ����ܾ�������������ֹ
//       HXEMV_NOT_ALLOWED : ��������
// Note: ĳЩ�����, �ú������Զ�����iEmvGAC2()
//       ��:CDAʧ�ܡ����ѻ��ն�GAC1��Ƭ����ARQC...
int iEmvGAC1(int *piCardAction);

// �ڶ���GAC
// in  : pszArc            : Ӧ����[2], NULL��""��ʾ����ʧ��
//       pszAuthCode	   : ��Ȩ��[6], NULL��""��ʾ����Ȩ������
//       psIssuerData      : ������Ӧ����(55������),һ��TLV��ʽ����, NULL��ʾ��������Ӧ����
//       iIssuerDataLen    : ������Ӧ���ݳ���
// out : piCardAction      : ��Ƭִ�н��
//                           GAC_ACTION_TC     : ��׼(����TC)
//							 GAC_ACTION_AAC    : �ܾ�(����AAC)
//                           GAC_ACTION_AAC_ADVICE : �ܾ�(����AAC,��Advice)
//                           GAC_ACTION_ARQC   : Ҫ������(����ARQC)
// ret : HXEMV_OK          : OK
//       HXEMV_CORE        : ��������
//       HXEMV_CARD_OP     : ��������
//       HXEMV_CARD_SW     : �Ƿ���״̬��
//       HXEMV_TERMINATE   : ����ܾ�������������ֹ
//       HXEMV_NOT_ALLOWED : ��������
int iEmvGAC2(uchar *pszArc, uchar *pszAuthCode, uchar *psIssuerData, int iIssuerDataLen, int *piCardAction);

#endif
