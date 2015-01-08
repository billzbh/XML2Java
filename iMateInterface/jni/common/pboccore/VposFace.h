/**************************************
File name     : VPOSFACE.H
Function      : Define VPOS interface
Author        : Yu Jun
First edition : Ver 1.00 Sep 24th, 1999
Update to     : Ver 2.00 Mar 8th, 2000
                         add # define int short
                         rename _vFunctionDes() to _vDes()
                                add TRI_ENCRYPT & TRI_DECRYPT
                         add macro DBGINFO
                         add communication functions
                Ver 2.01 Jun 14th, 2001
                         remove macro ATR_LENGTH and DATA_BUFFER_LEN
                         instead of DATA_BUFFER_LEN, use fixed length 256
                Ver 2.10 Aug 7th, 2001
                         adjust magnetic interface
                         _uiMagGet() and _vMagWrite() use new specification
                         add macro VPOS_H_VER
                Ver 2.11 Dec 30th, 2002
                         adjust function _uiPrint()
                         when print a full line and a '\n', only cause one line feed
                Ver 3.0  Mar 19th, 2004
                         add trail functions
                             vTrailInit(), vTrail(), uiTrailSet()
						 removed Mar 27th, 2012
                Ver 3.1  Nov 7th, 2005
                         add functions: _uiBackupLine() and _vRestoreLine()
                         add function: _uiGetPosType()
                Ver 3.1a Nov 10th, 2006
                         add macro TEL_ERROR_CANCEL
				Ver 3.2  2011
						 add new comm functions, _uiComm???()
				Ver 3.2a Jun 20th, 2012
						 add new functions _uiBackupScreen() _vRestoreScreen()
                Ver 3.3  Nov 26th, 2012
                         add new functions _uiDoApdu()
				Ver 3.4  Jun 27th, 2013
						 ��Pub.h��������һЩ������������
						 void vXor(uchar *psVect1, const uchar *psVect2, int iLength)
						 void vOr(uchar *psVect1, const uchar *psVect2, int iLength)
						 void vAnd(uchar *psVect1, const uchar *psVect2, int iLength)
						 void vOneTwo(const uchar *psIn, int iLength, uchar *psOut)
						 void vOneTwo0(const uchar *psIn, int iLength, uchar *pszOut)
						 void vTwoOne(const uchar *psIn, int iLength, uchar *psOut)
						 ulong ulStrToLong(const uchar *psBinString, int iLength)
						 void vLongToStr(ulong ulLongNumber, int iLength, uchar *psBinString)
						 ulong ulHexToLong(const uchar *psHexString, int iLength)
						 void vLongToHex(ulong ulLongNumber, int iLength, uchar *psHexString)
						 ulong ulA2L(const uchar *psString, int iLength)
						 void vMemcpy0(uchar *pszTarget, const uchar *psSource, int iLength)
						 ulong ulBcd2L(const uchar *psBcdString, int iLength)
						 void vL2Bcd(ulong ulLongNumber, int iLength, uchar *psBcdString)
				Ver 3.5  Apr 21st, 2014
						 ���Ӻ���_uiSetCardCtrlFunc(), ����֧���ⲿ�����IC����������
						 ���Ӻ���vOneTwoX()��vOneTwoX()
**************************************/
# ifndef _VPOSFACE_H
# define _VPOSFACE_H

# define VPOS_H_VER  0x0340

//# define VPOS_DEBUG

# ifndef uint
# define uint unsigned int
# endif

# ifndef uchar
# define uchar unsigned char
# endif

# ifndef ulong
# define ulong unsigned long
# endif

# ifndef ushort
# define ushort unsigned short
# endif

# define POS_TYPE_MASK              0xFF00

# define POS_TYPE_PC                0x0100

# define POS_TYPE_ALINK             0x0200
# define POS_TYPE_ALINK_ST2         0x0201
# define POS_TYPE_ALINK_ST5         0x0202

# define POS_TYPE_TALENTO           0x0300

# define POS_TYPE_ARTEMA            0x0400
# define POS_TYPE_ARTEMA_DECT       0x0401
# define POS_TYPE_ARTEMA_DESK       0x0402
# define POS_TYPE_ARTEMA_COMPACT    0x0403

# define POS_TYPE_YIN4              0x0500

# define POS_TYPE_VERIFONE          0x0600
# define POS_TYPE_VERIFONE_VX520    0x0601
# define POS_TYPE_VERIFONE_VX680    0x0602

# define POS_TYPE_YIN5              0x0700
# define POS_TYPE_YIN5_A            0x0701

# define POS_TYPE_LINUX             0x0800

// for display attribute
# define DISP_NORMAL    0x00
# define DISP_REVERSE   0x01
# define DISP_UNDERLINE 0x02
# define DISP_OVERLAP   0x04

// define keyboard code
# define _KEY_ENTER     0x0d
# define _KEY_BKSP      0x08
# define _KEY_ESC       0x1b
# define _KEY_CANCEL    0x1b
# define _KEY_DOT       '.'
# define _KEY_WELL      '#'
# define _KEY_STAR      '*'

# define _KEY_0         '0'
# define _KEY_1         '1'
# define _KEY_2         '2'
# define _KEY_3         '3'
# define _KEY_4         '4'
# define _KEY_5         '5'
# define _KEY_6         '6'
# define _KEY_7         '7'
# define _KEY_8         '8'
# define _KEY_9         '9'

