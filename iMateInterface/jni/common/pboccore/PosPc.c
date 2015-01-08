#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "VposFace.h"
#include "PushApdu.h"

uchar gl_szDbgInfo[1024];

// 不再支持以下三个老式接口
APDU_STATUS gl_ApduStatus;
APDU_IN     gl_LastApduIn;
APDU_OUT	gl_LastApduOut;

uchar gl_sLastApduIn[270];
uint  gl_uiLastApduInLen;
uchar gl_sLastApduOut[270];
uint  gl_uiLastApduOutLen;

// 外部传入的读卡器操作函数
#define MAX_IC_SLOT_NUM		10  // 最多支持的卡座数
typedef struct {
	int   iSetFlag;			    // 0:未设置IC卡函数, 按缺省行为进行 1:已设置IC卡函数, 使用设定的函数
	uchar ucCardType;           // 卡片类型, 0:T=0 1:T=1
	int   (*pfiTestCard)(void);
	int   (*pfiResetCard)(uchar *psAtr);
	int   (*pfiDoApdu)(int iApduInLen, uchar *psApduIn, int *piApduOutLen, uchar *psApduOut);
	int   (*pfiCloseCard)(void);
} stIcFunc;
static stIcFunc sg_aIcFunc[MAX_IC_SLOT_NUM];

// 1.1. 初始化POS系统，只能在程序起始处调用一次。
void  _vPosInit(void)
{
	memset(&sg_aIcFunc[0], 0, sizeof(sg_aIcFunc));
	srand(time(NULL));
	gl_uiLastApduInLen = 0;
	gl_uiLastApduOutLen = 0;
	memset(&gl_ApduStatus, 0, sizeof(gl_ApduStatus));
	memset(&gl_LastApduIn, 0, sizeof(gl_LastApduIn));
	memset(&gl_LastApduOut, 0, sizeof(gl_LastApduOut));
}

// 1.2. POS停机，不同的POS可能的表现有：下电、重启动、仅停止运行。
void  _vPosClose()
{
}

// 1.3. 取得POS型号
// 返    回：POS型号
//           目前支持 POS_TYPE_PC POS_TYPE_ST2 POS_TYPE_TALENTO POS_TYPE_ARTEMA
uint _uiGetPosType(void)
{
	return(POS_TYPE_PC);
}

// 2.1. 取屏幕可显示行数
// 返    回：屏幕可显示行数
uint  _uiGetVLines(void)
{
    return(0);
}

// 2.2. 取屏幕可显示列数
// 返    回：屏幕可显示列数
uint  _uiGetVCols(void)
{
    return(0);
}

// 2.3. 清除整个屏幕
void  _vCls(void)
{
}

// 2.4. 在某行显示字符串
// 输入参数：uiLine  : 要显示的行数，从第一行开始
//           pszText : 要显示的字串
// 说    明：如不存在该行，立即返回。如字串长度超出屏幕宽度，截去超出的部分。
void  _vDisp(uint uiLine, const uchar *puszTxt)
{
}

// 2.5. 在某行显示某属性的字符串
// 输入参数：uiLine  : 要显示的行数，从第一行开始
//           pszText : 要显示的字串
//           ucAttr  : 显示属性，可以是DISP_NORMAL、DISP_REVERSE、DISP_UNDERLINE的组合
// 说    明：如不存在该行，立即返回。如字串长度超出屏幕宽度，截去超出的部分。
void  _vDispX(uint uiLine, const uchar *puszTxt, uchar ucAttr)
{
}

// 2.6. 在某行某列显示字符串
// 输入参数：uiLine  : 要显示的行数，从第1行开始
//           uiCol   : 要显示的列数，从第0列开始
//           pszText : 要显示的字串
// 说    明：如不存在该行，立即返回。如字串长度超出屏幕宽度，则绕到下一行。
//           如字串超出屏幕最后一行，截去超出的部分。
void  _vDispAt(uint uiLine, uint uiCol, const uchar *puszTxt)
{
}

// 2.7. 在某行某列带属性显示字符串
// 输入参数：uiLine  : 要显示的行数，从第1行开始
//           uiCol   : 要显示的列数，从第0列开始
//           pszText : 要显示的字串
//           ucAttr  : 显示属性，可以是DISP_NORMAL、DISP_REVERSE、DISP_UNDERLINE的组合
// 说    明：如不存在该行，立即返回。如字串长度超出屏幕宽度，则绕到下一行。
//           如字串超出屏幕最后一行，截去超出的部分。
void  _vDispAtX(uint uiLine, uint uiCol, const uchar *puszTxt, uchar ucAttr)
{
}

// 2.8. 备份某行内容
// 输入参数：uiLine  : 要备份的行号
// 返    回：0       : 成功备份
//           1       : err
// 说    明：将指定的行内容连同属性保存起来
uint _uiBackupLine(uint uiLine)
{
    return(1);
}

// 2.9. 恢复最后一次行备份
// 说    明：将最后一次备份的行恢复原样
void _vRestoreLine(void)
{
}

// 2.10. 屏幕内容备份
// 返    回：0       : 成功
//           1       : err
uint _uiBackupScreen(void)
{
    return(1);
}

// 2.11. 屏幕内容恢复
// 返    回：0       : 成功
//           1       : err
void _vRestoreScreen(void)
{
}

// 3.1. 检测有否按键，立即返回
// 返    回：0 ：没有按键
//           1 ：检测到按键，可通过_uiGetKey()读取
uint  _uiKeyPressed(void)
{
    return(0);
}

