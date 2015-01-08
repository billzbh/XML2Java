#ifndef REMOTECALL_H
#define REMOTECALL_H

#include "WriteLog.h"
#include "unsigned.h"


//vposface
#define FUNC__uiTestCard 					1001 	//(uint uiReader);
#define FUNC__uiResetCard					1002 	//(uint uiReader, uchar *psResetData);
#define FUNC__uiCloseCard					1003 	//(uint uiReader);
#define FUNC__uiExchangeApduEx				1004  	//(uint uiReader, uchar ucIccType, uchar *pIn, uint uiInLen, uchar *pOut, uint *puiOutLen);
#define FUNC__uiPrint						1005	//(uchar const *pszText);
#define FUNC__ulSetXMemSize					1006	//(ulong ulSize);
#define FUNC__uiXMemRead					1007 	//(void *pBuffer, ulong ulOffset, uint uiLen);
#define FUNC__uiXMemWrite					1008 	//(const void *pBuffer, ulong ulOffset, uint uiLen);
#define FUNC__uiMagReset					1009	//(void);
#define FUNC__uiMagTest						1010	//(void);
#define FUNC__uiMagGet						1011	//(uint uiTrackNo, uchar *pszBuffer);
#define FUNC__vBuzzer						1012	//(void)
#define FUNC__ucAsyncSetPort				1015	//(uchar ucPortNo)
#define FUNC__ucAsyncOpen					1016	//(ulong ulBaud, uchar ucParity, uchar ucBits, uchar ucStop)
#define FUNC__ucAsyncClose					1017	//(void)
#define FUNC__ucAsyncReset					1018	//(void)
#define FUNC__ucAsyncSend					1019	//(uchar ucChar)
#define FUNC__ucAsyncTest					1020	//(void)
#define FUNC__ucAsyncGet					1021	//(void)
#define FUNC__ucAsyncSendBuf				1022	//(void *pBuf, uint uiLen)
#define FUNC__ucAsyncGetBuf					1023	//(void *pBuf, uint uiLen, uint uiTimeOut)
#define FUNC__uiXMemReadReserved			1024 	//(void *pBuffer, ulong ulOffset, uint uiLen);
#define FUNC__uiXMemWriteReserved			1025 	//(const void *pBuffer, ulong ulOffset, uint uiLen);

// vposex
#define FUNC_vSetLed						1050	//void(uchar ledid, uchar onoff)
#define FUNC_vSetUart						1051	//void(uchar ucUartId, uchar onoff)
#define FUNC_vSetCardVoltage				1052	//void(CARD_VOLTAGE_5V0);

//mif card
#define FUNC_uiMifCard						2001	//(uchar *psSerialNo)
#define FUNC_uiMifActive					2002	//(void)
#define FUNC_uiMifClose						2003	//(void)
#define FUNC_uiMifRemoved					2004	//(void)
#define FUNC_uiMifAuth						2005	//(uchar ucSecNo, uchar ucKeyAB, uchar *psKey)
#define FUNC_uiMifReadBlock					2006	//(uchar ucSecNo, uchar ucBlock, uchar *psData)
#define FUNC_uiMifWriteBlock				2007	//(uchar ucSecNo, uchar ucBlock, uchar *psData)
#define FUNC_uiMifIncrement					2008	//(uchar ucSecNo,uchar ucBlock,ulong ulValue)
#define FUNC_uiMifDecrement					2009	//(uchar ucSecNo,uchar ucBlock,ulong ulValue)
#define FUNC_uiMifCopy						2010	//(uchar ucSrcSecNo, uchar ucSrcBlock, uchar ucDesSecNo, uchar ucDesBlock)
#define FUNC_uiMifApdu						2011	//(uchar *psApduIn, uint uiInLen, uchar *psApduOut, uint *puiOutLen)
#define FUNC_uiMifAuthEx					2012	//(uchar ucSecNo, uchar ucKeyAB, uchar *psKey)

