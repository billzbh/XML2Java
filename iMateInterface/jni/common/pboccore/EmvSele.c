/**************************************
File name     : EMVSELE.C
Function      : EMV应用选择模块
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
模块概述      : EMV卡应用选择，使用方法
                调用iSelGetAids()建立候选应用列表
				    gl_iCardAppNum  : 终端与卡片同时支持的应用数目
					gl_aCardAppList : 终端与卡片同时支持的应用列表
                调用iSelSortCandidate()给候选列表排序
				与外部交互或自动选择，选中应用
                调用iSelFinalSelect()选择应用
				static int _iSelCandidateListOp(uchar ucCmd, stADFInfo *pAdf)
更新记录      : 20121220, 修改_iSelCandidateListOp(), 在增加应用到候选列表时, 检查是否重复, 如果重复, 不重复增加应用
更新记录      : 20130228, 修改_iSelCandidateListOp(), 在增加应用到候选列表时, 检查label、preferred name是否重复, 如果重复, 不重复增加应用
更新记录      : 20140814, 修改_iSelCandidateListOp(), 修正判断应用是否需要持卡人确认时错误
**************************************/
/*
模块详细描述:
	EMV应用选择支持模块
.	支持PSE方式与AID列表方式选择应用
.	支持DDF(EMV4.3/PBOC3.0已经取消对DDF的支持了, 终端可以不支持也可以支持DDF, 由于早就做好了DDF支持, 没有去除)
	_iSelDdfQueueOp()函数用来操作一个内部DDF队列
.	PSE方法处理
	将PSE("1PAY.SYS.DDF01")作为一个DDF加到DDF队列, 然后开始处理DDF队列
	如果DDF队列为空, 退出PSE处理方法, 否则循环执行
		从队列中取出一DDF, 处理其指明的记录文件, 依次读取记录处理(每条记录可能有多个入口,分别处理)
		如果入口为DDF, 将其排入DDF队列, 继续处理
		如果入口为支持的ADF, 将其加入支持的应用列表
.	AID列表方法处理
	依次选择终端支持的AID, 如果卡片存在, 则将其加入到支持的应用列表
	遇到部分名匹配, 需要判断ASI以决定是否支持, 如果支持部分名字且部分名字匹配, 需要继续选择同一AID, 直到卡片返回应用不支持
.	应用选择
	首选PSE方法, 如果失败或没有匹配应用, 继续使用AID列表法
.	最终应用匹配情况保存在EMV核心数据库中(EmvData.c)
		gl_iCardAppNum;                 // 终端与卡片同时支持的应用数目
		gl_aCardAppList[];              // 终端与卡片同时支持的应用列表
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

static uchar  sg_szLanguage[8+1];		 // PSE中出现的候选语言
static int    sg_iIssuerCodeTableIndex;  // PSE中出现的字符集代码, -1:not exist

static int    sg_iTotalMatchedApps;      // 卡片与终端最初同时支持的应用个数
static int    sg_iCardHolderConfirmed;   // 1:持卡人已经进行过确认 0:持卡人未进行过确认
                                         // 用于确定持卡人是否曾确定过某应用
static uchar  sg_sFciTlvBuf[300];        // 最近一次选择应用时保存的FCI数据

// 检查字符串是否全部属于公共字符集
// ret : 0 : 全部属于公共字符集
//       1 : 不全部属于公共字符集
static int iCharCheck(uchar *pszStr)
{
	int i;
	for(i=0; i<(int)strlen(pszStr); i++) {
		if(pszStr[i]<0x20 || pszStr[i]>0x7E)
			return(1);
	}
	return(0);
}

// DDF 队列管理
// 输入: ucCmd        : 队列指令
//                      DDF_QUEUE_INIT
//                      DDF_QUEUE_IN
//                      DDF_QUEUE_OUT
//                      DDF_QUEUE_STATUS
// 说明: 其它参数与返回值由于队列指令的不同而不同, 详述如下：
// ucCmd == DDF_QUEUE_INIT
//         功能: 清空队列，其它参数不使用
//         返回: 0              : 成功
// ucCmd == DDF_QUEUE_IN
//         功能: 新的DDF进入队列
//         输入: ucDdfNameLen   : DDF名字长度
//               psDdfName      : DDF名字
//         返回: 0              : 成功
//               -1             : 队列满
// ucCmd == DDF_QUEUE_OUT
//         功能: 从队列中取出DDF
//         输出: ucDdfNameLen   : DDF名字长度
//               psDdfName      : DDF名字
//         返回: 0              : 成功
//               -1             : 队列已空
// ucCmd == DDF_QUEUE_STATUS
//         功能: 查看队列当前状况，其它参数不用
//         返回: 0              : 队列已空
//               >0             : 队列中DDF个数
# define DDF_QUEUE_INIT     0
# define DDF_QUEUE_IN       1
# define DDF_QUEUE_OUT      2
# define DDF_QUEUE_STATUS   3
static int _iSelDdfQueueOp(uchar ucCmd, uchar *pucDdfNameLen, uchar *psDdfName)
{
    static uchar sDFQueue[200]; // DDF队列, 格式:Length[1]Name[n] + Length[1]Name[n]...
    static uchar ucCurrDFs;     // 当前队列中DDF个数
    static int iCurrLength;   // 当前队列已经使用的大小
    
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

// 候选AID列表操作(gl_EmvAppData.aCandidateList, gl_EmvAppData.uiNumCandidate)
// 输入: ucCmd        : 指令
//                      CANDIDATE_LIST_INIT
//                      CANDIDATE_LIST_ADD
//                      CANDIDATE_LIST_DEL
//                      CANDIDATE_LIST_STATUS
// 说明: 其它参数与返回值由于队列指令的不同而不同, 详述如下：
// ucCmd == CANDIDATE_LIST_INIT
//         功能: 清空列表，其它参数不使用
//         返回: 0              : 成功
// ucCmd == CANDIDATE_LIST_ADD
//         功能: 增加一个候选
//         输入: pCandidate     : 候选记录指针
//         返回: 0              : 成功
//               -1             : 列表满
// ucCmd == CANDIDATE_LIST_DEL
//         功能: 删除一个候选
//         输入: pCandidate     : 候选记录指针(只用其中的sDFName、ucDFNameLen)
//         返回: 0              : 成功
//               -1             : 无此记录
// ucCmd == CANDIDATE_LIST_STATUS
//         功能: 查看候选队列当前状况，其它参数不用
//         返回: 0              : 列表空
//               >0             : 列表中候选个数
# define CANDIDATE_LIST_INIT	0
# define CANDIDATE_LIST_ADD		1
# define CANDIDATE_LIST_DEL		2
# define CANDIDATE_LIST_STATUS	3
static int _iSelCandidateListOp(uchar ucCmd, stADFInfo *pAdf)
{
    int i;
	int iRet;
	int iFoundFlag;
	uchar *p, ucAppConfirmSupport;          // TDFxx, 1:支持应用确认 0:不支持应用确认(TAG_DFXX_AppConfirmSupport)

	switch(ucCmd) {
	case CANDIDATE_LIST_INIT:
		// 初始化应用候选列表
		memset(gl_aCardAppList, 0, sizeof(gl_aCardAppList));
		gl_iCardAppNum = 0;
		break;
    case CANDIDATE_LIST_ADD:
		// 修正, 20130129 by yujun
		// 如果终端不支持应用确认, 而候选AID需要确认, 不将其加入候选列表
		iRet = iTlvGetObjValue(gl_sTlvDbTermFixed, TAG_DFXX_AppConfirmSupport, &p); // 获取终端应用选择支持情况
		if(iRet <= 0)
			ucAppConfirmSupport = 1; // 如果出问题, 假设终端支持确认
		else
			ucAppConfirmSupport = *p;
		if(ucAppConfirmSupport == 0) {
			// 终端不支持持卡人确认
			if(pAdf->iPriority<0 || (pAdf->iPriority&0x80) == 0x80)
				return(0); // 该应用需要确认，不能将其加入候选列表
		}

		if(gl_iCardAppNum >= sizeof(gl_aCardAppList)/sizeof(gl_aCardAppList[0]))
			return(-1); // 列表满
		// 修正, 20121220 by yujun
		// 为防止AID列表方式选择应用时出现的重复情况, 例:case V2CB0310603v4.2c
		// 查找重复项, 如果该AID已经加入了候选列表, 不再重复加入
		iFoundFlag = 0;
		for(i=0; i<gl_iCardAppNum; i++) {
			if(memcmp(&gl_aCardAppList[i], pAdf, sizeof(stADFInfo)) == 0) {
				iFoundFlag = 1;
				break;
			}
		}
		// 修正, 20130228 by yujun
		// 为防止AID列表方式选择应用时出现的重复情况, 例:case V2CE.003.05
		// 强化查找重复项label、preferred name, 如果该AID已经加入了候选列表, 不再重复加入
		// (原理:判断两个ADF显示给持卡人选择的内容是否相同, 如果相同, 认为重复)
		// **************************
		// 再次修正, 20130407 by yujun
		// case 2CL.001.01, PSE方式返回了2个应用, 这两个应用Preferred Name相同, 但案例需要这两个应用都是候选
		// 解决方案:分析案例, V2CE.003.05中两个应用Label与PreferredName都相同, 而2CL.001.xx中Label不同PreferredName相同
		//          因此修正程序, 判断Label与PreferredName都相同时才认为是重复
#if 0
// 20130228修正内容
		for(i=0; i<gl_iCardAppNum; i++) {
			// 1.检查preferred name是否重复
			if(gl_aCardAppList[i].szPreferredName[0]) {
				// 存在preferred name时才有比较的意义
				if(iCharCheck(gl_aCardAppList[i].szPreferredName) == 0) {
					// 只包含公共字符集才有比较的意义
					if(pAdf->szPreferredName[0] == 0)
						continue; // 新Adf无preferred name, 原Adf有preferred name, 不同, 继续下一个ADF
					if(iCharCheck(pAdf->szPreferredName) == 1)
						continue; // 新Adf之preferred name存在非公共字符集字符, 原Adf之preferred name不存在非公共字符集字符, 不同, 继续下一个ADF
					if(strcmp(gl_aCardAppList[i].szPreferredName, pAdf->szPreferredName) == 0) {
						iFoundFlag = 1;
						break; // preferred name相同, 不必再比较
					} else
						continue; // preferred name不同, 继续下一个Adf
				}
			}
			// 2.检查label是否重复
			if(gl_aCardAppList[i].szLabel[0]) {
				// 存在Label才有比较意义
				if(strcmp(gl_aCardAppList[i].szLabel, pAdf->szLabel) == 0) {
					iFoundFlag = 1;
					break; // label相同, 不必再比较
				} else
					continue; // label不同, 不再比较adf name
			}
			// 3.检查Adf name
			if(gl_aCardAppList[i].ucAdfNameLen == pAdf->ucAdfNameLen) {
				// adf name长度相同比较才有意义
				if(memcmp(gl_aCardAppList[i].sAdfName, pAdf->sAdfName, pAdf->ucAdfNameLen) == 0) {
					iFoundFlag = 1;
					break; // adf name相同, 不必再比较
				} else
					continue;
			}
		}
#else
// 20130407修正内容
		for(i=0; i<gl_iCardAppNum; i++) {
			// 1.检查preferred name是否重复
			if(gl_aCardAppList[i].szPreferredName[0]) {
				// 存在preferred name时才有比较的意义
				if(iCharCheck(gl_aCardAppList[i].szPreferredName) == 0) {
					// 只包含公共字符集才有比较的意义
					if(pAdf->szPreferredName[0] == 0)
						continue; // 新Adf无preferred name, 原Adf有preferred name, 不同, 继续下一个ADF
					if(iCharCheck(pAdf->szPreferredName) == 1)
						continue; // 新Adf之preferred name存在非公共字符集字符, 原Adf之preferred name不存在非公共字符集字符, 不同, 继续下一个ADF
					if(strcmp(gl_aCardAppList[i].szPreferredName, pAdf->szPreferredName) != 0)
						continue; // preferred name不同, 继续下一个Adf
				}
			}
			// 2.检查label是否重复
			if(gl_aCardAppList[i].szLabel[0]) {
				// 存在Label才有比较意义
				if(strcmp(gl_aCardAppList[i].szLabel, pAdf->szLabel) != 0)
					continue; // label不同, 继续下一个Adf
			}
			// 3.Preferred Name与Label都相同
			//   检查Preferred Name与Label是否有效, 如果任一个有效, 认为相同, 不再进行Adf name检查
			if(gl_aCardAppList[i].szPreferredName[0] || gl_aCardAppList[i].szLabel[0]) {
				iFoundFlag = 1;
				break;
			}
			// 4.检查Adf name
			if(gl_aCardAppList[i].ucAdfNameLen == pAdf->ucAdfNameLen) {
				// adf name长度相同比较才有意义
				if(memcmp(gl_aCardAppList[i].sAdfName, pAdf->sAdfName, pAdf->ucAdfNameLen) == 0) {
					iFoundFlag = 1;
					break; // adf name相同, 不必再比较
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

// 功能：处理模板0x61, 将支持的AID放入候选列表
// 输入：psTlvObj61             : 0x61模板
//       iTlvObj61Len           : 0x61模板长度
// 返回：0	 			        : 成功完成
//       SEL_ERR_OVERFLOW       : 存储空间不足,DDF队列或应用列表
//       SEL_ERR_DATA_ERROR     : 相关数据非法
//       SEL_ERR_DATA_ABSENT    : 必要数据不存在
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
        return(SEL_ERR_DATA_ABSENT); // neither DDF nor ADF found, 必须数据不存在

    // 找到一个ADF入口
	memset(&OneAdf, 0, sizeof(OneAdf)); // 初始化
	OneAdf.iPriority = -1; // 初始化
	strcpy(OneAdf.szLanguage, sg_szLanguage); // 填从PSE处得到的值
	OneAdf.iIssuerCodeTableIndex = sg_iIssuerCodeTableIndex; // 填从PSE处得到的值
    for(i=0; i<4; i++) {
        // i = 0 : ADF name
        // i = 1 : label
        // i = 2 : preferred name
        // i = 3 : priority indicator
        // 没有在这里搜索T73，在最终应用选择时会搜索TBF0C
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
        if(iRet == TLV_ERR_NOT_FOUND && (i==2 || i==3)) // not found but optional, Refer to Test Case V2CL0050000v4.1a, Label缺失需要改用AidList方法
            continue; 
        if(iRet == TLV_ERR_NOT_FOUND)
            return(SEL_ERR_DATA_ABSENT); // not found and mandatory

        iRet = iTlvCheckTlvObject(psTlvObj);
        if(iRet < 0) {
            if(i==1 || i==2)
                continue; // label or priority indicater，如果数据非法，忽略
            return(SEL_ERR_DATA_ERROR);
        }

		// 保存数据到候选记录OneAdf中
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

	// 判断是否支持此应用
	for(i=0; i<gl_iTermSupportedAidNum; i++) {
		if(gl_aTermSupportedAidList[i].ucASI == 0) {
			// 支持部分名字选择
			if(gl_aTermSupportedAidList[i].ucAidLen > OneAdf.ucAdfNameLen)
				continue; // not match in length
			if(memcmp(gl_aTermSupportedAidList[i].sAid, OneAdf.sAdfName, gl_aTermSupportedAidList[i].ucAidLen) != 0)
				continue; // not match in content
			// match
		} else {
			// 不支持部分名字选择
			if(gl_aTermSupportedAidList[i].ucAidLen != OneAdf.ucAdfNameLen)
				continue; // not match in length
			if(memcmp(gl_aTermSupportedAidList[i].sAid, OneAdf.sAdfName, gl_aTermSupportedAidList[i].ucAidLen) != 0)
				continue; // not match in content
			// match
		}
   		// match found, 将其加入候选列表
		iRet = _iSelCandidateListOp(CANDIDATE_LIST_ADD, &OneAdf);
		if(iRet < 0)
    		return(SEL_ERR_OVERFLOW); // 存储空间满
		return(0); // 找到匹配，已成功加入候选列表
	} // for(i=0; i<gl_EmvTermPara.uiNumSupportedAid; i++
    return(0); // 没找到匹配，不支持此应用
}

// 功能：处理目录文件
// 输入：ucSFI                  : 目录文件
// 返回：0	 			        : 成功完成
//       SEL_ERR_CARD           : 卡片通讯错
//       SEL_ERR_CARD_SW        : 卡状态字非法
//       SEL_ERR_NOT_SUPPORT_T  : 终端不支持PSE方法
//       SEL_ERR_NOT_SUPPORT_C  : 卡片不支持PSE方法
//       SEL_ERR_NO_REC         : 目录文件无记录
//       SEL_ERR_OVERFLOW       : 存储空间不足,DDF队列或应用列表
//       SEL_ERR_DATA_ERROR     : 相关数据非法
//       SEL_ERR_DATA_ABSENT    : 必要数据不存在
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

	vPushApduPrepare(PUSHAPDU_READ_PSE_REC, ucSFI, 1, 255); // add by yujun 2012.10.29, 支持优化pboc2.0推送apdu
    
    for(ucRecNo=1; ; ucRecNo++) {
        uiRet = uiEmvCmdRdRec(ucSFI, ucRecNo, &ucRecLen, sRec);
        if(uiRet == 1)
            return(SEL_ERR_CARD);
        if(uiRet == 0x6a83) {
			if(ucRecNo == 1)
				return(SEL_ERR_NO_REC); // 第一条记录就被告知无此记录
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
            return(SEL_ERR_DATA_ERROR); // 无70模板

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

// 功能：用PSE方式找出候选应用列表
// 返回：>=0			        : 完成，找到的应用个数
//       SEL_ERR_CARD           : 卡片通讯错
//       SEL_ERR_CARD_SW        : 卡状态字非法
//       SEL_ERR_NOT_SUPPORT_T  : 终端不支持PSE方法
//       SEL_ERR_NOT_SUPPORT_C  : 卡片不支持PSE方法
//       SEL_ERR_CARD_BLOCKED   : 卡片已经被锁，该卡片应被拒绝，不必再尝试列表法
//       SEL_ERR_PSE_ERROR      : PSE方式出错
//       SEL_ERR_OVERFLOW       : 存储空间不足,DDF队列或应用列表
//       SEL_ERR_DATA_ERROR     : 相关数据非法
//       SEL_ERR_DATA_ABSENT    : 必要数据不存在
//       SEL_ERR_OTHER          : 其它错误
// Note: 找出的应用放在全局变量gl_aCardAppList[]中
//       找出的应用个数放在gl_iCardAppNum中
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

	_iSelCandidateListOp(CANDIDATE_LIST_INIT, NULL); // 初始化候选列表
	memset(sg_szLanguage, 0, sizeof(sg_szLanguage)); // 初始化PSE语言选项
	sg_iIssuerCodeTableIndex = -1;		    		 // 初始化PSE字符集定义
    _iSelDdfQueueOp(DDF_QUEUE_INIT, 0, 0); // 初始化DDF队列

    // queue the PSE as the first DDF
    strcpy(sDdfName, "1PAY.SYS.DDF01");
    ucDdfNameLen = strlen(sDdfName);
    _iSelDdfQueueOp(DDF_QUEUE_IN, &ucDdfNameLen, sDdfName);

    while(_iSelDdfQueueOp(DDF_QUEUE_OUT, &ucDdfNameLen, sDdfName) == 0) {
        // DDF队列未空
        uiRet = uiEmvCmdSelect(0x00/*P2:first occurrence*/ ,
                               ucDdfNameLen, sDdfName, &ucFciLen, sFci); // 选择该DDF
        if(uiRet == 1)
            return(SEL_ERR_CARD); // communication error
        if(!memcmp(sDdfName, "1PAY.SYS.DDF01", 14)) {
            // the current DDF is PSE
            if(uiRet == 0x6a81)
                return(SEL_ERR_CARD_BLOCKED); // 卡片被锁或卡片不支持Select指令，应终止Emv流程
            if(uiRet == 0x6a82)
                return(SEL_ERR_NOT_SUPPORT_C); // 卡片不支持PSE，应使用Aid列表方式选择应用
            if(uiRet == 0x6283)
                return(SEL_ERR_NOT_SUPPORT_C); // PSE被锁，应使用Aid列表方式选择应用
            if(uiRet)
                return(SEL_ERR_NOT_SUPPORT_C);// 任何其它返回， 应使用Aid列表方式选择应用
		} else {
			if(uiRet)
				continue; // 如果不是PSE, 在返回错误状态字时继续处理其它DDF
		}

        // check mandatory data object
	    strcpy(sTagRoute, "\x6F"); // search FCI
        iRet = iTlvSearchObjValue(sFci, (int)ucFciLen, 0, sTagRoute, &psFciValue);
	    if(iRet < 0)
		    return(SEL_ERR_DATA_ERROR);