// 3.4. 重新定义一个按键，使该键返回另外一个键码
// 输入参数：ucSource : 键盘上真正存在的键码
//           ucTarget : 重定义后该建返回的键码, 如果该键码与ucSource相同，则删除该键原先的定义
// 返    回：0        : 成功
//           1        : 重定义的键码太多
// 说    明：最多只能重定义10个键码
uint  _uiMapKey(uchar ucSource, uchar ucTarget)
{
    return(0);
}

// 3.2. 等待按键，直到读取到一个按键为止
// 返    回：0x00 - 0xff ：按键的ASCII码
uint  _uiGetKey(void)
{
    return(0);
}

// 3.3. 清除键盘缓冲区
void  _vFlushKey(void)
{
}

// 4.1. 检测卡片是否插入
// 输入参数：uiReader : 虚拟卡座号
// 返    回：0        : 无卡
//           1        : 有卡
// 说    明：只能检测用户卡座
uint  _uiTestCard(uint uiReader)
{
	int iRet;

	if(uiReader >= MAX_IC_SLOT_NUM)
		return(0);

	if(sg_aIcFunc[uiReader].iSetFlag) {
		// 使用传入的IC卡操作函数
		iRet = sg_aIcFunc[uiReader].pfiTestCard();
	} else {
		// 使用原IC卡操作函数
		iRet = 0;
	}
	if(iRet)
		return(1);
    return(0);
}

// 根据ATR判断卡片通讯协议
// in  : iAtrLen : ATR长度
//       psAtr   : ATR数据
// ret : 0       : T=0
//       1       : T=1
static uchar ucGetCardCommType(uchar iAtrLen, uchar *psAtr)
{
         uchar i, T0;
         i=1;
         T0 = psAtr[i];
         if(T0&0x10) i++;
         if(T0&0x20) i++;
         if(T0&0x40) i++;
         if(T0&0x80)
         {
                   i++;
                   return (psAtr[i]&0x0f);    //TD1         T=1
         }
         return 0;
}

// 4.2. 卡片复位
// 输入参数：uiReader    : 虚拟卡座号
// 输出参数：psResetData : ATR 数据
// 返    回：0           : 复位失败
//           >0          : 复位成功，返回ATR长度
uint  _uiResetCard(uint uiReader, uchar *pusResetData)
{
    int   iAtrLen;

	if(uiReader >= MAX_IC_SLOT_NUM)
		return(0);

	if(sg_aIcFunc[uiReader].iSetFlag) {
		// 使用传入的IC卡操作函数
		iAtrLen = sg_aIcFunc[uiReader].pfiResetCard(pusResetData);
	} else {
		// 使用原IC卡操作函数
		iAtrLen = 0;
	}
    if(iAtrLen) {
        // 复位成功, 判断卡片类型
		sg_aIcFunc[uiReader].ucCardType = ucGetCardCommType((uchar)iAtrLen, pusResetData);
		if(sg_aIcFunc[uiReader].ucCardType!=0 && sg_aIcFunc[uiReader].ucCardType!=1)
			return(0);
    }
	return(iAtrLen);
}

// 4.3. 卡片下电
// 输入参数：uiReader    : 虚拟卡座号
// 返    回：0           : 成功
//           1           : 失败
uint  _uiCloseCard(uint uiReader)
{
	int iRet;

	if(uiReader >= MAX_IC_SLOT_NUM)
		return(0);

	if(sg_aIcFunc[uiReader].iSetFlag) {
		// 使用传入的IC卡操作函数
		iRet = sg_aIcFunc[uiReader].pfiCloseCard();
	} else {
		// 使用原IC卡操作函数
		iRet = 0;
	}

    return(0);
}

// 4.4. 执行IC卡指令
// 输入参数：uiReader : 虚拟卡座号
//           pIn      : IC卡指令结构
//           pOut     : IC卡返回结果
// 返    回：0        : 成功
//           1        : 失败
uint  _uiExchangeApdu(uint uiReader, APDU_IN *pIn, APDU_OUT *pOut)
{
	if(uiReader >= MAX_IC_SLOT_NUM)
		return(1);

    return(1);
}

