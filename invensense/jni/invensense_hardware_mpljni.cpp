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

sp<IMplSysConnection> get_sysapi_binder() {
    //FUNC_LOG;
    static sp<IMplSysConnection> s_sapi = 0;

    /* Get the mpl service */
    if (s_sapi == NULL) {
        sp<ISensorServer> mSensorServer;
        const String16 name("sensorservice");
        while (getService(name, &mSensorServer) != NO_ERROR) {
            usleep(250000);
        }
        s_sapi = mSensorServer->createMplSysConnection();
    }
    if (s_sapi == NULL) {
      ALOGE("some problem with the sensor service");
      return false; /* return an errorcode... */
    }
    return s_sapi;
}



// ----------------------------------------------------------------------------



#ifdef __cplusplus
template<typename T> class SwigValueWrapper {
    T *tt;
public:
    SwigValueWrapper() : tt(0) { }
    SwigValueWrapper(const SwigValueWrapper<T>& rhs) : tt(new T(*rhs.tt)) { }
    SwigValueWrapper(const T& t) : tt(new T(t)) { }
    ~SwigValueWrapper() { delete tt; }
    SwigValueWrapper& operator=(const T& t) { delete tt; tt = new T(t); return *this; }
    operator T&() const { return *tt; }
    T *operator&() { return tt; }
private:
    SwigValueWrapper& operator=(const SwigValueWrapper<T>& rhs);
};

template <typename T> T SwigValueInit() {
  return T();
}
#endif

/* -----------------------------------------------------------------------------
 *  This section contains generic SWIG labels for method/variable
 *  declarations/attributes, and other compiler dependent labels.
 * ----------------------------------------------------------------------------- */

/* template workaround for compilers that cannot correctly implement the C++ standard */
#ifndef SWIGTEMPLATEDISAMBIGUATOR
# if defined(__SUNPRO_CC) && (__SUNPRO_CC <= 0x560)
#  define SWIGTEMPLATEDISAMBIGUATOR template
# elif defined(__HP_aCC)
/* Needed even with `aCC -AA' when `aCC -V' reports HP ANSI C++ B3910B A.03.55 */
/* If we find a maximum version that requires this, the test would be __HP_aCC <= 35500 for A.03.55 */
#  define SWIGTEMPLATEDISAMBIGUATOR template
# else
#  define SWIGTEMPLATEDISAMBIGUATOR
# endif
#endif

/* inline attribute */
#ifndef SWIGINLINE
# if defined(__cplusplus) || (defined(__GNUC__) && !defined(__STRICT_ANSI__))
#   define SWIGINLINE inline
# else
#   define SWIGINLINE
# endif
#endif

/* attribute recognised by some compilers to avoid 'unused' warnings */
#ifndef SWIGUNUSED
# if defined(__GNUC__)
#   if !(defined(__cplusplus)) || (__GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR__ >= 4))
#     define SWIGUNUSED __attribute__ ((__unused__))
#   else
#     define SWIGUNUSED
#   endif
# elif defined(__ICC)
#   define SWIGUNUSED __attribute__ ((__unused__))
# else
#   define SWIGUNUSED
# endif
#endif

#ifndef SWIG_MSC_UNSUPPRESS_4505
# if defined(_MSC_VER)
#   pragma warning(disable : 4505) /* unreferenced local function has been removed */
# endif
#endif

#ifndef SWIGUNUSEDPARM
# ifdef __cplusplus
#   define SWIGUNUSEDPARM(p)
# else
#   define SWIGUNUSEDPARM(p) p SWIGUNUSED
# endif
#endif

/* internal SWIG method */
#ifndef SWIGINTERN
# define SWIGINTERN static SWIGUNUSED
#endif

/* internal inline SWIG method */
#ifndef SWIGINTERNINLINE
# define SWIGINTERNINLINE SWIGINTERN SWIGINLINE
#endif

/* exporting methods */
#if (__GNUC__ >= 4) || (__GNUC__ == 3 && __GNUC_MINOR__ >= 4)
#  ifndef GCC_HASCLASSVISIBILITY
#    define GCC_HASCLASSVISIBILITY
#  endif
#endif

#ifndef SWIGEXPORT
# if defined(_WIN32) || defined(__WIN32__) || defined(__CYGWIN__)
#   if defined(STATIC_LINKED)
#     define SWIGEXPORT
#   else
#     define SWIGEXPORT __declspec(dllexport)
#   endif
# else
#   if defined(__GNUC__) && defined(GCC_HASCLASSVISIBILITY)
#     define SWIGEXPORT __attribute__ ((visibility("default")))
#   else
#     define SWIGEXPORT
#   endif
# endif
#endif

