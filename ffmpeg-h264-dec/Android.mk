LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

#SRCS := $(shell find ffmpeg-src -name "*.c" -not -name "*_template.c" -exec basename {} \;)
LOCAL_SRC_FILES := \
        ffmpeg-src/libavutil/display.c           \
        ffmpeg-src/libavutil/pixdesc.c           \
        ffmpeg-src/libavutil/error.c             \
        ffmpeg-src/libavutil/opt.c               \
        ffmpeg-src/libavutil/samplefmt.c         \
        ffmpeg-src/libavutil/channel_layout.c    \
        ffmpeg-src/libavutil/reverse.c           \
        ffmpeg-src/libavutil/utils.c             \
        ffmpeg-src/libavutil/time.c              \
        ffmpeg-src/libavutil/dict.c              \
        ffmpeg-src/libavutil/random_seed.c       \
        ffmpeg-src/libavutil/avstring.c          \
        ffmpeg-src/libavutil/file_open.c         \
        ffmpeg-src/libavutil/log2_tab.c          \
        ffmpeg-src/libavutil/rational.c          \
        ffmpeg-src/libavutil/frame.c             \
        ffmpeg-src/libavutil/stereo3d.c          \
        ffmpeg-src/libavutil/sha.c               \
        ffmpeg-src/libavutil/parseutils.c        \
        ffmpeg-src/libavutil/intmath.c           \
        ffmpeg-src/libavutil/mem.c               \
        ffmpeg-src/libavutil/bprint.c            \
        ffmpeg-src/libavutil/imgutils.c          \
        ffmpeg-src/libavutil/buffer.c            \
        ffmpeg-src/libavutil/mathematics.c       \
        ffmpeg-src/libavutil/log.c               \
        ffmpeg-src/libavutil/eval.c              \
        ffmpeg-src/libavcodec/h264_cabac.c       \
        ffmpeg-src/libavcodec/golomb.c           \
        ffmpeg-src/libavcodec/h264_direct.c      \
        ffmpeg-src/libavcodec/videodsp.c         \
        ffmpeg-src/libavcodec/options.c          \
        ffmpeg-src/libavcodec/h264qpel.c         \
        ffmpeg-src/libavcodec/parser.c           \
        ffmpeg-src/libavcodec/h264_mb.c          \
        ffmpeg-src/libavcodec/h264chroma.c       \
        ffmpeg-src/libavcodec/h2645_parse.c      \
        ffmpeg-src/libavcodec/h264.c             \
        ffmpeg-src/libavcodec/h264_picture.c     \
        ffmpeg-src/libavcodec/h264_parse.c       \
        ffmpeg-src/libavcodec/mathtables.c       \
        ffmpeg-src/libavcodec/h264_slice.c       \
        ffmpeg-src/libavcodec/utils.c            \
        ffmpeg-src/libavcodec/cabac.c            \
        ffmpeg-src/libavcodec/profiles.c         \
        ffmpeg-src/libavcodec/bitstream.c        \
        ffmpeg-src/libavcodec/h264pred.c         \
        ffmpeg-src/libavcodec/avpacket.c         \
        ffmpeg-src/libavcodec/h264_refs.c        \
        ffmpeg-src/libavcodec/h264data.c         \
        ffmpeg-src/libavcodec/error_resilience.c \
        ffmpeg-src/libavcodec/startcode.c        \
        ffmpeg-src/libavcodec/h264dsp.c          \
        ffmpeg-src/libavcodec/simple_idct.c      \
        ffmpeg-src/libavcodec/h264_ps.c          \
        ffmpeg-src/libavcodec/h264_sei.c         \
        ffmpeg-src/libavcodec/h264idct.c         \
        ffmpeg-src/libavcodec/me_cmp.c           \
        ffmpeg-src/libavcodec/h264_loopfilter.c  \
        ffmpeg-src/libavcodec/h264_cavlc.c       \
        ffmpeg-src/libavcodec/h264_parser.c      \
        ffmpeg-src/libavcodec/codec_desc.c

LOCAL_C_INCLUDES:= $(LOCAL_PATH)/ffmpeg-src

LOCAL_CFLAGS += -Wno-implicit-function-declaration
#LOCAL_SHARED_LIBRARIES :=

LOCAL_32_BIT_ONLY := true

LOCAL_MODULE := libffmpeg_avcdec

LOCAL_PROPRIETARY_MODULE := true
include $(BUILD_SHARED_LIBRARY)


############################################################################
include $(CLEAR_VARS)

LOCAL_SRC_FILES := avcdec_demo.c

LOCAL_C_INCLUDES:= $(LOCAL_PATH)/ffmpeg-src

LOCAL_32_BIT_ONLY := true

LOCAL_SHARED_LIBRARIES := libffmpeg_avcdec

LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE := ffH264DecDemo

include $(BUILD_EXECUTABLE)
