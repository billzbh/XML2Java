/**************************************
File name     : EMVSELE.C
Function      : EMVӦ��ѡ��ģ��
Author        : Yu Jun
First edition : Mar 13th, 2003
Note          : Refer to EMV2004 Book1, Part III, 11.3
                                 Book1, Part III, 12
                    EMV Application Note Bulletin No.06, June 1st, 2002
Reference     : TlvFunc.c EmvCmd.c
Modified      : July 31st, 2008
					From EMV2000 to EMV2004
			    Apr 5th, 2012
			        Port to fit pboc2.0 Level2 certification EmvCore data structure
ģ�����      : EMV��Ӧ��ѡ��ʹ�÷���
                ����iSelGetAids()������ѡӦ���б�
				    gl_iCardAppNum  : �ն��뿨Ƭͬʱ֧�ֵ�Ӧ����Ŀ
					gl_aCardAppList : �ն��뿨Ƭͬʱ֧�ֵ�Ӧ���б�
                ����iSelSortCandidate()����ѡ�б�����
				���ⲿ�������Զ�ѡ��ѡ��Ӧ��
                ����iSelFinalSelect()ѡ��Ӧ��
				static int _iSelCandidateListOp(uchar ucCmd, stADFInfo *pAdf)
���¼�¼      : 20121220, �޸�_iSelCandidateListOp(), ������Ӧ�õ���ѡ�б�ʱ, ����Ƿ��ظ�, ����ظ�, ���ظ�����Ӧ��
���¼�¼      : 20130228, �޸�_iSelCandidateListOp(), ������Ӧ�õ���ѡ�б�ʱ, ���label��preferred name�Ƿ��ظ�, ����ظ�, ���ظ�����Ӧ��
���¼�¼      : 20140814, �޸�_iSelCandidateListOp(), �����ж�Ӧ���Ƿ���Ҫ�ֿ���ȷ��ʱ����
**************************************/
/*
ģ����ϸ����:
	EMVӦ��ѡ��֧��ģ��
.	֧��PSE��ʽ��AID�б�ʽѡ��Ӧ��
.	֧��DDF(EMV4.3/PBOC3.0�Ѿ�ȡ����DDF��֧����, �ն˿��Բ�֧��Ҳ����֧��DDF, �������������DDF֧��, û��ȥ��)
	_iSelDdfQueueOp()������������һ���ڲ�DDF����
.	PSE��������
	��PSE("1PAY.SYS.DDF01")��Ϊһ��DDF�ӵ�DDF����, Ȼ��ʼ����DDF����
	���DDF����Ϊ��, �˳�PSE������, ����ѭ��ִ��
		�Ӷ�����ȡ��һDDF, ������ָ���ļ�¼�ļ�, ���ζ�ȡ��¼����(ÿ����¼�����ж�����,�ֱ���)
		������ΪDDF, ��������DDF����, ��������
		������Ϊ֧�ֵ�ADF, �������֧�ֵ�Ӧ���б�
.	AID�б�������
	����ѡ���ն�֧�ֵ�AID, �����Ƭ����, ������뵽֧�ֵ�Ӧ���б�
	����������ƥ��, ��Ҫ�ж�ASI�Ծ����Ƿ�֧��, ���֧�ֲ��������Ҳ�������ƥ��, ��Ҫ����ѡ��ͬһAID, ֱ����Ƭ����Ӧ�ò�֧��
.	Ӧ��ѡ��
	��ѡPSE����, ���ʧ�ܻ�û��ƥ��Ӧ��, ����ʹ��AID�б�
.	����Ӧ��ƥ�����������EMV�������ݿ���(EmvData.c)
		gl_iCardAppNum;                 // �ն��뿨Ƭͬʱ֧�ֵ�Ӧ����Ŀ
		gl_aCardAppList[];              // �ն��뿨Ƭͬʱ֧�ֵ�Ӧ���б�
*/
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "VposFace.h"
#include "PushApdu.h"
#include "Common.h"
#include "Arith.h"
#include "EmvCore.h"
#include "EmvCmd.h"
#include "TlvFunc.h"
#include "EmvIo.h"
#include "TagDef.h"
#include "EmvMsg.h"
#include "EmvSele.h"
#include "EmvData.h"

static uchar  sg_szLanguage[8+1];		 // PSE�г��ֵĺ�ѡ����
static int    sg_iIssuerCodeTableIndex;  // PSE�г��ֵ��ַ�������, -1:not exist

static int    sg_iTotalMatchedApps;      // ��Ƭ���ն����ͬʱ֧�ֵ�Ӧ�ø���
static int    sg_iCardHolderConfirmed;   // 1:�ֿ����Ѿ����й�ȷ�� 0:�ֿ���δ���й�ȷ��
                                         // ����ȷ���ֿ����Ƿ���ȷ����ĳӦ��
static uchar  sg_sFciTlvBuf[300];        // ���һ��ѡ��Ӧ��ʱ�����FCI����

// ����ַ����Ƿ�ȫ�����ڹ����ַ���
// ret : 0 : ȫ�����ڹ����ַ���
//       1 : ��ȫ�����ڹ����ַ���
static int iCharCheck(uchar *pszStr)
{
	int i;
	for(i=0; i<(int)strlen(pszStr); i++) {
		if(pszStr[i]<0x20 || pszStr[i]>0x7E)
			return(1);
	}
	return(0);
}

// DDF ���й���
// ����: ucCmd        : ����ָ��
//                      DDF_QUEUE_INIT
//                      DDF_QUEUE_IN
//                      DDF_QUEUE_OUT
//                      DDF_QUEUE_STATUS
// ˵��: ���������뷵��ֵ���ڶ���ָ��Ĳ�ͬ����ͬ, �������£�
// ucCmd == DDF_QUEUE_INIT
//         ����: ��ն��У�����������ʹ��
//         ����: 0              : �ɹ�
// ucCmd == DDF_QUEUE_IN
//         ����: �µ�DDF�������
//         ����: ucDdfNameLen   : DDF���ֳ���
//               psDdfName      : DDF����
//         ����: 0              : �ɹ�
//               -1             : ������
// ucCmd == DDF_QUEUE_OUT
//         ����: �Ӷ�����ȡ��DDF
//         ���: ucDdfNameLen   : DDF���ֳ���
//               psDdfName      : DDF����
//         ����: 0              : �ɹ�
//               -1             : �����ѿ�
// ucCmd == DDF_QUEUE_STATUS
//         ����: �鿴���е�ǰ״����������������
//         ����: 0              : �����ѿ�
//               >0             : ������DDF����
# define DDF_QUEUE_INIT     0
# define DDF_QUEUE_IN       1
# define DDF_QUEUE_OUT      2
# define DDF_QUEUE_STATUS   3
static int _iSelDdfQueueOp(uchar ucCmd, uchar *pucDdfNameLen, uchar *psDdfName)
{
    static uchar sDFQueue[200]; // DDF����, ��ʽ:Length[1]Name[n] + Length[1]Name[n]...
    static uchar ucCurrDFs;     // ��ǰ������DDF����
    static int iCurrLength;   // ��ǰ�����Ѿ�ʹ�õĴ�С
    
    int  i;

    switch(ucCmd) {
    case DDF_QUEUE_INIT:
        ucCurrDFs = 0;
        iCurrLength = 0;
        break;
    case DDF_QUEUE_IN:
        if(iCurrLength+*pucDdfNameLen+1 > sizeof(sDFQueue))
            return(-1);
        sDFQueue[iCurrLength] = *pucDdfNameLen;
        memcpy(&sDFQueue[iCurrLength+1], psDdfName, (int)(*pucDdfNameLen));
        iCurrLength += *pucDdfNameLen + 1;
        ucCurrDFs ++;
        break;
    case DDF_QUEUE_OUT:
        if(ucCurrDFs == 0)
            return(-1);
        *pucDdfNameLen = sDFQueue[0];
        memcpy(psDdfName, &sDFQueue[1], (int)(*pucDdfNameLen));
        for(i=0; i<iCurrLength-*pucDdfNameLen-1; i++)
            sDFQueue[i] = sDFQueue[i+*pucDdfNameLen+1];
        ucCurrDFs --;
        break;
    case DDF_QUEUE_STATUS:
        return(ucCurrDFs);
    } // switch(ucCmd
    return(0);
}

