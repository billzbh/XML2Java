/* DO NOT EDIT THIS FILE - it is machine generated */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <jni.h>
#include <android/log.h>

#include "SyncCommon.h"
#include "MemoryCardApiJni.h"

# ifndef uint
# define uint unsigned int
# endif

# ifndef uchar
# define uchar unsigned char
# endif

# ifndef ulong
# define ulong unsigned long
# endif

# ifndef ushort
# define ushort unsigned short
# endif


jint JNICALL JNI_OnLoad(JavaVM* vm, void* reserved)
{
	MemoryCard_SetupJavaVM(vm);

    return JNI_VERSION_1_4;
}

/*
 * Class:     com_hxsmart_imateinterface_memorycardapi_MemoryCard
 * Method:    MemoryCard_TestCard
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_com_hxsmart_imateinterface_memorycardapi_MemoryCard_TestCard
  (JNIEnv *env, jobject this)
{
	return MemoryCard_TestCard();
}

JNIEXPORT void JNICALL Java_com_hxsmart_imateinterface_memorycardapi_MemoryCard_SetCardVoltage
  (JNIEnv *env, jobject this, jint voltageTag)
{
	return MemoryCard_SetCardVoltage(voltageTag);
}

JNIEXPORT jint JNICALL Java_com_hxsmart_imateinterface_memorycardapi_MemoryCard_TestCardType
  (JNIEnv *env, jobject this)
{
	uchar sResetData[100];

	int iRet = MemoryCard_TestCardType(sResetData);
	return iRet;
}

/*
 * Class:     com_hxsmart_imateinterface_memorycardapi_MemoryCard
 * Method:    MemoryCard_SLE4442_Open
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_com_hxsmart_imateinterface_memorycardapi_MemoryCard_SLE4442_1Open
  (JNIEnv *env, jobject this)
{
	MemoryCard_SLE4442_Open();
}

JNIEXPORT int JNICALL Java_com_hxsmart_imateinterface_memorycardapi_MemoryCard_SLE4442_1OpenAuto
  (JNIEnv *env, jobject this)
{
	return MemoryCard_SLE4442_OpenAuto();
}

/*
 * Class:     com_hxsmart_imateinterface_memorycardapi_MemoryCard
 * Method:    MemoryCard_SLE4442_Close
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_com_hxsmart_imateinterface_memorycardapi_MemoryCard_SLE4442_1Close
  (JNIEnv *env, jobject this)
{
	MemoryCard_SLE4442_Close();
}

/*
 * Class:     com_hxsmart_imateinterface_memorycardapi_MemoryCard
 * Method:    MemoryCard_SLE4442_ChkCode
 * Signature: ([B)I
 */
JNIEXPORT jint JNICALL Java_com_hxsmart_imateinterface_memorycardapi_MemoryCard_SLE4442_1ChkCode
  (JNIEnv *env, jobject this, jbyteArray securityCode)
{
	if (securityCode == NULL)
		return 100;

	uchar sSecurityCode[3];
	char *data = (*env)->GetByteArrayElements(env, securityCode, 0);
	if (data) {
		memcpy(sSecurityCode, data, 3);
		(*env)->ReleaseByteArrayElements(env, securityCode, data, 0);
	}
	return MemoryCard_SLE4442_ChkCode(sSecurityCode);
}

JNIEXPORT jint JNICALL Java_com_hxsmart_imateinterface_memorycardapi_MemoryCard_SLE4442_1ChkCodeEx
  (JNIEnv *env, jobject this, jbyteArray securityCode)
{
	if (securityCode == NULL)
		return 100;

	uchar sSecurityCode[8];
	char *data = (*env)->GetByteArrayElements(env, securityCode, 0);
	if (data) {
		memcpy(sSecurityCode, data, 8);
		(*env)->ReleaseByteArrayElements(env, securityCode, data, 0);
	}
	return MemoryCard_SLE4442_ChkCodeEx(sSecurityCode);
}

/*
 * Class:     com_hxsmart_imateinterface_memorycardapi_MemoryCard
 * Method:    MemoryCard_SLE4442_Read
 * Signature: (II[B)V
 */
JNIEXPORT void JNICALL Java_com_hxsmart_imateinterface_memorycardapi_MemoryCard_SLE4442_1Read
  (JNIEnv *env, jobject this, jint offset, jint dataLen, jbyteArray dataBuff)
{
	if (dataBuff == NULL ||  (*env)->GetArrayLength(env, dataBuff) < dataLen)
		return;

	uchar *sDataBuff = malloc(dataLen);
	memset(sDataBuff, 0xff, dataLen);
	MemoryCard_SLE4442_Read(offset, dataLen, sDataBuff);

	(*env)->SetByteArrayRegion(env, dataBuff, 0, dataLen, sDataBuff);
	free(sDataBuff);
}

/*
 * Class:     com_hxsmart_imateinterface_memorycardapi_MemoryCard
 * Method:    MemoryCard_SLE4442_Write
 * Signature: (II[B)V
 */
JNIEXPORT void JNICALL Java_com_hxsmart_imateinterface_memorycardapi_MemoryCard_SLE4442_1Write
  (JNIEnv *env, jobject this, jint offset, jint dataLen, jbyteArray dataBuff)
{
	if (dataBuff == NULL ||  (*env)->GetArrayLength(env, dataBuff) < dataLen)
		return;

	uchar *sDataBuff = malloc(dataLen);
	char *data = (*env)->GetByteArrayElements(env, dataBuff, 0);
	if (data) {
		memcpy(sDataBuff, data, dataLen);
		(*env)->ReleaseByteArrayElements(env, dataBuff, data, 0);
	}
	MemoryCard_SLE4442_Write(offset, dataLen, sDataBuff);
	free(sDataBuff);
}

