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

// -11XXX 为应用选择操作错误码
#define SEL_ERR_CARD           -11001 // 卡片通讯错
#define SEL_ERR_CARD_SW        -11002 // 卡状态字非法
#define SEL_ERR_CARD_BLOCKED   -11003 // 卡片已经被锁
#define SEL_ERR_TLV_BUFFER     -11004 // TLV存储区错误，未初始化、溢出...
#define SEL_ERR_NOT_FOUND      -11005 // 未找到匹配
#define SEL_ERR_NOT_SUPPORT_T  -11006 // 终端不支持PSE方法
#define SEL_ERR_NOT_SUPPORT_C  -11007 // 卡片不支持PSE方法
#define SEL_ERR_NOT_AVAILABLE  -11008 // 卡片不支持PSE方法
#define SEL_ERR_PSE_ERROR      -11009 // 卡片PSE出现故障，不支持、被锁...
#define SEL_ERR_NO_REC         -11010 // 目录文件无记录
#define SEL_ERR_OVERFLOW       -11011 // 存储空间不足,DDF队列或应用列表
#define SEL_ERR_DATA_ERROR     -11012 // 数据错误
#define SEL_ERR_DATA_ABSENT    -11013 // 必要数据不存在
#define SEL_ERR_IS_DDF         -11014 // 是DDF
#define SEL_ERR_APP_BLOCKED    -11015 // 应用已经被锁
#define SEL_ERR_NO_APP         -11016 // 没有应用可选
#define SEL_ERR_NEED_RESELECT  -11017 // 需要重新选择
#define SEL_ERR_CANCEL         -11018 // 用户取消
#define SEL_ERR_TIMEOUT        -11019 // 超时
#define SEL_ERR_OTHER          -11999 // 其它错误

// 功能：用PSE+AidList方式找出候选应用列表
// 输入: iIgnoreBlock			: 1:忽略应用锁定标志, 0:锁定的应用不会被选择
// 返回：>=0			        : 完成，找到的应用个数
//       SEL_ERR_CARD           : 卡片通讯错
//       SEL_ERR_CARD_SW        : 非法状态字
//       SEL_ERR_CARD_BLOCKED   : 卡片已经被锁，该卡片应被拒绝，不必再尝试列表法
//       SEL_ERR_OVERFLOW       : 存储空间不足,DDF队列或应用列表
//       SEL_ERR_DATA_ERROR     : 应用相关数据非法
//       SEL_ERR_DATA_ABSENT    : 必要数据不存在
// Note: 找出的应用放在全局变量gl_aCardAppList[]中
//       找出的应用个数放在gl_iCardAppNum中
int iSelGetAids(int iIgnoreBlock);

// 功能：最终选择
// 输入: iIgnoreBlock			: !0:忽略应用锁定标志, 0:锁定的应用不会被选择
// 返回：0	 		            : 成功
//       SEL_ERR_CARD           : 卡片通讯错
//       SEL_ERR_NO_APP         : 没有应用可选
//		 SEL_ERR_APP_BLOCKED    : 应用已经被锁
//       SEL_ERR_DATA_ERROR     : 相关数据非法
//       SEL_ERR_CANCEL         : 用户取消
//       SEL_ERR_TIMEOUT        : 超时
//       SEL_ERR_OTHER          : 其它错误
// Note: 如果该函数成功执行，说明用户需要的应用已经被选择了
//       调用此函数前，必须成功调用了iSelGetAids建立候选应用列表
//       虽然函数叫最终选择(规范如此), 但GPO标明不支持该应用时，还需要回来重新选择
//       refer Emv2008 book1 12.4 (P147)
//             JR/T 0025.3—2010 12.3.4 (P54)
//             JR/T 0025.6—2010 7.2.5 (P13)
int iSelFinalSelect(int iIgnoreBlock);

// 功能：最终选择(提供给非回调核心使用)
// 输入: iIgnoreBlock			: !0:忽略应用锁定标志, 0:锁定的应用不会被选择
//       iAidLen                : AID长度
//       psAid                  : AID
// 返回：0	 		            : 成功
//       SEL_ERR_CARD           : 卡片通讯错
//       SEL_ERR_NO_APP         : 无此应用
//       SEL_ERR_NEED_RESELECT  : 需要重新选择
//       SEL_ERR_OTHER          : 其它错误
// Note: 如果该函数成功执行，说明用户需要的应用已经被选择了
//       调用此函数前，必须成功调用了iSelGetAids建立候选应用列表
//       虽然函数叫最终选择(规范如此), 但GPO标明不支持该应用时，还需要回来重新选择
//       refer Emv2008 book1 12.4 (P147)
//             JR/T 0025.3—2010 12.3.4 (P54)
//             JR/T 0025.6—2010 7.2.5 (P13)
int iSelFinalSelect2(int iIgnoreBlock, int iAidLen, uchar *psAid);

// 功能：从候选列表中删除一个应用
// 输入：ucAidLen       : 要删除的aid长度
//       psAid          : 要删除的aid
// 返回：>=0	 		: 完成，剩余的应用个数
//       SEL_ERR_NO_APP : 没找到要删除的应用
int iSelDelCandidate(uchar ucAidLen, uchar *psAid);

# endif