// ��ѡAID�б����(gl_EmvAppData.aCandidateList, gl_EmvAppData.uiNumCandidate)
// ����: ucCmd        : ָ��
//                      CANDIDATE_LIST_INIT
//                      CANDIDATE_LIST_ADD
//                      CANDIDATE_LIST_DEL
//                      CANDIDATE_LIST_STATUS
// ˵��: ���������뷵��ֵ���ڶ���ָ��Ĳ�ͬ����ͬ, �������£�
// ucCmd == CANDIDATE_LIST_INIT
//         ����: ����б�����������ʹ��
//         ����: 0              : �ɹ�
// ucCmd == CANDIDATE_LIST_ADD
//         ����: ����һ����ѡ
//         ����: pCandidate     : ��ѡ��¼ָ��
//         ����: 0              : �ɹ�
//               -1             : �б���
// ucCmd == CANDIDATE_LIST_DEL
//         ����: ɾ��һ����ѡ
//         ����: pCandidate     : ��ѡ��¼ָ��(ֻ�����е�sDFName��ucDFNameLen)
//         ����: 0              : �ɹ�
//               -1             : �޴˼�¼
// ucCmd == CANDIDATE_LIST_STATUS
//         ����: �鿴��ѡ���е�ǰ״����������������
//         ����: 0              : �б��
//               >0             : �б��к�ѡ����
# define CANDIDATE_LIST_INIT	0
# define CANDIDATE_LIST_ADD		1
# define CANDIDATE_LIST_DEL		2
# define CANDIDATE_LIST_STATUS	3
static int _iSelCandidateListOp(uchar ucCmd, stADFInfo *pAdf)
{
    int i;
	int iRet;
	int iFoundFlag;
	uchar *p, ucAppConfirmSupport;          // TDFxx, 1:֧��Ӧ��ȷ�� 0:��֧��Ӧ��ȷ��(TAG_DFXX_AppConfirmSupport)

	switch(ucCmd) {
	case CANDIDATE_LIST_INIT:
		// ��ʼ��Ӧ�ú�ѡ�б�
		memset(gl_aCardAppList, 0, sizeof(gl_aCardAppList));
		gl_iCardAppNum = 0;
		break;
    case CANDIDATE_LIST_ADD:
		// ����, 20130129 by yujun
		// ����ն˲�֧��Ӧ��ȷ��, ����ѡAID��Ҫȷ��, ����������ѡ�б�
		iRet = iTlvGetObjValue(gl_sTlvDbTermFixed, TAG_DFXX_AppConfirmSupport, &p); // ��ȡ�ն�Ӧ��ѡ��֧�����
		if(iRet <= 0)
			ucAppConfirmSupport = 1; // ���������, �����ն�֧��ȷ��
		else
			ucAppConfirmSupport = *p;
		if(ucAppConfirmSupport == 0) {
			// �ն˲�֧�ֳֿ���ȷ��
			if(pAdf->iPriority<0 || (pAdf->iPriority&0x80) == 0x80)
				return(0); // ��Ӧ����Ҫȷ�ϣ����ܽ�������ѡ�б�
		}

		if(gl_iCardAppNum >= sizeof(gl_aCardAppList)/sizeof(gl_aCardAppList[0]))
			return(-1); // �б���
		// ����, 20121220 by yujun
		// Ϊ��ֹAID�б�ʽѡ��Ӧ��ʱ���ֵ��ظ����, ��:case V2CB0310603v4.2c
		// �����ظ���, �����AID�Ѿ������˺�ѡ�б�, �����ظ�����
		iFoundFlag = 0;
		for(i=0; i<gl_iCardAppNum; i++) {
			if(memcmp(&gl_aCardAppList[i], pAdf, sizeof(stADFInfo)) == 0) {
				iFoundFlag = 1;
				break;
			}
		}
		// ����, 20130228 by yujun
		// Ϊ��ֹAID�б�ʽѡ��Ӧ��ʱ���ֵ��ظ����, ��:case V2CE.003.05
		// ǿ�������ظ���label��preferred name, �����AID�Ѿ������˺�ѡ�б�, �����ظ�����
		// (ԭ��:�ж�����ADF��ʾ���ֿ���ѡ��������Ƿ���ͬ, �����ͬ, ��Ϊ�ظ�)
		// **************************
		// �ٴ�����, 20130407 by yujun
		// case 2CL.001.01, PSE��ʽ������2��Ӧ��, ������Ӧ��Preferred Name��ͬ, ��������Ҫ������Ӧ�ö��Ǻ�ѡ
		// �������:��������, V2CE.003.05������Ӧ��Label��PreferredName����ͬ, ��2CL.001.xx��Label��ͬPreferredName��ͬ
		//          �����������, �ж�Label��PreferredName����ͬʱ����Ϊ���ظ�
#if 0
// 20130228��������
		for(i=0; i<gl_iCardAppNum; i++) {
			// 1.���preferred name�Ƿ��ظ�
			if(gl_aCardAppList[i].szPreferredName[0]) {
				// ����preferred nameʱ���бȽϵ�����
				if(iCharCheck(gl_aCardAppList[i].szPreferredName) == 0) {
					// ֻ���������ַ������бȽϵ�����
					if(pAdf->szPreferredName[0] == 0)
						continue; // ��Adf��preferred name, ԭAdf��preferred name, ��ͬ, ������һ��ADF
					if(iCharCheck(pAdf->szPreferredName) == 1)
						continue; // ��Adf֮preferred name���ڷǹ����ַ����ַ�, ԭAdf֮preferred name�����ڷǹ����ַ����ַ�, ��ͬ, ������һ��ADF
					if(strcmp(gl_aCardAppList[i].szPreferredName, pAdf->szPreferredName) == 0) {
						iFoundFlag = 1;
						break; // preferred name��ͬ, �����ٱȽ�
					} else
						continue; // preferred name��ͬ, ������һ��Adf
				}
			}
			// 2.���label�Ƿ��ظ�
			if(gl_aCardAppList[i].szLabel[0]) {
				// ����Label���бȽ�����
				if(strcmp(gl_aCardAppList[i].szLabel, pAdf->szLabel) == 0) {
					iFoundFlag = 1;
					break; // label��ͬ, �����ٱȽ�
				} else
					continue; // label��ͬ, ���ٱȽ�adf name
			}
			// 3.���Adf name
			if(gl_aCardAppList[i].ucAdfNameLen == pAdf->ucAdfNameLen) {
				// adf name������ͬ�Ƚϲ�������
				if(memcmp(gl_aCardAppList[i].sAdfName, pAdf->sAdfName, pAdf->ucAdfNameLen) == 0) {
					iFoundFlag = 1;
					break; // adf name��ͬ, �����ٱȽ�
				} else
					continue;
			}
		}
#else
// 20130407��������
		for(i=0; i<gl_iCardAppNum; i++) {
			// 1.���preferred name�Ƿ��ظ�
			if(gl_aCardAppList[i].szPreferredName[0]) {
				// ����preferred nameʱ���бȽϵ�����
				if(iCharCheck(gl_aCardAppList[i].szPreferredName) == 0) {
					// ֻ���������ַ������бȽϵ�����
					if(pAdf->szPreferredName[0] == 0)
						continue; // ��Adf��preferred name, ԭAdf��preferred name, ��ͬ, ������һ��ADF
					if(iCharCheck(pAdf->szPreferredName) == 1)
						continue; // ��Adf֮preferred name���ڷǹ����ַ����ַ�, ԭAdf֮preferred name�����ڷǹ����ַ����ַ�, ��ͬ, ������һ��ADF
					if(strcmp(gl_aCardAppList[i].szPreferredName, pAdf->szPreferredName) != 0)
						continue; // preferred name��ͬ, ������һ��Adf
				}
			}
			// 2.���label�Ƿ��ظ�
			if(gl_aCardAppList[i].szLabel[0]) {
				// ����Label���бȽ�����
				if(strcmp(gl_aCardAppList[i].szLabel, pAdf->szLabel) != 0)
					continue; // label��ͬ, ������һ��Adf
			}
			// 3.Preferred Name��Label����ͬ
			//   ���Preferred Name��Label�Ƿ���Ч, �����һ����Ч, ��Ϊ��ͬ, ���ٽ���Adf name���
			if(gl_aCardAppList[i].szPreferredName[0] || gl_aCardAppList[i].szLabel[0]) {
				iFoundFlag = 1;
				break;
			}
			// 4.���Adf name
			if(gl_aCardAppList[i].ucAdfNameLen == pAdf->ucAdfNameLen) {
				// adf name������ͬ�Ƚϲ�������
				if(memcmp(gl_aCardAppList[i].sAdfName, pAdf->sAdfName, pAdf->ucAdfNameLen) == 0) {
					iFoundFlag = 1;
					break; // adf name��ͬ, �����ٱȽ�
				} else
					continue;
			}
		}
#endif
		if(iFoundFlag == 0)
			memcpy(&gl_aCardAppList[gl_iCardAppNum++], pAdf, sizeof(*pAdf));
		break;
    case CANDIDATE_LIST_DEL:
		iFoundFlag = 0;
		for(i=0; i<gl_iCardAppNum; i++) {
			if((gl_aCardAppList[i].ucAdfNameLen == pAdf->ucAdfNameLen) &&
					(memcmp(gl_aCardAppList[i].sAdfName, pAdf->sAdfName, pAdf->ucAdfNameLen)==0)) {
				// found
                iFoundFlag = 1;
                break;
			}
		}
		if(iFoundFlag == 0)
			return(-1);
		for(i=i+1; i<gl_iCardAppNum; i++)
			memcpy(&gl_aCardAppList[i-1], &gl_aCardAppList[i], sizeof(gl_aCardAppList[i]));
		gl_iCardAppNum --;
		break;
    case CANDIDATE_LIST_STATUS:
		return(gl_iCardAppNum);
	}
    return(0);
}

// ���ܣ�����ģ��0x61, ��֧�ֵ�AID�����ѡ�б�
// ���룺psTlvObj61             : 0x61ģ��
//       iTlvObj61Len           : 0x61ģ�峤��
// ���أ�0	 			        : �ɹ����
//       SEL_ERR_OVERFLOW       : �洢�ռ䲻��,DDF���л�Ӧ���б�
//       SEL_ERR_DATA_ERROR     : ������ݷǷ�
//       SEL_ERR_DATA_ABSENT    : ��Ҫ���ݲ�����
static int _iSelProcessTlvObj61(char *psTlvObj61, int iTlvObj61Len)
{
    uchar  *psTlvObj, *psTlvObjValue;
    uchar  sTagRoute[10];
    uchar  ucDfLen;
    stADFInfo OneAdf;
    int  iRet;
    int  i;

    // search DDF name
    strcpy(sTagRoute, "\x61""\x9D"); // search DDF name
    iRet = iTlvSearchObj(psTlvObj61, iTlvObj61Len, 0, sTagRoute, &psTlvObj);
    if(iRet == TLV_ERR_BUF_FORMAT)
        return(SEL_ERR_DATA_ERROR);
    if(iRet != TLV_ERR_NOT_FOUND) {
        // found tag 0x9D, it's a DDF entry
        iRet = iTlvCheckTlvObject(psTlvObj);
        if(iRet < 0)
            return(SEL_ERR_DATA_ERROR);

        ucDfLen = (uchar)iTlvValue(psTlvObj, &psTlvObjValue);
        iRet = _iSelDdfQueueOp(DDF_QUEUE_IN, &ucDfLen, psTlvObjValue);
        if(iRet < 0)
            return(SEL_ERR_OVERFLOW);
        
        // search ADF name
        strcpy(sTagRoute, "\x61""\x4F"); // search ADF name
        iRet = iTlvSearchObj(psTlvObj61, iTlvObj61Len, 0, sTagRoute, &psTlvObj);
        if(iRet >= 0)
            return(SEL_ERR_DATA_ERROR); // both DDF and ADF exist
        return(0);
    }
    
    // search ADF name
    strcpy(sTagRoute, "\x61""\x4F"); // search ADF name
    iRet = iTlvSearchObj(psTlvObj61, iTlvObj61Len, 0, sTagRoute, &psTlvObj);
    if(iRet == TLV_ERR_BUF_FORMAT)
        return(SEL_ERR_DATA_ERROR);
    if(iRet == TLV_ERR_NOT_FOUND)
        return(SEL_ERR_DATA_ABSENT); // neither DDF nor ADF found, �������ݲ�����

    // �ҵ�һ��ADF���
	memset(&OneAdf, 0, sizeof(OneAdf)); // ��ʼ��
	OneAdf.iPriority = -1; // ��ʼ��
	strcpy(OneAdf.szLanguage, sg_szLanguage); // ���PSE���õ���ֵ
	OneAdf.iIssuerCodeTableIndex = sg_iIssuerCodeTableIndex; // ���PSE���õ���ֵ
    for(i=0; i<4; i++) {
        // i = 0 : ADF name
        // i = 1 : label
        // i = 2 : preferred name
        // i = 3 : priority indicator
        // û������������T73��������Ӧ��ѡ��ʱ������TBF0C
        switch(i) {
        case 0:
            strcpy(sTagRoute, "\x61""\x4F"); // search ADF name
            break;
        case 1:
            strcpy(sTagRoute, "\x61""\x50"); // search label
            break;
        case 2:
            strcpy(sTagRoute, "\x61""\x9f\x12"); // search preffered name
            break;
        case 3:
            strcpy(sTagRoute, "\x61""\x87"); // search priority indicator
            break;
        } // switch(i
        iRet = iTlvSearchObj(psTlvObj61, iTlvObj61Len, 0, sTagRoute, &psTlvObj);
        if(iRet == TLV_ERR_BUF_FORMAT)
            return(SEL_ERR_DATA_ERROR);
        if(iRet == TLV_ERR_NOT_FOUND && (i==2 || i==3)) // not found but optional, Refer to Test Case V2CL0050000v4.1a, Labelȱʧ��Ҫ����AidList����
            continue; 
        if(iRet == TLV_ERR_NOT_FOUND)
            return(SEL_ERR_DATA_ABSENT); // not found and mandatory

        iRet = iTlvCheckTlvObject(psTlvObj);
        if(iRet < 0) {
            if(i==1 || i==2)
                continue; // label or priority indicater��������ݷǷ�������
            return(SEL_ERR_DATA_ERROR);
        }

		// �������ݵ���ѡ��¼OneAdf��
        // i = 0 : ADF name
        // i = 1 : label
        // i = 2 : preferred name
        // i = 3 : priority indicator
        switch(i) {
        case 0:
			OneAdf.ucAdfNameLen = (uchar)iTlvValueLen(psTlvObj);
			memcpy(OneAdf.sAdfName, psTlvValue(psTlvObj), OneAdf.ucAdfNameLen);
            break;
        case 1:
			memcpy(OneAdf.szLabel, psTlvValue(psTlvObj), iTlvValueLen(psTlvObj));
            break;
        case 2:
			memcpy(OneAdf.szPreferredName, psTlvValue(psTlvObj), iTlvValueLen(psTlvObj));
            break;
        case 3:
			OneAdf.iPriority = *psTlvValue(psTlvObj);
            break;
        } // switch(i
    } // for(i=0; i<4; i++

	// �ж��Ƿ�֧�ִ�Ӧ��
	for(i=0; i<gl_iTermSupportedAidNum; i++) {
		if(gl_aTermSupportedAidList[i].ucASI == 0) {
			// ֧�ֲ�������ѡ��
			if(gl_aTermSupportedAidList[i].ucAidLen > OneAdf.ucAdfNameLen)
				continue; // not match in length
			if(memcmp(gl_aTermSupportedAidList[i].sAid, OneAdf.sAdfName, gl_aTermSupportedAidList[i].ucAidLen) != 0)
				continue; // not match in content
			// match
		} else {
			// ��֧�ֲ�������ѡ��
			if(gl_aTermSupportedAidList[i].ucAidLen != OneAdf.ucAdfNameLen)
				continue; // not match in length
			if(memcmp(gl_aTermSupportedAidList[i].sAid, OneAdf.sAdfName, gl_aTermSupportedAidList[i].ucAidLen) != 0)
				continue; // not match in content
			// match
		}
   		// match found, ��������ѡ�б�
		iRet = _iSelCandidateListOp(CANDIDATE_LIST_ADD, &OneAdf);
		if(iRet < 0)
    		return(SEL_ERR_OVERFLOW); // �洢�ռ���
		return(0); // �ҵ�ƥ�䣬�ѳɹ������ѡ�б�
	} // for(i=0; i<gl_EmvTermPara.uiNumSupportedAid; i++
    return(0); // û�ҵ�ƥ�䣬��֧�ִ�Ӧ��
}

