/**************************************
File name     : TLVFUNC.C
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
				Aug 14th, 2012
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
/*
ģ����ϸ����:
	ʵ��������TLV�йصĲ���
.	TLV���ݿ⹦��
	TLV���ݿ���һ���ڴ�������TLV����, iTlvSetBuffer()����������ʼ��TLV���ݿ�
	TLV���ݿ��ڴ���ͷ�������˸�TLV���ݿ���TLV����ĸ�����ʣ��ռ�
	ģ���ṩ��һ��TLV���ݺ�����ʵ��TLV���ݿ�����ӡ�ɾ������ȡ����������������
.	ģ�����
	�ṩ��һ�麯��ʵ����ģ����TLV������TLV���ݵ���ȡ����������
.	����DOL����DOL���ݿ�
	����PDOL��DDOL��CDOL1��CDOL2��TDOL���������ݿ�
.	����DOL���Ƿ����ض�TAG
	��������PDOL���Ƿ��н����
*/
# include <stdio.h>
# include <string.h>
# include <ctype.h>
# include "VposFace.h"
# include "TagAttr.h"
# include "TlvFunc.h"

# define TLV_BUFFER_IDENTIFIER                  0x5501

// TLV�洢�ṹ
//    TLV Buffer				N bytes		    // �ܴ洢��������10�ֽ�ͷ��
//    	TLV Buffer Identifier	  2 bytes		// fixed "\x55\x01"
//      Number of TLV object      2 bytes       // ��ǰ�洢�����ѱ����TLV�������
//    	Total space			      2 bytes		// ʣ��洢��+���ô洢��
//    	Space left				  2 bytes		// ʣ��Ĵ洢����С
//   	Space used			      2 bytes		// ���õĴ洢����С
//   	Buffer				      N-10 bytes	// TLV�洢��
//   		TLV1			        n1 bytes	// TLV object 1
//   		TLV2			        n2 bytes	// ..
//    		...
//    		TLVn			        nn bytes	// ..

struct stTlvBufferHead {
    ushort uiIdentifier;
    ushort uiTlvObjectNum;
    ushort uiTotalSpace;
    ushort uiSpaceLeft;
    ushort uiSpaceUsed;
};
static struct stTlvBufferHead gl_TlvBufferHead; // TLV���ݿ�ͷ�ṹ

// get gl_TlvBufferHead from psTlvBuffer
static void _vTlvGetHead(uchar *psTlvBuffer)
{
    gl_TlvBufferHead.uiIdentifier = (ushort)ulStrToLong(psTlvBuffer, 2);
    gl_TlvBufferHead.uiTlvObjectNum = (ushort)ulStrToLong(psTlvBuffer+2, 2);
    gl_TlvBufferHead.uiTotalSpace = (ushort)ulStrToLong(psTlvBuffer+4, 2);
    gl_TlvBufferHead.uiSpaceLeft = (ushort)ulStrToLong(psTlvBuffer+6, 2);
    gl_TlvBufferHead.uiSpaceUsed = (ushort)ulStrToLong(psTlvBuffer+8, 2);
}

// put gl_TlvBufferHead into psTlvBuffer
static void _vTlvPutHead(uchar *psTlvBuffer)
{
    vLongToStr(gl_TlvBufferHead.uiIdentifier, 2, psTlvBuffer);
    vLongToStr(gl_TlvBufferHead.uiTlvObjectNum, 2, psTlvBuffer+2);
    vLongToStr(gl_TlvBufferHead.uiTotalSpace, 2, psTlvBuffer+4);
    vLongToStr(gl_TlvBufferHead.uiSpaceLeft, 2, psTlvBuffer+6);
    vLongToStr(gl_TlvBufferHead.uiSpaceUsed, 2, psTlvBuffer+8);
}

// get the length of the tag
// ret : 1 or 2 : length of the tag
//       0      : error format
// note: in accordance with EMV2004 specification,
//       the length of a TLV tag field can't be longer than 2
static uint _uiTlvGetTagLen(uchar *psTag)
{
    if(psTag[0] == 0x00 || psTag[0] == 0xff)
        return(0); // error, tag can't start with 0x00 or 0xFF
    if((psTag[0] & 0x1f) != 0x1f)
        return(1);
    if((psTag[1] & 0x80) == 0x80) {
		if(psTag[0] != 0xDF)
			return(0); // 20130308, ֻ�з�DFxx, �ż��Tag��2�ֽڵ����λ, ����Ƿ�, ��Ϊ����
	}
    return(2);
}
// get the length of the length field of a TLV object
// ret : 1 or 2 : length of the length field
//       0      : error format
// note: in accordance with EMV2004 specification,
//       the length field of a TLV object can't be longer than 2
static uint _uiTlvGetLengthLen(uchar *psTlvObject)
{
    uchar *psPointer;

    psPointer = psTlvObject;
    if(_uiTlvGetTagLen(psTlvObject) == 0)
        return(0); // error tag
    psPointer += _uiTlvGetTagLen(psTlvObject);
    
    if(((*psPointer) & 0x80) != 0x80)
        return(1); // b8==0, only one byte for the length field
    if(*psPointer != 0x81)
        return(0); // only support 2-byte length
    return(((*psPointer) & 0x7f)+1); // definitely 2
}
// get the length of the Value field of a TLV object
// ret : 0xffff : error format
//       other  : length of the value field
static uint _uiTlvGetValueLen(uchar *psTlvObject)
{
    uchar *psPointer;

    if(_uiTlvGetTagLen(psTlvObject) == 0)
        return(0xffff); // tag format error
    if(_uiTlvGetLengthLen(psTlvObject) == 0)
        return(0xffff); // length format error

    psPointer = psTlvObject + _uiTlvGetTagLen(psTlvObject);
    
    if(((*psPointer) & 0x80) != 0x80)
        return(*psPointer); // b8==0, it is the length of the value field
    return((ushort)ulStrToLong(psPointer+1, (ushort)((*psPointer)&0x7f)));
}
// get the length of the TLV object
// ret : > 0 : length of the TLV object
//       0   : error format of the TLV object
static uint _uiTlvGetObjLen(uchar *psTlvObject)
{
    uint uiRet, uiLength;
    
    uiRet = _uiTlvGetTagLen(psTlvObject);
    if(uiRet == 0)
        return(0);
    uiLength = uiRet;
    
    uiRet = _uiTlvGetLengthLen(psTlvObject);
    if(uiRet == 0)
        return(0);
    uiLength += uiRet;
    
    uiRet = _uiTlvGetValueLen(psTlvObject);
    if(uiRet == 0xffff)
        return(0);
    uiLength += uiRet;
    return(uiLength);
}
// check if the TLV object is in a legal format
// ret : >0  : legal, the size of TLV object
//       <0  : illegal
//             -1 : has a tag of 0x00 or 0xff
//             -2 : more than 2 bytes of tag part
//             -3 : more than 2 bytes of length part
static int _iTlvCheckTlvObj(uchar *psTlvObj)
{
    int iTagLen;
    
    // check tag
    if(psTlvObj[0] == 0x00 || psTlvObj[0] == 0xff)
        return(-1);
    if((psTlvObj[0] & 0x1f) == 0x1f && (psTlvObj[1] & 0x80) == 0x80) {
		if(psTlvObj[0] != 0xDF)
			return(-2); // 20130308, ֻ�з�DFxx, �ż��Tag��2�ֽڵ����λ, ����Ƿ�, ��Ϊ����
	}
    
    // check length
    if((psTlvObj[0] & 0x1f) == 0x1f)
        iTagLen = 2;
    else
        iTagLen = 1;
        
    if((psTlvObj[iTagLen] & 0x80) == 0x80 && (psTlvObj[iTagLen] & 0x7f) != 1)
        return(-3); // only support 1 or 2 byte length

    return(_uiTlvGetObjLen(psTlvObj));
}

// ���ܣ���ʼ��TLV�洢����һ��TLV�洢�������ȳ�ʼ������ʹ��
// ���룺psTlvBuffer		: TLV�洢��
//       uiTlvBufferSize	: �ֽڵ�λ�Ĵ洢�����ȣ�����10�ֽ�
// ���أ�>=0				: ���, ���õĴ洢�ռ�
//       TLV_ERR_BUF_SPACE  : �洢�ռ䲻�㣬�洢������10�ֽ�
int iTlvSetBuffer(uchar *psTlvBuffer, uint uiTlvBufferSize)
{
    if(uiTlvBufferSize < 10)
        return(TLV_ERR_BUF_SPACE);
    memset(psTlvBuffer, 0, uiTlvBufferSize);
    gl_TlvBufferHead.uiIdentifier = TLV_BUFFER_IDENTIFIER;
    gl_TlvBufferHead.uiTlvObjectNum = 0;
    gl_TlvBufferHead.uiTotalSpace = uiTlvBufferSize - 10;
    gl_TlvBufferHead.uiSpaceLeft = gl_TlvBufferHead.uiTotalSpace;
    gl_TlvBufferHead.uiSpaceUsed = 0;
    _vTlvPutHead(psTlvBuffer);
    return(uiTlvBufferSize - 10);
}

