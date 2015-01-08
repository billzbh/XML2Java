/**************************************
File name     : EMVTRACE.H
Function      : ���̸�����Ϣ������debug
Author        : Yu Jun
First edition : Mar 27th, 2012
Modified      : 
**************************************/
#ifndef _EMVTRACE_H
#define _EMVTRACE_H

// ������������豸
#define TRACE_OFF           0   // �رո���
#define TRACE_COM1          1   // com1
#define TRACE_COM2          2   // com2
#define TRACE_COM3          3   // com3
#define TRACE_COM4          4   // com4
#define TRACE_COM5          5   // com5
#define TRACE_COM6          6   // com6
#define TRACE_COM7          7   // com7
#define TRACE_COM8          8   // com8
#define TRACE_CONSOLE       9   // ��׼���
#define TRACE_AUX           10  // �������
#define TRACE_PRINTER       11  // ��ӡ�����
#define TRACE_PTS0          12  // PTS0
#define TRACE_PTS1          13  // PTS0
#define TRACE_PTS2          14  // PTS0
#define TRACE_PTS3          15  // PTS0
#define TRACE_PTS4          16  // PTS0
#define TRACE_PTS5          17  // PTS0
#define TRACE_PTS6          18  // PTS0
#define TRACE_PTS7          19  // PTS0
#define TRACE_CALLBACK		20  // �ص�����

// ������������
#define TRACE_ALWAYS        0
#define TRACE_PROC          1
#define TRACE_APDU          2
#define TRACE_COMM          3
#define TRACE_TAG_DESC      4
#define TRACE_TLV_DB        5
#define TRACE_ERR_MSG       6

// �ṹ��Ϣ����
#define PARSE_TLV_OBJECT    1  // TLV����ṹ
#define PARSE_TLV_GPO80     2  // GPO��Tag80��ʽ���صĽṹ
#define PARSE_TLV_GAC180    3  // GAC, Tag80��ʽ���صĽṹ
#define PARSE_DOL           4  // DOL�ṹ
#define PARSE_TLV_DB        5  // TLV�������ݿ�ṹ
#define PARSE_FIELD55       6  // 8583����55���ʽ
#define INDENT_APDUOUT	    10 // APDUOUT������λ��

#define EMV_TRACE_PROC              vTraceWriteTxtCr(TRACE_PROC, "%s Line:%d", pszTraceGetFileBaseName(__FILE__), (int)__LINE__);
#define EMV_TRACE_PROC_RET(iRet)    vTraceWriteTxtCr(TRACE_PROC, "%s Line:%d (%d)", pszTraceGetFileBaseName(__FILE__), (int)__LINE__, (int)iRet);
#define EMV_TRACE_ERR_MSG           vTraceWriteTxtCr(TRACE_ERR_MSG, "%s Line:%d", pszTraceGetFileBaseName(__FILE__), (int)__LINE__);
#define EMV_TRACE_ERR_MSG_RET(iRet) vTraceWriteTxtCr(TRACE_ERR_MSG, "%s Line:%d (%d)", pszTraceGetFileBaseName(__FILE__), (int)__LINE__, (int)iRet);

// ���ø����豸
// in  : iTraceDev     : �����豸��TRACE_COM1-TRACE_COM8 | TRACE_CONSOLE | TRACE_PRINTER | TRACE_PTS0-7 | TRACE_CALLBACK
//                       TRACE_OFF��ʾ���������ݸ���
//       iTraceApdu    : ����apdu��־��0:������ 1:����
//       iTraceTagDesc : ����tag��ϸ��־��0:������ 1:����
//       iTraceTlvDb   : ����Tlv���ݿ��־��0:������ 1:����
//       pfvTraceOut   : �ص����ٺ���ָ��, �����豸ΪTRACE_CALLBACKʱ��Ҫ
// ret : 0         : OK
//       1         : ��֧�ָ����豸
//       2         : �����豸����
// Note: ����apduָ�����ݼ�Tag��ϸ�������Ƚϴ󣬹ʿ��趨���ٻ򲻸��ٱ�־
int iTraceSet(int iTraceDev, int iTraceApdu, int iTraceTagDesc, int iTraceTlvDb, void (*pfvTraceOut)(char *));

// ȡ�ļ�������
char *pszTraceGetFileBaseName(char *pszFileName);

// дtxt��������
// in  : iTraceType : TRACE_ALWAYS|TRACE_PROC|TRACE_APDU|TRACE_TAG_DESC|TRACE_ERR_MSG|TRACE_TLV_DB
void vTraceWriteTxt(int iTraceType, char *pszFormat, ...);

// дtxt���س���������
// in  : iTraceType : TRACE_ALWAYS|TRACE_PROC|TRACE_APDU|TRACE_TAG_DESC|TRACE_ERR_MSG|TRACE_TLV_DB
void vTraceWriteTxtCr(int iTraceType, char *pszFormat, ...);

// дbin��������
// in  : iTraceType : TRACE_ALWAYS|TRACE_PROC|TRACE_APDU|TRACE_TAG_DESC|TRACE_ERR_MSG|TRACE_TLV_DB
void vTraceWriteBin(int iTraceType, char *pszTitle, void *pData, int iLen);

// дbin���س���������
// in  : iTraceType : TRACE_ALWAYS|TRACE_PROC|TRACE_APDU|TRACE_TAG_DESC|TRACE_ERR_MSG|TRACE_TLV_DB
void vTraceWriteBinCr(int iTraceType, char *pszTitle, void *pData, int iLen);

// д�ϴ�apdu��������
// in  : pszFormat  : ��ʾ��Ϣ
void vTraceWriteLastApdu(char *pszFormat, ...);

// д�ṹ����������
// in  : iTraceType : TRACE_ALWAYS|TRACE_PROC|TRACE_APDU|TRACE_TAG_DESC|TRACE_ERR_MSG|TRACE_TLV_DB
//       iInfoType : ��Ϣ����, PARSE_TLV_OBJECT|PARSE_TLV_GPO80|PARSE_TLV_GAC180|PARSE_DOL|PARSE_FIELD55
//       psInfo    : ��Ϣ
//       iInfoLen  : ��Ϣ����
//       iIndent   : ����λ��
void vTraceWriteStruct(int iTraceType, int iInfoType, uchar *psInfo, int iInfoLen, int iIndent);

#endif
