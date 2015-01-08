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
                            遵照EMV4.1 book3 5.2, 如果TLV对象值域长度为0，认为不存在
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
					将输入输出参数中short型改为int型，ushort型改为uint型
				    添加或批量添加/拷贝TLV对象函数，增加了参数iConflictFlag, 用于控制冲突后行为
					iTlvCheckTlvObject()强化了TLV对象检查
					增加iTlvDbInfo()函数
				Apr 7th, 2012
				    增加iTlvMakeAddObj()函数，生成Tlv对象然后直接加到Tlv数据库中去
				Apr 20th, 2012
					修正iTlvBatchAddObj()函数, 只允许添加标明来自卡片的Tlv对象
				May 3rd, 2012
					修正iTlvMakeDOLBlock()函数, 增加iBlockSize用于防止存储越界
				May 4th, 2012
					修正iTlvMakeObject()函数, 增加iTlvObjectBufSize用于防止存储越界
                May 7th, 2012
					增加iTlvBatchAddField55Obj()函数, 用于添加来自8583 55域的Tlv对象到Tlv数据库中
				Jul 27th, 2012
					修正iTlvSearchObjValue()与iTlvSearchObj()函数, 搜索路径可以包含'\xFF'通配符
				Jul 27th, 2012
					增加iTlvCheckOrder()函数, 用于核查Tlv对象顺序合法性
				Aug 10th, 2012
					修正iTlvBatchAddObj()函数, 可以添加非Tag70与Tag77模板数据
				Aug 10th, 2012
					增加iTlvCheckDup()函数, 检查模板重复项
				Aug 14th, 2012
					修正iTlvBatchAddObj()函数, 增加一参数, 可以指示是否将长度为0的对象也加入到数据库中
				Aug 31th, 2012
					修正iTlvSearchDOLTag()函数, 增加一返回参数, 返回DOL数据块偏移量
Modified      : Mar 8th, 2013
					修正Tag长度解释, 对于DFxx私有Class的Tag, 第二字节最高位不解释成Tag扩展
					大于DF80的Tag用于华信Emv核心自有数据存储(此修正基于Emv规范不允许超过2字节Tag, 因此借用第二字节最高位)
					此修正仅限于DFxx, 其它class的Tag处理方式同前
                Mar 21st, 2013
				    71脚本从打包到DF71改为打包到DFF1, 72脚本从打包到DF72改为打包到DFF2
Modified      : Apr 2nd, 2013
                    增加iTlvObjLen()函数, 返回Tlv对象大小
Modified      : Mar 18th, 2014
					增加iTlvBatchAddField55ObjHost()函数, 用于后台添加来自8583 55域的Tlv对象到Tlv数据库中
**************************************/
/*
模块详细描述:
	实现所有与TLV有关的操作
.	TLV数据库功能
	TLV数据库以一段内存来保存TLV对象, iTlvSetBuffer()函数用来初始化TLV数据库
	TLV数据库内存在头部保存了该TLV数据库中TLV对象的个数、剩余空间
	模块提供了一组TLV操纵函数来实现TLV数据库的增加、删除、提取、拷贝、遍历操作
.	模板操作
	提供了一组函数实现了模板类TLV对象中TLV数据的提取、遍历操作
.	根据DOL构造DOL数据块
	根据PDOL、DDOL、CDOL1、CDOL2、TDOL来构造数据块
.	搜索DOL中是否有特定TAG
	比如搜索PDOL中是否有金额域
*/
# include <stdio.h>
# include <string.h>
# include <ctype.h>
# include "VposFace.h"
# include "TagAttr.h"
# include "TlvFunc.h"

# define TLV_BUFFER_IDENTIFIER                  0x5501

