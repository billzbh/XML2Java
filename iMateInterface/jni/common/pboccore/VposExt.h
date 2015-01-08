/**************************************
File name     : VPOSEXT.H
Function      : Implement the extended functions for talento pos
                At the same level compare with PosTal.c
Author        : Yu Jun
First edition : Apr 15th, 2004
**************************************/
# ifndef _VPOSEXT_H
# define _VPOSEXT_H

// ��ӡ��
#define PRT_STATUS_OK				0 // OK
#define PRT_STATUS_NO_PAPER			1 // ȱֽ
#define PRT_STATUS_LESS_PAPER		2 // ֽ��
#define PRT_ERROR_NO_PRINTER		3 // û���ִ�ӡ��
#define PRT_ERROR_OFFLINE			4 // ��ӡ���ѻ�
#define PRT_ERROR					9 // ����

// ��ȫģ��
#define SEC_STATUS_OK				0 // OK
#define SEC_STATUS_TIMEOUT			1 // ��ʱ
#define SEC_STATUS_CANCEL			2 // �û�ȡ��
#define SEC_STATUS_BYPASS			3 // �û�Bypass
#define SEC_ERROR_ALGO				4 // �㷨��֧��
#define SEC_ERROR_INDEX				5 // ������֧��
#define SEC_ERROR_NO_KEY			6 // ��Կ������
#define SEC_ERROR					9 // ����

// ��⺺�ֿ��Ƿ�������
// ���� : 1 = ������, 0 = δ����
// note : ��ʼ��ʱ�ú���Ӧ������
uint uiTestFont12(void);

// ȡ12�����ֿ�
// ������� : ucQm     : ��������
//            ucWm     : ����λ��
// ������� : psMatrix : ���ֵ���
void vGetFont12Matrix(uchar ucQm, uchar ucWm, uchar *psMatrix);

// ��װ12�����ֿ⺯����
// uiLoadFont12Start(), uiLoadFont12Data(), uiLoadFont12End()
// return 0 means OK, 1 means error
uint uiLoadFont12Begin(ulong ulFileSize);
uint uiLoadFont12AddBlock(uchar *psData, uint uiLength);
uint uiLoadFont12End(void);

// get pin from pin pad
// In  : psMessage1   : message displayed on line 1
//       psMessage2   : message displayed on line 2
//       psMessage3   : message displayed on line 2 after a key pressed
//       pszPan       : ���ʺ�
//       ucBypassFlag : ����bypass��־, 1:���� 0:������
//       ucMinLen     : minimum pin length
//       ucMaxLen     : maximum pin length
//       uiTimeOut    : time out time in second
// Out : pszPin       : entered pin
// Ret : SEC_STATUS_OK      : pin entered
//       SEC_STATUS_TIMEOUT : ��ʱ
//       SEC_STATUS_CANCEL  : �û�ȡ��
//       SEC_STATUS_BYPASS  : �û�Bypass
//       SEC_ERROR		    : ����
uchar ucPinPadInput(uchar *psMessage1, uchar *psMessage2, uchar *psMessage3, uchar *pszPan, uchar ucBypassFlag,
						  uchar ucMinLen, uchar ucMaxLen, uint uiTimeOut, uchar *pszPin);

// display message on pinpad
// In  : psMessage1 : message displayed on line 1
//       psMessage2 : message displayed on line 2
//       uiTimeOut  : time out time in second
void vPinPadDisp(uchar *pszMesg1,uchar *pszMesg2, uint uiTimeOut);

// ��ȫģ���ʼ��
// Ret : SEC_STATUS_OK      : OK
//       SEC_ERROR		    : ����
int iSecInit(void);

// ��ȫģ��������Կ�治����
// in  : iIndex      : ����Կ����[0-n], ����ȡ����POS
// Ret : SEC_STATUS_OK      : OK
//       SEC_ERROR_INDEX	: ������֧��
//       SEC_ERROR_NO_KEY	: ��Կ������
//       SEC_ERROR		    : ����
int iSecCheckMasterKey(int iIndex);

// ��ȫģ����������Կ
// in  : iIndex      : ����Կ����[0-n], ����ȡ����POS
//       psMasterKey : ����Կ[16]
// Ret : SEC_STATUS_OK      : OK
//       SEC_ERROR_INDEX	: ������֧��
//       SEC_ERROR		    : ����
int iSecSetMasterKey(int iIndex, uchar *psMasterKey);

