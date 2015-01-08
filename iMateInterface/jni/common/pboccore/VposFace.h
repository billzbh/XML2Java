/**************************************
File name     : VPOSFACE.H
Function      : Define VPOS interface
Author        : Yu Jun
First edition : Ver 1.00 Sep 24th, 1999
Update to     : Ver 2.00 Mar 8th, 2000
                         add # define int short
                         rename _vFunctionDes() to _vDes()
                                add TRI_ENCRYPT & TRI_DECRYPT
                         add macro DBGINFO
                         add communication functions
                Ver 2.01 Jun 14th, 2001
                         remove macro ATR_LENGTH and DATA_BUFFER_LEN
                         instead of DATA_BUFFER_LEN, use fixed length 256
                Ver 2.10 Aug 7th, 2001
                         adjust magnetic interface
                         _uiMagGet() and _vMagWrite() use new specification
                         add macro VPOS_H_VER
                Ver 2.11 Dec 30th, 2002
                         adjust function _uiPrint()
                         when print a full line and a '\n', only cause one line feed
                Ver 3.0  Mar 19th, 2004
                         add trail functions
                             vTrailInit(), vTrail(), uiTrailSet()
						 removed Mar 27th, 2012
                Ver 3.1  Nov 7th, 2005
                         add functions: _uiBackupLine() and _vRestoreLine()
                         add function: _uiGetPosType()
                Ver 3.1a Nov 10th, 2006
                         add macro TEL_ERROR_CANCEL
				Ver 3.2  2011
						 add new comm functions, _uiComm???()
				Ver 3.2a Jun 20th, 2012
						 add new functions _uiBackupScreen() _vRestoreScreen()
                Ver 3.3  Nov 26th, 2012
                         add new functions _uiDoApdu()
				Ver 3.4  Jun 27th, 2013
						 将Pub.h中声明的一些基础函数并入
						 void vXor(uchar *psVect1, const uchar *psVect2, int iLength)
						 void vOr(uchar *psVect1, const uchar *psVect2, int iLength)
						 void vAnd(uchar *psVect1, const uchar *psVect2, int iLength)
						 void vOneTwo(const uchar *psIn, int iLength, uchar *psOut)
						 void vOneTwo0(const uchar *psIn, int iLength, uchar *pszOut)
						 void vTwoOne(const uchar *psIn, int iLength, uchar *psOut)
						 ulong ulStrToLong(const uchar *psBinString, int iLength)
						 void vLongToStr(ulong ulLongNumber, int iLength, uchar *psBinString)
						 ulong ulHexToLong(const uchar *psHexString, int iLength)
						 void vLongToHex(ulong ulLongNumber, int iLength, uchar *psHexString)
						 ulong ulA2L(const uchar *psString, int iLength)
						 void vMemcpy0(uchar *pszTarget, const uchar *psSource, int iLength)
						 ulong ulBcd2L(const uchar *psBcdString, int iLength)
						 void vL2Bcd(ulong ulLongNumber, int iLength, uchar *psBcdString)
				Ver 3.5  Apr 21st, 2014
						 增加函数_uiSetCardCtrlFunc(), 用于支持外部传入的IC卡操作函数
						 增加函数vOneTwoX()与vOneTwoX()
**************************************/
# ifndef _VPOSFACE_H
# define _VPOSFACE_H

# define VPOS_H_VER  0x0340

//# define VPOS_DEBUG

# ifndef uint
# define uint unsigned int
# endif

# ifndef uchar
# define uchar unsigned char
# endif

# ifndef ulong
# define ulong unsigned long
# endif

# ifndef ushort
# define ushort unsigned short
# endif

# define POS_TYPE_MASK              0xFF00

# define POS_TYPE_PC                0x0100

# define POS_TYPE_ALINK             0x0200
# define POS_TYPE_ALINK_ST2         0x0201
# define POS_TYPE_ALINK_ST5         0x0202

# define POS_TYPE_TALENTO           0x0300

# define POS_TYPE_ARTEMA            0x0400
# define POS_TYPE_ARTEMA_DECT       0x0401
# define POS_TYPE_ARTEMA_DESK       0x0402
# define POS_TYPE_ARTEMA_COMPACT    0x0403

# define POS_TYPE_YIN4              0x0500

# define POS_TYPE_VERIFONE          0x0600
# define POS_TYPE_VERIFONE_VX520    0x0601
# define POS_TYPE_VERIFONE_VX680    0x0602

# define POS_TYPE_YIN5              0x0700
# define POS_TYPE_YIN5_A            0x0701

# define POS_TYPE_LINUX             0x0800

// for display attribute
# define DISP_NORMAL    0x00
# define DISP_REVERSE   0x01
# define DISP_UNDERLINE 0x02
# define DISP_OVERLAP   0x04

