/**************************************
File name     : EmvModul.c
Function      : Emv���Ĵ���ģ��
Author        : Yu Jun
First edition : Apr 18th, 2012
Modified      : Apr 1st, 2014
				    ���Ӷ�ATM��֧��
**************************************/
/*
ģ����ϸ����:
	EMV���Ĵ���ģ��
.	GPO
	�������PDOL, ɨ��PDOL, ��������н����, Ҫ��Ӧ�ò��ṩ���
	���ڿ��ܻ���ִ��GPO����, �ñ���sg_sGpoTlvBuf[]�������һ��GPO��Tlv����, ��������ѡ��Ӧ��ʱɾ���ϴ�GPO����
	�ն��Ƿ�֧��EC, ������Tag TAG_DFXX_ECTermSupportIndicator��ʾ��TLV������
		�����������(����,���С���ն�EC�޶��), ����9F7A���ݶ���ָ��֧��EC
.	����ƬӦ�����ݼ�¼
	����������û��9F74���ݶ������ж��Ƿ�ΪEC����
.	�ѻ�������֤
.	��������
.	�ն˷��չ���
.	�ֿ�����֤
.	�ն���Ϊ����
.	1st GAC
.	2nd GAC
.	�ű�����
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "VposFace.h"
#include "PushApdu.h"
#include "Arith.h"
#include "Sha.h"
#include "Common.h"
#include "EmvCmd.h"
#include "TlvFunc.h"
#include "EmvFunc.h"
#include "TagAttr.h"
#include "EmvData.h"
#include "EmvCvm.h"
#include "EmvCore.h"
#include "EmvMsg.h"
#include "EmvPara.h"
#include "EmvSele.h"
#include "EmvIo.h"
#include "EmvDAuth.h"
#include "EmvModul.h"
#include "TagDef.h"
#include "EmvTrace.h"
#include "PbocCtls.h"

uchar gl_sEmv4_1[] = "\x7A\x1B\x01\xDF\x94\xB7\xA9\x29\x79\x3E\xE4\x12\x63\x04\xB4\xA6";
static uchar  sg_sGpoTlvBuf[300];        // ���һ��GPOʱ�����Tlv����, ��������ѡ��Ӧ��ʱɾ���ϴ�GPO����

// GPO��ʼ��, ��ʼ��sg_sGpoTlvBuf
// ret : HXEMV_OK : �ɹ�
int iEmvGpoInit(void)
{
	// ��ʼ��GPO��ʱTlv���ݿ�
	iTlvSetBuffer(sg_sGpoTlvBuf, sizeof(sg_sGpoTlvBuf));
	return(HXEMV_OK);
}
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
int iEmvGpo(void)
{
	int   i;
	int   iRet;
	uint  uiRet;
	int   iLen;
	int   iPdolLen; // PDOL����(<0��ʾ��PDOL)
	uchar szAmount[13];
	uint  uiTransType;
	uchar *p;
	uchar *psPdol;  // PDOL����
	uchar *psTlvObj;
	uchar *psTlvObjValue;
	uchar sPdolBlock[256]; // pdol���ݿ�
	uchar sDataIn[256];    // apduָ������
	uchar ucDataInLen;     // apduָ�����ݳ���
	uchar sDataOut[256];   // apduӦ������
	uchar ucDataOutLen;    // apduӦ�𳤶�
	uchar ucECSupportIndicator; // EC֧�ֱ�־, 0:��֧�� 1:֧��
	uchar szECTransLimit[13];   // EC�޶�
    uchar ucAidLen, *psAid;     // ��¼��ǰѡ�е�AID, ����GPOʧ��ɾ��AID����ѡ��
	uchar ucTransRoute;    // �����ߵ�·��
	uchar *psAip;

	// ��ɾ�����ܵ��ϴ�GPO����
	for(i=0; ; i++) {
		iRet = iTlvGetObjByIndex(sg_sGpoTlvBuf, i, &psTlvObj);
		if(iRet < 0)
			break;
		iTlvDelObj(gl_sTlvDbCard, psTlvObj);
	}
	// ���³�ʼ��GPO��ʱTlv���ݿ�
	iTlvSetBuffer(sg_sGpoTlvBuf, sizeof(sg_sGpoTlvBuf));

	// ���֮ǰ����ѡ��Ӧ�ò�����������,ɾ��,��Ϊ������Ӧ�ú�ֿ��˿���ϣ���ṩ����һ�����
	iTlvDelObj(gl_sTlvDbTermVar, TAG_9F02_AmountN);
	iTlvDelObj(gl_sTlvDbTermVar, TAG_81_AmountB);
	iTlvDelObj(gl_sTlvDbTermVar, TAG_DFXX_PdolDataBlock);

	iPdolLen = iTlvGetObjValue(gl_sTlvDbCard, TAG_9F38_PDOL, &psPdol);
	if(iPdolLen < 0) {
		// ��pdol
		memcpy(sDataIn, "\x83\x00", 2);
		ucDataInLen = 2;
	} else {
		// search amount field, ���ж�Amount,other
        iRet =  iTlvSearchDOLTag(psPdol, iPdolLen, TAG_9F02_AmountN, NULL); // T9F02, Amount(num)
        if(iRet <= 0) {
            iRet = iTlvSearchDOLTag(psPdol, iPdolLen, TAG_81_AmountB, NULL); // T81, Amount(bin)
        }
		if(iRet > 0) {
			// pdol��Ҫ�����
			iRet = iTlvGetObjValue(gl_sTlvDbTermVar, TAG_DFXX_TransType, &psTlvObjValue);
			ASSERT(iRet == 2);
			if(iRet != 2) {
				EMV_TRACE_ERR_MSG
				return(HXEMV_CORE);
			}
			uiTransType = ulStrToLong(psTlvObjValue, 2);
			if(uiTransType==TRANS_TYPE_AVAILABLE_INQ || uiTransType==TRANS_TYPE_BALANCE_INQ)
				strcpy(szAmount, "0"); // ��ѯ����Ҫ������
			else {
				// attended terminal��ʱ��Ҫ������, unattended terminal��0����
				// refer to EMV2008 book4, 6.3.1, P55

				// eCash GPO׼��(֧��eCash��IC����Ȼ��PDOL, ��PDOL������Ҫ���ṩ�����Ҵ��롢eCash֧�ֱ�־)
				// ֧��eCash���ն�Ҳ������unattended����, �����ʱ���ṩ���, ��unattended�����ն˾Ͳ���֧��eCash��, �������ն�֧��eCash, GPOǰ����Ҫ��ȡ�����

				// ����ն�֧��֧��EC
				ucECSupportIndicator = 0; // EC֧�ֱ�־, 0:��֧�� 1:֧��
				iRet = iTlvGetObjValue(gl_sTlvDbTermVar, TAG_DFXX_ECTermSupportIndicator, &p);
				if(iRet == 1) {
					if(*p == 1) {
						if(uiTransType==TRANS_TYPE_SALE || uiTransType==TRANS_TYPE_GOODS || uiTransType==TRANS_TYPE_SERVICES) {
							// �ն�֧��EC, ���������ѽ���
							ucECSupportIndicator = 1; // EC֧�ֱ�־, 0:��֧�� 1:֧��
						}
					}
				}

				if(gl_iCardType==EMV_CONTACTLESS_CARD && iRet>0) {
					// �ǽӿ�����Ҫ���, ȡ��Ԥ������Ľ��
					strcpy(szAmount, pszPbocCtlsGetAmount());
				} else if(iEmvTermEnvironment()==TERM_ENV_ATTENDED || ucECSupportIndicator==1) {
					// ����ֵ��(attended) ���� EC����
					iRet = iEmvIOGetAmount(EMVIO_AMOUNT, szAmount);
					if(iRet == EMVIO_CANCEL)
						return(HXEMV_CANCEL);
					if(iRet == EMVIO_TIMEOUT)
						return(HXEMV_TIMEOUT);
					ASSERT(iRet == 0);
					if(iRet)
						return(HXEMV_CORE);
				} else {
					// ����ֵ��(unattended)
					strcpy(szAmount, "0");
				}
			}
			iRet = iEmvSaveAmount(szAmount); // �����浽tlv���ݿ���
			ASSERT(iRet == 0);
			if(iRet != 0)
				return(HXEMV_CORE);
		}

		// �ж��Ƿ�����EC֧��λ
		if(ucECSupportIndicator == 1) {
			// �ն�֧��EC, ��Ϊ���ѽ���, �������˽��
			// �ȽϽ����EC�޶�, �����Ƿ�������ν���ΪEC����
			iRet = iTlvGetObjValue(gl_sTlvDbTermVar, TAG_9F7B_ECTermTransLimit, &p);
			if(iRet == 6) {
				vOneTwo0(p, 6, szECTransLimit);
			} else {
				// û�ҵ�EC�����޶�, �Ƚ�Floor Limit
				iRet = iTlvGetObjValue(gl_sTlvDbTermVar, TAG_9F1B_TermFloorLimit, &p);
				if(iRet == 4) {
					sprintf(szECTransLimit, "%lu", ulStrToLong(p, 4));
				} else
					ucECSupportIndicator = 0; // ��EC�����޶������ն�����޶�, ��ʹ��EC(ECash�ĵ�7.5)
			}
			if(ucECSupportIndicator == 1) {
				// ����EC�����޶���ն�����޶�
				if(iStrNumCompare(szAmount, szECTransLimit, 10) < 0) {
					// ��Ȩ���С��EC�����޶�, ��־ΪEC����, ��T9F7A��ӵ�TLV���ݿ���
					iRet = iTlvMakeAddObj(gl_sTlvDbTermVar, TAG_9F7A_ECTermSupportIndicator, 1, &ucECSupportIndicator, TLV_CONFLICT_REPLACE);
					ASSERT(iRet >= 0);
					if(iRet < 0)
						return(HXEMV_CORE);
					// ����EC����ָʾλ
					iRet = iEmvSetECTransFlag(1); // 1 : ��EC����
					ASSERT(iRet == 0);
					if(iRet)
						return(HXEMV_CORE);
				}
			}
		}

		// search rand field
        iRet =  iTlvSearchDOLTag(psPdol, iPdolLen, TAG_9F37_TermRand, NULL);
        if(iRet > 0) {
			// �����
			vGetRand(sDataIn, iRet);
			iTlvMakeAddObj(gl_sTlvDbTermVar, TAG_9F37_TermRand, iRet, sDataIn, TLV_CONFLICT_REPLACE);
        }

        // ��¼��ǰAID, �Ա�GPOʧ�ܺ�ɾ��
		iRet = iTlvGetObjValue(gl_sTlvDbCard, TAG_4F_AID, &psAid); // ������ǰѡ�е�AID
		ASSERT(iRet > 0);
		if(iRet <= 0)
			return(HXEMV_CORE); // AID�������
        ucAidLen = (uchar)iRet;

	// ����pdol���ݿ�
		iRet = iTlvMakeDOLBlock(sPdolBlock, sizeof(sPdolBlock), psPdol, iPdolLen, gl_sTlvDbTermFixed, gl_sTlvDbTermVar, gl_sTlvDbCard, NULL);
        if(iRet<0 || iRet>252) {
		    iSelDelCandidate(ucAidLen, psAid);
   			return(HXEMV_RESELECT); // ���ݲ��Ϸ�,������Ӧ��,����ѡ��
        }
		i = iTlvMakeAddObj(gl_sTlvDbTermVar, TAG_DFXX_PdolDataBlock, iRet, sPdolBlock, TLV_CONFLICT_REPLACE); // ����Pdol���ݿ�, ���ڿ��ܵ�CDA
		ASSERT(i >= 0);
		if(i < 0)
			return(HXEMV_CORE);
		iRet = iTlvMakeObject(TAG_83_CmdTemplate, iRet, sPdolBlock, sDataIn, sizeof(sDataIn)); // make T83, sDataIn is T83 object
		ASSERT(i >= 0);
		if(iRet < 0)
			return(HXEMV_CORE);
		ucDataInLen = iRet;
	}

	uiRet = uiEmvCmdGetOptions(ucDataInLen, sDataIn, &ucDataOutLen, sDataOut);
	if(uiRet == 0x6985) {
		if(gl_iCardType == EMV_CONTACTLESS_CARD)
			return(HXEMV_CARD_SW); // �ǽӿ�����6985, �ο�JR/T0025.12��2013 6.5.3, P14
	    iSelDelCandidate(ucAidLen, psAid);
		return(HXEMV_RESELECT); // GPOʧ��,��Ҫ����ѡ��Ӧ��, Refer to Test Case 2CA.030.05
	}
	if(uiRet == 1)
		return(HXEMV_CARD_OP);
	if(uiRet)
		return(HXEMV_CARD_SW);
	// GPOָ��ִ�гɹ�, ����GPO����
	if(sDataOut[0] == 0x80) {
		// Tag is 0x80, Format 1
		iRet = iTlvCheckTlvObject(sDataOut);
        if(iRet < 0) {
            return(HXEMV_TERMINATE); // refer to EMVbook3, 7.5 P78
        }
        if(iRet > ucDataOutLen) {
            return(HXEMV_TERMINATE); // refer to EMVbook3, 7.5 P78
        }
		iLen = iTlvValue(sDataOut, &psTlvObjValue);
        if(iLen < 2+4) {
            return(HXEMV_TERMINATE); // refer to EMVbook3, 7.5 P78
        }
        if((iLen-2) % 4) {
            return(HXEMV_TERMINATE); // AFL������4�ı��� refer to EMVbook3, 7.5 P78
        }
		iTlvDelObj(gl_sTlvDbCard, TAG_82_AIP); // ���֮ǰ��һӦ��������GPO, ɾ����AIP, ע:������ʼ���������ֻ�����0x77��ʽ��Tlv����, 0x80��ʽ������Ҫ�ֹ����
		iTlvDelObj(gl_sTlvDbCard, TAG_94_AFL); // ���֮ǰ��һӦ��������GPO, ɾ����AFL
		iRet = iTlvMakeAddObj(gl_sTlvDbCard, TAG_82_AIP, 2, psTlvObjValue, TLV_CONFLICT_ERR); // aip
		ASSERT(iRet >= 0);
		if(iRet < 0)
			return(HXEMV_CORE);
		iRet = iTlvMakeAddObj(gl_sTlvDbCard, TAG_94_AFL, iLen-2, psTlvObjValue+2, TLV_CONFLICT_ERR); // afl
		ASSERT(iRet >= 0);
		if(iRet < 0)
			return(HXEMV_CORE);
	} else if(sDataOut[0] == 0x77) {
		// Tag is 0x77, Format 2
		// Search AIP, T82
    	iRet = iTlvSearchObjValue(sDataOut, ucDataOutLen, 0, "\x77""\x82", &psTlvObjValue);
        if(iRet != 2) {
            return(HXEMV_TERMINATE); // refer to EMVbook3, 7.5 P78
        }
		// Search AFL, T94
    	iRet = iTlvSearchObjValue(sDataOut, ucDataOutLen, 0, "\x77""\x94", &psTlvObjValue);
        if(iRet<4 || iRet%4) {
            // û�ҵ�AIP��AIP���Ȳ���
            return(HXEMV_TERMINATE); // refer to EMVbook3, 7.5 P78
        }
		// ������TLV������ӵ�TLV���ݿ�
		iTlvDelObj(gl_sTlvDbCard, TAG_82_AIP); // ���֮ǰ��һӦ��������GPO, ɾ����AIP, ע:������ʼ���������ֻ�����0x77��ʽ��Tlv����, 0x80��ʽ������Ҫ�ֹ����
		iTlvDelObj(gl_sTlvDbCard, TAG_94_AFL); // ���֮ǰ��һӦ��������GPO, ɾ����AFL
		// ��ӵ�GPO�������ݿ�, ��������GPOʱɾ������GPO����
        iRet = iTlvBatchAddObj(1/*������ж���*/, sg_sGpoTlvBuf, sDataOut, ucDataOutLen, TLV_CONFLICT_ERR, 0/*0:����Ϊ0�������*/);
		if(iRet==TLV_ERR_BUF_SPACE || iRet==TLV_ERR_BUF_FORMAT) {
    		ASSERT(0);
			return(HXEMV_CORE);
		}
        iRet = iTlvBatchAddObj(1/*������ж���*/, gl_sTlvDbCard, sDataOut, ucDataOutLen, TLV_CONFLICT_ERR, 0/*0:����Ϊ0�������*/);
		if(iRet==TLV_ERR_BUF_SPACE || iRet==TLV_ERR_BUF_FORMAT) {
    		ASSERT(0);
			return(HXEMV_CORE);
		}
        if(iRet < 0) {
   			// ���ݷǷ�
            return(HXEMV_TERMINATE); // refer to EMVbook3, 7.5 P78
        }
    } else {
        // GPO���ظ�ʽ����
        return(HXEMV_TERMINATE); // refer to EMVbook3, 7.5 P78
    }

	// �жϽ���·�� 0:�Ӵ�Pboc(��׼DC��EC) 1:�ǽӴ�Pboc(��׼DC��EC) 2:qPboc���� 3:qPboc�ѻ� 4:qPboc�ܾ�
	ucTransRoute = 0; // ȱʡΪ�Ӵ�pboc
	if(gl_iCardType == EMV_CONTACTLESS_CARD) {
		// �ο�JR/T0025.12��2013 6.5.4, P14
		iRet = iTlvGetObjValue(gl_sTlvDbCard, TAG_82_AIP, &psAip);
		if(iRet != 2)
			return(HXEMV_TERMINATE); // ��AIP, ��ΪGPO���ظ�ʽ����

		if((psPbocCtlsGet9F66()[0]&0x60)==0x40 || (psPbocCtlsGet9F66()[0]&0x60)==0x60 && (psAip[1]&0x80)==0x80) {
			// ��֧�ַǽ�pboc �� ֧�ַǽ�pboc��qPboc����aipָ���ߵ���pboc
			ucTransRoute = 1; // �ߵ��Ƿǽ�pbocͨ��
		} else if((psPbocCtlsGet9F66()[0]&0x60)==0x20 || (psPbocCtlsGet9F66()[0]&0x60)==0x60 && (psAip[1]&0x80)==0x00) {
			// ��֧��qPboc �� ֧�ַǽ�pboc��qPboc����aipָ���ߵ���qPboc
			iRet = iTlvGetObjValue(gl_sTlvDbCard, TAG_9F26_AC, &p);
			if(iRet > 0) {
				// �ߵ�qPboc, �ж��ѻ������������Ǿܾ�
				iRet = iTlvGetObjValue(gl_sTlvDbCard, TAG_9F10_IssuerAppData, &p);
				if(iRet < 5)
					return(HXEMV_TERMINATE); // ֻ�õ�5�ֽ�
				if((p[4]&0x30) == 0x10)
					ucTransRoute = 3; // TC
				else if((p[4]&0x30) == 0x20)
					ucTransRoute = 2; // ARQC
				else
					ucTransRoute = 4; // AAC (RFU��Ϊ��AAC)
			} else {
				// �ߵ�pboc
				if(psPbocCtlsGet9F66()[0]&0x60 == 0x20)
					return(HXEMV_TERMINATE); // ��֧��qPboc������pboc
				ucTransRoute = 1; // �ߵ��Ƿǽ�pbocͨ��
			}
		}
	}
	iRet = iTlvMakeAddObj(gl_sTlvDbTermVar, TAG_DFXX_TransRoute, 1, &ucTransRoute, TLV_CONFLICT_REPLACE);
	ASSERT(iRet >= 0);
	if(iRet < 0)
		return(HXEMV_CORE);

	return(HXEMV_OK);
}

