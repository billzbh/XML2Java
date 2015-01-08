/**************************************
File name     : EmvData.h
Function      : EMV/pboc2.0����Ǻ�������
Author        : Yu Jun
First edition : Mar 29th, 2012
Modified      : Apr 2nd, 2014
				����EMV_STATUS_PROC_RISTRICTIONS�궨��
**************************************/
#ifndef _EMVDATA_H
#define _EMVDATA_H
#include "EmvCore.h"

// ����״̬����
#define EMV_STATUS_INIT						     0  // ���ĳ�ʼ�����
#define EMV_STATUS_SET_PARA					    10  // �ն˲����������
#define EMV_STATUS_TRANS_INIT				    20  // ���׳�ʼ�����
#define EMV_STATUS_RESET						30  // ��Ƭ��λ���
#define EMV_STATUS_GET_SUPPORTED_APP            40  // ��ȡ֧�ֵ�Ӧ�����
#define EMV_STATUS_APP_SELECT				    50  // Ӧ��ѡ�����
#define EMV_STATUS_GPO						    60  // GPO���
#define EMV_STATUS_READ_REC					    70  // ����¼�������
#define EMV_STATUS_PROC_RISTRICTIONS			75  // �����������
#define EMV_STATUS_TERM_ACTION_ANALYSIS			80  // �ն˷��չ������
#define EMV_STATUS_GAC1							90  // GAC1���
#define EMV_STATUS_GAC2							100 // GAC2���

#define EMV_CONTACT_CARD						0	// �Ӵ�������
#define EMV_CONTACTLESS_CARD					1	// �ǽӿ�����

extern int   gl_iCoreStatus;                        // ���ĵ�ǰ״̬
extern int   gl_iCoreCallFlag;						// ���ĵ��÷�ʽ, HXCORE_CALLBACK or HXCORE_NOT_CALLBACK
extern int   gl_iCardType;							// ��ǰ�����Ŀ�Ƭ����, EMV_CONTACT_CARD	or EMV_CONTACTLESS_CARD
extern int   gl_iAppType;							// Ӧ�ó����־, 0:�ͼ����(�ϸ����ع淶) 1:Ӧ�ó���(ʵ��Ӧ�ó���)
													//    ����: 1. �ͼ����ֻ֧��ͨ��L2���Ľ���, Ӧ�ó���֧�����н���

// ����Tlv���ݿ�����
extern uchar gl_sTlvDbTermFixed[4096];              // �ն˲�����һ�����ã����ٸı�
extern uchar gl_sTlvDbTermVar[4096];                // �ն˿��ƵĽ����м�����
extern uchar gl_sTlvDbIssuer[2048];                 // ����������
extern uchar gl_sTlvDbCard[10240];                  // ��Ƭ����

// �ն�֧�ֵ�Aid�б�
extern int   gl_iTermSupportedAidNum;               // �ն˵�ǰ֧�ֵ�Aid�б���Ŀ
extern stTermAid gl_aTermSupportedAidList[100];     // �ն�֧�ֵ�Aid�б�(ռ��100*18�ֽ�)
// �ն�֧�ֵ�CA��Կ�б�
extern int   gl_iTermSupportedCaPublicKeyNum;       // �ն˵�ǰ֧�ֵ�CA��Կ��Ŀ
extern stCAPublicKey gl_aCAPublicKeyList[120];      // �ն˵�ǰ֧�ֵ�CA��Կ�б�(ռ��120*268�ֽ�)
// ��Ƭ���ն�ͬʱ֧�ֵ�Ӧ��
extern int   gl_iCardAppNum;                        // �ն��뿨Ƭͬʱ֧�ֵ�Ӧ����Ŀ
extern stADFInfo gl_aCardAppList[100];              // �ն��뿨Ƭͬʱ֧�ֵ�Ӧ���б�(ռ��100*68�ֽ�)

#endif
