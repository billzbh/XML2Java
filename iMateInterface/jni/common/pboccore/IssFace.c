#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include "vposface.h"
#include "IssData.h"
#include "isscmd.h"
#include "issproc.h"
#include "KeyFace.h"
#include "IssFace.h"

//QINGBO
#include "hsmsimu.h"
#include "WriteLog.h"

static void (*sg_pfvShowStatus)(char *pszStatus) = NULL;

//QINGBO
static void vCustomCardAppParam(uchar *psAtr, uint uiAtrLen);

// ���÷���������ʾ����
// in  : pfvShowStatus : ��ʾ����ָ��
int iIssSetStatusShowFunc(void (*pfvShowStatus)(char *))
{
	sg_pfvShowStatus = pfvShowStatus;
	return(0);
}

static void vShowStatus(char *pszFormat, ...)
{
	char szBuf[1024];
    va_list args;

	if(sg_pfvShowStatus) {
		va_start(args, pszFormat);
		vsprintf(szBuf, pszFormat, args);
		sg_pfvShowStatus(szBuf);
	    va_end(args);
	}
}

// ���ö�������, �����ⲿ�ṩIC���ӿ�
// �������: ����4�����������һ������NULL, ���ʾȡ��֮ǰ�Ըÿ������õĺ���
//           pfiTestCard  : ��⿨Ƭ�Ƿ���ں���ָ��
//           pfiResetCard : ��Ƭ��λ����ָ��
//           pfiDoApdu    : ִ��Apduָ���ָ��
//           pfiCloseCard : �رտ�Ƭ����ָ��
int iIssSetCardCtrlFunc(int (*pfiTestCard)(void),
				        int (*pfiResetCard)(uchar *psAtr),
				        int (*pfiDoApdu)(int iApduInLen, uchar *psApduIn, int *piApduOutLen, uchar *psApduOut),
				        int (*pfiCloseCard)(void))
{
	uint uiSlotNo = 7;
    _vPosInit();
	_uiSetCardCtrlFunc(uiSlotNo, pfiTestCard, pfiResetCard, pfiDoApdu,pfiCloseCard);
	uiIssCmdSetSlot(uiSlotNo);
	return(0);
}


// ���÷�������
// in  : iKeyMode  : ��Կ��ȡ��ʽ, 0:�����ڲ� 1:�����ļ� 2:���ܻ�
//       pszHsmIp  : ���ܻ�IP��ַ(�����Կ��ȡ��ʽΪ���ܻ�)
//       iHsmPort  : ���ܻ��˿ں�(�����Կ��ȡ��ʽΪ���ܻ�)
//       iLogFlag  : ������־��¼��־, 0:����¼ 1:��¼
// out : pszErrMsg : ������Ϣ
// ret : 0         : OK
//       1         : Error
int iIssSetEnv(int iKeyMode, char *pszHsmIp, int iHsmPort, int iLogFlag, char *pszErrMsg)
{
	int iRet;
    
	strcpy(pszErrMsg, "");
	vIssSetLog(iLogFlag, 0);
    vIssDataInit(0x20);
    iRet = iKeySetMode(iKeyMode, pszHsmIp, iHsmPort);
    if(iRet) {
        sprintf(pszErrMsg, "���ü��ܻ�ģʽ����[%04X]", iRet);
		return(1);
    }
	return(0);
}

