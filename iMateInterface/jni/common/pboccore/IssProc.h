/**************************************
File name     : IssProc.h
Function      : Issue pboc card
Author        : Yu Jun
First edition : Aug 1st, 2011
Note          :
**************************************/
#ifndef _ISSPROC_H
#define _ISSPROC_H

#ifdef __cplusplus
extern "C" {
#endif

int iSelectApp(int iAidLen, uchar *psAid, char *pszErrMsg);
int iDeleteApp(int iAidLen, uchar *psAid, char *pszErrMsg);
int iGetAppList(int *piAidListLen, uchar *psAidList, char *pszErrMsg);
int iEstablishSecureChannel(int iKeyIndex, int iCardMgrFlag, void *pTermRand, void *pCardData, char *pszErrMsg);
int iCreatePse(int iPPseFlag, char *pszErrMsg);
int iCreatePboc(char *pszErrMsg);
int iStorePseData(char *pszErrMsg);
int iStorePPseData(char *pszErrMsg);
int iStorePbocData(char *pszErrMsg);
    
//QINGBO
void vIssProcSetAppParam(int iIndex, char *pszPackageAid, char *pszAppletAid, char *pszC9Object);

#ifdef __cplusplus
}
#endif

#endif