/*
 * Class:     com_hxsmart_imateinterface_memorycardapi_MemoryCard
 * Method:    MemoryCard_SLE4428_Open
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_com_hxsmart_imateinterface_memorycardapi_MemoryCard_SLE4428_1Open
  (JNIEnv *env, jobject this)
{
	MemoryCard_SLE4428_Open();
}

JNIEXPORT int JNICALL Java_com_hxsmart_imateinterface_memorycardapi_MemoryCard_SLE4428_1OpenAuto
  (JNIEnv *env, jobject this)
{
	return MemoryCard_SLE4428_OpenAuto();
}

/*
 * Class:     com_hxsmart_imateinterface_memorycardapi_MemoryCard
 * Method:    MemoryCard_SLE4428_Close
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_com_hxsmart_imateinterface_memorycardapi_MemoryCard_SLE4428_1Close
  (JNIEnv *env, jobject this)
{
	MemoryCard_SLE4428_Close();
}

/*
 * Class:     com_hxsmart_imateinterface_memorycardapi_MemoryCard
 * Method:    MemoryCard_SLE4428_ChkCode
 * Signature: ([B)I
 */
JNIEXPORT jint JNICALL Java_com_hxsmart_imateinterface_memorycardapi_MemoryCard_SLE4428_1ChkCode
  (JNIEnv *env, jobject this, jbyteArray securityCode)
{
	if (securityCode == NULL)
		return 100;

	uchar sSecurityCode[2];
	char *data = (*env)->GetByteArrayElements(env, securityCode, 0);
	if (data) {
		memcpy(sSecurityCode, data, 2);
		(*env)->ReleaseByteArrayElements(env, securityCode, data, 0);
	}
	return MemoryCard_SLE4428_ChkCode(sSecurityCode);
}

JNIEXPORT jint JNICALL Java_com_hxsmart_imateinterface_memorycardapi_MemoryCard_SLE4428_1ChkCodeEx
  (JNIEnv *env, jobject this, jbyteArray securityCode)
{
	if (securityCode == NULL)
		return 100;

	uchar sSecurityCode[8];
	char *data = (*env)->GetByteArrayElements(env, securityCode, 0);
	if (data) {
		memcpy(sSecurityCode, data, 8);
		(*env)->ReleaseByteArrayElements(env, securityCode, data, 0);
	}
	return MemoryCard_SLE4428_ChkCodeEx(sSecurityCode);
}

/*
 * Class:     com_hxsmart_imateinterface_memorycardapi_MemoryCard
 * Method:    MemoryCard_SLE4428_Read
 * Signature: (II[B)V
 */
JNIEXPORT void JNICALL Java_com_hxsmart_imateinterface_memorycardapi_MemoryCard_SLE4428_1Read
  (JNIEnv *env, jobject this, jint offset, jint dataLen, jbyteArray dataBuff)
{
	if (dataBuff == NULL ||  (*env)->GetArrayLength(env, dataBuff) < dataLen)
		return;

	uchar *sDataBuff = malloc(dataLen);
	memset(sDataBuff, 0xff, dataLen);
	MemoryCard_SLE4428_Read(offset, dataLen, sDataBuff);

	(*env)->SetByteArrayRegion(env, dataBuff, 0, dataLen, sDataBuff);
	free(sDataBuff);
}

/*
 * Class:     com_hxsmart_imateinterface_memorycardapi_MemoryCard
 * Method:    MemoryCard_SLE4428_Write
 * Signature: (II[B)V
 */
JNIEXPORT void JNICALL Java_com_hxsmart_imateinterface_memorycardapi_MemoryCard_SLE4428_1Write
	(JNIEnv *env, jobject this, jint offset, jint dataLen, jbyteArray dataBuff)
{
	if (dataBuff == NULL ||  (*env)->GetArrayLength(env, dataBuff) < dataLen)
		return;

	uchar *sDataBuff = malloc(dataLen);
	char *data = (*env)->GetByteArrayElements(env, dataBuff, 0);
	if (data) {
		memcpy(sDataBuff, data, dataLen);
		(*env)->ReleaseByteArrayElements(env, dataBuff, data, 0);
	}
	MemoryCard_SLE4428_Write(offset, dataLen, sDataBuff);

	free(sDataBuff);
}

/*
 * Class:     com_hxsmart_imateinterface_memorycardapi_MemoryCard
 * Method:    MemoryCard_AT102_Open
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_com_hxsmart_imateinterface_memorycardapi_MemoryCard_AT102_1Open
  (JNIEnv *env, jobject this)
{
	MemoryCard_AT102_Open();
}

JNIEXPORT int JNICALL Java_com_hxsmart_imateinterface_memorycardapi_MemoryCard_AT102_1OpenAuto
  (JNIEnv *env, jobject this)
{
	return MemoryCard_AT102_OpenAuto();
}

/*
 * Class:     com_hxsmart_imateinterface_memorycardapi_MemoryCard
 * Method:    MemoryCard_AT102_Close
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_com_hxsmart_imateinterface_memorycardapi_MemoryCard_AT102_1Close
  (JNIEnv *env, jobject this)
{
	MemoryCard_AT102_Close();
}

/*
 * Class:     com_hxsmart_imateinterface_memorycardapi_MemoryCard
 * Method:    MemoryCard_AT102_ChkCode
 * Signature: ([B)I
 */
