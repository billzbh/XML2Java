/**************************************
File name     : EmvCvm.c
Function      : Emv�ֿ�����֤ģ��(�ص��ӿ�)
Author        : Yu Jun
First edition : Apr 25th, 2012
Modified      : Aug 14th, 2014
				    ����iEmvDoCardHolderVerify(), �����жϺ����Ƿ�֧����һ���ѻ�������֤����ʱδ�ж��ѻ�����PIN��֤
					����iCvmCheckCondition(), ԭ��ֻ�ж���CVM������, δ�ж��ն�֧�����
**************************************/
/*
ģ����ϸ����:
	�ص���ʽ�ֿ�����֤
.	֧�ֵĳֿ�����֤����
	�ѻ��������롢�����������롢�ֿ���ǩ�֡�NO CVM REQUIRED��FAIL CVM���ֿ���֤��
.	֧��Bypass PIN
.	֧�ֽ��ȷ��
	����������Ϊ����Ѿ��õ��ֿ���ȷ��
	����ֿ�����֤����ǰ��û�õ�ȷ��, ��Ҫ��ֿ���ȷ�Ͻ��
	sg_iConfirmAmountFlag���ڼ�¼���ȷ�ϱ�־
.	EC��������֧��
	����ж�ΪEC��������, ��û���������������, Ҫ��������������
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

static int sg_iOnlinePinBypassFlag;  // ��������bypass��־, ���ڼ�¼��������������bypass��, 0:û��bypass 1:bypass��
static int sg_iOfflinePinBypassFlag; // �ѻ�����bypass��־, ���ڼ�¼�ѻ�����������bypass��, 0:û��bypass 1:bypass��
static int sg_iOfflinePinBlockFlag;  // �ѻ�����������־, 0:δ֪ 1:����
static int sg_iConfirmAmountFlag;    // ���ȷ�ϱ�־, ���ڼ�¼����Ƿ�ȷ�Ϲ�, 0:û�� 1:��

// ��֤�ѻ���������
// ret : 0               : OK
//       -1              : bypass or ��֤ʧ��
//       HXEMV_CANCEL    : �û�ȡ��
//       HXEMV_TIMEOUT   : ��ʱ
//       HXEMV_CARD_OP   : ��������
//       HXEMV_CARD_SW   : �Ƿ���״̬��
//       HXEMV_TERMINATE : ����ܾ�������������ֹ
//       HXEMV_CORE      : �ڲ�����
// Note: refer to Emv2008 book3, figure11, P108, ����ͼX
static int iCvmVerifyOfflinePlainPin(void)
{
	int   iRet;
	uint  uiRet;
	uchar ucLen, *p, *psPan;
	uchar szPan[21];
	uchar szPin[12+1];
	uchar sBuf[16], sBuf2[8];
	int   iPinTryCounter = -1; // >=0:���Դ��� <0:û�ж���

	if(sg_iOnlinePinBypassFlag || sg_iOfflinePinBypassFlag) {
		iRet = iTlvGetObjValue(gl_sTlvDbTermFixed, TAG_DFXX_PinBypassBehavior, &p); // PIN bypass���� 0:ÿ��bypassֻ��ʾ�ô�bypass 1:һ��bypass,��������Ϊbypass
		ASSERT(iRet == 1);
		if(iRet != 1)
			return(HXEMV_CORE);
		if(*p == 1)
			return(-1); // ���뱻bypass��, ��Ϊ������bypass
	}
	if(sg_iOfflinePinBypassFlag)
		return(-1); // �ѻ����뱻bypass��, ��Ϊ�����ѻ������붼bypass
	if(sg_iOfflinePinBlockFlag)
		return(-1); // �ѻ���������

	uiRet = uiEmvCmdGetData(TAG_9F17_PINTryCounter, &ucLen, sBuf);
	if(uiRet == 1)
		return(HXEMV_CARD_OP);
	if(uiRet == 0) {
		iRet = iTlvCheckTlvObject(sBuf);
		if(iRet == ucLen) {
			iRet = iTlvValue(sBuf, &p);
			if(iRet == 1) {// ������ʣ�����볢�Դ���
				iPinTryCounter = *p;
				if(iPinTryCounter == 0) {
					// ���볢�Դ����ѵ�
					iEmvSetItemBit(EMV_ITEM_TVR, TVR_18_EXCEEDS_PIN_TRY_LIMIT, 1);
					sg_iOfflinePinBlockFlag = 1; // �ѻ���������
					return(-1); // ��֤ʧ��
				}
			}
		}
	}

	while(1) {
		iRet = iTlvGetObjValue(gl_sTlvDbCard, TAG_5A_PAN, &psPan);
		ASSERT(iRet>0 && iRet<=10);
		if(iRet<0 || iRet>10)
			return(HXEMV_CORE); // PAN�������
		vOneTwo0(psPan, iRet, szPan);
		vTrimTailF(szPan);
		sg_iConfirmAmountFlag = 1; // ��������ͬʱ��Ϊ���ȷ��
		if(iPinTryCounter == 1)
			iEmvIOPinpadDispMsg(EMVMSG_LAST_PIN, EMVIO_NEED_NO_CONFIRM); // ���һ����������
		if(iEmvTermEnvironment() == TERM_ENV_ATTENDED)
			iRet = iEmvIOGetPin(EMVIO_PLAIN_PIN, 1, szPin, szPan); // attended, ����bypass
		else
			iRet = iEmvIOGetPin(EMVIO_PLAIN_PIN, 0, szPin, szPan); // ��attended, ������bypass
		switch(iRet) {
		case EMVIO_OK: // OK
			break;
		case EMVIO_CANCEL: // ȡ��
			return(HXEMV_CANCEL);
		case EMVIO_BYPASS: // bypass���Թ�
			iEmvSetItemBit(EMV_ITEM_TVR, TVR_20_PIN_NOT_ENTERED, 1);
			sg_iOfflinePinBypassFlag = 1; // �ѻ�����bypass
			return(-1);
		case EMVIO_TIMEOUT: // ��ʱ
			return(HXEMV_TIMEOUT);
		default:
			iEmvSetItemBit(EMV_ITEM_TVR, TVR_19_NO_PINPAD, 1);
			return(-1);
		} // switch
		// �����������

		// make plain pin block
		memset(sBuf, 'F', 16);
		sBuf[0] = '2';
		sBuf[1] = strlen(szPin);
		memcpy(sBuf+2, szPin, strlen(szPin));
		vTwoOne(sBuf, 16, sBuf2);
		uiRet = uiEmvCmdVerifyPin(0x80/*P2:Plain Pin*/, 8, sBuf2);
		if(uiRet == 1)
			return(HXEMV_CARD_OP);
		if(uiRet && (uiRet&0xFFF0)!=0x63C0 && uiRet!=0x6983 && uiRet!=0x6984)
			return(HXEMV_CARD_SW);
		if((uiRet&0xFFF0)==0x63C0 && (uiRet&0x000F)>0) {
			// ������֤ʧ��, ����ʣ����֤����
			iEmvIOPinpadDispMsg(EMVMSG_0A_INCORRECT_PIN, EMVIO_NEED_NO_CONFIRM);
			iEmvIOPinpadDispMsg(EMVMSG_13_TRY_AGAIN, EMVIO_NEED_NO_CONFIRM);
			iPinTryCounter = uiRet & 0x000F;
			continue;
		}
		if(uiRet == 0)
			break; // ��֤�ɹ�
		sg_iOfflinePinBlockFlag = 1; // �ѻ���������
		iEmvIOPinpadDispMsg(EMVMSG_0A_INCORRECT_PIN, EMVIO_NEED_NO_CONFIRM);
		iEmvSetItemBit(EMV_ITEM_TVR, TVR_18_EXCEEDS_PIN_TRY_LIMIT, 1);
		return(-1);
	} // while(1
	
	iEmvIOPinpadDispMsg(EMVMSG_0D_PIN_OK, EMVIO_NEED_NO_CONFIRM);
	return(0); // ��֤�ɹ�
}

