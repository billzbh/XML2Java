#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "sha.h"
#include "RemoteCall.h"

static int sg_iRemoteCallMode = -1;

extern int syncCommon(unsigned char *sendData, int sendLength, unsigned char *receivedData, int *receivedLength, int timeout);

static uchar sg_sParaBuff[4096];
static uchar *sg_pPara[20];
static uchar *sg_sKey1 = (uchar*)"\x02\x82\xa0\x10\x28\x49\x36\x3e\x03\xcc\xa9\x26";

#define INTVALUE(a)		atoi((char*)a)

static uchar ucGetRand(void)
{
	srand((uint)time(NULL));
	return((uchar)rand());
}

static void vPassSpace(uchar **psPointer)
{
    while(**psPointer == ' ' || **psPointer == '\t')
        (*psPointer) ++;
}

void vPackData(uchar *psData, uint uiInLen, uchar *psMacRand)
{
	int i;
	uchar sWorkingKey[20], sTmp[20], sRand[8];

	for(i = 0; i < 8; i++)
		sRand[i] = ucGetRand();

	vSHA1Init();
    vSHA1Update(sg_sKey1, 12);
    vSHA1Update(sRand, 8);
    vSHA1Result2(sWorkingKey);

    vSHA1Update(psData, uiInLen);
    vSHA1Result(sTmp);

	for(i = 0; i < uiInLen; i++ ) {
		psData[i] ^= sWorkingKey[i % 20];
	}

    memcpy(psMacRand, sTmp, 4);
    memcpy(psMacRand + 4, sRand, 8);
    memcpy(psMacRand + 12, sTmp + 16, 4);

	return;
}

int iUnpackData(uchar *psData, uint uiInLen)
{
	int i, iLen = 0;
	uchar sWorkingKey[20], sTmp[20], sRand[8], sMac[8];

	if (uiInLen < 16)
		return -1;

	iLen = uiInLen - 16;
	memcpy(sMac, psData + uiInLen - 16 , 4);
	memcpy(sRand, psData + uiInLen - 12, 8);
	memcpy(sMac + 4, psData + uiInLen - 4 , 4);

	vSHA1Init();
    vSHA1Update(sg_sKey1, 12);
    vSHA1Update(sRand, 8);
    vSHA1Result2(sWorkingKey);

	for(i = 0; i < iLen; i++ ) {
		psData[i] ^= sWorkingKey[i % 20];
	}

    vSHA1Update(psData, iLen);
    vSHA1Result(sTmp);

    if(memcmp(sTmp, sMac, 4) || memcmp(sTmp + 16, sMac + 4, 4))
    	return -2;

    psData[iLen] = 0;

    vWriteLogHex("iUnpackData:", psData, iLen);

	return iLen;
}

// ret 0 : OK
int iFactorize(uchar *psReceivedBuf, uint uiDataLen)
{
    int iSubscript;
    uchar *psCmd;
    uchar *psPara;
    ulong ulTmp;
    char szTmp[64];
    int i;

    psCmd = psReceivedBuf;

	memset(sg_sParaBuff, 0, sizeof(sg_sParaBuff));
	for (i = 0; i < 20; i++)
		sg_pPara[i] = NULL;

	psPara = sg_sParaBuff;
    iSubscript = 0;
    sg_pPara[iSubscript++] = psPara;

    // Search return value
    vPassSpace(&psCmd);
    if (*psCmd != '(') return -1;
    psCmd++;
    vPassSpace(&psCmd);
    while(*psCmd != ')' && *psCmd != ' ' && *psCmd != '\t') {
        *psPara = *psCmd;
        psPara ++;
        psCmd ++;
    }
	psCmd++;
    *psPara = 0;
    psPara ++;
    sg_pPara[iSubscript++] = psPara;

    // search function name
    vPassSpace(&psCmd);
    while(*psCmd != '(' && *psCmd != ' ' && *psCmd != '\t') {
        *psPara = *psCmd;
        psPara ++;
        psCmd ++;
    }
    *psPara = 0;
    psPara ++;
    sg_pPara[iSubscript++] = psPara;

    // search '('
    vPassSpace(&psCmd);
    if(*psCmd != '(')
        return(1); // format error
    psCmd ++;

    // search parameters
    for(;;) {
    	//iParaType = 0;
        vPassSpace(&psCmd);
        if (psCmd - psReceivedBuf > uiDataLen)
        	return 2;
        if(*psCmd == ')') {
        	sg_pPara[iSubscript - 1] = NULL;
            return(0);
        }
        if(*psCmd == 's' || *psCmd == 'S' || *psCmd == 'b' || *psCmd == 'B') {
            i = (psCmd[1]-'0')*100 + (psCmd[2]-'0')*10 + (psCmd[3]-'0');
            memcpy(psPara, &psCmd[4], i);
            psCmd += i + 4;
	        psPara[i] = 0;
    	    psPara += i + 1;
        }  else  if(*psCmd == 'p' || *psCmd == 'P') {
            i = (psCmd[1]-'0')*1000 + (psCmd[2]-'0')*100 + (psCmd[3]-'0')*10 + (psCmd[4]-'0');
            memcpy(psPara, &psCmd[5], i);
            psCmd += i + 5;
	        psPara[i] = 0;
    	    psPara += i + 1;
        }
        else {
			i = 0;
            while(*psCmd != ',' && *psCmd != ' ' &&
                  *psCmd != '\t' && *psCmd != ')') {
                szTmp[i++] = *psCmd;
                psCmd ++;
            }
            szTmp[i] = 0;
            sscanf(szTmp, "%ld", &ulTmp);
            sprintf(szTmp, "%ld", ulTmp);
            strcpy((char *)psPara, szTmp);
            psPara += strlen(szTmp) + 1;
        }
        sg_pPara[iSubscript++] = psPara;
        vPassSpace(&psCmd);
        if(*psCmd == ',') {
            psCmd ++;
        } else if(*psCmd ==')')
            continue;
		  else
            return(3); // format error

    } // for(;;
    return 4;
}