// ���ܣ�����Ŀ¼�ļ�
// ���룺ucSFI                  : Ŀ¼�ļ�
// ���أ�0	 			        : �ɹ����
//       SEL_ERR_CARD           : ��ƬͨѶ��
//       SEL_ERR_CARD_SW        : ��״̬�ַǷ�
//       SEL_ERR_NOT_SUPPORT_T  : �ն˲�֧��PSE����
//       SEL_ERR_NOT_SUPPORT_C  : ��Ƭ��֧��PSE����
//       SEL_ERR_NO_REC         : Ŀ¼�ļ��޼�¼
//       SEL_ERR_OVERFLOW       : �洢�ռ䲻��,DDF���л�Ӧ���б�
//       SEL_ERR_DATA_ERROR     : ������ݷǷ�
//       SEL_ERR_DATA_ABSENT    : ��Ҫ���ݲ�����
static int _iSelProcessDirEF(uchar ucSFI)
{
    uchar  sRec[254];
    uchar  ucRecNo;               // record no of the Dir EF
    uchar  ucRecLen;              // record length
    uint   uiIndex;               // index of tag 61 in same record
    uchar  *psTlvObj61, ucTlvObj61Len;
	uchar  *p;
    uchar  sTagRoute[10];
    uint   uiRet;
    int    iRet;

	vPushApduPrepare(PUSHAPDU_READ_PSE_REC, ucSFI, 1, 255); // add by yujun 2012.10.29, ֧���Ż�pboc2.0����apdu
    
    for(ucRecNo=1; ; ucRecNo++) {
        uiRet = uiEmvCmdRdRec(ucSFI, ucRecNo, &ucRecLen, sRec);
        if(uiRet == 1)
            return(SEL_ERR_CARD);
        if(uiRet == 0x6a83) {
			if(ucRecNo == 1)
				return(SEL_ERR_NO_REC); // ��һ����¼�ͱ���֪�޴˼�¼
            return(0); // finished processing
		}
        if(uiRet)
            return(SEL_ERR_CARD_SW);

		// search Tag70
        strcpy(sTagRoute, "\x70"); // search Tag70
        iRet = iTlvSearchObj(sRec, (int)ucRecLen, 0, sTagRoute, &p);
        if(iRet == TLV_ERR_BUF_FORMAT)
            return(SEL_ERR_DATA_ERROR);
        if(iRet == TLV_ERR_NOT_FOUND)
            return(SEL_ERR_DATA_ERROR); // ��70ģ��

        for(uiIndex=0; ; uiIndex++) {
            strcpy(sTagRoute, "\x70""\x61"); // search template 0x61
            iRet = iTlvSearchObj(sRec, (int)ucRecLen, uiIndex, sTagRoute, &psTlvObj61);
            if(iRet == TLV_ERR_BUF_FORMAT)
                return(SEL_ERR_DATA_ERROR);
            if(iRet == TLV_ERR_NOT_FOUND)
                break; // no more entry
            ucTlvObj61Len = (uchar)iRet;

            iRet = _iSelProcessTlvObj61(psTlvObj61, ucTlvObj61Len);
            if(iRet < 0)
                return(iRet);
        } // for(uiIndex=0; ; uiIndex++
    } // for(ucRecNo=1; ; ucRecNo++
    
    return(0);
}

// ���ܣ���PSE��ʽ�ҳ���ѡӦ���б�
// ���أ�>=0			        : ��ɣ��ҵ���Ӧ�ø���
//       SEL_ERR_CARD           : ��ƬͨѶ��
//       SEL_ERR_CARD_SW        : ��״̬�ַǷ�
//       SEL_ERR_NOT_SUPPORT_T  : �ն˲�֧��PSE����
//       SEL_ERR_NOT_SUPPORT_C  : ��Ƭ��֧��PSE����
//       SEL_ERR_CARD_BLOCKED   : ��Ƭ�Ѿ��������ÿ�ƬӦ���ܾ��������ٳ����б�
//       SEL_ERR_PSE_ERROR      : PSE��ʽ����
//       SEL_ERR_OVERFLOW       : �洢�ռ䲻��,DDF���л�Ӧ���б�
//       SEL_ERR_DATA_ERROR     : ������ݷǷ�
//       SEL_ERR_DATA_ABSENT    : ��Ҫ���ݲ�����
//       SEL_ERR_OTHER          : ��������
// Note: �ҳ���Ӧ�÷���ȫ�ֱ���gl_aCardAppList[]��
//       �ҳ���Ӧ�ø�������gl_iCardAppNum��
static int iSelGetAidsByPse(void)
{
    uchar sDdfName[16], ucDdfNameLen;
    uchar sFci[256], ucFciLen, *psFciValue;
    uchar *psTlvObj, *psTlvObjValue;
    uchar ucSFI;
    uchar sTagRoute[10];
    uint  uiRet;
    int   iRet;
    int   i;

	_iSelCandidateListOp(CANDIDATE_LIST_INIT, NULL); // ��ʼ����ѡ�б�
	memset(sg_szLanguage, 0, sizeof(sg_szLanguage)); // ��ʼ��PSE����ѡ��
	sg_iIssuerCodeTableIndex = -1;		    		 // ��ʼ��PSE�ַ�������
    _iSelDdfQueueOp(DDF_QUEUE_INIT, 0, 0); // ��ʼ��DDF����

    // queue the PSE as the first DDF
    strcpy(sDdfName, "1PAY.SYS.DDF01");
    ucDdfNameLen = strlen(sDdfName);
    _iSelDdfQueueOp(DDF_QUEUE_IN, &ucDdfNameLen, sDdfName);

    while(_iSelDdfQueueOp(DDF_QUEUE_OUT, &ucDdfNameLen, sDdfName) == 0) {
        // DDF����δ��
        uiRet = uiEmvCmdSelect(0x00/*P2:first occurrence*/ ,
                               ucDdfNameLen, sDdfName, &ucFciLen, sFci); // ѡ���DDF
        if(uiRet == 1)
            return(SEL_ERR_CARD); // communication error
        if(!memcmp(sDdfName, "1PAY.SYS.DDF01", 14)) {
            // the current DDF is PSE
            if(uiRet == 0x6a81)
                return(SEL_ERR_CARD_BLOCKED); // ��Ƭ������Ƭ��֧��Selectָ�Ӧ��ֹEmv����
            if(uiRet == 0x6a82)
                return(SEL_ERR_NOT_SUPPORT_C); // ��Ƭ��֧��PSE��Ӧʹ��Aid�б�ʽѡ��Ӧ��
            if(uiRet == 0x6283)
                return(SEL_ERR_NOT_SUPPORT_C); // PSE������Ӧʹ��Aid�б�ʽѡ��Ӧ��
            if(uiRet)
                return(SEL_ERR_NOT_SUPPORT_C);// �κ��������أ� Ӧʹ��Aid�б�ʽѡ��Ӧ��
		} else {
			if(uiRet)
				continue; // �������PSE, �ڷ��ش���״̬��ʱ������������DDF
		}

        // check mandatory data object
	    strcpy(sTagRoute, "\x6F"); // search FCI
        iRet = iTlvSearchObjValue(sFci, (int)ucFciLen, 0, sTagRoute, &psFciValue);
	    if(iRet < 0)
		    return(SEL_ERR_DATA_ERROR);

// ����V2CA1190000v4.3aҪ������T84��TA5˳��ͬ, ����˳���� (�벻������ʱΪʲôר������˳����, ������˲���ͨ���İ����ٷ���ԭ��)
//		iRet = iTlvCheckOrder(psFciValue, iRet, "\x84""\xA5"); // check order
//		if(iRet != TLV_OK)
//			return(SEL_ERR_DATA_ERROR);

	    strcpy(sTagRoute, "\x6F""\xA5""\x88"); // search SFI of the DIR EF
        iRet = iTlvSearchObj(sFci, (int)ucFciLen, 0, sTagRoute, &psTlvObj);
	    if(iRet == TLV_ERR_BUF_FORMAT)
		    return(SEL_ERR_DATA_ERROR);
        if(iRet == TLV_ERR_NOT_FOUND)
	        return(SEL_ERR_DATA_ABSENT);
	    iRet = iTlvCheckTlvObject(psTlvObj);
        if(iRet < 0)
		    return(SEL_ERR_DATA_ERROR); // format error or length not comply with EMV2004
        ucSFI = *psTlvValue(psTlvObj);
	    if(ucSFI<1 || ucSFI>10)
		    return(SEL_ERR_DATA_ERROR); // SFI must be in the range of 1 - 10
        
        strcpy(sTagRoute, "\x6F""\x84"); // search DDF name
	    iRet = iTlvSearchObjValue(sFci, (int)ucFciLen, 0, sTagRoute, &psTlvObjValue); // ��DDF Name
		if(iRet == TLV_ERR_BUF_FORMAT)
			return(SEL_ERR_DATA_ERROR);
        if(iRet == TLV_ERR_NOT_FOUND)
	        return(SEL_ERR_DATA_ABSENT);
		if(iRet != (int)ucDdfNameLen || memcmp(sDdfName, psTlvObjValue, (int)ucDdfNameLen))
			return(SEL_ERR_DATA_ERROR); // FCI��DDF name��ѡ���õ�DDF name����
            
        if(!memcmp(sDdfName, "1PAY.SYS.DDF01", 14)) {
            // the current DDF is PSE
            // search optional data elements
            for(i=0; i<2; i++) {
                // i = 0 : Language Preference
                // i = 1 : Issuer Code Table Index
                if(i == 0)
                    strcpy(sTagRoute, "\x6F""\xA5""\x5F\x2D"); // search Language Preference
                else
                    strcpy(sTagRoute, "\x6F""\xA5""\x9F\x11"); // search Issuer code table index
                iRet = iTlvSearchObj(sFci, (int)ucFciLen, 0, sTagRoute, &psTlvObj);
                if(iRet == TLV_ERR_BUF_FORMAT)
                    return(SEL_ERR_DATA_ERROR);
                if(iRet == TLV_ERR_NOT_FOUND)
                    continue;
                iRet = iTlvCheckTlvObject(psTlvObj);
                if(iRet < 0)
                    continue; // language or issuer code table index��������ݷǷ�������
                // i = 0 : Language Preference
                // i = 1 : Issuer Code Table Index
				if(i == 0) {
					memcpy(sg_szLanguage, psTlvValue(psTlvObj), iTlvValueLen(psTlvObj));
				} else {
					sg_iIssuerCodeTableIndex = (int)*psTlvValue(psTlvObj);
					if(sg_iIssuerCodeTableIndex > 10)
						sg_iIssuerCodeTableIndex = -1; // ���ֵ�Ƿ������費����
				}
            } // for(i=0; i<2; i++
        }

        iRet = _iSelProcessDirEF(ucSFI);
		if(iRet == SEL_ERR_NO_REC) {
	        if(!memcmp(sDdfName, "1PAY.SYS.DDF01", 14))
				return(SEL_ERR_PSE_ERROR); // ���PSE��Ŀ¼�ļ�������, ����PSE��
			else
				iRet = 0; // �����PSE��Ŀ¼�ļ�������, ����Ϊ����
		}
        if(iRet < 0)
            return(iRet);
    } // while(_iSelDdfQueueOp(DDF_QUEUE_OUT, &ucDdfNameLen, sDdfName) == 0
    
    return(gl_iCardAppNum);
}