JNIEXPORT jint JNICALL Java_com_hxsmart_imateinterface_memorycardapi_MemoryCard_AT102_1ChkCode
  (JNIEnv *env, jobject this, jbyteArray securityCode)
{
	if (securityCode == NULL)
		return 100;

	uchar sSecurityCode[2];
	char *data = (*env)->GetByteArrayElements(env, securityCode, 0);
	if (data) {
		memcpy(sSecurityCode, data, 2);
		(*env)->ReleaseByteArrayElements(env, securityCode, data, 0);
	}
	return MemoryCard_AT102_ChkCode(sSecurityCode);
}

JNIEXPORT jint JNICALL Java_com_hxsmart_imateinterface_memorycardapi_MemoryCard_AT102_1ChkCodeEx
  (JNIEnv *env, jobject this, jbyteArray securityCode)
{
	if (securityCode == NULL)
		return 100;

	uchar sSecurityCode[8];
	char *data = (*env)->GetByteArrayElements(env, securityCode, 0);
	if (data) {
		memcpy(sSecurityCode, data, 8);
		(*env)->ReleaseByteArrayElements(env, securityCode, data, 0);
	}
	return MemoryCard_AT102_ChkCodeEx(sSecurityCode);
}

/*
 * Class:     com_hxsmart_imateinterface_memorycardapi_MemoryCard
 * Method:    MemoryCard_AT102_ReadWords
 * Signature: (II[B)V
 */
JNIEXPORT void JNICALL Java_com_hxsmart_imateinterface_memorycardapi_MemoryCard_AT102_1ReadWords
  (JNIEnv *env, jobject this, jint wordOffset, jint wordNum, jbyteArray dataBuff)
{
	if (dataBuff == NULL ||  (*env)->GetArrayLength(env, dataBuff) < wordNum * 2)
		return;

	uchar *sDataBuff = malloc(wordNum * 2);
	memset(sDataBuff, 0xff, wordNum * 2);
	MemoryCard_AT102_ReadWords(wordOffset, wordNum, sDataBuff);

	(*env)->SetByteArrayRegion(env, dataBuff, 0, wordNum * 2, sDataBuff);

	free(sDataBuff);
}

/*
 * Class:     com_hxsmart_imateinterface_memorycardapi_MemoryCard
 * Method:    MemoryCard_AT102_WriteWords
 * Signature: (II[B)I
 */
JNIEXPORT jint JNICALL Java_com_hxsmart_imateinterface_memorycardapi_MemoryCard_AT102_1WriteWords
  (JNIEnv *env, jobject this, jint wordOffset, jint wordNum, jbyteArray dataBuff)
{
	if (dataBuff == NULL ||  (*env)->GetArrayLength(env, dataBuff) < wordNum * 2)
		return 100;

	uchar *sDataBuff = malloc(wordNum * 2);
	char *data = (*env)->GetByteArrayElements(env, dataBuff, 0);
	if (data) {
		memcpy(sDataBuff, data, wordNum * 2);
		(*env)->ReleaseByteArrayElements(env, dataBuff, data, 0);
	}
	int iRet = MemoryCard_AT102_WriteWords(wordOffset, wordNum, sDataBuff);
	free(sDataBuff);

	return iRet;
}

/*
 * Class:     com_hxsmart_imateinterface_memorycardapi_MemoryCard
 * Method:    MemoryCard_AT102_EraseNonApp
 * Signature: (II)I
 */
JNIEXPORT jint JNICALL Java_com_hxsmart_imateinterface_memorycardapi_MemoryCard_AT102_1EraseNonApp
  (JNIEnv *env, jobject this, jint wordOffset, jint wordNum)
{
	return MemoryCard_AT102_EraseNonApp(wordOffset, wordNum);
}

/*
 * Class:     com_hxsmart_imateinterface_memorycardapi_MemoryCard
 * Method:    MemoryCard_AT102_EraseApp
 * Signature: (II[B)I
 */
JNIEXPORT jint JNICALL Java_com_hxsmart_imateinterface_memorycardapi_MemoryCard_AT102_1EraseApp
  (JNIEnv *env, jobject this, jint area, jint limited, jbyteArray eraseKey)
{
	uchar *psEraseKey = NULL;
	int length = 0;
	if (eraseKey != 0) {
		length = (*env)->GetArrayLength(env, eraseKey);
		if (length > 0) {
			psEraseKey = malloc(length);
		}
		char *data = (*env)->GetByteArrayElements(env, eraseKey, 0);
		if (data) {
			memcpy(psEraseKey, data, length);
			(*env)->ReleaseByteArrayElements(env, eraseKey, data, 0);
		}
	}

	int iRet = MemoryCard_AT102_EraseApp(area, limited, psEraseKey);
	free(psEraseKey);
	return iRet;
}

JNIEXPORT jint JNICALL Java_com_hxsmart_imateinterface_memorycardapi_MemoryCard_AT102_1EraseAppEx
  (JNIEnv *env, jobject this, jint area, jint limited, jbyteArray eraseKey)
{
	uchar *psEraseKey = NULL;
	int length = 0;
	if (eraseKey != 0) {
		length = (*env)->GetArrayLength(env, eraseKey);
		if (length > 0) {
			psEraseKey = malloc(length);
		}
		char *data = (*env)->GetByteArrayElements(env, eraseKey, 0);
		if (data) {
			memcpy(psEraseKey, data, length);
			(*env)->ReleaseByteArrayElements(env, eraseKey, data, 0);
		}
	}

	int iRet = MemoryCard_AT102_EraseAppEx(area, limited, psEraseKey);
	free(psEraseKey);
	return iRet;
}