// 案例V2CA1190000v4.3a要求允许T84与TA5顺序不同, 不做顺序检查 (想不起来当时为什么专门做了顺序检查, 发现因此不能通过的案例再分析原因)
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
	    iRet = iTlvSearchObjValue(sFci, (int)ucFciLen, 0, sTagRoute, &psTlvObjValue); // 读DDF Name
		if(iRet == TLV_ERR_BUF_FORMAT)
			return(SEL_ERR_DATA_ERROR);
        if(iRet == TLV_ERR_NOT_FOUND)
	        return(SEL_ERR_DATA_ABSENT);
		if(iRet != (int)ucDdfNameLen || memcmp(sDdfName, psTlvObjValue, (int)ucDdfNameLen))
			return(SEL_ERR_DATA_ERROR); // FCI中DDF name与选择用的DDF name不符
            
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
                    continue; // language or issuer code table index，如果数据非法，忽略
                // i = 0 : Language Preference
                // i = 1 : Issuer Code Table Index
				if(i == 0) {
					memcpy(sg_szLanguage, psTlvValue(psTlvObj), iTlvValueLen(psTlvObj));
				} else {
					sg_iIssuerCodeTableIndex = (int)*psTlvValue(psTlvObj);
					if(sg_iIssuerCodeTableIndex > 10)
						sg_iIssuerCodeTableIndex = -1; // 如果值非法，假设不存在
				}
            } // for(i=0; i<2; i++
        }

        iRet = _iSelProcessDirEF(ucSFI);
		if(iRet == SEL_ERR_NO_REC) {
	        if(!memcmp(sDdfName, "1PAY.SYS.DDF01", 14))
				return(SEL_ERR_PSE_ERROR); // 如果PSE的目录文件无内容, 返回PSE错
			else
				iRet = 0; // 如果非PSE的目录文件无内容, 不认为出错
		}
        if(iRet < 0)
            return(iRet);
    } // while(_iSelDdfQueueOp(DDF_QUEUE_OUT, &ucDdfNameLen, sDdfName) == 0
    
    return(gl_iCardAppNum);
}

