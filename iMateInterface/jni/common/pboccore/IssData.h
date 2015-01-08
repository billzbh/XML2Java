/**************************************
File name     : ISSDATA.H
Function      : ����PBOC2.0 D/C����������ؽṹ
Author        : Yu Jun
First edition : Jul 3rd, 2009
**************************************/
#ifndef _ISSDATA_H
#define _ISSDATA_H

// ��Կ����
typedef struct {
    int   iIssuerRsaKeyIndex;       // ������RSA��Կ����
    int   iKmcIndex;                // IC��������Կ����
    int   iAppKeyBaseIndex;         // IC��Ӧ����Կ������, ����ΪAuth��Mac��Enc��Dcvv
} stKeyRef;

// ��Ƭ����
typedef struct {
	uchar ucKmcKeyIndex;			// KMC������
	uchar ucKmcKeyDivFlag;			// KMC�ڿ�Ƭ���Ƿ���Ҫ��ɢ
	uchar ucKmcKeyVer;              // KMC�汾��
	uchar ucCardManagerAidLen;      // Card Manager Aid����
	uchar sCardManagerAid[16];		// Card Manager Aid
} stCardRef;

// PBOCʵ������
typedef struct {
	uchar ucMaxAppNum;                  // �������Ӧ�ø���
	uchar ucJavaPackageAidLen;			// Java Package Aid����
	uchar sJavaPackageAid[16];			// Java Package Aid
	uchar ucJavaAppletAidLen;			// Java Applet Aid����
	uchar sJavaAppletAid[16];			// Java Applet Aid
	uchar ucAidLen;						// Ӧ��ʵ��Aid����
	uchar sAid[16];						// Ӧ��ʵ��Aid
	uchar ucApplicationPrivilegesLen;	// Application Privileges����
	uchar sApplicationPrivileges[16];   // Application Privileges
} stInstanceRef;