// 4.5. 执行IC卡指令
// 输入参数：uiReader   : 虚拟卡座号
//           uiInLen    : Command Apdu指令长度
//           psIn       : Command APDU, 标准case1-case4指令结构
//           uiProtocol : 协议类型
//                        APDU_NORMAL        标准APDU, 不自动处理61xx、6Cxx
//                        APDU_AUTO_GET_RESP 标准APDU, 自动处理61xx、6Cxx
//                        APDU_EMV           执行EMV协议
//                        APDU_SI            执行社保2.0协议
//           puiOutLen  : Response APDU长度
// 输出参数：psOut      : Response APDU, RespData[n]+SW[2]
// 返    回：0          : 成功
//           1          : 失败
uint _uiDoApdu(uint uiReader, uint uiInLen, uchar *psIn, uint *puiOutLen, uchar *psOut, uint uiProtocol)
{
	uchar sApduIn[300], sApduOut[300];
	uint  uiApduInLen, uiApduOutLen;
    uint  uiOriginalRet;
	uint  uiStatus;
    uint  uiRet;
    uint  uiCase; // apdu case 1 - 4

	gl_uiLastApduInLen = 0;
	gl_uiLastApduOutLen = 0;
	if(uiInLen > sizeof(gl_sLastApduIn))
		return(1);
	gl_uiLastApduInLen = uiInLen;
	memcpy(gl_sLastApduIn, psIn, uiInLen);

	if(uiReader >= MAX_IC_SLOT_NUM)
		return(1);

	uiRet = uiPushDoApdu(uiReader, uiInLen, psIn, puiOutLen, psOut);
	if(uiRet == 1)
		return(1);
	if(uiRet == 0) {
		if(*puiOutLen <= sizeof(gl_sLastApduOut)) {
			gl_sLastApduIn[0] |= 0x01; // Class最低位为1表示该指令为批处理推送指令
			gl_uiLastApduOutLen = *puiOutLen;
			memcpy(gl_sLastApduOut, psOut, *puiOutLen);
		}
		return(0);
	}

	// 优化apdu推送指令缓冲区无内容, 使用单指令方式
	if(uiInLen > sizeof(sApduIn))
		return(1);
	// 判断case
    if(uiInLen == 4)
        uiCase = 1; // case 1
    else if(uiInLen == 5)        
        uiCase = 2; // case 2
    else if((int)uiInLen == 5+psIn[4])
        uiCase = 3; // case 3
    else if((int)uiInLen == 5+psIn[4]+1)
        uiCase = 4; // case 4
    else
        return(1); // error format

	uiApduInLen = uiInLen;
	memcpy(sApduIn, psIn, uiApduInLen);
	uiApduOutLen = 0;
	memset(sApduOut, 0, sizeof(sApduOut));
	if(sg_aIcFunc[uiReader].iSetFlag) {
		// 使用传入的IC卡操作函数
		uiRet = sg_aIcFunc[uiReader].pfiDoApdu(uiApduInLen, sApduIn, &uiApduOutLen, sApduOut);
	} else {
		// 使用原IC卡操作函数
		uiRet = 1;
	}
	if(uiRet)
		return(1); // apdu指令执行失败
	if(uiApduOutLen<2 || uiApduOutLen>sizeof(sApduOut))
		return(1); // apdu返回异常
	uiStatus = (uint)ulStrToLong(sApduOut+uiApduOutLen-2, 2);
	switch(uiProtocol) {
    case APDU_AUTO_GET_RESP: // 自动处理61xx、6Cxx
        if((uiStatus&0xFF00) == 0x6100) {
            memcpy(sApduIn, "\x00\xc0\x00\x00", 4);
			sApduIn[4] = uiStatus&0xFF;
			uiInLen = 5;
			if(sg_aIcFunc[uiReader].iSetFlag) {
				// 使用传入的IC卡操作函数
				uiRet = sg_aIcFunc[uiReader].pfiDoApdu(uiInLen, sApduIn, &uiApduOutLen, sApduOut);
			} else {
				// 使用原IC卡操作函数
				uiRet = 1;
			}
        } else if((uiStatus&0xFF00) == 0x6C00) {
			sApduIn[uiApduInLen-1] = uiStatus&0xFF;
			if(sg_aIcFunc[uiReader].iSetFlag) {
				// 使用传入的IC卡操作函数
				uiRet = sg_aIcFunc[uiReader].pfiDoApdu(uiApduInLen, sApduIn, &uiApduOutLen, sApduOut);
			} else {
				// 使用原IC卡操作函数
				uiRet = 1;
			}
        }
        break;
    case APDU_EMV:           // EMV规范
        uiOriginalRet = 0;
        if((uiStatus&0xFF00) == 0x6100) {
            memcpy(sApduIn, "\x00\xc0\x00\x00", 4);
			sApduIn[4] = uiStatus&0xFF;
			uiInLen = 5;
			if(sg_aIcFunc[uiReader].iSetFlag) {
				// 使用传入的IC卡操作函数
				uiRet = sg_aIcFunc[uiReader].pfiDoApdu(uiInLen, sApduIn, &uiApduOutLen, sApduOut);
			} else {
				// 使用原IC卡操作函数
				uiRet = 1;
			}
        } else if((uiStatus&0xFF00) == 0x6C00) {
			sApduIn[uiApduInLen-1] = uiStatus&0xFF;
			if(sg_aIcFunc[uiReader].iSetFlag) {
				// 使用传入的IC卡操作函数
				uiRet = sg_aIcFunc[uiReader].pfiDoApdu(uiApduInLen, sApduIn, &uiApduOutLen, sApduOut);
			} else {
				// 使用原IC卡操作函数
				uiRet = 1;
			}
        } else if((uiStatus&0xFF00) == 0x6300 || (uiStatus&0xFF00) == 0x6200 || ((uiStatus&0xF000)==0x9000 && uiStatus!=0x9000)) {
            if(sg_aIcFunc[uiReader].ucCardType==1 || uiCase!=4)
	     		break; // T=1卡不需要做额外处理, case4以外不需要做额外处理
	        // T=0卡 and Case 4
			// 62XX, 63XX, 9XXX(Not9000), 需要读取数据, Refer to EMV2008 book1 9.3, P106
			//                                                 book1 9.3.1.1.4 4.(b)
	        uiOriginalRet = uiStatus;
            memcpy(sApduIn, "\x00\xc0\x00\x00\x00", 5);
			uiInLen = 5;
			if(sg_aIcFunc[uiReader].iSetFlag) {
				// 使用传入的IC卡操作函数
				uiRet = sg_aIcFunc[uiReader].pfiDoApdu(uiInLen, sApduIn, &uiApduOutLen, sApduOut);
			} else {
				// 使用原IC卡操作函数
				uiRet = 1;
			}
			if(uiRet)
				return(1);
			if(uiApduOutLen<2 || uiApduOutLen>sizeof(sApduOut))
				return(1); // apdu返回异常
			uiStatus = (uint)ulStrToLong(sApduOut+uiApduOutLen-2, 2);
  	        if((uiStatus&0xFF00)==0x6C00 || (uiStatus&0xFF00)==0x6100) {
				sApduIn[uiInLen-1] = uiStatus&0xFF;
				if(sg_aIcFunc[uiReader].iSetFlag) {
					// 使用传入的IC卡操作函数
					uiRet = sg_aIcFunc[uiReader].pfiDoApdu(uiInLen, sApduIn, &uiApduOutLen, sApduOut);
				} else {
					// 使用原IC卡操作函数
					uiRet = 1;
				}
		    	if(uiRet)
    		    	return(1);
			}
            uiStatus = uiOriginalRet; // 恢复62XX 63XX 9XXX返回码
			vLongToStr((ulong)uiStatus, 2, sApduOut+uiApduOutLen-2);
        }
        break;
    case APDU_NORMAL:        // 不处理61xx、6Cxx
    case APDU_SI:            // 社保2.0协议, 暂不支持, 等同于不处理61xx、6Cxx
    default:                 // 未知协议, 等同于不处理61xx、6Cxx
        break;
    } // switch(uiProtocol)

	if(uiRet)
		  return(1);
	*puiOutLen = uiApduOutLen;
	memcpy(psOut, sApduOut, uiApduOutLen);
	if(*puiOutLen <= sizeof(gl_sLastApduOut)) {
		gl_uiLastApduOutLen = *puiOutLen;
		memcpy(gl_sLastApduOut, psOut, *puiOutLen);
	}
    return(0);
}