// ���ܣ���PPSE��ʽ�ҳ���ѡӦ���б�
// ���أ�>=0			        : ��ɣ��ҵ���Ӧ�ø���
//       SEL_ERR_CARD           : ��ƬͨѶ��
//       SEL_ERR_CARD_SW        : ��״̬�ַǷ�
//       SEL_ERR_CARD_BLOCKED   : ��Ƭ�Ѿ��������ÿ�ƬӦ���ܾ��������ٳ����б�
//       SEL_ERR_OVERFLOW       : �洢�ռ䲻��,DDF���л�Ӧ���б�
//       SEL_ERR_DATA_ERROR     : ������ݷǷ�
//       SEL_ERR_DATA_ABSENT    : ��Ҫ���ݲ�����
//       SEL_ERR_OTHER          : ��������
// Note: �ҳ���Ӧ�÷���ȫ�ֱ���gl_aCardAppList[]��
//       �ҳ���Ӧ�ø�������gl_iCardAppNum��
static int iSelGetAidsByPpse(void)
{
    uchar sFci[256], ucFciLen, *psFciValue;
    uchar *psTlvObj, *psTlvObjValue;
	uchar *psTlvObj61, ucTlvObj61Len;
	uint  uiIndex;
    uchar sTagRoute[10];
    stADFInfo OneAdf;
    uint  uiRet;
    int   iRet;
    int   i;

	_iSelCandidateListOp(CANDIDATE_LIST_INIT, NULL); // ��ʼ����ѡ�б�
	memset(sg_szLanguage, 0, sizeof(sg_szLanguage)); // ��ʼ��PSE����ѡ��
	sg_iIssuerCodeTableIndex = -1;		    		 // ��ʼ��PSE�ַ�������

    uiRet = uiEmvCmdSelect(0x00/*P2*/, 14, "2PAY.SYS.DDF01", &ucFciLen, sFci); // ѡ��PPSE
    if(uiRet == 1)
        return(SEL_ERR_CARD); // communication error
    if(uiRet == 0x6a81)
        return(SEL_ERR_CARD_BLOCKED); // ��Ƭ������Ƭ��֧��Selectָ�Ӧ��ֹEmv����
    if(uiRet)
        return(SEL_ERR_CARD_SW);

	// search DDF name
    strcpy(sTagRoute, "\x6F""\x84"); // search DDF name
    iRet = iTlvSearchObjValue(sFci, (int)ucFciLen, 0, sTagRoute, &psTlvObjValue); // ��DDF Name
	if(iRet == TLV_ERR_BUF_FORMAT)
		return(SEL_ERR_DATA_ERROR);
    if(iRet == TLV_ERR_NOT_FOUND)
        return(SEL_ERR_DATA_ABSENT);
	if(iRet != 14 || memcmp("2PAY.SYS.DDF01", psTlvObjValue, 14))
		return(SEL_ERR_DATA_ERROR); // FCI��DDF name��ѡ���õ�DDF name����

    // check mandatory data object
    strcpy(sTagRoute, "\x6F"); // search FCI
    iRet = iTlvSearchObjValue(sFci, (int)ucFciLen, 0, sTagRoute, &psFciValue);
    if(iRet < 0)
	    return(SEL_ERR_DATA_ERROR);

	// search T61
    for(uiIndex=0; ; uiIndex++) {
        strcpy(sTagRoute, "\x6F""\xA5""\xBF\x0C""\x61"); // search template 0x61
        iRet = iTlvSearchObj(sFci, (int)ucFciLen, uiIndex, sTagRoute, &psTlvObj61);
        if(iRet == TLV_ERR_BUF_FORMAT)
            return(SEL_ERR_DATA_ERROR);
        if(iRet == TLV_ERR_NOT_FOUND)
            break; // no more entry
        ucTlvObj61Len = (uchar)iRet;

		// T61 found, processing...
		memset(&OneAdf, 0, sizeof(OneAdf)); // ��ʼ��
		OneAdf.iPriority = -1;
		OneAdf.iIssuerCodeTableIndex = -1;

		// search ADF name
		iRet = iTlvSearchObj(psTlvObj61, ucTlvObj61Len, 0, "\x61\x4F", &psTlvObj);
		if(iRet == TLV_ERR_BUF_FORMAT)
		    return(SEL_ERR_DATA_ERROR);
		if(iRet == TLV_ERR_NOT_FOUND)
			return(SEL_ERR_DATA_ABSENT); // �������ݲ�����
		if(iTlvCheckTlvObject(psTlvObj) < 0)
			return(SEL_ERR_DATA_ERROR); // ���ݷǷ�
		OneAdf.ucAdfNameLen = (uchar)iTlvValueLen(psTlvObj);
		memcpy(OneAdf.sAdfName, psTlvValue(psTlvObj), OneAdf.ucAdfNameLen);
		// search label
		iRet = iTlvSearchObj(psTlvObj61, ucTlvObj61Len, 0, "\x61\x50", &psTlvObj);
		if(iRet == TLV_ERR_BUF_FORMAT)
		    return(SEL_ERR_DATA_ERROR);
		if(iRet > 0) {
			// �ҵ�label
			if(iTlvCheckTlvObject(psTlvObj) >= 0) {
				// ���ݺϷ�
				memcpy(OneAdf.szLabel, psTlvValue(psTlvObj), iTlvValueLen(psTlvObj));
			}
		}
		// search priority indicator
		iRet = iTlvSearchObj(psTlvObj61, ucTlvObj61Len, 0, "\x61\x87", &psTlvObj);
		if(iRet == TLV_ERR_BUF_FORMAT)
		    return(SEL_ERR_DATA_ERROR);
		if(iRet > 0) {
			// �ҵ�priority indicator
			if(iTlvCheckTlvObject(psTlvObj) >= 0) {
				// ���ݺϷ�
				OneAdf.iPriority = *psTlvValue(psTlvObj);
			}
		}

		// �ж��Ƿ�֧�ִ�Ӧ��
		for(i=0; i<gl_iTermSupportedAidNum; i++) {
			if(gl_aTermSupportedAidList[i].ucASI == 0) {
				// ֧�ֲ�������ѡ��
				if(gl_aTermSupportedAidList[i].ucAidLen > OneAdf.ucAdfNameLen)
					continue; // not match in length
				if(memcmp(gl_aTermSupportedAidList[i].sAid, OneAdf.sAdfName, gl_aTermSupportedAidList[i].ucAidLen) != 0)
					continue; // not match in content
				// match
			} else {
				// ��֧�ֲ�������ѡ��
				if(gl_aTermSupportedAidList[i].ucAidLen != OneAdf.ucAdfNameLen)
					continue; // not match in length
				if(memcmp(gl_aTermSupportedAidList[i].sAid, OneAdf.sAdfName, gl_aTermSupportedAidList[i].ucAidLen) != 0)
					continue; // not match in content
				// match
			}
   			// match found, ��������ѡ�б�
			iRet = _iSelCandidateListOp(CANDIDATE_LIST_ADD, &OneAdf);
			if(iRet < 0)
    			return(SEL_ERR_OVERFLOW); // �洢�ռ���
			break; // �ҵ�ƥ�䣬�ѳɹ������ѡ�б�
		} // for(i=0; i<gl_EmvTermPara.uiNumSupportedAid; i++
    } // for(uiIndex=0; ; uiIndex++

    return(gl_iCardAppNum);
}

// ���ܣ�����ģ��0x6F, �����Ԫ��
// ���룺psTlvObj6F             : 0x6Fģ��
//       iTlvObj6FLen           : 0x6Fģ�峤��
// �����pOneAdf                : ����ɹ����������������Է���
// ���أ�0	 			        : �ɹ����
//       SEL_ERR_DATA_ERROR     : ������ݷǷ�
//       SEL_ERR_DATA_ABSENT    : ��Ҫ���ݲ�����
//       SEL_ERR_IS_DDF         : ��DDF
static int _iSelProcessTlvObj6F(char *psTlvObj6F, int iTlvObj6FLen, stADFInfo *pOneAdf)
{
    uchar  *psTlvObj;
    uchar  sTagRoute[10];
    stADFInfo OneAdf;
	uchar  *psTlvObj6FValue;
	uchar  *psA5;
    int    iRet;
    int    i;

	// check dup
	iRet = iTlvCheckDup(psTlvObj6F, iTlvObj6FLen);
	if(iRet < 0)
		return(SEL_ERR_DATA_ERROR);
	// check order
    strcpy(sTagRoute, "\x6F"); // search FCI
    iRet = iTlvSearchObjValue(psTlvObj6F, iTlvObj6FLen, 0, sTagRoute, &psTlvObj6FValue);
    if(iRet < 0)
	    return(SEL_ERR_DATA_ERROR);

// ����V2CA1190000v4.3aҪ������T84��TA5˳��ͬ, ����˳���� (�벻������ʱΪʲôר������˳����, ������˲���ͨ���İ����ٷ���ԭ��)
//	iRet = iTlvCheckOrder(psTlvObj6FValue, iRet, "\x84""\xA5"); // check order
//	if(iRet != TLV_OK)
//		return(SEL_ERR_DATA_ERROR);

#if 0
// 4.3b���԰����Ѿ�ȡ����������������
    // ����2CL.032.00.06 2CL.032.00.07Ҫ��PDOL(T9F38)����ֱ�ӳ�����6Fģ��֮��
	strcpy(sTagRoute, "\x6F""\x9F\x38"); // search FCI
    iRet = iTlvSearchObj(psTlvObj6F, iTlvObj6FLen, 0, sTagRoute, &psTlvObj);
	if(iRet >= 0)
	    return(SEL_ERR_DATA_ERROR);
#endif

	// check A5 dup
    strcpy(sTagRoute, "\x6F""\xA5");
    iRet = iTlvSearchObj(psTlvObj6F, iTlvObj6FLen, 0, sTagRoute, &psA5);
    if(iRet < 0)
        return(SEL_ERR_DATA_ERROR);
	iRet = iTlvCheckDup(psA5, iRet);
	if(iRet < 0)
		return(SEL_ERR_DATA_ERROR);

    // search SFI of the Directory Elementary File
    strcpy(sTagRoute, "\x6F""\xA5""\x88");
    iRet = iTlvSearchObj(psTlvObj6F, iTlvObj6FLen, 0, sTagRoute, &psTlvObj);
    if(iRet == TLV_ERR_BUF_FORMAT)
        return(SEL_ERR_DATA_ERROR);
    if(iRet != TLV_ERR_NOT_FOUND) {
		// �ҵ�Tag88����Ϊ��PSE ���Ǹ� DDF
		// ������PSE��DDF����ѡ�б�
		return(SEL_ERR_IS_DDF);
	}

	// ��ADF
	// ��ȱʡֵ
	memset(&OneAdf, 0, sizeof(OneAdf)); // ��ʼ��
	OneAdf.iPriority = -1;
	OneAdf.iIssuerCodeTableIndex = -1;
    for(i=0; i<7; i++) {
        // i = 0 : ADF name
        // i = 1 : FCI Proprietary Template
        // i = 2 : label
        // i = 3 : preferred name
        // i = 4 : priority indicator
		// i = 5 : language preference
		// i = 6 : issuer code table index
        switch(i) {
        case 0:
            strcpy(sTagRoute, "\x6F""\x84"); // search ADF name
            break;
		case 1:
            strcpy(sTagRoute, "\x6F""\xA5"); // FCI Proprietary Template
            break;
        case 2:
            strcpy(sTagRoute, "\x6F""\xA5""\x50"); // search label
            break;
        case 3:
            strcpy(sTagRoute, "\x6F""\xA5""\x9f\x12"); // search preffered name
            break;
        case 4:
            strcpy(sTagRoute, "\x6F""\xA5""\x87"); // search priority indicator
            break;
        case 5:
            strcpy(sTagRoute, "\x6F""\xA5""\x5F\x2D"); // language preference
            break;
        case 6:
            strcpy(sTagRoute, "\x6F""\xA5""\x9F\x11"); // issuer code table index
            break;
        } // switch(i
        iRet = iTlvSearchObj(psTlvObj6F, iTlvObj6FLen, 0, sTagRoute, &psTlvObj);
        if(iRet == TLV_ERR_BUF_FORMAT)
            return(SEL_ERR_DATA_ERROR);
        if(iRet == TLV_ERR_NOT_FOUND && (i==2 || i==3 || i==4 || i==5 || i==6)) // label�ն���Ϊ�ǿ�ѡ
            continue; // not found but optional
        if(iRet == TLV_ERR_NOT_FOUND)
            return(SEL_ERR_DATA_ABSENT); // not found and mandatory

        iRet = iTlvCheckTlvObject(psTlvObj);
        if(iRet < 0) {
            if(i==2 || i==3)
                continue; // label or preffered name��������ݷǷ�������
            return(SEL_ERR_DATA_ERROR);
        }

		// �������ݵ���ѡ��¼OneCandidate��
        // i = 0 : ADF name
        // i = 1 : FCI Proprietary Template
        // i = 2 : label
        // i = 3 : preferred name
        // i = 4 : priority indicator
		// i = 5 : language preference
		// i = 6 : issuer code table index
        switch(i) {
        case 0:
			OneAdf.ucAdfNameLen = (uchar)iTlvValueLen(psTlvObj);
			memcpy(OneAdf.sAdfName, psTlvValue(psTlvObj), OneAdf.ucAdfNameLen);
            break;
		case 1:
			break;
        case 2:
			memcpy(OneAdf.szLabel, psTlvValue(psTlvObj), iTlvValueLen(psTlvObj));
            break;
        case 3:
			memcpy(OneAdf.szPreferredName, psTlvValue(psTlvObj), iTlvValueLen(psTlvObj));
            break;
        case 4:
			OneAdf.iPriority = *psTlvValue(psTlvObj);
            break;
        case 5:
			memcpy(OneAdf.szLanguage, psTlvValue(psTlvObj), iTlvValueLen(psTlvObj));
			break;
        case 6:
			OneAdf.iIssuerCodeTableIndex = *psTlvValue(psTlvObj);
			break;
        } // switch(i
    } // for(i=0; i<7; i++

    memcpy(pOneAdf, &OneAdf, sizeof(OneAdf));
    return(0);
}

