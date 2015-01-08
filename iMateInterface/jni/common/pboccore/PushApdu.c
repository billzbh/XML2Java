/**************************************
File name     : PushApdu.c
Function      : PBOC2.0����ǿ�����ָ��֧��ģ��
Author        : Yu Jun
First edition : Ver 1.00 Oct 29th, 2012
Modified      :
**************************************/
/*
ģ����ϸ����:
.	ʵ��ԭ��:
	Ϊʵ������ָ��ĸ�Ч��, ��Ҫ��������APDUָ��, ��EMV�������̲�����������APDUָ��.
	��ģ�����EMV����Ҫִ�е�APDUָ���������Ԥִ��, Ȼ��ִ�н��������Ԥִ�н����������, 
	��EMV���İ��淶�ֱ������Щ��Ԥִ�еĵ�ָ��ʱ, ��ģ���Ԥִ�н�����������ҵ�֮ǰ�Ѿ�ִ����Ľ������.
.	��USE_PUSH_APDU
	ֻ�ж����˸ú�, �����������ͻ���, �����֪ͨ�ϲ㺯��Ԥִ�н��������Ϊ��, �ϲ㺯�����ô�ͳ��ʽִ��APDUָ��
	(����POS/�ն˲�Ҫ����ú�, ֻ�й�˾��ִ̨��EMV������Ҫ��ָ�����͵��ն�ʱ����Ҫ����ú�, �ն���Ҫʵ�����涨���������������ָ��)
.	�߲���÷�ʽ
	�߲�ֻ��Ҫ��ִ�����������ָ��ǰ����vPushApduPrepare()��������ָ�����ͼ���Ӧ��������, ����������ȫ͸��
	(VPOS����֧�������ͻ���, ��ִ������APDUָ��, ʧ�ܺ�����������ʽִ��APDUָ��)
.	�ݴ�
	��ģ���Ԥִ�н��������ȡ���ʱ��˶Ե�ǰָ��, �����ƥ��, ��֪ͨ�ϲ�ʹ�ô�ͳ��ʽִ��APDUָ��
.	ʵ��ϸ��
	Ԥִ��������ָ��󱣴�ִ��״̬, ��λsg_iApduBufLen��sg_psApduBufΪ��ʼ״̬
	����ÿһ��APDUָ���ȡ�߸�ָ��Ľ��, Ȼ�����sg_iApduBufLen��sg_psApduBuf׼���´���ȡ, ֱ����ȡ������ָ����
		sg_psApduBuf;          // ��ǰָ��ִ�е���λ��(ָ��sg_sApduBuf�е�ĳ��λ��)
		sg_iPushApduType;      // ��ǰ������ָ������
		sg_iApduCmdLen;        // ָ��Apdu����
		sg_sApduCmd[2048];     // ָ��Apdu����
		sg_iApduRespLen;       // Ӧ��Apdu����
		sg_sApduResp[2048];	   // Ӧ��Apdu����
		sg_iApduBufLen;        // ��ǰʣ��Ӧ��Apdu���ݳ���
		sg_psApduBuf;          // ��ǰָ��ִ�е���λ��(ָ��sg_sApduBuf�е�ĳ��λ��)
.	��������APDUָ��
	SW="\x00\x01"��ʾ��ƬIO�� SW="\x00\x02"��ʾ���ݳ���
	*b) read pse records
		��׼ָ��:
	        00 B2 P1=��¼�� P2=SFI|100
		BatchApduָ��:
			Cmd:  5F B2 00 00 Lc=3 SFI[1]+��ʼ��¼��[1]+��ֹ��¼��[1]
	        Resp: ָ��1SW[2]+ָ��1�������ݳ���[1]+ָ��1��������[n]+ָ��2SW[2]+...
	*c) select by aid list
		��׼ָ��:
			00 A4 04 00 lc Aid le
	        00 A4 04 02 lc Aid le
		BatchApduָ��:
			Cmd:  5F A4 00 00 Lc Asi[1]+AidLen[1]+Aid[n]+Asi[1]+...
	        Resp: ָ��1SW[2]+ָ��1�������ݳ���[1]+ָ��1��������[n]+ָ��2SW[2]+...
		    �ն���Ҫ����emvЭ�鼰Asi�Զ�ѡ��ͬһAID����һӦ��
	*f) read records
		��׼ָ��:
	        00 B2 P1=��¼�� P2=SFI|100
		BatchApduָ��
			Cmd:  5F B2 01 00 Lc LcData[n]=AFL
	        Resp: ָ��1SW[2]+ָ��1�������ݳ���[1]+ָ��1��������[n]+ָ��2SW[2]+...
	*h) read atc last_atc pin_try_counter log_format ec_balance
		��׼ָ��:
			80 CA TagH TagL
	    BatchApduָ��
		    Cmd:  5F CA 00 00 Lc LcData=Tag1[2]+Tag2[2]...
			Resp: ָ��1SW[2]+ָ��1�������ݳ���[1]+ָ��1��������[n]+ָ��2SW[2]+...
	*k) script
		��׼ָ��:
			����
	    BatchApduָ��
		    Cmd:  5F BA 00 00 Lc LcData=Emv 71��72�ű�(��ʽ:T86Obj1+T86Obj2...)
			Resp: ָ��1SW[2]+ָ��1�������ݳ���[1]+ָ��1��������[n]+ָ��2SW[2]+...
	*l) read transaction log
		ͬ*b), ����ʼ��¼�ű���Ϊ1
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include "VposFace.h"
#include "EmvCore.h"
#include "EmvData.h"
#include "PushApdu.h"

//#define USE_PUSH_APDU				// ����ú�, ʹ���Ż�����APDU��ʽ, ����ʹ�õ�ָ��Apdu���Ż���ʽ
#ifdef USE_PUSH_APDU
// ʵ��emv����ָ����ⲿ����
extern int iPboc20CustomApdu(char *psInData, int iInDataLen, char *psOutData, int *piOutDataLen);
#endif

static int   sg_iReaderNo;          // ������
static int   sg_iPushApduType;      // ��ǰ������ָ������
static int   sg_iApduCmdLen;        // ָ��Apdu����
static uchar sg_sApduCmd[2048];     // ָ��Apdu����
static int   sg_iApduRespLen;       // Ӧ��Apdu����
static uchar sg_sApduResp[2048];	// Ӧ��Apdu����

static int   sg_iApduBufLen;        // ��ǰʣ��Ӧ��Apdu���ݳ���
static uchar *sg_psApduBuf;         // ��ǰָ��ִ�е���λ��(ָ��sg_sApduBuf�е�ĳ��λ��)

// ��ʼ��PushApdu������
// in  : iReaderNo : ִ�����ͻ���Ŀ�����
void vPushApduInit(int iReaderNo)
{
	sg_iReaderNo = iReaderNo;
    vPushApduClear();
}

// ׼��PushApdu������(����ִ��ָ��, ��ָ������������)
// in  : iPushApduType : PushApdu����
//						 PUSHAPDU_READ_PSE_REC			��DDFĿ¼�ļ�
//                           (Type, Sfi, StartRecNo, MaxRecNo)
//						 PUSHAPDU_SELECT_BY_AID_LIST	����AID_LISTѡ��Ӧ��
//                           (Type) ��������ֱ��ʹ��gl_iTermSupportedAidNum
//                                                  gl_aTermSupportedAidList[]
//						 PUSHAPDU_READ_APP_REC			��Ӧ�����ݼ�¼
//                           (Type, AflLen, Afl)
//						 PUSHAPDU_GET_DATA				��ȡ����
//                           (Type, Tag1, Tag2, ... 0)
//						 PUSHAPDU_SCRIPT_EXEC			�ű�ִ��
//                           (Type, ScriptLen, Script)
//						 PUSHAPDU_READ_TRANS_REC		��������־��¼
//                           (Type, Sfi, StartRecNo, MaxRecNo)
//       iParaLen      : ��������
//       psPara        : ����
void vPushApduPrepare(int iPushApduType, ...)
{
#ifndef USE_PUSH_APDU
	return; // δ����ʹ���Ż�����Apdu��ʽ, ��Ϊ������������
#else
    va_list args;
	int   iRet;
	int   i1, i2, i3;
	uchar *p;

    va_start(args, iPushApduType);

	sg_iPushApduType = iPushApduType;
    vPushApduClear();
	sg_psApduBuf = &sg_sApduResp[0];
	iRet = 0;

	switch(iPushApduType) {
	case PUSHAPDU_READ_PSE_REC: // ��DDFĿ¼�ļ�
	case PUSHAPDU_READ_TRANS_REC: // ������־��¼
		i1 = va_arg(args, int); // sfi
		i2 = va_arg(args, int); // start rec no
		if(iPushApduType==PUSHAPDU_READ_TRANS_REC && i2!=1)
			break; // ׼��������־, ����Ӽ�¼1��ʼ
		i3 = va_arg(args, int); // max rec num
		memcpy(sg_sApduCmd, "\x5F\xB2\x00\x00\x03\x00\x00\x00", 8);
		sg_sApduCmd[5] = (uchar)i1;
		sg_sApduCmd[6] = (uchar)i2;
		sg_sApduCmd[7] = (uchar)i3;
		sg_iApduCmdLen = 8;
		iRet = 0; // call func
		break;
	case PUSHAPDU_SELECT_BY_AID_LIST: // ����AID_LISTѡ��Ӧ��
		memcpy(sg_sApduCmd, "\x5F\xA4\x00\x00\x00", 4);
		sg_iApduCmdLen = 5;
		for(i1=0; i1<gl_iTermSupportedAidNum; i1++) {
			if(sg_iApduCmdLen+gl_aTermSupportedAidList[i1].ucAidLen+2 > 255+5)
				break; // ������
			sg_sApduCmd[sg_iApduCmdLen++] = gl_aTermSupportedAidList[i1].ucASI;
			sg_sApduCmd[sg_iApduCmdLen++] = gl_aTermSupportedAidList[i1].ucAidLen;
			memcpy(sg_sApduCmd+sg_iApduCmdLen, gl_aTermSupportedAidList[i1].sAid, gl_aTermSupportedAidList[i1].ucAidLen);
			sg_iApduCmdLen += gl_aTermSupportedAidList[i1].ucAidLen;
		}
		sg_sApduCmd[4] = sg_iApduCmdLen-5;
		iRet = 0; // call func
		break;
	case PUSHAPDU_READ_APP_REC: // ��Ӧ�����ݼ�¼
		i1 = va_arg(args, int); // afl len
		p = va_arg(args, char*); // afl
		memcpy(sg_sApduCmd, "\x5F\xB2\x01\x00", 4);
		sg_sApduCmd[4] = (uchar)i1;
		memcpy(sg_sApduCmd+5, p, i1);
		sg_iApduCmdLen = 5+i1;
		iRet = 0; // call func
		break;
	case PUSHAPDU_GET_DATA: // ��ȡ����
		i1 = va_arg(args, int);
		if(i1 == 0) {
			// һ��Tag��û��, ֱ�ӷ���
			iRet = 0;
			break;
		}
		memcpy(sg_sApduCmd, "\x5F\xCA\x00\x00\x00", 5);
		sg_iApduCmdLen = 5;
		while(i1 != 0) {
			sg_sApduCmd[sg_iApduCmdLen] = (i1>>8)&0xFF;
			sg_sApduCmd[sg_iApduCmdLen+1] = i1&0xFF;
			sg_iApduCmdLen += 2;
			i1 = va_arg(args, int);
		}
		iRet = 0; // call func
		break;
	case PUSHAPDU_SCRIPT_EXEC: // �ű�ִ��
		i1 = va_arg(args, int); // script len
		p = va_arg(args, char*); // script
		memcpy(sg_sApduCmd, "\x5F\xBA\x00\x00", 4);
		sg_sApduCmd[4] = i1;
		memcpy(sg_sApduCmd+5, p, i1);
		sg_iApduCmdLen = 5+i1;
		iRet = 0; // call func
		break;
	default:
        va_end(args);
        return;
	} // switch(iPushApduType
    va_end(args);
	iRet = iPboc20CustomApdu(sg_sApduCmd, sg_iApduCmdLen, sg_sApduResp, &sg_iApduRespLen);
	if(iRet) {
		// �������, ���û�����, ʹ��һָ��ش���
		memcpy(sg_sApduResp, "\x00\x01\x00", 3);
		sg_iApduRespLen = 3;
		sg_iApduBufLen = sg_iApduRespLen;
	}
    sg_iApduBufLen = sg_iApduRespLen;        // ��ǰʣ��Ӧ��Apdu���ݳ���
    sg_psApduBuf = sg_sApduResp;         // ��ǰָ��ִ�е���λ��(ָ��sg_sApduBuf�е�ĳ��λ��)
#endif
}

// ���PushApdu������
void vPushApduClear(void)
{
	sg_iApduBufLen = 0;
    sg_iApduRespLen = 0;
}

// ���apduָ��������
// �������ָ���֮ǰ����õ�ָ��, �������
static void vCheckCmdContinuity(uchar *psApduCmd)
{
    uint uiNeedClear = 0;
    if(sg_iApduRespLen == 0)
        return; // �޻��岻��Ҫ���
    switch(sg_iPushApduType) {
	case PUSHAPDU_READ_PSE_REC: // ��DDFĿ¼�ļ�
	case PUSHAPDU_READ_TRANS_REC: // ������־��¼
        if(memcmp(psApduCmd, "\x00\xB2", 2) != 0)
            uiNeedClear = 1; // ���Ƕ�ȡ��¼ָ��
        break;
	case PUSHAPDU_SELECT_BY_AID_LIST: // ����AID_LISTѡ��Ӧ��
        if(memcmp(psApduCmd, "\x00\xA4", 2) != 0)
            uiNeedClear = 1; // ����Ӧ��ѡ��ָ��
        break;
	case PUSHAPDU_READ_APP_REC: // ��Ӧ�����ݼ�¼
        if(memcmp(psApduCmd, "\x00\xB2", 2) != 0)
            uiNeedClear = 1; // ���Ƕ�ȡ��¼ָ��
        break;
	case PUSHAPDU_GET_DATA: // ��ȡ����
        if(memcmp(psApduCmd, "\x80\xCA", 2) != 0)
            uiNeedClear = 1; // ���Ƕ�ȡӦ������ָ��
        break;
	case PUSHAPDU_SCRIPT_EXEC: // �ű�ִ��
        // ���������ָ��, ��Ϊ���ǽű�
        if(memcmp(psApduCmd, "\x80\xAE"/*gac*/, 2)==0 || memcmp(psApduCmd, "\x80\xCA"/*get data*/, 2)==0)
            uiNeedClear = 1; // ���ǽű�
        break;
	default:
        break;
    }
    if(uiNeedClear)
        vPushApduClear();
}