// 4.6. 设置读卡函数, 用于外部提供IC卡接口时
// 输入参数: uiReader     : 卡座号, 0-9, 以后对该卡座号的操作将用所设置的函数进行
//           以下4个函数如果有一个传入NULL, 则表示取消之前对该卡座设置的函数
//           pfiTestCard  : 检测卡片是否存在函数指针
//           pfiResetCard : 卡片复位函数指针
//           pfiDoApdu    : 执行Apdu指令函数指针
//           pfiCloseCard : 关闭卡片函数指针
// 返    回: 0            : OK
//           1            : 失败
uint _uiSetCardCtrlFunc(uint uiReader,
					    int (*pfiTestCard)(void),
				        int (*pfiResetCard)(uchar *psAtr),
				        int (*pfiDoApdu)(int iApduInLen, uchar *psApduIn, int *piApduOutLen, uchar *psApduOut),
				        int (*pfiCloseCard)(void))
{
	if(uiReader >= MAX_IC_SLOT_NUM)
		return(1);

	if(pfiTestCard && pfiResetCard && pfiDoApdu && pfiCloseCard) {
		// 设定函数
		sg_aIcFunc[uiReader].iSetFlag = 1;
		sg_aIcFunc[uiReader].pfiTestCard = pfiTestCard;
		sg_aIcFunc[uiReader].pfiResetCard = pfiResetCard;
		sg_aIcFunc[uiReader].pfiDoApdu = pfiDoApdu;
		sg_aIcFunc[uiReader].pfiCloseCard = pfiCloseCard;
	} else {
		// 取消设定函数
		memset(&sg_aIcFunc[uiReader], 0, sizeof(sg_aIcFunc[uiReader]));
	}
	return(0);
}

// 5.1. 取打印机宽度
// 返    回：0  : 无打印机
//           >0 : 打印机可打印的最大宽度
uint  _uiGetPCols(void)
{
    return(0);
}

// 5.2. 打印数据
// 输入参数：pszText : 要打印的字串
// 返    回：0 : 打印成功
//           1 : 打印错误
// 说    明：打印字串超过打印机最大宽度时反绕到下一行。
//           '\n'字符会导致换行。
uint  _uiPrint(const uchar *puszTxt)
{
    return(1);
}


// 6.1. 设置当前系统时间
// 输入参数：pszTime : 要设置的时间，格式:YYYYMMDDHHMMSS
// 返    回：0       : 成功
//           1       : 失败
uint  _uiSetTime(const uchar *pszTime)
{
    return(1);
}

// 6.2. 读取系统时间
// 输出参数：pszTime : 当前系统时间，格式:YYYYMMDDHHMMSS
void  _vGetTime( uchar *puszTime)
{
    time_t t;
    struct tm tm;
 
    t = time( NULL );
    memcpy(&tm, localtime(&t), sizeof(struct tm));
    sprintf(puszTime, "%04d%02d%02d%02d%02d%02d",
                      tm.tm_year+1900, tm.tm_mon+1, tm.tm_mday,
                      tm.tm_hour, tm.tm_min, tm.tm_sec);
}

void  _vSetTimer(ulong *pulTimer, ulong ulNTick)
{
	*pulTimer = ((rand()<<16L) + rand()); // 不支持Timer, 填入随机数
}

uint  _uiTestTimer(ulong ulTimer)
{
    return(0);
}

// 6.5. 延迟一段时间
// 输入参数：uiNTick : 延迟的时间，单位为0.01秒
void  _vDelay(uint uiNTick)
{
}

// 7.1. 设置扩展内存大小
// 输入参数：ulSize : 要设置的扩展内存大小，单位为字节
// 返    回：0      : 设置成功
//           1      : 参数错误，申请空间必须大于32字节
//           2      : 申请失败
//           >2     : 扩展内存不足，返回值为最大可用扩展内存大小
ulong _ulSetXMemSize(ulong ulSize)
{
    return(2);
}

