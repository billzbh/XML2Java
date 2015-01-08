#ifndef _PUSHAPDU_H
#define _PUSHAPDU_H

#define PUSHAPDU_READ_PSE_REC			1 // ��DDFĿ¼�ļ�
#define PUSHAPDU_SELECT_BY_AID_LIST		2 // ����AID_LISTѡ��Ӧ��
#define PUSHAPDU_READ_APP_REC			3 // ��Ӧ�����ݼ�¼
#define PUSHAPDU_GET_DATA				4 // ��ȡ����
#define PUSHAPDU_SCRIPT_EXEC			5 // �ű�ִ��
#define PUSHAPDU_READ_TRANS_REC			6 // ��������־��¼

// ��ʼ��PushApdu������
// in  : iReaderNo : ִ�����ͻ���Ŀ�����
void vPushApduInit(int iReaderNo);

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
void vPushApduPrepare(int iPushApduType, ...);

// ���PushApdu������
void vPushApduClear(void);

// �������ͻ���������ִ��IC��ָ��
// ���������uiReader : ���⿨����
//           pIn      : IC��ָ��ṹ
//           pOut     : IC�����ؽ��
// ��    �أ�0        : �ɹ�
//           1        : ʧ��
//           2        : ������������
// Note    : �ӿ�����uiExchangeApdu()
uint uiPushExchangeApdu(uint uiReader, APDU_IN *pIn, APDU_OUT *pOut);

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
uint uiPushDoApdu(uint uiReader, uint uiInLen, uchar *psIn, uint *puiOutLen, uchar *psOut);

#endif