# define _KEY_F1        0xF1
# define _KEY_F2        0xF2
# define _KEY_F3        0xF3
# define _KEY_F4        0xF4
# define _KEY_F5        0xF5
# define _KEY_F6        0xF6
# define _KEY_F7        0xF7
# define _KEY_F8        0xF8
# define _KEY_F9        0xF9
# define _KEY_F10       0xFA
# define _KEY_F11       0xFB
# define _KEY_F12       0xFC
# define _KEY_OFF       0xFE
# define _KEY_OTHER     0xFF

# define _KEY_UP        0xE0
# define _KEY_DOWN      0xE1
# define _KEY_LEFT      0xE2
# define _KEY_RIGHT     0xE3
# define _KEY_NEXT      0xE4
# define _KEY_UP1       0xE5
# define _KEY_UP2       0xE6
# define _KEY_UP3       0xE7
# define _KEY_UP4       0xE8
# define _KEY_FN1       0xE9
# define _KEY_FN2       0xEA
# define _KEY_FN3       0xEB
# define _KEY_FN4       0xEC
# define _KEY_MENU      0xED
# define _KEY_00        0xEE
# define _KEY_FN        0xEF

// for des use
# define ENCRYPT        1
# define DECRYPT        2
# define TRI_ENCRYPT    3
# define TRI_DECRYPT    4

// for communication
# define COMM_NONE                  0
# define COMM_EVEN                  1
# define COMM_ODD                   2

# define COMM_SYNC                  1
# define COMM_ASYNC                 2
# define COMM_CCITT                 1
# define COMM_BELL                  2

# define TEL_OK                     0x0000
# define TEL_PARAMETER              0x0100

# define TEL_STATUS_WAIT            0x0200
# define TEL_STATUS_BUSY            0x0201
# define TEL_STATUS_TIMEOUT         0x0202

# define TEL_ERROR_NO_DEVICE        0x0300
# define TEL_ERROR_NO_TONE          0x0301
# define TEL_ERROR_NO_CARRY         0x0302
# define TEL_ERROR_NO_HDLC          0x0303
# define TEL_ERROR_NO_CONNECT       0x0304
# define TEL_ERROR_NO_ANSWER        0x0305
# define TEL_ERROR_NO_LINE          0x0306
# define TEL_ERROR_CANCEL           0x0398
# define TEL_ERROR_UNKNOWN          0x0399

// ����ͨѶ
# define VPOSCOMM_SDLC              1
# define VPOSCOMM_ASYNC             2
# define VPOSCOMM_ETHER             3
# define VPOSCOMM_GPRS              4
# define VPOSCOMM_CDMA              5
// ����ͨѶ������
# define COM_OK                     0x0000 // �ɹ�
# define COM_PARAMETER              0x0100 // ��������
# define COM_STATUS_PROCESSING      0x0200 // ����������
# define COM_STATUS_BUZY            0x0201 // ��·æ
# define COM_STATUS_TIMEOUT         0x0202 // ��ʱ
# define COM_ERR_DEVICE             0x0300 // �豸���ϻ��޴��豸
# define COM_ERR_NO_TONE            0x0301 // �޲�����
# define COM_ERR_NO_CARRY           0x0302 // ���ز�
# define COM_ERR_NO_CONNECT         0x0304 // û������
# define COM_ERR_NO_ANSWER          0x0305 // û��Ӧ��
# define COM_ERR_NO_LINE            0x0306 // û�ӵ绰��
# define COM_ERR_COM_TYPE           0x0310 // ͨѶ���Ͳ�֧��
# define COM_ERR_NO_REGISTER        0x0311 // ��û��ע������
# define COM_ERR_SEND               0x0312 // ����ʧ��
# define COM_ERR_RECV               0x0313 // ����ʧ��
# define COM_ERR_SYS                0x0314 // ϵͳ����
# define COM_ERR_CANCEL             0x0398 // �û�ȡ��
# define COM_ERR_UNKNOWN            0x0399 // δ֪����

# define APDU_NORMAL                0      // ��׼APDU, ���Զ�����61xx��6Cxx
# define APDU_AUTO_GET_RESP         1      // ��׼APDU, �Զ�����61xx��6Cxx
# define APDU_EMV                   0x80   // ִ��EMVЭ��
# define APDU_SI                    0x81   // ִ���籣2.0Э��

typedef struct {
    uchar sCommand[4];
    uchar ucLengthIn;
    uchar sDataIn[256];
    uchar ucLengthExpected;
} APDU_IN;
typedef struct {
    uchar  ucLengthOut;
    uchar  sDataOut[256];
    ushort uiStatus;
} APDU_OUT;
typedef struct {
    ushort uiReader;
    uchar  sCommand[4];
    uchar  ucLengthIn;
    uchar  ucLengthExpected;
    ushort uiStatus; // 0 menas card terminal i/o error
} APDU_STATUS;

typedef struct {
    ulong ulBaud;      // 1200, 2400, 4800, 9600
    uchar ucParity;    // COMM_NONE, COMM_ODD, COMM_EVEN
    uchar ucBits;      // 7, 8
    uchar ucStop;      // 1
    uchar szPABX[5];   // ���ߺ���, ����Ҫ�Ӷ���
    uchar szTelNo[15]; // example: "07553259571", "3237496"
    uchar ucType;      // COMM_CCITT, COMM_BELL
    uchar ucMode;      // COMM_SYNC, COMM_ASYNC
} TEL_CONFIG;

