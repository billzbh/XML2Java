/**************************************
File name     : EMVCMD.C
Function      : EMV��Ƭָ��
Author        : Yu Jun
First edition : Feb 24th, 2003
Note          : immigrate from PBOCCMD.C
Modified      : July 31st, 2008
				    from EMV2000 to EMV2004
				Mar 29th, 2012
				    from EMV2004 to EMV2008
					ushort -> uint
					apduָ���9Fxxʱ����Ϊ��Ҫ��ȡ��������
					ɾ���˽ű�ָ��
						APPLICATION BLOCK (post-issuance command)
						APPLICATION UNBLOCK (post-issuance command)
						CARD BLOCK (post-issuance command)
						PIN CHANGE/UNBLOCK (post-issuance command)
					�������ÿ�����ָ���uiEmvCmdSetSlot()��ָ�����ɾ���˿����Ŵ������
                Nov 26th, 2012
                    ʹ��Vpos�����ӿں���_uiDoApdu()ʵ��emv������
                Dec 11th, 2012
                    �޸�uiEmvCmdExchangeApdu(), ����APDU�ṹ, ��ȫʹ�������ӿں���_uiDoApdu()ʵ��
**************************************/
/*
ģ����ϸ����:
    �Ժ�����ʽ�ṩEMV�淶����Ҫ��IC��ָ��
.   ����IC��ָ���ǰ��Ҫ�趨IC��������(ȱʡΪ����VPOS 0�ſ���)
	��̬ȫ�ֱ���sg_iCardSlotNo���ڱ��浱ǰ������
*/
#include <stdio.h>
#include <string.h>
#include "VposFace.h"
#include "PushApdu.h"
#include "EmvCmd.h"
#include "EmvTrace.h"

static int      sg_iCardSlotNo = 0; // ������

// emv card slot select
// in  : uiSlotId   : virtual card slot id
// ret : 0          : OK
//       1          : ��֧�ִ˿���
uint uiEmvCmdSetSlot(uint uiSlotId)
{
	vTraceWriteTxtCr(TRACE_ALWAYS, "ѡ������%u", uiSlotId);
	if(uiSlotId > 9)
		return(1); // 0-3:�û��� 4-7:Sam�� 8:�ǽӴ���
	sg_iCardSlotNo = uiSlotId;
	return(0);
}

// test if card has been inserted
// ret : 0 : card not inserted
//       1 : card inserted
uint uiEmvTestCard(void)
{
	if(_uiTestCard(sg_iCardSlotNo))
		return(1);
	return(0);
}

// emv card reset
// ret : 0 : OK
//       1 : reset error
uint uiEmvCmdResetCard(void)
{
	uint  uiRet;
	uchar sResetData[100];

	vPushApduInit(sg_iCardSlotNo); // add by yujun 2012.10.29, ֧��pboc2.0�Ż�����apdu

	uiRet = _uiResetCard(sg_iCardSlotNo, sResetData);
	if(uiRet > 0) {
		vTraceWriteBinCr(TRACE_APDU, "IC����λ�ɹ�, ATR=", sResetData, uiRet);
		return(0);
	} 
	vTraceWriteTxtCr(TRACE_APDU, "IC����λʧ��");
	return(1);
}

// emv card close
// ret : 0 : OK
uint uiEmvCloseCard(void)
{
	_uiCloseCard(sg_iCardSlotNo);
	return(0);
}

// emv card apdu
// in  : uiInLen   : apdu ָ���
//       psApduIn  : ����emv�淶case1-case4
// out : puiOutLen : apdu Ӧ�𳤶�(ȥ����β����SW[2])
//       psApduOut : apduӦ��(ȥ����β����SW[2])
// ret : 0         : OK
//       1         : communication error between card and reader
//       other     : returned by card
uint uiEmvCmdExchangeApdu(uint uiInLen, uchar *psApduIn, uint *puiOutLen, uchar *psApduOut)
{
    uint  uiRet;
    uint  uiOutLen;
    uchar sApduOut[260];

    uiOutLen = 0;
    *puiOutLen = 0;
    uiRet = _uiDoApdu(sg_iCardSlotNo, uiInLen, psApduIn, &uiOutLen, sApduOut, APDU_EMV);
    if(uiRet)
        return(uiRet);
    if(uiOutLen < 2)
        return(1);
    *puiOutLen = uiOutLen - 2;
    memcpy(psApduOut, sApduOut, *puiOutLen);
    if(memcmp(sApduOut+*puiOutLen, "\x90\x00", 2) != 0)
        uiRet = ulStrToLong(sApduOut+*puiOutLen, 2);
    return(uiRet);
}