// 执行远程函数
// 返回码：0     :  成功
//        -1    :  通讯失败
//        -2    :  不支持该远程调用
//       < 0    :  解包失败
int iDoRemoteFunc(uchar *psFunc, uint uiFuncLen, uint uiTimeOut)
{
	int iRet, iTmp;
	uint uiDataLen;

	uchar *psDataBuf = malloc(2048);

	//vSetWriteLog(1);
	vWriteLogTxt((char*)psFunc);
    vWriteLogHex("psFunc:", psFunc, uiFuncLen);

	psDataBuf[0] = 0x00;
	uiDataLen = 1;
	memcpy(psDataBuf + uiDataLen, psFunc, uiFuncLen);
	uiDataLen += uiFuncLen;
	vPackData(psDataBuf + 1, uiFuncLen, psDataBuf + uiDataLen);
	uiDataLen += 16;
	iRet = syncCommon(psDataBuf, uiDataLen, psDataBuf, (int*)&uiDataLen, uiTimeOut);
	if (iRet == 0) {
		if (psDataBuf[0]) {
			sg_iRemoteCallMode = 0;
			vWriteLogTxt("syncCommon psDataBuf[0] = %d", psDataBuf[0]);
			free(psDataBuf);
			return -2;
		}
		iRet = iUnpackData(psDataBuf + 1, uiDataLen - 1);
		if (iRet <= 0) {
			vWriteLogTxt("iDoRemoteFunc iUnpackData error = %d", iRet);
			free(psDataBuf);
			return 1;
		}
		iTmp = iFactorize(psDataBuf + 1, iRet);
		if (iTmp) {
			vWriteLogTxt("iDoRemoteFunc iFactorize error = %d", iTmp);
			free(psDataBuf);
			return 2;
		}
		vWriteLogTxt(">>result:(%s)%s", sg_pPara[0], sg_pPara[1]);
		free(psDataBuf);
		return 0;
	}
    else {
        vWriteLogTxt("syncCommon ret = %d", iRet);
    }
	free(psDataBuf);
	return -2;
}

int iGetRemoteCallMode(void)
{
    if (sg_iRemoteCallMode >=0)
    	return sg_iRemoteCallMode;

    uchar sRequestDataBuffer[10], sResponseDataBuffer[100];
    int iResponseDataLength;

    sRequestDataBuffer[0] = 0x60;
    memset(sResponseDataBuffer, 0, sizeof(sResponseDataBuffer));
    int ret = syncCommon(sRequestDataBuffer, 1, sResponseDataBuffer, &iResponseDataLength, 1);
    if (ret || iResponseDataLength == 0 || sResponseDataBuffer[0]) {
    	sg_iRemoteCallMode = 0;
    	return sg_iRemoteCallMode;
    }
    char *p = strstr(sResponseDataBuffer + 1, ",");
    if (p == NULL) {
    	sg_iRemoteCallMode = 0;
    	return sg_iRemoteCallMode;
    }
    if (strcmp(p + 1, "3.0.1") >= 0) {
		sg_iRemoteCallMode = 1;
		return sg_iRemoteCallMode;
    }
	sg_iRemoteCallMode = 0;
	return sg_iRemoteCallMode;
}

uchar *psGetParaBuf(int index)
{
	return sg_pPara[index];
}

int iGetParaInt(int index)
{
	return atoi((char*)sg_pPara[index]);
}

long lGetParaLong(int index)
{
	return atol((char*)sg_pPara[index]);
}


