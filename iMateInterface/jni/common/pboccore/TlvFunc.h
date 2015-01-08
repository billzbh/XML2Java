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
				Aut 14th, 2012
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
# ifndef _TLVFUNC_H
# define _TLVFUNC_H

#ifdef __cplusplus
extern "C" {
#endif

// 冲突控制标志
#define TLV_CONFLICT_ERR		0	// 如果TLV对象发生冲突，认为出错，终止处理
#define TLV_CONFLICT_IGNORE		1	// 如果TLV对象发生冲突，忽略，继续处理
#define TLV_CONFLICT_REPLACE	2	// 如果TLV对象发生冲突，替换掉原TLV对象

// -10XXX 为TLV操作错误码
#define TLV_OK				 0      // 成功
#define TLV_ERR_BUF_SPACE    -10001 // TLV存储区无足够的存储空间
#define TLV_ERR_BUF_FORMAT   -10002 // TLV存储区格式错误
#define TLV_ERR_CONFLICT     -10003 // 冲突，TLV对象已存在
#define TLV_ERR_NOT_FOUND    -10004 // 没找到对应的TLV对象
#define TLV_ERR_OBJ_FORMAT   -10005 // TLV对象结构非法
#define TLV_ERR_TEMP_FORMAT  -10006 // TLV模板结构非法
#define TLV_ERR_DOL_FORMAT   -10007 // TLV对象结构非法
#define TLV_ERR_OBJ_LENGTH   -10008 // TLV对象长度不满足EMV规范
#define TLV_ERR_CHECK        -10009 // TLV对象检查错误
#define TLV_ERR_OTHER        -10999 // 其它错误

// 功能：初始化TLV存储区，一个TLV存储区必须先初始化才能使用
// 输入：psTlvBuffer		: TLV存储区
//       uiTlvBufferSize	: 字节单位的存储区长度，至少10字节
// 返回：>=0				: 完成, 可用的存储空间
//       TLV_ERR_BUF_SPACE  : 存储空间不足，存储区不足10字节
int iTlvSetBuffer(uchar *psTlvBuffer, uint uiTlvBufferSize);

// 功能：添加TLV对象到TLV存储区
// 输入：psTlvBuffer		: TLV存储区
//       psTlvObject		: TLV对象，可以是结构TLV或基本TLV
//       iConflictFlag		: 冲突处理标志, TLV_CONFLICT_ERR|TLV_CONFLICT_IGNORE|TLV_CONFLICT_REPLACE
// 返回：>0					: 添加成功或已存在完全相同的TLV对象, 对象大小
// 	     TLV_ERR_BUF_SPACE	: TLV存储区无足够的存储空间
//  	 TLV_ERR_BUF_FORMAT	: TLV存储区格式错误
//       TLV_ERR_CONFLICT	: 冲突，TLV对象已存在
//       TLV_ERR_OBJ_FORMAT	: TLV对象结构非法
int iTlvAddObj(uchar *psTlvBuffer, uchar *psTlvObject, int iConflictFlag);

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
int iTlvMakeAddObj(uchar *psTlvBuffer, uchar *psTag, uint uiLength, uchar *psValue, int iConflictFlag);

// 功能：从TLV存储区中删除TLV对象，可以省出空间存放其它的TLV对象
// 输入：psTlvBuffer		: TLV存储区
//       psTag				: 要删除的TLV对象的Tag
// 返回：>0					: 完成, 所删除的TLV对象大小
// 	     TLV_ERR_BUF_FORMAT	: TLV存储区格式错误
//       TLV_ERR_NOT_FOUND	: 没找到对应的TLV对象
int iTlvDelObj(uchar *psTlvBuffer, uchar *psTag);

// 功能：从TLV存储区中取出相应Tag的TLV对象的指针
// 输入：psTlvBuffer		: TLV存储区
// 	     psTag				: 要读取的TLV对象的Tag
// 输出：ppsTlvObject		: TLV对象指针
// 返回：>0					: 完成，TLV对象的长度
//  	 TLV_ERR_BUF_FORMAT	: TLV存储区格式错误
//  	 TLV_ERR_NOT_FOUND	: 没找到对应的TLV对象
int iTlvGetObj(uchar *psTlvBuffer,uchar *psTag,uchar **ppsTlvObject);

// 功能：从TLV存储区中取出相应位置的TLV对象的指针，用于遍历TLV存储区
// 输入：psTlvBuffer		: TLV存储区
// 	     uiIndex			: 索引号，0开始
// 输出：ppsTlvObject		: TLV对象指针
// 返回：>0					: 完成，TLV对象的长度
//	     TLV_ERR_BUF_FORMAT	: TLV存储区格式错误
//	     TLV_ERR_NOT_FOUND	: 没找到对应的TLV对象
int iTlvGetObjByIndex(uchar *psTlvBuffer, uint uiIndex, uchar **ppsTlvObject);

// 功能：从TLV存储区中取出相应Tag的TLV对象值的指针
// 输入：psTlvBuffer		: TLV存储区
// 	     psTag				: 要读取的TLV对象的Tag
// 输出：ppsValue			: TLV对象值的指针
// 返回：>=0				: TLV对象值长度
//	     TLV_ERR_BUF_FORMAT	: TLV存储区格式错误
//	     TLV_ERR_NOT_FOUND	: 没找到对应的TLV对象
int iTlvGetObjValue(uchar *psTlvBuffer, uchar *psTag, uchar **ppsValue);

// 功能：读取TLV对象值
// 输入：psTlvObj			: TLV对象
// 输出：ppsTlvObjValue	    : TLV对象值
// 返回：>=0				: TLV对象值大小
//	     TLV_ERR_OBJ_FORMAT	: TLV对象格式错误
int iTlvValue(uchar *psTlvObj, uchar **ppsTlvObjValue);

// 功能：读取TLV对象值长度
// 输入：psTlvObj			: TLV对象
// 返回：>=0				: TLV对象值大小
//	     TLV_ERR_OBJ_FORMAT	: TLV对象格式错误
int iTlvValueLen(uchar *psTlvObj);

// 功能：读取TLV对象Tag长度
// 输入：psTlvObj			: TLV对象
// 返回：1 or 2				: TLV对象Tag大小
//	     TLV_ERR_OBJ_FORMAT	: TLV对象格式错误
int iTlvTagLen(uchar *psTlvObj);

// 功能：读取TLV对象Length长度
// 输入：psTlvObj			: TLV对象
// 返回：1 or 2				: TLV对象Len大小
//	     TLV_ERR_OBJ_FORMAT	: TLV对象格式错误
int iTlvLengthLen(uchar *psTlvObj);

// 功能：读取TLV对象长度
// 输入：psTlvObj			: TLV对象
// 返回：>0                 : TLV对象大小
//	     TLV_ERR_OBJ_FORMAT	: TLV对象格式错误
// 注意: 00 or FF被认为是格式错误
int iTlvObjLen(uchar *psTlvObj);

// 功能：读取TLV对象值
// 输入：psTlvObj			: TLV对象
// 返回：!NULL			    : TLV对象值
//	     NULL				: TLV对象格式错误
uchar *psTlvValue(uchar *psTlvObj);

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
int iTlvSearchObj(uchar *psTemplate, int iTemplateLen, int iIndex, uchar *psTagRoute, uchar **ppsTlvObj);

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
int iTlvSearchObjValue(uchar *psTemplate, int iTemplateLen, int iIndex, uchar *psTagRoute, uchar **ppsTlvObjValue);

// 功能：从TLV模板中取出指定位置的TLV对象(可能是基本TLV对象，也可能是另外一个模板)，用于遍历模板
// 输入：psTemplate			: TLV模板
//       iTemplateLen       : 模板大小
// 	     iIndex			    : 指明第几个，0表示第一个
// 输出：ppsTlvObj		    : TLV对象
// 返回：>0					: 找到，TLV对象大小
//	     TLV_ERR_BUF_FORMAT	: TLV模板格式错误
//	     TLV_ERR_NOT_FOUND	: 不存在该索引的对象
int iTlvSerachTemplate(uchar *psTemplate, int iTemplateLen, int iIndex, uchar **ppsTlvObj);

// 功能：检查TLV对象是否合法
// 输入：psTlvObj           : TLV对象指针
// 返回：>0					: 找到，TLV对象的长度
//	     TLV_ERR_OBJ_FORMAT	: TLV对象格式错误
//       TLV_ERR_OBJ_LENGTH : TLV对象长度不满足EMV规范
int iTlvCheckTlvObject(uchar *psTlvObj);

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
int iTlvBatchAddObj(uchar ucFlag, uchar *psTlvBuffer, uchar *psTlvTemplate, int iTlvTemplateLen, int iConflictFlag, int iLen0Flag);

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
//	     TLV_ERR_TEMP_FORMAT: 55域中Tlv对象格式非法
//       TLV_ERR_OBJ_FORMAT	: TLV对象结构非法
// Note: 只添加来自发卡方的Tlv对象
int iTlvBatchAddField55Obj(uchar ucFlag, uchar *psTlvBuffer, uchar *psField55, int iField55Len, int iConflictFlag);

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
int iTlvBatchAddField55ObjHost(uchar ucFlag, uchar *psTlvBuffer, uchar *psField55, int iField55Len, int iConflictFlag);

// 功能：将一个TLV存储区内的所有TLV对象添加到另一个TLV存储区中
// 输入：psTlvSourceBuffer  : 源TLV存储区
//       iConflictFlag		: 冲突处理标志, TLV_CONFLICT_ERR|TLV_CONFLICT_IGNORE|TLV_CONFLICT_REPLACE
// 输出：psTlvTargetBuffer	: 目标TLV存储区
// 返回：>=0				: 完成, 拷贝的TLV对象个数
// 	     TLV_ERR_BUF_SPACE	: 目标TLV存储区无足够的存储空间
//	     TLV_ERR_BUF_FORMAT	: 存在TLV存储区格式错误
//	     TLV_ERR_CONFLICT	: 冲突，终止处理
int iTlvCopyBuffer(uchar *psTlvTargetBuffer, uchar *psTlvSourceBuffer, int iConflictFlag);

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
int iTlvMakeObject(uchar *psTag, uint uiLength, uchar *psValue, uchar *psTlvObject, int iTlvObjectBufSize);

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
          	         uchar *psTlvBuffer1, uchar *psTlvBuffer2, uchar *psTlvBuffer3, uchar *psTlvBuffer4);

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
          	    	 uchar *psTlvBuffer1, uchar *psTlvBuffer2, uchar *psTlvBuffer3, uchar *psTlvBuffer4);

