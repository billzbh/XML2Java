#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <jni.h>
#include <android/log.h>

#include "RemoteFunctions.h"
#include "JsbPboc.h"
#include "iMateExt.h"


static JavaVM *sg_vm = NULL;
static JNIEnv *sg_env;
static jclass  sg_class = NULL;
static jobject sg_this;
static jmethodID sendReceiveCallBack;

static void vSetIntegerValue(JNIEnv *env, jobject obj, int value);

/*
* Register several native methods for one class.
*/
static int registerNativeMethods(JNIEnv* env, const char* className,
                                 JNINativeMethod* gMethods, int numMethods)
{
    jclass clazz=(*env)->FindClass(env, className);
    if (clazz == NULL)
    {
        //__android_log_print(ANDROID_LOG_FATAL,"Android", "Native registration unable to find class '%s'", className);
        return JNI_FALSE;
    }
    jmethodID constr = (*env)->GetMethodID(env, clazz, "<init>", "()V");
    if(!constr) {
        //__android_log_print(ANDROID_LOG_FATAL,"Android", "Native registration unable to find  constructor for class '%s'", className);
        return JNI_FALSE;;
    }
    jobject obj = (*env)->NewObject(env, clazz, constr);
    sg_this = (*env)->NewGlobalRef(env, obj);
    if ((*env)->RegisterNatives(env, clazz, gMethods, numMethods) < 0)
    {
        //__android_log_print(ANDROID_LOG_FATAL,"Android", "RegisterNatives failed for '%s'", className);
        return JNI_FALSE;
    }

    sg_class = clazz;
    return JNI_TRUE;
}

/*
* Register native methods for all classes we know about.
*/
#define JNIREG_CLASS "com/hxsmart/imateinterface/jsbpbocsdk/JsbPbocApi"//指定要注册的类
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
        //__android_log_print(ANDROID_LOG_FATAL,"Android","GetEnv failed");
        return -1;
    }
    sg_env = env;
    if (!registerNatives(sg_env))
    {
        //__android_log_print(ANDROID_LOG_FATAL, "Android", "registerNatives failed");
        return -1;
    }
    sg_vm = vm;
    sendReceiveCallBack = (*sg_env)->GetMethodID(sg_env, sg_class, "sendReceiveCallBack", "([BI[BI)I");

    ICC_InitEnv(iIMateTestCard,iIMateResetCard,iIMateExchangeApdu,iIMateCloseCard);

    return JNI_VERSION_1_4;
}

/*
 * Class:     com_hxsmart_imateinterface_jsbpboc_JsbPbocAPi
 * Method:    ICC_GenARQC
 * Signature: (Ljava/lang/String;ILjava/lang/String;ILjava/lang/String;Ljava/lang/String;[BLjava/lang/Integer;)I
 */
