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

// 设置发卡过程显示函数
// in  : pfvShowStatus : 显示函数指针
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

// 设置读卡函数, 用于外部提供IC卡接口
// 输入参数: 以下4个函数如果有一个传入NULL, 则表示取消之前对该卡座设置的函数
//           pfiTestCard  : 检测卡片是否存在函数指针
//           pfiResetCard : 卡片复位函数指针
//           pfiDoApdu    : 执行Apdu指令函数指针
//           pfiCloseCard : 关闭卡片函数指针
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


// 设置发卡环境
// in  : iKeyMode  : 密钥获取方式, 0:程序内部 1:配置文件 2:加密机
//       pszHsmIp  : 加密机IP地址(如果密钥获取方式为加密机)
//       iHsmPort  : 加密机端口号(如果密钥获取方式为加密机)
//       iLogFlag  : 发卡日志记录标志, 0:不记录 1:记录
// out : pszErrMsg : 错误信息
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
        sprintf(pszErrMsg, "设置加密机模式出错[%04X]", iRet);
		return(1);
    }
	return(0);
}

// 设置发卡数据
// in  : iUsage    : 用途, 0:用于发卡 1:用于删除应用
//       pIssData  : 个人化数据
// out : pszErrMsg : 错误信息
// ret : 0         : OK
//       1         : Error
int iIssSetData(int iUsage, stIssData *pIssData, char *pszErrMsg)
{
    int  i;
    char szBuf[100];

	if(iUsage == 1) {
	    gl_KeyRef.iKmcIndex = pIssData->iKmcIndex;
		return(0); // 删除应用只需要KMC索引
	}

    if(strlen(pIssData->szAid)<10 || strlen(pIssData->szAid)>32) {
        strcpy(pszErrMsg, "AID长度必须介于5与16之间");
        return(1);
    }
    if(strlen(pIssData->szAid) % 2) {
        strcpy(pszErrMsg, "AID字符串内容不是偶数");
        return(1);
    }
    for(i=0; i<(int)strlen(pIssData->szAid); i++) {
        if(!isxdigit(pIssData->szAid[i])) {
            strcpy(pszErrMsg, "AID字符串必须为16进制数");
            return(1);
        }
    }
    if(strlen(pIssData->szLabel)==0 || strlen(pIssData->szLabel)>16) {
        strcpy(pszErrMsg, "请填写1-16字节长的应用标签");
        return(1);
    }
    if(pIssData->iIcRsaKeyLen<64 || pIssData->iIcRsaKeyLen>192) {
        strcpy(pszErrMsg, "IC卡RSA密钥长度必须介于64~192之间");
        return(1);
    }
    if(pIssData->iCaIndex<0 || pIssData->iCaIndex>255) {
        strcpy(pszErrMsg, "CA公钥索引必须介于0~255之间");
        return(1);
    }
    if(strlen(pIssData->szPan)<12 || strlen(pIssData->szPan)>20) {
        strcpy(pszErrMsg, "PAN长度必须介于12与20之间");
        return(1);
    }
    for(i=0; i<(int)strlen(pIssData->szPan); i++) {
        if(!isdigit(pIssData->szPan[i])) {
            strcpy(pszErrMsg, "PAN必须全部为数字");
            return(1);
        }
    }
	if(memcmp(pIssData->szPan, "88888", 5) != 0) {
        strcpy(pszErrMsg, "PAN必须以88888开始");
        return(1);
	}
    if(pIssData->iPanSerialNo >= 0) {
		if(pIssData->iPanSerialNo > 99) {
            strcpy(pszErrMsg, "PAN序列号必须小于99");
            return(1);
        }
    }
    if(pIssData->iHolderIdType<0 || pIssData->iHolderIdType>5) {
        strcpy(pszErrMsg, "证件类型必须介于0~5之间");
        return(1);
    }
	if(strlen(pIssData->szHolderName) > 45) {
        strcpy(pszErrMsg, "持卡人姓名长度必须小于等于45");
        return(1);
    }
    if(strlen(pIssData->szHolderId) > 40) {
        strcpy(pszErrMsg, "持卡人证件号码长度必须小于等于40");
        return(1);
    }
    if(strlen(pIssData->szExpireDate) != 6) {
        strcpy(pszErrMsg, "有效日期必须为6字节数字");
        return(1);
    }
    for(i=0; i<(int)strlen(pIssData->szExpireDate); i++) {
        if(!isdigit(pIssData->szExpireDate[i])) {
            strcpy(pszErrMsg, "有效期必须全部为数字");
            return(1);
        }
    }
	if(strlen(pIssData->szDefaultPin)<4 || strlen(pIssData->szDefaultPin)>12) {
        strcpy(pszErrMsg, "缺省密码长度必须介于4~12之间");
        return(1);
	}
    for(i=0; i<(int)strlen(pIssData->szDefaultPin); i++) {
        if(!isdigit(pIssData->szDefaultPin[i])) {
            strcpy(pszErrMsg, "缺省密码必须全部为数字");
            return(1);
        }
    }
    if(pIssData->iCountryCode<1 || pIssData->iCountryCode>999) {
        strcpy(pszErrMsg, "国家代码必须介于1~999之间");
        return(1);
    }
    if(pIssData->iCurrencyCode<1 || pIssData->iCurrencyCode>999) {
        strcpy(pszErrMsg, "货币代码必须介于1~999之间");
        return(1);
    }

    // 将参数填写到到发卡数据结构中
    gl_AppRef.ucECashExistFlag = 1;     // eCash存在标志
    gl_AppRef.ucQpbocExistFlag = 1;     // qPboc存在标志
	vLongToStr(pIssData->iAppVer, 2, gl_AppRef.sAppVerNo); // 应用版本号

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
		vTwoOne(szBuf, 2, &gl_AppRef.ucPANSerialNo); // 个人账号序列号(0xFF表示无此项), T5F34
	} else
        gl_AppRef.ucPANSerialNo = 0xFF; // 个人账号序列号(0xFF表示无此项), T5F34
    strcpy((char *)gl_AppRef.szHolderName, pIssData->szHolderName); // T5F20
    strcpy((char *)gl_AppRef.szHolderId, pIssData->szHolderId); // T9F61
	gl_AppRef.ucHolderIdType = pIssData->iHolderIdType;
    vTwoOne(pIssData->szExpireDate, 6, gl_AppRef.sExpireDate); // T5F24

    strcpy((char *)gl_AppRef.szPin, pIssData->szDefaultPin);  // 个人密码, 4-12

	sprintf(szBuf, "%04d", pIssData->iCountryCode);
	vTwoOne(szBuf, 4, gl_AppRef.sIssuerCountryCode); // T5F28
	sprintf(szBuf, "%04d", pIssData->iCurrencyCode);
	vTwoOne(szBuf, 4, gl_AppRef.sAppCurrencyCode); // T9F51

    return(0);
}