// ���AFL��ںϷ���
// in  : psAFL : AFL���
// ret : 0     : OK
//       1     : AFL�Ƿ�
// refer to EMV2008 book3, P78
static int iEmvCheckAFL(uchar *psAFL)
{
	uchar ucSFI;
	ucSFI = psAFL[0] >> 3;
	if(ucSFI==0 || ucSFI==31) {
		EMV_TRACE_ERR_MSG
		return(1); // An SFI of 0 or 31.
	}
	if(psAFL[1] == 0) {
		EMV_TRACE_ERR_MSG
		return(1); // A starting record number of 0.
	}
	if(psAFL[2] < psAFL[1]) {
		EMV_TRACE_ERR_MSG
		return(1); // An ending record number less than the starting record number (byte 3 < byte 2).
	}
	if(psAFL[3] > (psAFL[2]-psAFL[1])+1) {
		EMV_TRACE_ERR_MSG
		return(1); // Number of records participating in offline data authentication than the number of records (byte 4 > byte 3 - byte 2 + 1).
	}
	return(0);
}

// ��Ӧ�ü�¼
// ret : HXEMV_OK          : OK
//       HXEMV_CARD_OP     : ��������
//       HXEMV_CARD_SW     : �Ƿ���ָ��״̬��
//       HXEMV_TERMINATE   : ����ܾ�������������ֹ
//       HXEMV_CORE        : ��������
//		 HXEMV_EXPIRED_APP : ���ڿ�(�ǽ�)
int iEmvReadRecord(void)
{
	int   iRet;
	uint  uiRet;
	int   iAFLLen;
	uchar *psAFL;
	uchar *p;
	uchar ucRecLen;
	uchar sRecData[256];
	uchar ucSFI;
	int   i, j;
	int   iTlvObjLen, iTlvObjValueLen;
	uchar *psTlvObj, *psTlvObjValue;
	uchar szBuf[40];

	iAFLLen = iTlvGetObjValue(gl_sTlvDbCard, TAG_94_AFL, &psAFL);
	ASSERT(iAFLLen >= 0);
	if(iAFLLen < 0)
		return(HXEMV_CORE); // GPOʱ��ȷ��AFL�����ҺϷ�, ��Ӧ�ó��ִ����
	vPushApduPrepare(PUSHAPDU_READ_APP_REC, iAFLLen, psAFL); // add by yujun 2012.10.29, ֧���Ż�pboc2.0����apdu
	for(i=0, p=psAFL; i<iAFLLen/4; i++, p+=4) {
		iRet = iEmvCheckAFL(p);
		if(iRet) {
			EMV_TRACE_ERR_MSG
			return(HXEMV_TERMINATE); // AFL���Ϸ�
		}
        // ��ȡĳAFL���ָ���ļ�¼
		for(j=p[1]; j<=p[2]; j++) {
			ucSFI = p[0] >> 3; // j = RecNo
            iRet = uiEmvCmdRdRec(ucSFI, (uchar)j, &ucRecLen, sRecData);
			if(iRet == 1)
				return(HXEMV_CARD_OP); // ��������
			if(iRet)
				return(HXEMV_CARD_SW); // ��״̬�ַǷ�
            if(ucSFI>=1 && ucSFI<=10) {
        		iRet = iTlvCheckTlvObject(sRecData);
	    		if(iRet != ucRecLen)
		    		return(HXEMV_TERMINATE);  // ���ݲ��Ϸ�
            }
			if(ucSFI>=1 && ucSFI<=10 && sRecData[0]!=0x70)
				return(HXEMV_TERMINATE);  // SFI 1-10, must be T70
			if(j-p[1] < p[3]) {
				// �ü�¼Ϊ�ѻ�������֤��¼, ��Ҫ���ӵ��ѻ���֤������
        		iRet = iTlvCheckTlvObject(sRecData);
                if(iRet != ucRecLen) {
					// �ѻ�������֤��¼����ΪT70 Tlv����, ������ΪSDAʧ��(refer to EMV2008 book3, 10.3 P96)
					iEmvSDADataErr(); // ֪ͨ�ѻ���֤���ݼ�¼�Ƿ�
					// ���Լ�����������, ���ѻ�������֤����ɹ���
                }
				if(sRecData[0] != 0x70) {
					// �ѻ�������֤��¼����ΪT70 Tlv����, ������ΪSDAʧ��(refer to EMV2008 book3, 10.3 P96)
					iEmvSDADataErr(); // ֪ͨ�ѻ���֤���ݼ�¼�Ƿ�
					// ���Լ�����������, ���ѻ�������֤����ɹ���
				}
				if(ucSFI>=1 && ucSFI<=10) {
					iTlvObjValueLen = iTlvValue(sRecData, &psTlvObjValue); // ֮ǰ�Ѿ�������¼�Ϸ���
    				iRet = iEmvSDADataAdd((uchar)iTlvObjValueLen, psTlvObjValue);
				} else {
    				iRet = iEmvSDADataAdd(ucRecLen, sRecData);
				}
				ASSERT(iRet == EMV_DAUTH_OK);
				if(iRet != EMV_DAUTH_OK) {
					EMV_TRACE_ERR_MSG
					return(HXEMV_CORE); // SDA�ѻ���¼����������
				}
			}

			if(ucSFI<1 || ucSFI>10)
				continue; // EMV�淶֮���SFI������, ����
			if(sRecData[0] != 0x70) {
				EMV_TRACE_ERR_MSG
				return(HXEMV_TERMINATE);
			}
			if(iPbocCtlsGetRoute() == 3) {
				// �����ߵ�·��, 0:�Ӵ�Pboc(��׼DC��EC) 1:�ǽӴ�Pboc(��׼DC��EC) 2:qPboc���� 3:qPboc�ѻ� 4:qPboc�ܾ�
				// �ǽӿ����Ӵ���, refer JR/T0025.12��2013 7.7.20, P40
				// ��Ч�ڼ�� (�ݲ��ж�CA��Կ������֧�ּ���̬��֤��������ȿ����fDDAʧ�ܵ�����)
				// ?? �������ڡ�Pan�Ƿ���Track2��Panƥ�� ??
				// ?? Track2��Ҳ����Ч��, �ɲ�����GPO���ж�ʧЧ���� ??
				iRet = iTlvSearchObjValue(sRecData, ucRecLen, 0, "\x70""\x5F\x24", &psTlvObjValue);
				if(iRet == 3) {
					vOneTwo0(psTlvObjValue, 3, szBuf+2);
					if(memcmp(szBuf+2, "49", 2) <= 0)
						memcpy(szBuf, "20", 2);
					else
						memcpy(szBuf, "19", 2);
					_vGetTime(szBuf+20);
					if(memcmp(szBuf, szBuf+20, 8) < 0) {
						// ���ڿ�
						iEmvSetItemBit(EMV_ITEM_TVR, TVR_09_EXPIRED, 1); // ��TVR���ڱ�־λ
						return(HXEMV_EXPIRED_APP);
					}
				}
			}
			iRet = iTlvBatchAddObj(0/*ֻ��ӻ�������*/, gl_sTlvDbCard, sRecData, ucRecLen, TLV_CONFLICT_ERR, 0/*0:����Ϊ0�������*/);
			if(iRet==TLV_ERR_BUF_SPACE || iRet==TLV_ERR_BUF_FORMAT) {
			    ASSERT(0);
				return(HXEMV_CORE);
			}
			if(iRet < 0)
				return(HXEMV_TERMINATE);
		} // for(j
	} // for(i

	// ����gl_sTlvDbCard������ݺϷ���(refer to EMV2008 book3, 7.5 P77)
	for(i=0; ;) {
		iTlvObjLen = iTlvGetObjByIndex(gl_sTlvDbCard, i, &psTlvObj);
		if(iTlvObjLen < 0)
			break; // �������

		// ��������Դ
		uiRet = uiTagAttrGetFrom(psTlvObj);
		if(uiRet != TAG_FROM_CARD) {
			i ++;
			continue; // ����ö������ڿ�Ƭ, ����
		}

		// ���������Լ��
		iTlvObjLen = iTlvCheckTlvObject(psTlvObj);
		if(iTlvObjLen < 0) {
			// ���󲻺Ϸ�, �����Ϊ���ж���, ������
			if(memcmp(psTlvObj, TAG_5F20_HolderName, strlen(TAG_5F20_HolderName)) == 0 // Cardholder Name
					|| memcmp(psTlvObj, TAG_9F0B_HolderNameExt, strlen(TAG_9F0B_HolderNameExt)) == 0 // Cardholder Name Extended
					|| memcmp(psTlvObj, TAG_5F50_IssuerURL, strlen(TAG_5F50_IssuerURL)) == 0 // Issuer URL
					|| memcmp(psTlvObj, TAG_9F5A_IssuerURL2, strlen(TAG_9F5A_IssuerURL2)) == 0 // Issuer URL2
					|| memcmp(psTlvObj, TAG_9F4D_LogEntry, strlen(TAG_9F4D_LogEntry)) == 0 // Log Entry
					|| memcmp(psTlvObj, TAG_9F4F_LogFmt_I, strlen(TAG_9F4F_LogFmt_I)) == 0) { // Log Format
				iTlvDelObj(gl_sTlvDbCard, psTlvObj); // ���Դ��಻�Ϸ�����
				// ��Ҫ i++
				continue;
			}

			// ���󲻺Ϸ�, �����Ϊ���ж���(ECash��,��ҪGetData�ſ���ȡ��,��������ڼ�¼��,����), ������
            // ���˼������ΪEmv��ⰸ��2CJ012.00, �������ʹ��9F51��Ϊ��ʶ��Tag, ʵ��9F51ΪECash����, ��ɳ�ͻ
			if(memcmp(psTlvObj, TAG_9F51_AppCurrencyCode_I, strlen(TAG_9F51_AppCurrencyCode_I)) == 0 // Application Currency Code
					|| memcmp(psTlvObj, TAG_9F6D_ECResetThreshold_I, strlen(TAG_9F6D_ECResetThreshold_I)) == 0 // eCash: Reset Threshold
					|| memcmp(psTlvObj, TAG_9F77_ECBalanceLimit_I, strlen(TAG_9F77_ECBalanceLimit_I)) == 0 // EC Balance Limit
					|| memcmp(psTlvObj, TAG_9F79_ECBalance_I, strlen(TAG_9F79_ECBalance_I)) == 0) { // EC Balance
				iTlvDelObj(gl_sTlvDbCard, psTlvObj); // ���Դ��಻�Ϸ�����
				// ��Ҫ i++
				continue;
			}

			return(HXEMV_TERMINATE); // ��Ҫ���󲻺Ϸ�, ��ֹ����
		}

		// �����Ч�ڡ��������ںϷ���
		if(memcmp(psTlvObj, TAG_5F24_AppExpDate, strlen(TAG_5F24_AppExpDate)) == 0 // Application Expiration Date
			    || memcmp(psTlvObj, TAG_5F25_AppEffectiveDate, strlen(TAG_5F25_AppEffectiveDate)) == 0) { // Application Effective Date
            iTlvObjValueLen = iTlvValue(psTlvObj, &psTlvObjValue);
			if(iTlvObjValueLen != 3)
				return(HXEMV_TERMINATE); // ��Ҫ���󲻺Ϸ�, ��ֹ����
			vOneTwo0(psTlvObjValue, 3, sRecData); // ����sRecData��������
			if(iTestIfValidDate6(sRecData) != 0)
				return(HXEMV_TERMINATE); // ��Ҫ���󲻺Ϸ�, ��ֹ����
		}
		i ++;
	} // for(i ����

	// ������������
	if(iPbocCtlsGetRoute()==0 || iPbocCtlsGetRoute()==1) {
		// �����ߵ�·��, 0:�Ӵ�Pboc(��׼DC��EC) 1:�ǽӴ�Pboc(��׼DC��EC) 2:qPboc���� 3:qPboc�ѻ� 4:qPboc�ܾ�
		iRet = iTlvGetObj(gl_sTlvDbCard, TAG_5F24_AppExpDate, &psTlvObj); // Application Expiration Date
		if(iRet <= 0)
			return(HXEMV_TERMINATE); // �������ݲ�����, ��ֹ����
		iRet = iTlvGetObj(gl_sTlvDbCard, TAG_5A_PAN, &psTlvObj); // Application Primary Account Number(PAN)
		if(iRet <= 0)
			return(HXEMV_TERMINATE); // �������ݲ�����, ��ֹ����
		iRet = iTlvGetObj(gl_sTlvDbCard, TAG_8C_CDOL1, &psTlvObj); // Card Risk Management Data Object List 1(CDOL1)
		if(iRet <= 0)
			return(HXEMV_TERMINATE); // �������ݲ�����, ��ֹ����
		iRet = iTlvGetObj(gl_sTlvDbCard, TAG_8D_CDOL2, &psTlvObj); // Card Risk Management Data Object List 2(CDOL2)
		if(iRet <= 0)
			return(HXEMV_TERMINATE); // �������ݲ�����, ��ֹ����
	}

	// EC���׶��⴦��
	if(iEmvGetECTransFlag() == 1) {
		// �ն˳�����EC����
		iRet = iTlvGetObjValue(gl_sTlvDbCard, TAG_9F74_ECIssuerAuthCode, &p);
		if(iRet <= 0) {
			// ��EC��Ȩ��, ���ν��ײ���EC����, ���EC���ױ�־
			iRet = iEmvSetECTransFlag(0);
			ASSERT(iRet == 0);
			if(iRet)
				return(HXEMV_CORE);
		} else {
			// ����EC��Ȩ��, ���ν�����EC����
			if(iRet != 6)
				return(HXEMV_TERMINATE); // EC��Ȩ�벻����, ��ֹ����
			// ��ȡEC���
			iTlvObjLen = iTlvObjValueLen = 100;
			iRet = iHxGetCardNativeData(TAG_9F79_ECBalance_I, &iTlvObjLen, sRecData, &iTlvObjValueLen, sRecData+100); // ����sRecData��������������
			if(iRet==HXEMV_CARD_OP || iRet==HXEMV_CARD_SW || iRet==HXEMV_FLOW_ERROR || iRet==HXEMV_CORE)
				return(iRet);
			if(iRet!=HXEMV_OK || iTlvObjValueLen!=12)
				return(HXEMV_TERMINATE); // ���ݷǷ�, ��ֹ����
			// ���������Ϊ����ǰ���浽Tlv���ݿ�
			iRet = iTlvAddObj(gl_sTlvDbCard, sRecData, TLV_CONFLICT_REPLACE);
			if(iRet < 0)
				return(HXEMV_CORE);

			// ��ȡEC���÷�ֵ
			iTlvObjLen = iTlvObjValueLen = 100;
			iRet = iHxGetCardNativeData(TAG_9F6D_ECResetThreshold_I, &iTlvObjLen, sRecData, &iTlvObjValueLen, sRecData+100); // ����sRecData����������ֵ����
			if(iRet==HXEMV_CARD_OP || iRet==HXEMV_CARD_SW || iRet==HXEMV_FLOW_ERROR || iRet==HXEMV_CORE)
				return(iRet);
			if(iRet!=HXEMV_OK || iTlvObjValueLen!=12)
				return(HXEMV_TERMINATE); // ���ݷǷ�, ��ֹ����

#if 0
// Pboc3.0 Book13 7.4.2, ûҪ���EC�������
			// ��ȡEC�������
			iTlvObjLen = iTlvObjValueLen = 100;
			iRet = iHxGetCardNativeData(TAG_9F77_ECBalanceLimit_I, &iTlvObjLen, sRecData, &iTlvObjValueLen, sRecData+100); // ����sRecData��������������޶���
			if(iRet==HXEMV_CARD_OP || iRet==HXEMV_CARD_SW || iRet==HXEMV_FLOW_ERROR || iRet==HXEMV_CORE)
				return(iRet);
			if(iRet!=HXEMV_OK || iTlvObjValueLen!=12)
				return(HXEMV_TERMINATE); // ���ݷǷ�, ��ֹ����
#endif
		}
	}

	return(HXEMV_OK);
}