// PBOC D/C Ӧ������
typedef struct {
    uchar ucTrack2EquDataLen;           // ���ŵ���Ч���ݳ���
    uchar sTrack2EquData[19];           // ���ŵ���Ч����, T57
    uchar szHolderName[46];             // �ֿ�������, T5F20|T9F0B (<=26:T5F20 >26:T9F0B)
    uchar szHolderId[40+1];             // �ֿ���֤������, T9F61
    uchar ucHolderIdType;               // �ֿ���֤������, T9F62
                                        // 00�����֤ 01������֤ 02������ 03���뾳֤ 04����ʱ���֤ 05������
	uchar ucIssuerPkCertificateLen;     // �����й�Կ֤�鳤��
	uchar sIssuerPkCertificate[248];    // �����й�Կ֤��, T90
	uchar ucIssuerPkExponentLen;        // �����й�Կ����ָ������
	uchar sIssuerPkExponent[3];         // �����й�Կ����ָ��, T9F32
	uchar ucIssuerPkRemainderLen;       // �����й�Կʣ�೤��
	uchar sIssuerPkRemainder[36];       // �����й�Կʣ��, T92
	uchar ucCaPublicKeyIndex;           // CA��Կ����, T8F

    // ��׼DC����
	uchar ucDcSignedStaticDataLen;      // ��׼DCǩ����Ӧ�����ݳ���
	uchar sDcSignedStaticData[248];     // ��׼DCǩ����Ӧ������, T93
    uchar ucDcIcCertificateLen;         // ��׼DC��IC����Կ֤�鳤��
    uchar sDcIcCertificate[248];        // ��׼DC��IC����Կ֤��, T9F46
	uchar ucDcCVMListLen;
	uchar sDcCVMList[252];				// Cardholder Verification Method (CVM) List, T8E
	uchar sDcIACDefault[5];				// IAC - default, T9F0D
	uchar sDcIACDenial[5];				// IAC - denial, T9F0E
	uchar sDcIACOnline[5];				// IAC - online, T9F0F
    // ECash����
	uchar ucECashSignedStaticDataLen;   // ECashǩ����Ӧ�����ݳ���
	uchar sECashSignedStaticData[248];  // ECashǩ����Ӧ������, T93
    uchar ucECashIcCertificateLen;      // ECash��IC����Կ֤�鳤��
    uchar sECashIcCertificate[248];     // ECash��IC����Կ֤��, T9F46
	uchar ucECashCVMListLen;
	uchar sECashCVMList[252];			// Cardholder Verification Method (CVM) List, T8E
	uchar sECashIACDefault[5];			// IAC - default, T9F0D
	uchar sECashIACDenial[5];			// IAC - denial, T9F0E
	uchar sECashIACOnline[5];			// IAC - online, T9F0F
    ////////////

    uchar ucIcPublicKeyELen;            // IC����Կָ������
    uchar sIcPublicKeyE[3];             // IC����Կָ��, T9F47
    uchar ucIcPublicKeyRemainderLen;    // IC����Կ�����
    uchar sIcPublicKeyRemainder[42];    // IC����Կ����, T9F48
    uchar ucDDOLLen;                    // DDOL����
    uchar sDDOL[64];                    // DDOL, T9F49

	uchar sPAN[10];                     // �����˺�, T5A
	uchar ucPANSerialNo;                // �����˺����к�(0xFF��ʾ�޴���), T5F34
	uchar sExpireDate[3];               // ��Ч��, T5F24
	uchar sEffectDate[3];               // ��������("\xFF\xFF\xFF"��ʾ�޴���), T5F25
 	uchar sIssuerCountryCode[2];        // �����й��Ҵ���, T5F28
    uchar sUsageControl[2];             // Ӧ����;����(AUC), T9F07
    
	uchar ucCDOL1Len;
	uchar sCDOL1[252];	    			// CDOL1, T8C
	uchar ucCDOL2Len;
	uchar sCDOL2[252];			    	// CDOL2, T8D
	uchar sServiceCode[2];              // ������, T5F30
	uchar sAppVerNo[2];					// Ӧ�ð汾��, T9F08
	uchar ucSDATagListLen;              // SDA��ǩ�б���
	uchar sSDATagList[64];				// SDA��ǩ�б�, T9F4A
	uchar sDataAuthCode[2];             // Data Authentication Code, T9F45
    uchar sPinCounterRef[2];            // ��������, [0]:������ [1]:ʣ�����

    uchar ucLowerConsecutiveLimit;      // ������������(LCOL), T9F58
 	uchar ucUpperConsecutiveLimit;      // ������������(UCOL), T9F59
    uchar ucConsecutiveTransLimit;      // �����ѻ�����������(����-����), T9F53
	uchar sCTTAL[6];                    // �ۼ��ѻ����׽������(CTTAL), T9F54
	uchar sCTTAUL[6];                   // �ۼ��ѻ����׽������(CTTAUL), T9F5C
    uchar ucLogFormatLen;               // ��־��ʽ����
    uchar sLogFormat[128];              // ��־��ʽ, T9F4F
    uchar ucLoadLogFormatLen;           // Ȧ����־��ʽ����
    uchar sLoadLogFormat[128];          // Ȧ����־��ʽ, TDF4F

    // ECashר������
	uchar sECashIssuerAuthCode[6];      // �����ֽ𷢿�����Ȩ��, T9F74
	uchar sECashBalanceLimit[6];        // eCash: Balance Limit, T9F77
	uchar sECashSingleTransLimit[6];    // eCash: Single Transaction Limit, T9F78
	uchar sECashBalance[6];             // eCash: Balance, T9F79
	uchar sECashResetThreshold[6];      // eCash: Reset Threshold, T9F6D

    // QPbocר������
	uchar sQPbocAdditionalProcess[4];   // qPBOC: Card Additional Processes, T9F68
	uchar sQPbocTransQualifiers[2];     // qPBOC: Card Transaction Qualifiers (CTQ), T9F6C
	uchar sQPbocSpendingAmount[6];      // qPBOC: Available Offline spending Amount, T9F5D
	uchar sQPbocCvmLimitAmount[6];      // qPBOC: Card CVM Limit, T9F6B

	uchar sAppCurrencyCode[2];          // Application Currency Code, T9F51
    uchar sAppDefaultAction[2];         // Application Default Action (ADA), T9F52
	uchar ucIssuerAuthIndicator;        // Issuer Authentication Indicator, T9F56

    // ������Կ�������
    uchar sDesKeyAuth[16+3];            // Kmc������Կ���ܺ��Auth��Կ��У��λ
    uchar sDesKeyMac[16+3];             // Kmc������Կ���ܺ��Mac��Կ��У��λ
    uchar sDesKeyEnc[16+3];             // Kmc������Կ���ܺ��Enc��Կ��У��λ
    uchar sRsaKeyD[256];
    uchar sRsaKeyP[128];
    uchar sRsaKeyQ[128];
    uchar sRsaKeyDp[128];
    uchar sRsaKeyDq[128];
    uchar sRsaKeyQinv[128];
    uchar sPinBlock[8];
    ///

    // GPO��Ӧ����
    uchar sDcAip[2];                    // ��׼DC AIP, T82
    uchar ucDcAflLen;                   // ��׼DC AFL����
    uchar sDcAfl[64];                   // ��׼DC AFL, T94
	uchar ucDcFciIssuerAppDataLen;      // ��׼DC������ר������(IAD)����
	uchar sDcFciIssuerAppData[32];      // ��׼DC������ר������(IAD), T9F10 (����Ϊ0��ʾ�����ڸ���)
	                                    // eCashͬ�ô���Ŀ
    uchar sECashAip[2];                 // eCash AIP, T82
    uchar ucECashAflLen;                // eCash AFL����
    uchar sECashAfl[64];                // eCash AFL, T94
    uchar sQpbocAip[2];                 // qPboc AIP, T82
    uchar ucQpbocAflLen;                // qPboc AFL����
    uchar sQpbocAfl[64];                // qPboc AFL, T94
	uchar ucQpbocFciIssuerAppDataLen;   // qPboc������ר������(IAD)����
	uchar sQpbocFciIssuerAppData[32];   // qPboc������ר������(IAD), T9F10 (����Ϊ0��ʾ�����ڸ���)
	                                    // eCashͬ�ô���Ŀ
    uchar sMsdAip[2];                   // MSD AIP, T82
    uchar ucMsdAflLen;                  // MSD AFL����
    uchar sMsdAfl[64];                  // MSD AFL, T94

    // �Ӵ�����FCI�������, TA5
    uchar szAppLabel[17];               // Ӧ�ñ�ǩ, T50, �ǽ�FCIҲ�ô���
    uchar ucPriority;                   // Ӧ������ָʾ��, T87, �ǽ�FCIҲ�ô���
    uchar ucContactPdolLen;             // ����ѡ�����ݶ����б���
    uchar sContactPdol[64];             // ����ѡ�����ݶ����б�, T9F38
    uchar szLanguage[9];                // ������ѡ��, T5F2D, �ǽ�FCIҲ�ô���
    uchar ucFciPrivateLen;              // FCI�������Զ������ݳ���, �ǽ�FCIҲ�ô���
    uchar sFciPrivate[64];              // FCI�������Զ�������, TBF0C, �ǽ�FCIҲ�ô���
    uchar ucContactFciLen;              // �Ӵ�����FCI����
    uchar sContactFci[128];             // �Ӵ�����FCI
    // �ǽӴ�����FCI�������, TA5
    uchar ucCtLessPdolLen;              // ����ѡ�����ݶ����б���
    uchar sCtLessPdol[64];              // ����ѡ�����ݶ����б�, T9F38
    uchar ucCtLessFciLen;               // �ǽӴ�����FCI����
    uchar sCtLessFci[128];              // �ǽӴ�����FCI

	uchar ucIccPkLen;                   // ICC��Կ����
	uchar szPin[13];                    // ��������, 4-12
	uchar ucECashExistFlag;             // eCash���ڱ�־
	uchar ucQpbocExistFlag;             // qPboc���ڱ�־
} stAppRef;

#ifdef __cplusplus
extern "C" {
#endif

extern stKeyRef         gl_KeyRef;      // ��Կ����
extern stCardRef		gl_CardRef;		// ��Ƭ���Բ���
extern stInstanceRef	gl_InstanceRef;	// PBOC D/C ʵ�����Բ���
extern stAppRef		    gl_AppRef;      // PBOC D/C Ӧ�ò���

// ��ʼ����������
// in  : iAppVer : Ӧ�ð汾��, 0x20:2.0 0x30:3.0
void vIssDataInit(int iAppVer);

// ��ɷ�������׼��
// in  : psTermRand : �ն������
//       psCardData : IC����ʼ������
// out : pszErrMsg  : ������Ϣ
// ret : 0          : OK
//       1          : refer pszErrMsg
int iCompleteIssData(uchar *psTermRand, uchar *psCardData, uchar *pszErrMsg);

#ifdef __cplusplus
}
#endif

#endif
