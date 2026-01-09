LOCAL_PATH := $(call my-dir)

# ============================================================================
# kaldi-native-fbank (静态库) - 使用 Android 版本
# ============================================================================
include $(CLEAR_VARS)
LOCAL_MODULE := kaldi-native-fbank-core
KALDI_PATH := $(LOCAL_PATH)/../../1_third_party/kaldi_native_fbank
LOCAL_SRC_FILES := $(KALDI_PATH)/Android/arm64-v8a/libkaldi-native-fbank-core.a
LOCAL_EXPORT_C_INCLUDES := $(KALDI_PATH)/include
include $(PREBUILT_STATIC_LIBRARY)

# ============================================================================
# SenseVoice Audio Frontend Test (仅测试音频处理，不加载DLA)
# ============================================================================
include $(CLEAR_VARS)

LOCAL_MODULE := sensevoice_audio_test

LOCAL_C_INCLUDES := \
    $(LOCAL_PATH)/include \
    $(KALDI_PATH)/include

LOCAL_SRC_FILES := \
    src/audio_frontend_impl.cpp \
    src/test_audio_only.cpp

LOCAL_STATIC_LIBRARIES := kaldi-native-fbank-core

LOCAL_LDLIBS := -llog -landroid

LOCAL_CPPFLAGS := -std=c++11

include $(BUILD_EXECUTABLE)
