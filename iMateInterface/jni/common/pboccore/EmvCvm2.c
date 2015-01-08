/**************************************
File name     : EmvCvm2.c
Function      : Emv�ֿ�����֤ģ��(�ǻص��ӿ�)
Author        : Yu Jun
First edition : Apr 25th, 2012
Modified      : Jan 24th, 2013
                    �޸�Ϊ�ǻص���ʽ�ӿ�, Դ��EmvCvm.c
Modified      : Mar 20th, 2014
                    ������֤����ʧ�ܺ�δ����TVR�ֿ�����֤ʧ��λBug
                    ��������Bypass��δ����TVR�ֿ���δ��������λBug
                    �����������������δ���浽TLV���ݿ���Bug
				Mar 31st, 2014
					�����жϾܾ�λ, ���һЩ��������ͨ��, Cvmģ�鲻���жϾܾ�λ
					(����, 20140422, ����Ӧ�����;����Ƿ����жϾܾ�λ)
				Apr 4th, 2014
					����iCvm2GetMethod()����ΪCVM�б���ỹûͨ��ʱ, CVM Resultδ��ֵBug
					����iCvm2DoMethod()���ѻ�Pin+ǩ��ʱӦ������CVMResultΪδ֪������Ϊ�ɹ�Bug
				Apr 22nd, 2014
					�������2cj.002.05 0a����, ����֪��Ҫ�ܾ�ʱ��������Ҫ�����ʱ, ����Ӧ�����;����Ǽ��������̻��Ǿܾ�
				Aug 14th, 2014
				    ����iCvm2GetMethod(), �����жϺ����Ƿ�֧����һ���ѻ�������֤����ʱδ�ж��ѻ�����PIN��֤
					����iCvm2GetMethod(), �ڳֿ�����֤�׶�, �������ھܾ�λ���, ��Ϊ�������ĳЩ�ؼ�TVRλû����λ
					����iCvm2DoMethod(), �ڳֿ�����֤�׶�, �������ھܾ�λ���, ��Ϊ�������ĳЩ�ؼ�TVRλû����λ
**************************************/
/*
ģ����ϸ����:
	�ǻص���ʽ�ֿ�����֤
.	֧�ֵĳֿ�����֤����
	�ѻ��������롢�����������롢�ֿ���ǩ�֡�NO CVM REQUIRED��FAIL CVM���ֿ���֤��
.	֧��Bypass PIN
.	֧�ֽ��ȷ��
	����������Ϊ����Ѿ��õ��ֿ���ȷ��
	����ֿ�����֤����ǰ��û�õ�ȷ��, ��Ҫ��ֿ���ȷ�Ͻ��
	sg_iConfirmAmountFlag���ڼ�¼���ȷ�ϱ�־
.	EC��������֧��
	����ж�ΪEC��������, ��û���������������, Ҫ��������������
.	�ǻص���ʽCVM���˼��
	���뿨ƬCVM�б�, ����ѡ������������CVM, ���浽����sg_asCvmList
	Ӧ�ò����CVMʱ, ��sg_asCvmList����һѡ��, ����sg_iCvmProcIndex��¼������λ��
	����ֿ�����֤����(�ɹ�/ʧ��/����������CVM), ת�븽�Ӵ���
.	֧�ָ��Ӵ���
	EC����������Ҫ��������, �������CVM������������, ��EC�������׻�Ҫ��������������
	���ȷ��, ���֮ǰû���κ�CVM���й��ֿ��˽��ȷ��, ��Ҫ��ֿ���ȷ�Ͻ��
	����sg_iECExtProcStatus���ڼ�¼���Ӵ���״̬
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "VposFace.h"
#include "Arith.h"
#include "Common.h"
#include "EmvCmd.h"
#include "TlvFunc.h"
#include "EmvFunc.h"
#include "TagAttr.h"
#include "EmvData.h"
#include "EmvCore.h"
#include "EmvMsg.h"
#include "EmvIo.h"
#include "EmvModul.h"
#include "TagDef.h"
#include "EmvCvm.h"
#include "EmvCvm2.h"

static uchar sg_asCvmList[250];		 // ����������CVM�б�(���1[4]+���2[4]+Item1[2]+Item2[2]+...)
static int   sg_iCvmListLen;         // CVM�б���
static int   sg_iCvmProcIndex;       // ��һ��Ҫ�����CVM��CVM�б��е�ƫ����
                                     // -1��ʾ��δ��ʼ��

static int sg_iOnlinePinBypassFlag;  // ��������bypass��־, ���ڼ�¼��������������bypass��, 0:û��bypass 1:bypass��
static int sg_iOfflinePinBypassFlag; // �ѻ�����bypass��־, ���ڼ�¼�ѻ�����������bypass��, 0:û��bypass 1:bypass��
static int sg_iOfflinePinBlockFlag;  // �ѻ�����������־, 0:δ֪ 1:����
static int sg_iConfirmAmountFlag;    // ���ȷ�ϱ�־, ���ڼ�¼����Ƿ�ȷ�Ϲ�, 0:û�� 1:��

static int sg_iCvmOKFlag;            // �ֿ�����֤�ɹ���־, 0:δ�ɹ� 1:�ɹ�
static int sg_iCvmSupportFlag;       // ����֧�ֵ�CVM��־, 0:��֧�ֵ�CVM 1:��֧�ֵ�CVM
static int sg_iCvmNeedSignFlag;      // Ҫ��ǩ�ֱ�־, 0:��Ҫ��ǩ�� 1:Ҫ��ǩ��
static int sg_iECExtProcStatus;      // ���Ӵ����־״̬, 0:δ��ʼ
                                     //                   1:���ڴ���EC�������븽�Ӵ��� 2:EC�������븽�Ӵ������
                                     //                   3:���ڴ�����ȷ�ϸ��Ӵ��� 4:���ȷ�ϸ��Ӵ������
                                     //                   10:���Ӵ������

static int sg_iPinTryLeft;			 // ʣ���ѻ����볢�Դ���, 0:�����Ѿ����� >0:ʣ�ೢ�Դ��� -1:δ���� -2:���ܻ�ȡ

// CVM2�����ʼ��, emv���׳�ʼ��ʱ����
void vCvm2Init(void)
{
	sg_iCvmProcIndex = -1; // ��δ��ʼ��
	sg_iPinTryLeft = -1; // ����ʣ���ѻ����볢�Դ���Ϊδ����
}

// ׼��CVM����, ѡ������������CVM��
// ret : HXEMV_OK          : OK
//       HXEMV_TERMINATE   : CVM list��ʽ����, ��Ҫ��ֹ
//       HXEMV_NA		   : ��Ƭ��֧��
//       HXEMV_CORE        : �ڲ�����
static int iCvm2PrepareCvmList(void)
{
	int   iRet;
	uchar ucCvmListLen, *psCvmList;
	int   i;

	// ��ʼ������
	sg_iConfirmAmountFlag = 0;    // ���û�б�ȷ�Ϲ�
	sg_iOfflinePinBypassFlag = 0; // ����ѻ�����bypass���
	sg_iOfflinePinBlockFlag = 0;  // ����ѻ������������
	sg_iOnlinePinBypassFlag = 0;  // �����������bypass���

	sg_iCvmListLen = 0;           // ��ʼ��CVM�б���, ÿ����Ŀռ��2�ֽ�
    sg_iCvmProcIndex = 0;         // ��һ��Ҫ�����CVM��CVM�б��е��±�, 0��ʼ, ���α�ʾΪ2��4��6��...

	sg_iCvmOKFlag = 0;            // �ֿ�����֤�ɹ���־, 0:δ�ɹ� 1:�ɹ�
	sg_iCvmSupportFlag = 0;       // ����֧�ֵ�CVM��־, 0:��֧�ֵ�CVM 1:��֧�ֵ�CVM
	sg_iCvmNeedSignFlag = 0;      // ��ʼֵΪ��Ҫ��ǩ��
	sg_iECExtProcStatus = 0;      // ���Ӵ����־״̬, 0:δ��ʼ

	if(iEmvTestItemBit(EMV_ITEM_AIP, AIP_03_CVM_SUPPORTED) == 0)
		return(HXEMV_NA); // AIPָʾ��֧�ֳֿ�����֤
	iRet = iTlvGetObjValue(gl_sTlvDbCard, TAG_8E_CVMList, &psCvmList);
	if(iRet<=0 || iRet==8) {
		// ��CVM list(����Ϊ8��ʾֻ�н��X����Y, ��CVM rule)
		iEmvSetItemBit(EMV_ITEM_TVR, TVR_02_ICC_DATA_MISSING, 1);
		return(HXEMV_NA); 
	}
	if(iRet<8 || iRet%2)
		return(HXEMV_TERMINATE); // CVM list��ʽ����
	ucCvmListLen = (uchar)iRet;

	// ��ȡ�����������ĳֿ�����֤����
	for(i=8; i<(int)ucCvmListLen; i+=2) {
		// �ж�������
		iRet = iCvmCheckCondition(psCvmList+i, psCvmList);
		if(iRet!=0 && iRet!=HXEMV_NA)
			return(iRet); // ���ִ���, ����
		if(iRet == HXEMV_NA)
			continue; // ����������,������һ����֤����
		// ��������, ��ӵ��б�
		memcpy(&sg_asCvmList[sg_iCvmListLen], psCvmList+i, 2);
		sg_iCvmListLen += 2;
	} // for(i=8; i<(int)ucCvmListLen; i+=2) {

	return(HXEMV_OK);
}

// ִ�гֿ�����֤����
// in  : psCvmResult     : �±�0-1��ʾ�ֿ�����֤����
// out : psCvmResult     : �±�2��ʾ��֤���
//       piNeedSignFlag  : ����ɹ���ɣ�������Ҫǩ�ֱ�־��0:����Ҫǩ�� 1:��Ҫǩ��
// ret : 0               : ��֤�ɹ�
//       -1              : ��ʶ����֤����
//       -2              : ʶ����֤����, ��֧����֤����
//       -3              : ʶ����֤����, ֧����֤����, ��֤ʧ��
//       -4              : ʶ����֤����, ֧����֤����, ��Ҫ����
//       HXEMV_CARD_OP   : ��������
//       HXEMV_TERMINATE : ����ܾ�������������ֹ
//       HXEMV_CORE      : �ڲ�����
static int iCvm2DoVerify(uchar *psCvmResult, int *piNeedSignFlag)
{
	int   iRet;
	uint  uiRet;
	uchar ucLen, sBuf[30], *p;
	int   iCvmRecognisedFlag; // CVMʶ���־, 0:��ʶ�� 1:ʶ��
	int   iCvmSupportFlag; // CVM֧�ֱ�־, 0:��֧�� 1:֧��
	int   iCvmPassFlag; // CVMͨ����־, 0:ûͨ�� 1:ͨ��
	int   iCvmNeedInteraction; // ��Ҫ������־, 0:����Ҫ 1:��Ҫ

	iCvmSupportFlag = 0; // ���ȼٶ���֧����֤����
	*piNeedSignFlag = 0; // ���ȼٶ�����Ҫǩ��
	iCvmRecognisedFlag = 1; // ���ȼٶ�ʶ���CVM
	iCvmPassFlag = 0; // �ȼ���CVMû��ͨ��
	iCvmNeedInteraction = 0; // �ȼ��費��Ҫ����
	switch(psCvmResult[0] & 0x3F) {
	case 0: // Fail CVM processing
		iCvmSupportFlag = 1; // CVM֧�ֱ�־, 0:��֧�� 1:֧��
		psCvmResult[2] = 1;
		break;
	case 1: // Plaintext PIN verification performed by ICC
	case 3: // Plaintext PIN verification performed by ICC and signature
		if(iEmvTestItemBit(EMV_ITEM_TERM_CAP, TERM_CAP_08_PLAIN_PIN) == 0)
			break; // ��֧�ָ÷���
		if((psCvmResult[0]&0x3F) == 3) {
			// ��Ҫ����ǩ��
			if(iEmvTestItemBit(EMV_ITEM_TERM_CAP, TERM_CAP_10_SIGNATURE) == 0)
				break; // ��֧�ָ÷���
			*piNeedSignFlag = 1; // ��Ҫǩ��
		}
		iCvmSupportFlag = 1; // CVM֧�ֱ�־, 0:��֧�� 1:֧��
		if(sg_iOfflinePinBypassFlag || sg_iOfflinePinBlockFlag) {
			// �ѻ�����bypass || ��������
			psCvmResult[2] = 1; // 0:Unknown 1:Failed 2:Successfull
			break;
		}

		if(sg_iPinTryLeft == -1) {
			// sg_iPinTryLeft(0:�����Ѿ����� >0:ʣ�ೢ�Դ��� -1:δ���� -2:���ܻ�ȡ)
			// ֻ����sg_iPinTryLeft����δ����״̬����Ҫ�ж��ѻ������Ƿ���
			sg_iPinTryLeft = -2; // �ȼٶ�ʣ�����볢�Դ������ܻ�ȡ
			uiRet = uiEmvCmdGetData(TAG_9F17_PINTryCounter, &ucLen, sBuf);
			if(uiRet == 1)
				return(HXEMV_CARD_OP);
			if(uiRet == 0) {
				iRet = iTlvCheckTlvObject(sBuf);
				if(iRet == ucLen) {
					iRet = iTlvValue(sBuf, &p);
					if(iRet == 1) {// ������ʣ�����볢�Դ���
						sg_iPinTryLeft = *p;
						if(*p == 0) {
							// ���볢�Դ����ѵ�
							iEmvSetItemBit(EMV_ITEM_TVR, TVR_18_EXCEEDS_PIN_TRY_LIMIT, 1);
							sg_iOfflinePinBlockFlag = 1; // �ѻ���������
							psCvmResult[2] = 1; // 0:Unknown 1:Failed 2:Successfull
							break;
						}
					}
				}
			}
		} // if(sg_iPinTryLeft == -1) {
		iCvmNeedInteraction = 1; // ��Ҫ����
		break;
	case 2: // Enciphered PIN verified online
		if(iEmvTestItemBit(EMV_ITEM_TERM_CAP, TERM_CAP_09_ENC_ONLINE_PIN) == 0)
			break; // ��֧�ָ÷���
		iCvmSupportFlag = 1; // CVM֧�ֱ�־, 0:��֧�� 1:֧��
		if(sg_iOnlinePinBypassFlag) {
			// ��������bypass
			psCvmResult[2] = 1; // 0:Unknown 1:Failed 2:Successfull
			break;
		}
		iCvmNeedInteraction = 1; // ��Ҫ����
		break;
	case 4: // Enciphered PIN verification performed by ICC
		if(iEmvTestItemBit(EMV_ITEM_TERM_CAP, TERM_CAP_11_ENC_OFFLINE_PIN) == 0)
			break; // ��֧�ָ÷���
		break; // û��ʵ��, ��ʹ�ն�����ָʾλ��ʾ֧���ѻ�����������֤, Ҳ����Ϊ��:(
	case 5: // Enciphered PIN verification performed by ICC and signature
		if(iEmvTestItemBit(EMV_ITEM_TERM_CAP, TERM_CAP_11_ENC_OFFLINE_PIN) == 0)
			break; // ��֧�ָ÷���
		// ��Ҫ����ǩ��
		if(iEmvTestItemBit(EMV_ITEM_TERM_CAP, TERM_CAP_10_SIGNATURE) == 0)
			break; // ��֧�ָ÷���
		*piNeedSignFlag = 1; // ��Ҫǩ��
		break; // û��ʵ��, ��ʹ�ն�����ָʾλ��ʾ֧���ѻ�����������֤, Ҳ����Ϊ��:(
	case 0x1E: // signature
		if(iEmvTestItemBit(EMV_ITEM_TERM_CAP, TERM_CAP_10_SIGNATURE) == 0)
			break; // ��֧�ָ÷���
		iCvmSupportFlag = 1; // CVM֧�ֱ�־, 0:��֧�� 1:֧��
		*piNeedSignFlag = 1; // ��Ҫǩ��
		psCvmResult[2] = 0; // 0:Unknown 1:Failed 2:Successfull
		iCvmPassFlag = 1; // CVMͨ����־, 0:ûͨ�� 1:ͨ��
		break;
	case 0x1F: // No CVM required
		if(iEmvTestItemBit(EMV_ITEM_TERM_CAP, TERM_CAP_12_NO_CVM_REQUIRED) == 0)
			break; // ��֧�ָ÷���
		// ֧��No CVM required
		iCvmSupportFlag = 1; // CVM֧�ֱ�־, 0:��֧�� 1:֧��
		psCvmResult[2] = 2; // 0:Unknown 1:Failed 2:Successfull
		iCvmPassFlag = 1; // CVMͨ����־, 0:ûͨ�� 1:ͨ��
		break;
	case 0x20: // Pboc2.0 �ֿ���֤����֤
		iCvmSupportFlag = 1; // CVM֧�ֱ�־, 0:��֧�� 1:֧��
		// ȷ���ֿ���֤����֤�����걸
		iRet = iTlvGetObjValue(gl_sTlvDbCard, TAG_9F62_HolderIdType, &p);
		if(iRet <= 0) {
			// ����ȱʧ
			iEmvSetItemBit(EMV_ITEM_TVR, TVR_02_ICC_DATA_MISSING, 1); // data missing
			break; // ��Ϊ��֤ʧ��
		}
		if(*p > 5)
			break; // Ŀǰ֤������ֻ֧��0-5, ���Խ��, ��Ϊ��֤ʧ��
		iRet = iTlvGetObjValue(gl_sTlvDbCard, TAG_9F61_HolderId, &p);
		if(iRet <= 0) {
			// ����ȱʧ
			iEmvSetItemBit(EMV_ITEM_TVR, TVR_02_ICC_DATA_MISSING, 1); // data missing
			break; // ��Ϊ��֤ʧ��
		}
		iCvmNeedInteraction = 1; // ��Ҫ����
		break;
	default:
		iCvmRecognisedFlag = 0;
		break;
	}
	if(iCvmRecognisedFlag == 0) {
		// unrecognised CVM
		iEmvSetItemBit(EMV_ITEM_TVR, TVR_17_UNRECOGNISED_CVM, 1);
		return(-1); // ��ʶ����֤����
	}
	if(iCvmSupportFlag == 0) {
		// unsupported CVM
		return(-2); // ��֧����֤����
	}
	if(iCvmNeedInteraction == 1) {
		return(-4); // ��Ҫ����
	}
	if(iCvmPassFlag == 0) {
		// ��֤ʧ��
		return(-3);
	}
	return(0); // ��֤�ɹ�
}

// ׼����������
// in  : psCvm             : CVM����
// out : piCvm             : �ֿ�����֤����, ֧��HXCVM_PLAIN_PIN��HXCVM_CIPHERED_ONLINE_PIN��HXCVM_HOLDER_ID��HXCVM_CONFIRM_AMOUNT
//       piBypassFlag      : ����bypass��־, 0:������, 1:����
//       piMsgType         : ��ʾ��Ϣ��(�����֤����Ϊ֤����֤, ����֤��������)
//       piMsgType2        : ������ʾ��Ϣ, �����Ϊ0, ����ʾ����Ϣ
//       pszAmountStr      : ��ʾ�ø�ʽ�����(HXCVM_HOLDER_ID��֤��������Ҫ)
// ret : HXEMV_OK          : OK
//       HXEMV_CORE        : �ڲ�����
static int iCvm2PrepareInteractData(uchar *psCvm, int *piCvm, int *piBypassFlag, int *piMsgType, int *piMsgType2, uchar *pszAmountStr)
{
	int   iRet;
	uchar *pucHolderIdType;

	*piMsgType2 = 0;

	switch(psCvm[0] & 0x3F) {
	case 1: // Plaintext PIN verification performed by ICC
	case 3: // Plaintext PIN verification performed by ICC and signature
		*piCvm = HXCVM_PLAIN_PIN;
		if(iEmvTermEnvironment() == TERM_ENV_ATTENDED)
			*piBypassFlag = 1; // ����bypass
		else
			*piBypassFlag = 0; // ������bypass
		*piMsgType = EMVMSG_ENTER_PIN_OFFLINE;
		if(sg_iPinTryLeft == 1)
			*piMsgType2 = EMVMSG_LAST_PIN;// ��ʣ�����һ�λ���
		iRet = iMakeAmountStr(pszAmountStr);
		if(iRet <= 0)
			pszAmountStr[0] = 0; // �������Ϊ0, ����ʾ���
		break;
	case 2: // Enciphered PIN verified online
		*piCvm = HXCVM_CIPHERED_ONLINE_PIN;
		if(iEmvTermEnvironment() == TERM_ENV_ATTENDED)
			*piBypassFlag = 1; // ����bypass
		else
			*piBypassFlag = 0; // ������bypass
		*piMsgType = EMVMSG_ENTER_PIN_ONLINE;
		iRet = iMakeAmountStr(pszAmountStr);
		if(iRet <= 0)
			pszAmountStr[0] = 0; // �������Ϊ0, ����ʾ���
		break;
	case 0x20: // Pboc2.0 �ֿ���֤����֤
		*piCvm = HXCVM_HOLDER_ID;
		*piBypassFlag = 1;   // ����bypass
		// ���ڳֿ���֤����֤Ҫ�󷵻���Ϣ�϶�(��ʾ��֤��������ʾ��֤�����͡�֤��������ʾ��֤������)
		// ��˶���֤����֤, ֻ����֤�����͵Ĵ�����Ϣ, ������Ϣ��Ӧ�ò�������ȡ
		iRet = iTlvGetObjValue(gl_sTlvDbCard, TAG_9F62_HolderIdType, &pucHolderIdType);
		ASSERT(iRet == 1);
		if(iRet != 1)
			return(HXEMV_CORE); // ����¼ʱ��֤�����ݺϷ���, ��ȡ�ֿ�����֤����ʱ��֤�����ݴ���, ��Ӧ�ô�ʱ���ز��Ϸ�
		if(*pucHolderIdType == 0)
			*piMsgType = EMVMSG_IDENTIFICATION_CARD; // "���֤"
		else if(*pucHolderIdType == 1)
			*piMsgType = EMVMSG_CERTIFICATE_OF_OFFICERS; // "����֤"
		else if(*pucHolderIdType == 2)
			*piMsgType = EMVMSG_PASSPORT; // "����"
		else if(*pucHolderIdType == 3)
			*piMsgType = EMVMSG_ARRIVAL_CARD; // "�뾳֤"
		else if(*pucHolderIdType == 4)
			*piMsgType = EMVMSG_TEMPORARY_IDENTITY_CARD; // "��ʱ���֤"
		else if(*pucHolderIdType == 5)
			*piMsgType = EMVMSG_OTHER; // "����"
		else {
			ASSERT(0);
			return(HXEMV_CORE);
		}
		pszAmountStr[0] = 0; // 
		break;
	default:
		ASSERT(0);
		return(HXEMV_CORE);
	}
	return(HXEMV_OK);
}

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
int iCvm2GetMethod(int *piCvm, int *piBypassFlag, int *piMsgType, int *piMsgType2, uchar *pszAmountStr)
{
	static uchar sCvmResult[3];
	int   iRet;
	int   iNeedSignFlag;

	*piMsgType2 = 0;

	// ���ν��׵�һ�ε���, ׼������
	if(sg_iCvmProcIndex == -1) {
		iRet = iCvm2PrepareCvmList();
		if(iRet == HXEMV_TERMINATE)
			return(iRet);
		if(iRet!=HXEMV_OK && iRet!=HXEMV_NA)
			return(HXEMV_CORE);
		if(iRet == HXEMV_NA) {
			iTlvMakeAddObj(gl_sTlvDbTermVar, TAG_9F34_CVMResult, 3, "\x3F\x00\x00", TLV_CONFLICT_REPLACE);
			goto label_extra_proc;
		}
	}

	// �ж��Ƿ��Ѿ����븽�Ӵ���׶�, ����Ѿ�����, ֱ��ת�����Ӵ���
	if(sg_iECExtProcStatus != 0)
		goto label_extra_proc;

	for(; (sg_iCvmListLen-sg_iCvmProcIndex)>=2; sg_iCvmProcIndex+=2) {
		iEmvSetItemBit(EMV_ITEM_TSI, TSI_01_CVM_PERFORMED, 1); // �ֿ�����֤��ִ��
		memcpy(sCvmResult, &sg_asCvmList[sg_iCvmProcIndex], 2); // get CVRule
		sCvmResult[2] = 0x01; // 1:failed, assume failed first
		iRet = iCvm2DoVerify(sCvmResult, &iNeedSignFlag);
		// ret : 0               : ��֤�ɹ�
		//       -1              : ��ʶ����֤����
		//       -2              : ʶ����֤����, ��֧����֤����
		//       -3              : ʶ����֤����, ֧����֤����, ��֤ʧ��
		//       -4              : ʶ����֤����, ֧����֤����, ��Ҫ����
		//       HXEMV_CARD_OP   : ��������
		//       HXEMV_TERMINATE : ����ܾ�������������ֹ
		//       HXEMV_CORE      : �ڲ�����
		if(iRet == 0) {
			// �ɹ�
			sg_iCvmOKFlag = 1;
			sg_iCvmSupportFlag = 1;
			sg_iCvmNeedSignFlag = iNeedSignFlag;
			break;
		} else if(iRet == -1) {
			// ��ʶ����֤����
			if((sCvmResult[0]&0x40) == 0)
				break; // b7ָʾ�������֤����ʧ��,��Ϊ�ֿ�����֤ʧ��,���ؼ�����
			else
				continue; // b7ָʾ�������֤����ʧ��,������һ��֤����
		} else if(iRet == -2) {
			// ʶ����֤����, ��֧����֤����
			// refer to Emv2008 book3 Figure 9, ����T
			if((sCvmResult[0]&0x3F) == 2) {
				// online pin
				iEmvSetItemBit(EMV_ITEM_TVR, TVR_19_NO_PINPAD, 1);
			} else if((sCvmResult[0]&0x3F)==1 || (sCvmResult[0]&0x3F)==3 || (sCvmResult[0]&0x3F)==4 || (sCvmResult[0]&0x3F)==5) {
				// offline pin required
				if(iEmvTestItemBit(EMV_ITEM_TERM_CAP, TERM_CAP_08_PLAIN_PIN)==0
						&& iEmvTestItemBit(EMV_ITEM_TERM_CAP, TERM_CAP_11_ENC_OFFLINE_PIN)==0
						&& iEmvTestItemBit(EMV_ITEM_TERM_CAP, TERM_CAP_08_PLAIN_PIN)==0) {
					// �ն˲�֧���κ�һ���ѻ�������֤����
					iEmvSetItemBit(EMV_ITEM_TVR, TVR_19_NO_PINPAD, 1);
				}
			}
		} else if(iRet == -3) {
			// iRet == -3, ʶ����֤����, ֧����֤����, ��֤ʧ��
			sg_iCvmSupportFlag = 1;
		} else if(iRet == -4) {
			// ��Ҫ����
			sg_iCvmSupportFlag = 1;
			iRet = iCvm2PrepareInteractData(sCvmResult, piCvm, piBypassFlag, piMsgType, piMsgType2, pszAmountStr);
			if(iRet != HXEMV_OK)
				return(HXEMV_CORE);
#if 0
// 20140814
// �ڳֿ�����֤�׶�, �����������ڼ��, ��Ϊ�������ĳЩ�ؼ�TVRλû����λ
			if(gl_iAppType != 0/*0:�ͼ����*/) {
				// 20140422, ��Է��ͼ����, �����ж��Ƿ���Ծܾ�
				//           �����ͼ����, ������һЩ��������ȥ, ����2cj.002.05 0a, ��˶����ͼ�����򲻹����ж�
				iRet = iEmvIfDenial();
				if(iRet != HXEMV_OK)
					return(iRet); // ����Ѿ��ܾ�, ����֪ͨ
			}