// TLV存储结构
//    TLV Buffer				N bytes		    // 总存储区，包括10字节头部
//    	TLV Buffer Identifier	  2 bytes		// fixed "\x55\x01"
//      Number of TLV object      2 bytes       // 当前存储区中已保存的TLV对象个数
//    	Total space			      2 bytes		// 剩余存储区+已用存储区
//    	Space left				  2 bytes		// 剩余的存储区大小
//   	Space used			      2 bytes		// 已用的存储区大小
//   	Buffer				      N-10 bytes	// TLV存储区
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
static struct stTlvBufferHead gl_TlvBufferHead; // TLV数据库头结构

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
			return(0); // 20130308, 只有非DFxx, 才检查Tag第2字节的最高位, 如果非法, 认为出错
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
			return(-2); // 20130308, 只有非DFxx, 才检查Tag第2字节的最高位, 如果非法, 认为出错
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

// 功能：初始化TLV存储区，一个TLV存储区必须先初始化才能使用
// 输入：psTlvBuffer		: TLV存储区
//       uiTlvBufferSize	: 字节单位的存储区长度，至少10字节
// 返回：>=0				: 完成, 可用的存储空间
//       TLV_ERR_BUF_SPACE  : 存储空间不足，存储区不足10字节
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

// 功能：查询TLV数据库信息
// 输入：psTlvBuffer		: TLV存储区
// 输出：puiSpaceUsed       : 已用空间(byte)
//       puiSpaceLeft       : 剩余空间(byte)
// 返回：>=0				: Tlv对象个数
//       TLV_ERR_BUF_FORMAT	: TLV存储区格式错误
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

// 功能：添加TLV对象到TLV存储区
// 输入：psTlvBuffer		: TLV存储区
//       psTlvObject		: TLV对象，可以是结构TLV或基本TLV
//       iConflictFlag		: 冲突处理标志, TLV_CONFLICT_ERR|TLV_CONFLICT_IGNORE|TLV_CONFLICT_REPLACE
// 返回：>0					: 添加成功或已存在完全相同的TLV对象, 对象大小
// 	     TLV_ERR_BUF_SPACE	: TLV存储区无足够的存储空间
//  	 TLV_ERR_BUF_FORMAT	: TLV存储区格式错误
//       TLV_ERR_CONFLICT	: 冲突，TLV对象已存在
//       TLV_ERR_OBJ_FORMAT	: TLV对象结构非法
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
		// TLV对象已经存在
		if(iConflictFlag == TLV_CONFLICT_IGNORE)
			return(iTlvObjLength); // 忽略冲突，返回原TLV对象大小
		if(iConflictFlag == TLV_CONFLICT_ERR)
			return(TLV_ERR_CONFLICT); // 出错
		iTlvDelObj(psTlvBuffer, psTlvObject); // 删除原TLV对象
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

// 功能：构造TLV对象然后直接添加到TLV存储区
// 输入：psTlvBuffer		: TLV存储区
//       psTag              : Tag
//       uiLength           : Length
//       psValue            : Value
//       iConflictFlag		: 冲突处理标志, TLV_CONFLICT_ERR|TLV_CONFLICT_IGNORE|TLV_CONFLICT_REPLACE
// 返回：>0					: 添加成功或已存在完全相同的TLV对象, 对象大小
// 	     TLV_ERR_BUF_SPACE	: TLV存储区无足够的存储空间
//  	 TLV_ERR_BUF_FORMAT	: TLV存储区格式错误
//       TLV_ERR_CONFLICT	: 冲突，TLV对象已存在
//       TLV_ERR_OTHER      : 其它错误,Tag或Length非法
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

// 功能：从TLV存储区中删除TLV对象，可以省出空间存放其它的TLV对象
// 输入：psTlvBuffer		: TLV存储区
//       psTag				: 要删除的TLV对象的Tag
// 返回：>0					: 完成, 所删除的TLV对象大小
// 	     TLV_ERR_BUF_FORMAT	: TLV存储区格式错误
//       TLV_ERR_NOT_FOUND	: 没找到对应的TLV对象
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

// 功能：从TLV存储区中取出相应Tag的TLV对象的指针
// 输入：psTlvBuffer		: TLV存储区
// 	     psTag				: 要读取的TLV对象的Tag
// 输出：ppsTlvObject		: TLV对象指针
// 返回：>0					: 完成，TLV对象的长度
//  	 TLV_ERR_BUF_FORMAT	: TLV存储区格式错误
//  	 TLV_ERR_NOT_FOUND	: 没找到对应的TLV对象
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