typedef struct {
    uint  uiCommType;           //�������� VPOSCOMM_SDLC or VPOSCOMM_ASYNC or VPOSCOMM_ETHER or VPOSCOMM_GPRS or VPOSCOMM_CDMA
    uchar szLocalIP[16];	    //������ַ      ETHERNET    GPRS    CDMA
    uchar szSubNetMask[16];	    //��������      ETHERNET    GPRS    CDMA
    uchar szGateWay[16];	    //���ص�ַ      ETHERNET    GPRS    CDMA
    uchar szPriDNS[16];		    //��DNS��ַ     ETHERNET    GPRS    CDMA
    uchar szSecDNS[16];		    //��DNS��ַ     ETHERNET    GPRS    CDMA
    uchar szApn[64];		    //APN                       GPRS
    uchar szPhoneNumber[32];    //���ź���                  GPRS    CDMA
    uchar szUserName[32];	    //��¼�û���                GPRS    CDMA
    uchar szUserPwd[32];	    //��¼����                  GPRS    CDMA
} VPOSCOMM_CONFIG; // ����ͨѶ����

typedef struct {
    uchar szLocalIP[16];	    //������ַ      ETHERNET    GPRS    CDMA
    uchar szSubNetMask[16];	    //��������      ETHERNET    GPRS    CDMA
    uchar szGateWay[16];	    //���ص�ַ      ETHERNET    GPRS    CDMA
    uchar szMyPhID[20];		    //�����绰����              GPRS    CDMA
    uchar szSingal[4];			//�ź�ǿ��(0-100)           GPRS    CDMA
} VPOSCOMM_INFO;   // ������Ϣ

typedef struct {
    ushort uiFlag;           // ���û�ֹͣ��־
    ulong  ulXMemOffset;     // ��������������XMem�е���ʼ��ַ
    ulong  ulXMemSize;       // ��������������XMem�еĴ�С
    ushort uiMemSize;        // �ڴ沿�ָ������ݿ��С
    
    ulong  ulXMemRound;      // XMem������������д����
    ulong  ulXMemCurrOffset; // �������ݵ�ǰ��XMem�е�λ��(�����ulXMemOffset)
    ushort uiMemCurrOffset;  // �������ݵ�ǰ���ڴ��е�λ��
    uchar  sBuf[1024];       // �����������ڴ��еĻ�����
} TRAIL;

// ����֧������������ʽ�ӿ�
extern APDU_STATUS  gl_ApduStatus;
extern APDU_IN      gl_LastApduIn;
extern APDU_OUT     gl_LastApduOut;

extern uchar gl_sLastApduIn[270];
extern uint  gl_uiLastApduInLen;
extern uchar gl_sLastApduOut[270];
extern uint  gl_uiLastApduOutLen;

extern TRAIL gl_Trail;
# ifdef VPOS_DEBUG
extern uchar gl_szDbgInfo[];
# endif

// 1.   ϵͳ������

// 1.1. ��ʼ��POSϵͳ��ֻ���ڳ�����ʼ������һ�Ρ�
void _vPosInit(void);

// 1.2. POSͣ������ͬ��POS���ܵı����У��µ硢����������ֹͣ���С�
void _vPosClose(void);

// 1.3. ȡ��POS�ͺ�
// ��    �أ�POS�ͺ�
//           Ŀǰ֧�� POS_TYPE_PC POS_TYPE_ST2 POS_TYPE_TALENTO POS_TYPE_ARTEMA
uint _uiGetPosType(void);

// 2.   ��ʾ

// 2.1. ȡ��Ļ����ʾ����
// ��    �أ���Ļ����ʾ����
uint _uiGetVLines(void);

// 2.2. ȡ��Ļ����ʾ����
// ��    �أ���Ļ����ʾ����
uint _uiGetVCols(void);

// 2.3. ���������Ļ
void _vCls(void);

// 2.4. ��ĳ����ʾ�ַ���
// ���������uiLine  : Ҫ��ʾ���������ӵ�һ�п�ʼ
//           pszText : Ҫ��ʾ���ִ�
// ˵    �����粻���ڸ��У��������ء����ִ����ȳ�����Ļ��ȣ���ȥ�����Ĳ��֡�
void _vDisp(uint uiLine, const uchar *pszText);

// 2.5. ��ĳ����ʾĳ���Ե��ַ���
// ���������uiLine  : Ҫ��ʾ���������ӵ�һ�п�ʼ
//           pszText : Ҫ��ʾ���ִ�
//           ucAttr  : ��ʾ���ԣ�������DISP_NORMAL��DISP_REVERSE��DISP_UNDERLINE�����
// ˵    �����粻���ڸ��У��������ء����ִ����ȳ�����Ļ��ȣ���ȥ�����Ĳ��֡�
void _vDispX(uint uiLine, const uchar *pszText, uchar ucAttr);

