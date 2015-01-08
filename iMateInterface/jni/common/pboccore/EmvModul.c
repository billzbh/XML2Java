/**************************************
File name     : EmvModul.c
Function      : Emv核心处理模块
Author        : Yu Jun
First edition : Apr 18th, 2012
Modified      : Apr 1st, 2014
				    增加对ATM的支持
**************************************/
/*
模块详细描述:
	EMV核心处理模块
.	GPO
	如果存在PDOL, 扫描PDOL, 如果其中有金额域, 要求应用层提供金额
	由于可能会多次执行GPO操作, 用变量sg_sGpoTlvBuf[]保存最近一次GPO的Tlv数据, 用于重新选择应用时删除上次GPO数据
	终端是否支持EC, 保存在Tag TAG_DFXX_ECTermSupportIndicator表示的TLV对象中
		如果满足条件(消费,金额小于终端EC限额等), 构造9F7A数据对象指明支持EC
.	读卡片应用数据记录
	根据其中有没有9F74数据对象来判断是否为EC交易
.	脱机数据认证
.	处理限制
.	终端风险管理
.	持卡人验证
.	终端行为分析
.	1st GAC
.	2nd GAC
.	脚本处理
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "VposFace.h"
#include "PushApdu.h"
#include "Arith.h"
#include "Sha.h"
#include "Common.h"
#include "EmvCmd.h"
#include "TlvFunc.h"
#include "EmvFunc.h"
#include "TagAttr.h"
#include "EmvData.h"
#include "EmvCvm.h"
#include "EmvCore.h"
#include "EmvMsg.h"
#include "EmvPara.h"
#include "EmvSele.h"
#include "EmvIo.h"
#include "EmvDAuth.h"
#include "EmvModul.h"
#include "TagDef.h"
#include "EmvTrace.h"
#include "PbocCtls.h"

uchar gl_sEmv4_1[] = "\x7A\x1B\x01\xDF\x94\xB7\xA9\x29\x79\x3E\xE4\x12\x63\x04\xB4\xA6";
static uchar  sg_sGpoTlvBuf[300];        // 最近一次GPO时保存的Tlv数据, 用于重新选择应用时删除上次GPO数据

// GPO初始化, 初始化sg_sGpoTlvBuf
// ret : HXEMV_OK : 成功
int iEmvGpoInit(void)
{
	// 初始化GPO临时Tlv数据库
	iTlvSetBuffer(sg_sGpoTlvBuf, sizeof(sg_sGpoTlvBuf));
	return(HXEMV_OK);
}
// GPO
// ret : HXEMV_OK		: 成功
//       HXEMV_CANCEL   : 用户取消
//       HXEMV_TIMEOUT  : 用户超时
//       HXEMV_CORE     : 其它错误
//       HXEMV_RESELECT : 需要重新选择应用
//       HXEMV_CARD_OP  : 卡操作错
//       HXEMV_CARD_SW  : 非法卡状态字
//       HXEMV_TERMINATE: 满足拒绝条件，交易终止
// Note: 非接卡返回HXEMV_OK时, 有可能交易路径指示交易被拒绝
int iEmvGpo(void)
{
	int   i;
	int   iRet;
	uint  uiRet;
	int   iLen;
	int   iPdolLen; // PDOL长度(<0表示无PDOL)
	uchar szAmount[13];
	uint  uiTransType;
	uchar *p;
	uchar *psPdol;  // PDOL内容
	uchar *psTlvObj;
	uchar *psTlvObjValue;
	uchar sPdolBlock[256]; // pdol数据块
	uchar sDataIn[256];    // apdu指令数据
	uchar ucDataInLen;     // apdu指令数据长度
	uchar sDataOut[256];   // apdu应答数据
	uchar ucDataOutLen;    // apdu应答长度
	uchar ucECSupportIndicator; // EC支持标志, 0:不支持 1:支持
	uchar szECTransLimit[13];   // EC限额
    uchar ucAidLen, *psAid;     // 记录当前选中的AID, 用于GPO失败删除AID并重选用
	uchar ucTransRoute;    // 交易走的路径
	uchar *psAip;

	// 先删除可能的上次GPO数据
	for(i=0; ; i++) {
		iRet = iTlvGetObjByIndex(sg_sGpoTlvBuf, i, &psTlvObj);
		if(iRet < 0)
			break;
		iTlvDelObj(gl_sTlvDbCard, psTlvObj);
	}
	// 重新初始化GPO临时Tlv数据库
	iTlvSetBuffer(sg_sGpoTlvBuf, sizeof(sg_sGpoTlvBuf));

	// 如果之前曾经选过应用并且输入过金额,删除,因为更换了应用后持卡人可能希望提供另外一个金额
	iTlvDelObj(gl_sTlvDbTermVar, TAG_9F02_AmountN);
	iTlvDelObj(gl_sTlvDbTermVar, TAG_81_AmountB);
	iTlvDelObj(gl_sTlvDbTermVar, TAG_DFXX_PdolDataBlock);

	iPdolLen = iTlvGetObjValue(gl_sTlvDbCard, TAG_9F38_PDOL, &psPdol);
	if(iPdolLen < 0) {
		// 无pdol
		memcpy(sDataIn, "\x83\x00", 2);
		ucDataInLen = 2;
	} else {
		// search amount field, 不判断Amount,other
        iRet =  iTlvSearchDOLTag(psPdol, iPdolLen, TAG_9F02_AmountN, NULL); // T9F02, Amount(num)
        if(iRet <= 0) {
            iRet = iTlvSearchDOLTag(psPdol, iPdolLen, TAG_81_AmountB, NULL); // T81, Amount(bin)
        }
		if(iRet > 0) {
			// pdol需要金额域
			iRet = iTlvGetObjValue(gl_sTlvDbTermVar, TAG_DFXX_TransType, &psTlvObjValue);
			ASSERT(iRet == 2);
			if(iRet != 2) {
				EMV_TRACE_ERR_MSG
				return(HXEMV_CORE);
			}
			uiTransType = ulStrToLong(psTlvObjValue, 2);
			if(uiTransType==TRANS_TYPE_AVAILABLE_INQ || uiTransType==TRANS_TYPE_BALANCE_INQ)
				strcpy(szAmount, "0"); // 查询余额不需要输入金额
			else {
				// attended terminal此时需要输入金额, unattended terminal填0即可
				// refer to EMV2008 book4, 6.3.1, P55

				// eCash GPO准备(支持eCash的IC卡必然有PDOL, 且PDOL中至少要求提供金额、货币代码、eCash支持标志)
				// 支持eCash的终端也可能是unattended类型, 如果此时不提供金额, 则unattended类型终端就不能支持eCash了, 因此如果终端支持eCash, GPO前就需要获取到金额

				// 检查终端支不支持EC
				ucECSupportIndicator = 0; // EC支持标志, 0:不支持 1:支持
				iRet = iTlvGetObjValue(gl_sTlvDbTermVar, TAG_DFXX_ECTermSupportIndicator, &p);
				if(iRet == 1) {
					if(*p == 1) {
						if(uiTransType==TRANS_TYPE_SALE || uiTransType==TRANS_TYPE_GOODS || uiTransType==TRANS_TYPE_SERVICES) {
							// 终端支持EC, 并且是消费交易
							ucECSupportIndicator = 1; // EC支持标志, 0:不支持 1:支持
						}
					}
				}

				if(gl_iCardType==EMV_CONTACTLESS_CARD && iRet>0) {
					// 非接卡且需要金额, 取得预处理传入的金额
					strcpy(szAmount, pszPbocCtlsGetAmount());
				} else if(iEmvTermEnvironment()==TERM_ENV_ATTENDED || ucECSupportIndicator==1) {
					// 有人值守(attended) 或是 EC消费
					iRet = iEmvIOGetAmount(EMVIO_AMOUNT, szAmount);
					if(iRet == EMVIO_CANCEL)
						return(HXEMV_CANCEL);
					if(iRet == EMVIO_TIMEOUT)
						return(HXEMV_TIMEOUT);
					ASSERT(iRet == 0);
					if(iRet)
						return(HXEMV_CORE);
				} else {
					// 无人值守(unattended)
					strcpy(szAmount, "0");
				}
			}
			iRet = iEmvSaveAmount(szAmount); // 将金额保存到tlv数据库中
			ASSERT(iRet == 0);
			if(iRet != 0)
				return(HXEMV_CORE);
		}

		// 判断是否设置EC支持位
		if(ucECSupportIndicator == 1) {
			// 终端支持EC, 且为消费交易, 且输入了金额
			// 比较金额与EC限额, 决定是否表明本次交易为EC交易
			iRet = iTlvGetObjValue(gl_sTlvDbTermVar, TAG_9F7B_ECTermTransLimit, &p);
			if(iRet == 6) {
				vOneTwo0(p, 6, szECTransLimit);
			} else {
				// 没找到EC交易限额, 比较Floor Limit
				iRet = iTlvGetObjValue(gl_sTlvDbTermVar, TAG_9F1B_TermFloorLimit, &p);
				if(iRet == 4) {
					sprintf(szECTransLimit, "%lu", ulStrToLong(p, 4));
				} else
					ucECSupportIndicator = 0; // 无EC交易限额且无终端最低限额, 不使用EC(ECash文档7.5)
			}
			if(ucECSupportIndicator == 1) {
				// 存在EC交易限额或终端最低限额
				if(iStrNumCompare(szAmount, szECTransLimit, 10) < 0) {
					// 授权金额小于EC交易限额, 标志为EC交易, 将T9F7A添加到TLV数据库中
					iRet = iTlvMakeAddObj(gl_sTlvDbTermVar, TAG_9F7A_ECTermSupportIndicator, 1, &ucECSupportIndicator, TLV_CONFLICT_REPLACE);
					ASSERT(iRet >= 0);
					if(iRet < 0)
						return(HXEMV_CORE);
					// 设置EC交易指示位
					iRet = iEmvSetECTransFlag(1); // 1 : 是EC交易
					ASSERT(iRet == 0);
					if(iRet)
						return(HXEMV_CORE);
				}
			}
		}

		// search rand field
        iRet =  iTlvSearchDOLTag(psPdol, iPdolLen, TAG_9F37_TermRand, NULL);
        if(iRet > 0) {
			// 随机数
			vGetRand(sDataIn, iRet);
			iTlvMakeAddObj(gl_sTlvDbTermVar, TAG_9F37_TermRand, iRet, sDataIn, TLV_CONFLICT_REPLACE);
        }

        // 记录当前AID, 以备GPO失败后删除
		iRet = iTlvGetObjValue(gl_sTlvDbCard, TAG_4F_AID, &psAid); // 搜索当前选中的AID
		ASSERT(iRet > 0);
		if(iRet <= 0)
			return(HXEMV_CORE); // AID必须存在
        ucAidLen = (uchar)iRet;

	// 构造pdol数据块
		iRet = iTlvMakeDOLBlock(sPdolBlock, sizeof(sPdolBlock), psPdol, iPdolLen, gl_sTlvDbTermFixed, gl_sTlvDbTermVar, gl_sTlvDbCard, NULL);
        if(iRet<0 || iRet>252) {
		    iSelDelCandidate(ucAidLen, psAid);
   			return(HXEMV_RESELECT); // 数据不合法,放弃该应用,重新选择
        }
		i = iTlvMakeAddObj(gl_sTlvDbTermVar, TAG_DFXX_PdolDataBlock, iRet, sPdolBlock, TLV_CONFLICT_REPLACE); // 保存Pdol数据块, 用于可能的CDA
		ASSERT(i >= 0);
		if(i < 0)
			return(HXEMV_CORE);
		iRet = iTlvMakeObject(TAG_83_CmdTemplate, iRet, sPdolBlock, sDataIn, sizeof(sDataIn)); // make T83, sDataIn is T83 object
		ASSERT(i >= 0);
		if(iRet < 0)
			return(HXEMV_CORE);
		ucDataInLen = iRet;
	}

	uiRet = uiEmvCmdGetOptions(ucDataInLen, sDataIn, &ucDataOutLen, sDataOut);
	if(uiRet == 0x6985) {
		if(gl_iCardType == EMV_CONTACTLESS_CARD)
			return(HXEMV_CARD_SW); // 非接卡不判6985, 参考JR/T0025.12—2013 6.5.3, P14
	    iSelDelCandidate(ucAidLen, psAid);
		return(HXEMV_RESELECT); // GPO失败,需要重新选择应用, Refer to Test Case 2CA.030.05
	}
	if(uiRet == 1)
		return(HXEMV_CARD_OP);
	if(uiRet)
		return(HXEMV_CARD_SW);
	// GPO指令执行成功, 保存GPO数据
	if(sDataOut[0] == 0x80) {
		// Tag is 0x80, Format 1
		iRet = iTlvCheckTlvObject(sDataOut);
        if(iRet < 0) {
            return(HXEMV_TERMINATE); // refer to EMVbook3, 7.5 P78
        }
        if(iRet > ucDataOutLen) {
            return(HXEMV_TERMINATE); // refer to EMVbook3, 7.5 P78
        }
		iLen = iTlvValue(sDataOut, &psTlvObjValue);
        if(iLen < 2+4) {
            return(HXEMV_TERMINATE); // refer to EMVbook3, 7.5 P78
        }
        if((iLen-2) % 4) {
            return(HXEMV_TERMINATE); // AFL必须是4的倍数 refer to EMVbook3, 7.5 P78
        }
		iTlvDelObj(gl_sTlvDbCard, TAG_82_AIP); // 如果之前另一应用曾做过GPO, 删除其AIP, 注:函数开始的清除动作只能清除0x77格式的Tlv对象, 0x80格式对象需要手工清除
		iTlvDelObj(gl_sTlvDbCard, TAG_94_AFL); // 如果之前另一应用曾做过GPO, 删除其AFL
		iRet = iTlvMakeAddObj(gl_sTlvDbCard, TAG_82_AIP, 2, psTlvObjValue, TLV_CONFLICT_ERR); // aip
		ASSERT(iRet >= 0);
		if(iRet < 0)
			return(HXEMV_CORE);
		iRet = iTlvMakeAddObj(gl_sTlvDbCard, TAG_94_AFL, iLen-2, psTlvObjValue+2, TLV_CONFLICT_ERR); // afl
		ASSERT(iRet >= 0);
		if(iRet < 0)
			return(HXEMV_CORE);
	} else if(sDataOut[0] == 0x77) {
		// Tag is 0x77, Format 2
		// Search AIP, T82
    	iRet = iTlvSearchObjValue(sDataOut, ucDataOutLen, 0, "\x77""\x82", &psTlvObjValue);
        if(iRet != 2) {
            return(HXEMV_TERMINATE); // refer to EMVbook3, 7.5 P78
        }
		// Search AFL, T94
    	iRet = iTlvSearchObjValue(sDataOut, ucDataOutLen, 0, "\x77""\x94", &psTlvObjValue);
        if(iRet<4 || iRet%4) {
            // 没找到AIP或AIP长度不对
            return(HXEMV_TERMINATE); // refer to EMVbook3, 7.5 P78
        }
		// 将所有TLV对象添加到TLV数据库
		iTlvDelObj(gl_sTlvDbCard, TAG_82_AIP); // 如果之前另一应用曾做过GPO, 删除其AIP, 注:函数开始的清除动作只能清除0x77格式的Tlv对象, 0x80格式对象需要手工清除
		iTlvDelObj(gl_sTlvDbCard, TAG_94_AFL); // 如果之前另一应用曾做过GPO, 删除其AFL
		// 添加到GPO备份数据库, 用于重新GPO时删除本次GPO数据
        iRet = iTlvBatchAddObj(1/*添加所有对象*/, sg_sGpoTlvBuf, sDataOut, ucDataOutLen, TLV_CONFLICT_ERR, 0/*0:长度为0对象不添加*/);
		if(iRet==TLV_ERR_BUF_SPACE || iRet==TLV_ERR_BUF_FORMAT) {
    		ASSERT(0);
			return(HXEMV_CORE);
		}
        iRet = iTlvBatchAddObj(1/*添加所有对象*/, gl_sTlvDbCard, sDataOut, ucDataOutLen, TLV_CONFLICT_ERR, 0/*0:长度为0对象不添加*/);
		if(iRet==TLV_ERR_BUF_SPACE || iRet==TLV_ERR_BUF_FORMAT) {
    		ASSERT(0);
			return(HXEMV_CORE);
		}
        if(iRet < 0) {
   			// 数据非法
            return(HXEMV_TERMINATE); // refer to EMVbook3, 7.5 P78
        }
    } else {
        // GPO返回格式不对
        return(HXEMV_TERMINATE); // refer to EMVbook3, 7.5 P78
    }

	// 判断交易路径 0:接触Pboc(标准DC或EC) 1:非接触Pboc(标准DC或EC) 2:qPboc联机 3:qPboc脱机 4:qPboc拒绝
	ucTransRoute = 0; // 缺省为接触pboc
	if(gl_iCardType == EMV_CONTACTLESS_CARD) {
		// 参考JR/T0025.12—2013 6.5.4, P14
		iRet = iTlvGetObjValue(gl_sTlvDbCard, TAG_82_AIP, &psAip);
		if(iRet != 2)
			return(HXEMV_TERMINATE); // 无AIP, 认为GPO返回格式不对

		if((psPbocCtlsGet9F66()[0]&0x60)==0x40 || (psPbocCtlsGet9F66()[0]&0x60)==0x60 && (psAip[1]&0x80)==0x80) {
			// 仅支持非接pboc 或 支持非接pboc与qPboc并且aip指明走的是pboc
			ucTransRoute = 1; // 走的是非接pboc通道
		} else if((psPbocCtlsGet9F66()[0]&0x60)==0x20 || (psPbocCtlsGet9F66()[0]&0x60)==0x60 && (psAip[1]&0x80)==0x00) {
			// 仅支持qPboc 或 支持非接pboc与qPboc并且aip指明走的是qPboc
			iRet = iTlvGetObjValue(gl_sTlvDbCard, TAG_9F26_AC, &p);
			if(iRet > 0) {
				// 走的qPboc, 判断脱机、联机、或是拒绝
				iRet = iTlvGetObjValue(gl_sTlvDbCard, TAG_9F10_IssuerAppData, &p);
				if(iRet < 5)
					return(HXEMV_TERMINATE); // 只用到5字节
				if((p[4]&0x30) == 0x10)
					ucTransRoute = 3; // TC
				else if((p[4]&0x30) == 0x20)
					ucTransRoute = 2; // ARQC
				else
					ucTransRoute = 4; // AAC (RFU认为是AAC)
			} else {
				// 走的pboc
				if(psPbocCtlsGet9F66()[0]&0x60 == 0x20)
					return(HXEMV_TERMINATE); // 仅支持qPboc不能走pboc
				ucTransRoute = 1; // 走的是非接pboc通道
			}
		}
	}
	iRet = iTlvMakeAddObj(gl_sTlvDbTermVar, TAG_DFXX_TransRoute, 1, &ucTransRoute, TLV_CONFLICT_REPLACE);
	ASSERT(iRet >= 0);
	if(iRet < 0)
		return(HXEMV_CORE);

	return(HXEMV_OK);
}