JNIEXPORT jint JNICALL Java_com_hxsmart_imateinterface_jsbpbocsdk_JsbPbocApi_ICC_1GenARQC(JNIEnv *env, jobject thiz, jstring comNo, jint termType, jchar bpNo, jint icFlag, jstring txData, jstring appData, jbyteArray arqc, jobject arqcLen){
	////__android_log_print(ANDROID_LOG_DEBUG,"JsbPboc","Test");

	char *sComNo = NULL;
	char *sTxData = NULL;
	char *sAppData = NULL;
	const char *value;
	if(comNo != NULL){
		value = (*env)->GetStringUTFChars(env, comNo ,0);//转换成 char *
		if (value != NULL) {
			sComNo = malloc(strlen(value) + 1);
			strcpy(sComNo, value);
			(*env)->ReleaseStringUTFChars(env, comNo,value); //释放引用
		}
	}

	if(txData != NULL){
		value = (*env)->GetStringUTFChars(env, txData ,0);//转换成 char *
		if (value != NULL) {
			sTxData = malloc(strlen(value) + 1);
			strcpy(sTxData, value);
			(*env)->ReleaseStringUTFChars(env, txData,value); //释放引用
		}
	}

	if(appData != NULL){
		value = (*env)->GetStringUTFChars(env, appData ,0);//转换成 char *
		if (value != NULL) {
			sAppData = malloc(strlen(value) + 1);
			strcpy(sAppData, value);
			(*env)->ReleaseStringUTFChars(env, appData,value); //释放引用
		}
	}

	if(arqc == NULL || arqcLen == NULL){
		if(sComNo){
			free(sComNo);
		}

		if(sTxData){
			free(sTxData);
		}

		if(sAppData){
			free(sAppData);
		}
		return -8;
	}

	int mallocLength =  (*env)->GetArrayLength(env, arqc);
	if (mallocLength == 0){
		if(sComNo){
			free(sComNo);
		}

		if(sTxData){
			free(sTxData);
		}

		if(sAppData){
			free(sAppData);
		}
		return -8;
	}

	int iArqcLen = 0;
	char* sArqc = malloc(mallocLength);
	int iRet = ICC_GenARQC(sComNo, termType, bpNo, icFlag, sTxData, sAppData, &iArqcLen, sArqc);
	if(sComNo){
		free(sComNo);
	}

	if(sTxData){
		free(sTxData);
	}

	if(sAppData){
		free(sAppData);
	}

	if(iRet == 0){
		(*env)->SetByteArrayRegion(env, arqc, 0, iArqcLen, sArqc);
		vSetIntegerValue(env, arqcLen, iArqcLen);
	}
	free(sArqc);
	return iRet;
}

/*
 * Class:     com_hxsmart_imateinterface_jsbpboc_JsbPbocAPi
 * Method:    ICC_GetIcInfo
 * Signature: (Ljava/lang/String;ILjava/lang/String;ILjava/lang/String;[BLjava/lang/Integer;[BLjava/lang/Integer;)I
 */
JNIEXPORT jint JNICALL Java_com_hxsmart_imateinterface_jsbpbocsdk_JsbPbocApi_ICC_1GetIcInfo
  (JNIEnv *env, jobject thiz, jstring comNo, jint termType, jchar bpNo, jint icFlag, jstring tagList, jbyteArray userInfo, jobject userInfoLen, jbyteArray appData, jobject appDataLen, jobject icType){
	char *sComNo = NULL;
	char *sTagList = NULL;
	const char *value;
	if(comNo != NULL){
		value = (*env)->GetStringUTFChars(env, comNo ,0);//转换成 char *
		if (value != NULL) {
			sComNo = malloc(strlen(value) + 1);
			strcpy(sComNo, value);
			(*env)->ReleaseStringUTFChars(env, comNo,value); //释放引用
		}
	}

	if(tagList != NULL){
		value = (*env)->GetStringUTFChars(env, tagList ,0);//转换成 char *
		if (value != NULL) {
			sTagList = malloc(strlen(value) + 1);
			strcpy(sTagList, value);
			(*env)->ReleaseStringUTFChars(env, tagList,value); //释放引用
		}
	}

	if(userInfo == NULL || userInfoLen == NULL || appData == NULL || appDataLen == NULL || icType == NULL){
		if(sComNo) {
			free(sComNo);
		}

		if(sTagList){
			free(sTagList);
		}
		return -8;
	}

	int mallocLength =  (*env)->GetArrayLength(env, userInfo);
	if (mallocLength == 0){
		if(sComNo){
			free(sComNo);
		}

		if(sTagList){
			free(sTagList);
		}
		return -8;
	}

	int iUserInfoLen = 0;
	char* sUserInfo = malloc(mallocLength);

	mallocLength = (*env)->GetArrayLength(env, appData);
	if (mallocLength == 0){
		if(sComNo){
			free(sComNo);
		}

		if(sTagList){
			free(sTagList);
		}
		free(sUserInfo);
		return -8;
	}
	char* sAppData = malloc(mallocLength);

	int iIcType = 0;

	int iRet = ICC_GetIcInfo(sComNo, termType, bpNo, icFlag, sTagList, &iUserInfoLen, sUserInfo, sAppData, &iIcType);
	if(sComNo != NULL){
		free(sComNo);
	}

	if(sTagList != NULL){
		free(sTagList);
	}

	if(iRet == 0){
		(*env)->SetByteArrayRegion(env, userInfo, 0, iUserInfoLen, sUserInfo);
		(*env)->SetByteArrayRegion(env, appData, 0, strlen(sAppData), sAppData);
		vSetIntegerValue(env, userInfoLen, iUserInfoLen);
		vSetIntegerValue(env, appDataLen, strlen(sAppData));
		vSetIntegerValue(env, icType, iIcType);
	}

	free(sUserInfo);
	free(sAppData);

	return iRet;
}

