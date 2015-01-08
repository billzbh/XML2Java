/**************************************
File name     : IssProc.C
Function      : Issue EMV4.1 card
Author        : Yu Jun
First edition : Aug 21st, 2008
Note          :
**************************************/
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "VposFace.h"
#include "TlvFunc.h"
#include "isscmd.h"
#include "IssData.h"
#include "keyface.h"
#include "HsmProc.h"
#include "MakeDgi.h"
#include "issproc.h"

#include "WriteLog.h"

//QINGBO
typedef struct {
    uchar sPackageAid[16];
    uchar ucPackageAidLen;
    uchar sAppletAid[16];
    uchar ucAppletAidLen;
    uchar sC9Object[16];
    uchar ucC9ObjectLen;
}stCardAppParam;

stCardAppParam sg_CardAppParam[3] = {
    //PSE
    "\xA0\x00\x00\x00\x18\x30\x07\x01\x00\x00\x00\x00\x00\x00\x01\xFF", 16,
    "\xA0\x00\x00\x00\x18\x30\x07\x01\x00\x00\x00\x00\x00\x00\x01", 15,
    "\xC9\x02\x02\x60", 4,
    
    //PPSE
    "\xA0\x00\x00\x00\x18\x30\x07\x01\x00\x00\x00\x00\x00\x00\x01\xFF", 16,
    "\xA0\x00\x00\x00\x18\x30\x07\x01\x00\x00\x00\x00\x00\x00\x01", 15,
    "\xC9\x00", 2,
    
    //PBOC
    "\xA0\x00\x00\x03\x33\x01\x01\x30", 8,
    "\xA0\x00\x00\x03\x33\x01\x01", 7,
    "\xC9\x0C\xFF\xFF\x00\x00\x04\x00\x00\x00\x00\x01\x00\x01", 14
};

// 选择应用
// in  : iAidLen   : AID长度
//       psAid     : AID
// out : pszErrMsg : 错误信息
// ret : 0         : OK
//       1         : refer to pszErrMsg
int iSelectApp(int iAidLen, uchar *psAid, char *pszErrMsg)
{
    uint uiRet;
	uiRet = uiIssCmdSelect(0x00, (uchar)iAidLen, psAid, NULL, NULL);
    if(uiRet) {
        sprintf(pszErrMsg, "选择Card Manager出错[%04X]", uiRet);
        return(1);
    }
    return(0);
}

// 删除应用
// in  : iAidLen   : AID长度
//       psAid     : AID
// out : pszErrMsg : 错误信息
// ret : 0         : OK
//       1         : refer to pszErrMsg
int iDeleteApp(int iAidLen, uchar *psAid, char *pszErrMsg)
{
    uint uiRet;
	uiRet = uiIssCmdClearFile((uchar)iAidLen, psAid);
    if(uiRet) {
        sprintf(pszErrMsg, "删除应用出错[%04X]", uiRet);
        return(1);
    }
    return(0);
}

// 获取当前卡片instance列表
// out : piAidListLen : Aid列表长度
//       psAidList    : Aid列表
// ret : 0            : OK
//       1         : refer to pszErrMsg
int iGetAppList(int *piAidListLen, uchar *psAidList, char *pszErrMsg)
{
    uint  uiRet;
	uchar ucLengthOut;
	uiRet = uiIssGetStatus(0x40, 0x00, 2, "\x4F\x00", &ucLengthOut, psAidList);
    if(uiRet) {
        sprintf(pszErrMsg, "获取应用列表出错[%04X]", uiRet);
        return(1);
    }
	*piAidListLen = ucLengthOut;
    return(0);
}