// 功能：从TLV存储区中取出相应位置的TLV对象的指针，用于遍历TLV存储区
// 输入：psTlvBuffer		: TLV存储区
// 	     uiIndex			: 索引号，0开始
// 输出：ppsTlvObject		: TLV对象指针
// 返回：>0					: 完成，TLV对象的长度
//	     TLV_ERR_BUF_FORMAT	: TLV存储区格式错误
//	     TLV_ERR_NOT_FOUND	: 没找到对应的TLV对象
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

// 功能：从TLV存储区中取出相应Tag的TLV对象值的指针
// 输入：psTlvBuffer		: TLV存储区
// 	     psTag				: 要读取的TLV对象的Tag
// 输出：ppsValue			: TLV对象值的指针
// 返回：>=0				: TLV对象值长度
//	     TLV_ERR_BUF_FORMAT	: TLV存储区格式错误
//	     TLV_ERR_NOT_FOUND	: 没找到对应的TLV对象
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

// 功能：读取TLV对象值
// 输入：psTlvObj			: TLV对象
// 输出：ppsTlvObjValue	    : TLV对象值
// 返回：>=0				: TLV对象值大小
//	     TLV_ERR_OBJ_FORMAT	: TLV对象格式错误
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

// 功能：读取TLV对象值长度
// 输入：psTlvObj			: TLV对象
// 返回：>=0				: TLV对象值大小
//	     TLV_ERR_OBJ_FORMAT	: TLV对象格式错误
int iTlvValueLen(uchar *psTlvObj)
{
    uint  uiRet;

	uiRet = _uiTlvGetValueLen(psTlvObj);
	if(uiRet == 0xFFFF)
		return(TLV_ERR_OBJ_FORMAT);
	return(uiRet);
}

// 功能：读取TLV对象Tag长度
// 输入：psTlvObj			: TLV对象
// 返回：1 or 2				: TLV对象Tag大小
//	     TLV_ERR_OBJ_FORMAT	: TLV对象格式错误
int iTlvTagLen(uchar *psTlvObj)
{
	uint uiRet;
	uiRet = _uiTlvGetTagLen(psTlvObj);
	if(uiRet == 0)
		return(TLV_ERR_OBJ_FORMAT);
	return(uiRet);
}

// 功能：读取TLV对象Length长度
// 输入：psTlvObj			: TLV对象
// 返回：1 or 2				: TLV对象Len大小
//	     TLV_ERR_OBJ_FORMAT	: TLV对象格式错误
int iTlvLengthLen(uchar *psTlvObj)
{
	uint uiRet;
	uiRet = _uiTlvGetLengthLen(psTlvObj);
	if(uiRet == 0)
		return(TLV_ERR_OBJ_FORMAT);
	return(uiRet);
}

// 功能：读取TLV对象长度
// 输入：psTlvObj			: TLV对象
// 返回：>0                 : TLV对象大小
//	     TLV_ERR_OBJ_FORMAT	: TLV对象格式错误
// 注意: 00 or FF被认为是格式错误
int iTlvObjLen(uchar *psTlvObj)
{
    int    iRet;
    iRet = _iTlvCheckTlvObj(psTlvObj);
    if(iRet < 0)
        return(TLV_ERR_OBJ_FORMAT);
	return(iRet);
}

// 功能：读取TLV对象值
// 输入：psTlvObj			: TLV对象
// 返回：!NULL			    : TLV对象值
//	     NULL				: TLV对象格式错误
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

