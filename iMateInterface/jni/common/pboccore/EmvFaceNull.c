/**************************************
File name     : EmvFaceNull.c
Function      : EMV/pboc2.0����Ǻ����ⲿ�������ģ��ӿ�
Note          : ���Ӧ�ò��ʹ�÷ǻص��ӿ�, ����Ҫ��ģ��, ����Ҫ�ṩ��ԭ�ͺ����Թ�����.
                ��ģ��Ϊ�պ���ʵ��, ����ʵ���뿴<EmvFace.c>
Author        : Yu Jun
First edition : Mar 31st, 2012
Modified      : Apr 13th, 2013
				    ���Ӷ��Զ����׵�֧��
**************************************/
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "VposFace.h"
#include "EmvFace.h"

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
int iHxFaceSelectLang(uchar *pszLangs, uchar *pszDescs, uchar *pszSelectedLang, uchar *pszPrompt, uchar *pszLanguage)
{
	return(EMVFACE_ERROR);
}

// ѡ��Ӧ��
// in  : pAppList        : ��Ƭ���ն˶�֧�ֵ�Ӧ���б�
//       iAppNum         : ��Ƭ���ն˶�֧�ֵ�Ӧ�ø���
//       pszPrompt       : ��ʾ��Ϣ(������ʹ��, ��������ʾ)
//       pszLanguage     : ���Դ��룬ISO-639-1��Сд
// out : piAppNo         : ѡ�е�Ӧ����Ӧ���б��е��±�
// ret : EMVFACE_OK      : OK
//       EMVFACE_CANCEL  : ȡ��
//       EMVFACE_TIMEOUT : ��ʱ
//       EMVFACE_ERROR   : ��������
int iHxFaceSelectApp(stADFInfo *pAppList, int iAppNum, int *piAppNo, uchar *pszPrompt, uchar *pszLanguage)
{
	return(EMVFACE_ERROR);
}

// ȷ��Ӧ��
// in  : pApp            : Ӧ��
//       pszPrompt       : ��ʾ��Ϣ(������ʹ��, ��������ʾ)
//       pszLanguage     : ���Դ��룬ISO-639-1��Сд
// ret : EMVFACE_OK      : OK
//       EMVFACE_CANCEL  : ȡ��
//       EMVFACE_TIMEOUT : ��ʱ
//       EMVFACE_ERROR   : ��������
// Note: �ն�ֻ֧�ֹ����ַ�������ʹ��preferred name
int iHxFaceConfirmApp(stADFInfo *pApp, uchar *pszPrompt, uchar *pszLanguage)
{
	return(EMVFACE_ERROR);
}

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
int iHxFaceGetAmount(int iAmountType, uchar *pszAmount, uchar *pszPrompt, uchar *pszCurrencyCode, int iDecimalPosition, uchar *pszLanguage)
{
	return(EMVFACE_ERROR);
}

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
int iHxFaceGetPin(int iPinType, int iBypassFlag, uchar *psPin, uchar *pszPan, uchar *pszPrompt1, uchar *pszPrompt2, uchar *pszLanguage)
{
	return(EMVFACE_ERROR);
}

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
int iHxFaceVerifyHolderId(int iBypassFlag, uchar *pszPrompt, uchar *pszPromptIdType, uchar *pszIdType, uchar *pszPromptIdNo, uchar *pszIdNo, uchar *pszLanguage)
{
	return(EMVFACE_FAIL);
}

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
// Note: ���֮ǰiHxFaceShowMsg()��ʾ������, ��������Ҫ�����iHxFaceShowMsg()��ʾ������
int iHxFaceDispMsg(int iMsgType, int iConfirmFlag, uchar *pszPrompt1, uchar *pszPrompt2, uchar *pszLanguage)
{
	return(EMVFACE_OK);
}

// ��ʾ��Ϣ, ��Ϣ������Ļ��, �����˳�
// in  : iMsgType        : ��Ϣ���ͣ������ⲿҪ�Լ��ṩ��ʾ��Ϣʱ
//       pszPrompt1      : ��ʾ��Ϣ1(������ʹ��, ��������ʾ)
//       pszPrompt2      : ��ʾ��Ϣ2(������Ϣ, ����:Ҫȷ�ϵĽ��)
//       pszLanguage     : ���Դ��룬ISO-639-1��Сд
// ret : EMVFACE_OK      : OK
// Note: pszPrompt1��pszPrompt2��ΪNULL��ʾ���֮ǰ�ú�����ʾ����Ϣ
int iHxFaceShowMsg(int iMsgType, uchar *pszPrompt1, uchar *pszPrompt2, uchar *pszLanguage)
{
	return(EMVFACE_OK);
}

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
int iHxFacePinpadDispMsg(int iMsgType, int iConfirmFlag, uchar *pszPrompt1, uchar *pszPrompt2, uchar *pszLanguage)
{
	return(EMVFACE_OK);
}

// ֤������б��ѯ
// in  : psRid           : RID[5]
//       ucCaIndex       : CA��Կ����
//       psCertSerNo     : ֤�����к�[3]
// ret : EMVFACE_OK      : ����֤������б���
//       EMVFACE_BLACK   : ��֤������б���
//       EMVFACE_ERROR   : ��������
int iHxFaceCRLCheck(uchar *psRid, uchar ucCaIndex, uchar *psCertSerNo)
{
	return(EMVFACE_ERROR);
}

// ��������ѯ
// in  : psRid           : RID[5]
//       pszPan          : �˺ţ��Ѿ�ȥ����β��'F'
//       iPanSeqNo       : �˺����кţ�-1��ʾ������
// ret : EMVFACE_OK      : ���Ǻ�������
//       EMVFACE_BLACK   : ��������
//       EMVFACE_ERROR   : ��������
int iHxFaceBlackCardCheck(uchar *psRid, uchar *pszPan, int iPanSeqNo)
{
	return(EMVFACE_ERROR);
}

// �ֿ����Ѳ�ѯ
// in  : psRid           : RID[5]
//       pszPan          : �˺ţ��Ѿ�ȥ����β��'F'
//       iPanSeqNo       : �˺����кţ�-1��ʾ������
// out : pszAmount       : ���˻���ʷ��¼���(�鲻������"0"), 
// ret : EMVFACE_OK      : �ɹ�
//       EMVFACE_ERROR   : ��������
int iHxFaceSeparateSaleCheck(uchar *psRid, uchar *pszPan, int iPanSeqNo, uchar *pszAmount)
{
	return(EMVFACE_ERROR);
}
