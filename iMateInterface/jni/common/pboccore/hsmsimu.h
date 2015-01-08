/**************************************
File name     : HSMSIMU.H
Function      : ���ܻ�������
Author        : Yu Jun
First edition : Mar 18th, 2011
Modified      : 
**************************************/
#ifndef _HSMSIMU_H
#define _HSMSIMU_H

// ������, ͬ��ʿͨ���ܻ�
#define HSMERR_OK           0x00    // �ɹ�
#define HSMERR_NO_MASTERKEY 0x01    // LMK������
#define HSMERR_MSG_LENGTH   0x02	// ��Ϣ���ȴ�
#define HSMERR_RSA_E        0x08    // RSA��Կָ������
#define HSMERR_NOT_ECBCBC   0x10    // ��/����ģʽ����ECB/CBC
#define HSMERR_PIN_BLOCK    0x13    // PINBLOCK����
#define HSMERR_PIN_FORMAT   0x14    // PINBLOCK��ʽ����
#define HSMERR_MAC_ALGO     0x24    // MAC�㷨����
#define HSMERR_CHECK        0x2D    // У��ʧ��
#define HSMERR_PIN_LEN      0x31    // PIN���ȴ���
#define HSMERR_PIN          0x47    // PIN�Ƿ�
#define HSMERR_SAVE_DES     0x52    // ���������Կʧ��
#define HSMERR_KEY_LEN      0x5F    // ��Կ���ȴ���
#define HSMERR_IN_DATA      0x67    // �������ݴ���
#define HSMERR_LEN_CHECK    0x68    // ������ʵ�����벻��
#define HSMERR_INDEX        0x70    // ������Կ����Խ��
#define HSMERR_NO_DESKEY    0x72    // ������Կ������
#define HSMERR_FLAG         0x74    // ��־����
#define HSMERR_TYPE         0x75    // ��Կ���ʹ���
#define HSMERR_DIV_NUM      0x76    // ��ɢ��������
#define HSMERR_KEY_CHECK    0x81    // У��ֵ����
#define HSMERR_MODULUS_LEN  0xB0    // RSA��Կģ������
#define HSMERR_DERREF_PUB   0xB2    // ��DER��ʽ��Կת��ΪREF��ʽʧ��
#define HSMERR_RSA_INDEX    0xB4    // RSA��Կ����Խ��
#define HSMERR_GEN_RSAKEY   0xB5    // ����RSA��Կʧ��
#define HSMERR_REFDER_PUB   0xB6    // ��REF��ʽ��ԿתΪDER��ʽʧ��
#define HSMERR_REFDER_PRI   0xBE    // ��REF��ʽ˽Կת��ΪDER��ʽʧ��
#define HSMERR_DERREF_PRI   0xBF    // ��DER��ʽ˽Կת��ΪREF��ʽʧ��
//#define HSMERR_RSA_INDEX    0xC1    // RSA��Կ����Խ�� ��ʿָͨ�������ͬʱ�ṩ��C1��B4, ���������B4
#define HSMERR_DATAIN_LEN   0xC2    // �������ݳ��ȴ���
#define HSMERR_PASSWORD     0xC4    // �����
#define HSMERR_SAVE_RSA     0xC9    // ����RSA��Կʧ��
#define HSMERR_NO_RSAKEY    0xCB    // RSA��Կ������
#define HSMERR_MODE         0xE0    // Mode����
#define HSMERR_RSA_FORMAT   0xF1    // ��/˽Կ��ʽ����
// ���´�������ʿͨ���ܻ�û���ṩ, ����ܻ�ר��
#define HSMERR_INTERNAL     0xFA	// �ڲ�����
#define HSMERR_AC           0xFB	// ACУ��ʧ��
#define HSMERR_NO_ENCKEY    0xFC    // ������Կ������
#define HSMERR_PAD_FORMAT   0xFD    // ���ģʽ�Ƿ�
#define HSMERR_PARA         0xFE    // ������
#define HSMERR_INS          0xFF    // ָ�ʶ��

int iHsmInit0(char *pszKeyFileName);
int iLmkProcess0(int iFlag, void *pIn, void *pOut);
int iKeyBackup0(int iKeyType, int iKeyIndex, int *piOutLen, void *pOutData);
int iKeyRestore0(int iKeyType, int iKeyIndex, int iInLen, void *pInData);
int iKeyDelete0(int iKeyType, int iKeyIndex);
int iKeyGenDesKey0(int iKeyIndex, void *pKey);
int iKeyGenRsaKey0(int iKeyIndex, int iKeyLen, long lE, void *pN, void *pKey, int *piOutLen);
int iKeyRsaPrivateBlock0(int iKeyIndex, void *pKey, int iLen, void *pIn, void *pOut);
int iKeyRsaPublicBlock0(int iKeyIndex, int iLen, void *pIn, void *pOut);
int iKeyRsaGetInfo0(int iKeyIndex, void *pKey, int *piLen, long *plE, void *pModulus);
int iKeyRsaPublicBlock20(int iLen, long lE, void *pN, void *pIn, void *pOut);
int iKeyCalExtAuth0(int iKey1Index, void *pKey1, int iDiv1Level, void *pDiv1Data, void *pTermRand, void *pCardData, void *pOut);
int iKeyCalChangeKmc0(int iKey1Index, void *pKey1, int iDiv1Level, void *pDiv1Data, void *pTermRand, void *pCardData, int iKey2Index, void *pKey2, int iDiv2Level, void *pDiv2Data, void *pOut);
int iKeyCalDesKey0(int iKey1Index, void *pKey1, int iDiv1Level, void *pDiv1Data, void *pTermRand, void *pCardData, int iKey2Index, void *pKey2, int iDiv2Level, void *pDiv2Data, void *pOut);
int iKeyCalPin0(int iKey1Index, void *pKey1, int iDiv1Level, void *pDiv1Data, void *pTermRand, void *pCardData, void *pPinBlock, void *pOut);
int iKeyCalRsaKey0(int iKey1Index, void *pKey1, int iDiv1Level, void *pDiv1Data, void *pTermRand, void *pCardData, int iRsaKeyLen, long lE, void *pOut);
int iKeyVerifyAc0(int iKey1Index, void *pKey1, int iDiv1Level, void *pDiv1Data, void *pATC, int iTransDataLen, void *pTransData, void *pARQC, void *pARC, void *pARPC);
int iKeyScriptEncData0(int iKey1Index, void *pKey1, int iDiv1Level, void *pDiv1Data, void *pATC, int iPinFlag, int iDataLen, void *pData, int *piOutLen, void *pOutData);
int iKeyScriptMac0(int iKey1Index, void *pKey1, int iDiv1Level, void *pDiv1Data, void *pATC, int iDataLen, void *pData, void *pMac);
int iKeyReEncData0(int iKey1Index, void *pKey1, int iDiv1Level, void *pDiv1Data, void *pTermRand, void *pCardData, int iKey2Index, void *pKey2, int iDiv2Level, void *pDiv2Data, int iDataLen, void *pData, void *pOut);
int iKeyReEncPin0(int iKey1Index, void *pKey1, int iDiv1Level, void *pDiv1Data, void *pTermRand, void *pCardData, int iKey2Index, void *pKey2, int iDiv2Level, void *pDiv2Data, void *pPinBlock, void *pOut);
int iKeyDpGenDesKey0(int iKey1Index, void *pKey1, int iDiv1Level, void *pDiv1Data, void *pOut);
int iKeyDpCalMac0(int iKey1Index, void *pKey1, int iDiv1Level, void *pDiv1Data, void *pMacKey, void *pIv, int iDataLen, void *pData, void *pMac, int iBlockFlag);
int iKeyDpCalMacFull0(int iKey1Index, void *pKey1, int iDiv1Level, void *pDiv1Data, void *pMacKey, int iDiv2Level, void *pDiv2Data, int iAlgoFlag, void *pIv, int iDataLen, void *pData, void *pMac, int iBlockFlag);
int iKeyDpCalDesKey0(int iKey1Index, void *pKey1, int iDiv1Level, void *pDiv1Data, int iKey2Index, void *pKey2, int iDiv2Level, void *pDiv2Data, void *pOut);
int iKeyDpCalRsaKey0(int iKey1Index, void *pKey1, int iDiv1Level, void *pDiv1Data, int iRsaKeyLen, long lE, void *pOut);
int iKeyEncDec0(int iKey1Index, void *pKey1, int iDiv1Level, void *pDiv1Data, void *pKey2, int iDiv2Level, void *pDiv2Data, int iEncDecFlag, int iPadFlag, void *pIv, int iDataLen, void *pData, int *piOutLen, void *pOutData);
int iKeyEncDecEcb0(int iKey1Index, void *pKey1, int iDiv1Level, void *pDiv1Data, int iFlag, int iDataLen, void *pData, void *pOut);
int iKeyDecEncData0(int iKey1Index, void *pKey1, int iDiv1Level, void *pDiv1Data, int iKey1IndexP, void *pKey1P, int iDiv1LevelP, void *pDiv1DataP, void *pIv1,
                    int iKey2Index, void *pKey2, int iDiv2Level, void *pDiv2Data, int iKey2IndexP, void *pKey2P, int iDiv2LevelP, void *pDiv2DataP, void *pIv2,
                    int iDataLen, void *pData, void *pOut);

//QINGBO
void vKeyCalSetDivMode(int iDivMode);
void vHsmChangeKmcKey(uchar *psNewKey);

#endif
