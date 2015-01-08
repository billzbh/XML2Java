/**************************************
File name     : PbocCtls.c
Function      : �ǽӿ����Ĵ���ģ��
Author        : Yu Jun
First edition : May 23rd, 2013
Modified      : 
**************************************/
/*
ģ����ϸ����:
	Pboc��׼�ǽӴ������Ĵ���ģ��
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "VposFace.h"
#include "Iso4217.h"
#include "Arith.h"
#include "Sha.h"
#include "Common.h"
#include "EmvCmd.h"
#include "TlvFunc.h"
#include "EmvFunc.h"
#include "TagAttr.h"
#include "EmvData.h"
#include "EmvCore.h"
#include "EmvMsg.h"
#include "EmvPara.h"
#include "EmvSele.h"
#include "EmvDAuth.h"
#include "TagDef.h"
#include "EmvTrace.h"

static uchar sg_sCtlsAttr[4];		 // ���һ�ηǽӽ���Ԥ�����ó����ն˽�������
static uchar sg_szAmount[13] = {0};	 // Ԥ���������Ȩ���

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
int iPbocCtlsPreProc(uchar *pszAmount, uint uiCurrencyCode)
{
	uchar *psTlvObjValue;
	int   iRet;
	uchar szBuf[100];

	if(strlen(pszAmount) > 12)
		return(HXEMV_CORE);
	strcpy(sg_szAmount, pszAmount);

	// T9F66��ʽ: B1, b7-֧�ַǽ�pboc b6-֧��qPboc b5-֧�ֽӴ�pboc b4-��֧���ѻ� b3-֧������Pin b2-֧��ǩ��
	//            B2, b8-Ҫ���������� b7-Ҫ��CVM
	//            B3, Ԥ��
	//            B4, b8-֧��'01'�汾fDDA
	memcpy(sg_sCtlsAttr, "\x7E\x00\x00\x80", 4); // ����֧�ֵĹ��ܱ�ʾλ(B1��B4), B2Ҫ���������ж��Ƿ���λ
	// 1.����ն�����Ϊ֧��״̬��飬������Ȩ���Ϊһ�����ҵ�λ������״̬���Ҫ��ģ�������
	//   �����ն˽��������ֽ�2�еĵ�8λ��ʾ��Ҫ����Ӧ�����ġ�֧��״̬���Ӧ��һ�����õ�ѡ
	//   ���ʵʩʱӦ�򿪲��ܲ��������ּ���ȱʡ��ΪΪ�ر�

	//   ����һ�����ҵ�λ
	iIso4217SearchDigitCode(uiCurrencyCode, szBuf/*ignore ret value*/, &iRet); // ȡС����λ��
	strcpy(szBuf, "1");
	while(iRet--)
		strcat(szBuf, "0");
	if(0/*�ݲ�֧��״̬���,���֧��,�ü���־�滻*/ && iStrNumCompare(pszAmount, szBuf, 10)==0)
		vSetStrBit2(sg_sCtlsAttr, 1/*B2*/, 8/*b8*/, 1); // ����Ҫ������λ
	// 2.�����Ȩ���Ϊ�㣬�����ն�֧��qPBOC��չӦ�ã����������������ն�Ӧ���ն˽���������
	//   ��2�ĵ�8λ��ʾҪ������Ӧ������
	if(iStrNumCompare(pszAmount, "0", 10)) {
		// ��Ȩ��� == 0, ��֧��qPboc��չ
		if(iEmvTermCommunication() != TERM_COM_OFFLINE_ONLY)
			vSetStrBit2(sg_sCtlsAttr, 1/*B2*/, 8/*b8*/, 1); // ����������, ����Ҫ������λ
	}
	// 3.�����Ȩ���Ϊ�㣬�����ն�֧��qPBOC��չӦ�ã���֧���ѻ����ն�Ӧ��ֹ���ף���ʾ�ֿ�
	//   ��ʹ����һ�ֽ��棨������ڣ�
	if(iStrNumCompare(pszAmount, "0", 10)) {
		// ��Ȩ��� == 0, ��֧��qPboc��չ
		if(iEmvTermCommunication() == TERM_COM_OFFLINE_ONLY)
			return(HXEMV_TRY_OTHER_INTERFACE); // Ӧ�ò����о�����������
	}
	// 4.�����Ȩ�����ڻ�����ն˷ǽӴ������޶������ڣ������ն�Ӧ��ʾ�ֿ��˲�����һ�ֽ��淽ʽ
	iRet = iTlvGetObjValue(gl_sTlvDbTermFixed, TAG_DFXX_TermCtlsAmountLimit, &psTlvObjValue);
	if(iRet > 0) {
		// ���ڷǽӴ������޶�
		vOneTwo0(psTlvObjValue, iRet, szBuf);
		if(iStrNumCompare(pszAmount, szBuf, 10) >= 0)
			return(HXEMV_TRY_OTHER_INTERFACE); // Ӧ�ò����о�����������
	}
	// 5.�����Ȩ�����ڻ�����ն�ִ��CVM�޶������ڣ������ն�Ӧ���ն˽��������б�ʾ
	//   Ҫ��CVM����2�ֽڵ�7λ���Լ�֧�ֵ�CVM���ࡣ�����ֵ�ǰ�汾֧������PIN����1
	//   �ֽڵ�3λ����ǩ������1�ֽڵ�2λ��
	iRet = iTlvGetObjValue(gl_sTlvDbTermFixed, TAG_DFXX_TermCtlsCvmLimit, &psTlvObjValue);
	if(iRet > 0) {
		// ���ڷǽӴ������޶�
		vOneTwo0(psTlvObjValue, iRet, szBuf);
		if(iStrNumCompare(pszAmount, szBuf, 10) >= 0)
			vSetStrBit2(sg_sCtlsAttr, 1/*B2*/, 7/*b7*/, 1); // Ҫ��CVM
	}
	// 6.�����Ȩ����ǩ��9F02�������ڷǽӴ��ն��ѻ�����޶������ǽӴ��ն��ѻ����
	//   �޶���ڣ����õ��ն�����޶��ǩ��9F1B���������ն�Ӧ���ն˽������Ե�2�ֽڵ�8
	//   λ��ʾ��Ҫ����Ӧ������
	iRet = iTlvGetObjValue(gl_sTlvDbTermFixed, TAG_DFXX_TermCtlsOfflineAmountLimit, &psTlvObjValue);
	if(iRet <= 0) {
		// �޷ǽ��ѻ�����޶�, �����ն��޶�
		iRet = iTlvGetObjValue(gl_sTlvDbTermFixed, TAG_9F1B_TermFloorLimit, &psTlvObjValue);
		if(iRet <= 0) {
			// ע:���淶,Floor LimitΪaid��ز���, ��ʱ�ò���, ��˷ǽ��ѻ�����޶�ͱ������, ������ֹ�ǽӽ���
			return(HXEMV_TRY_OTHER_INTERFACE); // Ӧ�ò����о�����������
		}
	}
	// ���ڷǽ��ѻ������޶�(�������ն��޶�)
	vOneTwo0(psTlvObjValue, 6, szBuf);
	if(iStrNumCompare(pszAmount, szBuf, 10) > 0)
		vSetStrBit2(sg_sCtlsAttr, 1/*B2*/, 8/*b7*/, 1); // Ҫ������

	// ��ȡ�ն˷ǽ���������λ, �����ǽ��ն˽�������
	iRet = iTlvGetObjValue(gl_sTlvDbTermFixed, TAG_DFXX_TermCtlsCapMask, &psTlvObjValue);
	if(iRet != 4)
		return(HXEMV_CORE); // �ǽ���������λ�������
	vAnd(sg_sCtlsAttr, psTlvObjValue, 4);
	return(HXEMV_OK);
}