// 2.6. ��ĳ��ĳ����ʾ�ַ���
// ���������uiLine  : Ҫ��ʾ���������ӵ�1�п�ʼ
//           uiCol   : Ҫ��ʾ���������ӵ�0�п�ʼ
//           pszText : Ҫ��ʾ���ִ�
// ˵    �����粻���ڸ��У��������ء����ִ����ȳ�����Ļ��ȣ����Ƶ���һ�С�
//           ���ִ�������Ļ���һ�У���ȥ�����Ĳ��֡�
void _vDispAt(uint uiLine, uint uiCol, const uchar *pszText);

// 2.7. ��ĳ��ĳ�д�������ʾ�ַ���
// ���������uiLine  : Ҫ��ʾ���������ӵ�1�п�ʼ
//           uiCol   : Ҫ��ʾ���������ӵ�0�п�ʼ
//           pszText : Ҫ��ʾ���ִ�
//           ucAttr  : ��ʾ���ԣ�������DISP_NORMAL��DISP_REVERSE��DISP_UNDERLINE�����
// ˵    �����粻���ڸ��У��������ء����ִ����ȳ�����Ļ��ȣ����Ƶ���һ�С�
//           ���ִ�������Ļ���һ�У���ȥ�����Ĳ��֡�
void _vDispAtX(uint uiLine, uint uiCol, const uchar *pszText, uchar ucAttr);

// 2.8. ����ĳ������
// ���������uiLine  : Ҫ���ݵ��к�
// ��    �أ�0       : �ɹ�����
//           1       : ���ݿռ���
// ˵    ������ָ������������ͬ���Ա�������
uint _uiBackupLine(uint uiLine);

// 2.9. �ָ����һ���б���
// ˵    ���������һ�α��ݵ��лָ�ԭ��
void _vRestoreLine(void);

// 2.10. ��Ļ���ݱ���
// ��    �أ�0       : �ɹ�
//           1       : err
uint _uiBackupScreen(void);

// 2.11. ��Ļ���ݻָ�
// ��    �أ�0       : �ɹ�
//           1       : err
void _vRestoreScreen(void);

// 3. ����

// 3.1. ����з񰴼�����������
// ��    �أ�0 ��û�а���
//           1 ����⵽��������ͨ��_uiGetKey()��ȡ
uint _uiKeyPressed(void);

// 3.2. �ȴ�������ֱ����ȡ��һ������Ϊֹ
// ��    �أ�0x00 - 0xff ��������ASCII��
uint _uiGetKey(void);

// 3.3. ������̻�����
void _vFlushKey(void);

// 3.4. ���¶���һ��������ʹ�ü���������һ������
// ���������ucSource : �������������ڵļ���
//           ucTarget : �ض����ý����صļ���, ����ü�����ucSource��ͬ����ɾ���ü�ԭ�ȵĶ���
// ��    �أ�0        : �ɹ�
//           1        : �ض���ļ���̫��
// ˵    �������ֻ���ض���10������
uint _uiMapKey(uchar ucSource, uchar ucTarget);



// 4. IC��

// 4.1. ��⿨Ƭ�Ƿ����
// ���������uiReader : ���⿨����
// ��    �أ�0        : �޿�
//           1        : �п�
// ˵    ����ֻ�ܼ���û�����
uint _uiTestCard(uint uiReader);

// 4.2. ��Ƭ��λ
// ���������uiReader    : ���⿨����
// ���������psResetData : ATR ����
// ��    �أ�0           : ��λʧ��
//           >0          : ��λ�ɹ�������ATR����
uint _uiResetCard(uint uiReader, uchar *psResetData);

// 4.3. ��Ƭ�µ�
// ���������uiReader    : ���⿨����
// ��    �أ�0           : �ɹ�
//           1           : ʧ��
uint _uiCloseCard(uint uiReader);

// 4.4. ִ��IC��ָ��
// ���������uiReader : ���⿨����
//           pIn      : IC��ָ��ṹ
// ���������pOut     : IC�����ؽ��
// ��    �أ�0        : �ɹ�
//           1        : ʧ��
uint _uiExchangeApdu(uint uiReader, APDU_IN *pIn, APDU_OUT *pOut);

// 4.5. ִ��IC��ָ��
// ���������uiReader   : ���⿨����
//           uiInLen    : Command Apduָ���
//           psIn       : Command APDU, ��׼case1-case4ָ��ṹ
//           uiProtocol : Э������
//                        APDU_NORMAL        ��׼APDU, ���Զ�����61xx��6Cxx
//                        APDU_AUTO_GET_RESP ��׼APDU, �Զ�����61xx��6Cxx
//                        APDU_EMV           ִ��EMVЭ��
//                        APDU_SI            ִ���籣2.0Э��
//           puiOutLen  : Response APDU����
// ���������psOut      : Response APDU, RespData[n]+SW[2]
// ��    �أ�0          : �ɹ�
//           1          : ʧ��
uint _uiDoApdu(uint uiReader, uint uiInLen, uchar *psIn, uint *puiOutLen, uchar *psOut, uint uiProtocol);

// 4.6. ���ö�������, �����ⲿ�ṩIC���ӿ�ʱ
// �������: uiReader     : ������, 0-9, �Ժ�Ըÿ����ŵĲ������������õĺ�������
//           pfiTestCard  : ��⿨Ƭ�Ƿ���ں���ָ��
//           pfiResetCard : ��Ƭ��λ����ָ��
//           pfiDoApdu    : ִ��Apduָ���ָ��
//           pfiCloseCard : �رտ�Ƭ����ָ��
// ��    ��: 0            : OK
//           1            : ʧ��
uint _uiSetCardCtrlFunc(uint uiReader,
					    int (*pfiTestCard)(void),
				        int (*pfiResetCard)(uchar *psAtr),
				        int (*pfiDoApdu)(int iApduInLen, uchar *psApduIn, int *piApduOutLen, uchar *psApduOut),
				        int (*pfiCloseCard)(void));

