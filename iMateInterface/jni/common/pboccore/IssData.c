/**************************************
File name     : ISSDATA.C
Function      : ����PBOC2.0 D/C����������ؽṹ
Author        : Yu Jun
First edition : Jul 6th, 2009
Note          : <�����ؼ�����.doc>�жԹؼ����ݵ���ϸ����
**************************************/
# include <stdio.h>
# include <string.h>
# include <ctype.h>
# include "VposFace.h"
# include "TlvFunc.h"
# include "KeyFace.h"
# include "KeyProc.h"
# include "MakeDgi.h"
# include "IssData.h"

stKeyRef		gl_KeyRef;			// ��Կ���Բ���
stCardRef		gl_CardRef;			// ��Ƭ���Բ���
stInstanceRef	gl_InstanceRef;		// PBOCʵ�����Բ���
stAppRef		gl_AppRef;			// PBOCӦ�ò���

// ��ʼ����������
// in  : iAppVer : Ӧ�ð汾��, 0x20:2.0 0x30:3.0
void vIssDataInit(int iAppVer)
{
	uchar szBuf[512];

    memset(&gl_KeyRef, 0, sizeof(gl_KeyRef));
    memset(&gl_CardRef, 0, sizeof(gl_CardRef));
    memset(&gl_InstanceRef, 0, sizeof(gl_InstanceRef));
    memset(&gl_AppRef, 0, sizeof(gl_AppRef));

// ��Ƭ��������
	gl_CardRef.ucKmcKeyIndex = 0;				// KMC�ڿ�Ƭ��������
    gl_CardRef.ucKmcKeyDivFlag = 1;				// KMC�ڿ�Ƭ���Ƿ���Ҫ��ɢ, 0:����ɢ 1:��ɢ
	gl_CardRef.ucKmcKeyVer = 0;                 // KMC�汾��
    strcpy(szBuf, "A000000004000000");			// Card Manager Aid
	gl_CardRef.ucCardManagerAidLen = strlen(szBuf)/2;
	vTwoOne(szBuf, (ushort)strlen(szBuf), gl_CardRef.sCardManagerAid);

// PBOCʵ����������
    gl_InstanceRef.ucMaxAppNum = 6;             // ��������Ӧ�ø���(pse��ppse���������ӵļ�¼����)
    strcpy(szBuf, "A000000333010130");                              // Java Package Aid
	gl_InstanceRef.ucJavaPackageAidLen = strlen(szBuf)/2;
	vTwoOne(szBuf, strlen(szBuf), gl_InstanceRef.sJavaPackageAid);
    strcpy(szBuf, "A0000003330101");	                            // Java Applet Aid
    gl_InstanceRef.ucJavaAppletAidLen = strlen(szBuf)/2;
	vTwoOne(szBuf, strlen(szBuf), gl_InstanceRef.sJavaAppletAid);
	gl_InstanceRef.ucApplicationPrivilegesLen = 1;					// Application Privileges����
	memcpy(gl_InstanceRef.sApplicationPrivileges, "\x10", gl_InstanceRef.ucApplicationPrivilegesLen);// Application Privileges

// PBOCӦ������
    gl_AppRef.ucHolderIdType = 0;       // �ֿ���֤������, T9F62
                                        // 00�����֤ 01������֤ 02������ 03���뾳֤ 04����ʱ���֤ 05������
    strcpy(gl_AppRef.szHolderId, "650001199707010010");
    strcpy(szBuf, "7391CBCF974DC9F720B683B333C0B19C4649259C8AF5CA2B1A09A90C6D2549DB8EB9704E6C58CE5684E40AF27691EDD6F07E5BCA6E30AB6125339CA18ED442DF1F35DA9AF20B774F62A5693F3948B32942D8B2989DC2CA72E69FE02606791BDB85921AE56AE3BB0FA1F52BB23C37344399BA78A18FBC4EF574623A1DD8C56853B32C7CFA6F6492807EA6C95480471393D3E68FD1D2FC12D09B094A6F0696ABA150D109FE740375E47BC264D9F64A9E813173D1F624B9650161B40BB258BF0E5A");
	gl_AppRef.ucIssuerPkCertificateLen = strlen(szBuf)/2;
	vTwoOne(szBuf, strlen(szBuf), gl_AppRef.sIssuerPkCertificate); // �����й�Կ֤��, T90
    gl_AppRef.ucIssuerPkExponentLen = 1;// �����й�Կ����ָ������
    memcpy(gl_AppRef.sIssuerPkExponent, "\x03", gl_AppRef.ucIssuerPkExponentLen); // �����й�Կ����ָ��, T9F32
	strcpy(szBuf, "4981416F");
	gl_AppRef.ucIssuerPkRemainderLen = strlen(szBuf)/2;				// �����й�Կʣ�೤��, 0��ʾ�����ڷ����й�Կʣ��
	vTwoOne(szBuf, (ushort)strlen(szBuf), gl_AppRef.sIssuerPkRemainder);    // �����й�Կʣ��, T92

    // ��׼DC����
    strcpy(szBuf, "00000000""00000000" "0203""1F00"); // "��������PIN��֤,ʧ����ֹ" "����CVM"
    strcpy(szBuf, "00000000""00000000" "6003""4203""1F00"); // "��ʾ֤��,ʧ�ܼ���" "��������PIN��֤,ʧ�ܼ���" "����CVM"
    strcpy(szBuf, "00000000""00000000" "0203"); // "��������PIN��֤"
	gl_AppRef.ucDcCVMListLen = strlen(szBuf)/2;								// CVM List len
	vTwoOne(szBuf, strlen(szBuf), gl_AppRef.sDcCVMList);    		// Cardholder Verification Method (CVM) List, T8E
	memcpy(gl_AppRef.sDcIACDefault, "\xD8\x60\x04\xA8\x00", 5);		// IAC - default, T9F0D
	memcpy(gl_AppRef.sDcIACDenial,  "\x00\x10\x98\x00\x00", 5);		// IAC - denial, T9F0E
	memcpy(gl_AppRef.sDcIACOnline,  "\xD8\x68\x04\xF8\x00", 5);		// IAC - online, T9F0F

    // eCash����
    strcpy(szBuf, "00000000""00000000" "0303""1F03"); // "����PIN��֤,ʧ����ֹ" "����CVM"
    strcpy(szBuf, "00000000""00000000" "6003""4303""1F00"); // "��ʾ֤��,ʧ�ܼ���" "����PIN��֤,ʧ�ܼ���" "����CVM"
    strcpy(szBuf, "000007D0""00000000" "5F06""0103"); // "<x(x=0x7D0=2000)����������,ʧ�ܼ���" "����PIN��֤"
	gl_AppRef.ucECashCVMListLen = strlen(szBuf)/2;								// CVM List len
	vTwoOne(szBuf, strlen(szBuf), gl_AppRef.sECashCVMList);    		// Cardholder Verification Method (CVM) List, T8E
	memcpy(gl_AppRef.sECashIACDefault, "\xD8\x60\x3C\xA8\x00", 5);	// IAC - default, T9F0D
	memcpy(gl_AppRef.sECashIACDenial,  "\x00\x10\x80\x00\x00", 5);	// IAC - denial, T9F0E
	memcpy(gl_AppRef.sECashIACOnline,  "\xD8\x68\x3C\xF8\x00", 5);	// IAC - online, T9F0F

    gl_AppRef.ucIcPublicKeyELen = 1;            // IC����Կָ������
    memcpy(gl_AppRef.sIcPublicKeyE, "\x03", gl_AppRef.ucIcPublicKeyELen); // IC����Կָ��, T9F47
	strcpy(szBuf, "9F3704" // : Unpredictable Number
		          );
	gl_AppRef.ucDDOLLen = strlen(szBuf)/2;
	vTwoOne(szBuf, strlen(szBuf), gl_AppRef.sDDOL);				    // DDOL, T9F49
    gl_AppRef.ucPANSerialNo = 0xFF;     // �����˺����к�(0xFF��ʾ�޴���), T5F34
    memcpy(gl_AppRef.sEffectDate, "\xFF\xFF\xFF", 3);               // ��������("\xFF\xFF\xFF"��ʾ�޴���), T5F25
    memcpy(gl_AppRef.sIssuerCountryCode, "\x01\x56", 2);            // �����й��Ҵ���, T5F28
    memcpy(gl_AppRef.sUsageControl, "\xFF\x00", 2);                 // Ӧ����;����(AUC), T9F07

    strcpy(szBuf, "9F0206" // : Amount, Authorised (Numeric)
                  "9F0306" // : Amount, Other (Numeric)
                  "9F1A02" // : Terminal Country Code
                  "9505"   // : Terminal Verification Results
                  "5F2A02" // : Transaction Currency Code
                  "9A03"   // : Transaction Date
				  "9F2103" // : Transaction Time
                  "9C01"   // : Transaction Type
                  "9F3704" // : Unpredictable Number
                  "9F4E14" // : Merchant Name and Location
				  );
	gl_AppRef.ucCDOL1Len = strlen(szBuf)/2;
	vTwoOne(szBuf, strlen(szBuf), gl_AppRef.sCDOL1);				// CDOL1, T8C
    strcpy(szBuf, "8A02"   // : Authorisation Response Code
		          "9F0206"
		          "9F0306"
		          "9F1A02"
                  "9505"   // : Terminal Verification Results
		          "5F2A02"
		          "9A03"
		          "9F2103"
		          "9C01"
                  "9F3704" // : Unpredictable Number
				  );
	gl_AppRef.ucCDOL2Len = strlen(szBuf)/2;
	vTwoOne(szBuf, strlen(szBuf), gl_AppRef.sCDOL2);				// CDOL2, T8D
    memcpy(gl_AppRef.sServiceCode, "\x09\x01", 2);                  // ������, T5F30
	vLongToStr((ulong)iAppVer, 2, gl_AppRef.sAppVerNo);				// Ӧ�ð汾��, T9F08
    strcpy(szBuf, "82");
	gl_AppRef.ucSDATagListLen = strlen(szBuf)/2;
	vTwoOne(szBuf, strlen(szBuf), gl_AppRef.sSDATagList);		    // SDA��ǩ�б�, T9F4A
	memcpy(gl_AppRef.sDataAuthCode, "\xDA\xC0", 2);                 // Data Authentication Code, T9F45
	memcpy(gl_AppRef.sPinCounterRef, "\x03\x03", 2);                // ��������, [0]:������ [1]:ʣ�����
    
    gl_AppRef.ucLowerConsecutiveLimit = 0;                          // Lower Consecutive Offline Limit (Card Check), T9F58
 	gl_AppRef.ucUpperConsecutiveLimit = 0;                          // Upper Consecutive Offline Limit (Card Check), T9F59
	gl_AppRef.ucConsecutiveTransLimit = 0;                          // Consecutive Transaction Limit (International), T9F53
	memcpy(gl_AppRef.sCTTAL, "\x00\x00\x00\x00\x00\x00", 6);        // Cumulative Total Transaction Amount Limit (CTTAL), T9F54
	memcpy(gl_AppRef.sCTTAUL, "\x00\x00\x00\x00\x00\x00", 6);       // Cumulative Total Transaction Amount Upper Limit (CTTAUL), T9F5C
	strcpy(szBuf,
		    "9A03"	// ��������
		    "9F2103"	// ����ʱ��
		    "9F0206"	// amount, authorized
		    "9F0306" // amount, other
		    "9F1A02" // terminal coutry code
		    "5F2A02" // transaction currency code
		    "9F4E14" // �̻�����
		    "9C01"   // transaction type
		    "9F3602" // ATC
		    );
	gl_AppRef.ucLogFormatLen = strlen(szBuf)/2;		                // Log Record format����
	vTwoOne(szBuf, strlen(szBuf), gl_AppRef.sLogFormat);            // Log Record format, T9F4F
	// pboc3.0����Ȧ����־
	strcpy(szBuf,
		    "9A03"	// ��������
		    "9F2103"	// ����ʱ��
		    "9F1A02" // terminal coutry code
		    "9F4E14" // �̻�����
		    "9F3602" // ATC
		    );
	gl_AppRef.ucLoadLogFormatLen = strlen(szBuf)/2;		            // Load Log Record format����
	vTwoOne(szBuf, strlen(szBuf), gl_AppRef.sLoadLogFormat);        // Load Log Record format, TDF4F

    // eCash
	memcpy(gl_AppRef.sECashIssuerAuthCode, "ECC001", 6);		    // eCash: Issuer Authorization Code, T9F74
	memcpy(gl_AppRef.sECashBalanceLimit, "\x00\x00\x00\x20\x00\x00", 6); // eCash: Balance Limit, T9F77
	memcpy(gl_AppRef.sECashSingleTransLimit, "\x00\x00\x00\x07\x00\x00", 6); // eCash: Single Transaction Limit, T9F78
	memcpy(gl_AppRef.sECashBalance, "\x00\x00\x00\x10\x00\x00", 6); // eCash: Balance, T9F79
	memcpy(gl_AppRef.sECashResetThreshold, "\x00\x00\x00\x05\x00\x00", 6); // eCash: Reset Threshold, T9F6D
    
    // QPboc
	memcpy(gl_AppRef.sQPbocAdditionalProcess, "\x99\x40\xF0\x00", 4); // qPBOC: Card Additional Processes, T9F68
	memcpy(gl_AppRef.sQPbocTransQualifiers, "\x20\x00", 2); // qPBOC: Card Transaction Qualifiers (CTQ), T9F6C
	memcpy(gl_AppRef.sQPbocSpendingAmount, "\x00\x00\x00\x00\x00\x01", 6); // qPBOC: Available Offline spending Amount, T9F5D
	memcpy(gl_AppRef.sQPbocCvmLimitAmount, "\x99\x99\x99\x99\x99\x99", 6); // qPBOC: Available Offline spending Amount, T9F6B
	memcpy(gl_AppRef.sAppCurrencyCode, "\x01\x56", 2); // Application Currency Code, T9F51, , T9F42
	memcpy(gl_AppRef.sAppDefaultAction, "\x60\x10", 2); // Application Default Action (ADA), T9F52
    gl_AppRef.ucIssuerAuthIndicator = 0x80;        // Issuer Authentication Indicator, T9F56
    
    
    // GPO��Ӧ����
    memcpy(gl_AppRef.sDcAip, "\x7D\x00", 2); // ��׼DC AIP, T82     // first byte:
	                                                                // X RFU
	                                                                // X SDA supported (YES)
																	// X DDA supported (YES)
																	// X Cardholder verification is supported (YES)
																	// X Terminal risk management is to be performed (YES)
																	// X Issuer authentication is supported (YES)
																	// X RFU
																	// X CDA supported (YES)
																	// second byte, all RFU
    strcpy(szBuf, "080102001001050018010201");
    gl_AppRef.ucDcAflLen = strlen(szBuf)/2;
    vTwoOne(szBuf, strlen(szBuf), gl_AppRef.sDcAfl);    // ��׼DC AFL, T94
    strcpy(szBuf, "07" "01" "010300000001");
    gl_AppRef.ucDcFciIssuerAppDataLen = strlen(szBuf)/2;
    vTwoOne(szBuf, strlen(szBuf), gl_AppRef.sDcFciIssuerAppData);  // ��׼DC������ר������(IAD), T9F10 (����Ϊ0��ʾ�����ڸ���)

    memcpy(gl_AppRef.sECashAip, "\x7D\x00", 2); // eCash AIP, T82
    strcpy(szBuf, "0801020010010200100507001802020018030301");
    gl_AppRef.ucECashAflLen = strlen(szBuf)/2;
    vTwoOne(szBuf, strlen(szBuf), gl_AppRef.sECashAfl); // eCash AFL, T94

    memcpy(gl_AppRef.sQpbocAip, "\x7D\x00", 2); // qPboc AIP, T82, ע��, Ҫ��Ӵ�ʽ��AIP����һ��

    strcpy(szBuf, "080202001001050018010201" "30010100" "20010100"); // "30010100"ΪIC��RSA��Կ���ȴ���128�ֽ�ʱ������ļ�¼���, ���ڻ�ȡ��̬ǩ������(T9F4B)
                                                                     // ���IC��RSA��Կ����С�ڵ���128�ֽ�, ����׼��ʱ��gl_AppRef.sQpbocAflβ����ʼ-4ƫ����4�ֽڸ���β����ʼ-8ƫ����4�ֽ�, ����gl_AppRef.ucQpbocAflLen��ȥ4
    gl_AppRef.ucQpbocAflLen = strlen(szBuf)/2;
    vTwoOne(szBuf, strlen(szBuf), gl_AppRef.sQpbocAfl); // qPboc AFL, T94
    strcpy(szBuf, "07" "01" "01"/*���İ汾01*/ "0300000001" "0A01000000000000000000");
    gl_AppRef.ucQpbocFciIssuerAppDataLen = strlen(szBuf)/2;
    vTwoOne(szBuf, strlen(szBuf), gl_AppRef.sQpbocFciIssuerAppData); // qPboc������ר������(IAD), T9F10 (����Ϊ0��ʾ�����ڸ���)
    
    memcpy(gl_AppRef.sMsdAip, "\x00\x80", 2); // MSD AIP, T82
    strcpy(szBuf, "08010100");
    gl_AppRef.ucMsdAflLen = strlen(szBuf)/2;
    vTwoOne(szBuf, strlen(szBuf), gl_AppRef.sMsdAfl); // MSD AFL, T94
    
    // �Ӵ�����FCI�������, TA5
    strcpy(gl_AppRef.szAppLabel, "PBOC CREDIT"); // Ӧ�ñ�ǩ, T50
    gl_AppRef.ucPriority = 0x01; // Ӧ������ָʾ��, T87
    strcpy(szBuf, "9F7A019F02065F2A02");
    gl_AppRef.ucContactPdolLen = strlen(szBuf)/2;
    vTwoOne(szBuf, strlen(szBuf), gl_AppRef.sContactPdol); // ����ѡ�����ݶ����б�, T9F38
    strcpy(gl_AppRef.szLanguage, "ZH"); // ������ѡ��, T5F2D
    strcpy(szBuf, "9F4D020B0A");
	if(iAppVer >= 0x30)
	    strcat(szBuf, "DF4D020C0A"); // pboc3.0����Ȧ����־֧��
    gl_AppRef.ucFciPrivateLen = strlen(szBuf)/2;
    vTwoOne(szBuf, strlen(szBuf), gl_AppRef.sFciPrivate); // FCI�������Զ�������, TBF0C

    // �ǽӴ�����FCI�������, TA5
    strcpy(szBuf, "9F66049F02069F03069F1A0295055F2A029A039C019F3704");
    strcpy(szBuf, "9F66049F7A019F02069F03069F1A0295055F2A029A039C019F3704");
    gl_AppRef.ucCtLessPdolLen = strlen(szBuf)/2;
    vTwoOne(szBuf, strlen(szBuf), gl_AppRef.sCtLessPdol); // ����ѡ�����ݶ����б�, T9F38
}