// 检查AFL入口合法性
// in  : psAFL : AFL入口
// ret : 0     : OK
//       1     : AFL非法
// refer to EMV2008 book3, P78
static int iEmvCheckAFL(uchar *psAFL)
{
	uchar ucSFI;
	ucSFI = psAFL[0] >> 3;
	if(ucSFI==0 || ucSFI==31) {
		EMV_TRACE_ERR_MSG
		return(1); // An SFI of 0 or 31.
	}
	if(psAFL[1] == 0) {
		EMV_TRACE_ERR_MSG
		return(1); // A starting record number of 0.
	}
	if(psAFL[2] < psAFL[1]) {
		EMV_TRACE_ERR_MSG
		return(1); // An ending record number less than the starting record number (byte 3 < byte 2).
	}
	if(psAFL[3] > (psAFL[2]-psAFL[1])+1) {
		EMV_TRACE_ERR_MSG
		return(1); // Number of records participating in offline data authentication than the number of records (byte 4 > byte 3 - byte 2 + 1).
	}
	return(0);
}

// 读应用记录
// ret : HXEMV_OK          : OK
//       HXEMV_CARD_OP     : 卡操作错
//       HXEMV_CARD_SW     : 非法卡指令状态字
//       HXEMV_TERMINATE   : 满足拒绝条件，交易终止
//       HXEMV_CORE        : 其它错误
//		 HXEMV_EXPIRED_APP : 过期卡(非接)
int iEmvReadRecord(void)
{
	int   iRet;
	uint  uiRet;
	int   iAFLLen;
	uchar *psAFL;
	uchar *p;
	uchar ucRecLen;
	uchar sRecData[256];
	uchar ucSFI;
	int   i, j;
	int   iTlvObjLen, iTlvObjValueLen;
	uchar *psTlvObj, *psTlvObjValue;
	uchar szBuf[40];

	iAFLLen = iTlvGetObjValue(gl_sTlvDbCard, TAG_94_AFL, &psAFL);
	ASSERT(iAFLLen >= 0);
	if(iAFLLen < 0)
		return(HXEMV_CORE); // GPO时已确保AFL存在且合法, 不应该出现此情况
	vPushApduPrepare(PUSHAPDU_READ_APP_REC, iAFLLen, psAFL); // add by yujun 2012.10.29, 支持优化pboc2.0推送apdu
	for(i=0, p=psAFL; i<iAFLLen/4; i++, p+=4) {
		iRet = iEmvCheckAFL(p);
		if(iRet) {
			EMV_TRACE_ERR_MSG
			return(HXEMV_TERMINATE); // AFL不合法
		}
        // 读取某AFL入口指定的记录
		for(j=p[1]; j<=p[2]; j++) {
			ucSFI = p[0] >> 3; // j = RecNo
            iRet = uiEmvCmdRdRec(ucSFI, (uchar)j, &ucRecLen, sRecData);
			if(iRet == 1)
				return(HXEMV_CARD_OP); // 卡操作错
			if(iRet)
				return(HXEMV_CARD_SW); // 卡状态字非法
            if(ucSFI>=1 && ucSFI<=10) {
        		iRet = iTlvCheckTlvObject(sRecData);
	    		if(iRet != ucRecLen)
		    		return(HXEMV_TERMINATE);  // 数据不合法
            }
			if(ucSFI>=1 && ucSFI<=10 && sRecData[0]!=0x70)
				return(HXEMV_TERMINATE);  // SFI 1-10, must be T70
			if(j-p[1] < p[3]) {
				// 该记录为脱机数据认证记录, 需要增加到脱机认证数据中
        		iRet = iTlvCheckTlvObject(sRecData);
                if(iRet != ucRecLen) {
					// 脱机数据认证记录必须为T70 Tlv对象, 否则认为SDA失败(refer to EMV2008 book3, 10.3 P96)
					iEmvSDADataErr(); // 通知脱机认证数据记录非法
					// 可以继续后续操作, 但脱机数据认证不会成功了
                }
				if(sRecData[0] != 0x70) {
					// 脱机数据认证记录必须为T70 Tlv对象, 否则认为SDA失败(refer to EMV2008 book3, 10.3 P96)
					iEmvSDADataErr(); // 通知脱机认证数据记录非法
					// 可以继续后续操作, 但脱机数据认证不会成功了
				}
				if(ucSFI>=1 && ucSFI<=10) {
					iTlvObjValueLen = iTlvValue(sRecData, &psTlvObjValue); // 之前已经检查过记录合法性
    				iRet = iEmvSDADataAdd((uchar)iTlvObjValueLen, psTlvObjValue);
				} else {
    				iRet = iEmvSDADataAdd(ucRecLen, sRecData);
				}
				ASSERT(iRet == EMV_DAUTH_OK);
				if(iRet != EMV_DAUTH_OK) {
					EMV_TRACE_ERR_MSG
					return(HXEMV_CORE); // SDA脱机记录缓冲区不足
				}
			}

			if(ucSFI<1 || ucSFI>10)
				continue; // EMV规范之外的SFI不处理, 继续
			if(sRecData[0] != 0x70) {
				EMV_TRACE_ERR_MSG
				return(HXEMV_TERMINATE);
			}
			if(iPbocCtlsGetRoute() == 3) {
				// 交易走的路径, 0:接触Pboc(标准DC或EC) 1:非接触Pboc(标准DC或EC) 2:qPboc联机 3:qPboc脱机 4:qPboc拒绝
				// 非接卡附加处理, refer JR/T0025.12—2013 7.7.20, P40
				// 有效期检查 (暂不判断CA公钥索引不支持及静态认证数据有误等可造成fDDA失败的条件)
				// ?? 启用日期、Pan是否与Track2中Pan匹配 ??
				// ?? Track2中也有有效期, 可不可以GPO后判断失效日期 ??
				iRet = iTlvSearchObjValue(sRecData, ucRecLen, 0, "\x70""\x5F\x24", &psTlvObjValue);
				if(iRet == 3) {
					vOneTwo0(psTlvObjValue, 3, szBuf+2);
					if(memcmp(szBuf+2, "49", 2) <= 0)
						memcpy(szBuf, "20", 2);
					else
						memcpy(szBuf, "19", 2);
					_vGetTime(szBuf+20);
					if(memcmp(szBuf, szBuf+20, 8) < 0) {
						// 过期卡
						iEmvSetItemBit(EMV_ITEM_TVR, TVR_09_EXPIRED, 1); // 置TVR过期标志位
						return(HXEMV_EXPIRED_APP);
					}
				}
			}
			iRet = iTlvBatchAddObj(0/*只添加基本对象*/, gl_sTlvDbCard, sRecData, ucRecLen, TLV_CONFLICT_ERR, 0/*0:长度为0对象不添加*/);
			if(iRet==TLV_ERR_BUF_SPACE || iRet==TLV_ERR_BUF_FORMAT) {
			    ASSERT(0);
				return(HXEMV_CORE);
			}
			if(iRet < 0)
				return(HXEMV_TERMINATE);
		} // for(j
	} // for(i

	// 遍历gl_sTlvDbCard检测数据合法性(refer to EMV2008 book3, 7.5 P77)
	for(i=0; ;) {
		iTlvObjLen = iTlvGetObjByIndex(gl_sTlvDbCard, i, &psTlvObj);
		if(iTlvObjLen < 0)
			break; // 遍历完毕

		// 检查对象来源
		uiRet = uiTagAttrGetFrom(psTlvObj);
		if(uiRet != TAG_FROM_CARD) {
			i ++;
			continue; // 如果该对象不属于卡片, 忽略
		}

		// 对象完整性检查
		iTlvObjLen = iTlvCheckTlvObject(psTlvObj);
		if(iTlvObjLen < 0) {
			// 对象不合法, 但如果为下列对象, 不报错
			if(memcmp(psTlvObj, TAG_5F20_HolderName, strlen(TAG_5F20_HolderName)) == 0 // Cardholder Name
					|| memcmp(psTlvObj, TAG_9F0B_HolderNameExt, strlen(TAG_9F0B_HolderNameExt)) == 0 // Cardholder Name Extended
					|| memcmp(psTlvObj, TAG_5F50_IssuerURL, strlen(TAG_5F50_IssuerURL)) == 0 // Issuer URL
					|| memcmp(psTlvObj, TAG_9F5A_IssuerURL2, strlen(TAG_9F5A_IssuerURL2)) == 0 // Issuer URL2
					|| memcmp(psTlvObj, TAG_9F4D_LogEntry, strlen(TAG_9F4D_LogEntry)) == 0 // Log Entry
					|| memcmp(psTlvObj, TAG_9F4F_LogFmt_I, strlen(TAG_9F4F_LogFmt_I)) == 0) { // Log Format
				iTlvDelObj(gl_sTlvDbCard, psTlvObj); // 忽略此类不合法对象
				// 不要 i++
				continue;
			}

			// 对象不合法, 但如果为下列对象(ECash用,需要GetData才可以取出,如果出现在记录中,忽略), 不报错
            // 做此检查是因为Emv检测案例2CJ012.00, 测试软件使用9F51作为不识别Tag, 实际9F51为ECash所用, 造成冲突
			if(memcmp(psTlvObj, TAG_9F51_AppCurrencyCode_I, strlen(TAG_9F51_AppCurrencyCode_I)) == 0 // Application Currency Code
					|| memcmp(psTlvObj, TAG_9F6D_ECResetThreshold_I, strlen(TAG_9F6D_ECResetThreshold_I)) == 0 // eCash: Reset Threshold
					|| memcmp(psTlvObj, TAG_9F77_ECBalanceLimit_I, strlen(TAG_9F77_ECBalanceLimit_I)) == 0 // EC Balance Limit
					|| memcmp(psTlvObj, TAG_9F79_ECBalance_I, strlen(TAG_9F79_ECBalance_I)) == 0) { // EC Balance
				iTlvDelObj(gl_sTlvDbCard, psTlvObj); // 忽略此类不合法对象
				// 不要 i++
				continue;
			}

			return(HXEMV_TERMINATE); // 重要对象不合法, 终止交易
		}

		// 检查有效期、启用日期合法性
		if(memcmp(psTlvObj, TAG_5F24_AppExpDate, strlen(TAG_5F24_AppExpDate)) == 0 // Application Expiration Date
			    || memcmp(psTlvObj, TAG_5F25_AppEffectiveDate, strlen(TAG_5F25_AppEffectiveDate)) == 0) { // Application Effective Date
            iTlvObjValueLen = iTlvValue(psTlvObj, &psTlvObjValue);
			if(iTlvObjValueLen != 3)
				return(HXEMV_TERMINATE); // 重要对象不合法, 终止交易
			vOneTwo0(psTlvObjValue, 3, sRecData); // 借用sRecData做缓冲区
			if(iTestIfValidDate6(sRecData) != 0)
				return(HXEMV_TERMINATE); // 重要对象不合法, 终止交易
		}
		i ++;
	} // for(i 遍历

	// 检查必需数据项
	if(iPbocCtlsGetRoute()==0 || iPbocCtlsGetRoute()==1) {
		// 交易走的路径, 0:接触Pboc(标准DC或EC) 1:非接触Pboc(标准DC或EC) 2:qPboc联机 3:qPboc脱机 4:qPboc拒绝
		iRet = iTlvGetObj(gl_sTlvDbCard, TAG_5F24_AppExpDate, &psTlvObj); // Application Expiration Date
		if(iRet <= 0)
			return(HXEMV_TERMINATE); // 必须数据不存在, 终止交易
		iRet = iTlvGetObj(gl_sTlvDbCard, TAG_5A_PAN, &psTlvObj); // Application Primary Account Number(PAN)
		if(iRet <= 0)
			return(HXEMV_TERMINATE); // 必须数据不存在, 终止交易
		iRet = iTlvGetObj(gl_sTlvDbCard, TAG_8C_CDOL1, &psTlvObj); // Card Risk Management Data Object List 1(CDOL1)
		if(iRet <= 0)
			return(HXEMV_TERMINATE); // 必须数据不存在, 终止交易
		iRet = iTlvGetObj(gl_sTlvDbCard, TAG_8D_CDOL2, &psTlvObj); // Card Risk Management Data Object List 2(CDOL2)
		if(iRet <= 0)
			return(HXEMV_TERMINATE); // 必须数据不存在, 终止交易
	}

	// EC交易额外处理
	if(iEmvGetECTransFlag() == 1) {
		// 终端尝试做EC交易
		iRet = iTlvGetObjValue(gl_sTlvDbCard, TAG_9F74_ECIssuerAuthCode, &p);
		if(iRet <= 0) {
			// 无EC授权码, 本次交易不是EC交易, 清除EC交易标志
			iRet = iEmvSetECTransFlag(0);
			ASSERT(iRet == 0);
			if(iRet)
				return(HXEMV_CORE);
		} else {
			// 存在EC授权码, 本次交易是EC交易
			if(iRet != 6)
				return(HXEMV_TERMINATE); // EC授权码不存在, 终止交易
			// 读取EC余额
			iTlvObjLen = iTlvObjValueLen = 100;
			iRet = iHxGetCardNativeData(TAG_9F79_ECBalance_I, &iTlvObjLen, sRecData, &iTlvObjValueLen, sRecData+100); // 借用sRecData变量保存余额对象
			if(iRet==HXEMV_CARD_OP || iRet==HXEMV_CARD_SW || iRet==HXEMV_FLOW_ERROR || iRet==HXEMV_CORE)
				return(iRet);
			if(iRet!=HXEMV_OK || iTlvObjValueLen!=12)
				return(HXEMV_TERMINATE); // 数据非法, 终止交易
			// 将此余额作为交易前余额保存到Tlv数据库
			iRet = iTlvAddObj(gl_sTlvDbCard, sRecData, TLV_CONFLICT_REPLACE);
			if(iRet < 0)
				return(HXEMV_CORE);

			// 读取EC重置阀值
			iTlvObjLen = iTlvObjValueLen = 100;
			iRet = iHxGetCardNativeData(TAG_9F6D_ECResetThreshold_I, &iTlvObjLen, sRecData, &iTlvObjValueLen, sRecData+100); // 借用sRecData变量保存余额阀值对象
			if(iRet==HXEMV_CARD_OP || iRet==HXEMV_CARD_SW || iRet==HXEMV_FLOW_ERROR || iRet==HXEMV_CORE)
				return(iRet);
			if(iRet!=HXEMV_OK || iTlvObjValueLen!=12)
				return(HXEMV_TERMINATE); // 数据非法, 终止交易

#if 0
// Pboc3.0 Book13 7.4.2, 没要求读EC余额上限
			// 读取EC余额上限
			iTlvObjLen = iTlvObjValueLen = 100;
			iRet = iHxGetCardNativeData(TAG_9F77_ECBalanceLimit_I, &iTlvObjLen, sRecData, &iTlvObjValueLen, sRecData+100); // 借用sRecData变量保存余额上限对象
			if(iRet==HXEMV_CARD_OP || iRet==HXEMV_CARD_SW || iRet==HXEMV_FLOW_ERROR || iRet==HXEMV_CORE)
				return(iRet);
			if(iRet!=HXEMV_OK || iTlvObjValueLen!=12)
				return(HXEMV_TERMINATE); // 数据非法, 终止交易
#endif
		}
	}

	return(HXEMV_OK);
}

