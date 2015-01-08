#ifndef _ISSFACE_H
#define _ISSFACE_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	int  iAppVer;				// 应用版本号, 0x20或0x30, 分别表示Pboc2.0卡或Pboc3.0卡

	int  iKmcIndex;
	int  iAppKeyBaseIndex;
	int  iIssuerRsaKeyIndex;

	char szPan[19+1];			// 12-19
	int  iPanSerialNo;			// 0-99, -1表示无
	char szExpireDate[6+1];		// YYMMDD
	char szHolderName[45+1];	// Len>=2
	int  iHolderIdType;			// 0:身份证 1:军官证 2:护照 3:入境证 4:临时身份证 5:其它
	char szHolderId[40+1];
	char szDefaultPin[12+1];	// 4-12

	char szAid[32+1];			// OneTwo之后的
	char szLabel[16+1];			// 1-16
	int  iCaIndex;				// 0-255
	int  iIcRsaKeyLen;			// 64-192
	long lIcRsaE;				// 3 or 65537
	int  iCountryCode;			// 1-999
	int  iCurrencyCode;			// 1-999
} stIssData;

// 设置发卡过程显示函数
// in  : pfvShowStatus : 显示函数指针
int iIssSetStatusShowFunc(void (*pfvShowStatus)(char *));

// 设置读卡函数, 用于外部提供IC卡接口
// 输入参数: 以下4个函数如果有一个传入NULL, 则表示取消之前对该卡座设置的函数
//           pfiTestCard  : 检测卡片是否存在函数指针
//           pfiResetCard : 卡片复位函数指针
//           pfiDoApdu    : 执行Apdu指令函数指针
//           pfiCloseCard : 关闭卡片函数指针
int iIssSetCardCtrlFunc(int (*pfiTestCard)(void),
				        int (*pfiResetCard)(unsigned char *psAtr),
				        int (*pfiDoApdu)(int iApduInLen, unsigned char *psApduIn, int *piApduOutLen, unsigned char *psApduOut),
				        int (*pfiCloseCard)(void));

// 设置发卡环境
// in  : iKeyMode  : 密钥获取方式, 0:程序内部 1:配置文件 2:加密机
//       pszHsmIp  : 加密机IP地址(如果密钥获取方式为加密机)
//       iHsmPort  : 加密机端口号(如果密钥获取方式为加密机)
//       iLogFlag  : 发卡日志记录标志, 0:不记录 1:记录
// out : pszErrMsg : 错误信息
// ret : 0         : OK
//       1         : Error
int iIssSetEnv(int iKeyMode, char *pszHsmIp, int iHsmPort, int iLogFlag, char *pszErrMsg);

// 设置发卡数据
// in  : iUsage    : 用途, 0:用于发卡 1:用于删除应用
//     : pIssData  : 个人化数据
// out : pszErrMsg : 错误信息
// ret : 0         : OK
//       1         : Error
int iIssSetData(int iUsage, stIssData *pIssData, char *pszErrMsg);

// 发卡
// out : pszErrMsg : 错误信息
// ret : 0         : OK
//       1         : Error
int iIssCard(char *pszErrMsg);

// 删除应用
// out : pszErrMsg : 错误信息
// ret : 0         : OK
//       1         : Error
int iIssDelApps(char *pszErrMsg);

#ifdef __cplusplus
}
#endif

#endif
