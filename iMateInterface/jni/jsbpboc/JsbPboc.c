/**************************************
File name     : JsbPboc.c
Function      : ��������Pboc���ӿ�
Author        : Yu Jun
First edition : Apr 15th, 2014
Note          : ʵ�ֽ��������ṩ�Ľӿ�
Modified      :
**************************************/
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include "VposFace.h"
#include "EmvProc.h"
#include "JsbPboc.h"

//extern void vSetWriteLog(int iOnOff);
//extern void vWriteLogHex(char *pszTitle, void *pLog, int iLength);
//extern void vWriteLogTxt(char *pszFormat, ...);

/*******************************************************************************
�ӿ�˵��:
. ��������
  ÿ��������ǰ4������Ϊ��������, ������char *pszComNo, int nTermType, char cBpNo, int nIcFlag, ������ͳһ����, �Ժ�ÿ�����庯����������.
    pszComNo:	ͨѶ����, ���Žӿڲ���Ҫ
    nTermType:	�ն�����, ���Žӿڲ���Ҫ
    cBpNo:		BP�е�ת��, ���Žӿڲ���Ҫ
    nIcFlag:		������, 1:�Ӵ�IC�� 2:�ǽӴ�IC�� 3:�Զ��ж�
				ֻ�к���ICC_GetIcInfo()��ICC_GetTranDetail()��Ҫ
				�������������õ���ICC_GetIcInfo()����ʱȷ���Ŀ�����
*******************************************************************************/

int   sg_iRet;
static uchar sg_szAppData[4096];

