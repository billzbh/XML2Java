/**************************************
File name     : PushApdu.c
Function      : PBOC2.0借贷记卡推送指令支持模块
Author        : Yu Jun
First edition : Ver 1.00 Oct 29th, 2012
Modified      :
**************************************/
/*
模块详细描述:
.	实现原理:
	为实现推送指令的高效率, 需要批量处理APDU指令, 但EMV核心流程不方便批处理APDU指令.
	本模块针对EMV所需要执行的APDU指令进行批量预执行, 然后将执行结果保存在预执行结果缓冲区中, 
	待EMV核心按规范分别调用这些被预执行的的指令时, 本模块从预执行结果缓冲区中找到之前已经执行完的结果传出.
.	宏USE_PUSH_APDU
	只有定义了该宏, 才启动批推送机制, 否则会通知上层函数预执行结果缓冲区为空, 上层函数需用传统方式执行APDU指令
	(正常POS/终端不要定义该宏, 只有公司后台执行EMV核心需要将指令推送到终端时才需要定义该宏, 终端需要实现下面定义的特殊批量处理指令)
.	高层调用方式
	高层只需要在执行能批处理的指令前调用vPushApduPrepare()并传入批指令类型及相应参数即可, 后续操作完全透明
	(VPOS层需支持批推送机制, 先执行推送APDU指令, 失败后再以正常方式执行APDU指令)
.	容错
	本模块从预执行结果缓冲区取结果时会核对当前指令, 如果不匹配, 会通知上层使用传统方式执行APDU指令
.	实现细节
	预执行批推送指令后保存执行状态, 置位sg_iApduBufLen与sg_psApduBuf为初始状态
	后续每一次APDU指令会取走该指令的结果, 然后调整sg_iApduBufLen与sg_psApduBuf准备下次提取, 直至提取完所有指令结果
		sg_psApduBuf;          // 当前指令执行到的位置(指向sg_sApduBuf中的某个位置)
		sg_iPushApduType;      // 当前缓冲区指令类型
		sg_iApduCmdLen;        // 指令Apdu长度
		sg_sApduCmd[2048];     // 指令Apdu内容
		sg_iApduRespLen;       // 应答Apdu长度
		sg_sApduResp[2048];	   // 应答Apdu内容
		sg_iApduBufLen;        // 当前剩余应答Apdu内容长度
		sg_psApduBuf;          // 当前指令执行到的位置(指向sg_sApduBuf中的某个位置)
.	批量处理APDU指令
	SW="\x00\x01"表示卡片IO错 SW="\x00\x02"表示数据超界
	*b) read pse records
		标准指令:
	        00 B2 P1=记录号 P2=SFI|100
		BatchApdu指令:
			Cmd:  5F B2 00 00 Lc=3 SFI[1]+起始记录号[1]+终止记录号[1]
	        Resp: 指令1SW[2]+指令1返回内容长度[1]+指令1返回内容[n]+指令2SW[2]+...
	*c) select by aid list
		标准指令:
			00 A4 04 00 lc Aid le
	        00 A4 04 02 lc Aid le
		BatchApdu指令:
			Cmd:  5F A4 00 00 Lc Asi[1]+AidLen[1]+Aid[n]+Asi[1]+...
	        Resp: 指令1SW[2]+指令1返回内容长度[1]+指令1返回内容[n]+指令2SW[2]+...
		    终端需要根据emv协议及Asi自动选择同一AID的下一应用
	*f) read records
		标准指令:
	        00 B2 P1=记录号 P2=SFI|100
		BatchApdu指令
			Cmd:  5F B2 01 00 Lc LcData[n]=AFL
	        Resp: 指令1SW[2]+指令1返回内容长度[1]+指令1返回内容[n]+指令2SW[2]+...
	*h) read atc last_atc pin_try_counter log_format ec_balance
		标准指令:
			80 CA TagH TagL
	    BatchApdu指令
		    Cmd:  5F CA 00 00 Lc LcData=Tag1[2]+Tag2[2]...
			Resp: 指令1SW[2]+指令1返回内容长度[1]+指令1返回内容[n]+指令2SW[2]+...
	*k) script
		标准指令:
			不定
	    BatchApdu指令
		    Cmd:  5F BA 00 00 Lc LcData=Emv 71或72脚本(格式:T86Obj1+T86Obj2...)
			Resp: 指令1SW[2]+指令1返回内容长度[1]+指令1返回内容[n]+指令2SW[2]+...
	*l) read transaction log
		同*b), 但起始记录号必须为1
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include "VposFace.h"
#include "EmvCore.h"
#include "EmvData.h"
#include "PushApdu.h"

//#define USE_PUSH_APDU				// 定义该宏, 使用优化推送APDU方式, 否则使用单指令Apdu非优化方式
#ifdef USE_PUSH_APDU
// 实现emv推送指令的外部函数
extern int iPboc20CustomApdu(char *psInData, int iInDataLen, char *psOutData, int *piOutDataLen);
#endif

static int   sg_iReaderNo;          // 卡座号
static int   sg_iPushApduType;      // 当前缓冲区指令类型
static int   sg_iApduCmdLen;        // 指令Apdu长度
static uchar sg_sApduCmd[2048];     // 指令Apdu内容
static int   sg_iApduRespLen;       // 应答Apdu长度
static uchar sg_sApduResp[2048];	// 应答Apdu内容

static int   sg_iApduBufLen;        // 当前剩余应答Apdu内容长度
static uchar *sg_psApduBuf;         // 当前指令执行到的位置(指向sg_sApduBuf中的某个位置)

// 初始化PushApdu缓冲区
// in  : iReaderNo : 执行推送缓冲的卡座号
void vPushApduInit(int iReaderNo)
{
	sg_iReaderNo = iReaderNo;
    vPushApduClear();
}

// 准备PushApdu缓冲区(真正执行指令, 将指令结果缓冲起来)
// in  : iPushApduType : PushApdu类型
//						 PUSHAPDU_READ_PSE_REC			读DDF目录文件
//                           (Type, Sfi, StartRecNo, MaxRecNo)
//						 PUSHAPDU_SELECT_BY_AID_LIST	根据AID_LIST选择应用
//                           (Type) 其它参数直接使用gl_iTermSupportedAidNum
//                                                  gl_aTermSupportedAidList[]
//						 PUSHAPDU_READ_APP_REC			读应用数据记录
//                           (Type, AflLen, Afl)
//						 PUSHAPDU_GET_DATA				读取数据
//                           (Type, Tag1, Tag2, ... 0)
//						 PUSHAPDU_SCRIPT_EXEC			脚本执行
//                           (Type, ScriptLen, Script)
//						 PUSHAPDU_READ_TRANS_REC		读交易日志记录
//                           (Type, Sfi, StartRecNo, MaxRecNo)
//       iParaLen      : 参数长度
//       psPara        : 参数
void vPushApduPrepare(int iPushApduType, ...)
{
#ifndef USE_PUSH_APDU
	return; // 未定义使用优化推送Apdu方式, 认为缓冲区无内容
#else
    va_list args;
	int   iRet;
	int   i1, i2, i3;
	uchar *p;

    va_start(args, iPushApduType);

	sg_iPushApduType = iPushApduType;
    vPushApduClear();
	sg_psApduBuf = &sg_sApduResp[0];
	iRet = 0;

	switch(iPushApduType) {
	case PUSHAPDU_READ_PSE_REC: // 读DDF目录文件
	case PUSHAPDU_READ_TRANS_REC: // 交易日志记录
		i1 = va_arg(args, int); // sfi
		i2 = va_arg(args, int); // start rec no
		if(iPushApduType==PUSHAPDU_READ_TRANS_REC && i2!=1)
			break; // 准备交易日志, 必须从记录1开始
		i3 = va_arg(args, int); // max rec num
		memcpy(sg_sApduCmd, "\x5F\xB2\x00\x00\x03\x00\x00\x00", 8);
		sg_sApduCmd[5] = (uchar)i1;
		sg_sApduCmd[6] = (uchar)i2;
		sg_sApduCmd[7] = (uchar)i3;
		sg_iApduCmdLen = 8;
		iRet = 0; // call func
		break;
	case PUSHAPDU_SELECT_BY_AID_LIST: // 根据AID_LIST选择应用
		memcpy(sg_sApduCmd, "\x5F\xA4\x00\x00\x00", 4);
		sg_iApduCmdLen = 5;
		for(i1=0; i1<gl_iTermSupportedAidNum; i1++) {
			if(sg_iApduCmdLen+gl_aTermSupportedAidList[i1].ucAidLen+2 > 255+5)
				break; // 超界限
			sg_sApduCmd[sg_iApduCmdLen++] = gl_aTermSupportedAidList[i1].ucASI;
			sg_sApduCmd[sg_iApduCmdLen++] = gl_aTermSupportedAidList[i1].ucAidLen;
			memcpy(sg_sApduCmd+sg_iApduCmdLen, gl_aTermSupportedAidList[i1].sAid, gl_aTermSupportedAidList[i1].ucAidLen);
			sg_iApduCmdLen += gl_aTermSupportedAidList[i1].ucAidLen;
		}
		sg_sApduCmd[4] = sg_iApduCmdLen-5;
		iRet = 0; // call func
		break;
	case PUSHAPDU_READ_APP_REC: // 读应用数据记录
		i1 = va_arg(args, int); // afl len
		p = va_arg(args, char*); // afl
		memcpy(sg_sApduCmd, "\x5F\xB2\x01\x00", 4);
		sg_sApduCmd[4] = (uchar)i1;
		memcpy(sg_sApduCmd+5, p, i1);
		sg_iApduCmdLen = 5+i1;
		iRet = 0; // call func
		break;
	case PUSHAPDU_GET_DATA: // 读取数据
		i1 = va_arg(args, int);
		if(i1 == 0) {
			// 一个Tag都没有, 直接返回
			iRet = 0;
			break;
		}
		memcpy(sg_sApduCmd, "\x5F\xCA\x00\x00\x00", 5);
		sg_iApduCmdLen = 5;
		while(i1 != 0) {
			sg_sApduCmd[sg_iApduCmdLen] = (i1>>8)&0xFF;
			sg_sApduCmd[sg_iApduCmdLen+1] = i1&0xFF;
			sg_iApduCmdLen += 2;
			i1 = va_arg(args, int);
		}
		iRet = 0; // call func
		break;
	case PUSHAPDU_SCRIPT_EXEC: // 脚本执行
		i1 = va_arg(args, int); // script len
		p = va_arg(args, char*); // script
		memcpy(sg_sApduCmd, "\x5F\xBA\x00\x00", 4);
		sg_sApduCmd[4] = i1;
		memcpy(sg_sApduCmd+5, p, i1);
		sg_iApduCmdLen = 5+i1;
		iRet = 0; // call func
		break;
	default:
        va_end(args);
        return;
	} // switch(iPushApduType
    va_end(args);
	iRet = iPboc20CustomApdu(sg_sApduCmd, sg_iApduCmdLen, sg_sApduResp, &sg_iApduRespLen);
	if(iRet) {
		// 如果出错, 设置缓冲区, 使下一指令返回错误
		memcpy(sg_sApduResp, "\x00\x01\x00", 3);
		sg_iApduRespLen = 3;
		sg_iApduBufLen = sg_iApduRespLen;
	}
    sg_iApduBufLen = sg_iApduRespLen;        // 当前剩余应答Apdu内容长度
    sg_psApduBuf = sg_sApduResp;         // 当前指令执行到的位置(指向sg_sApduBuf中的某个位置)
#endif
}

// 清除PushApdu缓冲区
void vPushApduClear(void)
{
	sg_iApduBufLen = 0;
    sg_iApduRespLen = 0;
}

// 检查apdu指令连续性
// 如果本次指令不是之前缓冲好的指令, 清除缓冲
static void vCheckCmdContinuity(uchar *psApduCmd)
{
    uint uiNeedClear = 0;
    if(sg_iApduRespLen == 0)
        return; // 无缓冲不需要检查
    switch(sg_iPushApduType) {
	case PUSHAPDU_READ_PSE_REC: // 读DDF目录文件
	case PUSHAPDU_READ_TRANS_REC: // 交易日志记录
        if(memcmp(psApduCmd, "\x00\xB2", 2) != 0)
            uiNeedClear = 1; // 不是读取记录指令
        break;
	case PUSHAPDU_SELECT_BY_AID_LIST: // 根据AID_LIST选择应用
        if(memcmp(psApduCmd, "\x00\xA4", 2) != 0)
            uiNeedClear = 1; // 不是应用选择指令
        break;
	case PUSHAPDU_READ_APP_REC: // 读应用数据记录
        if(memcmp(psApduCmd, "\x00\xB2", 2) != 0)
            uiNeedClear = 1; // 不是读取记录指令
        break;
	case PUSHAPDU_GET_DATA: // 读取数据
        if(memcmp(psApduCmd, "\x80\xCA", 2) != 0)
            uiNeedClear = 1; // 不是读取应用数据指令
        break;
	case PUSHAPDU_SCRIPT_EXEC: // 脚本执行
        // 如果是以下指令, 认为不是脚本
        if(memcmp(psApduCmd, "\x80\xAE"/*gac*/, 2)==0 || memcmp(psApduCmd, "\x80\xCA"/*get data*/, 2)==0)
            uiNeedClear = 1; // 不是脚本
        break;
	default:
        break;
    }
    if(uiNeedClear)
        vPushApduClear();
}

