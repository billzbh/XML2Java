#ifndef REMOTEFUNCTIONS_H
#define REMOTEFUNCTIONS_H

#include "unsigned.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define	CARD_TYPE_NORMAL		0
#define	CARD_TYPE_EMV			1
#define	CARD_TYPE_SSC			2


extern uint _uiRcTestCard(uint uiReader);
extern uint _uiRcResetCard(uint uiReader, uchar *pusResetData);
extern uint _uiRcCloseCard(uint uiReader);
extern uint _uiRcExchangeApduEx(uint uiReader, uchar ucCardType, uchar *psApduIn, uint uiInLen, uchar *psApduOut, uint *puiOutLen);
extern uint _uiRcMagReset(void);
extern uint _uiRcMagTest(void);
extern uint _uiRcMagGet(uint uiTrackNo, uchar *pszBuffer);
extern void _vRcBuzzer(void);
    
extern uchar _ucRcAsyncSetPort(uchar ucPortNo);
extern uchar _ucRcAsyncOpen(ulong ulBaud, uchar ucParity, uchar ucBits, uchar ucStop);
extern uchar _ucRcAsyncClose(void);
extern uchar _ucRcAsyncReset(void);
extern uchar _ucRcAsyncSend(uchar ucChar);
extern uchar _ucRcAsyncTest(void);
extern uchar _ucRcAsyncGet(void);
extern uchar _ucRcAsyncSendBuf(void *pBuf, uint uiLen);
extern uchar _ucRcAsyncGetBuf(void *pBuf, uint uiLen, uint uiTimeOut);
extern uchar _ucRcXMemRead(void *pBuf, uint uiOffset, uint uiLen);
extern uchar _ucRcXMemWrite(void *pBuf, uint uiOffset, uint uiLen);
extern uchar _ucRcXMemReadReserved(void *pBuf, uint uiOffset, uint uiLen);
extern uchar _ucRcXMemWriteReserved(void *pBuf, uint uiOffset, uint uiLen);

extern void _vRcSetLed(uint uiLedNo, uchar ucOnOff);
extern void _vRcSetUart(uint uiPortNo);
extern void _vRcSetCardVoltage(uint uiVoltage);

extern uchar _ucRcAsyncSetPort(uchar ucPortNo);
extern uchar _ucRcAsyncOpen(ulong ulBaud, uchar ucParity, uchar ucBits, uchar ucStop);
extern uchar _ucRcAsyncClose(void);
extern uchar _ucRcAsyncReset(void);
extern uchar _ucRcAsyncSend(uchar ucChar);
extern uchar _ucRcAsyncTest(void);
extern uchar _ucRcAsyncGet(void);
extern uchar _ucRcAsyncSendBuf(void *pBuf, uint uiLen);
extern uchar _ucRcAsyncGetBuf(void *pBuf, uint uiLen, uint uiTimeOut);

extern uint _uiRcMifCard(uchar *psSerialNo);
extern uint _uiRcMifActive(void);
extern uint _uiRcMifClose(void);
extern uint _uiRcMifRemoved(void);
extern uint _uiRcMifAuth(uchar ucSecNo, uchar ucKeyAB, uchar *psKey);
extern uint _uiRcMifReadBlock(uchar ucSecNo, uchar ucBlock, uchar *psData);
extern uint _uiRcMifWriteBlock(uchar ucSecNo, uchar ucBlock, uchar *psData);
extern uint _uiRcMifIncrement(uchar ucSecNo,uchar ucBlock,ulong ulValue);
extern uint _uiRcMifDecrement(uchar ucSecNo,uchar ucBlock,ulong ulValue);
extern uint _uiRcMifCopy(uchar ucSrcSecNo, uchar ucSrcBlock, uchar ucDesSecNo, uchar ucDesBlock);
extern uint _uiRcMifApdu(uchar *psApduIn, uint uiInLen, uchar *psApduOut, uint *puiOutLen);

