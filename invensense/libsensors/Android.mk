# Copyright (C) 2008 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
# Modified 2011 by InvenSense, Inc


LOCAL_PATH := $(call my-dir)

ifneq ($(TARGET_SIMULATOR),true)

# InvenSense fragment of the HAL
include $(CLEAR_VARS)

LOCAL_MODULE := libinvensense_hal
 
LOCAL_MODULE_TAGS := optional
 
LOCAL_CFLAGS := -DLOG_TAG=\"Sensors\"

#
# NOTE: official HAL release builds are independent from the device the 
#       underlying MPL and kernel driver is build with. 
#       Therefore the MPU_NAME define should be kept empty or undefined.
#
#       Un-official or engineering builds might contains cross-device code
#       that needs to be compiled with one of the following defines 
#       CONFIG_MPU_SENSORS_MPUxxxx; in that case defining MPU_NAME equal to one 
#       of MPU3050, MPU6050A2, or MPU6050B1 provides the necessary definition.
#       e.g. when using a top level make, add MPU_NAME=MPUxxxx to the 
#            command line.
#            when using mmm, export MPU_NAME=MPUxxxx prior to running the build
#            and use 'mmm -e' to honor the environment variables' settings.
#
ifeq ($(MPU_NAME),MPU3050)
SDK_LIB_FOLDER := mlsdk_mpu3050
else
SDK_LIB_FOLDER := mlsdk
endif

ifeq ($(MPU_NAME),MPU3050)
LOCAL_CFLAGS += -DCONFIG_MPU_SENSORS_MPU3050=1
endif
ifeq ($(MPU_NAME),MPU6050A2)
LOCAL_CFLAGS += -DCONFIG_MPU_SENSORS_MPU6050A2=1
endif
ifeq ($(MPU_NAME),MPU6050B1)
LOCAL_CFLAGS += -DCONFIG_MPU_SENSORS_MPU6050B1=1
endif
 
LOCAL_SRC_FILES := SensorBase.cpp
LOCAL_SRC_FILES += MPLSensor.cpp
LOCAL_SRC_FILES += MPLSensorSysApi.cpp

LOCAL_C_INCLUDES += $(LOCAL_PATH)/$(SDK_LIB_FOLDER)/platform/include
LOCAL_C_INCLUDES += $(LOCAL_PATH)/$(SDK_LIB_FOLDER)/platform/include/linux
LOCAL_C_INCLUDES += $(LOCAL_PATH)/$(SDK_LIB_FOLDER)/platform/linux
LOCAL_C_INCLUDES += $(LOCAL_PATH)/$(SDK_LIB_FOLDER)/mllite
LOCAL_C_INCLUDES += $(LOCAL_PATH)/$(SDK_LIB_FOLDER)/mldmp
LOCAL_C_INCLUDES += $(LOCAL_PATH)/$(SDK_LIB_FOLDER)/external/aichi
LOCAL_C_INCLUDES += $(LOCAL_PATH)/$(SDK_LIB_FOLDER)/external/akmd

LOCAL_SHARED_LIBRARIES := liblog libcutils libutils libdl
LOCAL_SHARED_LIBRARIES += libmllite libmlplatform

#Additions for SysPed
LOCAL_SHARED_LIBRARIES += libmplmpu
LOCAL_C_INCLUDES += $(LOCAL_PATH)/$(SDK_LIB_FOLDER)/mldmp

LOCAL_CPPFLAGS += -DLINUX=1
LOCAL_CPPFLAGS += -DMPL_LIB_NAME=\"libmplmpu.so\"
LOCAL_CPPFLAGS += -DAICHI_LIB_NAME=\"libami.so\"
LOCAL_CPPFLAGS += -DAKM_LIB_NAME=\"libakmd.so\"
LOCAL_PRELINK_MODULE := false

include $(BUILD_SHARED_LIBRARY)

endif # !TARGET_SIMULATOR

# Build a temporary HAL that links the InvenSense .so
include $(CLEAR_VARS)
LOCAL_MODULE := sensors.$(TARGET_DEVICE)
LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)/hw