// ���÷�������
// in  : iUsage    : ��;, 0:���ڷ��� 1:����ɾ��Ӧ��
//       pIssData  : ���˻�����
// out : pszErrMsg : ������Ϣ
// ret : 0         : OK
//       1         : Error
int iIssSetData(int iUsage, stIssData *pIssData, char *pszErrMsg)
{
    int  i;
    char szBuf[100];

	if(iUsage == 1) {
	    gl_KeyRef.iKmcIndex = pIssData->iKmcIndex;
		return(0); // ɾ��Ӧ��ֻ��ҪKMC����
	}

    if(strlen(pIssData->szAid)<10 || strlen(pIssData->szAid)>32) {
        strcpy(pszErrMsg, "AID���ȱ������5��16֮��");
        return(1);
    }
    if(strlen(pIssData->szAid) % 2) {
        strcpy(pszErrMsg, "AID�ַ������ݲ���ż��");
        return(1);
    }
    for(i=0; i<(int)strlen(pIssData->szAid); i++) {
        if(!isxdigit(pIssData->szAid[i])) {
            strcpy(pszErrMsg, "AID�ַ�������Ϊ16������");
            return(1);
        }
    }
    if(strlen(pIssData->szLabel)==0 || strlen(pIssData->szLabel)>16) {
        strcpy(pszErrMsg, "����д1-16�ֽڳ���Ӧ�ñ�ǩ");
        return(1);
    }
    if(pIssData->iIcRsaKeyLen<64 || pIssData->iIcRsaKeyLen>192) {
        strcpy(pszErrMsg, "IC��RSA��Կ���ȱ������64~192֮��");
        return(1);
    }
    if(pIssData->iCaIndex<0 || pIssData->iCaIndex>255) {
        strcpy(pszErrMsg, "CA��Կ�����������0~255֮��");
        return(1);
    }
    if(strlen(pIssData->szPan)<12 || strlen(pIssData->szPan)>20) {
        strcpy(pszErrMsg, "PAN���ȱ������12��20֮��");
        return(1);
    }
    for(i=0; i<(int)strlen(pIssData->szPan); i++) {
        if(!isdigit(pIssData->szPan[i])) {
            strcpy(pszErrMsg, "PAN����ȫ��Ϊ����");
            return(1);
        }
    }
	if(memcmp(pIssData->szPan, "88888", 5) != 0) {
        strcpy(pszErrMsg, "PAN������88888��ʼ");
        return(1);
	}
    if(pIssData->iPanSerialNo >= 0) {
		if(pIssData->iPanSerialNo > 99) {
            strcpy(pszErrMsg, "PAN���кű���С��99");
            return(1);
        }
    }
    if(pIssData->iHolderIdType<0 || pIssData->iHolderIdType>5) {
        strcpy(pszErrMsg, "֤�����ͱ������0~5֮��");
        return(1);
    }
	if(strlen(pIssData->szHolderName) > 45) {
        strcpy(pszErrMsg, "�ֿ����������ȱ���С�ڵ���45");
        return(1);
    }
    if(strlen(pIssData->szHolderId) > 40) {
        strcpy(pszErrMsg, "�ֿ���֤�����볤�ȱ���С�ڵ���40");
        return(1);
    }
    if(strlen(pIssData->szExpireDate) != 6) {
        strcpy(pszErrMsg, "��Ч���ڱ���Ϊ6�ֽ�����");
        return(1);
    }
    for(i=0; i<(int)strlen(pIssData->szExpireDate); i++) {
        if(!isdigit(pIssData->szExpireDate[i])) {
            strcpy(pszErrMsg, "��Ч�ڱ���ȫ��Ϊ����");
            return(1);
        }
    }
	if(strlen(pIssData->szDefaultPin)<4 || strlen(pIssData->szDefaultPin)>12) {
        strcpy(pszErrMsg, "ȱʡ���볤�ȱ������4~12֮��");
        return(1);
	}
    for(i=0; i<(int)strlen(pIssData->szDefaultPin); i++) {
        if(!isdigit(pIssData->szDefaultPin[i])) {
            strcpy(pszErrMsg, "ȱʡ�������ȫ��Ϊ����");
            return(1);
        }
    }
    if(pIssData->iCountryCode<1 || pIssData->iCountryCode>999) {
        strcpy(pszErrMsg, "���Ҵ���������1~999֮��");
        return(1);
    }
    if(pIssData->iCurrencyCode<1 || pIssData->iCurrencyCode>999) {
        strcpy(pszErrMsg, "���Ҵ���������1~999֮��");
        return(1);
    }

    // ��������д�����������ݽṹ��
    gl_AppRef.ucECashExistFlag = 1;     // eCash���ڱ�־
    gl_AppRef.ucQpbocExistFlag = 1;     // qPboc���ڱ�־
	vLongToStr(pIssData->iAppVer, 2, gl_AppRef.sAppVerNo); // Ӧ�ð汾��

    gl_KeyRef.iKmcIndex = pIssData->iKmcIndex;
    gl_KeyRef.iAppKeyBaseIndex = pIssData->iAppKeyBaseIndex;
    gl_KeyRef.iIssuerRsaKeyIndex = pIssData->iIssuerRsaKeyIndex;
    
    gl_InstanceRef.ucAidLen = strlen(pIssData->szAid) / 2;
    vTwoOne(pIssData->szAid, gl_InstanceRef.ucAidLen*2, gl_InstanceRef.sAid);
    strcpy((char *)gl_AppRef.szAppLabel, pIssData->szLabel); // T50
    gl_AppRef.ucCaPublicKeyIndex = pIssData->iCaIndex; // T8F
    gl_AppRef.ucIccPkLen = pIssData->iIcRsaKeyLen;
    if(pIssData->lIcRsaE == 3) {
        gl_AppRef.ucIcPublicKeyELen = 1;
        memcpy(gl_AppRef.sIcPublicKeyE, "\x03", gl_AppRef.ucIcPublicKeyELen); // T9F47
    } else {
        gl_AppRef.ucIcPublicKeyELen = 3;
        memcpy(gl_AppRef.sIcPublicKeyE, "\x01\x00\x01", gl_AppRef.ucIcPublicKeyELen); // T9F47
    }
    strcpy(szBuf, pIssData->szPan);
    strcat(szBuf, "FFFFFFFF");
    szBuf[20] = 0;
    vTwoOne((uchar *)szBuf, 20, gl_AppRef.sPAN); // T5A
    if(pIssData->iPanSerialNo >= 0) {
		sprintf(szBuf, "%02d", pIssData->iPanSerialNo);
		vTwoOne(szBuf, 2, &gl_AppRef.ucPANSerialNo); // �����˺����к�(0xFF��ʾ�޴���), T5F34
	} else
        gl_AppRef.ucPANSerialNo = 0xFF; // �����˺����к�(0xFF��ʾ�޴���), T5F34
    strcpy((char *)gl_AppRef.szHolderName, pIssData->szHolderName); // T5F20
    strcpy((char *)gl_AppRef.szHolderId, pIssData->szHolderId); // T9F61
	gl_AppRef.ucHolderIdType = pIssData->iHolderIdType;
    vTwoOne(pIssData->szExpireDate, 6, gl_AppRef.sExpireDate); // T5F24

    strcpy((char *)gl_AppRef.szPin, pIssData->szDefaultPin);  // ��������, 4-12

	sprintf(szBuf, "%04d", pIssData->iCountryCode);
	vTwoOne(szBuf, 4, gl_AppRef.sIssuerCountryCode); // T5F28
	sprintf(szBuf, "%04d", pIssData->iCurrencyCode);
	vTwoOne(szBuf, 4, gl_AppRef.sAppCurrencyCode); // T9F51

    return(0);
}

