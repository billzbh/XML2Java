/**************************************
File name     : EmvMsg.h
Function      : EMV/pboc2.0������֧����Ϣ��Դ��
Author        : Yu Jun
First edition : Apr 6th, 2012
Modified      : 
**************************************/
#ifndef _EMVMSG_H
#define _EMVMSG_H

#include "EmvMCode.h" // EMV��Ϣ�����

// ��ȡ֧�ֵ�����
// out : pszLanguages : ֧�ֵ�����, ����:"enzh"��ʾ֧��Ӣ�ġ�����
//       pszDescs     : ��'='��β��֧�ֵ���������, ����:"ENGLISH=����="������Ӣ�ġ�����
// ret : 0            : OK
//       1            : ���ô���
// ����ı�����������, Ҫ�ֹ��޸ı������Է�����ȷ������
int iEmvMsgTableSupportedLanguage(uchar *pszLanguages, uchar *pszDescs);

// ��ʼ��������֧����Ϣ��, ��EMV���ĳ�ʼ��ʱ����
// in  : pszDefaultLanguage : ȱʡ����, ISO-639-1, 2�ֽ����Ա�ʾ
// ret : 0 : OK
//       1 : ���Բ�֧��, ��ʱ��ʹ���ڶ�ȱʡ����
int iEmvMsgTableInit(uchar *pszDefaultLanguage);

// ��ȡƥ������
// in  : pszCardPreferredLanguage : �ֿ���ƫ�������б�
// out : pszFinalLanguage         : ƥ������[2]
// ret : 0 : OK, �ҵ�ƥ������
//       1 : û��ƥ������ԣ����ص�ƥ������ʵΪȱʡ����
int iEmvMsgTableGetFitLanguage(uchar *pszCardPreferredLanguage, uchar *pszFinalLanguage);

// ��ȡPinpadƥ������
// in  : pszCardPreferredLanguage : �ֿ���ƫ�������б�
//       pszPinpadLanguage        : Pinpad֧�ֵ�����
// out : pszFinalLanguage         : ƥ������[2]
// ret : 0 : OK, �ҵ�ƥ������
//       1 : û��ƥ������ԣ����ص�ƥ������ʵΪȱʡ����
int iEmvMsgTableGetPinpadLanguage(uchar *pszCardPreferredLanguage, uchar *pszPinpadLanguage, uchar *pszFinalLanguage);

// ��������
// in  : psLanguageCode : ISO-639-1��׼��2�ֽ����Դ���
// ret : 0              : OK
//       1              : ���Բ�֧��
// Note: �����֧��ָ�������ԣ����趨Ϊȱʡ����
//       ���趨���ȡ��ƥ�����Բ�ͬ�����ԣ����ǣ���ʾ����ǿ�Ʋ��ø�����
int iEmvMsgTableSetLanguage(uchar *psLanguageCode);

// ��ȡĳ��Ϣ���ͱ�ʾ����Ϣ
// in  : iMsgType : ��Ϣ����
//       psLanguageCode : ISO-639-1��׼��2�ֽ����Դ���
//           NULL��ʾʹ��iEmvMsgTableSetLanguage()�趨������
// ret : ��Ϣ, ���û�ж�Ӧ��Ϣ�����ؿ��ַ���
char *pszEmvMsgTableGetInfo(int iMsgType, uchar *psLanguageCode);

#endif
