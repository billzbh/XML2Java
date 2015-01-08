/**************************************
File name     : MAKEDGI.C
Function      : 定义PBOC2.0 D/C发卡数据相关结构
Author        : Yu Jun
First edition : Feb 21st, 2011
Note          : 构造数据分组
**************************************/
# include <stdio.h>
# include <string.h>
# include <ctype.h>
# include "VposFace.h"
# include "TlvFunc.h"
# include "KeyProc.h"
# include "IssData.h"
# include "MakeDgi.h"

// pboc2.0应用数据分组集合
long gl_alPbocDgis[] = {
	0x9104, 0x9203, /*0x9206, msd*/ 0x9207, 0x9102, 0x9103,
	0x0101, 0x0102, 0x0201, 0x0202, 0x0203, 0x0204, 0x0205, 0x0206, 0x0207, 0x0301, 0x0302, 0x0303, 0x0401,
	0x0D01, 0x0E01,
	0x9200,
	0x8000, 0x9000,
	0x8205, 0x8204, 0x8203, 0x8202, 0x8201,
	0x8010, 0x9010,
	0x10000 // 结束标志
};

// 初始化一个数据分组缓冲区
// in  : lDgi     : 数据分组标识
// out : psDgiBuf : 数据分组数据块
// ret : 0        : OK
// Note: 由于IC卡指令限制，数据分组长度只允许1字节，最大值为251
static int iDgiInit(long lDgi, uchar *psDgiBuf)
{
	psDgiBuf[0] = (lDgi>>8) & 0xFF;
	psDgiBuf[1] = lDgi & 0xFF;
	psDgiBuf[2] = 0; // 当前数据分组缓冲区无内容
	return(0);
}
// 将记录数据分组数据内容用Tag70包装起来
// in  : psDgiBuf  : 当前数据分组缓冲区
// out : psDgiBuf  : 打包后数据分组缓冲区
// ret : 0         : OK
//       1         : 数据缓冲区溢出
static int iDgiPack70(uchar *psDgiBuf)
{
    uchar sBuf[256];
    int   iRet;
    iRet = iTlvMakeObject("\x70", (ushort)psDgiBuf[2], psDgiBuf+3, sBuf, sizeof(sBuf));
    if(iRet <= 0)
        return(1);
    psDgiBuf[2] = iRet;
    memcpy(psDgiBuf+3, sBuf, iRet);
    return(0);
}
// 增加一个项目到数据分组缓冲区中
// in  : psDgiBuf  : 当前数据分组缓冲区，新的项目将要增加到其中去
//       psItem    : 项目内容
//       uiItemLen : 项目长度
// out : psDgiBuf  : 新项目将被增加进来
// ret : 0         : OK
//       1         : 数据缓冲区溢出
static int iDgiAddItem(uchar *psDgiBuf, uchar *psItem, uint uiItemLen)
{
	if(uiItemLen > 251)
		return(1); // 溢出
	if(psDgiBuf[2] + uiItemLen > 251)
		return(1); // 溢出
	memcpy(psDgiBuf+3+psDgiBuf[2], psItem, uiItemLen);
    psDgiBuf[2] += uiItemLen;
	return(0);
}
// 增加一个TLV项目到数据分组缓冲区中
// in  : psDgiBuf : 当前数据分组缓冲区，新的项目将要增加到其中去
//       psTag    : 项目Tag
//       uiLength : 项目内容长度
//       psValue  : 项目内容
// out : psDgiBuf : 新项目将被增加进来
// ret : 0        : OK
//       1        : 数据缓冲区溢出
//       2        : 项目长度太长
//       3        : TLV对象构造失败
static int iDgiAddItemByTLV(uchar *psDgiBuf, uchar *psTag, uint uiLength, uchar *psValue)
{
	uchar sBuf[256];
	int   iRet, iLen;

	if(uiLength > 249)
		return(2);
	iLen = iTlvMakeObject(psTag, (ushort)uiLength, psValue, sBuf, sizeof(sBuf));
	if(iLen <= 0)
		return(3);
	iRet = iDgiAddItem(psDgiBuf, sBuf, iLen);
	if(iRet)
		return(1);
	return(0);
}
// 从TLV数据库中取一个项目，将其增加到数据分组缓冲区中
// in  : psDgiBuf : 当前数据分组缓冲区，新的项目将要增加到其中去
//       psTag    : 项目Tag
//       psTlvBuf : TLV数据库
// out : psDgiBuf : 新项目将被增加进来
// ret : 0        : OK
//       1        : 数据缓冲区溢出
//       4        : 在TLV数据库中未找到响应Tag
static int iDgiAddItemFromTLVBuf(uchar *psDgiBuf, uchar *psTag, uchar *psTlvBuf)
{
	uchar *psTlvObject;
	int   iRet, iLen;
	iLen = iTlvGetObj(psTlvBuf, psTag, &psTlvObject);
	if(iLen <= 0)
		return(4);
	iRet = iDgiAddItem(psDgiBuf, psTlvObject, iLen);
	if(iRet)
		return(1);
	return(0);
}