// ��֤�ֿ���֤��
// ret : 0               : OK
//       -1              : bypass or ��֤ʧ��
//       HXEMV_CANCEL    : �û�ȡ��
//       HXEMV_TIMEOUT   : ��ʱ
//       HXEMV_CORE      : �ڲ�����
// 2013.2.19����
static int iCvmVerifyHolderId(void)
{
	int   iRet;
	uchar *pucHolderIdType, *psHolderId;  // �ֿ���֤������, �ֿ���֤������
	uchar szHolderId[41]; // �ֿ���֤������

	iRet = iTlvGetObjValue(gl_sTlvDbCard, TAG_9F62_HolderIdType, &pucHolderIdType);
	if(iRet <= 0) {
		// ����ȱʧ
		iEmvSetItemBit(EMV_ITEM_TVR, TVR_02_ICC_DATA_MISSING, 1); // data missing
		return(-1); // ��Ϊ��֤ʧ��
	}
	iRet = iTlvGetObjValue(gl_sTlvDbCard, TAG_9F61_HolderId, &psHolderId);
	if(iRet <= 0) {
		// ����ȱʧ
		iEmvSetItemBit(EMV_ITEM_TVR, TVR_02_ICC_DATA_MISSING, 1); // data missing
		return(-1); // ��Ϊ��֤ʧ��
	}
	vMemcpy0(szHolderId, psHolderId, iRet);

	iRet = iEmvIOVerifyHolderId(1/*����bypass*/, *pucHolderIdType, szHolderId);
	switch(iRet) {
	case EMVIO_OK: // OK
		break;
	case EMVIO_CANCEL: // ȡ��
		return(HXEMV_CANCEL);
	case EMVIO_BYPASS: // bypass���Թ�
		return(-1);
	case EMVIO_TIMEOUT: // ��ʱ
		return(HXEMV_TIMEOUT);
	default:
		ASSERT(0);
		return(HXEMV_CORE);
	} // switch
	
	return(0); // ��֤�ɹ�
}