JNIEXPORT jstring JNICALL Java_com_hxsmart_imateinterface_jsbpbocsdk_JsbPbocApi_ICC_1GetPbocVersion
  (JNIEnv *env, jobject thiz) {
	sg_env = env;
	sg_this = thiz;

	char szTagValue[10];
	memset(szTagValue, 0, sizeof(szTagValue));

	int ret = ICC_GetIcPbocVersion(szTagValue);
	if (ret)
		return NULL;

	return  (*env)->NewStringUTF(env, szTagValue);
}

/*
 * Class:     com_hxsmart_imateinterface_jsbpboc_JsbPbocAPi
 * Method:    ICC_GetTranDetail
 * Signature: (Ljava/lang/String;ILjava/lang/String;I[BLjava/lang/Integer;)I
 */
JNIEXPORT jint JNICALL Java_com_hxsmart_imateinterface_jsbpbocsdk_JsbPbocApi_ICC_1GetTranDetail
  (JNIEnv *env, jobject thiz, jstring comNo, jint termType, jchar bpNo, jint icFlag, jbyteArray txDetail, jobject txDetailLen){
	char *sComNo = NULL;
	const char *value;
	if(comNo != NULL){
		value = (*env)->GetStringUTFChars(env, comNo ,0);//转换成 char *
		if (value != NULL) {
			sComNo = malloc(strlen(value) + 1);
			strcpy(sComNo, value);
			(*env)->ReleaseStringUTFChars(env, comNo,value); //释放引用
		}
	}

	if(txDetail == NULL || txDetailLen == NULL){
		if(sComNo){
			free(sComNo);
		}
		return -8;
	}

	int mallocLength =  (*env)->GetArrayLength(env, txDetail);
	if (mallocLength == 0){
		if(sComNo){
			free(sComNo);
		}
		return -8;
	}

	int iTxDetailLen = 0;
	char* sTxDetail = malloc(mallocLength);

	int iRet = ICC_GetTranDetail(sComNo, termType, bpNo, icFlag, &iTxDetailLen, sTxDetail);
	if(sComNo){
		free(sComNo);
	}

	if(iRet == 0){
		(*env)->SetByteArrayRegion(env, txDetail, 0, iTxDetailLen, sTxDetail);
		vSetIntegerValue(env, txDetailLen, iTxDetailLen);
	}

	free(sTxDetail);
	return iRet;
}

/*
 * Class:     com_hxsmart_imateinterface_jsbpboc_JsbPbocAPi
 * Method:    ICC_CtlScriptData
 * Signature: (Ljava/lang/String;ILjava/lang/String;ILjava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/Integer;[B[B)I
 */