// 建立安全通道
// in  : iKeyIndex    : 密钥在加密机中索引
//       iCardMgrFlag : Card Manager标志, 1:是Card Manager 0:不是Card Manager
// out : pTermRand    : 终端随机数[8]
//       pCardData    : 卡片数据[28]
//       pszErrMsg    : 错误信息
// ret : 0            : OK
//       1            : refer to pszErrMsg
int iEstablishSecureChannel(int iKeyIndex, int iCardMgrFlag, void *pTermRand, void *pCardData, char *pszErrMsg)
{
    uchar sTermRand[8], sCardData[28], sAuthData[16];
    uint  uiRet;
    int   iRet;
	int   i;

	for(i=0; i<8; i++)
		sTermRand[i] = _ucGetRand();
    if(iKeyIndex == gl_KeyRef.iKmcIndex)
       	uiRet = uiIssCmdInitialize(gl_CardRef.ucKmcKeyVer, gl_CardRef.ucKmcKeyIndex, sTermRand, sCardData);
    else
       	uiRet = uiIssCmdInitialize(0, 0, sTermRand, sCardData);
    if(uiRet) {
        sprintf(pszErrMsg, "建立安全通道IC卡初始化错误[%04X]", uiRet);
		return(1);
    }
    iRet = iKeyCalExtAuth(iKeyIndex, 0, NULL, 0, NULL, sTermRand, sCardData, sAuthData);
    if(iRet) {
        sprintf(pszErrMsg, "建立安全通道加密机计算外部认证数据错误[%04X]", iRet);
		return(1);
    }
	uiRet = uiIssCmdExternalAuth(0x84, sAuthData);
    if(uiRet) {
        sprintf(pszErrMsg, "建立安全通道外部认证数据错误[%04X]", uiRet);
		return(1);
    }
    memcpy(pTermRand, sTermRand, 8);
    memcpy(pCardData, sCardData, 28);
    return(0);
}