/*
 * Class:     com_hxsmart_imateinterface_memorycardapi_MemoryCard
 * Method:    MemoryCard_AT102_ReadAZ
 * Signature: (I[B)V
 */
JNIEXPORT void JNICALL Java_com_hxsmart_imateinterface_memorycardapi_MemoryCard_AT102_1ReadAZ
  (JNIEnv *env, jobject this, jint area, jbyteArray dataBuff)
{
	if (dataBuff == NULL ||  (*env)->GetArrayLength(env, dataBuff) < 64)
		return ;
	uchar sDataBuff[64];
	memset(sDataBuff, 0xff, 64);
	MemoryCard_AT102_ReadAZ(area, sDataBuff);

	(*env)->SetByteArrayRegion(env, dataBuff, 0,64, sDataBuff);
}

/*
 * Class:     com_hxsmart_imateinterface_memorycardapi_MemoryCard
 * Method:    MemoryCard_AT102_WriteAZ
 * Signature: (I[B)I
 */
JNIEXPORT jint JNICALL Java_com_hxsmart_imateinterface_memorycardapi_MemoryCard_AT102_1WriteAZ
  (JNIEnv *env, jobject this, jint area, jbyteArray dataBuff)
{
	if (dataBuff == NULL ||  (*env)->GetArrayLength(env, dataBuff) < 64)
		return 100;

	uchar sDataBuff[64];
	char *data = (*env)->GetByteArrayElements(env, dataBuff, 0);
	if (data) {
		memcpy(sDataBuff, data, 64);
		(*env)->ReleaseByteArrayElements(env, dataBuff, data, 0);
	}
	return MemoryCard_AT102_WriteAZ(area, sDataBuff);
}

/*
 * Class:     com_hxsmart_imateinterface_memorycardapi_MemoryCard
 * Method:    MemoryCard_AT102_ReadMTZ
 * Signature: ([B)V
 */
JNIEXPORT void JNICALL Java_com_hxsmart_imateinterface_memorycardapi_MemoryCard_AT102_1ReadMTZ
  (JNIEnv *env, jobject this, jbyteArray dataBuff)
{
	if (dataBuff == NULL ||  (*env)->GetArrayLength(env, dataBuff) < 2)
		return ;
	uchar sDataBuff[2];
	memset(sDataBuff, 0xff, 2);
	MemoryCard_AT102_ReadMTZ(sDataBuff);

	(*env)->SetByteArrayRegion(env, dataBuff, 0, 2, sDataBuff);
}

/*
 * Class:     com_hxsmart_imateinterface_memorycardapi_MemoryCard
 * Method:    MemoryCard_AT102_UpdateMTZ
 * Signature: ([B)I
 */
JNIEXPORT jint JNICALL Java_com_hxsmart_imateinterface_memorycardapi_MemoryCard_AT102_1UpdateMTZ
  (JNIEnv *env, jobject this, jbyteArray dataBuff)
{
	if (dataBuff == NULL ||  (*env)->GetArrayLength(env, dataBuff) < 2)
		return 100;
	uchar sDataBuff[2];
	memset(sDataBuff, 0xff, 2);
	return MemoryCard_AT102_UpdateMTZ(sDataBuff);
}

/*
 * Class:     com_hxsmart_imateinterface_memorycardapi_MemoryCard
 * Method:    MemoryCard_AT1604_Open
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_com_hxsmart_imateinterface_memorycardapi_MemoryCard_AT1604_1Open
  (JNIEnv *env, jobject this)
{
	MemoryCard_AT1604_Open();
}

JNIEXPORT int JNICALL Java_com_hxsmart_imateinterface_memorycardapi_MemoryCard_AT1604_1OpenAuto
  (JNIEnv *env, jobject this)
{
	return MemoryCard_AT1604_OpenAuto();
}

/*
 * Class:     com_hxsmart_imateinterface_memorycardapi_MemoryCard
 * Method:    MemoryCard_AT1604_Close
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_com_hxsmart_imateinterface_memorycardapi_MemoryCard_AT1604_1Close
  (JNIEnv *env, jobject this)
{
	MemoryCard_AT1604_Close();
}

/*
 * Class:     com_hxsmart_imateinterface_memorycardapi_MemoryCard
 * Method:    MemoryCard_AT1604_ChkCode
 * Signature: ([B)I
 */
JNIEXPORT jint JNICALL Java_com_hxsmart_imateinterface_memorycardapi_MemoryCard_AT1604_1ChkCode
  (JNIEnv *env, jobject this, jbyteArray securityCode)
{
	if (securityCode == NULL)
		return 100;

	uchar sSecurityCode[2];
	char *data = (*env)->GetByteArrayElements(env, securityCode, 0);
	if (data) {
		memcpy(sSecurityCode, data, 2);
		(*env)->ReleaseByteArrayElements(env, securityCode, data, 0);
	}
	return MemoryCard_AT1604_ChkCode(sSecurityCode);
}

JNIEXPORT jint JNICALL Java_com_hxsmart_imateinterface_memorycardapi_MemoryCard_AT1604_1ChkCodeEx
  (JNIEnv *env, jobject this, jbyteArray securityCode)
{
	if (securityCode == NULL)
		return 100;

	uchar sSecurityCode[8];
	char *data = (*env)->GetByteArrayElements(env, securityCode, 0);
	if (data) {
		memcpy(sSecurityCode, data, 8);
		(*env)->ReleaseByteArrayElements(env, securityCode, data, 0);
	}
	return MemoryCard_AT1604_ChkCodeEx(sSecurityCode);
}