// ����
// out : pszErrMsg : ������Ϣ
// ret : 0         : OK
//       1         : Error
int iIssCard(char *pszErrMsg)
{
    int  iRet;
    char szErrMsg[120];
    uchar sTermRand[8], sCardData[28];
    //QINGBO
    uchar sAtr[100];

    vShowStatus("����������...");

    // ��λ��Ƭ
    vShowStatus("��λIC��Ƭ");
    iRet = uiIssTestCard();
    if(iRet == 0) {
        sprintf(szErrMsg, "û�ҵ���Ƭ");
        goto label_error;
    }
    /*
	iRet = uiIssCmdResetCard();
	if(iRet) {
        sprintf(szErrMsg, "��Ƭ��λ����");
        goto label_error;
	}
     */
    //QINGBO
    memset(sAtr, 0, sizeof(sAtr));
    iRet = uiIssCmdResetCard(sAtr);
    if(iRet == 0) {
        sprintf(szErrMsg, "��Ƭ��λ����");
        goto label_error;
    }
    //QINGBO
    vCustomCardAppParam(sAtr, iRet);
    
    vWriteLogHex("atr =", sAtr, 10);

    // ����PBOCӦ��
    // ѡ��Card Manager
    vShowStatus("ѡ��Card Manager");

    iRet = iSelectApp(gl_CardRef.ucCardManagerAidLen, gl_CardRef.sCardManagerAid, szErrMsg);
    if(iRet) {
		// try A000000003000000
		vTwoOne("A000000003000000", 16, gl_CardRef.sCardManagerAid);
		gl_CardRef.ucCardManagerAidLen = 8;
	    iRet = iSelectApp(gl_CardRef.ucCardManagerAidLen, gl_CardRef.sCardManagerAid, szErrMsg);
		if(iRet) {
			// try A000000004000000
			vTwoOne("A000000004000000", 16, gl_CardRef.sCardManagerAid);
			gl_CardRef.ucCardManagerAidLen = 8;
			iRet = iSelectApp(gl_CardRef.ucCardManagerAidLen, gl_CardRef.sCardManagerAid, szErrMsg);
			if(iRet) {
		        goto label_error;
			}
		}
	}
    vShowStatus("����Card Manager��ȫͨ��");
    iRet = iEstablishSecureChannel(gl_KeyRef.iKmcIndex, 1/*1:Card Manager*/, sTermRand, sCardData, szErrMsg);
    if(iRet)
        goto label_error;

//    iDeleteApp(6, (uchar *)"\x32\x50\x41\x59\x2E\x53", szErrMsg); // ɾ��2PAY.S, �������2PAY.SYS.DDF01Ӧ��ʧ�ܣ�����
//    iDeleteApp(8, (uchar *)"\xA0\x00\x00\x03\x33\x01\x01\x01", szErrMsg);

    vShowStatus("����PBOCӦ��");
    iDeleteApp(gl_InstanceRef.ucAidLen, gl_InstanceRef.sAid, szErrMsg);
    iRet = iCreatePboc(szErrMsg); // ����PbocӦ��
    if(iRet)
        goto label_error;
    // дPboc����
    vShowStatus("ѡ��PBOCӦ��");
    iRet = iSelectApp(gl_InstanceRef.ucAidLen, gl_InstanceRef.sAid, szErrMsg);
    if(iRet)
        goto label_error;
    vShowStatus("����PBOCӦ�ð�ȫͨ��");
    iRet = iEstablishSecureChannel(gl_KeyRef.iKmcIndex, 0, sTermRand, sCardData, szErrMsg);
    if(iRet)
        goto label_error;
    vShowStatus("���з�������׼��");
    iRet = iCompleteIssData(sTermRand, sCardData, (uchar *)szErrMsg); // ��ɷ�������
    if(iRet)
        goto label_error;
    vShowStatus("��װPBOC����");
    iRet = iStorePbocData(szErrMsg);
    if(iRet)
        goto label_error;

    // ����PSE
    // ѡ��Card Manager
    vShowStatus("ѡ��Card Manager");
    iRet = iSelectApp(gl_CardRef.ucCardManagerAidLen, gl_CardRef.sCardManagerAid, szErrMsg);
    if(iRet)
        goto label_error;
    vShowStatus("����Card Manager��ȫͨ��");
    iRet = iEstablishSecureChannel(gl_KeyRef.iKmcIndex, 1/*1:Card Manager*/, sTermRand, sCardData, szErrMsg);
    if(iRet)
        goto label_error;

    vShowStatus("����PSEӦ��");
    iDeleteApp(14, (uchar *)"1PAY.SYS.DDF01", szErrMsg);
//    iRet = iCreatePboc(szErrMsg); // ����PbocӦ��
    iRet = iCreatePse(0/*0:PSE*/, szErrMsg); // ����PSEӦ��
    if(iRet)
        goto label_error;
    // дPSE����
    vShowStatus("ѡ��PSEӦ��");
    iRet = iSelectApp(0x0E, (uchar *)"1PAY.SYS.DDF01", szErrMsg);
    if(iRet)
        goto label_error;
    vShowStatus("����PSEӦ�ð�ȫͨ��");
    iRet = iEstablishSecureChannel(gl_KeyRef.iKmcIndex, 0, sTermRand, sCardData, szErrMsg);
    if(iRet)
        goto label_error;
    vShowStatus("��װPSE����");
    iRet = iStorePseData(szErrMsg);
    if(iRet)
        goto label_error;

    // ����PPSE
    // ѡ��Card Manager
    vShowStatus("ѡ��Card Manager");
    iRet = iSelectApp(gl_CardRef.ucCardManagerAidLen, gl_CardRef.sCardManagerAid, szErrMsg);
    if(iRet)
        goto label_error;
    vShowStatus("����Card Manager��ȫͨ��");
    iRet = iEstablishSecureChannel(gl_KeyRef.iKmcIndex, 1/*1:Card Manager*/, sTermRand, sCardData, szErrMsg);
    if(iRet)
        goto label_error;

    vShowStatus("����PPSEӦ��");
    iDeleteApp(14, (uchar *)"2PAY.SYS.DDF01", szErrMsg);
    iRet = iCreatePse(1/*0:PPSE*/, szErrMsg); // ����PPSEӦ��
    if(iRet)
        goto label_error;
    // дPPSE����
    vShowStatus("ѡ��PPSEӦ��");
    iRet = iSelectApp(0x0E, (uchar *)"2PAY.SYS.DDF01", szErrMsg);
    if(iRet)
        goto label_error;
    vShowStatus("����PPSEӦ�ð�ȫͨ��");
    iRet = iEstablishSecureChannel(gl_KeyRef.iKmcIndex, 0, sTermRand, sCardData, szErrMsg);
    if(iRet)
        goto label_error;
    vShowStatus("��װPPSE����");
    iRet = iStorePPseData(szErrMsg);
    if(iRet)
        goto label_error;

    vShowStatus("�����ɹ�");
    uiIssCloseCard();
	return(0);
label_error:
	strcpy(pszErrMsg, szErrMsg);
    uiIssCloseCard();
	return(1);
}