// 建立PSE/PPSE
// in  : iPPseFlag : 0 : 建立PSE
//                   1 : 建立PPSE
// out : pszErrMsg : err message
// ret : 0         : OK
//       1         : refer to pszErrMsg
int iCreatePse(int iPPseFlag, char *pszErrMsg)
{
	uchar ucLength;
	uchar sBuf[256];
    uint  uiRet;
    
    //QINGBO
    /*
    ucLength = 0;
    sBuf[ucLength++] = 0x10; // length of package AID
    memcpy(sBuf+ucLength, "\xA0\x00\x00\x00\x18\x30\x07\x01\x00\x00\x00\x00\x00\x00\x01\xFF", 0x10); // package AID
    ucLength += 0x10;
    sBuf[ucLength++] = 0x0F;  // length of applet AID
    memcpy(sBuf+ucLength, "\xA0\x00\x00\x00\x18\x30\x07\x01\x00\x00\x00\x00\x00\x00\x01", 0x0F);  // applet AID
    ucLength += 0x0F;
    sBuf[ucLength++] = 0x0E;            // length of instance AID
    if(iPPseFlag == 0)
        memcpy(sBuf+ucLength, "1PAY.SYS.DDF01", 0x0E);  // instance AID, PSE
    else
        memcpy(sBuf+ucLength, "2PAY.SYS.DDF01", 0x0E);  // instance AID, PPSE
    ucLength += 0x0E;
    sBuf[ucLength++] = 0x01; // length of application privileges
    memcpy(sBuf+ucLength, "\x00", 0x01); // application privileges
    ucLength += 0x01;
    // 打包"Application specific parameters", TC9
    if(iPPseFlag == 0) {
        // PSE
        memcpy(sBuf+ucLength, "\x04\xC9\x02\x02\x60", 5); // C9对象长度[1]+TagC9[1]+LenC9[1]+C9内容[0]
        ucLength += 5;
    } else {
        // PPSE
        memcpy(sBuf+ucLength, "\x02\xC9\x00", 3); // C9对象长度[1]+TagC9[1]+LenC9[1]+C9内容[0]
        ucLength += 3;
    }
    */
    
    ucLength = 0;
    if(iPPseFlag == 0) { // PSE
        sBuf[ucLength++] = sg_CardAppParam[0].ucPackageAidLen; // length of package AID
        memcpy(sBuf+ucLength, sg_CardAppParam[0].sPackageAid, sg_CardAppParam[0].ucPackageAidLen); // package AID
        ucLength += sg_CardAppParam[0].ucPackageAidLen;
        sBuf[ucLength++] = sg_CardAppParam[0].ucAppletAidLen;  // length of applet AID
        memcpy(sBuf+ucLength, sg_CardAppParam[0].sAppletAid, sg_CardAppParam[0].ucAppletAidLen);  // applet AID
        ucLength += sg_CardAppParam[0].ucAppletAidLen;
        sBuf[ucLength++] = 0x0E;            // length of instance AID
        memcpy(sBuf+ucLength, "1PAY.SYS.DDF01", 0x0E);  // instance AID, PSE
        ucLength += 0x0E;
        sBuf[ucLength++] = 0x01; // length of application privileges
        memcpy(sBuf+ucLength, "\x00", 0x01); // application privileges
        ucLength += 0x01;
        // 打包"Application specific parameters", TC9
        sBuf[ucLength++] = sg_CardAppParam[0].ucC9ObjectLen;
        memcpy(sBuf+ucLength, sg_CardAppParam[0].sC9Object, sg_CardAppParam[0].ucC9ObjectLen); // C9对象长度[1]+TagC9[1]+LenC9[1]+C9内容[0]
        ucLength += sg_CardAppParam[0].ucC9ObjectLen;
    }
    else {
        sBuf[ucLength++] = sg_CardAppParam[1].ucPackageAidLen; // length of package AID
        memcpy(sBuf+ucLength, sg_CardAppParam[1].sPackageAid, sg_CardAppParam[1].ucPackageAidLen); // package AID
        ucLength += sg_CardAppParam[1].ucPackageAidLen;
        sBuf[ucLength++] = sg_CardAppParam[1].ucAppletAidLen;  // length of applet AID
        memcpy(sBuf+ucLength, sg_CardAppParam[1].sAppletAid, sg_CardAppParam[1].ucAppletAidLen);  // applet AID
        ucLength += sg_CardAppParam[1].ucAppletAidLen;
        sBuf[ucLength++] = 0x0E;            // length of instance AID
        memcpy(sBuf+ucLength, "2PAY.SYS.DDF01", 0x0E);  // instance AID, PPSE
        ucLength += 0x0E;
        sBuf[ucLength++] = 0x01; // length of application privileges
        memcpy(sBuf+ucLength, "\x00", 0x01); // application privileges
        ucLength += 0x01;
        // 打包"Application specific parameters", TC9
        sBuf[ucLength++] = sg_CardAppParam[1].ucC9ObjectLen;
        memcpy(sBuf+ucLength, sg_CardAppParam[1].sC9Object, sg_CardAppParam[1].ucC9ObjectLen); // C9对象长度[1]+TagC9[1]+LenC9[1]+C9内容[0]
        ucLength += sg_CardAppParam[1].ucC9ObjectLen;
    }
	sBuf[ucLength++] = 0x00; // length of install token

    // install for inatall
	// 80E60C00
	uiRet = uiIssCmdInstall(0x80, 0x0C, ucLength, sBuf);
    if(uiRet) {
        sprintf(pszErrMsg, "建立PSE/PPSE错误[%04X]", uiRet);
		return(1);
    }
    return(0);
}

