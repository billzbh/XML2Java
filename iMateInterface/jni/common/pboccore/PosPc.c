#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "VposFace.h"
#include "PushApdu.h"

uchar gl_szDbgInfo[1024];

// ����֧������������ʽ�ӿ�
APDU_STATUS gl_ApduStatus;
APDU_IN     gl_LastApduIn;
APDU_OUT	gl_LastApduOut;

uchar gl_sLastApduIn[270];
uint  gl_uiLastApduInLen;
uchar gl_sLastApduOut[270];
uint  gl_uiLastApduOutLen;

// �ⲿ����Ķ�������������
#define MAX_IC_SLOT_NUM		10  // ���֧�ֵĿ�����
typedef struct {
	int   iSetFlag;			    // 0:δ����IC������, ��ȱʡ��Ϊ���� 1:������IC������, ʹ���趨�ĺ���
	uchar ucCardType;           // ��Ƭ����, 0:T=0 1:T=1
	int   (*pfiTestCard)(void);
	int   (*pfiResetCard)(uchar *psAtr);
	int   (*pfiDoApdu)(int iApduInLen, uchar *psApduIn, int *piApduOutLen, uchar *psApduOut);
	int   (*pfiCloseCard)(void);
} stIcFunc;
static stIcFunc sg_aIcFunc[MAX_IC_SLOT_NUM];

// 1.1. ��ʼ��POSϵͳ��ֻ���ڳ�����ʼ������һ�Ρ�
void  _vPosInit(void)
{
	memset(&sg_aIcFunc[0], 0, sizeof(sg_aIcFunc));
	srand(time(NULL));
	gl_uiLastApduInLen = 0;
	gl_uiLastApduOutLen = 0;
	memset(&gl_ApduStatus, 0, sizeof(gl_ApduStatus));
	memset(&gl_LastApduIn, 0, sizeof(gl_LastApduIn));
	memset(&gl_LastApduOut, 0, sizeof(gl_LastApduOut));
}

// 1.2. POSͣ������ͬ��POS���ܵı����У��µ硢����������ֹͣ���С�
void  _vPosClose()
{
}

// 1.3. ȡ��POS�ͺ�
// ��    �أ�POS�ͺ�
//           Ŀǰ֧�� POS_TYPE_PC POS_TYPE_ST2 POS_TYPE_TALENTO POS_TYPE_ARTEMA
uint _uiGetPosType(void)
{
	return(POS_TYPE_PC);
}

// 2.1. ȡ��Ļ����ʾ����
// ��    �أ���Ļ����ʾ����
uint  _uiGetVLines(void)
{
    return(0);
}

// 2.2. ȡ��Ļ����ʾ����
// ��    �أ���Ļ����ʾ����
uint  _uiGetVCols(void)
{
    return(0);
}

// 2.3. ���������Ļ
void  _vCls(void)
{
}

// 2.4. ��ĳ����ʾ�ַ���
// ���������uiLine  : Ҫ��ʾ���������ӵ�һ�п�ʼ
//           pszText : Ҫ��ʾ���ִ�
// ˵    �����粻���ڸ��У��������ء����ִ����ȳ�����Ļ��ȣ���ȥ�����Ĳ��֡�
void  _vDisp(uint uiLine, const uchar *puszTxt)
{
}

// 2.5. ��ĳ����ʾĳ���Ե��ַ���
// ���������uiLine  : Ҫ��ʾ���������ӵ�һ�п�ʼ
//           pszText : Ҫ��ʾ���ִ�
//           ucAttr  : ��ʾ���ԣ�������DISP_NORMAL��DISP_REVERSE��DISP_UNDERLINE�����
// ˵    �����粻���ڸ��У��������ء����ִ����ȳ�����Ļ��ȣ���ȥ�����Ĳ��֡�
void  _vDispX(uint uiLine, const uchar *puszTxt, uchar ucAttr)
{
}

// 2.6. ��ĳ��ĳ����ʾ�ַ���
// ���������uiLine  : Ҫ��ʾ���������ӵ�1�п�ʼ
//           uiCol   : Ҫ��ʾ���������ӵ�0�п�ʼ
//           pszText : Ҫ��ʾ���ִ�
// ˵    �����粻���ڸ��У��������ء����ִ����ȳ�����Ļ��ȣ����Ƶ���һ�С�
//           ���ִ�������Ļ���һ�У���ȥ�����Ĳ��֡�
void  _vDispAt(uint uiLine, uint uiCol, const uchar *puszTxt)
{
}

// 2.7. ��ĳ��ĳ�д�������ʾ�ַ���
// ���������uiLine  : Ҫ��ʾ���������ӵ�1�п�ʼ
//           uiCol   : Ҫ��ʾ���������ӵ�0�п�ʼ
//           pszText : Ҫ��ʾ���ִ�
//           ucAttr  : ��ʾ���ԣ�������DISP_NORMAL��DISP_REVERSE��DISP_UNDERLINE�����
// ˵    �����粻���ڸ��У��������ء����ִ����ȳ�����Ļ��ȣ����Ƶ���һ�С�
//           ���ִ�������Ļ���һ�У���ȥ�����Ĳ��֡�
void  _vDispAtX(uint uiLine, uint uiCol, const uchar *puszTxt, uchar ucAttr)
{
}

// 2.8. ����ĳ������
// ���������uiLine  : Ҫ���ݵ��к�
// ��    �أ�0       : �ɹ�����
//           1       : err
// ˵    ������ָ������������ͬ���Ա�������
uint _uiBackupLine(uint uiLine)
{
    return(1);
}

// 2.9. �ָ����һ���б���
// ˵    ���������һ�α��ݵ��лָ�ԭ��
void _vRestoreLine(void)
{
}

