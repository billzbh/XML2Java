/**************************************
File name     : EmvPara.h
Function      : EMV/pboc2.0借贷记参数管理
Author        : Yu Jun
First edition : Mar 31st, 2012
Modified      : 
**************************************/
#ifndef _EMVPARA_H
#define _EMVPARA_H

#ifdef __cplusplus
extern "C" {
#endif

// 设置终端参数
// in  : pTermParam        : 终端参数
// out : pszErrTag         : 如果设置出错, 出错的Tag值
// ret : HXEMV_OK          : OK
//       HXEMV_PARA        : 参数错误, 错误Tag见pszErrTag
//       HXEMV_CORE        : 内部错误
//       HXEMV_FLOW_ERROR  : EMV流程错误
int iEmvSetTermParam(stTermParam *pTermParam, uchar *pszErrTag);

// 装载终端支持的Aid
// in  : pTermAid          : 终端支持的Aid
//       iFlag             : 动作, HXEMV_PARA_INIT:初始化 HXEMV_PARA_ADD:增加 HXEMV_PARA_DEL:删除
// out : pszErrTag         : 如果设置出错, 出错的Tag值
// ret : HXEMV_OK          : OK
//       HXEMV_LACK_MEMORY : 存储空间不足
//       HXEMV_PARA        : 参数错误, 错误Tag见pszErrTag
//       HXEMV_FLOW_ERROR  : EMV流程错误
// Note: 删除时只传入AID即可
int iEmvLoadTermAid(stTermAid *pTermAid, int iFlag, uchar *pszErrTag);

// 装载CA公钥
// in  : pCAPublicKey      : CA公钥
//       iFlag             : 动作, HXEMV_PARA_INIT:初始化 HXEMV_PARA_ADD:增加 HXEMV_PARA_DEL:删除
// ret : HXEMV_OK          : OK
//       HXEMV_LACK_MEMORY : 存储空间不足
//       HXEMV_PARA        : 参数错误
//       HXEMV_FLOW_ERROR  : EMV流程错误
// Note: 删除时只传入RID、index即可
int iEmvLoadCAPublicKey(stCAPublicKey *pCAPublicKey, int iFlag);

#ifdef __cplusplus
}
#endif

#endif
