/**************************************
File name     : KeyFace.h
Function      : PBOC2.0 ��Կ���ʽӿ�ģ��
                ���ӿ��ṩ������ܻ��޹ص�PBOC2.0��Կ���ʽӿ�
                ��Կ���Դ����� 1:ģ���ڲ� 2:�����ļ� 3:��������ܻ� 4:��ʿͨ���ܻ�
Author        : Yu Jun
First edition : Jul 5th, 2011
**************************************/
#ifndef _KEYFACE_H
#define _KEYFACE_H
#ifdef __cplusplus
extern "C" {
#endif

// Key Face Ret value
#define KFR_OK                  0x0000 // �ɹ�
#define KFR_PARA                0x0001 // ��������
#define KFR_COMM                0x0002 // ͨѶʧ��
#define KFR_INTERNAL            0x0003 // �ڲ�����
#define KFR_DATA_CHK            0x0004 // ���ܻ����������Լ�����
#define KFR_KEY_TYPE            0x0005 // ��Կ���Ͳ�֧��
#define KFR_RSA_PARSE           0x0006 // RSA��Կ�����������Ԥ��
#define KFR_OTHER               0x00FF // δ֪����
#define KFR_WESTONE             0x1000 // 0x10XX��ʾ��ʿͨ���ܻ�����, XXΪ�������

// ���ò���
// in  : iMode    : ģʽ 0:ģ������Կģʽ 1:���������ļ�ģʽ 2:��ʿͨ���ܻ�ģʽ
//       pszIp    : ���ܻ�IP��ַ, ģʽ2��ģʽ3����
//       iPortNo  : ���ܻ��˿ں�, ģʽ2��ģʽ3����
// ret : KFR_OK   : �ɹ�
//       ����     : ʧ��
int iKeySetMode(int iMode, char *pszIp, int iPortNo);

// ��Կ����
// in  : iKeyType  : 1:DES��Կ 2:RSA��Կ
//       iKeyIndex : ��Կ����
// out : piOutLen  : �������ݳ���
//       pOutData  : ��������
// ret : KFR_OK    : �ɹ�
//       ����      : ʧ��
int iKeyBackup(int iKeyType, int iKeyIndex, int *piOutLen, void *pOutData);

// ��Կ�ָ�
// in  : iKeyType  : 1:DES��Կ 2:RSA��Կ
//       iKeyIndex : ��Կ����
//       iInLen    : �������ݳ���
//       pInData   : ��������
// ret : KFR_OK    : �ɹ�
//       ����      : ʧ��
int iKeyRestore(int iKeyType, int iKeyIndex, int iInLen, void *pInData);

// ��Կɾ��
// in  : iKeyType  : 1:DES��Կ 2:RSA��Կ
//       iKeyIndex : ��Կ����
// ret : KFR_OK    : �ɹ�
//       ����      : ʧ��
int iKeyDelete(int iKeyType, int iKeyIndex);

// ����DES��Կ
// in  : iKeyIndex : ��Կ����(0-999), -1��ʾ���洢
//       pKey      : LMK���ܺ����, NULL��ʾ�����
// out : pKey      : LMK���ܺ�����Կ[16]+cks[8], NULL��ʾ�����
// ret : KFR_OK    : �ɹ�
//       ����      : ʧ��
// Note: iKeyIndexΪ-1ʱpKey����ΪNULL
int iKeyNewDesKey(int iKeyIndex, void *pKey);

// ����RSA��Կ
// in  : iKeyIndex : ��Կ����(0-19), -1��ʾ���洢
//       iKeyLen   : ��Կ����, ��λ:�ֽ� 64-248, ����Ϊż��
//       lE        : ����ָ��, 3��65537
//       pKey      : LMK���ܺ�����Կ, NULL��ʾ�����
// out : pModulus  : ����ģ��������ΪiKeyLen, NULL��ʾ�����
//       piOutLen  : ����ļ��ܺ���Կ����
//       pKey      : LMK���ܺ�����Կ, NULL��ʾ�����
// ret : KFR_OK    : �ɹ�
//       ����      : ʧ��
// Note: iKeyIndexΪ-1ʱpKey����ΪNULL
int iKeyNewRsaKey(int iKeyIndex, int iKeyLen, long lE, void *pModulus, int *piOutLen, void *pKey);

// RSA˽Կ����
// in  : iKeyIndex : RSA��Կ����
//       iKeyLen   : LMK���ܺ�RSA˽Կ����
//       pKey      : LMK���ܺ�RSA˽Կ, NULL��ʾʹ��������Կ
//       iLen      : �������ݳ���
//       pIn       : ��������
// out : pOut      : ������, ���ȵ���iLen
// ret : KFR_OK    : �ɹ�
//       ����      : ʧ��
int iKeyRsaPrivateBlock(int iKeyIndex, int iKeyLen, void *pKey, int iLen, void *pIn, void *pOut);

// RSA��Կ����
// in  : iKeyIndex : RSA��Կ����
//       iKeyLen   : ģ������(�ֽ�)
//       pModulus  : ģ��, NULL��ʾʹ��������Կ
//       lE        : ����ָ��, pMudulus!=NULLʱʹ��
//       iLen      : �������ݳ���
//       pIn       : ��������
// out : pOut      : ������, ���ȵ���iLen
// ret : KFR_OK    : �ɹ�
//       ����      : ʧ��
int iKeyRsaPublicBlock(int iKeyIndex, int iKeyLen, void *pModulus, long lE, int iLen, void *pIn, void *pOut);

// ��ȡRSA��Կ
// in  : iKeyIndex : RSA��Կ����
// out : piLen     : RSA��Կ����, ��λ:�ֽ�
//       plE       : ����ָ��
//       pModulus  : ģ��
// ret : KFR_OK    : �ɹ�
//       ����      : ʧ��
int iKeyRsaGetInfo(int iKeyIndex, int *piLen, long *plE, void *pModulus);

// ������Ƭ�ⲿ��֤����
// in  : iKey1Index : ��Կ����
//       iKey1Len   : LMK���ܺ���Կ����
//       pKey1      : LMK���ܺ���Կ, NULL��ʾʹ��������Կ
//       iDiv1Level : ��Կ��ɢ����(0-3)
//       pDiv1Data  : ��Կ��ɢ����[8*n]
//       pTermRand  : �ն������[8]
//       pCardData  : ��Ƭ����[28]
// out : pOut       : �ⲿ��֤���ļ�Mac[16]
// ret : KFR_OK     : �ɹ�
//       ����       : ʧ��
int iKeyCalExtAuth(int iKey1Index, int iKey1Len, void *pKey1, int iDiv1Level, void *pDiv1Data, void *pTermRand, void *pCardData, void *pOut);

// �����޸�KMC����
// in  : iKey1Index : ԭ��Կ����
//       iKey1Len   : LMK���ܺ�ԭ��Կ����
//       pKey1      : LMK���ܺ�ԭ��Կ, NULL��ʾʹ��������Կ
//       iDiv1Level : ԭ��Կ��ɢ����(0-3)
//       pDiv1Data  : ԭ��Կ��ɢ����[8*n]
//       pTermRand  : �ն������[8]
//       pCardData  : ��Ƭ����[28]
//       iKey2Index : ����Կ����
//       iKey2Len   : LMK���ܺ�����Կ����
//       pKey2      : LMK���ܺ�����Կ, NULL��ʾʹ��������Կ
//       iDiv2Level : ����Կ��ɢ����(0-3)
//       pDiv2Data  : ����Կ��ɢ����[8*n]
// out : pOut       : ����AuthKey[16]+AuthKeyKcv[3]+����MacKey[16]+MacKeyKcv[3]+����EncKey[16]+EncKeyKcv[3]
// ret : KFR_OK     : �ɹ�
//       ����       : ʧ��
int iKeyCalChangeKmc(int iKey1Index, int iKey1Len, void *pKey1, int iDiv1Level, void *pDiv1Data, void *pTermRand, void *pCardData, int iKey2Index, int iKey2Len, void *pKey2, int iDiv2Level, void *pDiv2Data, void *pOut);

// ����IC��DES��Կ����(��ʱ����)
// in  : iKey1Index : ������Կ����
//       iKey1Len   : LMK���ܺ�������Կ����
//       pKey1      : LMK���ܺ�������Կ, NULL��ʾʹ��������Կ
//       iDiv1Level : ������Կ��ɢ����(0-3)
//       pDiv1Data  : ������Կ��ɢ����[8*n]
//       pTermRand  : �ն������[8]
//       pCardData  : ��Ƭ����[28]
//       iKey2Index : Ӧ����Կ����
//       iKey2Len   : LMK���ܺ�Ӧ����Կ����
//       pKey2      : LMK���ܺ�Ӧ����Կ, NULL��ʾʹ��������Կ
//       iDiv2Level : Ӧ����Կ��ɢ����(0-3)
//       pDiv2Data  : Ӧ����Կ��ɢ����[8*n]
// out : pOut       : ����Key[16]+KeyKcv[3]
// ret : KFR_OK     : �ɹ�
//       ����       : ʧ��
int iKeyCalDesKey(int iKey1Index, int iKey1Len, void *pKey1, int iDiv1Level, void *pDiv1Data, void *pTermRand, void *pCardData, int iKey2Index, int iKey2Len, void *pKey2, int iDiv2Level, void *pDiv2Data, void *pOut);

// ����IC��PIN����(��ʱ����)
// in  : iKey1Index : ������Կ����
//       iKey1Len   : LMK���ܺ�������Կ����
//       pKey1      : LMK���ܺ�������Կ, NULL��ʾʹ��������Կ
//       iDiv1Level : ������Կ��ɢ����(0-3)
//       pDiv1Data  : ������Կ��ɢ����[8*n]
//       pTermRand  : �ն������[8]
//       pCardData  : ��Ƭ����[28]
//       pPinBlock  : �����������ݿ�[8]
// out : pOut       : ���ĸ����������ݿ�[8]
// ret : KFR_OK     : �ɹ�
//       ����       : ʧ��
int iKeyCalPin(int iKey1Index, int iKey1Len, void *pKey1, int iDiv1Level, void *pDiv1Data, void *pTermRand, void *pCardData, void *pPinBlock, void *pOut);

// ����IC��RSA��Կ����(��ʱ����)
// in  : iKey1Index : ������Կ����
//       iKey1Len   : LMK���ܺ�������Կ����
//       pKey1      : LMK���ܺ�������Կ, NULL��ʾʹ��������Կ
//       iDiv1Level : ������Կ��ɢ����(0-3)
//       pDiv1Data  : ������Կ��ɢ����[8*n]
//       pTermRand  : �ն������[8]
//       pCardData  : ��Ƭ����[28]
//       iRsaKeyLen : RSA��Կ����, ��λ:�ֽ�, 64-248, ����Ϊż��
//       lE         : ����ָ��ֵ, 3����65537
// out : pOut       : RSA��Կ��Կ����[n]+D����[n1]+P����[n2]+Q����[n2]+dP����[n2]+dQ����[n2]+qInv����[n2]
//                    n = iRsaKeyLen, n1 = (iRsaKeyLen+8)/8*8, n2 = (iRsaKeyLen/2+8)/8*8
// ret : KFR_OK     : �ɹ�
//       ����       : ʧ��
int iKeyCalRsaKey(int iKey1Index, int iKey1Len, void *pKey1, int iDiv1Level, void *pDiv1Data, void *pTermRand, void *pCardData, int iRsaKeyLen, long lE, void *pOut);

// ��֤ARQC��TC, ����ARPC
// in  : iKey1Index     : ��Կ����
//       iKey1Len       : LMK���ܺ���Կ����
//       pKey1          : LMK���ܺ���Կ, NULL��ʾʹ��������Կ
//       iDiv1Level     : ��Կ��ɢ����(0-3)
//       pDiv1Data      : ��Կ��ɢ����[8*n]
//       pATC           : �������к�[2]
//       iTransDataLen  : �������ݳ���
//       pTransData     : ��������
//       pARQC          : ARQC��TC
//       pARC           : ����Ӧ����, NULL��ʾֻ��֤ARQC��TC
// out : pARPC          : ARPC
// ret : KFR_OK         : �ɹ�
//       ����           : ʧ��
int iKeyVerifyAc(int iKey1Index, int iKey1Len, void *pKey1, int iDiv1Level, void *pDiv1Data, void *pATC, int iTransDataLen, void *pTransData, void *pARQC, void *pARC, void *pARPC);

// �ù�����Կ��������(�ű�)
// in  : iKey1Index    : ��Կ����
//       iKey1Len      : LMK���ܺ���Կ����
//       pKey1         : LMK���ܺ���Կ, NULL��ʾʹ��������Կ
//       iDiv1Level    : ��Կ��ɢ����(0-3)
//       pDiv1Data     : ��Կ��ɢ����[8*n]
//       pATC          : �������к�[2]
//       iPinFlag      : PinBlock���ܱ�־, 0:����PinBlock, 1:��PinBlock
//       iDataLen      : ���ݳ���
//       pData         : ����
// out : piOutLen      : ������ݳ���
//       pOutData      : �������
// ret : KFR_OK        : �ɹ�
//       ����          : ʧ��
int iKeyScriptEncData(int iKey1Index, int iKey1Len, void *pKey1, int iDiv1Level, void *pDiv1Data, void *pATC, int iPinFlag, int iDataLen, void *pData, int *piOutLen, void *pOutData);

// �ù�����Կ����Mac(�ű�)
// in  : iKey1Index    : ��Կ����
//       iKey1Len      : LMK���ܺ���Կ����
//       pKey1         : LMK���ܺ���Կ, NULL��ʾʹ��������Կ
//       iDiv1Level    : ��Կ��ɢ����(0-3)
//       pDiv1Data     : ��Կ��ɢ����[8*n]
//       pATC          : �������к�[2]
//       iDataLen      : ���ݳ���
//       pData         : ����
// out : pMac          : Mac[8]
// ret : KFR_OK        : �ɹ�
//       ����          : ʧ��
int iKeyScriptMac(int iKey1Index, int iKey1Len, void *pKey1, int iDiv1Level, void *pDiv1Data, void *pATC, int iDataLen, void *pData, void *pMac);

// ����������Կת��(����׼���ļ�����)
// in  : iKey1Index : ������Կ����
//       iKey1Len   : LMK���ܺ�������Կ����
//       pKey1      : LMK���ܺ�������Կ, NULL��ʾʹ��������Կ
//       iDiv1Level : ������Կ��ɢ����(0-3)
//       pDiv1Data  : ������Կ��ɢ����[8*n]
//       pTermRand  : �ն������[8]
//       pCardData  : ��Ƭ����[28]
//       iKey2Index : Ӧ����Կ����
//       iKey2Len   : LMK���ܺ�Ӧ����Կ����
//       pKey2      : LMK���ܺ�Ӧ����Կ, NULL��ʾʹ��������Կ
//       iDiv2Level : Ӧ����Կ��ɢ����(0-3)
//       pDiv2Data  : Ӧ����Կ��ɢ����[8*n]
//       iDataLen   : ԭ�������ݳ���, ����Ϊ8�ı���
//       pData      : ԭ��������
// out : pOut       : ת���ܺ�����, ���ȵ���ԭ�������ݳ���
// ret : KFR_OK     : �ɹ�
//       ����       : ʧ��
// Note: ����ԭΪӦ����Կ����, ��������Կ�Ĺ�����Կת����
int iKeyReEncData(int iKey1Index, int iKey1Len, void *pKey1, int iDiv1Level, void *pDiv1Data, void *pTermRand, void *pCardData, int iKey2Index, int iKey2Len, void *pKey2, int iDiv2Level, void *pDiv2Data, int iDataLen, void *pData, void *pOut);

// ����������Կת��(����׼���ļ�����)
// in  : iKey1Index : ������Կ����
//       iKey1Len   : LMK���ܺ�������Կ����
//       pKey1      : LMK���ܺ�������Կ, NULL��ʾʹ��������Կ
//       iDiv1Level : ������Կ��ɢ����(0-3)
//       pDiv1Data  : ������Կ��ɢ����[8*n]
//       pTermRand  : �ն������[8]
//       pCardData  : ��Ƭ����[28]
//       iKey2Index : Ӧ����Կ����
//       iKey2Len   : LMK���ܺ�Ӧ����Կ����
//       pKey2      : LMK���ܺ�Ӧ����Կ, NULL��ʾʹ��������Կ
//       iDiv2Level : Ӧ����Կ��ɢ����(0-3)
//       pDiv2Data  : Ӧ����Կ��ɢ����[8*n]
//       iPinFormat : ������ʽ, 1:��ʽ 2:��ʽ2
//       pPinBlock  : ԭ���ܺ������[8]
// out : pOut       : ת���ܺ������[8]
// ret : KFR_OK     : �ɹ�
//       ����       : ʧ��
// Note: ����ԭΪӦ����Կ����, ��������Կ�Ĺ�����Կת����
int iKeyReEncPin(int iKey1Index, int iKey1Len, void *pKey1, int iDiv1Level, void *pDiv1Data, void *pTermRand, void *pCardData, int iKey2Index, int iKey2Len, void *pKey2, int iDiv2Level, void *pDiv2Data, int iPinFormat, void *pPinBlock, void *pOut);

// Mac����(����׼��)
// in  : iKey1Index  : Ӧ����Կ����
//       iKey1Len    : LMK���ܺ�Ӧ����Կ����
//       pKey1       : LMK���ܺ�Ӧ����Կ, NULL��ʾʹ��������Կ
//       iDiv1Level  : Ӧ����Կ��ɢ����(0-3)
//       pDiv1Data   : Ӧ����Կ��ɢ����[8*n]
//       pMacKey     : ��Ӧ����Կ���ܺ��Mac��Կ
//       pIv         : ��ʼ����[8]
//       iDataLen    : ���ݳ���, ����Ϊ8�ı���
//       pData       : ����
// out : pMacKey     : ��Ӧ����Կ���ܺ�������ʱ��Կ[16]
//       pMac        : Mac[8]
// ret : KFR_OK      : �ɹ�
//       ����        : ʧ��
int iKeyDpCalMac(int iKey1Index, int iKey1Len, void *pKey1, int iDiv1Level, void *pDiv1Data, void *pIv, int iDataLen, void *pData, void *pMacKey, void *pMac);

// ��������׼��DES��Կ����(����׼��)
// in  : iKey1Index  : TK��Կ����
//       iKey1Len    : LMK���ܺ�TK��Կ����
//       pKey1       : LMK���ܺ�TK��Կ, NULL��ʾʹ��������Կ
//       iDiv1Level  : TK��Կ��ɢ����(0-3)
//       pDiv1Data   : TK��Կ��ɢ����[8*n]
//       iKey2Index  : Ӧ����Կ����
//       iKey2Len    : LMK���ܺ�Ӧ����Կ����
//       pKey2       : LMK���ܺ�Ӧ����Կ, NULL��ʾʹ��������Կ
//       iDiv2Level  : Ӧ����Կ��ɢ����(0-3)
//       pDiv2Data   : Ӧ����Կ��ɢ����[8*n]
// out : pOut        : OutKey[16]+Cks[3]
// ret : KFR_OK     : �ɹ�
//       ����       : ʧ��
int iKeyDpCalDesKey(int iKey1Index, int iKey1Len, void *pKey1, int iDiv1Level, void *pDiv1Data, int iKey2Index, int iKey2Len, void *pKey2, int iDiv2Level, void *pDiv2Data, void *pOut);

// ��������׼��RSA��Կ����(����׼��)
// in  : iKey1Index  : TK��Կ����
//       iKey1Len    : LMK���ܺ�TK��Կ����
//       pKey1       : LMK���ܺ�TK��Կ, NULL��ʾʹ��������Կ
//       iDiv1Level  : TK��Կ��ɢ����(0-3)
//       pDiv1Data   : TK��Կ��ɢ����[8*n]
//       iRsaKeyLen  : RSA��Կ����, ��λ:�ֽ�, 64-248, ����Ϊż��
//       lE          : ����ָ��ֵ, 3����65537
// out : pOut        : RSA��Կ��Կ����[n]+D����[n1]+P����[n2]+Q����[n2]+dP����[n2]+dQ����[n2]+qInv����[n2]
//                     n = iRsaKeyLen, n1 = (iRsaKeyLen+8)/8*8, n2 = (iRsaKeyLen/2+8)/8*8
// ret : KFR_OK      : �ɹ�
//       ����        : ʧ��
int iKeyDpCalRsaKey(int iKey1Index, int iKey1Len, void *pKey1, int iDiv1Level, void *pDiv1Data, int iRsaKeyLen, long lE, void *pOut);

#ifdef __cplusplus
}
#endif
#endif