// 2.10. ��Ļ���ݱ���
// ��    �أ�0       : �ɹ�
//           1       : err
uint _uiBackupScreen(void)
{
    return(1);
}

// 2.11. ��Ļ���ݻָ�
// ��    �أ�0       : �ɹ�
//           1       : err
void _vRestoreScreen(void)
{
}

// 3.1. ����з񰴼�����������
// ��    �أ�0 ��û�а���
//           1 ����⵽��������ͨ��_uiGetKey()��ȡ
uint  _uiKeyPressed(void)
{
    return(0);
}

// 3.4. ���¶���һ��������ʹ�ü���������һ������
// ���������ucSource : �������������ڵļ���
//           ucTarget : �ض����ý����صļ���, ����ü�����ucSource��ͬ����ɾ���ü�ԭ�ȵĶ���
// ��    �أ�0        : �ɹ�
//           1        : �ض���ļ���̫��
// ˵    �������ֻ���ض���10������
uint  _uiMapKey(uchar ucSource, uchar ucTarget)
{
    return(0);
}

// 3.2. �ȴ�������ֱ����ȡ��һ������Ϊֹ
// ��    �أ�0x00 - 0xff ��������ASCII��
uint  _uiGetKey(void)
{
    return(0);
}

// 3.3. ������̻�����
void  _vFlushKey(void)
{
}

// 4.1. ��⿨Ƭ�Ƿ����
// ���������uiReader : ���⿨����
// ��    �أ�0        : �޿�
//           1        : �п�
// ˵    ����ֻ�ܼ���û�����
uint  _uiTestCard(uint uiReader)
{
	int iRet;

	if(uiReader >= MAX_IC_SLOT_NUM)
		return(0);

	if(sg_aIcFunc[uiReader].iSetFlag) {
		// ʹ�ô����IC����������
		iRet = sg_aIcFunc[uiReader].pfiTestCard();
	} else {
		// ʹ��ԭIC����������
		iRet = 0;
	}
	if(iRet)
		return(1);
    return(0);
}

// ����ATR�жϿ�ƬͨѶЭ��
// in  : iAtrLen : ATR����
//       psAtr   : ATR����
// ret : 0       : T=0
//       1       : T=1
static uchar ucGetCardCommType(uchar iAtrLen, uchar *psAtr)
{
         uchar i, T0;
         i=1;
         T0 = psAtr[i];
         if(T0&0x10) i++;
         if(T0&0x20) i++;
         if(T0&0x40) i++;
         if(T0&0x80)
         {
                   i++;
                   return (psAtr[i]&0x0f);    //TD1         T=1
         }
         return 0;
}

// 4.2. ��Ƭ��λ
// ���������uiReader    : ���⿨����
// ���������psResetData : ATR ����
// ��    �أ�0           : ��λʧ��
//           >0          : ��λ�ɹ�������ATR����
uint  _uiResetCard(uint uiReader, uchar *pusResetData)
{
    int   iAtrLen;

	if(uiReader >= MAX_IC_SLOT_NUM)
		return(0);

	if(sg_aIcFunc[uiReader].iSetFlag) {
		// ʹ�ô����IC����������
		iAtrLen = sg_aIcFunc[uiReader].pfiResetCard(pusResetData);
	} else {
		// ʹ��ԭIC����������
		iAtrLen = 0;
	}
    if(iAtrLen) {
        // ��λ�ɹ�, �жϿ�Ƭ����
		sg_aIcFunc[uiReader].ucCardType = ucGetCardCommType((uchar)iAtrLen, pusResetData);
		if(sg_aIcFunc[uiReader].ucCardType!=0 && sg_aIcFunc[uiReader].ucCardType!=1)
			return(0);
    }
	return(iAtrLen);
}

// 4.3. ��Ƭ�µ�
// ���������uiReader    : ���⿨����
// ��    �أ�0           : �ɹ�
//           1           : ʧ��
uint  _uiCloseCard(uint uiReader)
{
	int iRet;

	if(uiReader >= MAX_IC_SLOT_NUM)
		return(0);

	if(sg_aIcFunc[uiReader].iSetFlag) {
		// ʹ�ô����IC����������
		iRet = sg_aIcFunc[uiReader].pfiCloseCard();
	} else {
		// ʹ��ԭIC����������
		iRet = 0;
	}

    return(0);
}

