/**************************************
File name     : EmvMCode.h
Function      : EMV��ʾ��Ϣ�����
Author        : Yu Jun
First edition : Apr 6th, 2012
Modified      : 
**************************************/
#ifndef _EMVMCODE_H
#define _EMVMCODE_H

// ��׼��ϢID��EMVMSG_XX_DESC
//             XX  :��׼��Ϣ����(emv2008 book4 11.2 (P88))
//             DESC:��Ϣ����
#define EMVMSG_00_NULL				0x00 // ����Ϣ, ����Ҫ��ʾ
#define EMVMSG_01_AMOUNT			0x01
#define EMVMSG_02_AMOUNT_OK		    0x02
#define EMVMSG_03_APPROVED			0x03
#define EMVMSG_04_CALL_YOUR_BANK	0x04
#define EMVMSG_05_CANCEL_OR_ENTER   0x05
#define EMVMSG_06_CARD_ERROR		0x06
#define EMVMSG_07_DECLINED			0x07
#define EMVMSG_08_ENTER_AMOUNT		0x08
#define EMVMSG_09_ENTER_PIN			0x09
#define EMVMSG_0A_INCORRECT_PIN		0x0A
#define EMVMSG_0B_INSERT_CARD		0x0B
#define EMVMSG_0C_NOT_ACCEPTED		0x0C
#define EMVMSG_0D_PIN_OK			0x0D
#define EMVMSG_0E_PLEASE_WAIT		0x0E
#define EMVMSG_0F_PROCESSING_ERROR	0x0F
#define EMVMSG_10_REMOVE_CARD		0x10
#define EMVMSG_11_USE_CHIP_REAEDER	0x11
#define EMVMSG_12_USE_MAG_STRIPE	0x12
#define EMVMSG_13_TRY_AGAIN			0x13

// �Զ�����ϢID��EMVMSG_DESC
//               DESC  :��Ϣ����
#define EMVMSG_CLEAR_SCR            0x0100	// ����
#define EMVMSG_NATIVE_ERR           0x0101  // �ڲ�����
#define EMVMSG_APPCALL_ERR			0x0102  // Ӧ�ò����
#define EMVMSG_SELECT_LANG          0x0103	// ѡ��ֿ�������(���Ĳ�֧��ѡ�����Ա����)
#define EMVMSG_SELECT_APP           0x0104	// ѡ��Ӧ��
#define EMVMSG_ENTER_PIN_OFFLINE    0x0105  // �����ѻ�����
#define EMVMSG_ENTER_PIN_ONLINE     0x0106  // ������������
#define EMVMSG_LAST_PIN				0x0107  // ���һ����������
#define EMVMSG_SERVICE_NOT_ALLOWED  0x0108  // ��������
#define EMVMSG_TRANS_NOT_ALLOWED    0x0109  // ���ײ�֧��
#define EMVMSG_CANCEL               0x010A  // ��ȡ��
#define EMVMSG_TIMEOUT              0x010B  // ��ʱ
#define EMVMSG_TERMINATE            0x010C  // ��ֹ����
#define EMVMSG_TRY_OTHER_INTERFACE	0x010D	// ��������ͨ�Ž���
#define EMVMSG_EXPIRED_APP			0x010E  // ��Ƭ����Ч��,����ʧ��

// ������ϢΪpboc2.0֤����֤ר��
#define EMVMSG_VERIFY_HOLDER_ID		0x0201 // "�����ֿ���֤��"
#define EMVMSG_HOLDER_ID_TYPE		0x0202 // "�ֿ���֤������"
#define EMVMSG_IDENTIFICATION_CARD	0x0203 // "���֤"
#define EMVMSG_CERTIFICATE_OF_OFFICERS	0x0204 // "����֤"
#define EMVMSG_PASSPORT				0x0205 // "����"
#define EMVMSG_ARRIVAL_CARD			0x0206 // "�뾳֤"
#define EMVMSG_TEMPORARY_IDENTITY_CARD	0x0207 // "��ʱ���֤"
#define EMVMSG_OTHER				0x0208 // "����"
#define EMVMSG_HOLDER_ID_NO			0x0209 // "�ֿ���֤������"


// ��̬��Ϣ, ���ڵ���0x1000����ϢΪ��̬��Ϣ, ��ʾ������Ϣ���ͱ�ʾ����Ϣ��, ����Ҫ�������̶�����Ϣ
#define EMVMSG_VAR					0x1000  // ��̬��Ϣ
#define EMVMSG_AMOUNT_CONFIRM		0x1001  // ���ȷ��

#endif