// ��ʼ������
// ��ʼ��������Ҫ����IC���������ָ��
// in  : pfiTestCard	: ��⿨Ƭ�Ƿ����
//                        ret : 0 : ������
//                              1 : ����
//       pfiResetCard   : ��Ƭ��λ
//                        ret : <=0 : ��λ����
//                              >0  : ��λ�ɹ�, ����ֵΪATR����
//       pfiDoApdu      : ִ��APDUָ��
//                        in  : iApduInLen   : Apduָ���
//                              psApduIn     : Apduָ��, ��ʽ: Cla Ins P1 P2 Lc DataIn Le
//                        out : piApduOutLen : ApduӦ�𳤶�
//                              psApduOut    : ApduӦ��, ��ʽ: DataOut Sw1 Sw2
//                        ret : 0            : �������ɹ�
//                              1            : ��������
//       pfiCloseCard   : �رտ�Ƭ
//                        ret : ������
// ret : 0				: �ɹ�
//       <0             : ʧ��
// Note: �ڵ����κ������ӿ�ǰ�������ȳ�ʼ������
int ICC_InitEnv(int (*pfiTestCard)(void),
			    int (*pfiResetCard)(unsigned char *psAtr),
			    int (*pfiDoApdu)(int iApduInLen, unsigned char *psApduIn, int *piApduOutLen, unsigned char *psApduOut),
			    int (*pfiCloseCard)(void))
{
	static int iInitFlag = 0;
	stHxTermParam HxTermParam;
	stHxTermAid   aHxTermAid[5];

	if(iInitFlag)
		return(0);

	_vPosInit();

	sg_iRet = iHxEmvInit(pfiTestCard, pfiResetCard, pfiDoApdu, pfiCloseCard);
	if(sg_iRet)
		return(JSBPBOC_CORE_INIT);

	// �����ն˲���
	memset(&HxTermParam, 0, sizeof(HxTermParam));
	HxTermParam.ucTermType = 0x11;
	memcpy(HxTermParam.sTermCapability, "\xA0\x00\x00", 3);
	memcpy(HxTermParam.sAdditionalTermCapability, "\xEF\x80\xF0\xF0\x00", 5);
	strcpy(HxTermParam.szMerchantId, "123456789000001");
	strcpy(HxTermParam.szTermId, "12345601");
	HxTermParam.uiTermCountryCode = 156;
	strcpy(HxTermParam.szAcquirerId, "000000");
	HxTermParam.iMerchantCategoryCode = -1;
	HxTermParam.ucPinBypassBehavior = 0;
	HxTermParam.ucAppConfirmSupport = 1;

	memcpy(HxTermParam.AidCommonPara.sTermAppVer, "\x00\x20", 2);
	HxTermParam.AidCommonPara.ulFloorLimit = 0xFFFFFFFEL;
	HxTermParam.AidCommonPara.iMaxTargetPercentage = -1;
	HxTermParam.AidCommonPara.iTargetPercentage = -1;
	HxTermParam.AidCommonPara.ulThresholdValue = 0xFFFFFFFFL;
	HxTermParam.AidCommonPara.ucECashSupport = 0;
	strcpy(HxTermParam.AidCommonPara.szTermECashTransLimit, "999999999999");
	HxTermParam.AidCommonPara.ucTacDefaultExistFlag = 1;
	memcpy(HxTermParam.AidCommonPara.sTacDefault, "\xFF\xFF\xFF\xFF\xFF", 5);
	HxTermParam.AidCommonPara.ucTacDenialExistFlag = 1;
	memcpy(HxTermParam.AidCommonPara.sTacDenial, "\x00\x00\x00\x00\x00", 5);
	HxTermParam.AidCommonPara.ucTacOnlineExistFlag = 1;
	memcpy(HxTermParam.AidCommonPara.sTacOnline, "\xFF\xFF\xFF\xFF\xFF", 5);
	HxTermParam.AidCommonPara.iDefaultDDOLLen = 3;
	memcpy(HxTermParam.AidCommonPara.sDefaultDDOL, "\x9F\x37\x04", 3);
	HxTermParam.AidCommonPara.iDefaultTDOLLen = -1;
	sg_iRet = iHxEmvSetParam(&HxTermParam);
	if(sg_iRet)
		return(JSBPBOC_CORE_INIT);

	/*
	 * 	unsigned char sTermAppVer[2];               // T9F09, �ն�Ӧ�ð汾��, "\xFF\xFF"��ʾ������
	unsigned long ulFloorLimit;					// T9F1B, �ն��޶�, ��λΪ��, 0xFFFFFFFF��ʾ������
    int			  iMaxTargetPercentage;         // ���ѡ�����ٷֱȣ�-1:������
    int			  iTargetPercentage;            // ���ѡ��Ŀ��ٷֱȣ�-1:������
	unsigned long ulThresholdValue;             // ���ѡ����ֵ, 0xFFFFFFFF��ʾ������
    unsigned char ucECashSupport;				// 1:֧�ֵ����ֽ� 0:��֧�ֵ����ֽ�, 0xFF��ʾ������
	unsigned char szTermECashTransLimit[12+1];  // T9F7B, �ն˵����ֽ����޶�, �ձ�ʾ������
	unsigned char ucTacDefaultExistFlag;        // 1:TacDefault����, 0:TacDefault������
    unsigned char sTacDefault[5];				// TAC-Default, �ο�TVR�ṹ
	unsigned char ucTacDenialExistFlag;         // 1:TacDenial����, 0:TacDenial������
    unsigned char sTacDenial[5];				// TAC-Denial, �ο�TVR�ṹ
	unsigned char ucTacOnlineExistFlag;         // 1:TacOnline����, 0:TacOnline������
    unsigned char sTacOnline[5];				// TAC-Online, �ο�TVR�ṹ
    int           iDefaultDDOLLen;              // Default DDOL����,-1��ʾ��
    unsigned char sDefaultDDOL[252];            // Default DDOL(TAG_DFXX_DefaultDDOL)
    int           iDefaultTDOLLen;              // Default TDOL����,-1��ʾ��
    unsigned char sDefaultTDOL[252];            // Default TDOL(TAG_DFXX_DefaultTDOL)
	 */

	memset(&aHxTermAid[0], 0, sizeof(aHxTermAid));
	aHxTermAid[0].ucAidLen = 8;
	memcpy(aHxTermAid[0].sAid, "\xA0\x00\x00\x03\x33\x01\x01\x01", 8);
	aHxTermAid[0].ucASI = 1;
	aHxTermAid[0].cOnlinePinSupport = 1;
	memcpy(aHxTermAid[0].sTermAppVer, "\xFF\xFF", 2);
	aHxTermAid[0].ulFloorLimit = 0xFFFFFFFFL;
	aHxTermAid[0].iMaxTargetPercentage = -1;
	aHxTermAid[0].iTargetPercentage = -1;
	aHxTermAid[0].ulThresholdValue = 0xFFFFFFFFL;
	aHxTermAid[0].ucECashSupport = 0xFF;
	aHxTermAid[0].iDefaultDDOLLen = -1;
	aHxTermAid[0].iDefaultTDOLLen = -1;
	memcpy(&aHxTermAid[1], &aHxTermAid[0], sizeof(stHxTermAid));
	aHxTermAid[1].ucAidLen = 8;
	memcpy(aHxTermAid[0].sAid, "\xA0\x00\x00\x03\x33\x01\x01\x02", 8);
	memcpy(&aHxTermAid[2], &aHxTermAid[0], sizeof(stHxTermAid));
	aHxTermAid[2].ucAidLen = 8;
	memcpy(aHxTermAid[0].sAid, "\xA0\x00\x00\x03\x33\x01\x01\x03", 8);
	sg_iRet = iHxEmvLoadAid(&aHxTermAid[0], 3);
	if(sg_iRet)
		return(JSBPBOC_CORE_INIT);

	sg_szAppData[0] = 0;
	iInitFlag = 1;

	return(JSBPBOC_OK);
}