// 功能：用PPSE方式找出候选应用列表
// 返回：>=0			        : 完成，找到的应用个数
//       SEL_ERR_CARD           : 卡片通讯错
//       SEL_ERR_CARD_SW        : 卡状态字非法
//       SEL_ERR_CARD_BLOCKED   : 卡片已经被锁，该卡片应被拒绝，不必再尝试列表法
//       SEL_ERR_OVERFLOW       : 存储空间不足,DDF队列或应用列表
//       SEL_ERR_DATA_ERROR     : 相关数据非法
//       SEL_ERR_DATA_ABSENT    : 必要数据不存在
//       SEL_ERR_OTHER          : 其它错误
// Note: 找出的应用放在全局变量gl_aCardAppList[]中
//       找出的应用个数放在gl_iCardAppNum中
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

	_iSelCandidateListOp(CANDIDATE_LIST_INIT, NULL); // 初始化候选列表
	memset(sg_szLanguage, 0, sizeof(sg_szLanguage)); // 初始化PSE语言选项
	sg_iIssuerCodeTableIndex = -1;		    		 // 初始化PSE字符集定义

    uiRet = uiEmvCmdSelect(0x00/*P2*/, 14, "2PAY.SYS.DDF01", &ucFciLen, sFci); // 选择PPSE
    if(uiRet == 1)
        return(SEL_ERR_CARD); // communication error
    if(uiRet == 0x6a81)
        return(SEL_ERR_CARD_BLOCKED); // 卡片被锁或卡片不支持Select指令，应终止Emv流程
    if(uiRet)
        return(SEL_ERR_CARD_SW);

	// search DDF name
    strcpy(sTagRoute, "\x6F""\x84"); // search DDF name
    iRet = iTlvSearchObjValue(sFci, (int)ucFciLen, 0, sTagRoute, &psTlvObjValue); // 读DDF Name
	if(iRet == TLV_ERR_BUF_FORMAT)
		return(SEL_ERR_DATA_ERROR);
    if(iRet == TLV_ERR_NOT_FOUND)
        return(SEL_ERR_DATA_ABSENT);
	if(iRet != 14 || memcmp("2PAY.SYS.DDF01", psTlvObjValue, 14))
		return(SEL_ERR_DATA_ERROR); // FCI中DDF name与选择用的DDF name不符

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
		memset(&OneAdf, 0, sizeof(OneAdf)); // 初始化
		OneAdf.iPriority = -1;
		OneAdf.iIssuerCodeTableIndex = -1;

		// search ADF name
		iRet = iTlvSearchObj(psTlvObj61, ucTlvObj61Len, 0, "\x61\x4F", &psTlvObj);
		if(iRet == TLV_ERR_BUF_FORMAT)
		    return(SEL_ERR_DATA_ERROR);
		if(iRet == TLV_ERR_NOT_FOUND)
			return(SEL_ERR_DATA_ABSENT); // 必须数据不存在
		if(iTlvCheckTlvObject(psTlvObj) < 0)
			return(SEL_ERR_DATA_ERROR); // 数据非法
		OneAdf.ucAdfNameLen = (uchar)iTlvValueLen(psTlvObj);
		memcpy(OneAdf.sAdfName, psTlvValue(psTlvObj), OneAdf.ucAdfNameLen);
		// search label
		iRet = iTlvSearchObj(psTlvObj61, ucTlvObj61Len, 0, "\x61\x50", &psTlvObj);
		if(iRet == TLV_ERR_BUF_FORMAT)
		    return(SEL_ERR_DATA_ERROR);
		if(iRet > 0) {
			// 找到label
			if(iTlvCheckTlvObject(psTlvObj) >= 0) {
				// 数据合法
				memcpy(OneAdf.szLabel, psTlvValue(psTlvObj), iTlvValueLen(psTlvObj));
			}
		}
		// search priority indicator
		iRet = iTlvSearchObj(psTlvObj61, ucTlvObj61Len, 0, "\x61\x87", &psTlvObj);
		if(iRet == TLV_ERR_BUF_FORMAT)
		    return(SEL_ERR_DATA_ERROR);
		if(iRet > 0) {
			// 找到priority indicator
			if(iTlvCheckTlvObject(psTlvObj) >= 0) {
				// 数据合法
				OneAdf.iPriority = *psTlvValue(psTlvObj);
			}
		}

		// 判断是否支持此应用
		for(i=0; i<gl_iTermSupportedAidNum; i++) {
			if(gl_aTermSupportedAidList[i].ucASI == 0) {
				// 支持部分名字选择
				if(gl_aTermSupportedAidList[i].ucAidLen > OneAdf.ucAdfNameLen)
					continue; // not match in length
				if(memcmp(gl_aTermSupportedAidList[i].sAid, OneAdf.sAdfName, gl_aTermSupportedAidList[i].ucAidLen) != 0)
					continue; // not match in content
				// match
			} else {
				// 不支持部分名字选择
				if(gl_aTermSupportedAidList[i].ucAidLen != OneAdf.ucAdfNameLen)
					continue; // not match in length
				if(memcmp(gl_aTermSupportedAidList[i].sAid, OneAdf.sAdfName, gl_aTermSupportedAidList[i].ucAidLen) != 0)
					continue; // not match in content
				// match
			}
   			// match found, 将其加入候选列表
			iRet = _iSelCandidateListOp(CANDIDATE_LIST_ADD, &OneAdf);
			if(iRet < 0)
    			return(SEL_ERR_OVERFLOW); // 存储空间满
			break; // 找到匹配，已成功加入候选列表
		} // for(i=0; i<gl_EmvTermPara.uiNumSupportedAid; i++
    } // for(uiIndex=0; ; uiIndex++

    return(gl_iCardAppNum);
}

