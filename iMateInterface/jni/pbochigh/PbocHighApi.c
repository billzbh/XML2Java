#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <jni.h>
#include <android/log.h>

#include "RemoteFunctions.h"
#include "PbocHigh.h"
#include "iMateExt.h"

static JavaVM *sg_vm = NULL;
static JNIEnv *sg_env;
static jclass  sg_class = NULL;
static jobject sg_this;
static jmethodID sendReceiveCallBack;

static void vSetIntegerValue(JNIEnv *env, jobject obj, int value);
static void vSetFieldString(JNIEnv *env, jobject obj, const char *szParamName, const char *szValue);
static void vSetFieldInt(JNIEnv *env, jobject obj, const char *szParamName, int iValue);

/*
static void vPrintAndroidLog(char *szLogStr)
{
	__android_log_print(ANDROID_LOG_DEBUG,"HxSmart_PbocHight",szLogStr);
}
*/

//static jstring stoJstring(JNIEnv* env, char* pat);
/*
* Register several native methods for one class.
*/
static int registerNativeMethods(JNIEnv* env, const char* className,
                                 JNINativeMethod* gMethods, int numMethods)
{
    jclass clazz=(*env)->FindClass(env, className);
    if (clazz == NULL)
    {
        return JNI_FALSE;
    }
    jmethodID constr = (*env)->GetMethodID(env, clazz, "<init>", "()V");
    if(!constr) {
        return JNI_FALSE;
    }
    jobject obj = (*env)->NewObject(env, clazz, constr);
    sg_this = (*env)->NewGlobalRef(env, obj);
    if ((*env)->RegisterNatives(env, clazz, gMethods, numMethods) < 0)
    {
        return JNI_FALSE;
    }

    sg_class = clazz;
    return JNI_TRUE;
}

/*
* Register native methods for all classes we know about.
*/
#define JNIREG_CLASS "com/hxsmart/imateinterface/pbochighsdk/PbocHighApi"//指定要注册的类
static JNINativeMethod gMethods[] = {  //添加要注册的方法

};
static int registerNatives(JNIEnv* env)
{
    if (!registerNativeMethods(env, JNIREG_CLASS, gMethods, sizeof(gMethods) / sizeof(gMethods[0])))
        return JNI_FALSE;
    return JNI_TRUE;
}

jint JNICALL JNI_OnLoad(JavaVM* vm, void* reserved)
{
	JNIEnv* env = NULL;
    sg_vm = 0;

    if ((*vm)->GetEnv(vm, (void**)&env, JNI_VERSION_1_4) != JNI_OK)
    {
        return -1;
    }
    sg_env = env;
    if (!registerNatives(sg_env))
    {
        return -1;
    }
    sg_vm = vm;
    sendReceiveCallBack = (*sg_env)->GetMethodID(sg_env, sg_class, "sendReceiveCallBack", "([BI[BI)I");

    iHxEmvInit(iIMateTestCard,iIMateResetCard,iIMateExchangeApdu,iIMateCloseCard);

    return JNI_VERSION_1_4;
}

/*
 * Class:     com_hxsmart_imateinterface_pbochighsdk_PbocHighApi
 * Method:    iHxPbocHighInitCore
 * Signature: (Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;II)I
 */
JNIEXPORT jint JNICALL Java_com_hxsmart_imateinterface_pbochighsdk_PbocHighApi_iHxPbocHighInitCore
  (JNIEnv *env, jobject thiz, jstring merchantId, jstring termId, jstring merchantName, jint countryCode, jint currencyCode)
{

	char szMerchantId[15+1], szTermId[8+1], szMerchantName[40+1];

	sg_env = env;
	sg_this = thiz;

	memset(szMerchantId, 0, sizeof(szMerchantId));
	memset(szTermId, 0, sizeof(szTermId));
	memset(szMerchantName, 0, sizeof(szMerchantName));

	if (merchantId == NULL || (*env)->GetStringLength(env, merchantId) > 15)
		return HXPBOC_HIGH_PARA;
	if (termId == NULL || (*env)->GetStringLength(env, termId) > 8)
		return HXPBOC_HIGH_PARA;
	if (merchantName == NULL || (*env)->GetStringLength(env, merchantName) > 40)
		return HXPBOC_HIGH_PARA;

	const char *value = (*env)->GetStringUTFChars(env, merchantId, 0);//转换成 char *
	if (value != NULL) {
		strcpy(szMerchantId, value);
		(*env)->ReleaseStringUTFChars(env, merchantId,value); //释放引用
	}
	value = (*env)->GetStringUTFChars(env, termId ,0);//转换成 char *
	if (value != NULL) {
		strcpy(szTermId, value);
		(*env)->ReleaseStringUTFChars(env, termId,value); //释放引用
	}
	value = (*env)->GetStringUTFChars(env, merchantName ,0);//转换成 char *
	if (value != NULL) {
		strcpy(szMerchantName, value);
		(*env)->ReleaseStringUTFChars(env, merchantName,value); //释放引用
	}

	//set trace to android log
	//iHxEmvTraceSet(vPrintAndroidLog);

	return iHxPbocHighInitCore(szMerchantId, szTermId, szMerchantName, countryCode, currencyCode);
}