// �����ַ���
// in  : iFlag   : ��־, 0:ȥ��ͷ��'0' 1:ȥ��β��'F'
//     : pszData : �������ַ���
// out : pszData : ����õ��ַ���
static int iReformString(int iFlag, uchar *pszData)
{
	int   iLen, i;
	uchar szData[256];

	iLen = strlen((char *)pszData);
	switch(iFlag) {
	case 0: // ȥ��ͷ��'0'
		for(i=0; i<iLen; i++)
			if(pszData[i] != '0')
				break;
		if(i >= iLen) {
			strcpy((char *)pszData, "0"); // ȫ��'0'
			break;
		}
		strcpy((char *)szData, (char *)pszData+i);
		strcpy((char *)pszData, (char *)szData);
		break;
	case 1: // ȥ��β��'F'
		for(i=iLen-1; i>=0; i--) {
			if(pszData[i]!='F' && pszData[i]!='f')
				break;
			pszData[i] = 0;
		}
		break;
	default:
		break; // ������
	}
	return(strlen((char *)pszData));
}

// ������Ϣ
// In  : pszTagList   : ��ʾ��Ҫ���ص��û����ݵı�ǩ�б�
// Out : pnUsrInfoLen :	���ص��û����ݳ���
//       pszUserInfo  : ���ص��û�����, ���������ٱ���1024�ֽ�
//       pszAppData   :	IC������, ����Tag8C��Tag8D��, ��������55��, ���������ٱ���4096�ֽ�
//       pnIcType     : �ϵ�ɹ��Ŀ�����, 1:�Ӵ��� 2:�ǽӿ�
//						���ں���ICC_GenARQC()��ICC_CtlScriptData()����
// Ret : 0            : �ɹ�
//       <0           : ʧ��
int ICC_GetIcInfo(char *pszComNo, int nTermType, char cBpNo, int nIcFlag,
						char *pszTaglist,
						int *pnUsrInfoLen, char *pszUserInfo, char *pszAppData, int *pnIcType)
{
	int i;

	if(nIcFlag!=1 && nIcFlag!=3)
		return(JSBPBOC_CARD_TYPE);

	sg_iRet = iHxEmvTransInit(0);
	if(sg_iRet)
		return(JSBPBOC_TRANS_INIT);

	sg_szAppData[0] = 0;

	for(;;) {
		stHxAdfInfo aHxAdfInfo[5];
		int iHxAdfNum;
		iHxAdfNum = sizeof(aHxAdfInfo) / sizeof(aHxAdfInfo[0]);
		memset(&aHxAdfInfo[0], 0, sizeof(aHxAdfInfo));
		sg_iRet = iHxEmvGetSupportedApp(0/*Ӧ������ʱ����ѡ*/, &aHxAdfInfo[0], &iHxAdfNum);
		if(sg_iRet == HXEMV_NO_APP)
			return(JSBPBOC_NO_APP);
		if(sg_iRet == HXEMV_CARD_SW || sg_iRet == HXEMV_CARD_OP || sg_iRet == HXEMV_CARD_REMOVED)
			return(JSBPBOC_CARD_IO);
		if(sg_iRet && sg_iRet!=HXEMV_AUTO_SELECT)
			return(JSBPBOC_UNKNOWN);

		sg_iRet = iHxEmvAppSelect(0/*Ӧ������ʱ����ѡ*/, aHxAdfInfo[0].ucAdfNameLen, aHxAdfInfo[0].sAdfName);
		if(sg_iRet == HXEMV_CARD_SW || sg_iRet == HXEMV_CARD_OP || sg_iRet == HXEMV_CARD_REMOVED)
			return(JSBPBOC_CARD_IO);
		if(sg_iRet == HXEMV_RESELECT)
			continue;
		if(sg_iRet)
			return(JSBPBOC_UNKNOWN);

		sg_iRet = iHxEmvGPO("20140415000000", 1, 0x00, "0", 156);
		if(sg_iRet == HXEMV_CARD_SW || sg_iRet == HXEMV_CARD_OP || sg_iRet == HXEMV_CARD_REMOVED)
			return(JSBPBOC_CARD_IO);
		if(sg_iRet == HXEMV_RESELECT)
			continue;
		if(sg_iRet)
			return(JSBPBOC_UNKNOWN);
		break;
	} // for(;;

	sg_iRet = iHxEmvReadRecord();
	if(sg_iRet == HXEMV_CARD_SW || sg_iRet == HXEMV_CARD_OP || sg_iRet == HXEMV_CARD_REMOVED)
		return(JSBPBOC_CARD_IO);
	if(sg_iRet)
		return(JSBPBOC_UNKNOWN);

	sg_iRet = iHxEmvOfflineDataAuth(NULL, NULL, NULL, NULL);
	if(sg_iRet)
		return(JSBPBOC_UNKNOWN);

	sg_iRet = iHxEmvProcRistrictions();
	if(sg_iRet)
		return(JSBPBOC_UNKNOWN);

	sg_iRet = iHxEmvTermRiskManage();
	if(sg_iRet)
		return(JSBPBOC_UNKNOWN);

	sg_iRet = iHxEmvTermActionAnalysis();
	if(sg_iRet)
		return(JSBPBOC_UNKNOWN);

	// ��������, ��֯�ӿ�����
	*pnIcType = 1; // ���ڽӿ�ֻ֧�ֽӴ���, һ���ߵ��ǽӴ�ͨ��
	*pnUsrInfoLen = 0;
	for(i=0; i<(int)strlen(pszTaglist); i++) {
		uchar sData[128], szData[256];
		int   iDataLen;
		switch(pszTaglist[i]) {
		case 'A': // Ӧ�����˺�
			iDataLen = sizeof(szData);
			sg_iRet = iHxEmvGetData("\x5A", NULL, NULL, &iDataLen, szData);
			if(sg_iRet)
				return(JSBPBOC_DATA_LOSS);
			break;
		case 'B': // ����2 ��Ч����
			iDataLen = sizeof(sData);
			sg_iRet = iHxEmvGetData("\x57", NULL, NULL, &iDataLen, sData);
			if(sg_iRet) {
				szData[0] = 0; // �ŵ�2��Ч���ݲ��Ǳر�����
			}
			vOneTwoX0(sData, iDataLen, szData);
			iReformString(1/*1:ȥ��β����'F'*/, szData);
			break;
		case 'C': // �����ֽ����
			iDataLen = sizeof(szData);
			sg_iRet = iHxEmvGetCardNativeData("\x9F\x79", NULL, NULL, &iDataLen, szData);
			if(sg_iRet == HXEMV_CARD_SW || sg_iRet == HXEMV_CARD_OP || sg_iRet == HXEMV_CARD_REMOVED)
				return(JSBPBOC_CARD_IO);
			if(sg_iRet) {
				szData[0] = 0;
			} else {
				iReformString(0/*0:ȥ��ͷ��'0'*/, szData);
			}
			break;
		case 'D': // �����ֽ��������
			iDataLen = sizeof(szData);
			sg_iRet = iHxEmvGetCardNativeData("\x9F\x77", NULL, NULL, &iDataLen, szData);
			if(sg_iRet == HXEMV_CARD_SW || sg_iRet == HXEMV_CARD_OP || sg_iRet == HXEMV_CARD_REMOVED)
				return(JSBPBOC_CARD_IO);
			if(sg_iRet) {
				szData[0] = 0;
			} else {
				iReformString(0/*0:ȥ��ͷ��'0'*/, szData);
			}
			break;
		case 'E': // Ӧ��ʧЧ����
			iDataLen = sizeof(szData);
			sg_iRet = iHxEmvGetData("\x5F\x24", NULL, NULL, &iDataLen, szData);
			if(sg_iRet)
				return(JSBPBOC_DATA_LOSS);
			break;
		case 'F': // �����ֽ𵥱ʽ����޶�
			iDataLen = sizeof(szData);
			sg_iRet = iHxEmvGetCardNativeData("\x9F\x78", NULL, NULL, &iDataLen, szData);
			if(sg_iRet == HXEMV_CARD_SW || sg_iRet == HXEMV_CARD_OP || sg_iRet == HXEMV_CARD_REMOVED)
				return(JSBPBOC_CARD_IO);
			if(sg_iRet) {
				szData[0] = 0;
			} else {
				iReformString(0/*0:ȥ��ͷ��'0'*/, szData);
			}
			break;
		case 'G': // Ӧ�����˺����к�
			iDataLen = sizeof(szData);
			sg_iRet = iHxEmvGetData("\x5F\x34", NULL, NULL, &iDataLen, szData);
			if(sg_iRet) {
				szData[0] = 0;
			} else {
				iReformString(0/*0:ȥ��ͷ��'0'*/, szData);
			}
			break;
		case 'H': // Ӧ�ý��׼�����
			iDataLen = sizeof(sData);
			sg_iRet = iHxEmvGetCardNativeData("\x9F\x36", NULL, NULL, &iDataLen, sData);
			if(sg_iRet == HXEMV_CARD_SW || sg_iRet == HXEMV_CARD_OP || sg_iRet == HXEMV_CARD_REMOVED)
				return(JSBPBOC_CARD_IO);
			if(sg_iRet)
				return(JSBPBOC_DATA_LOSS);
			vOneTwo0(sData, 2, szData);
			break;
		case 'I': // �ֿ���֤������
			iDataLen = sizeof(sData);
			sg_iRet = iHxEmvGetData("\x9F\x62", NULL, NULL, &iDataLen, sData);
			if(sg_iRet) {
				szData[0] = 0;
			} else {
				sprintf(szData, "%d", sData[0]);
			}
			break;
		case 'J': // �ֿ���֤����
			iDataLen = sizeof(szData);
			sg_iRet = iHxEmvGetData("\x9F\x61", NULL, NULL, &iDataLen, szData);
			if(sg_iRet) {
				szData[0] = 0;
			}
			break;
		case 'K': // �ֿ�������
			iDataLen = sizeof(szData);
			sg_iRet = iHxEmvGetData("\x5F\x20", NULL, NULL, &iDataLen, szData);
			if(sg_iRet) {
				szData[0] = 0;
			}
			break;
		case 'L': // �ֿ���������չ
			iDataLen = sizeof(szData);
			sg_iRet = iHxEmvGetData("\x9F\x0B", NULL, NULL, &iDataLen, szData);
			if(sg_iRet) {
				szData[0] = 0;
			}
			break;
		default:
			return(JSBPBOC_TAG_UNKNOWN);
		}
		iDataLen = strlen((char *)szData);
		pszUserInfo[(*pnUsrInfoLen)++] = pszTaglist[i];
		sprintf(&pszUserInfo[*pnUsrInfoLen], "%03d", iDataLen);
		(*pnUsrInfoLen) += 3;
		memcpy(&pszUserInfo[*pnUsrInfoLen], szData, iDataLen);
		(*pnUsrInfoLen) += iDataLen;
	} // for(i=0; i<strlen(pszTaglist); i++) {

	{
		static unsigned short auiTag[] = {
			0x4F00, 0x5000, 0x5A00, 0x5F20, 0x5F24, 0x5F25, 0x5F28, 0x5F30,
			0x5F34, 0x5F53, 0x5F54, 0x8200, 0x8C00, 0x8D00, 0x8E00, 0x9F05,
			0x9F07, 0x9F08, 0x9F0B, 0x9F0D, 0x9F0E, 0x9F0F, 0x9F12, 0x9F13,
			0x9F14, 0x9F23, 0x9F36, 0x9F38, 0x9F42, 0x9F4D, 0x9F4F, 0x9F61,
			0x9F62, 0x9F63, 0x9F6D, 0x9F74, 0
		};
		uchar sOutBuf[2048];
		int   iOutLen;
		iOutLen = 0;
		for(i=0; ; i++) {
			uchar sTag[2], sTlvObj[270];
			int   iTlvObjLen;
			int   iRet;
			if(auiTag[i] == 0)
				break;
			vLongToStr((ulong)auiTag[i], 2, sTag);
			iTlvObjLen = sizeof(sTlvObj);
			iRet = iHxEmvGetData(sTag, &iTlvObjLen, sTlvObj, NULL, NULL);
			if(iRet) {
				// û�ж�������, �ж��Ƿ�Ϊ��Ƭ�ڲ�����
				if(auiTag[i]==0x9F13 || auiTag[i]==0x9F36 || auiTag[i]==0x9F4F || auiTag[i]==0x9F6D) {
					// ��Ƭ�ڲ�����
					iTlvObjLen = sizeof(sTlvObj);
					iRet = iHxEmvGetCardNativeData(sTag, &iTlvObjLen, sTlvObj, NULL, NULL);
				}
			}
			if(iRet)
				continue;
			memcpy(sOutBuf+iOutLen, sTlvObj, iTlvObjLen);
			iOutLen += iTlvObjLen;
		}
		vOneTwo0(sOutBuf, iOutLen, pszAppData);
		strcpy(sg_szAppData, pszAppData);
	}

	return(JSBPBOC_OK);
}