// ������������
// ret : 0              : OK
//       HXEMV_NA       : bypass
//       HXEMV_CANCEL   : �û�ȡ��
//       HXEMV_TIMEOUT  : ��ʱ
//       HXEMV_CORE     : �ڲ�����
// Note: refer to Emv2008 book3, figure10, P107, ����ͼU
int iCvmEnterOnlinePin(void)
{
	int   iRet;
	uchar *p;
	uchar sPin[8], *psPan;
	uchar szPan[21];

	if(sg_iOnlinePinBypassFlag || sg_iOfflinePinBypassFlag) {
		iRet = iTlvGetObjValue(gl_sTlvDbTermFixed, TAG_DFXX_PinBypassBehavior, &p); // PIN bypass���� 0:ÿ��bypassֻ��ʾ�ô�bypass 1:һ��bypass,��������Ϊbypass
		ASSERT(iRet == 1);
		if(iRet != 1)
			return(HXEMV_CORE);
		if(*p == 1)
			return(HXEMV_NA); // ���뱻bypass��, ��Ϊ������bypass
	}
	if(sg_iOnlinePinBypassFlag)
		return(HXEMV_NA); // �������뱻bypass��, ��Ϊ���������������붼bypass

	iRet = iTlvGetObjValue(gl_sTlvDbCard, TAG_5A_PAN, &psPan);
	ASSERT(iRet>0 && iRet<=10);
	if(iRet<0 || iRet>10)
		return(HXEMV_CORE); // PAN�������
	vOneTwo0(psPan, iRet, szPan);
	vTrimTailF(szPan);
	sg_iConfirmAmountFlag = 1; // ��������ͬʱ��Ϊ���ȷ��
	if(iEmvTermEnvironment() == TERM_ENV_ATTENDED)
		iRet = iEmvIOGetPin(EMVIO_CIPHERED_PIN, 1, sPin, szPan); // attended, ����bypass
	else
		iRet = iEmvIOGetPin(EMVIO_CIPHERED_PIN, 0, sPin, szPan); // ��attended, ������bypass
	switch(iRet) {
	case EMVIO_OK: // OK
		break;
	case EMVIO_CANCEL: // ȡ��
		return(HXEMV_CANCEL);
	case EMVIO_BYPASS: // bypass���Թ�
		iEmvSetItemBit(EMV_ITEM_TVR, TVR_20_PIN_NOT_ENTERED, 1);
		sg_iOnlinePinBypassFlag = 1; // ��������bypass
		return(HXEMV_NA);
	case EMVIO_TIMEOUT: // ��ʱ
		return(HXEMV_TIMEOUT);
	default:
		iEmvSetItemBit(EMV_ITEM_TVR, TVR_19_NO_PINPAD, 1);
		return(HXEMV_NA);
	} // switch
	// �����������

	iRet = iTlvMakeAddObj(gl_sTlvDbTermVar, TAG_DFXX_EncOnlinePIN, 8, sPin, TLV_CONFLICT_REPLACE);
	ASSERT(iRet > 0);
	if(iRet < 0)
		return(HXEMV_CORE);
	return(0); // �������
}