/* calling conventions for Windows */
#ifndef SWIGSTDCALL
# if defined(_WIN32) || defined(__WIN32__) || defined(__CYGWIN__)
#   define SWIGSTDCALL __stdcall
# else
#   define SWIGSTDCALL
# endif
#endif

/* Deal with Microsoft's attempt at deprecating C standard runtime functions */
#if !defined(SWIG_NO_CRT_SECURE_NO_DEPRECATE) && defined(_MSC_VER) && !defined(_CRT_SECURE_NO_DEPRECATE)
# define _CRT_SECURE_NO_DEPRECATE
#endif

/* Deal with Microsoft's attempt at deprecating methods in the standard C++ library */
#if !defined(SWIG_NO_SCL_SECURE_NO_DEPRECATE) && defined(_MSC_VER) && !defined(_SCL_SECURE_NO_DEPRECATE)
# define _SCL_SECURE_NO_DEPRECATE
#endif



/* Fix for jlong on some versions of gcc on Windows */
#if defined(__GNUC__) && !defined(__INTELC__)
  typedef long long __int64;
#endif

/* Fix for jlong on 64-bit x86 Solaris */
#if defined(__x86_64)
# ifdef _LP64
#   undef _LP64
# endif
#endif

#include <stdlib.h>
#include <string.h>


/* Support for throwing Java exceptions */
typedef enum {
  SWIG_JavaOutOfMemoryError = 1,
  SWIG_JavaIOException,
  SWIG_JavaRuntimeException,
  SWIG_JavaIndexOutOfBoundsException,
  SWIG_JavaArithmeticException,
  SWIG_JavaIllegalArgumentException,
  SWIG_JavaNullPointerException,
  SWIG_JavaDirectorPureVirtual,
  SWIG_JavaUnknownError
} SWIG_JavaExceptionCodes;

typedef struct {
  SWIG_JavaExceptionCodes code;
  const char *java_exception;
} SWIG_JavaExceptions_t;


static void SWIGUNUSED SWIG_JavaThrowException(JNIEnv *jenv, SWIG_JavaExceptionCodes code, const char *msg) {
  jclass excep;
  static const SWIG_JavaExceptions_t java_exceptions[] = {
    { SWIG_JavaOutOfMemoryError, "java/lang/OutOfMemoryError" },
    { SWIG_JavaIOException, "java/io/IOException" },
    { SWIG_JavaRuntimeException, "java/lang/RuntimeException" },
    { SWIG_JavaIndexOutOfBoundsException, "java/lang/IndexOutOfBoundsException" },
    { SWIG_JavaArithmeticException, "java/lang/ArithmeticException" },
    { SWIG_JavaIllegalArgumentException, "java/lang/IllegalArgumentException" },
    { SWIG_JavaNullPointerException, "java/lang/NullPointerException" },
    { SWIG_JavaDirectorPureVirtual, "java/lang/RuntimeException" },
    { SWIG_JavaUnknownError,  "java/lang/UnknownError" },
    { (SWIG_JavaExceptionCodes)0,  "java/lang/UnknownError" } };
  const SWIG_JavaExceptions_t *except_ptr = java_exceptions;

  while (except_ptr->code != code && except_ptr->code)
    except_ptr++;

  jenv->ExceptionClear();
  excep = jenv->FindClass(except_ptr->java_exception);
  if (excep)
    jenv->ThrowNew(excep, msg);
}


/* Contract support */

#define SWIG_contract_assert(nullreturn, expr, msg) if (!(expr)) {SWIG_JavaThrowException(jenv, SWIG_JavaIllegalArgumentException, msg); return nullreturn; } else


#if defined(SWIG_NOINCLUDE) || defined(SWIG_NOARRAYS)


int SWIG_JavaArrayInBool (JNIEnv *jenv, jboolean **jarr, bool **carr, jbooleanArray input);
void SWIG_JavaArrayArgoutBool (JNIEnv *jenv, jboolean *jarr, bool *carr, jbooleanArray input);
jbooleanArray SWIG_JavaArrayOutBool (JNIEnv *jenv, bool *result, jsize sz);


int SWIG_JavaArrayInSchar (JNIEnv *jenv, jbyte **jarr, signed char **carr, jbyteArray input);
void SWIG_JavaArrayArgoutSchar (JNIEnv *jenv, jbyte *jarr, signed char *carr, jbyteArray input);
jbyteArray SWIG_JavaArrayOutSchar (JNIEnv *jenv, signed char *result, jsize sz);


int SWIG_JavaArrayInUchar (JNIEnv *jenv, jshort **jarr, unsigned char **carr, jshortArray input);
void SWIG_JavaArrayArgoutUchar (JNIEnv *jenv, jshort *jarr, unsigned char *carr, jshortArray input);
jshortArray SWIG_JavaArrayOutUchar (JNIEnv *jenv, unsigned char *result, jsize sz);


