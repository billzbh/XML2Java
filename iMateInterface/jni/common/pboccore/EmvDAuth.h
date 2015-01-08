/**************************************
File name     : EmvDAuth.h
Function      : Emv�ѻ�������֤ģ��
Author        : Yu Jun
First edition : Apr 19th, 2012
Modified      : 
**************************************/
#ifndef _EMVDAUTH_H
#define _EMVDAUTH_H

#define EMV_DAUTH_OK					0 // OK
#define EMV_DAUTH_DATA_MISSING			1 // ����ȱʧ
#define EMV_DAUTH_DATA_ILLEGAL			2 // ���ݷǷ�
#define EMV_DAUTH_NATIVE				3 // �ڲ�����
#define EMV_DAUTH_INDEX_NOT_SUPPORT		4 // CA��Կ������֧��
#define EMV_DAUTH_FAIL					5 // �����ѻ�������֤ʧ�ܵ���������
#define EMV_DAUTH_CARD_IO				6 // ��Ƭ����ʧ��
#define EMV_DAUTH_CARD_SW				7 // �Ƿ���״̬��

// �ѻ�������֤��ʼ��
// ret : EMV_DAUTH_OK : OK
int iEmvDAuthInit(void);

// ����һ��SDA�ѻ���֤��¼����
// in  : ucRecDataLen          : ��¼���ݳ���
//       psRecData             : ��¼����
// ret : EMV_DAUTH_OK		   : OK
//       EMV_DAUTH_LACK_MEMORY : Ԥ���洢�ռ䲻��
int iEmvSDADataAdd(uchar ucRecDataLen, uchar *psRecData);

// ����SDA��¼���ݴ���, �ѻ���֤��¼����T70 Tlv����
// ret : EMV_DAUTH_OK		   : OK
int iEmvSDADataErr(void);

// �ж�SDA�Ƿ��Ѿ�ʧ��
// ret : EMV_DAUTH_OK		   : OK
//       EMV_DAUTH_FAIL		   : SDA�Ѿ�ʧ��
int iEmvSDADataErrTest(void);

// �����������й�Կ
// ret : EMV_DAUTH_OK				 : OK
//       EMV_DAUTH_DATA_MISSING		 : ����ȱʧ
//       EMV_DAUTH_DATA_ILLEGAL		 : ���ݷǷ�
//       EMV_DAUTH_NATIVE			 : �ڲ�����
//       EMV_DAUTH_INDEX_NOT_SUPPORT : CA��Կ������֧��
//       EMV_DAUTH_FAIL              : �����ѻ�������֤ʧ�ܵ���������
int iEmvGetIssuerPubKey(void);

// ������IC����Կ
// ret : EMV_DAUTH_OK				 : OK
//       EMV_DAUTH_DATA_MISSING		 : ����ȱʧ
//       EMV_DAUTH_DATA_ILLEGAL		 : ���ݷǷ�
//       EMV_DAUTH_NATIVE			 : �ڲ�����
//       EMV_DAUTH_INDEX_NOT_SUPPORT : CA��Կ������֧��
//       EMV_DAUTH_FAIL              : �����ѻ�������֤ʧ�ܵ���������
int iEmvGetIcPubKey(void);

// SDA������֤
// ret : EMV_DAUTH_OK				 : OK
//       EMV_DAUTH_DATA_MISSING		 : ����ȱʧ
//       EMV_DAUTH_NATIVE			 : �ڲ�����
//       EMV_DAUTH_FAIL              : �����ѻ�������֤ʧ�ܵ���������
int iEmvSda(void);

// DDA������֤
// ret : EMV_DAUTH_OK				 : OK
//       EMV_DAUTH_DATA_MISSING		 : ����ȱʧ
//       EMV_DAUTH_NATIVE			 : �ڲ�����
//       EMV_DAUTH_DATA_ILLEGAL      : ���ݷǷ�
//       EMV_DAUTH_FAIL              : �����ѻ�������֤ʧ�ܵ���������
//       EMV_DAUTH_CARD_IO			 : ��Ƭ����ʧ��
//       EMV_DAUTH_CARD_SW			 : �Ƿ���״̬��
int iEmvDda(void);

// CDA������֤
// in  : ucGacFlag              : 1:��һ��GAC 2:�ڶ���GAC
//       psGacDataOut           : GACָ�������
//       ucGacDataOutLen        : GACָ������ݳ���
// ret : EMV_DAUTH_OK			: OK
//       EMV_DAUTH_DATA_MISSING	: ����ȱʧ
//       EMV_DAUTH_NATIVE		: �ڲ�����
//       EMV_DAUTH_FAIL         : �����ѻ�������֤ʧ�ܵ���������
int iEmvCda(uchar ucGacFlag, uchar *psGacDataOut, uchar ucGacDataOutLen);

// FDDA������֤
// ret : EMV_DAUTH_OK				 : OK
//       EMV_DAUTH_DATA_MISSING		 : ����ȱʧ
//       EMV_DAUTH_NATIVE			 : �ڲ�����
//       EMV_DAUTH_DATA_ILLEGAL      : ���ݷǷ�
//       EMV_DAUTH_FAIL              : �����ѻ�������֤ʧ�ܵ���������
// Note: FDDAʧ�ܺ�, Ӧ�ò���Ҫ���м��9F6C B1 b6b5, �Ծ����Ǿܾ����ǽ�������
int iEmvFdda(void);

#endif