// emv card read record file
// in  : ucSFI      : short file identifier
//       ucRecordNo : record number, starting from 1
// out : psBuf      : data buffer
//       pucLength  : length read
// ret : 0          : OK
//       1          : communication error between card and reader
//       other      : returned by card
// note: refer to EMV2004 book3 Part II, 6.5.11
uint uiEmvCmdRdRec(uchar ucSFI, uchar ucRecordNo, uchar *pucLength, uchar *psBuf)
{
    uint  uiRet;
	uchar ucSW1;
    uint  uiInLen, uiOutLen;
    uchar sApduIn[262], sApduOut[260];

    uiInLen = 0;
    sApduIn[uiInLen++] = 0x00;
    sApduIn[uiInLen++] = 0xb2;
    sApduIn[uiInLen++] = ucRecordNo;
    sApduIn[uiInLen++] = (ucSFI<<3)|0x04; // 0x04 : p1 is a record number
    sApduIn[uiInLen++] = 0;
    uiRet = uiEmvCmdExchangeApdu(uiInLen, sApduIn, &uiOutLen, sApduOut);
	vTraceWriteLastApdu("* Apdu����¼");
	ucSW1 = uiRet>>8;
	if(uiRet!=1 && (ucSW1==0x00 || ucSW1==0x62 || ucSW1==0x63)) {
	    *pucLength = uiOutLen;
		memcpy(psBuf, sApduOut, uiOutLen);
	}
	return(uiRet);
}

// emv card select file
// in  : ucP2      : parameter 2, 0x00 : first occurrence, 0x02 : next occurrence
//       ucAidLen  : length of the AID
//       psAid     : AID
// out : pucFciLen : FCI length, NULL means don't get FCI info
//       psFci     : file control infomation, NULL means don't get FCI info
// ret : 0         : OK
//       1         : communication error between card and reader
//       other     : returned by card
// note: refer to EMV2004 book1 Part III, 11.3
uint uiEmvCmdSelect(uchar ucP2, uchar ucAidLen, uchar *psAid, uchar *pucFciLen, uchar *psFci)
{
    uint  uiRet;
	uchar ucSW1;
    uint  uiInLen, uiOutLen;
    uchar sApduIn[262], sApduOut[260];

    uiInLen = 0;
    sApduIn[uiInLen++] = 0x00;
    sApduIn[uiInLen++] = 0xa4;
    sApduIn[uiInLen++] = 0x04;
    sApduIn[uiInLen++] = ucP2;
    sApduIn[uiInLen++] = ucAidLen;
    memcpy(sApduIn+uiInLen, psAid, ucAidLen);
    uiInLen += ucAidLen;
    sApduIn[uiInLen++] = 0;
    uiRet = uiEmvCmdExchangeApdu(uiInLen, sApduIn, &uiOutLen, sApduOut);
	vTraceWriteLastApdu("* Apduѡ��Ӧ��");
	ucSW1 = uiRet>>8;
	if(uiRet!=1 && (ucSW1==0x00 || ucSW1==0x62 || ucSW1==0x63)) {
		if(pucFciLen && psFci) {
		    *pucFciLen = uiOutLen;
		    memcpy(psFci, sApduOut, uiOutLen);
		}
	}
	return(uiRet);
}

// emv card external authentication
// in  : ucLength : length of data
//       psIn     : ciphered data
// ret : 0        : OK
//       1        : communication error between card and reader
//       other    : returned by card
// note: refer to EMV2004 book3 Part II, 6.5.4
uint uiEmvCmdExternalAuth(uchar ucLength, uchar *psIn)
{
    uint  uiRet;
    uint  uiInLen, uiOutLen;
    uchar sApduIn[262], sApduOut[260];

    uiInLen = 0;
    sApduIn[uiInLen++] = 0x00;
    sApduIn[uiInLen++] = 0x82;
    sApduIn[uiInLen++] = 0x00;
    sApduIn[uiInLen++] = 0x00;
    sApduIn[uiInLen++] = ucLength;
    memcpy(sApduIn+uiInLen, psIn, ucLength);
    uiInLen += ucLength;
    uiRet = uiEmvCmdExchangeApdu(uiInLen, sApduIn, &uiOutLen, sApduOut);
	vTraceWriteLastApdu("* Apdu�ⲿ��֤");
    return(uiRet);
}