// 5. ��ӡ��

// 5.1. ȡ��ӡ�����
// ��    �أ�0  : �޴�ӡ��
//           >0 : ��ӡ���ɴ�ӡ�������
uint _uiGetPCols(void);

// 5.2. ��ӡ����
// ���������pszText : Ҫ��ӡ���ִ�
// ��    �أ�0 : ��ӡ�ɹ�
//           1 : ��ӡ����
// ˵    ������ӡ�ִ�������ӡ�������ʱ���Ƶ���һ�С�
//           '\n'�ַ��ᵼ�»��С�
uint _uiPrint(uchar const *pszText);



// 6. ʱ��

// 6.1. ���õ�ǰϵͳʱ��
// ���������pszTime : Ҫ���õ�ʱ�䣬��ʽ:YYYYMMDDHHMMSS
// ��    �أ�0       : �ɹ�
//           1       : ʧ��
uint _uiSetTime(uchar const *pszTime);

// 6.2. ��ȡϵͳʱ��
// ���������pszTime : ��ǰϵͳʱ�䣬��ʽ:YYYYMMDDHHMMSS
void _vGetTime(uchar *pszTime);

// 6.3.���ü�ʱ��
// ���������ulNTick   : ��ʱʱ�䣬��λΪ0.01��
// ���������pulTimer  : ��ʱ������
// ˵    ������ȷ��0.01��
void _vSetTimer(ulong *pulTimer, ulong ulNTick);

// 6.4. �ж��Ƿ񵽴��ʱ��������������ʱ��
// ���������ulTimer : ��ʱ������
// ��    �أ�0       : û�г�ʱ
//           1       : ��ʱ
uint _uiTestTimer(ulong ulTimer);

// 6.5. �ӳ�һ��ʱ��
// ���������uiNTick : �ӳٵ�ʱ�䣬��λΪ0.01��
void _vDelay(uint uiNTick);



// 7. ��չ�ڴ�

// 7.1. ������չ�ڴ��С
// ���������ulSize : Ҫ���õ���չ�ڴ��С����λΪ�ֽ�
// ��    �أ�0      : ���óɹ�
//           1      : ��������, ����Ŀռ������ڵ���32
//           2      : ����ʧ��
//           >=32   : ��չ�ڴ治�㣬����ֵΪ��������չ�ڴ��С
ulong _ulSetXMemSize(ulong ulSize);

// 7.2. ����չ�ڴ�
// ���������ulOffset : ��չ�ڴ����ֽ�Ϊ��λ��ƫ����
//           uiLen    �����ֽ�Ϊ��λ�ĳ���
// ���������pBuffer  : ��ȡ������
// ��    �أ�0        : �ɹ�
//           1        : ʧ��
uint _uiXMemRead(void *pBuffer, ulong ulOffset, uint uiLen);

// 7.3. д��չ�ڴ�
// ���������pBuffer  : Ҫд������
//           ulOffset : ��չ�ڴ����ֽ�Ϊ��λ��ƫ����
//           uiLen    �����ֽ�Ϊ��λ�ĳ���
// ��    �أ�0        : �ɹ�
//           1        : ʧ��
uint _uiXMemWrite(const void *pBuffer, ulong ulOffset, uint uiLen);



// 8. ������

// 8.1. ��ſ�������
// ��    �أ�0 : �ɹ�
//           1 : ʧ��
uint _uiMagReset(void);

// 8.2. ����Ƿ��дſ�ˢ��
// ��    �أ�0 : �޿�
//           1 : �дſ�ˢ��
uint _uiMagTest(void);

// 8.3. ���ŵ���Ϣ
// ���������uiTrackNo : �ŵ��ţ�1-3
// ���������pszBuffer : �ŵ���Ϣ
// ��    �أ�0 : ��ȡ��Ϣ��ȷ
//           1 : û������
//           2 : ��⵽����
// ˵    ����û����ʼ������������У��λ���ַ�����Ϊ: 0x30-0x3f
uint _uiMagGet(uint uiTrackNo, uchar *pszBuffer);

// 8.4. д�ŵ���Ϣ
// ���������pszTrack2 : 2�ŵ���Ϣ
//         ��pszTrack3 : 3�ŵ���Ϣ
// ˵    ����û����ʼ������������У��λ���ַ�����Ϊ: 0x30-0x3f
void _vMagWrite(uchar *pszTrack2, uchar *pszTrack3);


// 9. ͨѶ

// 9.1. ���õ�ǰ���ں�
// ���������ucPortNo : �´��ں�, �Ӵ���1��ʼ
// ��    �أ�ԭ�ȵĴ��ں�
uchar _ucAsyncSetPort(uchar ucPortNo);