// define keyboard code
# define _KEY_ENTER     0x0d
# define _KEY_BKSP      0x08
# define _KEY_ESC       0x1b
# define _KEY_CANCEL    0x1b
# define _KEY_DOT       '.'
# define _KEY_WELL      '#'
# define _KEY_STAR      '*'

# define _KEY_0         '0'
# define _KEY_1         '1'
# define _KEY_2         '2'
# define _KEY_3         '3'
# define _KEY_4         '4'
# define _KEY_5         '5'
# define _KEY_6         '6'
# define _KEY_7         '7'
# define _KEY_8         '8'
# define _KEY_9         '9'

# define _KEY_F1        0xF1
# define _KEY_F2        0xF2
# define _KEY_F3        0xF3
# define _KEY_F4        0xF4
# define _KEY_F5        0xF5
# define _KEY_F6        0xF6
# define _KEY_F7        0xF7
# define _KEY_F8        0xF8
# define _KEY_F9        0xF9
# define _KEY_F10       0xFA
# define _KEY_F11       0xFB
# define _KEY_F12       0xFC
# define _KEY_OFF       0xFE
# define _KEY_OTHER     0xFF

# define _KEY_UP        0xE0
# define _KEY_DOWN      0xE1
# define _KEY_LEFT      0xE2
# define _KEY_RIGHT     0xE3
# define _KEY_NEXT      0xE4
# define _KEY_UP1       0xE5
# define _KEY_UP2       0xE6
# define _KEY_UP3       0xE7
# define _KEY_UP4       0xE8
# define _KEY_FN1       0xE9
# define _KEY_FN2       0xEA
# define _KEY_FN3       0xEB
# define _KEY_FN4       0xEC
# define _KEY_MENU      0xED
# define _KEY_00        0xEE
# define _KEY_FN        0xEF

// for des use
# define ENCRYPT        1
# define DECRYPT        2
# define TRI_ENCRYPT    3
# define TRI_DECRYPT    4

// for communication
# define COMM_NONE                  0
# define COMM_EVEN                  1
# define COMM_ODD                   2

# define COMM_SYNC                  1
# define COMM_ASYNC                 2
# define COMM_CCITT                 1
# define COMM_BELL                  2

# define TEL_OK                     0x0000
# define TEL_PARAMETER              0x0100

# define TEL_STATUS_WAIT            0x0200
# define TEL_STATUS_BUSY            0x0201
# define TEL_STATUS_TIMEOUT         0x0202

# define TEL_ERROR_NO_DEVICE        0x0300
# define TEL_ERROR_NO_TONE          0x0301
# define TEL_ERROR_NO_CARRY         0x0302
# define TEL_ERROR_NO_HDLC          0x0303
# define TEL_ERROR_NO_CONNECT       0x0304
# define TEL_ERROR_NO_ANSWER        0x0305
# define TEL_ERROR_NO_LINE          0x0306
# define TEL_ERROR_CANCEL           0x0398
# define TEL_ERROR_UNKNOWN          0x0399

// 网络通讯
# define VPOSCOMM_SDLC              1
# define VPOSCOMM_ASYNC             2
# define VPOSCOMM_ETHER             3
# define VPOSCOMM_GPRS              4
# define VPOSCOMM_CDMA              5
// 网络通讯返回码
# define COM_OK                     0x0000 // 成功
# define COM_PARAMETER              0x0100 // 参数错误
# define COM_STATUS_PROCESSING      0x0200 // 操作进行中
# define COM_STATUS_BUZY            0x0201 // 线路忙
# define COM_STATUS_TIMEOUT         0x0202 // 超时
# define COM_ERR_DEVICE             0x0300 // 设备故障或无此设备
# define COM_ERR_NO_TONE            0x0301 // 无拨号音
# define COM_ERR_NO_CARRY           0x0302 // 无载波
# define COM_ERR_NO_CONNECT         0x0304 // 没有连接
# define COM_ERR_NO_ANSWER          0x0305 // 没有应答
# define COM_ERR_NO_LINE            0x0306 // 没接电话线
# define COM_ERR_COM_TYPE           0x0310 // 通讯类型不支持
# define COM_ERR_NO_REGISTER        0x0311 // 还没有注册网络
# define COM_ERR_SEND               0x0312 // 发送失败
# define COM_ERR_RECV               0x0313 // 接收失败
# define COM_ERR_SYS                0x0314 // 系统错误
# define COM_ERR_CANCEL             0x0398 // 用户取消
# define COM_ERR_UNKNOWN            0x0399 // 未知错误

# define APDU_NORMAL                0      // 标准APDU, 不自动处理61xx、6Cxx
# define APDU_AUTO_GET_RESP         1      // 标准APDU, 自动处理61xx、6Cxx
# define APDU_EMV                   0x80   // 执行EMV协议
# define APDU_SI                    0x81   // 执行社保2.0协议