// ���ֿ�����֤���������Ƿ�����
// in  : psCvRule        : �ֿ�����֤����
//       psCvmList       : CVM List, ������ȡX��Y���
// ret : 0               : ����
//       HXEMV_NA        : ������
//       HXEMV_CORE      : �ڲ�����
int iCvmCheckCondition(uchar *psCvRule, uchar *psCvmList)
{
	int   iRet;
	uint  uiTransType;
	uchar *psAppCurrencyCode, *psTransCurrencyCode;
	uchar *psTransType, *pucOnlinePinSupport;
	uchar szAmountX[13], szAmountY[13]; // CVM list���
	uchar *psAmount, szAmount[13]; // ������Ȩ���
	uchar ucCurrencyMatchFlag; // Ӧ�û��Ҵ����뽻�׻��Ҵ���ƥ���־, 0:��ƥ�� 1:ƥ��

	ucCurrencyMatchFlag = 0;
	// �ж�Ӧ�û��Ҵ���(T9F42)�뽻�׻��Ҵ���(T5F2A)�Ƿ����
	iRet = iTlvGetObjValue(gl_sTlvDbTermVar, TAG_5F2A_TransCurrencyCode, &psTransCurrencyCode);
	ASSERT(iRet == 2);
	if(iRet != 2)
		return(HXEMV_CORE); // ���׻��Ҵ���������
	iRet = iTlvGetObjValue(gl_sTlvDbCard, TAG_9F42_AppCurrencyCode, &psAppCurrencyCode);
	if(iRet == 2) {
		// ���׻��Ҵ�������Ҹ�ʽ��ȷ
		if(memcmp(psTransCurrencyCode, psAppCurrencyCode, 2) == 0)
			ucCurrencyMatchFlag = 1; // Ӧ�û��Ҵ����뽻�׻��Ҵ���ƥ��
	} else if(iRet > 0)
		return(HXEMV_CORE); // Ӧ�û��Ҵ����ʽ����
	if(ucCurrencyMatchFlag) {
		// ȡ�����X����Y
		sprintf(szAmountX, "%lu", ulStrToLong(psCvmList, 4));
		sprintf(szAmountY, "%lu", ulStrToLong(psCvmList+4, 4));
	}

	iRet = iTlvGetObjValue(gl_sTlvDbTermVar, TAG_DFXX_TransType, &psTransType); // ȡ�ý�������
	ASSERT(iRet == 2);
	if(iRet != 2)
		return(HXEMV_CORE); // �������ͱ������
	uiTransType = ulStrToLong(psTransType, 2);
	iRet = iTlvGetObjValue(gl_sTlvDbTermVar, TAG_9F02_AmountN, &psAmount); // ȡ����Ȩ���
	ASSERT(iRet == 6);
	if(iRet != 6)
		return(HXEMV_CORE); // ��Ȩ���������
	vOneTwo0(psAmount, 6, szAmount);

	switch(psCvRule[1]) {
	// break  -- if terminal supports the CVM
	// return -- ����"if terminal supports the CVM"֮�������, ������ʶ�������
	case 0: // always
		break; // ��������
	case 1: // if unattended cash
		if(uiTransType==TRANS_TYPE_CASH && iEmvTermEnvironment()==TERM_ENV_UNATTENDED)
			break; // ��������
		return(HXEMV_NA); // ����������
	case 2: // if not unattended cash and not manual cash and not purchase with cashback
		if(uiTransType != TRANS_TYPE_CASH)
			break; // ��������
		return(HXEMV_NA); // ����������
	case 3: // if terminal supports the CVM
		break; // break, �Ժ����ж�
	case 4: // if manual cash
		if(uiTransType==TRANS_TYPE_CASH && iEmvTermEnvironment()==TERM_ENV_ATTENDED)
			break; // ��������
		return(HXEMV_NA); // ����������
	case 5: // if purchase with cashback
		return(HXEMV_NA); // �����ݲ�֧��cashback
	case 6: // if transaction is in the application currency and is under X value
		if(ucCurrencyMatchFlag == 0)
			return(HXEMV_NA); // ֻ�н��׻��Ҵ�����Ӧ�û��Ҵ���ƥ��ʱ�ſɱȽϽ��
		if(iStrNumCompare(szAmount, szAmountX, 10) < 0)
			break; // ��������
		return(HXEMV_NA); // ����������
	case 7: // if transaction is in the application currency and is over X value
		if(ucCurrencyMatchFlag == 0)
			return(HXEMV_NA); // ֻ�н��׻��Ҵ�����Ӧ�û��Ҵ���ƥ��ʱ�ſɱȽϽ��
		if(iStrNumCompare(szAmount, szAmountX, 10) > 0)
			break; // ��������
		return(HXEMV_NA); // ����������
	case 8: // if transaction is in the application currency and is under Y value
		if(ucCurrencyMatchFlag == 0)
			return(HXEMV_NA); // ֻ�н��׻��Ҵ�����Ӧ�û��Ҵ���ƥ��ʱ�ſɱȽϽ��
		if(iStrNumCompare(szAmount, szAmountY, 10) < 0)
			break; // ��������
		return(HXEMV_NA); // ����������
	case 9: // if transaction is in the application currency and is over Y value
		if(ucCurrencyMatchFlag == 0)
			return(HXEMV_NA); // ֻ�н��׻��Ҵ�����Ӧ�û��Ҵ���ƥ��ʱ�ſɱȽϽ��
		if(iStrNumCompare(szAmount, szAmountY, 10) > 0)
			break; // ��������
		return(HXEMV_NA); // ����������
	default:
		return(HXEMV_NA); // ��ʶ��, ��Ϊ����������
	} // switch(psCvRule[1]) {


	// if terminal supports the CVM
	switch(psCvRule[0] & 0x3F) { // b1-b6 of the first byte
	// break  -- ��֧����֤����
	// return(0) -- ֧�ִ���֤����
	case 0: // Fail CVM processing
		return(0); // ֧�ָ÷���
	case 1: // Plaintext PIN verification performed by ICC
		if(iEmvTestItemBit(EMV_ITEM_TERM_CAP, TERM_CAP_08_PLAIN_PIN) == 0)
			break; // ��֧�ָ÷���
		return(0); // ֧�ָ÷���
	case 2: // Enciphered PIN verified online
		if(iEmvTestItemBit(EMV_ITEM_TERM_CAP, TERM_CAP_09_ENC_ONLINE_PIN) == 0)
			break; // ��֧�ָ÷���
    	iRet = iTlvGetObjValue(gl_sTlvDbTermVar, TAG_DFXX_OnlinePINSupport, &pucOnlinePinSupport); // ȡ����������֧�����
        if(iRet == 1) {
            if(*pucOnlinePinSupport == 0)
                break; // AID��֧����������(���AID��û�и�����ֵ, ȱʡ��Ϊ֧����������)
        }
		return(0); // ֧�ָ÷���
	case 3: // Plaintext PIN verification performed by ICC and signature
		if(iEmvTestItemBit(EMV_ITEM_TERM_CAP, TERM_CAP_08_PLAIN_PIN) == 0)
			break; // ��֧�ָ÷���
		if(iEmvTestItemBit(EMV_ITEM_TERM_CAP, TERM_CAP_10_SIGNATURE) == 0)
			break; // ��֧�ָ÷���
		return(0); // ֧�ָ÷���
	case 4: // Enciphered PIN verification performed by ICC
		if(iEmvTestItemBit(EMV_ITEM_TERM_CAP, TERM_CAP_11_ENC_OFFLINE_PIN) == 0)
			break; // ��֧�ָ÷���
		return(0); // ֧�ָ÷���
	case 5: // Enciphered PIN verification performed by ICC and signature
		if(iEmvTestItemBit(EMV_ITEM_TERM_CAP, TERM_CAP_11_ENC_OFFLINE_PIN) == 0)
			break; // ��֧�ָ÷���
		if(iEmvTestItemBit(EMV_ITEM_TERM_CAP, TERM_CAP_10_SIGNATURE) == 0)
			break; // ��֧�ָ÷���
		return(0); // ֧�ָ÷���
	case 0x1E: // signature
		if(iEmvTestItemBit(EMV_ITEM_TERM_CAP, TERM_CAP_10_SIGNATURE) == 0)
			break; // ��֧�ָ÷���
		return(0); // ֧�ָ÷���
	case 0x1F: // No CVM required
		if(iEmvTestItemBit(EMV_ITEM_TERM_CAP, TERM_CAP_12_NO_CVM_REQUIRED) == 0)
			break; // ��֧�ָ÷���
		return(0); // ֧�ָ÷���
	case 0x20: // Pboc2.0 �ֿ���֤����֤
		// 2013.2.19����
		if(iEmvTestItemBit(EMV_ITEM_TERM_CAP, TERM_CAP_15_HOLDER_ID) == 0)
			break; // ��֧�ָ÷���
		if(iEmvTermEnvironment() != TERM_ENV_ATTENDED)
			break; // ����ֵ���ն˲�֧�ֳֿ���֤����֤
		return(0); // ֧�ָ÷���
	default:
		break; // ��֧�ָ÷���
	} // switch(psCvRule[0] & 0x3F) { // b1-b6 of the first byte
	return(HXEMV_NA); // ��֧�ָ÷���
}

