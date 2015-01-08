/**************************************
File name     : VPOSEXT.H
Function      : Implement the extended functions for talento pos
                At the same level compare with PosTal.c
Author        : Yu Jun
First edition : Apr 15th, 2004
**************************************/
# ifndef _VPOSEXT_H
# define _VPOSEXT_H

// 打印机
#define PRT_STATUS_OK				0 // OK
#define PRT_STATUS_NO_PAPER			1 // 缺纸
#define PRT_STATUS_LESS_PAPER		2 // 纸少
#define PRT_ERROR_NO_PRINTER		3 // 没发现打印机
#define PRT_ERROR_OFFLINE			4 // 打印机脱机
#define PRT_ERROR					9 // 故障

// 安全模块
#define SEC_STATUS_OK				0 // OK
#define SEC_STATUS_TIMEOUT			1 // 超时
#define SEC_STATUS_CANCEL			2 // 用户取消
#define SEC_STATUS_BYPASS			3 // 用户Bypass
#define SEC_ERROR_ALGO				4 // 算法不支持
#define SEC_ERROR_INDEX				5 // 索引不支持
#define SEC_ERROR_NO_KEY			6 // 密钥不存在
#define SEC_ERROR					9 // 故障

// 检测汉字库是否已下载
// 返回 : 1 = 已下载, 0 = 未下载
// note : 初始化时该函数应被调用
uint uiTestFont12(void);

// 取12点阵汉字库
// 输入参数 : ucQm     : 汉字区码
//            ucWm     : 汉字位码
// 输出参数 : psMatrix : 汉字点阵
void vGetFont12Matrix(uchar ucQm, uchar ucWm, uchar *psMatrix);

// 下装12点阵字库函数组
// uiLoadFont12Start(), uiLoadFont12Data(), uiLoadFont12End()
// return 0 means OK, 1 means error
uint uiLoadFont12Begin(ulong ulFileSize);
uint uiLoadFont12AddBlock(uchar *psData, uint uiLength);
uint uiLoadFont12End(void);

// get pin from pin pad
// In  : psMessage1   : message displayed on line 1
//       psMessage2   : message displayed on line 2
//       psMessage3   : message displayed on line 2 after a key pressed
//       pszPan       : 主帐号
//       ucBypassFlag : 允许bypass标志, 1:允许 0:不允许
//       ucMinLen     : minimum pin length
//       ucMaxLen     : maximum pin length
//       uiTimeOut    : time out time in second
// Out : pszPin       : entered pin
// Ret : SEC_STATUS_OK      : pin entered
//       SEC_STATUS_TIMEOUT : 超时
//       SEC_STATUS_CANCEL  : 用户取消
//       SEC_STATUS_BYPASS  : 用户Bypass
//       SEC_ERROR		    : 故障
uchar ucPinPadInput(uchar *psMessage1, uchar *psMessage2, uchar *psMessage3, uchar *pszPan, uchar ucBypassFlag,
						  uchar ucMinLen, uchar ucMaxLen, uint uiTimeOut, uchar *pszPin);

// display message on pinpad
// In  : psMessage1 : message displayed on line 1
//       psMessage2 : message displayed on line 2
//       uiTimeOut  : time out time in second
void vPinPadDisp(uchar *pszMesg1,uchar *pszMesg2, uint uiTimeOut);

// 安全模块初始化
// Ret : SEC_STATUS_OK      : OK
//       SEC_ERROR		    : 故障
int iSecInit(void);

// 安全模块检查主密钥存不存在
// in  : iIndex      : 主密钥索引[0-n], 容量取决于POS
// Ret : SEC_STATUS_OK      : OK
//       SEC_ERROR_INDEX	: 索引不支持
//       SEC_ERROR_NO_KEY	: 密钥不存在
//       SEC_ERROR		    : 故障
int iSecCheckMasterKey(int iIndex);

// 安全模块设置主密钥
// in  : iIndex      : 主密钥索引[0-n], 容量取决于POS
//       psMasterKey : 主密钥[16]
// Ret : SEC_STATUS_OK      : OK
//       SEC_ERROR_INDEX	: 索引不支持
//       SEC_ERROR		    : 故障
int iSecSetMasterKey(int iIndex, uchar *psMasterKey);