// 4.4. ִ��IC��ָ��
// ���������uiReader : ���⿨����
//           pIn      : IC��ָ��ṹ
//           pOut     : IC�����ؽ��
// ��    �أ�0        : �ɹ�
//           1        : ʧ��
uint  _uiExchangeApdu(uint uiReader, APDU_IN *pIn, APDU_OUT *pOut)
{
	if(uiReader >= MAX_IC_SLOT_NUM)
		return(1);

    return(1);
}

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
uint _uiDoApdu(uint uiReader, uint uiInLen, uchar *psIn, uint *puiOutLen, uchar *psOut, uint uiProtocol)
{
	uchar sApduIn[300], sApduOut[300];
	uint  uiApduInLen, uiApduOutLen;
    uint  uiOriginalRet;
	uint  uiStatus;
    uint  uiRet;
    uint  uiCase; // apdu case 1 - 4

	gl_uiLastApduInLen = 0;
	gl_uiLastApduOutLen = 0;
	if(uiInLen > sizeof(gl_sLastApduIn))
		return(1);
	gl_uiLastApduInLen = uiInLen;
	memcpy(gl_sLastApduIn, psIn, uiInLen);

	if(uiReader >= MAX_IC_SLOT_NUM)
		return(1);

	uiRet = uiPushDoApdu(uiReader, uiInLen, psIn, puiOutLen, psOut);
	if(uiRet == 1)
		return(1);
	if(uiRet == 0) {
		if(*puiOutLen <= sizeof(gl_sLastApduOut)) {
			gl_sLastApduIn[0] |= 0x01; // Class���λΪ1��ʾ��ָ��Ϊ����������ָ��
			gl_uiLastApduOutLen = *puiOutLen;
			memcpy(gl_sLastApduOut, psOut, *puiOutLen);
		}
		return(0);
	}

	// �Ż�apdu����ָ�����������, ʹ�õ�ָ�ʽ
	if(uiInLen > sizeof(sApduIn))
		return(1);
	// �ж�case
    if(uiInLen == 4)
        uiCase = 1; // case 1
    else if(uiInLen == 5)        
        uiCase = 2; // case 2
    else if((int)uiInLen == 5+psIn[4])
        uiCase = 3; // case 3
    else if((int)uiInLen == 5+psIn[4]+1)
        uiCase = 4; // case 4
    else
        return(1); // error format

	uiApduInLen = uiInLen;
	memcpy(sApduIn, psIn, uiApduInLen);
	uiApduOutLen = 0;
	memset(sApduOut, 0, sizeof(sApduOut));
	if(sg_aIcFunc[uiReader].iSetFlag) {
		// ʹ�ô����IC����������
		uiRet = sg_aIcFunc[uiReader].pfiDoApdu(uiApduInLen, sApduIn, &uiApduOutLen, sApduOut);
	} else {
		// ʹ��ԭIC����������
		uiRet = 1;
	}
	if(uiRet)
		return(1); // apduָ��ִ��ʧ��
	if(uiApduOutLen<2 || uiApduOutLen>sizeof(sApduOut))
		return(1); // apdu�����쳣
	uiStatus = (uint)ulStrToLong(sApduOut+uiApduOutLen-2, 2);
	switch(uiProtocol) {
    case APDU_AUTO_GET_RESP: // �Զ�����61xx��6Cxx
        if((uiStatus&0xFF00) == 0x6100) {
            memcpy(sApduIn, "\x00\xc0\x00\x00", 4);
			sApduIn[4] = uiStatus&0xFF;
			uiInLen = 5;
			if(sg_aIcFunc[uiReader].iSetFlag) {
				// ʹ�ô����IC����������
				uiRet = sg_aIcFunc[uiReader].pfiDoApdu(uiInLen, sApduIn, &uiApduOutLen, sApduOut);
			} else {
				// ʹ��ԭIC����������
				uiRet = 1;
			}
        } else if((uiStatus&0xFF00) == 0x6C00) {
			sApduIn[uiApduInLen-1] = uiStatus&0xFF;
			if(sg_aIcFunc[uiReader].iSetFlag) {
				// ʹ�ô����IC����������
				uiRet = sg_aIcFunc[uiReader].pfiDoApdu(uiApduInLen, sApduIn, &uiApduOutLen, sApduOut);
			} else {
				// ʹ��ԭIC����������
				uiRet = 1;
			}
        }
        break;
    case APDU_EMV:           // EMV�淶
        uiOriginalRet = 0;
        if((uiStatus&0xFF00) == 0x6100) {
            memcpy(sApduIn, "\x00\xc0\x00\x00", 4);
			sApduIn[4] = uiStatus&0xFF;
			uiInLen = 5;
			if(sg_aIcFunc[uiReader].iSetFlag) {
				// ʹ�ô����IC����������
				uiRet = sg_aIcFunc[uiReader].pfiDoApdu(uiInLen, sApduIn, &uiApduOutLen, sApduOut);
			} else {
				// ʹ��ԭIC����������
				uiRet = 1;
			}
        } else if((uiStatus&0xFF00) == 0x6C00) {
			sApduIn[uiApduInLen-1] = uiStatus&0xFF;
			if(sg_aIcFunc[uiReader].iSetFlag) {
				// ʹ�ô����IC����������
				uiRet = sg_aIcFunc[uiReader].pfiDoApdu(uiApduInLen, sApduIn, &uiApduOutLen, sApduOut);
			} else {
				// ʹ��ԭIC����������
				uiRet = 1;
			}
        } else if((uiStatus&0xFF00) == 0x6300 || (uiStatus&0xFF00) == 0x6200 || ((uiStatus&0xF000)==0x9000 && uiStatus!=0x9000)) {
            if(sg_aIcFunc[uiReader].ucCardType==1 || uiCase!=4)
	     		break; // T=1������Ҫ�����⴦��, case4���ⲻ��Ҫ�����⴦��
	        // T=0�� and Case 4
			// 62XX, 63XX, 9XXX(Not9000), ��Ҫ��ȡ����, Refer to EMV2008 book1 9.3, P106
			//                                                 book1 9.3.1.1.4 4.(b)
	        uiOriginalRet = uiStatus;
            memcpy(sApduIn, "\x00\xc0\x00\x00\x00", 5);
			uiInLen = 5;
			if(sg_aIcFunc[uiReader].iSetFlag) {
				// ʹ�ô����IC����������
				uiRet = sg_aIcFunc[uiReader].pfiDoApdu(uiInLen, sApduIn, &uiApduOutLen, sApduOut);
			} else {
				// ʹ��ԭIC����������
				uiRet = 1;
			}
			if(uiRet)
				return(1);
			if(uiApduOutLen<2 || uiApduOutLen>sizeof(sApduOut))
				return(1); // apdu�����쳣
			uiStatus = (uint)ulStrToLong(sApduOut+uiApduOutLen-2, 2);
  	        if((uiStatus&0xFF00)==0x6C00 || (uiStatus&0xFF00)==0x6100) {
				sApduIn[uiInLen-1] = uiStatus&0xFF;
				if(sg_aIcFunc[uiReader].iSetFlag) {
					// ʹ�ô����IC����������
					uiRet = sg_aIcFunc[uiReader].pfiDoApdu(uiInLen, sApduIn, &uiApduOutLen, sApduOut);
				} else {
					// ʹ��ԭIC����������
					uiRet = 1;
				}
		    	if(uiRet)
    		    	return(1);
			}
            uiStatus = uiOriginalRet; // �ָ�62XX 63XX 9XXX������
			vLongToStr((ulong)uiStatus, 2, sApduOut+uiApduOutLen-2);
        }
        break;
    case APDU_NORMAL:        // ������61xx��6Cxx
    case APDU_SI:            // �籣2.0Э��, �ݲ�֧��, ��ͬ�ڲ�����61xx��6Cxx
    default:                 // δ֪Э��, ��ͬ�ڲ�����61xx��6Cxx
        break;
    } // switch(uiProtocol)

	if(uiRet)
		  return(1);
	*puiOutLen = uiApduOutLen;
	memcpy(psOut, sApduOut, uiApduOutLen);
	if(*puiOutLen <= sizeof(gl_sLastApduOut)) {
		gl_uiLastApduOutLen = *puiOutLen;
		memcpy(gl_sLastApduOut, psOut, *puiOutLen);
	}
    return(0);
}