// emv card generate AC
// in  : ucP1         : parameter 1
//       ucLengthIn   : data length send to card
//       psDataIn     : data send to the card
// out : pucLengthOut : data length received from card
//       psDataOut    : data received from card
// ret : 0            : OK
//       1            : communication error between card and reader
//       other        : returned by card
// note: refer to EMV2004 book3 Part II, 6.5.5
uint uiEmvCmdGenerateAC(uchar ucP1, uchar ucLengthIn, uchar *psDataIn, uchar *pucLengthOut, uchar *psDataOut)
{
    uint  uiRet;
	uchar ucSW1;
    uint  uiInLen, uiOutLen;
    uchar sApduIn[262], sApduOut[260];

    uiInLen = 0;
    sApduIn[uiInLen++] = 0x80;
    sApduIn[uiInLen++] = 0xae;
    sApduIn[uiInLen++] = ucP1;
    sApduIn[uiInLen++] = 0x00;
    sApduIn[uiInLen++] = ucLengthIn;
    memcpy(sApduIn+uiInLen, psDataIn, ucLengthIn);
    uiInLen += ucLengthIn;
    sApduIn[uiInLen++] = 0;
    uiRet = uiEmvCmdExchangeApdu(uiInLen, sApduIn, &uiOutLen, sApduOut);
	vTraceWriteLastApdu("* Apdu Generate AC");
	ucSW1 = uiRet>>8;
	if(uiRet!=1 && (ucSW1==0x00 || ucSW1==0x62 || ucSW1==0x63)) {
	    *pucLengthOut = uiOutLen;
		memcpy(psDataOut, sApduOut, uiOutLen);
	}
	return(uiRet);
}

// emv card get 8-byte random number from pboc card
// out : pucLength: ����
//       psOut    : random number from pboc card
// ret : 0        : OK
//       1        : communication error between card and reader
//       other    : returned by card
// note: refer to EMV2004 book3 Part II, 6.5.6
uint uiEmvCmdGetChallenge(uchar *pucLength, uchar *psOut)
{
    uint  uiRet;
	uchar ucSW1;
    uint  uiInLen, uiOutLen;
    uchar sApduIn[262], sApduOut[260];

    uiInLen = 0;
    sApduIn[uiInLen++] = 0x00;
    sApduIn[uiInLen++] = 0x84;
    sApduIn[uiInLen++] = 0x00;
    sApduIn[uiInLen++] = 0x00;
    sApduIn[uiInLen++] = 0;
    uiRet = uiEmvCmdExchangeApdu(uiInLen, sApduIn, &uiOutLen, sApduOut);
	vTraceWriteLastApdu("* Apduȡ�����");
	ucSW1 = uiRet>>8;
	if(uiRet!=1 && (ucSW1==0x00 || ucSW1==0x62 || ucSW1==0x63)) {
	    *pucLength = uiOutLen;
		memcpy(psOut, sApduOut, uiOutLen);
	}
	return(uiRet);
}

// emv card get data
// in  : psTag     : tag, can only be "\x9f\x36" or "\x9f\x13" or "\x9f\x17" or "\x9f\x4f"
// out : pucLength : length of data
//       psData    : data
// ret : 0         : OK
//       1         : communication error between card and reader
//       other     : returned by card
// note: refer to EMV2004 book3 Part II, 6.5.7
uint uiEmvCmdGetData(uchar *psTag, uchar *pucLength, uchar *psData)
{
    uint  uiRet;
	uchar ucSW1;
    uint  uiInLen, uiOutLen;
    uchar sApduIn[262], sApduOut[260];

    uiInLen = 0;
    sApduIn[uiInLen++] = 0x80;
    sApduIn[uiInLen++] = 0xca;
    sApduIn[uiInLen++] = psTag[0];
    sApduIn[uiInLen++] = psTag[1];
    sApduIn[uiInLen++] = 0;
    uiRet = uiEmvCmdExchangeApdu(uiInLen, sApduIn, &uiOutLen, sApduOut);
	vTraceWriteLastApdu("* Apdu Get Data");
	ucSW1 = uiRet>>8;
	if(uiRet!=1 && (ucSW1==0x00 || ucSW1==0x62 || ucSW1==0x63)) {
	    *pucLength = uiOutLen;
		memcpy(psData, sApduOut, uiOutLen);
	}
	return(uiRet);
}

