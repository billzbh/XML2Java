#ifndef _ISSFACE_H
#define _ISSFACE_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	int  iAppVer;				// Ӧ�ð汾��, 0x20��0x30, �ֱ��ʾPboc2.0����Pboc3.0��

	int  iKmcIndex;
	int  iAppKeyBaseIndex;
	int  iIssuerRsaKeyIndex;

	char szPan[19+1];			// 12-19
	int  iPanSerialNo;			// 0-99, -1��ʾ��
	char szExpireDate[6+1];		// YYMMDD
	char szHolderName[45+1];	// Len>=2
	int  iHolderIdType;			// 0:���֤ 1:����֤ 2:���� 3:�뾳֤ 4:��ʱ���֤ 5:����
	char szHolderId[40+1];
	char szDefaultPin[12+1];	// 4-12

	char szAid[32+1];			// OneTwo֮���
	char szLabel[16+1];			// 1-16
	int  iCaIndex;				// 0-255
	int  iIcRsaKeyLen;			// 64-192
	long lIcRsaE;				// 3 or 65537
	int  iCountryCode;			// 1-999
	int  iCurrencyCode;			// 1-999
} stIssData;

// ���÷���������ʾ����
// in  : pfvShowStatus : ��ʾ����ָ��
int iIssSetStatusShowFunc(void (*pfvShowStatus)(char *));

// ���ö�������, �����ⲿ�ṩIC���ӿ�
// �������: ����4�����������һ������NULL, ���ʾȡ��֮ǰ�Ըÿ������õĺ���
//           pfiTestCard  : ��⿨Ƭ�Ƿ���ں���ָ��
//           pfiResetCard : ��Ƭ��λ����ָ��
//           pfiDoApdu    : ִ��Apduָ���ָ��
//           pfiCloseCard : �رտ�Ƭ����ָ��
int iIssSetCardCtrlFunc(int (*pfiTestCard)(void),
				        int (*pfiResetCard)(unsigned char *psAtr),
				        int (*pfiDoApdu)(int iApduInLen, unsigned char *psApduIn, int *piApduOutLen, unsigned char *psApduOut),
				        int (*pfiCloseCard)(void));

// ���÷�������
// in  : iKeyMode  : ��Կ��ȡ��ʽ, 0:�����ڲ� 1:�����ļ� 2:���ܻ�
//       pszHsmIp  : ���ܻ�IP��ַ(�����Կ��ȡ��ʽΪ���ܻ�)
//       iHsmPort  : ���ܻ��˿ں�(�����Կ��ȡ��ʽΪ���ܻ�)
//       iLogFlag  : ������־��¼��־, 0:����¼ 1:��¼
// out : pszErrMsg : ������Ϣ
// ret : 0         : OK
//       1         : Error
int iIssSetEnv(int iKeyMode, char *pszHsmIp, int iHsmPort, int iLogFlag, char *pszErrMsg);

// ���÷�������
// in  : iUsage    : ��;, 0:���ڷ��� 1:����ɾ��Ӧ��
//     : pIssData  : ���˻�����
// out : pszErrMsg : ������Ϣ
// ret : 0         : OK
//       1         : Error
int iIssSetData(int iUsage, stIssData *pIssData, char *pszErrMsg);

// ����
// out : pszErrMsg : ������Ϣ
// ret : 0         : OK
//       1         : Error
int iIssCard(char *pszErrMsg);

// ɾ��Ӧ��
// out : pszErrMsg : ������Ϣ
// ret : 0         : OK
//       1         : Error
int iIssDelApps(char *pszErrMsg);

#ifdef __cplusplus
}
#endif

#endif