// ��ȫģ������ԿDES����
// in  : iIndex      : ����Կ����[0-n], ����ȡ����POS
//       iMode       : �㷨��ʶ, TRI_ENCRYPT:3Des���� TRI_DECRYPT:3Des����
//       psIn        : ��������
// out : psOut       : �������
// Ret : SEC_STATUS_OK      : OK
//       SEC_ERROR_INDEX	: ������֧��
//       SEC_ERROR_NO_KEY	: ��Կ������
//       SEC_ERROR_ALGO		: �㷨��֧��
//       SEC_ERROR		    : ����
int iSecDes(int iIndex, int iMode, uchar *psIn, uchar *psOut);

// ��ȫģ�鹤����ԿRetail-CBC-Mac����
// in  : iIndex       : ����Կ����[0-n], ����ȡ����POS
//       psWorkingKey : ������Կ����
//       psIn         : ��������
//       iInLen       : �������ݳ���, ����8�ı�����Ჹ0
// out : psMac        : Mac���[8]
// Ret : SEC_STATUS_OK      : OK
//       SEC_ERROR_INDEX	: ������֧��
//       SEC_ERROR_NO_KEY	: ��Կ������
//       SEC_ERROR_ALGO		: �㷨��֧��
//       SEC_ERROR		    : ����
// Note: �����3DES����, Ҳ���ô˺���
int iSec3DesCbcMacRetail(int iIndex, uchar *psWorkingKey, uchar *psIn, int iInLen, uchar *psMac);

// ���������������PIN
// In  : psMessage1   : message displayed on line 1
//       psMessage2   : message displayed on line 2
//       psMessage3   : message displayed on line 2 after a key pressed
//       iIndex       : ����Կ����[0-n], ����ȡ����POS
//       psPinKey     : PinKey����
//       pszPan       : ���ʺ�
//       ucBypassFlag : ����bypass��־, 1:���� 0:������
//       ucMinLen     : minimum pin length
//       ucMaxLen     : maximum pin length
//       uiTimeOut    : time out time in second
// Out : sPinBlock    : pinblock[8]
// Ret : SEC_STATUS_OK      : OK
//       SEC_STATUS_TIMEOUT : ��ʱ
//       SEC_STATUS_CANCEL  : �û�ȡ��
//       SEC_STATUS_BYPASS  : �û�Bypass
//       SEC_ERROR_INDEX	: ������֧��
//       SEC_ERROR_NO_KEY	: ��Կ������
//       SEC_ERROR		    : ����
int iSecPinPadInput(uchar *psMessage1, uchar *psMessage2, uchar *psMessage3,
                    int iIndex, uchar *psPinKey, uchar *pszPan, uchar ucBypassFlag,
                    uchar ucMinLen, uchar ucMaxLen, uint uiTimeOut, uchar *psPinBlock);

// ��ȡ���״̬
// ret : 0-100 : ��ص���
uint uiGetBatteryCharge(void);

// ��ȡ���״̬
// ret : 0 : �ǳ��״̬
//       1 : ���״̬
uint uiGetBatteryRecharge(void);

// LED�ƿ���
void _vSetLed(uchar ucNo,uchar ucOnOff);

// ��ӡ����ֽ
void _vPrintCut(void);

// ��ӡ���建����
void _vPrintClear(void);

// ȡ�ô�ӡ��״̬
// ��    ��: PRT_STATUS_OK			// OK
//           PRT_STATUS_NO_PAPER	// ȱֽ
//           PRT_STATUS_LESS_PAPER	// ֽ��
//           PRT_ERROR_NO_PRINTER	// û���ִ�ӡ��
//           PRT_ERROR_OFFLINE		// ��ӡ���ѻ�
//           PRT_ERROR				// ����
uint _uiPrintGetStatus(void);

// �򿪴�ӡ����׼��װֽ
// ret : PRT_STATUS_OK : �򿪳ɹ�
//       PRT_ERROR	   : ��ʧ��, ��ӡ������ȻΪ�ر�״̬
uint uiOpenPaperBox(void);

// ����ӡ���п���״̬
// ret : 0 : ��״̬
//       1 : �ر�״̬
uint uiTestPaperBox(void);

// ��ȡ�ն����к�
// ret : 0 : OK
//       1 : ERROR
uint uiGetSerialNo(uchar *pszSerialNo);

# endif