JNIEXPORT jint JNICALL Java_com_hxsmart_imateinterface_jsbpbocsdk_JsbPbocApi_ICC_1CtlScriptData
  (JNIEnv *env, jobject thiz, jstring comNo, jint termType, jchar bpNo, jint icFlag, jstring txData, jstring appData, jstring arpc, jbyteArray tc, jobject tcLen, jbyteArray scriptResult, jobject scriptResultLen){
	char *sComNo = NULL;
	char *sTxData = NULL;
	char *sAppData = NULL;
	char *sArpc = NULL;
	const char *value;
	if(comNo != NULL){
		value = (*env)->GetStringUTFChars(env, comNo ,0);//转换成 char *
		if (value != NULL) {
			sComNo = malloc(strlen(value) + 1);
			strcpy(sComNo, value);
			(*env)->ReleaseStringUTFChars(env, comNo,value); //释放引用
		}
	}

	if(txData != NULL){
		value = (*env)->GetStringUTFChars(env, txData ,0);//转换成 char *
		if (value != NULL) {
			sTxData = malloc(strlen(value) + 1);
			strcpy(sTxData, value);
			(*env)->ReleaseStringUTFChars(env, txData,value); //释放引用
		}
	}

	if(appData != NULL){
		value = (*env)->GetStringUTFChars(env, appData ,0);//转换成 char *
		if (value != NULL) {
			sAppData = malloc(strlen(value) + 1);
			strcpy(sAppData, value);
			(*env)->ReleaseStringUTFChars(env, appData,value); //释放引用
		}
	}

	if(arpc != NULL){
		value = (*env)->GetStringUTFChars(env, arpc ,0);//转换成 char *
		if (value != NULL) {
			sArpc = malloc(strlen(value) + 1);
			strcpy(sArpc, value);
			(*env)->ReleaseStringUTFChars(env, arpc,value); //释放引用
		}
	}

	if(tc == NULL || tcLen == NULL || scriptResult == NULL || scriptResultLen == NULL ){
		if(sComNo){
			free(sComNo);
		}

		if(sTxData){
			free(sTxData);
		}

		if(sAppData){
			free(sAppData);
		}

		if(sArpc){
			free(sArpc);
		}
		return -8;
	}

	int mallocLength =  (*env)->GetArrayLength(env, tc);
	if (mallocLength == 0){
		if(sComNo){
			free(sComNo);
		}

		if(sTxData){
			free(sTxData);
		}

		if(sAppData){
			free(sAppData);
		}

		if(sArpc){
			free(sArpc);
		}
		return -8;
	}

	int iTcLen = 0;
	char* sTc = malloc(mallocLength);

	mallocLength =  (*env)->GetArrayLength(env, scriptResult);
	if (mallocLength == 0){
		if(sComNo){
			free(sComNo);
		}

		if(sTxData){
			free(sTxData);
		}

		if(sAppData){
			free(sAppData);
		}

		if(sArpc){
			free(sArpc);
		}
		free(sTc);
		return -8;
	}

	char* sScriptResult = malloc(mallocLength);

	int iRet = ICC_CtlScriptData(sComNo, termType, bpNo, icFlag, sTxData, sAppData, sArpc, &iTcLen, sTc, sScriptResult);
	if(sComNo){
		free(sComNo);
	}

	if(sTxData){
		free(sTxData);
	}

	if(sAppData){
		free(sAppData);
	}

	if(sArpc){
		free(sArpc);
	}

	if(iRet == 0){
		(*env)->SetByteArrayRegion(env, tc, 0, iTcLen, sArpc);
		vSetIntegerValue(env, tcLen, iTcLen);

		(*env)->SetByteArrayRegion(env, scriptResult, 0, strlen(sScriptResult), sScriptResult);
		vSetIntegerValue(env, scriptResultLen, strlen(sScriptResult));
	}

	free(sTc);
	free(sScriptResult);

	return iRet;
}

extern int sg_iRet;
JNIEXPORT jint JNICALL Java_com_hxsmart_imateinterface_jsbpbocsdk_JsbPbocApi_ICC_1GetCoreRetCode(JNIEnv *env, jobject thiz)
{
	return sg_iRet;
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

static void vSetIntegerValue(JNIEnv *env, jobject obj, int value)
{
	jclass objclass = (*env)->FindClass(env, "java/lang/Integer");
	jfieldID fieldId =  (*env)->GetFieldID(env, objclass , "value", "I");
	(*env)->SetIntField(env, obj, fieldId, value);
	(*sg_env)->DeleteLocalRef(env, objclass);
}