// ���ܣ���AID LIST��ʽ�ҳ���ѡӦ���б�
// ����: iIgnoreBlock			: !0:����Ӧ��������־, 0:������Ӧ�ò��ᱻѡ��
// ���أ�>=0	 		        : ��ɣ��ҵ���Ӧ�ø���
//       SEL_ERR_CARD           : ��ƬͨѶ��
//       SEL_ERR_CARD_BLOCKED   : ��Ƭ�Ѿ��������ÿ�ƬӦ���ܾ��������ٳ���
//       SEL_ERR_OVERFLOW       : �洢�ռ䲻��,DDF���л�Ӧ���б�
//       SEL_ERR_DATA_ERROR     : Ӧ��������ݷǷ�
//       SEL_ERR_DATA_ABSENT    : ��Ҫ���ݲ�����
// Note: ****** �����������κδ��󣬶�Ҫ��ֹemv���� ******
static int iSelGetAidsByAidList(int iIgnoreBlock)
{
    uchar sDfName[16], ucDfNameLen;
    uchar sFci[256], ucFciLen;
    uchar *psTlvObj, *psTlvObjValue;
    uchar sTagRoute[10];
    stADFInfo OneAdf;
	uchar ucAppBlockFlag; // 0:app not blocked, 1:app blocked
	uchar ucP2;           // ѡ��Ӧ��ʱ��������ѡ��һ��APP����ѡ��һ��APP��IC��ָ�����P2
    uint  uiRet;
    int   iRet;
    int   i;

	_iSelCandidateListOp(CANDIDATE_LIST_INIT, NULL); // ��ʼ����ѡ�б�
	vPushApduPrepare(PUSHAPDU_SELECT_BY_AID_LIST); // add by yujun 2012.10.29, ֧���Ż�pboc2.0����apdu
	for(i=0, ucP2=0x00; i<gl_iTermSupportedAidNum;) {
		// ���γ���ѡ��֧�ֵ�Ӧ��
		uiRet = uiEmvCmdSelect(ucP2,
							   gl_aTermSupportedAidList[i].ucAidLen,
							   gl_aTermSupportedAidList[i].sAid,
							   &ucFciLen, sFci);
		if(uiRet == 1)
			return(SEL_ERR_CARD);
		if(uiRet==0x6A81 && i==0 && ucP2==0x00)
			return(SEL_ERR_CARD_BLOCKED); // ֻ�ڵ�һ��ѡ��Ӧ��ʱ�ż�鿨Ƭ�������

   		if(uiRet!=0 && uiRet!=0x6283/*0x6283=app blocked*/) {
            // Ӧ��û�ҵ�
			ucP2 = 0x00;
    		i ++;
		    continue;
		}
		if(uiRet == 0x6283)
			ucAppBlockFlag = 1; // ���µ�ǰӦ��������־
		else
			ucAppBlockFlag = 0;

		// process FCI
        strcpy(sTagRoute, "\x6F""\x84"); // search DF Name
        iRet = iTlvSearchObj(sFci, (int)ucFciLen, 0, sTagRoute, &psTlvObj);
        if(iRet==TLV_ERR_BUF_FORMAT || iRet==TLV_ERR_NOT_FOUND) {
			// ���������ؼ�����û�ҵ�������ӱ�aid������
   			ucP2 = 0x00; // ��һ��֧�ֵ�Ӧ�ã����¿�ʼ
			i ++;
     		continue;
		}
        iRet = iTlvCheckTlvObject(psTlvObj);
        if(iRet < 0) {
			// �ؼ����ݸ�ʽ���󣬲���ӱ�aid������
   			ucP2 = 0x00; // ��һ��֧�ֵ�Ӧ�ã����¿�ʼ
			i ++;
     		continue;
		}
		ucDfNameLen = (uchar)iTlvValue(psTlvObj, &psTlvObjValue); // ��DF Name
		memcpy(sDfName, psTlvObjValue, ucDfNameLen);
		if(ucDfNameLen < gl_aTermSupportedAidList[i].ucAidLen) {
			// ADF���ֳ���С��ѡ���Aid����, ����ӱ�Aid, ����
   			ucP2 = 0x00; // ��һ��֧�ֵ�Ӧ�ã����¿�ʼ
			i ++;
     		continue;
		}
		if(memcmp(sDfName, gl_aTermSupportedAidList[i].sAid, gl_aTermSupportedAidList[i].ucAidLen)!=0) {
			// ADF���ּȲ���ȫƥ���ֲ�����ƥ��, ����ӱ�Aid, ����
   			ucP2 = 0x00; // ��һ��֧�ֵ�Ӧ�ã����¿�ʼ
			i ++;
     		continue;
		}
        if(ucDfNameLen==gl_aTermSupportedAidList[i].ucAidLen) {
			// ��ȫ����ƥ��
			if(ucAppBlockFlag && !iIgnoreBlock) {
				// Ӧ�ñ�������ָ��Ӧ�ñ����󲻱�ѡ��
    			ucP2 = 0x00; // ��һ��֧�ֵ�Ӧ�ã����¿�ʼ
				i ++;
	     		continue;
			}
			iRet = _iSelProcessTlvObj6F(sFci, ucFciLen, &OneAdf); // ��������
			if(iRet < 0) {
				// ��������, ����
    			ucP2 = 0x00; // ��һ��֧�ֵ�Ӧ�ã����¿�ʼ
				i ++;
				continue;
			}
/*
            if(strlen(OneAdf.szLabel) == 0) {
				strcpy(OneAdf.szLabel, gl_aTermSupportedAidList[i].szLabel); // ���tag6F��û��label�����ն˲����и�Ӧ�õ�label����
            }
*/
        	// ���ӵ���ѡ�б�
	        iRet = _iSelCandidateListOp(CANDIDATE_LIST_ADD, &OneAdf); // ���ӵ���ѡ�б�
	        if(iRet < 0)
   		        return(SEL_ERR_OVERFLOW); // �洢�ռ���
			ucP2 = 0x00; // ��һ��֧�ֵ�Ӧ�ã����¿�ʼ
			i ++;
			continue;
		}
        // ����ƥ��
		if(gl_aTermSupportedAidList[i].ucASI != 0) {
			// ��AID��֧�ֲ�������ѡ��
			ucP2 = 0x00; // ��һ��֧�ֵ�Ӧ�ã����¿�ʼ
			i ++;
			continue;
		}

		// ��AID֧�ֲ�������ѡ��
		ucP2 = 0x02; // �´μ����ò�������ѡ��
		if(ucAppBlockFlag==0 || iIgnoreBlock) {
			// Ӧ�����û�б�����ָ���������������ӵ���ѡ�б�
    		iRet = _iSelProcessTlvObj6F(sFci, ucFciLen, &OneAdf); // ��������
	    	if(iRet < 0)
				continue; // ��������, ����
/*
            if(strlen(OneAdf.szLabel) == 0) {
				strcpy(OneAdf.szLabel, gl_aTermSupportedAidList[i].szLabel); // ���tag6F��û��label�����ն˲����и�Ӧ�õ�label����
            }
*/
        	// ���ӵ���ѡ�б�
	        iRet = _iSelCandidateListOp(CANDIDATE_LIST_ADD, &OneAdf); // ���ӵ���ѡ�б�
	        if(iRet < 0)
   		        return(SEL_ERR_OVERFLOW); // �洢�ռ���
		}
	} // for(i=0; i<gl_EmvTermPara.uiNumSupportedAid; i++

    return(gl_iCardAppNum);
}

// �Ƚ����ȼ�
// ���� : iP1  : ���ȼ�1
//        iP2  : ���ȼ�2
// ���� : 0    : ���
//        1    : iP1�����ȼ�����iP2
//        -1   : iP1�����ȼ�С��iP2
static int _iSelComparePriority(int iP1, int iP2)
{
	if((iP1==-1 || (iP1&0x0F)==0) && (iP2==-1 || (iP2&0x0F)==0))
		return(0); // iP1��iP2�����������ȼ��������в����������ȼ�����Ϊ�������ȼ����
	if(iP2==-1 || (iP2&0x0F)==0)
		return(1); // iP2���������ȼ��������в����������ȼ���iP1����
	if(iP1==-1 || (iP1&0x0F)==0)
		return(-1); // iP1���������ȼ��������в����������ȼ���iP2����

	iP1 &= 0x0F;
	iP2 &= 0x0F;
	if(iP1 < iP2)
		return(1); // ucP1�����ȼ���
	if(iP1 > iP2)
		return(-1); // ucP1�����ȼ�С
	return(0); // ���
}

// ���ܣ�����ѡ�б�����(gl_aCardAppList[])
// ���أ�>=0 : ��ɣ����к�ѡӦ�ø���
static int iSelSortCandidate(void)
{
    stADFInfo OneAdf;
	int iMax;
	int i, j;

	for(i=0; i<gl_iCardAppNum; i++) {
		iMax = i;
		for(j=i+1; j<gl_iCardAppNum; j++) {
			if(_iSelComparePriority(gl_aCardAppList[i].iPriority, gl_aCardAppList[j].iPriority) < 0) {
				// [j]�����ȼ�����[i]�����ȼ�������
				memcpy(&OneAdf, &gl_aCardAppList[i], sizeof(OneAdf));
				memcpy(&gl_aCardAppList[i], &gl_aCardAppList[j], sizeof(OneAdf));
				memcpy(&gl_aCardAppList[j], &OneAdf, sizeof(OneAdf));
			}
		}
	}
    return(gl_iCardAppNum);
}

// ���ܣ���PSE+AidList��ʽ�ҳ���ѡӦ���б�
// ����: iIgnoreBlock			: !0:����Ӧ��������־, 0:������Ӧ�ò��ᱻѡ��
// ���أ�>=0			        : ��ɣ��ҵ���Ӧ�ø���
//       SEL_ERR_CARD           : ��ƬͨѶ��
//       SEL_ERR_CARD_SW        : �Ƿ�״̬��
//       SEL_ERR_CARD_BLOCKED   : ��Ƭ�Ѿ��������ÿ�ƬӦ���ܾ��������ٳ����б�
//       SEL_ERR_OVERFLOW       : �洢�ռ䲻��,DDF���л�Ӧ���б�
//       SEL_ERR_DATA_ERROR     : Ӧ��������ݷǷ�
//       SEL_ERR_DATA_ABSENT    : ��Ҫ���ݲ�����
// Note: �ҳ���Ӧ�÷���ȫ�ֱ���gl_aCardAppList[]��
//       �ҳ���Ӧ�ø�������gl_iCardAppNum��
int iSelGetAids(int iIgnoreBlock)
{
	int iRet;

	// ��ʼ��FCI��ʱ���ݿ�
	iTlvSetBuffer(sg_sFciTlvBuf, sizeof(sg_sFciTlvBuf));
	sg_iCardHolderConfirmed = 0;   // ��ʼ���ֿ���ȷ�ϱ��

	if(gl_iCardType == EMV_CONTACT_CARD) {
		// �Ӵ���
		// ������pse������ȡӦ���б�
		iRet = iSelGetAidsByPse();
		// ����V2CB0230111v4.1a˵��, ����¼ʱ��Ƭ����״̬�ַǷ�������ֹ����, ҪתΪAidList��ʽ
		if(iRet==SEL_ERR_CARD || /*iRet==SEL_ERR_CARD_SW ||*/ iRet==SEL_ERR_CARD_BLOCKED || iRet==SEL_ERR_OVERFLOW) {
			// pse��ʽ��ȡaid�б�ʱ���������������ս�emv����
			//   ��ƬͨѶ��   ||       �Ƿ�״̬��        ||       ��Ƭ�Ѿ�����           ||       DDF���д洢�ռ䲻��
			return(iRet);
		}
		if(iRet <= 0) {
			// ���pse������ȡӦ���б�ʧ�ܣ�������aid�б�
			iRet = iSelGetAidsByAidList(iIgnoreBlock);
			if(iRet < 0) {
				// aid�б���ȡaid�б�ʱ�����������ս�emv����
				return(iRet);
			}
		}
	} else {
		// �ǽӿ�
		// ��ppse������ȡӦ���б�
		iRet = iSelGetAidsByPpse();
		if(iRet==SEL_ERR_CARD || iRet==SEL_ERR_CARD_SW || iRet==SEL_ERR_CARD_BLOCKED || iRet==SEL_ERR_OVERFLOW) {
			// pse��ʽ��ȡaid�б�ʱ���������������ս�emv����
			//   ��ƬͨѶ��   ||       �Ƿ�״̬��      ||       ��Ƭ�Ѿ�����         ||       DDF���д洢�ռ䲻��
			return(iRet);
		}
	}

	// �ɹ�����������(ע��������ƥ�䣬��iRet==0)
	iRet = iSelSortCandidate();
    sg_iTotalMatchedApps = iRet; // ��Ƭ���ն����ͬʱ֧�ֵ�Ӧ�ø���
	return(iRet); // ���ػ�ȡ��aid����
}