#define FUNC__PCD_Reset						2101	//(void);
#define FUNC__PCD_FieldOn					2102	//(void);
#define FUNC__PCD_FieldOff					2103	//(void);
#define FUNC__PCD_FieldReset				2104	//(void);
#define FUNC__PCD_LoadKey					2105	//u8(u8 *Key);
#define FUNC__PCD_LoadKeyE2					2106	//u8(u8 KeyNo);
#define FUNC__PCD_StoreKeyE2				2107	//u8(u8 KeyNo, u8 *Key);
#define FUNC__PCD_ReadE2					2108	//u8(u16 Address, u16 Length, u8 *ReadE2Buff);
#define FUNC__PCD_WriteE2					2109	//u8(u16 Address, u16 Length, u8 *WriteE2Buff);
#define FUNC__PCD_WriteE2Page				2110	//u8(u8 Page, u8 *WriteE2PageBuff);
#define FUNC__PICC_Polling					2111	//u8(void);
#define FUNC__PICC_AntiCollision			2112	//u8(u8 *UID_Length, u8 *UID);
#define FUNC__PICC_Activation				2113	//u8(void);
#define FUNC__PICC_Removal					2114	//u8(void);
#define FUNC__PICC_Apdu						2115	//u8(u8 *C_Apdu, u16 C_ApduLen, u8 *R_Apdu, u16 *R_ApduLen);
#define FUNC__PICC_Deselect					2116	//u8(void);
#define FUNC__PICC_Mifare_Authentication	2117	//u8(u8 KeyType, u8 block, u8 *csn);
#define FUNC__PICC_Mifare_Read				2118	//u8(u8 block, u8 *recbuff);
#define FUNC__PICC_Mifare_Write				2119	//u8(u8 block, u8 *sendbuff);
#define FUNC__PICC_Mifare_Decrement			2120	//u8(u8 block, s32 value);
#define FUNC__PICC_Mifare_Increment			2121	//u8(u8 block, s32 value);
#define FUNC__PICC_Mifare_Restore			2122	//u8(u8 source_block, u8 object_block);
#define FUNC__PICC_Mifare_Transfer			2123	//u8(u8 block);
#define FUNC__PICC_Mifare_Value_Block_Init	2124	//u8(u8 block, s32 value);

//id card
#define FUNC__Identity_Open					3101	//void(void);
#define FUNC__Identity_Close				3102	//void(void);
#define FUNC__Identity_Polling				3103	//u8(void);
#define FUNC__Identity_Removal				3104	//u8(void);
#define FUNC__Identity_Card_Info			3105	//u8(u8 *Info, u16 *Length);	// 0x01 0x00 0x04 0x00 256Bytes 1024Bytes

//Fingerprint device
#define FUNC__uiFingerprintOpen				4001	//(void)
#define FUNC__vFingerprintClose				4002	//(void)
#define FUNC__uiFingerprintLink				4003	//(void)
#define FUNC__vFingerprintSend				4004	//(uchar *psIn, uint uiInLen)
#define FUNC__uiFingerprintRecv				4005	//(uchar *psOut, uint *puiOutLen, ulong ulTimeOutMs)

//Fingerprint device
#define FUNC_iTsFingerprintOpen				4101	//(void)
#define FUNC_vTsFingerprintClose			4102	//(void)
#define FUNC_TSFID_RUDLL_SetOverTime		4103	//(int _timeout)
#define FUNC_TSFID_RUDLL_GetFinger			4104	//(char fpflag[3],unsigned char fpminu1[200],unsigned char fpminu2[100]);
#define FUNC_TSFID_RUDLL_EnrollFinger		4105	//(char fpflag[3],int order,unsigned char fpminu1[200],unsigned char fpminu2[100])
#define FUNC_TSFID_RUDLL_SetDeviceNo		4106	//(char fpflag[3],unsigned char deviceno[12])
#define FUNC_TSFID_RUDLL_GetDeviceNo		4107	//(char fpflag[3],unsigned char deviceno[12])
#define FUNC_TSFID_RUDLL_GetErrorMSG		4108	//(int errorno,char msgptr[MAXLENGTH_ERRORMSG])
#define FUNC_TSFID_RUDLL_SetDeviceType		4109	//(char fpflag[3],unsigned char devicetype)
#define FUNC_TSFID_RUDLL_GetDeviceType		4110	//(char fpflag[3],unsigned char *devicetype)
#define FUNC_TSFID_RUDLL_GetDeviceInfo		4111	//(char fpflag[3], char firmver[10], char deviceinfo[10])