// 发卡
// out : pszErrMsg : 错误信息
// ret : 0         : OK
//       1         : Error
int iIssCard(char *pszErrMsg)
{
    int  iRet;
    char szErrMsg[120];
    uchar sTermRand[8], sCardData[28];
    //QINGBO
    uchar sAtr[100];

    vShowStatus("发卡过程中...");

    // 复位卡片
    vShowStatus("复位IC卡片");
    iRet = uiIssTestCard();
    if(iRet == 0) {
        sprintf(szErrMsg, "没找到卡片");
        goto label_error;
    }
    /*
	iRet = uiIssCmdResetCard();
	if(iRet) {
        sprintf(szErrMsg, "卡片复位错误");
        goto label_error;
	}
     */
    //QINGBO
    memset(sAtr, 0, sizeof(sAtr));
    iRet = uiIssCmdResetCard(sAtr);
    if(iRet == 0) {
        sprintf(szErrMsg, "卡片复位错误");
        goto label_error;
    }
    //QINGBO
    vCustomCardAppParam(sAtr, iRet);
    
    vWriteLogHex("atr =", sAtr, 10);

    // 建立PBOC应用
    // 选择Card Manager
    vShowStatus("选择Card Manager");

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
    vShowStatus("建立Card Manager安全通道");
    iRet = iEstablishSecureChannel(gl_KeyRef.iKmcIndex, 1/*1:Card Manager*/, sTermRand, sCardData, szErrMsg);
    if(iRet)
        goto label_error;

//    iDeleteApp(6, (uchar *)"\x32\x50\x41\x59\x2E\x53", szErrMsg); // 删除2PAY.S, 如果建立2PAY.SYS.DDF01应用失败，试试
//    iDeleteApp(8, (uchar *)"\xA0\x00\x00\x03\x33\x01\x01\x01", szErrMsg);

    vShowStatus("建立PBOC应用");
    iDeleteApp(gl_InstanceRef.ucAidLen, gl_InstanceRef.sAid, szErrMsg);
    iRet = iCreatePboc(szErrMsg); // 建立Pboc应用
    if(iRet)
        goto label_error;
    // 写Pboc数据
    vShowStatus("选择PBOC应用");
    iRet = iSelectApp(gl_InstanceRef.ucAidLen, gl_InstanceRef.sAid, szErrMsg);
    if(iRet)
        goto label_error;
    vShowStatus("建立PBOC应用安全通道");
    iRet = iEstablishSecureChannel(gl_KeyRef.iKmcIndex, 0, sTermRand, sCardData, szErrMsg);
    if(iRet)
        goto label_error;
    vShowStatus("进行发卡数据准备");
    iRet = iCompleteIssData(sTermRand, sCardData, (uchar *)szErrMsg); // 完成发卡数据
    if(iRet)
        goto label_error;
    vShowStatus("安装PBOC数据");
    iRet = iStorePbocData(szErrMsg);
    if(iRet)
        goto label_error;

    // 建立PSE
    // 选择Card Manager
    vShowStatus("选择Card Manager");
    iRet = iSelectApp(gl_CardRef.ucCardManagerAidLen, gl_CardRef.sCardManagerAid, szErrMsg);
    if(iRet)
        goto label_error;
    vShowStatus("建立Card Manager安全通道");
    iRet = iEstablishSecureChannel(gl_KeyRef.iKmcIndex, 1/*1:Card Manager*/, sTermRand, sCardData, szErrMsg);
    if(iRet)
        goto label_error;

    vShowStatus("建立PSE应用");
    iDeleteApp(14, (uchar *)"1PAY.SYS.DDF01", szErrMsg);
//    iRet = iCreatePboc(szErrMsg); // 建立Pboc应用
    iRet = iCreatePse(0/*0:PSE*/, szErrMsg); // 建立PSE应用
    if(iRet)
        goto label_error;
    // 写PSE数据
    vShowStatus("选择PSE应用");
    iRet = iSelectApp(0x0E, (uchar *)"1PAY.SYS.DDF01", szErrMsg);
    if(iRet)
        goto label_error;
    vShowStatus("建立PSE应用安全通道");
    iRet = iEstablishSecureChannel(gl_KeyRef.iKmcIndex, 0, sTermRand, sCardData, szErrMsg);
    if(iRet)
        goto label_error;
    vShowStatus("安装PSE数据");
    iRet = iStorePseData(szErrMsg);
    if(iRet)
        goto label_error;

    // 建立PPSE
    // 选择Card Manager
    vShowStatus("选择Card Manager");
    iRet = iSelectApp(gl_CardRef.ucCardManagerAidLen, gl_CardRef.sCardManagerAid, szErrMsg);
    if(iRet)
        goto label_error;
    vShowStatus("建立Card Manager安全通道");
    iRet = iEstablishSecureChannel(gl_KeyRef.iKmcIndex, 1/*1:Card Manager*/, sTermRand, sCardData, szErrMsg);
    if(iRet)
        goto label_error;

    vShowStatus("建立PPSE应用");
    iDeleteApp(14, (uchar *)"2PAY.SYS.DDF01", szErrMsg);
    iRet = iCreatePse(1/*0:PPSE*/, szErrMsg); // 建立PPSE应用
    if(iRet)
        goto label_error;
    // 写PPSE数据
    vShowStatus("选择PPSE应用");
    iRet = iSelectApp(0x0E, (uchar *)"2PAY.SYS.DDF01", szErrMsg);
    if(iRet)
        goto label_error;
    vShowStatus("建立PPSE应用安全通道");
    iRet = iEstablishSecureChannel(gl_KeyRef.iKmcIndex, 0, sTermRand, sCardData, szErrMsg);
    if(iRet)
        goto label_error;
    vShowStatus("安装PPSE数据");
    iRet = iStorePPseData(szErrMsg);
    if(iRet)
        goto label_error;

    vShowStatus("发卡成功");
    uiIssCloseCard();
	return(0);
label_error:
	strcpy(pszErrMsg, szErrMsg);
    uiIssCloseCard();
	return(1);
}

