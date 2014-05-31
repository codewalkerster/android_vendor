LOCAL_PATH := $(call my-dir)

# the library
# ============================================================
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
            $(call all-subdir-java-files)

LOCAL_MODULE_TAGS := eng optional

# This is the target being built.
LOCAL_MODULE:= com.invensense.android.hardware.sysapi

include $(BUILD_HOST_JAVA_LIBRARY)

