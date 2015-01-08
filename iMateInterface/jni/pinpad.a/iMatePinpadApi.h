#ifndef __PINPADAPI_H__
#define __PINPADAPI_H__

#include <jni.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PINPAD_MODEL_KMY                0   //凯明扬-KMY3512
#define PINPAD_MODEL_XYD                1   //信雅达-P90

#define ALGO_ENCRYPT                    1
#define ALGO_DECRYPT                    2


#define DECRYPT_KEY_MODE                0
#define ENCRYPT_KEY_MODE                1
#define PIN_KEY_MODE                    2

// 设置Java虚拟机参数指针
// 使用说明：
// 在开始使用Memory之前调用该接口，按C结构方式将JavaVM变量传入接口
// JavaVM可以通过实现 JNI 的初始化函数 JNI_OnLoad 获取
void Pinpad_SetupJavaVM(JavaVM *vm);

// 设置Pinpad型号
// in  : pinpadModel: KMY_MODEL,凯明扬密码键盘; XYD_MODEL,信雅达密码键盘
void Pinpad_SetPinpadModel(int pinpadModel);

// 设置算法类型，用于下载masterkey或workingkey之前调用，仅用于信雅达的Pinpad
// in  : mode		: 主密钥类型包括，DECRYPT_KEY_MODE，ENCRYPT_KEY_MODE，PIN_KEY_MODE
void Pinpad_SetKeyMode(int mode);

// Pinpad上电
int Pinpad_PowerOn(void);

// Pinpad下电
int Pinpad_PowerOff(void);

// 设置密码键盘参数, 仅支持信雅达密码键盘
// in  : paramName	: 参数名称，
//					  "AuthCode", 认证密钥, 16字节长度, 缺省值为16个0x00
//					  "UID", Pinpad UID, 缺省值为{'1','2','3','4','5','6','7','8','9','0','1','2','3','4','5'}
//					  "WorkDirNum",子目录编号, 缺省值为1
//		 value		: 参数值，二进制形式, AuthCode和UID长度为16，WorkDirNum的值长度为1
// ret : 0     		: 成功
//       1     		: 失败
void Pinpad_ParamSetup(char *paramName, unsigned char* value);

// 取消密码输入操作，用于取消密码输入
void Pinpad_Cancel(void);

// Pinpad复位自检
// in  : initFlag  	: 1-清除Pinpad中的密钥, 0-不清除密钥
// ret : 0			: 成功
//		 其它		: 失败
int Pinpad_Reset(int initFlag);

// Pinpad固件版本号
// out : firmwareVersion  	: 版本号输出缓冲区，maxlength = 50
//		 versionLength		: 输出的版本号长度
// ret : 0					: 成功
//		 其它				: 失败
int Pinpad_GetVersion(unsigned char *firmwareVersion, int *versionLength);

// Pinpad下装主密钥
// in  : is3des  	: 是否采用3DES算法，0表示使用DES算法，1表示使用3DES算法
//		 index		: 主密钥索引，0-15
//	     mastKey	: 主密钥，长度为8，16或24
//		 keyLength	: 主密钥长度，支持的长度为8，16，24
// ret : 0			: 成功
//		 其它		: 失败
int Pinpad_DownloadMasterKey(int is3des, int index, unsigned char* masterKey, int keyLength);

// Pinpad下装工作密钥
// in  : is3des  		: 是否采用3DES算法，0表示使用DES算法，1表示使用3DES算法
//		 masterIndex	: 主密钥索引，0~15
//	     workingIndex	: 工作密钥索引
//       workingKey     : 工作密钥，长度为8，16或24
//		 keyLength		: 工作密钥长度，长度为8，16或24
// ret : 0				: 成功
//		 其它			: 失败
int Pinpad_DownloadWorkingKey(int is3des, int masterIndex, int workingIndex, unsigned char* workingKey, int keyLength);

// Pinpad输入密码
// in  : is3des  		: 是否采用3DES算法，0表示使用DES算法
//		 isAutoReturn	: 输入pin长度后，是否自动返回
//	     masterIndex	: 主密钥索引，0~15
//	     workingIndex	: 工作密钥索引，0~15
//		 cardNo			: 卡号/帐号（最少12位数字）
//		 pinLength		: 需要输入PIN的长度
//		 timeout		: 输入密码等待超时时间 <= 255 秒
// out : pinblock		: pinpad输出的pinblock，8字节长度
// ret : 0				: 成功
//		 其它			: 失败
int Pinpad_InputPinblock(int is3des, int isAutoReturn, int masterIndex, int workingIndex, char* cardNo, int pinLength, unsigned char *pinblock, int timeout);

// Pinpad加解密数据
// in  : is3des  		: 是否采用3DES算法，false表示使用DES算法
//		 algo			: 算法，取值: ALGO_ENCRYPT, ALGO_DECRYPT, 以ECB方式进行加解密运算
//		 masterIndex	: 主密钥索引
//	     workingIndex	: 工作密钥索引， 如果为-1，用masterKey进行加解密
//		 inData			: 加解密输入数据
//		 dataLength		: indata数据长度，必须为8的倍数
// out : outData		: 加解密输出的结果，长度和inData相同
// ret : 0				: 成功
//		 其它			: 失败
int Pinpad_Encrypt(int is3des, int algo, int masterIndex, int workingIndex, unsigned char*  inData, int dataLength, unsigned char * outData);

// Pinpad数据MAC运算（ANSIX9.9）
// in  : is3des  		: 是否采用3DES算法，false表示使用DES算法
//		 masterIndex	: 主密钥索引
//	     workingIndex	: 工作密钥索引， 如果为-1，用masterKey进行加解密
//		 inData			: 加解密输入数据
//		 dataLength		: indata数据长度，必须为8的倍数
// out : outData		: MAC计算输出的结果，8字节长度
// ret : 0				: 成功
//		 其它			: 失败
int Pinpad_Mac(int is3des, int masterIndex, int workingIndex, unsigned char* data, int dataLength, unsigned char *mac);

#ifdef __cplusplus
}
#endif
#endif  // 结束宏定义