// ��ȡPbocӦ�ð汾, ������ICC_GetIcInfo֮��ִ��
// Out : pszPbocVer   :	���ص�PBOC�汾�ţ�"0002"��ʶPBOC2.0��"0003"��ʶPBOC3.0�� ��������С >= 5bytes
// Ret : 0            : �ɹ�
//       !=0          : ʧ��
int ICC_GetIcPbocVersion(char *pszPbocVer)
{
    uchar sOutData[10];
    int iOutDataLen, retCode;

    iOutDataLen = sizeof(sOutData);
    retCode = iHxEmvGetData("\x9f\x08", NULL, NULL, &iOutDataLen, sOutData);
    if (retCode)
        return retCode;

    vOneTwo0(sOutData, 2, pszPbocVer);

    return 0;
}


// Ϊ�������ж��Ƶ�TLV���ݿ��滻����
// ��EmvCore.cģ����
extern int iHxReplaceTlvDb_Jsb(uchar *pszAmount, uchar *pszAmountOther, int iCurrencyCode,
						uchar *pszTransDate, uchar ucTransType, uchar *pszTransTime, int iCountryCode, uchar *pszNodeName);
static int iReplaceCoreData(char *pszTxData)
{
	uchar szAmount[13], szAmountOther[13], szTransDate[7], ucTransType, szTransTime[7], szNodeName[256];
	int   iCurrencyCode, iCountryCode;

	szAmount[0] = 0;
	szAmountOther[0] = 0;
	szTransDate[0] = 0;
	ucTransType = 0xFF;
	szTransTime[0] = 0;
	szNodeName[0] = 0;
	iCurrencyCode = -1;
	iCountryCode = -1;
	while(*pszTxData) {
		int   iTxDataLen;
		uchar *p;
		iTxDataLen = ulA2L(pszTxData+1, 3);
		p = pszTxData;
		pszTxData += 4 + iTxDataLen;
		switch(p[0]) {
		case 'P': // ��Ȩ���
			if(iTxDataLen > 12)
				return(JSBPBOC_PARAM);
			vMemcpy0(szAmount, p+4, iTxDataLen);
			break;
		case 'Q': // �������
			if(iTxDataLen > 12)
				return(JSBPBOC_PARAM);
			vMemcpy0(szAmountOther, p+4, iTxDataLen);
			break;
		case 'R': // ���Ҵ���
			iCurrencyCode = ulA2L(p+4, iTxDataLen);
			break;
		case 'S': // �������� YYMMDD
			if(iTxDataLen != 6)
				return(JSBPBOC_PARAM);
			vMemcpy0(szTransDate, p+4, iTxDataLen);
			break;
		case 'T': // ��������
			if(iTxDataLen == 0)
				break;
			if(iTxDataLen > 2)
				return(JSBPBOC_PARAM);
			if(iTxDataLen == 1)
				ucTransType = p[4] - 0x30;
			else
				vTwoOne(p+4, 2, &ucTransType);
			break;
		case 'U': // ����ʱ�� hhmmss
			if(iTxDataLen != 6)
				return(JSBPBOC_PARAM);
			vMemcpy0(szTransTime, p+4, iTxDataLen);
			break;
		case 'V': // ���Ҵ���
			iCountryCode = ulA2L(p+4, iTxDataLen);
			break;
		case 'W': // �̻�����
			if(iTxDataLen > 250)
				return(JSBPBOC_PARAM);
			vMemcpy0(szNodeName, p+4, iTxDataLen);
			break;
		default:
			return(JSBPBOC_TAG_UNKNOWN);
		} // switch(*pszTxData) {
	} // while(*pszTxData

	sg_iRet = iHxReplaceTlvDb_Jsb(szAmount, szAmountOther, iCurrencyCode,
						       szTransDate, ucTransType, szTransTime, iCountryCode, szNodeName);
	if(sg_iRet)
		return(JSBPBOC_PARAM);

	return(0);
}