// 4.6. ���ö�������, �����ⲿ�ṩIC���ӿ�ʱ
// �������: uiReader     : ������, 0-9, �Ժ�Ըÿ����ŵĲ������������õĺ�������
//           ����4�����������һ������NULL, ���ʾȡ��֮ǰ�Ըÿ������õĺ���
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
				        int (*pfiCloseCard)(void))
{
	if(uiReader >= MAX_IC_SLOT_NUM)
		return(1);

	if(pfiTestCard && pfiResetCard && pfiDoApdu && pfiCloseCard) {
		// �趨����
		sg_aIcFunc[uiReader].iSetFlag = 1;
		sg_aIcFunc[uiReader].pfiTestCard = pfiTestCard;
		sg_aIcFunc[uiReader].pfiResetCard = pfiResetCard;
		sg_aIcFunc[uiReader].pfiDoApdu = pfiDoApdu;
		sg_aIcFunc[uiReader].pfiCloseCard = pfiCloseCard;
	} else {
		// ȡ���趨����
		memset(&sg_aIcFunc[uiReader], 0, sizeof(sg_aIcFunc[uiReader]));
	}
	return(0);
}

// 5.1. ȡ��ӡ�����
// ��    �أ�0  : �޴�ӡ��
//           >0 : ��ӡ���ɴ�ӡ�������
uint  _uiGetPCols(void)
{
    return(0);
}

// 5.2. ��ӡ����
// ���������pszText : Ҫ��ӡ���ִ�
// ��    �أ�0 : ��ӡ�ɹ�
//           1 : ��ӡ����
// ˵    ������ӡ�ִ�������ӡ�������ʱ���Ƶ���һ�С�
//           '\n'�ַ��ᵼ�»��С�
uint  _uiPrint(const uchar *puszTxt)
{
    return(1);
}


// 6.1. ���õ�ǰϵͳʱ��
// ���������pszTime : Ҫ���õ�ʱ�䣬��ʽ:YYYYMMDDHHMMSS
// ��    �أ�0       : �ɹ�
//           1       : ʧ��
uint  _uiSetTime(const uchar *pszTime)
{
    return(1);
}

// 6.2. ��ȡϵͳʱ��
// ���������pszTime : ��ǰϵͳʱ�䣬��ʽ:YYYYMMDDHHMMSS
void  _vGetTime( uchar *puszTime)
{
    time_t t;
    struct tm tm;
 
    t = time( NULL );
    memcpy(&tm, localtime(&t), sizeof(struct tm));
    sprintf(puszTime, "%04d%02d%02d%02d%02d%02d",
                      tm.tm_year+1900, tm.tm_mon+1, tm.tm_mday,
                      tm.tm_hour, tm.tm_min, tm.tm_sec);
}

void  _vSetTimer(ulong *pulTimer, ulong ulNTick)
{
	*pulTimer = ((rand()<<16L) + rand()); // ��֧��Timer, ���������
}

uint  _uiTestTimer(ulong ulTimer)
{
    return(0);
}

// 6.5. �ӳ�һ��ʱ��
// ���������uiNTick : �ӳٵ�ʱ�䣬��λΪ0.01��
void  _vDelay(uint uiNTick)
{
}

// 7.1. ������չ�ڴ��С
// ���������ulSize : Ҫ���õ���չ�ڴ��С����λΪ�ֽ�
// ��    �أ�0      : ���óɹ�
//           1      : ������������ռ�������32�ֽ�
//           2      : ����ʧ��
//           >2     : ��չ�ڴ治�㣬����ֵΪ��������չ�ڴ��С
ulong _ulSetXMemSize(ulong ulSize)
{
    return(2);
}

// 7.2. ����չ�ڴ�
// ���������ulOffset : ��չ�ڴ����ֽ�Ϊ��λ��ƫ����
//           uiLen    �����ֽ�Ϊ��λ�ĳ���
// ���������pBuffer  : ��ȡ������
// ��    �أ�0        : �ɹ�
//           1        : ʧ��
uint  _uiXMemRead(void *pusBuffer, ulong ulOffset, uint uiLen)
{
    return(1);
}

