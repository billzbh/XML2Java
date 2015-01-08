/**************************************
File name     : EMVSELE.H
Function      : Implement EMV2004 standard functions about Application Selecting
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
**************************************/
# ifndef _EMVSELE_H
# define _EMVSELE_H

// -11XXX ΪӦ��ѡ�����������
#define SEL_ERR_CARD           -11001 // ��ƬͨѶ��
#define SEL_ERR_CARD_SW        -11002 // ��״̬�ַǷ�
#define SEL_ERR_CARD_BLOCKED   -11003 // ��Ƭ�Ѿ�����
#define SEL_ERR_TLV_BUFFER     -11004 // TLV�洢������δ��ʼ�������...
#define SEL_ERR_NOT_FOUND      -11005 // δ�ҵ�ƥ��
#define SEL_ERR_NOT_SUPPORT_T  -11006 // �ն˲�֧��PSE����
#define SEL_ERR_NOT_SUPPORT_C  -11007 // ��Ƭ��֧��PSE����
#define SEL_ERR_NOT_AVAILABLE  -11008 // ��Ƭ��֧��PSE����
#define SEL_ERR_PSE_ERROR      -11009 // ��ƬPSE���ֹ��ϣ���֧�֡�����...
#define SEL_ERR_NO_REC         -11010 // Ŀ¼�ļ��޼�¼
#define SEL_ERR_OVERFLOW       -11011 // �洢�ռ䲻��,DDF���л�Ӧ���б�
#define SEL_ERR_DATA_ERROR     -11012 // ���ݴ���
#define SEL_ERR_DATA_ABSENT    -11013 // ��Ҫ���ݲ�����
#define SEL_ERR_IS_DDF         -11014 // ��DDF
#define SEL_ERR_APP_BLOCKED    -11015 // Ӧ���Ѿ�����
#define SEL_ERR_NO_APP         -11016 // û��Ӧ�ÿ�ѡ
#define SEL_ERR_NEED_RESELECT  -11017 // ��Ҫ����ѡ��
#define SEL_ERR_CANCEL         -11018 // �û�ȡ��
#define SEL_ERR_TIMEOUT        -11019 // ��ʱ
#define SEL_ERR_OTHER          -11999 // ��������

// ���ܣ���PSE+AidList��ʽ�ҳ���ѡӦ���б�
// ����: iIgnoreBlock			: 1:����Ӧ��������־, 0:������Ӧ�ò��ᱻѡ��
// ���أ�>=0			        : ��ɣ��ҵ���Ӧ�ø���
//       SEL_ERR_CARD           : ��ƬͨѶ��
//       SEL_ERR_CARD_SW        : �Ƿ�״̬��
//       SEL_ERR_CARD_BLOCKED   : ��Ƭ�Ѿ��������ÿ�ƬӦ���ܾ��������ٳ����б�
//       SEL_ERR_OVERFLOW       : �洢�ռ䲻��,DDF���л�Ӧ���б�
//       SEL_ERR_DATA_ERROR     : Ӧ��������ݷǷ�
//       SEL_ERR_DATA_ABSENT    : ��Ҫ���ݲ�����
// Note: �ҳ���Ӧ�÷���ȫ�ֱ���gl_aCardAppList[]��
//       �ҳ���Ӧ�ø�������gl_iCardAppNum��
int iSelGetAids(int iIgnoreBlock);

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
int iSelFinalSelect(int iIgnoreBlock);

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
int iSelFinalSelect2(int iIgnoreBlock, int iAidLen, uchar *psAid);

// ���ܣ��Ӻ�ѡ�б���ɾ��һ��Ӧ��
// ���룺ucAidLen       : Ҫɾ����aid����
//       psAid          : Ҫɾ����aid
// ���أ�>=0	 		: ��ɣ�ʣ���Ӧ�ø���
//       SEL_ERR_NO_APP : û�ҵ�Ҫɾ����Ӧ��
int iSelDelCandidate(uchar ucAidLen, uchar *psAid);

# endif