// �ն���Ϊ����
// in  : iFlag : 0 : �����ն���Ϊ����(������׼DC��EC)
//               1 : ֻ�ж�IAC/TAC default, ������������������ʱ�Ƿ�ܾ�����
//               2 : �����ܾ�λ, ��ε���, ���ھ���ܾ���������ֹ����Ҫ������
//               3 : ECר��, �����ն���Ϊ����, �ж��Ƿ���Ҫ����EC��Ҫ����������
// ret : 0 : ����
//       1 : �ܾ�
//       2 : ����
//       3 : �ѻ�
//       4 : ��Ҫ����EC��Ҫ����������, ֻ��iFlag=3ʱ�ſ��ܷ���
//       EC�����������뷵��ֵ
//       -1: �û�ȡ��
//       -2: ��ʱ
static int iTermActionAnalysis(int iFlag)
{
	int    iRet;
	uchar  *p;
	uchar  sIACDenial[5], sIACOnline[5], sIACDefault[5];
	uchar  sTACDenial[5], sTACOnline[5], sTACDefault[5];
	uchar  sTVR[5];
	int    iTermAction; // 1:�ܾ� 2:���� 3:�ѻ�
	uchar  szECBalance[13], szECResetThreshold[13], szAmount[13], szTmp[13];

	// ��ȡIACs
	iRet = iTlvGetObjValue(gl_sTlvDbCard, TAG_9F0E_IACDenial, &p);
	if(iRet == 5)
		memcpy(sIACDenial, p, 5);
	else
		memset(sIACDenial, 0, 5);
	iRet = iTlvGetObjValue(gl_sTlvDbCard, TAG_9F0F_IACOnline, &p);
	if(iRet == 5)
		memcpy(sIACOnline, p, 5);
	else
		memset(sIACOnline, 0xFF, 5);
	iRet = iTlvGetObjValue(gl_sTlvDbCard, TAG_9F0D_IACDefault, &p);
	if(iRet == 5)
		memcpy(sIACDefault, p, 5);
	else
		memset(sIACDefault, 0xFF, 5);
	// ��ȡTACs
	iRet = iTlvGetObjValue(gl_sTlvDbTermVar, TAG_DFXX_TACDenial, &p);
	if(iRet == 5)
		memcpy(sTACDenial, p, 5);
	else
		memset(sTACDenial, 0, 5);
	iRet = iTlvGetObjValue(gl_sTlvDbTermVar, TAG_DFXX_TACOnline, &p);
	if(iRet == 5)
		memcpy(sTACOnline, p, 5);
	else
		memset(sTACOnline, 0, 5);
	iRet = iTlvGetObjValue(gl_sTlvDbTermVar, TAG_DFXX_TACDefault, &p);
	if(iRet == 5)
		memcpy(sTACDefault, p, 5);
	else
		memset(sTACDefault, 0, 5);
	// ��ȡTVR
	iRet = iTlvGetObjValue(gl_sTlvDbTermVar, TAG_95_TVR, &p);
	ASSERT(iRet == 5);
	if(iRet == 5)
		memcpy(sTVR, p, 5);
	else
		return(0); // TVR���ȱ������5

	// ����TACs
	vOr(sIACDenial, sTACDenial, 5);
	vOr(sIACOnline, sTACOnline, 5);
	vOr(sIACDefault, sTACDefault, 5);

	// ����TVR
	vAnd(sIACDenial, sTVR, 5);
	vAnd(sIACOnline, sTVR, 5);
	vAnd(sIACDefault, sTVR, 5);

	if(iFlag == 1) {
		// �ж��ն�ȱʡ��Ϊ
		if(iEmvTermCommunication() == TERM_COM_ONLINE_ONLY) {
			iTermAction = 1; // �������ն�ȱʡ��ΪΪ�ܾ�
			goto label_ec_analysis;
		}
		if(iTestStrZero(sIACDefault, 5) == 0)
			iTermAction = 3; // ȱʡ��ΪΪ���
		else
			iTermAction = 1; // ȱʡ��ΪΪ�ܾ�
		goto label_ec_analysis;
	}

	// �ն���Ϊ����
	if(iTestStrZero(sIACDenial, 5) != 0) {
		iTermAction = 1; // �ܾ�
		goto label_ec_analysis;
	}
	if(iEmvTermCommunication() == TERM_COM_ONLINE_ONLY) {
		// �������ն����û�о����ܾ�����, ����Ҫ������������
		iTermAction = 2; // online
		goto label_ec_analysis;
	}
	if(iEmvTermCommunication() != TERM_COM_OFFLINE_ONLY) {
		// �������������ն���Ҫ�ж���������λ
		if(iTestStrZero(sIACOnline, 5) != 0)
			iTermAction = 2; // online
		else
			iTermAction = 3; // �ѻ����
		goto label_ec_analysis;
	}
	if(iEmvTermCommunication() == TERM_COM_OFFLINE_ONLY) {
		// ���ѻ��ն���Ҫ�ж�ȱʡ����λ�Ѿ����Ƿ��ѻ���ɽ���
		if(iTestStrZero(sIACDefault, 5) == 0) {
			iTermAction = 3; // �ѻ����
			goto label_ec_analysis;
		}
	}
	iTermAction = 1; // �ܾ�

label_ec_analysis:
	// EC�������
	if(iEmvGetECTransFlag() != 1) // EC���ױ�־ 0:��EC���� 1:EC���� 2:GPO��Ƭ����EC��Ȩ��,��δ��ECͨ��
		return(iTermAction); // ��EC����, ֱ�ӷ���֮ǰȷ�����ն���Ϊ

	// EC����	iTermAction== 1:�ܾ� 2:���� 3:�ѻ�
	if(iTermAction == 1) {
		return(iTermAction); // �ܾ�����ֱ�ӷ���
	}
	if(iTermAction == 2) {
	    // �������ײ���ECͨ��
		iRet = iEmvSetECTransFlag(2); // 2:GPO��Ƭ����EC��Ȩ��,��δ��ECͨ��
		return(iTermAction);
	}

	// �ѻ�����, ����ECash�淶JR/T 0025.13��2010 7.4.4
    // ȡ��Ȩ���
	iRet = iTlvGetObjValue(gl_sTlvDbTermVar, TAG_9F02_AmountN, &p);
	ASSERT(iRet == 6);
	if(iRet != 6)
		return(0); // ����
	vOneTwo0(p, 6, szAmount);
	// ȡEC���
	iRet = iTlvGetObjValue(gl_sTlvDbCard, TAG_9F79_ECBalance_I, &p);
	ASSERT(iRet == 6);
	if(iRet != 6)
		return(0); // ����
	vOneTwo0(p, 6, szECBalance);
	// ȡEC���÷�ֵ
	iRet = iTlvGetObjValue(gl_sTlvDbCard, TAG_9F6D_ECResetThreshold_I, &p);
	ASSERT(iRet == 6);
	if(iRet != 6)
		return(0); // ����
	vOneTwo0(p, 6, szECResetThreshold);
    
	memset(szTmp, 0, sizeof(szTmp)); // szTmp׼�������洢ECBalance-Amount
	iRet = iStrNumSub(szTmp, 12, szECBalance, szAmount, 10, 0);
	ASSERT(iRet >= 0);
	if(iRet < 0)
		return(0); // ���ַǷ�
	if(iRet) {
		// �н�λ, ECBalance������С��Amount, ������, IC������ѡ��ECͨ��, ������·�����, ����
		iRet = iEmvSetECTransFlag(2); // 2:GPO��Ƭ����EC��Ȩ��,��δ��ECͨ��
		return(2);
	}
	if(iStrNumCompare(szTmp, szECResetThreshold, 10) >= 0) {
		// �����ֽ�����ȥ��Ȩ�����ڻ���ڵ����ֽ�������ֵ
		return(3); // �����ѻ���׼
	}

	// �����ֽ�����ȥ��Ȩ���С�ڵ����ֽ�������ֵ
	vTraceWriteTxtCr(TRACE_ALWAYS, "�����ֽ�����ȥ��Ȩ���С�ڵ����ֽ�������ֵ");

	if(iEmvTermCommunication() == TERM_COM_OFFLINE_ONLY) {
		// �ն˲��߱���������
		return(3); // �����ѻ���׼
	}

	if(iFlag == 2) {
		// �ܾ�λ���, ���ּ��Ӧ���������н���, ��˲�������������
		return(iTermAction);
	}

	// �ն˾߱���������, ������������Ҫ��ֿ�����������PIN
	iRet = iHxGetData(TAG_DFXX_EncOnlinePIN, NULL, NULL, NULL, NULL);
	if(iRet == HXEMV_OK) {
		// ���������Ѿ�����, ��������
		iRet = iEmvSetECTransFlag(2); // 2:GPO��Ƭ����EC��Ȩ��,��δ��ECͨ��
		return(2);
	}
	// ֮ǰû��������������, �ڴ˴�Ҫ��������������
	// (����Ҫ��淶δ��˵��)
	//     ֮ǰ�Ƿ���ʾ��������������(bypass)?
	//     �ն��Ƿ�֧����������?
	// ����˴���:���֮ǰҪ�������������뵫bypass��,�ѻ����
	//            ��ʱ�����������뵫bypass,���ѻ����(���IAC/TACҪ��bypass������Ҫ����, ���������)
	//            ���û�������豸�������豸���ܹ���, �ѻ����
	//            ������CVMResult(ȷ��EMV��׼����CVMResult������)
	//            �������������PIN, ����TVR����PIN����λ
	if(iEmvTestItemBit(EMV_ITEM_TERM_CAP, TERM_CAP_09_ENC_ONLINE_PIN) == 0) {
		return(3); // ��֧��������������, �ѻ����
	}

	if(iFlag == 3)
		return(4); // 4:��Ҫ����EC��Ҫ����������
	return(3); // �ѻ����
}

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
int iEmvOfflineDataAuth(int iInCrlFlag)
{
	int iRet;

	iRet = iPbocCtlsGetRoute(); // ��ȡ����·��
	if(iRet == 3) {
		// iRet==3 : qPboc�ѻ�, ֻ��qPboc�ѻ����ײ���Ҫִ��fDDA		
		if(iInCrlFlag == 0)
			iRet = iEmvGetIcPubKey(); // ��ȡIC����Կ
		else
			iRet = HXEMV_FDDA_FAIL; // ������CRL��־, ֱ����Ϊ�ѻ�������֤ʧ��
		if(iRet == EMV_DAUTH_NATIVE)
			return(HXEMV_CORE);
		if(iRet == EMV_DAUTH_DATA_ILLEGAL)
			return(HXEMV_TERMINATE);
		if(iRet == EMV_DAUTH_DATA_MISSING)
			iEmvSetItemBit(EMV_ITEM_TVR, TVR_02_ICC_DATA_MISSING, 1); // �������ȱʧ,����TVR����ȱʧλ
		if(iRet != EMV_DAUTH_OK) {
			// DDAʧ��, TSI��λ�ѻ���ִ֤��λ, TVR��λDDAʧ��λ
			iEmvSetItemBit(EMV_ITEM_TSI, TSI_00_DATA_AUTH_PERFORMED, 1);
			iEmvSetItemBit(EMV_ITEM_TVR, TVR_04_DDA_FAILED, 1);
			return(HXEMV_FDDA_FAIL); // FDDAʧ��
		}
		// ��ȡIC����Կ�ɹ�, ִ��fDDA��֤
		iEmvSetItemBit(EMV_ITEM_TSI, TSI_00_DATA_AUTH_PERFORMED, 1); // TSI��λ�ѻ���ִ֤��λ
		iRet = iEmvFdda();
		if(iRet == EMV_DAUTH_NATIVE)
			return(HXEMV_CORE);
		if(iRet == EMV_DAUTH_DATA_MISSING)
			iEmvSetItemBit(EMV_ITEM_TVR, TVR_02_ICC_DATA_MISSING, 1); // �������ȱʧ,����TVR����ȱʧλ
		if(iRet!=EMV_DAUTH_OK || iInCrlFlag==1) {
			// fDDAʧ��, TVR��λDDAʧ��λ
			iEmvSetItemBit(EMV_ITEM_TVR, TVR_04_DDA_FAILED, 1);
			return(HXEMV_FDDA_FAIL);
		}
		return(HXEMV_OK);
	} else if(iRet==2 || iRet==4) {
		// iRet==2:qPboc���� iRet==4:qPboc�ܾ�, qPboc������qPboc�ܾ� ������Ҫ�����ѻ�������֤
		return(HXEMV_FLOW_ERROR);
	}

// ����ն���IC����֧��CDA, ִ��CDA��֤
	if(iEmvTestItemBit(EMV_ITEM_AIP, AIP_07_CDA_SUPPORTED)
			&& iEmvTestItemBit(EMV_ITEM_TERM_CAP, TERM_CAP_20_CDA)) {
		if(iInCrlFlag == 0)
			iRet = iEmvGetIcPubKey(); // ��ȡIC����Կ
		else
			iRet = EMV_DAUTH_FAIL; // ������CRL��־, ֱ����Ϊ�ѻ�������֤ʧ��
		if(iRet == EMV_DAUTH_NATIVE)
			return(HXEMV_CORE);
		if(iRet == EMV_DAUTH_DATA_ILLEGAL)
			return(HXEMV_TERMINATE);
		if(iRet == EMV_DAUTH_DATA_MISSING)
			iEmvSetItemBit(EMV_ITEM_TVR, TVR_02_ICC_DATA_MISSING, 1); // �������ȱʧ,����TVR����ȱʧλ
		if(iRet != EMV_DAUTH_OK) {
			// CDAʧ��, TSI��λ�ѻ���ִ֤��λ, TVR��λCDAʧ��λ
			iEmvSetItemBit(EMV_ITEM_TSI, TSI_00_DATA_AUTH_PERFORMED, 1); // ��λ֮ǰ�����Ѿ���λ
			iEmvSetItemBit(EMV_ITEM_TVR, TVR_05_CDA_FAILED, 1);
		}
		// CDA��ʹ����ʧ��, Ҳ��Ҫ����ִ�н���, refer to EMV2008 book4, 6.3.2 P44
		// ע����GACʱ��Ҫ�ٴ��ж�CDA�Ƿ�����ʧ�ܹ�, �Ա㰴�淶ִ����Ӧ����
		goto _label_dauth_finished;
	}
	
// ����ն���IC����֧��DDA, ִ��DDA��֤
	if(iEmvTestItemBit(EMV_ITEM_AIP, AIP_02_DDA_SUPPORTED)
			&& iEmvTestItemBit(EMV_ITEM_TERM_CAP, TERM_CAP_17_DDA)) {
		if(iInCrlFlag == 0)
			iRet = iEmvGetIcPubKey(); // ��ȡIC����Կ
		else
			iRet = EMV_DAUTH_FAIL; // ������CRL��־, ֱ����Ϊ�ѻ�������֤ʧ��
		if(iRet == EMV_DAUTH_NATIVE)
			return(HXEMV_CORE);
		if(iRet == EMV_DAUTH_DATA_ILLEGAL)
			return(HXEMV_TERMINATE);
		if(iRet == EMV_DAUTH_DATA_MISSING)
			iEmvSetItemBit(EMV_ITEM_TVR, TVR_02_ICC_DATA_MISSING, 1); // �������ȱʧ,����TVR����ȱʧλ
		if(iRet != EMV_DAUTH_OK) {
			// DDAʧ��, TSI��λ�ѻ���ִ֤��λ, TVR��λDDAʧ��λ
			iEmvSetItemBit(EMV_ITEM_TSI, TSI_00_DATA_AUTH_PERFORMED, 1);
			iEmvSetItemBit(EMV_ITEM_TVR, TVR_04_DDA_FAILED, 1);
			goto _label_dauth_finished; // DDAʧ��
		}
		// ��ȡIC����Կ�ɹ�, ִ��DDA��֤
		iRet = iEmvDda();
		if(iRet == EMV_DAUTH_NATIVE)
			return(HXEMV_CORE);
		if(iRet == EMV_DAUTH_CARD_IO)
			return(HXEMV_CARD_OP);
		if(iRet == EMV_DAUTH_CARD_SW)
			return(HXEMV_CARD_SW);
		if(iRet == EMV_DAUTH_DATA_ILLEGAL)
			return(HXEMV_TERMINATE);

		// TSI��λ�ѻ���ִ֤��λ
		iEmvSetItemBit(EMV_ITEM_TSI, TSI_00_DATA_AUTH_PERFORMED, 1);

		if(iRet == EMV_DAUTH_DATA_MISSING)
			iEmvSetItemBit(EMV_ITEM_TVR, TVR_02_ICC_DATA_MISSING, 1); // �������ȱʧ,����TVR����ȱʧλ
		if(iRet != EMV_DAUTH_OK) {
			// DDAʧ��, TVR��λDDAʧ��λ
			iEmvSetItemBit(EMV_ITEM_TVR, TVR_04_DDA_FAILED, 1);
			goto _label_dauth_finished; // DDAʧ��, ����
		}
		// DDA��֤�ɹ�
		goto _label_dauth_finished;
	}

// ����ն���IC����֧��SDA, ִ��SDA��֤
	if(iEmvTestItemBit(EMV_ITEM_AIP, AIP_01_SDA_SUPPORTED)
			&& iEmvTestItemBit(EMV_ITEM_TERM_CAP, TERM_CAP_16_SDA)) {
		if(iInCrlFlag == 0)
			iRet = iEmvGetIssuerPubKey(); // ��ȡ�����й�Կ
		else
			iRet = EMV_DAUTH_FAIL; // ������CRL��־, ֱ����Ϊ�ѻ�������֤ʧ��
		if(iRet == EMV_DAUTH_NATIVE)
			return(HXEMV_CORE);
		if(iRet == EMV_DAUTH_DATA_ILLEGAL)
			return(HXEMV_TERMINATE);
		if(iRet == EMV_DAUTH_DATA_MISSING)
			iEmvSetItemBit(EMV_ITEM_TVR, TVR_02_ICC_DATA_MISSING, 1); // �������ȱʧ,����TVR����ȱʧλ
		if(iRet != EMV_DAUTH_OK) {
			// SDAʧ��, TSI��λ�ѻ���ִ֤��λ, TVR��λSDAʧ��λ
			iEmvSetItemBit(EMV_ITEM_TSI, TSI_00_DATA_AUTH_PERFORMED, 1);
			iEmvSetItemBit(EMV_ITEM_TVR, TVR_01_SDA_FAILED, 1);
			goto _label_dauth_finished; // SDAʧ��
		}
		// ��ȡ�����й�Կ�ɹ�, ִ��SDA��֤
		iRet = iEmvSda();
		if(iRet == EMV_DAUTH_NATIVE)
			return(HXEMV_CORE);

		// TSI��λ�ѻ���ִ֤��λ
		iEmvSetItemBit(EMV_ITEM_TSI, TSI_00_DATA_AUTH_PERFORMED, 1);

		if(iRet == EMV_DAUTH_DATA_MISSING)
			iEmvSetItemBit(EMV_ITEM_TVR, TVR_02_ICC_DATA_MISSING, 1); // �������ȱʧ,����TVR����ȱʧλ
		if(iRet != EMV_DAUTH_OK) {
			// SDAʧ��, TVR��λSDAʧ��λ
			iEmvSetItemBit(EMV_ITEM_TVR, TVR_01_SDA_FAILED, 1);
			goto _label_dauth_finished; // SDAʧ��, ����
		}
		// SDA��֤�ɹ�
		goto _label_dauth_finished;
	}

// �ն˲�֧���ѻ�������֤
	// TVR��λ�ѻ�������֤û��ִ��λ
	iEmvSetItemBit(EMV_ITEM_TVR, TVR_00_DATA_AUTH_NOT_PERFORMED, 1);

_label_dauth_finished:
	// �ѻ�������֤���
#if 0
// wlfxy
// marked by yujun 20121212, for Test case J2CT0380000v4.1a_6 ��
//                           ���స��Ҫ���ڿ�Ƭ���ڵ�����¼�������Ƶ�ȷ���, ���������оܾ�λ���,
//                           ��IAC-denialΪȫFF����½�����ִ��Ƶ�ȷ���
//                           Ϊͨ�����స��, ��ִ��EMV�淶��3��涨�Ŀ��Զ��ִ���ն���Ϊ������ѡ��
//                           ����, ���EMV�ٷ���BCTC��֪���స��������(������ΪӦ�ñ�����), ����Կ��Ŵ�����
	iRet = iEmvIfDenial();
	if(iRet != HXEMV_OK)
		return(iRet);
#endif
	return(HXEMV_OK);
}