int SWIG_JavaArrayInShort (JNIEnv *jenv, jshort **jarr, short **carr, jshortArray input);
void SWIG_JavaArrayArgoutShort (JNIEnv *jenv, jshort *jarr, short *carr, jshortArray input);
jshortArray SWIG_JavaArrayOutShort (JNIEnv *jenv, short *result, jsize sz);


int SWIG_JavaArrayInUshort (JNIEnv *jenv, jint **jarr, unsigned short **carr, jintArray input);
void SWIG_JavaArrayArgoutUshort (JNIEnv *jenv, jint *jarr, unsigned short *carr, jintArray input);
jintArray SWIG_JavaArrayOutUshort (JNIEnv *jenv, unsigned short *result, jsize sz);


int SWIG_JavaArrayInInt (JNIEnv *jenv, jint **jarr, int **carr, jintArray input);
void SWIG_JavaArrayArgoutInt (JNIEnv *jenv, jint *jarr, int *carr, jintArray input);
jintArray SWIG_JavaArrayOutInt (JNIEnv *jenv, int *result, jsize sz);


int SWIG_JavaArrayInUint (JNIEnv *jenv, jlong **jarr, unsigned int **carr, jlongArray input);
void SWIG_JavaArrayArgoutUint (JNIEnv *jenv, jlong *jarr, unsigned int *carr, jlongArray input);
jlongArray SWIG_JavaArrayOutUint (JNIEnv *jenv, unsigned int *result, jsize sz);


int SWIG_JavaArrayInLong (JNIEnv *jenv, jlong **jarr, long **carr, jLongArray input);
void SWIG_JavaArrayArgoutLong (JNIEnv *jenv, jlong *jarr, long *carr, jlongArray input);
jintArray SWIG_JavaArrayOutLong (JNIEnv *jenv, long *result, jsize sz);


int SWIG_JavaArrayInUlong (JNIEnv *jenv, jlong **jarr, unsigned long **carr, jlongArray input);
void SWIG_JavaArrayArgoutUlong (JNIEnv *jenv, jlong *jarr, unsigned long *carr, jlongArray input);
jlongArray SWIG_JavaArrayOutUlong (JNIEnv *jenv, unsigned long *result, jsize sz);


int SWIG_JavaArrayInLonglong (JNIEnv *jenv, jlong **jarr, jlong **carr, jlongArray input);
void SWIG_JavaArrayArgoutLonglong (JNIEnv *jenv, jlong *jarr, jlong *carr, jlongArray input);
jlongArray SWIG_JavaArrayOutLonglong (JNIEnv *jenv, jlong *result, jsize sz);


int SWIG_JavaArrayInFloat (JNIEnv *jenv, jfloat **jarr, float **carr, jfloatArray input);
void SWIG_JavaArrayArgoutFloat (JNIEnv *jenv, jfloat *jarr, float *carr, jfloatArray input);
jfloatArray SWIG_JavaArrayOutFloat (JNIEnv *jenv, float *result, jsize sz);


int SWIG_JavaArrayInDouble (JNIEnv *jenv, jdouble **jarr, double **carr, jdoubleArray input);
void SWIG_JavaArrayArgoutDouble (JNIEnv *jenv, jdouble *jarr, double *carr, jdoubleArray input);
jdoubleArray SWIG_JavaArrayOutDouble (JNIEnv *jenv, double *result, jsize sz);


#else


/* bool[] support */
int SWIG_JavaArrayInBool (JNIEnv *jenv, jboolean **jarr, bool **carr, jbooleanArray input) {
  int i;
  jsize sz;
  if (!input) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException, "null array");
    return 0;
  }
  sz = jenv->GetArrayLength(input);
  *jarr = jenv->GetBooleanArrayElements(input, 0);
  if (!*jarr)
    return 0;
  *carr = new bool[sz];
  if (!*carr) {
    SWIG_JavaThrowException(jenv, SWIG_JavaOutOfMemoryError, "array memory allocation failed");
    return 0;
  }
  for (i=0; i<sz; i++)
    (*carr)[i] = ((*jarr)[i] != 0);
  return 1;
}

void SWIG_JavaArrayArgoutBool (JNIEnv *jenv, jboolean *jarr, bool *carr, jbooleanArray input) {
  int i;
  jsize sz = jenv->GetArrayLength(input);
  for (i=0; i<sz; i++)
    jarr[i] = (jboolean)carr[i];
  jenv->ReleaseBooleanArrayElements(input, jarr, 0);
}

