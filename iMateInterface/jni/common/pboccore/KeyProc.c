/**************************************
File name     : KEYPROC.C
Function      : Pboc2.0������Կ��ش���
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

// ǩ�ֺ���
// in  : psMsg1           : ��Ϣ, ���ȱ���Ϊ��Կģ������-22(Ϊ����ǩ������MSG��MSG1����)
//       psHashCode       : MSG1+MSG2��hashcode
//       iPrivateKeyIndex : ˽Կ����
//       iPrivateKeyLen   : ˽Կ����(in bytes)
// out : psSignature      : ����ǩ��(���ȵ���˽Կģ������)
//       pszErrMsg        : ������Ϣ
// ret : 0                : OK
//       1                : refer pszErrMsg
// Note: ���ó���Ҫ�Լ�����hashֵ
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
        sprintf(pszErrMsg, "���ܻ�˽Կ�������, ����[%d], ������[%04X]", iPrivateKeyIndex, iRet);
        return(1);
    }
    return(0);
}

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
							  uchar *pszErrMsg)
{
    int   iIssuerRsaKeyLen; // ������˽Կģ������
	uchar sBuf[300];
	uchar sTmp[80];
	int   iKeyBlockLen; // ��Կ���ֳ���
	uchar ucEBlockLen;  // ����ָ������
	uchar sHashCode[20];
    long  lE;
	int   iRet;

    iRet = iKeyRsaGetInfo(iIssuerRsaKeyIndex, &iIssuerRsaKeyLen, &lE, sBuf);
    if(iRet) {
        sprintf(pszErrMsg, "���ܻ���ȡRSA��Կ��Ϣ��, ����[%d], ������[%04X]", iIssuerRsaKeyIndex, iRet);
        return(1);
    }

	if(ucIccPkLen > iIssuerRsaKeyLen) {
		strcpy(pszErrMsg, "IC��RSA��Կ���ȳ����˷�����RSA��Կ����");
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
	ucEBlockLen     = lIccPkExponent==65537?3:1; // Icc Public Key Exponent Length (3��Ҫ1�ֽ�, 65537��Ҫ3�ֽ�)
	sBuf[19]        =  ucEBlockLen;
   	memset(sBuf+20, 0xBB, iIssuerRsaKeyLen - 42);
	if(ucIccPkLen+42 <= iIssuerRsaKeyLen) {
	    // Icc��Կ�Ƚ϶�,������Ҫ���'0xBB',��remainder
		iKeyBlockLen = iIssuerRsaKeyLen - 42;
		*pucIccPkRemainderLen = 0;
	} else {
		// Icc��Կ�Ƚϳ�,����Ҫ���'0xBB',��remainder
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
int iSignStaticData(ushort uiStaticDataLen, uchar *psStaticData, uchar *psDataAuthCode, int iIssuerRsaKeyIndex, uchar *pucSignedDataLen, uchar *psSignedData, uchar *pszErrMsg)
{
	int   iRet;
    int   iIssuerRsaKeyLen; // ������˽Կģ������
    long  lE;
	uchar sBuf[256];
	uchar sHashCode[20];

    iRet = iKeyRsaGetInfo(iIssuerRsaKeyIndex, &iIssuerRsaKeyLen, &lE, sBuf);
    if(iRet) {
        sprintf(pszErrMsg, "���ܻ���ȡRSA��Կ��Ϣ��, ����[%d], ������[%04X]", iIssuerRsaKeyIndex, iRet);
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

// ������������Կ֤�鲢���浽������
// Ϊ������������Կ֤�������,��debug��ʽ��,���ϵ�����vCopyClipBoard()��,�Ӽ�������ȡ������
// �ڼ��ܻ�������CA��Issuer��Կ�Ժ�, ������ֵ����iCaRsaKeyIndex��iIssuerRsaKeyIndex
void vGenerateIssuerPkCertificate(void)
{
	const int    CAPK_LEN = 1536;     // CA��Կ����(in bits)
	const int    ISSUERPK_LEN = 1280; // ��������Կ����(in bits)
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
    uchar sIssuerRsaModulus[256]; // ������RSA��Կģ��
    long  lCaRsaE;
    long  lIssuerRsaE; // ������RSA��Կ����ָ��

    // ���ڲ���������֤�����Կ����
    int   iCaRsaKeyIndex = 1;
    int   iIssuerRsaKeyIndex = 2;

    // ��ȡCA��Կ����
    iRet = iKeyRsaGetInfo(iCaRsaKeyIndex, &iCaKeyLen, &lCaRsaE, sBuf);
    if(iRet)
        return;
    // ��ȡIssuer��Կ����
    iRet = iKeyRsaGetInfo(iIssuerRsaKeyIndex, &iIssuerKeyLen, &lIssuerRsaE, sIssuerRsaModulus);
    if(iRet)
        return;
    if(iIssuerKeyLen > iCaKeyLen)
        return; // Issuer��Կ���ȱ������CA��Կ����

	// ��֤���ʽ��д
	sBuf[0]         = 0x02; // Issuer Public Key Certificate Format
	memcpy(sBuf+1,  "\x88\x88\x8F\xFF", 4); // Leftmost 3-8 digits of PAN **** ע��Ҫ�뽫��Ҫǩ�ֵĸ��˿���ƥ�� ****
	memcpy(sBuf+5,  "\x12\x49", 2); // Valide Date (MMYY)
	memcpy(sBuf+7,  "\x00\x00\x01", 3); // Serial No
	sBuf[10]        = 0x01; // Hash Algorithm Indicator (0x01:SHA-1)
    sBuf[11]        = 0x01; // Public Key Algorithm Indicator (0x01:RSA)
	sBuf[12]        = iIssuerKeyLen; // Issuer Public Key Length (1280b)
	// Issuer Public Key
    if(lIssuerRsaE == 3)
        sBuf[13] = 1; // e����
    else
        sBuf[13] = 3; // e ����
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

	// ǩ��
	vSHA1Init();
	vSHA1Update(sBuf, (ushort)(14+ucKeyBlockLen+sBuf[13]));
    vSHA1Result(sHashCode);
	
    iRet = iGenSignature(sBuf, sHashCode, iCaRsaKeyIndex, iCaKeyLen, sCertificate, szBuf);
    if(iRet)
        return;

    // ��ʾ���
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