// ��������
// ret : HXEMV_OK            : OK
//       HXEMV_CORE          : ��������
//       HXEMV_DENIAL        : �ն���Ϊ�������Ϊ�ѻ��ܾ�
//       HXEMV_DENIAL_ADVICE : �ն���Ϊ�������Ϊ�ѻ��ܾ�, ��Advice
//       HXEMV_CARD_OP       : �ն���Ϊ�������Ϊ�ѻ��ܾ�, ����AACʱ��Ƭ����
//       HXEMV_TERMINATE     : ����ܾ�������������ֹ
int iEmvProcRistrictions(void)
{
	int   iRet;
	uchar *psCardVer, *psTermVer;
	uchar *psIssuerCountryCode, *psTermCountryCode;
	uint  uiTransType;
	uchar *psTransDate, *psEffectiveDate, *psExpirationDate;
	uchar *p;
	int   iInternationalFlag; // 1:���� 0:����

	// application version number
	iRet = iTlvGetObjValue(gl_sTlvDbCard, TAG_9F08_AppVerCard, &psCardVer); // Application Version Number, Card
	if(iRet > 0) {
		// ��Ƭ����Ӧ�ð汾��
		iRet = iTlvGetObjValue(gl_sTlvDbTermVar, TAG_9F09_AppVerTerm, &psTermVer); // Application Version Number, Terminal
		ASSERT(iRet == 2);
		if(iRet != 2)
			return(HXEMV_CORE); // �ն˱�����ڴ���
		if(memcmp(psCardVer, psTermVer, 2) != 0) {
			// ��Ƭ���ն�Ӧ�ð汾�Ų���, TVR��λ��Ӧ��ʶλ
			iEmvSetItemBit(EMV_ITEM_TVR, TVR_08_DIFFERENT_VER, 1);
		}
	}

	// application usage control
	iRet = iTlvGetObjValue(gl_sTlvDbCard, TAG_9F07_AUC, &p); // AUC
	if(iRet == 2) {
		// AUC����
		// 20140401, ���Ӷ�ATM��֧��
		if(iEmvTestIfIsAtm()) {
			// ��ATM
			if(!iEmvTestItemBit(EMV_ITEM_AUC, AUC_06_ATMS))
				iEmvSetItemBit(EMV_ITEM_TVR, TVR_11_SERVICE_NOT_ALLOWED, 1); // ��Ƭ�������ڷ�ATM�Ͻ���, ��TVR��������λ
		} else {
			// ����ATM
			if(!iEmvTestItemBit(EMV_ITEM_AUC, AUC_07_NO_ATMS))
				iEmvSetItemBit(EMV_ITEM_TVR, TVR_11_SERVICE_NOT_ALLOWED, 1); // ��Ƭ�������ڷ�ATM�Ͻ���, ��TVR��������λ
		}
		iRet = iTlvGetObjValue(gl_sTlvDbTermFixed, TAG_9F1A_TermCountryCode, &psTermCountryCode); // terminal country code
		ASSERT(iRet == 2);
		if(iRet != 2)
			return(HXEMV_CORE); // �ն˹��Ҵ���������
		iRet = iTlvGetObjValue(gl_sTlvDbCard, TAG_5F28_IssuerCountryCode, &psIssuerCountryCode); // issuer country code
		if(iRet == 2) {
			// �����й��Ҵ������
			if(memcmp(psIssuerCountryCode, psTermCountryCode, 2) == 0)
				iInternationalFlag = 0; // ���ڿ�
			else
				iInternationalFlag = 1; // ���⿨
			iRet = iTlvGetObjValue(gl_sTlvDbTermVar, TAG_DFXX_TransType, &p); // trans type
			if(iRet != 2)
				return(HXEMV_CORE); // �������ͱ������
			uiTransType = ulStrToLong(p, 2);
			switch(uiTransType) {
			case TRANS_TYPE_CASH:
				if(iInternationalFlag == 0)
					iRet = iEmvTestItemBit(EMV_ITEM_AUC, AUC_00_DOMESTIC_CASH);
				else
					iRet = iEmvTestItemBit(EMV_ITEM_AUC, AUC_01_INTERNATIONAL_CASH);
				if(iRet == 0)
					iEmvSetItemBit(EMV_ITEM_TVR, TVR_11_SERVICE_NOT_ALLOWED, 1); // ��Ƭ������, ��TVR��������λ
				break;
			case TRANS_TYPE_SALE:
			case TRANS_TYPE_GOODS:
			case TRANS_TYPE_SERVICES:
				// refer emv bulletin AN27, ֻҪ��purchase���ײ���Goods��Services, AUCֻҪ֧��Goods��Services����һ��, ����Ϊ֧��
				if(iInternationalFlag == 0) {
					if(iEmvTestItemBit(EMV_ITEM_AUC, AUC_02_DOMESTIC_GOODS)==0
						    && iEmvTestItemBit(EMV_ITEM_AUC, AUC_04_DOMESTIC_SERVICES)==0)
						iEmvSetItemBit(EMV_ITEM_TVR, TVR_11_SERVICE_NOT_ALLOWED, 1); // ��Ƭ������, ��TVR��������λ
				} else {
					if(iEmvTestItemBit(EMV_ITEM_AUC, AUC_03_INTERNATIONAL_GOODS)==0
						    && iEmvTestItemBit(EMV_ITEM_AUC, AUC_05_INTERNATIONAL_SERVICES)==0)
						iEmvSetItemBit(EMV_ITEM_TVR, TVR_11_SERVICE_NOT_ALLOWED, 1); // ��Ƭ������, ��TVR��������λ
				}
				break;
			default:
				break; // ������������AUC������λ, ��Ϊ���
			}
		} // �����й��Ҵ������
	} // AUC����

	// ��Ч/�������ڼ��
	iRet = iTlvGetObjValue(gl_sTlvDbTermVar, TAG_9A_TransDate, &psTransDate); // ��������
	ASSERT(iRet == 3);
	if(iRet != 3)
		return(HXEMV_CORE); // �������ڱ������
	iRet = iTlvGetObjValue(gl_sTlvDbCard, TAG_5F24_AppExpDate, &psExpirationDate); // ʧЧ����
	ASSERT(iRet == 3);
	if(iRet != 3)
		return(HXEMV_CORE); // ʧЧ���ڱ������
	if(iCompDate3(psTransDate, psExpirationDate) > 0)
		iEmvSetItemBit(EMV_ITEM_TVR, TVR_09_EXPIRED, 1); // ��TVR���ڱ�־λ
	iRet = iTlvGetObjValue(gl_sTlvDbCard, TAG_5F25_AppEffectiveDate, &psEffectiveDate); // ��Ч����
	if(iRet == 3) {
		// ��Ч���ڴ����ҺϷ�
		if(iCompDate3(psTransDate, psEffectiveDate) < 0)
			iEmvSetItemBit(EMV_ITEM_TVR, TVR_10_NOT_EFFECTIVE, 1); // ��TVRδ���������ڱ�־λ
	}
#if 0
// wlfxy, ����������, �򲻻᷵�ؿ�Ƭ������صķ���ֵ
// marked by yujun 20121212, for Test case J2CT0380000v4.1a_6 ��
//                           ���స��Ҫ���ڿ�Ƭ���ڵ�����¼�������Ƶ�ȷ���, ���������оܾ�λ���,
//                           ��IAC-denialΪȫFF����½�����ִ��Ƶ�ȷ���
//                           Ϊͨ�����స��, ��ִ��EMV�淶��3��涨�Ŀ��Զ��ִ���ն���Ϊ������ѡ��
//                           ����, ���EMV�ٷ���BCTC��֪���స��������(������ΪӦ�ñ�����), ����Կ��Ŵ�����
	iRet = iEmvIfDenial();
	if(iRet != HXEMV_OK)
		return(iRet);
#endif
	return(HXEMV_OK);
}