// 功能：从TLV模板中以指定的路径取出TLV对象
// 输入：psTemplate			: TLV模板，也可能是一个或几个基本TLV
//       iTemplateLen       : 模板大小
// 	     iIndex			    : 如果终极Tag有多个匹配，指明第几个，0表示第一个
// 	     psTagRoute			: 要读取的TLV对象的Tag, 包括路径, '\x00'Tag表示结束, '\xFF'Tag表示可匹配任何Tag
// 输出：ppsTlvObj		    : TLV对象
// 返回：>0					: 找到，TLV对象大小
//	     TLV_ERR_BUF_FORMAT	: TLV模板格式错误
//	     TLV_ERR_NOT_FOUND	: 没找到对应的TLV对象
// 举例：要读取缓冲区长度为86的psT中的模板6F中的模板A5中的标记88的TLV对象,做如下调用:
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
            uiTagLen = 1; // '\xFF'设定为单字节通配Tag
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
                // 遵照EMV4.1 book3 5.2, 如果TLV对象值域长度为0，认为不存在
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

// 功能：从TLV模板中以指定的路径取出相应Tag的TLV对象值
// 输入：psTemplate			: TLV模板，也可能是一个或几个基本TLV
//       iTemplateLen       : 模板大小
// 	     iIndex			    : 如果终极Tag有多个匹配，指明第几个，0表示第一个
// 	     psTagRoute			: 要读取的TLV对象的Tag, 包括路径, '\x00'Tag表示结束, '\xFF'Tag表示可匹配任何Tag
// 输出：psTlvObjValue		: TLV对象值指针
// 返回：>0					: 找到，TLV对象值的长度
//	     TLV_ERR_BUF_FORMAT	: TLV模板格式错误
//	     TLV_ERR_NOT_FOUND	: 没找到对应的TLV对象
// 举例：要读取缓冲区长度为86的psT中的模板6F中的模板A5中的标记88的值,做如下调用:
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

// 功能：从TLV模板中取出指定位置的TLV对象(可能是基本TLV对象，也可能是另外一个模板)，用于遍历模板
// 输入：psTemplate			: TLV模板
//       iTemplateLen       : 模板大小
// 	     iIndex			    : 指明第几个，0表示第一个
// 输出：ppsTlvObj		    : TLV对象
// 返回：>0					: 找到，TLV对象大小
//	     TLV_ERR_BUF_FORMAT	: TLV模板格式错误
//	     TLV_ERR_NOT_FOUND	: 不存在该索引的对象
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
		return(TLV_ERR_BUF_FORMAT); // 不是模板

	iDataLen = iTlvValue(psTemplate, &psData); // 读取模板的值
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
			// 找到
			*ppsTlvObj = psData;
			return(iRet);
		}

		iIndex --;
		psData += iRet;
		iDataLen -= iRet;
    } // while(iDataLen > 0    
    
    return(TLV_ERR_NOT_FOUND);
}