// 9.2. �򿪵�ǰ����
// ���������ulBaud   : ������
//           ucParity : ��żУ���־��COMM_EVEN��COMM_ODD��COMM_NONE
//           ucBits   : ����λ����
//           ucStop   : ֹͣλ����
// ��    �أ�0        : �ɹ�
//           1        : ��������
//           2        : ʧ��
uchar _ucAsyncOpen(ulong ulBaud, uchar ucParity, uchar ucBits, uchar ucStop);

// 9.3. �رյ�ǰ����
// ��    �أ�0        : �ɹ�
//           1        : ʧ��
uchar _ucAsyncClose(void);

// 9.4. ��λ��ǰ���ڣ���ս��ջ�����
// ��    �أ�0        : �ɹ�
//           1        : ʧ��
uchar _ucAsyncReset(void);

// 9.5. ����һ���ֽ�
// ���������ucChar : Ҫ���͵��ַ�
// ��    �أ�0      : �ɹ�
//           1      : ʧ��
uchar _ucAsyncSend(uchar ucChar);

// 9.6. �����û���ַ��յ�
// ��    �أ�0 : û���ַ��յ�
//           1 : ���ַ��յ�
uchar _ucAsyncTest(void);

// 9.7. ����һ���ַ���û����һֱ�ȴ�
// ��    �أ����յ����ַ�
uchar _ucAsyncGet(void);

// 9.8.	����һ������
// ���������pBuf  : Ҫ���͵��ַ�
//           uiLen : ���ݳ���
// ��    �أ�0     : �ɹ�
//           1     : ʧ��
uchar _ucAsyncSendBuf(void *pBuf, uint uiLen);

// 9.9.	����ָ�����ȵ�һ������
// ���������pBuf      : Ҫ���͵�����
//           uiLen     : ���ݳ���
//           uiTimeOut : ����Ϊ��λ�ĳ�ʱʱ��
// ��    �أ�0         : �ɹ�
//           1         : ��ʱ
uchar _ucAsyncGetBuf(void *pBuf, uint uiLen, uint uiTimeOut);

// 9.10. ���Ž�������
// ���������pTelConfig : ���Žṹָ�룬�� TEL_CONFIG �ṹ
//           uiTimeOut  : ��ʱʱ�䣬��λΪ��
// ��    �أ�0          : �ɹ�
//           1          : ʧ�ܣ�����_uiTelGetStatus()ȡ�ô���
uchar _ucTelConnect(const TEL_CONFIG *pTelConfig, uint uiTimeOut);

// 9.11. �������
// ��    �أ�0 : �ɹ�
//           1 : ʧ�ܣ�����_uiTelGetStatus()ȡ�ô���
uchar _ucTelDisconnect(void);

// 9.12. ȡ�ز������ӵ�ǰ״̬
// ��    �أ�0x0000 : ����
//           0x01?? : ��������
//           0x02?? : ��������λ�ֽڱ�ʾһ��״̬
//           0x03?? : ���󣬵�λ�ֽڱ�ʾ����ԭ��
uint _uiTelGetStatus(void);

// 9.13. �����߷�������
// ���������uiLength : ���ͱ��ĳ���
//           pBuffer  : ���ͱ�������
// ��    �أ�0        : ����
//           1        : ʧ�ܣ�����_uiTelGetStatus()ȡ�ô���
uchar _ucTelSend(uint uiLength, void const *pBuffer);

// 9.14. �����߽�������
// ���������puiLength : ���ձ��ĳ���
//           pBuffer   : ���ձ�������
//           uiTimeOut : ��ʱʱ�䣬��λΪ��
// ��    �أ�0         : ����
//           1         : ʧ�ܣ�����_uiTelGetStatus()ȡ�ô���
uchar _ucTelGet(uint *puiLength, void *pBuffer, uint uiTimeOut);



// 10. ����

// 10.1. ��һ����
void _vBuzzer(void);

// 10.2. ȡһ�����
// ��    �أ�0-255�������
uchar _ucGetRand(void);

// 10.3. ��DES����
// ���������uiMode   : ENCRYPT -> ���ܣ�DECRYPT -> ����
//                      TRI_ENCRYPT -> 3Des���ܣ�TRI_DECRYPT -> 3Des����
//           psSource : Դ
//           psKey    : ��Կ
// ���������psResult : Ŀ��
void _vDes(uint uiMode, uchar *psSource, uchar *psKey, uchar *psResult);



// 11. ���Ժ�
// ֻ����VPOSFACE.H���� # define VPOS_DEBUG ʱ���º��������

// 11.1. ASSERT(bVal)
// ���ܣ����bValΪ�٣�����ʾ�ú����ڵ��ļ���������
// ԭ�ͣ�ASSART(BOOL bVal)
# ifdef VPOS_DEBUG
extern void _vDispAssert(uchar *pszFileName, ulong uiLineNo);
# define ASSERT(expr)                                              \
      if(!(expr)) {                                                \
          strcpy((char *)gl_szDbgInfo, __FILE__);                          \
          _vDispAssert(gl_szDbgInfo, (ulong)(__LINE__));           \
      }
# else
# define ASSERT(expr)
# endif

// 11.2. DBGINFO()
// ���ܣ���ʾ�ú����ڵ�������pPointer��ָ���ݣ�������16��������ʾ6�ֽ�
// ԭ�ͣ�DBGINFO(void *pPointer)
# ifdef VPOS_DEBUG
extern void _vDispDbgInfo(uchar *pszFileName, ulong ulLineNo, void *pPointer);
# define DBGINFO(pPointer)                                         \
      strcpy((char *)gl_szDbgInfo, __FILE__);                 \
      _vDispDbgInfo(gl_szDbgInfo, (ulong)(__LINE__), (void *)(pPointer));