# Additions
LOCAL_SHARED_LIBRARIES += libmplmpu
LOCAL_C_INCLUDES += $(LOCAL_PATH)/$(SDK_LIB_FOLDER)/platform/include
LOCAL_C_INCLUDES += $(LOCAL_PATH)/$(SDK_LIB_FOLDER)/platform/include/linux
LOCAL_C_INCLUDES += $(LOCAL_PATH)/$(SDK_LIB_FOLDER)/platform/linux
LOCAL_C_INCLUDES += $(LOCAL_PATH)/$(SDK_LIB_FOLDER)/mllite
LOCAL_C_INCLUDES += $(LOCAL_PATH)/$(SDK_LIB_FOLDER)/mldmp
LOCAL_C_INCLUDES += $(LOCAL_PATH)/$(SDK_LIB_FOLDER)/external/aichi
LOCAL_C_INCLUDES += $(LOCAL_PATH)/$(SDK_LIB_FOLDER)/external/akmd
LOCAL_C_INCLUDES += $(LOCAL_PATH)/$(SDK_LIB_FOLDER)/mldmp

LOCAL_PRELINK_MODULE := false
LOCAL_MODULE_TAGS := optional
LOCAL_CFLAGS := -DLOG_TAG=\"Sensors\"
ifeq ($(BOARD_HAVE_BMP180),true)
LOCAL_CFLAGS += -DBOARD_HAVE_BMP180
LOCAL_CFLAGS += -DBMP180_INPUT_NAME=\"$(BOARD_BMP180_INPUT_NAME)\"
LOCAL_CFLAGS += -DWITH_AMBIENT_TEMPERATURE
ifeq ($(BOARD_BMP180_INPUT_NAME), ioboard-sensor)
LOCAL_CFLAGS += -DA4
endif
endif

LOCAL_SRC_FILES := sensors_mpl.cpp 
LOCAL_SRC_FILES += SensorBase.cpp
LOCAL_SRC_FILES += InputEventReader.cpp
LOCAL_SRC_FILES += LightSensor.cpp
ifeq ($(BOARD_HAVE_BMP180),true)
LOCAL_SRC_FILES += PressureSensor.cpp
endif
LOCAL_SRC_FILES += SamsungSensorBase.cpp

LOCAL_SHARED_LIBRARIES := libinvensense_hal libcutils libutils libdl
include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := libmplmpu
ifeq ($(MPU_NAME),MPU3050)
LOCAL_SRC_FILES := libmplmpu3050.so
else
LOCAL_SRC_FILES := libmplmpu.so
endif
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_SUFFIX := .so
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_PATH := $(TARGET_OUT)/lib
OVERRIDE_BUILT_MODULE_PATH := $(TARGET_OUT_INTERMEDIATE_LIBRARIES)
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := libmllite
ifeq ($(MPU_NAME),MPU3050)
LOCAL_SRC_FILES := libmllite3050.so
else
LOCAL_SRC_FILES := libmllite.so
endif
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_SUFFIX := .so
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_PATH := $(TARGET_OUT)/lib
OVERRIDE_BUILT_MODULE_PATH := $(TARGET_OUT_INTERMEDIATE_LIBRARIES)
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := libmlplatform
ifeq ($(MPU_NAME),MPU3050)
LOCAL_SRC_FILES := libmlplatform3050.so
else
LOCAL_SRC_FILES := libmlplatform.so
endif
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_SUFFIX := .so
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_PATH := $(TARGET_OUT)/lib
OVERRIDE_BUILT_MODULE_PATH := $(TARGET_OUT_INTERMEDIATE_LIBRARIES)
include $(BUILD_PREBUILT)

include $(CLEAR_VARS) 
LOCAL_MODULE := libakmd 
LOCAL_SRC_FILES := libakmd.so 
LOCAL_MODULE_TAGS := eng
LOCAL_MODULE_SUFFIX := .so 
LOCAL_MODULE_CLASS := SHARED_LIBRARIES 
LOCAL_MODULE_PATH := $(TARGET_OUT)/lib 
OVERRIDE_BUILT_MODULE_PATH := $(TARGET_OUT_INTERMEDIATE_LIBRARIES) 
include $(BUILD_PREBUILT)
