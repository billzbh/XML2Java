/**************************************
File name     : JsbPboc.h
Function      : ��������Pboc���ӿ�
Author        : Yu Jun
First edition : Apr 15th, 2014
Modified      :
**************************************/
/*******************************************************************************
�ӿ�˵��:
. ��������
  ÿ��������ǰ4������Ϊ��������, ������char *pszComNo, int nTermType, char cBpNo, int nIcFlag, ������ͳһ����, �Ժ�ÿ�����庯����������.
    pszComNo:	ͨѶ����, ���Žӿڲ���Ҫ
    nTermType:	�ն�����, ���Žӿڲ���Ҫ
    cBpNo:		BP�е�ת��, ���Žӿڲ���Ҫ
    nIcFlag:		������, 1:�Ӵ�IC�� 2:�ǽӴ�IC�� 3:�Զ��ж�
				ֻ�к���ICC_GetIcInfo()��ICC_GetTranDetail()��Ҫ
				�������������õ���ICC_GetIcInfo()����ʱȷ���Ŀ�����
*******************************************************************************/

#ifndef _JSBPBOC_H
#define _JSBPBOC_H

#ifdef __cplusplus
extern "C" {
#endif

#define JSBPBOC_OK				0   // OK
#define JSBPBOC_CORE_INIT		-1  // ���ĳ�ʼ������
#define JSBPBOC_CARD_TYPE		-2  // ��֧�ֵĿ�Ƭ����
#define JSBPBOC_TRANS_INIT		-3  // ���׳�ʼ������
#define JSBPBOC_NO_APP			-4  // ��֧�ֵ�Ӧ��
#define JSBPBOC_CARD_IO			-5  // ��������
#define JSBPBOC_TAG_UNKNOWN		-6  // ��ʶ��ı�ǩ
#define JSBPBOC_DATA_LOSS       -7  // �ر����ݲ�����
#define JSBPBOC_PARAM			-8  // ��������
#define JSBPBOC_APP_DATA		-9  // �����Ӧ�������봫����Ӧ�����ݲ���
#define JSBPBOC_GEN_ARQC		-10 // ��Ƭδ����ARQC
#define JSBPBOC_LOG_FORMAT		-11 // ��־��ʽ��֧��
#define JSBPBOC_UNKNOWN			-99 // δ֪����

// ��ʼ������
int ICC_InitEnv(int (*pfiTestCard)(void),
			    int (*pfiResetCard)(unsigned char *psAtr),
			    int (*pfiDoApdu)(int iApduInLen, unsigned char *psApduIn, int *piApduOutLen, unsigned char *psApduOut),
			    int (*pfiCloseCard)(void));

// ������Ϣ
// In  : pszTagList   : ��ʾ��Ҫ���ص��û����ݵı�ǩ�б�
// Out : pnUsrInfoLen :	���ص��û����ݳ���
//       pszUserInfo  : ���ص��û�����, ���������ٱ���1024�ֽ�
//       pszAppData   :	IC������, ����Tag8C��Tag8D��, ��������55��, ���������ٱ���4096�ֽ�
//       pnIcType     : �ϵ�ɹ��Ŀ�����, 1:�Ӵ��� 2:�ǽӿ�
//						���ں���ICC_GenARQC()��ICC_CtlScriptData()����
// Ret : 0            : �ɹ�
//       <0           : ʧ��
int ICC_GetIcInfo(char *pszComNo, int nTermType, char cBpNo, int nIcFlag,
						char *pszTaglist,
						int *pnUsrInfoLen, char *pszUserInfo, char *pszAppData, int *pnIcType);

// ��ȡPbocӦ�ð汾, ������ICC_GetIcInfo֮��ִ��
// Out : pszPbocVer   :	���ص�PBOC�汾�ţ�"0002"��ʶPBOC2.0��"0003"��ʶPBOC3.0
// Ret : 0            : �ɹ�
//       <0           : ʧ��
int ICC_GetIcPbocVersion(char *pszPbocVer);

// ����ARQC
// In  : pszTxData    : ��������, ��ǩ�ӳ������ݸ�ʽ
//       pszAppData   : ICC_GetIcInfo�������ص�Ӧ������, ���뱣����ԭ������ͬ
// Out : pnARQCLen    : ���ص�ARQC����
//       pszARQC      : ���ص�ARQC, TLV��ʽ, ת��Ϊ16���ƿɶ���ʽ
// Ret : 0            : �ɹ�
//       <0           : ʧ��
int ICC_GenARQC(char *pszComNo, int nTermType, char cBpNo, int nIcFlag,
						char *pszTxData, char *pszAppData, int *pnARQCLen, char *pszARQC);

// ���׽�������
// In  : pszTxData    : ��������, ��ǩ�ӳ������ݸ�ʽ
//       pszAppData   : ICC_GetIcInfo�������ص�Ӧ������, ���뱣����ԭ������ͬ
//       pszARPC      : ����������, TLV��ʽ, ת��Ϊ16���ƿɶ���ʽ
// Out : pnTCLen      : ���ص�TC����
//       pszTC        : ���ص�TC, TLV��ʽ, ת��Ϊ16���ƿɶ���ʽ
// Ret : 0            : �ɹ�
//       <0           : ʧ��
int ICC_CtlScriptData(char *pszComNo, int nTermType, char cBpNo, int nIcFlag,
						char *pszTxData, char *pszAppData, char *pszARPC,
						int *pnTCLen, char *pszTC, char *pszScriptResult);

// ��ȡ������ϸ
// Out : pnTxDetailLen:	������ϸ����
//       TxDetail     :	������ϸ, ��ʽΪ
//                      ��ϸ����(2�ֽ�ʮ����)+ÿ����ϸ�ĳ���(3�ֽ�ʮ����) + ��ϸ1+��ϸ2+...
// Ret : 0            : �ɹ�
//       <0           : ʧ��
// Note: ���10����¼
int ICC_GetTranDetail(char *pszComNo, int nTermType, char BpNo, int nIcFlag,
						int *pnTxDetailLen, char *TxDetail);

#ifdef __cplusplus
}
#endif

#endif