/*
 * Class:     com_hxsmart_imateinterface_memorycardapi_MemoryCard
 * Method:    MemoryCard_AT1604_ChkAreaCode
 * Signature: (I[B)I
 */
JNIEXPORT jint JNICALL Java_com_hxsmart_imateinterface_memorycardapi_MemoryCard_AT1604_1ChkAreaCode
  (JNIEnv *env, jobject this, jint area, jbyteArray securityCode)
{
	if (securityCode == NULL)
		return 100;

	uchar sSecurityCode[2];
	char *data = (*env)->GetByteArrayElements(env, securityCode, 0);
	if (data) {
		memcpy(sSecurityCode, data, 2);
		(*env)->ReleaseByteArrayElements(env, securityCode, data, 0);
	}
	return MemoryCard_AT1604_ChkAreaCode(area, sSecurityCode);
}

JNIEXPORT jint JNICALL Java_com_hxsmart_imateinterface_memorycardapi_MemoryCard_AT1604_1ChkAreaCodeEx
  (JNIEnv *env, jobject this, jint area, jbyteArray securityCode)
{
	if (securityCode == NULL)
		return 100;

	uchar sSecurityCode[8];
	char *data = (*env)->GetByteArrayElements(env, securityCode, 0);
	if (data) {
		memcpy(sSecurityCode, data, 8);
		(*env)->ReleaseByteArrayElements(env, securityCode, data, 0);
	}
	return MemoryCard_AT1604_ChkAreaCodeEx(area, sSecurityCode);
}

JNIEXPORT jint JNICALL Java_com_hxsmart_imateinterface_memorycardapi_MemoryCard_AT1604_1ChkAreaEraseCode
  (JNIEnv *env, jobject this, jint area, jbyteArray securityCode)
{
	if (securityCode == NULL)
		return 100;

	uchar sSecurityCode[2];
	char *data = (*env)->GetByteArrayElements(env, securityCode, 0);
	if (data) {
		memcpy(sSecurityCode, data, 2);
		(*env)->ReleaseByteArrayElements(env, securityCode, data, 0);
	}
	return MemoryCard_AT1604_ChkAreaEraseCode(area, sSecurityCode);
}

JNIEXPORT jint JNICALL Java_com_hxsmart_imateinterface_memorycardapi_MemoryCard_AT1604_1ChkAreaEraseCodeEx
  (JNIEnv *env, jobject this, jint area, jbyteArray securityCode)
{
	if (securityCode == NULL)
		return 100;

	uchar sSecurityCode[8];
	char *data = (*env)->GetByteArrayElements(env, securityCode, 0);
	if (data) {
		memcpy(sSecurityCode, data, 8);
		(*env)->ReleaseByteArrayElements(env, securityCode, data, 0);
	}
	return MemoryCard_AT1604_ChkAreaEraseCodeEx(area, sSecurityCode);
}

/*
 * Class:     com_hxsmart_imateinterface_memorycardapi_MemoryCard
 * Method:    MemoryCard_AT1604_Read
 * Signature: (II[B)V
 */
JNIEXPORT void JNICALL Java_com_hxsmart_imateinterface_memorycardapi_MemoryCard_AT1604_1Read
  (JNIEnv *env, jobject this, jint offset, jint dataLen, jbyteArray dataBuff)
{
	if (dataBuff == NULL ||  (*env)->GetArrayLength(env, dataBuff) < dataLen)
		return;

	uchar *sDataBuff = malloc(dataLen);
	memset(sDataBuff, 0xff, dataLen);
	MemoryCard_AT1604_Read(offset, dataLen, sDataBuff);

	(*env)->SetByteArrayRegion(env, dataBuff, 0, dataLen, sDataBuff);

	free(sDataBuff);
}

/*
 * Class:     com_hxsmart_imateinterface_memorycardapi_MemoryCard
 * Method:    MemoryCard_AT1604_Write
 * Signature: (II[B)I
 */
JNIEXPORT jint JNICALL Java_com_hxsmart_imateinterface_memorycardapi_MemoryCard_AT1604_1Write
  (JNIEnv *env, jobject this, jint offset, jint dataLen, jbyteArray dataBuff)
{
	if (dataBuff == NULL ||  (*env)->GetArrayLength(env, dataBuff) < dataLen)
		return 100;

	uchar *sDataBuff = malloc(dataLen);
	char *data = (*env)->GetByteArrayElements(env, dataBuff, 0);
	if (data) {
		memcpy(sDataBuff, data, dataLen);
		(*env)->ReleaseByteArrayElements(env, dataBuff, data, 0);
	}
	int iRet = MemoryCard_AT1604_Write(offset, dataLen, sDataBuff);
	free(sDataBuff);

	return iRet;
}

/*
 * Class:     com_hxsmart_imateinterface_memorycardapi_MemoryCard
 * Method:    MemoryCard_AT1604_ReadAZ
 * Signature: (III[B)V
 */
JNIEXPORT void JNICALL Java_com_hxsmart_imateinterface_memorycardapi_MemoryCard_AT1604_1ReadAZ
  (JNIEnv *env, jobject this, jint area, jint offset, jint dataLen, jbyteArray dataBuff)
{
	if (dataBuff == NULL ||  (*env)->GetArrayLength(env, dataBuff) < dataLen)
		return;

	uchar *sDataBuff = malloc(dataLen);
	memset(sDataBuff, 0xff, dataLen);
	MemoryCard_AT1604_ReadAZ(area, offset, dataLen, sDataBuff);

	(*env)->SetByteArrayRegion(env, dataBuff, 0, dataLen, sDataBuff);

	free(sDataBuff);
}