// ���ò����ÿ����ò���
void vSetExampleCardData()
{
	uchar szBuf[512];

    gl_KeyRef.iKmcIndex = 1;            // ���ܻ���KMC����
    gl_KeyRef.iAppKeyBaseIndex = 3;     // ���ܻ���Ӧ����Կ������
    gl_KeyRef.iIssuerRsaKeyIndex = 2;   // ���ܻ��з�����˽Կ����

    strcpy(szBuf, "A000000333010102");
    gl_InstanceRef.ucAidLen = strlen(szBuf)/2;
    vTwoOne(szBuf, strlen(szBuf), gl_InstanceRef.sAid); // Ӧ��AID

    gl_AppRef.ucECashExistFlag = 1;     // eCash���ڱ�־
    gl_AppRef.ucQpbocExistFlag = 1;     // qPboc���ڱ�־
    gl_AppRef.ucCaPublicKeyIndex = 0x01;// CA��Կ����, T8F
    gl_AppRef.ucIccPkLen = 128;         // ICC��Կ����

    memcpy(gl_AppRef.sPAN, "\x88\x88\x80\x00\x00\x00\x00\x01\xFF\xFF", 10);     // �����˺�, T5A
    memcpy(gl_AppRef.sExpireDate, "491231", 3);
    strcpy(gl_AppRef.szPin, "000000");  // ��������, 4-12
    strcpy(gl_AppRef.szHolderName, "Sample Card");
	gl_AppRef.ucHolderIdType = 0; // ���֤
    strcpy(gl_AppRef.szHolderId, "650001199707010010");
}

