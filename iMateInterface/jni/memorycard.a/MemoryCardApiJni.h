#ifndef __MEMORYCARDAPI_STATIC_H__
#define __MEMORYCARDAPI_STATIC_H__

#include <jni.h>

#ifdef __cplusplus
extern "C" {
#endif

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

#define	CARD_VOLTAGE_1V8		0
#define	CARD_VOLTAGE_3V0		1
#define	CARD_VOLTAGE_5V0		2

#define SLE4442_TYPE			0x01
#define AT102_TYPE				0x02
#define AT1608_TYPE				0x04
#define AT1604_TYPE				0x05
#define SLE4428_TYPE			0x06
#define AT24Cxx_TYPE			0x10

// 设置JavaVM变量
// 使用说明：
// 		在开始使用Memory之前调用该接口，按C结构方式将JavaVM变量传入接口
// 		JavaVM可以通过实现 JNI 的初始化函数 JNI_OnLoad 获取
void MemoryCard_SetupJavaVM(JavaVM *vm);

// 检测插卡
// 返回码：
//      0       :   未插卡
//		1		:	已插卡
//      99      :   不支持该功能
int MemoryCard_TestCard(void);

// 功能：检测存储卡类型(4442\102\1604\1608\4428\24Cxx)
// 输出: psResetData 复位数据 < 100bytes, 第一个字节为长度
// 返回: 0xff		不识别的卡
//		其它
//					SLE4442_TYPE		0x01
//					AT102_TYPE			0x02
//					AT1608_TYPE			0x04
//					AT1604_TYPE			0x05
//					SLE4428_TYPE		0x06
//					AT24Cxx_TYPE		0x10
int MemoryCard_TestCardType(uchar *psResetData);

// 设置卡片电压
void MemoryCard_SetCardVoltage(int voltageTag);


// 打开SLE4442卡
void MemoryCard_SLE4442_Open(void);

// 打开SLE4442卡, 自动检测电压和卡类型
// 返回: 	0		: 	打开成功
//			其它		:	打开失败
int MemoryCard_SLE4442_OpenAuto(void);

// 关闭SLE4442卡
void MemoryCard_SLE4442_Close(void);

// 验证SLE4442卡密码
// 输入参数：
//      securityCode  :   密码数据缓冲，3字节长度
// 返回码：
//      0   :   成功
//      其它 :   失败
int MemoryCard_SLE4442_ChkCode(uchar *securityCode);

// 验证SLE4442卡密码(安全方式）
// 输入参数：
//      securityCode  :   加密后的密码数据，8字节长度
// 返回码：
//      0   :   成功
//      其它 :   失败
int MemoryCard_SLE4442_ChkCodeEx(uchar *securityCode);

// SLE4442读数据
// 输入参数：
//		offset	:	数据偏移量，0~255
//		dataLen	:	数据长度
// 输出参数：
//		dataBuff:	读数据缓冲区地址, maxlength=255
void MemoryCard_SLE4442_Read(uint offset, uint dataLen, uchar* dataBuff);

// SLE4442写数据
// 输入参数：
//		offset	:	数据偏移量，0~255
//		dataLen	:	数据长度
//		dataBuff:	写数据缓冲区地址, maxlength=255
void MemoryCard_SLE4442_Write(uint offset, uint dataLen, uchar* dataBuff);


// 打开SLE4428卡
void MemoryCard_SLE4428_Open(void);

// 打开SLE4428卡, 自动检测电压和卡类型
// 返回: 	0		: 	打开成功
//			其它		:	打开失败
int MemoryCard_SLE4428_OpenAuto(void);

// 关闭SLE4428卡
void MemoryCard_SLE4428_Close(void);

// 验证SLE4428卡密码
// 输入参数：
//      securityCode  :   密码数据缓冲，2字节长度
// 返回码：
//      0   :   成功
//      其它 :   失败
int MemoryCard_SLE4428_ChkCode(uchar *securityCode);

// 验证SLE4428卡密码（安全方式）
// 输入参数：
//      securityCode  :   加密后的密码数据，8字节长度
// 返回码：
//      0   :   成功
//      其它 :   失败
int MemoryCard_SLE4428_ChkCodeEx(uchar *securityCode);

// SLE4428读数据
// 输入参数：
//		offset	:	数据偏移量，0~1024
//		dataLen	:	数据长度
// 输出参数：
//		dataBuff:	读数据缓冲区地址, maxlength = 1024
void MemoryCard_SLE4428_Read(uint offset, uint dataLen, uchar* dataBuff);

// SLE4428写数据
// 输入参数：
//		offset	:	数据偏移量，0~1024
//		dataLen	:	数据长度
//		dataBuff:	写数据缓冲区地址, maxlength = 1024
void MemoryCard_SLE4428_Write(uint offset, uint dataLen, uchar* dataBuff);


// 打开AT102卡
void MemoryCard_AT102_Open(void);

// 打开AT102卡, 自动检测电压和卡类型
// 返回: 	0		: 	打开成功
//			其它		:	打开失败
int MemoryCard_AT102_OpenAuto(void);

// 关闭AT102卡
void MemoryCard_AT102_Close(void);

// 验证AT102卡密码
// 输入参数：
//      securityCode  :   密码数据缓冲，2字节长度
// 返回码：
//      0   :   成功
//      其它 :   失败
int MemoryCard_AT102_ChkCode(uchar *securityCode);

// 验证AT102卡密码（安全方式）
// 输入参数：
//      securityCode  :   加密后的密码数据，8字节长度
// 返回码：
//      0   :   成功
//      其它 :   失败
int MemoryCard_AT102_ChkCodeEx(uchar *securityCode);

// AT102读数据(按双字节读取）
// 输入参数：
//		wordOffset :	数据偏移量，<= 128
//		wordNum	:	数据长度（字长度）, <= 128
// 输出参数：
//		dataBuff:	读数据缓冲区地址, maxlength = 256
void MemoryCard_AT102_ReadWords(uint wordOffset, uint wordNum, uchar* dataBuff);

// AT102写数据(按双字节写卡）
// 输入参数：
//		wordOffset 	:	数据偏移量, <= 128
//		wordNum		:	数据长度（字长度）<= 128
//		dataBuff:	写数据缓冲区地址, maxlength = 256
// 返回码：
//      0   :   成功
//      其它 :   失败
int MemoryCard_AT102_WriteWords(uint wordOffset, uint wordNum, uchar* dataBuff);

// AT102擦除非应用区的数据
// 输入参数：
//		wordOffset 	:	数据偏移量
//		wordNum		:	删除字长度
// 返回码：
//      0   :   成功
//      其它 :   失败
int MemoryCard_AT102_EraseNonApp(uchar wordOffset, uchar wordNum);

// AT102擦除应用区的数据
// 输入参数：
//		area  	: 应用区	1:	应用区1	2:	应用区2
//		limited : 指出第2应用区的擦除次数是否受限制。此参数当且仅当area= 2时起作用。
//		eraseKey: 擦除密钥, 应用区1的擦除密码长度为6bytes，应用区2的擦除密码为4bytes，
//			  Security Level 1 方式下，eraseKey=NULL则不需要提供EraseKey方式进行擦除
// 返回码：
//      0   :   成功
//      其它 :   失败
int MemoryCard_AT102_EraseApp(uint area, uchar limited, uchar *eraseKey);

// AT102擦除应用区的数据（安全方式）
// 输入参数：
//		area  	: 应用区	1:	应用区1	2:	应用区2
//		limited : 指出第2应用区的擦除次数是否受限制。此参数当且仅当area= 2时起作用。
//		eraseKey: 擦除密钥, 加密后的应用区1的擦除密码，8字节，
//			  Security Level 1 方式下，eraseKey=NULL则不需要提供EraseKey方式进行擦除
// 返回码：
//      0   :   成功
//      其它 :   失败
int MemoryCard_AT102_EraseAppEx(uint area, uchar limited, uchar *eraseKey);

// AT102读应用区数据
// 输入参数：
//		area  	: 应用区	1:	应用区1	2:	应用区2
// 输出参数：
//		dataBuff:	读数据缓冲区地址, maxlength = 64
void MemoryCard_AT102_ReadAZ(uint area, uchar* dataBuff);

// AT102写应用区数据
// 输入参数：
//		area  	: 应用区	1:	应用区1	2:	应用区2
//		dataBuff:	读数据缓冲区地址, maxlength = 64
// 返回码：
//      0   :   成功
//      其它 :   失败
int MemoryCard_AT102_WriteAZ(uint area, uchar* dataBuff);


// AT102读测试区数据
// 输出参数：
//		dataBuff:	读数据缓冲区地址, 获取2字节数据
void MemoryCard_AT102_ReadMTZ(uchar* dataBuff);

// AT102写测试区数据
// 输入参数：
//		dataBuff:	读数据缓冲区地址, 2字节数据
// 返回码：
//      0   :   成功
//      其它 :   失败
int MemoryCard_AT102_UpdateMTZ(uchar* dataBuff);


// 打开AT1604卡
void MemoryCard_AT1604_Open(void);

// 打开AT1604卡, 自动检测电压和卡类型
// 返回: 	0		: 	打开成功
//			其它		:	打开失败
int MemoryCard_AT1604_OpenAuto(void);

// 关闭AT1604卡
void MemoryCard_AT1604_Close(void);

// 验证AT1604卡主密码
// 输入参数：
//      securityCode  :   密码数据缓冲，2字节长度
// 返回码：
//      0   :   成功
//      其它 :   失败
int MemoryCard_AT1604_ChkCode(uchar *securityCode);

// 验证AT1604卡主密码（安全方式）
// 输入参数：
//      securityCode  :   加密后的密码数据，8字节长度
// 返回码：
//      0   :   成功
//      其它 :   失败
int MemoryCard_AT1604_ChkCodeEx(uchar *securityCode);

// 验证AT1604卡分区密码
// 输入参数：
//		area		  :   分区号1、2、3、4
//      securityCode  :   密码数据缓冲，2字节长度
// 返回码：
//      0   :   成功
//      其它 :   失败
int MemoryCard_AT1604_ChkAreaCode(uint area, uchar *securityCode);

// 验证AT1604卡分区密码（安全方式）
// 输入参数：
//		area		  :   分区号1、2、3、4
//      securityCode  :   加密后的密码数据，8字节长度
// 返回码：
//      0   :   成功
//      其它 :   失败
int MemoryCard_AT1604_ChkAreaCodeEx(uint area, uchar *securityCode);

// 验证AT1604卡分区擦除密码
// 输入参数：
//		area		  :   分区号1、2、3、4
//      securityCode  :   密码数据缓冲，2字节长度
// 返回码：
//      0   :   成功
//      其它 :   失败
int MemoryCard_AT1604_ChkAreaEraseCode(uint area, uchar *securityCode);

// 验证AT1604卡分区擦除密码（安全方式）
// 输入参数：
//		area		  :   分区号1、2、3、4
//      securityCode  :   加密后的密码数据，8字节长度
// 返回码：
//      0   :   成功
//      其它 :   失败
int MemoryCard_AT1604_ChkAreaEraseCodeEx(uint area, uchar *securityCode);

// AT1604读数据
// 输入参数：
//		offset	:	数据偏移量，0~2047
//		dataLen	:	数据长度
// 输出参数：
//		dataBuff:	读数据缓冲区地址, maxlength = 2048
void MemoryCard_AT1604_Read(uint offset, uint dataLen, uchar* dataBuff);

// AT1604写数据
// 输入参数：
//		offset	:	数据偏移量，0~2047
//		dataLen	:	数据长度
//		dataBuff:	写数据缓冲区地址, maxlength = 2048
// 返回码：
//      0   :   成功
//      其它 :   失败
int MemoryCard_AT1604_Write(uint offset, uint dataLen, uchar* dataBuff);

// AT1604读分区数据
// 输入参数：
//		area	:   分区号1、2、3、4
//		offset	:	数据偏移量，0~511
//		dataLen	:	数据长度,1、2、3区512bytes 4区457bytes
// 输出参数：
//		dataBuff:	读数据缓冲区地址, maxlength = 512
void MemoryCard_AT1604_ReadAZ(uint area, uint offset, uint dataLen, uchar* dataBuff);

// AT1604写分区数据
// 输入参数：
//		area	:   分区号1、2、3、4
//		offset	:	数据偏移量，0~511
//		dataLen	:	数据长度,1、2、3区512bytes 4区457bytes
//		dataBuff:	写数据缓冲区地址, maxlength = 512
// 返回码：
//      0   :   成功
//      其它 :   失败
int MemoryCard_AT1604_WriteAZ(uint area, uint offset, uint dataLen, uchar* dataBuff);

// AT1604擦除数据
// 输入参数：
//		offset	:	数据偏移量，0~2047
//		dataLen	:	数据长度
// 返回码：
//      0   :   成功
//      其它 :   失败
int MemoryCard_AT1604_Erase(uint offset, uint dataLen);

// AT1604擦除分区数据
// 输入参数：
//		area	:   分区号1、2、3、4
//		offset	:	数据偏移量，0~511
//		dataLen	:	数据长度,1、2、3区512bytes 4区457bytes
// 返回码：
//      0   :   成功
//      其它 :   失败
int MemoryCard_AT1604_EraseAZ(uint area, uint offset, uint dataLen);

// AT1604读测试区数据
// 输出参数：
//		dataBuff:	读数据缓冲区地址, length = 2
void MemoryCard_AT1604_ReadMTZ(uchar* dataBuff);

// AT1604写测试区数据
// 输入参数：
//		dataBuff:	写数据缓冲区地址, length = 2
// 返回码：
//      0   :   成功
//      其它 :   失败
int MemoryCard_AT1604_WriteMTZ(uchar* dataBuff);


// 打开AT1608卡
// 输出参数：
//		resetData:	复位数据, length = 4bytes
void MemoryCard_AT1608_Open(uchar *resetData);

// 打开AT1608卡, 自动检测电压和卡类型
// 返回: 	0		: 	打开成功
//			其它		:	打开失败
int MemoryCard_AT1608_OpenAuto(uchar *resetData);

// 关闭AT1608卡
void MemoryCard_AT1608_Close(void);


// 读AT88SC1608熔丝状况
// 返 回 值:	0--OK	1--Error
int	MemoryCard_AT1608_ReadFuse(uchar *Fuse);

// 写AT88SC1608熔丝
// 返 回 值:	0--OK	1--Error
int	MemoryCard_AT1608_WriteFuse(void);

// AT1608卡设置应用区
// 输入参数：
//      az	: 应用区，0~7
// 返回码：
//      0   :   成功
//      其它 :   失败
int MemoryCard_AT1608_SetAZ(uchar az);

// 验证AT1608卡读写密码
// 输入参数：
// 		index		: 密码索引号 写密码索引号(0--7) 读密码索引号(0x80--0x87)
//      securityCode: 密码数据缓冲，3字节长度
// 返回码：
//      0   :   成功
//      其它 :   失败
int MemoryCard_AT1608_ChkCode(uint index, uchar *securityCode);

// 验证AT1608卡读写密码（安全方式）
// 输入参数：
// 		index		: 密码索引号 写密码索引号(0--7) 读密码索引号(0x80--0x87)
//      securityCode: 加密后的密码数据，8字节长度
// 返回码：
//      0   :   成功
//      其它 :   失败
int MemoryCard_AT1608_ChkCodeEx(uint index, uchar *securityCode);

// AT1608卡安全认证
// 输入参数：
//      gc	: 密钥，8字节长度
// 返回码：
//      0   :   成功
//      其它 :   失败
int MemoryCard_AT1608_Auth(uchar *gc);

// AT1608卡安全认证（安全方式）
// 输入参数：
//      gc	: 密钥，16字节长度
// 返回码：
//      0   :   成功
//      其它 :   失败
int MemoryCard_AT1608_AuthEx(uchar *gc);

// AT1608读数据
// 输入参数：
//		level	:   0:应用区，1：设置区
//		offset	:	设置区(0--127)	应用区(0--255)
//		dataLen	:
// 输出参数：
//		dataBuff:	读数据缓冲区地址, maxlength = 256
int MemoryCard_AT1608_Read(uint level, uint offset, uint dataLen, uchar* dataBuff);

// AT1608写数据
// 输入参数：
//		level	:   0:应用区，1：设置区
//		offset	:	设置区(0--127)	应用区(0--255)
//		dataLen	:
//		dataBuff:	读数据缓冲区地址, maxlength = 256
// 返回码：
//      0   :   成功
//      其它 :   失败
int MemoryCard_AT1608_Write(uint level, uint offset, uint dataLen, uchar* dataBuff);


// 打开AT24Cxx卡
void MemoryCard_AT24Cxx_Open(void);

// 打开AT24Cxx卡, 自动检测卡类型
int MemoryCard_AT24Cxx_OpenAuto(void);

// 关闭AT24Cxx卡
void MemoryCard_AT24Cxx_Close(void);

// AT24Cxx读数据
// 输入参数：
//		offset	:	数据偏移量
//		dataLen	:	数据长度
// 输出参数：
//		dataBuff:	读数据缓冲区地址, 根据具体情况确定
// 返回码：
//      0   :   成功
//      其它 :   失败
int MemoryCard_AT24Cxx_Read(uint offset, uint dataLen, uchar* dataBuff);

// AT24Cxx写数据
// 输入参数：
//		offset	:	数据偏移量
//		dataLen	:	数据长度
//		dataBuff:	写数据缓冲区地址, 根据具体情况确定
// 返回码：
//      0   :   成功
//      其它 :   失败
int MemoryCard_AT24Cxx_Write(uint offset, uint dataLen, uchar* dataBuff);

// AT24C32读数据
// 输入参数：
//		offset	:	数据偏移量
//		dataLen	:	数据长度
// 输出参数：
//		dataBuff:	读数据缓冲区地址, 根据具体情况确定
// 返回码：
//      0   :   成功
//      其它 :   失败
int MemoryCard_AT24C32_Read(uint offset, uint dataLen, uchar* dataBuff);

// AT24C32写数据
// 输入参数：
//		offset	:	数据偏移量
//		dataLen	:	数据长度
//		dataBuff:	写数据缓冲区地址, 根据具体情况确定
// 返回码：
//      0   :   成功
//      其它 :   失败
int MemoryCard_AT24C32_Write(uint offset, uint dataLen, uchar* dataBuff);

// 产生通讯密钥
// 输入参数：
//	masterKeyId	: 主密钥ID, 一般使用6
//	random		: 随机数，8字节
// 返回码：
//		0	:	成功
//		其它 ：	失败
int MemoryCard_GenCommKey(uint masterKeyId, uchar *random);


#ifdef __cplusplus
}
#endif
#endif  // 结束宏定义
