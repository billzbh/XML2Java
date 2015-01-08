/**************************************
File name     : EmvIO.h
Function      : EMV/pboc2.0输入输出模块
                根据终端类型、性能决定交互对象(操作员|持卡人)
Author        : Yu Jun
First edition : Apr 13th, 2012
Modified      : 
**************************************/
#ifndef _EMVIO_H
#define _EMVIO_H

#define EMVIO_OK                    0   // OK
#define EMVIO_CANCEL                1   // 取消
#define EMVIO_BYPASS                2   // bypass，略过
#define EMVIO_TIMEOUT               3   // 超时
#define EMVIO_BLACK                 4   // 黑名单
#define EMVIO_ERROR                 99  // 其它错误

#define EMVIO_AMOUNT                0   // 金额类型 Amount
#define EMVIO_AMOUNT_OTHER          1   // 金额类型 Amount other
#define EMVIO_PLAIN_PIN             0   // 明文PIN
#define EMVIO_CIPHERED_PIN          1   // 密文PIN
#define EMVIO_NEED_NO_CONFIRM       0   // 不要求确认
#define EMVIO_NEED_CONFIRM          1   // 要求确认

// 初始化输入输出模块
int iEmvIOInit(void);

// 设置交易金额
// in  : pszAmount      : 金额
//       pszAmountOther : 其它金额
// ret : EMVIO_OK       : OK
//       EMVIO_ERROR    : 其它错误
int iEmvIOSetTransAmount(uchar *pszAmount, uchar *pszAmountOther);

// 设置账号信息
// in  : iBlackFlag      : 黑名单卡标志, 0:不是黑名单 1:黑名单
//       pszRecentAmount : 最近借记金额
// ret : EMVIO_OK        : OK
//       EMVIO_ERROR     : 其它错误
int iEmvIOSetPanInfo(int iBlackFlag, uchar *pszRecentAmount);

// 获取发卡行公钥证书序列号
// out : psCertSerNo    : 发卡行公钥证书序列号
// ret : EMVIO_OK       : OK
//       EMVIO_ERROR    : 其它错误
int iEmvIOGetCertSerNo(uchar *psCertSerNo);

// 选择语言
// out : pszSelectedLang : 选中的语言代码, 例如:"zh"
//       pszLangs        : 可选的语言列表, 例如:"frzhen"
//       pszLangsDesc    : 可选的语言描述列表, '='号结尾, 例如:"FRANCE=中文=ENGLISH="
// ret : EMVIO_OK        : OK
//       EMVIO_CANCEL    : 取消
//       EMVIO_TIMEOUT   : 超时
//       EMVIO_ERROR     : 其它错误
// Note: 不支持操作员选择语言, 此选择语言为选择持卡人语言
//       对于attended终端, 要从pinpad支持语言中选
//       对于unattended终端, 也要从pinpad支持语言中选, 因此unattended终端必须配置pinpad语言等同于终端支持语言
int iEmvIOSelectLang(uchar *pszSelectedLang, uchar *pszLangs, uchar *pszLangsDesc);

// 选择应用
// in  : pAppList      : 卡片与终端都支持的应用列表
//       iAppNum       : 卡片与终端都支持的应用个数
// out : piAppNo       : 选中的应用在应用列表中得下标
// ret : EMVIO_OK      : OK
//       EMVIO_CANCEL  : 取消
//       EMVIO_TIMEOUT : 超时
//       EMVIO_ERROR   : 其它错误
int iEmvIOSelectApp(stADFInfo *pAppList, int iAppNum, int *piAppNo);

// 确认应用
// in  : pApp          : 应用
// ret : EMVIO_OK      : OK
//       EMVIO_CANCEL  : 取消
//       EMVIO_TIMEOUT : 超时
//       EMVIO_ERROR   : 其它错误
int iEmvIOConfirmApp(stADFInfo *pApp);