// ִ�гֿ�����֤����
// in  : psCvmResult     : �±�0-1��ʾ�ֿ�����֤����
// out : psCvmResult     : �±�2��ʾ��֤���
//       piNeedSignFlag  : ����ɹ���ɣ�������Ҫǩ�ֱ�־��0:����Ҫǩ�� 1:��Ҫǩ��
// ret : 0               : ��֤�ɹ�
//       -1              : ��ʶ����֤����
//       -2              : ʶ����֤����, ��֧����֤����
//       -3              : ʶ����֤����, ֧����֤����, ��֤ʧ��
//       HXEMV_CANCEL    : �û�ȡ��
//       HXEMV_TIMEOUT   : ��ʱ
//       HXEMV_CARD_OP   : ��������
//       HXEMV_CARD_SW   : �Ƿ���״̬��
//       HXEMV_TERMINATE : ����ܾ�������������ֹ
//       HXEMV_CORE      : �ڲ�����
static int iCvmDoVerify(uchar *psCvmResult, int *piNeedSignFlag)
{
	int   iRet;
	int   iCvmRecognisedFlag; // CVMʶ���־, 0:��ʶ�� 1:ʶ��
	int   iCvmSupportFlag; // CVM֧�ֱ�־, 0:��֧�� 1:֧��
	int   iCvmPassFlag; // CVMͨ����־, 0:ûͨ�� 1:ͨ��

	iCvmSupportFlag = 0; // ���ȼٶ���֧����֤����
	*piNeedSignFlag = 0; // ���ȼٶ�����Ҫǩ��
	iCvmRecognisedFlag = 1; // ���ȼٶ�ʶ���CVM
	iCvmPassFlag = 0; // �ȼ���CVMû��ͨ��
	switch(psCvmResult[0] & 0x3F) {
	case 0: // Fail CVM processing
		iCvmSupportFlag = 1; // CVM֧�ֱ�־, 0:��֧�� 1:֧��
		psCvmResult[2] = 1;
		break;
	case 1: // Plaintext PIN verification performed by ICC
		if(iEmvTestItemBit(EMV_ITEM_TERM_CAP, TERM_CAP_08_PLAIN_PIN) == 0)
			break; // ��֧�ָ÷���
		iCvmSupportFlag = 1; // CVM֧�ֱ�־, 0:��֧�� 1:֧��
		iRet = iCvmVerifyOfflinePlainPin();
		if(iRet == 0) {
			// ��֤�ɹ�
			iCvmPassFlag = 1;
			psCvmResult[2] = 2; // 0:Unknown 1:Failed 2:Successfull
			break;
		}
		if(iRet == -1) {
			// ��֤ʧ��
			psCvmResult[2] = 1; // 0:Unknown 1:Failed 2:Successfull
			break;
		}
		return(iRet); // ����������
	case 3: // Plaintext PIN verification performed by ICC and signature
		if(iEmvTestItemBit(EMV_ITEM_TERM_CAP, TERM_CAP_08_PLAIN_PIN) == 0)
			break; // ��֧�ָ÷���
		// ��Ҫ����ǩ��
		if(iEmvTestItemBit(EMV_ITEM_TERM_CAP, TERM_CAP_10_SIGNATURE) == 0)
			break; // ��֧�ָ÷���
		*piNeedSignFlag = 1; // ��Ҫǩ��

		iCvmSupportFlag = 1; // CVM֧�ֱ�־, 0:��֧�� 1:֧��
		iRet = iCvmVerifyOfflinePlainPin();
		if(iRet == 0) {
			// ��֤�ɹ�
			iCvmPassFlag = 1;
			psCvmResult[2] = 0; // 0:Unknown 1:Failed 2:Successfull
			break;
		}
		if(iRet == -1) {
			// ��֤ʧ��
			psCvmResult[2] = 1; // 0:Unknown 1:Failed 2:Successfull
			break;
		}
		return(iRet); // ����������
	case 2: // Enciphered PIN verified online
		if(iEmvTestItemBit(EMV_ITEM_TERM_CAP, TERM_CAP_09_ENC_ONLINE_PIN) == 0)
			break; // ��֧�ָ÷���
		iCvmSupportFlag = 1; // CVM֧�ֱ�־, 0:��֧�� 1:֧��
		iRet = iCvmEnterOnlinePin();
		if(iRet == 0) {
			// ����PIN
			iCvmPassFlag = 1;
			psCvmResult[2] = 0; // 0:Unknown 1:Failed 2:Successfull
			iEmvSetItemBit(EMV_ITEM_TVR, TVR_21_ONLINE_PIN_ENTERED, 1);
			break;
		}
		if(iRet == HXEMV_NA) {
			// ��֤ʧ��
			psCvmResult[2] = 1; // 0:Unknown 1:Failed 2:Successfull
			break;
		}
		return(iRet); // ����������
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
		if(iEmvTestItemBit(EMV_ITEM_TERM_CAP, TERM_CAP_15_HOLDER_ID) == 0)
			break; // ��֧�ָ÷���
		if(iEmvTermEnvironment() != TERM_ENV_ATTENDED)
			break; // ����ֵ���ն˲�֧�ֳֿ���֤����֤
		iCvmSupportFlag = 1; // CVM֧�ֱ�־, 0:��֧�� 1:֧��
		iRet = iCvmVerifyHolderId();
		if(iRet == 0) {
			// ��֤�ɹ�
			iCvmPassFlag = 1;
			psCvmResult[2] = 2; // 0:Unknown 1:Failed 2:Successfull
			break;
		}
		if(iRet == -1) {
			// ��֤ʧ��
			psCvmResult[2] = 1; // 0:Unknown 1:Failed 2:Successfull
			break;
		}
		return(iRet); // ����������
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
	if(iCvmPassFlag == 0) {
		// ��֤ʧ��
		return(-3);
	}
	return(0); // ��֤�ɹ�
}

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
static int iEmvDoCardHolderVerify(int *piNeedSignFlag)
{
	int   iRet;
	uchar ucCvmListLen, *psCvmList;
	uchar sCvmResult[3];
	int   iCvmOKFlag; // �ֿ�����֤�ɹ���־, 0:δ�ɹ� 1:�ɹ�
	int   iCvmSupportFlag; // �ֿ�����֤����֧�ֱ�־, 0:û��֧�ֵ���֤���� 1:����֧��һ����֤����
	int   i;

	if(iEmvTestItemBit(EMV_ITEM_AIP, AIP_03_CVM_SUPPORTED) == 0) {
		// AIPָʾ��֧�ֳֿ�����֤
		iTlvMakeAddObj(gl_sTlvDbTermVar, TAG_9F34_CVMResult, 3, "\x3F\x00\x00", TLV_CONFLICT_REPLACE);
		return(HXEMV_OK);
	}
	iRet = iTlvGetObjValue(gl_sTlvDbCard, TAG_8E_CVMList, &psCvmList);
	if(iRet<=0 || iRet==8) {
		// ��CVM list(����Ϊ8��ʾֻ�н��X����Y, ��CVM rule)
		iEmvSetItemBit(EMV_ITEM_TVR, TVR_02_ICC_DATA_MISSING, 1);
		iTlvMakeAddObj(gl_sTlvDbTermVar, TAG_9F34_CVMResult, 3, "\x3F\x00\x00", TLV_CONFLICT_REPLACE);
		return(HXEMV_OK); 
	}
	if(iRet<8 || iRet%2)
		return(HXEMV_TERMINATE); // CVM list��ʽ����
	ucCvmListLen = (uchar)iRet;

	// ִ�гֿ�����֤
	iCvmOKFlag = 0; // �����óֿ�����֤δ�ɹ�
	iCvmSupportFlag = 0; // ���ȼ���û��֧�ֵ���֤����
	sg_iOfflinePinBypassFlag = 0; // ����ѻ�����bypass���
	sg_iOfflinePinBlockFlag = 0;  // ����ѻ������������

	sg_iOnlinePinBypassFlag = 0; // �����������bypass���
	for(i=8; i<(int)ucCvmListLen; i+=2) {
		memcpy(sCvmResult, psCvmList+i, 2); // get CVRule
		sCvmResult[2] = 0x01; // 1:failed, assume failed first
		iRet = iCvmCheckCondition(sCvmResult, psCvmList);
		if(iRet!=0 && iRet!=HXEMV_NA)
			return(iRet); // ���ִ���, ����
		if(iRet == HXEMV_NA)
			continue; // ����������,������һ����֤����
		iRet = iCvmDoVerify(sCvmResult, piNeedSignFlag);
		// ret : 0               : ��֤�ɹ�
		//       -1              : ��ʶ����֤����
		//       -2              : ʶ����֤����, ��֧����֤����
		//       -3              : ʶ����֤����, ֧����֤����, ��֤ʧ��
		//       HXEMV_CANCEL    : �û�ȡ��
		//       HXEMV_TIMEOUT   : ��ʱ
		//       HXEMV_CARD_OP   : ��������
		//       HXEMV_TERMINATE : ����ܾ�������������ֹ
		//       HXEMV_CORE      : �ڲ�����
		if(iRet == 0) {
			// �ɹ�
			iCvmSupportFlag = 1;
			iCvmOKFlag = 1;
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
			iCvmSupportFlag = 1;
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
	} // for(i=8; i<iT8EValueLen; i+=2

	// �ֿ�����֤���̽���
	if(iCvmOKFlag == 0) {
		// ��֤ʧ��, refer to Emv2008 book3, Figure 12, ����Y
		if(iCvmSupportFlag == 0) {
			// û��֧�ֵĳֿ�����֤����
			memcpy(sCvmResult, "\x3F\x00\x01", 3);
		}
		iEmvSetItemBit(EMV_ITEM_TVR, TVR_16_CVM_NOT_SUCCESS, 1); // �ֿ�����֤ʧ��
	}
	iTlvMakeAddObj(gl_sTlvDbTermVar, TAG_9F34_CVMResult, 3, sCvmResult, TLV_CONFLICT_REPLACE);
	iEmvSetItemBit(EMV_ITEM_TSI, TSI_01_CVM_PERFORMED, 1); // �ֿ�����֤��ִ��
	return(HXEMV_OK);
}

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
int iEmvCardHolderVerify(int *piNeedSignFlag)
{
	uint  uiTransType;
	int iRet;
	uchar *psTransType;

	sg_iConfirmAmountFlag = 0; // ���ȼ�����û�б�ȷ�Ϲ�
	iRet = iEmvDoCardHolderVerify(piNeedSignFlag);
	if(iRet)
		return(iRet); // ����, ֱ�ӷ���

	// �ж��Ƿ���Ҫ����EC��Ҫ����������
	if(iEmvIfNeedECOnlinePin() == HXEMV_OK) {
		// ��Ҫ����EC��������
		iRet = iCvmEnterOnlinePin();
		if(iRet==HXEMV_CANCEL || iRet==HXEMV_TIMEOUT || iRet==HXEMV_CORE)
			return(iRet);
	}

	if(sg_iConfirmAmountFlag)
		return(HXEMV_OK); // ����Ѿ���ȷ�Ϲ�, ����
	// ȷ�Ͻ��
	iRet = iTlvGetObjValue(gl_sTlvDbTermVar, TAG_DFXX_TransType, &psTransType); // ȡ�ý�������
	ASSERT(iRet == 2);
	if(iRet != 2)
		return(HXEMV_CORE); // �������ͱ������
	uiTransType = ulStrToLong(psTransType, 2);
	if(uiTransType==TRANS_TYPE_AVAILABLE_INQ || uiTransType==TRANS_TYPE_BALANCE_INQ)
		return(HXEMV_OK); // ���������ѯ, ����ȷ�Ͻ��
	iRet = iEmvIOPinpadDispMsg(EMVMSG_AMOUNT_CONFIRM, EMVIO_NEED_CONFIRM);
	if(iRet == EMVIO_CANCEL)
		return(HXEMV_CANCEL);
	if(iRet == EMVIO_TIMEOUT)
		return(HXEMV_TIMEOUT);
	ASSERT(iRet == 0);
	if(iRet)
		return(HXEMV_CORE);
	return(HXEMV_OK);
}