// 7.3. д��չ�ڴ�
// ���������pBuffer  : Ҫд������
//           ulOffset : ��չ�ڴ����ֽ�Ϊ��λ��ƫ����
//           uiLen    �����ֽ�Ϊ��λ�ĳ���
// ��    �أ�0        : �ɹ�
//           1        : ʧ��
uint  _uiXMemWrite(const void *pusBuffer, ulong ulOffset, uint uiLen)
{
    return(1);
}

// 8.1. ��ſ�������
// ��    �أ�0 : �ɹ�
//           1 : ʧ��
uint _uiMagReset(void)
{
    return(1);
}

// 8.2. ����Ƿ��дſ�ˢ��
// ��    �أ�0 : �޿�
//           1 : �дſ�ˢ��
uint _uiMagTest(void)
{
    return(0);
}

// 8.3. ���ŵ���Ϣ
// ���������uiTrackNo : �ŵ��ţ�1-3
// ���������pszBuffer : �ŵ���Ϣ
// ��    �أ�0 : ��ȡ��Ϣ��ȷ
//           1 : û������
//           2 : ��⵽����
uint _uiMagGet(uint uiTrackNo, uchar *pszBuffer)
{
    return(1);
}

// 8.4. д�ŵ���Ϣ
// ���������pszTrack2 : 2�ŵ���Ϣ
//         ��pszTrack3 : 3�ŵ���Ϣ
void _vMagWrite(uchar *pszTrack2, uchar *pszTrack3)
{
}

// 9.1. ���õ�ǰ���ں�
// ���������ucPortNo : �´��ں�, �Ӵ���1��ʼ
// ��    �أ�ԭ�ȵĴ��ں�
uchar _ucAsyncSetPort(uchar ucPortNo)
{
    return(1);
}

// 9.2. �򿪵�ǰ����
// ���������ulBaud   : ������
//           ucParity : ��żУ���־��COMM_EVEN��COMM_ODD��COMM_NONE
//           ucBits   : ����λ����
//           ucStop   : ֹͣλ����
// ��    �أ�0        : �ɹ�
//           1        : ��������
//           2        : ʧ��
uchar _ucAsyncOpen(ulong ulBaud, uchar ucParity, uchar ucBits, uchar ucStop)
{
    return(2);
}

// 9.3. �رյ�ǰ����
// ��    �أ�0        : �ɹ�
//           1        : ʧ��
uchar _ucAsyncClose(void)
{
    return(1);
}

// 9.4. ��λ��ǰ���ڣ���ս��ջ�����
// ��    �أ�0        : �ɹ�
//           1        : ʧ��
uchar _ucAsyncReset(void)
{
    return(1);
}

// 9.5. ����һ���ֽ�
// ���������ucChar : Ҫ���͵��ַ�
// ��    �أ�0      : �ɹ�
//           1      : ʧ��
uchar _ucAsyncSend(uchar ucChar)
{
    return(1);
}

// 9.6. �����û���ַ��յ�
// ��    �أ�0 : û���ַ��յ�
//           1 : ���ַ��յ�
uchar _ucAsyncTest(void)
{
    return(0);
}

// 9.7. ����һ���ַ���û����һֱ�ȴ�
// ��    �أ����յ����ַ�
uchar _ucAsyncGet(void)
{
    return(0);
}

// 9.8.	����һ������
// ���������pBuf  : Ҫ���͵��ַ�
//           uiLen : ���ݳ���
// ��    �أ�0     : �ɹ�
//           1     : ʧ��
uchar _ucAsyncSendBuf(void *pBuf, uint uiLen)
{
    return(1);
}

// 9.9.	����ָ�����ȵ�һ������
// ���������pBuf      : Ҫ���͵�����
//           uiLen     : ���ݳ���
//           uiTimeOut : ����Ϊ��λ�ĳ�ʱʱ��
// ��    �أ�0         : �ɹ�
//           1         : ��ʱ
uchar _ucAsyncGetBuf(void *pBuf, uint uiLen, uint uiTimeOut)
{
    return(1);
}

// 9.10. ���Ž�������
// ���������pTelConfig : ���Žṹָ�룬�� TEL_CONFIG �ṹ
//           uiTimeOut  : ��ʱʱ�䣬��λΪ��
// ��    �أ�0          : �ɹ�
//           1          : ʧ�ܣ�����_uiTelGetStatus()ȡ�ô���
uchar _ucTelConnect(const TEL_CONFIG *pTelConfig, uint uiTimeOut)
{
    return(1);
}

// 9.11. �������
// ��    �أ�0 : �ɹ�
//           1 : ʧ�ܣ�����_uiTelGetStatus()ȡ�ô���
uchar _ucTelDisconnect(void)
{
    return(0);
}

// 9.12. ȡ�ز������ӵ�ǰ״̬
// ��    �أ�0x0000 : ����
//           0x01?? : ��������
//           0x02?? : ��������λ�ֽڱ�ʾһ��״̬
//           0x03?? : ���󣬵�λ�ֽڱ�ʾ����ԭ��
uint _uiTelGetStatus(void)
{
    return(0x0399);
}

// 9.13. �����߷�������
// ���������uiLength : ���ͱ��ĳ���
//           pBuffer  : ���ͱ�������
// ��    �أ�0        : ����
//           1        : ʧ�ܣ�����_uiTelGetStatus()ȡ�ô���
uchar _ucTelSend(uint uiLength, void const *pBuffer)
{
    return(1);
}

// 9.14. �����߽�������
// ���������puiLength : ���ձ��ĳ���
//           pBuffer   : ���ձ�������
//           uiTimeOut : ��ʱʱ�䣬��λΪ��
// ��    �أ�0         : ����
//           1         : ʧ�ܣ�����_uiTelGetStatus()ȡ�ô���
uchar _ucTelGet(uint *puiLength, void *pBuffer, uint uiTimeOut)
{
    return(1);
}