// ���ܣ���ѯTLV���ݿ���Ϣ
// ���룺psTlvBuffer		: TLV�洢��
// �����puiSpaceUsed       : ���ÿռ�(byte)
//       puiSpaceLeft       : ʣ��ռ�(byte)
// ���أ�>=0				: Tlv�������
//       TLV_ERR_BUF_FORMAT	: TLV�洢����ʽ����
int iTlvDbInfo(uchar *psTlvBuffer, uint *puiSpaceUsed, uint *puiSpaceLeft)
{
    _vTlvGetHead(psTlvBuffer);
    if(gl_TlvBufferHead.uiIdentifier != TLV_BUFFER_IDENTIFIER)
        return(TLV_ERR_BUF_FORMAT);
	if(puiSpaceUsed)
		*puiSpaceUsed = gl_TlvBufferHead.uiSpaceUsed;
	if(puiSpaceLeft)
		*puiSpaceLeft = gl_TlvBufferHead.uiSpaceLeft;
	return(gl_TlvBufferHead.uiTlvObjectNum);
}

// ���ܣ����TLV����TLV�洢��
// ���룺psTlvBuffer		: TLV�洢��
//       psTlvObject		: TLV���󣬿����ǽṹTLV�����TLV
//       iConflictFlag		: ��ͻ�����־, TLV_CONFLICT_ERR|TLV_CONFLICT_IGNORE|TLV_CONFLICT_REPLACE
// ���أ�>0					: ��ӳɹ����Ѵ�����ȫ��ͬ��TLV����, �����С
// 	     TLV_ERR_BUF_SPACE	: TLV�洢�����㹻�Ĵ洢�ռ�
//  	 TLV_ERR_BUF_FORMAT	: TLV�洢����ʽ����
//       TLV_ERR_CONFLICT	: ��ͻ��TLV�����Ѵ���
//       TLV_ERR_OBJ_FORMAT	: TLV����ṹ�Ƿ�
int iTlvAddObj(uchar *psTlvBuffer, uchar *psTlvObject, int iConflictFlag)
{
    uchar *psCurrTlvObject; // current TLV object in the buffer with the same tag
    int   iTlvObjLength;

    if(_iTlvCheckTlvObj(psTlvObject) <= 0)
        return(TLV_ERR_OBJ_FORMAT);
    
    iTlvObjLength = iTlvGetObj(psTlvBuffer, psTlvObject, &psCurrTlvObject);
    if(iTlvObjLength == TLV_ERR_BUF_FORMAT)
        return(TLV_ERR_BUF_FORMAT); // format error

    if(iTlvObjLength > 0) {
		// TLV�����Ѿ�����
		if(iConflictFlag == TLV_CONFLICT_IGNORE)
			return(iTlvObjLength); // ���Գ�ͻ������ԭTLV�����С
		if(iConflictFlag == TLV_CONFLICT_ERR)
			return(TLV_ERR_CONFLICT); // ����
		iTlvDelObj(psTlvBuffer, psTlvObject); // ɾ��ԭTLV����
    }

    iTlvObjLength = _uiTlvGetObjLen(psTlvObject);
    if(gl_TlvBufferHead.uiSpaceLeft < iTlvObjLength)
        return(TLV_ERR_BUF_SPACE); // no enough space
    memcpy(psTlvBuffer+10+gl_TlvBufferHead.uiSpaceUsed, psTlvObject, iTlvObjLength);
    gl_TlvBufferHead.uiTlvObjectNum ++;
    gl_TlvBufferHead.uiSpaceLeft -= iTlvObjLength;
    gl_TlvBufferHead.uiSpaceUsed += iTlvObjLength;
    _vTlvPutHead(psTlvBuffer);

    return(iTlvObjLength);
}

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
int iTlvMakeAddObj(uchar *psTlvBuffer, uchar *psTag, uint uiLength, uchar *psValue, int iConflictFlag)
{
	int iRet;
	uchar sTlvObject[260];
	iRet = iTlvMakeObject(psTag, uiLength, psValue, sTlvObject, sizeof(sTlvObject));
	if(iRet < 0)
		return(iRet);
	iRet = iTlvAddObj(psTlvBuffer, sTlvObject, iConflictFlag);
	return(iRet);
}

// ���ܣ���TLV�洢����ɾ��TLV���󣬿���ʡ���ռ���������TLV����
// ���룺psTlvBuffer		: TLV�洢��
//       psTag				: Ҫɾ����TLV�����Tag
// ���أ�>0					: ���, ��ɾ����TLV�����С
// 	     TLV_ERR_BUF_FORMAT	: TLV�洢����ʽ����
//       TLV_ERR_NOT_FOUND	: û�ҵ���Ӧ��TLV����
int iTlvDelObj(uchar *psTlvBuffer, uchar *psTag)
{
    uchar *psTlvObject;
    int   iTlvObjLength;
    
    iTlvObjLength = iTlvGetObj(psTlvBuffer, psTag, &psTlvObject);
    if(iTlvObjLength < 0)
        return(iTlvObjLength); // not found or error

    memmove(psTlvObject, psTlvObject+iTlvObjLength,
            psTlvBuffer+10+gl_TlvBufferHead.uiTotalSpace-psTlvObject-iTlvObjLength);

    gl_TlvBufferHead.uiTlvObjectNum --;
    gl_TlvBufferHead.uiSpaceLeft += iTlvObjLength;
    gl_TlvBufferHead.uiSpaceUsed -= iTlvObjLength;
    _vTlvPutHead(psTlvBuffer);
    
    return(iTlvObjLength);
}

// ���ܣ���TLV�洢����ȡ����ӦTag��TLV�����ָ��
// ���룺psTlvBuffer		: TLV�洢��
// 	     psTag				: Ҫ��ȡ��TLV�����Tag
// �����ppsTlvObject		: TLV����ָ��
// ���أ�>0					: ��ɣ�TLV����ĳ���
//  	 TLV_ERR_BUF_FORMAT	: TLV�洢����ʽ����
//  	 TLV_ERR_NOT_FOUND	: û�ҵ���Ӧ��TLV����
int iTlvGetObj(uchar *psTlvBuffer,uchar *psTag,uchar **ppsTlvObject)
{
    uint   uiTlvObjLen;
    int    iCounter;
    uchar  *psPointer;
    
    _vTlvGetHead(psTlvBuffer);
    if(gl_TlvBufferHead.uiIdentifier != TLV_BUFFER_IDENTIFIER)
        return(TLV_ERR_BUF_FORMAT);
    if(gl_TlvBufferHead.uiTotalSpace !=
                        gl_TlvBufferHead.uiSpaceLeft + gl_TlvBufferHead.uiSpaceUsed)
        return(TLV_ERR_BUF_FORMAT);
        
    psPointer = psTlvBuffer + 10; // point to the storage part
    for(iCounter=0; iCounter<gl_TlvBufferHead.uiTlvObjectNum; iCounter++) {
        uiTlvObjLen = _uiTlvGetObjLen(psPointer);
        if(!memcmp(psPointer, psTag, _uiTlvGetTagLen(psTag))) {
            *ppsTlvObject = psPointer;
            return(uiTlvObjLen);
        }
        psPointer += uiTlvObjLen;
    } // for(iCounter=0; iCounter<gl_TlvBufferHead.uiTlvObjectNum; iCounter++
    return(TLV_ERR_NOT_FOUND); // not found
}

// ���ܣ���TLV�洢����ȡ����Ӧλ�õ�TLV�����ָ�룬���ڱ���TLV�洢��
// ���룺psTlvBuffer		: TLV�洢��
// 	     uiIndex			: �����ţ�0��ʼ
// �����ppsTlvObject		: TLV����ָ��
// ���أ�>0					: ��ɣ�TLV����ĳ���
//	     TLV_ERR_BUF_FORMAT	: TLV�洢����ʽ����
//	     TLV_ERR_NOT_FOUND	: û�ҵ���Ӧ��TLV����
int iTlvGetObjByIndex(uchar *psTlvBuffer, uint uiIndex, uchar **ppsTlvObject)
{
    uint   uiTlvObjLen;
    int    iCounter;
    uchar  *psPointer;
    
    _vTlvGetHead(psTlvBuffer);
    if(gl_TlvBufferHead.uiIdentifier != TLV_BUFFER_IDENTIFIER)
        return(TLV_ERR_BUF_FORMAT);
    if(gl_TlvBufferHead.uiTotalSpace !=
                        gl_TlvBufferHead.uiSpaceLeft + gl_TlvBufferHead.uiSpaceUsed)
        return(TLV_ERR_BUF_FORMAT);
    if(uiIndex >= gl_TlvBufferHead.uiTlvObjectNum)
        return(TLV_ERR_NOT_FOUND);
        
    psPointer = psTlvBuffer + 10; // point to the storage part
    for(iCounter=0; iCounter<=(int)uiIndex; iCounter++) {
        uiTlvObjLen = _uiTlvGetObjLen(psPointer);
        psPointer += uiTlvObjLen;
    } // for(iCounter=0; iCounter<gl_TlvBufferHead.uiTlvObjectNum; iCounter++
    *ppsTlvObject = psPointer - uiTlvObjLen;
    return(uiTlvObjLen);
}

// ���ܣ���TLV�洢����ȡ����ӦTag��TLV����ֵ��ָ��
// ���룺psTlvBuffer		: TLV�洢��
// 	     psTag				: Ҫ��ȡ��TLV�����Tag
// �����ppsValue			: TLV����ֵ��ָ��
// ���أ�>=0				: TLV����ֵ����
//	     TLV_ERR_BUF_FORMAT	: TLV�洢����ʽ����
//	     TLV_ERR_NOT_FOUND	: û�ҵ���Ӧ��TLV����
int iTlvGetObjValue(uchar *psTlvBuffer, uchar *psTag, uchar **ppsValue)
{
    int   iRet;
    uchar *psTlvObject;
    int   iTagLenLengthLen;
    
    iRet = iTlvGetObj(psTlvBuffer, psTag, &psTlvObject);
    if(iRet < 0)
        return(iRet);
    iTagLenLengthLen = _uiTlvGetTagLen(psTlvObject)+_uiTlvGetLengthLen(psTlvObject);
    *ppsValue = psTlvObject+iTagLenLengthLen;
    return(iRet-iTagLenLengthLen);
}