// ���ܣ���gl_aCardAppList[]��ɾ��һ��Ӧ��
// ���룺ucAidLen       : Ҫɾ����aid����
//       psAid          : Ҫɾ����aid
// ���أ�>=0	 		: ��ɣ�ʣ���Ӧ�ø���
//       SEL_ERR_NO_APP : û�ҵ�Ҫɾ����Ӧ��
int iSelDelCandidate(uchar ucAidLen, uchar *psAid)
{
	int   i;

	for(i=0; i<gl_iCardAppNum; i++) {
		if(ucAidLen == gl_aCardAppList[i].ucAdfNameLen) {
			if(memcmp(psAid, gl_aCardAppList[i].sAdfName, ucAidLen) == 0)
				break;
		}
	}
	if(i >= gl_iCardAppNum)
		return(SEL_ERR_NO_APP); // û�ҵ�Ҫɾ����Ӧ��

	// iΪҪɾ��Ӧ�õ��±�
	for(; i<gl_iCardAppNum-1; i++)
		memcpy(&gl_aCardAppList[i], &gl_aCardAppList[i+1], sizeof(stADFInfo));
	memset(&gl_aCardAppList[gl_iCardAppNum-1], 0, sizeof(stADFInfo));
	gl_iCardAppNum --;
	return(0);
}

// ���ܣ���ѡӦ��
// ���룺pAdfInfo               : Ҫ��ѡ��Ӧ��
//       iIgnoreBlock			: !0:����Ӧ��������־, 0:������Ӧ�ò��ᱻѡ��
// ���أ�0	 		            : �ɹ�
//       SEL_ERR_CARD           : ��ƬͨѶ��
//       SEL_ERR_DATA_ERROR     : ������ݷǷ�
//       SEL_ERR_DATA_ABSENT    : ��Ҫ���ݲ�����
//		 SEL_ERR_APP_BLOCKED    : Ӧ���Ѿ�����
//       SEL_ERR_OTHER          : ��������
// Note: ����ú����ɹ�ִ�У�ѡ�е�Ӧ���й����ݻᱣ�浽TLV���ݿ�gl_sTlvDbCard[]��sg_sFciTlvBuf��
static int _iSelTrySelect(stADFInfo *pAdfInfo, int iIgnoreBlock)
{
	int   i;
	stADFInfo AdfInfo; // ��ѡ���Ӧ�÷��ص�AdfInfo��ע��:pAdfInfo������pse��ȡ������
    uchar sFci[256], ucFciLen;
    uchar *psTlvObj;
    uchar sTagRoute[10];
	uchar sBuf[10];
    uint  uiRet;
	int   iLen;
    int   iRet, iRet2;
	int   iAppBlockFlag;

	iAppBlockFlag = 0;
	// ɾ���ϴο�����ӵ�gl_sTlvDbCard[]���ݿ��е�Ӧ��FCI Tag���ݣ���sg_sFciTlvBufΪ����
	for(i=0; ; i++) {
		iRet = iTlvGetObjByIndex(sg_sFciTlvBuf, i, &psTlvObj);
		if(iRet < 0)
			break;
		iTlvDelObj(gl_sTlvDbCard, psTlvObj);
	}
	// ɾ���ϴο�����ӵ�gl_sTlvDbTermVar[]���ݿ��е��ն����� TAG_9F06_AID
	iTlvDelObj(gl_sTlvDbTermVar, TAG_9F06_AID);
	// ��ʼ��Fci��ʱTlv���ݿ�sg_sFciTlvBuf[]
	iTlvSetBuffer(sg_sFciTlvBuf, sizeof(sg_sFciTlvBuf));

	// ѡ��Ӧ��
	uiRet = uiEmvCmdSelect(0x00/*P2*/, pAdfInfo->ucAdfNameLen, pAdfInfo->sAdfName, &ucFciLen, sFci);
	if(uiRet == 1)
		return(SEL_ERR_CARD);
	if(uiRet==0x6283) {
	    // Ӧ�ñ���
	    if(!iIgnoreBlock)
	        return(SEL_ERR_APP_BLOCKED); // Ҫ�󲻺���Ӧ������, ����Ӧ�ñ���
	    else
    		iAppBlockFlag = 1; // Ҫ�����Ӧ������, ��¼������־, ����
	}
	else if(uiRet)
		return(SEL_ERR_OTHER);

	// process FCI
	iRet = _iSelProcessTlvObj6F(sFci, ucFciLen, &AdfInfo);
   	if(iRet < 0)
    	return(SEL_ERR_OTHER);

	// �ж�Ӧ�÷��ص�AdfName������ѡ��Ӧ�õ�Aid�Ƿ���ͬ
	// refer Emv2008 book1 P148
	if(pAdfInfo->ucAdfNameLen != AdfInfo.ucAdfNameLen)
    	return(SEL_ERR_OTHER);
	if(memcmp(pAdfInfo->sAdfName, AdfInfo.sAdfName, pAdfInfo->ucAdfNameLen) != 0)
    	return(SEL_ERR_OTHER);

	// �����������
	// T9F06
	iRet = iTlvMakeAddObj(gl_sTlvDbTermVar, TAG_9F06_AID, pAdfInfo->ucAdfNameLen, pAdfInfo->sAdfName, TLV_CONFLICT_REPLACE);
	ASSERT(iRet >= 0);
	if(iRet < 0)
    	return(SEL_ERR_OTHER);
	// T84
	iRet = iTlvMakeAddObj(gl_sTlvDbCard, TAG_84_DFName, AdfInfo.ucAdfNameLen, AdfInfo.sAdfName, TLV_CONFLICT_REPLACE);
	ASSERT(iRet >= 0);
	iRet2= iTlvMakeAddObj(sg_sFciTlvBuf, TAG_84_DFName, AdfInfo.ucAdfNameLen, AdfInfo.sAdfName, TLV_CONFLICT_REPLACE);
	ASSERT(iRet2 >= 0);
	if(iRet<0 || iRet2<0)
    	return(SEL_ERR_OTHER);
	// T4F, ��T84������T4F�ٴ洢һ��
	iRet = iTlvMakeAddObj(gl_sTlvDbCard, TAG_4F_AID, AdfInfo.ucAdfNameLen, AdfInfo.sAdfName, TLV_CONFLICT_REPLACE);
	ASSERT(iRet >= 0);
	iRet2= iTlvMakeAddObj(sg_sFciTlvBuf, TAG_4F_AID, AdfInfo.ucAdfNameLen, AdfInfo.sAdfName, TLV_CONFLICT_REPLACE);
	ASSERT(iRet2 >= 0);
	if(iRet<0 || iRet2<0)
    	return(SEL_ERR_OTHER);
	// T50
	if(strlen(AdfInfo.szLabel)) {
		// T50��ȻΪ��ѡ����淶Ҫ���ն���û�ҵ�T50ʱ��Ȼ����
		iRet = iTlvMakeAddObj(gl_sTlvDbCard, TAG_50_AppLabel, strlen(AdfInfo.szLabel), AdfInfo.szLabel, TLV_CONFLICT_REPLACE);
    	ASSERT(iRet >= 0);
		iRet2= iTlvMakeAddObj(sg_sFciTlvBuf, TAG_50_AppLabel, strlen(AdfInfo.szLabel), AdfInfo.szLabel, TLV_CONFLICT_REPLACE);
	    ASSERT(iRet2 >= 0);
		if(iRet<0 || iRet2<0)
			return(SEL_ERR_OTHER);
	}
	// T9F12
	if(strlen(AdfInfo.szPreferredName)) {
		iRet = iTlvMakeAddObj(gl_sTlvDbCard, TAG_9F12_AppPrefName, strlen(AdfInfo.szPreferredName), AdfInfo.szPreferredName, TLV_CONFLICT_REPLACE);
        ASSERT(iRet >= 0);
		iRet2= iTlvMakeAddObj(sg_sFciTlvBuf, TAG_9F12_AppPrefName, strlen(AdfInfo.szPreferredName), AdfInfo.szPreferredName, TLV_CONFLICT_REPLACE);
	    ASSERT(iRet2 >= 0);
		if(iRet<0 || iRet2<0)
			return(SEL_ERR_OTHER);
	}
	// T9F11
	if(AdfInfo.iIssuerCodeTableIndex >= 0) {
		sBuf[0] = (uchar)AdfInfo.iIssuerCodeTableIndex;
		iRet = iTlvMakeAddObj(gl_sTlvDbCard, TAG_9F11_IssuerCodeTableIndex, 1, sBuf, TLV_CONFLICT_REPLACE);
    	ASSERT(iRet >= 0);
		iRet2= iTlvMakeAddObj(sg_sFciTlvBuf, TAG_9F11_IssuerCodeTableIndex, 1, sBuf, TLV_CONFLICT_REPLACE);
    	ASSERT(iRet2 >= 0);
		if(iRet<0 || iRet2<0)
			return(SEL_ERR_OTHER);
	}
	// T5F2D
	if(strlen(AdfInfo.szLanguage)) {
		iRet = iTlvMakeAddObj(gl_sTlvDbCard, TAG_5F2D_LanguagePrefer, strlen(AdfInfo.szLanguage), AdfInfo.szLanguage, TLV_CONFLICT_REPLACE);
    	ASSERT(iRet >= 0);
		iRet2= iTlvMakeAddObj(sg_sFciTlvBuf, TAG_5F2D_LanguagePrefer, strlen(AdfInfo.szLanguage), AdfInfo.szLanguage, TLV_CONFLICT_REPLACE);
    	ASSERT(iRet2 >= 0);
		if(iRet<0 || iRet2<0)
			return(SEL_ERR_OTHER);
	}
	// T87
	if(AdfInfo.iPriority >= 0) {
		sBuf[0] = (uchar)AdfInfo.iPriority;
		iRet = iTlvMakeAddObj(gl_sTlvDbCard, TAG_87_AppPriorIndicator, 1, sBuf, TLV_CONFLICT_REPLACE);
    	ASSERT(iRet >= 0);
		iRet2= iTlvMakeAddObj(sg_sFciTlvBuf, TAG_87_AppPriorIndicator, 1, sBuf, TLV_CONFLICT_REPLACE);
    	ASSERT(iRet2 >= 0);
		if(iRet<0 || iRet2<0)
			return(SEL_ERR_OTHER);
	}

	// ������������
    // search PDOL
    strcpy(sTagRoute, "\x6F""\xA5""\x9F\x38"); // search T9F38
    iRet = iTlvSearchObj(sFci, ucFciLen, 0, sTagRoute, &psTlvObj);
    if(iRet == TLV_ERR_BUF_FORMAT)
        return(SEL_ERR_DATA_ERROR);
    if(iRet > 0) {
	    // �ҵ�PDOL
		iRet = iTlvCheckTlvObject(psTlvObj);
	    if(iRet < 0)
		    return(SEL_ERR_DATA_ERROR);
		iRet = iTlvAddObj(gl_sTlvDbCard, psTlvObj, TLV_CONFLICT_REPLACE);
		iRet2= iTlvAddObj(sg_sFciTlvBuf, psTlvObj, TLV_CONFLICT_REPLACE);
		if(iRet<0 || iRet2<0)
			return(SEL_ERR_OTHER);
	}

    // search TBF0C
    strcpy(sTagRoute, "\x6F""\xA5""\xBF\x0C");
    iRet = iTlvSearchObj(sFci, ucFciLen, 0, sTagRoute, &psTlvObj);
    if(iRet == TLV_ERR_BUF_FORMAT)
        return(SEL_ERR_DATA_ERROR);
    if(iRet > 0) {
	    // �ҵ�TBF0C
		iRet = iTlvCheckTlvObject(psTlvObj);
	    if(iRet < 0)
		    return(SEL_ERR_DATA_ERROR);
		// ���TBF0Cģ���ڵ����ж������ݿ�
		iLen = iRet;
		iRet = iTlvBatchAddObj(1/*0:ֻ��ӻ���TLV����!0:�������TLV����*/, gl_sTlvDbCard, psTlvObj, iLen, TLV_CONFLICT_ERR, 0);
    	ASSERT(iRet >= 0);
		iRet2= iTlvBatchAddObj(1/*0:ֻ��ӻ���TLV����!0:�������TLV����*/, sg_sFciTlvBuf, psTlvObj, iLen, TLV_CONFLICT_ERR, 0);
    	ASSERT(iRet2 >= 0);
		if(iRet<0 || iRet2<0)
			return(SEL_ERR_OTHER);
	}
	if(iAppBlockFlag)
		return(SEL_ERR_APP_BLOCKED); // Ӧ��������־����, ����Ӧ�ñ���״̬
    return(0);
}

