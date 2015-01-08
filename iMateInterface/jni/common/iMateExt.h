#ifndef IMATEEXT_H
#define IMATEEXT_H

#include "unsigned.h"

// 设置IC读卡器类型，0：芯片读卡器；1：射频读卡器
void vSetCardReaderType(int iCardReaderType);

int  iIMateTestCard(void);

// 卡片复位
// ret : <=0 : 复位错误
//       >0  : 复位成功, 返回值为ATR长度
int  iIMateResetCard(uchar *psResetData);

// 关闭卡片
// ret : 不关心
int  iIMateCloseCard(void);

// 执行APDU指令
// in  : iInLen   	: Apdu指令长度
// 		 pIn     	: Apdu指令, 格式: Cla Ins P1 P2 Lc DataIn Le
// out : piOutLen 	: Apdu应答长度
//       pOut    	: Apdu应答, 格式: DataOut Sw1 Sw2
// ret : 0          : 卡操作成功
//       1          : 卡操作错
int  iIMateExchangeApdu(int iInLen, uchar *pIn, int *piOutLen, uchar *pOut);

#endif
