LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_C_INCLUDES := $(LOCAL_PATH)/../common/pboccore $(LOCAL_PATH)/../common

LOCAL_MODULE    := pbochighapi
LOCAL_SRC_FILES := PbocHighApi.c PbocHigh.c ../common/iMateExt.c ../common/RemoteCall.c ../common/WriteLog.c ../common/RemoteFunctions.c ../common/fdes.c ../common/pboccore/Arith.c ../common/pboccore/Common.c ../common/pboccore/EmvCmd.c ../common/pboccore/EmvCore.c ../common/pboccore/EmvCvm.c ../common/pboccore/EmvCvm2.c ../common/pboccore/EmvData.c ../common/pboccore/EmvDAuth.c ../common/pboccore/EmvFaceNull.c ../common/pboccore/EmvFunc.c ../common/pboccore/EmvIO.c ../common/pboccore/EmvModul.c ../common/pboccore/EmvMsg.c ../common/pboccore/EmvPara.c ../common/pboccore/EmvProc.c ../common/pboccore/EmvSele.c ../common/pboccore/EmvTrace.c ../common/pboccore/iso4217.c ../common/pboccore/Nn.c ../common/pboccore/PosPc.c ../common/pboccore/PushApdu.c ../common/pboccore/Rsa.c ../common/Sha.c ../common/pboccore/TagAttr.c ../common/pboccore/TlvFunc.c ../common/pboccore/PbocCtls.c
LOCAL_LDLIBS := -L$(LOCAL_PATH)/../../obj/local/armeabi -llog

include $(BUILD_SHARED_LIBRARY)

