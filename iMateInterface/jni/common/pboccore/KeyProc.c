/**************************************
File name     : KEYPROC.C
Function      : Pboc2.0发卡密钥相关处理
Author        : Yu Jun
First edition : Jul 3rd, 2009
Modified      : 
**************************************/
# include <stdio.h>
# include <string.h>
# include <ctype.h>
# include "VposFace.h"
# include "sha.h"
# include "KeyFace.h"
# include "keyproc.h"

// 签字函数
// in  : psMsg1           : 消息, 长度必须为密钥模数长度-22(为整个签字数据MSG的MSG1部分)
//       psHashCode       : MSG1+MSG2的hashcode
//       iPrivateKeyIndex : 私钥索引
//       iPrivateKeyLen   : 私钥长度(in bytes)
// out : psSignature      : 数字签名(长度等于私钥模数长度)
//       pszErrMsg        : 错误信息
// ret : 0                : OK
//       1                : refer pszErrMsg
// Note: 调用程序要自己计算hash值
static int iGenSignature(char *psMsg1, char *psHashCode, int iPrivateKeyIndex, int iPrivateKeyLen, char *psSignature, uchar *pszErrMsg)
{
	int    iRet;
	uchar  sBuf[256];

	sBuf[0] = 0x6A;
	memcpy(sBuf+1, psMsg1, iPrivateKeyLen-22);
	memcpy(sBuf+1+iPrivateKeyLen-22, psHashCode, 20);
	sBuf[iPrivateKeyLen-1] = 0xBC;

    iRet = iKeyRsaPrivateBlock(iPrivateKeyIndex, 0, NULL, iPrivateKeyLen, sBuf, psSignature);
    if(iRet) {
        sprintf(pszErrMsg, "加密机私钥运算出错, 索引[%d], 返回码[%04X]", iPrivateKeyIndex, iRet);
        return(1);
    }
    return(0);
}

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
							  uchar *pszErrMsg)
{
    int   iIssuerRsaKeyLen; // 发卡行私钥模数长度
	uchar sBuf[300];
	uchar sTmp[80];
	int   iKeyBlockLen; // 密钥部分长度
	uchar ucEBlockLen;  // 公共指数长度
	uchar sHashCode[20];
    long  lE;
	int   iRet;

    iRet = iKeyRsaGetInfo(iIssuerRsaKeyIndex, &iIssuerRsaKeyLen, &lE, sBuf);
    if(iRet) {
        sprintf(pszErrMsg, "加密机获取RSA密钥信息错, 索引[%d], 返回码[%04X]", iIssuerRsaKeyIndex, iRet);
        return(1);
    }

	if(ucIccPkLen > iIssuerRsaKeyLen) {
		strcpy(pszErrMsg, "IC卡RSA密钥长度超出了发卡行RSA密钥长度");
		return(1);
	}
    sBuf[0]         = 0x04; // Icc Public Key Certificate Format
	strcpy(sTmp, pszPAN);
	strcat(sTmp, "FFFFFFFFFFFFFFFFFFFF");
	vTwoOne(sTmp, 20, sBuf+1); // PAN
	vTwoOne(pszExpireDate+2, 2, sBuf+11); // Valid Date MM
	vTwoOne(pszExpireDate, 2, sBuf+12); // Valid Date YY
	memcpy(sBuf+13,  "\x00\x00\x01", 3); // Serial No
	sBuf[16]        = 0x01; // Hash Algorithm Indicator (0x01:SHA-1)
    sBuf[17]        = 0x01; // Public Key Algorithm Indicator (0x01:RSA)
	sBuf[18]        = ucIccPkLen; // Icc Public Key Length
	ucEBlockLen     = lIccPkExponent==65537?3:1; // Icc Public Key Exponent Length (3需要1字节, 65537需要3字节)
	sBuf[19]        =  ucEBlockLen;
   	memset(sBuf+20, 0xBB, iIssuerRsaKeyLen - 42);
	if(ucIccPkLen+42 <= iIssuerRsaKeyLen) {
	    // Icc密钥比较短,可能需要填充'0xBB',无remainder
		iKeyBlockLen = iIssuerRsaKeyLen - 42;
		*pucIccPkRemainderLen = 0;
	} else {
		// Icc密钥比较长,不需要填充'0xBB',有remainder
		iKeyBlockLen = ucIccPkLen;
		*pucIccPkRemainderLen = ucIccPkLen + 42 - iIssuerRsaKeyLen;
		memcpy(psIccPkRemainder, psIccPk + iIssuerRsaKeyLen - 42 , *pucIccPkRemainderLen);
	}
	memcpy(sBuf+20, psIccPk, ucIccPkLen); // Icc Public Key
	if(ucEBlockLen == 3) {
		memcpy(sBuf+20+iKeyBlockLen, "\x01\x00\x01", ucEBlockLen);
	} else {
		memcpy(sBuf+20+iKeyBlockLen, "\x03", ucEBlockLen);
	}

	vSHA1Init();
	vSHA1Update(sBuf, (ushort)(20+iKeyBlockLen+ucEBlockLen));
	vSHA1Update(psStaticData, (ushort)uiStaticDataLen);
    vSHA1Result(sHashCode);

    iRet = iGenSignature(sBuf, sHashCode, iIssuerRsaKeyIndex, iIssuerRsaKeyLen, psIccPkCertificate, pszErrMsg);
	if(iRet)
		return(iRet);
    *pucIccPkCertificateLen = iIssuerRsaKeyLen;
	return(0);
}

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
int iSignStaticData(ushort uiStaticDataLen, uchar *psStaticData, uchar *psDataAuthCode, int iIssuerRsaKeyIndex, uchar *pucSignedDataLen, uchar *psSignedData, uchar *pszErrMsg)
{
	int   iRet;
    int   iIssuerRsaKeyLen; // 发卡行私钥模数长度
    long  lE;
	uchar sBuf[256];
	uchar sHashCode[20];

    iRet = iKeyRsaGetInfo(iIssuerRsaKeyIndex, &iIssuerRsaKeyLen, &lE, sBuf);
    if(iRet) {
        sprintf(pszErrMsg, "加密机获取RSA密钥信息错, 索引[%d], 返回码[%04X]", iIssuerRsaKeyIndex, iRet);
        return(1);
    }
	memset(sBuf, 0xBB, 256);
	sBuf[0] = 0x03; // signed data format
	sBuf[1] = 0x01;  // Hash Algorithm Indicator (0x01:SHA-1)
	memcpy(sBuf+2, psDataAuthCode, 2);
	vSHA1Init();
	vSHA1Update(sBuf, (ushort)(iIssuerRsaKeyLen-22)); // 0xBB...
	vSHA1Update(psStaticData, uiStaticDataLen);       // Static Data
    vSHA1Result(sHashCode);
	*pucSignedDataLen = iIssuerRsaKeyLen;
	iRet = iGenSignature(sBuf, sHashCode, iIssuerRsaKeyIndex, iIssuerRsaKeyLen, psSignedData, pszErrMsg);
	return(iRet);
}