// 终端行为分析
// in  : iFlag : 0 : 完整终端行为分析(包括标准DC与EC)
//               1 : 只判断IAC/TAC default, 即需联机但不能联机时是否拒绝交易
//               2 : 仅检查拒绝位, 多次调用, 用于尽早拒绝交易以阻止不必要的输入
//               3 : EC专用, 完整终端行为分析, 判断是否需要输入EC需要的联机密码
// ret : 0 : 出错
//       1 : 拒绝
//       2 : 联机
//       3 : 脱机
//       4 : 需要输入EC需要的联机密码, 只有iFlag=3时才可能返回
//       EC输入联机密码返回值
//       -1: 用户取消
//       -2: 超时
static int iTermActionAnalysis(int iFlag)
{
	int    iRet;
	uchar  *p;
	uchar  sIACDenial[5], sIACOnline[5], sIACDefault[5];
	uchar  sTACDenial[5], sTACOnline[5], sTACDefault[5];
	uchar  sTVR[5];
	int    iTermAction; // 1:拒绝 2:联机 3:脱机
	uchar  szECBalance[13], szECResetThreshold[13], szAmount[13], szTmp[13];

	// 获取IACs
	iRet = iTlvGetObjValue(gl_sTlvDbCard, TAG_9F0E_IACDenial, &p);
	if(iRet == 5)
		memcpy(sIACDenial, p, 5);
	else
		memset(sIACDenial, 0, 5);
	iRet = iTlvGetObjValue(gl_sTlvDbCard, TAG_9F0F_IACOnline, &p);
	if(iRet == 5)
		memcpy(sIACOnline, p, 5);
	else
		memset(sIACOnline, 0xFF, 5);
	iRet = iTlvGetObjValue(gl_sTlvDbCard, TAG_9F0D_IACDefault, &p);
	if(iRet == 5)
		memcpy(sIACDefault, p, 5);
	else
		memset(sIACDefault, 0xFF, 5);
	// 获取TACs
	iRet = iTlvGetObjValue(gl_sTlvDbTermVar, TAG_DFXX_TACDenial, &p);
	if(iRet == 5)
		memcpy(sTACDenial, p, 5);
	else
		memset(sTACDenial, 0, 5);
	iRet = iTlvGetObjValue(gl_sTlvDbTermVar, TAG_DFXX_TACOnline, &p);
	if(iRet == 5)
		memcpy(sTACOnline, p, 5);
	else
		memset(sTACOnline, 0, 5);
	iRet = iTlvGetObjValue(gl_sTlvDbTermVar, TAG_DFXX_TACDefault, &p);
	if(iRet == 5)
		memcpy(sTACDefault, p, 5);
	else
		memset(sTACDefault, 0, 5);
	// 获取TVR
	iRet = iTlvGetObjValue(gl_sTlvDbTermVar, TAG_95_TVR, &p);
	ASSERT(iRet == 5);
	if(iRet == 5)
		memcpy(sTVR, p, 5);
	else
		return(0); // TVR长度必须等于5

	// 联合TACs
	vOr(sIACDenial, sTACDenial, 5);
	vOr(sIACOnline, sTACOnline, 5);
	vOr(sIACDefault, sTACDefault, 5);

	// 联合TVR
	vAnd(sIACDenial, sTVR, 5);
	vAnd(sIACOnline, sTVR, 5);
	vAnd(sIACDefault, sTVR, 5);

	if(iFlag == 1) {
		// 判断终端缺省行为
		if(iEmvTermCommunication() == TERM_COM_ONLINE_ONLY) {
			iTermAction = 1; // 仅联机终端缺省行为为拒绝
			goto label_ec_analysis;
		}
		if(iTestStrZero(sIACDefault, 5) == 0)
			iTermAction = 3; // 缺省行为为完成
		else
			iTermAction = 1; // 缺省行为为拒绝
		goto label_ec_analysis;
	}

	// 终端行为分析
	if(iTestStrZero(sIACDenial, 5) != 0) {
		iTermAction = 1; // 拒绝
		goto label_ec_analysis;
	}
	if(iEmvTermCommunication() == TERM_COM_ONLINE_ONLY) {
		// 仅联机终端如果没有决定拒绝交易, 就需要进行联机尝试
		iTermAction = 2; // online
		goto label_ec_analysis;
	}
	if(iEmvTermCommunication() != TERM_COM_OFFLINE_ONLY) {
		// 有联机能力的终端需要判断联机屏蔽位
		if(iTestStrZero(sIACOnline, 5) != 0)
			iTermAction = 2; // online
		else
			iTermAction = 3; // 脱机完成
		goto label_ec_analysis;
	}
	if(iEmvTermCommunication() == TERM_COM_OFFLINE_ONLY) {
		// 纯脱机终端需要判断缺省屏蔽位已决定是否脱机完成交易
		if(iTestStrZero(sIACDefault, 5) == 0) {
			iTermAction = 3; // 脱机完成
			goto label_ec_analysis;
		}
	}
	iTermAction = 1; // 拒绝

label_ec_analysis:
	// EC额外分析
	if(iEmvGetECTransFlag() != 1) // EC交易标志 0:非EC交易 1:EC交易 2:GPO卡片返回EC授权码,但未走EC通道
		return(iTermAction); // 非EC交易, 直接返回之前确定的终端行为

	// EC交易	iTermAction== 1:拒绝 2:联机 3:脱机
	if(iTermAction == 1) {
		return(iTermAction); // 拒绝交易直接返回
	}
	if(iTermAction == 2) {
	    // 联机交易不走EC通道
		iRet = iEmvSetECTransFlag(2); // 2:GPO卡片返回EC授权码,但未走EC通道
		return(iTermAction);
	}

	// 脱机交易, 依据ECash规范JR/T 0025.13—2010 7.4.4
    // 取授权金额
	iRet = iTlvGetObjValue(gl_sTlvDbTermVar, TAG_9F02_AmountN, &p);
	ASSERT(iRet == 6);
	if(iRet != 6)
		return(0); // 出错
	vOneTwo0(p, 6, szAmount);
	// 取EC余额
	iRet = iTlvGetObjValue(gl_sTlvDbCard, TAG_9F79_ECBalance_I, &p);
	ASSERT(iRet == 6);
	if(iRet != 6)
		return(0); // 出错
	vOneTwo0(p, 6, szECBalance);
	// 取EC重置阀值
	iRet = iTlvGetObjValue(gl_sTlvDbCard, TAG_9F6D_ECResetThreshold_I, &p);
	ASSERT(iRet == 6);
	if(iRet != 6)
		return(0); // 出错
	vOneTwo0(p, 6, szECResetThreshold);
    
	memset(szTmp, 0, sizeof(szTmp)); // szTmp准备用来存储ECBalance-Amount
	iRet = iStrNumSub(szTmp, 12, szECBalance, szAmount, 10, 0);
	ASSERT(iRet >= 0);
	if(iRet < 0)
		return(0); // 数字非法
	if(iRet) {
		// 有借位, ECBalance不可能小于Amount, 如果如此, IC卡不会选择EC通道, 如果此事发生了, 联机
		iRet = iEmvSetECTransFlag(2); // 2:GPO卡片返回EC授权码,但未走EC通道
		return(2);
	}
	if(iStrNumCompare(szTmp, szECResetThreshold, 10) >= 0) {
		// 电子现金余额减去授权金额大于或等于电子现金重置阈值
		return(3); // 请求脱机批准
	}

	// 电子现金余额减去授权金额小于电子现金重置阈值
	vTraceWriteTxtCr(TRACE_ALWAYS, "电子现金余额减去授权金额小于电子现金重置阈值");

	if(iEmvTermCommunication() == TERM_COM_OFFLINE_ONLY) {
		// 终端不具备联机能力
		return(3); // 请求脱机批准
	}

	if(iFlag == 2) {
		// 拒绝位检查, 此种检查应避免过早进行交互, 因此不尝试输入密码
		return(iTermAction);
	}

	// 终端具备联机能力, 请求联机，并要求持卡人输入联机PIN
	iRet = iHxGetData(TAG_DFXX_EncOnlinePIN, NULL, NULL, NULL, NULL);
	if(iRet == HXEMV_OK) {
		// 联机密码已经输入, 请求联机
		iRet = iEmvSetECTransFlag(2); // 2:GPO卡片返回EC授权码,但未走EC通道
		return(2);
	}
	// 之前没有输入联机密码, 在此处要求输入联机密码
	// (具体要求规范未作说明)
	//     之前是否提示过输入联机密码(bypass)?
	//     终端是否支持密码输入?
	// 我如此处理:如果之前要求输入联机密码但bypass了,脱机完成
	//            此时输入联机密码但bypass,则脱机完成(如果IAC/TAC要求bypass密码需要联机, 则进行联机)
	//            如果没有密码设备或密码设备不能工作, 脱机完成
	//            不修正CVMResult(确保EMV标准流程CVMResult完整性)
	//            如果输入了联机PIN, 设置TVR联机PIN输入位
	if(iEmvTestItemBit(EMV_ITEM_TERM_CAP, TERM_CAP_09_ENC_ONLINE_PIN) == 0) {
		return(3); // 不支持联机密码输入, 脱机完成
	}

	if(iFlag == 3)
		return(4); // 4:需要输入EC需要的联机密码
	return(3); // 脱机完成
}

