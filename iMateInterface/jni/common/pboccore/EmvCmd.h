/**************************************
File name     : EMVCMD.H
Function      : Implement EMV2004 standard card low level functions
Author        : Yu Jun
First edition : Feb 24th, 2003
Note          : immigrate from PBOCCMD.H
Modified      : July 31st, 2008
				    from EMV2000 to EMV2004
				Mar 29th, 2012
				    from EMV2004 to EMV2008
					ushort -> uint
					apdu指令返回9Fxx时不认为是要求取可用数据
					删除了脚本指令
						APPLICATION BLOCK (post-issuance command)
						APPLICATION UNBLOCK (post-issuance command)
						CARD BLOCK (post-issuance command)
						PIN CHANGE/UNBLOCK (post-issuance command)
**************************************/
# ifndef _EMVCMD_H
# define _EMVCMD_H

#ifdef __cplusplus
extern "C" {
#endif

// emv card slot select
// in  : uiSlotId   : virtual card slot id
// ret : 0          : OK
//       1          : 不支持此卡座
uint uiEmvCmdSetSlot(uint uiSlotId);

// test if card has been inserted
// ret : 0 : card not inserted
//       1 : card inserted
uint uiEmvTestCard(void);

// emv card reset
// ret : 0 : OK
//       1 : reset error
uint uiEmvCmdResetCard(void);

// emv card close
// ret : 0 : OK
uint uiEmvCloseCard(void);

// emv card apdu
// in  : uiInLen   : apdu 指令长度
//       psApduIn  : 遵照emv规范case1-case4
// out : puiOutLen : apdu 应答长度
//       psApduOut : 遵照emv规范case1-case4
// ret : 0         : OK
//       1         : communication error between card and reader
//       other     : returned by card
uint uiEmvCmdExchangeApdu(uint uiInLen, uchar *psApduIn, uint *puiOutLen, uchar *psApduOut);

// emv card read record file
// in  : ucSFI      : short file identifier
//       ucRecordNo : record number, starting from 1
// out : psBuf      : data buffer
//       pucLength  : length read
// ret : 0          : OK
//       1          : communication error between card and reader
//       other      : returned by card
// note: see EMV2004 book3 Part II, 6.5.11
uint uiEmvCmdRdRec(uchar ucSFI, uchar ucRecordNo, uchar *pucLength, uchar *psBuf);

// emv card select file
// in  : ucP2      : parameter 2, 0x00 : first occurrence, 0x02 : next occurrence
//       ucAidLen  : length of the AID
//       psAid     : AID
// out : pucFciLen : FCI length, NULL means don't get FCI info
//       psFci     : file control infomation, NULL means don't get FCI info
// ret : 0         : OK
//       1         : communication error between card and reader
//       other     : returned by card
// note: see EMV2004 book1 Part III, 11.3
uint uiEmvCmdSelect(uchar ucP2, uchar ucAidLen, uchar *psAid, uchar *pucFciLen, uchar *psFci);

// emv card external authentication
// in  : ucLength : length of data
//       psIn     : ciphered data
// ret : 0        : OK
//       1        : communication error between card and reader
//       other    : returned by card
// note: see EMV2004 book3 Part II, 6.5.4
uint uiEmvCmdExternalAuth(uchar ucLength, uchar *psIn);

// emv card generate AC
// in  : ucP1         : parameter 1
//       ucLengthIn   : data length send to card
//       psDataIn     : data send to the card
// out : pucLengthOut : data length received from card
//       psDataOut    : data received from card
// ret : 0            : OK
//       1            : communication error between card and reader
//       other        : returned by card
// note: see EMV2004 book3 Part II, 6.5.5
uint uiEmvCmdGenerateAC(uchar ucP1, uchar ucLengthIn, uchar *psDataIn, uchar *pucLengthOut, uchar *psDataOut);

// emv card get 8-byte random number from pboc card
// out : pucLength: 长度
//       psOut    : random number from pboc card
// ret : 0        : OK
//       1        : communication error between card and reader
//       other    : returned by card
// note: refer to EMV2004 book3 Part II, 6.5.6
uint uiEmvCmdGetChallenge(uchar *pucLength, uchar *psOut);

// emv card get data
// in  : psTag     : tag, can only be "\x9f\x36" or "\x9f\x13" or "\x9f\x17" or "\x9f\x4f"
// out : pucLength : length of data
//       psData    : data
// ret : 0         : OK
//       1         : communication error between card and reader
//       other     : returned by card
// note: see EMV2004 book3 Part II, 6.5.7
uint uiEmvCmdGetData(uchar *psTag, uchar *pucLength, uchar *psData);

// emv card get processing options
// in  : ucLengthIn   : data length send to card
//       psDataIn     : data send to the card
// out : pucLengthOut : data length received from card
//       psDataOut    : data received from card
// ret : 0            : OK
//       1            : communication error between card and reader
//       other        : returned by card
// note: see EMV2004 book3 Part II, 6.5.8
uint uiEmvCmdGetOptions(uchar ucLengthIn, uchar *psDataIn, uchar *pucLengthOut, uchar *psDataOut);

// emv card internal authentication
// in  : ucLengthIn   : data length send to card
//       psIn         : data send to the card
// out : pucLengthOut : data length received from the card
//       psOut        : data received from the card
// ret : 0            : OK
//       1            : communication error between card and reader
//       other        : returned by card
// note: see EMV2004 book3 Part II, 6.5.9
uint uiEmvCmdInternalAuth(uchar ucLengthIn, uchar *psIn, uchar *pucLengthOut, uchar *psOut);

// emv card verify user pin
// in  : ucP2      : parameter 2
//       ucLength  : length of pin relevant data
//       psPinData : pin data
// ret : 0         : OK
//       1         : communication error between card and reader
//       other     : returned by card
// note: see EMV2004 book3 Part II, 6.5.12
uint uiEmvCmdVerifyPin(uchar ucP2, uchar ucLength, uchar *psPinData);

#ifdef __cplusplus
}
#endif

# endif