// ���ܣ���ʼ��gl_sTlvDbTermVar���ݿ�, ��gl_sTlvDbTermFixed���Ѿ��������ݵ��ն�/AID�����������Ƶ�gl_sTlvDbTermVar��
// ���أ�0	 		            : �ɹ�
//       SEL_ERR_OTHER          : ��������
static int iInitTermDbVarWithCommonPara(void)
{
	uchar *pTlvObj;
	int   iRet, iTlvObjLen;
	int   iNum, i;
	uchar asTagList[30][3];

	iNum = 0;
	strcpy(asTagList[iNum++], TAG_9F09_AppVerTerm);
	strcpy(asTagList[iNum++], TAG_DFXX_TACDefault);
	strcpy(asTagList[iNum++], TAG_DFXX_TACDenial);
	strcpy(asTagList[iNum++], TAG_DFXX_TACOnline);
	strcpy(asTagList[iNum++], TAG_9F1B_TermFloorLimit);
	strcpy(asTagList[iNum++], TAG_DFXX_MaxTargetPercentage);
	strcpy(asTagList[iNum++], TAG_DFXX_TargetPercentage);
	strcpy(asTagList[iNum++], TAG_DFXX_RandomSelThreshold);
	strcpy(asTagList[iNum++], TAG_DFXX_DefaultDDOL);
	strcpy(asTagList[iNum++], TAG_DFXX_DefaultTDOL);
	strcpy(asTagList[iNum++], TAG_DFXX_ForceOnlineSupport);
	strcpy(asTagList[iNum++], TAG_DFXX_ForceAcceptSupport);
	strcpy(asTagList[iNum++], TAG_9F1D_TermRistManaData);
	strcpy(asTagList[iNum++], TAG_DFXX_ECTermSupportIndicator);
	strcpy(asTagList[iNum++], TAG_9F7B_ECTermTransLimit);
	strcpy(asTagList[iNum++], TAG_DFXX_OnlinePINSupport);

	for(i=0; i<iNum; i++) {
		iTlvObjLen = iTlvGetObj(gl_sTlvDbTermFixed, asTagList[i], &pTlvObj);
		if(iTlvObjLen >= 0) {
			iRet = iTlvAddObj(gl_sTlvDbTermVar, pTlvObj, TLV_CONFLICT_REPLACE);
			if(iRet < 0)
				return(SEL_ERR_OTHER);
		} else {
			iTlvDelObj(gl_sTlvDbTermVar, asTagList[i]);
		}
	}
	return(0);
}

// ���ܣ���ȡѡ�е�Ӧ����ز���
// ���룺pAdfInfo               : ѡ�е�Ӧ��
// ���أ�0	 		            : �ɹ�
//       SEL_ERR_OTHER          : ��������
// Note: �������浽gl_sTlvDbTermVar��
static int iSelGetAidPara(stADFInfo *pAdfInfo)
{
	stTermAid *pMatchedAid;
	int   i;
   	int   iRet;
	uchar sBuf[256];

	// �ҵ�ѡ�е�AID
	for(i=0; i<gl_iTermSupportedAidNum; i++) {
		if(gl_aTermSupportedAidList[i].ucASI == 0) {
			// ��������ƥ��
			if(gl_aTermSupportedAidList[i].ucAidLen <= pAdfInfo->ucAdfNameLen) {
				if(memcmp(gl_aTermSupportedAidList[i].sAid, pAdfInfo->sAdfName, gl_aTermSupportedAidList[i].ucAidLen)==0)
					break; // ƥ��
			}
		} else {
			// ȫ������ƥ��
			if(gl_aTermSupportedAidList[i].ucAidLen == pAdfInfo->ucAdfNameLen) {
				if(memcmp(gl_aTermSupportedAidList[i].sAid, pAdfInfo->sAdfName, gl_aTermSupportedAidList[i].ucAidLen)==0)
					break; // ƥ��
			}
		}
	}
	if(i >= gl_iTermSupportedAidNum)
		return(SEL_ERR_OTHER); // ��ȻΪѡ�е�Ӧ��, �������Ҳ���ƥ��
	pMatchedAid = &gl_aTermSupportedAidList[i];

	// ��ʼ���ն�/AID������������, ������gl_sTlvDbTermVar���ݿ���
	iRet = iInitTermDbVarWithCommonPara();
	if(iRet)
		return(SEL_ERR_OTHER);

	// ��Aid���в�����ӵ�gl_sTlvDbTermVar��
	// �����ͻ, ��Ϊ�ò���Ϊ�ն�ͨ�ò���, ����
	// T9F09, �ն�Ӧ�ð汾��
	if(pMatchedAid->ucTermAppVerExistFlag) {
		iRet = iTlvMakeAddObj(gl_sTlvDbTermVar, TAG_9F09_AppVerTerm, 2, pMatchedAid->sTermAppVer, TLV_CONFLICT_ERR);
		ASSERT(iRet>0 || iRet==TLV_ERR_CONFLICT);
		if(iRet<0 && iRet!=TLV_ERR_CONFLICT)
			return(SEL_ERR_OTHER);
	}

	// TDFxx, Default DDOL(TAG_DFXX_DefaultDDOL)
	if(pMatchedAid->iDefaultDDOLLen >= 0) {
		if(pMatchedAid->iDefaultDDOLLen > 252)
			return(SEL_ERR_OTHER);
		iRet = iTlvMakeAddObj(gl_sTlvDbTermVar, TAG_DFXX_DefaultDDOL, pMatchedAid->iDefaultDDOLLen, pMatchedAid->sDefaultDDOL, TLV_CONFLICT_ERR);
		ASSERT(iRet>0 || iRet==TLV_ERR_CONFLICT);
		if(iRet<0 && iRet!=TLV_ERR_CONFLICT)
			return(SEL_ERR_OTHER);
	}

	// TDFxx, Default TDOL(TAG_DFXX_DefaultTDOL)
	if(pMatchedAid->iDefaultTDOLLen >= 0) {
		if(pMatchedAid->iDefaultTDOLLen > 252)
			return(SEL_ERR_OTHER);
		iRet = iTlvMakeAddObj(gl_sTlvDbTermVar, TAG_DFXX_DefaultTDOL, pMatchedAid->iDefaultTDOLLen, pMatchedAid->sDefaultTDOL, TLV_CONFLICT_ERR);
		ASSERT(iRet>0 || iRet==TLV_ERR_CONFLICT);
		if(iRet<0 && iRet!=TLV_ERR_CONFLICT)
			return(SEL_ERR_OTHER);
	}

	// TDFxx, Maximum Target Percentage to be used for Biased Random Selection��-1:�޴�����(TAG_DFXX_MaxTargetPercentage)
	if(pMatchedAid->iMaxTargetPercentage >= 0) {
		if(pMatchedAid->iMaxTargetPercentage<0 || pMatchedAid->iMaxTargetPercentage>99)
			return(SEL_ERR_OTHER);
		if(pMatchedAid->iMaxTargetPercentage < pMatchedAid->iTargetPercentage)
			return(SEL_ERR_OTHER); // iMaxTergetPercentage����Ҫ��iTargetPercentageһ����
		sBuf[0] = (uchar)pMatchedAid->iMaxTargetPercentage;
		iRet = iTlvMakeAddObj(gl_sTlvDbTermVar, TAG_DFXX_MaxTargetPercentage, 1, sBuf, TLV_CONFLICT_ERR);
		ASSERT(iRet>0 || iRet==TLV_ERR_CONFLICT);
		if(iRet<0 && iRet!=TLV_ERR_CONFLICT)
			return(SEL_ERR_OTHER);
	}

	// TDFxx, Target Percentage to be used for Random Selection��-1:�޴�����(TAG_DFXX_TargetPercentage)
	if(pMatchedAid->iTargetPercentage >= 0) {
		if(pMatchedAid->iTargetPercentage<0 || pMatchedAid->iTargetPercentage>99)
			return(SEL_ERR_OTHER);
		sBuf[0] = (uchar)pMatchedAid->iTargetPercentage;
		iRet = iTlvMakeAddObj(gl_sTlvDbTermVar, TAG_DFXX_TargetPercentage, 1, sBuf, TLV_CONFLICT_ERR);
		ASSERT(iRet>0 || iRet==TLV_ERR_CONFLICT);
		if(iRet<0 && iRet!=TLV_ERR_CONFLICT)
			return(SEL_ERR_OTHER);
	}

	// T9F1B, Terminal Floor Limit
	if(pMatchedAid->ucFloorLimitExistFlag) {
		vLongToStr(pMatchedAid->ulFloorLimit, 4, sBuf);
		iRet = iTlvMakeAddObj(gl_sTlvDbTermVar, TAG_9F1B_TermFloorLimit, 4, sBuf, TLV_CONFLICT_ERR);
		ASSERT(iRet>0 || iRet==TLV_ERR_CONFLICT);
		if(iRet<0 && iRet!=TLV_ERR_CONFLICT)
			return(SEL_ERR_OTHER);
	}

	// TDFxx, Threshold Value for Biased Random Selection
	if(pMatchedAid->ucThresholdExistFlag) {
		if(pMatchedAid->ulThresholdValue >= pMatchedAid->ulFloorLimit)
			return(SEL_ERR_OTHER); // Threshold����С��FloorLimit
		vLongToStr(pMatchedAid->ulThresholdValue, 4, sBuf);
		iRet = iTlvMakeAddObj(gl_sTlvDbTermVar, TAG_DFXX_RandomSelThreshold, 4, sBuf, TLV_CONFLICT_ERR);
		ASSERT(iRet>0 || iRet==TLV_ERR_CONFLICT);
		if(iRet<0 && iRet!=TLV_ERR_CONFLICT)
			return(SEL_ERR_OTHER);
	}

	// TDFxx, TAC-Default(TAG_DFXX_TACDefault)
	if(pMatchedAid->ucTacDefaultExistFlag) {
		iRet = iTlvMakeAddObj(gl_sTlvDbTermVar, TAG_DFXX_TACDefault, 5, pMatchedAid->sTacDefault, TLV_CONFLICT_ERR);
		ASSERT(iRet>0 || iRet==TLV_ERR_CONFLICT);
		if(iRet<0 && iRet!=TLV_ERR_CONFLICT)
			return(SEL_ERR_OTHER);
	}

	// TDFxx, TAC-Denial(TAG_DFXX_TACDenial)
	if(pMatchedAid->ucTacDenialExistFlag) {
		iRet = iTlvMakeAddObj(gl_sTlvDbTermVar, TAG_DFXX_TACDenial, 5, pMatchedAid->sTacDenial, TLV_CONFLICT_ERR);
		ASSERT(iRet>0 || iRet==TLV_ERR_CONFLICT);
		if(iRet<0 && iRet!=TLV_ERR_CONFLICT)
			return(SEL_ERR_OTHER);
	}

	// TDFxx, TAC-Online(TAG_DFXX_TACOnline)
	if(pMatchedAid->ucTacOnlineExistFlag) {
		iRet = iTlvMakeAddObj(gl_sTlvDbTermVar, TAG_DFXX_TACOnline, 5, pMatchedAid->sTacOnline, TLV_CONFLICT_ERR);
		ASSERT(iRet>0 || iRet==TLV_ERR_CONFLICT);
		if(iRet<0 && iRet!=TLV_ERR_CONFLICT)
			return(SEL_ERR_OTHER);
	}

	// TDFxx, Force Online Support(TAG_DFXX_ForceOnlineSupport)
	if(pMatchedAid->cForcedOnlineSupport >= 0) {
		if(pMatchedAid->cForcedOnlineSupport > 1)
			return(SEL_ERR_OTHER);
		iRet = iTlvMakeAddObj(gl_sTlvDbTermVar, TAG_DFXX_ForceOnlineSupport, 1, &pMatchedAid->cForcedOnlineSupport, TLV_CONFLICT_ERR);
		ASSERT(iRet>0 || iRet==TLV_ERR_CONFLICT);
		if(iRet<0 && iRet!=TLV_ERR_CONFLICT)
			return(SEL_ERR_OTHER);
	}

	// TDFxx, Force accept Support(TAG_DFXX_ForceAcceptSupport)
	if(pMatchedAid->cForcedAcceptanceSupport >= 0) {
		if(pMatchedAid->cForcedAcceptanceSupport > 1)
			return(SEL_ERR_OTHER);
		iRet = iTlvMakeAddObj(gl_sTlvDbTermVar, TAG_DFXX_ForceAcceptSupport, 1, &pMatchedAid->cForcedAcceptanceSupport, TLV_CONFLICT_ERR);
		ASSERT(iRet>0 || iRet==TLV_ERR_CONFLICT);
		if(iRet<0 && iRet!=TLV_ERR_CONFLICT)
			return(SEL_ERR_OTHER);
	}

	// TDFxx, EC term support indicator(TAG_DFXX_ECTermSupportIndicator)
	if(pMatchedAid->cECashSupport >= 0) {
		if(pMatchedAid->cECashSupport > 1)
			return(SEL_ERR_OTHER);
		iRet = iTlvMakeAddObj(gl_sTlvDbTermVar, TAG_DFXX_ECTermSupportIndicator, 1, &pMatchedAid->cECashSupport, TLV_CONFLICT_ERR);
		ASSERT(iRet>0 || iRet==TLV_ERR_CONFLICT);
		if(iRet<0 && iRet!=TLV_ERR_CONFLICT)
			return(SEL_ERR_OTHER);
	}

	// T9F7B, �ն˵����ֽ����޶�
	if(strlen(pMatchedAid->szTermECashTransLimit)) {
		uchar szBuf[50];
		if(strlen(pMatchedAid->szTermECashTransLimit) > 12) {
			return(SEL_ERR_OTHER);
		}
		if(iTestStrDecimal(pMatchedAid->szTermECashTransLimit, strlen(pMatchedAid->szTermECashTransLimit)) != 0) {
			return(HXEMV_PARA); // ������ʮ�����ַ�����ʾ
		}
		memset(szBuf, '0', 12);
		szBuf[12] = 0;
		memcpy(szBuf+12-strlen(pMatchedAid->szTermECashTransLimit), pMatchedAid->szTermECashTransLimit, strlen(pMatchedAid->szTermECashTransLimit));
		vTwoOne(szBuf, 12, sBuf);
		iRet = iTlvMakeAddObj(gl_sTlvDbTermVar, TAG_9F7B_ECTermTransLimit, 6, sBuf, TLV_CONFLICT_ERR);
		if(iRet<0 && iRet!=TLV_ERR_CONFLICT)
			return(SEL_ERR_OTHER);
	}

	// T9F1D, Terminal Risk Management Data
	if(pMatchedAid->ucTermRiskDataLen) {
		if(pMatchedAid->ucTermRiskDataLen>8)
			return(SEL_ERR_OTHER);
		iRet = iTlvMakeAddObj(gl_sTlvDbTermVar, TAG_9F1D_TermRistManaData, pMatchedAid->ucTermRiskDataLen, pMatchedAid->sTermRiskData, TLV_CONFLICT_ERR);
		if(iRet<0 && iRet!=TLV_ERR_CONFLICT)
			return(SEL_ERR_OTHER);
	}

    // TDFxx, Online PIN support(TAG_DFXX_OnlinePINSupport)
	if(pMatchedAid->cOnlinePinSupport >= 0) {
		if(pMatchedAid->cOnlinePinSupport > 1)
			return(SEL_ERR_OTHER);
		iRet = iTlvMakeAddObj(gl_sTlvDbTermVar, TAG_DFXX_OnlinePINSupport, 1, &pMatchedAid->cOnlinePinSupport, TLV_CONFLICT_ERR);
		ASSERT(iRet>0 || iRet==TLV_ERR_CONFLICT);
		if(iRet<0 && iRet!=TLV_ERR_CONFLICT)
			return(SEL_ERR_OTHER);
	}

	return(0);
}