// 建立Pboc2.0应用
// out : pszErrMsg : err message
// ret : 0         : OK
//       1         : refer to pszErrMsg
int iCreatePboc(char *pszErrMsg)
{
	uchar ucLength;
	uchar sBuf[256];
    uint  uiRet;

    //QINGBO
    /*
	ucLength = 0;
	sBuf[ucLength++] = gl_InstanceRef.ucJavaPackageAidLen; // length of package AID
	memcpy(sBuf+ucLength, gl_InstanceRef.sJavaPackageAid, gl_InstanceRef.ucJavaPackageAidLen); // package AID
	ucLength += gl_InstanceRef.ucJavaPackageAidLen;
	sBuf[ucLength++] = gl_InstanceRef.ucJavaAppletAidLen;  // length of applet AID
	memcpy(sBuf+ucLength, gl_InstanceRef.sJavaAppletAid, gl_InstanceRef.ucJavaAppletAidLen);  // applet AID
	ucLength += gl_InstanceRef.ucJavaAppletAidLen;
	sBuf[ucLength++] = gl_InstanceRef.ucAidLen;            // length of instance AID
	memcpy(sBuf+ucLength, gl_InstanceRef.sAid, gl_InstanceRef.ucAidLen);            // instance AID
	ucLength += gl_InstanceRef.ucAidLen;
	sBuf[ucLength++] = gl_InstanceRef.ucApplicationPrivilegesLen; // length of application privileges
	memcpy(sBuf+ucLength, gl_InstanceRef.sApplicationPrivileges, gl_InstanceRef.ucApplicationPrivilegesLen); // application privileges
	ucLength += gl_InstanceRef.ucApplicationPrivilegesLen;
	// 打包"Application specific parameters", TC9
#if 0
    memcpy(sBuf+ucLength, "\x02\xC9\x00", 3); // C9对象长度[1]+TagC9[1]+LenC9[1]+C9内容[0]
    ucLength += 3;
#else
    memcpy(sBuf+ucLength, "\x0A\xC9\x08\xFF\xFF\x00\x00\x04\x00\x00\x01", 11); // C9对象长度[1]+TagC9[1]+LenC9[1]+C9内容[8]
    memcpy(sBuf+ucLength, "\x0E\xC9\x0C\xFF\xFF\x00\x00\x04\x00\x00\x00\x00\x01\x00\x01", 15); // C9对象长度[1]+TagC9[1]+LenC9[1]+C9内容[8]
    ucLength += 15; // wlfxy
#endif
     */
    ucLength = 0;
    sBuf[ucLength++] = sg_CardAppParam[2].ucPackageAidLen; // length of package AID
    memcpy(sBuf+ucLength, sg_CardAppParam[2].sPackageAid, sg_CardAppParam[2].ucPackageAidLen); // package AID
    ucLength += sg_CardAppParam[2].ucPackageAidLen;
    sBuf[ucLength++] = sg_CardAppParam[2].ucAppletAidLen;  // length of applet AID
    memcpy(sBuf+ucLength, sg_CardAppParam[2].sAppletAid, sg_CardAppParam[2].ucAppletAidLen);  // applet AID
    ucLength += sg_CardAppParam[2].ucAppletAidLen;
    sBuf[ucLength++] = gl_InstanceRef.ucAidLen;            // length of instance AID
    memcpy(sBuf+ucLength, gl_InstanceRef.sAid, gl_InstanceRef.ucAidLen);            // instance AID
    ucLength += gl_InstanceRef.ucAidLen;
    sBuf[ucLength++] = gl_InstanceRef.ucApplicationPrivilegesLen; // length of application privileges
    memcpy(sBuf+ucLength, gl_InstanceRef.sApplicationPrivileges, gl_InstanceRef.ucApplicationPrivilegesLen); // application privileges
    ucLength += gl_InstanceRef.ucApplicationPrivilegesLen;
    // 打包"Application specific parameters", TC9
    sBuf[ucLength++] = sg_CardAppParam[2].ucC9ObjectLen;
    memcpy(sBuf+ucLength, sg_CardAppParam[2].sC9Object, sg_CardAppParam[2].ucC9ObjectLen); // C9对象长度[1]+TagC9[1]+LenC9[1]+C9内容[8]
    ucLength += sg_CardAppParam[2].ucC9ObjectLen; // wlfxy

	sBuf[ucLength++] = 0x00; // length of install token
    
	uiRet = uiIssCmdInstall(0x80, 0x0C, ucLength, sBuf);
    if(uiRet) {
        sprintf(pszErrMsg, "建立PBOC应用错误[%04X]", uiRet);
		return(1);
    }
    return(0);
}

