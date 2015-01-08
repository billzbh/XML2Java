/**************************************
File name     : TLVFUNC.H
Function      : Implement EMV2004 standard TLV functions
Author        : Yu Jun
First edition : Mar 5th, 2003
Note          : Refer to EMV2004 Specification Book3 Part IV, Annex A
                                               Book3 Part IV, Annex B
                                               Book3 Part II, 5.4
Reference     : TagAttr.c
Modified      : Mar 11th, 2003
                    support only one or two bytes of TLV length field
                Mar 12th, 2003
                    add function
                    iTlvCopyBuffer(uchar *psTlvTargetBuffer, uchar *psTlvSourceBuffer)
				Jul 31st, 2008
					from EMV2000 to EMV2004
              : Aug 8th, 2008
					add 2 functions
						short iTlvValueLen(uchar *psTlvObj);
						uchar *psTlvValue(uchar *psTlvObj);
                Aug 20th, 2008
                    modify function
                        iTlvSearchObj()
                        iTlvBatchAddObj()
                            ����EMV4.1 book3 5.2, ���TLV����ֵ�򳤶�Ϊ0����Ϊ������
			  : Jul 8th, 2009
			        add 2 functions
						short iTlvMakeRecBlock(...)
			  : Jul 13th, 2009
			        adjust function ret value
					    short iTlvBatchAddObj(...)
              : Sep 21st, 2009
                    add function
                        short iTlvSearchDOLTag(uchar *psDOL, short iDOLLen,	uchar *psTag);
Modified      : Mar 28th, 2012
					���������������short�͸�Ϊint�ͣ�ushort�͸�Ϊuint��
				    ��ӻ��������/����TLV�������������˲���iConflictFlag, ���ڿ��Ƴ�ͻ����Ϊ
					iTlvCheckTlvObject()ǿ����TLV������
					����iTlvDbInfo()����
				Apr 7th, 2012
				    ����iTlvMakeAddObj()����������Tlv����Ȼ��ֱ�Ӽӵ�Tlv���ݿ���ȥ
				Apr 20th, 2012
					����iTlvBatchAddObj()����, ֻ������ӱ������Կ�Ƭ��Tlv����
				May 3rd, 2012
					����iTlvMakeDOLBlock()����, ����iBlockSize���ڷ�ֹ�洢Խ��
				May 4th, 2012
					����iTlvMakeObject()����, ����iTlvObjectBufSize���ڷ�ֹ�洢Խ��
                May 7th, 2012
					����iTlvBatchAddField55Obj()����, �����������8583 55���Tlv����Tlv���ݿ���
				Jul 27th, 2012
					����iTlvSearchObjValue()��iTlvSearchObj()����, ����·�����԰���'\xFF'ͨ���
				Jul 27th, 2012
					����iTlvCheckOrder()����, ���ں˲�Tlv����˳��Ϸ���
				Aug 10th, 2012
					����iTlvBatchAddObj()����, ������ӷ�Tag70��Tag77ģ������
				Aug 10th, 2012
					����iTlvCheckDup()����, ���ģ���ظ���
				Aut 14th, 2012
					����iTlvBatchAddObj()����, ����һ����, ����ָʾ�Ƿ񽫳���Ϊ0�Ķ���Ҳ���뵽���ݿ���
				Aug 31th, 2012
					����iTlvSearchDOLTag()����, ����һ���ز���, ����DOL���ݿ�ƫ����
Modified      : Mar 8th, 2013
					����Tag���Ƚ���, ����DFxx˽��Class��Tag, �ڶ��ֽ����λ�����ͳ�Tag��չ
					����DF80��Tag���ڻ���Emv�����������ݴ洢(����������Emv�淶��������2�ֽ�Tag, ��˽��õڶ��ֽ����λ)
					������������DFxx, ����class��Tag����ʽͬǰ
                Mar 21st, 2013
				    71�ű��Ӵ����DF71��Ϊ�����DFF1, 72�ű��Ӵ����DF72��Ϊ�����DFF2
Modified      : Apr 2nd, 2013
                    ����iTlvObjLen()����, ����Tlv�����С
Modified      : Mar 18th, 2014
					����iTlvBatchAddField55ObjHost()����, ���ں�̨�������8583 55���Tlv����Tlv���ݿ���
**************************************/
# ifndef _TLVFUNC_H
# define _TLVFUNC_H