// SDA或DDA
// in  : iInCrlFlag          : 发卡行证书在CRL列表之中, 1:发卡行证书在CRL列表 0:发卡行证书是否在CRL列表中未知
//                             此参数专门为非回调方式使用, 传入1时设置脱机数据认证失败位
// ret : HXEMV_OK            : OK
//       HXEMV_CARD_OP       : 卡操作错
//       HXEMV_CARD_SW       : 非法卡状态字
//       HXEMV_TERMINATE     : 满足拒绝条件，交易终止
//       HXEMV_DENIAL        : 终端行为分析结果为脱机拒绝
//       HXEMV_DENIAL_ADVICE : 终端行为分析结果为脱机拒绝, 有Advice
//       HXEMV_FLOW_ERROR    : EMV流程错误
//       HXEMV_CORE          : 内部错误
//		 HXEMV_FDDA_FAIL	 : fDDA失败(非接)
int iEmvOfflineDataAuth(int iInCrlFlag)
{
	int iRet;

	iRet = iPbocCtlsGetRoute(); // 获取交易路径
	if(iRet == 3) {
		// iRet==3 : qPboc脱机, 只有qPboc脱机交易才需要执行fDDA		
		if(iInCrlFlag == 0)
			iRet = iEmvGetIcPubKey(); // 获取IC卡公钥
		else
			iRet = HXEMV_FDDA_FAIL; // 设置了CRL标志, 直接认为脱机数据认证失败
		if(iRet == EMV_DAUTH_NATIVE)
			return(HXEMV_CORE);
		if(iRet == EMV_DAUTH_DATA_ILLEGAL)
			return(HXEMV_TERMINATE);
		if(iRet == EMV_DAUTH_DATA_MISSING)
			iEmvSetItemBit(EMV_ITEM_TVR, TVR_02_ICC_DATA_MISSING, 1); // 如果数据缺失,设置TVR数据缺失位
		if(iRet != EMV_DAUTH_OK) {
			// DDA失败, TSI置位脱机认证执行位, TVR置位DDA失败位
			iEmvSetItemBit(EMV_ITEM_TSI, TSI_00_DATA_AUTH_PERFORMED, 1);
			iEmvSetItemBit(EMV_ITEM_TVR, TVR_04_DDA_FAILED, 1);
			return(HXEMV_FDDA_FAIL); // FDDA失败
		}
		// 获取IC卡公钥成功, 执行fDDA验证
		iEmvSetItemBit(EMV_ITEM_TSI, TSI_00_DATA_AUTH_PERFORMED, 1); // TSI置位脱机认证执行位
		iRet = iEmvFdda();
		if(iRet == EMV_DAUTH_NATIVE)
			return(HXEMV_CORE);
		if(iRet == EMV_DAUTH_DATA_MISSING)
			iEmvSetItemBit(EMV_ITEM_TVR, TVR_02_ICC_DATA_MISSING, 1); // 如果数据缺失,设置TVR数据缺失位
		if(iRet!=EMV_DAUTH_OK || iInCrlFlag==1) {
			// fDDA失败, TVR置位DDA失败位
			iEmvSetItemBit(EMV_ITEM_TVR, TVR_04_DDA_FAILED, 1);
			return(HXEMV_FDDA_FAIL);
		}
		return(HXEMV_OK);
	} else if(iRet==2 || iRet==4) {
		// iRet==2:qPboc联机 iRet==4:qPboc拒绝, qPboc联机与qPboc拒绝 都不需要进行脱机数据认证
		return(HXEMV_FLOW_ERROR);
	}

// 如果终端与IC卡都支持CDA, 执行CDA认证
	if(iEmvTestItemBit(EMV_ITEM_AIP, AIP_07_CDA_SUPPORTED)
			&& iEmvTestItemBit(EMV_ITEM_TERM_CAP, TERM_CAP_20_CDA)) {
		if(iInCrlFlag == 0)
			iRet = iEmvGetIcPubKey(); // 获取IC卡公钥
		else
			iRet = EMV_DAUTH_FAIL; // 设置了CRL标志, 直接认为脱机数据认证失败
		if(iRet == EMV_DAUTH_NATIVE)
			return(HXEMV_CORE);
		if(iRet == EMV_DAUTH_DATA_ILLEGAL)
			return(HXEMV_TERMINATE);
		if(iRet == EMV_DAUTH_DATA_MISSING)
			iEmvSetItemBit(EMV_ITEM_TVR, TVR_02_ICC_DATA_MISSING, 1); // 如果数据缺失,设置TVR数据缺失位
		if(iRet != EMV_DAUTH_OK) {
			// CDA失败, TSI置位脱机认证执行位, TVR置位CDA失败位
			iEmvSetItemBit(EMV_ITEM_TSI, TSI_00_DATA_AUTH_PERFORMED, 1); // 该位之前可能已经置位
			iEmvSetItemBit(EMV_ITEM_TVR, TVR_05_CDA_FAILED, 1);
		}
		// CDA即使早期失败, 也需要继续执行交易, refer to EMV2008 book4, 6.3.2 P44
		// 注意在GAC时需要再次判断CDA是否早期失败过, 以便按规范执行相应操作
		goto _label_dauth_finished;
	}
	
// 如果终端与IC卡都支持DDA, 执行DDA认证
	if(iEmvTestItemBit(EMV_ITEM_AIP, AIP_02_DDA_SUPPORTED)
			&& iEmvTestItemBit(EMV_ITEM_TERM_CAP, TERM_CAP_17_DDA)) {
		if(iInCrlFlag == 0)
			iRet = iEmvGetIcPubKey(); // 获取IC卡公钥
		else
			iRet = EMV_DAUTH_FAIL; // 设置了CRL标志, 直接认为脱机数据认证失败
		if(iRet == EMV_DAUTH_NATIVE)
			return(HXEMV_CORE);
		if(iRet == EMV_DAUTH_DATA_ILLEGAL)
			return(HXEMV_TERMINATE);
		if(iRet == EMV_DAUTH_DATA_MISSING)
			iEmvSetItemBit(EMV_ITEM_TVR, TVR_02_ICC_DATA_MISSING, 1); // 如果数据缺失,设置TVR数据缺失位
		if(iRet != EMV_DAUTH_OK) {
			// DDA失败, TSI置位脱机认证执行位, TVR置位DDA失败位
			iEmvSetItemBit(EMV_ITEM_TSI, TSI_00_DATA_AUTH_PERFORMED, 1);
			iEmvSetItemBit(EMV_ITEM_TVR, TVR_04_DDA_FAILED, 1);
			goto _label_dauth_finished; // DDA失败
		}
		// 获取IC卡公钥成功, 执行DDA验证
		iRet = iEmvDda();
		if(iRet == EMV_DAUTH_NATIVE)
			return(HXEMV_CORE);
		if(iRet == EMV_DAUTH_CARD_IO)
			return(HXEMV_CARD_OP);
		if(iRet == EMV_DAUTH_CARD_SW)
			return(HXEMV_CARD_SW);
		if(iRet == EMV_DAUTH_DATA_ILLEGAL)
			return(HXEMV_TERMINATE);

		// TSI置位脱机认证执行位
		iEmvSetItemBit(EMV_ITEM_TSI, TSI_00_DATA_AUTH_PERFORMED, 1);

		if(iRet == EMV_DAUTH_DATA_MISSING)
			iEmvSetItemBit(EMV_ITEM_TVR, TVR_02_ICC_DATA_MISSING, 1); // 如果数据缺失,设置TVR数据缺失位
		if(iRet != EMV_DAUTH_OK) {
			// DDA失败, TVR置位DDA失败位
			iEmvSetItemBit(EMV_ITEM_TVR, TVR_04_DDA_FAILED, 1);
			goto _label_dauth_finished; // DDA失败, 返回
		}
		// DDA认证成功
		goto _label_dauth_finished;
	}

// 如果终端与IC卡都支持SDA, 执行SDA认证
	if(iEmvTestItemBit(EMV_ITEM_AIP, AIP_01_SDA_SUPPORTED)
			&& iEmvTestItemBit(EMV_ITEM_TERM_CAP, TERM_CAP_16_SDA)) {
		if(iInCrlFlag == 0)
			iRet = iEmvGetIssuerPubKey(); // 获取发卡行公钥
		else
			iRet = EMV_DAUTH_FAIL; // 设置了CRL标志, 直接认为脱机数据认证失败
		if(iRet == EMV_DAUTH_NATIVE)
			return(HXEMV_CORE);
		if(iRet == EMV_DAUTH_DATA_ILLEGAL)
			return(HXEMV_TERMINATE);
		if(iRet == EMV_DAUTH_DATA_MISSING)
			iEmvSetItemBit(EMV_ITEM_TVR, TVR_02_ICC_DATA_MISSING, 1); // 如果数据缺失,设置TVR数据缺失位
		if(iRet != EMV_DAUTH_OK) {
			// SDA失败, TSI置位脱机认证执行位, TVR置位SDA失败位
			iEmvSetItemBit(EMV_ITEM_TSI, TSI_00_DATA_AUTH_PERFORMED, 1);
			iEmvSetItemBit(EMV_ITEM_TVR, TVR_01_SDA_FAILED, 1);
			goto _label_dauth_finished; // SDA失败
		}
		// 获取发卡行公钥成功, 执行SDA验证
		iRet = iEmvSda();
		if(iRet == EMV_DAUTH_NATIVE)
			return(HXEMV_CORE);

		// TSI置位脱机认证执行位
		iEmvSetItemBit(EMV_ITEM_TSI, TSI_00_DATA_AUTH_PERFORMED, 1);

		if(iRet == EMV_DAUTH_DATA_MISSING)
			iEmvSetItemBit(EMV_ITEM_TVR, TVR_02_ICC_DATA_MISSING, 1); // 如果数据缺失,设置TVR数据缺失位
		if(iRet != EMV_DAUTH_OK) {
			// SDA失败, TVR置位SDA失败位
			iEmvSetItemBit(EMV_ITEM_TVR, TVR_01_SDA_FAILED, 1);
			goto _label_dauth_finished; // SDA失败, 返回
		}
		// SDA认证成功
		goto _label_dauth_finished;
	}

// 终端不支持脱机数据认证
	// TVR置位脱机数据认证没有执行位
	iEmvSetItemBit(EMV_ITEM_TVR, TVR_00_DATA_AUTH_NOT_PERFORMED, 1);

_label_dauth_finished:
	// 脱机数据认证完成
#if 0
// wlfxy
// marked by yujun 20121212, for Test case J2CT0380000v4.1a_6 等
//                           此类案例要求在卡片过期的情况下继续进行频度分析, 如果过早进行拒绝位检查,
//                           在IAC-denial为全FF情况下将不会执行频度分析
//                           为通过此类案例, 不执行EMV规范第3册规定的可以多次执行终端行为分析的选项
//                           另外, 如果EMV官方或BCTC告知此类案例被修正(个人认为应该被修正), 则可以开放此类检查
	iRet = iEmvIfDenial();
	if(iRet != HXEMV_OK)
		return(iRet);
#endif
	return(HXEMV_OK);
}

// 处理限制
// ret : HXEMV_OK            : OK
//       HXEMV_CORE          : 其它错误
//       HXEMV_DENIAL        : 终端行为分析结果为脱机拒绝
//       HXEMV_DENIAL_ADVICE : 终端行为分析结果为脱机拒绝, 有Advice
//       HXEMV_CARD_OP       : 终端行为分析结果为脱机拒绝, 申请AAC时卡片出错
//       HXEMV_TERMINATE     : 满足拒绝条件，交易终止
int iEmvProcRistrictions(void)
{
	int   iRet;
	uchar *psCardVer, *psTermVer;
	uchar *psIssuerCountryCode, *psTermCountryCode;
	uint  uiTransType;
	uchar *psTransDate, *psEffectiveDate, *psExpirationDate;
	uchar *p;
	int   iInternationalFlag; // 1:国外 0:国内

	// application version number
	iRet = iTlvGetObjValue(gl_sTlvDbCard, TAG_9F08_AppVerCard, &psCardVer); // Application Version Number, Card
	if(iRet > 0) {
		// 卡片存在应用版本号
		iRet = iTlvGetObjValue(gl_sTlvDbTermVar, TAG_9F09_AppVerTerm, &psTermVer); // Application Version Number, Terminal
		ASSERT(iRet == 2);
		if(iRet != 2)
			return(HXEMV_CORE); // 终端必须存在此项
		if(memcmp(psCardVer, psTermVer, 2) != 0) {
			// 卡片与终端应用版本号不符, TVR置位相应标识位
			iEmvSetItemBit(EMV_ITEM_TVR, TVR_08_DIFFERENT_VER, 1);
		}
	}

	// application usage control
	iRet = iTlvGetObjValue(gl_sTlvDbCard, TAG_9F07_AUC, &p); // AUC
	if(iRet == 2) {
		// AUC存在
		// 20140401, 增加对ATM的支持
		if(iEmvTestIfIsAtm()) {
			// 是ATM
			if(!iEmvTestItemBit(EMV_ITEM_AUC, AUC_06_ATMS))
				iEmvSetItemBit(EMV_ITEM_TVR, TVR_11_SERVICE_NOT_ALLOWED, 1); // 卡片不允许在非ATM上交易, 置TVR服务不允许位
		} else {
			// 不是ATM
			if(!iEmvTestItemBit(EMV_ITEM_AUC, AUC_07_NO_ATMS))
				iEmvSetItemBit(EMV_ITEM_TVR, TVR_11_SERVICE_NOT_ALLOWED, 1); // 卡片不允许在非ATM上交易, 置TVR服务不允许位
		}
		iRet = iTlvGetObjValue(gl_sTlvDbTermFixed, TAG_9F1A_TermCountryCode, &psTermCountryCode); // terminal country code
		ASSERT(iRet == 2);
		if(iRet != 2)
			return(HXEMV_CORE); // 终端国家代码必须存在
		iRet = iTlvGetObjValue(gl_sTlvDbCard, TAG_5F28_IssuerCountryCode, &psIssuerCountryCode); // issuer country code
		if(iRet == 2) {
			// 发卡行国家代码存在
			if(memcmp(psIssuerCountryCode, psTermCountryCode, 2) == 0)
				iInternationalFlag = 0; // 国内卡
			else
				iInternationalFlag = 1; // 国外卡
			iRet = iTlvGetObjValue(gl_sTlvDbTermVar, TAG_DFXX_TransType, &p); // trans type
			if(iRet != 2)
				return(HXEMV_CORE); // 交易类型必须存在
			uiTransType = ulStrToLong(p, 2);
			switch(uiTransType) {
			case TRANS_TYPE_CASH:
				if(iInternationalFlag == 0)
					iRet = iEmvTestItemBit(EMV_ITEM_AUC, AUC_00_DOMESTIC_CASH);
				else
					iRet = iEmvTestItemBit(EMV_ITEM_AUC, AUC_01_INTERNATIONAL_CASH);
				if(iRet == 0)
					iEmvSetItemBit(EMV_ITEM_TVR, TVR_11_SERVICE_NOT_ALLOWED, 1); // 卡片不允许, 置TVR服务不允许位
				break;
			case TRANS_TYPE_SALE:
			case TRANS_TYPE_GOODS:
			case TRANS_TYPE_SERVICES:
				// refer emv bulletin AN27, 只要是purchase交易不管Goods或Services, AUC只要支持Goods或Services任意一种, 都认为支持
				if(iInternationalFlag == 0) {
					if(iEmvTestItemBit(EMV_ITEM_AUC, AUC_02_DOMESTIC_GOODS)==0
						    && iEmvTestItemBit(EMV_ITEM_AUC, AUC_04_DOMESTIC_SERVICES)==0)
						iEmvSetItemBit(EMV_ITEM_TVR, TVR_11_SERVICE_NOT_ALLOWED, 1); // 卡片不允许, 置TVR服务不允许位
				} else {
					if(iEmvTestItemBit(EMV_ITEM_AUC, AUC_03_INTERNATIONAL_GOODS)==0
						    && iEmvTestItemBit(EMV_ITEM_AUC, AUC_05_INTERNATIONAL_SERVICES)==0)
						iEmvSetItemBit(EMV_ITEM_TVR, TVR_11_SERVICE_NOT_ALLOWED, 1); // 卡片不允许, 置TVR服务不允许位
				}
				break;
			default:
				break; // 其它交易由于AUC无限制位, 认为许可
			}
		} // 发卡行国家代码存在
	} // AUC存在

	// 有效/启用日期检查
	iRet = iTlvGetObjValue(gl_sTlvDbTermVar, TAG_9A_TransDate, &psTransDate); // 交易日期
	ASSERT(iRet == 3);
	if(iRet != 3)
		return(HXEMV_CORE); // 交易日期必须存在
	iRet = iTlvGetObjValue(gl_sTlvDbCard, TAG_5F24_AppExpDate, &psExpirationDate); // 失效日期
	ASSERT(iRet == 3);
	if(iRet != 3)
		return(HXEMV_CORE); // 失效日期必须存在
	if(iCompDate3(psTransDate, psExpirationDate) > 0)
		iEmvSetItemBit(EMV_ITEM_TVR, TVR_09_EXPIRED, 1); // 置TVR过期标志位
	iRet = iTlvGetObjValue(gl_sTlvDbCard, TAG_5F25_AppEffectiveDate, &psEffectiveDate); // 起效日期
	if(iRet == 3) {
		// 起效日期存在且合法
		if(iCompDate3(psTransDate, psEffectiveDate) < 0)
			iEmvSetItemBit(EMV_ITEM_TVR, TVR_10_NOT_EFFECTIVE, 1); // 置TVR未到启用日期标志位
	}
#if 0
// wlfxy, 如果不做检查, 则不会返回卡片操作相关的返回值
// marked by yujun 20121212, for Test case J2CT0380000v4.1a_6 等
//                           此类案例要求在卡片过期的情况下继续进行频度分析, 如果过早进行拒绝位检查,
//                           在IAC-denial为全FF情况下将不会执行频度分析
//                           为通过此类案例, 不执行EMV规范第3册规定的可以多次执行终端行为分析的选项
//                           另外, 如果EMV官方或BCTC告知此类案例被修正(个人认为应该被修正), 则可以开放此类检查
	iRet = iEmvIfDenial();
	if(iRet != HXEMV_OK)
		return(iRet);
#endif
	return(HXEMV_OK);
}

