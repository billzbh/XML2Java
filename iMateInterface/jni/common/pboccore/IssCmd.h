/**************************************
File name     : ISSCMD.H
Function      : Gemalto pboc2.0 Dual Pboc ������ָ��
Author        : Yu Jun
First edition : Jun 29th, 2009
Note          : immigrate from EMVCMD.C
Modified      : Aug 13th, 2013
					����uiIssGetStatus()����
**************************************/
# ifndef _ISSCMD_H
# define _ISSCMD_H

// ����APDU��־
// in  : iLogFlag  : 0:������־ 1:��¼��־
//       iEmptyLog : �����־��־, 0:����� 1:���
void vIssSetLog(int iLogFlag, int iEmptyLog);

// emv card slot select
// in  : uiSlotId   : virtual card slot id
// ret : 0          : OK
//       1          : ��֧�ִ˿���
uint uiIssCmdSetSlot(uint uiSlotId);

// test if card has been inserted
// ret : 0 : card not inserted
//       1 : card inserted
uint uiIssTestCard(void);

// emv card reset
// ret : 0 : OK
//       1 : reset error
//uint uiIssCmdResetCard(void);

//QINGBO
// emv card reset
// ret : >0 : OK
//       0  : reset error
uint uiIssCmdResetCard(uchar *psAtr);


// emv card close
// ret : 0 : OK
uint uiIssCloseCard(void);

// emv card apdu
// in  : uiInLen   : apdu ָ���
//       psApduIn  : ����emv�淶case1-case4
// out : puiOutLen : apdu Ӧ�𳤶�(ȥ����β����SW[2])
//       psApduOut : apduӦ��(ȥ����β����SW[2])
// ret : 0         : OK
//       1         : communication error between card and reader
//       other     : returned by card
uint uiIssCmdExchangeApdu(uint uiInLen, uchar *psApduIn, uint *puiOutLen, uchar *psApduOut);

// Install
// in  : ucClass  : C-APDU class, 80 or 84
//       ucP1     : P1
//       ucLength : length of install data
//       psIn     : install data
// ret : 0        : OK
//       1        : communication error between card and reader
//       other    : returned by card
// note: refer to Perso*.pdf 3.1
uint uiIssCmdInstall(uchar ucClass, uchar ucP1, uchar ucLength, uchar *psIn);

// Initialize
// in  : ucKeyVer   : Key Set Version
//       ucKeyIndex : Key Index
//       psHostRand : Host Challenge [8]
// out : psOut      : Out Data [28]
// ret : 0          : OK
//       1          : communication error between card and reader
//       0x8001     : length out error
//       other      : returned by card
// note: refer to proso*.pdf 6.2.1.1
uint uiIssCmdInitialize(uchar ucKeyVer, uchar ucKeyIndex, uchar *psHostRand, uchar *psOut);

// external authentication
// in  : ucClass  : 0x80 or 0x84
//       psIn     : ciphered data
// ret : 0        : OK
//       1        : communication error between card and reader
//       other    : returned by card
// note: refer to perso*.pdf 6.2.1.2
//       ��ֻ֧��secure message��ʽ
uint uiIssCmdExternalAuth(uchar ucClass, uchar *psIn);

// put data
// in  : ucP1         : parameter 1
//       ucP2         : parameter 2
//       ucLengthIn   : data length send to card
//       psDataIn     : data send to the card
// ret : 0            : OK
//       1            : communication error between card and reader
//       other        : returned by card
// note: refer to perso*.pdf 6.2.1.3
uint uiIssCmdPutData(uchar ucP1, uchar ucP2, uchar ucLengthIn, uchar *psDataIn);

// append record
// in  : ucP2         : parameter 2
//       ucLengthIn   : data length send to card
//       psDataIn     : data send to the card
// ret : 0            : OK
//       1            : communication error between card and reader
//       other        : returned by card
// note: refer to perso*.pdf 6.2.1.4
uint uiIssCmdAppendRecord(uchar ucP2, uchar ucLengthIn, uchar *psDataIn);

// update record
// in  : ucP1         : parameter 1
//       ucP2         : parameter 2
//       ucLengthIn   : data length send to card
//       psDataIn     : data send to the card
// ret : 0            : OK
//       1            : communication error between card and reader
//       other        : returned by card
uint uiIssCmdUpdateRecord(uchar ucP1, uchar ucP2, uchar ucLengthIn, uchar *psDataIn);

// put key
// in  : ucP1         : P1
//       ucP2         : P2
//       ucLengthIn   : data length send to card
//       psDataIn     : data send to the card
// ret : 0            : OK
//       1            : communication error between card and reader
//       other        : returned by card
// note: refer to perso*.pdf 6.2.1.5
uint uiIssCmdPutKey(uchar ucP1, uchar ucP2, uchar ucLengthIn, uchar *psDataIn);

// pin change
// in  : ucP2         : P2 for pin try limit
//       psDataIn     : enciphered pin data [8]
// ret : 0            : OK
//       1            : communication error between card and reader
//       other        : returned by card
// note: refer to perso*.pdf 6.2.1.6
uint uiIssCmdPinChange(uchar ucP2, uchar *psDataIn);