jbooleanArray SWIG_JavaArrayOutBool (JNIEnv *jenv, bool *result, jsize sz) {
  jboolean *arr;
  int i;
  jbooleanArray jresult = jenv->NewBooleanArray(sz);
  if (!jresult)
    return NULL;
  arr = jenv->GetBooleanArrayElements(jresult, 0);
  if (!arr)
    return NULL;
  for (i=0; i<sz; i++)
    arr[i] = (jboolean)result[i];
  jenv->ReleaseBooleanArrayElements(jresult, arr, 0);
  return jresult;
}


/* signed char[] support */
int SWIG_JavaArrayInSchar (JNIEnv *jenv, jbyte **jarr, signed char **carr, jbyteArray input) {
  int i;
  jsize sz;
  if (!input) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException, "null array");
    return 0;
  }
  sz = jenv->GetArrayLength(input);
  *jarr = jenv->GetByteArrayElements(input, 0);
  if (!*jarr)
    return 0;
  *carr = new signed char[sz];
  if (!*carr) {
    SWIG_JavaThrowException(jenv, SWIG_JavaOutOfMemoryError, "array memory allocation failed");
    return 0;
  }
  for (i=0; i<sz; i++)
    (*carr)[i] = (signed char)(*jarr)[i];
  return 1;
}

void SWIG_JavaArrayArgoutSchar (JNIEnv *jenv, jbyte *jarr, signed char *carr, jbyteArray input) {
  int i;
  jsize sz = jenv->GetArrayLength(input);
  for (i=0; i<sz; i++)
    jarr[i] = (jbyte)carr[i];
  jenv->ReleaseByteArrayElements(input, jarr, 0);
}

jbyteArray SWIG_JavaArrayOutSchar (JNIEnv *jenv, signed char *result, jsize sz) {
  jbyte *arr;
  int i;
  jbyteArray jresult = jenv->NewByteArray(sz);
  if (!jresult)
    return NULL;
  arr = jenv->GetByteArrayElements(jresult, 0);
  if (!arr)
    return NULL;
  for (i=0; i<sz; i++)
    arr[i] = (jbyte)result[i];
  jenv->ReleaseByteArrayElements(jresult, arr, 0);
  return jresult;
}


/* unsigned char[] support */
int SWIG_JavaArrayInUchar (JNIEnv *jenv, jshort **jarr, unsigned char **carr, jshortArray input) {
  int i;
  jsize sz;
  if (!input) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException, "null array");
    return 0;
  }
  sz = jenv->GetArrayLength(input);
  *jarr = jenv->GetShortArrayElements(input, 0);
  if (!*jarr)
    return 0;
  *carr = new unsigned char[sz];
  if (!*carr) {
    SWIG_JavaThrowException(jenv, SWIG_JavaOutOfMemoryError, "array memory allocation failed");
    return 0;
  }
  for (i=0; i<sz; i++)
    (*carr)[i] = (unsigned char)(*jarr)[i];
  return 1;
}

void SWIG_JavaArrayArgoutUchar (JNIEnv *jenv, jshort *jarr, unsigned char *carr, jshortArray input) {
  int i;
  jsize sz = jenv->GetArrayLength(input);
  for (i=0; i<sz; i++)
    jarr[i] = (jshort)carr[i];
  jenv->ReleaseShortArrayElements(input, jarr, 0);
}

jshortArray SWIG_JavaArrayOutUchar (JNIEnv *jenv, unsigned char *result, jsize sz) {
  jshort *arr;
  int i;
  jshortArray jresult = jenv->NewShortArray(sz);
  if (!jresult)
    return NULL;
  arr = jenv->GetShortArrayElements(jresult, 0);
  if (!arr)
    return NULL;
  for (i=0; i<sz; i++)
    arr[i] = (jshort)result[i];
  jenv->ReleaseShortArrayElements(jresult, arr, 0);
  return jresult;
}


/* short[] support */
int SWIG_JavaArrayInShort (JNIEnv *jenv, jshort **jarr, short **carr, jshortArray input) {
  int i;
  jsize sz;
  if (!input) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException, "null array");
    return 0;
  }
  sz = jenv->GetArrayLength(input);
  *jarr = jenv->GetShortArrayElements(input, 0);
  if (!*jarr)
    return 0;
  *carr = new short[sz];
  if (!*carr) {
    SWIG_JavaThrowException(jenv, SWIG_JavaOutOfMemoryError, "array memory allocation failed");
    return 0;
  }
  for (i=0; i<sz; i++)
    (*carr)[i] = (short)(*jarr)[i];
  return 1;
}

void SWIG_JavaArrayArgoutShort (JNIEnv *jenv, jshort *jarr, short *carr, jshortArray input) {
  int i;
  jsize sz = jenv->GetArrayLength(input);
  for (i=0; i<sz; i++)
    jarr[i] = (jshort)carr[i];
  jenv->ReleaseShortArrayElements(input, jarr, 0);
}

