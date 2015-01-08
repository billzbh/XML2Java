/**************************************
File name     : EmvFace.h
Function      : EMV/pboc2.0借贷记核心外部输入输出模块接口
Note          : 如果应用层仅使用非回调接口, 则不需要本模块, 但需要提供空原型函数以供链接.
Author        : Yu Jun
First edition : Mar 31st, 2012
Modified      : 
**************************************/
#ifndef _EMVFACE_H
#define _EMVFACE_H
#include "EmvCore.h"

#define EMVFACE_OK					0   // OK
#define EMVFACE_CANCEL				1   // 取消
#define EMVFACE_BYPASS				2   // bypass，略过
#define EMVFACE_TIMEOUT				3   // 超时
#define EMVFACE_BLACK				4   // 黑名单卡
#define EMVFACE_FAIL                5   // 失败(持卡人证件验证)
#define EMVFACE_ERROR				99  // 其它错误

#define EMVFACE_AMOUNT				0   // 金额类型 Amount
#define EMVFACE_AMOUNT_OTHER		1   // 金额类型 Amount other
#define EMVFACE_PLAIN_PIN			0   // 明文PIN
#define EMVFACE_CIPHERED_PIN        1   // 密文PIN
#define EMVFACE_NEED_NO_CONFIRM     0   // 不要求确认
#define EMVFACE_NEED_CONFIRM        1   // 要求确认

// 选择语言
// in  : pszLangs        : 支持的语言, 例如:"enzh"表示支持英文、中文
//       pszDescs        : 以'='结尾的支持的语言描述, 例如:"ENGLISH=中文="描述了英文、中文
//       pszPrompt       : 提示信息(不必须使用, 可自行提示)
//       pszLanguage     : 语言代码(按钮文字用)，ISO-639-1，小写
// out : pszSelectedLang : 选中的语言代码, 例如:"zh"
// ret : EMVFACE_OK      : OK
//       EMVFACE_CANCEL  : 取消
//       EMVFACE_TIMEOUT : 超时
//       EMVFACE_ERROR   : 其它错误
// Note: 不支持操作员选择语言, 此选择语言为选择持卡人语言
int iHxFaceSelectLang(uchar *pszLangs, uchar *pszDescs, uchar *pszSelectedLang, uchar *pszPrompt, uchar *pszLanguage);

// 选择应用
// in  : pAppList        : 卡片与终端都支持的应用列表
//       iAppNum         : 卡片与终端都支持的应用个数
//       pszPrompt       : 提示信息(不必须使用, 可自行提示)
//       pszLanguage     : 应该使用的语言代码，ISO-639-1，小写
// out : piAppNo         : 选中的应用在应用列表中得下标
// ret : EMVFACE_OK      : OK
//       EMVFACE_CANCEL  : 取消
//       EMVFACE_TIMEOUT : 超时
//       EMVFACE_ERROR   : 其它错误
int iHxFaceSelectApp(stADFInfo *pAppList, int iAppNum, int *piAppNo, uchar *pszPrompt, uchar *pszLanguage);

// 确认应用
// in  : pApp            : 应用
//       pszPrompt       : 提示信息(不必须使用, 可自行提示)
//       pszLanguage     : 应该使用的语言代码，ISO-639-1，小写
// ret : EMVFACE_OK      : OK
//       EMVFACE_CANCEL  : 取消
//       EMVFACE_TIMEOUT : 超时
//       EMVFACE_ERROR   : 其它错误
// Note: 终端只支持公共字符集，不使用preferred name
int iHxFaceConfirmApp(stADFInfo *pApp, uchar *pszPrompt, uchar *pszLanguage);

// 读取金额
// in  : iAmountType     : 金额类型，EMVFACE_AMOUNT:Amount
//                                   EMVFACE_AMOUNT_OTHER:Amount other
//       pszPrompt       : 提示信息(不必须使用, 可自行提示)
//       pszCurrencyCode : 3字节货币表示代码
//       iDecimalPosition: 0-3, 小数点位置(小数点后数字个数)
//       pszLanguage     : 语言代码，ISO-639-1，小写
// out : pszAmount       : 金额[12]
// ret : EMVFACE_OK      : OK
//       EMVFACE_CANCEL  : 取消
//       EMVFACE_TIMEOUT : 超时
//       EMVFACE_ERROR   : 其它错误
// Note: 高层如果已经取得了响应金额，不需再要求输入，直接返回即可
int iHxFaceGetAmount(int iAmountType, uchar *pszAmount, uchar *pszPrompt, uchar *pszCurrencyCode, int iDecimalPosition, uchar *pszLanguage);

// 读取密码
// in  : iPinType        : 密码类型, EMVFACE_PLAIN_PIN:明文PIN
//                                   EMVFACE_CIPHERED_PIN:密文PIN
//       iBypassFlag     : 允许Bypass标记, 0:不允许bypass 1:允许bypass
//       pszPan          : 账号
//       pszPrompt1      : 提示信息1(格式化好的金额)
//       pszPrompt2      : 提示信息2(不必须使用, 可自行提示)
//       pszLanguage     : 语言代码，ISO-639-1，小写
// out : psPin           : 4-12位明文密码('\0'结尾的字符串)或8字节密文PIN
// ret : EMVFACE_OK      : OK
//       EMVFACE_CANCEL  : 取消
//       EMVFACE_BYPASS  : bypass，略过
//       EMVFACE_TIMEOUT : 超时
//       EMVFACE_ERROR   : 其它错误
// Note: 联机密文暂时只简单将密码TwoOne一下
int iHxFaceGetPin(int iPinType, int iBypassFlag, uchar *psPin, uchar *pszPan, uchar *pszPrompt1, uchar *pszPrompt2, uchar *pszLanguage);