// �����������
static int iGenOutData(uchar *psOutData)
{
	static unsigned short auiTag[] = {
		0x9F26, // Ӧ������ ��8�ֽڣ���
		0x9F27, // ������Ϣ���� ��1�ֽڣ�
		0x9F10, // ������Ӧ������ �����32�ֽڣ���
		0x9F37, // �����(4�ֽ�)
		0x9F36, // ATC ��2�ֽڣ�
		0x9500, // �ն���֤��� ��5�ֽڣ�
		0x9A00, // �������ڣ�3�ֽ�)
		0x9C00, // �������� ��1�ֽڣ�
		0x9F02, // ��Ȩ��� ��6�ֽڣ�
		0x5F2A, // ���׻��Ҵ��� ��2�ֽڣ�
		0x8200, // Ӧ�ý������� ��2�ֽڣ�
		0x9F1A, // �ն˹��Ҵ��� ��2�ֽڣ�
		0x9F03, // ������� ��6�ֽڣ�
		0x9F33, // �ն����� ��3�ֽڣ�
		0x9F13, // �ϴ�����Ӧ�ý��׼������Ĵ��� ��1�ֽڣ�
		0x5A00, // Ӧ�����˺ţ�PAN�� �����10�ֽڣ�
		0x5F34, // Ӧ��PAN���к� ��1�ֽڣ�

		0xDF31,
		0
	};
	int   i;
	int   iOutLen;
	iOutLen = 0;
	for(i=0; ; i++) {
		uchar sTag[2], sTlvObj[270];
		int   iTlvObjLen;
		int   iRet;
		if(auiTag[i] == 0)
			break;
		vLongToStr((ulong)auiTag[i], 2, sTag);
		iTlvObjLen = sizeof(sTlvObj);
		iRet = iHxEmvGetData(sTag, &iTlvObjLen, sTlvObj, NULL, NULL);
		if(iRet)
			continue;
		memcpy(psOutData+iOutLen, sTlvObj, iTlvObjLen);
		iOutLen += iTlvObjLen;
	}
	return(iOutLen);
}