// ���ܣ���ȡTLV����ֵ
// ���룺psTlvObj			: TLV����
// �����ppsTlvObjValue	    : TLV����ֵ
// ���أ�>=0				: TLV����ֵ��С
//	     TLV_ERR_OBJ_FORMAT	: TLV�����ʽ����
int iTlvValue(uchar *psTlvObj, uchar **ppsTlvObjValue)
{
    uint   uiRet;
    uchar  ucPos;

    uiRet = _uiTlvGetTagLen(psTlvObj);
    if(uiRet == 0)
        return(TLV_ERR_BUF_FORMAT);
    ucPos = (uchar)uiRet;

    uiRet = _uiTlvGetLengthLen(psTlvObj);
    if(uiRet == 0)
        return(TLV_ERR_BUF_FORMAT);
    ucPos += (uchar)uiRet;
    
    *ppsTlvObjValue = psTlvObj + ucPos;
    return(_uiTlvGetValueLen(psTlvObj));
}

// ���ܣ���ȡTLV����ֵ����
// ���룺psTlvObj			: TLV����
// ���أ�>=0				: TLV����ֵ��С
//	     TLV_ERR_OBJ_FORMAT	: TLV�����ʽ����
int iTlvValueLen(uchar *psTlvObj)
{
    uint  uiRet;

	uiRet = _uiTlvGetValueLen(psTlvObj);
	if(uiRet == 0xFFFF)
		return(TLV_ERR_OBJ_FORMAT);
	return(uiRet);
}

// ���ܣ���ȡTLV����Tag����
// ���룺psTlvObj			: TLV����
// ���أ�1 or 2				: TLV����Tag��С
//	     TLV_ERR_OBJ_FORMAT	: TLV�����ʽ����
int iTlvTagLen(uchar *psTlvObj)
{
	uint uiRet;
	uiRet = _uiTlvGetTagLen(psTlvObj);
	if(uiRet == 0)
		return(TLV_ERR_OBJ_FORMAT);
	return(uiRet);
}

// ���ܣ���ȡTLV����Length����
// ���룺psTlvObj			: TLV����
// ���أ�1 or 2				: TLV����Len��С
//	     TLV_ERR_OBJ_FORMAT	: TLV�����ʽ����
int iTlvLengthLen(uchar *psTlvObj)
{
	uint uiRet;
	uiRet = _uiTlvGetLengthLen(psTlvObj);
	if(uiRet == 0)
		return(TLV_ERR_OBJ_FORMAT);
	return(uiRet);
}

// ���ܣ���ȡTLV���󳤶�
// ���룺psTlvObj			: TLV����
// ���أ�>0                 : TLV�����С
//	     TLV_ERR_OBJ_FORMAT	: TLV�����ʽ����
// ע��: 00 or FF����Ϊ�Ǹ�ʽ����
int iTlvObjLen(uchar *psTlvObj)
{
    int    iRet;
    iRet = _iTlvCheckTlvObj(psTlvObj);
    if(iRet < 0)
        return(TLV_ERR_OBJ_FORMAT);
	return(iRet);
}

// ���ܣ���ȡTLV����ֵ
// ���룺psTlvObj			: TLV����
// ���أ�!NULL			    : TLV����ֵ
//	     NULL				: TLV�����ʽ����
uchar *psTlvValue(uchar *psTlvObj)
{
    uint uiRet;
    uchar  ucPos;

    uiRet = _uiTlvGetTagLen(psTlvObj);
    if(uiRet == 0)
        return(NULL);
    ucPos = (uchar)uiRet;

    uiRet = _uiTlvGetLengthLen(psTlvObj);
    if(uiRet == 0)
        return(NULL);
    ucPos += (uchar)uiRet;
    
	return(psTlvObj + ucPos);
}

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
int iTlvSearchObj(uchar *psTemplate, int iTemplateLen, int iIndex,
                  uchar *psTagRoute, uchar **ppsTlvObj)
{
    int    iDataLen;              // data left needed to be processed
    uchar  *psData;               // data pointer being processed
    uchar  *psTag;                // Current processed tag
    uint   uiTagLen, uiTlvObjLen; //
    int    iRet;
    
    psData = psTemplate;
    iDataLen = iTemplateLen;
    psTag = psTagRoute;
    while(iDataLen > 0) {
        while(*psData == 0x00 || *psData == 0xff) {
            // pass 00s and FFs
            psData ++;
            iDataLen --;
        }
        if(iDataLen <= 0)
            break;

        // check integrity
        iRet = _iTlvCheckTlvObj(psData);
        if(iRet < 0 || iRet > iDataLen)
            return(TLV_ERR_BUF_FORMAT);
        uiTlvObjLen = iRet;
        
        uiTagLen = _uiTlvGetTagLen(psTag);
        if(*psTag == 0xFF)
            uiTagLen = 1; // '\xFF'�趨Ϊ���ֽ�ͨ��Tag
        if(!memcmp(psData, psTag, uiTagLen) || *psTag==0xFF) {
            // match the current tag
            if(psTag[uiTagLen] != 0) {
                // match, but not the final tag, search into the template
                psTag += uiTagLen;
                iDataLen = _uiTlvGetValueLen(psData);
                psData += _uiTlvGetTagLen(psData) + _uiTlvGetLengthLen(psData);
                continue;
            }
            // match, and it's the final tag
            if(iIndex == 0) {
                // found the desired TLV object
                // inserted by yujun 20080820
                // ����EMV4.1 book3 5.2, ���TLV����ֵ�򳤶�Ϊ0����Ϊ������
                if(_uiTlvGetValueLen(psData) == 0)
                    return(TLV_ERR_NOT_FOUND);
                // inserted by yujun 20080820 end
                *ppsTlvObj = psData;
                return(uiTlvObjLen);
            }
            // skip this match
            iIndex --;
            psData += uiTlvObjLen;
            iDataLen -= uiTlvObjLen;
            continue;
        } // if(!memcmp(psData, psTag, uiTagLen
        
        // does not match the current tag
        psData += uiTlvObjLen;
        iDataLen -= uiTlvObjLen;
    } // while(iDataLen > 0    
    
    return(TLV_ERR_NOT_FOUND);
}

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
int iTlvSearchObjValue(uchar *psTemplate, int iTemplateLen, int iIndex,
                       uchar *psTagRoute, uchar **ppsTlvObjValue)
{
    short iRet;
    uchar *psTlvObj;
    
    iRet = iTlvSearchObj(psTemplate, iTemplateLen, iIndex, psTagRoute, &psTlvObj);
    if(iRet < 0)
        return(iRet);
    *ppsTlvObjValue = psTlvObj + _uiTlvGetTagLen(psTlvObj) +
                                 _uiTlvGetLengthLen(psTlvObj);
    return(_uiTlvGetValueLen(psTlvObj));
}

// ���ܣ���TLVģ����ȡ��ָ��λ�õ�TLV����(�����ǻ���TLV����Ҳ����������һ��ģ��)�����ڱ���ģ��
// ���룺psTemplate			: TLVģ��
//       iTemplateLen       : ģ���С
// 	     iIndex			    : ָ���ڼ�����0��ʾ��һ��
// �����ppsTlvObj		    : TLV����
// ���أ�>0					: �ҵ���TLV�����С
//	     TLV_ERR_BUF_FORMAT	: TLVģ���ʽ����
//	     TLV_ERR_NOT_FOUND	: �����ڸ������Ķ���
int iTlvSerachTemplate(uchar *psTemplate, int iTemplateLen, int iIndex, uchar **ppsTlvObj)
{
    int    iDataLen;              // data left needed to be processed
    uchar  *psData;               // data pointer being processed
    int    iRet;
    
    // check integrity
    iRet = _iTlvCheckTlvObj(psTemplate);
    if(iRet < 0 || iRet > iTemplateLen)
        return(TLV_ERR_BUF_FORMAT);

    if(((psTemplate[0] & 0x20) != 0x20))
		return(TLV_ERR_BUF_FORMAT); // ����ģ��

	iDataLen = iTlvValue(psTemplate, &psData); // ��ȡģ���ֵ
    while(iDataLen > 0) {
        while((*psData==0x00 || *psData==0xff) && (iDataLen>0)) {
            // pass 00s and FFs
            psData ++;
            iDataLen --;
        }
        if(iDataLen <= 0)
            break;

        // check integrity
        iRet = _iTlvCheckTlvObj(psData);
        if(iRet < 0 || iRet > iDataLen)
            return(TLV_ERR_BUF_FORMAT);
		if(iIndex <= 0) {
			// �ҵ�
			*ppsTlvObj = psData;
			return(iRet);
		}

		iIndex --;
		psData += iRet;
		iDataLen -= iRet;
    } // while(iDataLen > 0    
    
    return(TLV_ERR_NOT_FOUND);
}