jshortArray SWIG_JavaArrayOutShort (JNIEnv *jenv, short *result, jsize sz) {
  jshort *arr;
  int i;
  jshortArray jresult = jenv->NewShortArray(sz);
  if (!jresult)
    return NULL;
  arr = jenv->GetShortArrayElements(jresult, 0);
  if (!arr)
    return NULL;
  for (i=0; i<sz; i++)
    arr[i] = (jshort)result[i];
  jenv->ReleaseShortArrayElements(jresult, arr, 0);
  return jresult;
}


/* unsigned short[] support */
int SWIG_JavaArrayInUshort (JNIEnv *jenv, jint **jarr, unsigned short **carr, jintArray input) {
  int i;
  jsize sz;
  if (!input) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException, "null array");
    return 0;
  }
  sz = jenv->GetArrayLength(input);
  *jarr = jenv->GetIntArrayElements(input, 0);
  if (!*jarr)
    return 0;
  *carr = new unsigned short[sz];
  if (!*carr) {
    SWIG_JavaThrowException(jenv, SWIG_JavaOutOfMemoryError, "array memory allocation failed");
    return 0;
  }
  for (i=0; i<sz; i++)
    (*carr)[i] = (unsigned short)(*jarr)[i];
  return 1;
}

void SWIG_JavaArrayArgoutUshort (JNIEnv *jenv, jint *jarr, unsigned short *carr, jintArray input) {
  int i;
  jsize sz = jenv->GetArrayLength(input);
  for (i=0; i<sz; i++)
    jarr[i] = (jint)carr[i];
  jenv->ReleaseIntArrayElements(input, jarr, 0);
}

jintArray SWIG_JavaArrayOutUshort (JNIEnv *jenv, unsigned short *result, jsize sz) {
  jint *arr;
  int i;
  jintArray jresult = jenv->NewIntArray(sz);
  if (!jresult)
    return NULL;
  arr = jenv->GetIntArrayElements(jresult, 0);
  if (!arr)
    return NULL;
  for (i=0; i<sz; i++)
    arr[i] = (jint)result[i];
  jenv->ReleaseIntArrayElements(jresult, arr, 0);
  return jresult;
}


/* int[] support */
int SWIG_JavaArrayInInt (JNIEnv *jenv, jint **jarr, int **carr, jintArray input) {
  int i;
  jsize sz;
  if (!input) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException, "null array");
    return 0;
  }
  sz = jenv->GetArrayLength(input);
  *jarr = jenv->GetIntArrayElements(input, 0);
  if (!*jarr)
    return 0;
  *carr = new int[sz];
  if (!*carr) {
    SWIG_JavaThrowException(jenv, SWIG_JavaOutOfMemoryError, "array memory allocation failed");
    return 0;
  }
  for (i=0; i<sz; i++)
    (*carr)[i] = (int)(*jarr)[i];
  return 1;
}

void SWIG_JavaArrayArgoutInt (JNIEnv *jenv, jint *jarr, int *carr, jintArray input) {
  int i;
  jsize sz = jenv->GetArrayLength(input);
  for (i=0; i<sz; i++)
    jarr[i] = (jint)carr[i];
  jenv->ReleaseIntArrayElements(input, jarr, 0);
}

jintArray SWIG_JavaArrayOutInt (JNIEnv *jenv, int *result, jsize sz) {
  jint *arr;
  int i;
  jintArray jresult = jenv->NewIntArray(sz);
  if (!jresult)
    return NULL;
  arr = jenv->GetIntArrayElements(jresult, 0);
  if (!arr)
    return NULL;
  for (i=0; i<sz; i++)
    arr[i] = (jint)result[i];
  jenv->ReleaseIntArrayElements(jresult, arr, 0);
  return jresult;
}


/* unsigned int[] support */
int SWIG_JavaArrayInUint (JNIEnv *jenv, jlong **jarr, unsigned int **carr, jlongArray input) {
  int i;
  jsize sz;
  if (!input) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException, "null array");
    return 0;
  }
  sz = jenv->GetArrayLength(input);
  *jarr = jenv->GetLongArrayElements(input, 0);
  if (!*jarr)
    return 0;
  *carr = new unsigned int[sz];
  if (!*carr) {
    SWIG_JavaThrowException(jenv, SWIG_JavaOutOfMemoryError, "array memory allocation failed");
    return 0;
  }
  for (i=0; i<sz; i++)
    (*carr)[i] = (unsigned int)(*jarr)[i];
  return 1;
}