typedef struct {
    uchar sCommand[4];
    uchar ucLengthIn;
    uchar sDataIn[256];
    uchar ucLengthExpected;
} APDU_IN;
typedef struct {
    uchar  ucLengthOut;
    uchar  sDataOut[256];
    ushort uiStatus;
} APDU_OUT;
typedef struct {
    ushort uiReader;
    uchar  sCommand[4];
    uchar  ucLengthIn;
    uchar  ucLengthExpected;
    ushort uiStatus; // 0 menas card terminal i/o error
} APDU_STATUS;

typedef struct {
    ulong ulBaud;      // 1200, 2400, 4800, 9600
    uchar ucParity;    // COMM_NONE, COMM_ODD, COMM_EVEN
    uchar ucBits;      // 7, 8
    uchar ucStop;      // 1
    uchar szPABX[5];   // 外线号码, 不需要加逗号
    uchar szTelNo[15]; // example: "07553259571", "3237496"
    uchar ucType;      // COMM_CCITT, COMM_BELL
    uchar ucMode;      // COMM_SYNC, COMM_ASYNC
} TEL_CONFIG;

typedef struct {
    uint  uiCommType;           //网络类型 VPOSCOMM_SDLC or VPOSCOMM_ASYNC or VPOSCOMM_ETHER or VPOSCOMM_GPRS or VPOSCOMM_CDMA
    uchar szLocalIP[16];	    //本机地址      ETHERNET    GPRS    CDMA
    uchar szSubNetMask[16];	    //子网掩码      ETHERNET    GPRS    CDMA
    uchar szGateWay[16];	    //网关地址      ETHERNET    GPRS    CDMA
    uchar szPriDNS[16];		    //主DNS地址     ETHERNET    GPRS    CDMA
    uchar szSecDNS[16];		    //次DNS地址     ETHERNET    GPRS    CDMA
    uchar szApn[64];		    //APN                       GPRS
    uchar szPhoneNumber[32];    //拨号号码                  GPRS    CDMA
    uchar szUserName[32];	    //登录用户名                GPRS    CDMA
    uchar szUserPwd[32];	    //登录密码                  GPRS    CDMA
} VPOSCOMM_CONFIG; // 网络通讯参数

typedef struct {
    uchar szLocalIP[16];	    //本机地址      ETHERNET    GPRS    CDMA
    uchar szSubNetMask[16];	    //子网掩码      ETHERNET    GPRS    CDMA
    uchar szGateWay[16];	    //网关地址      ETHERNET    GPRS    CDMA
    uchar szMyPhID[20];		    //本机电话号码              GPRS    CDMA
    uchar szSingal[4];			//信号强度(0-100)           GPRS    CDMA
} VPOSCOMM_INFO;   // 网络信息

typedef struct {
    ushort uiFlag;           // 启用或停止标志
    ulong  ulXMemOffset;     // 整个跟踪数据在XMem中的起始地址
    ulong  ulXMemSize;       // 整个跟踪数据在XMem中的大小
    ushort uiMemSize;        // 内存部分跟踪数据块大小
    
    ulong  ulXMemRound;      // XMem被跟踪数据重写次数
    ulong  ulXMemCurrOffset; // 跟踪数据当前在XMem中的位置(相对于ulXMemOffset)
    ushort uiMemCurrOffset;  // 跟踪数据当前在内存中的位置
    uchar  sBuf[1024];       // 跟踪数据在内存中的缓冲区
} TRAIL;

// 不再支持以下三个老式接口
extern APDU_STATUS  gl_ApduStatus;
extern APDU_IN      gl_LastApduIn;
extern APDU_OUT     gl_LastApduOut;

extern uchar gl_sLastApduIn[270];
extern uint  gl_uiLastApduInLen;
extern uchar gl_sLastApduOut[270];
extern uint  gl_uiLastApduOutLen;

extern TRAIL gl_Trail;
# ifdef VPOS_DEBUG
extern uchar gl_szDbgInfo[];
# endif

// 1.   系统管理函数

// 1.1. 初始化POS系统，只能在程序起始处调用一次。
void _vPosInit(void);

// 1.2. POS停机，不同的POS可能的表现有：下电、重启动、仅停止运行。
void _vPosClose(void);

// 1.3. 取得POS型号
// 返    回：POS型号
//           目前支持 POS_TYPE_PC POS_TYPE_ST2 POS_TYPE_TALENTO POS_TYPE_ARTEMA
uint _uiGetPosType(void);

// 2.   显示

// 2.1. 取屏幕可显示行数
// 返    回：屏幕可显示行数
uint _uiGetVLines(void);

// 2.2. 取屏幕可显示列数
// 返    回：屏幕可显示列数
uint _uiGetVCols(void);