#endif
			return(HXEMV_OK);
		} else {
			// ��������, EMV���̲��ܼ�����ȥ
			return(iRet);
		}
		// iRet == -2, ʶ����֤����, ��֧����֤����
		// or
		// iRet == -3, ʶ����֤����, ֧����֤����, ��֤ʧ��
		// ���ݱ�־�ж��Ƿ������һ��֤����
		if((sCvmResult[0]&0x3F) == 0x00)
			break; // Fail CVM, ��֤ʧ��, �����b7, ���ؼ�����
		if((sCvmResult[0]&0x40) == 0)
			break; // b7ָʾ�������֤����ʧ��,��Ϊ�ֿ�����֤ʧ��,���ؼ�����
	} // for(; (sg_iCvmListLen-sg_iCvmProcIndex)>=2; sg_iCvmProcIndex+=2) {

	// EMV��֤����
	sg_iECExtProcStatus = 1; // ������������CVM��, ׼������EC�������븽�Ӵ���
	// �ֿ�����֤���̽���
	if(sg_iCvmOKFlag == 0) {
		// ��֤ʧ��, refer to Emv2008 book3, Figure 12, ����Y
		if(sg_iCvmSupportFlag == 0) {
			// û��֧�ֵĳֿ�����֤����
			memcpy(sCvmResult, "\x3F\x00\x01", 3);
		}
		iEmvSetItemBit(EMV_ITEM_TVR, TVR_16_CVM_NOT_SUCCESS, 1); // �ֿ�����֤ʧ��
		iEmvSetItemBit(EMV_ITEM_TSI, TSI_01_CVM_PERFORMED, 1); // �ֿ�����֤��ִ��
	}
	iTlvMakeAddObj(gl_sTlvDbTermVar, TAG_9F34_CVMResult, 3, sCvmResult, TLV_CONFLICT_REPLACE);
	sg_iECExtProcStatus = 1; // ������������CVM��, ׼������EC�������븽�Ӵ���
	// ת�븽�Ӵ���

label_extra_proc:
	// sg_asCvmList�б����޺�����֤����������, ��ʱ���Ӽ�����������Ҫ�Ĵ���, 1:EC�������� 2:���ȷ��
	// ���Ӵ���״̬sg_iECExtProcStatus���� 0:δ��ʼ
    //                                     1:���ڴ���EC�������븽�Ӵ��� 2:EC�������븽�Ӵ������
    //                                     3:���ڴ�����ȷ�ϸ��Ӵ��� 4:���ȷ�ϸ��Ӵ������
    //                                     10:���Ӵ������
	if(sg_iECExtProcStatus==0 || sg_iECExtProcStatus==1) {
		// ��Ҫ����EC�������븽�Ӵ���
		if(iEmvIfNeedECOnlinePin()==HXEMV_OK && sg_iOnlinePinBypassFlag==0) {
			// ��Ҫ����EC�������� ���� ��������û�б�bypass��
			// Ҫ��������������
			*piCvm = HXCVM_CIPHERED_ONLINE_PIN;
			if(iEmvTermEnvironment() == TERM_ENV_ATTENDED)
				*piBypassFlag = 1; // ����bypass
			else
				*piBypassFlag = 0; // ������bypass
			*piMsgType = EMVMSG_ENTER_PIN_ONLINE;
			iRet = iMakeAmountStr(pszAmountStr);
			if(iRet <= 0)
				pszAmountStr[0] = 0; // �������Ϊ0, ����ʾ���
			sg_iECExtProcStatus = 1; // 1:���ڴ���EC�������븽�Ӵ���
			return(HXEMV_OK);
		} else {
			// ��Ҫ������EC�������� ���� �������뱻bypass��, ����Ҫ��������������
			sg_iECExtProcStatus = 2; // ת��--> 2:EC�������븽�Ӵ������
		}
	}

	if(sg_iECExtProcStatus==2 || sg_iECExtProcStatus==3) {
		// ��Ҫ���н��ȷ�ϸ��Ӵ���
		if(sg_iConfirmAmountFlag == 0) {
			// ���û�б�ȷ�Ϲ�, ���н��ȷ�ϸ��Ӵ���
			*piCvm = HXCVM_CONFIRM_AMOUNT;
			*piBypassFlag = 1; // ����bypass
			*piMsgType = EMVMSG_AMOUNT_CONFIRM;
			iRet = iMakeAmountStr(pszAmountStr);
			ASSERT(iRet >= 0);
			if(iRet < 0)
				pszAmountStr[0] = 0; // ����, ����ʾ���
			sg_iECExtProcStatus = 3; // 3:���ڴ�����ȷ�ϸ��Ӵ���
			return(HXEMV_OK);
		} else {
			// ��ȷ�Ϲ�, ����Ҫ����ȷ��
			sg_iECExtProcStatus = 4; // ת��--> 4:���ȷ�ϸ��Ӵ������
		}
	}

	if(sg_iECExtProcStatus >= 4) {
		// ��������������Ӵ���, �ڴ����
		sg_iECExtProcStatus =10;
	}

	if(sg_iECExtProcStatus != 10) {
		ASSERT(0);
		return(HXEMV_CORE);
	}

	if(gl_iAppType != 0/*0:�ͼ����*/) {
		// 20140422, ��Է��ͼ����, �����ж��Ƿ���Ծܾ�
		//           �����ͼ����, ������һЩ��������ȥ, ����2cj.002.05 0a, ��˶����ͼ�����򲻹����ж�
		iRet = iEmvIfDenial();
		if(iRet != HXEMV_OK)
			return(iRet); // ����Ѿ��ܾ�, ����֪ͨ
	}

	return(HXEMV_NO_DATA);
}