// ��ȡ��ǩ�־�̬����
// out : iFlag     : ��־, 0:��׼DC 1:eCash
//       piOutLen  : ������ݳ���
//       psOutData : �������
// ret : 0         : OK
//       1         : error
static int iGetDataNeedSign(int iFlag, int *piOutLen, uchar *psOutData)
{
    uchar sBuf[512];
    int   iAflLen;
    uchar *psAfl, *psAip;
    uchar *psRecValue;
    uchar ucSfi;
    int   iRet;
    int   i, j;

    if(iFlag == 0) {
        // ��׼DC
        iAflLen = gl_AppRef.ucDcAflLen;
        psAfl = gl_AppRef.sDcAfl;
        psAip = gl_AppRef.sDcAip;
    } else if(iFlag == 1) {
        // eCash
        iAflLen = gl_AppRef.ucECashAflLen;
        psAfl = gl_AppRef.sECashAfl;
        psAip = gl_AppRef.sECashAip;
    } else
        return(1);

    *piOutLen = 0;
    for(i=0; i<iAflLen; i+=4, psAfl+=4) {
        if(psAfl[3] == 0)
            continue; // ����Ҫǩ�ּ�¼
        ucSfi = psAfl[0] >> 3;
        for(j=psAfl[1]; j<psAfl[1]+psAfl[3]; j++) {
            // jΪ��Ҫǩ�ֵļ�¼��
            iRet = iMakePbocDgi(ucSfi*0x100+j/*DGI*/, sBuf);
            if(iRet <= 0)
                return(1);
            if((psAfl[0]>>3)>=1 && (psAfl[0]>>3)<=10) {
                // SFI��1-10֮��
                iRet = iTlvValue(sBuf+3, &psRecValue);
                if(iRet <= 0)
                    return(1);
                memcpy(psOutData, psRecValue, iRet);
                *piOutLen += iRet;
                psOutData += iRet;
            } else {
                memcpy(psOutData, sBuf+3, sBuf[2]);
                *piOutLen += sBuf[2];
                psOutData += sBuf[2];
            }
        }
    }
    if(gl_AppRef.ucSDATagListLen==1 && gl_AppRef.sSDATagList[0]==0x82) {
        memcpy(psOutData, psAip, 2);
        *piOutLen += 2;
        psOutData += 2;
    }

    return(0);
}