// ���ܣ�����ѡ��
// ����: iIgnoreBlock			: !0:����Ӧ��������־, 0:������Ӧ�ò��ᱻѡ��
// ���أ�0	 		            : �ɹ�
//       SEL_ERR_CARD           : ��ƬͨѶ��
//       SEL_ERR_NO_APP         : û��Ӧ�ÿ�ѡ
//		 SEL_ERR_APP_BLOCKED    : Ӧ���Ѿ�����
//       SEL_ERR_DATA_ERROR     : ������ݷǷ�
//       SEL_ERR_CANCEL         : �û�ȡ��
//       SEL_ERR_TIMEOUT        : ��ʱ
//       SEL_ERR_OTHER          : ��������
// Note: ����ú����ɹ�ִ�У�˵���û���Ҫ��Ӧ���Ѿ���ѡ����
//       ���ô˺���ǰ������ɹ�������iSelGetAids������ѡӦ���б�
//       ��Ȼ����������ѡ��(�淶���), ��GPO������֧�ָ�Ӧ��ʱ������Ҫ��������ѡ��
//       refer Emv2008 book1 12.4 (P147)
//             JR/T 0025.3��2010 12.3.4 (P54)
//             JR/T 0025.6��2010 7.2.5 (P13)
int iSelFinalSelect(int iIgnoreBlock)
{
    int   iRet;
	int   iAppSubscript;                  // ��ѡ�е�Ӧ����gl_aCardAppList[]�е��±�
	uchar *pucAppConfirmSupport;          // TDFxx, 1:֧��Ӧ��ȷ�� 0:��֧��Ӧ��ȷ��(TAG_DFXX_AppConfirmSupport)
	
	// ��ȡ�ն�Ӧ��ѡ��֧�����
	iRet = iTlvGetObjValue(gl_sTlvDbTermFixed, TAG_DFXX_AppConfirmSupport, &pucAppConfirmSupport);
	if(iRet <= 0)
		return(SEL_ERR_OTHER);

	for(;;) {
// step 1 �ж�����Ӧ��
		if(gl_iCardAppNum == 0)
			break; // ���û��֧�ֵ�Ӧ��
        if(gl_iCardAppNum != sg_iTotalMatchedApps && sg_iCardHolderConfirmed) {
            // ��ǰӦ�ø�����ͬ�ڿ�Ƭ���ն����ͬʱ֧�ֵ�Ӧ�ø���, ˵�����ǵ�һ��ѡ��Ӧ��, ��Ҫ��ʾ"Try Again"
            iEmvIODispMsg(EMVMSG_13_TRY_AGAIN, EMVIO_NEED_NO_CONFIRM);
        }

// step 1.5
		if(gl_iCardType == EMV_CONTACTLESS_CARD) {
			iAppSubscript = 0;
   			goto label_select_finished; // �ǽ�Ӧ�ò���Ҫȷ�ϣ��Զ�ѡ�����ȼ���ߵ�Ӧ��
		}

// step 2 ���ֻ��һ��Ӧ��
		if(gl_iCardAppNum == 1) {
			// ע������ֿ����������й�ȷ�ϣ���ʹֻ��һ��Ӧ�ã�Ҳ�����Զ�ѡ��
			iAppSubscript = 0; // ֻ��һ��Ӧ��
			if(sg_iCardHolderConfirmed==0/*0:�ֿ���δ���й�ȷ��*/) // ֻ��֮ǰûȷ�Ϲ�Ӧ��ʱ�������Զ�ѡ��Ӧ��
    			if(gl_aCardAppList[iAppSubscript].iPriority>=0 && (gl_aCardAppList[iAppSubscript].iPriority&0x80) == 0)
	    			goto label_select_finished; // ��Ӧ�ò���Ҫȷ�ϣ��Զ�ѡ�и�Ӧ��
			// Ҫ��ֿ���ȷ��
			if(*pucAppConfirmSupport == 0)
				break; // �ն˲�֧�ֳֿ���ȷ��
			iRet = iEmvIOConfirmApp(&gl_aCardAppList[iAppSubscript]);
			if(iRet == EMVIO_OK) {
				sg_iCardHolderConfirmed = 1; // �ֿ���������ȷ��
				goto label_select_finished; // �û�ȷ�ϣ�ѡ�и�Ӧ��
			}
			if(iRet == EMVIO_CANCEL)
				return(SEL_ERR_CANCEL);
			if(iRet == EMVIO_TIMEOUT)
				return(SEL_ERR_TIMEOUT);
			return(SEL_ERR_OTHER);
		}
// step 3 �ж��ն��Ƿ�֧�ֳֿ���ȷ��
		if(*pucAppConfirmSupport == 1) {
// step 4 �ն�֧�ֳֿ���ȷ��
			iRet = iEmvIOSelectApp(&gl_aCardAppList[0], gl_iCardAppNum, &iAppSubscript);
			if(iRet == EMVIO_OK) {
				sg_iCardHolderConfirmed = 1; // �ֿ���������ѡ��
				if(iAppSubscript<0 || iAppSubscript>=gl_iCardAppNum)
					return(SEL_ERR_OTHER); // ѡ���Ӧ���±�Խ��
				goto label_select_finished; // �û�ȷ�ϣ�ѡ�и�Ӧ��
			}
			if(iRet == EMVIO_CANCEL)
				return(SEL_ERR_CANCEL);
			if(iRet == EMVIO_TIMEOUT)
				return(SEL_ERR_TIMEOUT);
			return(SEL_ERR_OTHER);
		} else {
// step 5 �ն˲�֧�ֳֿ���ȷ��
			// ѡ�����ȼ���ߵ��Ҳ���Ҫȷ�ϵ�Ӧ��
			for(iAppSubscript=0; iAppSubscript<gl_iCardAppNum; iAppSubscript++) {
				if(gl_aCardAppList[iAppSubscript].iPriority < 0)
					continue; // �����Ӧ�������ȼ���Թ�
				if((gl_aCardAppList[iAppSubscript].iPriority&0x80) == 0)
					goto label_select_finished; // ��Ӧ�ò���Ҫȷ�ϣ��Զ�ѡ�и�Ӧ��
			}
			break; // û�п����Զ�ѡ�е�Ӧ��
		}

label_select_finished: // �û�ѡ�������׼��ִ�п�ƬӦ��ѡ��ָ��
		iRet = _iSelTrySelect(&gl_aCardAppList[iAppSubscript], iIgnoreBlock);
		if(iRet==0 || (iRet==SEL_ERR_APP_BLOCKED && iIgnoreBlock)) {
			// ѡ��ɹ���Ӧ�ñ�����Ҫ�����Ӧ������, ����������Ӧ���ϵ��ն˲������浽Tlv���ݿ�gl_sTlvDbTermVar��
			iRet = iSelGetAidPara(&gl_aCardAppList[iAppSubscript]);
			return(iRet);
		}
		if(iRet == SEL_ERR_CARD)
			return(SEL_ERR_CARD);

        // ��ѡʧ�ܣ�ɾ����Ӧ�ã�����ѡ��
        iSelDelCandidate(gl_aCardAppList[iAppSubscript].ucAdfNameLen, gl_aCardAppList[iAppSubscript].sAdfName);

		if(gl_iCardType == EMV_CONTACTLESS_CARD)
			break; // ����Ƿǽӿ�, ֱ���˳�, �ο�JR/T0025.12��2013ͼ4, P11
	} // for(;;
	return(SEL_ERR_NO_APP); 
}

// ���ܣ�����ѡ��(�ṩ���ǻص�����ʹ��)
// ����: iIgnoreBlock			: !0:����Ӧ��������־, 0:������Ӧ�ò��ᱻѡ��
//       iAidLen                : AID����
//       psAid                  : AID
// ���أ�0	 		            : �ɹ�
//       SEL_ERR_CARD           : ��ƬͨѶ��
//       SEL_ERR_NO_APP         : �޴�Ӧ��
//       SEL_ERR_NEED_RESELECT  : ��Ҫ����ѡ��
//       SEL_ERR_OTHER          : ��������
// Note: ����ú����ɹ�ִ�У�˵���û���Ҫ��Ӧ���Ѿ���ѡ����
//       ���ô˺���ǰ������ɹ�������iSelGetAids������ѡӦ���б�
//       ��Ȼ����������ѡ��(�淶���), ��GPO������֧�ָ�Ӧ��ʱ������Ҫ��������ѡ��
//       refer Emv2008 book1 12.4 (P147)
//             JR/T 0025.3��2010 12.3.4 (P54)
//             JR/T 0025.6��2010 7.2.5 (P13)
int iSelFinalSelect2(int iIgnoreBlock, int iAidLen, uchar *psAid)
{
    int   iRet;
	int   iAppSubscript;                  // ��ѡ�е�Ӧ����gl_aCardAppList[]�е��±�
	
	// ���ѡ���Ӧ���Ƿ���֧�ֵ�Ӧ�÷�Χ֮��
	for(iAppSubscript=0; iAppSubscript<sg_iTotalMatchedApps; iAppSubscript++) {
		if(iAidLen == gl_aCardAppList[iAppSubscript].ucAdfNameLen) {
			if(memcmp(psAid, gl_aCardAppList[iAppSubscript].sAdfName, iAidLen) == 0)
				break;
		}
	}
	if(iAppSubscript >= gl_iCardAppNum)
		return(SEL_ERR_NO_APP); // ���ò���뱣֤��Ӧ�ô���

	iRet = _iSelTrySelect(&gl_aCardAppList[iAppSubscript], iIgnoreBlock);
	if(iRet==0 || (iRet==SEL_ERR_APP_BLOCKED && iIgnoreBlock)) {
		// ѡ��ɹ�, ����������Ӧ���ϵ��ն˲������浽Tlv���ݿ�gl_sTlvDbTermVar��
		iRet = iSelGetAidPara(&gl_aCardAppList[iAppSubscript]);
		if(iRet)
			return(SEL_ERR_OTHER);
		return(0);
	}

	if(iRet == SEL_ERR_CARD)
		return(SEL_ERR_CARD);
    // ��ѡʧ�ܣ�ɾ����Ӧ�ã�����ѡ��
	iSelDelCandidate(gl_aCardAppList[iAppSubscript].ucAdfNameLen, gl_aCardAppList[iAppSubscript].sAdfName);
	return(SEL_ERR_NEED_RESELECT);
}