void SWIG_JavaArrayArgoutUint (JNIEnv *jenv, jlong *jarr, unsigned int *carr, jlongArray input) {
  int i;
  jsize sz = jenv->GetArrayLength(input);
  for (i=0; i<sz; i++)
    jarr[i] = (jlong)carr[i];
  jenv->ReleaseLongArrayElements(input, jarr, 0);
}

jlongArray SWIG_JavaArrayOutUint (JNIEnv *jenv, unsigned int *result, jsize sz) {
  jlong *arr;
  int i;
  jlongArray jresult = jenv->NewLongArray(sz);
  if (!jresult)
    return NULL;
  arr = jenv->GetLongArrayElements(jresult, 0);
  if (!arr)
    return NULL;
  for (i=0; i<sz; i++)
    arr[i] = (jlong)result[i];
  jenv->ReleaseLongArrayElements(jresult, arr, 0);
  return jresult;
}


/* long[] support */
int SWIG_JavaArrayInLong (JNIEnv *jenv, jlong **jarr, long **carr, jlongArray input) {
  int i;
  jsize sz;
  if (!input) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException, "null array");
    return 0;
  }
  sz = jenv->GetArrayLength(input);
  *jarr = jenv->GetLongArrayElements(input, 0);
  if (!*jarr)
    return 0;
  *carr = new long[sz];
  if (!*carr) {
    SWIG_JavaThrowException(jenv, SWIG_JavaOutOfMemoryError, "array memory allocation failed");
    return 0;
  }
  for (i=0; i<sz; i++)
    (*carr)[i] = (long)(*jarr)[i];
  return 1;
}

void SWIG_JavaArrayArgoutLong (JNIEnv *jenv, jlong *jarr, long *carr, jlongArray input) {
  int i;
  jsize sz = jenv->GetArrayLength(input);
  for (i=0; i<sz; i++)
    jarr[i] = (jint)carr[i];
  jenv->ReleaseLongArrayElements(input, jarr, 0);
}

jintArray SWIG_JavaArrayOutLong (JNIEnv *jenv, long *result, jsize sz) {
  jint *arr;
  int i;
  jintArray jresult = jenv->NewIntArray(sz);
  if (!jresult)
    return NULL;
  arr = jenv->GetIntArrayElements(jresult, 0);
  if (!arr)
    return NULL;
  for (i=0; i<sz; i++)
    arr[i] = (jint)result[i];
  jenv->ReleaseIntArrayElements(jresult, arr, 0);
  return jresult;
}


/* unsigned long[] support */
int SWIG_JavaArrayInUlong (JNIEnv *jenv, jlong **jarr, unsigned long **carr, jlongArray input) {
  int i;
  jsize sz;
  if (!input) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException, "null array");
    return 0;
  }
  sz = jenv->GetArrayLength(input);
  *jarr = jenv->GetLongArrayElements(input, 0);
  if (!*jarr)
    return 0;
  *carr = new unsigned long[sz];
  if (!*carr) {
    SWIG_JavaThrowException(jenv, SWIG_JavaOutOfMemoryError, "array memory allocation failed");
    return 0;
  }
  for (i=0; i<sz; i++)
    (*carr)[i] = (unsigned long)(*jarr)[i];
  return 1;
}

void SWIG_JavaArrayArgoutUlong (JNIEnv *jenv, jlong *jarr, unsigned long *carr, jlongArray input) {
  int i;
  jsize sz = jenv->GetArrayLength(input);
  for (i=0; i<sz; i++)
    jarr[i] = (jlong)carr[i];
  jenv->ReleaseLongArrayElements(input, jarr, 0);
}

jlongArray SWIG_JavaArrayOutUlong (JNIEnv *jenv, unsigned long *result, jsize sz) {
  jlong *arr;
  int i;
  jlongArray jresult = jenv->NewLongArray(sz);
  if (!jresult)
    return NULL;
  arr = jenv->GetLongArrayElements(jresult, 0);
  if (!arr)
    return NULL;
  for (i=0; i<sz; i++)
    arr[i] = (jlong)result[i];
  jenv->ReleaseLongArrayElements(jresult, arr, 0);
  return jresult;
}


/* jlong[] support */
int SWIG_JavaArrayInLonglong (JNIEnv *jenv, jlong **jarr, jlong **carr, jlongArray input) {
  int i;
  jsize sz;
  if (!input) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException, "null array");
    return 0;
  }
  sz = jenv->GetArrayLength(input);
  *jarr = jenv->GetLongArrayElements(input, 0);
  if (!*jarr)
    return 0;
  *carr = new jlong[sz];
  if (!*carr) {
    SWIG_JavaThrowException(jenv, SWIG_JavaOutOfMemoryError, "array memory allocation failed");
    return 0;
  }
  for (i=0; i<sz; i++)
    (*carr)[i] = (jlong)(*jarr)[i];
  return 1;
}