// SD Card Functions
#define FUNC_SD_Init  						5001	//SD_Error(void)
#define FUNC_SD_DeInit  					5002	//void(void)
#define FUNC_SD_WaitTransfer  				5003	//SDTransferState(void)
#define FUNC_SD_ReadBlock  					5004	//SD_Error(uint8_t *readbuff, uint32_t BlockAddr, uint16_t BlockSize)
#define FUNC_SD_WriteBlock  				5005	//SD_Error(uint8_t *writebuff, uint32_t BlockAddr, uint16_t BlockSize)
#define FUNC_SD_StopTransfer	    		5006	//SD_Error(void)
#define FUNC_SD_WaitReadOperation	  		5007	//SD_Error(void)
#define FUNC_SD_WaitWriteOperation	   		5008	//SD_Error(void)
#define FUNC_SD_GetCardInfo		     		5009	//SD_Error(SD_CardInfo *cardinfo)
#define FUNC_SD_GetCardStatus	    		5010	//SD_Error(SD_CardStatus *cardstatus)
// sd card std icc functions
#define FUNC_SDSCConnectDev	    			5021	//u32(void)
#define FUNC_SDSCDisconnectDev	    		5022	//u32(void)
#define FUNC_SDSCGetFirmwareVer				5023	//u32(u8 *pbFirmwareVer, u32 *pulFirmwareVerLen)
#define FUNC_SDSCResetCard	    			5024	//u32(u8 *pbAtr, u32 *pulAtrLen)
#define FUNC_SDSCResetController	    	5025	//u32(u32 ulSCPowerMode)
#define FUNC_SDSCTransmit	    			5026	//u32(u8 *pbCommand, u32 ulCommandLen, u32 ulTimeOutMode, u8 *pbOutData, u32 *pulOutDataLen, u32 *pulCosState);
#define FUNC_SDSCTransmitEx	    			5027	//u32(u8 *pbCommand, u32 ulCommandLen, u32 ulTimeOutMode, u8 *pbOutData, u32 *pulOutDataLen);
#define FUNC_SDSCGetSDKVersion	    		5028	//u32(u8 *pszVersion, u32 *pulVersionLen);
#define FUNC_SDSCGetSCIOType	    		5029	//u32(u32 *pulSCIOType);
// sd card fatfs
#define FUNC_f_open	    					5040	//FRESULT(FIL* fp, const TCHAR* path, u8 mode);				/* Open or create a file */
#define FUNC_f_close	    				5041	//FRESULT(FIL* fp);											/* Close an open file object */
#define FUNC_f_read	    					5042	//FRESULT(FIL* fp, void* buff, u32 btr, u32* br);			/* Read data from a file */
#define FUNC_f_write	    				5043	//FRESULT(FIL* fp, const void* buff, u32 btw, u32* bw);	/* Write data to a file */
#define FUNC_f_lseek	    				5045	//FRESULT f_lseek (FIL* fp, u32 ofs);								/* Move file pointer of a file object */
#define FUNC_f_sync	    					5047	//FRESULT(FIL* fp);											/* Flush cached data of a writing file */
#define FUNC_f_mount	    				5063	//FRESULT(FATFS* fs, const TCHAR* path, u8 opt);			/* Mount/Unmount a logical drive */
#define FUNC_f_mkfs	    					5064	//FRESULT(const TCHAR* path, u8 sfd, u32 au);				/* Create a file system on the volume */