// 安全模块主密钥DES计算
// in  : iIndex      : 主密钥索引[0-n], 容量取决于POS
//       iMode       : 算法标识, TRI_ENCRYPT:3Des加密 TRI_DECRYPT:3Des解密
//       psIn        : 输入数据
// out : psOut       : 输出数据
// Ret : SEC_STATUS_OK      : OK
//       SEC_ERROR_INDEX	: 索引不支持
//       SEC_ERROR_NO_KEY	: 密钥不存在
//       SEC_ERROR_ALGO		: 算法不支持
//       SEC_ERROR		    : 故障
int iSecDes(int iIndex, int iMode, uchar *psIn, uchar *psOut);

// 安全模块工作密钥Retail-CBC-Mac计算
// in  : iIndex       : 主密钥索引[0-n], 容量取决于POS
//       psWorkingKey : 工作密钥密文
//       psIn         : 输入数据
//       iInLen       : 输入数据长度, 不足8的倍数后会补0
// out : psMac        : Mac结果[8]
// Ret : SEC_STATUS_OK      : OK
//       SEC_ERROR_INDEX	: 索引不支持
//       SEC_ERROR_NO_KEY	: 密钥不存在
//       SEC_ERROR_ALGO		: 算法不支持
//       SEC_ERROR		    : 故障
// Note: 如果做3DES加密, 也可用此函数
int iSec3DesCbcMacRetail(int iIndex, uchar *psWorkingKey, uchar *psIn, int iInLen, uchar *psMac);

// 密码键盘输入密文PIN
// In  : psMessage1   : message displayed on line 1
//       psMessage2   : message displayed on line 2
//       psMessage3   : message displayed on line 2 after a key pressed
//       iIndex       : 主密钥索引[0-n], 容量取决于POS
//       psPinKey     : PinKey密文
//       pszPan       : 主帐号
//       ucBypassFlag : 允许bypass标志, 1:允许 0:不允许
//       ucMinLen     : minimum pin length
//       ucMaxLen     : maximum pin length
//       uiTimeOut    : time out time in second
// Out : sPinBlock    : pinblock[8]
// Ret : SEC_STATUS_OK      : OK
//       SEC_STATUS_TIMEOUT : 超时
//       SEC_STATUS_CANCEL  : 用户取消
//       SEC_STATUS_BYPASS  : 用户Bypass
//       SEC_ERROR_INDEX	: 索引不支持
//       SEC_ERROR_NO_KEY	: 密钥不存在
//       SEC_ERROR		    : 故障
int iSecPinPadInput(uchar *psMessage1, uchar *psMessage2, uchar *psMessage3,
                    int iIndex, uchar *psPinKey, uchar *pszPan, uchar ucBypassFlag,
                    uchar ucMinLen, uchar ucMaxLen, uint uiTimeOut, uchar *psPinBlock);

// 获取电池状态
// ret : 0-100 : 电池电量
uint uiGetBatteryCharge(void);

// 获取充电状态
// ret : 0 : 非充电状态
//       1 : 充电状态
uint uiGetBatteryRecharge(void);

// LED灯控制
void _vSetLed(uchar ucNo,uchar ucOnOff);

// 打印机切纸
void _vPrintCut(void);

// 打印机清缓冲区
void _vPrintClear(void);

// 取得打印机状态
// 返    回: PRT_STATUS_OK			// OK
//           PRT_STATUS_NO_PAPER	// 缺纸
//           PRT_STATUS_LESS_PAPER	// 纸少
//           PRT_ERROR_NO_PRINTER	// 没发现打印机
//           PRT_ERROR_OFFLINE		// 打印机脱机
//           PRT_ERROR				// 故障
uint _uiPrintGetStatus(void);

// 打开打印机盒准备装纸
// ret : PRT_STATUS_OK : 打开成功
//       PRT_ERROR	   : 打开失败, 打印机盒依然为关闭状态
uint uiOpenPaperBox(void);

// 检测打印机盒开关状态
// ret : 0 : 打开状态
//       1 : 关闭状态
uint uiTestPaperBox(void);

// 读取终端序列号
// ret : 0 : OK
//       1 : ERROR
uint uiGetSerialNo(uchar *pszSerialNo);

# endif