void SWIG_JavaArrayArgoutLonglong (JNIEnv *jenv, jlong *jarr, jlong *carr, jlongArray input) {
  int i;
  jsize sz = jenv->GetArrayLength(input);
  for (i=0; i<sz; i++)
    jarr[i] = (jlong)carr[i];
  jenv->ReleaseLongArrayElements(input, jarr, 0);
}

jlongArray SWIG_JavaArrayOutLonglong (JNIEnv *jenv, jlong *result, jsize sz) {
  jlong *arr;
  int i;
  jlongArray jresult = jenv->NewLongArray(sz);
  if (!jresult)
    return NULL;
  arr = jenv->GetLongArrayElements(jresult, 0);
  if (!arr)
    return NULL;
  for (i=0; i<sz; i++)
    arr[i] = (jlong)result[i];
  jenv->ReleaseLongArrayElements(jresult, arr, 0);
  return jresult;
}


/* float[] support */
int SWIG_JavaArrayInFloat (JNIEnv *jenv, jfloat **jarr, float **carr, jfloatArray input) {
  int i;
  jsize sz;
  if (!input) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException, "null array");
    return 0;
  }
  sz = jenv->GetArrayLength(input);
  *jarr = jenv->GetFloatArrayElements(input, 0);
  if (!*jarr)
    return 0;
  *carr = new float[sz];
  if (!*carr) {
    SWIG_JavaThrowException(jenv, SWIG_JavaOutOfMemoryError, "array memory allocation failed");
    return 0;
  }
  for (i=0; i<sz; i++)
    (*carr)[i] = (float)(*jarr)[i];
  return 1;
}

void SWIG_JavaArrayArgoutFloat (JNIEnv *jenv, jfloat *jarr, float *carr, jfloatArray input) {
  int i;
  jsize sz = jenv->GetArrayLength(input);
  for (i=0; i<sz; i++)
    jarr[i] = (jfloat)carr[i];
  jenv->ReleaseFloatArrayElements(input, jarr, 0);
}

jfloatArray SWIG_JavaArrayOutFloat (JNIEnv *jenv, float *result, jsize sz) {
  jfloat *arr;
  int i;
  jfloatArray jresult = jenv->NewFloatArray(sz);
  if (!jresult)
    return NULL;
  arr = jenv->GetFloatArrayElements(jresult, 0);
  if (!arr)
    return NULL;
  for (i=0; i<sz; i++)
    arr[i] = (jfloat)result[i];
  jenv->ReleaseFloatArrayElements(jresult, arr, 0);
  return jresult;
}


/* double[] support */
int SWIG_JavaArrayInDouble (JNIEnv *jenv, jdouble **jarr, double **carr, jdoubleArray input) {
  int i;
  jsize sz;
  if (!input) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException, "null array");
    return 0;
  }
  sz = jenv->GetArrayLength(input);
  *jarr = jenv->GetDoubleArrayElements(input, 0);
  if (!*jarr)
    return 0;
  *carr = new double[sz];
  if (!*carr) {
    SWIG_JavaThrowException(jenv, SWIG_JavaOutOfMemoryError, "array memory allocation failed");
    return 0;
  }
  for (i=0; i<sz; i++)
    (*carr)[i] = (double)(*jarr)[i];
  return 1;
}

void SWIG_JavaArrayArgoutDouble (JNIEnv *jenv, jdouble *jarr, double *carr, jdoubleArray input) {
  int i;
  jsize sz = jenv->GetArrayLength(input);
  for (i=0; i<sz; i++)
    jarr[i] = (jdouble)carr[i];
  jenv->ReleaseDoubleArrayElements(input, jarr, 0);
}

jdoubleArray SWIG_JavaArrayOutDouble (JNIEnv *jenv, double *result, jsize sz) {
  jdouble *arr;
  int i;
  jdoubleArray jresult = jenv->NewDoubleArray(sz);
  if (!jresult)
    return NULL;
  arr = jenv->GetDoubleArrayElements(jresult, 0);
  if (!arr)
    return NULL;
  for (i=0; i<sz; i++)
    arr[i] = (jdouble)result[i];
  jenv->ReleaseDoubleArrayElements(jresult, arr, 0);
  return jresult;
}


#endif