// 功能：处理模板0x6F, 解出各元素
// 输入：psTlvObj6F             : 0x6F模板
//       iTlvObj6FLen           : 0x6F模板长度
// 输出：pOneAdf                : 如果成功，将解析出的属性返回
// 返回：0	 			        : 成功完成
//       SEL_ERR_DATA_ERROR     : 相关数据非法
//       SEL_ERR_DATA_ABSENT    : 必要数据不存在
//       SEL_ERR_IS_DDF         : 是DDF
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

// 案例V2CA1190000v4.3a要求允许T84与TA5顺序不同, 不做顺序检查 (想不起来当时为什么专门做了顺序检查, 发现因此不能通过的案例再分析原因)
//	iRet = iTlvCheckOrder(psTlvObj6FValue, iRet, "\x84""\xA5"); // check order
//	if(iRet != TLV_OK)
//		return(SEL_ERR_DATA_ERROR);

#if 0
// 4.3b测试案例已经取消了以下两个案例
    // 案例2CL.032.00.06 2CL.032.00.07要求PDOL(T9F38)不能直接出现在6F模板之下
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
		// 找到Tag88，认为是PSE 或是个 DDF
		// 不增加PSE或DDF到候选列表
		return(SEL_ERR_IS_DDF);
	}

	// 是ADF
	// 填缺省值
	memset(&OneAdf, 0, sizeof(OneAdf)); // 初始化
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
        if(iRet == TLV_ERR_NOT_FOUND && (i==2 || i==3 || i==4 || i==5 || i==6)) // label终端认为是可选
            continue; // not found but optional
        if(iRet == TLV_ERR_NOT_FOUND)
            return(SEL_ERR_DATA_ABSENT); // not found and mandatory

        iRet = iTlvCheckTlvObject(psTlvObj);
        if(iRet < 0) {
            if(i==2 || i==3)
                continue; // label or preffered name，如果数据非法，忽略
            return(SEL_ERR_DATA_ERROR);
        }

		// 保存数据到候选记录OneCandidate中
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