// ��ɷ�������׼��
// in  : psTermRand : �ն������
//       psCardData : IC����ʼ������
// out : pszErrMsg  : ������Ϣ
// ret : 0          : OK
//       1          : refer pszErrMsg
int iCompleteIssData(uchar *psTermRand, uchar *psCardData, uchar *pszErrMsg)
{
    uchar sIcRsaKeyModulus[248];
    uchar szExpireDate[7];
    uchar szPan[21], ucDivLevel, sDivData[24];
    uchar szBuf[512], sBuf[1536];
    int   iLen;
    uchar *p;
    int   iRet;
    int   i;

    // �������
    vOneTwo0(gl_AppRef.sPAN, 10, szPan);
    for(i=0; i<20; i++)
        if(szPan[i] == 'F')
            szPan[i] = 0;

    // ��ɷ�ɢ����
	ucDivLevel = 1;
	memset(szBuf, '0', 16);
	szBuf[16] = 0;
	strcat((char *)szBuf, (char *)szPan);
	vTwoOne(szBuf+strlen((char *)szBuf)-14, 14, sDivData);
    if(gl_AppRef.ucPANSerialNo == 0xFF)
    	sDivData[7] = 0;
    else
    	sDivData[7] = gl_AppRef.ucPANSerialNo;

    // ������ŵ�����, T57
    strcpy(szBuf, szPan);
	strcat(szBuf, "D"); // seperator
    vOneTwo0(gl_AppRef.sExpireDate, 2, szBuf+strlen(szBuf));
    vOneTwo0(gl_AppRef.sServiceCode, 2, sBuf);
    strcat(szBuf, sBuf+1);
    strcat(szBuf, "000000000000000000000000000000");
    szBuf[37] = 'F';
    vTwoOne(szBuf, 38, gl_AppRef.sTrack2EquData);
    gl_AppRef.ucTrack2EquDataLen = 19;

    // ����IC��RSA��Կ��
    // out : pOut       : RSA��Կ��Կ����[n]+D����[n1]+P����[n2]+Q����[n2]+dP����[n2]+dQ����[n2]+qInv����[n2]
    //                    n = iRsaKeyLen, n1 = (iRsaKeyLen+8)/8*8, n2 = (iRsaKeyLen/2+8)/8*8
    iRet = iKeyCalRsaKey(gl_KeyRef.iKmcIndex, 0, NULL, 0, NULL, psTermRand, psCardData,
                         gl_AppRef.ucIccPkLen, gl_AppRef.ucIcPublicKeyELen==1?3:65537, sBuf);
    if(iRet) {
        sprintf(pszErrMsg, "���ܻ�����IC��RSA��Կ�Գ���[%04X]", iRet);
        return(1);
    }
    p = sBuf;
    memcpy(sIcRsaKeyModulus, p, gl_AppRef.ucIccPkLen);
    p += gl_AppRef.ucIccPkLen;
    memcpy(gl_AppRef.sRsaKeyD, p, (gl_AppRef.ucIccPkLen+8)/8*8);
    p += (gl_AppRef.ucIccPkLen+8)/8*8;
    memcpy(gl_AppRef.sRsaKeyP, p, (gl_AppRef.ucIccPkLen/2+8)/8*8);
    p += (gl_AppRef.ucIccPkLen/2+8)/8*8;
    memcpy(gl_AppRef.sRsaKeyQ, p, (gl_AppRef.ucIccPkLen/2+8)/8*8);
    p += (gl_AppRef.ucIccPkLen/2+8)/8*8;
    memcpy(gl_AppRef.sRsaKeyDp, p, (gl_AppRef.ucIccPkLen/2+8)/8*8);
    p += (gl_AppRef.ucIccPkLen/2+8)/8*8;
    memcpy(gl_AppRef.sRsaKeyDq, p, (gl_AppRef.ucIccPkLen/2+8)/8*8);
    p += (gl_AppRef.ucIccPkLen/2+8)/8*8;
    memcpy(gl_AppRef.sRsaKeyQinv, p, (gl_AppRef.ucIccPkLen/2+8)/8*8);
    p += (gl_AppRef.ucIccPkLen/2+8)/8*8;

    // ����IC��DES��Կ
    iRet = iKeyCalDesKey(gl_KeyRef.iKmcIndex, 0, NULL, 0, NULL, psTermRand, psCardData,
                         gl_KeyRef.iAppKeyBaseIndex+0, 0, NULL, ucDivLevel, sDivData, gl_AppRef.sDesKeyAuth);
    if(iRet) {
        sprintf(pszErrMsg, "���ܻ�����IC��Auth��Կ����[%04X]", iRet);
        return(1);
    }
    iRet = iKeyCalDesKey(gl_KeyRef.iKmcIndex, 0, NULL, 0, NULL, psTermRand, psCardData,
                         gl_KeyRef.iAppKeyBaseIndex+1, 0, NULL, ucDivLevel, sDivData, gl_AppRef.sDesKeyMac);
    if(iRet) {
        sprintf(pszErrMsg, "���ܻ�����IC��Mac��Կ����[%04X]", iRet);
        return(1);
    }
    iRet = iKeyCalDesKey(gl_KeyRef.iKmcIndex, 0, NULL, 0, NULL, psTermRand, psCardData,
                         gl_KeyRef.iAppKeyBaseIndex+2, 0, NULL, ucDivLevel, sDivData, gl_AppRef.sDesKeyEnc);
    if(iRet) {
        sprintf(pszErrMsg, "���ܻ�����IC��Enc��Կ����[%04X]", iRet);
        return(1);
    }

    // ����PinBlock
    memset(szBuf, 'F', 14);
    strcpy(szBuf, gl_AppRef.szPin);
    memset(szBuf+strlen(gl_AppRef.szPin), 'F', 14);
    sBuf[0] = 0x20;
    sBuf[0] |= strlen(gl_AppRef.szPin);
    vTwoOne(szBuf, 14, sBuf+1);
    iRet = iKeyCalPin(gl_KeyRef.iKmcIndex, 0, NULL, 0, NULL, psTermRand, psCardData,
                      sBuf, gl_AppRef.sPinBlock);
    if(iRet) {
        sprintf(pszErrMsg, "���ܻ�����IC��PinBlock����[%04X]", iRet);
        return(1);
    }

    // ��׼DC��̬����ǩ��, T93
    iRet = iGetDataNeedSign(0/*0:��׼DC*/, &iLen, sBuf);
    if(iRet) {
        sprintf(pszErrMsg, "��ȡ��׼DC��̬ǩ�����ݴ�");
        return(1);
    }
    iRet = iSignStaticData((ushort)iLen, sBuf, gl_AppRef.sDataAuthCode, gl_KeyRef.iIssuerRsaKeyIndex,
                           &gl_AppRef.ucDcSignedStaticDataLen, gl_AppRef.sDcSignedStaticData, pszErrMsg);
    if(iRet)
        return(1);
    // ��׼DC��IC����Կ֤��, T9F46
    vOneTwo0(gl_AppRef.sExpireDate, 3, szExpireDate);
    iRet = iGenerateIccPkCertificate(szPan, szExpireDate, gl_AppRef.ucIccPkLen, sIcRsaKeyModulus, gl_AppRef.ucIcPublicKeyELen==1?3:65537,
							         (ushort)iLen, sBuf, gl_KeyRef.iIssuerRsaKeyIndex,
                                     &gl_AppRef.ucDcIcCertificateLen, gl_AppRef.sDcIcCertificate,
                                     &gl_AppRef.ucIcPublicKeyRemainderLen, gl_AppRef.sIcPublicKeyRemainder, pszErrMsg);
    if(iRet)
        return(1);

    // eCash��̬����ǩ��, T93
    iRet = iGetDataNeedSign(1/*1:eCash*/, &iLen, sBuf);
    if(iRet) {
        sprintf(pszErrMsg, "��ȡeCash��̬ǩ�����ݴ�");
        return(1);
    }
    iRet = iSignStaticData((ushort)iLen, sBuf, gl_AppRef.sDataAuthCode, gl_KeyRef.iIssuerRsaKeyIndex,
                           &gl_AppRef.ucECashSignedStaticDataLen, gl_AppRef.sECashSignedStaticData, pszErrMsg);
    if(iRet)
        return(1);
    // eCash��IC����Կ֤��, T9F46
    vOneTwo0(gl_AppRef.sExpireDate, 3, szExpireDate);
    iRet = iGenerateIccPkCertificate(szPan, szExpireDate, gl_AppRef.ucIccPkLen, sIcRsaKeyModulus, gl_AppRef.ucIcPublicKeyELen==1?3:65537,
							         (ushort)iLen, sBuf, gl_KeyRef.iIssuerRsaKeyIndex,
                                     &gl_AppRef.ucECashIcCertificateLen, gl_AppRef.sECashIcCertificate,
                                     &gl_AppRef.ucIcPublicKeyRemainderLen, gl_AppRef.sIcPublicKeyRemainder, pszErrMsg);
    if(iRet)
        return(1);

    // FCI�������, TA5
    for(i=0; i<2; i++) {
        // i==0:�Ӵ����� i==1:�ǽӴ�����
        p = sBuf;
        iLen = 0;
        iRet = iTlvMakeObject("\x50", (ushort)strlen(gl_AppRef.szAppLabel), gl_AppRef.szAppLabel, p, sizeof(sBuf)); // T50
        if(iRet <= 0) {
            sprintf(pszErrMsg, "����TLV50����[%d]", iRet);
            return(1);
        }
        iLen += iRet;
        p += iRet;
        iRet = iTlvMakeObject("\x87", 1, &gl_AppRef.ucPriority, p, sizeof(sBuf)+p-sBuf); // T87
        if(iRet <= 0) {
            sprintf(pszErrMsg, "����TLV87����[%d]", iRet);
            return(1);
        }
        iLen += iRet;
        p += iRet;
        if(i == 0)
            iRet = iTlvMakeObject("\x9F\x38", gl_AppRef.ucContactPdolLen, gl_AppRef.sContactPdol, p, sizeof(sBuf)+p-sBuf); // T9F38
        else
            iRet = iTlvMakeObject("\x9F\x38", gl_AppRef.ucCtLessPdolLen, gl_AppRef.sCtLessPdol, p, sizeof(sBuf)+p-sBuf); // T9F38
        if(iRet <= 0) {
            sprintf(pszErrMsg, "����TLV9F38����[%d]", iRet);
            return(1);
        }
        iLen += iRet;
        p += iRet;
        iRet = iTlvMakeObject("\x5F\x2D", (ushort)strlen(gl_AppRef.szLanguage), gl_AppRef.szLanguage, p, sizeof(sBuf)+p-sBuf); // T5F2D
        if(iRet <= 0) {
            sprintf(pszErrMsg, "����TLV5F2D����[%d]", iRet);
            return(1);
        }
        iLen += iRet;
        p += iRet;
        iRet = iTlvMakeObject("\xBF\x0C", gl_AppRef.ucFciPrivateLen, gl_AppRef.sFciPrivate, p, sizeof(sBuf)+p-sBuf); // TBF0C
        if(iRet <= 0) {
            sprintf(pszErrMsg, "����TLVBF0C����[%d]", iRet);
            return(1);
        }
        iLen += iRet;
        p += iRet;

        // make TA5
        iRet = iTlvMakeObject("\xA5", (ushort)iLen, sBuf, szBuf, sizeof(sBuf)); // TA5
        if(iRet <= 0) {
            sprintf(pszErrMsg, "����TLVA5����[%d]", iRet);
            return(1);
        }
        if(i == 0) {
            gl_AppRef.ucContactFciLen = iRet;
            memcpy(gl_AppRef.sContactFci, szBuf, iRet);
        } else {
            gl_AppRef.ucCtLessFciLen = iRet;
            memcpy(gl_AppRef.sCtLessFci, szBuf, iRet);
        }
    }

    return(0);
}