// 最低限额检查
// ret : HXEMV_OK          : OK
//       HXEMV_CORE        : 其它错误
static int iEmvFloorLimitChecking(void)
{
	int iRet;
	uchar *psFloorLimit, szFloorLimit[13];
	uchar *psAmount, szAmount[20];
	uchar szHistoryAmount[13];
	uchar *p;

	if(iEmvGetECTransFlag() == 1)
		return(HXEMV_OK); // EC交易不做最低限额检查

	// 取最低限额
	iRet = iTlvGetObjValue(gl_sTlvDbTermVar, TAG_9F1B_TermFloorLimit, &psFloorLimit);
	ASSERT(iRet == 4);
	if(iRet != 4)
		return(HXEMV_CORE); // floor limit必须存在
	sprintf(szFloorLimit, "%012lu", ulStrToLong(psFloorLimit, 4));
	// 取授权金额
	iRet = iTlvGetObjValue(gl_sTlvDbTermVar, TAG_9F02_AmountN, &psAmount); // Amount, Authorised(Numeric)
	ASSERT(iRet == 6);
	if(iRet != 6)
		return(HXEMV_CORE); // amount, authorised必须存在
	vOneTwo0(psAmount, 6, szAmount);

	if(iStrNumCompare(szAmount, szFloorLimit, 10) >= 0) {
		// 超出最低限额, 置位TVR相应指示位
		iEmvSetItemBit(EMV_ITEM_TVR, TVR_24_EXCEEDS_FLOOR_LIMIT, 1);
		return(HXEMV_OK);
	}

	// 分开消费检查
	iRet = iTlvGetObjValue(gl_sTlvDbTermFixed, TAG_DFXX_SeparateSaleSupport, &p); // 获取分开消费检查标志
	if(iRet==1 && *p) {
		// 支持分开消费, 累计金额
		iRet = iEmvIOSeparateSaleCheck(szHistoryAmount);
		ASSERT(iRet == EMVIO_OK);
		if(iRet != EMVIO_OK)
			return(HXEMV_CORE);
		
		iStrNumAdd(szAmount, sizeof(szAmount)-1, szAmount, szHistoryAmount, 10, '0');
	}

	if(iStrNumCompare(szAmount, szFloorLimit, 10) >= 0) {
		// 超出最低限额, 置位TVR相应指示位
		iEmvSetItemBit(EMV_ITEM_TVR, TVR_24_EXCEEDS_FLOOR_LIMIT, 1);
	}
	return(HXEMV_OK);
}
// 随机交易检查
// ret : HXEMV_OK          : OK
//       HXEMV_CORE        : 其它错误
static int iEmvRandomTransSelection(void)
{
	int   iRet;
	uchar *p;
	int   iMaxTargetPercentage, iTargetPercentage;
	uchar szRandomSelThreshold[13];
	uchar szFloorLimit[13], szAmount[20];
	int   iTransactionTargetPercent;
	uchar szInterpolationFactor[20];
	long  lInterpolationFactor;
	uchar ucRand;

	if(iEmvGetECTransFlag() == 1)
		return(HXEMV_OK); // EC交易不做随机交易检查

	if(iEmvTermCommunication() == TERM_COM_OFFLINE_ONLY)
		return(HXEMV_OK); // 脱机终端不用做随机交易检查

	iRet = iTlvGetObjValue(gl_sTlvDbTermVar, TAG_DFXX_MaxTargetPercentage, &p);
	if(iRet != 1)
		return(HXEMV_OK); // 无MaxTargetPercentage, 不做随机检查
	iMaxTargetPercentage = (int)*p;

	iRet = iTlvGetObjValue(gl_sTlvDbTermVar, TAG_DFXX_TargetPercentage, &p);
	if(iRet != 1)
		return(HXEMV_OK); // 无TargetPercentage, 不做随机检查
	iTargetPercentage = (int)*p;

	iRet = iTlvGetObjValue(gl_sTlvDbTermVar, TAG_DFXX_RandomSelThreshold, &p);
	if(iRet != 4)
		return(HXEMV_OK); // 无RandomSelThreshold, 不做随机检查
	sprintf(szRandomSelThreshold, "%012lu", ulStrToLong(p, 4));

	iRet = iTlvGetObjValue(gl_sTlvDbTermVar, TAG_9F1B_TermFloorLimit, &p);
	ASSERT(iRet == 4);
	if(iRet != 4)
		return(HXEMV_CORE); // FloorLimit必须存在
	sprintf(szFloorLimit, "%012lu", ulStrToLong(p, 4));

	iRet = iTlvGetObjValue(gl_sTlvDbTermVar, TAG_9F02_AmountN, &p);
	ASSERT(iRet == 6);
	if(iRet != 6)
		return(HXEMV_CORE); // Amount必须存在
	vOneTwo0(p, 6, szAmount);

	// 计算Transaction Target Percent, refer to Emv2008 book3, 10.6.2, P112
	if(iStrNumCompare(szAmount, szFloorLimit, 10) >= 0)
		return(HXEMV_OK); // 金额大于等于FloorLimit, 此种情况由限额检查处理
	if(iStrNumCompare(szAmount, szRandomSelThreshold, 10) <= 0) {
		// 金额小于等于阀值, 交易联机概率等于iTargetPercentage%
		iTransactionTargetPercent = iTargetPercentage;
	} else {
		// 金额大于阀值, 计算联机概率

		// InterpolationFactor = (Amount-ThresholdValue) / (FloorLimit-ThresholdValue)
		// 为提高精度, InterpolationFactor乘了10000倍
		iStrNumSub(szAmount, sizeof(szAmount)-1, szAmount, szRandomSelThreshold, 10, 0);
		iStrNumMult(szAmount, sizeof(szAmount)-1, szAmount, "10000", 10, 0); // 大数运算只支持整数, 乘以10000用以提高精度
		iStrNumSub(szFloorLimit, sizeof(szFloorLimit)-1, szFloorLimit, szRandomSelThreshold, 10, 0);
		iStrNumDiv(szInterpolationFactor, sizeof(szInterpolationFactor)-1, NULL, 0, szAmount, szFloorLimit, 10, 0);
		lInterpolationFactor = atol(szInterpolationFactor); // 注意,lInterpolationFactor为实际值的10000倍, 例如0.45表示为4500

		// 计算iTransactionTargetPercent
		// iTransactionTargetPercent = ((MaximumTargetPercent-TargetPercent)*InterpolationFactor)+TargetPercent
		iTransactionTargetPercent = (((iMaxTargetPercentage-iTargetPercentage)*lInterpolationFactor)+5000) / 10000;
		iTransactionTargetPercent += iTargetPercentage;
		if(iTransactionTargetPercent > iMaxTargetPercentage)
			iTransactionTargetPercent = iMaxTargetPercentage; // 容错
	}

	// 产生1-99的随机数
	vGetRand(szAmount, 3);
	ucRand = (uchar)(ulStrToLong(szAmount, 3)%99) + 1;
	iRet = iTlvMakeAddObj(gl_sTlvDbTermVar, TAG_DFXX_RandForSelect, 1, &ucRand, TLV_CONFLICT_REPLACE);
	if(ucRand <= iTransactionTargetPercent) {
		// 交易被随机选中, 设置TVR相应位
		iEmvSetItemBit(EMV_ITEM_TVR, TVR_27_RANDOMLY_SELECTED, 1);
	}

	return(HXEMV_OK);
}
// 频度检查
// ret : HXEMV_OK          : OK
//       HXEMV_CORE        : 其它错误
//       HXEMV_CARD_OP     : 卡操作错
//       HXEMV_TERMINATE   : 满足拒绝条件，交易终止
static int iEmvVelocityChecking(void)
{
	int   iRet;
    uint  uiRet;
	uchar *p;
	uchar ucLen, sBuf[256];
	uchar *pucLowerLimit, *pucUpperLimit;
	uchar sATC[2], sLastOnlineATC[2];
	uchar ucATCFlag, ucLastOnlineATCFlag; // ATC、LastOnlineATC读出标记, 0:未读出 1:成功读出
	uint  uiDifference;

	if(iEmvGetECTransFlag() == 1)
		return(HXEMV_OK); // EC交易不做频度检查

	iRet = iTlvGetObjValue(gl_sTlvDbCard, TAG_9F14_LCOL, &pucLowerLimit);
	if(iRet != 1)
		return(HXEMV_OK); // 无Lower Consecutive Offline Limit, 不做频度检查
	iRet = iTlvGetObjValue(gl_sTlvDbCard, TAG_9F23_UCOL, &pucUpperLimit);
	if(iRet != 1)
		return(HXEMV_OK); // 无Upper Consecutive Offline Limit, 不做频度检查

	uiRet = uiEmvCmdGetData(TAG_9F36_ATC, &ucLen, sBuf);
	if(uiRet == 1)
		return(HXEMV_CARD_OP); // 卡片操作错
	ucATCFlag = 0; // 先假设没有读出ATC
	if(uiRet == 0) {
		// 成功读出ATC
		if(iTlvCheckTlvObject(sBuf) == (int)ucLen) {
			iRet = iTlvValue(sBuf, &p);
			if(iRet == 2) {
				memcpy(sATC, p, 2);
				ucATCFlag = 1;
			}
		}
	}

	uiRet = uiEmvCmdGetData(TAG_9F13_LastOnlineATC, &ucLen, sBuf);
	if(uiRet == 1)
		return(HXEMV_CARD_OP);
	ucLastOnlineATCFlag = 0; // 先假设没有读出Last Online ATC
	if(uiRet == 0) {
		// 成功读出LastOnlineATC
		if(iTlvCheckTlvObject(sBuf) == (int)ucLen) {
			iRet = iTlvValue(sBuf, &p);
			if(iRet == 2) {
				memcpy(sLastOnlineATC, p, 2);
				ucLastOnlineATCFlag = 1;
			}
		}
	}

	// 参考EMV测试案例 2CA.029.04, 如果没有读出ATC或Last On Line ATC, 设置数据丢失位
	if(ucATCFlag==0 || ucLastOnlineATCFlag==0)
		iEmvSetItemBit(EMV_ITEM_TVR, TVR_02_ICC_DATA_MISSING, 1); // data missing

	// 参考 Emv2008 book3 10.6.3, P113
	if(ucATCFlag==0 || ucLastOnlineATCFlag==0 || memcmp(sATC, sLastOnlineATC, 2)<=0) {
		// 设置TVR相应位
		iEmvSetItemBit(EMV_ITEM_TVR, TVR_25_EXCEEDS_LOWER_LIMIT, 1); // lower consecutive offline limit exceeded
		iEmvSetItemBit(EMV_ITEM_TVR, TVR_26_EXCEEDS_UPPER_LIMIT, 1); // upper consecutive offline limit exceeded
		if(iTestStrZero(sLastOnlineATC, 2) == 0) {
			// 置TVR新卡标志
			iEmvSetItemBit(EMV_ITEM_TVR, TVR_12_NEW_CARD, 1); // new card
		}
		return(HXEMV_OK);
	}
	uiDifference = ulStrToLong(sATC, 2) - ulStrToLong(sLastOnlineATC, 2);
	if(uiDifference > (uint)*pucLowerLimit) {
		// 超过了lower consecutive offline limit exceeded
		iEmvSetItemBit(EMV_ITEM_TVR, TVR_25_EXCEEDS_LOWER_LIMIT, 1); // lower consecutive offline limit exceeded
		if(uiDifference > (uint)*pucUpperLimit) {
			iEmvSetItemBit(EMV_ITEM_TVR, TVR_26_EXCEEDS_UPPER_LIMIT, 1); // upper consecutive offline limit exceeded
		}
	}
	if(iTestStrZero(sLastOnlineATC, 2) == 0) {
		// 置TVR新卡标志
		iEmvSetItemBit(EMV_ITEM_TVR, TVR_12_NEW_CARD, 1); // new card
	}

	return(HXEMV_OK);
}
// 黑名单检查
// ret : HXEMV_OK          : OK
//       HXEMV_CORE        : 其它错误
static int iExceptionFileChecking(void)
{
	int   iRet;
	uchar *p;

	iRet = iTlvGetObjValue(gl_sTlvDbTermFixed, TAG_DFXX_BlacklistSupport, &p);
	if(iRet != 1)
		return(HXEMV_OK); // 终端不支持黑名单检查
	if(*p == 0)
		return(HXEMV_OK); // 终端不支持黑名单检查

	// 终端支持黑名单检查
	iRet = iEmvIOBlackCardCheck();
	ASSERT(iRet != EMVIO_ERROR);
	if(iRet == EMVIO_ERROR)
		return(HXEMV_CORE);
	if(iRet == EMVIO_BLACK) {
		// 黑名单卡, 设置TVR相应位
		iEmvSetItemBit(EMV_ITEM_TVR, TVR_03_BLACK_CARD, 1); // 黑名单卡
	}

	return(HXEMV_OK);
}

// 终端风险管理
// ret : HXEMV_OK            : OK
//       HXEMV_CORE          : 其它错误
//       HXEMV_CARD_OP       : 卡操作错
//       HXEMV_TERMINATE     : 满足拒绝条件，交易终止
//       HXEMV_DENIAL        : 终端行为分析结果为脱机拒绝
//       HXEMV_DENIAL_ADVICE : 终端行为分析结果为脱机拒绝, 有Advice
//       HXEMV_CANCEL        : 被取消
//       HXEMV_TIMEOUT       : 超时
// Note: 不关心AIP设置, 强制执行
int iEmvTermRiskManage(void)
{
	int   iRet;
	uchar *psTlvObjValue;
	uint  uiTransType;
	uchar szAmount[13];

	iRet = iTlvGetObjValue(gl_sTlvDbTermVar, TAG_9F02_AmountN, &psTlvObjValue);
	if(iRet > 0)
		vOneTwo0(psTlvObjValue, 6, szAmount);
	if(iRet<0 || strcmp((char *)szAmount, "000000000000")==0) {
		// 之前没有输入金额, 终端风险管理前必须输入
		iRet = iTlvGetObjValue(gl_sTlvDbTermVar, TAG_DFXX_TransType, &psTlvObjValue);
        ASSERT(iRet == 2);
		if(iRet != 2)
			return(HXEMV_CORE);
		uiTransType = ulStrToLong(psTlvObjValue, 2);
		if(uiTransType==TRANS_TYPE_AVAILABLE_INQ || uiTransType==TRANS_TYPE_BALANCE_INQ)
			strcpy(szAmount, "0"); // 查询余额不需要输入金额
		else {
			iRet = iEmvIOGetAmount(EMVIO_AMOUNT, szAmount);
			if(iRet == EMVIO_CANCEL)
				return(HXEMV_CANCEL);
			if(iRet == EMVIO_TIMEOUT)
				return(HXEMV_TIMEOUT);
            ASSERT(iRet == 0);
			if(iRet)
				return(HXEMV_CORE);
		}
		iRet = iEmvSaveAmount(szAmount); // 将金额保存到tlv数据库中
		if(iRet != 0)
			return(HXEMV_CORE);
	}

	iRet = iEmvFloorLimitChecking(); // 最低限额检查
	if(iRet == HXEMV_CORE)
		return(iRet);
	iRet = iEmvRandomTransSelection(); // 随机交易检查
	if(iRet == HXEMV_CORE)
		return(iRet);
	iRet = iEmvVelocityChecking(); // 频度检查
	if(iRet != HXEMV_OK)
		return(iRet);
	iRet = iExceptionFileChecking(); // 黑名单检查
	if(iRet == HXEMV_CORE)
		return(iRet);
	iEmvSetItemBit(EMV_ITEM_TSI, TSI_04_TERM_RISK_PERFORMED, 1); // 终端风险管理已执行
    // 注, 此时必须进行拒绝位检查, 以阻止注定拒绝的交易还要求持卡人进行持卡人验证
	iRet = iEmvIfDenial();
	if(iRet != HXEMV_OK)
		return(iRet);

	return(HXEMV_OK);
}

// 持卡人验证
// out : piNeedSignFlag      : 如果成功完成，返回需要签字标志，0:不需要签字 1:需要签字
// ret : HXEMV_OK            : OK
//       HXEMV_CANCEL        : 用户取消
//       HXEMV_TIMEOUT       : 超时
//       HXEMV_CARD_OP       : 卡操作错
//       HXEMV_CARD_SW       : 非法卡状态字
//       HXEMV_TERMINATE     : 满足拒绝条件，交易终止
//       HXEMV_DENIAL        : 终端行为分析结果为脱机拒绝
//       HXEMV_DENIAL_ADVICE : 终端行为分析结果为脱机拒绝, 有Advice
//       HXEMV_FLOW_ERROR    : EMV流程错误
//       HXEMV_CORE          : 内部错误
int iEmvModuleCardHolderVerify(int *piNeedSignFlag)
{
	int   iRet;

	iRet = iEmvCardHolderVerify(piNeedSignFlag);
	if(iRet != HXEMV_OK)
		return(iRet);

	iRet = iEmvIfDenial();
	if(iRet != HXEMV_OK)
		return(iRet);
	return(HXEMV_OK);
}

// 判断是否需要输入EC需要的联机密码
// ret : HXEMV_OK		   : 需要
//       HXEMV_NA          : 不需要
int iEmvIfNeedECOnlinePin(void)
{
	int    iRet;

	iRet = iTermActionAnalysis(3); // 3:EC专用, 完整终端行为分析, 判断是否需要输入EC需要的联机密码
	if(iRet == 4)
		return(HXEMV_OK); // 需要EC联机密码
	return(HXEMV_NA); // 不需要EC联机密码
}

// 判断是否需要拒绝交易
// ret : HXEMV_OK		     : 不需要
//       HXEMV_CARD_OP       : 卡操作错
//       HXEMV_TERMINATE     : 满足拒绝条件，交易终止
//       HXEMV_DENIAL        : 终端行为分析结果为脱机拒绝
//       HXEMV_DENIAL_ADVICE : 终端行为分析结果为脱机拒绝, 有Advice
//       HXEMV_CORE          : 内部错误
int iEmvIfDenial(void)
{
	int    iRet;
	int    iCardAction;

	iRet = iTermActionAnalysis(2); // 2:拒绝位检查
	if(iRet == 1) {
		// 交易被拒绝
		iEmvIOShowMsg(EMVMSG_0E_PLEASE_WAIT); // newadd
		iRet = iEmvGAC1(&iCardAction);
		if(iRet==HXEMV_CORE || iRet==HXEMV_CARD_OP || iRet==HXEMV_TERMINATE)
			return(iRet);
		if(iRet != HXEMV_OK)
			return(HXEMV_DENIAL);
		if(iCardAction == GAC_ACTION_AAC_ADVICE)
			return(HXEMV_DENIAL_ADVICE);
		return(HXEMV_DENIAL);
	}
	return(HXEMV_OK);
}

// 终端行为分析
// ret : HXEMV_OK		     : OK
//       HXEMV_CORE          : 其它错误
//       HXEMV_DENIAL        : 终端行为分析结果为脱机拒绝
//       HXEMV_DENIAL_ADVICE : 终端行为分析结果为脱机拒绝, 有Advice
//       HXEMV_CARD_OP       : 终端行为分析结果为脱机拒绝, 申请AAC时卡片出错
//       HXEMV_TERMINATE     : 满足拒绝条件，交易终止
int iEmvTermActionAnalysis(void)
{
	int    iRet;
	int    iCardAction;

	iRet = iTermActionAnalysis(0); // 0:正常终端行为分析
	if(iRet == 0)
		return(HXEMV_CORE);
	if(iRet == 1) {
		// 交易被拒绝
		iRet = iEmvGAC1(&iCardAction); // 忽略iCardAction
		if(iRet==HXEMV_CORE || iRet==HXEMV_CARD_OP || iRet==HXEMV_TERMINATE)
			return(iRet);
		if(iRet != HXEMV_OK)
			return(HXEMV_DENIAL);
		if(iCardAction == GAC_ACTION_AAC_ADVICE)
			return(HXEMV_DENIAL_ADVICE);
		return(HXEMV_DENIAL);
	}
	return(HXEMV_OK); // 只要不是拒绝, 就返回OK
}