// 功能：用AID LIST方式找出候选应用列表
// 输入: iIgnoreBlock			: !0:忽略应用锁定标志, 0:锁定的应用不会被选择
// 返回：>=0	 		        : 完成，找到的应用个数
//       SEL_ERR_CARD           : 卡片通讯错
//       SEL_ERR_CARD_BLOCKED   : 卡片已经被锁，该卡片应被拒绝，不必再尝试
//       SEL_ERR_OVERFLOW       : 存储空间不足,DDF队列或应用列表
//       SEL_ERR_DATA_ERROR     : 应用相关数据非法
//       SEL_ERR_DATA_ABSENT    : 必要数据不存在
// Note: ****** 本函数返回任何错误，都要终止emv流程 ******
static int iSelGetAidsByAidList(int iIgnoreBlock)
{
    uchar sDfName[16], ucDfNameLen;
    uchar sFci[256], ucFciLen;
    uchar *psTlvObj, *psTlvObjValue;
    uchar sTagRoute[10];
    stADFInfo OneAdf;
	uchar ucAppBlockFlag; // 0:app not blocked, 1:app blocked
	uchar ucP2;           // 选择应用时，决定是选第一个APP还是选下一个APP的IC卡指令参数P2
    uint  uiRet;
    int   iRet;
    int   i;

	_iSelCandidateListOp(CANDIDATE_LIST_INIT, NULL); // 初始化候选列表
	vPushApduPrepare(PUSHAPDU_SELECT_BY_AID_LIST); // add by yujun 2012.10.29, 支持优化pboc2.0推送apdu
	for(i=0, ucP2=0x00; i<gl_iTermSupportedAidNum;) {
		// 依次尝试选择支持的应用
		uiRet = uiEmvCmdSelect(ucP2,
							   gl_aTermSupportedAidList[i].ucAidLen,
							   gl_aTermSupportedAidList[i].sAid,
							   &ucFciLen, sFci);
		if(uiRet == 1)
			return(SEL_ERR_CARD);
		if(uiRet==0x6A81 && i==0 && ucP2==0x00)
			return(SEL_ERR_CARD_BLOCKED); // 只在第一次选择应用时才检查卡片被锁情况

   		if(uiRet!=0 && uiRet!=0x6283/*0x6283=app blocked*/) {
            // 应用没找到
			ucP2 = 0x00;
    		i ++;
		    continue;
		}
		if(uiRet == 0x6283)
			ucAppBlockFlag = 1; // 记下当前应用锁定标志
		else
			ucAppBlockFlag = 0;

		// process FCI
        strcpy(sTagRoute, "\x6F""\x84"); // search DF Name
        iRet = iTlvSearchObj(sFci, (int)ucFciLen, 0, sTagRoute, &psTlvObj);
        if(iRet==TLV_ERR_BUF_FORMAT || iRet==TLV_ERR_NOT_FOUND) {
			// 搜索错误或关键数据没找到，不添加本aid，继续
   			ucP2 = 0x00; // 下一个支持的应用，重新开始
			i ++;
     		continue;
		}
        iRet = iTlvCheckTlvObject(psTlvObj);
        if(iRet < 0) {
			// 关键数据格式错误，不添加本aid，继续
   			ucP2 = 0x00; // 下一个支持的应用，重新开始
			i ++;
     		continue;
		}
		ucDfNameLen = (uchar)iTlvValue(psTlvObj, &psTlvObjValue); // 读DF Name
		memcpy(sDfName, psTlvObjValue, ucDfNameLen);
		if(ucDfNameLen < gl_aTermSupportedAidList[i].ucAidLen) {
			// ADF名字长度小于选择的Aid长度, 不添加本Aid, 继续
   			ucP2 = 0x00; // 下一个支持的应用，重新开始
			i ++;
     		continue;
		}
		if(memcmp(sDfName, gl_aTermSupportedAidList[i].sAid, gl_aTermSupportedAidList[i].ucAidLen)!=0) {
			// ADF名字既不完全匹配又不部分匹配, 不添加本Aid, 继续
   			ucP2 = 0x00; // 下一个支持的应用，重新开始
			i ++;
     		continue;
		}
        if(ucDfNameLen==gl_aTermSupportedAidList[i].ucAidLen) {
			// 完全名字匹配
			if(ucAppBlockFlag && !iIgnoreBlock) {
				// 应用被锁并且指明应用被锁后不被选择
    			ucP2 = 0x00; // 下一个支持的应用，重新开始
				i ++;
	     		continue;
			}
			iRet = _iSelProcessTlvObj6F(sFci, ucFciLen, &OneAdf); // 解析数据
			if(iRet < 0) {
				// 解析错误, 继续
    			ucP2 = 0x00; // 下一个支持的应用，重新开始
				i ++;
				continue;
			}
/*
            if(strlen(OneAdf.szLabel) == 0) {
				strcpy(OneAdf.szLabel, gl_aTermSupportedAidList[i].szLabel); // 如果tag6F中没有label，用终端参数中该应用的label代替
            }
*/
        	// 增加到候选列表
	        iRet = _iSelCandidateListOp(CANDIDATE_LIST_ADD, &OneAdf); // 增加到候选列表
	        if(iRet < 0)
   		        return(SEL_ERR_OVERFLOW); // 存储空间满
			ucP2 = 0x00; // 下一个支持的应用，重新开始
			i ++;
			continue;
		}
        // 部分匹配
		if(gl_aTermSupportedAidList[i].ucASI != 0) {
			// 该AID不支持部分名字选择
			ucP2 = 0x00; // 下一个支持的应用，重新开始
			i ++;
			continue;
		}

		// 该AID支持部分名字选择
		ucP2 = 0x02; // 下次继续用部分名字选择
		if(ucAppBlockFlag==0 || iIgnoreBlock) {
			// 应用如果没有被锁或指明忽略锁定，增加到候选列表
    		iRet = _iSelProcessTlvObj6F(sFci, ucFciLen, &OneAdf); // 解析数据
	    	if(iRet < 0)
				continue; // 解析错误, 继续
/*
            if(strlen(OneAdf.szLabel) == 0) {
				strcpy(OneAdf.szLabel, gl_aTermSupportedAidList[i].szLabel); // 如果tag6F中没有label，用终端参数中该应用的label代替
            }
*/
        	// 增加到候选列表
	        iRet = _iSelCandidateListOp(CANDIDATE_LIST_ADD, &OneAdf); // 增加到候选列表
	        if(iRet < 0)
   		        return(SEL_ERR_OVERFLOW); // 存储空间满
		}
	} // for(i=0; i<gl_EmvTermPara.uiNumSupportedAid; i++

    return(gl_iCardAppNum);
}