// ���ܣ����TLV�����Ƿ�Ϸ�
// ���룺psTlvObj           : TLV����ָ��
// ���أ�>0					: �ҵ���TLV����ĳ���
//	     TLV_ERR_OBJ_FORMAT	: TLV�����ʽ����
//       TLV_ERR_OBJ_LENGTH : TLV���󳤶Ȳ�����EMV�淶
int iTlvCheckTlvObject(uchar *psTlvObj)
{
    int    iRet, iValueLen;
	uchar  *psTlvObjValue;
    uint   uiRet;
	uint   uiTagType;
    int    iLeast, iMost;

    iRet = _iTlvCheckTlvObj(psTlvObj);
    if(iRet < 0)
        return(TLV_ERR_OBJ_FORMAT);
        
    uiRet = _uiTlvGetValueLen(psTlvObj);
    if(uiRet == 0xffff)
        return(TLV_ERR_OBJ_FORMAT);

    iValueLen = (short)uiRet;
    uiTagType = uiTagAttrGetRange(psTlvObj, &iLeast, &iMost);
	switch(uiTagType) {
	case TAG_ATTR_N:
	    if(iValueLen<(iLeast+1)/2 || iValueLen>(iMost+1)/2)
		    return(TLV_ERR_OBJ_LENGTH);
		if(iValueLen== (iMost+1)/2 && iMost%2) {
			// ��ǰ��������Ѵﵽ��󳤶� �� �淶�������������󳤶�Ϊ����
			// ���������������4bit�Ƿ�Ϊ0, �����Ϊ0����Ϊ�������
			iTlvValue(psTlvObj, &psTlvObjValue);
			if((psTlvObjValue[0]&0xF0) != 0x00)
				return(TLV_ERR_OBJ_LENGTH);
		}
		break;
	case TAG_ATTR_CN:
	    if(iValueLen<(iLeast+1)/2 || iValueLen>(iMost+1)/2)
		    return(TLV_ERR_OBJ_LENGTH);
		if(iValueLen== (iMost+1)/2 && iMost%2) {
			// ��ǰ��������Ѵﵽ��󳤶� �� �淶�������������󳤶�Ϊ����
			// ���������������4bit�Ƿ�ΪF, �����ΪF����Ϊ�������
			iTlvValue(psTlvObj, &psTlvObjValue);
			if((psTlvObjValue[iValueLen-1]&0x0F) != 0x0F)
				return(TLV_ERR_OBJ_LENGTH);
		}
		break;
	case TAG_ATTR_UNKNOWN:
		return(_uiTlvGetObjLen(psTlvObj)); // unknown tag, don't check it's length
	default:
	    if(iValueLen<iLeast || iValueLen>iMost)
		    return(TLV_ERR_OBJ_LENGTH);
		break;
	}
    return(_uiTlvGetObjLen(psTlvObj));
}

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
int iTlvBatchAddObj(uchar ucFlag, uchar *psTlvBuffer, uchar *psTlvTemplate, int iTlvTemplateLen, int iConflictFlag, int iLen0Flag)
{
    int    iDataLen;              // data left needed to be processed
    uchar  *psData;               // data pointer being processed
    int    iRet;
    uint   uiTlvObjLen;           // 
	int    iCount;
    
    psData = psTlvTemplate;
    iDataLen = iTlvTemplateLen;
	iCount = 0;

    // pass 00s & FFs
    while(*psData == 0x00 || *psData == 0xff) {
        psData ++;
        iDataLen --;
    }
    // validity check
// Aut 10th, 2012
//    if(*psData != 0x70 && *psData != 0x77)
//        return(TLV_ERR_OTHER);
    iRet = _iTlvCheckTlvObj(psData);
    if(iRet < 0 || iRet > iDataLen)
        return(TLV_ERR_TEMP_FORMAT);

    // point to the content of the template
    iDataLen = _uiTlvGetValueLen(psData);
    psData += _uiTlvGetTagLen(psData) + _uiTlvGetLengthLen(psData);

    while(iDataLen > 0) {
        while(*psData == 0x00 || *psData == 0xff) {
            // pass 00s and FFs
            psData ++;
            iDataLen --;
        }
        if(iDataLen <= 0)
            break;

        // check integrity
        iRet = _iTlvCheckTlvObj(psData);
        if(iRet < 0 || iRet > iDataLen)
            return(TLV_ERR_OBJ_FORMAT);
        uiTlvObjLen = iRet;

        if(((psData[0] & 0x20) != 0x20) || ucFlag) {
            // it's a primitive TLV object or allow to add all TLV object
			if(iLen0Flag || _uiTlvGetValueLen(psData)!=0) {
				if(uiTagAttrGetFrom(psData) == TAG_FROM_CARD) { // by yujun 20120420, ֻ������IC��Ƭ��Tlv������������
					iCount ++;
		            iRet = iTlvAddObj(psTlvBuffer, psData, iConflictFlag);
			        if(iRet < 0)
				        return(iRet);       // error
				}
            }
        }
        
        // pass the TLV object
        psData += uiTlvObjLen;
        iDataLen -= uiTlvObjLen;
    } // while(iDataLen > 0
    return(iCount);
}

// ���ܣ���8583 55���ڵ�����TLV������ӵ�TLV�洢��(ֻ��ӵ�һ��TLV���󣬲���ݹ����)
// ���룺ucFlag             : 0:ֻ��ӻ���TLV����!0:�������TLV����
//       psTlvBuffer		: TLV�洢��
//       psField55			: 55��
//       iField55Len	    : 55�򳤶�
//       iConflictFlag		: ��ͻ�����־, TLV_CONFLICT_ERR|TLV_CONFLICT_IGNORE|TLV_CONFLICT_REPLACE
//       iHostFlag          : 1:��̨�� 0:���Ǻ�̨��
// ���أ�>=0				: ���, ����ֵΪ��ӵ�TLV�������
// 	     TLV_ERR_BUF_SPACE	: TLV�洢�����㹻�Ĵ洢�ռ�
//	     TLV_ERR_BUF_FORMAT	: TLV�洢����ʽ����
//	     TLV_ERR_CONFLICT	: ��ͻ����ֹ����
//       TLV_ERR_OBJ_FORMAT	: 55����Tlv�����ʽ�Ƿ�
// Note: �ṩ��iTlvBatchAddField55Obj()��iTlvBatchAddField55ObjHost()����
//       ����71��72�ű����ܻ��ж��, 71��72�ű��ᱻ�����DFF1��DFF2���б���
static int iTlvBatchAddField55ObjBase(uchar ucFlag, uchar *psTlvBuffer, uchar *psField55, int iField55Len, int iConflictFlag, int iHostFlag)
{
    int    iDataLen;                  // data left needed to be processed
    uchar  *psData;                   // data pointer being processed
    int    iRet;
    uint   uiTlvObjLen;               // 
	uchar  sPackage[256], *psPackage; // �ű���
	int    iPackageLen;			      // �ű�������
	int    iCount;
    
    psData = psField55;
    iDataLen = iField55Len;
	iCount = 0;

    while(iDataLen > 0) {
        while(*psData == 0x00 || *psData == 0xff) {
            // pass 00s and FFs
            psData ++;
            iDataLen --;
        }
        if(iDataLen <= 0)
            break;

        // check integrity
		iRet = iTlvCheckTlvObject(psData);
		if(iRet > iDataLen)
			return(TLV_ERR_OBJ_FORMAT);
		if(iRet < 0) {
			if(iRet != TLV_ERR_OBJ_LENGTH)
				return(TLV_ERR_OBJ_FORMAT);
			iRet = iTlvValueLen(psData);
			if(iRet != 0)
				return(TLV_ERR_OBJ_FORMAT); // Tlv����ֵ���ȴ��ҳ��Ȳ�����0, ��ΪTlv�����ʽ����
			// Tlv����ֵ����0, ���Ըö���
			iRet = iTlvObjLen(psData);
	        // pass the TLV object
		    psData += iRet;
			iDataLen -= iRet;
			if(iDataLen < 0)
				return(TLV_ERR_OBJ_FORMAT);
			continue;
		}
        uiTlvObjLen = iRet;

        if(((psData[0] & 0x20) != 0x20) || ucFlag) {
            // it's a primitive TLV object or allow to add all TLV object
            if(_uiTlvGetValueLen(psData) != 0) { // inserted by yujun 20080820, ����EMV4.1 book3 5.2, ���TLV����ֵ�򳤶�Ϊ0����Ϊ������
				// modified by yujun, 20140318, ����iHostFlag�����Ǻ�̨���û����ն˵���
				if((uiTagAttrGetFrom(psData)==TAG_FROM_ISSUER && iHostFlag==0) || (uiTagAttrGetFrom(psData)!=TAG_FROM_ISSUER && iHostFlag!=0)) {
					iCount ++;
					if(psData[0]==0x71 || psData[0]==0x72) {
						// �ű���ʶ, ��Ҫ�洢��DFF1��DFF2����
						if(psData[0] == 0x71)
							iPackageLen = iTlvGetObjValue(psTlvBuffer, "\xDF\xF1", &psPackage);
						else
							iPackageLen = iTlvGetObjValue(psTlvBuffer, "\xDF\xF2", &psPackage);
						if(iPackageLen <= 0) {
							iPackageLen = 0;
						} else {
							memcpy(sPackage, psPackage, iPackageLen);
						}
						if(iPackageLen+uiTlvObjLen > sizeof(sPackage))
							return(TLV_ERR_OTHER); // sBuf����������
						memcpy(sPackage+iPackageLen, psData, uiTlvObjLen);
						iPackageLen += uiTlvObjLen;
						if(psData[0] == 0x71)
							iRet = iTlvMakeAddObj(psTlvBuffer, "\xDF\xF1", iPackageLen, sPackage, TLV_CONFLICT_REPLACE);
						else
							iRet = iTlvMakeAddObj(psTlvBuffer, "\xDF\xF2", iPackageLen, sPackage, TLV_CONFLICT_REPLACE);
						if(iRet < 0)
							return(iRet);
					} else {
						// ����Tag, ֱ�ӱ���
			            iRet = iTlvAddObj(psTlvBuffer, psData, iConflictFlag);
				        if(iRet < 0)
					        return(iRet);       // error
					}
				}
            }
        }
        
        // pass the TLV object
        psData += uiTlvObjLen;
        iDataLen -= uiTlvObjLen;
    } // while(iDataLen > 0
    return(iCount);
}

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
//       TLV_ERR_OBJ_FORMAT	: 55����Tlv�����ʽ�Ƿ�
// Note: ֻ������Է�������Tlv����
//       ����71��72�ű����ܻ��ж��, 71��72�ű��ᱻ�����DFF1��DFF2���б���
int iTlvBatchAddField55Obj(uchar ucFlag, uchar *psTlvBuffer, uchar *psField55, int iField55Len, int iConflictFlag)
{
	int iRet;
	iRet = iTlvBatchAddField55ObjBase(ucFlag, psTlvBuffer, psField55, iField55Len, iConflictFlag, 0);
	return(iRet);
}

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
int iTlvBatchAddField55ObjHost(uchar ucFlag, uchar *psTlvBuffer, uchar *psField55, int iField55Len, int iConflictFlag)
{
	int iRet;
	iRet = iTlvBatchAddField55ObjBase(ucFlag, psTlvBuffer, psField55, iField55Len, iConflictFlag, 1);
	return(iRet);
}