// 构造Pboc2.0借贷记卡应用数据分组
// in  : lDgi      : 数据分组标识
// out : psDgiBuf : 数据分组数据
// ret : >0        : OK, 值为数据分组数据长度
//       0         : 不认识的数据分组
//       -1        : 其它错误
int iMakePbocDgi(long lDgi, uchar *psDgiBuf)
{
	int   iLen;
//	uchar sDgiBuf[300];
	uchar sBuf[256];
	int   iRet;

	iDgiInit(lDgi, psDgiBuf); // 初始化数据分组数据缓冲区
	switch(lDgi) {
    case 0x0101:
        iRet = iDgiAddItemByTLV(psDgiBuf, "\x57", gl_AppRef.ucTrack2EquDataLen, gl_AppRef.sTrack2EquData); // T57
		if(iRet)
			return(-1);
        if(gl_AppRef.ucPANSerialNo != 0xFF) {
            iRet = iDgiAddItemByTLV(psDgiBuf, "\x5F\x34", 1, &gl_AppRef.ucPANSerialNo); // T5F34
	    	if(iRet)
		    	return(-1);
        }
        iRet = iDgiPack70(psDgiBuf);
		if(iRet)
			return(-1);
        break;
    case 0x0102:
		if(strlen(gl_AppRef.szHolderName) <= 26) {
	        iRet = iDgiAddItemByTLV(psDgiBuf, "\x5F\x20", strlen(gl_AppRef.szHolderName), gl_AppRef.szHolderName); // T5F20
			if(iRet)
				return(-1);
		} else {
            iRet = iDgiAddItemByTLV(psDgiBuf, "\x9F\x0B", strlen(gl_AppRef.szHolderName), gl_AppRef.szHolderName); // T9F0B
	    	if(iRet)
		    	return(-1);
		}
        iRet = iDgiAddItemByTLV(psDgiBuf, "\x9F\x61", strlen(gl_AppRef.szHolderId), gl_AppRef.szHolderId); // T9F61
		if(iRet)
			return(-1);
        iRet = iDgiAddItemByTLV(psDgiBuf, "\x9F\x62", 1, &gl_AppRef.ucHolderIdType); // T9F62
		if(iRet)
			return(-1);
        iRet = iDgiPack70(psDgiBuf);
		if(iRet)
			return(-1);
        break;
    case 0x0201:
        iRet = iDgiAddItemByTLV(psDgiBuf, "\x90", gl_AppRef.ucIssuerPkCertificateLen, gl_AppRef.sIssuerPkCertificate); // T90
		if(iRet)
			return(-1);
        iRet = iDgiPack70(psDgiBuf);
		if(iRet)
			return(-1);
        break;
    case 0x0202:
        iRet = iDgiAddItemByTLV(psDgiBuf, "\x9F\x32", gl_AppRef.ucIssuerPkExponentLen, gl_AppRef.sIssuerPkExponent); // T9F32
		if(iRet)
			return(-1);
        iRet = iDgiAddItemByTLV(psDgiBuf, "\x92", gl_AppRef.ucIssuerPkRemainderLen, gl_AppRef.sIssuerPkRemainder); // T92
		if(iRet)
			return(-1);
        iRet = iDgiAddItemByTLV(psDgiBuf, "\x8F", 1, &gl_AppRef.ucCaPublicKeyIndex); // T8F
		if(iRet)
			return(-1);
        iRet = iDgiPack70(psDgiBuf);
		if(iRet)
			return(-1);
        break;
    case 0x0203:
        iRet = iDgiAddItemByTLV(psDgiBuf, "\x93", gl_AppRef.ucDcSignedStaticDataLen, gl_AppRef.sDcSignedStaticData); // T93
		if(iRet)
			return(-1);
        iRet = iDgiPack70(psDgiBuf);
		if(iRet)
			return(-1);
        break;
    case 0x0204:
        iRet = iDgiAddItemByTLV(psDgiBuf, "\x9F\x46", gl_AppRef.ucDcIcCertificateLen, gl_AppRef.sDcIcCertificate); // T9F46
		if(iRet)
			return(-1);
        iRet = iDgiPack70(psDgiBuf);
		if(iRet)
			return(-1);
        break;
    case 0x0205:
        iRet = iDgiAddItemByTLV(psDgiBuf, "\x9F\x47", gl_AppRef.ucIcPublicKeyELen, gl_AppRef.sIcPublicKeyE); // T9F47
		if(iRet)
			return(-1);
        iRet = iDgiAddItemByTLV(psDgiBuf, "\x9F\x48", gl_AppRef.ucIcPublicKeyRemainderLen, gl_AppRef.sIcPublicKeyRemainder); // T9F48
		if(iRet)
			return(-1);
        iRet = iDgiAddItemByTLV(psDgiBuf, "\x9F\x49", gl_AppRef.ucDDOLLen, gl_AppRef.sDDOL); // T9F49
		if(iRet)
			return(-1);
        iRet = iDgiPack70(psDgiBuf);
		if(iRet)
			return(-1);
        break;
    case 0x0206:
        iRet = iDgiAddItemByTLV(psDgiBuf, "\x93", gl_AppRef.ucECashSignedStaticDataLen, gl_AppRef.sECashSignedStaticData); // T93
		if(iRet)
			return(-1);
        iRet = iDgiPack70(psDgiBuf);
		if(iRet)
			return(-1);
        break;
    case 0x0207:
        iRet = iDgiAddItemByTLV(psDgiBuf, "\x9F\x46", gl_AppRef.ucECashIcCertificateLen, gl_AppRef.sECashIcCertificate); // T9F46
		if(iRet)
			return(-1);
        iRet = iDgiPack70(psDgiBuf);
		if(iRet)
			return(-1);
        break;
    case 0x0301: // 标准DC卡风险管理数据
        iRet = iDgiAddItemByTLV(psDgiBuf, "\x5A", 10, gl_AppRef.sPAN); // T5A
		if(iRet)
			return(-1);
        if(gl_AppRef.ucDcCVMListLen) {
            iRet = iDgiAddItemByTLV(psDgiBuf, "\x8E", gl_AppRef.ucDcCVMListLen, gl_AppRef.sDcCVMList); // T8E
	    	if(iRet)
    			return(-1);
        }
        iRet = iDgiAddItemByTLV(psDgiBuf, "\x9F\x0D", 5, gl_AppRef.sDcIACDefault); // T9F0D
		if(iRet)
			return(-1);
        iRet = iDgiAddItemByTLV(psDgiBuf, "\x9F\x0E", 5, gl_AppRef.sDcIACDenial); // T9F0E
		if(iRet)
			return(-1);
        iRet = iDgiAddItemByTLV(psDgiBuf, "\x9F\x0F", 5, gl_AppRef.sDcIACOnline); // T9F0F
		if(iRet)
			return(-1);
        iRet = iDgiAddItemByTLV(psDgiBuf, "\x5F\x24", 3, gl_AppRef.sExpireDate); // T5F24
		if(iRet)
			return(-1);
        iRet = iDgiAddItemByTLV(psDgiBuf, "\x5F\x28", 2, gl_AppRef.sIssuerCountryCode); // T5F28
		if(iRet)
			return(-1);
        iRet = iDgiAddItemByTLV(psDgiBuf, "\x9F\x42", 2, gl_AppRef.sAppCurrencyCode); // T9F42
      	if(iRet)
	    	return(-1);
        iRet = iDgiAddItemByTLV(psDgiBuf, "\x9F\x07", 2, gl_AppRef.sUsageControl); // T9F07
		if(iRet)
			return(-1);
        if(memcmp(gl_AppRef.sEffectDate, "\xFF\xFF\xFF", 3) != 0) {
            iRet = iDgiAddItemByTLV(psDgiBuf, "\x5F\x25", 3, gl_AppRef.sEffectDate); // T5F25
	    	if(iRet)
    			return(-1);
        }
        iRet = iDgiPack70(psDgiBuf);
		if(iRet)
			return(-1);
        break;
    case 0x0302:
        iRet = iDgiAddItemByTLV(psDgiBuf, "\x8C", gl_AppRef.ucCDOL1Len, gl_AppRef.sCDOL1); // T8C
		if(iRet)
			return(-1);
        iRet = iDgiAddItemByTLV(psDgiBuf, "\x8D", gl_AppRef.ucCDOL2Len, gl_AppRef.sCDOL2); // T8D
		if(iRet)
			return(-1);
        iRet = iDgiAddItemByTLV(psDgiBuf, "\x5F\x30", 2, gl_AppRef.sServiceCode); // T5F30
		if(iRet)
			return(-1);
        iRet = iDgiAddItemByTLV(psDgiBuf, "\x9F\x08", 2, gl_AppRef.sAppVerNo); // T9F08
		if(iRet)
			return(-1);
        if(gl_AppRef.ucSDATagListLen) {
            iRet = iDgiAddItemByTLV(psDgiBuf, "\x9F\x4A", gl_AppRef.ucSDATagListLen, gl_AppRef.sSDATagList); // T9F4A
	    	if(iRet)
		    	return(-1);
        }
        iRet = iDgiPack70(psDgiBuf);
		if(iRet)
			return(-1);
        break;
    case 0x0303: // eCash卡风险管理数据
        iRet = iDgiAddItemByTLV(psDgiBuf, "\x5A", 10, gl_AppRef.sPAN); // T5A
		if(iRet)
			return(-1);
        if(gl_AppRef.ucDcCVMListLen) {
            iRet = iDgiAddItemByTLV(psDgiBuf, "\x8E", gl_AppRef.ucECashCVMListLen, gl_AppRef.sECashCVMList); // T8E
	    	if(iRet)
    			return(-1);
        }
        iRet = iDgiAddItemByTLV(psDgiBuf, "\x9F\x0D", 5, gl_AppRef.sECashIACDefault); // T9F0D
		if(iRet)
			return(-1);
        iRet = iDgiAddItemByTLV(psDgiBuf, "\x9F\x0E", 5, gl_AppRef.sECashIACDenial); // T9F0E
		if(iRet)
			return(-1);
        iRet = iDgiAddItemByTLV(psDgiBuf, "\x9F\x0F", 5, gl_AppRef.sECashIACOnline); // T9F0F
		if(iRet)
			return(-1);
        iRet = iDgiAddItemByTLV(psDgiBuf, "\x5F\x24", 3, gl_AppRef.sExpireDate); // T5F24
		if(iRet)
			return(-1);
        iRet = iDgiAddItemByTLV(psDgiBuf, "\x5F\x28", 2, gl_AppRef.sIssuerCountryCode); // T5F28
		if(iRet)
			return(-1);
        iRet = iDgiAddItemByTLV(psDgiBuf, "\x9F\x42", 2, gl_AppRef.sAppCurrencyCode); // T9F42
      	if(iRet)
	    	return(-1);
        iRet = iDgiAddItemByTLV(psDgiBuf, "\x9F\x07", 2, gl_AppRef.sUsageControl); // T9F07
		if(iRet)
			return(-1);
        if(memcmp(gl_AppRef.sEffectDate, "\xFF\xFF\xFF", 3) != 0) {
            iRet = iDgiAddItemByTLV(psDgiBuf, "\x5F\x25", 3, gl_AppRef.sEffectDate); // T5F25
	    	if(iRet)
    			return(-1);
        }
        iRet = iDgiAddItemByTLV(psDgiBuf, "\x9F\x74", 6, gl_AppRef.sECashIssuerAuthCode); // T9F74
		if(iRet)
			return(-1);
        iRet = iDgiPack70(psDgiBuf);
		if(iRet)
			return(-1);
        break;
	case 0x0401:
        iRet = iDgiAddItemByTLV(psDgiBuf, "\x9F\x74", 6, gl_AppRef.sECashIssuerAuthCode); // T9F74
		if(iRet)
			return(-1);
        iRet = iDgiPack70(psDgiBuf);
		if(iRet)
			return(-1);
        break;

    case 0x0D01:
        iRet = iDgiAddItemByTLV(psDgiBuf, "\x9F\x58", 1, &gl_AppRef.ucLowerConsecutiveLimit); // T9F58
		if(iRet)
			return(-1);
        iRet = iDgiAddItemByTLV(psDgiBuf, "\x9F\x59", 1, &gl_AppRef.ucUpperConsecutiveLimit); // T9F59
		if(iRet)
			return(-1);
        iRet = iDgiAddItemByTLV(psDgiBuf, "\x9F\x53", 1, &gl_AppRef.ucConsecutiveTransLimit); // T9F53
		if(iRet)
			return(-1);
        iRet = iDgiAddItemByTLV(psDgiBuf, "\x9F\x54", 6, gl_AppRef.sCTTAL); // T9F54
		if(iRet)
			return(-1);
        iRet = iDgiAddItemByTLV(psDgiBuf, "\x9F\x5C", 6, gl_AppRef.sCTTAUL); // T9F5C
		if(iRet)
			return(-1);
        iRet = iDgiAddItemByTLV(psDgiBuf, "\x9F\x4F", gl_AppRef.ucLogFormatLen, gl_AppRef.sLogFormat); // T9F4F
		if(iRet)
			return(-1);
		if(memcmp(gl_AppRef.sAppVerNo, "\x00\x30", 2) >= 0) {
			// 3.0以上版本, 增加圈存日志
	        iRet = iDgiAddItemByTLV(psDgiBuf, "\xDF\x4F", gl_AppRef.ucLoadLogFormatLen, gl_AppRef.sLoadLogFormat); // TDF4F
			if(iRet)
				return(-1);
		}

        if(gl_AppRef.ucECashExistFlag) {
            iRet = iDgiAddItemByTLV(psDgiBuf, "\x9F\x77", 6, gl_AppRef.sECashBalanceLimit); // T9F77
	    	if(iRet)
		    	return(-1);
            iRet = iDgiAddItemByTLV(psDgiBuf, "\x9F\x78", 6, gl_AppRef.sECashSingleTransLimit); // T9F78
	    	if(iRet)
		    	return(-1);
            iRet = iDgiAddItemByTLV(psDgiBuf, "\x9F\x79", 6, gl_AppRef.sECashBalance); // T9F79
	    	if(iRet)
		    	return(-1);
            iRet = iDgiAddItemByTLV(psDgiBuf, "\x9F\x6D", 6, gl_AppRef.sECashResetThreshold); // T9F6D
	    	if(iRet)
		    	return(-1);
        }
        if(gl_AppRef.ucQpbocExistFlag) {
            iRet = iDgiAddItemByTLV(psDgiBuf, "\x9F\x68", 4, gl_AppRef.sQPbocAdditionalProcess); // T9F68
	    	if(iRet)
		    	return(-1);
            iRet = iDgiAddItemByTLV(psDgiBuf, "\x9F\x6C", 2, gl_AppRef.sQPbocTransQualifiers); // T9F6C
	    	if(iRet)
		    	return(-1);
            iRet = iDgiAddItemByTLV(psDgiBuf, "\x9F\x5D", 6, gl_AppRef.sQPbocSpendingAmount); // T9F5D
	    	if(iRet)
		    	return(-1);
	        iRet = iDgiAddItemByTLV(psDgiBuf, "\x9F\x6B", 6, gl_AppRef.sQPbocCvmLimitAmount); // T9F6b
			if(iRet)
				return(-1);
            iRet = iDgiAddItemByTLV(psDgiBuf, "\x57", gl_AppRef.ucTrack2EquDataLen, gl_AppRef.sTrack2EquData); // T57
	    	if(iRet)
		    	return(-1);
        }
        break;
    case 0x0E01:
        iRet = iDgiAddItemByTLV(psDgiBuf, "\x9F\x51", 2, gl_AppRef.sAppCurrencyCode); // T9F51
      	if(iRet)
	    	return(-1);
        iRet = iDgiAddItemByTLV(psDgiBuf, "\x9F\x52", 2, gl_AppRef.sAppDefaultAction); // T9F52
      	if(iRet)
	    	return(-1);
        if(gl_AppRef.ucPANSerialNo != 0xFF) {
            iRet = iDgiAddItemByTLV(psDgiBuf, "\x5F\x34", 1, &gl_AppRef.ucPANSerialNo); // T5F34
	    	if(iRet)
		    	return(-1);
        }
        iRet = iDgiAddItemByTLV(psDgiBuf, "\x9F\x56", 1, &gl_AppRef.ucIssuerAuthIndicator); // T9F56
		if(iRet)
			return(-1);
        break;
    case 0x9200: // GAC响应数据
        iRet = iDgiAddItemByTLV(psDgiBuf, "\x9F\x10", gl_AppRef.ucDcFciIssuerAppDataLen, gl_AppRef.sDcFciIssuerAppData); // T9F10
		if(iRet)
			return(-1);
        break;
    case 0x8000: // DES应用密钥
        memcpy(sBuf, gl_AppRef.sDesKeyAuth, 16);
        memcpy(sBuf+16, gl_AppRef.sDesKeyMac, 16);
        memcpy(sBuf+32, gl_AppRef.sDesKeyEnc, 16);
        iRet = iDgiAddItem(psDgiBuf, sBuf, 3*16);
		if(iRet)
			return(-1);
        break;
    case 0x8001: // MSD应用密钥
        memcpy(sBuf, gl_AppRef.sDesKeyAuth, 16);
        iRet = iDgiAddItem(psDgiBuf, sBuf, 16);
		if(iRet)
			return(-1);
        break;
    case 0x9000: // DES应用密钥校验
        memcpy(sBuf, gl_AppRef.sDesKeyAuth+16, 3);
        memcpy(sBuf+3, gl_AppRef.sDesKeyMac+16, 3);
        memcpy(sBuf+6, gl_AppRef.sDesKeyEnc+16, 3);
        iRet = iDgiAddItem(psDgiBuf, sBuf, 3*3);
		if(iRet)
			return(-1);
        break;
	case 0x8201: // qInv
        iLen = (gl_AppRef.ucIccPkLen/2+8)/8*8;
        iRet = iDgiAddItem(psDgiBuf, gl_AppRef.sRsaKeyQinv, iLen);
		if(iRet)
			return(-1);
        break;
	case 0x8202: // dQ
        iLen = (gl_AppRef.ucIccPkLen/2+8)/8*8;
        iRet = iDgiAddItem(psDgiBuf, gl_AppRef.sRsaKeyDq, iLen);
		if(iRet)
			return(-1);
        break;
	case 0x8203: // dP
        iLen = (gl_AppRef.ucIccPkLen/2+8)/8*8;
        iRet = iDgiAddItem(psDgiBuf, gl_AppRef.sRsaKeyDp, iLen);
		if(iRet)
			return(-1);
        break;
	case 0x8204: // Q
        iLen = (gl_AppRef.ucIccPkLen/2+8)/8*8;
        iRet = iDgiAddItem(psDgiBuf, gl_AppRef.sRsaKeyQ, iLen);
		if(iRet)
			return(-1);
        break;
	case 0x8205: // P
        iLen = (gl_AppRef.ucIccPkLen/2+8)/8*8;
        iRet = iDgiAddItem(psDgiBuf, gl_AppRef.sRsaKeyP, iLen);
		if(iRet)
			return(-1);
        break;
    case 0x8010: // 个人密码
        iRet = iDgiAddItem(psDgiBuf, gl_AppRef.sPinBlock, 8);
		if(iRet)
			return(-1);
        break;
    case 0x9010: // 个人密码计数器
        iRet = iDgiAddItem(psDgiBuf, gl_AppRef.sPinCounterRef, 2);
		if(iRet)
			return(-1);
        break;
    case 0x9104: // 标准借贷记交易GPO响应数据
        iRet = iDgiAddItemByTLV(psDgiBuf, "\x82", 2, gl_AppRef.sDcAip); // T82
		if(iRet)
			return(-1);
        iRet = iDgiAddItemByTLV(psDgiBuf, "\x94", gl_AppRef.ucDcAflLen, gl_AppRef.sDcAfl); // T94
		if(iRet)
			return(-1);
        break;
	case 0x9102: // 接触交易选择FCI
        iRet = iDgiAddItem(psDgiBuf, gl_AppRef.sContactFci, gl_AppRef.ucContactFciLen); // TA5
		if(iRet)
			return(-1);
        break;
	case 0x9103: // 非接触交易选择FCI
        iRet = iDgiAddItem(psDgiBuf, gl_AppRef.sCtLessFci, gl_AppRef.ucCtLessFciLen); // TA5
		if(iRet)
			return(-1);
        break;
    case 0x9203: // eCash交易GPO响应数据
        iRet = iDgiAddItemByTLV(psDgiBuf, "\x82", 2, gl_AppRef.sECashAip); // T82
		if(iRet)
			return(-1);
        iRet = iDgiAddItemByTLV(psDgiBuf, "\x94", gl_AppRef.ucECashAflLen, gl_AppRef.sECashAfl); // T94
		if(iRet)
			return(-1);
        break;
    case 0x9206: // MSD交易GPO响应数据
        iRet = iDgiAddItemByTLV(psDgiBuf, "\x82", 2, gl_AppRef.sMsdAip); // T82
		if(iRet)
			return(-1);
        iRet = iDgiAddItemByTLV(psDgiBuf, "\x94", gl_AppRef.ucMsdAflLen, gl_AppRef.sMsdAfl); // T94
		if(iRet)
			return(-1);
        break;
    case 0x9207: // qPboc交易GPO响应数据
        iRet = iDgiAddItemByTLV(psDgiBuf, "\x82", 2, gl_AppRef.sQpbocAip); // T82
		if(iRet)
			return(-1);
		iLen = gl_AppRef.ucQpbocAflLen;
		memcpy(sBuf, gl_AppRef.sQpbocAfl, iLen);
	    if(gl_AppRef.ucIccPkLen <= 128) {
			// IC卡RSA密钥模长小于等于128时, 签字数据会随GPO返回, 就不需要AFL中指明虚拟记录了
			// qPboc AFL在倒数第2入口增加了虚拟记录SFI=6、REC=1的入口, 如果IC卡RSA密钥长度小于等于128, 则不需要该虚拟记录
			memcpy(sBuf+iLen-8, sBuf+iLen-4, 4);
			iLen -= 4;
		}
        iRet = iDgiAddItemByTLV(psDgiBuf, "\x94", iLen, sBuf); // T94
		if(iRet)
			return(-1);
        if(gl_AppRef.ucQpbocFciIssuerAppDataLen) {
            iRet = iDgiAddItemByTLV(psDgiBuf, "\x9F\x10", gl_AppRef.ucQpbocFciIssuerAppDataLen, gl_AppRef.sQpbocFciIssuerAppData); // T9F10
		    if(iRet)
			    return(-1);
        }
        break;
	default:
		return(0); // 不认识的数据分组
	}
	iLen = psDgiBuf[2] + 3;
//	memcpy(psDgiBuf, sDgiBuf, iLen);
    return(iLen);
}