// ����ARQC
// In  : pszTxData    : ��������, ��ǩ�ӳ������ݸ�ʽ
//       pszAppData   : ICC_GetIcInfo�������ص�Ӧ������, ���뱣����ԭ������ͬ
// Out : pnARQCLen    : ���ص�ARQC����
//       pszARQC      : ���ص�ARQC, TLV��ʽ, ת��Ϊ16���ƿɶ���ʽ
// Ret : 0            : �ɹ�
//       <0           : ʧ��
int ICC_GenARQC(char *pszComNo, int nTermType, char cBpNo, int nIcFlag,
						char *pszTxData, char *pszAppData, int *pnARQCLen, char *pszARQC)
{
	uchar sOutData[2048];
	int   iOutLen;
	int   iCardAction;
	int   iRet;

	if(nIcFlag!=1 && nIcFlag!=2)
		return(JSBPBOC_CARD_TYPE);
	/*
	if(strcmp(pszAppData, sg_szAppData) != 0) {
		vSetWriteLog(1);
		vWriteLogTxt("ICC_GenARQC ret = %d", JSBPBOC_APP_DATA);
		vWriteLogTxt("pszAppData(%d)   = [%s]", strlen(pszAppData), pszAppData);
		vWriteLogTxt("sg_szAppData(%d) = [%s]", strlen(sg_szAppData), sg_szAppData);
		vSetWriteLog(0);
		return(JSBPBOC_APP_DATA);
	}
	*/

	iRet = iReplaceCoreData(pszTxData);
	if(iRet)
		return(iRet);

	// GAC1
	sg_iRet = iHxEmvGac1(1/*1:ǿ������*/ , &iCardAction);
	if(sg_iRet == HXEMV_CARD_SW || sg_iRet == HXEMV_CARD_OP || sg_iRet == HXEMV_CARD_REMOVED)
		return(JSBPBOC_CARD_IO);
	if(sg_iRet)
		return(JSBPBOC_UNKNOWN);
	if(iCardAction != GAC_ACTION_ARQC)
		return(JSBPBOC_GEN_ARQC);

	// �����������
	iOutLen = iGenOutData(sOutData);
	vOneTwo0(sOutData, iOutLen, pszARQC);
	*pnARQCLen = iOutLen * 2;

	return(0);
}