// 根据推送缓冲区虚拟执行IC卡指令
// 输入参数：uiReader : 虚拟卡座号
//           pIn      : IC卡指令结构
//           pOut     : IC卡返回结果
// 返    回：0        : 成功
//           1        : 失败
//           2        : 缓冲区无内容
// Note    : 接口类似uiExchangeApdu()
uint uiPushExchangeApdu(uint uiReader, APDU_IN *pIn, APDU_OUT *pOut)
{
#ifndef USE_PUSH_APDU
	return(2); // 未定义使用优化推送Apdu方式, 认为缓冲区无内容
#else
	int   i;
	int   iRecNo;
	int   iBufLeftLen;
	uchar *p;

	if((int)uiReader != sg_iReaderNo)
		return(2); // 不是设定的读卡器操作, 认为缓冲区无内容

    vCheckCmdContinuity(pIn->sCommand);

	if(sg_iPushApduType==PUSHAPDU_READ_TRANS_REC && sg_iApduRespLen>0) {
		// 上次准备的缓冲区为卡片交易记录, 由于读取卡片交易记录的指令顺序可能随机, 因此特殊处理读卡片交易记录指令
		if(pIn->sCommand[0]==0x00 && pIn->sCommand[1]==0xB2 && (pIn->sCommand[3]>>3)==sg_sApduCmd[5]) {
			// 本次指令为读准备好的卡片交易记录, 从缓冲区中获取
			if(sg_sApduResp[0] == 0x00) {
				// 读记录1指令卡片操作失败, 则读任何记录都认为失败
				return(1);
			}
			memset(pOut, 0, sizeof(APDU_OUT));
			iRecNo = pIn->sCommand[3]>>3;
			if(iRecNo==0 || iRecNo>sg_sApduCmd[7]) {
				// 记录号超界, 认为无此记录
				pOut->uiStatus = 0x6A83; // 无此记录
				return(0);
			}
			p = sg_sApduResp;
			iBufLeftLen = sg_iApduRespLen;
			for(i=1; i<iRecNo; i++) {
				if(iBufLeftLen <= 0) {
					// 缓冲区无内容了, 认为无此记录
					pOut->uiStatus = 0x6A83; // 无此记录
					return(0);
				}
				iBufLeftLen += 2+1+p[2];
				p += 2+1+p[2];
			}
			if(iBufLeftLen <= 0) {
				// 缓冲区无内容了, 认为无此记录
				pOut->uiStatus = 0x6A83; // 无此记录
				return(0);
			}
			// 找到所读记录返回内容
			pOut->uiStatus = (uint)ulStrToLong(p, 2);
			pOut->ucLengthOut = p[2];
			memcpy(pOut->sDataOut, p+3, pOut->ucLengthOut);
			return(0);
		} else {
			// 本次指令不是读取准备好的卡片交易记录, 清除卡片交易记录数据
			sg_iPushApduType = 0;      // 当前缓冲区指令类型
            vPushApduClear();
		}
	}

	if(sg_iApduBufLen == 0)
		return(2); // 缓冲区无内容
	if(sg_iApduBufLen < 3) {
		sg_iApduBufLen = 0;
		return(1); // 缓冲区格式错, 内容长度不足SW[2]+长度[1], 认为读卡失败
	}
	if(2+1+sg_psApduBuf[2] > sg_iApduBufLen) {
		sg_iApduBufLen = 0;
		return(1); // 缓冲区格式错, 内容长度超长, 认为读卡失败
	}
	if(sg_psApduBuf[0] == 0x00) {
		// 卡片操作返回"\x00\x01"(卡通讯错)或"\x00\x02"(缓冲区不足), 都认为是卡片操作错
        vPushApduClear();
		return(1);
	}
	pOut->uiStatus = (uint)ulStrToLong(sg_psApduBuf, 2);
	pOut->ucLengthOut = sg_psApduBuf[2];
	memcpy(pOut->sDataOut, sg_psApduBuf+3, pOut->ucLengthOut);
	sg_iApduBufLen -= 3+sg_psApduBuf[2];
	sg_psApduBuf += 3+sg_psApduBuf[2];
	return(0);
#endif
}