extern uint _uiRcSetMemoryCardCode(uchar ucCardType, uchar ucOffset, uchar ucLen, uchar *sCardCode);
extern uint _uiRcTestMemoryCardType(uchar *psResetData);
extern uint _uiRcTestMemorySpecifiedType(uchar ucCardType,uchar *sResetData);

extern void _vRcSLE4442_Open(void);
extern void _vRcSLE4442_Close(void);

extern uchar _ucRcSLE4442_ChkCode(uchar *sSecurityCode);
extern void _vRcSLE4442_Read(uchar Addr, int DataLen, uchar* DataBuff);
extern void _vRcSLE4442_Write(uchar Addr,int DataLen,uchar* DataBuff);
extern void _vRcSLE4442_PrRead(uchar Addr, int DataLen, uchar* DataBuff);
extern void _vRcSLE4442_PrWrite(uchar Addr,int DataLen,uchar* DataBuff);

extern void _vRcSLE4428_Open(void);
extern void _vRcSLE4428_Close(void);

extern uchar _ucRcSLE4428_ChkPSC(uchar *sSecurityCode);
extern uchar _ucRcSLE4428_UpdPSC(uchar *sSecurityCode);

extern void _vRcSLE4428_Read(uchar Addr, int DataLen, uchar* DataBuff);
extern void _vRcSLE4428_Write(uchar Addr,int DataLen,uchar* DataBuff);
extern void _vRcSLE4428_PrRead(uchar Addr, int DataLen, uchar* DataBuff);
extern void _vRcSLE4428_PrWrite(uchar Addr,int DataLen,uchar* DataBuff);

extern void _vRcAT102_OpenCard(void);
extern void _vRcAT102_CloseCard(void);
extern uchar _ucRcAT102_ChkSecurityCode(uchar *psSC);
extern void _vRcAT102_ReadWords(uchar ucWordAddr, uchar ucWordNum, uchar *psDataBuf );
extern uchar _ucRcAT102_EraseNonApp(uchar ucWordAddr, uchar ucWordNum);
extern uchar _ucRcAT102_EraseApp(uchar ucArea, uchar ucLimited, uchar *psEraseKey);
extern uchar _ucRcAT102_WriteWords(uchar ucWordAddr, uchar ucWordNum, uchar *psDataBuf);
extern void _vRcAT102_ReadAZ(uchar ucArea, uchar *psAZ);
extern uchar _ucRcAT102_WriteAZ(uchar ucArea, uchar *psAZ);
extern void _vRcAT102_ReadMTZ(uchar *psMTZ);
extern uchar _ucRcAT102_UpdateMTZ(uchar *psMTZ);

extern void _vRcAT1604_OpenCard(void);
extern void _vRcAT1604_CloseCard(void);
extern uchar _ucRcAT1604_ChkSC(uchar *SC);
extern uchar _ucRcAT1604_ChkSCn(uchar Area,uchar *SCn);
extern void _vRcAT1604_Read(uint Addr,uint Len,uchar *DataBuf);
extern void _vRcAT1604_ReadAZ(uchar Area,uint Addr,uint Len,uchar *DataBuf);
extern uchar _ucRcAT1604_Erase(uint Addr,uint Len);
extern uchar _ucRcAT1604_EraseAZ(uchar Area,uint Addr,uint Len);
extern uchar _ucRcAT1604_Write(uint Addr,uint Len,uchar *DataBuf);
extern uchar _ucRcAT1604_WriteAZ(uchar Area,uint Addr,uint Len,uchar *DataBuf);
extern uchar _ucRcAT1604_ChkEZn(uchar Area,uchar *EZn);
extern void _vRcAT1604_ReadMTZ(uchar *MTZ);
extern uchar _ucRcAT1604_UpdMTZ(uchar *MTZ);