/*
 * Class:     com_hxsmart_imateinterface_pbochighsdk_viHxPbocHighSetCardReaderType
 * Method:    viHxPbocHighSetCardReaderType
 */
JNIEXPORT void JNICALL Java_com_hxsmart_imateinterface_pbochighsdk_PbocHighApi_vHxPbocHighSetCardReaderType
  (JNIEnv *env, jobject thiz, jint cardReaderType)
{
	sg_env = env;
	sg_this = thiz;

	vSetCardReaderType(cardReaderType);
}

/*
 * Class:     com_hxsmart_imateinterface_pbochighsdk_PbocHighApi
 * Method:    iHxPbocHighInitTrans
 * Signature: (Ljava/lang/String;IIILcom/hxsmart/imateinterface/pbochighsdk/PbocCardData;)I
 */
JNIEXPORT jint JNICALL Java_com_hxsmart_imateinterface_pbochighsdk_PbocHighApi_iHxPbocHighInitTrans
  (JNIEnv *env, jobject thiz, jstring dateTime, jint atc, jint transType, jint amount, jobject cardData)
{
	char szDateTime[14+1], szField55[512+1], szPan[19+1], szTrack2[40], szExtInfo[513];
	unsigned long ulAtc;
	unsigned char ucTransType, szAmount[12+1];
	int iPanSeqNo;

	sg_env = env;
	sg_this = thiz;

	memset(szField55, 0, sizeof(szField55));
	memset(szPan, 0, sizeof(szPan));
	memset(szTrack2, 0, sizeof(szTrack2));
	memset(szExtInfo, 0, sizeof(szExtInfo));

	if (dateTime == NULL || (*env)->GetStringLength(env, dateTime) != 14)
		return HXPBOC_HIGH_PARA;
	if (amount < 0)
		return HXPBOC_HIGH_PARA;
	if (transType < 0 || transType > 256)
		return HXPBOC_HIGH_PARA;

	if (cardData == NULL)
		return HXPBOC_HIGH_PARA;

	const char *value = (*env)->GetStringUTFChars(env, dateTime ,0);//转换成 char *
	if (value != NULL) {
		strcpy(szDateTime, value);
		(*env)->ReleaseStringUTFChars(env, dateTime,value); //释放引用
	}
	ulAtc = (unsigned long)atc;
	ucTransType = (unsigned char)transType;
	sprintf(szAmount, "%d", (unsigned int)amount);

	int iRet = iHxPbocHighInitTrans(szDateTime, ulAtc, ucTransType, szAmount,
							 szField55, szPan, &iPanSeqNo, szTrack2, szExtInfo);
	if (iRet == 0) {
		vSetFieldString(env, cardData, "field55", szField55);
		vSetFieldString(env, cardData, "pan", szPan);
		vSetFieldInt(env, cardData, "panSeqNo", iPanSeqNo);
		vSetFieldString(env, cardData, "track2", szTrack2);
		vSetFieldString(env, cardData, "extInfo", szExtInfo);
	}
	return iRet;
}

/*
 * Class:     com_hxsmart_imateinterface_pbochighsdk_PbocHighApi
 * Method:    iHxPbocHighDoTrans
 * Signature: (Ljava/lang/String;[BLjava/lang/Integer;)I
 */
JNIEXPORT jint JNICALL Java_com_hxsmart_imateinterface_pbochighsdk_PbocHighApi_iHxPbocHighDoTrans
  (JNIEnv *env, jobject thiz, jstring issuerData, jbyteArray field55, jobject dataLen)
{
	char szIssuerData[513], szField55[513];

	sg_env = env;
	sg_this = thiz;

	memset(szIssuerData, 0, sizeof(szIssuerData));
	memset(szField55, 0, sizeof(szField55));

	if (issuerData == NULL || field55 == NULL || dataLen == NULL)
		return HXPBOC_HIGH_PARA;

	const char *value = (*env)->GetStringUTFChars(env, issuerData ,0);//转换成 char *
	if (value != NULL) {
		strcpy(szIssuerData, value);
		(*env)->ReleaseStringUTFChars(env, issuerData,value); //释放引用
	}
	int iRet = iHxPbocHighDoTrans(szIssuerData, szField55);
	if (iRet == 0 || iRet == HXPBOC_HIGH_DENIAL) {
		int length = strlen(szField55);
		if ((*env)->GetArrayLength(env, field55) < length)
			return HXPBOC_HIGH_PARA;
		(*env)->SetByteArrayRegion(env, field55, 0, length, szField55);
		vSetIntegerValue(env, dataLen, length);
	}
	return iRet;
}