// 验证持卡人证件
// in  : iBypassFlag     : 允许Bypass标记, 0:不允许bypass 1:允许bypass
//       pszPrompt       : 主提示信息("请查验持卡人证件")
//       pszPromptIdType : 提示信息, 证件类型("持卡人证件类型")
//       pszIdType       : 证件类型描述
//       pszPromptIdNo   : 提示信息, 证件号码("持卡人证件号码")
//       pszIdNo         : 证件号码
//       pszLanguage     : 语言代码，ISO-639-1，小写
// ret : EMVFACE_OK      : OK, 验证通过
//       EMVFACE_CANCEL  : 取消
//       EMVFACE_BYPASS  : bypass，略过或验证失败
//       EMVFACE_FAIL    : 证件不符
//       EMVFACE_TIMEOUT : 超时
// Note: 提示信息不必须使用, 可自行提示
int iHxFaceVerifyHolderId(int iBypassFlag, uchar *pszPrompt, uchar *pszPromptIdType, uchar *pszIdType, uchar *pszPromptIdNo, uchar *pszIdNo, uchar *pszLanguage);

// 显示信息, 等待超时或交互后清除显示内容
// in  : iMsgType        : 信息类型，用于外部要自己提供提示信息时
//       iConfirmFlag    : 是否要确认标志, EMVFACE_NEED_NO_CONFIRM:不要求确认
//                                         EMVFACE_NEED_CONFIRM:要求确认
//       pszPrompt1      : 提示信息1(不必须使用, 可自行提示)
//       pszPrompt2      : 提示信息2(附加信息, 例如:要确认的金额)
//       pszLanguage     : 语言代码，ISO-639-1，小写
// ret : EMVFACE_OK      : OK, 如果要求确认，已经确认
//       EMVFACE_CANCEL  : 取消
//       EMVFACE_TIMEOUT : 超时
int iHxFaceDispMsg(int iMsgType, int iConfirmFlag, uchar *pszPrompt1, uchar *pszPrompt2, uchar *pszLanguage);

// 显示信息, 信息留在屏幕上, 立即退出
// in  : iMsgType        : 信息类型，用于外部要自己提供提示信息时
//       pszPrompt1      : 提示信息1(不必须使用, 可自行提示)
//       pszPrompt2      : 提示信息2(附加信息, 例如:要确认的金额)
//       pszLanguage     : 语言代码，ISO-639-1，小写
// ret : EMVFACE_OK      : OK
// Note: pszPrompt1与pszPrompt2都为NULL表示清除之前该函数显示的信息
int iHxFaceShowMsg(int iMsgType, uchar *pszPrompt1, uchar *pszPrompt2, uchar *pszLanguage);

// Pinpad显示信息
// in  : iMsgType        : 信息类型，用于外部要自己提供提示信息时
//       iConfirmFlag    : 是否要确认标志, EMVFACE_NEED_NO_CONFIRM:不要求确认
//                                         EMVFACE_NEED_CONFIRM:要求确认
//       pszPrompt1      : 提示信息1(不必须使用, 可自行提示)
//       pszPrompt2      : 提示信息2(附加信息, 例如:要确认的金额)
//       pszLanguage     : 语言代码，ISO-639-1，小写
// ret : EMVFACE_OK      : OK, 如果要求确认，已经确认
//       EMVFACE_CANCEL  : 取消
//       EMVFACE_TIMEOUT : 超时
int iHxFacePinpadDispMsg(int iMsgType, int iConfirmFlag, uchar *pszPrompt1, uchar *pszPrompt2, uchar *pszLanguage);

// 证书回收列表查询
// in  : psRid           : RID[5]
//       ucCaIndex       : CA公钥索引
//       psCertSerNo     : 证书序列号[3]
// ret : EMVFACE_OK      : 不在证书回收列表中
//       EMVFACE_BLACK   : 在证书回收列表中
//       EMVFACE_ERROR   : 其它错误
int iHxFaceCRLCheck(uchar *psRid, uchar ucCaIndex, uchar *psCertSerNo);

// 黑名单查询
// in  : psRid           : RID[5]
//       pszPan          : 账号，已经去除了尾部'F'
//       iPanSeqNo       : 账号序列号，-1表示不存在
// ret : EMVFACE_OK      : 不是黑名单卡
//       EMVFACE_BLACK   : 黑名单卡
//       EMVFACE_ERROR   : 其它错误
int iHxFaceBlackCardCheck(uchar *psRid, uchar *pszPan, int iPanSeqNo);

// 分开消费查询
// in  : psRid           : RID[5]
//       pszPan          : 账号，已经去除了尾部'F'
//       iPanSeqNo       : 账号序列号，-1表示不存在
// out : pszAmount       : 该账户历史记录金额(查不到传出"0"), 
// ret : EMVFACE_OK      : 成功
//       EMVFACE_ERROR   : 其它错误
int iHxFaceSeparateSaleCheck (uchar *psRid, uchar *pszPan, int iPanSeqNo, uchar *pszAmount);

/****----****----****----****----****----****----****----****----****
以上函数为Emv核心回调函数定义的必备函数, 必须实现, 使用非回调接口时必须定义
以下函数为送检程序为方便显示增加的辅助函数, 可不使用, 使用非回调接口时不必定义
****----****----****----****----****----****----****----****----****/
// 初始化"请等待"状态
void vHxFaceClearWaitFlag(void);
// 设置交易类型
void vHxFaceSetTransType(uint uiTransType);
// 显示交易类型标题
void vHxFaceShowTransTitle(uint uiTransType, uchar ucTransRoute);

#endif