// mem card
#define FUNC_uiSetMemoryCardCode			9100	//(uchar ucCardType, uchar ucOffset, uchar ucLen, uchar *sCardCode)
#define FUNC_uiTestMemoryCardType			9101	//(uchar *sResetData)
#define FUNC_uiTestMemorySpecifiedType		9102	//(uchar ucCardType,uchar *sResetData)
#define FUNC_AT102_OpenCard					9103	//( void );
#define FUNC_AT102_CloseCard				9104	//( void );
#define FUNC_AT102_ChkSecurityCode 			9105	//( u8 *psSC );
#define FUNC_AT102_ReadWords 				9106	//( u8 ucWordAddr, u8 ucWordNum, u8 *psDataBuf );
#define FUNC_AT102_EraseNonApp				9107	//( u8 ucWordAddr, u8 ucWordNum );
#define FUNC_AT102_EraseApp					9108	//( u8 ucArea, u8 ucLimited, u8 *psEraseKey );
#define FUNC_AT102_WriteWords 				9109	//( u8 ucWordAddr, u8 ucWordNum, u8 *psDataBuf );
#define FUNC_AT102_BloweIssuFus				9110	//( void );
#define FUNC_AT102_BlowMFZFuse				9111	//( void );
#define FUNC_AT102_UpdSecurityCode			9112	//( u8 *psSC );
#define FUNC_AT102_UpdEraseKey				9113	//( u8 ucArea, u8 *psEraseKey );
#define FUNC_AT102_BlowEC2ENFuse			9114	//(void);
#define FUNC_AT102_ReadFZ 					9115	//( u8 *psFC );
#define FUNC_AT102_ReadIZ 					9116	//( u8 *psIC );
#define FUNC_AT102_ReadCPZ 					9117	//( u8 *psCPZ );
#define FUNC_AT102_ReadSCAC 				9118	//( u8 *psSCAC );
#define FUNC_AT102_ReadAZ 					9119	//( u8 ucArea, u8 *psAZ );
#define FUNC_AT102_ReadMTZ 					9120	//( u8 *psMTZ );
#define FUNC_AT102_ReadMFZ 					9121	//( u8 *psMFZ );
#define FUNC_AT102_UpdateIZ 				9122	//( u8 *psIC );
#define FUNC_AT102_UpdateCPZ 				9123	//( u8 *psCPZ );
#define FUNC_AT102_ReadPAC					9124	//( u8 ucIndex, u8 *psPAC );
#define FUNC_AT102_WriteAZ 					9125	//( u8 ucArea, u8 *psAZ );
#define FUNC_AT102_UpdateMTZ 				9126	//( u8 *psMTZ );
#define FUNC_AT102_UpdateMFZ 				9127	//( u8 *psMFZ );
#define FUNC_AT102_ChkSecurityCodeEx 		9128	//( u8 *psSC );
#define FUNC_AT102_EraseAppEx				9129	//( u8 ucArea, u8 ucLimited, u8 *psEraseKey );

#define FUNC_SLE4442_Open					9201	//(void);
#define FUNC_SLE4442_Close					9202	//(void);
#define FUNC_SLE4442_Read					9203	//(u8 Addr,u16 DataLen, u8* DataBuff);
#define FUNC_SLE4442_Write					9204	//(u8 Addr,u16 DataLen,u8* DataBuff);
#define FUNC_SLE4442_PrRead					9205	//(u8 Addr,u8 DataLen, u8* DataBuff);
#define FUNC_SLE4442_PrWrite				9206	//(u8 Addr,u8 DataLen,u8* DataBuff);
#define FUNC_SLE4442_ChkCode				9207	//(u8 *SecurityCode);
#define FUNC_SLE4442_ChkCodeEx				9208	//(u8 *SecurityCode);

#define FUNC_SLE4428_Open					9301	//((void);
#define FUNC_SLE4428_Close					9302	//((void);
#define FUNC_SLE4428_Read					9303	//((u16 Addr,u16 DataLen,u8 *DataBuff);
#define FUNC_SLE4428_PrRead					9304	//((u16 Addr,u16 DataLen,u8 *DataBuff,u8 *ProteBit);
#define FUNC_SLE4428_Write					9305	//((u16 Addr,u16 DataLen,u8* DataBuff);
#define FUNC_SLE4428_PrWrite				9306	//((u16 Addr,u16 DataLen,u8* DataBuff);
#define FUNC_SLE4428_ChkPSC					9307	//((u8 *Code);
#define FUNC_SLE4428_UpdPSC					9308	//((u8 *Code);
#define FUNC_SLE4428_ChkPSCEx				9309	//((u8 *Code);

