/**************************************
File name     : EmvData.c
Function      : EMV/pboc2.0����Ǻ�������
Author        : Yu Jun
First edition : Mar 29th, 2012
Modified      : 
**************************************/
/*
ģ����ϸ����:
	EMV�������ݿ�
.	�ն˲�����֧�ֵ�����CA��Կ��֧�ֵ�����Ӧ�ö�����������
	��Щ������Ҫ�ն����㹻���ڴ�ռ�, �������ڵ��ն��ڴ涼���Ǵ�����
.	�����ն��뿨Ƭƥ���Ӧ���б�
.	����4�����͵�TLV���ݿ�
.	�������Ҫ���ڴ�ռ�
			   4096
			   4096
			   2048
	          10240
	 100*18 =  1800
	120*268 = 32160
	100*68  =  6800    +
   ----------------------
			     61K�ֽ� 
*/
#include "VposFace.h"
#include "EmvCore.h"

int   gl_iCoreStatus;                       // ���ĵ�ǰ״̬

int   gl_iCoreCallFlag;                     // ���ĵ��÷�ʽ, HXCORE_CALLBACK      : ָ��ʹ�ûص��ӿ�
											//               HXCORE_NOT_CALLBACK  : ָ��ʹ�÷ǻص��ӿ�
int   gl_iCardType;							// ��ǰ�����Ŀ�Ƭ����, ���ĸ��ݴ�ȫ�ֱ�����ȷ�������Ŀ�����
											//				 EMV_CONTACT_CARD	  : �Ӵ���
											//				 EMV_CONTACTLESS_CARD : �ǽӿ�
int   gl_iAppType;							// Ӧ�ó����־, 0:�ͼ����(�ϸ����ع淶) 1:Ӧ�ó���(ʵ��Ӧ�ó���)
											//    ����: 1. �ͼ����ֻ֧��ͨ��L2���Ľ���, Ӧ�ó���֧�����н���


// ����Tlv���ݿ�����
uchar gl_sTlvDbTermFixed[4096];             // �ն˲�����һ�����ã����ٸı�
uchar gl_sTlvDbTermVar[4096];               // �ն˿��ƵĽ����м�����
uchar gl_sTlvDbIssuer[2048];                // ����������
uchar gl_sTlvDbCard[10240];                 // ��Ƭ����

// �ն�֧�ֵ�Aid�б�
int   gl_iTermSupportedAidNum;              // �ն˵�ǰ֧�ֵ�Aid�б���Ŀ
stTermAid gl_aTermSupportedAidList[100];    // �ն�֧�ֵ�Aid�б�(ռ��100*18�ֽ�)
// �ն�֧�ֵ�CA��Կ�б�
int   gl_iTermSupportedCaPublicKeyNum;      // �ն˵�ǰ֧�ֵ�CA��Կ��Ŀ
stCAPublicKey gl_aCAPublicKeyList[120];     // �ն˵�ǰ֧�ֵ�CA��Կ�б�(ռ��120*268�ֽ�)
// ��Ƭ���ն�ͬʱ֧�ֵ�Ӧ��
int   gl_iCardAppNum;                       // �ն��뿨Ƭͬʱ֧�ֵ�Ӧ����Ŀ
stADFInfo gl_aCardAppList[100];             // �ն��뿨Ƭͬʱ֧�ֵ�Ӧ���б�(ռ��100*68�ֽ�), cAdfNameLen==0��ʾ�б�β��
