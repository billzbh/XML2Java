/**************************************
File name     : EmvFace.h
Function      : EMV/pboc2.0����Ǻ����ⲿ�������ģ��ӿ�
Note          : ���Ӧ�ò��ʹ�÷ǻص��ӿ�, ����Ҫ��ģ��, ����Ҫ�ṩ��ԭ�ͺ����Թ�����.
Author        : Yu Jun
First edition : Mar 31st, 2012
Modified      : 
**************************************/
#ifndef _EMVFACE_H
#define _EMVFACE_H
#include "EmvCore.h"

#define EMVFACE_OK					0   // OK
#define EMVFACE_CANCEL				1   // ȡ��
#define EMVFACE_BYPASS				2   // bypass���Թ�
#define EMVFACE_TIMEOUT				3   // ��ʱ
#define EMVFACE_BLACK				4   // ��������
#define EMVFACE_FAIL                5   // ʧ��(�ֿ���֤����֤)
#define EMVFACE_ERROR				99  // ��������

#define EMVFACE_AMOUNT				0   // ������� Amount
#define EMVFACE_AMOUNT_OTHER		1   // ������� Amount other
#define EMVFACE_PLAIN_PIN			0   // ����PIN
#define EMVFACE_CIPHERED_PIN        1   // ����PIN
#define EMVFACE_NEED_NO_CONFIRM     0   // ��Ҫ��ȷ��
#define EMVFACE_NEED_CONFIRM        1   // Ҫ��ȷ��

// ѡ������
// in  : pszLangs        : ֧�ֵ�����, ����:"enzh"��ʾ֧��Ӣ�ġ�����
//       pszDescs        : ��'='��β��֧�ֵ���������, ����:"ENGLISH=����="������Ӣ�ġ�����
//       pszPrompt       : ��ʾ��Ϣ(������ʹ��, ��������ʾ)
//       pszLanguage     : ���Դ���(��ť������)��ISO-639-1��Сд
// out : pszSelectedLang : ѡ�е����Դ���, ����:"zh"
// ret : EMVFACE_OK      : OK
//       EMVFACE_CANCEL  : ȡ��
//       EMVFACE_TIMEOUT : ��ʱ
//       EMVFACE_ERROR   : ��������
// Note: ��֧�ֲ���Աѡ������, ��ѡ������Ϊѡ��ֿ�������
int iHxFaceSelectLang(uchar *pszLangs, uchar *pszDescs, uchar *pszSelectedLang, uchar *pszPrompt, uchar *pszLanguage);

// ѡ��Ӧ��
// in  : pAppList        : ��Ƭ���ն˶�֧�ֵ�Ӧ���б�
//       iAppNum         : ��Ƭ���ն˶�֧�ֵ�Ӧ�ø���
//       pszPrompt       : ��ʾ��Ϣ(������ʹ��, ��������ʾ)
//       pszLanguage     : Ӧ��ʹ�õ����Դ��룬ISO-639-1��Сд
// out : piAppNo         : ѡ�е�Ӧ����Ӧ���б��е��±�
// ret : EMVFACE_OK      : OK
//       EMVFACE_CANCEL  : ȡ��
//       EMVFACE_TIMEOUT : ��ʱ
//       EMVFACE_ERROR   : ��������
int iHxFaceSelectApp(stADFInfo *pAppList, int iAppNum, int *piAppNo, uchar *pszPrompt, uchar *pszLanguage);

// ȷ��Ӧ��
// in  : pApp            : Ӧ��
//       pszPrompt       : ��ʾ��Ϣ(������ʹ��, ��������ʾ)
//       pszLanguage     : Ӧ��ʹ�õ����Դ��룬ISO-639-1��Сд
// ret : EMVFACE_OK      : OK
//       EMVFACE_CANCEL  : ȡ��
//       EMVFACE_TIMEOUT : ��ʱ
//       EMVFACE_ERROR   : ��������
// Note: �ն�ֻ֧�ֹ����ַ�������ʹ��preferred name
int iHxFaceConfirmApp(stADFInfo *pApp, uchar *pszPrompt, uchar *pszLanguage);

