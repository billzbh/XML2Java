/**************************************
File name     : KEYPROC.H
Function      : EMV������Կ��ش���
Author        : Yu Jun
First edition : Jul 3rd, 2009
Modified      : 
**************************************/
#ifndef _KEYPROC_H
#define _KEYPROC_H
#ifdef __cplusplus
extern "C" {
#endif

// ����IC����Կ֤��(Icc Public Key Certificate)
// in  : pszPAN					: PAN
//       pszExpireDate			: ��Ч�� (YYMMDD)
//	     ucIccPkLen				: Icc Public Key modulus length
//		 lIccPkExponent			: Icc Public Key public exponent(3 or 65537)
//       uiStaticDataLen        : Static Data To Be Authenticated length
//       psStaticData           : Static Data To Be Authenticated
//       iIssuerRsaKeyIndex     : Issuer Private Key Index
// out : pucIccPkCertificateLen	: Icc Public Key Certificate length
//       psIccPkCertificate		: Icc Public Key Certificate
//		 pucIccPkRemainderLen	: Icc Public Key Remainder length
//		 psIccPkRemainder		: Icc Public Key Remainder
//       pszErrMsg   : ������Ϣ
// ret : 0           : OK
//       1           : refer pszErrMsg
// Note: ȷ��sg_IssuerPrivateKey�Ѿ�����ֵ
int iGenerateIccPkCertificate(uchar *pszPAN, uchar *pszExpireDate, uchar ucIccPkLen, uchar *psIccPk, long lIccPkExponent,
							  ushort uiStaticDataLen, uchar *psStaticData,
                              int   iIssuerRsaKeyIndex,
							  uchar *pucIccPkCertificateLen, uchar *psIccPkCertificate, uchar *pucIccPkRemainderLen, uchar *psIccPkRemainder,
							  uchar *pszErrMsg);

// ������̬����ǩ��(Icc Signed Static Application Data)
// in    uiStaticDataLen        : Static Data To Be Authenticated length
//       psStaticData           : Static Data To Be Authenticated
//       psDataAuthCode;        : Data Authentication Code
//       iIssuerRsaKeyIndex     : Issuer Private Key Index
// out : pucSignedDataLen   	: length of Signed Static Application Data
//       psSignedData   		: Signed Static Application Data
//       pszErrMsg   : ������Ϣ
// ret : 0           : OK
//       1           : refer pszErrMsg
// Note: ȷ��sg_IssuerPrivateKey�Ѿ�����ֵ
int iSignStaticData(ushort uiStaticDataLen, uchar *psStaticData, uchar *psDataAuthCode, int iIssuerRsaKeyIndex, uchar *pucSignedDataLen, uchar *psSignedData, uchar *pszErrMsg);

// ������������Կ֤�鲢���浽������
// Ϊ������������Կ֤�������,��debug��ʽ��,���ϵ�����vCopyClipBoard()��,�Ӽ�������ȡ������
// �ڼ��ܻ�������CA��Issuer��Կ�Ժ�, ������ֵ����iCaRsaKeyIndex��iIssuerRsaKeyIndex
void vGenerateIssuerPkCertificate(void);

#ifdef __cplusplus
}
#endif
#endif