// 删除应用
// out : pszErrMsg : 错误信息
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

    vShowStatus("删除应用...");

    // 复位卡片
    vShowStatus("复位IC卡片");
    iRet = uiIssTestCard();
    if(iRet == 0) {
        sprintf(szErrMsg, "没找到卡片");
        goto label_error;
    }
    /*
     iRet = uiIssCmdResetCard();
     if(iRet) {
     sprintf(szErrMsg, "卡片复位错误");
     goto label_error;
     }
     */
    //QINGBO
    iRet = uiIssCmdResetCard(sAtr);
    if(iRet == 0) {
        sprintf(szErrMsg, "卡片复位错误");
        goto label_error;
    }
    
    //QINGBO
    vCustomCardAppParam(sAtr, iRet);
    
    // 选择Card Manager
    vShowStatus("选择Card Manager");

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
    vShowStatus("建立Card Manager安全通道");
    iRet = iEstablishSecureChannel(gl_KeyRef.iKmcIndex, 1/*1:Card Manager*/, sTermRand, sCardData, szErrMsg);
    if(iRet)
        goto label_error;

	vShowStatus("删除应用实例");
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

	sprintf(szErrMsg, "删除了%d个应用", iAppNum);
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
    // 金邦达第二批卡
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
    // 华为演示卡
    else if (uiAtrLen > 5 && memcmp(psAtr + 3, "\x00\xff", 2) == 0) {
        gl_CardRef.ucKmcKeyDivFlag = 0;
        gl_CardRef.ucCardManagerAidLen = 0;
        
        gl_InstanceRef.ucApplicationPrivilegesLen = 1;	// Application Privileges长度
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
        // 金邦达第一批卡
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
