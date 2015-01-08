#ifndef _PUSHAPDU_H
#define _PUSHAPDU_H

#define PUSHAPDU_READ_PSE_REC			1 // 读DDF目录文件
#define PUSHAPDU_SELECT_BY_AID_LIST		2 // 根据AID_LIST选择应用
#define PUSHAPDU_READ_APP_REC			3 // 读应用数据记录
#define PUSHAPDU_GET_DATA				4 // 读取数据
#define PUSHAPDU_SCRIPT_EXEC			5 // 脚本执行
#define PUSHAPDU_READ_TRANS_REC			6 // 读交易日志记录

// 初始化PushApdu缓冲区
// in  : iReaderNo : 执行推送缓冲的卡座号
void vPushApduInit(int iReaderNo);

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
void vPushApduPrepare(int iPushApduType, ...);

// 清除PushApdu缓冲区
void vPushApduClear(void);

// 根据推送缓冲区虚拟执行IC卡指令
// 输入参数：uiReader : 虚拟卡座号
//           pIn      : IC卡指令结构
//           pOut     : IC卡返回结果
// 返    回：0        : 成功
//           1        : 失败
//           2        : 缓冲区无内容
// Note    : 接口类似uiExchangeApdu()
uint uiPushExchangeApdu(uint uiReader, APDU_IN *pIn, APDU_OUT *pOut);

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
uint uiPushDoApdu(uint uiReader, uint uiInLen, uchar *psIn, uint *puiOutLen, uchar *psOut);

#endif