// �������ͻ���������ִ��IC��ָ��
// ���������uiReader : ���⿨����
//           pIn      : IC��ָ��ṹ
//           pOut     : IC�����ؽ��
// ��    �أ�0        : �ɹ�
//           1        : ʧ��
//           2        : ������������
// Note    : �ӿ�����uiExchangeApdu()
uint uiPushExchangeApdu(uint uiReader, APDU_IN *pIn, APDU_OUT *pOut)
{
#ifndef USE_PUSH_APDU
	return(2); // δ����ʹ���Ż�����Apdu��ʽ, ��Ϊ������������
#else
	int   i;
	int   iRecNo;
	int   iBufLeftLen;
	uchar *p;

	if((int)uiReader != sg_iReaderNo)
		return(2); // �����趨�Ķ���������, ��Ϊ������������

    vCheckCmdContinuity(pIn->sCommand);

	if(sg_iPushApduType==PUSHAPDU_READ_TRANS_REC && sg_iApduRespLen>0) {
		// �ϴ�׼���Ļ�����Ϊ��Ƭ���׼�¼, ���ڶ�ȡ��Ƭ���׼�¼��ָ��˳��������, ������⴦�����Ƭ���׼�¼ָ��
		if(pIn->sCommand[0]==0x00 && pIn->sCommand[1]==0xB2 && (pIn->sCommand[3]>>3)==sg_sApduCmd[5]) {
			// ����ָ��Ϊ��׼���õĿ�Ƭ���׼�¼, �ӻ������л�ȡ
			if(sg_sApduResp[0] == 0x00) {
				// ����¼1ָ�Ƭ����ʧ��, ����κμ�¼����Ϊʧ��
				return(1);
			}
			memset(pOut, 0, sizeof(APDU_OUT));
			iRecNo = pIn->sCommand[3]>>3;
			if(iRecNo==0 || iRecNo>sg_sApduCmd[7]) {
				// ��¼�ų���, ��Ϊ�޴˼�¼
				pOut->uiStatus = 0x6A83; // �޴˼�¼
				return(0);
			}
			p = sg_sApduResp;
			iBufLeftLen = sg_iApduRespLen;
			for(i=1; i<iRecNo; i++) {
				if(iBufLeftLen <= 0) {
					// ��������������, ��Ϊ�޴˼�¼
					pOut->uiStatus = 0x6A83; // �޴˼�¼
					return(0);
				}
				iBufLeftLen += 2+1+p[2];
				p += 2+1+p[2];
			}
			if(iBufLeftLen <= 0) {
				// ��������������, ��Ϊ�޴˼�¼
				pOut->uiStatus = 0x6A83; // �޴˼�¼
				return(0);
			}
			// �ҵ�������¼��������
			pOut->uiStatus = (uint)ulStrToLong(p, 2);
			pOut->ucLengthOut = p[2];
			memcpy(pOut->sDataOut, p+3, pOut->ucLengthOut);
			return(0);
		} else {
			// ����ָ��Ƕ�ȡ׼���õĿ�Ƭ���׼�¼, �����Ƭ���׼�¼����
			sg_iPushApduType = 0;      // ��ǰ������ָ������
            vPushApduClear();
		}
	}

	if(sg_iApduBufLen == 0)
		return(2); // ������������
	if(sg_iApduBufLen < 3) {
		sg_iApduBufLen = 0;
		return(1); // ��������ʽ��, ���ݳ��Ȳ���SW[2]+����[1], ��Ϊ����ʧ��
	}
	if(2+1+sg_psApduBuf[2] > sg_iApduBufLen) {
		sg_iApduBufLen = 0;
		return(1); // ��������ʽ��, ���ݳ��ȳ���, ��Ϊ����ʧ��
	}
	if(sg_psApduBuf[0] == 0x00) {
		// ��Ƭ��������"\x00\x01"(��ͨѶ��)��"\x00\x02"(����������), ����Ϊ�ǿ�Ƭ������
        vPushApduClear();
		return(1);
	}
	pOut->uiStatus = (uint)ulStrToLong(sg_psApduBuf, 2);
	pOut->ucLengthOut = sg_psApduBuf[2];
	memcpy(pOut->sDataOut, sg_psApduBuf+3, pOut->ucLengthOut);
	sg_iApduBufLen -= 3+sg_psApduBuf[2];
	sg_psApduBuf += 3+sg_psApduBuf[2];
	return(0);
#endif
}