// ɾ��Ӧ��
// out : pszErrMsg : ������Ϣ
// ret : 0         : OK
//       1         : Error
int iIssDelApps(char *pszErrMsg)
{
    int   iRet;
    char  szErrMsg[120];
    uchar sTermRand[8], sCardData[28];
	int   iAidListLen;
	uchar sAidList[256], *psAidList;
	int   iAppNum;
    
    //QINGBO
    uchar sAtr[100];

    vShowStatus("ɾ��Ӧ��...");

    // ��λ��Ƭ
    vShowStatus("��λIC��Ƭ");
    iRet = uiIssTestCard();
    if(iRet == 0) {
        sprintf(szErrMsg, "û�ҵ���Ƭ");
        goto label_error;
    }
    /*
     iRet = uiIssCmdResetCard();
     if(iRet) {
     sprintf(szErrMsg, "��Ƭ��λ����");
     goto label_error;
     }
     */
    //QINGBO
    iRet = uiIssCmdResetCard(sAtr);
    if(iRet == 0) {
        sprintf(szErrMsg, "��Ƭ��λ����");
        goto label_error;
    }
    
    //QINGBO
    vCustomCardAppParam(sAtr, iRet);
    
    // ѡ��Card Manager
    vShowStatus("ѡ��Card Manager");

    iRet = iSelectApp(gl_CardRef.ucCardManagerAidLen, gl_CardRef.sCardManagerAid, szErrMsg);
    if(iRet) {
		// try A000000003000000
		vTwoOne((unsigned char *)"A000000003000000", 16, gl_CardRef.sCardManagerAid);
		gl_CardRef.ucCardManagerAidLen = 8;
	    iRet = iSelectApp(gl_CardRef.ucCardManagerAidLen, gl_CardRef.sCardManagerAid, szErrMsg);
		if(iRet) {
			// try A000000004000000
			vTwoOne((unsigned char *)"A000000004000000", 16, gl_CardRef.sCardManagerAid);
			gl_CardRef.ucCardManagerAidLen = 8;
			iRet = iSelectApp(gl_CardRef.ucCardManagerAidLen, gl_CardRef.sCardManagerAid, szErrMsg);
			if(iRet) {
		        goto label_error;
			}
		}
	}
    vShowStatus("����Card Manager��ȫͨ��");
    iRet = iEstablishSecureChannel(gl_KeyRef.iKmcIndex, 1/*1:Card Manager*/, sTermRand, sCardData, szErrMsg);
    if(iRet)
        goto label_error;

	vShowStatus("ɾ��Ӧ��ʵ��");
	iRet = iGetAppList(&iAidListLen, sAidList, szErrMsg);
	iAppNum = 0;
	if(iRet == 0) {
		psAidList = &sAidList[0];
		while(iAidListLen > 1) {
			iDeleteApp(psAidList[0], psAidList+1, szErrMsg);
			iAidListLen -= 1+psAidList[0]+2;
			psAidList += 1+psAidList[0]+2;
			iAppNum ++;
		}
	}

	sprintf(szErrMsg, "ɾ����%d��Ӧ��", iAppNum);
    vShowStatus(szErrMsg);
    uiIssCloseCard();
	return(0);
label_error:
    vShowStatus(szErrMsg);
    uiIssCloseCard();
	return(1);
}