# else
# define DBGINFO(pPointer)
# endif

// ����֧�ָ��ٹ���
// 12. ����
//

// 13. ���繦�ܺ���(Ҳ������SDLC���첽ͨѶ), �ӿڲ�����VeriFone520 vCommCE�ӿ�
// 13.1. �����ʼ��
uint _uiCommInit(void);

// 13.2 ע������
// in  : uiCommType  : �������� VPOSCOMM_SDLC or VPOSCOMM_ASYNC or VPOSCOMM_ETHER or VPOSCOMM_GPRS or VPOSCOMM_CDMA
//       pCommConfig : �������
//       uiTimeout   : ��ʱʱ��(��), 0��ʾ���ȴ�,��������
// ret : VPOSCOMM_
// Note: ������ע������ſ�ʹ������
uint _uiCommLogin(uint uiCommType, VPOSCOMM_CONFIG *pCommConfig, uint uiTimeout);

// 13.2 ע������
// in  : uiCommType  : �������� VPOSCOMM_SDLC or VPOSCOMM_ASYNC or VPOSCOMM_ETHER or VPOSCOMM_GPRS or VPOSCOMM_CDMA
uint _uiCommLogOut(uint uiCommType);

// 13.3 ����ע��״̬
// in  : uiCommType  : �������� VPOSCOMM_SDLC or VPOSCOMM_ASYNC or VPOSCOMM_ETHER or VPOSCOMM_GPRS or VPOSCOMM_CDMA
// Note: ������ֶ�ʧ��ע��, ��ͼ����ע��
uint _uiCommKeepLogin(uint uiCommType);

// 13.4 ���ӷ�����
// in  : uiCommType  : �������� VPOSCOMM_SDLC or VPOSCOMM_ASYNC or VPOSCOMM_ETHER or VPOSCOMM_GPRS or VPOSCOMM_CDMA
//       psParam1    : ����1, ����VPOSCOMM_SDLC VPOSCOMM_ASYNC : �绰����
//                            ����VPOSCOMM_ETHER VPOSCOMM_GPRS VPOSCOMM_CDMA : Ip��ַ
//       psParam2    : ����2, ����VPOSCOMM_SDLC VPOSCOMM_ASYNC : ���ߺ���
//                            ����VPOSCOMM_ETHER VPOSCOMM_GPRS VPOSCOMM_CDMA : �˿ں�
//       psParam3    : ����3, ����VPOSCOMM_SDLC VPOSCOMM_ASYNC : "0"-����Ⲧ���� "1"-��Ⲧ����
//                            ����VPOSCOMM_ETHER VPOSCOMM_GPRS VPOSCOMM_CDMA : "0"-TCP "1"-UDP
//       uiTimeout   : ��ʱʱ��(��), 0��ʾ���ȴ�,��������
uint _uiCommConnect(uint uiCommType, uchar *psParam1, uchar *psParam2, uchar *psParam3, uint uiTimeout);

// 13.5 �����ͨ��
// in  : uiCommType  : �������� VPOSCOMM_SDLC or VPOSCOMM_ASYNC or VPOSCOMM_ETHER or VPOSCOMM_GPRS or VPOSCOMM_CDMA
uint _uiCommTestConnect(uint uiCommType);

// 13.6 �Ͽ�����
// in  : uiCommType  : �������� VPOSCOMM_SDLC or VPOSCOMM_ASYNC or VPOSCOMM_ETHER or VPOSCOMM_GPRS or VPOSCOMM_CDMA
uint _uiCommDisconnect(uint uiCommType);

// 13.7 ��������״̬
// in  : uiCommType  : �������� VPOSCOMM_SDLC or VPOSCOMM_ASYNC or VPOSCOMM_ETHER or VPOSCOMM_GPRS or VPOSCOMM_CDMA
uint _uiCommKeepConnect(uint uiCommType);

// 13.8 ��ȡ������Ϣ
// in  : uiCommType  : �������� VPOSCOMM_SDLC or VPOSCOMM_ASYNC or VPOSCOMM_ETHER or VPOSCOMM_GPRS or VPOSCOMM_CDMA
// out : pCommInfo   : ������Ϣ
uint _uiCommGetInfo(uint uiCommType, VPOSCOMM_INFO *pCommInfo);

// 13.9 ��������
// in  : uiCommType  : �������� VPOSCOMM_SDLC or VPOSCOMM_ASYNC or VPOSCOMM_ETHER or VPOSCOMM_GPRS or VPOSCOMM_CDMA
//       psSendData  : ���͵�����
//       uiSendLen   : ���͵����ݳ���
uint _uiCommSendData(uint uiCommType, uchar *psSendData, uint uiSendLen);

// 13.10 ��������
// in  : uiCommType  : �������� VPOSCOMM_SDLC or VPOSCOMM_ASYNC or VPOSCOMM_ETHER or VPOSCOMM_GPRS or VPOSCOMM_CDMA
//       piRecvLen   : �������ݻ�������С
//       uiTimeout   : ��ʱʱ��(��)
// out : psRecvData  : ���յ�����
//       piRecvLen   : ���յ����ݳ���
uint _uiCommRecvData(uint uiCommType, uchar *psRecvData, uint *puiRecvLen, uint uiTimeout);