// 生成TC Hash Value
// ret : HXEMV_OK          : OK
//       HXEMV_CORE        : 其它错误
static int iGenTCHashValue(void)
{
	int   iRet;
	uchar *p;
	uchar ucTdolLen, sTdol[252];
	uchar sBuf[10240];

	// 搜索卡片Tdol
	iRet = iTlvGetObjValue(gl_sTlvDbCard, TAG_97_TDOL, &p);
	if(iRet > 0) {
		// 找到卡片TDOL
		ASSERT(iRet <= 252);
		if(iRet > 252)
			return(HXEMV_CORE); // 读记录时检查数据项应确保此时TDOL长度不超过252
		memcpy(sTdol, p, iRet);
		ucTdolLen = (uchar)iRet;
	} else {
		// 卡片无TDOL, 搜索终端Default TDOL
		iRet = iTlvGetObjValue(gl_sTlvDbTermVar, TAG_DFXX_DefaultTDOL, &p);
		if(iRet > 0) {
			// 找到Default TDOL
			memcpy(sTdol, p, iRet);
			ucTdolLen = (uchar)iRet;
			iEmvSetItemBit(EMV_ITEM_TVR, TVR_32_DEFAULT_TDOL_USED, 1); // 设置TVR Default TDOL使用位
		} else {
			// 没找到Default TDOL
			ucTdolLen = 0;
		}
	}
		// 产生TC Hash Value
    // 打包TDOL
	iRet = iTlvMakeDOLBlock(sBuf/*目标*/, sizeof(sBuf), sTdol, ucTdolLen,
								gl_sTlvDbTermFixed, gl_sTlvDbTermVar, gl_sTlvDbCard, gl_sTlvDbIssuer);
	if(iRet < 0)
		return(HXEMV_CORE);
	// 构造TDOL目标块成功, 计算TC Hash Value
	vSHA1Init();
	vSHA1Update(sBuf, (ushort)iRet);
    vSHA1Result(sBuf); // sBuf is the TC Hash Value
	iRet = iTlvMakeAddObj(gl_sTlvDbTermVar, TAG_98_TCHashValue, 20, sBuf, TLV_CONFLICT_REPLACE);
	ASSERT(iRet >= 0);
	if(iRet < 0)
		return(HXEMV_CORE);
	return(HXEMV_OK);
}

// 提取GAC应答数据
// in  : ucP1            : 发送GAC指令时的参数P1
//       psDataOut       : GAC指令的返回数据
//       ucDataOutLen    : GAC指令的返回数据长度
// ret : HXEMV_OK        : OK
//       HXEMV_CORE      : 其它错误
//       HXEMV_TERMINATE : 满足拒绝条件，交易终止
static int iGetACData(uchar ucP1, uchar *psDataOut, uchar ucDataOutLen)
{
	uchar ucTlvObjValueLen, *psTlvObjValue, *psTlvObj;
	uchar sTlvObj9F10[40]; // 保存T9F10数据对象
	int   iRet;

	// 删除GAC可能返回的数据项, 之前初始化、读记录、DDA时, 错误的卡可能会有这些伪数据
	iTlvDelObj(gl_sTlvDbCard, TAG_9F27_CID);
	iTlvDelObj(gl_sTlvDbCard, TAG_9F36_ATC);
	iTlvDelObj(gl_sTlvDbCard, TAG_9F26_AC);
	if(ucP1 & 0x10) {
		iTlvDelObj(gl_sTlvDbCard, TAG_9F4B_SDAData); // 本次要求CDA, 作废之前可能的签字动态数据
	}
	iRet = iTlvGetObj(gl_sTlvDbCard, TAG_9F10_IssuerAppData, &psTlvObj);
	if(iRet > 0)
		memcpy(sTlvObj9F10, psTlvObj, iRet); // 备份T9F10数据对象
	else
		sTlvObj9F10[0] = 0;
	iTlvDelObj(gl_sTlvDbCard, TAG_9F10_IssuerAppData); // 该项如果GAC1存在但GAC2不存在时

	iRet = iTlvCheckTlvObject(psDataOut); // 检查应答数据TLV格式完整性
	if(iRet<0 || iRet>ucDataOutLen)
		return(HXEMV_TERMINATE);
	if(psDataOut[0] == 0x80) {
		// Tag is 0x80, Format 1 CID(T9F27)[1] + ATC(T9F36) + AC(T9F26)[8] + IAD(T9F10)(Option)[n]
		iRet = iTlvValue(psDataOut, &psTlvObjValue);
		if(iRet<11 || iRet>11+32) {
			// GPO指令返回数据长度不足, need at least CID[1] + ATC[2] + AC[8]
			// or GPO指令返回数据长度超长, need at most CID[1] + ATC[2] + AC[8] + IAD[32]
			return(HXEMV_TERMINATE);
		}
		ucTlvObjValueLen = (uchar)iRet;

		// psTlvObjValue=T80的值 ucTlvObjValueLen=T80的值的长度
		if(ucP1 & 0x10) {
			// 如果请求CDA，不允许format1
			if(psTlvObjValue[0] & 0xC0) {
				// 在返回不是AAC时才需要作此检查
				return(HXEMV_TERMINATE);
			}
		}
		// add T9F27
		iRet = iTlvMakeAddObj(gl_sTlvDbCard, TAG_9F27_CID, 1, psTlvObjValue+0, TLV_CONFLICT_REPLACE);
    	ASSERT(iRet >= 0);
		if(iRet < 0)
			return(HXEMV_CORE);
		// add T9F36
		iRet = iTlvMakeAddObj(gl_sTlvDbCard, TAG_9F36_ATC, 2, psTlvObjValue+1, TLV_CONFLICT_REPLACE);
    	ASSERT(iRet >= 0);
		if(iRet < 0)
			return(HXEMV_CORE);
		// add T9F26
		vRandShuffle(ulStrToLong(psTlvObjValue+3, 4)); // 借用AC扰动随机数发生器
		iRet = iTlvMakeAddObj(gl_sTlvDbCard, TAG_9F26_AC, 8, psTlvObjValue+3, TLV_CONFLICT_REPLACE);
    	ASSERT(iRet >= 0);
		if(iRet < 0)
			return(HXEMV_CORE);
		if(ucTlvObjValueLen > 1+2+8) {
			// 存在T9F10, add T9F10
			iRet = iTlvMakeAddObj(gl_sTlvDbCard, TAG_9F10_IssuerAppData, ucTlvObjValueLen-11, psTlvObjValue+11, TLV_CONFLICT_REPLACE);
        	ASSERT(iRet >= 0);
			if(iRet < 0)
				return(HXEMV_CORE);
		} else if(sTlvObj9F10[0]) {
			// 不存在T9F10, 但如果之前曾备份过卡片数据库中的T9F10, 恢复
			iRet = iTlvAddObj(gl_sTlvDbCard, sTlvObj9F10, TLV_CONFLICT_REPLACE);
			ASSERT(iRet >= 0);
			if(iRet < 0)
				return(HXEMV_CORE);
		}
	} else if(psDataOut[0] == 0x77) {
		// Tag is 0x77, Format 2
		// 将卡片返回数据添加到卡片TLV数据库
		iRet = iTlvBatchAddObj(0/*only primitive*/, gl_sTlvDbCard, psDataOut, ucDataOutLen, TLV_CONFLICT_ERR, 0/*0:长度为0对象不添加*/);
		if(iRet==TLV_ERR_BUF_SPACE || iRet==TLV_ERR_BUF_FORMAT) {
        	ASSERT(0);
			return(HXEMV_CORE);
		}
		if(iRet < 0)
			return(HXEMV_TERMINATE);
		if(iTlvGetObj(gl_sTlvDbCard, TAG_9F10_IssuerAppData, &psTlvObj) <= 0) {
			// T77包中无T9F10数据对象
			if(sTlvObj9F10[0]) {
				// 如果之前存在T9F10数据对象, 恢复
				iRet = iTlvAddObj(gl_sTlvDbCard, sTlvObj9F10, TLV_CONFLICT_REPLACE);
				ASSERT(iRet >= 0);
				if(iRet < 0)
					return(HXEMV_CORE);
			}
		}

		// check T9F27 T9F36 T9F26
	    iRet = iTlvGetObjValue(gl_sTlvDbCard, TAG_9F36_ATC, &psTlvObjValue);
		if(iRet != 2)
			return(HXEMV_TERMINATE); // 没找到ATC或格式错误
	    iRet = iTlvGetObjValue(gl_sTlvDbCard, TAG_9F27_CID, &psTlvObjValue);
		if(iRet != 1)
			return(HXEMV_TERMINATE); // 没找到CID或格式错误
		if((ucP1&0x10) && !(psTlvObjValue[0]&0xC0)) {
			// 如果是请求CDA，并且CID指明返回了AAC, 拒绝交易, refer Emv2008 book 6.6.2, P74
			return(HXEMV_OK); // 不能返回HXEMV_TERMINATE
		}
		if(!(ucP1&0x10) || !(psTlvObjValue[0]&0xC0)) {
			// 如果不是请求CDA，或者CID指明返回了AAC, 要求必须存在AC
		    iRet = iTlvGetObjValue(gl_sTlvDbCard, TAG_9F26_AC, &psTlvObjValue);
			if(iRet != 8)
				return(HXEMV_TERMINATE); // 没找到AC或格式错误
			vRandShuffle(ulStrToLong(psTlvObjValue, 4)); // 借用AC扰动随机数发生器
		}
	} else {
		return(HXEMV_TERMINATE); // GAC指令返回数据格式码不识别
	}

	return(HXEMV_OK);
}

// 从9F10中获取EC余额, 如果没有金额信息, 则重新读取
// ret : HXEMV_OK   : OK
// Note: Pboc2.0 Book13, EC规范, 7.4.6
//       由于交易已经成功, 如果获取余额失败, 不认为出错
static int iGetECBalanceFrom9F10(void)
{
	uchar *psValue;
	int   iValueLen;
	uchar ucLen;
	uchar sBalance[20];
	int   iRet;
	uint  uiRet;

	iValueLen = iTlvGetObjValue(gl_sTlvDbCard, TAG_9F10_IssuerAppData, &psValue);
	// 9F10数据格式: 规范自定义数据长度[1]+规范自定义数据[n]+发卡行自定义数据长度[1]+发卡行自定义数据[n]
	// 余额在发卡行自定义数据中, 格式:标识(余额标识为0x01)[1]+余额[5]+Mac[4]
	// 检查自定义数据完整性
	if(iValueLen <= 0)
		goto _label_getdata_readbal; // 没读到9F10
	if(psValue[0]+1 >= iValueLen)
		goto _label_getdata_readbal; // 无发卡行自定义数据
	// psValue 与 iValueLen调整到发卡行自定义数据
	iValueLen -= psValue[0]+1;
	psValue += psValue[0]+1;
	// 检查发卡行自定义数据完整性
	if(psValue[0]+1 > iValueLen)
		goto _label_getdata_readbal; // 发卡行自定义数据格式错
	if(psValue[0] < 10)
		goto _label_getdata_readbal; // 余额需要10字节空间, 发卡行自定义数据长度不足
	// psValue 与 iValueLen调整到发卡行自定义数据内容
	iValueLen --;
	psValue ++;
	// 检查是否为余额标识
	if(psValue[0] != 0x01)
		goto _label_getdata_readbal; // 不是余额标识
	// 成功获取了余额, 用其替换原余额(忽略Mac)
	sBalance[0] = 0;
	memcpy(&sBalance[1], psValue+1, 5);
	iRet = iTlvMakeAddObj(gl_sTlvDbCard, TAG_9F79_ECBalance_I, 6, sBalance, TLV_CONFLICT_REPLACE);
	// ignore iRet
	return(HXEMV_OK);

_label_getdata_readbal:
	// 9F10没有获取到余额, 通过getdata指令读取
	iValueLen = sizeof(sBalance);
	uiRet = uiEmvCmdGetData(TAG_9F79_ECBalance_I, &ucLen, sBalance);
	if(uiRet != 0)
		return(HXEMV_OK); // 个人分析 : 虽然读余额失败, 但既然GAC1已经成功, 不能算错, 只是不更新余额
	if(ucLen < 2)
		return(HXEMV_OK); // 9F09对象格式 : Tag[2]+Len[1]+Value[6]
	if(memcmp(sBalance, TAG_9F79_ECBalance_I, 2) != 0)
		return(HXEMV_OK); // 读取的对象不是余额
	iRet = iTlvCheckTlvObject(sBalance);
	if(iRet <= 0)
		return(HXEMV_OK); // 格式错
	// 成功读出了余额, 将其替换原余额
	iRet = iTlvAddObj(gl_sTlvDbCard, sBalance, TLV_CONFLICT_REPLACE);
	// ignore iRet
	return(HXEMV_OK);
}