// 7.2. 读扩展内存
// 输入参数：ulOffset : 扩展内存以字节为单位的偏移量
//           uiLen    ：以字节为单位的长度
// 输出参数：pBuffer  : 读取得数据
// 返    回：0        : 成功
//           1        : 失败
uint  _uiXMemRead(void *pusBuffer, ulong ulOffset, uint uiLen)
{
    return(1);
}

// 7.3. 写扩展内存
// 输入参数：pBuffer  : 要写的数据
//           ulOffset : 扩展内存以字节为单位的偏移量
//           uiLen    ：以字节为单位的长度
// 返    回：0        : 成功
//           1        : 失败
uint  _uiXMemWrite(const void *pusBuffer, ulong ulOffset, uint uiLen)
{
    return(1);
}

// 8.1. 清磁卡缓冲区
// 返    回：0 : 成功
//           1 : 失败
uint _uiMagReset(void)
{
    return(1);
}

// 8.2. 检测是否有磁卡刷过
// 返    回：0 : 无卡
//           1 : 有磁卡刷过
uint _uiMagTest(void)
{
    return(0);
}

// 8.3. 读磁道信息
// 输入参数：uiTrackNo : 磁道号，1-3
// 输出参数：pszBuffer : 磁道信息
// 返    回：0 : 读取信息正确
//           1 : 没有数据
//           2 : 检测到错误
uint _uiMagGet(uint uiTrackNo, uchar *pszBuffer)
{
    return(1);
}

// 8.4. 写磁道信息
// 输入参数：pszTrack2 : 2磁道信息
//         ：pszTrack3 : 3磁道信息
void _vMagWrite(uchar *pszTrack2, uchar *pszTrack3)
{
}

// 9.1. 设置当前串口号
// 输入参数：ucPortNo : 新串口号, 从串口1开始
// 返    回：原先的串口号
uchar _ucAsyncSetPort(uchar ucPortNo)
{
    return(1);
}

// 9.2. 打开当前串口
// 输入参数：ulBaud   : 波特率
//           ucParity : 奇偶校验标志，COMM_EVEN、COMM_ODD、COMM_NONE
//           ucBits   : 数据位长度
//           ucStop   : 停止位长度
// 返    回：0        : 成功
//           1        : 参数错误
//           2        : 失败
uchar _ucAsyncOpen(ulong ulBaud, uchar ucParity, uchar ucBits, uchar ucStop)
{
    return(2);
}

// 9.3. 关闭当前串口
// 返    回：0        : 成功
//           1        : 失败
uchar _ucAsyncClose(void)
{
    return(1);
}

// 9.4. 复位当前串口，清空接收缓冲区
// 返    回：0        : 成功
//           1        : 失败
uchar _ucAsyncReset(void)
{
    return(1);
}

// 9.5. 发送一个字节
// 输入参数：ucChar : 要发送的字符
// 返    回：0      : 成功
//           1      : 失败
uchar _ucAsyncSend(uchar ucChar)
{
    return(1);
}

// 9.6. 检测有没有字符收到
// 返    回：0 : 没有字符收到
//           1 : 有字符收到
uchar _ucAsyncTest(void)
{
    return(0);
}

// 9.7. 接收一个字符，没有则一直等待
// 返    回：接收到的字符
uchar _ucAsyncGet(void)
{
    return(0);
}

// 9.8.	发送一串数据
// 输入参数：pBuf  : 要发送的字符
//           uiLen : 数据长度
// 返    回：0     : 成功
//           1     : 失败
uchar _ucAsyncSendBuf(void *pBuf, uint uiLen)
{
    return(1);
}

// 9.9.	接收指定长度的一串数据
// 输入参数：pBuf      : 要发送的数据
//           uiLen     : 数据长度
//           uiTimeOut : 以秒为单位的超时时间
// 返    回：0         : 成功
//           1         : 超时
uchar _ucAsyncGetBuf(void *pBuf, uint uiLen, uint uiTimeOut)
{
    return(1);
}

// 9.10. 拨号建立链接
// 输入参数：pTelConfig : 拨号结构指针，见 TEL_CONFIG 结构
//           uiTimeOut  : 超时时间，单位为秒
// 返    回：0          : 成功
//           1          : 失败，可用_uiTelGetStatus()取得错误
uchar _ucTelConnect(const TEL_CONFIG *pTelConfig, uint uiTimeOut)
{
    return(1);
}

// 9.11. 拆除链接
// 返    回：0 : 成功
//           1 : 失败，可用_uiTelGetStatus()取得错误
uchar _ucTelDisconnect(void)
{
    return(0);
}

// 9.12. 取回拨号链接当前状态
// 返    回：0x0000 : 正常
//           0x01?? : 参数错误
//           0x02?? : 正常，低位字节表示一种状态
//           0x03?? : 错误，低位字节表示错误原因
uint _uiTelGetStatus(void)
{
    return(0x0399);
}

// 9.13. 拨号线发送数据
// 输入参数：uiLength : 发送报文长度
//           pBuffer  : 发送报文内容
// 返    回：0        : 正常
//           1        : 失败，可用_uiTelGetStatus()取得错误
uchar _ucTelSend(uint uiLength, void const *pBuffer)
{
    return(1);
}

