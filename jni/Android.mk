LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE    := ffmpeg_test
LOCAL_SRC_FILES := ffmpeg_test.cpp
LOCAL_SRC_FILES += ChromaKeyRenderer.cpp

LOCAL_LDLIBS := -llog -ljnigraphics -lz -landroid
LOCAL_SHARED_LIBRARIES := libavformat libavcodec libswscale libavutil


include $(BUILD_SHARED_LIBRARY)

$(call import-module,ffmpeg/android/arm)