// ִ�гֿ�����֤
// in  : psCvmProc           : ��֤��������ʽ, HXCVM_PROC_OK or HXCVM_BYPASS or HXCVM_FAIL or HXCVM_CANCEL or HXCVM_TIMEOUT
//       psCvmData           : ���������, ���Ϊ��������, ����β��Ҫ��0
// out : piMsgType           : ����HXEMV_OKʱʹ��
//                             �����Ϊ0, ����Ҫ��ʾ��������Ϣ
//       piMsgType2          : ����HXEMV_OKʱʹ��
//                             �����Ϊ0, ����Ҫ��ʾ��������Ϣ
// ret : HXEMV_OK            : OK, ��Ҫ�������гֿ�����֤, ��������iHxGetCvmMethod()
//       HXEMV_PARA  		   : ��������
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
int iCvm2DoMethod(int iCvmProc, uchar *psCvmData, int *piMsgType, int *piMsgType2)
{
	int   iRet;
	uint  uiRet;
	uchar *p;
	uchar szBuf[32], sBuf[16];
	uchar sCvmResult[3];

	*piMsgType = EMVMSG_00_NULL;
	*piMsgType2 = EMVMSG_00_NULL;
	if(sg_iCvmProcIndex==-1 || sg_iECExtProcStatus==10)
		return(HXEMV_FLOW_ERROR);

	if(iCvmProc == HXCVM_CANCEL)
		return(HXEMV_CANCEL);
	if(iCvmProc == HXCVM_TIMEOUT)
		return(HXEMV_TIMEOUT);

	if(sg_iECExtProcStatus == 0) {
		// ���ڴ���CVM�б���׶�
		memcpy(sCvmResult, &sg_asCvmList[sg_iCvmProcIndex], 2); // ��ǰ�����CVM
		sCvmResult[2] = 1; // 1:failed, assume failed first
		switch(sCvmResult[0] & 0x3F) {
		case 1: // Plaintext PIN verification performed by ICC
		case 3: // Plaintext PIN verification performed by ICC and signature
			sg_iConfirmAmountFlag = 1;
			if(iCvmProc == HXCVM_BYPASS) {
				iEmvSetItemBit(EMV_ITEM_TVR, TVR_20_PIN_NOT_ENTERED, 1); // add by yujun 20140320, ����Bug
				                                                         // wlfxy, ���CVM����2��������������֤, ����������֤�ֿ���û��bypass, ��λ�������?
				sg_iOfflinePinBypassFlag = 1;
				// ���pin bypass����
				iRet = iTlvGetObjValue(gl_sTlvDbTermFixed, TAG_DFXX_PinBypassBehavior, &p); // PIN bypass���� 0:ÿ��bypassֻ��ʾ�ô�bypass 1:һ��bypass,��������Ϊbypass
				if(iRet == 1) {
					if(*p == 1) // 1:һ��bypass,����Pin����Ϊbypass
						sg_iOnlinePinBypassFlag = 1; // ͬʱ��������Pin bypass��־
				}
			}
			if(iCvmProc == HXCVM_PROC_OK) {
				// ��������, ׼����֤
				// make plain pin block
				if(strlen(psCvmData)<4 || strlen(psCvmData)>12) {
					ASSERT(0);
					return(HXEMV_PARA);
				}
				memset(szBuf, 'F', 16);
				szBuf[0] = '2';
				szBuf[1] = strlen(psCvmData);
				memcpy(szBuf+2, psCvmData, strlen(psCvmData));
				vTwoOne(szBuf, 16, sBuf);
				uiRet = uiEmvCmdVerifyPin(0x80/*P2:Plain Pin*/, 8, sBuf);
				if(uiRet == 1)
					return(HXEMV_CARD_OP);
				if(uiRet && (uiRet&0xFFF0)!=0x63C0 && uiRet!=0x6983 && uiRet!=0x6984)
					return(HXEMV_CARD_SW);
				if((uiRet&0xFFF0) == 0x63C0) {
					sg_iPinTryLeft = uiRet & 0x000F; // ����ʣ�ೢ�Դ���
				}
				if((uiRet&0xFFF0)==0x63C0 && (uiRet&0x000F)>0) {
					// ������֤ʧ��, ����ʣ����֤����
					*piMsgType = EMVMSG_0A_INCORRECT_PIN;
					*piMsgType2 = EMVMSG_13_TRY_AGAIN;
					return(HXEMV_OK);
				}
				if(uiRet == 0) {
					*piMsgType = EMVMSG_0D_PIN_OK;
					sCvmResult[2] = 2; // ok
					sg_iCvmOKFlag = 1;
					sg_iECExtProcStatus = 1; // ������������CVM��, ׼������EC�������븽�Ӵ���
					if((sCvmResult[0]&0x3F) == 3) {
						sCvmResult[2] = 0; // �ѻ�������֤�ɹ�, ����Ҫǩ��, ���óֿ�����֤���Ϊδ֪
						sg_iCvmNeedSignFlag = 1; // ��Ҫǩ��
					}
					iTlvMakeAddObj(gl_sTlvDbTermVar, TAG_9F34_CVMResult, 3, sCvmResult, TLV_CONFLICT_REPLACE);
					return(HXEMV_OK); // �������и��Ӵ���
				}
				// ��֤ʧ��
				sg_iOfflinePinBlockFlag = 1; // �ѻ���������
				iEmvSetItemBit(EMV_ITEM_TVR, TVR_18_EXCEEDS_PIN_TRY_LIMIT, 1);
				*piMsgType = EMVMSG_0A_INCORRECT_PIN;
			}
			// û�������� �� ������֤ʧ��
			sCvmResult[2] = 1; // fail
			iTlvMakeAddObj(gl_sTlvDbTermVar, TAG_9F34_CVMResult, 3, sCvmResult, TLV_CONFLICT_REPLACE);
			if((sCvmResult[0]&0x40) == 0) {
				// b7ָʾ�������֤����ʧ��,��Ϊ�ֿ�����֤ʧ��,���ؼ�����
				iEmvSetItemBit(EMV_ITEM_TVR, TVR_16_CVM_NOT_SUCCESS, 1); // �ֿ�����֤ʧ��, add by yujun 20140320, �޸�Bug
				sg_iECExtProcStatus = 1; // ������������CVM��, ׼������EC�������븽�Ӵ���
			} else {
				sg_iCvmProcIndex += 2; // ���˵���CVM
			}
#if 0
// 20140814
// �ڳֿ�����֤�׶�, �����������ڼ��, ��Ϊ�������ĳЩ�ؼ�TVRλû����λ
			if(gl_iAppType != 0/*0:�ͼ����*/) {
				// 20140422, ��Է��ͼ����, �����ж��Ƿ���Ծܾ�
				//           �����ͼ����, ������һЩ��������ȥ, ����2cj.002.05 0a, ��˶����ͼ�����򲻹����ж�
				iRet = iEmvIfDenial();
				if(iRet != HXEMV_OK)
					return(iRet); // ����Ѿ��ܾ�, ����֪ͨ
			}
#endif
			return(HXEMV_OK); // ������һ��֤���� �� ���и��Ӵ���
		case 2: // Enciphered PIN verified online
			sg_iConfirmAmountFlag = 1;
			if(iCvmProc == HXCVM_BYPASS) {
				iEmvSetItemBit(EMV_ITEM_TVR, TVR_20_PIN_NOT_ENTERED, 1); // add by yujun 20140320, ����Bug
				                                                         // wlfxy, ���CVM����2��������������֤, ����������֤�ֿ���û��bypass, ��λ�������?
				sg_iOnlinePinBypassFlag = 1;
				// ���pin bypass����
				iRet = iTlvGetObjValue(gl_sTlvDbTermFixed, TAG_DFXX_PinBypassBehavior, &p); // PIN bypass���� 0:ÿ��bypassֻ��ʾ�ô�bypass 1:һ��bypass,��������Ϊbypass
				if(iRet == 1) {
					if(*p == 1) // 1:һ��bypass,����Pin����Ϊbypass
						sg_iOfflinePinBypassFlag = 1; // ͬʱ�����ѻ�Pin bypass��־
				}
			}
			if(iCvmProc == HXCVM_PROC_OK) {
				// ��������
				sCvmResult[2] = 0; // unknown
				iTlvMakeAddObj(gl_sTlvDbTermVar, TAG_9F34_CVMResult, 3, sCvmResult, TLV_CONFLICT_REPLACE);
				sg_iCvmOKFlag = 1;
				sg_iECExtProcStatus = 1; // ������������CVM��, ׼������EC�������븽�Ӵ���
				iEmvSetItemBit(EMV_ITEM_TVR, TVR_21_ONLINE_PIN_ENTERED, 1);
				iTlvMakeAddObj(gl_sTlvDbTermVar, TAG_DFXX_EncOnlinePIN, 8, psCvmData, TLV_CONFLICT_REPLACE);
				return(HXEMV_OK); // �������и��Ӵ���
			}
			// û���������ʧ��
			sCvmResult[2] = 1; // fail
			iTlvMakeAddObj(gl_sTlvDbTermVar, TAG_9F34_CVMResult, 3, sCvmResult, TLV_CONFLICT_REPLACE);
			if((sCvmResult[0]&0x40) == 0) {
				// b7ָʾ�������֤����ʧ��,��Ϊ�ֿ�����֤ʧ��,���ؼ�����
				iEmvSetItemBit(EMV_ITEM_TVR, TVR_16_CVM_NOT_SUCCESS, 1); // �ֿ�����֤ʧ��, add by yujun 20140320, �޸�Bug
				sg_iECExtProcStatus = 1; // ������������CVM��, ׼������EC�������븽�Ӵ���
			} else {
				sg_iCvmProcIndex += 2; // ���˵���CVM
			}
#if 0
// 20140814
// �ڳֿ�����֤�׶�, �����������ڼ��, ��Ϊ�������ĳЩ�ؼ�TVRλû����λ
			if(gl_iAppType != 0/*0:�ͼ����*/) {
				// 20140422, ��Է��ͼ����, �����ж��Ƿ���Ծܾ�
				//           �����ͼ����, ������һЩ��������ȥ, ����2cj.002.05 0a, ��˶����ͼ�����򲻹����ж�
				iRet = iEmvIfDenial();
				if(iRet != HXEMV_OK)
					return(iRet); // ����Ѿ��ܾ�, ����֪ͨ
			}
#endif
			return(HXEMV_OK); // ������һ��֤���� �� ���и��Ӵ���
		case 0x20: // Pboc2.0 �ֿ���֤����֤
			if(iCvmProc == HXCVM_PROC_OK) {
				// ȷ��֤��
				sCvmResult[2] = 2; // OK
				iTlvMakeAddObj(gl_sTlvDbTermVar, TAG_9F34_CVMResult, 3, sCvmResult, TLV_CONFLICT_REPLACE);
				sg_iCvmOKFlag = 1;
				sg_iECExtProcStatus = 1; // ������������CVM��, ׼������EC�������븽�Ӵ���
				return(HXEMV_OK); // �������и��Ӵ���
			}
			// Bypass��ʧ��
			sCvmResult[2] = 1; // fail
			iTlvMakeAddObj(gl_sTlvDbTermVar, TAG_9F34_CVMResult, 3, sCvmResult, TLV_CONFLICT_REPLACE);
			if((sCvmResult[0]&0x40) == 0) {
				// b7ָʾ�������֤����ʧ��,��Ϊ�ֿ�����֤ʧ��,���ؼ�����
				sg_iECExtProcStatus = 1; // ������������CVM��, ׼������EC�������븽�Ӵ���
				iEmvSetItemBit(EMV_ITEM_TVR, TVR_16_CVM_NOT_SUCCESS, 1); // �ֿ�����֤ʧ��, add by yujun 20140320, �޸�Bug
			} else {
				sg_iCvmProcIndex += 2; // ���˵���CVM
			}
#if 0
// 20140814
// �ڳֿ�����֤�׶�, �����������ڼ��, ��Ϊ�������ĳЩ�ؼ�TVRλû����λ
			if(gl_iAppType != 0/*0:�ͼ����*/) {
				// 20140422, ��Է��ͼ����, �����ж��Ƿ���Ծܾ�
				//           �����ͼ����, ������һЩ��������ȥ, ����2cj.002.05 0a, ��˶����ͼ�����򲻹����ж�
				iRet = iEmvIfDenial();
				if(iRet != HXEMV_OK)
					return(iRet); // ����Ѿ��ܾ�, ����֪ͨ
			}
#endif
			return(HXEMV_OK); // ������һ��֤���� �� ���и��Ӵ���
		default:
			ASSERT(0);
			return(HXEMV_CORE);
		} // switch(sCvmResult[0] & 0x3F) {
	} else {
		// ���ڸ��Ӵ���׶�
		// ���Ӵ���״̬sg_iECExtProcStatus���� 0:δ��ʼ
		//                                     1:���ڴ���EC�������븽�Ӵ��� 2:EC�������븽�Ӵ������
	    //                                     3:���ڴ�����ȷ�ϸ��Ӵ��� 4:���ȷ�ϸ��Ӵ������
		//                                     10:���Ӵ������
		switch(sg_iECExtProcStatus) {
		case 1: // 1:���ڴ���EC�������븽�Ӵ���
			sg_iConfirmAmountFlag = 1;
			if(iCvmProc == HXCVM_PROC_OK) {
				iEmvSetItemBit(EMV_ITEM_TVR, TVR_21_ONLINE_PIN_ENTERED, 1);
				iTlvMakeAddObj(gl_sTlvDbTermVar, TAG_DFXX_EncOnlinePIN, 8, psCvmData, TLV_CONFLICT_REPLACE);
			}
			sg_iECExtProcStatus = 2; // 2:EC�������븽�Ӵ������
			return(HXEMV_OK); // �������и��Ӵ���
		case 3: // 3:���ڴ�����ȷ�ϸ��Ӵ���
			sg_iConfirmAmountFlag = 1;
			if(iCvmProc!=HXCVM_BYPASS && iCvmProc!=HXCVM_PROC_OK)
				return(HXEMV_CANCEL); // ����bypass��ȷ����, ����Ϊ��ȡ��
			sg_iECExtProcStatus = 4; // 4:���ȷ�ϸ��Ӵ������
			return(HXEMV_OK); // �������и��Ӵ���
		default:
			ASSERT(0);
			return(HXEMV_CORE);
		}
	}
#if 0
// 20140814
// �ڳֿ�����֤�׶�, �����������ڼ��, ��Ϊ�������ĳЩ�ؼ�TVRλû����λ
	if(gl_iAppType != 0/*0:�ͼ����*/) {
		// 20140422, ��Է��ͼ����, �����ж��Ƿ���Ծܾ�
		//           �����ͼ����, ������һЩ��������ȥ, ����2cj.002.05 0a, ��˶����ͼ�����򲻹����ж�
		iRet = iEmvIfDenial();
		if(iRet != HXEMV_OK)
			return(iRet); // ����Ѿ��ܾ�, ����֪ͨ
	}
#endif
	return(HXEMV_OK);
}

// ��ȡCVM��֤�����費��Ҫǩ��
// out : piNeedSignFlag    : �������HXEMV_OK����ʾ��Ҫǩ�ֱ�־��0:����Ҫǩ�� 1:��Ҫǩ��
// ret : HXEMV_OK          : OK
int iCvm2GetSignFlag(int *piNeedSignFlag)
{
	*piNeedSignFlag = sg_iCvmNeedSignFlag;
	return(HXEMV_OK);
}