// 功能：检查TLV对象是否合法
// 输入：psTlvObj           : TLV对象指针
// 返回：>0					: 找到，TLV对象的长度
//	     TLV_ERR_OBJ_FORMAT	: TLV对象格式错误
//       TLV_ERR_OBJ_LENGTH : TLV对象长度不满足EMV规范
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
			// 当前数据项长度已达到最大长度 且 规范定义该数据项最大长度为奇数
			// 检查多余出的最左面4bit是否为0, 如果不为0，认为数据项超长
			iTlvValue(psTlvObj, &psTlvObjValue);
			if((psTlvObjValue[0]&0xF0) != 0x00)
				return(TLV_ERR_OBJ_LENGTH);
		}
		break;
	case TAG_ATTR_CN:
	    if(iValueLen<(iLeast+1)/2 || iValueLen>(iMost+1)/2)
		    return(TLV_ERR_OBJ_LENGTH);
		if(iValueLen== (iMost+1)/2 && iMost%2) {
			// 当前数据项长度已达到最大长度 且 规范定义该数据项最大长度为奇数
			// 检查多余出的最右面4bit是否为F, 如果不为F，认为数据项超长
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

// 功能：将一个模板内的所有TLV对象添加到TLV存储区(只添加第一层TLV对象，不会递归解析模板)
// 输入：ucFlag             : 0:只添加基本TLV对象，!0:添加所有TLV对象
//       psTlvBuffer		: TLV存储区
//       psTlvTemplate		: TLV模板，只能是"\x70"或"\x77"
//       iTlvTemplateLen    : TLV模板大小
//       iConflictFlag		: 冲突处理标志, TLV_CONFLICT_ERR|TLV_CONFLICT_IGNORE|TLV_CONFLICT_REPLACE
//       iLen0Flag          : 长度为0的对象添加标志, 0:长度为0的对象不添加 1:长度为0的对象也添加
// 返回：>=0				: 完成, 返回值为添加的TLV对象个数
// 	     TLV_ERR_BUF_SPACE	: TLV存储区无足够的存储空间
//	     TLV_ERR_BUF_FORMAT	: TLV存储区格式错误
//	     TLV_ERR_CONFLICT	: 冲突，终止处理
//	     TLV_ERR_TEMP_FORMAT: TLV模板结构非法
//       TLV_ERR_OBJ_FORMAT	: TLV对象结构非法
// Note: 只添加来自IC卡片的Tlv对象
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
				if(uiTagAttrGetFrom(psData) == TAG_FROM_CARD) { // by yujun 20120420, 只有来自IC卡片的Tlv对象才允许添加
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

// 功能：将8583 55域内的所有TLV对象添加到TLV存储区(只添加第一层TLV对象，不会递归解析)
// 输入：ucFlag             : 0:只添加基本TLV对象，!0:添加所有TLV对象
//       psTlvBuffer		: TLV存储区
//       psField55			: 55域
//       iField55Len	    : 55域长度
//       iConflictFlag		: 冲突处理标志, TLV_CONFLICT_ERR|TLV_CONFLICT_IGNORE|TLV_CONFLICT_REPLACE
//       iHostFlag          : 1:后台用 0:不是后台用
// 返回：>=0				: 完成, 返回值为添加的TLV对象个数
// 	     TLV_ERR_BUF_SPACE	: TLV存储区无足够的存储空间
//	     TLV_ERR_BUF_FORMAT	: TLV存储区格式错误
//	     TLV_ERR_CONFLICT	: 冲突，终止处理
//       TLV_ERR_OBJ_FORMAT	: 55域中Tlv对象格式非法
// Note: 提供给iTlvBatchAddField55Obj()与iTlvBatchAddField55ObjHost()调用
//       由于71与72脚本可能会有多个, 71与72脚本会被打包到DFF1与DFF2包中保存
static int iTlvBatchAddField55ObjBase(uchar ucFlag, uchar *psTlvBuffer, uchar *psField55, int iField55Len, int iConflictFlag, int iHostFlag)
{
    int    iDataLen;                  // data left needed to be processed
    uchar  *psData;                   // data pointer being processed
    int    iRet;
    uint   uiTlvObjLen;               // 
	uchar  sPackage[256], *psPackage; // 脚本包
	int    iPackageLen;			      // 脚本包长度
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
				return(TLV_ERR_OBJ_FORMAT); // Tlv对象值长度错并且长度不等于0, 认为Tlv对象格式错误
			// Tlv对象值等于0, 忽略该对象
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
            if(_uiTlvGetValueLen(psData) != 0) { // inserted by yujun 20080820, 遵照EMV4.1 book3 5.2, 如果TLV对象值域长度为0，认为不存在
				// modified by yujun, 20140318, 跟具iHostFlag决定是后台调用还是终端调用
				if((uiTagAttrGetFrom(psData)==TAG_FROM_ISSUER && iHostFlag==0) || (uiTagAttrGetFrom(psData)!=TAG_FROM_ISSUER && iHostFlag!=0)) {
					iCount ++;
					if(psData[0]==0x71 || psData[0]==0x72) {
						// 脚本标识, 需要存储到DFF1或DFF2包中
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
							return(TLV_ERR_OTHER); // sBuf缓冲区不足
						memcpy(sPackage+iPackageLen, psData, uiTlvObjLen);
						iPackageLen += uiTlvObjLen;
						if(psData[0] == 0x71)
							iRet = iTlvMakeAddObj(psTlvBuffer, "\xDF\xF1", iPackageLen, sPackage, TLV_CONFLICT_REPLACE);
						else
							iRet = iTlvMakeAddObj(psTlvBuffer, "\xDF\xF2", iPackageLen, sPackage, TLV_CONFLICT_REPLACE);
						if(iRet < 0)
							return(iRet);
					} else {
						// 其它Tag, 直接保存
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

// 非Host用
// 功能：将8583 55域内的所有TLV对象添加到TLV存储区(只添加第一层TLV对象，不会递归解析)
// 输入：ucFlag             : 0:只添加基本TLV对象，!0:添加所有TLV对象
//       psTlvBuffer		: TLV存储区
//       psField55			: 55域
//       iField55Len	    : 55域长度
//       iConflictFlag		: 冲突处理标志, TLV_CONFLICT_ERR|TLV_CONFLICT_IGNORE|TLV_CONFLICT_REPLACE
// 返回：>=0				: 完成, 返回值为添加的TLV对象个数
// 	     TLV_ERR_BUF_SPACE	: TLV存储区无足够的存储空间
//	     TLV_ERR_BUF_FORMAT	: TLV存储区格式错误
//	     TLV_ERR_CONFLICT	: 冲突，终止处理
//       TLV_ERR_OBJ_FORMAT	: 55域中Tlv对象格式非法
// Note: 只添加来自发卡方的Tlv对象
//       由于71与72脚本可能会有多个, 71与72脚本会被打包到DFF1与DFF2包中保存
int iTlvBatchAddField55Obj(uchar ucFlag, uchar *psTlvBuffer, uchar *psField55, int iField55Len, int iConflictFlag)
{
	int iRet;
	iRet = iTlvBatchAddField55ObjBase(ucFlag, psTlvBuffer, psField55, iField55Len, iConflictFlag, 0);
	return(iRet);
}

// Host用
// 功能：将8583 55域内的所有TLV对象添加到TLV存储区(只添加第一层TLV对象，不会递归解析)
// 输入：ucFlag             : 0:只添加基本TLV对象，!0:添加所有TLV对象
//       psTlvBuffer		: TLV存储区
//       psField55			: 55域
//       iField55Len	    : 55域长度
//       iConflictFlag		: 冲突处理标志, TLV_CONFLICT_ERR|TLV_CONFLICT_IGNORE|TLV_CONFLICT_REPLACE
// 返回：>=0				: 完成, 返回值为添加的TLV对象个数
// 	     TLV_ERR_BUF_SPACE	: TLV存储区无足够的存储空间
//	     TLV_ERR_BUF_FORMAT	: TLV存储区格式错误
//	     TLV_ERR_CONFLICT	: 冲突，终止处理
//       TLV_ERR_OBJ_FORMAT	: 55域中Tlv对象格式非法
// Note: 添加除发卡方以外的Tlv对象
//       由于71与72脚本可能会有多个, 71与72脚本会被打包到DFF1与DFF2包中保存
int iTlvBatchAddField55ObjHost(uchar ucFlag, uchar *psTlvBuffer, uchar *psField55, int iField55Len, int iConflictFlag)
{
	int iRet;
	iRet = iTlvBatchAddField55ObjBase(ucFlag, psTlvBuffer, psField55, iField55Len, iConflictFlag, 1);
	return(iRet);
}

// 功能：将一个TLV存储区内的所有TLV对象添加到另一个TLV存储区中
// 输入：psTlvSourceBuffer  : 源TLV存储区
//       iConflictFlag		: 冲突处理标志, TLV_CONFLICT_ERR|TLV_CONFLICT_IGNORE|TLV_CONFLICT_REPLACE
// 输出：psTlvTargetBuffer	: 目标TLV存储区
// 返回：>=0				: 完成, 拷贝的TLV对象个数
// 	     TLV_ERR_BUF_SPACE	: 目标TLV存储区无足够的存储空间
//	     TLV_ERR_BUF_FORMAT	: 存在TLV存储区格式错误
//	     TLV_ERR_CONFLICT	: 冲突，终止处理
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

// 功能：构造一个TLV对象
// 输入：psTag              : Tag
//       uiLength           : Length
//       psValue            : Value
//       iTlvObjectBufSize  : psTlvObject空间大小
// 输出：psTlvObject        : 构造好的TLV对象
// 返回：>0					: 完成, 构造好的TLV对象大小
//       TLV_ERR_BUF_SPACE  : psTlvObject空间不足
//       TLV_ERR_OTHER      : 其它错误,Tag或Length非法
//                            只允许最多2字节Tag、最长255字节长度
int iTlvMakeObject(uchar *psTag, uint uiLength, uchar *psValue, uchar *psTlvObject, int iTlvObjectBufSize)
{
    int iObjLen;
    
    // check tag
    if(psTag[0] == 0x00 || psTag[0] == 0xff)
        return(TLV_ERR_OTHER); // '00' tag or 'ff' tag
    if((psTag[0] & 0x1f) == 0x1f && (psTag[1] & 0x80) == 0x80) {
		if(psTag[0] != 0xDF)
			return(TLV_ERR_OTHER); // 20130308, 只有非DFxx, 才检查Tag第2字节的最高位, 如果非法, 认为出错
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

// 功能：根据DOL构造数据块, 从TLV存储区中搜索所需目标，最多可提供4个TLV存储区
// 输入：iBlockSize			: psBlock空间字节数
//		 psDOL				: Data Object List
// 	     iDOLLen		    : DOL长度
// 	     psTlvBuffer1		: TLV存储区1，NULL表示没有第1个存储区
// 	     psTlvBuffer2		: TLV存储区2，NULL表示没有第2个存储区
// 	     psTlvBuffer3		: TLV存储区3，NULL表示没有第3个存储区
// 	     psTlvBuffer4		: TLV存储区4，NULL表示没有第4个存储区
//                            先从存储区1开始搜索，最后存储区4
// 输出：psBlock			: 按照DOL打包的数据块
// 返回：>=0				: 完成, 打包后的数据块长度
// 	     TLV_ERR_BUF_FORMAT	: TLV存储区格式错误
//       TLV_ERR_DOL_FORMAT : DOL格式错误
//       TLV_ERR_BUF_SPACE  : psBlock存储空间不足
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

// 功能：根据Tag列表构造数据记录, 从TLV存储区中搜索所需目标，最多可提供4个TLV存储区
// 输入：psTags				: Tag列表
// 	     iTagsLen		    : Tag列表长度
// 	     psTlvBuffer1		: TLV存储区1，NULL表示没有第1个存储区
// 	     psTlvBuffer2		: TLV存储区2，NULL表示没有第2个存储区
// 	     psTlvBuffer3		: TLV存储区3，NULL表示没有第3个存储区
// 	     psTlvBuffer4		: TLV存储区4，NULL表示没有第4个存储区
//                            先从存储区1开始搜索，最后存储区4
// 输出：psBlock			: 按照Tags列表内容, 用Tag70打包的记录块, 要留出至少254字节空间
// 返回：>=0				: 完成, 打包后的记录长度
// 	     TLV_ERR_BUF_FORMAT	: TLV存储区格式错误
//       TLV_ERR_OTHER      : 其它错误
// Note: 如果Tags列表中某Tag在任何TLV存储区都未找到, 或找到后Tlv对象的Value域长度为0, 则不将该对象添加到记录中
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
			return(TLV_ERR_OTHER); // 记录长度超出252

		memcpy(psRecData, psTlvObj, iTlvObjLen);
        iTagsLen -= uiTagLen;
        psTags += uiTagLen;
		psRecData += iTlvObjLen;
		iBlockLen += iTlvObjLen;
    } // while(iTagsLen > 0

	iBlockLen = iTlvMakeObject("\x70", iBlockLen, sRecData, psBlock, 254);
    return(iBlockLen);
}

// 功能：搜索DOL中是否包括某一Tag
// 输入：psDOL				: Data Object List
// 	     iDOLLen		    : DOL长度
//       psTag              : 要搜索的Tag
// 输出  piOffset           : 如果DOL中包含搜索的TAG, 返回该TAG在DOL数据块中的偏移量, NULL表示不需要返回
// 返回：>=0				: Tag存在, 返回该Tag需要的长度
//       TLV_ERR_DOL_FORMAT : DOL格式错误
// 注意: piOffset返回的是用该DOL做模板打包成的数据块中的偏移量
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

// 功能：核查Tlv对象顺序合法性
// 输入：psTemplate			: TLV模板内容
//       iTemplateLen       : 模板内容大小
//       psTagOrder         : 要求的TLV对象顺序, '\x00'Tag表示结束
// 返回：0					: 检查正确
//	     TLV_ERR_BUF_FORMAT	: TLV模板格式错误
//       TLV_ERR_CHECK      : 检查失败
//       TLV_OK             : 检查成功
// 注意: psTagOrder标明的Tag必须以正确顺序出现在psTemplate中
int iTlvCheckOrder(uchar *psTemplate, int iTemplateLen, uchar *psOrder)
{
#if 1
	// 案例V2CA0990001v4.2c说明允许私有tag存在, 因此修正本函数, 只判断顺序
	int    i;
	uchar  *psTlvObj;
	int    iTlvObjLen;
	uint   uiTagLen;
	for(i=0; ; i++) {
		iTlvObjLen = iTlvSearchObj(psTemplate, iTemplateLen, i, "\xFF", &psTlvObj);
		if(iTlvObjLen == TLV_ERR_BUF_FORMAT)
			return(TLV_ERR_BUF_FORMAT);
		if(iTlvObjLen == TLV_ERR_NOT_FOUND)
			break; // 模板中已经没有Tlv数据对象了
        uiTagLen = _uiTlvGetTagLen(psTlvObj);
		if(memcmp(psTlvObj, psOrder, uiTagLen) != 0)
			continue;
		psOrder += uiTagLen;
		if(*psOrder == 0)
			break; // psOrder标识的Tag全部存在且顺序正常
	}
	if(*psOrder != 0)
		return(TLV_ERR_CHECK); // 模版数据中缺失Tlv对象不存在, 报错
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
			break; // 模板中已经没有Tlv数据对象了
		if(*psOrder == 0)
			return(TLV_ERR_CHECK); // 模版数据中存在多余Tlv对象, 报错
        uiTagLen = _uiTlvGetTagLen(psTlvObj);
		if(memcmp(psTlvObj, psOrder, uiTagLen) != 0)
			return(TLV_ERR_CHECK); // 模版数据中Tlv顺序不对, 报错
		psOrder += uiTagLen;
	}
	if(*psOrder != 0)
		return(TLV_ERR_CHECK); // 模版数据中缺失Tlv对象不存在, 报错
    return(TLV_OK);
#endif
}

// 功能：核查Tlv模板对象内容是否有重复
// 输入：psTemplate			: TLV模板内容
//       iTemplateLen       : 模板内容大小
// 返回：0					: 检查正确
//	     TLV_ERR_BUF_FORMAT	: TLV模板格式错误
//       TLV_ERR_CHECK      : 检查失败
//       TLV_OK             : 检查成功
int iTlvCheckDup(uchar *psTemplate, int iTemplateLen)
{
	uchar sTlvBuf[400];
	int   iRet;

	iTlvSetBuffer(sTlvBuf, sizeof(sTlvBuf));
	iRet = iTlvCheckTlvObject(psTemplate);
	if(iRet<0 || iRet>iTemplateLen)
		return(TLV_ERR_CHECK);
	iRet = iTlvBatchAddObj(1/*1:添加所有对象*/, sTlvBuf, psTemplate, iTemplateLen, TLV_CONFLICT_ERR, 1/*1:长度为0对象也添加*/);
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

// 功能：检查TLV对象是否合法
// 输入：psTlvObj           : TLV对象指针
// 返回：>0					: 找到，TLV对象的长度
//	     TLV_ERR_OBJ_FORMAT	: TLV对象格式错误
//       TLV_ERR_OBJ_LENGTH : TLV对象长度不满足EMV规范
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
    iRet = iTlvBatchAddObj(0, sTlvBuf1, sBuf, iLen, TLV_CONFLICT_ERR, 0/*0:长度为0对象不添加*/);

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