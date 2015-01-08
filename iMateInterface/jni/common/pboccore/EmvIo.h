/**************************************
File name     : EmvIO.h
Function      : EMV/pboc2.0�������ģ��
                �����ն����͡����ܾ�����������(����Ա|�ֿ���)
Author        : Yu Jun
First edition : Apr 13th, 2012
Modified      : 
**************************************/
#ifndef _EMVIO_H
#define _EMVIO_H

#define EMVIO_OK                    0   // OK
#define EMVIO_CANCEL                1   // ȡ��
#define EMVIO_BYPASS                2   // bypass���Թ�
#define EMVIO_TIMEOUT               3   // ��ʱ
#define EMVIO_BLACK                 4   // ������
#define EMVIO_ERROR                 99  // ��������

#define EMVIO_AMOUNT                0   // ������� Amount
#define EMVIO_AMOUNT_OTHER          1   // ������� Amount other
#define EMVIO_PLAIN_PIN             0   // ����PIN
#define EMVIO_CIPHERED_PIN          1   // ����PIN
#define EMVIO_NEED_NO_CONFIRM       0   // ��Ҫ��ȷ��
#define EMVIO_NEED_CONFIRM          1   // Ҫ��ȷ��

// ��ʼ���������ģ��
int iEmvIOInit(void);

// ���ý��׽��
// in  : pszAmount      : ���
//       pszAmountOther : �������
// ret : EMVIO_OK       : OK
//       EMVIO_ERROR    : ��������
int iEmvIOSetTransAmount(uchar *pszAmount, uchar *pszAmountOther);

// �����˺���Ϣ
// in  : iBlackFlag      : ����������־, 0:���Ǻ����� 1:������
//       pszRecentAmount : �����ǽ��
// ret : EMVIO_OK        : OK
//       EMVIO_ERROR     : ��������
int iEmvIOSetPanInfo(int iBlackFlag, uchar *pszRecentAmount);

// ��ȡ�����й�Կ֤�����к�
// out : psCertSerNo    : �����й�Կ֤�����к�
// ret : EMVIO_OK       : OK
//       EMVIO_ERROR    : ��������
int iEmvIOGetCertSerNo(uchar *psCertSerNo);

// ѡ������
// out : pszSelectedLang : ѡ�е����Դ���, ����:"zh"
//       pszLangs        : ��ѡ�������б�, ����:"frzhen"
//       pszLangsDesc    : ��ѡ�����������б�, '='�Ž�β, ����:"FRANCE=����=ENGLISH="
// ret : EMVIO_OK        : OK
//       EMVIO_CANCEL    : ȡ��
//       EMVIO_TIMEOUT   : ��ʱ
//       EMVIO_ERROR     : ��������
// Note: ��֧�ֲ���Աѡ������, ��ѡ������Ϊѡ��ֿ�������
//       ����attended�ն�, Ҫ��pinpad֧��������ѡ
//       ����unattended�ն�, ҲҪ��pinpad֧��������ѡ, ���unattended�ն˱�������pinpad���Ե�ͬ���ն�֧������
int iEmvIOSelectLang(uchar *pszSelectedLang, uchar *pszLangs, uchar *pszLangsDesc);

// ѡ��Ӧ��
// in  : pAppList      : ��Ƭ���ն˶�֧�ֵ�Ӧ���б�
//       iAppNum       : ��Ƭ���ն˶�֧�ֵ�Ӧ�ø���
// out : piAppNo       : ѡ�е�Ӧ����Ӧ���б��е��±�
// ret : EMVIO_OK      : OK
//       EMVIO_CANCEL  : ȡ��
//       EMVIO_TIMEOUT : ��ʱ
//       EMVIO_ERROR   : ��������
int iEmvIOSelectApp(stADFInfo *pAppList, int iAppNum, int *piAppNo);

// ȷ��Ӧ��
// in  : pApp          : Ӧ��
// ret : EMVIO_OK      : OK
//       EMVIO_CANCEL  : ȡ��
//       EMVIO_TIMEOUT : ��ʱ
//       EMVIO_ERROR   : ��������
int iEmvIOConfirmApp(stADFInfo *pApp);

