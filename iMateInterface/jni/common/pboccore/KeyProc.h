/**************************************
File name     : KEYPROC.H
Function      : EMV发卡密钥相关处理
Author        : Yu Jun
First edition : Jul 3rd, 2009
Modified      : 
**************************************/
#ifndef _KEYPROC_H
#define _KEYPROC_H
#ifdef __cplusplus
extern "C" {
#endif

// 产生IC卡公钥证书(Icc Public Key Certificate)
// in  : pszPAN					: PAN
//       pszExpireDate			: 有效期 (YYMMDD)
//	     ucIccPkLen				: Icc Public Key modulus length
//		 lIccPkExponent			: Icc Public Key public exponent(3 or 65537)
//       uiStaticDataLen        : Static Data To Be Authenticated length
//       psStaticData           : Static Data To Be Authenticated
//       iIssuerRsaKeyIndex     : Issuer Private Key Index
// out : pucIccPkCertificateLen	: Icc Public Key Certificate length
//       psIccPkCertificate		: Icc Public Key Certificate
//		 pucIccPkRemainderLen	: Icc Public Key Remainder length
//		 psIccPkRemainder		: Icc Public Key Remainder
//       pszErrMsg   : 错误信息
// ret : 0           : OK
//       1           : refer pszErrMsg
// Note: 确保sg_IssuerPrivateKey已经被赋值
int iGenerateIccPkCertificate(uchar *pszPAN, uchar *pszExpireDate, uchar ucIccPkLen, uchar *psIccPk, long lIccPkExponent,
							  ushort uiStaticDataLen, uchar *psStaticData,
                              int   iIssuerRsaKeyIndex,
							  uchar *pucIccPkCertificateLen, uchar *psIccPkCertificate, uchar *pucIccPkRemainderLen, uchar *psIccPkRemainder,
							  uchar *pszErrMsg);

// 产生静态数据签字(Icc Signed Static Application Data)
// in    uiStaticDataLen        : Static Data To Be Authenticated length
//       psStaticData           : Static Data To Be Authenticated
//       psDataAuthCode;        : Data Authentication Code
//       iIssuerRsaKeyIndex     : Issuer Private Key Index
// out : pucSignedDataLen   	: length of Signed Static Application Data
//       psSignedData   		: Signed Static Application Data
//       pszErrMsg   : 错误信息
// ret : 0           : OK
//       1           : refer pszErrMsg
// Note: 确保sg_IssuerPrivateKey已经被赋值
int iSignStaticData(ushort uiStaticDataLen, uchar *psStaticData, uchar *psDataAuthCode, int iIssuerRsaKeyIndex, uchar *pucSignedDataLen, uchar *psSignedData, uchar *pszErrMsg);

// 产生发卡方公钥证书并保存到剪贴板
// 为产生发卡方公钥证书等数据,在debug方式下,将断点设在vCopyClipBoard()后,从剪贴板中取出数据
// 在加密机产生好CA及Issuer密钥对后, 将索引值赋给iCaRsaKeyIndex和iIssuerRsaKeyIndex
void vGenerateIssuerPkCertificate(void);

#ifdef __cplusplus
}
#endif
#endif
