/**************************************
File name     : EmvPara.h
Function      : EMV/pboc2.0����ǲ�������
Author        : Yu Jun
First edition : Mar 31st, 2012
Modified      : 
**************************************/
#ifndef _EMVPARA_H
#define _EMVPARA_H

#ifdef __cplusplus
extern "C" {
#endif

// �����ն˲���
// in  : pTermParam        : �ն˲���
// out : pszErrTag         : ������ó���, �����Tagֵ
// ret : HXEMV_OK          : OK
//       HXEMV_PARA        : ��������, ����Tag��pszErrTag
//       HXEMV_CORE        : �ڲ�����
//       HXEMV_FLOW_ERROR  : EMV���̴���
int iEmvSetTermParam(stTermParam *pTermParam, uchar *pszErrTag);

// װ���ն�֧�ֵ�Aid
// in  : pTermAid          : �ն�֧�ֵ�Aid
//       iFlag             : ����, HXEMV_PARA_INIT:��ʼ�� HXEMV_PARA_ADD:���� HXEMV_PARA_DEL:ɾ��
// out : pszErrTag         : ������ó���, �����Tagֵ
// ret : HXEMV_OK          : OK
//       HXEMV_LACK_MEMORY : �洢�ռ䲻��
//       HXEMV_PARA        : ��������, ����Tag��pszErrTag
//       HXEMV_FLOW_ERROR  : EMV���̴���
// Note: ɾ��ʱֻ����AID����
int iEmvLoadTermAid(stTermAid *pTermAid, int iFlag, uchar *pszErrTag);

// װ��CA��Կ
// in  : pCAPublicKey      : CA��Կ
//       iFlag             : ����, HXEMV_PARA_INIT:��ʼ�� HXEMV_PARA_ADD:���� HXEMV_PARA_DEL:ɾ��
// ret : HXEMV_OK          : OK
//       HXEMV_LACK_MEMORY : �洢�ռ䲻��
//       HXEMV_PARA        : ��������
//       HXEMV_FLOW_ERROR  : EMV���̴���
// Note: ɾ��ʱֻ����RID��index����
int iEmvLoadCAPublicKey(stCAPublicKey *pCAPublicKey, int iFlag);

#ifdef __cplusplus
}
#endif

#endif