// 2.3. 清除整个屏幕
void _vCls(void);

// 2.4. 在某行显示字符串
// 输入参数：uiLine  : 要显示的行数，从第一行开始
//           pszText : 要显示的字串
// 说    明：如不存在该行，立即返回。如字串长度超出屏幕宽度，截去超出的部分。
void _vDisp(uint uiLine, const uchar *pszText);

// 2.5. 在某行显示某属性的字符串
// 输入参数：uiLine  : 要显示的行数，从第一行开始
//           pszText : 要显示的字串
//           ucAttr  : 显示属性，可以是DISP_NORMAL、DISP_REVERSE、DISP_UNDERLINE的组合
// 说    明：如不存在该行，立即返回。如字串长度超出屏幕宽度，截去超出的部分。
void _vDispX(uint uiLine, const uchar *pszText, uchar ucAttr);

// 2.6. 在某行某列显示字符串
// 输入参数：uiLine  : 要显示的行数，从第1行开始
//           uiCol   : 要显示的列数，从第0列开始
//           pszText : 要显示的字串
// 说    明：如不存在该行，立即返回。如字串长度超出屏幕宽度，则绕到下一行。
//           如字串超出屏幕最后一行，截去超出的部分。
void _vDispAt(uint uiLine, uint uiCol, const uchar *pszText);

// 2.7. 在某行某列带属性显示字符串
// 输入参数：uiLine  : 要显示的行数，从第1行开始
//           uiCol   : 要显示的列数，从第0列开始
//           pszText : 要显示的字串
//           ucAttr  : 显示属性，可以是DISP_NORMAL、DISP_REVERSE、DISP_UNDERLINE的组合
// 说    明：如不存在该行，立即返回。如字串长度超出屏幕宽度，则绕到下一行。
//           如字串超出屏幕最后一行，截去超出的部分。
void _vDispAtX(uint uiLine, uint uiCol, const uchar *pszText, uchar ucAttr);

// 2.8. 备份某行内容
// 输入参数：uiLine  : 要备份的行号
// 返    回：0       : 成功备份
//           1       : 备份空间满
// 说    明：将指定的行内容连同属性保存起来
uint _uiBackupLine(uint uiLine);

// 2.9. 恢复最后一次行备份
// 说    明：将最后一次备份的行恢复原样
void _vRestoreLine(void);

// 2.10. 屏幕内容备份
// 返    回：0       : 成功
//           1       : err
uint _uiBackupScreen(void);

// 2.11. 屏幕内容恢复
// 返    回：0       : 成功
//           1       : err
void _vRestoreScreen(void);

// 3. 键盘

// 3.1. 检测有否按键，立即返回
// 返    回：0 ：没有按键
//           1 ：检测到按键，可通过_uiGetKey()读取
uint _uiKeyPressed(void);

// 3.2. 等待按键，直到读取到一个按键为止
// 返    回：0x00 - 0xff ：按键的ASCII码
uint _uiGetKey(void);

// 3.3. 清除键盘缓冲区
void _vFlushKey(void);

// 3.4. 重新定义一个按键，使该键返回另外一个键码
// 输入参数：ucSource : 键盘上真正存在的键码
//           ucTarget : 重定义后该建返回的键码, 如果该键码与ucSource相同，则删除该键原先的定义
// 返    回：0        : 成功
//           1        : 重定义的键码太多
// 说    明：最多只能重定义10个键码
uint _uiMapKey(uchar ucSource, uchar ucTarget);



// 4. IC卡

// 4.1. 检测卡片是否插入
// 输入参数：uiReader : 虚拟卡座号
// 返    回：0        : 无卡
//           1        : 有卡
// 说    明：只能检测用户卡座
uint _uiTestCard(uint uiReader);

// 4.2. 卡片复位
// 输入参数：uiReader    : 虚拟卡座号
// 输出参数：psResetData : ATR 数据
// 返    回：0           : 复位失败
//           >0          : 复位成功，返回ATR长度
uint _uiResetCard(uint uiReader, uchar *psResetData);

// 4.3. 卡片下电
// 输入参数：uiReader    : 虚拟卡座号
// 返    回：0           : 成功
//           1           : 失败
uint _uiCloseCard(uint uiReader);

// 4.4. 执行IC卡指令
// 输入参数：uiReader : 虚拟卡座号
//           pIn      : IC卡指令结构
// 输出参数：pOut     : IC卡返回结果
// 返    回：0        : 成功
//           1        : 失败
uint _uiExchangeApdu(uint uiReader, APDU_IN *pIn, APDU_OUT *pOut);

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
uint _uiDoApdu(uint uiReader, uint uiInLen, uchar *psIn, uint *puiOutLen, uchar *psOut, uint uiProtocol);

