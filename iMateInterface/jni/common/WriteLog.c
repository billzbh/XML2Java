#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <jni.h>
#include <android/log.h>

#include <unsigned.h>

static int sg_iLogFlag = 0;

static void vOneTwo(const uchar *psIn, int iLength, uchar *psOut)
{
    static const uchar aucHexToChar[17] = "0123456789ABCDEF";
    int iCounter;

    for(iCounter = 0; iCounter < iLength; iCounter++){
        psOut[2*iCounter] = aucHexToChar[((psIn[iCounter] >> 4)) & 0x0F];
        psOut[2*iCounter+1] = aucHexToChar[(psIn[iCounter] & 0x0F)];
    }
}
static void vOneTwo0(const uchar *psIn, int iLength, uchar *pszOut)
{
    vOneTwo(psIn, iLength, pszOut);
	if(iLength < 0)
		iLength = 0;
    pszOut[2*iLength]=0;
}

void vSetWriteLog(int iOnOff)
{
	sg_iLogFlag = iOnOff;
}

void vWriteLogHex(char *pszTitle, void *pLog, int iLength)
{
    char szBuf[4096];

	if(sg_iLogFlag == 0)
		return;

    memset(szBuf,0,sizeof(szBuf));
    if(pszTitle) {
        sprintf(szBuf, "%s(%3d) : ", pszTitle, iLength);
    }
	vOneTwo0(pLog, (ushort)iLength, szBuf+strlen(szBuf));
	__android_log_print(ANDROID_LOG_DEBUG,"HxSmart_Jni",szBuf);
}

void vWriteLogTxt(char *pszFormat, ...)
{
    va_list args;
    char buff[4096];

	if(sg_iLogFlag == 0)
		return;

    va_start(args, pszFormat);
    vsprintf(buff,pszFormat, args);
	__android_log_print(ANDROID_LOG_DEBUG,"HxSmart_Jni",buff);
    va_end(args);
}