// 装载PSE数据
// out : pszErrMsg : err message
// ret : 0         : OK
//       1         : refer to pszErrMsg
int iStorePseData(char *pszErrMsg)
{
	uint  uiRet;
	uchar ucLength;
	uchar sBuf[256];
	uchar ucIndex;

	ucIndex = 0; // store data指令序号
	// store sfi:1 rec:1
	ucLength = 0;
	memcpy(sBuf+ucLength, "\x01\x01", 2); // dgi
	ucLength += 2;
	sBuf[ucLength++] = 0x00; // dgi长度，先空下，稍后再添
	sBuf[ucLength++] = 0x70; // T70
	sBuf[ucLength++] = 0x00; // 先空下,稍后再填
	sBuf[ucLength++] = 0x61; // T61
	sBuf[ucLength++] = 0x00; // 先空下,稍后再填
	sBuf[ucLength++] = 0x4F; // T4F, AID
	sBuf[ucLength++] = gl_InstanceRef.ucAidLen; // Length
	memcpy(sBuf+ucLength, gl_InstanceRef.sAid, gl_InstanceRef.ucAidLen);
	ucLength += gl_InstanceRef.ucAidLen;
	sBuf[ucLength++] = 0x50; // T50, Label
	sBuf[ucLength++] = strlen(gl_AppRef.szAppLabel); // Length
	strcpy(sBuf+ucLength, gl_AppRef.szAppLabel);
	ucLength += strlen(gl_AppRef.szAppLabel);
	if(gl_AppRef.ucPriority != 0xFF) {
    	sBuf[ucLength++] = 0x87; // T87, Priority
    	sBuf[ucLength++] = 0x01; // Length
    	sBuf[ucLength++] = gl_AppRef.ucPriority;
	}
	sBuf[2] = ucLength - 3;
	sBuf[4] = ucLength - 5;
	sBuf[6] = ucLength - 7;
	uiRet = uiIssCmdStoreData(0x00, ucIndex++, ucLength, sBuf);
    if(uiRet) {
        sprintf(pszErrMsg, "装载PSE数据分组0101出错[%04X]", uiRet);
		return(1);
    }
	// store fci of pse
	ucLength = 0;
	memcpy(sBuf+ucLength, "\x91\x02", 2); // dgi
	ucLength += 2;
	sBuf[ucLength++] = 0x00; // dgi长度，先空下，稍后再添
   	sBuf[ucLength++] = 0xA5; // TA5
	sBuf[ucLength++] = 0x00; // 先空下,稍后再填
	memcpy(sBuf+ucLength, "\x88\x01\x01", 3); // T88, sfi of entrys
	ucLength += 3;
	memcpy(sBuf+ucLength, "\x5F\x2D", 2); // T5F2D, 语言优选项
	ucLength += 2;
	sBuf[ucLength++] = strlen(gl_AppRef.szLanguage);
	memcpy(sBuf+ucLength, gl_AppRef.szLanguage, strlen(gl_AppRef.szLanguage));
	ucLength += strlen(gl_AppRef.szLanguage);
	sBuf[2] = ucLength - 3;
	sBuf[4] = ucLength - 5;
	uiRet = uiIssCmdStoreData(0x80/*0x80:last store data command*/, ucIndex++, ucLength, sBuf);
    if(uiRet) {
        sprintf(pszErrMsg, "装载PSE数据分组9102出错[%04X]", uiRet);
		return(1);
    }
    return(0);
}