// 根据推送缓冲区虚拟执行IC卡指令
// 输入参数：uiReader : 虚拟卡座号
//           uiInLen    : Command Apdu指令长度
//           psIn       : Command APDU, 标准case1-case4指令结构
//           puiOutLen  : Response APDU长度
// 输出参数：psOut      : Response APDU, RespData[n]+SW[2]
// 返    回：0          : 成功
//           1          : 失败
//           2          : 缓冲区无内容
// Note    : 接口类似_uiDoApdu()
uint uiPushDoApdu(uint uiReader, uint uiInLen, uchar *psIn, uint *puiOutLen, uchar *psOut)
{
#ifndef USE_PUSH_APDU
	return(2); // 未定义使用优化推送Apdu方式, 认为缓冲区无内容
#else
	int   i;
	int   iRecNo;
	int   iBufLeftLen;
	uchar *p;

	if((int)uiReader != sg_iReaderNo)
		return(2); // 不是设定的读卡器操作, 认为缓冲区无内容

    vCheckCmdContinuity(psIn);

	if(sg_iPushApduType==PUSHAPDU_READ_TRANS_REC && sg_iApduRespLen>0) {
		// 上次准备的缓冲区为卡片交易记录, 由于读取卡片交易记录的指令顺序可能随机, 因此特殊处理读卡片交易记录指令
		if(psIn[0]==0x00 && psIn[1]==0xB2 && (psIn[3]>>3)==sg_sApduCmd[5]) {
			// 本次指令为读准备好的卡片交易记录, 从缓冲区中获取
			if(sg_sApduResp[0] == 0x00) {
				// 读记录1指令卡片操作失败, 则读任何记录都认为失败
				return(1);
			}
			iRecNo = psIn[3]>>3;
			if(iRecNo==0 || iRecNo>sg_sApduCmd[7]) {
				// 记录号超界, 认为无此记录
                memcpy(psOut, "\x6A\x83", 2); // 无此记录
                *puiOutLen = 2;
				return(0);
			}
			p = sg_sApduResp;
			iBufLeftLen = sg_iApduRespLen;
			for(i=1; i<iRecNo; i++) {
				if(iBufLeftLen <= 0) {
					// 缓冲区无内容了, 认为无此记录
                    memcpy(psOut, "\x6A\x83", 2); // 无此记录
                    *puiOutLen = 2;
					return(0);
				}
				iBufLeftLen += 2+1+p[2];
				p += 2+1+p[2];
			}
			if(iBufLeftLen <= 0) {
				// 缓冲区无内容了, 认为无此记录
                memcpy(psOut, "\x6A\x83", 2); // 无此记录
                *puiOutLen = 2;
				return(0);
			}
			// 找到所读记录返回内容
            memcpy(psOut, p+3, p[2]);
            memcpy(psOut+p[2], p, 2);
            *puiOutLen = p[2] + 2;
			return(0);
		} else {
			// 本次指令不是读取准备好的卡片交易记录, 清除卡片交易记录数据
			sg_iPushApduType = 0;      // 当前缓冲区指令类型
            vPushApduClear();
		}
	}

	if(sg_iApduBufLen == 0)
		return(2); // 缓冲区无内容
	if(sg_iApduBufLen < 3) {
		sg_iApduBufLen = 0;
		return(1); // 缓冲区格式错, 内容长度不足SW[2]+长度[1], 认为读卡失败
	}
	if(2+1+sg_psApduBuf[2] > sg_iApduBufLen) {
		sg_iApduBufLen = 0;
		return(1); // 缓冲区格式错, 内容长度超长, 认为读卡失败
	}
	if(sg_psApduBuf[0] == 0x00) {
		// 卡片操作返回"\x00\x01"(卡通讯错)或"\x00\x02"(缓冲区不足), 都认为是卡片操作错
        vPushApduClear();
		return(1);
	}
    memcpy(psOut, sg_psApduBuf+3, sg_psApduBuf[2]);
    memcpy(psOut+sg_psApduBuf[2], sg_psApduBuf, 2);
    *puiOutLen = sg_psApduBuf[2] + 2;
	sg_iApduBufLen -= 3+sg_psApduBuf[2];
	sg_psApduBuf += 3+sg_psApduBuf[2];
    return(0);
#endif
}