JNIEXPORT jint JNICALL Java_com_hxsmart_imateinterface_pbochighsdk_PbocHighApi_iHxPbocHighReadInfoEx
(JNIEnv *env, jobject thiz, jbyteArray outData,jint infoType)
{
	char szOutData[1024];  //c 缓冲区
	sg_env = env;
	sg_this = thiz;

	int iRet = iHxPbocHighReadInfoEx(szOutData,infoType);
	if(iRet == 0)
	{
		int len = strlen(szOutData);
		__android_log_print(ANDROID_LOG_DEBUG,"hxsmart","信息的字节长度： %d \n",len );

		(*env)->SetByteArrayRegion(sg_env,outData,0,len,szOutData); // 将szOutData赋值到outData引用的内存区域。
	}
	return iRet;
}

JNIEXPORT jstring JNICALL Java_com_hxsmart_imateinterface_pbochighsdk_PbocHighApi_szHxPbocHighGetTagValue
(JNIEnv *env, jobject thiz, jstring tagString)
{
	char szTag[10];
	memset(szTag, 0, sizeof(szTag));

	sg_env = env;
	sg_this = thiz;

	const char *value = (*env)->GetStringUTFChars(env, tagString, 0);//转换成 char *
	if (value == NULL)
		return NULL;
	strcpy(szTag, value);
	(*env)->ReleaseStringUTFChars(env, tagString,value); //释放引用

	//vSetWriteLog(1);
	vWriteLogTxt("tag = [%s]", szTag);
	char *retValue =  szHxPbocHighGetTagValue(szTag);
	vWriteLogTxt("retValue = [%s]", retValue);

	return  (*env)->NewStringUTF(env, retValue);
}

int syncCommon(uchar *sIn, int iInLen, uchar *sOut, int *piOutLen, int timeout)
{
	int retCode;

	jbyteArray dataIn = (*sg_env)->NewByteArray(sg_env,iInLen);
	(*sg_env)->SetByteArrayRegion(sg_env,dataIn, 0, (jint)iInLen, sIn);

	jbyteArray dataOut = (*sg_env)->NewByteArray(sg_env,2048);

	retCode = 1;
    int ret = (int) (*sg_env)->CallIntMethod(sg_env, sg_this,
    		sendReceiveCallBack, dataIn, iInLen, dataOut, timeout);

    if (ret > 0) {
		uchar *bytes = (*sg_env)->GetByteArrayElements(sg_env, dataOut, 0);
		memcpy(sOut, bytes, ret);
		*piOutLen = ret;
		(*sg_env)->ReleaseByteArrayElements(sg_env, dataOut, (jbyte *) bytes, 0);
		retCode = 0;
    }
	(*sg_env)->DeleteLocalRef(sg_env, dataIn);
	(*sg_env)->DeleteLocalRef(sg_env, dataOut);

	return retCode;
}

/*
 * Local Functions
 */
static void vSetIntegerValue(JNIEnv *env, jobject obj, int value)
{
	jclass objclass = (*env)->FindClass(env, "java/lang/Integer");
	jfieldID fieldId =  (*env)->GetFieldID(env, objclass , "value", "I");
	(*env)->SetIntField(env, obj, fieldId, value);
	(*env)->DeleteLocalRef(env, objclass);
}

/*
//char* to jstring
static jstring stoJstring(JNIEnv* env, char* pat)
{
   jclass strClass = (*env)->FindClass(env, "Ljava/lang/String;");
   jmethodID ctorID = (*env)->GetMethodID(env, strClass, "<init>", "([BLjava/lang/String;)V");
   jbyteArray bytes = (*env)->NewByteArray(env, strlen(pat));
   (*env)->SetByteArrayRegion(env, bytes, 0, strlen(pat), (jbyte*)pat);
   jstring encoding = (*env)->NewStringUTF(env, "utf-8");
   return (jstring)(*env)->NewObject(env, strClass, ctorID, bytes, encoding);
}
*/
static void vSetFieldString(JNIEnv *env, jobject obj, const char *szParamName, const char *szValue)
{
	if (obj == NULL)
		return;

    jclass objclass = (*env)->GetObjectClass(env, obj);
	jfieldID fieldId =  (*env)->GetFieldID(env, objclass , szParamName , "Ljava/lang/String;"); //获得属性句柄
	if (fieldId == NULL)
		return;
	if (szValue == NULL)
		szValue = "";
	jstring valueString =  (*env)->NewStringUTF(env, szValue); //构造一个jstring对象
	(*env)->SetObjectField(env, obj , fieldId , valueString); // 设置该字段的值
	(*env)->DeleteLocalRef(env, valueString);
	(*env)->DeleteLocalRef(env, objclass);
}

static void vSetFieldInt(JNIEnv *env, jobject obj, const char *szParamName, int iValue)
{
	if (obj == NULL)
		return;
    jclass objclass = (*env)->GetObjectClass(env, obj);
	jfieldID fieldId =  (*env)->GetFieldID(env, objclass , szParamName , "I"); //获得属性句柄
	(*env)->SetIntField(env, obj, fieldId, iValue);
	(*env)->DeleteLocalRef(env, objclass);
}



