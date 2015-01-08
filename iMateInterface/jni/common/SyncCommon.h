#ifndef _SYNCCOMMON_H
#define _SYNCCOMMON_H

#include <jni.h>

extern void syncCommonSetEnv(JNIEnv *env);

extern void syncCommonSetVM(JavaVM* vm);

extern int syncCommon(unsigned char *sIn, int iInLen, unsigned char *sOut, int *piOutLen, int timeout);

#endif