// ����޶���
// ret : HXEMV_OK          : OK
//       HXEMV_CORE        : ��������
static int iEmvFloorLimitChecking(void)
{
	int iRet;
	uchar *psFloorLimit, szFloorLimit[13];
	uchar *psAmount, szAmount[20];
	uchar szHistoryAmount[13];
	uchar *p;

	if(iEmvGetECTransFlag() == 1)
		return(HXEMV_OK); // EC���ײ�������޶���

	// ȡ����޶�
	iRet = iTlvGetObjValue(gl_sTlvDbTermVar, TAG_9F1B_TermFloorLimit, &psFloorLimit);
	ASSERT(iRet == 4);
	if(iRet != 4)
		return(HXEMV_CORE); // floor limit�������
	sprintf(szFloorLimit, "%012lu", ulStrToLong(psFloorLimit, 4));
	// ȡ��Ȩ���
	iRet = iTlvGetObjValue(gl_sTlvDbTermVar, TAG_9F02_AmountN, &psAmount); // Amount, Authorised(Numeric)
	ASSERT(iRet == 6);
	if(iRet != 6)
		return(HXEMV_CORE); // amount, authorised�������
	vOneTwo0(psAmount, 6, szAmount);

	if(iStrNumCompare(szAmount, szFloorLimit, 10) >= 0) {
		// ��������޶�, ��λTVR��Ӧָʾλ
		iEmvSetItemBit(EMV_ITEM_TVR, TVR_24_EXCEEDS_FLOOR_LIMIT, 1);
		return(HXEMV_OK);
	}

	// �ֿ����Ѽ��
	iRet = iTlvGetObjValue(gl_sTlvDbTermFixed, TAG_DFXX_SeparateSaleSupport, &p); // ��ȡ�ֿ����Ѽ���־
	if(iRet==1 && *p) {
		// ֧�ַֿ�����, �ۼƽ��
		iRet = iEmvIOSeparateSaleCheck(szHistoryAmount);
		ASSERT(iRet == EMVIO_OK);
		if(iRet != EMVIO_OK)
			return(HXEMV_CORE);
		
		iStrNumAdd(szAmount, sizeof(szAmount)-1, szAmount, szHistoryAmount, 10, '0');
	}

	if(iStrNumCompare(szAmount, szFloorLimit, 10) >= 0) {
		// ��������޶�, ��λTVR��Ӧָʾλ
		iEmvSetItemBit(EMV_ITEM_TVR, TVR_24_EXCEEDS_FLOOR_LIMIT, 1);
	}
	return(HXEMV_OK);
}
// ������׼��
// ret : HXEMV_OK          : OK
//       HXEMV_CORE        : ��������
static int iEmvRandomTransSelection(void)
{
	int   iRet;
	uchar *p;
	int   iMaxTargetPercentage, iTargetPercentage;
	uchar szRandomSelThreshold[13];
	uchar szFloorLimit[13], szAmount[20];
	int   iTransactionTargetPercent;
	uchar szInterpolationFactor[20];
	long  lInterpolationFactor;
	uchar ucRand;

	if(iEmvGetECTransFlag() == 1)
		return(HXEMV_OK); // EC���ײ���������׼��

	if(iEmvTermCommunication() == TERM_COM_OFFLINE_ONLY)
		return(HXEMV_OK); // �ѻ��ն˲�����������׼��

	iRet = iTlvGetObjValue(gl_sTlvDbTermVar, TAG_DFXX_MaxTargetPercentage, &p);
	if(iRet != 1)
		return(HXEMV_OK); // ��MaxTargetPercentage, ����������
	iMaxTargetPercentage = (int)*p;

	iRet = iTlvGetObjValue(gl_sTlvDbTermVar, TAG_DFXX_TargetPercentage, &p);
	if(iRet != 1)
		return(HXEMV_OK); // ��TargetPercentage, ����������
	iTargetPercentage = (int)*p;

	iRet = iTlvGetObjValue(gl_sTlvDbTermVar, TAG_DFXX_RandomSelThreshold, &p);
	if(iRet != 4)
		return(HXEMV_OK); // ��RandomSelThreshold, ����������
	sprintf(szRandomSelThreshold, "%012lu", ulStrToLong(p, 4));

	iRet = iTlvGetObjValue(gl_sTlvDbTermVar, TAG_9F1B_TermFloorLimit, &p);
	ASSERT(iRet == 4);
	if(iRet != 4)
		return(HXEMV_CORE); // FloorLimit�������
	sprintf(szFloorLimit, "%012lu", ulStrToLong(p, 4));

	iRet = iTlvGetObjValue(gl_sTlvDbTermVar, TAG_9F02_AmountN, &p);
	ASSERT(iRet == 6);
	if(iRet != 6)
		return(HXEMV_CORE); // Amount�������
	vOneTwo0(p, 6, szAmount);

	// ����Transaction Target Percent, refer to Emv2008 book3, 10.6.2, P112
	if(iStrNumCompare(szAmount, szFloorLimit, 10) >= 0)
		return(HXEMV_OK); // �����ڵ���FloorLimit, ����������޶��鴦��
	if(iStrNumCompare(szAmount, szRandomSelThreshold, 10) <= 0) {
		// ���С�ڵ��ڷ�ֵ, �����������ʵ���iTargetPercentage%
		iTransactionTargetPercent = iTargetPercentage;
	} else {
		// �����ڷ�ֵ, ������������

		// InterpolationFactor = (Amount-ThresholdValue) / (FloorLimit-ThresholdValue)
		// Ϊ��߾���, InterpolationFactor����10000��
		iStrNumSub(szAmount, sizeof(szAmount)-1, szAmount, szRandomSelThreshold, 10, 0);
		iStrNumMult(szAmount, sizeof(szAmount)-1, szAmount, "10000", 10, 0); // ��������ֻ֧������, ����10000������߾���
		iStrNumSub(szFloorLimit, sizeof(szFloorLimit)-1, szFloorLimit, szRandomSelThreshold, 10, 0);
		iStrNumDiv(szInterpolationFactor, sizeof(szInterpolationFactor)-1, NULL, 0, szAmount, szFloorLimit, 10, 0);
		lInterpolationFactor = atol(szInterpolationFactor); // ע��,lInterpolationFactorΪʵ��ֵ��10000��, ����0.45��ʾΪ4500

		// ����iTransactionTargetPercent
		// iTransactionTargetPercent = ((MaximumTargetPercent-TargetPercent)*InterpolationFactor)+TargetPercent
		iTransactionTargetPercent = (((iMaxTargetPercentage-iTargetPercentage)*lInterpolationFactor)+5000) / 10000;
		iTransactionTargetPercent += iTargetPercentage;
		if(iTransactionTargetPercent > iMaxTargetPercentage)
			iTransactionTargetPercent = iMaxTargetPercentage; // �ݴ�
	}

	// ����1-99�������
	vGetRand(szAmount, 3);
	ucRand = (uchar)(ulStrToLong(szAmount, 3)%99) + 1;
	iRet = iTlvMakeAddObj(gl_sTlvDbTermVar, TAG_DFXX_RandForSelect, 1, &ucRand, TLV_CONFLICT_REPLACE);
	if(ucRand <= iTransactionTargetPercent) {
		// ���ױ����ѡ��, ����TVR��Ӧλ
		iEmvSetItemBit(EMV_ITEM_TVR, TVR_27_RANDOMLY_SELECTED, 1);
	}

	return(HXEMV_OK);
}
// Ƶ�ȼ��
// ret : HXEMV_OK          : OK
//       HXEMV_CORE        : ��������
//       HXEMV_CARD_OP     : ��������
//       HXEMV_TERMINATE   : ����ܾ�������������ֹ
static int iEmvVelocityChecking(void)
{
	int   iRet;
    uint  uiRet;
	uchar *p;
	uchar ucLen, sBuf[256];
	uchar *pucLowerLimit, *pucUpperLimit;
	uchar sATC[2], sLastOnlineATC[2];
	uchar ucATCFlag, ucLastOnlineATCFlag; // ATC��LastOnlineATC�������, 0:δ���� 1:�ɹ�����
	uint  uiDifference;

	if(iEmvGetECTransFlag() == 1)
		return(HXEMV_OK); // EC���ײ���Ƶ�ȼ��

	iRet = iTlvGetObjValue(gl_sTlvDbCard, TAG_9F14_LCOL, &pucLowerLimit);
	if(iRet != 1)
		return(HXEMV_OK); // ��Lower Consecutive Offline Limit, ����Ƶ�ȼ��
	iRet = iTlvGetObjValue(gl_sTlvDbCard, TAG_9F23_UCOL, &pucUpperLimit);
	if(iRet != 1)
		return(HXEMV_OK); // ��Upper Consecutive Offline Limit, ����Ƶ�ȼ��

	uiRet = uiEmvCmdGetData(TAG_9F36_ATC, &ucLen, sBuf);
	if(uiRet == 1)
		return(HXEMV_CARD_OP); // ��Ƭ������
	ucATCFlag = 0; // �ȼ���û�ж���ATC
	if(uiRet == 0) {
		// �ɹ�����ATC
		if(iTlvCheckTlvObject(sBuf) == (int)ucLen) {
			iRet = iTlvValue(sBuf, &p);
			if(iRet == 2) {
				memcpy(sATC, p, 2);
				ucATCFlag = 1;
			}
		}
	}

	uiRet = uiEmvCmdGetData(TAG_9F13_LastOnlineATC, &ucLen, sBuf);
	if(uiRet == 1)
		return(HXEMV_CARD_OP);
	ucLastOnlineATCFlag = 0; // �ȼ���û�ж���Last Online ATC
	if(uiRet == 0) {
		// �ɹ�����LastOnlineATC
		if(iTlvCheckTlvObject(sBuf) == (int)ucLen) {
			iRet = iTlvValue(sBuf, &p);
			if(iRet == 2) {
				memcpy(sLastOnlineATC, p, 2);
				ucLastOnlineATCFlag = 1;
			}
		}
	}

	// �ο�EMV���԰��� 2CA.029.04, ���û�ж���ATC��Last On Line ATC, �������ݶ�ʧλ
	if(ucATCFlag==0 || ucLastOnlineATCFlag==0)
		iEmvSetItemBit(EMV_ITEM_TVR, TVR_02_ICC_DATA_MISSING, 1); // data missing

	// �ο� Emv2008 book3 10.6.3, P113
	if(ucATCFlag==0 || ucLastOnlineATCFlag==0 || memcmp(sATC, sLastOnlineATC, 2)<=0) {
		// ����TVR��Ӧλ
		iEmvSetItemBit(EMV_ITEM_TVR, TVR_25_EXCEEDS_LOWER_LIMIT, 1); // lower consecutive offline limit exceeded
		iEmvSetItemBit(EMV_ITEM_TVR, TVR_26_EXCEEDS_UPPER_LIMIT, 1); // upper consecutive offline limit exceeded
		if(iTestStrZero(sLastOnlineATC, 2) == 0) {
			// ��TVR�¿���־
			iEmvSetItemBit(EMV_ITEM_TVR, TVR_12_NEW_CARD, 1); // new card
		}
		return(HXEMV_OK);
	}
	uiDifference = ulStrToLong(sATC, 2) - ulStrToLong(sLastOnlineATC, 2);
	if(uiDifference > (uint)*pucLowerLimit) {
		// ������lower consecutive offline limit exceeded
		iEmvSetItemBit(EMV_ITEM_TVR, TVR_25_EXCEEDS_LOWER_LIMIT, 1); // lower consecutive offline limit exceeded
		if(uiDifference > (uint)*pucUpperLimit) {
			iEmvSetItemBit(EMV_ITEM_TVR, TVR_26_EXCEEDS_UPPER_LIMIT, 1); // upper consecutive offline limit exceeded
		}
	}
	if(iTestStrZero(sLastOnlineATC, 2) == 0) {
		// ��TVR�¿���־
		iEmvSetItemBit(EMV_ITEM_TVR, TVR_12_NEW_CARD, 1); // new card
	}

	return(HXEMV_OK);
}
// ���������
// ret : HXEMV_OK          : OK
//       HXEMV_CORE        : ��������
static int iExceptionFileChecking(void)
{
	int   iRet;
	uchar *p;

	iRet = iTlvGetObjValue(gl_sTlvDbTermFixed, TAG_DFXX_BlacklistSupport, &p);
	if(iRet != 1)
		return(HXEMV_OK); // �ն˲�֧�ֺ��������
	if(*p == 0)
		return(HXEMV_OK); // �ն˲�֧�ֺ��������

	// �ն�֧�ֺ��������
	iRet = iEmvIOBlackCardCheck();
	ASSERT(iRet != EMVIO_ERROR);
	if(iRet == EMVIO_ERROR)
		return(HXEMV_CORE);
	if(iRet == EMVIO_BLACK) {
		// ��������, ����TVR��Ӧλ
		iEmvSetItemBit(EMV_ITEM_TVR, TVR_03_BLACK_CARD, 1); // ��������
	}

	return(HXEMV_OK);
}

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
int iEmvTermRiskManage(void)
{
	int   iRet;
	uchar *psTlvObjValue;
	uint  uiTransType;
	uchar szAmount[13];

	iRet = iTlvGetObjValue(gl_sTlvDbTermVar, TAG_9F02_AmountN, &psTlvObjValue);
	if(iRet > 0)
		vOneTwo0(psTlvObjValue, 6, szAmount);
	if(iRet<0 || strcmp((char *)szAmount, "000000000000")==0) {
		// ֮ǰû��������, �ն˷��չ���ǰ��������
		iRet = iTlvGetObjValue(gl_sTlvDbTermVar, TAG_DFXX_TransType, &psTlvObjValue);
        ASSERT(iRet == 2);
		if(iRet != 2)
			return(HXEMV_CORE);
		uiTransType = ulStrToLong(psTlvObjValue, 2);
		if(uiTransType==TRANS_TYPE_AVAILABLE_INQ || uiTransType==TRANS_TYPE_BALANCE_INQ)
			strcpy(szAmount, "0"); // ��ѯ����Ҫ������
		else {
			iRet = iEmvIOGetAmount(EMVIO_AMOUNT, szAmount);
			if(iRet == EMVIO_CANCEL)
				return(HXEMV_CANCEL);
			if(iRet == EMVIO_TIMEOUT)
				return(HXEMV_TIMEOUT);
            ASSERT(iRet == 0);
			if(iRet)
				return(HXEMV_CORE);
		}
		iRet = iEmvSaveAmount(szAmount); // �����浽tlv���ݿ���
		if(iRet != 0)
			return(HXEMV_CORE);
	}

	iRet = iEmvFloorLimitChecking(); // ����޶���
	if(iRet == HXEMV_CORE)
		return(iRet);
	iRet = iEmvRandomTransSelection(); // ������׼��
	if(iRet == HXEMV_CORE)
		return(iRet);
	iRet = iEmvVelocityChecking(); // Ƶ�ȼ��
	if(iRet != HXEMV_OK)
		return(iRet);
	iRet = iExceptionFileChecking(); // ���������
	if(iRet == HXEMV_CORE)
		return(iRet);
	iEmvSetItemBit(EMV_ITEM_TSI, TSI_04_TERM_RISK_PERFORMED, 1); // �ն˷��չ�����ִ��
    // ע, ��ʱ������оܾ�λ���, ����ֹע���ܾ��Ľ��׻�Ҫ��ֿ��˽��гֿ�����֤
	iRet = iEmvIfDenial();
	if(iRet != HXEMV_OK)
		return(iRet);

	return(HXEMV_OK);
}

// �ֿ�����֤
// out : piNeedSignFlag      : ����ɹ���ɣ�������Ҫǩ�ֱ�־��0:����Ҫǩ�� 1:��Ҫǩ��
// ret : HXEMV_OK            : OK
//       HXEMV_CANCEL        : �û�ȡ��
//       HXEMV_TIMEOUT       : ��ʱ
//       HXEMV_CARD_OP       : ��������
//       HXEMV_CARD_SW       : �Ƿ���״̬��
//       HXEMV_TERMINATE     : ����ܾ�������������ֹ
//       HXEMV_DENIAL        : �ն���Ϊ�������Ϊ�ѻ��ܾ�
//       HXEMV_DENIAL_ADVICE : �ն���Ϊ�������Ϊ�ѻ��ܾ�, ��Advice
//       HXEMV_FLOW_ERROR    : EMV���̴���
//       HXEMV_CORE          : �ڲ�����
int iEmvModuleCardHolderVerify(int *piNeedSignFlag)
{
	int   iRet;

	iRet = iEmvCardHolderVerify(piNeedSignFlag);
	if(iRet != HXEMV_OK)
		return(iRet);

	iRet = iEmvIfDenial();
	if(iRet != HXEMV_OK)
		return(iRet);
	return(HXEMV_OK);
}

// �ж��Ƿ���Ҫ����EC��Ҫ����������
// ret : HXEMV_OK		   : ��Ҫ
//       HXEMV_NA          : ����Ҫ
int iEmvIfNeedECOnlinePin(void)
{
	int    iRet;

	iRet = iTermActionAnalysis(3); // 3:ECר��, �����ն���Ϊ����, �ж��Ƿ���Ҫ����EC��Ҫ����������
	if(iRet == 4)
		return(HXEMV_OK); // ��ҪEC��������
	return(HXEMV_NA); // ����ҪEC��������
}

// �ж��Ƿ���Ҫ�ܾ�����
// ret : HXEMV_OK		     : ����Ҫ
//       HXEMV_CARD_OP       : ��������
//       HXEMV_TERMINATE     : ����ܾ�������������ֹ
//       HXEMV_DENIAL        : �ն���Ϊ�������Ϊ�ѻ��ܾ�
//       HXEMV_DENIAL_ADVICE : �ն���Ϊ�������Ϊ�ѻ��ܾ�, ��Advice
//       HXEMV_CORE          : �ڲ�����
int iEmvIfDenial(void)
{
	int    iRet;
	int    iCardAction;

	iRet = iTermActionAnalysis(2); // 2:�ܾ�λ���
	if(iRet == 1) {
		// ���ױ��ܾ�
		iEmvIOShowMsg(EMVMSG_0E_PLEASE_WAIT); // newadd
		iRet = iEmvGAC1(&iCardAction);
		if(iRet==HXEMV_CORE || iRet==HXEMV_CARD_OP || iRet==HXEMV_TERMINATE)
			return(iRet);
		if(iRet != HXEMV_OK)
			return(HXEMV_DENIAL);
		if(iCardAction == GAC_ACTION_AAC_ADVICE)
			return(HXEMV_DENIAL_ADVICE);
		return(HXEMV_DENIAL);
	}
	return(HXEMV_OK);
}