// ��ȡ���
// in  : iAmountType     : ������ͣ�EMVFACE_AMOUNT:Amount
//                                   EMVFACE_AMOUNT_OTHER:Amount other
//       pszPrompt       : ��ʾ��Ϣ(������ʹ��, ��������ʾ)
//       pszCurrencyCode : 3�ֽڻ��ұ�ʾ����
//       iDecimalPosition: 0-3, С����λ��(С��������ָ���)
//       pszLanguage     : ���Դ��룬ISO-639-1��Сд
// out : pszAmount       : ���[12]
// ret : EMVFACE_OK      : OK
//       EMVFACE_CANCEL  : ȡ��
//       EMVFACE_TIMEOUT : ��ʱ
//       EMVFACE_ERROR   : ��������
// Note: �߲�����Ѿ�ȡ������Ӧ��������Ҫ�����룬ֱ�ӷ��ؼ���
int iHxFaceGetAmount(int iAmountType, uchar *pszAmount, uchar *pszPrompt, uchar *pszCurrencyCode, int iDecimalPosition, uchar *pszLanguage);

// ��ȡ����
// in  : iPinType        : ��������, EMVFACE_PLAIN_PIN:����PIN
//                                   EMVFACE_CIPHERED_PIN:����PIN
//       iBypassFlag     : ����Bypass���, 0:������bypass 1:����bypass
//       pszPan          : �˺�
//       pszPrompt1      : ��ʾ��Ϣ1(��ʽ���õĽ��)
//       pszPrompt2      : ��ʾ��Ϣ2(������ʹ��, ��������ʾ)
//       pszLanguage     : ���Դ��룬ISO-639-1��Сд
// out : psPin           : 4-12λ��������('\0'��β���ַ���)��8�ֽ�����PIN
// ret : EMVFACE_OK      : OK
//       EMVFACE_CANCEL  : ȡ��
//       EMVFACE_BYPASS  : bypass���Թ�
//       EMVFACE_TIMEOUT : ��ʱ
//       EMVFACE_ERROR   : ��������
// Note: ����������ʱֻ�򵥽�����TwoOneһ��
int iHxFaceGetPin(int iPinType, int iBypassFlag, uchar *psPin, uchar *pszPan, uchar *pszPrompt1, uchar *pszPrompt2, uchar *pszLanguage);

// ��֤�ֿ���֤��
// in  : iBypassFlag     : ����Bypass���, 0:������bypass 1:����bypass
//       pszPrompt       : ����ʾ��Ϣ("�����ֿ���֤��")
//       pszPromptIdType : ��ʾ��Ϣ, ֤������("�ֿ���֤������")
//       pszIdType       : ֤����������
//       pszPromptIdNo   : ��ʾ��Ϣ, ֤������("�ֿ���֤������")
//       pszIdNo         : ֤������
//       pszLanguage     : ���Դ��룬ISO-639-1��Сд
// ret : EMVFACE_OK      : OK, ��֤ͨ��
//       EMVFACE_CANCEL  : ȡ��
//       EMVFACE_BYPASS  : bypass���Թ�����֤ʧ��
//       EMVFACE_FAIL    : ֤������
//       EMVFACE_TIMEOUT : ��ʱ
// Note: ��ʾ��Ϣ������ʹ��, ��������ʾ
int iHxFaceVerifyHolderId(int iBypassFlag, uchar *pszPrompt, uchar *pszPromptIdType, uchar *pszIdType, uchar *pszPromptIdNo, uchar *pszIdNo, uchar *pszLanguage);

// ��ʾ��Ϣ, �ȴ���ʱ�򽻻��������ʾ����
// in  : iMsgType        : ��Ϣ���ͣ������ⲿҪ�Լ��ṩ��ʾ��Ϣʱ
//       iConfirmFlag    : �Ƿ�Ҫȷ�ϱ�־, EMVFACE_NEED_NO_CONFIRM:��Ҫ��ȷ��
//                                         EMVFACE_NEED_CONFIRM:Ҫ��ȷ��
//       pszPrompt1      : ��ʾ��Ϣ1(������ʹ��, ��������ʾ)
//       pszPrompt2      : ��ʾ��Ϣ2(������Ϣ, ����:Ҫȷ�ϵĽ��)
//       pszLanguage     : ���Դ��룬ISO-639-1��Сд
// ret : EMVFACE_OK      : OK, ���Ҫ��ȷ�ϣ��Ѿ�ȷ��
//       EMVFACE_CANCEL  : ȡ��
//       EMVFACE_TIMEOUT : ��ʱ
int iHxFaceDispMsg(int iMsgType, int iConfirmFlag, uchar *pszPrompt1, uchar *pszPrompt2, uchar *pszLanguage);