// 比较优先级
// 输入 : iP1  : 优先级1
//        iP2  : 优先级2
// 返回 : 0    : 相等
//        1    : iP1的优先级大于iP2
//        -1   : iP1的优先级小于iP2
static int _iSelComparePriority(int iP1, int iP2)
{
	if((iP1==-1 || (iP1&0x0F)==0) && (iP2==-1 || (iP2&0x0F)==0))
		return(0); // iP1、iP2都标明无优先级参数或有参数但无优先级，认为它们优先级相等
	if(iP2==-1 || (iP2&0x0F)==0)
		return(1); // iP2标明无优先级参数或有参数但无优先级，iP1优先
	if(iP1==-1 || (iP1&0x0F)==0)
		return(-1); // iP1标明无优先级参数或有参数但无优先级，iP2优先

	iP1 &= 0x0F;
	iP2 &= 0x0F;
	if(iP1 < iP2)
		return(1); // ucP1的优先级大
	if(iP1 > iP2)
		return(-1); // ucP1的优先级小
	return(0); // 相等
}

// 功能：给候选列表排序(gl_aCardAppList[])
// 返回：>=0 : 完成，所有候选应用个数
static int iSelSortCandidate(void)
{
    stADFInfo OneAdf;
	int iMax;
	int i, j;

	for(i=0; i<gl_iCardAppNum; i++) {
		iMax = i;
		for(j=i+1; j<gl_iCardAppNum; j++) {
			if(_iSelComparePriority(gl_aCardAppList[i].iPriority, gl_aCardAppList[j].iPriority) < 0) {
				// [j]的优先级大于[i]的优先级，调换
				memcpy(&OneAdf, &gl_aCardAppList[i], sizeof(OneAdf));
				memcpy(&gl_aCardAppList[i], &gl_aCardAppList[j], sizeof(OneAdf));
				memcpy(&gl_aCardAppList[j], &OneAdf, sizeof(OneAdf));
			}
		}
	}
    return(gl_iCardAppNum);
}

// 功能：用PSE+AidList方式找出候选应用列表
// 输入: iIgnoreBlock			: !0:忽略应用锁定标志, 0:锁定的应用不会被选择
// 返回：>=0			        : 完成，找到的应用个数
//       SEL_ERR_CARD           : 卡片通讯错
//       SEL_ERR_CARD_SW        : 非法状态字
//       SEL_ERR_CARD_BLOCKED   : 卡片已经被锁，该卡片应被拒绝，不必再尝试列表法
//       SEL_ERR_OVERFLOW       : 存储空间不足,DDF队列或应用列表
//       SEL_ERR_DATA_ERROR     : 应用相关数据非法
//       SEL_ERR_DATA_ABSENT    : 必要数据不存在
// Note: 找出的应用放在全局变量gl_aCardAppList[]中
//       找出的应用个数放在gl_iCardAppNum中
int iSelGetAids(int iIgnoreBlock)
{
	int iRet;

	// 初始化FCI临时数据库
	iTlvSetBuffer(sg_sFciTlvBuf, sizeof(sg_sFciTlvBuf));
	sg_iCardHolderConfirmed = 0;   // 初始化持卡人确认标记

	if(gl_iCardType == EMV_CONTACT_CARD) {
		// 接触卡
		// 首先用pse方法获取应用列表
		iRet = iSelGetAidsByPse();
		// 案例V2CB0230111v4.1a说明, 读记录时卡片返回状态字非法不能终止操作, 要转为AidList方式
		if(iRet==SEL_ERR_CARD || /*iRet==SEL_ERR_CARD_SW ||*/ iRet==SEL_ERR_CARD_BLOCKED || iRet==SEL_ERR_OVERFLOW) {
			// pse方式获取aid列表时发生致命错误，需终结emv流程
			//   卡片通讯错   ||       非法状态字        ||       卡片已经被锁           ||       DDF队列存储空间不足
			return(iRet);
		}
		if(iRet <= 0) {
			// 如果pse方法获取应用列表失败，尝试用aid列表法
			iRet = iSelGetAidsByAidList(iIgnoreBlock);
			if(iRet < 0) {
				// aid列表法获取aid列表时发生错误，需终结emv流程
				return(iRet);
			}
		}
	} else {
		// 非接卡
		// 用ppse方法获取应用列表
		iRet = iSelGetAidsByPpse();
		if(iRet==SEL_ERR_CARD || iRet==SEL_ERR_CARD_SW || iRet==SEL_ERR_CARD_BLOCKED || iRet==SEL_ERR_OVERFLOW) {
			// pse方式获取aid列表时发生致命错误，需终结emv流程
			//   卡片通讯错   ||       非法状态字      ||       卡片已经被锁         ||       DDF队列存储空间不足
			return(iRet);
		}
	}

	// 成功，进行排序(注：可能无匹配，即iRet==0)
	iRet = iSelSortCandidate();
    sg_iTotalMatchedApps = iRet; // 卡片与终端最初同时支持的应用个数
	return(iRet); // 返回获取的aid个数
}

// 功能：从gl_aCardAppList[]中删除一个应用
// 输入：ucAidLen       : 要删除的aid长度
//       psAid          : 要删除的aid
// 返回：>=0	 		: 完成，剩余的应用个数
//       SEL_ERR_NO_APP : 没找到要删除的应用
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
		return(SEL_ERR_NO_APP); // 没找到要删除的应用

	// i为要删除应用的下标
	for(; i<gl_iCardAppNum-1; i++)
		memcpy(&gl_aCardAppList[i], &gl_aCardAppList[i+1], sizeof(stADFInfo));
	memset(&gl_aCardAppList[gl_iCardAppNum-1], 0, sizeof(stADFInfo));
	gl_iCardAppNum --;
	return(0);
}