// 4.6. 设置读卡函数, 用于外部提供IC卡接口时
// 输入参数: uiReader     : 卡座号, 0-9, 以后对该卡座号的操作将用所设置的函数进行
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
				        int (*pfiCloseCard)(void));

// 5. 打印机

// 5.1. 取打印机宽度
// 返    回：0  : 无打印机
//           >0 : 打印机可打印的最大宽度
uint _uiGetPCols(void);

// 5.2. 打印数据
// 输入参数：pszText : 要打印的字串
// 返    回：0 : 打印成功
//           1 : 打印错误
// 说    明：打印字串超过打印机最大宽度时反绕到下一行。
//           '\n'字符会导致换行。
uint _uiPrint(uchar const *pszText);



// 6. 时间

// 6.1. 设置当前系统时间
// 输入参数：pszTime : 要设置的时间，格式:YYYYMMDDHHMMSS
// 返    回：0       : 成功
//           1       : 失败
uint _uiSetTime(uchar const *pszTime);

// 6.2. 读取系统时间
// 输出参数：pszTime : 当前系统时间，格式:YYYYMMDDHHMMSS
void _vGetTime(uchar *pszTime);

// 6.3.设置计时器
// 输入参数：ulNTick   : 超时时间，单位为0.01秒
// 输出参数：pulTimer  : 计时器变量
// 说    明：精确到0.01秒
void _vSetTimer(ulong *pulTimer, ulong ulNTick);

// 6.4. 判断是否到达计时器变量所标明的时间
// 输入参数：ulTimer : 计时器变量
// 返    回：0       : 没有超时
//           1       : 超时
uint _uiTestTimer(ulong ulTimer);

// 6.5. 延迟一段时间
// 输入参数：uiNTick : 延迟的时间，单位为0.01秒
void _vDelay(uint uiNTick);



// 7. 扩展内存

// 7.1. 设置扩展内存大小
// 输入参数：ulSize : 要设置的扩展内存大小，单位为字节
// 返    回：0      : 设置成功
//           1      : 参数错误, 申请的空间必须大于等于32
//           2      : 申请失败
//           >=32   : 扩展内存不足，返回值为最大可用扩展内存大小
ulong _ulSetXMemSize(ulong ulSize);

// 7.2. 读扩展内存
// 输入参数：ulOffset : 扩展内存以字节为单位的偏移量
//           uiLen    ：以字节为单位的长度
// 输出参数：pBuffer  : 读取得数据
// 返    回：0        : 成功
//           1        : 失败
uint _uiXMemRead(void *pBuffer, ulong ulOffset, uint uiLen);

// 7.3. 写扩展内存
// 输入参数：pBuffer  : 要写的数据
//           ulOffset : 扩展内存以字节为单位的偏移量
//           uiLen    ：以字节为单位的长度
// 返    回：0        : 成功
//           1        : 失败
uint _uiXMemWrite(const void *pBuffer, ulong ulOffset, uint uiLen);



// 8. 磁条卡

// 8.1. 清磁卡缓冲区
// 返    回：0 : 成功
//           1 : 失败
uint _uiMagReset(void);

// 8.2. 检测是否有磁卡刷过
// 返    回：0 : 无卡
//           1 : 有磁卡刷过
uint _uiMagTest(void);

// 8.3. 读磁道信息
// 输入参数：uiTrackNo : 磁道号，1-3
// 输出参数：pszBuffer : 磁道信息
// 返    回：0 : 读取信息正确
//           1 : 没有数据
//           2 : 检测到错误
// 说    明：没有起始符、结束符、校验位，字符编码为: 0x30-0x3f
uint _uiMagGet(uint uiTrackNo, uchar *pszBuffer);

// 8.4. 写磁道信息
// 输入参数：pszTrack2 : 2磁道信息
//         ：pszTrack3 : 3磁道信息
// 说    明：没有起始符、结束符、校验位，字符编码为: 0x30-0x3f
void _vMagWrite(uchar *pszTrack2, uchar *pszTrack3);


// 9. 通讯

// 9.1. 设置当前串口号
// 输入参数：ucPortNo : 新串口号, 从串口1开始
// 返    回：原先的串口号
uchar _ucAsyncSetPort(uchar ucPortNo);

// 9.2. 打开当前串口
// 输入参数：ulBaud   : 波特率
//           ucParity : 奇偶校验标志，COMM_EVEN、COMM_ODD、COMM_NONE
//           ucBits   : 数据位长度
//           ucStop   : 停止位长度
// 返    回：0        : 成功
//           1        : 参数错误
//           2        : 失败
uchar _ucAsyncOpen(ulong ulBaud, uchar ucParity, uchar ucBits, uchar ucStop);

// 9.3. 关闭当前串口
// 返    回：0        : 成功
//           1        : 失败
uchar _ucAsyncClose(void);