// ��ʾ��Ϣ, ��Ϣ������Ļ��, �����˳�
// in  : iMsgType        : ��Ϣ���ͣ������ⲿҪ�Լ��ṩ��ʾ��Ϣʱ
//       pszPrompt1      : ��ʾ��Ϣ1(������ʹ��, ��������ʾ)
//       pszPrompt2      : ��ʾ��Ϣ2(������Ϣ, ����:Ҫȷ�ϵĽ��)
//       pszLanguage     : ���Դ��룬ISO-639-1��Сд
// ret : EMVFACE_OK      : OK
// Note: pszPrompt1��pszPrompt2��ΪNULL��ʾ���֮ǰ�ú�����ʾ����Ϣ
int iHxFaceShowMsg(int iMsgType, uchar *pszPrompt1, uchar *pszPrompt2, uchar *pszLanguage);

// Pinpad��ʾ��Ϣ
// in  : iMsgType        : ��Ϣ���ͣ������ⲿҪ�Լ��ṩ��ʾ��Ϣʱ
//       iConfirmFlag    : �Ƿ�Ҫȷ�ϱ�־, EMVFACE_NEED_NO_CONFIRM:��Ҫ��ȷ��
//                                         EMVFACE_NEED_CONFIRM:Ҫ��ȷ��
//       pszPrompt1      : ��ʾ��Ϣ1(������ʹ��, ��������ʾ)
//       pszPrompt2      : ��ʾ��Ϣ2(������Ϣ, ����:Ҫȷ�ϵĽ��)
//       pszLanguage     : ���Դ��룬ISO-639-1��Сд
// ret : EMVFACE_OK      : OK, ���Ҫ��ȷ�ϣ��Ѿ�ȷ��
//       EMVFACE_CANCEL  : ȡ��
//       EMVFACE_TIMEOUT : ��ʱ
int iHxFacePinpadDispMsg(int iMsgType, int iConfirmFlag, uchar *pszPrompt1, uchar *pszPrompt2, uchar *pszLanguage);

// ֤������б��ѯ
// in  : psRid           : RID[5]
//       ucCaIndex       : CA��Կ����
//       psCertSerNo     : ֤�����к�[3]
// ret : EMVFACE_OK      : ����֤������б���
//       EMVFACE_BLACK   : ��֤������б���
//       EMVFACE_ERROR   : ��������
int iHxFaceCRLCheck(uchar *psRid, uchar ucCaIndex, uchar *psCertSerNo);

// ��������ѯ
// in  : psRid           : RID[5]
//       pszPan          : �˺ţ��Ѿ�ȥ����β��'F'
//       iPanSeqNo       : �˺����кţ�-1��ʾ������
// ret : EMVFACE_OK      : ���Ǻ�������
//       EMVFACE_BLACK   : ��������
//       EMVFACE_ERROR   : ��������
int iHxFaceBlackCardCheck(uchar *psRid, uchar *pszPan, int iPanSeqNo);

// �ֿ����Ѳ�ѯ
// in  : psRid           : RID[5]
//       pszPan          : �˺ţ��Ѿ�ȥ����β��'F'
//       iPanSeqNo       : �˺����кţ�-1��ʾ������
// out : pszAmount       : ���˻���ʷ��¼���(�鲻������"0"), 
// ret : EMVFACE_OK      : �ɹ�
//       EMVFACE_ERROR   : ��������
int iHxFaceSeparateSaleCheck (uchar *psRid, uchar *pszPan, int iPanSeqNo, uchar *pszAmount);

/****----****----****----****----****----****----****----****----****
���Ϻ���ΪEmv���Ļص���������ıر�����, ����ʵ��, ʹ�÷ǻص��ӿ�ʱ���붨��
���º���Ϊ�ͼ����Ϊ������ʾ���ӵĸ�������, �ɲ�ʹ��, ʹ�÷ǻص��ӿ�ʱ���ض���
****----****----****----****----****----****----****----****----****/
// ��ʼ��"��ȴ�"״̬
void vHxFaceClearWaitFlag(void);
// ���ý�������
void vHxFaceSetTransType(uint uiTransType);
// ��ʾ�������ͱ���
void vHxFaceShowTransTitle(uint uiTransType, uchar ucTransRoute);

#endif