// set status
// in  : ucP1         : P1, 0x40 : Security Domain Life Cycle State
//       ucP2         : P2, 0x0F : Status value for the Application: Personalized
// ret : 0            : OK
//       1            : communication error between card and reader
//       other        : returned by card
// note: refer to perso*.pdf 6.2.1.7
uint uiIssCmdSetStatus(uchar ucP1, uchar ucP2);

// clear file
// in  : ucAidLen : length of aid
//       psAid    : aid
// ret : 0        : OK
//       1            : communication error between card and reader
//       other        : returned by card
// note: refer to java card SRC from XiaFei (PageDelete.cpp)
uint uiIssCmdClearFile(uchar ucAidLen, uchar *psAid);

// store data
// in  : ucP1         : P1
//       ucP2         : P2
//       ucLengthIn   : data length send to card
//       psDataIn     : data send to the card
// ret : 0            : OK
//       1            : communication error between card and reader
//       other        : returned by card
// note: refer to
uint uiIssCmdStoreData(uchar ucP1, uchar ucP2, uchar ucLengthIn, uchar *psDataIn);

// get status
// in  : ucP1         : P1
//       ucP2         : P2
//       ucLengthIn   : data length send to card
//       psDataIn     : data send to the card
// ret : 0            : OK
//       1            : communication error between card and reader
//       other        : returned by card
// note: refer to
uint uiIssGetStatus(uchar ucP1, uchar ucP2, uchar ucLengthIn, uchar *psDataIn, uchar *pucLengthOut, uchar *psDataOut);

// ����ΪEMV�淶ָ��

// emv card read record file
// in  : ucSFI      : short file identifier
//       ucRecordNo : record number, starting from 1
// out : psBuf      : data buffer
//       pucLength  : length read
// ret : 0          : OK
//       1          : communication error between card and reader
//       other      : returned by card
// note: refer to EMV2004 book3 Part II, 6.5.11
uint uiIssCmdRdRec(uchar ucSFI, uchar ucRecordNo, uchar *pucLength, uchar *psBuf);

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
uint uiIssCmdSelect(uchar ucP2, uchar ucAidLen, uchar *psAid, uchar *pucFciLen, uchar *psFci);

// emv card external authentication
// in  : ucLength : length of data
//       psIn     : ciphered data
// ret : 0        : OK
//       1        : communication error between card and reader
//       other    : returned by card
// note: refer to EMV2004 book3 Part II, 6.5.4
uint uiIssCmdExternalAuth_Emv(uchar ucLength, uchar *psIn);

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
uint uiIssCmdGenerateAC(uchar ucP1, uchar ucLengthIn, uchar *psDataIn, uchar *pucLengthOut, uchar *psDataOut);

// emv card get 8-byte random number from pboc card
// out : pucLength: ����
//       psOut    : random number from pboc card
// ret : 0        : OK
//       1        : communication error between card and reader
//       other    : returned by card
// note: refer to EMV2004 book3 Part II, 6.5.6
uint uiIssCmdGetChallenge(uchar *pucLength, uchar *psOut);

// emv card get data
// in  : psTag     : tag, can only be "\x9f\x36" or "\x9f\x13" or "\x9f\x17" or "\x9f\x4f"
// out : pucLength : length of data
//       psData    : data
// ret : 0         : OK
//       1         : communication error between card and reader
//       other     : returned by card
// note: refer to EMV2004 book3 Part II, 6.5.7
uint uiIssCmdGetData(uchar *psTag, uchar *pucLength, uchar *psData);

// emv card get processing options
// in  : ucLengthIn   : data length send to card
//       psDataIn     : data send to the card
// out : pucLengthOut : data length received from card
//       psDataOut    : data received from card
// ret : 0            : OK
//       1            : communication error between card and reader
//       other        : returned by card
// note: refer to EMV2004 book3 Part II, 6.5.8
uint uiIssCmdGetOptions(uchar ucLengthIn, uchar *psDataIn, uchar *pucLengthOut, uchar *psDataOut);

// emv card internal authentication
// in  : ucLengthIn   : data length send to card
//       psIn         : data send to the card
// out : pucLengthOut : data length received from the card
//       psOut        : data received from the card
// ret : 0            : OK
//       1            : communication error between card and reader
//       other        : returned by card
// note: refer to EMV2004 book3 Part II, 6.5.9
uint uiIssCmdInternalAuth(uchar ucLengthIn, uchar *psIn, uchar *pucLengthOut, uchar *psOut);

// emv card verify user pin
// in  : ucP2      : parameter 2
//       ucLength  : length of pin relevant data
//       psPinData : pin data
// ret : 0         : OK
//       1         : communication error between card and reader
//       other     : returned by card
// note: refer to EMV2004 book3 Part II, 6.5.12
uint uiIssCmdVerifyPin(uchar ucP2, uchar ucLength, uchar *psPinData);

#endif
