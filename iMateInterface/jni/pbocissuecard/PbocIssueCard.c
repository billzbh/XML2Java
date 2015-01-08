#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <jni.h>
#include <android/log.h>

#include "unsigned.h"
#include "IssFace.h"
#include "RemoteFunctions.h"
#include "iMateExt.h"


static JNIEnv *sg_env;
static jobject sg_this;

static jmethodID sendReceiveCallBack;
static jmethodID pbocResetCallBack;
static jmethodID showStatusCallBack;

//获取类中每一个变量的定义


static jfieldID appVerId;					// 0x20,0x30分别表示2.0或3.0的应用
static jfieldID kmcIndexId;
static jfieldID appKeyBaseIndexId;
static jfieldID issuerRsaKeyIndexId;

static jfieldID panStringId;
static jfieldID panSerialNoId;;
static jfieldID expireDateStringId;
static jfieldID holderNameStringId;
static jfieldID holderIdTypeId;
static jfieldID holderIdStringId;
static jfieldID defaultPinStringId;
static jfieldID aidStringId;
static jfieldID labelStringId;

static jfieldID caIndexId;
static jfieldID icRsaKeyLenId;
static jfieldID icRsaEId;
static jfieldID countryCodeId;
static jfieldID currencyCodeId;
static jfieldID errorStringId;

static void vShowStatus(char *pszStatus);
static jstring stoJstring(JNIEnv* env, const char* pat);

int Java_com_hxsmart_imateinterface_pbocissuecard_PbocIssueCard_issSetEnv( JNIEnv *env,
						       jobject thiz )
{
	char szErrMsg[50];

	sg_env = env;
	sg_this = thiz;

	jclass objectClass = (*env)->GetObjectClass(env, thiz);

    sendReceiveCallBack = (*env)->GetMethodID(env, objectClass, "sendReceiveCallBack", "([BI[BI)I");
    if (sendReceiveCallBack == 0)
    	return 1;

    pbocResetCallBack = (*env)->GetMethodID(env, objectClass, "pbocResetCallBack", "([BI)I");
    if (pbocResetCallBack == 0)
    	return 3;

    showStatusCallBack = (*env)->GetMethodID(env, objectClass, "showStatusCallBack", "(Ljava/lang/String;)V");
    if (showStatusCallBack == 0)
    	return 4;

    //获取类中每一个变量的定义
	appVerId = (*env)->GetFieldID(env, objectClass,"appVer","I");
	kmcIndexId = (*env)->GetFieldID(env, objectClass,"kmcIndex","I");
	appKeyBaseIndexId = (*env)->GetFieldID(env, objectClass,"appKeyBaseIndex","I");
	issuerRsaKeyIndexId = (*env)->GetFieldID(env, objectClass,"issuerRsaKeyIndex","I");

    panStringId = (*env)->GetFieldID(env, objectClass,"panString","Ljava/lang/String;");
    panSerialNoId = (*env)->GetFieldID(env, objectClass,"panSerialNo","I");
    expireDateStringId = (*env)->GetFieldID(env, objectClass,"expireDateString","Ljava/lang/String;");
    holderNameStringId = (*env)->GetFieldID(env, objectClass,"holderNameString","Ljava/lang/String;");
    holderIdTypeId = (*env)->GetFieldID(env, objectClass,"holderIdType","I");
    holderIdStringId = (*env)->GetFieldID(env, objectClass,"holderIdString","Ljava/lang/String;");
    defaultPinStringId = (*env)->GetFieldID(env, objectClass,"defaultPinString","Ljava/lang/String;");
    aidStringId = (*env)->GetFieldID(env, objectClass,"aidString","Ljava/lang/String;");
    labelStringId = (*env)->GetFieldID(env, objectClass,"labelString","Ljava/lang/String;");

    caIndexId = (*env)->GetFieldID(env, objectClass,"caIndex","I");
    icRsaKeyLenId = (*env)->GetFieldID(env, objectClass,"icRsaKeyLen","I");
    icRsaEId = (*env)->GetFieldID(env, objectClass,"icRsaE","I");
    countryCodeId = (*env)->GetFieldID(env, objectClass,"countryCode","I");
    currencyCodeId = (*env)->GetFieldID(env, objectClass,"currencyCode","I");
    errorStringId = (*env)->GetFieldID(env, objectClass,"errorString","Ljava/lang/String;");

    iIssSetCardCtrlFunc(iIMateTestCard,iIMateResetCard,iIMateExchangeApdu,iIMateCloseCard);

	if (iIssSetEnv(0, NULL, 0, 0, szErrMsg))
		return 5;

	iIssSetStatusShowFunc(vShowStatus);

	return 0;
}