// 9.14. 拨号线接收数据
// 输入参数：puiLength : 接收报文长度
//           pBuffer   : 接收报文内容
//           uiTimeOut : 超时时间，单位为秒
// 返    回：0         : 正常
//           1         : 失败，可用_uiTelGetStatus()取得错误
uchar _ucTelGet(uint *puiLength, void *pBuffer, uint uiTimeOut)
{
    return(1);
}

// 10.1. 发一短声
void  _vBuzzer(void)
{
}

// 10.2. 取一随机数
// 返    回：0-255的随机数
uchar _ucGetRand(void)
{
    return(rand());
}

// 10.3. 做DES运算
// 输入参数：uiMode   : ENCRYPT -> 加密，DECRYPT -> 解密
//                      TRI_ENCRYPT -> 3Des加密，TRI_DECRYPT -> 3Des解密
//           psSource : 源
//           psKey    : 密钥
// 输出参数：psResult : 目的
extern void _fDes(char cryp_decrypt,unsigned char *DES_DATA,unsigned char *DESKEY,unsigned char *DES_RESULT);
void _vDes(uint uiMode, uchar *psSource, uchar *psKey, uchar *psResult)
{
    uchar sBuf[8];
    switch(uiMode) {
        case ENCRYPT:
        case DECRYPT:
            _fDes((char)uiMode, (uchar*)psSource, (uchar*)psKey, psResult);
            break;
        case TRI_ENCRYPT:
            _fDes(ENCRYPT, (uchar*)psSource, (uchar*)psKey, psResult);
            _fDes(DECRYPT, (uchar*)psResult, (uchar*)psKey+8, sBuf);
            _fDes(ENCRYPT, (uchar*)sBuf, (uchar*)psKey, psResult);
            break;
        case TRI_DECRYPT:
            _fDes(DECRYPT, (uchar*)psSource, (uchar*)psKey, psResult);
            _fDes(ENCRYPT, (uchar*)psResult, (uchar*)psKey+8, sBuf);
            _fDes(DECRYPT, (uchar*)sBuf, (uchar*)psKey, psResult);
			break;
    }
}

// 11.1. Debug 使用, 功能：显示文件名与行数
void _vDispAssert(uchar *pszFileName, ulong ulLineNo)
{
}

// 11.2. Debug 使用, 功能：显示行数与pPointer所指内容，内容用16进制数显示6字节
void _vDispDbgInfo(uchar *pszFileName,ulong ulLineNo, void *pPointer)
{
}

// 13. 网络功能函数(也包括了SDLC、异步通讯), 接口参照了VeriFone520 vCommCE接口
// 13.1. 网络初始化
uint _uiCommInit(void)
{
	return(COM_OK);
}

// 13.2 注册网络
// in  : uiCommType  : 网络类型 VPOSCOMM_SDLC or VPOSCOMM_ASYNC or VPOSCOMM_ETHER or VPOSCOMM_GPRS or VPOSCOMM_CDMA
//       pCommConfig : 网络参数
//       uiTimeout   : 超时时间(秒), 0表示不等待,立即返回
// ret : VPOSCOMM_
// Note: 必须先注册网络才可使用网络
uint _uiCommLogin(uint uiCommType, VPOSCOMM_CONFIG *pCommConfig, uint uiTimeout)
{
	return(COM_OK);
}

// 13.2 注销网络
// in  : uiCommType  : 网络类型 VPOSCOMM_SDLC or VPOSCOMM_ASYNC or VPOSCOMM_ETHER or VPOSCOMM_GPRS or VPOSCOMM_CDMA
uint _uiCommLogOut(uint uiCommType)
{
	return(COM_OK);
}

// 13.3 保持注册状态
// in  : uiCommType  : 网络类型 VPOSCOMM_SDLC or VPOSCOMM_ASYNC or VPOSCOMM_ETHER or VPOSCOMM_GPRS or VPOSCOMM_CDMA
// Note: 如果发现丢失了注册, 试图重新注册
uint _uiCommKeepLogin(uint uiCommType)
{
	return(COM_OK);
}

// 13.4 连接服务器
// in  : uiCommType  : 网络类型 VPOSCOMM_SDLC or VPOSCOMM_ASYNC or VPOSCOMM_ETHER or VPOSCOMM_GPRS or VPOSCOMM_CDMA
//       psParam1    : 参数1, 对于VPOSCOMM_SDLC VPOSCOMM_ASYNC : 电话号码
//                            对于VPOSCOMM_ETHER VPOSCOMM_GPRS VPOSCOMM_CDMA : Ip地址
//       psParam2    : 参数2, 对于VPOSCOMM_SDLC VPOSCOMM_ASYNC : 外线号码
//                            对于VPOSCOMM_ETHER VPOSCOMM_GPRS VPOSCOMM_CDMA : 端口号
//       psParam3    : 参数3, 对于VPOSCOMM_SDLC VPOSCOMM_ASYNC : "0"-不检测拨号音 "1"-检测拨号音
//                            对于VPOSCOMM_ETHER VPOSCOMM_GPRS VPOSCOMM_CDMA : "0"-TCP "1"-UDP
//       uiTimeout   : 超时时间(秒), 0表示不等待,立即返回
uint _uiCommConnect(uint uiCommType, uchar *psParam1, uchar *psParam2, uchar *psParam3, uint uiTimeout)
{
    return(COM_ERR_NO_CONNECT); /* can't connect */
}

// 13.5 检测连通性
// in  : uiCommType  : 网络类型 VPOSCOMM_SDLC or VPOSCOMM_ASYNC or VPOSCOMM_ETHER or VPOSCOMM_GPRS or VPOSCOMM_CDMA
uint _uiCommTestConnect(uint uiCommType)
{
	return(COM_ERR_NO_CONNECT);
}

