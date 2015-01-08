/**************************************
File name     : EmvMsg.h
Function      : EMV/pboc2.0多语言支持信息资源表
Author        : Yu Jun
First edition : Apr 6th, 2012
Modified      : 
**************************************/
#ifndef _EMVMSG_H
#define _EMVMSG_H

#include "EmvMCode.h" // EMV信息代码表

// 获取支持的语言
// out : pszLanguages : 支持的语言, 例如:"enzh"表示支持英文、中文
//       pszDescs     : 以'='结尾的支持的语言描述, 例如:"ENGLISH=中文="描述了英文、中文
// ret : 0            : OK
//       1            : 配置错误
// 如果改变了语言配置, 要手工修改本函数以返回正确的配置
int iEmvMsgTableSupportedLanguage(uchar *pszLanguages, uchar *pszDescs);

// 初始化多语言支持信息表, 在EMV核心初始化时调用
// in  : pszDefaultLanguage : 缺省语言, ISO-639-1, 2字节语言表示
// ret : 0 : OK
//       1 : 语言不支持, 此时会使用内定缺省语言
int iEmvMsgTableInit(uchar *pszDefaultLanguage);

// 获取匹配语言
// in  : pszCardPreferredLanguage : 持卡人偏好语言列表
// out : pszFinalLanguage         : 匹配语言[2]
// ret : 0 : OK, 找到匹配语言
//       1 : 没有匹配的语言，返回的匹配语言实为缺省语言
int iEmvMsgTableGetFitLanguage(uchar *pszCardPreferredLanguage, uchar *pszFinalLanguage);

// 获取Pinpad匹配语言
// in  : pszCardPreferredLanguage : 持卡人偏好语言列表
//       pszPinpadLanguage        : Pinpad支持的语言
// out : pszFinalLanguage         : 匹配语言[2]
// ret : 0 : OK, 找到匹配语言
//       1 : 没有匹配的语言，返回的匹配语言实为缺省语言
int iEmvMsgTableGetPinpadLanguage(uchar *pszCardPreferredLanguage, uchar *pszPinpadLanguage, uchar *pszFinalLanguage);

// 设置语言
// in  : psLanguageCode : ISO-639-1标准的2字节语言代码
// ret : 0              : OK
//       1              : 语言不支持
// Note: 如果不支持指定的语言，会设定为缺省语言
//       可设定与获取的匹配语言不同的语言，如是，表示核心强制采用该语言
int iEmvMsgTableSetLanguage(uchar *psLanguageCode);

// 读取某信息类型表示的信息
// in  : iMsgType : 信息类型
//       psLanguageCode : ISO-639-1标准的2字节语言代码
//           NULL表示使用iEmvMsgTableSetLanguage()设定的语言
// ret : 信息, 如果没有对应信息，返回空字符串
char *pszEmvMsgTableGetInfo(int iMsgType, uchar *psLanguageCode);

#endif
