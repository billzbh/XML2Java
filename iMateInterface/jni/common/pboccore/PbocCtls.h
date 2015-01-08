/**************************************
File name     : PbocCtls.h
Function      : �ǽӿ����Ĵ���ģ��
Author        : Yu Jun
First edition : May 23rd, 2013
Modified      : 
**************************************/
#ifndef _PBOCCTLS_H
#define _PBOCCTLS_H

// �ǽӴ�������Ԥ����
// in  : pszAmount					: ���׽��, ascii�봮, ��ҪС����, Ĭ��ΪȱʡС����λ��, ����:RMB1.00��ʾΪ"100"
//       uiCurrencyCode				: ���Ҵ���
// ret : HXEMV_OK					: OK
//		 HXEMV_TRY_OTHER_INTERFACE	: ����ܾ�������������ֹ, ��������ͨ�Ž���
//									  Ӧ��ʾEMVMSG_TERMINATE��Ϣ, ����ն�֧������ͨ�Ž���, ��ʾ�û���������ͨ�Ž���
//       HXEMV_CORE					: �ڲ�����
// Note: ÿ�ʷǽӴ�����ǰ��������Ԥ����, ֻ�гɹ������Ԥ����ſ��Խ��зǽӽ���
//       Ӧ�ò�Ҫȷ������������Ԥ��������һ��
//       Ԥ������Ϊ�ն˽�������, ������sg_sCtlsAttr[4]
//       �ο�: JR/T0025.12��2013, 6.2, p9
int iPbocCtlsPreProc(uchar *pszAmount, uint uiCurrencyCode);

// ��ȡ�ն˽�������
// ret : 9F66ֵ, ��ȡǰ������Ԥ����
uchar *psPbocCtlsGet9F66(void);

// ��ȡԤ������Ľ��
// ret : Ԥ������Ľ��
uchar *pszPbocCtlsGetAmount(void);

// ��ȡ����·��
// ret : �����ߵ�·��, 0:�Ӵ�Pboc(��׼DC��EC) 1:�ǽӴ�Pboc(��׼DC��EC) 2:qPboc���� 3:qPboc�ѻ� 4:qPboc�ܾ�
// Note : �ǽ�GPO�������ȡ��ֵ��������
int iPbocCtlsGetRoute(void);

// �ǽӿ�GPO���ݷ���
// out : piTransRoute               : �����ߵ�·�� 0:�Ӵ�Pboc(��׼DC��EC) 1:�ǽӴ�Pboc(��׼DC��EC) 2:qPboc���� 3:qPboc�ѻ� 4:qPboc�ܾ�
//       piSignFlag					: ��Ҫǩ�ֱ�־(0:����Ҫǩ��, 1:��Ҫǩ��), �����ߵ���qPboc����ʱ����Ч
//       piNeedOnlinePin            : ��Ҫ���������־(0:����Ҫ��������, 1:��Ҫ��������), �����ߵ���qPboc��������ʱ����Ч
// ret : HXEMV_OK					: OK, ����piTransRoute������������
//       HXEMV_TERMINATE			: ����ܾ�������������ֹ
//		 HXEMV_TRY_OTHER_INTERFACE	: ��������ͨ�Ž���
//       HXEMV_CORE					: �ڲ�����
// Note: GPO�ɹ���ſ��Ե���
// �ο� JR/T0025.12��2013, 7.8 P40
int iPbocCtlsGpoAnalyse(int *piTransRoute, int *piSignFlag, int *piNeedOnlinePin);

#endif