// 13.6 断开连接
// in  : uiCommType  : 网络类型 VPOSCOMM_SDLC or VPOSCOMM_ASYNC or VPOSCOMM_ETHER or VPOSCOMM_GPRS or VPOSCOMM_CDMA
uint _uiCommDisconnect(uint uiCommType)
{
	return(COM_OK);
}

// 13.7 保持连接状态
// in  : uiCommType  : 网络类型 VPOSCOMM_SDLC or VPOSCOMM_ASYNC or VPOSCOMM_ETHER or VPOSCOMM_GPRS or VPOSCOMM_CDMA
uint _uiCommKeepConnect(uint uiCommType)
{
	return(COM_OK);
}

// 13.8 获取网络信息
// in  : uiCommType  : 网络类型 VPOSCOMM_SDLC or VPOSCOMM_ASYNC or VPOSCOMM_ETHER or VPOSCOMM_GPRS or VPOSCOMM_CDMA
// out : pCommInfo   : 网络信息
uint _uiCommGetInfo(uint uiCommType, VPOSCOMM_INFO *pCommInfo)
{
	return(COM_ERR_SYS);
}

// 13.9 发送数据
// in  : uiCommType  : 网络类型 VPOSCOMM_SDLC or VPOSCOMM_ASYNC or VPOSCOMM_ETHER or VPOSCOMM_GPRS or VPOSCOMM_CDMA
//       psSendData  : 发送的数据
//       uiSendLen   : 发送的数据长度
uint _uiCommSendData(uint uiCommType, uchar *psSendData, uint uiSendLen)
{
    return (COM_ERR_SEND);
}

// 13.10 接收数据
// in  : uiCommType  : 网络类型 VPOSCOMM_SDLC or VPOSCOMM_ASYNC or VPOSCOMM_ETHER or VPOSCOMM_GPRS or VPOSCOMM_CDMA
//       piRecvLen   : 接收数据缓冲区大小
//       uiTimeout   : 超时时间(秒)
// out : psRecvData  : 接收的数据
//       piRecvLen   : 接收的数据长度
uint _uiCommRecvData(uint uiCommType, uchar *psRecvData, uint *piRecvLen, uint uiTimeout)
{
    return(COM_ERR_RECV);
}

// 14. 基础函数
// 14.1 将两个串按字节异或
// In       : psVect1  : 目标串
//            psVect2  : 源串
//            iLength  : 字节数
void vXor(uchar *psVect1, const uchar *psVect2, int iLength)
{
    int i;

    for(i=0; i<iLength; i++)
        psVect1[i] ^= psVect2[i];
}

// 14.2 将两个串按字节或
// In       : psVect1  : 目标串
//            psVect2  : 源串
//            iLength  : 字节数
void vOr(uchar *psVect1, const uchar *psVect2, int iLength)
{
    int i;

    for(i=0; i<iLength; i++)
        psVect1[i] |= psVect2[i];
}

// 14.3 将两个串按字节与
// In       : psVect1  : 目标串
//            psVect2  : 源串
//            iLength  : 字节数
void vAnd(uchar *psVect1, const uchar *psVect2, int iLength)
{
    int i;

    for(i=0; i<iLength; i++)
        psVect1[i] &= psVect2[i];
}

// 14.4 将二进制源串分解成双倍长度可读的16进制串
// In       : psIn     : 源串
//            iLength  : 源串长度
// Out      : psOut    : 目标串
void vOneTwo(const uchar *psIn, int iLength, uchar *psOut)
{
    static const uchar aucHexToChar[17] = "0123456789ABCDEF";
    int iCounter;

    for(iCounter = 0; iCounter < iLength; iCounter++){
        psOut[2*iCounter] = aucHexToChar[((psIn[iCounter] >> 4)) & 0x0F];
        psOut[2*iCounter+1] = aucHexToChar[(psIn[iCounter] & 0x0F)];
    }
}

// 14.5 将二进制源串分解成双倍长度可读的16进制串, 并在末尾添'\0'
// In       : psIn     : 源串
//            iLength  : 源串长度
// Out      : pszOut   : 目标串
void vOneTwo0(const uchar *psIn, int iLength, uchar *pszOut)
{
    vOneTwo(psIn, iLength, pszOut);
	if(iLength < 0)
		iLength = 0;
    pszOut[2*iLength]=0;
}

// 14.6 将可读的16进制表示串压缩成其一半长度的二进制串
// In       : psIn     : 源串
//            iLength  : 源串长度
// Out      : psOut    : 目标串
// Attention: 源串必须为合法的十六进制表示，大小写均可
//            长度如果为奇数，函数会靠近到比它大一位的偶数
void vTwoOne(const uchar *psIn, int iLength, uchar *psOut)
{
    uchar ucTmp;
    int   i;

    for(i=0; i<iLength; i+=2) {
        ucTmp = psIn[i];
        if(ucTmp > '9')
            ucTmp = toupper(ucTmp) - 'A' + 0x0a;
        else
            ucTmp &= 0x0f;
        psOut[i/2] = ucTmp << 4;

        ucTmp = psIn[i+1];
        if(ucTmp > '9')
            ucTmp = toupper(ucTmp) - 'A' + 0x0a;
        else
            ucTmp &= 0x0f;
        psOut[i/2] += ucTmp;
    } // for(i=0; i<uiLength; i+=2) {
}