// 第一次GAC
// out : piCardAction      : 卡片执行结果
//                           GAC_ACTION_TC     : 批准(生成TC)
//							 GAC_ACTION_AAC    : 拒绝(生成AAC)
//                           GAC_ACTION_AAC_ADVICE : 拒绝(生成AAC,有Advice)
//                           GAC_ACTION_ARQC   : 要求联机(生成ARQC)
// ret : HXEMV_OK          : OK
//       HXEMV_CORE        : 其它错误
//       HXEMV_CARD_OP     : 卡操作错
//       HXEMV_CARD_SW     : 非法卡状态字
//       HXEMV_TERMINATE   : 满足拒绝条件，交易终止
//       HXEMV_NOT_ALLOWED : 服务不允许
//       HXEMV_NOT_ACCEPTED: 不接受
// Note: 某些情况下, 该函数会自动调用iEmvGAC2()
//       如:CDA失败、仅脱机终端GAC1卡片返回ARQC...
//       不因为请求ARQC而不要求CDA
int iEmvGAC1(int *piCardAction)
{
	uint  uiRet;
	int   iRet, iRet2;
	uint  uiTransType;
	uchar *p;
	int   iCdol1Len;
	uchar *psCdol1;
	int   iReqCdaFlag; // 要求CDA标志, 0:不要CDA, 1:要CDA
	int   iTermAction; // 终端行为分析结果, 1:拒绝 2:联机 3:脱机
	uchar ucP1; // GAC指令参数P1
	uchar ucDataInLen, sDataIn[256];
	uchar ucDataOutLen, sDataOut[256];
	uchar szArc[3];
	uchar ucCid;
	uchar sBuf[260];

	// 准备数据
	// 随机数
	vGetRand(sBuf, 4);
	iTlvMakeAddObj(gl_sTlvDbTermVar, TAG_9F37_TermRand, 4, sBuf, TLV_CONFLICT_REPLACE);
	// 在CDOL1中搜索TC Hash Value域, 如果存在, 准备TC Hash Value
	iCdol1Len = iTlvGetObjValue(gl_sTlvDbCard, TAG_8C_CDOL1, &psCdol1);
   	ASSERT(iCdol1Len > 0);
	if(iCdol1Len <= 0)
		return(HXEMV_CORE); // CDOL1必须存在
    iRet =  iTlvSearchDOLTag(psCdol1, iCdol1Len, TAG_98_TCHashValue, NULL);
	if(iRet > 0) {
		// CDOL1要求TC Hash Value, 生成TC Hash Value
		iRet = iGenTCHashValue();
		if(iRet)
			return(iRet);
	}

	iReqCdaFlag = 1; // 首先假设要求CDA
	if(iEmvTestItemBit(EMV_ITEM_AIP, AIP_07_CDA_SUPPORTED)==0
			|| iEmvTestItemBit(EMV_ITEM_TERM_CAP, TERM_CAP_20_CDA)==0)
		iReqCdaFlag = 0; // 只有终端与卡片都支持CDA时才执行CDA
	if(iEmvTestItemBit(EMV_ITEM_TVR, TVR_05_CDA_FAILED))
		iReqCdaFlag = 0; // CDA准备阶段已经失败, 不执行CDA

	iTermAction = iTermActionAnalysis(0/*0:正常终端行为分析*/); // 返回0:出错 1:拒绝 2:联机 3:脱机
	if(iTermAction <= 0)
		return(HXEMV_CORE);
	szArc[0] = 0;
	switch(iTermAction) {
	case 1: // Denial
        ucP1 = 0x00; // require AAC
		if(iReqCdaFlag) {
			iEmvSetItemBit(EMV_ITEM_TVR, TVR_00_DATA_AUTH_NOT_PERFORMED, 1);
			iReqCdaFlag = 0; // 请求AAC不执行CDA
		}
		strcpy(szArc, "Z1");
		break;
	case 2: // Online
		ucP1 = 0x80; // require ARQC (10XX XXXX)
		break;
	case 3: // Offline
		iRet = iTlvGetObjValue(gl_sTlvDbTermVar, TAG_DFXX_TransType, &p);
		ASSERT(iRet == 2);
		if(iRet != 2)
			return(HXEMV_CORE); // 交易类型必须存在
		uiTransType = ulStrToLong(p, 2);
		if(uiTransType==TRANS_TYPE_AVAILABLE_INQ || uiTransType==TRANS_TYPE_BALANCE_INQ) {
			// 查询交易强制联机
			ucP1 = 0x80; // require ARQC (10XX XXXX)
		} else {
			ucP1 = 0x40; // require TC (01XX XXXX)
			strcpy(szArc, "Y1");
		}
		break;
	}
	if(iReqCdaFlag)
		ucP1 |= 0x10; // CDA request (XXX1 XXXX)

	if(szArc[0]) {
		// szArc有内容, 虽然脱机完成, 但T8A还是放到发卡行TLV数据库中
		iTlvMakeAddObj(gl_sTlvDbIssuer, TAG_8A_AuthRspCode, 2, szArc, TLV_CONFLICT_REPLACE);
	}

	if(iReqCdaFlag)
		iEmvSetItemBit(EMV_ITEM_TSI, TSI_00_DATA_AUTH_PERFORMED, 1); // 如果请求了CDA, 设置TSI脱机数据认证执行标志

    // 打包CDOL1目标串
	iRet = iTlvMakeDOLBlock(sDataIn/*CDOL1目标串*/, sizeof(sDataIn), psCdol1, iCdol1Len,
          				    gl_sTlvDbTermFixed, gl_sTlvDbTermVar, gl_sTlvDbCard, gl_sTlvDbIssuer);
	ASSERT(iRet >= 0);
	if(iRet < 0)
		return(HXEMV_CORE);
	ucDataInLen = iRet;
	iRet = iTlvMakeAddObj(gl_sTlvDbTermVar, TAG_DFXX_Cdol1DataBlock, ucDataInLen, sDataIn, TLV_CONFLICT_REPLACE);
	ASSERT(iRet >= 0);
	if(iRet < 0)
		return(HXEMV_CORE);

	// 执行Generate AC指令
	iEmvSetItemBit(EMV_ITEM_TSI, TSI_02_CARD_RISK_PERFORMED, 1); // IC卡风险管理已执行, 先于GAC指令, Refer to Emv2008 book4 10.1 page 79
	uiRet = uiEmvCmdGenerateAC(ucP1, ucDataInLen, sDataIn, &ucDataOutLen, sDataOut);
	if(uiRet == 1)
		return(HXEMV_CARD_OP);
	if(uiRet)
		return(HXEMV_CARD_SW);

	// 提取GAC数据
	iRet = iGetACData(ucP1, sDataOut, ucDataOutLen);
	if(iRet)
		return(iRet);

	// check CID, refer to Emv2008 book3 9.3 P88
    iTlvGetObjValue(gl_sTlvDbCard, TAG_9F27_CID, &p);
	if((p[0]&0xC0) == 0xC0)
		return(HXEMV_TERMINATE); // 返回值非法
	if((ucP1&0xC0) == 0x00) {
		// require AAC
		if((p[0]&0xC0) != 0x00) {
			// 在请求AAC时没有返回AAC
			return(HXEMV_TERMINATE);
		}
	} else if((ucP1&0xC0) == 0x80) {
		// require ARQC
		if((p[0]&0xC0) == 0x40) {
			// 在请求ARQC时不能返回TC
			return(HXEMV_TERMINATE);
		}
	}

	iRet = iTlvGetObjValue(gl_sTlvDbCard, TAG_9F27_CID, &p);
	ASSERT(iRet == 1);
	if(iRet != 1)
		return(HXEMV_CORE); // CID必须存在且合法
	ucCid = *p;

	if(iReqCdaFlag && (ucCid&0xC0)!=0x00) {
		// 要求CDA验证, 且CID指示没有返回AAC, 执行CDA验证
		iRet = iEmvCda(1/*1:GAC1*/, sDataOut, ucDataOutLen);
		if(iRet == EMV_DAUTH_NATIVE)
			return(HXEMV_CORE);
		if(iRet == EMV_DAUTH_DATA_MISSING)
			return(HXEMV_TERMINATE); // refer Emv bulletin SU-54
		if(iRet) {
			// CDA失败, 如果CID指示返回TC, decline, 否则请求AAC之后decline
			iEmvSetItemBit(EMV_ITEM_TVR, TVR_05_CDA_FAILED, 1);
			// 如果CDA失败, CID值可能会改变, 需要重新获取
			iTlvGetObjValue(gl_sTlvDbCard, TAG_9F27_CID, &p);
			ucCid = *p;
			if((ucCid&0xC0) == 0xC0) {
				// 返回值非法(该值原表示AAR, 现在为RFU)
				return(HXEMV_TERMINATE);
			}
			if((ucCid&0xC0) != 0x40) {
				// 不是TC, 需要请求AAC
				iRet2 = iEmvGAC2("Z1", NULL, NULL, 0, piCardAction); // Z1:Offline declined
				if(iRet2==HXEMV_CORE || iRet2==HXEMV_CARD_OP || iRet2==HXEMV_CARD_SW)
					return(iRet2);
			}
			*piCardAction = GAC_ACTION_AAC; // 1:脱机拒绝, CDA失败不考虑Advice位
			return(HXEMV_OK);
		}
	} else if(iReqCdaFlag && (ucCid&0xC0)==0x00) {
		// 要求CDA验证, 且CID指示返回AAC
		// 分析认为Bulletin N44指明在返回AAC时不认为CDA失败, 但案例2CC.122.00、2CC.122.01要求GAC(with CDA)返回AAC时置位CDA失败位
//		iEmvSetItemBit(EMV_ITEM_TVR, TVR_05_CDA_FAILED, 1); // modified 20130227
	}

	if((ucCid&0x07) == 0x01) {
	    // CID中advice位指示为"Service not allowed", 终止交易, Refer to Emv2008, book4, 6.3.7
		return(HXEMV_NOT_ACCEPTED);
	}

	switch(ucCid&0xC0) {
	case 0x00: // AAC
		if(ucCid & 0x08)
			*piCardAction = GAC_ACTION_AAC_ADVICE; // 脱机拒绝, 有Advice
		else
			*piCardAction = GAC_ACTION_AAC; // 脱机拒绝, 无Advice
		break;
	case 0x40: // TC
		*piCardAction = GAC_ACTION_TC; // 脱机接受
		if(iEmvGetECTransFlag() == 1) {
			// 本此交易为电子现金交易, 需要获取交易后EC余额
			// Pboc2.0 Book13, EC规范, 7.4.6
			// EC余额应从9F10中获取, 如果没有金额信息, 则重新读取
			iRet = iGetECBalanceFrom9F10();
			if(iRet != HXEMV_OK)
				return(iRet);
		}
		break;
	case 0x80: // ARQC
		*piCardAction = GAC_ACTION_ARQC; // 要求联机
		if(iEmvGetECTransFlag() == 1) { // EC交易标志 0:非EC交易 1:EC交易 2:GPO卡片返回EC授权码,但未走EC通道
			// 如果ECash脱机交易GAC返回ARQC, 说明应用未走ECash通道
			iEmvSetECTransFlag(2);
		}
		break;
	default: // 出错
		return(HXEMV_TERMINATE);
	}
	return(HXEMV_OK);
}

// 脚本处理
// in  : ucScriptType    : 处理的脚本类型, 0x71 or 0x72
// ret : HXEMV_OK        : OK
//       HXEMV_CORE      : 其它错误
// Note: 支持诸如TDFF1|TDFF2这种华信定义的Package方式脚本, 也支持T71|T72这种银联直连POS定义的单包脚本
//       TDFF1|TDFF2包方式可以支持多T71|T72脚本
//       优先执行T71|T72单包脚本
static int iEmvScriptProc(uchar ucScriptType)
{
	uchar *psPackage;   // 71 or 72 Package(TDFF1 or TDFF2) 内容
	int   iPackageLen;  // 71 or 72 Package(TDFF1 or TDFF2) 内容 Len
    uchar *psSingle;    // 71 or 72 单包(T71 or T72) 对象
    int   iSingleLen;   // 71 or 72 单包(T71 or T72) 对象 Len
	uchar *psTemplate;  // 71 or 72 Template
	int   iTemplateLen; // 71 or 72 Template len
	uchar *pT86Value;
	int   iT86ValueLen;
	uchar sRoute[10];
	uchar ucScriptResult;
	uchar *psScriptId;
	uchar sBuf[260];
	int   iErrFlag;
	int   i;
	int   iRet;
	uchar *p;
	uint  uiRet;
    uint  uiApduOutLen;
	int   iProcessedPackageLen; // 已经处理过的数据包长度

	if(ucScriptType!=0x71 && ucScriptType!=0x72)
		return(HXEMV_CORE);
    if(ucScriptType == 0x71) {
		iSingleLen = iTlvGetObj(gl_sTlvDbIssuer, TAG_71_ScriptTemplate1, &psSingle);
		iPackageLen = iTlvGetObjValue(gl_sTlvDbIssuer, TAG_DFXX_ScriptPackage71, &psPackage);
    } else {
		iSingleLen = iTlvGetObj(gl_sTlvDbIssuer, TAG_72_ScriptTemplate2, &psSingle);
		iPackageLen = iTlvGetObjValue(gl_sTlvDbIssuer, TAG_DFXX_ScriptPackage72, &psPackage);
    }

    if(iPackageLen<=0 && iSingleLen<=0)
		return(HXEMV_OK); // 无指定类型的脚本

	iProcessedPackageLen = 0;
	while(iPackageLen>0 || iSingleLen>0) {
        if(iSingleLen > 0) {
            psTemplate = psSingle;
            iTemplateLen = iSingleLen;
        } else {
            psTemplate = psPackage;
    		iTemplateLen = iTlvCheckTlvObject(psTemplate);
        }
        if(iSingleLen<=0 && (iTemplateLen<0 || iTemplateLen>iPackageLen)) {
		    ASSERT(0);
			vTraceWriteTxt(TRACE_ERR_MSG, "脚本Package格式错");
			return(HXEMV_OK); // iTemplateLen必须合法, 否则停止执行脚本
        }

		// 获取脚本标识
		strcpy(sRoute, "\xFF""\x9F\x18");
		sRoute[0] = ucScriptType;
		iRet = iTlvSearchObjValue(psTemplate, (ushort)iTemplateLen, 0, sRoute, &psScriptId);
		if(iRet != 4)
			psScriptId = NULL; // 如果无脚本标识或脚本标识长度不合法, 忽略脚本标识

		// 检查脚本合法性, Refer to Test Case V2CJ1940101v4
		//     脚本中只能存在脚本标识Tag(T9F18)与脚本指令(T86)
		iErrFlag = 0; // 0:no error 1:error
		for(i=0; ; i++) {
			sRoute[0] = ucScriptType;
			strcpy(sRoute+1, "\xFF");
			iRet = iTlvSearchObj(psTemplate, (ushort)iTemplateLen, i, sRoute, &p);
			if(iRet == TLV_ERR_NOT_FOUND)
				break;
			if(iRet < 0) {
				// 脚本完整性错, 忽略该脚本
				iErrFlag = 1;
				break;
			}
			if(memcmp(p, TAG_9F18_ScriptId, 2) && memcmp(p, TAG_86_IssuerScriptCmd, 1)) {
				// 存在不识别的Tag, 忽略该脚本
				iErrFlag = 1;
				break;
			}
		}

		if(iErrFlag || iSingleLen>254 || (iSingleLen<=0 && (iProcessedPackageLen+iTemplateLen>254))) {
			// 脚本长度限制为254字节(Modified 2013.2.19, 原来长度限制为128, 更改为长度限制为254)
			// (DFF1/DFF2最长支持255字节脚本, 但案例要求测试超长情况, 因策设置最大长度为254, 以供传输255字节这个超长长度来测试)
			// 脚本格式错 或者 脚本长度超限
			// 检查脚本长度限制, refer to test case 2CO.034.02 and 2CO.034.03
			// 由于规范中没有做更多描述, 暂按案例提供的脚本完成检查代码, 即处理71脚本时只累计71脚本长度, 即处理72脚本时只累计72脚本长度
			// 脚本长度超限, 不执行该脚本
			iEmvSetItemBit(EMV_ITEM_TSI, TSI_05_SCRIPT_PERFORMED, 1); // 设置TSI响应位

			iRet = iTlvGetObjValue(gl_sTlvDbTermVar, TAG_DF31_ScriptResult, &p);
			if(iRet < 0) {
				// 之前无脚本执行结果
				iRet = 0;
			} else {
				// 之前有脚本执行结果
				if(iRet+5 > sizeof(sBuf)) {
				    ASSERT(0);
					return(HXEMV_CORE); // sBuf缓冲区不足
				}
				memcpy(sBuf, p, iRet);
			}
			sBuf[iRet] = 0x00; // 按照案例要求, 填0x00
			if(psScriptId)
				memcpy(sBuf+iRet+1, psScriptId, 4);
			else
				memset(sBuf+iRet+1, 0, 4); // 无脚本标识, 填0
			iRet = iTlvMakeAddObj(gl_sTlvDbTermVar, TAG_DF31_ScriptResult, iRet+5, sBuf, TLV_CONFLICT_REPLACE);
        	ASSERT(iRet >= 0);
			if(iRet < 0)
				return(HXEMV_CORE);

			if(ucScriptType == 0x71)
				iEmvSetItemBit(EMV_ITEM_TVR, TVR_34_SCRIPT_FAILED_BEFORE_AC, 1); // 脚本71执行失败
			else
				iEmvSetItemBit(EMV_ITEM_TVR, TVR_35_SCRIPT_FAILED_AFTER_AC, 1); // 脚本72执行失败

            if(iSingleLen > 0) {
                // 本次脚本执行的是T7?脚本
                iSingleLen = 0;
            } else {
                // 本次脚本执行的是T7?脚本
    			psPackage += iTemplateLen;
	    		iPackageLen -= iTemplateLen;
		    	iProcessedPackageLen += iTemplateLen;
            }
			continue;
		}
		
		// 依次执行脚本
		strcpy(sRoute, "\xFF""\x86");
		sRoute[0] = ucScriptType;
		ucScriptResult = 0; // 脚本执行结果 高4位 0:未处理 1:处理失败 2:处理成功
		                    //              低4位 0:未指定 1-14:脚本序号 15:15或以上的脚本序号
    	vPushApduPrepare(PUSHAPDU_SCRIPT_EXEC, iTemplateLen, psTemplate); // add by yujun 2012.10.29, 支持优化pboc2.0推送apdu
	    for(i=0; ; i++) {
			iT86ValueLen = iTlvSearchObjValue(psTemplate, (ushort)iTemplateLen, (ushort)i, sRoute, &pT86Value);
			if(iT86ValueLen < 0)
				break;
			iEmvSetItemBit(EMV_ITEM_TSI, TSI_05_SCRIPT_PERFORMED, 1); // 至少一个脚本被执行, 设置TSI响应位
			ucScriptResult = 0x20; // 假设执行成功
            uiRet = uiEmvCmdExchangeApdu(iT86ValueLen, pT86Value, &uiApduOutLen, sBuf);
			vTraceWriteLastApdu("Apdu脚本指令");
			if(uiRet!=0 && (uiRet&0xFF00)!=0x6200 && (uiRet&0xFF00)!=0x6300) {
				// 脚本执行出错
	            i ++;
		        if(i > 15)
			        i = 15;
				ucScriptResult = 0x10 + i;
				if(ucScriptType == 0x71)
					iEmvSetItemBit(EMV_ITEM_TVR, TVR_34_SCRIPT_FAILED_BEFORE_AC, 1); // 脚本71执行失败
				else
					iEmvSetItemBit(EMV_ITEM_TVR, TVR_35_SCRIPT_FAILED_AFTER_AC, 1); // 脚本72执行失败
				break;
		    }
		}
		if(ucScriptResult != 0) {
			// 至少一个脚本指令被执行, 保存执行结果
			// 脚本执行完成, 修正脚本执行结果TLV对象, 因为可能有多个脚本, 需要取出之前可能的脚本执行结果, 将本次脚本执行结果添加进去
			iRet = iTlvGetObjValue(gl_sTlvDbTermVar, TAG_DF31_ScriptResult, &p);
			if(iRet < 0) {
				// 之前无脚本执行结果
				iRet = 0;
			} else {
				// 之前有脚本执行结果
				if(iRet+5 > sizeof(sBuf)) {
				    ASSERT(0);
					return(HXEMV_CORE); // sBuf缓冲区不足
				}
				memcpy(sBuf, p, iRet);
			}
			sBuf[iRet] = ucScriptResult;
			if(psScriptId)
				memcpy(sBuf+iRet+1, psScriptId, 4);
			else
				memset(sBuf+iRet+1, 0, 4); // 无脚本标识, 填0
			iRet = iTlvMakeAddObj(gl_sTlvDbTermVar, TAG_DF31_ScriptResult, iRet+5, sBuf, TLV_CONFLICT_REPLACE);
        	ASSERT(iRet >= 0);
			if(iRet < 0)
				return(HXEMV_CORE);
		}
        if(iSingleLen > 0) {
            // 本次脚本执行的是T7?脚本
            iSingleLen = 0;
        } else {
            // 本次脚本执行的是T7?脚本
			psPackage += iTemplateLen;
       		iPackageLen -= iTemplateLen;
	    	iProcessedPackageLen += iTemplateLen;
        }
	} // while(iPackageLen > 0

	return(HXEMV_OK);
}

