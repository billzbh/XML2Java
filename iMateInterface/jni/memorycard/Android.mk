LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_C_INCLUDES := $(LOCAL_PATH)/../common $(LOCAL_PATH)/../memorycard.a

LOCAL_MODULE    := memorycardapi
LOCAL_SRC_FILES := memorycardapi.c ../memorycard.a/MemoryCardApiJni.c ../common/SyncCommon.c ../common/RemoteCall.c ../common/RemoteFunctions.c ../common/Sha.c ../common/WriteLog.c
LOCAL_LDLIBS := -L$(LOCAL_PATH)/../../obj/local/armeabi -llog

include $(BUILD_SHARED_LIBRARY)