/*
 * Class:     com_hxsmart_imateinterface_memorycardapi_MemoryCard
 * Method:    MemoryCard_AT1604_WriteAZ
 * Signature: (III[B)I
 */
JNIEXPORT jint JNICALL Java_com_hxsmart_imateinterface_memorycardapi_MemoryCard_AT1604_1WriteAZ
  (JNIEnv *env, jobject this, jint area, jint offset, jint dataLen, jbyteArray dataBuff)
{
	if (dataBuff == NULL ||  (*env)->GetArrayLength(env, dataBuff) < dataLen)
		return 100;

	uchar *sDataBuff = malloc(dataLen);
	char *data = (*env)->GetByteArrayElements(env, dataBuff, 0);
	if (data) {
		memcpy(sDataBuff, data, dataLen);
		(*env)->ReleaseByteArrayElements(env, dataBuff, data, 0);
	}
	int iRet = MemoryCard_AT1604_WriteAZ(area, offset, dataLen, sDataBuff);
	free(sDataBuff);

	return iRet;
}

/*
 * Class:     com_hxsmart_imateinterface_memorycardapi_MemoryCard
 * Method:    MemoryCard_AT1604_Erase
 * Signature: (II)I
 */
JNIEXPORT jint JNICALL Java_com_hxsmart_imateinterface_memorycardapi_MemoryCard_AT1604_1Erase
  (JNIEnv *env, jobject this, jint offset, jint dataLen)
{
	return MemoryCard_AT1604_Erase(offset, dataLen);
}

/*
 * Class:     com_hxsmart_imateinterface_memorycardapi_MemoryCard
 * Method:    MemoryCard_AT1604_EraseAZ
 * Signature: (III)I
 */
JNIEXPORT jint JNICALL Java_com_hxsmart_imateinterface_memorycardapi_MemoryCard_AT1604_1EraseAZ
  (JNIEnv *env, jobject this, jint area, jint offset, jint dataLen)
{
	return MemoryCard_AT1604_EraseAZ(area, offset, dataLen);
}

/*
 * Class:     com_hxsmart_imateinterface_memorycardapi_MemoryCard
 * Method:    MemoryCard_AT1604_ReadMTZ
 * Signature: ([B)V
 */
JNIEXPORT void JNICALL Java_com_hxsmart_imateinterface_memorycardapi_MemoryCard_AT1604_1ReadMTZ
  (JNIEnv *env, jobject this, jbyteArray dataBuff)
{
	if (dataBuff == NULL ||  (*env)->GetArrayLength(env, dataBuff) < 2)
		return ;
	uchar sDataBuff[2];
	memset(sDataBuff, 0xff, 2);
	MemoryCard_AT1604_ReadMTZ(sDataBuff);

	(*env)->SetByteArrayRegion(env, dataBuff, 0, 2, sDataBuff);
}

/*
 * Class:     com_hxsmart_imateinterface_memorycardapi_MemoryCard
 * Method:    MemoryCard_AT1604_WriteMTZ
 * Signature: ([B)I
 */
JNIEXPORT jint JNICALL Java_com_hxsmart_imateinterface_memorycardapi_MemoryCard_AT1604_1WriteMTZ
  (JNIEnv *env, jobject this, jbyteArray dataBuff)
{
	if (dataBuff == NULL ||  (*env)->GetArrayLength(env, dataBuff) < 2)
		return 100;
	uchar sDataBuff[2];
	char *data = (*env)->GetByteArrayElements(env, dataBuff, 0);
	if (data) {
		memcpy(sDataBuff, data, 2);
		(*env)->ReleaseByteArrayElements(env, dataBuff, data, 0);
	}

	return MemoryCard_AT1604_WriteMTZ(sDataBuff);
}

/*
 * Class:     com_hxsmart_imateinterface_memorycardapi_MemoryCard
 * Method:    MemoryCard_AT1608_Open
 * Signature: ([B)V
 */
JNIEXPORT void JNICALL Java_com_hxsmart_imateinterface_memorycardapi_MemoryCard_AT1608_1Open
  (JNIEnv *env, jobject this, jbyteArray resetData)
{
	if (resetData == NULL)
		return ;
	uchar sDataBuff[100];
	memset(sDataBuff, 0xff, 4);
	MemoryCard_AT1608_Open(sDataBuff);

	(*env)->SetByteArrayRegion(env, resetData, 0, 4, sDataBuff);
}

JNIEXPORT int JNICALL Java_com_hxsmart_imateinterface_memorycardapi_MemoryCard_AT1608_1OpenAuto
  (JNIEnv *env, jobject this, jbyteArray resetData)
{
	if (resetData == NULL ||  (*env)->GetArrayLength(env, resetData) < 4)
		return 1;

	uchar sDataBuff[100];
	int ret = MemoryCard_AT1608_OpenAuto(sDataBuff);

	(*env)->SetByteArrayRegion(env, resetData, 0, 4, sDataBuff);

	return ret;
}

JNIEXPORT int JNICALL Java_com_hxsmart_imateinterface_memorycardapi_MemoryCard_AT1608_1ReadFuse
  (JNIEnv *env, jobject this, jbyteArray fuse)
{
	if (fuse == NULL ||  (*env)->GetArrayLength(env, fuse) < 1)
		return 1;

	uchar sDataBuff[100];
	int ret = MemoryCard_AT1608_ReadFuse(sDataBuff);

	(*env)->SetByteArrayRegion(env, fuse, 0, 1, sDataBuff);

	return ret;
}

JNIEXPORT int JNICALL Java_com_hxsmart_imateinterface_memorycardapi_MemoryCard_AT1608_1WriteFuse
  (JNIEnv *env, jobject this)
{
	return MemoryCard_AT1608_WriteFuse();
}

