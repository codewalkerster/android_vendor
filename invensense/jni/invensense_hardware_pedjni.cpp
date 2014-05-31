/*
 * Copyright 2008, The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
/* modified from the original (sensormanager) for use as a new Gesture Manager
 * by kpowell@invensense.com */

#define LOG_TAG "MplSysApiJni"
#define LOG_NDEBUG 0
#include "utils/Log.h"
#include <binder/IServiceManager.h>
#include <gui/ISensorServer.h>
#include <gui/IMplSysConnection.h>
#include <gui/IMplSysPedConnection.h>

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stddef.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/poll.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <linux/input.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "jni.h"
#include "JNIHelp.h"

#define FUNC_LOG ALOGV("%s", __FUNCTION__)

using namespace android;

sp<IMplSysPedConnection> get_sysped_binder() {
    //FUNC_LOG;
    static sp<IMplSysPedConnection> s_sped = 0;

    /* Get the mpl service */
    if (s_sped == NULL)
    {
        sp<ISensorServer> mSensorServer;
    const String16 name("sensorservice");
    while (getService(name, &mSensorServer) != NO_ERROR) {
        usleep(250000);
    }

    s_sped = mSensorServer->createMplSysPedConnection();
    }
    if (s_sped == NULL)
    {
      ALOGE("some problem with the sensor service");
      return false; /* return an errorcode... */
    }
    return s_sped;
}

jint JNICALL sys_startPed(JNIEnv *jenv, jclass jcls)
{
    sp<IMplSysPedConnection> s_sapi = get_sysped_binder(); if(s_sapi==0) return false;
    return (int)s_sapi->rpcStartPed();
}

jint JNICALL sys_stopPed(JNIEnv *jenv, jclass jcls)
{
    sp<IMplSysPedConnection> s_sapi = get_sysped_binder(); if(s_sapi==0) return false;
    return (int)s_sapi->rpcStopPed();
}

jint JNICALL sys_getSteps(JNIEnv *jenv, jclass jcls)
{
    sp<IMplSysPedConnection> s_sapi = get_sysped_binder(); if(s_sapi==0) return false;
    return (int)s_sapi->rpcGetSteps();
}

jdouble JNICALL sys_getWalkTime(JNIEnv *jenv, jclass jcls)
{
    sp<IMplSysPedConnection> s_sapi = get_sysped_binder(); if(s_sapi==0) return false;
    return s_sapi->rpcGetWalkTime();
}

jint JNICALL sys_clearPedData(JNIEnv *jenv, jclass jcls)
{
    sp<IMplSysPedConnection> s_sapi = get_sysped_binder(); if(s_sapi==0) return false;
    return (int)s_sapi->rpcClearPedData();
}

static JNINativeMethod sysPedMethods[] = {
    {"startPed",          "()I",   (void*)sys_startPed},
    {"stopPed",           "()I",   (void*)sys_stopPed},
    {"getSteps",          "()I",   (void*)sys_getSteps},
    {"getWalkTime",       "()D",   (void*)sys_getWalkTime},
    {"clearPedData",      "()I",   (void*)sys_clearPedData},
};

/*
 * This is called by the VM when the shared library is first loaded.
 */
jint JNI_OnLoad(JavaVM* vm, void* reserved) {
    FUNC_LOG;
    JNIEnv* env = NULL;
    jint result = -1;

    if (vm->GetEnv((void**) &env, JNI_VERSION_1_4) != JNI_OK) {
        ALOGE("ERROR: GetEnv failed\n");
        goto bail;
    }
    assert(env != NULL);

    if( jniRegisterNativeMethods(env, "com/invensense/android/hardware/pedapi/PedApi", sysPedMethods, NELEM(sysPedMethods)) != 0) {
        ALOGE("ERROR: Could not register native methods for demoapi\n");
        goto bail;
    }
    /* success -- return valid version number */
    result = JNI_VERSION_1_4;

bail:
    return result;
}