// 读取金额
// in  : iAmountType   : 金额类型，EMVIO_AMOUNT:Amount
//                                 EMVIO_AMOUNT_OTHER:Amount other
// out : pszAmount     : 金额[12]
// ret : EMVIO_OK      : OK
//       EMVIO_CANCEL  : 取消
//       EMVIO_TIMEOUT : 超时
//       EMVIO_ERROR   : 其它错误
int iEmvIOGetAmount(int iAmountType, uchar *pszAmount);

// 读取密码
// in  : iPinType      : 密码类型, EMVIO_PLAIN_PIN:明文PIN
//                                 EMVIO_CIPHERED_PIN:密文PIN
//       iBypassFlag   : 允许Bypass标记, 0:不允许bypass 1:允许bypass
//       pszPan        : 账号
// out : psPin         : 4-12位明文密码('\0'结尾的字符串)或8字节密文PIN
// ret : EMVIO_OK      : OK
//       EMVIO_CANCEL  : 取消
//       EMVIO_BYPASS  : bypass，略过
//       EMVIO_TIMEOUT : 超时
//       EMVIO_ERROR   : 其它错误
int iEmvIOGetPin(int iPinType, int iBypassFlag, uchar *psPin, uchar *pszPan);

// 验证持卡人身份证件
// in  : iBypassFlag    : 允许Bypass标记, 0:不允许bypass 1:允许bypass
//       ucHolderIdType : 证件类型
//       szHolderId     : 证件号码
// ret : EMVIO_OK       : OK
//       EMVIO_CANCEL   : 取消
//       EMVIO_BYPASS   : bypass，略过或证件不符
//       EMVIO_TIMEOUT  : 超时
//       EMVIO_ERROR    : 其它错误
int iEmvIOVerifyHolderId(int iBypassFlag, uchar ucHolderIdType, uchar *pszHolderId);

// 显示信息, 等待超时或交互后清除显示内容
// in  : iMsgType      : 消息类型，见EmvMsg.h
//       iConfirmFlag  : 确认标志 EMVIO_NEED_NO_CONFIRM : 不要求确认
//                                EMVIO_NEED_CONFIRM    : 要求确认
// ret : EMVIO_OK      : OK, 如果要求确认，已经确认
//       EMVIO_CANCEL  : 取消
//       EMVIO_TIMEOUT : 超时
int iEmvIODispMsg(int iMsgType, int iConfirmFlag);

// 显示信息, 信息留在屏幕上, 立即退出
// in  : iMsgType      : 消息类型，见EmvMsg.h
//                       iMsgType==-1表示清除之前显示的信息
// ret : EMVIO_OK      : OK
int iEmvIOShowMsg(int iMsgType);

// 密码键盘显示信息
// in  : iMsgType      : 消息类型，见EmvMsg.h
//       iConfirmFlag  : 确认标志 EMVIO_NEED_NO_CONFIRM : 不要求确认
//                                EMVIO_NEED_CONFIRM    : 要求确认
// ret : EMVIO_OK      : OK, 如果要求确认，已经确认
//       EMVIO_CANCEL  : 取消
//       EMVIO_TIMEOUT : 超时
// Note: 对于unattended终端或attended终端但无Pinpad，该信息会显示到唯一的显示器上
int iEmvIOPinpadDispMsg(int iMsgType, int iConfirmFlag);

// 证书回收列表查询
// in  : psCertSerNo     : 证书序列号[3]
// ret : EMVIO_OK        : 不在证书回收列表中
//       EMVIO_BLACK     : 在证书回收列表中
//       EMVIO_ERROR     : 其它错误
int iEmvIOCRLCheck(uchar *psCertSerNo);

// 黑名单查询
// ret : EMVIO_OK        : 不是黑名单卡
//       EMVIO_BLACK     : 黑名单卡
//       EMVIO_ERROR     : 其它错误
int iEmvIOBlackCardCheck(void);

// 分开消费查询
// out : pszAmount       : 该账户历史记录金额(查不到传出"0"), 
// ret : EMVIO_OK        : 成功
//       EMVIO_ERROR     : 其它错误
int iEmvIOSeparateSaleCheck (uchar *pszAmount);

#endif