// �ն���Ϊ����
// ret : HXEMV_OK		     : OK
//       HXEMV_CORE          : ��������
//       HXEMV_DENIAL        : �ն���Ϊ�������Ϊ�ѻ��ܾ�
//       HXEMV_DENIAL_ADVICE : �ն���Ϊ�������Ϊ�ѻ��ܾ�, ��Advice
//       HXEMV_CARD_OP       : �ն���Ϊ�������Ϊ�ѻ��ܾ�, ����AACʱ��Ƭ����
//       HXEMV_TERMINATE     : ����ܾ�������������ֹ
int iEmvTermActionAnalysis(void)
{
	int    iRet;
	int    iCardAction;

	iRet = iTermActionAnalysis(0); // 0:�����ն���Ϊ����
	if(iRet == 0)
		return(HXEMV_CORE);
	if(iRet == 1) {
		// ���ױ��ܾ�
		iRet = iEmvGAC1(&iCardAction); // ����iCardAction
		if(iRet==HXEMV_CORE || iRet==HXEMV_CARD_OP || iRet==HXEMV_TERMINATE)
			return(iRet);
		if(iRet != HXEMV_OK)
			return(HXEMV_DENIAL);
		if(iCardAction == GAC_ACTION_AAC_ADVICE)
			return(HXEMV_DENIAL_ADVICE);
		return(HXEMV_DENIAL);
	}
	return(HXEMV_OK); // ֻҪ���Ǿܾ�, �ͷ���OK
}

// ����TC Hash Value
// ret : HXEMV_OK          : OK
//       HXEMV_CORE        : ��������
static int iGenTCHashValue(void)
{
	int   iRet;
	uchar *p;
	uchar ucTdolLen, sTdol[252];
	uchar sBuf[10240];

	// ������ƬTdol
	iRet = iTlvGetObjValue(gl_sTlvDbCard, TAG_97_TDOL, &p);
	if(iRet > 0) {
		// �ҵ���ƬTDOL
		ASSERT(iRet <= 252);
		if(iRet > 252)
			return(HXEMV_CORE); // ����¼ʱ���������Ӧȷ����ʱTDOL���Ȳ�����252
		memcpy(sTdol, p, iRet);
		ucTdolLen = (uchar)iRet;
	} else {
		// ��Ƭ��TDOL, �����ն�Default TDOL
		iRet = iTlvGetObjValue(gl_sTlvDbTermVar, TAG_DFXX_DefaultTDOL, &p);
		if(iRet > 0) {
			// �ҵ�Default TDOL
			memcpy(sTdol, p, iRet);
			ucTdolLen = (uchar)iRet;
			iEmvSetItemBit(EMV_ITEM_TVR, TVR_32_DEFAULT_TDOL_USED, 1); // ����TVR Default TDOLʹ��λ
		} else {
			// û�ҵ�Default TDOL
			ucTdolLen = 0;
		}
	}
		// ����TC Hash Value
    // ���TDOL
	iRet = iTlvMakeDOLBlock(sBuf/*Ŀ��*/, sizeof(sBuf), sTdol, ucTdolLen,
								gl_sTlvDbTermFixed, gl_sTlvDbTermVar, gl_sTlvDbCard, gl_sTlvDbIssuer);
	if(iRet < 0)
		return(HXEMV_CORE);
	// ����TDOLĿ���ɹ�, ����TC Hash Value
	vSHA1Init();
	vSHA1Update(sBuf, (ushort)iRet);
    vSHA1Result(sBuf); // sBuf is the TC Hash Value
	iRet = iTlvMakeAddObj(gl_sTlvDbTermVar, TAG_98_TCHashValue, 20, sBuf, TLV_CONFLICT_REPLACE);
	ASSERT(iRet >= 0);
	if(iRet < 0)
		return(HXEMV_CORE);
	return(HXEMV_OK);
}

// ��ȡGACӦ������
// in  : ucP1            : ����GACָ��ʱ�Ĳ���P1
//       psDataOut       : GACָ��ķ�������
//       ucDataOutLen    : GACָ��ķ������ݳ���
// ret : HXEMV_OK        : OK
//       HXEMV_CORE      : ��������
//       HXEMV_TERMINATE : ����ܾ�������������ֹ
static int iGetACData(uchar ucP1, uchar *psDataOut, uchar ucDataOutLen)
{
	uchar ucTlvObjValueLen, *psTlvObjValue, *psTlvObj;
	uchar sTlvObj9F10[40]; // ����T9F10���ݶ���
	int   iRet;

	// ɾ��GAC���ܷ��ص�������, ֮ǰ��ʼ��������¼��DDAʱ, ����Ŀ����ܻ�����Щα����
	iTlvDelObj(gl_sTlvDbCard, TAG_9F27_CID);
	iTlvDelObj(gl_sTlvDbCard, TAG_9F36_ATC);
	iTlvDelObj(gl_sTlvDbCard, TAG_9F26_AC);
	if(ucP1 & 0x10) {
		iTlvDelObj(gl_sTlvDbCard, TAG_9F4B_SDAData); // ����Ҫ��CDA, ����֮ǰ���ܵ�ǩ�ֶ�̬����
	}
	iRet = iTlvGetObj(gl_sTlvDbCard, TAG_9F10_IssuerAppData, &psTlvObj);
	if(iRet > 0)
		memcpy(sTlvObj9F10, psTlvObj, iRet); // ����T9F10���ݶ���
	else
		sTlvObj9F10[0] = 0;
	iTlvDelObj(gl_sTlvDbCard, TAG_9F10_IssuerAppData); // �������GAC1���ڵ�GAC2������ʱ

	iRet = iTlvCheckTlvObject(psDataOut); // ���Ӧ������TLV��ʽ������
	if(iRet<0 || iRet>ucDataOutLen)
		return(HXEMV_TERMINATE);
	if(psDataOut[0] == 0x80) {
		// Tag is 0x80, Format 1 CID(T9F27)[1] + ATC(T9F36) + AC(T9F26)[8] + IAD(T9F10)(Option)[n]
		iRet = iTlvValue(psDataOut, &psTlvObjValue);
		if(iRet<11 || iRet>11+32) {
			// GPOָ������ݳ��Ȳ���, need at least CID[1] + ATC[2] + AC[8]
			// or GPOָ������ݳ��ȳ���, need at most CID[1] + ATC[2] + AC[8] + IAD[32]
			return(HXEMV_TERMINATE);
		}
		ucTlvObjValueLen = (uchar)iRet;

		// psTlvObjValue=T80��ֵ ucTlvObjValueLen=T80��ֵ�ĳ���
		if(ucP1 & 0x10) {
			// �������CDA��������format1
			if(psTlvObjValue[0] & 0xC0) {
				// �ڷ��ز���AACʱ����Ҫ���˼��
				return(HXEMV_TERMINATE);
			}
		}
		// add T9F27
		iRet = iTlvMakeAddObj(gl_sTlvDbCard, TAG_9F27_CID, 1, psTlvObjValue+0, TLV_CONFLICT_REPLACE);
    	ASSERT(iRet >= 0);
		if(iRet < 0)
			return(HXEMV_CORE);
		// add T9F36
		iRet = iTlvMakeAddObj(gl_sTlvDbCard, TAG_9F36_ATC, 2, psTlvObjValue+1, TLV_CONFLICT_REPLACE);
    	ASSERT(iRet >= 0);
		if(iRet < 0)
			return(HXEMV_CORE);
		// add T9F26
		vRandShuffle(ulStrToLong(psTlvObjValue+3, 4)); // ����AC�Ŷ������������
		iRet = iTlvMakeAddObj(gl_sTlvDbCard, TAG_9F26_AC, 8, psTlvObjValue+3, TLV_CONFLICT_REPLACE);
    	ASSERT(iRet >= 0);
		if(iRet < 0)
			return(HXEMV_CORE);
		if(ucTlvObjValueLen > 1+2+8) {
			// ����T9F10, add T9F10
			iRet = iTlvMakeAddObj(gl_sTlvDbCard, TAG_9F10_IssuerAppData, ucTlvObjValueLen-11, psTlvObjValue+11, TLV_CONFLICT_REPLACE);
        	ASSERT(iRet >= 0);
			if(iRet < 0)
				return(HXEMV_CORE);
		} else if(sTlvObj9F10[0]) {
			// ������T9F10, �����֮ǰ�����ݹ���Ƭ���ݿ��е�T9F10, �ָ�
			iRet = iTlvAddObj(gl_sTlvDbCard, sTlvObj9F10, TLV_CONFLICT_REPLACE);
			ASSERT(iRet >= 0);
			if(iRet < 0)
				return(HXEMV_CORE);
		}
	} else if(psDataOut[0] == 0x77) {
		// Tag is 0x77, Format 2
		// ����Ƭ����������ӵ���ƬTLV���ݿ�
		iRet = iTlvBatchAddObj(0/*only primitive*/, gl_sTlvDbCard, psDataOut, ucDataOutLen, TLV_CONFLICT_ERR, 0/*0:����Ϊ0�������*/);
		if(iRet==TLV_ERR_BUF_SPACE || iRet==TLV_ERR_BUF_FORMAT) {
        	ASSERT(0);
			return(HXEMV_CORE);
		}
		if(iRet < 0)
			return(HXEMV_TERMINATE);
		if(iTlvGetObj(gl_sTlvDbCard, TAG_9F10_IssuerAppData, &psTlvObj) <= 0) {
			// T77������T9F10���ݶ���
			if(sTlvObj9F10[0]) {
				// ���֮ǰ����T9F10���ݶ���, �ָ�
				iRet = iTlvAddObj(gl_sTlvDbCard, sTlvObj9F10, TLV_CONFLICT_REPLACE);
				ASSERT(iRet >= 0);
				if(iRet < 0)
					return(HXEMV_CORE);
			}
		}

		// check T9F27 T9F36 T9F26
	    iRet = iTlvGetObjValue(gl_sTlvDbCard, TAG_9F36_ATC, &psTlvObjValue);
		if(iRet != 2)
			return(HXEMV_TERMINATE); // û�ҵ�ATC���ʽ����
	    iRet = iTlvGetObjValue(gl_sTlvDbCard, TAG_9F27_CID, &psTlvObjValue);
		if(iRet != 1)
			return(HXEMV_TERMINATE); // û�ҵ�CID���ʽ����
		if((ucP1&0x10) && !(psTlvObjValue[0]&0xC0)) {
			// ���������CDA������CIDָ��������AAC, �ܾ�����, refer Emv2008 book 6.6.2, P74
			return(HXEMV_OK); // ���ܷ���HXEMV_TERMINATE
		}
		if(!(ucP1&0x10) || !(psTlvObjValue[0]&0xC0)) {
			// �����������CDA������CIDָ��������AAC, Ҫ��������AC
		    iRet = iTlvGetObjValue(gl_sTlvDbCard, TAG_9F26_AC, &psTlvObjValue);
			if(iRet != 8)
				return(HXEMV_TERMINATE); // û�ҵ�AC���ʽ����
			vRandShuffle(ulStrToLong(psTlvObjValue, 4)); // ����AC�Ŷ������������
		}
	} else {
		return(HXEMV_TERMINATE); // GACָ������ݸ�ʽ�벻ʶ��
	}

	return(HXEMV_OK);
}