// 功能：搜索DOL中是否包括某一Tag
// 输入：psDOL				: Data Object List
// 	     iDOLLen		    : DOL长度
//       psTag              : 要搜索的Tag
// 输出  piOffset           : 如果DOL中包含搜索的TAG, 返回该TAG在DOL数据块中的偏移量, NULL表示不需要返回
// 返回：>=0				: Tag存在, 返回该Tag需要的长度
//       TLV_ERR_DOL_FORMAT : DOL格式错误
// 注意: piOffset返回的是用该DOL做模板打包成的数据块中的偏移量
int iTlvSearchDOLTag(uchar *psDOL, int iDOLLen,	uchar *psTag, int *piOffset);

// 功能：核查Tlv对象顺序合法性
// 输入：psTemplate			: TLV模板内容
//       iTemplateLen       : 模板内容大小
//       psTagOrder         : 要求的TLV对象顺序, '\x00'Tag表示结束
// 返回：0					: 检查正确
//	     TLV_ERR_BUF_FORMAT	: TLV模板格式错误
//       TLV_ERR_CHECK      : 检查失败
//       TLV_OK             : 检查成功
// 注意: psTagOrder标明的Tag必须以正确顺序出现在psTemplate中
int iTlvCheckOrder(uchar *psTemplate, int iTemplateLen, uchar *psOrder);

// 功能：核查Tlv模板对象内容是否有重复
// 输入：psTemplate			: TLV模板内容
//       iTemplateLen       : 模板内容大小
// 返回：0					: 检查正确
//	     TLV_ERR_BUF_FORMAT	: TLV模板格式错误
//       TLV_ERR_CHECK      : 检查失败
//       TLV_OK             : 检查成功
int iTlvCheckDup(uchar *psTemplate, int iTemplateLen);

#ifdef __cplusplus
}
#endif

# endif