// ���׽�������
// In  : pszTxData    : ��������, ��ǩ�ӳ������ݸ�ʽ
//       pszAppData   : ICC_GetIcInfo�������ص�Ӧ������, ���뱣����ԭ������ͬ
//       pszARPC      : ����������, TLV��ʽ, ת��Ϊ16���ƿɶ���ʽ
// Out : pnTCLen      : ���ص�TC����
//       pszTC        : ���ص�TC, TLV��ʽ, ת��Ϊ16���ƿɶ���ʽ
// Ret : 0            : �ɹ�
//       <0           : ʧ��
int ICC_CtlScriptData(char *pszComNo, int nTermType, char cBpNo, int nIcFlag,
						char *pszTxData, char *pszAppData, char *pszARPC,
						int *pnTCLen, char *pszTC, char *pszScriptResult)
{
	uchar sOutData[2048];
	int   iOutLen;
	uchar sIssuerData[256];
	int   iIssuerDataLen;
	int   iCardAction;
	int   iRet;

	if(nIcFlag!=1 && nIcFlag!=2)
		return(JSBPBOC_CARD_TYPE);

	if(strcmp(pszAppData, sg_szAppData) != 0) {
		//vSetWriteLog(1);
		//vWriteLogTxt("ICC_CtlScriptData ret = %d", JSBPBOC_APP_DATA);
		//vWriteLogTxt("pszAppData(%d)   = [%s]", strlen(pszAppData), pszAppData);
		//vWriteLogTxt("sg_szAppData(%d) = [%s]", strlen(sg_szAppData), sg_szAppData);
		//vSetWriteLog(0);
		return(JSBPBOC_APP_DATA);
	}

	iIssuerDataLen = strlen(pszARPC);
	if(iIssuerDataLen>512 || iIssuerDataLen%2)
		return(JSBPBOC_PARAM);
	vTwoOne(pszARPC, iIssuerDataLen, sIssuerData);
	iIssuerDataLen /= 2;

	iRet = iReplaceCoreData(pszTxData);
	if(iRet)
		return(iRet);

	sg_iRet = iHxEmvGac2("00", NULL, sIssuerData, iIssuerDataLen, &iCardAction);
	if(sg_iRet == HXEMV_CARD_SW || sg_iRet == HXEMV_CARD_OP || sg_iRet == HXEMV_CARD_REMOVED)
		return(JSBPBOC_CARD_IO);
	if(sg_iRet)
		return(JSBPBOC_UNKNOWN);

	// �����������
	iOutLen = iGenOutData(sOutData);
	vOneTwo0(sOutData, iOutLen, pszTC);
	*pnTCLen = iOutLen * 2;

	iOutLen = sizeof(sOutData);
	iRet = iHxEmvGetData("\xDF\x31", &iOutLen, sOutData, NULL, NULL);
	if(iRet) {
		// ����޽ű�, ���ɽű�δִ��Tlv����
		memcpy(sOutData, "\xDF\x31\x05\x00\x00\x00\x00\x00", 8);
		iOutLen = 8;
	}
	vOneTwo0(sOutData, iOutLen, pszScriptResult);

	return(0);
}

