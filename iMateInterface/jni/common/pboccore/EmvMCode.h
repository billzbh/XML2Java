/**************************************
File name     : EmvMCode.h
Function      : EMV提示信息代码表
Author        : Yu Jun
First edition : Apr 6th, 2012
Modified      : 
**************************************/
#ifndef _EMVMCODE_H
#define _EMVMCODE_H

// 标准信息ID，EMVMSG_XX_DESC
//             XX  :标准信息代码(emv2008 book4 11.2 (P88))
//             DESC:信息含义
#define EMVMSG_00_NULL				0x00 // 无信息, 不需要显示
#define EMVMSG_01_AMOUNT			0x01
#define EMVMSG_02_AMOUNT_OK		    0x02
#define EMVMSG_03_APPROVED			0x03
#define EMVMSG_04_CALL_YOUR_BANK	0x04
#define EMVMSG_05_CANCEL_OR_ENTER   0x05
#define EMVMSG_06_CARD_ERROR		0x06
#define EMVMSG_07_DECLINED			0x07
#define EMVMSG_08_ENTER_AMOUNT		0x08
#define EMVMSG_09_ENTER_PIN			0x09
#define EMVMSG_0A_INCORRECT_PIN		0x0A
#define EMVMSG_0B_INSERT_CARD		0x0B
#define EMVMSG_0C_NOT_ACCEPTED		0x0C
#define EMVMSG_0D_PIN_OK			0x0D
#define EMVMSG_0E_PLEASE_WAIT		0x0E
#define EMVMSG_0F_PROCESSING_ERROR	0x0F
#define EMVMSG_10_REMOVE_CARD		0x10
#define EMVMSG_11_USE_CHIP_REAEDER	0x11
#define EMVMSG_12_USE_MAG_STRIPE	0x12
#define EMVMSG_13_TRY_AGAIN			0x13

// 自定义信息ID，EMVMSG_DESC
//               DESC  :信息含义
#define EMVMSG_CLEAR_SCR            0x0100	// 清屏
#define EMVMSG_NATIVE_ERR           0x0101  // 内部错误
#define EMVMSG_APPCALL_ERR			0x0102  // 应用层错误
#define EMVMSG_SELECT_LANG          0x0103	// 选择持卡人语言(核心不支持选择操作员语言)
#define EMVMSG_SELECT_APP           0x0104	// 选择应用
#define EMVMSG_ENTER_PIN_OFFLINE    0x0105  // 输入脱机密码
#define EMVMSG_ENTER_PIN_ONLINE     0x0106  // 输入联机密码
#define EMVMSG_LAST_PIN				0x0107  // 最后一次密码输入
#define EMVMSG_SERVICE_NOT_ALLOWED  0x0108  // 服务不允许
#define EMVMSG_TRANS_NOT_ALLOWED    0x0109  // 交易不支持
#define EMVMSG_CANCEL               0x010A  // 被取消
#define EMVMSG_TIMEOUT              0x010B  // 超时
#define EMVMSG_TERMINATE            0x010C  // 终止交易
#define EMVMSG_TRY_OTHER_INTERFACE	0x010D	// 尝试其它通信界面
#define EMVMSG_EXPIRED_APP			0x010E  // 卡片过有效期,交易失败

// 以下信息为pboc2.0证件验证专用
#define EMVMSG_VERIFY_HOLDER_ID		0x0201 // "请查验持卡人证件"
#define EMVMSG_HOLDER_ID_TYPE		0x0202 // "持卡人证件类型"
#define EMVMSG_IDENTIFICATION_CARD	0x0203 // "身份证"
#define EMVMSG_CERTIFICATE_OF_OFFICERS	0x0204 // "军官证"
#define EMVMSG_PASSPORT				0x0205 // "护照"
#define EMVMSG_ARRIVAL_CARD			0x0206 // "入境证"
#define EMVMSG_TEMPORARY_IDENTITY_CARD	0x0207 // "临时身份证"
#define EMVMSG_OTHER				0x0208 // "其它"
#define EMVMSG_HOLDER_ID_NO			0x0209 // "持卡人证件号码"


// 动态信息, 大于等于0x1000的消息为动态消息, 表示除了消息类型表示的信息外, 还需要其它不固定的信息
#define EMVMSG_VAR					0x1000  // 动态信息
#define EMVMSG_AMOUNT_CONFIRM		0x1001  // 金额确认

#endif