// 9.4. 复位当前串口，清空接收缓冲区
// 返    回：0        : 成功
//           1        : 失败
uchar _ucAsyncReset(void);

// 9.5. 发送一个字节
// 输入参数：ucChar : 要发送的字符
// 返    回：0      : 成功
//           1      : 失败
uchar _ucAsyncSend(uchar ucChar);

// 9.6. 检测有没有字符收到
// 返    回：0 : 没有字符收到
//           1 : 有字符收到
uchar _ucAsyncTest(void);

// 9.7. 接收一个字符，没有则一直等待
// 返    回：接收到的字符
uchar _ucAsyncGet(void);

// 9.8.	发送一串数据
// 输入参数：pBuf  : 要发送的字符
//           uiLen : 数据长度
// 返    回：0     : 成功
//           1     : 失败
uchar _ucAsyncSendBuf(void *pBuf, uint uiLen);

// 9.9.	接收指定长度的一串数据
// 输入参数：pBuf      : 要发送的数据
//           uiLen     : 数据长度
//           uiTimeOut : 以秒为单位的超时时间
// 返    回：0         : 成功
//           1         : 超时
uchar _ucAsyncGetBuf(void *pBuf, uint uiLen, uint uiTimeOut);

// 9.10. 拨号建立链接
// 输入参数：pTelConfig : 拨号结构指针，见 TEL_CONFIG 结构
//           uiTimeOut  : 超时时间，单位为秒
// 返    回：0          : 成功
//           1          : 失败，可用_uiTelGetStatus()取得错误
uchar _ucTelConnect(const TEL_CONFIG *pTelConfig, uint uiTimeOut);

// 9.11. 拆除链接
// 返    回：0 : 成功
//           1 : 失败，可用_uiTelGetStatus()取得错误
uchar _ucTelDisconnect(void);

// 9.12. 取回拨号链接当前状态
// 返    回：0x0000 : 正常
//           0x01?? : 参数错误
//           0x02?? : 正常，低位字节表示一种状态
//           0x03?? : 错误，低位字节表示错误原因
uint _uiTelGetStatus(void);

// 9.13. 拨号线发送数据
// 输入参数：uiLength : 发送报文长度
//           pBuffer  : 发送报文内容
// 返    回：0        : 正常
//           1        : 失败，可用_uiTelGetStatus()取得错误
uchar _ucTelSend(uint uiLength, void const *pBuffer);

// 9.14. 拨号线接收数据
// 输入参数：puiLength : 接收报文长度
//           pBuffer   : 接收报文内容
//           uiTimeOut : 超时时间，单位为秒
// 返    回：0         : 正常
//           1         : 失败，可用_uiTelGetStatus()取得错误
uchar _ucTelGet(uint *puiLength, void *pBuffer, uint uiTimeOut);



// 10. 其它

// 10.1. 发一短声
void _vBuzzer(void);

// 10.2. 取一随机数
// 返    回：0-255的随机数
uchar _ucGetRand(void);

// 10.3. 做DES运算
// 输入参数：uiMode   : ENCRYPT -> 加密，DECRYPT -> 解密
//                      TRI_ENCRYPT -> 3Des加密，TRI_DECRYPT -> 3Des解密
//           psSource : 源
//           psKey    : 密钥
// 输出参数：psResult : 目的
void _vDes(uint uiMode, uchar *psSource, uchar *psKey, uchar *psResult);



// 11. 调试宏
// 只有在VPOSFACE.H中有 # define VPOS_DEBUG 时以下宏才起作用

// 11.1. ASSERT(bVal)
// 功能：如果bVal为假，则显示该宏所在的文件名与行数
// 原型：ASSART(BOOL bVal)
# ifdef VPOS_DEBUG
extern void _vDispAssert(uchar *pszFileName, ulong uiLineNo);
# define ASSERT(expr)                                              \
      if(!(expr)) {                                                \
          strcpy((char *)gl_szDbgInfo, __FILE__);                          \
          _vDispAssert(gl_szDbgInfo, (ulong)(__LINE__));           \
      }
# else
# define ASSERT(expr)
# endif

// 11.2. DBGINFO()
// 功能：显示该宏所在的行数与pPointer所指内容，内容用16进制数显示6字节
// 原型：DBGINFO(void *pPointer)
# ifdef VPOS_DEBUG
extern void _vDispDbgInfo(uchar *pszFileName, ulong ulLineNo, void *pPointer);
# define DBGINFO(pPointer)                                         \
      strcpy((char *)gl_szDbgInfo, __FILE__);                 \
      _vDispDbgInfo(gl_szDbgInfo, (ulong)(__LINE__), (void *)(pPointer));
# else
# define DBGINFO(pPointer)
# endif

// 不再支持跟踪功能
// 12. 跟踪
//