/*
 * Class:     com_hxsmart_imateinterface_memorycardapi_MemoryCard
 * Method:    MemoryCard_AT1608_Close
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_com_hxsmart_imateinterface_memorycardapi_MemoryCard_AT1608_1Close
  (JNIEnv *env, jobject this)
{
	MemoryCard_AT1608_Close();
}

/*
 * Class:     com_hxsmart_imateinterface_memorycardapi_MemoryCard
 * Method:    MemoryCard_AT1608_SetAZ
 * Signature: (I)I
 */
JNIEXPORT jint JNICALL Java_com_hxsmart_imateinterface_memorycardapi_MemoryCard_AT1608_1SetAZ
  (JNIEnv *env, jobject this, jint az)
{
	return MemoryCard_AT1608_SetAZ(az);
}

/*
 * Class:     com_hxsmart_imateinterface_memorycardapi_MemoryCard
 * Method:    MemoryCard_AT1608_ChkCode
 * Signature: (I[B)I
 */
JNIEXPORT jint JNICALL Java_com_hxsmart_imateinterface_memorycardapi_MemoryCard_AT1608_1ChkCode
  (JNIEnv *env, jobject this, jint index, jbyteArray securityCode)
{
	if (securityCode == NULL)
		return 100;

	uchar sSecurityCode[3];
	char *data = (*env)->GetByteArrayElements(env, securityCode, 0);
	if (data) {
		memcpy(sSecurityCode, data, 3);
		(*env)->ReleaseByteArrayElements(env, securityCode, data, 0);
	}
	return MemoryCard_AT1608_ChkCode(index, sSecurityCode);
}

JNIEXPORT jint JNICALL Java_com_hxsmart_imateinterface_memorycardapi_MemoryCard_AT1608_1ChkCodeEx
  (JNIEnv *env, jobject this, jint index, jbyteArray securityCode)
{
	if (securityCode == NULL)
		return 100;

	uchar sSecurityCode[8];
	char *data = (*env)->GetByteArrayElements(env, securityCode, 0);
	if (data) {
		memcpy(sSecurityCode, data, 8);
		(*env)->ReleaseByteArrayElements(env, securityCode, data, 0);
	}
	return MemoryCard_AT1608_ChkCodeEx(index, sSecurityCode);
}

/*
 * Class:     com_hxsmart_imateinterface_memorycardapi_MemoryCard
 * Method:    MemoryCard_AT1608_Auth
 * Signature: ([B)I
 */
JNIEXPORT jint JNICALL Java_com_hxsmart_imateinterface_memorycardapi_MemoryCard_AT1608_1Auth
  (JNIEnv *env, jobject this, jbyteArray gc)
{
	if (gc == NULL)
		return 100;

	uchar sGc[8];
	char *data = (*env)->GetByteArrayElements(env, gc, 0);
	if (data) {
		memcpy(sGc, data, 8);
		(*env)->ReleaseByteArrayElements(env, gc, data, 0);
	}
	return MemoryCard_AT1608_Auth(sGc);
}

JNIEXPORT jint JNICALL Java_com_hxsmart_imateinterface_memorycardapi_MemoryCard_AT1608_1AuthEx
  (JNIEnv *env, jobject this, jbyteArray gc)
{
	if (gc == NULL)
		return 100;

	uchar sGc[16];
	char *data = (*env)->GetByteArrayElements(env, gc, 0);
	if (data) {
		memcpy(sGc, data, 16);
		(*env)->ReleaseByteArrayElements(env, gc, data, 0);
	}
	return MemoryCard_AT1608_AuthEx(sGc);
}

/*
 * Class:     com_hxsmart_imateinterface_memorycardapi_MemoryCard
 * Method:    MemoryCard_AT102_Read
 * Signature: (III[B)I
 */
JNIEXPORT jint JNICALL Java_com_hxsmart_imateinterface_memorycardapi_MemoryCard_AT1608_1Read
  (JNIEnv *env, jobject this, jint level, jint offset, jint dataLen, jbyteArray dataBuff)
{
	if (dataBuff == NULL ||  (*env)->GetArrayLength(env, dataBuff) < dataLen)
		return 100;

	uchar *sDataBuff = malloc(dataLen);
	memset(sDataBuff, 0xff, dataLen);
	int iRet = MemoryCard_AT1608_Read(level, offset, dataLen, sDataBuff);
	if (iRet == 0)
		(*env)->SetByteArrayRegion(env, dataBuff, 0, dataLen, sDataBuff);

	free(sDataBuff);

	return iRet;
}

/*
 * Class:     com_hxsmart_imateinterface_memorycardapi_MemoryCard
 * Method:    MemoryCard_AT1608_Write
 * Signature: (III[B)I
 */
JNIEXPORT jint JNICALL Java_com_hxsmart_imateinterface_memorycardapi_MemoryCard_AT1608_1Write
 (JNIEnv *env, jobject this, jint level, jint offset, jint dataLen, jbyteArray dataBuff)
{
	if (dataBuff == NULL ||  (*env)->GetArrayLength(env, dataBuff) < dataLen)
		return 100;

	uchar *sDataBuff = malloc(dataLen);
	char *data = (*env)->GetByteArrayElements(env, dataBuff, 0);
	if (data) {
		memcpy(sDataBuff, data, dataLen);
		(*env)->ReleaseByteArrayElements(env, dataBuff, data, 0);
	}
	int iRet = MemoryCard_AT1608_Write(level, offset, dataLen, sDataBuff);
	free(sDataBuff);

	return iRet;
}