int Java_com_hxsmart_imateinterface_pbocissuecard_PbocIssueCard_issueCard(JNIEnv *env,
						       jobject thiz, jint timeout )
{
	char szErrMsg[50];
	uchar sResetCard[80];
	stIssData issDataStru;

	sg_env = env;
	sg_this = thiz;

	if (iWaitCard(env, 0, sResetCard, timeout) == 0) {
    	(*env)->SetObjectField(env, thiz, errorStringId, (*env)->NewStringUTF(env, "未检测到卡片或卡片复位失败"));
    	return 1;
	}

    memset(&issDataStru,0,sizeof(issDataStru));

    issDataStru.iAppVer = (*env)->GetIntField(env, thiz, appVerId);
    issDataStru.iKmcIndex = (*env)->GetIntField(env, thiz, kmcIndexId);
    issDataStru.iAppKeyBaseIndex = (*env)->GetIntField(env, thiz, appKeyBaseIndexId);
    issDataStru.iIssuerRsaKeyIndex = (*env)->GetIntField(env, thiz, issuerRsaKeyIndexId);

    jstring jstr = (*env)->GetObjectField(env, thiz, panStringId);
    const char *str = (*env)->GetStringUTFChars(env, jstr, NULL);
    if (str != NULL) {
        strcpy(issDataStru.szPan, str);
        (*env)->ReleaseStringUTFChars(env, jstr, str);
    }

    issDataStru.iPanSerialNo = (*env)->GetIntField(env, thiz, panSerialNoId);

    jstr = (*env)->GetObjectField(env, thiz, expireDateStringId);
    str = (*env)->GetStringUTFChars(env, jstr, NULL);
    if (str != NULL) {
        strcpy(issDataStru.szExpireDate, str);
        (*env)->ReleaseStringUTFChars(env, jstr, str);
    }

    jstr = (*env)->GetObjectField(env, thiz, holderNameStringId);
    str = (*env)->GetStringUTFChars(env, jstr, NULL);
    if (str != NULL) {
        strcpy(issDataStru.szHolderName , str);
        (*env)->ReleaseStringUTFChars(env, jstr, str);
    }

    issDataStru.iHolderIdType  = (*env)->GetIntField(env, thiz, holderIdTypeId);

    jstr = (*env)->GetObjectField(env, thiz, holderIdStringId);
    str = (*env)->GetStringUTFChars(env, jstr, NULL);
    if (str != NULL) {
        strcpy(issDataStru.szHolderId , str);
        (*env)->ReleaseStringUTFChars(env, jstr, str);
    }

    jstr = (*env)->GetObjectField(env, thiz, defaultPinStringId);
    str = (*env)->GetStringUTFChars(env, jstr, NULL);
    if (str != NULL) {
        strcpy(issDataStru.szDefaultPin , str);
        (*env)->ReleaseStringUTFChars(env, jstr, str);
    }

    jstr = (*env)->GetObjectField(env, thiz, aidStringId);
    str = (*env)->GetStringUTFChars(env, jstr, NULL);
    if (str != NULL) {
        strcpy(issDataStru.szAid , str);
        (*env)->ReleaseStringUTFChars(env, jstr, str);
    }

    jstr = (*env)->GetObjectField(env, thiz, labelStringId);
    str = (*env)->GetStringUTFChars(env, jstr, NULL);
    if (str != NULL) {
        strcpy(issDataStru.szLabel , str);
        (*env)->ReleaseStringUTFChars(env, jstr, str);
    }
    issDataStru.iCaIndex  = (*env)->GetIntField(env, thiz, caIndexId);

    issDataStru.iIcRsaKeyLen  = (*env)->GetIntField(env, thiz, icRsaKeyLenId);

    issDataStru.lIcRsaE  = (*env)->GetIntField(env, thiz, icRsaEId);

    issDataStru.iCountryCode  = (*env)->GetIntField(env, thiz, countryCodeId);

    issDataStru.iCurrencyCode  = (*env)->GetIntField(env, thiz, currencyCodeId);

    if (iIssSetData(0, &issDataStru, szErrMsg)) {
    	(*env)->SetObjectField(env, thiz,errorStringId,  stoJstring(sg_env, szErrMsg));
    	return 2;
    }
    if (iIssCard(szErrMsg)) {
    	(*env)->SetObjectField(env, thiz,errorStringId, stoJstring(sg_env, szErrMsg));
    	return 3;
    }

    return 0;
}

int iWaitCard(JNIEnv *env, uint uiReader, uchar *sResetData, int timeout)
{
	jbyteArray resetData = (*env)->NewByteArray(env,80);

    int ret = (int) (*env)->CallIntMethod(env, sg_this,
    		pbocResetCallBack, resetData, timeout);
    if (ret != 0) {
		uchar *bytes = (*env)->GetByteArrayElements(env, resetData,
				0);
		memcpy(sResetData, bytes, ret);
		(*env)->ReleaseByteArrayElements(env, resetData, (jbyte *) bytes,
				0);
    }
	(*env)->DeleteLocalRef(env, resetData);

	return ret;
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

jstring stoJstring(JNIEnv* env, const char* pat)
{
	int        strLen    = strlen(pat);
	jclass     jstrObj   = (*env)->FindClass(env, "java/lang/String");
	jmethodID  methodId  = (*env)->GetMethodID(env, jstrObj, "<init>", "([BLjava/lang/String;)V");
	jbyteArray byteArray = (*env)->NewByteArray(env, strLen);
	jstring    encode    = (*env)->NewStringUTF(env, "gbk");
	(*env)->SetByteArrayRegion(env, byteArray, 0, strLen, (jbyte*)pat);
	return (jstring)(*env)->NewObject(env, jstrObj, methodId, byteArray, encode);
}

void vShowStatus(char *pszStatus)
{
    (*sg_env)->CallVoidMethod(sg_env, sg_this,
    		showStatusCallBack, stoJstring(sg_env, pszStatus));
}