// ��9F10�л�ȡEC���, ���û�н����Ϣ, �����¶�ȡ
// ret : HXEMV_OK   : OK
// Note: Pboc2.0 Book13, EC�淶, 7.4.6
//       ���ڽ����Ѿ��ɹ�, �����ȡ���ʧ��, ����Ϊ����
static int iGetECBalanceFrom9F10(void)
{
	uchar *psValue;
	int   iValueLen;
	uchar ucLen;
	uchar sBalance[20];
	int   iRet;
	uint  uiRet;

	iValueLen = iTlvGetObjValue(gl_sTlvDbCard, TAG_9F10_IssuerAppData, &psValue);
	// 9F10���ݸ�ʽ: �淶�Զ������ݳ���[1]+�淶�Զ�������[n]+�������Զ������ݳ���[1]+�������Զ�������[n]
	// ����ڷ������Զ���������, ��ʽ:��ʶ(����ʶΪ0x01)[1]+���[5]+Mac[4]
	// ����Զ�������������
	if(iValueLen <= 0)
		goto _label_getdata_readbal; // û����9F10
	if(psValue[0]+1 >= iValueLen)
		goto _label_getdata_readbal; // �޷������Զ�������
	// psValue �� iValueLen�������������Զ�������
	iValueLen -= psValue[0]+1;
	psValue += psValue[0]+1;
	// ��鷢�����Զ�������������
	if(psValue[0]+1 > iValueLen)
		goto _label_getdata_readbal; // �������Զ������ݸ�ʽ��
	if(psValue[0] < 10)
		goto _label_getdata_readbal; // �����Ҫ10�ֽڿռ�, �������Զ������ݳ��Ȳ���
	// psValue �� iValueLen�������������Զ�����������
	iValueLen --;
	psValue ++;
	// ����Ƿ�Ϊ����ʶ
	if(psValue[0] != 0x01)
		goto _label_getdata_readbal; // ��������ʶ
	// �ɹ���ȡ�����, �����滻ԭ���(����Mac)
	sBalance[0] = 0;
	memcpy(&sBalance[1], psValue+1, 5);
	iRet = iTlvMakeAddObj(gl_sTlvDbCard, TAG_9F79_ECBalance_I, 6, sBalance, TLV_CONFLICT_REPLACE);
	// ignore iRet
	return(HXEMV_OK);

_label_getdata_readbal:
	// 9F10û�л�ȡ�����, ͨ��getdataָ���ȡ
	iValueLen = sizeof(sBalance);
	uiRet = uiEmvCmdGetData(TAG_9F79_ECBalance_I, &ucLen, sBalance);
	if(uiRet != 0)
		return(HXEMV_OK); // ���˷��� : ��Ȼ�����ʧ��, ����ȻGAC1�Ѿ��ɹ�, �������, ֻ�ǲ��������
	if(ucLen < 2)
		return(HXEMV_OK); // 9F09�����ʽ : Tag[2]+Len[1]+Value[6]
	if(memcmp(sBalance, TAG_9F79_ECBalance_I, 2) != 0)
		return(HXEMV_OK); // ��ȡ�Ķ��������
	iRet = iTlvCheckTlvObject(sBalance);
	if(iRet <= 0)
		return(HXEMV_OK); // ��ʽ��
	// �ɹ����������, �����滻ԭ���
	iRet = iTlvAddObj(gl_sTlvDbCard, sBalance, TLV_CONFLICT_REPLACE);
	// ignore iRet
	return(HXEMV_OK);
}

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
//       HXEMV_NOT_ACCEPTED: ������
// Note: ĳЩ�����, �ú������Զ�����iEmvGAC2()
//       ��:CDAʧ�ܡ����ѻ��ն�GAC1��Ƭ����ARQC...
//       ����Ϊ����ARQC����Ҫ��CDA
int iEmvGAC1(int *piCardAction)
{
	uint  uiRet;
	int   iRet, iRet2;
	uint  uiTransType;
	uchar *p;
	int   iCdol1Len;
	uchar *psCdol1;
	int   iReqCdaFlag; // Ҫ��CDA��־, 0:��ҪCDA, 1:ҪCDA
	int   iTermAction; // �ն���Ϊ�������, 1:�ܾ� 2:���� 3:�ѻ�
	uchar ucP1; // GACָ�����P1
	uchar ucDataInLen, sDataIn[256];
	uchar ucDataOutLen, sDataOut[256];
	uchar szArc[3];
	uchar ucCid;
	uchar sBuf[260];

	// ׼������
	// �����
	vGetRand(sBuf, 4);
	iTlvMakeAddObj(gl_sTlvDbTermVar, TAG_9F37_TermRand, 4, sBuf, TLV_CONFLICT_REPLACE);
	// ��CDOL1������TC Hash Value��, �������, ׼��TC Hash Value
	iCdol1Len = iTlvGetObjValue(gl_sTlvDbCard, TAG_8C_CDOL1, &psCdol1);
   	ASSERT(iCdol1Len > 0);
	if(iCdol1Len <= 0)
		return(HXEMV_CORE); // CDOL1�������
    iRet =  iTlvSearchDOLTag(psCdol1, iCdol1Len, TAG_98_TCHashValue, NULL);
	if(iRet > 0) {
		// CDOL1Ҫ��TC Hash Value, ����TC Hash Value
		iRet = iGenTCHashValue();
		if(iRet)
			return(iRet);
	}

	iReqCdaFlag = 1; // ���ȼ���Ҫ��CDA
	if(iEmvTestItemBit(EMV_ITEM_AIP, AIP_07_CDA_SUPPORTED)==0
			|| iEmvTestItemBit(EMV_ITEM_TERM_CAP, TERM_CAP_20_CDA)==0)
		iReqCdaFlag = 0; // ֻ���ն��뿨Ƭ��֧��CDAʱ��ִ��CDA
	if(iEmvTestItemBit(EMV_ITEM_TVR, TVR_05_CDA_FAILED))
		iReqCdaFlag = 0; // CDA׼���׶��Ѿ�ʧ��, ��ִ��CDA

	iTermAction = iTermActionAnalysis(0/*0:�����ն���Ϊ����*/); // ����0:���� 1:�ܾ� 2:���� 3:�ѻ�
	if(iTermAction <= 0)
		return(HXEMV_CORE);
	szArc[0] = 0;
	switch(iTermAction) {
	case 1: // Denial
        ucP1 = 0x00; // require AAC
		if(iReqCdaFlag) {
			iEmvSetItemBit(EMV_ITEM_TVR, TVR_00_DATA_AUTH_NOT_PERFORMED, 1);
			iReqCdaFlag = 0; // ����AAC��ִ��CDA
		}
		strcpy(szArc, "Z1");
		break;
	case 2: // Online
		ucP1 = 0x80; // require ARQC (10XX XXXX)
		break;
	case 3: // Offline
		iRet = iTlvGetObjValue(gl_sTlvDbTermVar, TAG_DFXX_TransType, &p);
		ASSERT(iRet == 2);
		if(iRet != 2)
			return(HXEMV_CORE); // �������ͱ������
		uiTransType = ulStrToLong(p, 2);
		if(uiTransType==TRANS_TYPE_AVAILABLE_INQ || uiTransType==TRANS_TYPE_BALANCE_INQ) {
			// ��ѯ����ǿ������
			ucP1 = 0x80; // require ARQC (10XX XXXX)
		} else {
			ucP1 = 0x40; // require TC (01XX XXXX)
			strcpy(szArc, "Y1");
		}
		break;
	}
	if(iReqCdaFlag)
		ucP1 |= 0x10; // CDA request (XXX1 XXXX)

	if(szArc[0]) {
		// szArc������, ��Ȼ�ѻ����, ��T8A���Ƿŵ�������TLV���ݿ���
		iTlvMakeAddObj(gl_sTlvDbIssuer, TAG_8A_AuthRspCode, 2, szArc, TLV_CONFLICT_REPLACE);
	}

	if(iReqCdaFlag)
		iEmvSetItemBit(EMV_ITEM_TSI, TSI_00_DATA_AUTH_PERFORMED, 1); // ���������CDA, ����TSI�ѻ�������ִ֤�б�־

    // ���CDOL1Ŀ�괮
	iRet = iTlvMakeDOLBlock(sDataIn/*CDOL1Ŀ�괮*/, sizeof(sDataIn), psCdol1, iCdol1Len,
          				    gl_sTlvDbTermFixed, gl_sTlvDbTermVar, gl_sTlvDbCard, gl_sTlvDbIssuer);
	ASSERT(iRet >= 0);
	if(iRet < 0)
		return(HXEMV_CORE);
	ucDataInLen = iRet;
	iRet = iTlvMakeAddObj(gl_sTlvDbTermVar, TAG_DFXX_Cdol1DataBlock, ucDataInLen, sDataIn, TLV_CONFLICT_REPLACE);
	ASSERT(iRet >= 0);
	if(iRet < 0)
		return(HXEMV_CORE);

	// ִ��Generate ACָ��
	iEmvSetItemBit(EMV_ITEM_TSI, TSI_02_CARD_RISK_PERFORMED, 1); // IC�����չ�����ִ��, ����GACָ��, Refer to Emv2008 book4 10.1 page 79
	uiRet = uiEmvCmdGenerateAC(ucP1, ucDataInLen, sDataIn, &ucDataOutLen, sDataOut);
	if(uiRet == 1)
		return(HXEMV_CARD_OP);
	if(uiRet)
		return(HXEMV_CARD_SW);

	// ��ȡGAC����
	iRet = iGetACData(ucP1, sDataOut, ucDataOutLen);
	if(iRet)
		return(iRet);

	// check CID, refer to Emv2008 book3 9.3 P88
    iTlvGetObjValue(gl_sTlvDbCard, TAG_9F27_CID, &p);
	if((p[0]&0xC0) == 0xC0)
		return(HXEMV_TERMINATE); // ����ֵ�Ƿ�
	if((ucP1&0xC0) == 0x00) {
		// require AAC
		if((p[0]&0xC0) != 0x00) {
			// ������AACʱû�з���AAC
			return(HXEMV_TERMINATE);
		}
	} else if((ucP1&0xC0) == 0x80) {
		// require ARQC
		if((p[0]&0xC0) == 0x40) {
			// ������ARQCʱ���ܷ���TC
			return(HXEMV_TERMINATE);
		}
	}

	iRet = iTlvGetObjValue(gl_sTlvDbCard, TAG_9F27_CID, &p);
	ASSERT(iRet == 1);
	if(iRet != 1)
		return(HXEMV_CORE); // CID��������ҺϷ�
	ucCid = *p;

	if(iReqCdaFlag && (ucCid&0xC0)!=0x00) {
		// Ҫ��CDA��֤, ��CIDָʾû�з���AAC, ִ��CDA��֤
		iRet = iEmvCda(1/*1:GAC1*/, sDataOut, ucDataOutLen);
		if(iRet == EMV_DAUTH_NATIVE)
			return(HXEMV_CORE);
		if(iRet == EMV_DAUTH_DATA_MISSING)
			return(HXEMV_TERMINATE); // refer Emv bulletin SU-54
		if(iRet) {
			// CDAʧ��, ���CIDָʾ����TC, decline, ��������AAC֮��decline
			iEmvSetItemBit(EMV_ITEM_TVR, TVR_05_CDA_FAILED, 1);
			// ���CDAʧ��, CIDֵ���ܻ�ı�, ��Ҫ���»�ȡ
			iTlvGetObjValue(gl_sTlvDbCard, TAG_9F27_CID, &p);
			ucCid = *p;
			if((ucCid&0xC0) == 0xC0) {
				// ����ֵ�Ƿ�(��ֵԭ��ʾAAR, ����ΪRFU)
				return(HXEMV_TERMINATE);
			}
			if((ucCid&0xC0) != 0x40) {
				// ����TC, ��Ҫ����AAC
				iRet2 = iEmvGAC2("Z1", NULL, NULL, 0, piCardAction); // Z1:Offline declined
				if(iRet2==HXEMV_CORE || iRet2==HXEMV_CARD_OP || iRet2==HXEMV_CARD_SW)
					return(iRet2);
			}
			*piCardAction = GAC_ACTION_AAC; // 1:�ѻ��ܾ�, CDAʧ�ܲ�����Adviceλ
			return(HXEMV_OK);
		}
	} else if(iReqCdaFlag && (ucCid&0xC0)==0x00) {
		// Ҫ��CDA��֤, ��CIDָʾ����AAC
		// ������ΪBulletin N44ָ���ڷ���AACʱ����ΪCDAʧ��, ������2CC.122.00��2CC.122.01Ҫ��GAC(with CDA)����AACʱ��λCDAʧ��λ
//		iEmvSetItemBit(EMV_ITEM_TVR, TVR_05_CDA_FAILED, 1); // modified 20130227
	}

	if((ucCid&0x07) == 0x01) {
	    // CID��adviceλָʾΪ"Service not allowed", ��ֹ����, Refer to Emv2008, book4, 6.3.7
		return(HXEMV_NOT_ACCEPTED);
	}

	switch(ucCid&0xC0) {
	case 0x00: // AAC
		if(ucCid & 0x08)
			*piCardAction = GAC_ACTION_AAC_ADVICE; // �ѻ��ܾ�, ��Advice
		else
			*piCardAction = GAC_ACTION_AAC; // �ѻ��ܾ�, ��Advice
		break;
	case 0x40: // TC
		*piCardAction = GAC_ACTION_TC; // �ѻ�����
		if(iEmvGetECTransFlag() == 1) {
			// ���˽���Ϊ�����ֽ���, ��Ҫ��ȡ���׺�EC���
			// Pboc2.0 Book13, EC�淶, 7.4.6
			// EC���Ӧ��9F10�л�ȡ, ���û�н����Ϣ, �����¶�ȡ
			iRet = iGetECBalanceFrom9F10();
			if(iRet != HXEMV_OK)
				return(iRet);
		}
		break;
	case 0x80: // ARQC
		*piCardAction = GAC_ACTION_ARQC; // Ҫ������
		if(iEmvGetECTransFlag() == 1) { // EC���ױ�־ 0:��EC���� 1:EC���� 2:GPO��Ƭ����EC��Ȩ��,��δ��ECͨ��
			// ���ECash�ѻ�����GAC����ARQC, ˵��Ӧ��δ��ECashͨ��
			iEmvSetECTransFlag(2);
		}
		break;
	default: // ����
		return(HXEMV_TERMINATE);
	}
	return(HXEMV_OK);
}