// ��ȡ���
// in  : iAmountType   : ������ͣ�EMVIO_AMOUNT:Amount
//                                 EMVIO_AMOUNT_OTHER:Amount other
// out : pszAmount     : ���[12]
// ret : EMVIO_OK      : OK
//       EMVIO_CANCEL  : ȡ��
//       EMVIO_TIMEOUT : ��ʱ
//       EMVIO_ERROR   : ��������
int iEmvIOGetAmount(int iAmountType, uchar *pszAmount);

// ��ȡ����
// in  : iPinType      : ��������, EMVIO_PLAIN_PIN:����PIN
//                                 EMVIO_CIPHERED_PIN:����PIN
//       iBypassFlag   : ����Bypass���, 0:������bypass 1:����bypass
//       pszPan        : �˺�
// out : psPin         : 4-12λ��������('\0'��β���ַ���)��8�ֽ�����PIN
// ret : EMVIO_OK      : OK
//       EMVIO_CANCEL  : ȡ��
//       EMVIO_BYPASS  : bypass���Թ�
//       EMVIO_TIMEOUT : ��ʱ
//       EMVIO_ERROR   : ��������
int iEmvIOGetPin(int iPinType, int iBypassFlag, uchar *psPin, uchar *pszPan);

// ��֤�ֿ������֤��
// in  : iBypassFlag    : ����Bypass���, 0:������bypass 1:����bypass
//       ucHolderIdType : ֤������
//       szHolderId     : ֤������
// ret : EMVIO_OK       : OK
//       EMVIO_CANCEL   : ȡ��
//       EMVIO_BYPASS   : bypass���Թ���֤������
//       EMVIO_TIMEOUT  : ��ʱ
//       EMVIO_ERROR    : ��������
int iEmvIOVerifyHolderId(int iBypassFlag, uchar ucHolderIdType, uchar *pszHolderId);

// ��ʾ��Ϣ, �ȴ���ʱ�򽻻��������ʾ����
// in  : iMsgType      : ��Ϣ���ͣ���EmvMsg.h
//       iConfirmFlag  : ȷ�ϱ�־ EMVIO_NEED_NO_CONFIRM : ��Ҫ��ȷ��
//                                EMVIO_NEED_CONFIRM    : Ҫ��ȷ��
// ret : EMVIO_OK      : OK, ���Ҫ��ȷ�ϣ��Ѿ�ȷ��
//       EMVIO_CANCEL  : ȡ��
//       EMVIO_TIMEOUT : ��ʱ
int iEmvIODispMsg(int iMsgType, int iConfirmFlag);

// ��ʾ��Ϣ, ��Ϣ������Ļ��, �����˳�
// in  : iMsgType      : ��Ϣ���ͣ���EmvMsg.h
//                       iMsgType==-1��ʾ���֮ǰ��ʾ����Ϣ
// ret : EMVIO_OK      : OK
int iEmvIOShowMsg(int iMsgType);

// ���������ʾ��Ϣ
// in  : iMsgType      : ��Ϣ���ͣ���EmvMsg.h
//       iConfirmFlag  : ȷ�ϱ�־ EMVIO_NEED_NO_CONFIRM : ��Ҫ��ȷ��
//                                EMVIO_NEED_CONFIRM    : Ҫ��ȷ��
// ret : EMVIO_OK      : OK, ���Ҫ��ȷ�ϣ��Ѿ�ȷ��
//       EMVIO_CANCEL  : ȡ��
//       EMVIO_TIMEOUT : ��ʱ
// Note: ����unattended�ն˻�attended�ն˵���Pinpad������Ϣ����ʾ��Ψһ����ʾ����
int iEmvIOPinpadDispMsg(int iMsgType, int iConfirmFlag);

// ֤������б��ѯ
// in  : psCertSerNo     : ֤�����к�[3]
// ret : EMVIO_OK        : ����֤������б���
//       EMVIO_BLACK     : ��֤������б���
//       EMVIO_ERROR     : ��������
int iEmvIOCRLCheck(uchar *psCertSerNo);

// ��������ѯ
// ret : EMVIO_OK        : ���Ǻ�������
//       EMVIO_BLACK     : ��������
//       EMVIO_ERROR     : ��������
int iEmvIOBlackCardCheck(void);

// �ֿ����Ѳ�ѯ
// out : pszAmount       : ���˻���ʷ��¼���(�鲻������"0"), 
// ret : EMVIO_OK        : �ɹ�
//       EMVIO_ERROR     : ��������
int iEmvIOSeparateSaleCheck (uchar *pszAmount);

#endif