// 14.7 将二进制串转变成长整数
// In       : psBinString : 二进制串，高位在前
//            iLength     : 二进制串长度，可为{1,2,3,4}之一
// Ret      : 转变后的长整数
ulong ulStrToLong(const uchar *psBinString, int iLength)
{
    ulong  l;
    int    i;

    for(i=0, l=0l; i<iLength; i++)
        l = l*256 + (long)psBinString[i];
    return(l);
}

// 14.8 将长整数转变成二进制串
// In       : ulLongNumber : 要转变的长整数
//            iLength      : 转变之后二进制串长度
// Out      : psBinString  : 转变后的二进制串，高位在前
void vLongToStr(ulong ulLongNumber, int iLength, uchar *psBinString)
{
    int i;

    for(i = iLength-1; i>=0; i--, ulLongNumber/=256l )
        psBinString[i] = (uchar)(ulLongNumber % 256);
}

// 14.9 将十六进制可读串转变成长整数
// In       : psHexString : 十六进制串
//            iLength     : 十六进制串长度，0-8
// Ret      : 转变后的长整数
ulong ulHexToLong(const uchar *psHexString, int iLength)
{
    ulong  l;
    int    i;
	ulong  ulDigit;

    for(i=0, l=0l; i<iLength; i++) {
		if(psHexString[i]>='0' && psHexString[i]<='9')
			ulDigit = psHexString[i] - '0';
		else if(psHexString[i]>='A' && psHexString[i]<='Z')
			ulDigit = psHexString[i] - 'A' + 10;
		else if(psHexString[i]>='a' && psHexString[i]<='z')
			ulDigit = psHexString[i] - 'a' + 10;
        l = l*16 + ulDigit;
	}
    return(l);
}

// 14.10 将长整数转变成十六进制可读串
// In       : ulLongNumber : 要转变的长整数
//            iLength      : 转变之后十六进制串长度
// Out      : psHexString  : 转变后的十六进制串
void vLongToHex(ulong ulLongNumber, int iLength, uchar *psHexString)
{
	uchar sBuf[4], szBuf[8];
	vLongToStr(ulLongNumber, 4, sBuf);
	vOneTwo(sBuf, 4, szBuf);
	memset(psHexString, '0', iLength);
	if(iLength > 8)
		memcpy(psHexString+iLength-8, szBuf, 8);
	else
		memcpy(psHexString, szBuf+8-iLength, iLength);
}

// 14.11 将数字串转变成长整数
// In       : psString : 数字串，必须为合法的数字，不需要'\0'结尾
//            iLength  : 数字串长度
// Ret      : 转变后的长整数
ulong ulA2L(const uchar *psString, int iLength)
{
    ulong  l;
    int    i;

    for(i=0, l=0l; i<iLength; i++) {
        if(psString[i]<'0' || psString[i]>'9')
            break;
        l = l*10 + psString[i]-'0';
    }
    return(l);
}

// 14.12 内存复制, 类似memcpy()，在目标串末尾添'\0'
// In       : psSource  : 源地址
//            iLength   : 要拷贝的串长度
// Out      : pszTarget : 目标地址
void vMemcpy0(uchar *pszTarget, const uchar *psSource, int iLength)
{
    memcpy(pszTarget, psSource, iLength);
    pszTarget[iLength] = 0;
}

// 14.13 将压缩BCD码串转变成长整数
// In       : psBcdString : BCD码串，必须为合法的BCD码，不需要'\0'结尾
//            iLength     : BCD码串长度，必须小于等于12
// Ret      : 转变后的长整数
ulong ulBcd2L(const uchar *psBcdString, int iLength)
{
    uchar  sTmp[24];

    ASSERT(iLength <= 12 && iLength >= 0);
    if(iLength>12 || iLength<0)
        return(0);
    vOneTwo(psBcdString, iLength, sTmp);
    return(ulA2L(sTmp, iLength*2));
}

// 14.14 将长整数转变成压缩BCD码串
// In       : ulLongNumber : 要转变的长整数
//            iLength      : 转变之后BCD码串长度, 必须小于等于12
// Out      : psBcdString  : 转变后的BCD码串，高位在前
void vL2Bcd(ulong ulLongNumber, int iLength, uchar *psBcdString)
{
    uchar sTmp[24+1];

    ASSERT(iLength<=12 && iLength>0);
    if(iLength > 12 || iLength < 0)
        return;
    sprintf((char*)sTmp, "%024lu", ulLongNumber);
    vTwoOne(sTmp+24-iLength*2, iLength*2, psBcdString);
}

// 14.15 将二进制源串分解成双倍长度可读的3X格式串
// In       : psIn     : 源串
//            iLength  : 源串长度
// Out      : psOut    : 目标串
void vOneTwoX(const uchar *psIn, int iLength, uchar *psOut)
{
    int iCounter;

    for(iCounter = 0; iCounter < iLength; iCounter++){
        psOut[2*iCounter] = 0x30 + (((psIn[iCounter] >> 4)) & 0x0F);
        psOut[2*iCounter+1] = 0x30 + (psIn[iCounter] & 0x0F);
    }
}

// 14.16 将二进制源串分解成双倍长度可读的3X格式串, 并在末尾添'\0'
// In       : psIn     : 源串
//            iLength  : 源串长度
// Out      : pszOut   : 目标串
void vOneTwoX0(const uchar *psIn, int iLength, uchar *pszOut)
{
    vOneTwo(psIn, iLength, pszOut);
	if(iLength < 0)
		iLength = 0;
    pszOut[2*iLength]=0;
}