// 10.1. ��һ����
void  _vBuzzer(void)
{
}

// 10.2. ȡһ�����
// ��    �أ�0-255�������
uchar _ucGetRand(void)
{
    return(rand());
}

// 10.3. ��DES����
// ���������uiMode   : ENCRYPT -> ���ܣ�DECRYPT -> ����
//                      TRI_ENCRYPT -> 3Des���ܣ�TRI_DECRYPT -> 3Des����
//           psSource : Դ
//           psKey    : ��Կ
// ���������psResult : Ŀ��
extern void _fDes(char cryp_decrypt,unsigned char *DES_DATA,unsigned char *DESKEY,unsigned char *DES_RESULT);
void _vDes(uint uiMode, uchar *psSource, uchar *psKey, uchar *psResult)
{
    uchar sBuf[8];
    switch(uiMode) {
        case ENCRYPT:
        case DECRYPT:
            _fDes((char)uiMode, (uchar*)psSource, (uchar*)psKey, psResult);
            break;
        case TRI_ENCRYPT:
            _fDes(ENCRYPT, (uchar*)psSource, (uchar*)psKey, psResult);
            _fDes(DECRYPT, (uchar*)psResult, (uchar*)psKey+8, sBuf);
            _fDes(ENCRYPT, (uchar*)sBuf, (uchar*)psKey, psResult);
            break;
        case TRI_DECRYPT:
            _fDes(DECRYPT, (uchar*)psSource, (uchar*)psKey, psResult);
            _fDes(ENCRYPT, (uchar*)psResult, (uchar*)psKey+8, sBuf);
            _fDes(DECRYPT, (uchar*)sBuf, (uchar*)psKey, psResult);
			break;
    }
}

// 11.1. Debug ʹ��, ���ܣ���ʾ�ļ���������
void _vDispAssert(uchar *pszFileName, ulong ulLineNo)
{
}

// 11.2. Debug ʹ��, ���ܣ���ʾ������pPointer��ָ���ݣ�������16��������ʾ6�ֽ�
void _vDispDbgInfo(uchar *pszFileName,ulong ulLineNo, void *pPointer)
{
}

// 13. ���繦�ܺ���(Ҳ������SDLC���첽ͨѶ), �ӿڲ�����VeriFone520 vCommCE�ӿ�
// 13.1. �����ʼ��
uint _uiCommInit(void)
{
	return(COM_OK);
}

// 13.2 ע������
// in  : uiCommType  : �������� VPOSCOMM_SDLC or VPOSCOMM_ASYNC or VPOSCOMM_ETHER or VPOSCOMM_GPRS or VPOSCOMM_CDMA
//       pCommConfig : �������
//       uiTimeout   : ��ʱʱ��(��), 0��ʾ���ȴ�,��������
// ret : VPOSCOMM_
// Note: ������ע������ſ�ʹ������
uint _uiCommLogin(uint uiCommType, VPOSCOMM_CONFIG *pCommConfig, uint uiTimeout)
{
	return(COM_OK);
}

// 13.2 ע������
// in  : uiCommType  : �������� VPOSCOMM_SDLC or VPOSCOMM_ASYNC or VPOSCOMM_ETHER or VPOSCOMM_GPRS or VPOSCOMM_CDMA
uint _uiCommLogOut(uint uiCommType)
{
	return(COM_OK);
}

// 13.3 ����ע��״̬
// in  : uiCommType  : �������� VPOSCOMM_SDLC or VPOSCOMM_ASYNC or VPOSCOMM_ETHER or VPOSCOMM_GPRS or VPOSCOMM_CDMA
// Note: ������ֶ�ʧ��ע��, ��ͼ����ע��
uint _uiCommKeepLogin(uint uiCommType)
{
	return(COM_OK);
}

// 13.4 ���ӷ�����
// in  : uiCommType  : �������� VPOSCOMM_SDLC or VPOSCOMM_ASYNC or VPOSCOMM_ETHER or VPOSCOMM_GPRS or VPOSCOMM_CDMA
//       psParam1    : ����1, ����VPOSCOMM_SDLC VPOSCOMM_ASYNC : �绰����
//                            ����VPOSCOMM_ETHER VPOSCOMM_GPRS VPOSCOMM_CDMA : Ip��ַ
//       psParam2    : ����2, ����VPOSCOMM_SDLC VPOSCOMM_ASYNC : ���ߺ���
//                            ����VPOSCOMM_ETHER VPOSCOMM_GPRS VPOSCOMM_CDMA : �˿ں�
//       psParam3    : ����3, ����VPOSCOMM_SDLC VPOSCOMM_ASYNC : "0"-����Ⲧ���� "1"-��Ⲧ����
//                            ����VPOSCOMM_ETHER VPOSCOMM_GPRS VPOSCOMM_CDMA : "0"-TCP "1"-UDP
//       uiTimeout   : ��ʱʱ��(��), 0��ʾ���ȴ�,��������
uint _uiCommConnect(uint uiCommType, uchar *psParam1, uchar *psParam2, uchar *psParam3, uint uiTimeout)
{
    return(COM_ERR_NO_CONNECT); /* can't connect */
}

// 13.5 �����ͨ��
// in  : uiCommType  : �������� VPOSCOMM_SDLC or VPOSCOMM_ASYNC or VPOSCOMM_ETHER or VPOSCOMM_GPRS or VPOSCOMM_CDMA
uint _uiCommTestConnect(uint uiCommType)
{
	return(COM_ERR_NO_CONNECT);
}

// 13.6 �Ͽ�����
// in  : uiCommType  : �������� VPOSCOMM_SDLC or VPOSCOMM_ASYNC or VPOSCOMM_ETHER or VPOSCOMM_GPRS or VPOSCOMM_CDMA
uint _uiCommDisconnect(uint uiCommType)
{
	return(COM_OK);
}