// �ű�����
// in  : ucScriptType    : ����Ľű�����, 0x71 or 0x72
// ret : HXEMV_OK        : OK
//       HXEMV_CORE      : ��������
// Note: ֧������TDFF1|TDFF2���ֻ��Ŷ����Package��ʽ�ű�, Ҳ֧��T71|T72��������ֱ��POS����ĵ����ű�
//       TDFF1|TDFF2����ʽ����֧�ֶ�T71|T72�ű�
//       ����ִ��T71|T72�����ű�
static int iEmvScriptProc(uchar ucScriptType)
{
	uchar *psPackage;   // 71 or 72 Package(TDFF1 or TDFF2) ����
	int   iPackageLen;  // 71 or 72 Package(TDFF1 or TDFF2) ���� Len
    uchar *psSingle;    // 71 or 72 ����(T71 or T72) ����
    int   iSingleLen;   // 71 or 72 ����(T71 or T72) ���� Len
	uchar *psTemplate;  // 71 or 72 Template
	int   iTemplateLen; // 71 or 72 Template len
	uchar *pT86Value;
	int   iT86ValueLen;
	uchar sRoute[10];
	uchar ucScriptResult;
	uchar *psScriptId;
	uchar sBuf[260];
	int   iErrFlag;
	int   i;
	int   iRet;
	uchar *p;
	uint  uiRet;
    uint  uiApduOutLen;
	int   iProcessedPackageLen; // �Ѿ�����������ݰ�����

	if(ucScriptType!=0x71 && ucScriptType!=0x72)
		return(HXEMV_CORE);
    if(ucScriptType == 0x71) {
		iSingleLen = iTlvGetObj(gl_sTlvDbIssuer, TAG_71_ScriptTemplate1, &psSingle);
		iPackageLen = iTlvGetObjValue(gl_sTlvDbIssuer, TAG_DFXX_ScriptPackage71, &psPackage);
    } else {
		iSingleLen = iTlvGetObj(gl_sTlvDbIssuer, TAG_72_ScriptTemplate2, &psSingle);
		iPackageLen = iTlvGetObjValue(gl_sTlvDbIssuer, TAG_DFXX_ScriptPackage72, &psPackage);
    }

    if(iPackageLen<=0 && iSingleLen<=0)
		return(HXEMV_OK); // ��ָ�����͵Ľű�

	iProcessedPackageLen = 0;
	while(iPackageLen>0 || iSingleLen>0) {
        if(iSingleLen > 0) {
            psTemplate = psSingle;
            iTemplateLen = iSingleLen;
        } else {
            psTemplate = psPackage;
    		iTemplateLen = iTlvCheckTlvObject(psTemplate);
        }
        if(iSingleLen<=0 && (iTemplateLen<0 || iTemplateLen>iPackageLen)) {
		    ASSERT(0);
			vTraceWriteTxt(TRACE_ERR_MSG, "�ű�Package��ʽ��");
			return(HXEMV_OK); // iTemplateLen����Ϸ�, ����ִֹͣ�нű�
        }

		// ��ȡ�ű���ʶ
		strcpy(sRoute, "\xFF""\x9F\x18");
		sRoute[0] = ucScriptType;
		iRet = iTlvSearchObjValue(psTemplate, (ushort)iTemplateLen, 0, sRoute, &psScriptId);
		if(iRet != 4)
			psScriptId = NULL; // ����޽ű���ʶ��ű���ʶ���Ȳ��Ϸ�, ���Խű���ʶ

		// ���ű��Ϸ���, Refer to Test Case V2CJ1940101v4
		//     �ű���ֻ�ܴ��ڽű���ʶTag(T9F18)��ű�ָ��(T86)
		iErrFlag = 0; // 0:no error 1:error
		for(i=0; ; i++) {
			sRoute[0] = ucScriptType;
			strcpy(sRoute+1, "\xFF");
			iRet = iTlvSearchObj(psTemplate, (ushort)iTemplateLen, i, sRoute, &p);
			if(iRet == TLV_ERR_NOT_FOUND)
				break;
			if(iRet < 0) {
				// �ű������Դ�, ���Ըýű�
				iErrFlag = 1;
				break;
			}
			if(memcmp(p, TAG_9F18_ScriptId, 2) && memcmp(p, TAG_86_IssuerScriptCmd, 1)) {
				// ���ڲ�ʶ���Tag, ���Ըýű�
				iErrFlag = 1;
				break;
			}
		}

		if(iErrFlag || iSingleLen>254 || (iSingleLen<=0 && (iProcessedPackageLen+iTemplateLen>254))) {
			// �ű���������Ϊ254�ֽ�(Modified 2013.2.19, ԭ����������Ϊ128, ����Ϊ��������Ϊ254)
			// (DFF1/DFF2�֧��255�ֽڽű�, ������Ҫ����Գ������, ���������󳤶�Ϊ254, �Թ�����255�ֽ������������������)
			// �ű���ʽ�� ���� �ű����ȳ���
			// ���ű���������, refer to test case 2CO.034.02 and 2CO.034.03
			// ���ڹ淶��û������������, �ݰ������ṩ�Ľű���ɼ�����, ������71�ű�ʱֻ�ۼ�71�ű�����, ������72�ű�ʱֻ�ۼ�72�ű�����
			// �ű����ȳ���, ��ִ�иýű�
			iEmvSetItemBit(EMV_ITEM_TSI, TSI_05_SCRIPT_PERFORMED, 1); // ����TSI��Ӧλ

			iRet = iTlvGetObjValue(gl_sTlvDbTermVar, TAG_DF31_ScriptResult, &p);
			if(iRet < 0) {
				// ֮ǰ�޽ű�ִ�н��
				iRet = 0;
			} else {
				// ֮ǰ�нű�ִ�н��
				if(iRet+5 > sizeof(sBuf)) {
				    ASSERT(0);
					return(HXEMV_CORE); // sBuf����������
				}
				memcpy(sBuf, p, iRet);
			}
			sBuf[iRet] = 0x00; // ���հ���Ҫ��, ��0x00
			if(psScriptId)
				memcpy(sBuf+iRet+1, psScriptId, 4);
			else
				memset(sBuf+iRet+1, 0, 4); // �޽ű���ʶ, ��0
			iRet = iTlvMakeAddObj(gl_sTlvDbTermVar, TAG_DF31_ScriptResult, iRet+5, sBuf, TLV_CONFLICT_REPLACE);
        	ASSERT(iRet >= 0);
			if(iRet < 0)
				return(HXEMV_CORE);

			if(ucScriptType == 0x71)
				iEmvSetItemBit(EMV_ITEM_TVR, TVR_34_SCRIPT_FAILED_BEFORE_AC, 1); // �ű�71ִ��ʧ��
			else
				iEmvSetItemBit(EMV_ITEM_TVR, TVR_35_SCRIPT_FAILED_AFTER_AC, 1); // �ű�72ִ��ʧ��

            if(iSingleLen > 0) {
                // ���νű�ִ�е���T7?�ű�
                iSingleLen = 0;
            } else {
                // ���νű�ִ�е���T7?�ű�
    			psPackage += iTemplateLen;
	    		iPackageLen -= iTemplateLen;
		    	iProcessedPackageLen += iTemplateLen;
            }
			continue;
		}
		
		// ����ִ�нű�
		strcpy(sRoute, "\xFF""\x86");
		sRoute[0] = ucScriptType;
		ucScriptResult = 0; // �ű�ִ�н�� ��4λ 0:δ���� 1:����ʧ�� 2:����ɹ�
		                    //              ��4λ 0:δָ�� 1-14:�ű���� 15:15�����ϵĽű����
    	vPushApduPrepare(PUSHAPDU_SCRIPT_EXEC, iTemplateLen, psTemplate); // add by yujun 2012.10.29, ֧���Ż�pboc2.0����apdu
	    for(i=0; ; i++) {
			iT86ValueLen = iTlvSearchObjValue(psTemplate, (ushort)iTemplateLen, (ushort)i, sRoute, &pT86Value);
			if(iT86ValueLen < 0)
				break;
			iEmvSetItemBit(EMV_ITEM_TSI, TSI_05_SCRIPT_PERFORMED, 1); // ����һ���ű���ִ��, ����TSI��Ӧλ
			ucScriptResult = 0x20; // ����ִ�гɹ�
            uiRet = uiEmvCmdExchangeApdu(iT86ValueLen, pT86Value, &uiApduOutLen, sBuf);
			vTraceWriteLastApdu("Apdu�ű�ָ��");
			if(uiRet!=0 && (uiRet&0xFF00)!=0x6200 && (uiRet&0xFF00)!=0x6300) {
				// �ű�ִ�г���
	            i ++;
		        if(i > 15)
			        i = 15;
				ucScriptResult = 0x10 + i;
				if(ucScriptType == 0x71)
					iEmvSetItemBit(EMV_ITEM_TVR, TVR_34_SCRIPT_FAILED_BEFORE_AC, 1); // �ű�71ִ��ʧ��
				else
					iEmvSetItemBit(EMV_ITEM_TVR, TVR_35_SCRIPT_FAILED_AFTER_AC, 1); // �ű�72ִ��ʧ��
				break;
		    }
		}
		if(ucScriptResult != 0) {
			// ����һ���ű�ָ�ִ��, ����ִ�н��
			// �ű�ִ�����, �����ű�ִ�н��TLV����, ��Ϊ�����ж���ű�, ��Ҫȡ��֮ǰ���ܵĽű�ִ�н��, �����νű�ִ�н����ӽ�ȥ
			iRet = iTlvGetObjValue(gl_sTlvDbTermVar, TAG_DF31_ScriptResult, &p);
			if(iRet < 0) {
				// ֮ǰ�޽ű�ִ�н��
				iRet = 0;
			} else {
				// ֮ǰ�нű�ִ�н��
				if(iRet+5 > sizeof(sBuf)) {
				    ASSERT(0);
					return(HXEMV_CORE); // sBuf����������
				}
				memcpy(sBuf, p, iRet);
			}
			sBuf[iRet] = ucScriptResult;
			if(psScriptId)
				memcpy(sBuf+iRet+1, psScriptId, 4);
			else
				memset(sBuf+iRet+1, 0, 4); // �޽ű���ʶ, ��0
			iRet = iTlvMakeAddObj(gl_sTlvDbTermVar, TAG_DF31_ScriptResult, iRet+5, sBuf, TLV_CONFLICT_REPLACE);
        	ASSERT(iRet >= 0);
			if(iRet < 0)
				return(HXEMV_CORE);
		}
        if(iSingleLen > 0) {
            // ���νű�ִ�е���T7?�ű�
            iSingleLen = 0;
        } else {
            // ���νű�ִ�е���T7?�ű�
			psPackage += iTemplateLen;
       		iPackageLen -= iTemplateLen;
	    	iProcessedPackageLen += iTemplateLen;
        }
	} // while(iPackageLen > 0

	return(HXEMV_OK);
}

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
//       HXEMV_NOT_ACCEPTED: ������
int iEmvGAC2(uchar *pszArc, uchar *pszAuthCode, uchar *psIssuerData, int iIssuerDataLen, int *piCardAction)
{
	uint  uiRet;
	int   iRet;
	uchar szArc[2+1]; // Ӧ����
	uchar *p;
	int   iCdol2Len;
	uchar *psCdol2;
	int   iReqCdaFlag; // Ҫ��CDA��־, 0:��ҪCDA, 1:ҪCDA
	int   iTermAction; // �ն���Ϊ, 1:�ܾ� 3:�ѻ�
	uchar ucP1; // GACָ�����P1
	uchar ucDataInLen, sDataIn[256];
	uchar ucDataOutLen, sDataOut[256];
	uchar ucCid;
	uchar ucCidIllegalFlag; // CID�Ƿ���־ 0:CID���� 1:CID�Ƿ�(����AAC�����˷�AAC, ����TC�����˷�TC�ҷ�AAC)
	uchar sBuf[260];

	if(psIssuerData) {
		// ����55�򷢿��з�������
		iRet = iTlvBatchAddField55Obj(1/*������ж���*/, gl_sTlvDbIssuer, psIssuerData, iIssuerDataLen, TLV_CONFLICT_ERR);
		if(iRet==TLV_ERR_BUF_SPACE || iRet==TLV_ERR_BUF_FORMAT) {
		    ASSERT(0);
			return(HXEMV_CORE);
		}
		if(iRet < 0) {
			// ��������������, ��Ϊ����ʧ��
			pszArc = NULL;
			pszAuthCode = NULL;
			psIssuerData = NULL;
			iIssuerDataLen = 0;
			iTlvDelObj(gl_sTlvDbIssuer, TAG_91_IssuerAuthData);
			iTlvDelObj(gl_sTlvDbIssuer, TAG_DFXX_ScriptPackage71);
			iTlvDelObj(gl_sTlvDbIssuer, TAG_DFXX_ScriptPackage72);
		} 
	}
	// �ж��ն���Ϊ
	memset(szArc, 0, 3);
	if(pszArc)
		if(pszArc[0])
			memcpy(szArc, pszArc, 2); // �����ɹ�, ��̨����ARC
	if(szArc[0] == 0) {
		// ����ʧ��, �ж��ն�ȱʡ��Ϊ
		iTermAction = iTermActionAnalysis(1/*1:�ж��ն�ȱʡ��Ϊ*/); // 0:���� 1:�ܾ� 3:�ѻ�
		if(iTermAction <= 0) {
		    ASSERT(0);
			return(HXEMV_CORE);
		}
		if(iTermAction == 3) {
			// �ն�ȱʡ��Ϊ������ɽ���
			if(iEmvTermCommunication() == TERM_COM_OFFLINE_ONLY)
				strcpy(szArc, "Y1"); // offline approved Y1
			else
				strcpy(szArc, "Y3"); // Unable to go online, offline approved Y3
		} else {
			// �ն�ȱʡ��ΪҪ��ܾ�����
			if(iEmvTermCommunication() == TERM_COM_OFFLINE_ONLY)
				strcpy(szArc, "Z1"); // offline declined Z1
			else
				strcpy(szArc, "Z3"); // Unable to go online, offline declined Z3
		}
	} else {
		// �����ɹ�, ���ݷ�����ARC�����ն���Ϊ(עARC=="Z1" || ARC=="Y1"ʱ��û������)
		//   ���岿��.��Ƭ�淶 (16.6, P55) & (��A1, P76)
		//   ��Ȩ��Ӧ��Ϊ00��10 ��11 ���������н��ܽ���
		//   ��Ȩ��Ӧ��Ϊ01 ��02 ��������������ο�
		//   ����ֵ���������оܾ�����Ƭ�����ն������׾ܾ����д���
		if(strcmp(szArc, "00")==0 || strcmp(szArc, "10")==0 || strcmp(szArc, "11")==0 || strcmp(szArc, "Y1")==0)
			iTermAction = 3; // ����
		else if(strcmp(szArc, "01")==0 || strcmp(szArc, "02")==0) {
			// ����������ο�, ���Ĳ�֧�ֲο�, ���������ж��ǽ��ܻ��Ǿܾ�
			iRet = iTlvGetObjValue(gl_sTlvDbTermFixed, TAG_DFXX_VoiceReferralSupport, &p); // 1:֧�� 2:��֧��,ȱʡapprove 3:��֧��,ȱʡdecline
			if(iRet != 1)
				return(HXEMV_CORE); // �����вο�ȱʡ���ñ������
			if(*p == 2)
				iTermAction = 3; // ����
			else if(*p == 3)
				iTermAction = 1; // �ܾ�
			else {
            	ASSERT(0);
				return(HXEMV_CORE); // ���Ĳ�֧�ַ����вο�
			}

		} else
			iTermAction = 1; // �ܾ�
	}

	// ���淢��������
	// ����ARC
	iTlvMakeAddObj(gl_sTlvDbIssuer, TAG_8A_AuthRspCode, 2, szArc, TLV_CONFLICT_REPLACE);
	// ������Ȩ��
	if(pszAuthCode) {
		if(strlen(pszAuthCode) == 6)
			iTlvMakeAddObj(gl_sTlvDbIssuer, TAG_89_AuthCode, 6, pszAuthCode, TLV_CONFLICT_REPLACE);
	}

	// �ⲿ��֤����
	if(iEmvTestItemBit(EMV_ITEM_AIP, AIP_05_ISSUER_AUTH_SUPPORED)) {
		// AIPָʾ֧�ַ�������֤
		iRet = iTlvGetObjValue(gl_sTlvDbIssuer, TAG_91_IssuerAuthData, &p);
		if(iRet > 0) {
			// �����з�������֤����
			iEmvSetItemBit(EMV_ITEM_TSI, TSI_03_ISSUER_AUTH_PERFORMED, 1); // ����TSI��������ִ֤��λ
			uiRet = uiEmvCmdExternalAuth((uchar)iRet, p);
			if(uiRet == 1)
				return(HXEMV_CARD_OP);
			if(uiRet) {
				// �ⲿ��֤ʧ��, ����TVR�ⲿ��֤ʧ��λ, Refer to Emv2008 book3, Annex F, P177
                //                                               Emv2008 book3, 10.9, P120
				iEmvSetItemBit(EMV_ITEM_TVR, TVR_33_ISSUER_AUTH_FAILED, 1);
			}
		}
	}

	// Script71 ����
	iRet = iEmvScriptProc(0x71); // 0x71��ʾ����ű�71
	if(iRet)
		return(iRet);

	// ׼������
	// �����
	vGetRand(sBuf, 4);
	iTlvMakeAddObj(gl_sTlvDbTermVar, TAG_9F37_TermRand, 4, sBuf, TLV_CONFLICT_REPLACE);
	// ��CDOL2������TC Hash Value��, �������, ׼��TC Hash Value
	iCdol2Len = iTlvGetObjValue(gl_sTlvDbCard, TAG_8D_CDOL2, &psCdol2);
	ASSERT(iCdol2Len > 0);
	if(iCdol2Len <= 0)
		return(HXEMV_CORE); // CDOL2�������
    iRet =  iTlvSearchDOLTag(psCdol2, iCdol2Len, TAG_98_TCHashValue, NULL);
	if(iRet > 0) {
		// CDOL2Ҫ��TC Hash Value, ����TC Hash Value
		iRet = iGenTCHashValue();
		if(iRet)
			return(iRet);
	}

	iReqCdaFlag = 1; // ���ȼ���Ҫ��CDA
	if(iEmvTestItemBit(EMV_ITEM_AIP, AIP_07_CDA_SUPPORTED)==0
			|| iEmvTestItemBit(EMV_ITEM_TERM_CAP, TERM_CAP_20_CDA)==0)
		iReqCdaFlag = 0; // ֻ���ն��뿨Ƭ��֧��CDAʱ��ִ��CDA
	if(iEmvTestItemBit(EMV_ITEM_TVR, TVR_05_CDA_FAILED))
		iReqCdaFlag = 0; // CDA׼���׶��Ѿ�ʧ��, ��ִ��CDA

	switch(iTermAction) {
	case 1: // Denial
        ucP1 = 0x00; // require AAC
		if(iReqCdaFlag)
			iReqCdaFlag = 0; // ����AAC��ִ��CDA
		break;
	case 3: // Accept
		ucP1 = 0x40; // require TC (01XX XXXX)
		break;
	}
	if(iReqCdaFlag)
		ucP1 |= 0x10; // CDA request (XXX1 XXXX)

	if(iReqCdaFlag)
		iEmvSetItemBit(EMV_ITEM_TSI, TSI_00_DATA_AUTH_PERFORMED, 1); // ���������CDA, ����TSI�ѻ�������ִ֤�б�־

    // ���CDOL2Ŀ�괮
	iRet = iTlvMakeDOLBlock(sDataIn/*CDOL2Ŀ�괮*/, sizeof(sDataIn), psCdol2, iCdol2Len,
          				    gl_sTlvDbTermFixed, gl_sTlvDbTermVar, gl_sTlvDbCard, gl_sTlvDbIssuer);
	ASSERT(iRet >= 0);
	if(iRet < 0)
		return(HXEMV_CORE);
	ucDataInLen = iRet;
	iRet = iTlvMakeAddObj(gl_sTlvDbTermVar, TAG_DFXX_Cdol2DataBlock, ucDataInLen, sDataIn, TLV_CONFLICT_REPLACE);
	ASSERT(iRet >= 0);
	if(iRet < 0)
		return(HXEMV_CORE);

	// ִ��Generate ACָ��
	uiRet = uiEmvCmdGenerateAC(ucP1, ucDataInLen, sDataIn, &ucDataOutLen, sDataOut);
	if(uiRet == 1)
		return(HXEMV_CARD_OP);
	if(uiRet)
		return(HXEMV_CARD_SW);
	iEmvSetItemBit(EMV_ITEM_TSI, TSI_02_CARD_RISK_PERFORMED, 1); // IC�����չ�����ִ��

	// ��ȡGAC����
	iRet = iGetACData(ucP1, sDataOut, ucDataOutLen);
	if(iRet)
		return(iRet);

	// check CID, refer to Emv2008 book3 9.3 P88
	ucCidIllegalFlag = 0; // ���ȼ���CID�Ϸ�
    iTlvGetObjValue(gl_sTlvDbCard, TAG_9F27_CID, &p);
	if((ucP1&0xC0) == 0x00) {
		// require AAC
		if((p[0]&0xC0) != 0x00) {
			// ������AACʱû�з���AAC
			ucCidIllegalFlag = 1; // CID�Ƿ�
		}
	} else {
		// require TC
		if((p[0]&0xC0)!=0x40 && (p[0]&0xC0)!=0x00) {
			// ������TCʱû�з���TCҲû�з���AAC
			ucCidIllegalFlag = 1; // CID�Ƿ�
		}
	}

	iRet = iTlvGetObjValue(gl_sTlvDbCard, TAG_9F27_CID, &p);
	ASSERT(iRet == 1);
	if(iRet != 1)
		return(HXEMV_CORE); // CID��������ҺϷ�
	ucCid = *p;

	if(iReqCdaFlag && (ucCid&0xC0)!=0x00 && ucCidIllegalFlag!=1) {
		// Ҫ��CDA��֤, ��CIDָʾû�з���AAC, ��CIDû�зǷ�, ִ��CDA��֤
		iRet = iEmvCda(2/*2:GAC2*/, sDataOut, ucDataOutLen);
		if(iRet == EMV_DAUTH_NATIVE)
			return(HXEMV_CORE);
		if(iRet == EMV_DAUTH_DATA_MISSING)
			return(HXEMV_TERMINATE); // refer Emv bulletin SU-54
		if(iRet) {
			// CDAʧ��
			iEmvSetItemBit(EMV_ITEM_TVR, TVR_05_CDA_FAILED, 1);
			// ���CDAʧ��, CIDֵ���ܻ�ı�, ��Ҫ���»�ȡ
			iTlvGetObjValue(gl_sTlvDbCard, TAG_9F27_CID, &p);
			ucCid = *p;
			if((ucCid&0xC0) != 0xC0) // 0xC0ԭ��ʾAAR, ����ΪRFU
				*piCardAction = GAC_ACTION_AAC; // 1:�ܾ�, CDAʧ�ܲ�����Adviceλ
			else
				*piCardAction = GAC_ACTION_AAC; // �ܾ�, CDAʧ�ܲ�����Adviceλ, refer to Emv2008 book3 9.3 P88
												// ��ʹ������RFU, �ڶ���GACҲ��Ϊ��AAC, modified 20131009
			// ��֪��CDAʧ���Ƿ���Ҫִ�нű� ????
			return(HXEMV_OK);
		}
	} else if(iReqCdaFlag && (ucCid&0xC0)==0x00) {
		// Ҫ��CDA��֤, ��CIDָʾ����AAC
		// ������ΪBulletin N44ָ���ڷ���AACʱ����ΪCDAʧ��, ������2CC.122.00��2CC.122.01Ҫ��GAC(with CDA)����AACʱ��λCDAʧ��λ
//		iEmvSetItemBit(EMV_ITEM_TVR, TVR_05_CDA_FAILED, 1); // modified 20130227
	}

	// Script72 ����
	iRet = iEmvScriptProc(0x72); // 0x72��ʾ����ű�72
	if(iRet)
		return(iRet);

	// ����cid
	if((ucCid&0x07) == 0x01) {
	    // CID��adviceλָʾΪ"Service not allowed", ��ֹ����, Refer to Emv2008, book4, 6.3.7
		return(HXEMV_NOT_ACCEPTED);
	}

	if((ucCid&0xC0)==0x00 || ucCidIllegalFlag==1) {
		// CIDָʾΪAAC �� CID�Ƿ�(��ʱ��ͬ�ڷ���AAC)
		if(ucCid & 0x08)
			*piCardAction = GAC_ACTION_AAC_ADVICE; // �ѻ��ܾ�, ��Advice
		else
			*piCardAction = GAC_ACTION_AAC; // �ѻ��ܾ�, ��Advice
	} else {
		// CIDָʾ����TC
		*piCardAction = GAC_ACTION_TC; // ����
	}

	return(HXEMV_OK);
}