/*
 * Class:     com_hxsmart_imateinterface_memorycardapi_MemoryCard
 * Method:    MemoryCard_AT24Cxx_Open
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_com_hxsmart_imateinterface_memorycardapi_MemoryCard_AT24Cxx_1Open
  (JNIEnv *env, jobject this)
{
	MemoryCard_AT24Cxx_Open();
}

JNIEXPORT int JNICALL Java_com_hxsmart_imateinterface_memorycardapi_MemoryCard_AT24Cxx_1OpenAuto
  (JNIEnv *env, jobject this)
{
	return MemoryCard_AT24Cxx_OpenAuto();
}

/*
 * Class:     com_hxsmart_imateinterface_memorycardapi_MemoryCard
 * Method:    MemoryCard_AT24Cxx_Close
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_com_hxsmart_imateinterface_memorycardapi_MemoryCard_AT24Cxx_1Close
  (JNIEnv *env, jobject this)
{
	MemoryCard_AT24Cxx_Close();
}

/*
 * Class:     com_hxsmart_imateinterface_memorycardapi_MemoryCard
 * Method:    MemoryCard_AT24Cxx_Read
 * Signature: (II[B)I
 */
JNIEXPORT jint JNICALL Java_com_hxsmart_imateinterface_memorycardapi_MemoryCard_AT24Cxx_1Read
  (JNIEnv *env, jobject this, jint offset, jint dataLen, jbyteArray dataBuff)
{
	if (dataBuff == NULL ||  (*env)->GetArrayLength(env, dataBuff) < dataLen)
		return 100;

	uchar *sDataBuff = malloc(dataLen);
	memset(sDataBuff, 0xff, dataLen);
	int iRet = MemoryCard_AT24Cxx_Read(offset, dataLen, sDataBuff);
	if (iRet == 0)
		(*env)->SetByteArrayRegion(env, dataBuff, 0, dataLen, sDataBuff);

	free(sDataBuff);

	return iRet;
}

/*
 * Class:     com_hxsmart_imateinterface_memorycardapi_MemoryCard
 * Method:    MemoryCard_AT24Cxx_Write
 * Signature: (II[B)I
 */
JNIEXPORT jint JNICALL Java_com_hxsmart_imateinterface_memorycardapi_MemoryCard_AT24Cxx_1Write
  (JNIEnv *env, jobject this, jint offset, jint dataLen, jbyteArray dataBuff)
{
	if (dataBuff == NULL ||  (*env)->GetArrayLength(env, dataBuff) < dataLen)
		return 100;

	uchar *sDataBuff = malloc(dataLen);
	char *data = (*env)->GetByteArrayElements(env, dataBuff, 0);
	if (data) {
		memcpy(sDataBuff, data, dataLen);
		(*env)->ReleaseByteArrayElements(env, dataBuff, data, 0);
	}
	int iRet = MemoryCard_AT24Cxx_Write(offset, dataLen, sDataBuff);
	free(sDataBuff);

	return iRet;
}

/*
 * Class:     com_hxsmart_imateinterface_memorycardapi_MemoryCard
 * Method:    MemoryCard_AT24C32_Read
 * Signature: (II[B)I
 */
JNIEXPORT jint JNICALL Java_com_hxsmart_imateinterface_memorycardapi_MemoryCard_AT24C32_1Read
(JNIEnv *env, jobject this, jint offset, jint dataLen, jbyteArray dataBuff)
{
	if (dataBuff == NULL ||  (*env)->GetArrayLength(env, dataBuff) < dataLen)
		return 100;

	uchar *sDataBuff = malloc(dataLen);
	memset(sDataBuff, 0xff, dataLen);
	int iRet = MemoryCard_AT24C32_Read(offset, dataLen, sDataBuff);
	if (iRet == 0)
		(*env)->SetByteArrayRegion(env, dataBuff, 0, dataLen, sDataBuff);

	free(sDataBuff);

	return iRet;
}

/*
 * Class:     com_hxsmart_imateinterface_memorycardapi_MemoryCard
 * Method:    MemoryCard_AT24C32_Write
 * Signature: (II[B)I
 */
JNIEXPORT jint JNICALL Java_com_hxsmart_imateinterface_memorycardapi_MemoryCard_AT24C32_1Write
  (JNIEnv *env, jobject this, jint offset, jint dataLen, jbyteArray dataBuff)
{
	if (dataBuff == NULL ||  (*env)->GetArrayLength(env, dataBuff) < dataLen)
		return 100;

	uchar *sDataBuff = malloc(dataLen);
	char *data = (*env)->GetByteArrayElements(env, dataBuff, 0);
	if (data) {
		memcpy(sDataBuff, data, dataLen);
		(*env)->ReleaseByteArrayElements(env, dataBuff, data, 0);
	}
	int iRet = MemoryCard_AT24C32_Write(offset, dataLen, sDataBuff);
	free(sDataBuff);

	return iRet;
}

JNIEXPORT jint JNICALL Java_com_hxsmart_imateinterface_memorycardapi_MemoryCard_GenCommKey
  (JNIEnv *env, jobject this, jint masterKeyId, jbyteArray random)
{
	if (random == NULL ||  (*env)->GetArrayLength(env, random) < 8)
		return 100;

	uchar sRandom[8];
	char *data = (*env)->GetByteArrayElements(env, random, 0);
	if (data) {
		memcpy(sRandom, data, 8);
		(*env)->ReleaseByteArrayElements(env, random, data, 0);
	}
	int iRet = MemoryCard_GenCommKey(masterKeyId, sRandom);

	return iRet;
}