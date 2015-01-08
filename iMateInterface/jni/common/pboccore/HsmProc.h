#ifndef _KEYPROC_H
#define _KEYPROC_H

#ifdef __cplusplus
extern "C" {
#endif

int iHsmSetMode(int iMode, char *pszIp, int iPortNo);
int iHsmProc(int iSendLen, void *pSendBuf, int *piRecvLen, void *pRecvBuf);

#ifdef __cplusplus
}
#endif

#endif