#define FUNC_AT1604_OpenCard				9401	//(void);
#define FUNC_AT1604_CloseCard				9402	//(void);
#define FUNC_AT1604_ChkSC					9403	//(u8 *SC);
#define FUNC_AT1604_ChkSCn					9404	//(u8 Area,u8 *SCn);
#define FUNC_AT1604_ChkEZn					9405	//(u8 Area,u8 *EZn);
#define FUNC_AT1604_Read					9406	//(u16 Addr,u16 Len,u8 *DataBuf);
#define FUNC_AT1604_ReadAZ					9407	//(u8 Area,u16 Addr,u16 Len,u8 *DataBuf);
#define FUNC_AT1604_Erase					9408	//(u16 Addr,u16 Len);
#define FUNC_AT1604_EraseAZ					9409	//(u8 Area,u16 Addr,u16 Len);
#define FUNC_AT1604_Write					9410	//(u16 Addr,u16 Len,u8 *DataBuf);
#define FUNC_AT1604_WriteAZ					9411	//( u8 Area,u16 Addr,u16 Len,u8 *DataBuf );
#define FUNC_AT1604_ReadFZ					9412	//(u8 *FZ);
#define FUNC_AT1604_ReadIZ					9413	//(u8 *IZ);
#define FUNC_AT1604_ReadSC					9414	//(u8 *SC);
#define FUNC_AT1604_ReadSCAC				9415	//(u8 *SCAC);
#define FUNC_AT1604_ReadCPZ					9416	//(u8 *CPZ);
#define FUNC_AT1604_ReadSCn					9417	//(u8 Area,u8 *SCn);
#define FUNC_AT1604_ReadSnAC				9418	//(u8 Area,u8 *SnAC);
#define FUNC_AT1604_ReadEZn					9419	//(u8 Area,u8 *EZn);
#define FUNC_AT1604_ReadEnAC				9420	//(u8 Area,u8 *EnAC);
#define FUNC_AT1604_ReadMTZ					9421	//( u8 *MTZ );
#define FUNC_AT1604_UpdIZ					9422	//(u8 *IZ);
#define FUNC_AT1604_UpdSC					9423	//(u8 *SC);
#define FUNC_AT1604_UpdSCAC					9424	//(u8 *SCAC);
#define FUNC_AT1604_UpdCPZ					9425	//(u8 *CPZ);
#define FUNC_AT1604_UpdSCn					9426	//(u8 Area,u8 *SCn);
#define FUNC_AT1604_UpdSnAC					9427	//(u8 Area,u8 *SnAC);
#define FUNC_AT1604_UpdEZn					9428	//(u8 Area,u8 *EZn);
#define FUNC_AT1604_UpdEnAC					9429	//(u8 Area,u8 *EnAC);
#define FUNC_AT1604_UpdMTZ					9430	//( u8 *MTZ );
#define FUNC_AT1604_ChkSCEx					9431	//(u8 *SC);
#define FUNC_AT1604_ChkSCnEx				9432	//(u8 Area,u8 *SCn);
#define FUNC_AT1604_ChkEZnEx				9433	//(u8 Area,u8 *EZn);

#define FUNC_AT1608_OpenCard				9501	//(u8 *RstData);
#define FUNC_AT1608_CloseCard				9502	//(void);
#define FUNC_AT1608_SetAZ					9503	//(u8 AZ);
#define FUNC_AT1608_Read					9504	//(u8 Level,u8 Addr,u16 Len,u8 *DataBuf);
#define FUNC_AT1608_Write					9505	//(u8 Level,u8 Addr,u16 Len,u8 *DataBuf);
#define FUNC_AT1608_ReadFuse				9506	//(u8 *Fuse);
#define FUNC_AT1608_WriteFuse				9507	//(void);
#define FUNC_AT1608_VerifyPassword			9508	//(u8 Index,u8 *Password);
#define FUNC_AT1608_Auth					9509	//(u8 *Gc);
#define FUNC_AT1608_VerifyPasswordEx		9510	//(u8 Index,u8 *Password);
#define FUNC_AT1608_AuthEx					9511	//(u8 *Gc);

#define FUNC_AT24Cxx_OpenCard				9701	//(void)
#define FUNC_AT24Cxx_CloseCard				9702	//(void);
#define FUNC_AT24Cxx_Read					9703	//(u8 *Buff, u16 Addr, u16 Len);
#define FUNC_AT24Cxx_Write					9704	//(u8 *Buff, u16 Addr, u16 Len);
#define FUNC_AT24C32_Read					9705	//(u8 *Buff, u16 Addr, u16 Len);
#define FUNC_AT24C32_Write					9706	//(u8 *Buff, u16 Addr, u16 Len);

#define FUNC_uiGenCommKey					9801	//(uchar ucMasterKeyId, uchar *psRandom)


// 查询远程调用模式是否支持
int iGetRemoteCallMode(void);

// 远程调用数据发送之前进行打包和加密处理
void vPackData(uchar *psData, uint uiInLen, uchar *psMacRand);

// 接收到的返回数据进行解包处理，返回解包后的数据长度
// ret : > 0 : 成功
//		 <=0 : 失败
int iUnpackData(uchar *psData, uint uiInLen);

// 报文数据解析
// ret : 0 	 : 成功
//       !=0 : 失败
int iFactorize(uchar *psReceivedBuf, uint uiDataLen);

// 执行远程函数
int iDoRemoteFunc(uchar *psFunc, uint uiFuncLen, uint uiTimeOut);

uchar *psGetParaBuf(int index);
int iGetParaInt(int index);
long lGetParaLong(int index);

#endif