// ���ܣ���һ��TLV�洢���ڵ�����TLV������ӵ���һ��TLV�洢����
// ���룺psTlvSourceBuffer  : ԴTLV�洢��
//       iConflictFlag		: ��ͻ�����־, TLV_CONFLICT_ERR|TLV_CONFLICT_IGNORE|TLV_CONFLICT_REPLACE
// �����psTlvTargetBuffer	: Ŀ��TLV�洢��
// ���أ�>=0				: ���, ������TLV�������
// 	     TLV_ERR_BUF_SPACE	: Ŀ��TLV�洢�����㹻�Ĵ洢�ռ�
//	     TLV_ERR_BUF_FORMAT	: ����TLV�洢����ʽ����
//	     TLV_ERR_CONFLICT	: ��ͻ����ֹ����
int iTlvCopyBuffer(uchar *psTlvTargetBuffer, uchar *psTlvSourceBuffer, int iConflictFlag)
{
    int   iRet, i;
    uchar *psTlvObject;

    for(i=0; ; i++) {
        iRet = iTlvGetObjByIndex(psTlvSourceBuffer, i, &psTlvObject);
        if(iRet == TLV_ERR_NOT_FOUND)
            break;
        if(iRet < 0)
            return(iRet);
        iRet = iTlvAddObj(psTlvTargetBuffer, psTlvObject, iConflictFlag);
        if(iRet < 0)
            return(iRet);
    } // for(i=0; ; i++
    return(i);
}

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
int iTlvMakeObject(uchar *psTag, uint uiLength, uchar *psValue, uchar *psTlvObject, int iTlvObjectBufSize)
{
    int iObjLen;
    
    // check tag
    if(psTag[0] == 0x00 || psTag[0] == 0xff)
        return(TLV_ERR_OTHER); // '00' tag or 'ff' tag
    if((psTag[0] & 0x1f) == 0x1f && (psTag[1] & 0x80) == 0x80) {
		if(psTag[0] != 0xDF)
			return(TLV_ERR_OTHER); // 20130308, ֻ�з�DFxx, �ż��Tag��2�ֽڵ����λ, ����Ƿ�, ��Ϊ����
	}
    
    // check length
    if(uiLength > 255)
        return(TLV_ERR_OTHER);
    
    if((psTag[0] & 0x1f) == 0x1f)
        iObjLen = 2;
    else
        iObjLen = 1;
	iTlvObjectBufSize -= iObjLen;
	if(iTlvObjectBufSize < 0)
		return(TLV_ERR_BUF_SPACE);
    memcpy(psTlvObject, psTag, iObjLen);
    psTlvObject += iObjLen;

    if(uiLength > 127) {
		iTlvObjectBufSize --;
		if(iTlvObjectBufSize < 0)
			return(TLV_ERR_BUF_SPACE);
        *psTlvObject++ = 0x81;
        iObjLen ++;
    }
	iTlvObjectBufSize --;
	if(iTlvObjectBufSize < 0)
		return(TLV_ERR_BUF_SPACE);
    *psTlvObject++ = (uchar)uiLength;
    iObjLen ++;

	iTlvObjectBufSize -= uiLength;
	if(iTlvObjectBufSize < 0)
		return(TLV_ERR_BUF_SPACE);
    memcpy(psTlvObject, psValue, (int)uiLength);
    iObjLen += uiLength;
    return(iObjLen);
}

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
          	         uchar *psTlvBuffer1, uchar *psTlvBuffer2, uchar *psTlvBuffer3, uchar *psTlvBuffer4)
{
    uint   uiTagLen;
    uint   uiTagAttr;
    int    iBlockLen, i;
    int    iValueLen;       // length of value from TLV buffer
    int    iIndicatedLen;   // length indicated by DOL
    uchar  *psTlvBuffer, *psTlvObjValue;
    
    iBlockLen = 0;
    while(iDOLLen > 0) {
        uiTagLen = _uiTlvGetTagLen(psDOL);
        if(uiTagLen == 0)
            return(TLV_ERR_DOL_FORMAT); // DOL format error
        if(iDOLLen < (int)uiTagLen+1)
            break;

        // clear the value field first
        iIndicatedLen = psDOL[uiTagLen];
		if(iBlockLen+iIndicatedLen > iBlockSize)
			return(TLV_ERR_BUF_SPACE);
        memset((char *)psBlock, 0, (int)iIndicatedLen);

        uiTagAttr = uiTagAttrGetType(psDOL);
        if(uiTagAttr == TAG_ATTR_UNKNOWN || (psDOL[0]&0x20))
            goto label_next_tag; // unknown or is not a primitive TLV object, skip it

        for(i=1; i<=4; i++) {
            // search the 3 entries
            if(i == 1)
                psTlvBuffer = psTlvBuffer1;
            else if(i == 2)
                psTlvBuffer = psTlvBuffer2;
            else if(i == 3)
                psTlvBuffer = psTlvBuffer3;
            else
                psTlvBuffer = psTlvBuffer4;
            if(!psTlvBuffer)
                continue;
            iValueLen = iTlvGetObjValue(psTlvBuffer, psDOL, &psTlvObjValue);
            if(iValueLen == TLV_ERR_BUF_FORMAT)
                return(TLV_ERR_BUF_FORMAT);
            if(iValueLen >= 0)
                break;
        } // for(i=1; i<=4; i++
        if(i <= 4) {
            // found, attach it to the block, refer to EMV2004 book3 Part II, 5.4.2
            if(iIndicatedLen < iValueLen && uiTagAttr == TAG_ATTR_N) {
                memcpy((char *)psBlock, psTlvObjValue+(iValueLen-iIndicatedLen), iIndicatedLen);
                goto label_next_tag;
            }
            if(iIndicatedLen <= iValueLen) {
                memcpy((char *)psBlock, psTlvObjValue, iIndicatedLen);
                goto label_next_tag;
            }
            if(uiTagAttr == TAG_ATTR_N) {
                memcpy((char *)psBlock+(iIndicatedLen-iValueLen), psTlvObjValue, iValueLen);
                goto label_next_tag;
            }
            if(uiTagAttr == TAG_ATTR_CN)
                memset((char *)psBlock, 0xff, (int)iIndicatedLen);
            memcpy((char *)psBlock, psTlvObjValue, iValueLen);
        } // if(i <= 4
label_next_tag:
        iDOLLen -= uiTagLen+1;
        psDOL += uiTagLen+1;
        psBlock += iIndicatedLen;
        iBlockLen += iIndicatedLen;
    } // while(iDOLLength > 0
    return(iBlockLen);
}

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
          	    	 uchar *psTlvBuffer1, uchar *psTlvBuffer2, uchar *psTlvBuffer3, uchar *psTlvBuffer4)
{
    uint   uiTagLen;
    uint   uiTagAttr;
    int    iBlockLen, i;
    int    iValueLen;       // length of value from TLV buffer
	int    iTlvObjLen;      // length of found TLV object
    uchar  *psTlvBuffer, *psTlvObjValue, *psTlvObj;
	uchar  sRecData[255];
	uchar  *psRecData;
    
    iBlockLen = 0;
	psRecData = sRecData;
    while(iTagsLen > 0) {
        uiTagLen = _uiTlvGetTagLen(psTags);
        if(uiTagLen == 0)
            return(TLV_ERR_OTHER); // format error
        if(iTagsLen < (int)uiTagLen)
			return(TLV_ERR_OTHER); // format error

        uiTagAttr = uiTagAttrGetType(psTags);
        if(uiTagAttr == TAG_ATTR_UNKNOWN) {
            iTagsLen -= uiTagLen;
            psTags += uiTagLen;
			continue;
		}

        for(i=1; i<=4; i++) {
            // search the 3 entries
            if(i == 1)
                psTlvBuffer = psTlvBuffer1;
            else if(i == 2)
                psTlvBuffer = psTlvBuffer2;
            else if(i == 3)
                psTlvBuffer = psTlvBuffer3;
            else
                psTlvBuffer = psTlvBuffer4;
            if(!psTlvBuffer)
                continue;
            iValueLen = iTlvGetObjValue(psTlvBuffer, psTags, &psTlvObjValue);
            if(iValueLen == TLV_ERR_BUF_FORMAT)
                return(TLV_ERR_BUF_FORMAT);
            iTlvObjLen = iTlvGetObj(psTlvBuffer, psTags, &psTlvObj);
            if(iTlvObjLen == TLV_ERR_BUF_FORMAT)
                return(TLV_ERR_BUF_FORMAT);
            if(iValueLen >= 0)
                break;
        } // for(i=1; i<=4; i++
		if(i > 4) {
            iTagsLen -= uiTagLen;
            psTags += uiTagLen;
			continue; // not found
		}
		if(iValueLen == 0) {
            iTagsLen -= uiTagLen;
            psTags += uiTagLen;
			continue; // found, but the length of value field is 0
		}

		if(iBlockLen+iTlvObjLen > 252)
			return(TLV_ERR_OTHER); // ��¼���ȳ���252

		memcpy(psRecData, psTlvObj, iTlvObjLen);
        iTagsLen -= uiTagLen;
        psTags += uiTagLen;
		psRecData += iTlvObjLen;
		iBlockLen += iTlvObjLen;
    } // while(iTagsLen > 0

	iBlockLen = iTlvMakeObject("\x70", iBlockLen, sRecData, psBlock, 254);
    return(iBlockLen);
}