// 第二次GAC
// in  : pszArc            : 应答码[2], NULL或""表示联机失败
//       pszAuthCode	   : 授权码[6], NULL或""表示无授权码数据
//       psIssuerData      : 联机响应数据(55域内容),一组TLV格式数据, NULL表示无联机响应数据
//       iIssuerDataLen    : 联机响应数据长度
// out : piCardAction      : 卡片执行结果
//                           GAC_ACTION_TC     : 批准(生成TC)
//							 GAC_ACTION_AAC    : 拒绝(生成AAC)
//                           GAC_ACTION_AAC_ADVICE : 拒绝(生成AAC,有Advice)
//                           GAC_ACTION_ARQC   : 要求联机(生成ARQC)
// ret : HXEMV_OK          : OK
//       HXEMV_CORE        : 其它错误
//       HXEMV_CARD_OP     : 卡操作错
//       HXEMV_CARD_SW     : 非法卡状态字
//       HXEMV_TERMINATE   : 满足拒绝条件，交易终止
//       HXEMV_NOT_ALLOWED : 服务不允许
//       HXEMV_NOT_ACCEPTED: 不接受
int iEmvGAC2(uchar *pszArc, uchar *pszAuthCode, uchar *psIssuerData, int iIssuerDataLen, int *piCardAction)
{
	uint  uiRet;
	int   iRet;
	uchar szArc[2+1]; // 应答码
	uchar *p;
	int   iCdol2Len;
	uchar *psCdol2;
	int   iReqCdaFlag; // 要求CDA标志, 0:不要CDA, 1:要CDA
	int   iTermAction; // 终端行为, 1:拒绝 3:脱机
	uchar ucP1; // GAC指令参数P1
	uchar ucDataInLen, sDataIn[256];
	uchar ucDataOutLen, sDataOut[256];
	uchar ucCid;
	uchar ucCidIllegalFlag; // CID非法标志 0:CID正常 1:CID非法(请求AAC返回了非AAC, 请求TC返回了非TC且非AAC)
	uchar sBuf[260];

	if(psIssuerData) {
		// 保存55域发卡行返回数据
		iRet = iTlvBatchAddField55Obj(1/*添加所有对象*/, gl_sTlvDbIssuer, psIssuerData, iIssuerDataLen, TLV_CONFLICT_ERR);
		if(iRet==TLV_ERR_BUF_SPACE || iRet==TLV_ERR_BUF_FORMAT) {
		    ASSERT(0);
			return(HXEMV_CORE);
		}
		if(iRet < 0) {
			// 发卡行数据有误, 认为联机失败
			pszArc = NULL;
			pszAuthCode = NULL;
			psIssuerData = NULL;
			iIssuerDataLen = 0;
			iTlvDelObj(gl_sTlvDbIssuer, TAG_91_IssuerAuthData);
			iTlvDelObj(gl_sTlvDbIssuer, TAG_DFXX_ScriptPackage71);
			iTlvDelObj(gl_sTlvDbIssuer, TAG_DFXX_ScriptPackage72);
		} 
	}
	// 判断终端行为
	memset(szArc, 0, 3);
	if(pszArc)
		if(pszArc[0])
			memcpy(szArc, pszArc, 2); // 联机成功, 后台返回ARC
	if(szArc[0] == 0) {
		// 联机失败, 判断终端缺省行为
		iTermAction = iTermActionAnalysis(1/*1:判断终端缺省行为*/); // 0:出错 1:拒绝 3:脱机
		if(iTermAction <= 0) {
		    ASSERT(0);
			return(HXEMV_CORE);
		}
		if(iTermAction == 3) {
			// 终端缺省行为允许完成交易
			if(iEmvTermCommunication() == TERM_COM_OFFLINE_ONLY)
				strcpy(szArc, "Y1"); // offline approved Y1
			else
				strcpy(szArc, "Y3"); // Unable to go online, offline approved Y3
		} else {
			// 终端缺省行为要求拒绝交易
			if(iEmvTermCommunication() == TERM_COM_OFFLINE_ONLY)
				strcpy(szArc, "Z1"); // offline declined Z1
			else
				strcpy(szArc, "Z3"); // Unable to go online, offline declined Z3
		}
	} else {
		// 联机成功, 根据发卡行ARC决定终端行为(注ARC=="Z1" || ARC=="Y1"时并没有联机)
		//   第五部分.卡片规范 (16.6, P55) & (表A1, P76)
		//   授权响应码为00，10 或11 表明发卡行接受交易
		//   授权响应码为01 或02 表明发卡行请求参考
		//   其它值表明发卡行拒绝，卡片按照终端请求交易拒绝进行处理
		if(strcmp(szArc, "00")==0 || strcmp(szArc, "10")==0 || strcmp(szArc, "11")==0 || strcmp(szArc, "Y1")==0)
			iTermAction = 3; // 接受
		else if(strcmp(szArc, "01")==0 || strcmp(szArc, "02")==0) {
			// 发卡行请求参考, 核心不支持参考, 根据设置判断是接受还是拒绝
			iRet = iTlvGetObjValue(gl_sTlvDbTermFixed, TAG_DFXX_VoiceReferralSupport, &p); // 1:支持 2:不支持,缺省approve 3:不支持,缺省decline
			if(iRet != 1)
				return(HXEMV_CORE); // 发卡行参考缺省设置必须存在
			if(*p == 2)
				iTermAction = 3; // 接受
			else if(*p == 3)
				iTermAction = 1; // 拒绝
			else {
            	ASSERT(0);
				return(HXEMV_CORE); // 核心不支持发卡行参考
			}

		} else
			iTermAction = 1; // 拒绝
	}

	// 保存发卡行数据
	// 保存ARC
	iTlvMakeAddObj(gl_sTlvDbIssuer, TAG_8A_AuthRspCode, 2, szArc, TLV_CONFLICT_REPLACE);
	// 保存授权码
	if(pszAuthCode) {
		if(strlen(pszAuthCode) == 6)
			iTlvMakeAddObj(gl_sTlvDbIssuer, TAG_89_AuthCode, 6, pszAuthCode, TLV_CONFLICT_REPLACE);
	}

	// 外部认证处理
	if(iEmvTestItemBit(EMV_ITEM_AIP, AIP_05_ISSUER_AUTH_SUPPORED)) {
		// AIP指示支持发卡行认证
		iRet = iTlvGetObjValue(gl_sTlvDbIssuer, TAG_91_IssuerAuthData, &p);
		if(iRet > 0) {
			// 发卡行返回了认证数据
			iEmvSetItemBit(EMV_ITEM_TSI, TSI_03_ISSUER_AUTH_PERFORMED, 1); // 设置TSI发卡行认证执行位
			uiRet = uiEmvCmdExternalAuth((uchar)iRet, p);
			if(uiRet == 1)
				return(HXEMV_CARD_OP);
			if(uiRet) {
				// 外部认证失败, 设置TVR外部认证失败位, Refer to Emv2008 book3, Annex F, P177
                //                                               Emv2008 book3, 10.9, P120
				iEmvSetItemBit(EMV_ITEM_TVR, TVR_33_ISSUER_AUTH_FAILED, 1);
			}
		}
	}

	// Script71 处理
	iRet = iEmvScriptProc(0x71); // 0x71表示处理脚本71
	if(iRet)
		return(iRet);

	// 准备数据
	// 随机数
	vGetRand(sBuf, 4);
	iTlvMakeAddObj(gl_sTlvDbTermVar, TAG_9F37_TermRand, 4, sBuf, TLV_CONFLICT_REPLACE);
	// 在CDOL2中搜索TC Hash Value域, 如果存在, 准备TC Hash Value
	iCdol2Len = iTlvGetObjValue(gl_sTlvDbCard, TAG_8D_CDOL2, &psCdol2);
	ASSERT(iCdol2Len > 0);
	if(iCdol2Len <= 0)
		return(HXEMV_CORE); // CDOL2必须存在
    iRet =  iTlvSearchDOLTag(psCdol2, iCdol2Len, TAG_98_TCHashValue, NULL);
	if(iRet > 0) {
		// CDOL2要求TC Hash Value, 生成TC Hash Value
		iRet = iGenTCHashValue();
		if(iRet)
			return(iRet);
	}

	iReqCdaFlag = 1; // 首先假设要求CDA
	if(iEmvTestItemBit(EMV_ITEM_AIP, AIP_07_CDA_SUPPORTED)==0
			|| iEmvTestItemBit(EMV_ITEM_TERM_CAP, TERM_CAP_20_CDA)==0)
		iReqCdaFlag = 0; // 只有终端与卡片都支持CDA时才执行CDA
	if(iEmvTestItemBit(EMV_ITEM_TVR, TVR_05_CDA_FAILED))
		iReqCdaFlag = 0; // CDA准备阶段已经失败, 不执行CDA

	switch(iTermAction) {
	case 1: // Denial
        ucP1 = 0x00; // require AAC
		if(iReqCdaFlag)
			iReqCdaFlag = 0; // 请求AAC不执行CDA
		break;
	case 3: // Accept
		ucP1 = 0x40; // require TC (01XX XXXX)
		break;
	}
	if(iReqCdaFlag)
		ucP1 |= 0x10; // CDA request (XXX1 XXXX)

	if(iReqCdaFlag)
		iEmvSetItemBit(EMV_ITEM_TSI, TSI_00_DATA_AUTH_PERFORMED, 1); // 如果请求了CDA, 设置TSI脱机数据认证执行标志

    // 打包CDOL2目标串
	iRet = iTlvMakeDOLBlock(sDataIn/*CDOL2目标串*/, sizeof(sDataIn), psCdol2, iCdol2Len,
          				    gl_sTlvDbTermFixed, gl_sTlvDbTermVar, gl_sTlvDbCard, gl_sTlvDbIssuer);
	ASSERT(iRet >= 0);
	if(iRet < 0)
		return(HXEMV_CORE);
	ucDataInLen = iRet;
	iRet = iTlvMakeAddObj(gl_sTlvDbTermVar, TAG_DFXX_Cdol2DataBlock, ucDataInLen, sDataIn, TLV_CONFLICT_REPLACE);
	ASSERT(iRet >= 0);
	if(iRet < 0)
		return(HXEMV_CORE);

	// 执行Generate AC指令
	uiRet = uiEmvCmdGenerateAC(ucP1, ucDataInLen, sDataIn, &ucDataOutLen, sDataOut);
	if(uiRet == 1)
		return(HXEMV_CARD_OP);
	if(uiRet)
		return(HXEMV_CARD_SW);
	iEmvSetItemBit(EMV_ITEM_TSI, TSI_02_CARD_RISK_PERFORMED, 1); // IC卡风险管理已执行

	// 提取GAC数据
	iRet = iGetACData(ucP1, sDataOut, ucDataOutLen);
	if(iRet)
		return(iRet);

	// check CID, refer to Emv2008 book3 9.3 P88
	ucCidIllegalFlag = 0; // 首先假设CID合法
    iTlvGetObjValue(gl_sTlvDbCard, TAG_9F27_CID, &p);
	if((ucP1&0xC0) == 0x00) {
		// require AAC
		if((p[0]&0xC0) != 0x00) {
			// 在请求AAC时没有返回AAC
			ucCidIllegalFlag = 1; // CID非法
		}
	} else {
		// require TC
		if((p[0]&0xC0)!=0x40 && (p[0]&0xC0)!=0x00) {
			// 在请求TC时没有返回TC也没有返回AAC
			ucCidIllegalFlag = 1; // CID非法
		}
	}

	iRet = iTlvGetObjValue(gl_sTlvDbCard, TAG_9F27_CID, &p);
	ASSERT(iRet == 1);
	if(iRet != 1)
		return(HXEMV_CORE); // CID必须存在且合法
	ucCid = *p;

	if(iReqCdaFlag && (ucCid&0xC0)!=0x00 && ucCidIllegalFlag!=1) {
		// 要求CDA验证, 且CID指示没有返回AAC, 且CID没有非法, 执行CDA验证
		iRet = iEmvCda(2/*2:GAC2*/, sDataOut, ucDataOutLen);
		if(iRet == EMV_DAUTH_NATIVE)
			return(HXEMV_CORE);
		if(iRet == EMV_DAUTH_DATA_MISSING)
			return(HXEMV_TERMINATE); // refer Emv bulletin SU-54
		if(iRet) {
			// CDA失败
			iEmvSetItemBit(EMV_ITEM_TVR, TVR_05_CDA_FAILED, 1);
			// 如果CDA失败, CID值可能会改变, 需要重新获取
			iTlvGetObjValue(gl_sTlvDbCard, TAG_9F27_CID, &p);
			ucCid = *p;
			if((ucCid&0xC0) != 0xC0) // 0xC0原表示AAR, 现在为RFU
				*piCardAction = GAC_ACTION_AAC; // 1:拒绝, CDA失败不考虑Advice位
			else
				*piCardAction = GAC_ACTION_AAC; // 拒绝, CDA失败不考虑Advice位, refer to Emv2008 book3 9.3 P88
												// 即使返回了RFU, 第二次GAC也认为是AAC, modified 20131009
			// 不知道CDA失败是否需要执行脚本 ????
			return(HXEMV_OK);
		}
	} else if(iReqCdaFlag && (ucCid&0xC0)==0x00) {
		// 要求CDA验证, 且CID指示返回AAC
		// 分析认为Bulletin N44指明在返回AAC时不认为CDA失败, 但案例2CC.122.00、2CC.122.01要求GAC(with CDA)返回AAC时置位CDA失败位
//		iEmvSetItemBit(EMV_ITEM_TVR, TVR_05_CDA_FAILED, 1); // modified 20130227
	}

	// Script72 处理
	iRet = iEmvScriptProc(0x72); // 0x72表示处理脚本72
	if(iRet)
		return(iRet);

	// 处理cid
	if((ucCid&0x07) == 0x01) {
	    // CID中advice位指示为"Service not allowed", 终止交易, Refer to Emv2008, book4, 6.3.7
		return(HXEMV_NOT_ACCEPTED);
	}

	if((ucCid&0xC0)==0x00 || ucCidIllegalFlag==1) {
		// CID指示为AAC 或 CID非法(此时等同于返回AAC)
		if(ucCid & 0x08)
			*piCardAction = GAC_ACTION_AAC_ADVICE; // 脱机拒绝, 有Advice
		else
			*piCardAction = GAC_ACTION_AAC; // 脱机拒绝, 无Advice
	} else {
		// CID指示返回TC
		*piCardAction = GAC_ACTION_TC; // 接受
	}

	return(HXEMV_OK);
}