extern void _vRcAT1608_OpenCard(uchar *RstData);
extern void _vRcAT1608_CloseCard(void);
extern uchar _ucRcAT1608_VerifyPassword(uchar Index,uchar *Password);
extern uchar _ucRcAT1608_Read(uchar Level,uchar Addr,uint Len,uchar *DataBuf);
extern uchar _ucRcAT1608_Write(uchar Level,uchar Addr,uint Len,uchar *DataBuf);
extern uchar _ucRcAT1608_Auth(uchar *Gc);
extern uchar _ucRcAT1608_SetAZ(uchar AZ);
extern uchar _ucRcAT1608_ReadFuse(uchar *Fuse);
extern uchar _ucRcAT1608_WriteFuse(void);

extern void _vRcAT24Cxx_OpenCard(void);
extern void _vRcAT24Cxx_CloseCard(void);
extern uchar _ucRcAT24Cxx_Read(uchar* DataBuff, uchar Addr, int DataLen);
extern uchar _ucRcAT24Cxx_Write(uchar* DataBuff, uchar Addr,int DataLen);
extern uchar _ucRcAT24C32_Read(uchar* DataBuff, uchar Addr, int DataLen);
extern uchar _ucRcAT24C32_Write(uchar* DataBuff, uchar Addr,int DataLen);
    
//Fingerprint device
extern uint _uiRcFingerprintOpen(void);
extern void _vRcFingerprintClose(void);
extern uint _uiRcFingerprintLink(void);
extern void _vRcFingerprintSend(uchar *psIn, uint uiInLen);
extern uint _uiRcFingerprintRecv(uchar *psOut, uint *puiOutLen, ulong ulTimeOutMs);

extern int iRcTsFingerprintOpen(void);
extern void vRcTsFingerprintClose(void);
extern void vRcTSFID_RUDLL_SetOverTime(int timeout);
extern int iRcTSFID_RUDLL_GetFinger(char fpflag[3],unsigned char fpminu1[200],unsigned char fpminu2[100]);
extern int iRcTSFID_RUDLL_EnrollFinger(char fpflag[3],int order,unsigned char fpminu1[200],unsigned char fpminu2[100]);
extern int iRcTSFID_RUDLL_SetDeviceNo(char fpflag[3],unsigned char deviceno[12]);
extern int iRcTSFID_RUDLL_GetDeviceNo(char fpflag[3],unsigned char deviceno[12]);
extern void vRcTSFID_RUDLL_GetErrorMSG(int errorno,char msgptr[80]);
extern int iRcTSFID_RUDLL_SetDeviceType(char fpflag[3],unsigned char devicetype);
extern int iRcTSFID_RUDLL_GetDeviceType(char fpflag[3],unsigned char *devicetype);
extern int iRcTSFID_RUDLL_GetDeviceInfo(char fpflag[3], char firmver[10], char deviceinfo[10]);
    
// TF ICC Functions
extern uint _uiRcSD_Init(void);
extern void _vRcSD_DeInit(void);
extern uint _uiRcSDSCConnectDev(void);
extern uint _uiRcSDSCDisconnectDev(void);
extern uint _uiRcSDSCGetFirmwareVer(uchar *psFirmwareVer, uint *puiFirmwareVerLen);
extern uint _uiRcSDSCResetCard(uchar *psAtr, uint *puiAtrLen);
extern uint _uiRcSDSCResetController(uint uiSCPowerMode);
extern uint _uiRcSDSCTransmit(uchar *psCommand, uint uiCommandLen, uint uiTimeOutMode, uchar *psOutData, uint *puiOutDataLen, uint *puiCosState);
extern uint _uiRcSDSCTransmitEx(uchar *psCommand, uint uiCommandLen, uint uiTimeOutMode, uchar *psOutData, uint *puiOutDataLen);
extern uint _uiRcSDSCGetSDKVersion(char *pszVersion, uint *puiVersionLen);
extern uint _uiRcSDSCGetSCIOType(uint *puiSCIOType);

// iGas产生通讯密钥
extern uint uiGenCommKey(uchar ucMasterKeyId, uchar *psRandom);

#ifdef __cplusplus
}
#endif

#endif