// 13. 网络功能函数(也包括了SDLC、异步通讯), 接口参照了VeriFone520 vCommCE接口
// 13.1. 网络初始化
uint _uiCommInit(void);

// 13.2 注册网络
// in  : uiCommType  : 网络类型 VPOSCOMM_SDLC or VPOSCOMM_ASYNC or VPOSCOMM_ETHER or VPOSCOMM_GPRS or VPOSCOMM_CDMA
//       pCommConfig : 网络参数
//       uiTimeout   : 超时时间(秒), 0表示不等待,立即返回
// ret : VPOSCOMM_
// Note: 必须先注册网络才可使用网络
uint _uiCommLogin(uint uiCommType, VPOSCOMM_CONFIG *pCommConfig, uint uiTimeout);

// 13.2 注销网络
// in  : uiCommType  : 网络类型 VPOSCOMM_SDLC or VPOSCOMM_ASYNC or VPOSCOMM_ETHER or VPOSCOMM_GPRS or VPOSCOMM_CDMA
uint _uiCommLogOut(uint uiCommType);

// 13.3 保持注册状态
// in  : uiCommType  : 网络类型 VPOSCOMM_SDLC or VPOSCOMM_ASYNC or VPOSCOMM_ETHER or VPOSCOMM_GPRS or VPOSCOMM_CDMA
// Note: 如果发现丢失了注册, 试图重新注册
uint _uiCommKeepLogin(uint uiCommType);

// 13.4 连接服务器
// in  : uiCommType  : 网络类型 VPOSCOMM_SDLC or VPOSCOMM_ASYNC or VPOSCOMM_ETHER or VPOSCOMM_GPRS or VPOSCOMM_CDMA
//       psParam1    : 参数1, 对于VPOSCOMM_SDLC VPOSCOMM_ASYNC : 电话号码
//                            对于VPOSCOMM_ETHER VPOSCOMM_GPRS VPOSCOMM_CDMA : Ip地址
//       psParam2    : 参数2, 对于VPOSCOMM_SDLC VPOSCOMM_ASYNC : 外线号码
//                            对于VPOSCOMM_ETHER VPOSCOMM_GPRS VPOSCOMM_CDMA : 端口号
//       psParam3    : 参数3, 对于VPOSCOMM_SDLC VPOSCOMM_ASYNC : "0"-不检测拨号音 "1"-检测拨号音
//                            对于VPOSCOMM_ETHER VPOSCOMM_GPRS VPOSCOMM_CDMA : "0"-TCP "1"-UDP
//       uiTimeout   : 超时时间(秒), 0表示不等待,立即返回
uint _uiCommConnect(uint uiCommType, uchar *psParam1, uchar *psParam2, uchar *psParam3, uint uiTimeout);

// 13.5 检测连通性
// in  : uiCommType  : 网络类型 VPOSCOMM_SDLC or VPOSCOMM_ASYNC or VPOSCOMM_ETHER or VPOSCOMM_GPRS or VPOSCOMM_CDMA
uint _uiCommTestConnect(uint uiCommType);

// 13.6 断开连接
// in  : uiCommType  : 网络类型 VPOSCOMM_SDLC or VPOSCOMM_ASYNC or VPOSCOMM_ETHER or VPOSCOMM_GPRS or VPOSCOMM_CDMA
uint _uiCommDisconnect(uint uiCommType);

// 13.7 保持连接状态
// in  : uiCommType  : 网络类型 VPOSCOMM_SDLC or VPOSCOMM_ASYNC or VPOSCOMM_ETHER or VPOSCOMM_GPRS or VPOSCOMM_CDMA
uint _uiCommKeepConnect(uint uiCommType);

// 13.8 获取网络信息
// in  : uiCommType  : 网络类型 VPOSCOMM_SDLC or VPOSCOMM_ASYNC or VPOSCOMM_ETHER or VPOSCOMM_GPRS or VPOSCOMM_CDMA
// out : pCommInfo   : 网络信息
uint _uiCommGetInfo(uint uiCommType, VPOSCOMM_INFO *pCommInfo);

// 13.9 发送数据
// in  : uiCommType  : 网络类型 VPOSCOMM_SDLC or VPOSCOMM_ASYNC or VPOSCOMM_ETHER or VPOSCOMM_GPRS or VPOSCOMM_CDMA
//       psSendData  : 发送的数据
//       uiSendLen   : 发送的数据长度
uint _uiCommSendData(uint uiCommType, uchar *psSendData, uint uiSendLen);

// 13.10 接收数据
// in  : uiCommType  : 网络类型 VPOSCOMM_SDLC or VPOSCOMM_ASYNC or VPOSCOMM_ETHER or VPOSCOMM_GPRS or VPOSCOMM_CDMA
//       piRecvLen   : 接收数据缓冲区大小
//       uiTimeout   : 超时时间(秒)
// out : psRecvData  : 接收的数据
//       piRecvLen   : 接收的数据长度
uint _uiCommRecvData(uint uiCommType, uchar *psRecvData, uint *puiRecvLen, uint uiTimeout);