#if 0
# include <windows.h>

// 产生发卡方公钥证书并保存到剪贴板
// 为产生发卡方公钥证书等数据,在debug方式下,将断点设在vCopyClipBoard()后,从剪贴板中取出数据
// 在加密机产生好CA及Issuer密钥对后, 将索引值赋给iCaRsaKeyIndex和iIssuerRsaKeyIndex
void vGenerateIssuerPkCertificate(void)
{
	const int    CAPK_LEN = 1536;     // CA公钥长度(in bits)
	const int    ISSUERPK_LEN = 1280; // 发卡方公钥长度(in bits)
	uchar ucCaKeyLen = CAPK_LEN/8;    // CA public key length in bytes
	uchar ucKeyLen = ISSUERPK_LEN/8;  // issuer public key length in bytes
	uchar ucKeyBlockLen; // ==ucKeyLen or == ucKeyLen+BBsLen

	uchar sBuf[300];
	uchar szBuf[1000];
	uchar sCertificate[256];
	uchar sHashCode[20];
	int   iRemainderLen;
	uchar sRemainder[256];
    int   iRet;

    int   iCaKeyLen, iIssuerKeyLen;
    uchar sIssuerRsaModulus[256]; // 发卡行RSA密钥模数
    long  lCaRsaE;
    long  lIssuerRsaE; // 发卡行RSA密钥公共指数

    // 用于产生发卡行证书的密钥索引
    int   iCaRsaKeyIndex = 1;
    int   iIssuerRsaKeyIndex = 2;

    // 获取CA公钥长度
    iRet = iKeyRsaGetInfo(iCaRsaKeyIndex, &iCaKeyLen, &lCaRsaE, sBuf);
    if(iRet)
        return;
    // 获取Issuer公钥数据
    iRet = iKeyRsaGetInfo(iIssuerRsaKeyIndex, &iIssuerKeyLen, &lIssuerRsaE, sIssuerRsaModulus);
    if(iRet)
        return;
    if(iIssuerKeyLen > iCaKeyLen)
        return; // Issuer密钥长度必须短于CA密钥长度

	// 按证书格式填写
	sBuf[0]         = 0x02; // Issuer Public Key Certificate Format
	memcpy(sBuf+1,  "\x88\x88\x8F\xFF", 4); // Leftmost 3-8 digits of PAN **** 注意要与将来要签字的个人卡号匹配 ****
	memcpy(sBuf+5,  "\x12\x49", 2); // Valide Date (MMYY)
	memcpy(sBuf+7,  "\x00\x00\x01", 3); // Serial No
	sBuf[10]        = 0x01; // Hash Algorithm Indicator (0x01:SHA-1)
    sBuf[11]        = 0x01; // Public Key Algorithm Indicator (0x01:RSA)
	sBuf[12]        = iIssuerKeyLen; // Issuer Public Key Length (1280b)
	// Issuer Public Key
    if(lIssuerRsaE == 3)
        sBuf[13] = 1; // e长度
    else
        sBuf[13] = 3; // e 长度
    memcpy(sBuf+14, sIssuerRsaModulus, iIssuerKeyLen);
	vTwoOne("DC049819455574FDC444D1A562F65AFA7D64EABB71517B3F9345123A0C6662EB280539E0576B7062C06386F56889448DAE793A65F1928E50F1B8389BE75658422D7BD249E7338A5A23F5F9B4454040CAD1A56AE60071C747608E7A3AF4B36544143CC381816DE69A6230EEEBA864A6F5DC3CE389068D17683D203A992D56E0CF6F4F14C801349B39A504B9AB8C2F7F24BD098BE214F384051E5726D84981416F", (ushort)(ucKeyLen*2), sBuf+14);
	ucKeyBlockLen = iIssuerKeyLen;
	if(iIssuerKeyLen+36 < iCaKeyLen) {
		memset(sBuf+14+iIssuerKeyLen, 0xBB, iCaKeyLen-(iIssuerKeyLen+36)); // fill 'BB's
		ucKeyBlockLen = iCaKeyLen - 36;
	}
    if(lIssuerRsaE == 3)
    	memcpy(sBuf+14+ucKeyBlockLen, "\x03", 1); // exponent == 3
    else
    	memcpy(sBuf+14+ucKeyBlockLen, "\x01\x00\x01", 3); // exponent == 65537

	// 签字
	vSHA1Init();
	vSHA1Update(sBuf, (ushort)(14+ucKeyBlockLen+sBuf[13]));
    vSHA1Result(sHashCode);
	
    iRet = iGenSignature(sBuf, sHashCode, iCaRsaKeyIndex, iCaKeyLen, sCertificate, szBuf);
    if(iRet)
        return;

    // 显示结果
    iRemainderLen = 0;
    if(iIssuerKeyLen+36 > iCaKeyLen) {
		// remainder exist
		iRemainderLen = iIssuerKeyLen + 36 - iCaKeyLen;
		memcpy(sRemainder, sBuf+14+iIssuerKeyLen-iRemainderLen, iRemainderLen);
	}
 
	sprintf(szBuf, "CAPkLength     = %d\n", iCaKeyLen);
	sprintf(szBuf+strlen(szBuf), "IssuerPkLength = %d\n", iIssuerKeyLen);
    if(lIssuerRsaE == 3)
		sprintf(szBuf+strlen(szBuf), "e = 3\n");
	else
		sprintf(szBuf+strlen(szBuf), "e = 65537\n");
	sprintf(szBuf+strlen(szBuf), "IssuerPkCertificate : ");
	vOneTwo0(sCertificate, iCaKeyLen, szBuf+strlen(szBuf));
	sprintf(szBuf+strlen(szBuf), "\nIssuerPkRemainderLen = %d\n", iRemainderLen);
	if(iRemainderLen) {
		sprintf(szBuf+strlen(szBuf), "IssuerPkRemainder = ");
		vOneTwo0(sRemainder, (ushort)iRemainderLen, szBuf+strlen(szBuf));
		strcat(szBuf+strlen(szBuf), "\n");
	}

    // copy to clipboard
    if(OpenClipboard(NULL)) {
        HGLOBAL clipbuffer;
        char * buffer;
        EmptyClipboard();
        clipbuffer = GlobalAlloc(GMEM_DDESHARE, strlen(szBuf)+1);
        buffer = (char*)GlobalLock(clipbuffer);
        strcpy(buffer, szBuf);
        GlobalUnlock(clipbuffer);
        SetClipboardData(CF_TEXT,clipbuffer);
        CloseClipboard();
/*
        CAPkLength     = 192
        IssuerPkLength = 160
        PanLeft = 88888
        e = 3
        IssuerPkCertificate : 7391CBCF974DC9F720B683B333C0B19C4649259C8AF5CA2B1A09A90C6D2549DB8EB9704E6C58CE5684E40AF27691EDD6F07E5BCA6E30AB6125339CA18ED442DF1F35DA9AF20B774F62A5693F3948B32942D8B2989DC2CA72E69FE02606791BDB85921AE56AE3BB0FA1F52BB23C37344399BA78A18FBC4EF574623A1DD8C56853B32C7CFA6F6492807EA6C95480471393D3E68FD1D2FC12D09B094A6F0696ABA150D109FE740375E47BC264D9F64A9E813173D1F624B9650161B40BB258BF0E5A
        IssuerPkRemainderLen = 4
        IssuerPkRemainder = 4981416F
*/
    }
}
#endif