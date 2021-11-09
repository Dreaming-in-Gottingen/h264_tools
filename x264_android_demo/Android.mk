LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

SRCS = common/osdep.c   \
       common/base.c    \
       common/cpu.c     \
       common/tables.c  \
       encoder/api.c    \

SRCS_X = common/mc.c            \
         common/predict.c       \
         common/pixel.c         \
         common/macroblock.c    \
         common/frame.c         \
         common/dct.c           \
         common/cabac.c         \
         common/common.c        \
         common/rectangle.c     \
         common/set.c           \
         common/quant.c         \
         common/deblock.c       \
         common/vlc.c           \
         common/mvpred.c        \
         common/bitstream.c     \
         encoder/analyse.c      \
         encoder/me.c           \
         encoder/ratecontrol.c  \
         encoder/set.c          \
         encoder/macroblock.c   \
         encoder/cabac.c        \
         encoder/cavlc.c        \
         encoder/encoder.c      \
         encoder/lookahead.c    \

LOCAL_SRC_FILES := $(SRCS)      \
                   $(SRCS_X)

LOCAL_C_INCLUDES := $(LOCAL_PATH)           \
                    $(LOCAL_PATH)/common    \
                    $(LOCAL_PATH)/encoder

LOCAL_SHARED_LIBRARIES := libutils libcutils

LOCAL_CFLAGS := -std=c99 -DHIGH_BIT_DEPTH=0 -DBIT_DEPTH=8

LOCAL_MODULE := libx264

LOCAL_32_BIT_ONLY := true
LOCAL_PROPRIETARY_MODULE := true

include $(BUILD_SHARED_LIBRARY)


################################################################################
include $(CLEAR_VARS)

LOCAL_SRC_FILES := example.c

LOCAL_SHARED_LIBRARIES := libx264

LOCAL_MODULE := x264_demo

LOCAL_32_BIT_ONLY := true
LOCAL_PROPRIETARY_MODULE := true

include $(BUILD_EXECUTABLE)