#ifdef __cplusplus
extern "C" {
#endif

// ��ͻ���Ʊ�־
#define TLV_CONFLICT_ERR		0	// ���TLV��������ͻ����Ϊ������ֹ����
#define TLV_CONFLICT_IGNORE		1	// ���TLV��������ͻ�����ԣ���������
#define TLV_CONFLICT_REPLACE	2	// ���TLV��������ͻ���滻��ԭTLV����

// -10XXX ΪTLV����������
#define TLV_OK				 0      // �ɹ�
#define TLV_ERR_BUF_SPACE    -10001 // TLV�洢�����㹻�Ĵ洢�ռ�
#define TLV_ERR_BUF_FORMAT   -10002 // TLV�洢����ʽ����
#define TLV_ERR_CONFLICT     -10003 // ��ͻ��TLV�����Ѵ���
#define TLV_ERR_NOT_FOUND    -10004 // û�ҵ���Ӧ��TLV����
#define TLV_ERR_OBJ_FORMAT   -10005 // TLV����ṹ�Ƿ�
#define TLV_ERR_TEMP_FORMAT  -10006 // TLVģ��ṹ�Ƿ�
#define TLV_ERR_DOL_FORMAT   -10007 // TLV����ṹ�Ƿ�
#define TLV_ERR_OBJ_LENGTH   -10008 // TLV���󳤶Ȳ�����EMV�淶
#define TLV_ERR_CHECK        -10009 // TLV���������
#define TLV_ERR_OTHER        -10999 // ��������

// ���ܣ���ʼ��TLV�洢����һ��TLV�洢�������ȳ�ʼ������ʹ��
// ���룺psTlvBuffer		: TLV�洢��
//       uiTlvBufferSize	: �ֽڵ�λ�Ĵ洢�����ȣ�����10�ֽ�
// ���أ�>=0				: ���, ���õĴ洢�ռ�
//       TLV_ERR_BUF_SPACE  : �洢�ռ䲻�㣬�洢������10�ֽ�
int iTlvSetBuffer(uchar *psTlvBuffer, uint uiTlvBufferSize);

// ���ܣ����TLV����TLV�洢��
// ���룺psTlvBuffer		: TLV�洢��
//       psTlvObject		: TLV���󣬿����ǽṹTLV�����TLV
//       iConflictFlag		: ��ͻ�����־, TLV_CONFLICT_ERR|TLV_CONFLICT_IGNORE|TLV_CONFLICT_REPLACE
// ���أ�>0					: ��ӳɹ����Ѵ�����ȫ��ͬ��TLV����, �����С
// 	     TLV_ERR_BUF_SPACE	: TLV�洢�����㹻�Ĵ洢�ռ�
//  	 TLV_ERR_BUF_FORMAT	: TLV�洢����ʽ����
//       TLV_ERR_CONFLICT	: ��ͻ��TLV�����Ѵ���
//       TLV_ERR_OBJ_FORMAT	: TLV����ṹ�Ƿ�
int iTlvAddObj(uchar *psTlvBuffer, uchar *psTlvObject, int iConflictFlag);

// ���ܣ�����TLV����Ȼ��ֱ����ӵ�TLV�洢��
// ���룺psTlvBuffer		: TLV�洢��
//       psTag              : Tag
//       uiLength           : Length
//       psValue            : Value
//       iConflictFlag		: ��ͻ�����־, TLV_CONFLICT_ERR|TLV_CONFLICT_IGNORE|TLV_CONFLICT_REPLACE
// ���أ�>0					: ��ӳɹ����Ѵ�����ȫ��ͬ��TLV����, �����С
// 	     TLV_ERR_BUF_SPACE	: TLV�洢�����㹻�Ĵ洢�ռ�
//  	 TLV_ERR_BUF_FORMAT	: TLV�洢����ʽ����
//       TLV_ERR_CONFLICT	: ��ͻ��TLV�����Ѵ���
//       TLV_ERR_OTHER      : ��������,Tag��Length�Ƿ�
int iTlvMakeAddObj(uchar *psTlvBuffer, uchar *psTag, uint uiLength, uchar *psValue, int iConflictFlag);

// ���ܣ���TLV�洢����ɾ��TLV���󣬿���ʡ���ռ���������TLV����
// ���룺psTlvBuffer		: TLV�洢��
//       psTag				: Ҫɾ����TLV�����Tag
// ���أ�>0					: ���, ��ɾ����TLV�����С
// 	     TLV_ERR_BUF_FORMAT	: TLV�洢����ʽ����
//       TLV_ERR_NOT_FOUND	: û�ҵ���Ӧ��TLV����
int iTlvDelObj(uchar *psTlvBuffer, uchar *psTag);

// ���ܣ���TLV�洢����ȡ����ӦTag��TLV�����ָ��
// ���룺psTlvBuffer		: TLV�洢��
// 	     psTag				: Ҫ��ȡ��TLV�����Tag
// �����ppsTlvObject		: TLV����ָ��
// ���أ�>0					: ��ɣ�TLV����ĳ���
//  	 TLV_ERR_BUF_FORMAT	: TLV�洢����ʽ����
//  	 TLV_ERR_NOT_FOUND	: û�ҵ���Ӧ��TLV����
int iTlvGetObj(uchar *psTlvBuffer,uchar *psTag,uchar **ppsTlvObject);

// ���ܣ���TLV�洢����ȡ����Ӧλ�õ�TLV�����ָ�룬���ڱ���TLV�洢��
// ���룺psTlvBuffer		: TLV�洢��
// 	     uiIndex			: �����ţ�0��ʼ
// �����ppsTlvObject		: TLV����ָ��
// ���أ�>0					: ��ɣ�TLV����ĳ���
//	     TLV_ERR_BUF_FORMAT	: TLV�洢����ʽ����
//	     TLV_ERR_NOT_FOUND	: û�ҵ���Ӧ��TLV����
int iTlvGetObjByIndex(uchar *psTlvBuffer, uint uiIndex, uchar **ppsTlvObject);

// ���ܣ���TLV�洢����ȡ����ӦTag��TLV����ֵ��ָ��
// ���룺psTlvBuffer		: TLV�洢��
// 	     psTag				: Ҫ��ȡ��TLV�����Tag
// �����ppsValue			: TLV����ֵ��ָ��
// ���أ�>=0				: TLV����ֵ����
//	     TLV_ERR_BUF_FORMAT	: TLV�洢����ʽ����
//	     TLV_ERR_NOT_FOUND	: û�ҵ���Ӧ��TLV����
int iTlvGetObjValue(uchar *psTlvBuffer, uchar *psTag, uchar **ppsValue);

// ���ܣ���ȡTLV����ֵ
// ���룺psTlvObj			: TLV����
// �����ppsTlvObjValue	    : TLV����ֵ
// ���أ�>=0				: TLV����ֵ��С
//	     TLV_ERR_OBJ_FORMAT	: TLV�����ʽ����
int iTlvValue(uchar *psTlvObj, uchar **ppsTlvObjValue);

// ���ܣ���ȡTLV����ֵ����
// ���룺psTlvObj			: TLV����
// ���أ�>=0				: TLV����ֵ��С
//	     TLV_ERR_OBJ_FORMAT	: TLV�����ʽ����
int iTlvValueLen(uchar *psTlvObj);

// ���ܣ���ȡTLV����Tag����
// ���룺psTlvObj			: TLV����
// ���أ�1 or 2				: TLV����Tag��С
//	     TLV_ERR_OBJ_FORMAT	: TLV�����ʽ����
int iTlvTagLen(uchar *psTlvObj);

// ���ܣ���ȡTLV����Length����
// ���룺psTlvObj			: TLV����
// ���أ�1 or 2				: TLV����Len��С
//	     TLV_ERR_OBJ_FORMAT	: TLV�����ʽ����
int iTlvLengthLen(uchar *psTlvObj);

// ���ܣ���ȡTLV���󳤶�
// ���룺psTlvObj			: TLV����
// ���أ�>0                 : TLV�����С
//	     TLV_ERR_OBJ_FORMAT	: TLV�����ʽ����
// ע��: 00 or FF����Ϊ�Ǹ�ʽ����
int iTlvObjLen(uchar *psTlvObj);

// ���ܣ���ȡTLV����ֵ
// ���룺psTlvObj			: TLV����
// ���أ�!NULL			    : TLV����ֵ
//	     NULL				: TLV�����ʽ����
uchar *psTlvValue(uchar *psTlvObj);

// ���ܣ���TLVģ������ָ����·��ȡ��TLV����
// ���룺psTemplate			: TLVģ�壬Ҳ������һ���򼸸�����TLV
//       iTemplateLen       : ģ���С
// 	     iIndex			    : ����ռ�Tag�ж��ƥ�䣬ָ���ڼ�����0��ʾ��һ��
// 	     psTagRoute			: Ҫ��ȡ��TLV�����Tag, ����·��, '\x00'Tag��ʾ����, '\xFF'Tag��ʾ��ƥ���κ�Tag
// �����ppsTlvObj		    : TLV����
// ���أ�>0					: �ҵ���TLV�����С
//	     TLV_ERR_BUF_FORMAT	: TLVģ���ʽ����
//	     TLV_ERR_NOT_FOUND	: û�ҵ���Ӧ��TLV����
// ������Ҫ��ȡ����������Ϊ86��psT�е�ģ��6F�е�ģ��A5�еı��88��TLV����,�����µ���:
//       iTlvSearchObj(psT, 86, 0, "\x6F\xA5\x88\x00", psTlvObj)
int iTlvSearchObj(uchar *psTemplate, int iTemplateLen, int iIndex, uchar *psTagRoute, uchar **ppsTlvObj);

// ���ܣ���TLVģ������ָ����·��ȡ����ӦTag��TLV����ֵ
// ���룺psTemplate			: TLVģ�壬Ҳ������һ���򼸸�����TLV
//       iTemplateLen       : ģ���С
// 	     iIndex			    : ����ռ�Tag�ж��ƥ�䣬ָ���ڼ�����0��ʾ��һ��
// 	     psTagRoute			: Ҫ��ȡ��TLV�����Tag, ����·��, '\x00'Tag��ʾ����, '\xFF'Tag��ʾ��ƥ���κ�Tag
// �����psTlvObjValue		: TLV����ֵָ��
// ���أ�>0					: �ҵ���TLV����ֵ�ĳ���
//	     TLV_ERR_BUF_FORMAT	: TLVģ���ʽ����
//	     TLV_ERR_NOT_FOUND	: û�ҵ���Ӧ��TLV����
// ������Ҫ��ȡ����������Ϊ86��psT�е�ģ��6F�е�ģ��A5�еı��88��ֵ,�����µ���:
//       iTlvSearchObj(psT, 86, 0, "\x6F\xA5\x88\x00", psValue)
int iTlvSearchObjValue(uchar *psTemplate, int iTemplateLen, int iIndex, uchar *psTagRoute, uchar **ppsTlvObjValue);

// ���ܣ���TLVģ����ȡ��ָ��λ�õ�TLV����(�����ǻ���TLV����Ҳ����������һ��ģ��)�����ڱ���ģ��
// ���룺psTemplate			: TLVģ��
//       iTemplateLen       : ģ���С
// 	     iIndex			    : ָ���ڼ�����0��ʾ��һ��
// �����ppsTlvObj		    : TLV����
// ���أ�>0					: �ҵ���TLV�����С
//	     TLV_ERR_BUF_FORMAT	: TLVģ���ʽ����
//	     TLV_ERR_NOT_FOUND	: �����ڸ������Ķ���
int iTlvSerachTemplate(uchar *psTemplate, int iTemplateLen, int iIndex, uchar **ppsTlvObj);

// ���ܣ����TLV�����Ƿ�Ϸ�
// ���룺psTlvObj           : TLV����ָ��
// ���أ�>0					: �ҵ���TLV����ĳ���
//	     TLV_ERR_OBJ_FORMAT	: TLV�����ʽ����
//       TLV_ERR_OBJ_LENGTH : TLV���󳤶Ȳ�����EMV�淶
int iTlvCheckTlvObject(uchar *psTlvObj);

// ���ܣ���һ��ģ���ڵ�����TLV������ӵ�TLV�洢��(ֻ��ӵ�һ��TLV���󣬲���ݹ����ģ��)
// ���룺ucFlag             : 0:ֻ��ӻ���TLV����!0:�������TLV����
//       psTlvBuffer		: TLV�洢��
//       psTlvTemplate		: TLVģ�壬ֻ����"\x70"��"\x77"
//       iTlvTemplateLen    : TLVģ���С
//       iConflictFlag		: ��ͻ�����־, TLV_CONFLICT_ERR|TLV_CONFLICT_IGNORE|TLV_CONFLICT_REPLACE
//       iLen0Flag          : ����Ϊ0�Ķ�����ӱ�־, 0:����Ϊ0�Ķ������ 1:����Ϊ0�Ķ���Ҳ���
// ���أ�>=0				: ���, ����ֵΪ��ӵ�TLV�������
// 	     TLV_ERR_BUF_SPACE	: TLV�洢�����㹻�Ĵ洢�ռ�
//	     TLV_ERR_BUF_FORMAT	: TLV�洢����ʽ����
//	     TLV_ERR_CONFLICT	: ��ͻ����ֹ����
//	     TLV_ERR_TEMP_FORMAT: TLVģ��ṹ�Ƿ�
//       TLV_ERR_OBJ_FORMAT	: TLV����ṹ�Ƿ�
// Note: ֻ�������IC��Ƭ��Tlv����
int iTlvBatchAddObj(uchar ucFlag, uchar *psTlvBuffer, uchar *psTlvTemplate, int iTlvTemplateLen, int iConflictFlag, int iLen0Flag);

// ��Host��
// ���ܣ���8583 55���ڵ�����TLV������ӵ�TLV�洢��(ֻ��ӵ�һ��TLV���󣬲���ݹ����)
// ���룺ucFlag             : 0:ֻ��ӻ���TLV����!0:�������TLV����
//       psTlvBuffer		: TLV�洢��
//       psField55			: 55��
//       iField55Len	    : 55�򳤶�
//       iConflictFlag		: ��ͻ�����־, TLV_CONFLICT_ERR|TLV_CONFLICT_IGNORE|TLV_CONFLICT_REPLACE
// ���أ�>=0				: ���, ����ֵΪ��ӵ�TLV�������
// 	     TLV_ERR_BUF_SPACE	: TLV�洢�����㹻�Ĵ洢�ռ�
//	     TLV_ERR_BUF_FORMAT	: TLV�洢����ʽ����
//	     TLV_ERR_CONFLICT	: ��ͻ����ֹ����
//	     TLV_ERR_TEMP_FORMAT: 55����Tlv�����ʽ�Ƿ�
//       TLV_ERR_OBJ_FORMAT	: TLV����ṹ�Ƿ�
// Note: ֻ������Է�������Tlv����
int iTlvBatchAddField55Obj(uchar ucFlag, uchar *psTlvBuffer, uchar *psField55, int iField55Len, int iConflictFlag);

// Host��
// ���ܣ���8583 55���ڵ�����TLV������ӵ�TLV�洢��(ֻ��ӵ�һ��TLV���󣬲���ݹ����)
// ���룺ucFlag             : 0:ֻ��ӻ���TLV����!0:�������TLV����
//       psTlvBuffer		: TLV�洢��
//       psField55			: 55��
//       iField55Len	    : 55�򳤶�
//       iConflictFlag		: ��ͻ�����־, TLV_CONFLICT_ERR|TLV_CONFLICT_IGNORE|TLV_CONFLICT_REPLACE
// ���أ�>=0				: ���, ����ֵΪ��ӵ�TLV�������
// 	     TLV_ERR_BUF_SPACE	: TLV�洢�����㹻�Ĵ洢�ռ�
//	     TLV_ERR_BUF_FORMAT	: TLV�洢����ʽ����
//	     TLV_ERR_CONFLICT	: ��ͻ����ֹ����
//       TLV_ERR_OBJ_FORMAT	: 55����Tlv�����ʽ�Ƿ�
// Note: ��ӳ������������Tlv����
//       ����71��72�ű����ܻ��ж��, 71��72�ű��ᱻ�����DFF1��DFF2���б���
int iTlvBatchAddField55ObjHost(uchar ucFlag, uchar *psTlvBuffer, uchar *psField55, int iField55Len, int iConflictFlag);

// ���ܣ���һ��TLV�洢���ڵ�����TLV������ӵ���һ��TLV�洢����
// ���룺psTlvSourceBuffer  : ԴTLV�洢��
//       iConflictFlag		: ��ͻ�����־, TLV_CONFLICT_ERR|TLV_CONFLICT_IGNORE|TLV_CONFLICT_REPLACE
// �����psTlvTargetBuffer	: Ŀ��TLV�洢��
// ���أ�>=0				: ���, ������TLV�������
// 	     TLV_ERR_BUF_SPACE	: Ŀ��TLV�洢�����㹻�Ĵ洢�ռ�
//	     TLV_ERR_BUF_FORMAT	: ����TLV�洢����ʽ����
//	     TLV_ERR_CONFLICT	: ��ͻ����ֹ����
int iTlvCopyBuffer(uchar *psTlvTargetBuffer, uchar *psTlvSourceBuffer, int iConflictFlag);

// ���ܣ�����һ��TLV����
// ���룺psTag              : Tag
//       uiLength           : Length
//       psValue            : Value
//       iTlvObjectBufSize  : psTlvObject�ռ��С
// �����psTlvObject        : ����õ�TLV����
// ���أ�>0					: ���, ����õ�TLV�����С
//       TLV_ERR_BUF_SPACE  : psTlvObject�ռ䲻��
//       TLV_ERR_OTHER      : ��������,Tag��Length�Ƿ�
//                            ֻ�������2�ֽ�Tag���255�ֽڳ���
int iTlvMakeObject(uchar *psTag, uint uiLength, uchar *psValue, uchar *psTlvObject, int iTlvObjectBufSize);

// ���ܣ�����DOL�������ݿ�, ��TLV�洢������������Ŀ�꣬�����ṩ4��TLV�洢��
// ���룺iBlockSize			: psBlock�ռ��ֽ���
//		 psDOL				: Data Object List
// 	     iDOLLen		    : DOL����
// 	     psTlvBuffer1		: TLV�洢��1��NULL��ʾû�е�1���洢��
// 	     psTlvBuffer2		: TLV�洢��2��NULL��ʾû�е�2���洢��
// 	     psTlvBuffer3		: TLV�洢��3��NULL��ʾû�е�3���洢��
// 	     psTlvBuffer4		: TLV�洢��4��NULL��ʾû�е�4���洢��
//                            �ȴӴ洢��1��ʼ���������洢��4
// �����psBlock			: ����DOL��������ݿ�
// ���أ�>=0				: ���, ���������ݿ鳤��
// 	     TLV_ERR_BUF_FORMAT	: TLV�洢����ʽ����
//       TLV_ERR_DOL_FORMAT : DOL��ʽ����
//       TLV_ERR_BUF_SPACE  : psBlock�洢�ռ䲻��
int iTlvMakeDOLBlock(uchar *psBlock, int iBlockSize, uchar *psDOL, int iDOLLen,
          	         uchar *psTlvBuffer1, uchar *psTlvBuffer2, uchar *psTlvBuffer3, uchar *psTlvBuffer4);

// ���ܣ�����Tag�б������ݼ�¼, ��TLV�洢������������Ŀ�꣬�����ṩ4��TLV�洢��
// ���룺psTags				: Tag�б�
// 	     iTagsLen		    : Tag�б���
// 	     psTlvBuffer1		: TLV�洢��1��NULL��ʾû�е�1���洢��
// 	     psTlvBuffer2		: TLV�洢��2��NULL��ʾû�е�2���洢��
// 	     psTlvBuffer3		: TLV�洢��3��NULL��ʾû�е�3���洢��
// 	     psTlvBuffer4		: TLV�洢��4��NULL��ʾû�е�4���洢��
//                            �ȴӴ洢��1��ʼ���������洢��4
// �����psBlock			: ����Tags�б�����, ��Tag70����ļ�¼��, Ҫ��������254�ֽڿռ�
// ���أ�>=0				: ���, �����ļ�¼����
// 	     TLV_ERR_BUF_FORMAT	: TLV�洢����ʽ����
//       TLV_ERR_OTHER      : ��������
// Note: ���Tags�б���ĳTag���κ�TLV�洢����δ�ҵ�, ���ҵ���Tlv�����Value�򳤶�Ϊ0, �򲻽��ö�����ӵ���¼��
int iTlvMakeRecBlock(uchar *psBlock, uchar *psTags, int iTagsLen,
          	    	 uchar *psTlvBuffer1, uchar *psTlvBuffer2, uchar *psTlvBuffer3, uchar *psTlvBuffer4);

// ���ܣ�����DOL���Ƿ����ĳһTag
// ���룺psDOL				: Data Object List
// 	     iDOLLen		    : DOL����
//       psTag              : Ҫ������Tag
// ���  piOffset           : ���DOL�а���������TAG, ���ظ�TAG��DOL���ݿ��е�ƫ����, NULL��ʾ����Ҫ����
// ���أ�>=0				: Tag����, ���ظ�Tag��Ҫ�ĳ���
//       TLV_ERR_DOL_FORMAT : DOL��ʽ����
// ע��: piOffset���ص����ø�DOL��ģ�����ɵ����ݿ��е�ƫ����
int iTlvSearchDOLTag(uchar *psDOL, int iDOLLen,	uchar *psTag, int *piOffset);

// ���ܣ��˲�Tlv����˳��Ϸ���
// ���룺psTemplate			: TLVģ������
//       iTemplateLen       : ģ�����ݴ�С
//       psTagOrder         : Ҫ���TLV����˳��, '\x00'Tag��ʾ����
// ���أ�0					: �����ȷ
//	     TLV_ERR_BUF_FORMAT	: TLVģ���ʽ����
//       TLV_ERR_CHECK      : ���ʧ��
//       TLV_OK             : ���ɹ�
// ע��: psTagOrder������Tag��������ȷ˳�������psTemplate��
int iTlvCheckOrder(uchar *psTemplate, int iTemplateLen, uchar *psOrder);

// ���ܣ��˲�Tlvģ����������Ƿ����ظ�
// ���룺psTemplate			: TLVģ������
//       iTemplateLen       : ģ�����ݴ�С
// ���أ�0					: �����ȷ
//	     TLV_ERR_BUF_FORMAT	: TLVģ���ʽ����
//       TLV_ERR_CHECK      : ���ʧ��
//       TLV_OK             : ���ɹ�
int iTlvCheckDup(uchar *psTemplate, int iTemplateLen);

#ifdef __cplusplus
}
#endif

# endif