// 14. ��������
// 14.1 �����������ֽ����
// In       : psVect1  : Ŀ�괮
//            psVect2  : Դ��
//            iLength  : �ֽ���
void vXor(uchar *psVect1, const uchar *psVect2, int iLength);

// 14.2 �����������ֽڻ�
// In       : psVect1  : Ŀ�괮
//            psVect2  : Դ��
//            iLength  : �ֽ���
void vOr(uchar *psVect1, const uchar *psVect2, int iLength);

// 14.3 �����������ֽ���
// In       : psVect1  : Ŀ�괮
//            psVect2  : Դ��
//            iLength  : �ֽ���
void vAnd(uchar *psVect1, const uchar *psVect2, int iLength);

// 14.4 ��������Դ���ֽ��˫�����ȿɶ���16���ƴ�
// In       : psIn     : Դ��
//            iLength  : Դ������
// Out      : psOut    : Ŀ�괮
void vOneTwo(const uchar *psIn, int iLength, uchar *psOut);

// 14.5 ��������Դ���ֽ��˫�����ȿɶ���16���ƴ�, ����ĩβ��'\0'
// In       : psIn     : Դ��
//            iLength  : Դ������
// Out      : pszOut   : Ŀ�괮
void vOneTwo0(const uchar *psIn, int iLength, uchar *pszOut);

// 14.6 ���ɶ���16���Ʊ�ʾ��ѹ������һ�볤�ȵĶ����ƴ�
// In       : psIn     : Դ��
//            iLength  : Դ������
// Out      : psOut    : Ŀ�괮
// Attention: Դ������Ϊ�Ϸ���ʮ�����Ʊ�ʾ����Сд����
//            �������Ϊ�����������῿����������һλ��ż��
void vTwoOne(const uchar *psIn, int iLength, uchar *psOut);

// 14.7 �������ƴ�ת��ɳ�����
// In       : psBinString : �����ƴ�����λ��ǰ
//            iLength     : �����ƴ����ȣ���Ϊ{1,2,3,4}֮һ
// Ret      : ת���ĳ�����
ulong ulStrToLong(const uchar *psBinString, int iLength);

// 14.8 ��������ת��ɶ����ƴ�
// In       : ulLongNumber : Ҫת��ĳ�����
//            iLength      : ת��֮������ƴ�����
// Out      : psBinString  : ת���Ķ����ƴ�����λ��ǰ
void vLongToStr(ulong ulLongNumber, int iLength, uchar *psBinString);

// 14.9 ��ʮ�����ƿɶ���ת��ɳ�����
// In       : psHexString : ʮ�����ƴ�
//            iLength     : ʮ�����ƴ����ȣ�0-8
// Ret      : ת���ĳ�����
ulong ulHexToLong(const uchar *psHexString, int iLength);

// 14.10 ��������ת���ʮ�����ƿɶ���
// In       : ulLongNumber : Ҫת��ĳ�����
//            iLength      : ת��֮��ʮ�����ƴ�����
// Out      : psHexString  : ת����ʮ�����ƴ�
void vLongToHex(ulong ulLongNumber, int iLength, uchar *psHexString);

// 14.11 �����ִ�ת��ɳ�����
// In       : psString : ���ִ�������Ϊ�Ϸ������֣�����Ҫ'\0'��β
//            iLength  : ���ִ�����
// Ret      : ת���ĳ�����
ulong ulA2L(const uchar *psString, int iLength);

// 14.12 �ڴ渴��, ����memcpy()����Ŀ�괮ĩβ��'\0'
// In       : psSource  : Դ��ַ
//            iLength   : Ҫ�����Ĵ�����
// Out      : pszTarget : Ŀ���ַ
void vMemcpy0(uchar *pszTarget, const uchar *psSource, int iLength);

// 14.13 ��ѹ��BCD�봮ת��ɳ�����
// In       : psBcdString : BCD�봮������Ϊ�Ϸ���BCD�룬����Ҫ'\0'��β
//            iLength     : BCD�봮���ȣ�����С�ڵ���12
// Ret      : ת���ĳ�����
ulong ulBcd2L(const uchar *psBcdString, int iLength);

// 14.14 ��������ת���ѹ��BCD�봮
// In       : ulLongNumber : Ҫת��ĳ�����
//            iLength      : ת��֮��BCD�봮����, ����С�ڵ���12
// Out      : psBcdString  : ת����BCD�봮����λ��ǰ
void vL2Bcd(ulong ulLongNumber, int iLength, uchar *psBcdString);

// 14.15 ��������Դ���ֽ��˫�����ȿɶ���3X��ʽ��
// In       : psIn     : Դ��
//            iLength  : Դ������
// Out      : psOut    : Ŀ�괮
void vOneTwoX(const uchar *psIn, int iLength, uchar *psOut);

// 14.16 ��������Դ���ֽ��˫�����ȿɶ���3X��ʽ��, ����ĩβ��'\0'
// In       : psIn     : Դ��
//            iLength  : Դ������
// Out      : pszOut   : Ŀ�괮
void vOneTwoX0(const uchar *psIn, int iLength, uchar *pszOut);

# endif // # ifndef VPOSFACE_H
