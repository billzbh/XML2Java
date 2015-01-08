#include <stdlib.h>
#include <stdio.h>
#include <SyncCommon.h>
#include <android/log.h>

JNIEnv *sg_syncCommonEnv = 0;
JavaVM* sg_syncCommonVm = 0;

void syncCommonSetEnv(JNIEnv *env)
{
	sg_syncCommonEnv = env;
}

void syncCommonSetVM(JavaVM* vm)
{
	sg_syncCommonVm = vm;
}

int syncCommon(unsigned char *sIn, int iInLen, unsigned char *sOut, int *piOutLen, int timeout)
{
	int retCode;

	JNIEnv *env;

	if (sg_syncCommonEnv == 0 && sg_syncCommonVm == 0)
		return -100;

	env = sg_syncCommonEnv;
	if (sg_syncCommonVm) {
		int getEnvStat = (*sg_syncCommonVm)->GetEnv(sg_syncCommonVm, (void**)&env, JNI_VERSION_1_4);
		if (getEnvStat != JNI_OK) {
			if (getEnvStat == JNI_EDETACHED) {
				__android_log_write(ANDROID_LOG_DEBUG,"HxSmart_Jni","GetEnv: not attached");
				if ((*sg_syncCommonVm)->AttachCurrentThread(sg_syncCommonVm, (JNIEnv**)&env, 0) != 0) {
					__android_log_write(ANDROID_LOG_DEBUG,"HxSmart_Jni","Failed to attach");
					return -101;
				}
			} else if (getEnvStat == JNI_EVERSION) {
				__android_log_write(ANDROID_LOG_DEBUG,"HxSmart_Jni","GetEnv: version not supported");
				return -102;
			}
			__android_log_write(ANDROID_LOG_DEBUG,"HxSmart_Jni","GetEnv: other error");
			return -103;
		}
	}
	jbyteArray dataIn = (*env)->NewByteArray(env,iInLen);
	(*env)->SetByteArrayRegion(env,dataIn, 0, (jint)iInLen, sIn);

	jbyteArray dataOut = (*env)->NewByteArray(env, 4096);

	jclass objclass = (*env)->FindClass(env, "com/hxsmart/imateinterface/BluetoothThread");
	jmethodID sendReceiveCallBack = (*env)->GetMethodID(env, objclass, "sendReceive", "([BI[BI)I");

	// 获得THIS对象
	jfieldID fieldId =  (*env)->GetStaticFieldID(env, objclass , "THIS", "Lcom/hxsmart/imateinterface/BluetoothThread;");
	jobject bluetoothObj = (*env)->GetStaticObjectField(env, objclass , fieldId);

	retCode = 1;
	int ret = (int) (*env)->CallIntMethod(env, bluetoothObj, sendReceiveCallBack, dataIn, iInLen, dataOut, timeout);

    if (ret > 0) {
    	if (sOut) {
			unsigned char *bytes = (*env)->GetByteArrayElements(env, dataOut, 0);
			memcpy(sOut, bytes, ret);
			(*env)->ReleaseByteArrayElements(env, dataOut, (jbyte *)bytes, 0);
    	}
    	if (piOutLen)
    		*piOutLen = ret;

		retCode = 0;
    }
	(*env)->DeleteLocalRef(env, dataIn);
	(*env)->DeleteLocalRef(env, dataOut);
	(*env)->DeleteLocalRef(env, objclass);
	(*env)->DeleteLocalRef(env, bluetoothObj);

	return retCode;
}
