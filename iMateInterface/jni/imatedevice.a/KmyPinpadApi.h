#ifndef __KMYPINPADAPI_H__
#define __KMYPINPADAPI_H__

#include "unsigned.h"

#ifdef __cplusplus
extern "C" {
#endif

// 取消密码输入操作，用于取消密码输入
void KmyPinpad_Cancel(void);

// Pinpad复位自检
// in  : initFlag  	: 1-清除Pinpad中的密钥, 0-不清除密钥
// ret : 0			: 成功
//		 其它		: 失败
int KmyPinpad_Reset(int initFlag);

// Pinpad固件版本号
// out : firmwareVersion  	: 版本号输出缓冲区，maxlength = 50
//		 versionLength		: 版本号长度
// ret : 0					: 成功
//		 其它				: 失败
int KmyPinpad_GetVersion(uchar *firmwareVersion, int *versionLength);

// Pinpad下装主密钥
// in  : is3des  	: 是否采用3DES算法，false表示使用DES算法
//		 index		: 主密钥索引
//	     mastKey	: 主密钥
//		 keyLength	: 主密钥长度
// ret : 0			: 成功
//		 其它		: 失败
int KmyPinpad_DownloadMasterKey(int is3des, int index, uchar* masterKey, int keyLength);

// Pinpad下装工作密钥
// in  : is3des  		: 是否采用3DES算法，false表示使用DES算法
//		 index			: 主密钥索引
//	     mastKey		: 主密钥
//	     workingIndex	: 工作密钥索引
//		 keyLength		: 主密钥长度
// ret : 0				: 成功
//		 其它			: 失败
int KmyPinpad_DownloadWorkingKey(int is3des, int masterIndex, int workingIndex, uchar* workingKey, int keyLength);

// Pinpad输入密码
// in  : is3des  		: 是否采用3DES算法，false表示使用DES算法
//		 isAutoReturn	: 输入pin长度后，是否自动返回
//	     masterIndex	: 主密钥索引
//	     workingIndex	: 工作密钥索引
//		 cardNo			: 卡号/帐号（最少12位数字）
//		 pinLength		: 需要输入PIN的长度
//		 timeout		: 输入密码等待超时时间 <= 255 秒
// out : pinblock		: pinpad输出的pinblock
// ret : 0				: 成功
//		 其它			: 失败
int KmyPinpad_InputPinblock(int is3des, int isAutoReturn, int masterIndex, int workingIndex, char* cardNo, int pinLength, uchar *pinblock, int timeout);

// Pinpad加解密数据
// in  : is3des  		: 是否采用3DES算法，false表示使用DES算法
//		 algo			: 算法，取值: ALGO_ENCRYPT, ALGO_DECRYPT, 以ECB方式进行加解密运算
//		 masterIndex	: 主密钥索引
//	     workingIndex	: 工作密钥索引， 如果为-1，用masterKey进行加解密
//		 inData			: 加解密输入数据
//		 dataLength		: indata数据长度，必须为8的倍数
// out : outData		: 加解密输出的结果
// ret : 0				: 成功
//		 其它			: 失败
int KmyPinpad_Encrypt(int is3des, int algo, int masterIndex, int workingIndex, uchar*  inData, int dataLength, uchar * outData);

// Pinpad数据MAC运算（ANSIX9.9）
// in  : is3des  		: 是否采用3DES算法，false表示使用DES算法
//		 masterIndex	: 主密钥索引
//	     workingIndex	: 工作密钥索引， 如果为-1，用masterKey进行加解密
//		 inData			: 加解密输入数据
//		 dataLength		: indata数据长度，必须为8的倍数
// out : outData		: MAC计算输出的结果
// ret : 0				: 成功
//		 其它			: 失败
int KmyPinpad_Mac(int is3des, int masterIndex, int workingIndex, uchar* data, int dataLength, uchar *mac);

#ifdef __cplusplus
}
#endif
#endif  // 结束宏定义