// ���ܣ�����DOL���Ƿ����ĳһTag
// ���룺psDOL				: Data Object List
// 	     iDOLLen		    : DOL����
//       psTag              : Ҫ������Tag
// ���  piOffset           : ���DOL�а���������TAG, ���ظ�TAG��DOL���ݿ��е�ƫ����, NULL��ʾ����Ҫ����
// ���أ�>=0				: Tag����, ���ظ�Tag��Ҫ�ĳ���
//       TLV_ERR_DOL_FORMAT : DOL��ʽ����
// ע��: piOffset���ص����ø�DOL��ģ�����ɵ����ݿ��е�ƫ����
int iTlvSearchDOLTag(uchar *psDOL, int iDOLLen,	uchar *psTag, int *piOffset)
{
    uint uiTagLen;
	int  iOffset;

    iOffset = 0;
    while(iDOLLen > 0) {
        uiTagLen = _uiTlvGetTagLen(psDOL);
        if(uiTagLen == 0)
            return(TLV_ERR_DOL_FORMAT); // DOL format error
        if(iDOLLen < (int)uiTagLen+1)
            break;
        if(memcmp(psDOL, psTag, uiTagLen) == 0) {
            // found
			if(piOffset)
				*piOffset = iOffset;
            return(psDOL[uiTagLen]);
        }
		iOffset += psDOL[uiTagLen];
        iDOLLen -= uiTagLen+1;
        psDOL += uiTagLen+1;
    } // while(iDOLLength > 0
    return(TLV_ERR_NOT_FOUND);
}

// ���ܣ��˲�Tlv����˳��Ϸ���
// ���룺psTemplate			: TLVģ������
//       iTemplateLen       : ģ�����ݴ�С
//       psTagOrder         : Ҫ���TLV����˳��, '\x00'Tag��ʾ����
// ���أ�0					: �����ȷ
//	     TLV_ERR_BUF_FORMAT	: TLVģ���ʽ����
//       TLV_ERR_CHECK      : ���ʧ��
//       TLV_OK             : ���ɹ�
// ע��: psTagOrder������Tag��������ȷ˳�������psTemplate��
int iTlvCheckOrder(uchar *psTemplate, int iTemplateLen, uchar *psOrder)
{
#if 1
	// ����V2CA0990001v4.2c˵������˽��tag����, �������������, ֻ�ж�˳��
	int    i;
	uchar  *psTlvObj;
	int    iTlvObjLen;
	uint   uiTagLen;
	for(i=0; ; i++) {
		iTlvObjLen = iTlvSearchObj(psTemplate, iTemplateLen, i, "\xFF", &psTlvObj);
		if(iTlvObjLen == TLV_ERR_BUF_FORMAT)
			return(TLV_ERR_BUF_FORMAT);
		if(iTlvObjLen == TLV_ERR_NOT_FOUND)
			break; // ģ�����Ѿ�û��Tlv���ݶ�����
        uiTagLen = _uiTlvGetTagLen(psTlvObj);
		if(memcmp(psTlvObj, psOrder, uiTagLen) != 0)
			continue;
		psOrder += uiTagLen;
		if(*psOrder == 0)
			break; // psOrder��ʶ��Tagȫ��������˳������
	}
	if(*psOrder != 0)
		return(TLV_ERR_CHECK); // ģ��������ȱʧTlv���󲻴���, ����
    return(TLV_OK);
#else
	int    i;
	uchar  *psTlvObj;
	int    iTlvObjLen;
	uint   uiTagLen;
	for(i=0; ; i++) {
		iTlvObjLen = iTlvSearchObj(psTemplate, iTemplateLen, i, "\xFF", &psTlvObj);
		if(iTlvObjLen == TLV_ERR_BUF_FORMAT)
			return(TLV_ERR_BUF_FORMAT);
		if(iTlvObjLen == TLV_ERR_NOT_FOUND)
			break; // ģ�����Ѿ�û��Tlv���ݶ�����
		if(*psOrder == 0)
			return(TLV_ERR_CHECK); // ģ�������д��ڶ���Tlv����, ����
        uiTagLen = _uiTlvGetTagLen(psTlvObj);
		if(memcmp(psTlvObj, psOrder, uiTagLen) != 0)
			return(TLV_ERR_CHECK); // ģ��������Tlv˳�򲻶�, ����
		psOrder += uiTagLen;
	}
	if(*psOrder != 0)
		return(TLV_ERR_CHECK); // ģ��������ȱʧTlv���󲻴���, ����
    return(TLV_OK);
#endif
}

// ���ܣ��˲�Tlvģ����������Ƿ����ظ�
// ���룺psTemplate			: TLVģ������
//       iTemplateLen       : ģ�����ݴ�С
// ���أ�0					: �����ȷ
//	     TLV_ERR_BUF_FORMAT	: TLVģ���ʽ����
//       TLV_ERR_CHECK      : ���ʧ��
//       TLV_OK             : ���ɹ�
int iTlvCheckDup(uchar *psTemplate, int iTemplateLen)
{
	uchar sTlvBuf[400];
	int   iRet;

	iTlvSetBuffer(sTlvBuf, sizeof(sTlvBuf));
	iRet = iTlvCheckTlvObject(psTemplate);
	if(iRet<0 || iRet>iTemplateLen)
		return(TLV_ERR_CHECK);
	iRet = iTlvBatchAddObj(1/*1:������ж���*/, sTlvBuf, psTemplate, iTemplateLen, TLV_CONFLICT_ERR, 1/*1:����Ϊ0����Ҳ���*/);
	if(iRet == TLV_ERR_CONFLICT)
		return(TLV_ERR_CHECK);
	if(iRet < 0)
		return(TLV_ERR_BUF_FORMAT);
	return(TLV_OK);
}