// ��ȡ�ն˽�������
// ret : 9F66ֵ, ��ȡǰ������Ԥ����
uchar *psPbocCtlsGet9F66(void)
{
	return(sg_sCtlsAttr);
}

// ��ȡԤ������Ľ��
// ret : Ԥ������Ľ��
uchar *pszPbocCtlsGetAmount(void)
{
	return(sg_szAmount);
}

// ��ȡ����·��
// ret : �����ߵ�·��, 0:�Ӵ�Pboc(��׼DC��EC) 1:�ǽӴ�Pboc(��׼DC��EC) 2:qPboc���� 3:qPboc�ѻ� 4:qPboc�ܾ�
// Note : �ǽ�GPO�������ȡ��ֵ��������
int iPbocCtlsGetRoute(void)
{
	int   iRet;
	uchar *p;
	iRet = iTlvGetObjValue(gl_sTlvDbTermVar, TAG_DFXX_TransRoute, &p);
	if(iRet <= 0)
		return(0); // �޴�����, �����ߵ��ǽӴ�pboc
	return(*p);
}

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
int iPbocCtlsGpoAnalyse(int *piTransRoute, int *piSignFlag, int *piNeedOnlinePin)
{
	uchar *ps9F6CValue;
	int   i9F6CLen;

	*piSignFlag = 0;
	*piNeedOnlinePin = 0;

	*piTransRoute = iPbocCtlsGetRoute(); // 0:�Ӵ�Pboc(��׼DC��EC) 1:�ǽӴ�Pboc(��׼DC��EC) 2:qPboc���� 3:qPboc�ѻ� 4:qPboc�ܾ�
	if(*piTransRoute==0 || *piTransRoute==1)
		return(HXEMV_OK); // pboc��pboc����

	i9F6CLen = iTlvGetObjValue(gl_sTlvDbCard, TAG_9F6C_CardTransQualifiers, &ps9F6CValue);

	if(*piTransRoute == 2) {
		// qPboc����, �ο�JR/T0025.12��2013, 7.8.3 P41
		if(i9F6CLen == 2) {
			// �ɹ�������9F6C
			if(iTestStrBit2(ps9F6CValue, 0, 8))
				*piNeedOnlinePin = 1; // ��Ҫ����PIN
			if(iTestStrBit2(ps9F6CValue, 0, 7))
				*piSignFlag = 1; // ��Ҫǩ��
		} else {
			// û�гɹ�����9F6C
			if(iEmvTestItemBit(EMV_ITEM_TERM_CAP, TERM_CAP_10_SIGNATURE))
				*piSignFlag = 1; // ��Ҫǩ��, refer JR/T0025.12��2013, 7.6 p21
		}
		return(HXEMV_OK); // ��qPboc��������
	}

	if(*piTransRoute == 3) {
		// qPboc�ѻ�, �ο�JR/T0025.12��2013, 7.8.2 P41
		if(i9F6CLen == 2) {
			// �ɹ�������9F6C
			if(iTestStrBit2(ps9F6CValue, 0, 7))
				*piSignFlag = 1; // ��Ҫǩ��
		} else {
			// û�гɹ�����9F6C
			if(iEmvTestItemBit(EMV_ITEM_TERM_CAP, TERM_CAP_10_SIGNATURE))
				*piSignFlag = 1; // ��Ҫǩ��, refer JR/T0025.12��2013, 7.6 p21
		}
		return(HXEMV_OK); // ��qPboc��������
	}

	if(*piTransRoute == 4) {
		return(HXEMV_OK); // �ܾ�
	}

	ASSERT(0);
	return(HXEMV_CORE);
}