//QINGBO
static void vCustomCardAppParam(uchar *psAtr, uint uiAtrLen)
{
    //QINGBO
    // ����ڶ�����
    if (uiAtrLen > 5 && memcmp(psAtr + 3, "\x00\x86", 2) == 0) {
        gl_CardRef.ucKmcKeyDivFlag = 0;
        gl_CardRef.ucCardManagerAidLen = 8;
        vTwoOne("A000000003000000", 16, gl_CardRef.sCardManagerAid);
        
        vKeyCalSetDivMode(0);
        vHsmChangeKmcKey("\x40\x41\x42\x43\x44\x45\x46\x47\x48\x49\x4A\x4B\x4C\x4D\x4E\x4F");
        
        //PSE
        vIssProcSetAppParam(0, "315041592E", "315041592E5359532E4444463031", "C900");
        //PPSE
        vIssProcSetAppParam(1, "315041592E", "315041592E5359532E4444463031", "C900");
        //PBOC
        vIssProcSetAppParam(2, "A000000333010130", "A0000003330101", "C90120");
    }
    // ��Ϊ��ʾ��
    else if (uiAtrLen > 5 && memcmp(psAtr + 3, "\x00\xff", 2) == 0) {
        gl_CardRef.ucKmcKeyDivFlag = 0;
        gl_CardRef.ucCardManagerAidLen = 0;
        
        gl_InstanceRef.ucApplicationPrivilegesLen = 1;	// Application Privileges����
        memcpy(gl_InstanceRef.sApplicationPrivileges, "\x02", gl_InstanceRef.ucApplicationPrivilegesLen);
        
        vKeyCalSetDivMode(0);
        vHsmChangeKmcKey("\x40\x41\x42\x43\x44\x45\x46\x47\x48\x49\x4A\x4B\x4C\x4D\x4E\x4F");
        
        //PSE
        vIssProcSetAppParam(0, "50424f435f444343", "50424f435f4443435f3031", "C9020260");
        //PPSE
        vIssProcSetAppParam(1, "50424f435f444343", "50424f435f4443435f3031", "C900");
        //PBOC
        vIssProcSetAppParam(2, "50424F435F444343", "50424F435F4443435F3031", "C900");
    }
    else {
        // �����һ����
        gl_CardRef.ucKmcKeyDivFlag = 0;
        gl_CardRef.ucCardManagerAidLen = 8;
        vTwoOne("A000000004000000", 16, gl_CardRef.sCardManagerAid);
        
        vKeyCalSetDivMode(1);
        vHsmChangeKmcKey("\x46\x45\x4C\x58\x51\x52\x45\x52\x52\x4F\x52\x40\x4C\x51\x4C\x45");
        
        //PSE
        vIssProcSetAppParam(0, "A00000001830070100000000000001FF", "A00000001830070100000000000001", "C9020260");
        //PPSE
        vIssProcSetAppParam(1, "A00000001830070100000000000001FF", "A00000001830070100000000000001", "C900");
        //PBOC
        vIssProcSetAppParam(2, "A000000333010130", "A0000003330101", "C90CFFFF00000400000000010001");
    }
}