// 功能：试选应用
// 输入：pAdfInfo               : 要试选的应用
//       iIgnoreBlock			: !0:忽略应用锁定标志, 0:锁定的应用不会被选择
// 返回：0	 		            : 成功
//       SEL_ERR_CARD           : 卡片通讯错
//       SEL_ERR_DATA_ERROR     : 相关数据非法
//       SEL_ERR_DATA_ABSENT    : 必要数据不存在
//		 SEL_ERR_APP_BLOCKED    : 应用已经被锁
//       SEL_ERR_OTHER          : 其它错误
// Note: 如果该函数成功执行，选中的应用有关数据会保存到TLV数据库gl_sTlvDbCard[]与sg_sFciTlvBuf中
static int _iSelTrySelect(stADFInfo *pAdfInfo, int iIgnoreBlock)
{
	int   i;
	stADFInfo AdfInfo; // 被选择的应用返回的AdfInfo，注意:pAdfInfo可能是pse获取的数据
    uchar sFci[256], ucFciLen;
    uchar *psTlvObj;
    uchar sTagRoute[10];
	uchar sBuf[10];
    uint  uiRet;
	int   iLen;
    int   iRet, iRet2;
	int   iAppBlockFlag;

	iAppBlockFlag = 0;
	// 删除上次可能添加到gl_sTlvDbCard[]数据库中的应用FCI Tag数据，以sg_sFciTlvBuf为蓝本
	for(i=0; ; i++) {
		iRet = iTlvGetObjByIndex(sg_sFciTlvBuf, i, &psTlvObj);
		if(iRet < 0)
			break;
		iTlvDelObj(gl_sTlvDbCard, psTlvObj);
	}
	// 删除上次可能添加到gl_sTlvDbTermVar[]数据库中的终端数据 TAG_9F06_AID
	iTlvDelObj(gl_sTlvDbTermVar, TAG_9F06_AID);
	// 初始化Fci临时Tlv数据库sg_sFciTlvBuf[]
	iTlvSetBuffer(sg_sFciTlvBuf, sizeof(sg_sFciTlvBuf));

	// 选择应用
	uiRet = uiEmvCmdSelect(0x00/*P2*/, pAdfInfo->ucAdfNameLen, pAdfInfo->sAdfName, &ucFciLen, sFci);
	if(uiRet == 1)
		return(SEL_ERR_CARD);
	if(uiRet==0x6283) {
	    // 应用被锁
	    if(!iIgnoreBlock)
	        return(SEL_ERR_APP_BLOCKED); // 要求不忽略应用锁定, 返回应用被锁
	    else
    		iAppBlockFlag = 1; // 要求忽略应用锁定, 记录锁定标志, 继续
	}
	else if(uiRet)
		return(SEL_ERR_OTHER);

	// process FCI
	iRet = _iSelProcessTlvObj6F(sFci, ucFciLen, &AdfInfo);
   	if(iRet < 0)
    	return(SEL_ERR_OTHER);

	// 判断应用返回的AdfName和用于选择应用的Aid是否相同
	// refer Emv2008 book1 P148
	if(pAdfInfo->ucAdfNameLen != AdfInfo.ucAdfNameLen)
    	return(SEL_ERR_OTHER);
	if(memcmp(pAdfInfo->sAdfName, AdfInfo.sAdfName, pAdfInfo->ucAdfNameLen) != 0)
    	return(SEL_ERR_OTHER);

	// 保存基本数据
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
	// T4F, 将T84内容以T4F再存储一次
	iRet = iTlvMakeAddObj(gl_sTlvDbCard, TAG_4F_AID, AdfInfo.ucAdfNameLen, AdfInfo.sAdfName, TLV_CONFLICT_REPLACE);
	ASSERT(iRet >= 0);
	iRet2= iTlvMakeAddObj(sg_sFciTlvBuf, TAG_4F_AID, AdfInfo.ucAdfNameLen, AdfInfo.sAdfName, TLV_CONFLICT_REPLACE);
	ASSERT(iRet2 >= 0);
	if(iRet<0 || iRet2<0)
    	return(SEL_ERR_OTHER);
	// T50
	if(strlen(AdfInfo.szLabel)) {
		// T50虽然为必选项，但规范要求终端在没找到T50时仍然继续
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

	// 保存其它数据
    // search PDOL
    strcpy(sTagRoute, "\x6F""\xA5""\x9F\x38"); // search T9F38
    iRet = iTlvSearchObj(sFci, ucFciLen, 0, sTagRoute, &psTlvObj);
    if(iRet == TLV_ERR_BUF_FORMAT)
        return(SEL_ERR_DATA_ERROR);
    if(iRet > 0) {
	    // 找到PDOL
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
	    // 找到TBF0C
		iRet = iTlvCheckTlvObject(psTlvObj);
	    if(iRet < 0)
		    return(SEL_ERR_DATA_ERROR);
		// 添加TBF0C模板内的所有对象到数据库
		iLen = iRet;
		iRet = iTlvBatchAddObj(1/*0:只添加基本TLV对象，!0:添加所有TLV对象*/, gl_sTlvDbCard, psTlvObj, iLen, TLV_CONFLICT_ERR, 0);
    	ASSERT(iRet >= 0);
		iRet2= iTlvBatchAddObj(1/*0:只添加基本TLV对象，!0:添加所有TLV对象*/, sg_sFciTlvBuf, psTlvObj, iLen, TLV_CONFLICT_ERR, 0);
    	ASSERT(iRet2 >= 0);
		if(iRet<0 || iRet2<0)
			return(SEL_ERR_OTHER);
	}
	if(iAppBlockFlag)
		return(SEL_ERR_APP_BLOCKED); // 应用锁定标志存在, 返回应用被锁状态
    return(0);
}

// 功能：初始化gl_sTlvDbTermVar数据库, 将gl_sTlvDbTermFixed中已经设置数据的终端/AID公共参数复制到gl_sTlvDbTermVar中
// 返回：0	 		            : 成功
//       SEL_ERR_OTHER          : 其它错误
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

// 功能：获取选中的应用相关参数
// 输入：pAdfInfo               : 选中的应用
// 返回：0	 		            : 成功
//       SEL_ERR_OTHER          : 其它错误
// Note: 参数保存到gl_sTlvDbTermVar中
static int iSelGetAidPara(stADFInfo *pAdfInfo)
{
	stTermAid *pMatchedAid;
	int   i;
   	int   iRet;
	uchar sBuf[256];

	// 找到选中的AID
	for(i=0; i<gl_iTermSupportedAidNum; i++) {
		if(gl_aTermSupportedAidList[i].ucASI == 0) {
			// 部分名字匹配
			if(gl_aTermSupportedAidList[i].ucAidLen <= pAdfInfo->ucAdfNameLen) {
				if(memcmp(gl_aTermSupportedAidList[i].sAid, pAdfInfo->sAdfName, gl_aTermSupportedAidList[i].ucAidLen)==0)
					break; // 匹配
			}
		} else {
			// 全部名字匹配
			if(gl_aTermSupportedAidList[i].ucAidLen == pAdfInfo->ucAdfNameLen) {
				if(memcmp(gl_aTermSupportedAidList[i].sAid, pAdfInfo->sAdfName, gl_aTermSupportedAidList[i].ucAidLen)==0)
					break; // 匹配
			}
		}
	}
	if(i >= gl_iTermSupportedAidNum)
		return(SEL_ERR_OTHER); // 既然为选中的应用, 不可能找不到匹配
	pMatchedAid = &gl_aTermSupportedAidList[i];

	// 初始化终端/AID参数公共部分, 保存在gl_sTlvDbTermVar数据库中
	iRet = iInitTermDbVarWithCommonPara();
	if(iRet)
		return(SEL_ERR_OTHER);

	// 将Aid自有参数添加到gl_sTlvDbTermVar中
	// 如果冲突, 认为该参数为终端通用参数, 忽略
	// T9F09, 终端应用版本号
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

	// TDFxx, Maximum Target Percentage to be used for Biased Random Selection，-1:无此数据(TAG_DFXX_MaxTargetPercentage)
	if(pMatchedAid->iMaxTargetPercentage >= 0) {
		if(pMatchedAid->iMaxTargetPercentage<0 || pMatchedAid->iMaxTargetPercentage>99)
			return(SEL_ERR_OTHER);
		if(pMatchedAid->iMaxTargetPercentage < pMatchedAid->iTargetPercentage)
			return(SEL_ERR_OTHER); // iMaxTergetPercentage至少要与iTargetPercentage一样大
		sBuf[0] = (uchar)pMatchedAid->iMaxTargetPercentage;
		iRet = iTlvMakeAddObj(gl_sTlvDbTermVar, TAG_DFXX_MaxTargetPercentage, 1, sBuf, TLV_CONFLICT_ERR);
		ASSERT(iRet>0 || iRet==TLV_ERR_CONFLICT);
		if(iRet<0 && iRet!=TLV_ERR_CONFLICT)
			return(SEL_ERR_OTHER);
	}

	// TDFxx, Target Percentage to be used for Random Selection，-1:无此数据(TAG_DFXX_TargetPercentage)
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
			return(SEL_ERR_OTHER); // Threshold必须小于FloorLimit
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

	// T9F7B, 终端电子现金交易限额
	if(strlen(pMatchedAid->szTermECashTransLimit)) {
		uchar szBuf[50];
		if(strlen(pMatchedAid->szTermECashTransLimit) > 12) {
			return(SEL_ERR_OTHER);
		}
		if(iTestStrDecimal(pMatchedAid->szTermECashTransLimit, strlen(pMatchedAid->szTermECashTransLimit)) != 0) {
			return(HXEMV_PARA); // 必须是十进制字符串表示
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
int iSelFinalSelect(int iIgnoreBlock)
{
    int   iRet;
	int   iAppSubscript;                  // 被选中的应用在gl_aCardAppList[]中的下标
	uchar *pucAppConfirmSupport;          // TDFxx, 1:支持应用确认 0:不支持应用确认(TAG_DFXX_AppConfirmSupport)
	
	// 获取终端应用选择支持情况
	iRet = iTlvGetObjValue(gl_sTlvDbTermFixed, TAG_DFXX_AppConfirmSupport, &pucAppConfirmSupport);
	if(iRet <= 0)
		return(SEL_ERR_OTHER);

	for(;;) {
// step 1 判断有无应用
		if(gl_iCardAppNum == 0)
			break; // 如果没有支持的应用
        if(gl_iCardAppNum != sg_iTotalMatchedApps && sg_iCardHolderConfirmed) {
            // 当前应用个数不同于卡片与终端最初同时支持的应用个数, 说明不是第一次选择应用, 需要显示"Try Again"
            iEmvIODispMsg(EMVMSG_13_TRY_AGAIN, EMVIO_NEED_NO_CONFIRM);
        }

// step 1.5
		if(gl_iCardType == EMV_CONTACTLESS_CARD) {
			iAppSubscript = 0;
   			goto label_select_finished; // 非接应用不需要确认，自动选中优先级最高的应用
		}

// step 2 如果只有一个应用
		if(gl_iCardAppNum == 1) {
			// 注：如果持卡人曾经进行过确认，即使只有一个应用，也不能自动选择
			iAppSubscript = 0; // 只有一个应用
			if(sg_iCardHolderConfirmed==0/*0:持卡人未进行过确认*/) // 只有之前没确认过应用时才允许自动选择应用
    			if(gl_aCardAppList[iAppSubscript].iPriority>=0 && (gl_aCardAppList[iAppSubscript].iPriority&0x80) == 0)
	    			goto label_select_finished; // 该应用不需要确认，自动选中该应用
			// 要求持卡人确认
			if(*pucAppConfirmSupport == 0)
				break; // 终端不支持持卡人确认
			iRet = iEmvIOConfirmApp(&gl_aCardAppList[iAppSubscript]);
			if(iRet == EMVIO_OK) {
				sg_iCardHolderConfirmed = 1; // 持卡人做出了确认
				goto label_select_finished; // 用户确认，选中该应用
			}
			if(iRet == EMVIO_CANCEL)
				return(SEL_ERR_CANCEL);
			if(iRet == EMVIO_TIMEOUT)
				return(SEL_ERR_TIMEOUT);
			return(SEL_ERR_OTHER);
		}
// step 3 判断终端是否支持持卡人确认
		if(*pucAppConfirmSupport == 1) {
// step 4 终端支持持卡人确认
			iRet = iEmvIOSelectApp(&gl_aCardAppList[0], gl_iCardAppNum, &iAppSubscript);
			if(iRet == EMVIO_OK) {
				sg_iCardHolderConfirmed = 1; // 持卡人做出了选择
				if(iAppSubscript<0 || iAppSubscript>=gl_iCardAppNum)
					return(SEL_ERR_OTHER); // 选择的应用下标越界
				goto label_select_finished; // 用户确认，选中该应用
			}
			if(iRet == EMVIO_CANCEL)
				return(SEL_ERR_CANCEL);
			if(iRet == EMVIO_TIMEOUT)
				return(SEL_ERR_TIMEOUT);
			return(SEL_ERR_OTHER);
		} else {
// step 5 终端不支持持卡人确认
			// 选择优先级最高的且不需要确认的应用
			for(iAppSubscript=0; iAppSubscript<gl_iCardAppNum; iAppSubscript++) {
				if(gl_aCardAppList[iAppSubscript].iPriority < 0)
					continue; // 如果该应用无优先级项，略过
				if((gl_aCardAppList[iAppSubscript].iPriority&0x80) == 0)
					goto label_select_finished; // 该应用不需要确认，自动选中该应用
			}
			break; // 没有可以自动选中的应用
		}

label_select_finished: // 用户选择结束，准备执行卡片应用选择指令
		iRet = _iSelTrySelect(&gl_aCardAppList[iAppSubscript], iIgnoreBlock);
		if(iRet==0 || (iRet==SEL_ERR_APP_BLOCKED && iIgnoreBlock)) {
			// 选择成功或应用被锁但要求忽略应用锁定, 将关联到该应用上的终端参数保存到Tlv数据库gl_sTlvDbTermVar中
			iRet = iSelGetAidPara(&gl_aCardAppList[iAppSubscript]);
			return(iRet);
		}
		if(iRet == SEL_ERR_CARD)
			return(SEL_ERR_CARD);

        // 试选失败，删除该应用，重新选择
        iSelDelCandidate(gl_aCardAppList[iAppSubscript].ucAdfNameLen, gl_aCardAppList[iAppSubscript].sAdfName);

		if(gl_iCardType == EMV_CONTACTLESS_CARD)
			break; // 如果是非接卡, 直接退出, 参考JR/T0025.12—2013图4, P11
	} // for(;;
	return(SEL_ERR_NO_APP); 
}

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
int iSelFinalSelect2(int iIgnoreBlock, int iAidLen, uchar *psAid)
{
    int   iRet;
	int   iAppSubscript;                  // 被选中的应用在gl_aCardAppList[]中的下标
	
	// 检查选择的应用是否在支持的应用范围之内
	for(iAppSubscript=0; iAppSubscript<sg_iTotalMatchedApps; iAppSubscript++) {
		if(iAidLen == gl_aCardAppList[iAppSubscript].ucAdfNameLen) {
			if(memcmp(psAid, gl_aCardAppList[iAppSubscript].sAdfName, iAidLen) == 0)
				break;
		}
	}
	if(iAppSubscript >= gl_iCardAppNum)
		return(SEL_ERR_NO_APP); // 调用层必须保证该应用存在

	iRet = _iSelTrySelect(&gl_aCardAppList[iAppSubscript], iIgnoreBlock);
	if(iRet==0 || (iRet==SEL_ERR_APP_BLOCKED && iIgnoreBlock)) {
		// 选择成功, 将关联到该应用上的终端参数保存到Tlv数据库gl_sTlvDbTermVar中
		iRet = iSelGetAidPara(&gl_aCardAppList[iAppSubscript]);
		if(iRet)
			return(SEL_ERR_OTHER);
		return(0);
	}

	if(iRet == SEL_ERR_CARD)
		return(SEL_ERR_CARD);
    // 试选失败，删除该应用，重新选择
	iSelDelCandidate(gl_aCardAppList[iAppSubscript].ucAdfNameLen, gl_aCardAppList[iAppSubscript].sAdfName);
	return(SEL_ERR_NEED_RESELECT);
}