// emv card get processing options
// in  : ucLengthIn   : data length send to card
//       psDataIn     : data send to the card
// out : pucLengthOut : data length received from card
//       psDataOut    : data received from card
// ret : 0            : OK
//       1            : communication error between card and reader
//       other        : returned by card
// note: refer to EMV2004 book3 Part II, 6.5.8
uint uiEmvCmdGetOptions(uchar ucLengthIn, uchar *psDataIn, uchar *pucLengthOut, uchar *psDataOut)
{
    uint  uiRet;
	uchar ucSW1;
    uint  uiInLen, uiOutLen;
    uchar sApduIn[262], sApduOut[260];

    uiInLen = 0;
    sApduIn[uiInLen++] = 0x80;
    sApduIn[uiInLen++] = 0xa8;
    sApduIn[uiInLen++] = 0x00;
    sApduIn[uiInLen++] = 0x00;
    sApduIn[uiInLen++] = ucLengthIn;
    memcpy(sApduIn+uiInLen, psDataIn, ucLengthIn);
    uiInLen += ucLengthIn;
    sApduIn[uiInLen++] = 0;
    uiRet = uiEmvCmdExchangeApdu(uiInLen, sApduIn, &uiOutLen, sApduOut);
	vTraceWriteLastApdu("* Apdu GPO");
	ucSW1 = uiRet>>8;
	if(uiRet!=1 && (ucSW1==0x00 || ucSW1==0x62 || ucSW1==0x63)) {
	    *pucLengthOut = uiOutLen;
		memcpy(psDataOut, sApduOut, uiOutLen);
	}
	return(uiRet);
}

// emv card internal authentication
// in  : ucLengthIn   : data length send to card
//       psIn         : data send to the card
// out : pucLengthOut : data length received from the card
//       psOut        : data received from the card
// ret : 0            : OK
//       1            : communication error between card and reader
//       other        : returned by card
// note: refer to EMV2004 book3 Part II, 6.5.9
uint uiEmvCmdInternalAuth(uchar ucLengthIn, uchar *psIn, uchar *pucLengthOut, uchar *psOut)
{
    uint  uiRet;
	uchar ucSW1;
    uint  uiInLen, uiOutLen;
    uchar sApduIn[262], sApduOut[260];

    uiInLen = 0;
    sApduIn[uiInLen++] = 0x00;
    sApduIn[uiInLen++] = 0x88;
    sApduIn[uiInLen++] = 0x00;
    sApduIn[uiInLen++] = 0x00;
    sApduIn[uiInLen++] = ucLengthIn;
    memcpy(sApduIn+uiInLen, psIn, ucLengthIn);
    uiInLen += ucLengthIn;
    sApduIn[uiInLen++] = 0;
    uiRet = uiEmvCmdExchangeApdu(uiInLen, sApduIn, &uiOutLen, sApduOut);
	vTraceWriteLastApdu("* Apdu�ڲ���֤");
	ucSW1 = uiRet>>8;
	if(uiRet!=1 && (ucSW1==0x00 || ucSW1==0x62 || ucSW1==0x63)) {
	    *pucLengthOut = uiOutLen;
		memcpy(psOut, sApduOut, uiOutLen);
	}
	return(uiRet);
}

// emv card verify user pin
// in  : ucP2      : parameter 2
//       ucLength  : length of pin relevant data
//       psPinData : pin data
// ret : 0         : OK
//       1         : communication error between card and reader
//       other     : returned by card
// note: refer to EMV2004 book3 Part II, 6.5.12
uint uiEmvCmdVerifyPin(uchar ucP2, uchar ucLength, uchar *psPinData)
{
    uint  uiRet;
    uint  uiInLen, uiOutLen;
    uchar sApduIn[262], sApduOut[260];

    uiInLen = 0;
    sApduIn[uiInLen++] = 0x00;
    sApduIn[uiInLen++] = 0x20;
    sApduIn[uiInLen++] = 0x00;
    sApduIn[uiInLen++] = ucP2;
    sApduIn[uiInLen++] = ucLength;
    memcpy(sApduIn+uiInLen, psPinData, ucLength);
    uiInLen += ucLength;
    uiRet = uiEmvCmdExchangeApdu(uiInLen, sApduIn, &uiOutLen, sApduOut);
	vTraceWriteLastApdu("* Apdu��֤����");
    return(uiRet);
}