// 13.7 ��������״̬
// in  : uiCommType  : �������� VPOSCOMM_SDLC or VPOSCOMM_ASYNC or VPOSCOMM_ETHER or VPOSCOMM_GPRS or VPOSCOMM_CDMA
uint _uiCommKeepConnect(uint uiCommType)
{
	return(COM_OK);
}

// 13.8 ��ȡ������Ϣ
// in  : uiCommType  : �������� VPOSCOMM_SDLC or VPOSCOMM_ASYNC or VPOSCOMM_ETHER or VPOSCOMM_GPRS or VPOSCOMM_CDMA
// out : pCommInfo   : ������Ϣ
uint _uiCommGetInfo(uint uiCommType, VPOSCOMM_INFO *pCommInfo)
{
	return(COM_ERR_SYS);
}

// 13.9 ��������
// in  : uiCommType  : �������� VPOSCOMM_SDLC or VPOSCOMM_ASYNC or VPOSCOMM_ETHER or VPOSCOMM_GPRS or VPOSCOMM_CDMA
//       psSendData  : ���͵�����
//       uiSendLen   : ���͵����ݳ���
uint _uiCommSendData(uint uiCommType, uchar *psSendData, uint uiSendLen)
{
    return (COM_ERR_SEND);
}

// 13.10 ��������
// in  : uiCommType  : �������� VPOSCOMM_SDLC or VPOSCOMM_ASYNC or VPOSCOMM_ETHER or VPOSCOMM_GPRS or VPOSCOMM_CDMA
//       piRecvLen   : �������ݻ�������С
//       uiTimeout   : ��ʱʱ��(��)
// out : psRecvData  : ���յ�����
//       piRecvLen   : ���յ����ݳ���
uint _uiCommRecvData(uint uiCommType, uchar *psRecvData, uint *piRecvLen, uint uiTimeout)
{
    return(COM_ERR_RECV);
}

// 14. ��������
// 14.1 �����������ֽ����
// In       : psVect1  : Ŀ�괮
//            psVect2  : Դ��
//            iLength  : �ֽ���
void vXor(uchar *psVect1, const uchar *psVect2, int iLength)
{
    int i;

    for(i=0; i<iLength; i++)
        psVect1[i] ^= psVect2[i];
}

// 14.2 �����������ֽڻ�
// In       : psVect1  : Ŀ�괮
//            psVect2  : Դ��
//            iLength  : �ֽ���
void vOr(uchar *psVect1, const uchar *psVect2, int iLength)
{
    int i;

    for(i=0; i<iLength; i++)
        psVect1[i] |= psVect2[i];
}

// 14.3 �����������ֽ���
// In       : psVect1  : Ŀ�괮
//            psVect2  : Դ��
//            iLength  : �ֽ���
void vAnd(uchar *psVect1, const uchar *psVect2, int iLength)
{
    int i;

    for(i=0; i<iLength; i++)
        psVect1[i] &= psVect2[i];
}

// 14.4 ��������Դ���ֽ��˫�����ȿɶ���16���ƴ�
// In       : psIn     : Դ��
//            iLength  : Դ������
// Out      : psOut    : Ŀ�괮
void vOneTwo(const uchar *psIn, int iLength, uchar *psOut)
{
    static const uchar aucHexToChar[17] = "0123456789ABCDEF";
    int iCounter;

    for(iCounter = 0; iCounter < iLength; iCounter++){
        psOut[2*iCounter] = aucHexToChar[((psIn[iCounter] >> 4)) & 0x0F];
        psOut[2*iCounter+1] = aucHexToChar[(psIn[iCounter] & 0x0F)];
    }
}

// 14.5 ��������Դ���ֽ��˫�����ȿɶ���16���ƴ�, ����ĩβ��'\0'
// In       : psIn     : Դ��
//            iLength  : Դ������
// Out      : pszOut   : Ŀ�괮
void vOneTwo0(const uchar *psIn, int iLength, uchar *pszOut)
{
    vOneTwo(psIn, iLength, pszOut);
	if(iLength < 0)
		iLength = 0;
    pszOut[2*iLength]=0;
}

// 14.6 ���ɶ���16���Ʊ�ʾ��ѹ������һ�볤�ȵĶ����ƴ�
// In       : psIn     : Դ��
//            iLength  : Դ������
// Out      : psOut    : Ŀ�괮
// Attention: Դ������Ϊ�Ϸ���ʮ�����Ʊ�ʾ����Сд����
//            �������Ϊ�����������῿����������һλ��ż��
void vTwoOne(const uchar *psIn, int iLength, uchar *psOut)
{
    uchar ucTmp;
    int   i;

    for(i=0; i<iLength; i+=2) {
        ucTmp = psIn[i];
        if(ucTmp > '9')
            ucTmp = toupper(ucTmp) - 'A' + 0x0a;
        else
            ucTmp &= 0x0f;
        psOut[i/2] = ucTmp << 4;

        ucTmp = psIn[i+1];
        if(ucTmp > '9')
            ucTmp = toupper(ucTmp) - 'A' + 0x0a;
        else
            ucTmp &= 0x0f;
        psOut[i/2] += ucTmp;
    } // for(i=0; i<uiLength; i+=2) {
}

// 14.7 �������ƴ�ת��ɳ�����
// In       : psBinString : �����ƴ�����λ��ǰ
//            iLength     : �����ƴ����ȣ���Ϊ{1,2,3,4}֮һ
// Ret      : ת���ĳ�����
ulong ulStrToLong(const uchar *psBinString, int iLength)
{
    ulong  l;
    int    i;

    for(i=0, l=0l; i<iLength; i++)
        l = l*256 + (long)psBinString[i];
    return(l);
}