// 装载PPSE数据
// out : pszErrMsg : err message
// ret : 0         : OK
//       1         : refer to pszErrMsg
int iStorePPseData(char *pszErrMsg)
{
	uint  uiRet;
	uchar ucLength;
	uchar sBuf[256];
	uchar ucIndex;

    ucIndex = 0; // store data指令序号
	// store fci of ppse
	ucLength = 0;
	memcpy(sBuf+ucLength, "\x91\x02", 2); // dgi
	ucLength += 2;
	sBuf[ucLength++] = 0x00; // dgi长度，先空下，稍后再添
   	sBuf[ucLength++] = 0xA5; // TA5
	sBuf[ucLength++] = 0x00; // 先空下,稍后再填
	memcpy(sBuf+ucLength, "\xBF\x0C", 2); // TBF0C
	ucLength += 2;
	sBuf[ucLength++] = 0x00; // 先空下,稍后再填（注意：此处假设为单字节长度，如果fci信息过长，需要调整此处）
	sBuf[ucLength++] = 0x61; // T61
	sBuf[ucLength++] = 0x00; // 先空下,稍后再填
	sBuf[ucLength++] = 0x4F; // T4F, AID
	sBuf[ucLength++] = gl_InstanceRef.ucAidLen; // Length
	memcpy(sBuf+ucLength, gl_InstanceRef.sAid, gl_InstanceRef.ucAidLen);
	ucLength += gl_InstanceRef.ucAidLen;
	sBuf[ucLength++] = 0x50; // T50, Label
	sBuf[ucLength++] = strlen(gl_AppRef.szAppLabel); // Length
	strcpy(sBuf+ucLength, gl_AppRef.szAppLabel);
	ucLength += strlen(gl_AppRef.szAppLabel);
	if(gl_AppRef.ucPriority != 0xFF) {
    	sBuf[ucLength++] = 0x87; // T87, Priority
    	sBuf[ucLength++] = 0x01; // Length
    	sBuf[ucLength++] = gl_AppRef.ucPriority;
	}
	sBuf[2] = ucLength - 3;
	sBuf[4] = ucLength - 5;
	sBuf[7] = ucLength - 8;
	sBuf[9] = ucLength - 10;
	uiRet = uiIssCmdStoreData(0x80/*0x80:last store data command*/, ucIndex++, ucLength, sBuf);
    if(uiRet) {
        sprintf(pszErrMsg, "装载PPSE数据分组9102出错[%04X]", uiRet);
		return(1);
    }
    return(0);
}

// 装载Pboc数据
// out : pszErrMsg : err message
// ret : 0         : OK
//       1         : refer to pszErrMsg
int iStorePbocData(char *pszErrMsg)
{
	short iRet;
	uint  uiRet;
	uchar ucLength;
	int   i;
	uchar ucP1;
	uchar sBuf[256];

    // store pboc2.0 app data
	for(i=0; ; i++) {
		if(gl_alPbocDgis[i] == 0x10000)
			break; // 结束标志
        iRet = iMakePbocDgi(gl_alPbocDgis[i], sBuf);
		if(iRet <= 0) {
			sprintf(pszErrMsg, "构造Pboc数据分组%lX出错%d", gl_alPbocDgis[i], iRet);
			return(1);
		}
		ucLength = (uchar)iRet;

		ucP1 = 0x00;
		if(gl_alPbocDgis[i]>=0x8000 && gl_alPbocDgis[i]<=0x8FFF)
			ucP1 |= 0x60; // 0x8000-0x8FFF为加密的数据分组
		if(gl_alPbocDgis[i+1] == 0x10000)
			ucP1 |= 0x80; // 本指令为最后一个StoreData指令

        //if(gl_alPbocDgis[i] == 0x8205)
        //    ucP1 = ucP1;
		uiRet = uiIssCmdStoreData(ucP1, (uchar)i, ucLength, sBuf);
        if(uiRet) {
            sprintf(pszErrMsg, "装载Pboc数据分组%lX出错[%04X]", gl_alPbocDgis[i], uiRet);
		    return(1);
        }
	}
	return(0);
}

//QINGBO
void vIssProcSetAppParam(int iIndex, char *pszPackageAid, char *pszAppletAid, char *pszC9Object)
{
    if (iIndex > 2)
        return;
    vTwoOne(pszPackageAid, (unsigned int)strlen(pszPackageAid), sg_CardAppParam[iIndex].sPackageAid);
    sg_CardAppParam[iIndex].ucPackageAidLen = strlen(pszPackageAid) / 2;
    vTwoOne(pszAppletAid, (unsigned int)strlen(pszAppletAid), sg_CardAppParam[iIndex].sAppletAid);
    sg_CardAppParam[iIndex].ucAppletAidLen = strlen(pszAppletAid) / 2;
    vTwoOne(pszC9Object, (unsigned int)strlen(pszC9Object), sg_CardAppParam[iIndex].sC9Object);
    sg_CardAppParam[iIndex].ucC9ObjectLen = strlen(pszC9Object) / 2;
}