// ��ȡ������ϸ
// Out : pnTxDetailLen:	������ϸ����
//       TxDetail     :	������ϸ, ��ʽΪ
//                      ��ϸ����(2�ֽ�ʮ����)+ÿ����ϸ�ĳ���(3�ֽ�ʮ����) + ��ϸ1+��ϸ2+...
// Ret : 0            : �ɹ�
//       <0           : ʧ��
// Note: ���10����¼
int ICC_GetTranDetail(char *pszComNo, int nTermType, char BpNo, int nIcFlag,
						int *pnTxDetailLen, char *TxDetail)
{
	int   i;
	int   iLogLen;
	uchar sLog[100], *p;
	uchar szBuf[21];
	int   iMaxRecNum;

	if(nIcFlag!=1 && nIcFlag!=3)
		return(JSBPBOC_CARD_TYPE);

	sg_iRet = iHxEmvTransInit(0);
	if(sg_iRet)
		return(JSBPBOC_TRANS_INIT);

	sg_szAppData[0] = 0;

	for(;;) {
		stHxAdfInfo aHxAdfInfo[5];
		int iHxAdfNum;
		iHxAdfNum = sizeof(aHxAdfInfo) / sizeof(aHxAdfInfo[0]);
		memset(&aHxAdfInfo[0], 0, sizeof(aHxAdfInfo));
		sg_iRet = iHxEmvGetSupportedApp(1/*Ӧ������ʱ��ѡ*/, &aHxAdfInfo[0], &iHxAdfNum);
		if(sg_iRet == HXEMV_NO_APP)
			return(JSBPBOC_NO_APP);
		if(sg_iRet == HXEMV_CARD_SW || sg_iRet == HXEMV_CARD_OP || sg_iRet == HXEMV_CARD_REMOVED)
			return(JSBPBOC_CARD_IO);
		if(sg_iRet && sg_iRet!=HXEMV_AUTO_SELECT)
			return(JSBPBOC_UNKNOWN);

		sg_iRet = iHxEmvAppSelect(1/*Ӧ������ʱ��ѡ*/, aHxAdfInfo[0].ucAdfNameLen, aHxAdfInfo[0].sAdfName);
		if(sg_iRet == HXEMV_CARD_SW || sg_iRet == HXEMV_CARD_OP || sg_iRet == HXEMV_CARD_REMOVED)
			return(JSBPBOC_CARD_IO);
		if(sg_iRet == HXEMV_RESELECT)
			continue;
		if(sg_iRet)
			return(JSBPBOC_UNKNOWN);
		break;
	} // for(;;

	sg_iRet = iHxEmvGetLogInfo(0/*0:��׼������ˮ*/, &iMaxRecNum);
	if(sg_iRet) {
		strcpy(TxDetail, "00106");
		*pnTxDetailLen = strlen(TxDetail);
		return(0);
	}
	if(iMaxRecNum > 10)
		iMaxRecNum = 10;

	iLogLen = sizeof(sLog);
	sg_iRet = iHxEmvGetCardNativeData("\x9F\x4F", NULL, NULL, &iLogLen, sLog);
	if(sg_iRet == HXEMV_CARD_SW || sg_iRet == HXEMV_CARD_OP || sg_iRet == HXEMV_CARD_REMOVED)
		return(JSBPBOC_CARD_IO);
	if(sg_iRet) {
		strcpy(TxDetail, "00106");
		*pnTxDetailLen = strlen(TxDetail);
		return(0);
	}
	if(iLogLen != 25)
		return(JSBPBOC_LOG_FORMAT);
	if(memcmp(sLog, "\x9A\x03\x9F\x21\x03\x9F\x02\x06\x9F\x03\x06\x9F\x1A\x02\x5F\x2A\x02\x9F\x4E\x14\x9C\x01\x9F\x36\x02", iLogLen) != 0)
		return(JSBPBOC_LOG_FORMAT);

	// ׼���������
	strcpy(TxDetail, "00106"); // ����0, ������Ϊ��ȷֵ
	*pnTxDetailLen = 5;
	p = TxDetail + 5;
	for(i=1; i<=iMaxRecNum; i++) {
		iLogLen = sizeof(sLog);
		sg_iRet = iHxEmvReadLog(0/*0:��׼������ˮ*/, i, &iLogLen, sLog);
		if(sg_iRet == HXEMV_CARD_SW || sg_iRet == HXEMV_CARD_OP || sg_iRet == HXEMV_CARD_REMOVED)
			return(JSBPBOC_CARD_IO);
		if(sg_iRet == HXEMV_NO_RECORD)
			break;
		if(sg_iRet)
			return(JSBPBOC_UNKNOWN);
		
		// ��Ȩ���
		strcpy(p, "P012");
		vOneTwo0(sLog+6, 6, p+4);
		p += 16;
		// �������
		strcpy(p, "Q012");
		vOneTwo0(sLog+12, 6, p+4);
		p += 16;
		// ���׻��Ҵ���
		strcpy(p, "R004");
		vOneTwo0(sLog+20, 2, p+4);
		p += 8;
		// ��������
		strcpy(p, "S006");
		vOneTwo0(sLog+0, 3, p+4);
		p += 10;
		// ��������
		strcpy(p, "T002");
		vOneTwo0(sLog+42, 1, p+4);
		p += 6;
		// ����ʱ��
		strcpy(p, "U006");
		vOneTwo0(sLog+3, 3, p+4);
		p += 10;
		// �ն˹��Ҵ���
		strcpy(p, "V004");
		vOneTwo0(sLog+18, 2, p+4);
		p += 8;
		// �̻�����
		strcpy(p, "W020");
		vMemcpy0(szBuf, sLog+22, 20);
		memset(p+4, ' ', 20);
		memcpy(p+4, szBuf, strlen(szBuf));
		p += 24;
		// ATC
		strcpy(p, "X004");
		vOneTwo0(sLog+43, 2, p+4);
		p += 8;

		*pnTxDetailLen += 106;
	} // for(i=1

	// ������д��¼����
	sprintf(szBuf, "%02d", (int)((*pnTxDetailLen - 5) / 106));
	memcpy(TxDetail, szBuf, 2);

	return(0);
}