// 14. 基础函数
// 14.1 将两个串按字节异或
// In       : psVect1  : 目标串
//            psVect2  : 源串
//            iLength  : 字节数
void vXor(uchar *psVect1, const uchar *psVect2, int iLength);

// 14.2 将两个串按字节或
// In       : psVect1  : 目标串
//            psVect2  : 源串
//            iLength  : 字节数
void vOr(uchar *psVect1, const uchar *psVect2, int iLength);

// 14.3 将两个串按字节与
// In       : psVect1  : 目标串
//            psVect2  : 源串
//            iLength  : 字节数
void vAnd(uchar *psVect1, const uchar *psVect2, int iLength);

// 14.4 将二进制源串分解成双倍长度可读的16进制串
// In       : psIn     : 源串
//            iLength  : 源串长度
// Out      : psOut    : 目标串
void vOneTwo(const uchar *psIn, int iLength, uchar *psOut);

// 14.5 将二进制源串分解成双倍长度可读的16进制串, 并在末尾添'\0'
// In       : psIn     : 源串
//            iLength  : 源串长度
// Out      : pszOut   : 目标串
void vOneTwo0(const uchar *psIn, int iLength, uchar *pszOut);

// 14.6 将可读的16进制表示串压缩成其一半长度的二进制串
// In       : psIn     : 源串
//            iLength  : 源串长度
// Out      : psOut    : 目标串
// Attention: 源串必须为合法的十六进制表示，大小写均可
//            长度如果为奇数，函数会靠近到比它大一位的偶数
void vTwoOne(const uchar *psIn, int iLength, uchar *psOut);

// 14.7 将二进制串转变成长整数
// In       : psBinString : 二进制串，高位在前
//            iLength     : 二进制串长度，可为{1,2,3,4}之一
// Ret      : 转变后的长整数
ulong ulStrToLong(const uchar *psBinString, int iLength);

// 14.8 将长整数转变成二进制串
// In       : ulLongNumber : 要转变的长整数
//            iLength      : 转变之后二进制串长度
// Out      : psBinString  : 转变后的二进制串，高位在前
void vLongToStr(ulong ulLongNumber, int iLength, uchar *psBinString);

// 14.9 将十六进制可读串转变成长整数
// In       : psHexString : 十六进制串
//            iLength     : 十六进制串长度，0-8
// Ret      : 转变后的长整数
ulong ulHexToLong(const uchar *psHexString, int iLength);

// 14.10 将长整数转变成十六进制可读串
// In       : ulLongNumber : 要转变的长整数
//            iLength      : 转变之后十六进制串长度
// Out      : psHexString  : 转变后的十六进制串
void vLongToHex(ulong ulLongNumber, int iLength, uchar *psHexString);

// 14.11 将数字串转变成长整数
// In       : psString : 数字串，必须为合法的数字，不需要'\0'结尾
//            iLength  : 数字串长度
// Ret      : 转变后的长整数
ulong ulA2L(const uchar *psString, int iLength);

// 14.12 内存复制, 类似memcpy()，在目标串末尾添'\0'
// In       : psSource  : 源地址
//            iLength   : 要拷贝的串长度
// Out      : pszTarget : 目标地址
void vMemcpy0(uchar *pszTarget, const uchar *psSource, int iLength);

// 14.13 将压缩BCD码串转变成长整数
// In       : psBcdString : BCD码串，必须为合法的BCD码，不需要'\0'结尾
//            iLength     : BCD码串长度，必须小于等于12
// Ret      : 转变后的长整数
ulong ulBcd2L(const uchar *psBcdString, int iLength);

// 14.14 将长整数转变成压缩BCD码串
// In       : ulLongNumber : 要转变的长整数
//            iLength      : 转变之后BCD码串长度, 必须小于等于12
// Out      : psBcdString  : 转变后的BCD码串，高位在前
void vL2Bcd(ulong ulLongNumber, int iLength, uchar *psBcdString);

// 14.15 将二进制源串分解成双倍长度可读的3X格式串
// In       : psIn     : 源串
//            iLength  : 源串长度
// Out      : psOut    : 目标串
void vOneTwoX(const uchar *psIn, int iLength, uchar *psOut);

// 14.16 将二进制源串分解成双倍长度可读的3X格式串, 并在末尾添'\0'
// In       : psIn     : 源串
//            iLength  : 源串长度
// Out      : pszOut   : 目标串
void vOneTwoX0(const uchar *psIn, int iLength, uchar *pszOut);

# endif // # ifndef VPOSFACE_H