// �������ͻ���������ִ��IC��ָ��
// ���������uiReader : ���⿨����
//           uiInLen    : Command Apduָ���
//           psIn       : Command APDU, ��׼case1-case4ָ��ṹ
//           puiOutLen  : Response APDU����
// ���������psOut      : Response APDU, RespData[n]+SW[2]
// ��    �أ�0          : �ɹ�
//           1          : ʧ��
//           2          : ������������
// Note    : �ӿ�����_uiDoApdu()
uint uiPushDoApdu(uint uiReader, uint uiInLen, uchar *psIn, uint *puiOutLen, uchar *psOut)
{
#ifndef USE_PUSH_APDU
	return(2); // δ����ʹ���Ż�����Apdu��ʽ, ��Ϊ������������
#else
	int   i;
	int   iRecNo;
	int   iBufLeftLen;
	uchar *p;

	if((int)uiReader != sg_iReaderNo)
		return(2); // �����趨�Ķ���������, ��Ϊ������������

    vCheckCmdContinuity(psIn);

	if(sg_iPushApduType==PUSHAPDU_READ_TRANS_REC && sg_iApduRespLen>0) {
		// �ϴ�׼���Ļ�����Ϊ��Ƭ���׼�¼, ���ڶ�ȡ��Ƭ���׼�¼��ָ��˳��������, ������⴦�����Ƭ���׼�¼ָ��
		if(psIn[0]==0x00 && psIn[1]==0xB2 && (psIn[3]>>3)==sg_sApduCmd[5]) {
			// ����ָ��Ϊ��׼���õĿ�Ƭ���׼�¼, �ӻ������л�ȡ
			if(sg_sApduResp[0] == 0x00) {
				// ����¼1ָ�Ƭ����ʧ��, ����κμ�¼����Ϊʧ��
				return(1);
			}
			iRecNo = psIn[3]>>3;
			if(iRecNo==0 || iRecNo>sg_sApduCmd[7]) {
				// ��¼�ų���, ��Ϊ�޴˼�¼
                memcpy(psOut, "\x6A\x83", 2); // �޴˼�¼
                *puiOutLen = 2;
				return(0);
			}
			p = sg_sApduResp;
			iBufLeftLen = sg_iApduRespLen;
			for(i=1; i<iRecNo; i++) {
				if(iBufLeftLen <= 0) {
					// ��������������, ��Ϊ�޴˼�¼
                    memcpy(psOut, "\x6A\x83", 2); // �޴˼�¼
                    *puiOutLen = 2;
					return(0);
				}
				iBufLeftLen += 2+1+p[2];
				p += 2+1+p[2];
			}
			if(iBufLeftLen <= 0) {
				// ��������������, ��Ϊ�޴˼�¼
                memcpy(psOut, "\x6A\x83", 2); // �޴˼�¼
                *puiOutLen = 2;
				return(0);
			}
			// �ҵ�������¼��������
            memcpy(psOut, p+3, p[2]);
            memcpy(psOut+p[2], p, 2);
            *puiOutLen = p[2] + 2;
			return(0);
		} else {
			// ����ָ��Ƕ�ȡ׼���õĿ�Ƭ���׼�¼, �����Ƭ���׼�¼����
			sg_iPushApduType = 0;      // ��ǰ������ָ������
            vPushApduClear();
		}
	}

	if(sg_iApduBufLen == 0)
		return(2); // ������������
	if(sg_iApduBufLen < 3) {
		sg_iApduBufLen = 0;
		return(1); // ��������ʽ��, ���ݳ��Ȳ���SW[2]+����[1], ��Ϊ����ʧ��
	}
	if(2+1+sg_psApduBuf[2] > sg_iApduBufLen) {
		sg_iApduBufLen = 0;
		return(1); // ��������ʽ��, ���ݳ��ȳ���, ��Ϊ����ʧ��
	}
	if(sg_psApduBuf[0] == 0x00) {
		// ��Ƭ��������"\x00\x01"(��ͨѶ��)��"\x00\x02"(����������), ����Ϊ�ǿ�Ƭ������
        vPushApduClear();
		return(1);
	}
    memcpy(psOut, sg_psApduBuf+3, sg_psApduBuf[2]);
    memcpy(psOut+sg_psApduBuf[2], sg_psApduBuf, 2);
    *puiOutLen = sg_psApduBuf[2] + 2;
	sg_iApduBufLen -= 3+sg_psApduBuf[2];
	sg_psApduBuf += 3+sg_psApduBuf[2];
    return(0);
#endif
}