// 14.8 ��������ת��ɶ����ƴ�
// In       : ulLongNumber : Ҫת��ĳ�����
//            iLength      : ת��֮������ƴ�����
// Out      : psBinString  : ת���Ķ����ƴ�����λ��ǰ
void vLongToStr(ulong ulLongNumber, int iLength, uchar *psBinString)
{
    int i;

    for(i = iLength-1; i>=0; i--, ulLongNumber/=256l )
        psBinString[i] = (uchar)(ulLongNumber % 256);
}

// 14.9 ��ʮ�����ƿɶ���ת��ɳ�����
// In       : psHexString : ʮ�����ƴ�
//            iLength     : ʮ�����ƴ����ȣ�0-8
// Ret      : ת���ĳ�����
ulong ulHexToLong(const uchar *psHexString, int iLength)
{
    ulong  l;
    int    i;
	ulong  ulDigit;

    for(i=0, l=0l; i<iLength; i++) {
		if(psHexString[i]>='0' && psHexString[i]<='9')
			ulDigit = psHexString[i] - '0';
		else if(psHexString[i]>='A' && psHexString[i]<='Z')
			ulDigit = psHexString[i] - 'A' + 10;
		else if(psHexString[i]>='a' && psHexString[i]<='z')
			ulDigit = psHexString[i] - 'a' + 10;
        l = l*16 + ulDigit;
	}
    return(l);
}

// 14.10 ��������ת���ʮ�����ƿɶ���
// In       : ulLongNumber : Ҫת��ĳ�����
//            iLength      : ת��֮��ʮ�����ƴ�����
// Out      : psHexString  : ת����ʮ�����ƴ�
void vLongToHex(ulong ulLongNumber, int iLength, uchar *psHexString)
{
	uchar sBuf[4], szBuf[8];
	vLongToStr(ulLongNumber, 4, sBuf);
	vOneTwo(sBuf, 4, szBuf);
	memset(psHexString, '0', iLength);
	if(iLength > 8)
		memcpy(psHexString+iLength-8, szBuf, 8);
	else
		memcpy(psHexString, szBuf+8-iLength, iLength);
}

// 14.11 �����ִ�ת��ɳ�����
// In       : psString : ���ִ�������Ϊ�Ϸ������֣�����Ҫ'\0'��β
//            iLength  : ���ִ�����
// Ret      : ת���ĳ�����
ulong ulA2L(const uchar *psString, int iLength)
{
    ulong  l;
    int    i;

    for(i=0, l=0l; i<iLength; i++) {
        if(psString[i]<'0' || psString[i]>'9')
            break;
        l = l*10 + psString[i]-'0';
    }
    return(l);
}

// 14.12 �ڴ渴��, ����memcpy()����Ŀ�괮ĩβ��'\0'
// In       : psSource  : Դ��ַ
//            iLength   : Ҫ�����Ĵ�����
// Out      : pszTarget : Ŀ���ַ
void vMemcpy0(uchar *pszTarget, const uchar *psSource, int iLength)
{
    memcpy(pszTarget, psSource, iLength);
    pszTarget[iLength] = 0;
}

// 14.13 ��ѹ��BCD�봮ת��ɳ�����
// In       : psBcdString : BCD�봮������Ϊ�Ϸ���BCD�룬����Ҫ'\0'��β
//            iLength     : BCD�봮���ȣ�����С�ڵ���12
// Ret      : ת���ĳ�����
ulong ulBcd2L(const uchar *psBcdString, int iLength)
{
    uchar  sTmp[24];

    ASSERT(iLength <= 12 && iLength >= 0);
    if(iLength>12 || iLength<0)
        return(0);
    vOneTwo(psBcdString, iLength, sTmp);
    return(ulA2L(sTmp, iLength*2));
}

// 14.14 ��������ת���ѹ��BCD�봮
// In       : ulLongNumber : Ҫת��ĳ�����
//            iLength      : ת��֮��BCD�봮����, ����С�ڵ���12
// Out      : psBcdString  : ת����BCD�봮����λ��ǰ
void vL2Bcd(ulong ulLongNumber, int iLength, uchar *psBcdString)
{
    uchar sTmp[24+1];

    ASSERT(iLength<=12 && iLength>0);
    if(iLength > 12 || iLength < 0)
        return;
    sprintf((char*)sTmp, "%024lu", ulLongNumber);
    vTwoOne(sTmp+24-iLength*2, iLength*2, psBcdString);
}

// 14.15 ��������Դ���ֽ��˫�����ȿɶ���3X��ʽ��
// In       : psIn     : Դ��
//            iLength  : Դ������
// Out      : psOut    : Ŀ�괮
void vOneTwoX(const uchar *psIn, int iLength, uchar *psOut)
{
    int iCounter;

    for(iCounter = 0; iCounter < iLength; iCounter++){
        psOut[2*iCounter] = 0x30 + (((psIn[iCounter] >> 4)) & 0x0F);
        psOut[2*iCounter+1] = 0x30 + (psIn[iCounter] & 0x0F);
    }
}

// 14.16 ��������Դ���ֽ��˫�����ȿɶ���3X��ʽ��, ����ĩβ��'\0'
// In       : psIn     : Դ��
//            iLength  : Դ������
// Out      : pszOut   : Ŀ�괮
void vOneTwoX0(const uchar *psIn, int iLength, uchar *pszOut)
{
    vOneTwo(psIn, iLength, pszOut);
	if(iLength < 0)
		iLength = 0;
    pszOut[2*iLength]=0;
}