# if 0
void vTestTlvFunc(void)
{
    uchar  sTlvBuf1[110], sTlvBuf2[60], sTlvBuf3[60], sTlvBuf4[60];
    uchar  *psTlvObject, *psTlvValue;
    uchar  sBuf[200], sBuf2[200];
    uchar  sDOL[50];
    int    iDOLLen;
    int    iCounter, iLen;
    int    iRet;

// short iTlvSetBuffer(uchar *psTlvBuffer, ushort uiTlvBufferSize);
    iRet = iTlvSetBuffer(sTlvBuf1, sizeof(sTlvBuf1));
    iRet = iTlvSetBuffer(sTlvBuf2, sizeof(sTlvBuf2));
    iRet = iTlvSetBuffer(sTlvBuf3, sizeof(sTlvBuf3));
    
//short iTlvAddObj(uchar *psTlvBuffer, uchar *psTlvObject);
    memcpy(sBuf, "\x88\x01\x01", 3);
    iRet = iTlvAddObj(sTlvBuf4, sBuf, TLV_CONFLICT_ERR); // error, not initialized
    iRet = iTlvAddObj(sTlvBuf1, sBuf, TLV_CONFLICT_ERR); // OK
    iRet = iTlvAddObj(sTlvBuf1, sBuf, TLV_CONFLICT_ERR); // OK, already exist, but identical
    memcpy(sBuf, "\x88\x01\x02", 3);
    iRet = iTlvAddObj(sTlvBuf1, sBuf, TLV_CONFLICT_ERR); // error, already exist
    memcpy(sBuf, "\x80\x20""11111111111111112222222222222222", 34);
    iRet = iTlvAddObj(sTlvBuf2, sBuf, TLV_CONFLICT_ERR); // OK
    memcpy(sBuf, "\x81\x0f""123456789012345", 17);
    iRet = iTlvAddObj(sTlvBuf2, sBuf, TLV_CONFLICT_ERR); // error, not enough space
    memcpy(sBuf, "\x9f\x01\x0d""12345678901234", 16);
    iRet = iTlvAddObj(sTlvBuf2, sBuf, TLV_CONFLICT_ERR); // OK

//short iTlvDelObj(uchar *psTlvBuffer, uchar *psTag);
    iRet = iTlvDelObj(sTlvBuf1, "\x89"); // error, not found    
    iRet = iTlvDelObj(sTlvBuf1, "\x88"); // OK
    iRet = iTlvDelObj(sTlvBuf2, "\x80"); // OK
    iRet = iTlvDelObj(sTlvBuf2, "\x9f\x01"); // OK

//short iTlvGetObj(uchar *psTlvBuffer,uchar *psTag,uchar **ppsTlvObject);
    iRet = iTlvSetBuffer(sTlvBuf1, sizeof(sTlvBuf1));
    iRet = iTlvSetBuffer(sTlvBuf2, sizeof(sTlvBuf2));
    memcpy(sBuf, "\x88\x01\x01", 3);
    iRet = iTlvAddObj(sTlvBuf1, sBuf, TLV_CONFLICT_ERR); // OK
    memcpy(sBuf, "\x80\x20""11111111111111112222222222222222", 34);
    iRet = iTlvAddObj(sTlvBuf2, sBuf, TLV_CONFLICT_ERR); // OK
    memcpy(sBuf, "\x9f\x01\x0d""12345678901234", 16);
    iRet = iTlvAddObj(sTlvBuf2, sBuf, TLV_CONFLICT_ERR); // OK

    iRet = iTlvGetObj(sTlvBuf1, "\x89", &psTlvObject); // not found
    iRet = iTlvGetObj(sTlvBuf1, "\x88", &psTlvObject); // OK
    iRet = iTlvGetObj(sTlvBuf2, "\x9f\x02", &psTlvObject); // not found
    iRet = iTlvGetObj(sTlvBuf2, "\x9f\x01", &psTlvObject); // OK
    iRet = iTlvGetObj(sTlvBuf2, "\x80", &psTlvObject); // OK

//short iTlvGetObjByIndex(uchar *psTlvBuffer, ushort uiIndex, uchar **ppsTlvObject);
    iRet = iTlvSetBuffer(sTlvBuf1, sizeof(sTlvBuf1));
    memcpy(sBuf, "\x88\x01\x01", 3);
    iRet = iTlvAddObj(sTlvBuf1, sBuf, TLV_CONFLICT_ERR); // OK
    memcpy(sBuf, "\x80\x20""11111111111111112222222222222222", 34);
    iRet = iTlvAddObj(sTlvBuf1, sBuf, TLV_CONFLICT_ERR); // OK
    memcpy(sBuf, "\x9f\x01\x0d""12345678901234", 16);
    iRet = iTlvAddObj(sTlvBuf1, sBuf, TLV_CONFLICT_ERR); // OK

    for(iCounter=0; ; iCounter++) {
        iRet = iTlvGetObjByIndex(sTlvBuf1, iCounter, &psTlvObject);
        if(iRet < 0)
            break;
    }

//short iTlvGetObjValue(uchar *psTlvBuffer, uchar *psTag, uchar **ppsValue);
    iRet = iTlvSetBuffer(sTlvBuf1, sizeof(sTlvBuf1));
    iRet = iTlvSetBuffer(sTlvBuf2, sizeof(sTlvBuf2));
    memcpy(sBuf, "\x88\x01\x01", 3);
    iRet = iTlvAddObj(sTlvBuf1, sBuf, TLV_CONFLICT_ERR); // OK
    memcpy(sBuf, "\x80\x20""11111111111111112222222222222222", 34);
    iRet = iTlvAddObj(sTlvBuf2, sBuf, TLV_CONFLICT_ERR); // OK
    memcpy(sBuf, "\x9f\x01\x0d""12345678901234", 16);
    iRet = iTlvAddObj(sTlvBuf2, sBuf, TLV_CONFLICT_ERR); // OK

    iRet = iTlvGetObjValue(sTlvBuf1, "\x89", &psTlvValue); // not found
    iRet = iTlvGetObjValue(sTlvBuf1, "\x88", &psTlvValue); // OK
    iRet = iTlvGetObjValue(sTlvBuf2, "\x9f\x02", &psTlvValue); // not found
    iRet = iTlvGetObjValue(sTlvBuf2, "\x9f\x01", &psTlvValue); // OK
    iRet = iTlvGetObjValue(sTlvBuf2, "\x80", &psTlvValue); // OK

//short iTlvSearchObj(uchar *psTemplate, short iTemplateLen, short iIndex,
//                    uchar *psTagRoute, uchar **ppsTlvObj);
    // example of a EMV card, the directory file record 1
    // 7014                                             // the EMV proprietory template
    //     6112                                         // application template
    //         4F07                                     // ADF name
    //             A0000000031010                       //
    //         5004                                     // application label
    //             56495341                             // "VISA"
    //         8701                                     // application priority indicator
    //             01                                   // 1->highest
    strcpy(sBuf2, "701461124F07A0000000031010500456495341870101");
    iLen = strlen(sBuf2);
    vTwoOne(sBuf2, iLen, sBuf);
    iLen /= 2;
    iRet = iTlvSearchObj(sBuf, iLen, 0, "\x70", &psTlvObject); // OK
    iRet = iTlvSearchObj(sBuf, iLen, 0, "\x70\x61", &psTlvObject); // OK
    iRet = iTlvSearchObj(sBuf, iLen, 0, "\x70\x61\x4f", &psTlvObject); // OK
    iRet = iTlvSearchObj(sBuf, iLen, 0, "\x70\x61\x50", &psTlvObject); // OK
    iRet = iTlvSearchObj(sBuf, iLen, 0, "\x70\x61\x87", &psTlvObject); // OK
    iRet = iTlvSearchObj(sBuf, iLen, 1, "\x70\x61\x87", &psTlvObject); // not found
    iRet = iTlvSearchObj(sBuf, iLen, 0, "\x71", &psTlvObject); // not found
    iRet = iTlvSearchObj(sBuf, iLen, 0, "\x70\x62", &psTlvObject); // not found
    iRet = iTlvSearchObj(sBuf, iLen, 0, "\x70\x61\x51", &psTlvObject); // not found
    iRet = iTlvSearchObj(sBuf, iLen, 0, "\x70\x62\x51", &psTlvObject); // not found

//short iTlvSearchObjValue(uchar *psTemplate, short iTemplateLen, short iIndex,
//                    uchar *psTagRoute, uchar **ppsTlvObjValue);
    // example of a EMV card, the directory file record 1
    // 7014                                             // the EMV proprietory template
    //     6112                                         // application template
    //         4F07                                     // ADF name
    //             A0000000031010                       //
    //         5004                                     // application label
    //             56495341                             // "VISA"
    //         8701                                     // application priority indicator
    //             01                                   // 1->highest
    strcpy(sBuf2, "701461124F07A0000000031010500456495341870101");
    iLen = strlen(sBuf2);
    vTwoOne(sBuf2, iLen, sBuf);
    iLen /= 2;
    iRet = iTlvSearchObjValue(sBuf, iLen, 0, "\x70", &psTlvValue); // OK
    iRet = iTlvSearchObjValue(sBuf, iLen, 0, "\x70\x61", &psTlvValue); // OK
    iRet = iTlvSearchObjValue(sBuf, iLen, 0, "\x70\x61\x4f", &psTlvValue); // OK
    iRet = iTlvSearchObjValue(sBuf, iLen, 0, "\x70\x61\x50", &psTlvValue); // OK
    iRet = iTlvSearchObjValue(sBuf, iLen, 0, "\x70\x61\x87", &psTlvValue); // OK
    iRet = iTlvSearchObjValue(sBuf, iLen, 1, "\x70\x61\x87", &psTlvValue); // not found
    iRet = iTlvSearchObjValue(sBuf, iLen, 0, "\x71", &psTlvValue); // not found
    iRet = iTlvSearchObjValue(sBuf, iLen, 0, "\x70\x62", &psTlvValue); // not found
    iRet = iTlvSearchObjValue(sBuf, iLen, 0, "\x70\x61\x51", &psTlvValue); // not found
    iRet = iTlvSearchObjValue(sBuf, iLen, 0, "\x70\x62\x51", &psTlvValue); // not found

    // example of a EMV card, the FCI of PSE
    // 6F20                                             // FCI template
    //     840E                                         // DDF name
    //         315041592E5359532E4444463031             // "1PAY.SYS.DDF01"
    //     A50E                                         // FCI proprietary template
    //         8801                                     // SFI of the directory elementory file
    //             01                                   //
    //         5F2D08                                   // language preference
    //             6573656E66726465                     // "esenfrde"
    strcpy(sBuf2, "6F20840E315041592E5359532E4444463031A50E8801015F2D086573656E66726465");
    iLen = strlen(sBuf2);
    vTwoOne(sBuf2, iLen, sBuf);
    iLen /= 2;
    iRet = iTlvSearchObjValue(sBuf, iLen, 0, "\x6f", &psTlvValue); // OK
    iRet = iTlvSearchObjValue(sBuf, iLen, 0, "\x6f\x84", &psTlvValue); // OK
    iRet = iTlvSearchObjValue(sBuf, iLen, 0, "\x6f\xa5", &psTlvValue); // OK
    iRet = iTlvSearchObjValue(sBuf, iLen, 0, "\x6f\xa5\x88", &psTlvValue); // OK
    iRet = iTlvSearchObjValue(sBuf, iLen, 0, "\x6f\xa5\x5f\x2d", &psTlvValue); // OK

    strcat(sBuf2, "6F026162");
    iLen = strlen(sBuf2);
    vTwoOne(sBuf2, iLen, sBuf);
    iLen /= 2;
    iRet = iTlvSearchObjValue(sBuf, iLen, 0, "\x6f", &psTlvValue); // OK
    iRet = iTlvSearchObjValue(sBuf, iLen, 1, "\x6f", &psTlvValue); // OK
    iRet = iTlvSearchObjValue(sBuf, iLen, 2, "\x6f", &psTlvValue); // OK

    strcpy(sBuf2, "6F0E00FF80026162FF0080024142FF00");
    iLen = strlen(sBuf2);
    vTwoOne(sBuf2, iLen, sBuf);
    iLen /= 2;
    iRet = iTlvSearchObjValue(sBuf, iLen, 0, "\x6f\x80", &psTlvValue); // OK
    iRet = iTlvSearchObjValue(sBuf, iLen, 1, "\x6f\x80", &psTlvValue); // OK
    iRet = iTlvSearchObjValue(sBuf, iLen, 2, "\x6f\x80", &psTlvValue); // OK

// ���ܣ����TLV�����Ƿ�Ϸ�
// ���룺psTlvObj           : TLV����ָ��
// ���أ�>0					: �ҵ���TLV����ĳ���
//	     TLV_ERR_OBJ_FORMAT	: TLV�����ʽ����
//       TLV_ERR_OBJ_LENGTH : TLV���󳤶Ȳ�����EMV�淶
//short iTlvCheckTlvObject(uchar *psTlvObj)
    iRet = iTlvCheckTlvObject("\xff"); // illegal
    iRet = iTlvCheckTlvObject("\x88\x01\x01"); // ok
    iRet = iTlvCheckTlvObject("\x88\x02\x01\x01"); // illegal
    iRet = iTlvCheckTlvObject("\x9f\x7f\x02\x31\x32"); // ok, unknown tag, assume legal

//short iTlvBatchAddObj(uchar ucFlag, uchar *psTlvBuffer, 
//                      uchar *psTlvTemplate, short iTlvTemplateLen, int iLen0Flag)
    // example of a EMV card, one record of a EF
    // 7036
    //     5F2403                                   // application expiration date
    //         101231                               // Dec 31st, 2010
    //     5A08                                     // PAN
    //         4761739001010010                     //
    //     5F3401                                   // PAN sequence number
    //         01                                   //
    //     9F0702                                   // application usage control
    //         FF00                                 //
    //     9F0D05                                   // issuer action code - default
    //         2040048800
    //     9F0E05                                   // issuer action code - denial
    //         1010B80000
    //     9F0F05                                   // issuer action code - online
    //         2040049800
    //     5F2802                                   // issuer country code
    //         0840                                 // USA
    strcpy(sBuf2, "70365F24031012315A0847617390010100105F3401019F07"
                  "02FF009F0D0520400488009F0E051010B800009F0F052040"
                  "0498005F28020840");
    iLen = strlen(sBuf2);
    vTwoOne(sBuf2, iLen, sBuf);
    iLen /= 2;

    iRet = iTlvSetBuffer(sTlvBuf1, sizeof(sTlvBuf1));
    iRet = iTlvBatchAddObj(0, sTlvBuf1, sBuf, iLen, TLV_CONFLICT_ERR, 0/*0:����Ϊ0�������*/);

    iRet = iTlvGetObj(sTlvBuf1, "\x5f\x24", &psTlvObject); // ok
    iRet = iTlvGetObj(sTlvBuf1, "\x5a", &psTlvObject); // ok
    iRet = iTlvGetObj(sTlvBuf1, "\x5f\x34", &psTlvObject); // ok
    iRet = iTlvGetObj(sTlvBuf1, "\x9f\x07", &psTlvObject); // ok
    iRet = iTlvGetObj(sTlvBuf1, "\x9f\x0d", &psTlvObject); // ok
    iRet = iTlvGetObj(sTlvBuf1, "\x9f\x0e", &psTlvObject); // ok
    iRet = iTlvGetObj(sTlvBuf1, "\x9f\x0f", &psTlvObject); // ok
    iRet = iTlvGetObj(sTlvBuf1, "\x5f\x28", &psTlvObject); // ok
    iRet = iTlvGetObj(sTlvBuf1, "\x5f\x29", &psTlvObject); // not found
    iRet = iTlvGetObjValue(sTlvBuf1, "\x5f\x24", &psTlvValue); // ok
    iRet = iTlvGetObjValue(sTlvBuf1, "\x5a\x08", &psTlvValue); // ok
    iRet = iTlvGetObjValue(sTlvBuf1, "\x5f\x34", &psTlvValue); // ok
    iRet = iTlvGetObjValue(sTlvBuf1, "\x9f\x07", &psTlvValue); // ok
    iRet = iTlvGetObjValue(sTlvBuf1, "\x9f\x0d", &psTlvValue); // ok
    iRet = iTlvGetObjValue(sTlvBuf1, "\x9f\x0e", &psTlvValue); // ok
    iRet = iTlvGetObjValue(sTlvBuf1, "\x9f\x0f", &psTlvValue); // ok
    iRet = iTlvGetObjValue(sTlvBuf1, "\x5f\x28", &psTlvValue); // ok
    iRet = iTlvGetObjValue(sTlvBuf1, "\x5f\x29", &psTlvValue); // not found

//short iTlvCopyBuffer(uchar *psTlvTargetBuffer, uchar *psTlvSourceBuffer)
    iRet = iTlvSetBuffer(sTlvBuf1, sizeof(sTlvBuf1));
    iRet = iTlvSetBuffer(sTlvBuf2, sizeof(sTlvBuf2));
    memcpy(sBuf, "\x4f""\x04""\x01\x02\x03\x04", 6);
    iRet = iTlvAddObj(sTlvBuf1, sBuf, TLV_CONFLICT_ERR);
    memcpy(sBuf, "\x50""\x04""abcd", 6);
    iRet = iTlvAddObj(sTlvBuf2, sBuf, TLV_CONFLICT_ERR);
    memcpy(sBuf, "\x5a""\x04""\x56\x78\x90\xff", 6);
    iRet = iTlvAddObj(sTlvBuf2, sBuf, TLV_CONFLICT_ERR);
    iRet = iTlvCopyBuffer(sTlvBuf1, sTlvBuf2, TLV_CONFLICT_ERR);
    
//short iTlvMakeObject(uchar *psTag, ushort uiLength, uchar *psValue, uchar *psTlvObject, int iTlvObjectBufSize)
    memset(sBuf, 0, sizeof(sBuf));
    iRet = iTlvMakeObject("\x50", 10, "abcdefghij", sBuf, sizeof(sBuf));
    memset(sBuf, 0, sizeof(sBuf));
    strcpy(sBuf2, "12345678901234567890123456789012345678901234567890123456789012345"
                  "67890123456789012345678901234567890123456789012345678901234567890");
    iRet = iTlvMakeObject("\x9f\x11", 130, "abcdefghij", sBuf, sizeof(sBuf));

//short iTlvMakeDOLBlock(uchar *psBlock, int iBlockSize, uchar *psDOL, short, iDOLLen,
//          		     uchar *psTlvBuffer1, uchar *psTlvBuffer2, uchar *psTlvBuffer3, uchar *psTlvBuffer4);
    iRet = iTlvSetBuffer(sTlvBuf1, sizeof(sTlvBuf1));
    iRet = iTlvSetBuffer(sTlvBuf2, sizeof(sTlvBuf2));
    iRet = iTlvSetBuffer(sTlvBuf3, sizeof(sTlvBuf3));
    memcpy(sBuf, "\x4f""\x04""\x01\x02\x03\x04", 6);
    iRet = iTlvAddObj(sTlvBuf1, sBuf, TLV_CONFLICT_ERR);
    memcpy(sBuf, "\x50""\x04""abcd", 6);
    iRet = iTlvAddObj(sTlvBuf1, sBuf, TLV_CONFLICT_ERR);
    memcpy(sBuf, "\x5a""\x04""\x56\x78\x90\xff", 6);
    iRet = iTlvAddObj(sTlvBuf1, sBuf, TLV_CONFLICT_ERR);
    memcpy(sBuf, "\x5f\x24""\x04""\x00\x11\x22\x33", 7);
    iRet = iTlvAddObj(sTlvBuf1, sBuf, TLV_CONFLICT_ERR);
    // B   4f   04 01020304
    // ANS 50   04 61626364
    // CN  5a   04 567890ff
    // N   5f24 04 00112233

    // normal
    strcpy(sBuf2, "4F0450045A045F2404");
    iDOLLen = strlen(sBuf2);
    vTwoOne(sBuf2, iDOLLen, sDOL);
    iDOLLen /= 2;
    memset(sBuf, 0, sizeof(sBuf));
    iRet = iTlvMakeDOLBlock(sBuf, sizeof(sBuf), sDOL, iDOLLen, sTlvBuf1, 0, 0, 0);

    // indicated length is longer
    strcpy(sBuf2, "4F0550055A055F2405");
    iDOLLen = strlen(sBuf2);
    vTwoOne(sBuf2, iDOLLen, sDOL);
    iDOLLen /= 2;
    memset(sBuf, 0, sizeof(sBuf));
    iRet = iTlvMakeDOLBlock(sBuf, sizeof(sBuf), sDOL, iDOLLen, sTlvBuf1, 0, 0, 0);

    // indicated length is shorter
    strcpy(sBuf2, "4F0350035A035F2403");
    iDOLLen = strlen(sBuf2);
    vTwoOne(sBuf2, iDOLLen, sDOL);
    iDOLLen /= 2;
    memset(sBuf, 0, sizeof(sBuf));
    iRet = iTlvMakeDOLBlock(sBuf, sizeof(sBuf), sDOL, iDOLLen, sTlvBuf1, 0, 0, 0);

    // shorter & not found & longer & not unrecognized
    strcpy(sBuf2, "4F0390035A055F9903");
    iDOLLen = strlen(sBuf2);
    vTwoOne(sBuf2, iDOLLen, sDOL);
    iDOLLen /= 2;
    memset(sBuf, 0, sizeof(sBuf));
    iRet = iTlvMakeDOLBlock(sBuf, sizeof(sBuf), sDOL, iDOLLen, sTlvBuf1, 0, 0, 0);
}
# endif