#ifdef __cplusplus
extern "C" {
#endif




/* ----------------------------------------------------------------------- */
/* Sytem Integrator APIs                                                   */
/* ----------------------------------------------------------------------- */


SWIGEXPORT jint JNICALL Java_com_invensense_android_hardware_demoapi_get_biases(JNIEnv *jenv, jclass jcls, jfloatArray jarg1) {
  jint jresult = 0 ;
  float *arg1 = (float *) 0 ;
  jfloat *jarr1 ;
  int result;

  (void)jenv;
  (void)jcls;
  if (!SWIG_JavaArrayInFloat(jenv, &jarr1, &arg1, jarg1)) return 0;
  sp<IMplSysConnection> s_sapi = get_sysapi_binder(); if(s_sapi==0) return false;
  result = (int)s_sapi->getBiases(arg1);
  jresult = (jint)result;
  SWIG_JavaArrayArgoutFloat(jenv, jarr1, arg1, jarg1);
  delete [] arg1;
  return jresult;
}

SWIGEXPORT jint JNICALL Java_com_invensense_android_hardware_demoapi_set_biases(JNIEnv *jenv, jclass jcls, jfloatArray jarg1) {
  jint jresult = 0 ;
  float *arg1 = (float *) 0 ;
  jfloat *jarr1 ;
  int result;

  (void)jenv;
  (void)jcls;
  if (!SWIG_JavaArrayInFloat(jenv, &jarr1, &arg1, jarg1)) return 0;
  sp<IMplSysConnection> s_sapi = get_sysapi_binder(); if(s_sapi==0) return false;
  result = (int)s_sapi->setBiases(arg1);
  jresult = (jint)result;
  SWIG_JavaArrayArgoutFloat(jenv, jarr1, arg1, jarg1);
  delete [] arg1;
  return jresult;
}

jint JNICALL sys_setSensors(JNIEnv *jenv, jclass jcls, jlong jarg1)
{
    sp<IMplSysConnection> s_sapi = get_sysapi_binder(); if(s_sapi==0) return false;
    return (int)s_sapi->setSensors(jarg1);
}

jint JNICALL sys_setBiasUpdateFunc(JNIEnv *jenv, jclass jcls, jlong jarg1)
{
    sp<IMplSysConnection> s_sapi = get_sysapi_binder(); if(s_sapi==0) return false;
    return (int)s_sapi->setBiasUpdateFunc(jarg1);
}

jint JNICALL sys_resetCal(JNIEnv *jenv, jclass jcls)
{
    sp<IMplSysConnection> s_sapi = get_sysapi_binder(); if(s_sapi==0) return false;
    return (int)s_sapi->resetCal();
}

jint JNICALL sys_selfTest(JNIEnv *jenv, jclass jcls)
{
    sp<IMplSysConnection> s_sapi = get_sysapi_binder(); if(s_sapi==0) return false;
    return (int)s_sapi->selfTest();
}

jint JNICALL sys_getSensors(JNIEnv *jenv, jclass jcls, jlongArray jarg1)
{
  jint jresult = 0 ;
  long *arg1 = (long *) 0 ;
  jlong *jarr1 ;
  int result;

  (void)jenv;
  (void)jcls;
  if (!SWIG_JavaArrayInLong(jenv, &jarr1, &arg1, jarg1)) return 0;
  sp<IMplSysConnection> s_sapi = get_sysapi_binder(); if(s_sapi==0) return false;
  result = (int)s_sapi->getSensors(arg1);
  jresult = (jint)result;
  SWIG_JavaArrayArgoutLong(jenv, jarr1, arg1, jarg1);
  delete [] arg1;
  return jresult;
}

jint JNICALL sys_setLocalMagField(JNIEnv *jenv, jclass jcls, jfloat x, jfloat y, jfloat z)
{
    sp<IMplSysConnection> s_sapi = get_sysapi_binder(); if(s_sapi==0) return false;
    return (int)s_sapi->rpcSetLocalMagField(x, y, z);
}

/* ------ end of demo apis -------------- */

#ifdef __cplusplus
}
#endif


static JNINativeMethod sysMethods[] = {
    {"resetCal",          "()I" ,  (void*)sys_resetCal},
    {"selfTest",          "()I" ,  (void*)sys_selfTest},
    {"setBiasUpdateFunc", "(J)I",  (void*)sys_setBiasUpdateFunc},
    {"setSensors",        "(J)I" , (void*)sys_setSensors},
    {"getSensors",        "([J)I", (void*)sys_getSensors},
    {"getBiases",         "([F)I", (void*)Java_com_invensense_android_hardware_demoapi_get_biases},
    {"setBiases",         "([F)I", (void*)Java_com_invensense_android_hardware_demoapi_set_biases},
    {"setLocalMagField",  "(FFF)I",(void*)sys_setLocalMagField},
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

    if( jniRegisterNativeMethods(env, "com/invensense/android/hardware/sysapi/SysApi", sysMethods, NELEM(sysMethods)) != 0) {
        ALOGE("ERROR: Could not register native methods for demoapi\n");
        goto bail;
    }
    /* success -- return valid version number */
    result = JNI_VERSION_1_4;

bail:
    return result;
}

