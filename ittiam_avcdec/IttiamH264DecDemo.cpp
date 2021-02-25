#define LOG_TAG "IH264DDemo"
#include <utils/Log.h>

#include <utils/Errors.h>
#include <utils/Timers.h>

#include "bs_read.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <ih264_typedefs.h>
#include <ih264d.h>

#define CODEC_MAX_NUM_CORES 4
#define MIN(a,b) ((a)<(b)?(a):(b))

namespace android {

#define UNUSED __attribute__ ((unused))

typedef struct {
    int startcodeprefix_len;         //! 4 for parameter sets and first slice in picture, 3 for everything else (suggested)
    unsigned len;                    //! Length of the NAL unit (Excluding the start code, which does not belong to the NALU) -> include startcode
    unsigned max_size;               //! Nal Unit Buffer size
    int forbidden_bit;               //! should be always FALSE
    int nal_reference_idc;           //! NALU_PRIORITY_xxxx
    int nal_unit_type;               //! NALU_TYPE_xxxx
    unsigned char *buf;              //! contains the first byte followed by the EBSP -> include startcode
    unsigned char frame_type;        //! nalu frame type (I/P/B)
} NALU_t;

static inline int FindStartCodeLen3 (unsigned char *Buf){
    if(Buf[0]==0 && Buf[1]==0 && Buf[2]==1)
        return 1;   //0x000001
    else
        return 0;   //others
}

static inline int FindStartCodeLen4 (unsigned char *Buf){
    if(Buf[0]==0 && Buf[1]==0 && Buf[2]==0 && Buf[3]==1)
        return 1;   //0x00000001
    else
        return 0;
}

/**
 * @return data_len(startcode_len + nalu_len)
 */
int GetAnnexbNALU (NALU_t *nalu, FILE *fp_bs)
{
    int len3=0, len4=0;
    int pos = 0;
    int StartCodeFound, rewind;
    unsigned char *Buf = nalu->buf;

    nalu->startcodeprefix_len=3;

    // find current startcode
    if (3 != fread (Buf, 1, 3, fp_bs)) {
        return 0;
    }
    len3 = FindStartCodeLen3 (Buf);
    if (len3 == 0) {
        if(1 != fread(Buf+3, 1, 1, fp_bs)){
            return -1;
        }
        len4 = FindStartCodeLen4 (Buf);
        if (len4 == 0) {    // unknown
            return -1;
        } else {            // 0x00,0x00,0x00,0x01
            pos = 4;
            nalu->startcodeprefix_len = 4;
        }
    } else {                // 0x00,0x00,0x01
        nalu->startcodeprefix_len = 3;
        pos = 3;
    }

    StartCodeFound = 0;
    len3 = 0;
    len4 = 0;

    // find another startcode
    while (!StartCodeFound) {
        if (feof (fp_bs)){
            nalu->len = (pos-1);    //-nalu->startcodeprefix_len;
            nalu->forbidden_bit = nalu->buf[0] & 0x80; //1 bit
            nalu->nal_reference_idc = nalu->buf[0] & 0x60; // 2 bit
            nalu->nal_unit_type = (nalu->buf[0]) & 0x1f;// 5 bit
            return pos-1;
        }
        Buf[pos++] = fgetc (fp_bs);
        len4 = FindStartCodeLen4(&Buf[pos-4]);
        if(len4 != 1)
            len3 = FindStartCodeLen3(&Buf[pos-3]);
        StartCodeFound = (len3 == 1 || len4 == 1);
    }

    // Here, we have found another start code (and read length of startcode bytes more than we should
    // have.  Hence, go back in the file
    rewind = (len4 == 1)? -4 : -3;

    if (0 != fseek (fp_bs, rewind, SEEK_CUR)){
        printf("GetAnnexbNALU: Cannot fseek in the bit stream file");
    }

    // Here the Start code, the complete NALU, and the next start code is in the Buf.
    // The size of Buf is pos, pos+rewind are the number of bytes excluding the next
    // start code, and (pos+rewind)-startcodeprefix_len is the size of the NALU excluding the start code

    nalu->len = (pos+rewind);   //-nalu->startcodeprefix_len;
    nalu->forbidden_bit = nalu->buf[0] & 0x80; //1 bit
    nalu->nal_reference_idc = nalu->buf[0] & 0x60; // 2 bit
    nalu->nal_unit_type = (nalu->buf[0]) & 0x1f;// 5 bit

    return (pos+rewind);
}

class IH264Decoder {
public:
    status_t InitDecoder();
    status_t DeinitDecoder();
    status_t DecodeOneFrame(UWORD8 *pInBuf, uint32_t inLen, UWORD8 *pOutBuf, uint32_t *pOutWidth, uint32_t *pOutHeight);

    static IH264Decoder* CreateDecoder() {
        return new IH264Decoder();
    }
    void DestroyDecoder();

private:
    IH264Decoder();
    ~IH264Decoder();

    status_t SetStrideParam();
    void SetDecodeArgs(
        ivd_video_decode_ip_t *ps_dec_ip,
        ivd_video_decode_op_t *ps_dec_op,
        UWORD8 *p_in_buf, size_t in_len,
        UWORD8 *p_out_buf);

    size_t mNumCores;     // Number of cores to be uesd by the codec

    size_t mStride;
    size_t mWidth;
    size_t mHeight;

    nsecs_t mTimeStart;   // Time at the start of decode()
    nsecs_t mTimeEnd;     // Time at the end of decode()

    iv_obj_t *mpCodecCtx;
};

IH264Decoder::IH264Decoder()
        : mStride(1920),
          mWidth(1920),
          mHeight(1080),
          mTimeStart(0),
          mTimeEnd(0),
          mpCodecCtx(NULL)
{
    ALOGD("IH264Decoder ctor!");
}

IH264Decoder::~IH264Decoder()
{
    ALOGD("IH264Decoder dtor!");
}

void IH264Decoder::DestroyDecoder()
{
    DeinitDecoder();
    delete this;
}

static void *ivd_aligned_malloc(void *ctxt UNUSED, WORD32 alignment, WORD32 size)
{
    return memalign(alignment, size);
}

static void ivd_aligned_free(void *ctxt UNUSED, void *buf)
{
    free(buf);
    return;
}

status_t IH264Decoder::InitDecoder()
{
    IV_API_CALL_STATUS_T status;

#if defined(_SC_NPROCESSORS_ONLN)
    mNumCores = sysconf(_SC_NPROCESSORS_ONLN);
#else
    mNumCores = sysconf(_SC_NPROC_ONLN);
#endif
    ALOGD("mNumCores=%zu", mNumCores);

    /* Initialize the decoder */
    {
        ih264d_create_ip_t s_create_ip;
        ih264d_create_op_t s_create_op;

        void *dec_fxns = (void *)ih264d_api_function;

        s_create_ip.s_ivd_create_ip_t.u4_size = sizeof(ih264d_create_ip_t);
        s_create_ip.s_ivd_create_ip_t.e_cmd = IVD_CMD_CREATE;
        s_create_ip.s_ivd_create_ip_t.u4_share_disp_buf = 0;
        s_create_ip.s_ivd_create_ip_t.e_output_format = IV_YUV_420P;    // mIvColorFormat;
        s_create_ip.s_ivd_create_ip_t.pf_aligned_alloc = ivd_aligned_malloc;
        s_create_ip.s_ivd_create_ip_t.pf_aligned_free = ivd_aligned_free;
        s_create_ip.s_ivd_create_ip_t.pv_mem_ctxt = NULL;

        s_create_op.s_ivd_create_op_t.u4_size = sizeof(ih264d_create_op_t);

        status = ih264d_api_function(mpCodecCtx, (void *)&s_create_ip, (void *)&s_create_op);
        if (status != IV_SUCCESS) {
            ALOGE("Error in create: %#x", s_create_op.s_ivd_create_op_t.u4_error_code);
            mpCodecCtx = NULL;
            return UNKNOWN_ERROR;
        } else {
            ALOGD("success to create avc decoder!");
        }

        mpCodecCtx = (iv_obj_t*)s_create_op.s_ivd_create_op_t.pv_handle;
        mpCodecCtx->pv_fxns = dec_fxns;
        mpCodecCtx->u4_size = sizeof(iv_obj_t);
    }

    /* dump codec version */
    {
        ivd_ctl_getversioninfo_ip_t s_ctl_ip;
        ivd_ctl_getversioninfo_op_t s_ctl_op;
        UWORD8 au1_buf[512];

        s_ctl_ip.e_cmd = IVD_CMD_VIDEO_CTL;
        s_ctl_ip.e_sub_cmd = IVD_CMD_CTL_GETVERSION;
        s_ctl_ip.u4_size = sizeof(ivd_ctl_getversioninfo_ip_t);
        s_ctl_op.u4_size = sizeof(ivd_ctl_getversioninfo_op_t);
        s_ctl_ip.pv_version_buffer = au1_buf;
        s_ctl_ip.u4_version_buffer_size = sizeof(au1_buf);

        status = ih264d_api_function(mpCodecCtx, (void *)&s_ctl_ip, (void *)&s_ctl_op);
        if (status != IV_SUCCESS) {
            ALOGE("Error in getting version number: %#x", s_ctl_op.u4_error_code);
        } else {
            ALOGD("Ittiam decoder version number: %s", (char *)s_ctl_ip.pv_version_buffer);
        }
    }

    /* Set the run time (dynamic) parameters */
    {
        ivd_ctl_set_config_ip_t s_ctl_ip;
        ivd_ctl_set_config_op_t s_ctl_op;

        s_ctl_ip.u4_disp_wd = mStride;
        s_ctl_ip.e_frm_skip_mode = IVD_SKIP_NONE;

        s_ctl_ip.e_frm_out_mode = IVD_DISPLAY_FRAME_OUT;
        s_ctl_ip.e_vid_dec_mode = IVD_DECODE_FRAME;
        s_ctl_ip.e_cmd = IVD_CMD_VIDEO_CTL;
        s_ctl_ip.e_sub_cmd = IVD_CMD_CTL_SETPARAMS;
        s_ctl_ip.u4_size = sizeof(ivd_ctl_set_config_ip_t);
        s_ctl_op.u4_size = sizeof(ivd_ctl_set_config_op_t);

        ALOGD("Set the run-time (dynamic) parameters stride = %zu", mStride);

        status = ih264d_api_function(mpCodecCtx, (void *)&s_ctl_ip, (void *)&s_ctl_op);
        if (status != IV_SUCCESS) {
            ALOGE("Error in setting the run-time parameters: %#x", s_ctl_op.u4_error_code);
            return UNKNOWN_ERROR;
        }
    }

    /* Set number of cores/threads to be used by the codec */
    {
        ih264d_ctl_set_num_cores_ip_t s_set_cores_ip;
        ih264d_ctl_set_num_cores_op_t s_set_cores_op;
        s_set_cores_ip.e_cmd = IVD_CMD_VIDEO_CTL;
        s_set_cores_ip.e_sub_cmd = (IVD_CONTROL_API_COMMAND_TYPE_T)IH264D_CMD_CTL_SET_NUM_CORES;
        s_set_cores_ip.u4_num_cores = MIN(mNumCores, CODEC_MAX_NUM_CORES);
        s_set_cores_ip.u4_size = sizeof(ih264d_ctl_set_num_cores_ip_t);
        s_set_cores_op.u4_size = sizeof(ih264d_ctl_set_num_cores_op_t);
        status = ih264d_api_function(mpCodecCtx, (void *)&s_set_cores_ip, (void *)&s_set_cores_op);
        if (IV_SUCCESS != status) {
            ALOGE("Error in setting number of cores: %#x", s_set_cores_op.u4_error_code);
            return UNKNOWN_ERROR;
        }
    }

    return OK;
}

status_t IH264Decoder::SetStrideParam()
{
    IV_API_CALL_STATUS_T status;

    /* Set the run time (dynamic) parameters */
    ivd_ctl_set_config_ip_t s_ctl_ip;
    ivd_ctl_set_config_op_t s_ctl_op;

    s_ctl_ip.u4_disp_wd = mStride;
    s_ctl_ip.e_frm_skip_mode = IVD_SKIP_NONE;

    s_ctl_ip.e_frm_out_mode = IVD_DISPLAY_FRAME_OUT;
    s_ctl_ip.e_vid_dec_mode = IVD_DECODE_FRAME;
    s_ctl_ip.e_cmd = IVD_CMD_VIDEO_CTL;
    s_ctl_ip.e_sub_cmd = IVD_CMD_CTL_SETPARAMS;
    s_ctl_ip.u4_size = sizeof(ivd_ctl_set_config_ip_t);
    s_ctl_op.u4_size = sizeof(ivd_ctl_set_config_op_t);

    ALOGD("fix the run-time (dynamic) parameters stride = %zu", mStride);

    status = ih264d_api_function(mpCodecCtx, (void *)&s_ctl_ip, (void *)&s_ctl_op);
    if (status != IV_SUCCESS) {
        ALOGE("Error in setting the run-time parameters: %#x", s_ctl_op.u4_error_code);
        return UNKNOWN_ERROR;
    } else {
        return OK;
    }
}

status_t IH264Decoder::DeinitDecoder()
{
    IV_API_CALL_STATUS_T status;

    if (mpCodecCtx) {
        ih264d_delete_ip_t s_delete_ip;
        ih264d_delete_op_t s_delete_op;

        s_delete_ip.s_ivd_delete_ip_t.u4_size = sizeof(ih264d_delete_ip_t);
        s_delete_ip.s_ivd_delete_ip_t.e_cmd = IVD_CMD_DELETE;

        s_delete_op.s_ivd_delete_op_t.u4_size = sizeof(ih264d_delete_op_t);

        status = ih264d_api_function(mpCodecCtx, (void *)&s_delete_ip, (void *)&s_delete_op);
        if (status != IV_SUCCESS) {
            ALOGE("Error in delete: %#x", s_delete_op.s_ivd_delete_op_t.u4_error_code);
            return UNKNOWN_ERROR;
        }
    }

    return OK;
}

void IH264Decoder::SetDecodeArgs(
        ivd_video_decode_ip_t *ps_dec_ip,
        ivd_video_decode_op_t *ps_dec_op,
        UWORD8 *p_in_buf, size_t in_len,
        UWORD8 *p_out_buf)
{
    size_t sizeY = mWidth * mHeight;
    size_t sizeUV = sizeY >> 2;

    ps_dec_ip->u4_size = sizeof(ivd_video_decode_ip_t);
    ps_dec_op->u4_size = sizeof(ivd_video_decode_op_t);
    ps_dec_ip->e_cmd = IVD_CMD_VIDEO_DECODE;

    ps_dec_ip->u4_ts = 0;
    ps_dec_ip->pv_stream_buffer = p_in_buf;
    ps_dec_ip->u4_num_Bytes = in_len;

    ps_dec_ip->s_out_buffer.u4_min_out_buf_size[0] = sizeY;
    ps_dec_ip->s_out_buffer.u4_min_out_buf_size[1] = sizeUV;
    ps_dec_ip->s_out_buffer.u4_min_out_buf_size[2] = sizeUV;

    ps_dec_ip->s_out_buffer.pu1_bufs[0] = p_out_buf;
    ps_dec_ip->s_out_buffer.pu1_bufs[1] = p_out_buf + sizeY;
    ps_dec_ip->s_out_buffer.pu1_bufs[2] = p_out_buf + sizeY + sizeUV;
    ps_dec_ip->s_out_buffer.u4_num_bufs = 3;
}

status_t IH264Decoder::DecodeOneFrame(UWORD8 *pInBuf, uint32_t inLen, UWORD8 *pOutBuf, uint32_t *pOutWidth, uint32_t *pOutHeight)
{
    ivd_video_decode_ip_t s_dec_ip;
    ivd_video_decode_op_t s_dec_op;

    nsecs_t timeDelay, timeTaken;
    SetDecodeArgs(&s_dec_ip, &s_dec_op, pInBuf, inLen, pOutBuf);

    mTimeStart = systemTime();
    timeDelay = mTimeStart - mTimeEnd;

    IV_API_CALL_STATUS_T status;
    status = ih264d_api_function(mpCodecCtx, (void *)&s_dec_ip, (void *)&s_dec_op);

    if (status != IV_SUCCESS) {
        ALOGE("Error in header decode %#x\n",  s_dec_op.u4_error_code);
    }

    /* Check for unsupported dimensions */
    bool unsupportedResolution = (IVD_STREAM_WIDTH_HEIGHT_NOT_SUPPORTED == (s_dec_op.u4_error_code & 0xFF));
    if (unsupportedResolution) {
        ALOGE("Unsupported resolution : %zu x %zu", mWidth, mHeight);
        return UNKNOWN_ERROR;
    }

    bool allocationFailed = (IVD_MEM_ALLOC_FAILED == (s_dec_op.u4_error_code & 0xFF));
    if (allocationFailed) {
        ALOGE("Allocation failure in decoder");
        return UNKNOWN_ERROR;
    }

    bool resChanged = (IVD_RES_CHANGED == (s_dec_op.u4_error_code & 0xFF));
    if (resChanged) {
        ALOGW("Resolution changed!");
    }
    //getVUIParams();

    mTimeEnd = systemTime();
    /* Compute time taken for decode() */
    timeTaken = mTimeEnd - mTimeStart;

    ALOGD("timeTaken=%6lldus, delay=%6lldus, consumed_bytes=%6d, output_present=%d, wxh=%zux%zu", (long long)(timeTaken / 1000ll),
            (long long)(timeDelay / 1000ll), s_dec_op.u4_num_bytes_consumed, s_dec_op.u4_output_present, mWidth, mHeight);
    if ((mWidth!=s_dec_op.u4_pic_wd) || (mHeight!=s_dec_op.u4_pic_ht)) {
        *pOutWidth = mWidth = mStride = s_dec_op.u4_pic_wd;
        *pOutHeight = mHeight = s_dec_op.u4_pic_ht;
        SetStrideParam();
    }

    if (s_dec_op.u4_output_present) {
        //ALOGD("success to decode one frame!");
        return OK;
    } else {
        //ALOGD("no frame out!");
        return UNKNOWN_ERROR;
    }
}

} // namespace android


using namespace android;
int main()
{
    ALOGI("--------------IttiamH264DecDemo begin-------------------");

    IH264Decoder *pDecoder = IH264Decoder::CreateDecoder();
    pDecoder->InitDecoder();

    FILE *fp_in = fopen("/mnt/sdcard/input.h264", "rb");
    if (fp_in == NULL) {
        ALOGE("fopen in_file fail!");
        return -1;
    }

    FILE *fp_out = fopen("/mnt/sdcard/output.yuv", "wb");
    if (fp_out == NULL) {
        ALOGE("fopen out_file fail!");
        return -1;
    }

    NALU_t *nalu;
    int buffersize = 1000000;   // max bs size: 1MB
    nalu = (NALU_t*)calloc (1, sizeof (NALU_t));
    if (nalu == NULL) {
        ALOGE("Alloc NALU fail!");
        return -1;
    }
    nalu->max_size=buffersize;
    nalu->buf = (unsigned char*)calloc (buffersize, sizeof (char));
    if (nalu->buf == NULL) {
        free(nalu);
        ALOGE("Alloc NALU buf fail!");
        return 0;
    }

    uint32_t yuv_w = 1920;
    uint32_t yuv_h = 1080;
    uint32_t yuv_sz = yuv_w * yuv_h * 3 >> 1;

    uint8_t * yuv_buf = (uint8_t *)malloc(yuv_sz);
    uint32_t dec_out_w, dec_out_h;

    status_t dec_ret;
    int data_len;
    while (!feof(fp_in)) {
        data_len = GetAnnexbNALU(nalu, fp_in);
        //ALOGD("data_len:%d, %#x, %#x, %#x, %#x, %#x", data_len, nalu->buf[0], nalu->buf[1], nalu->buf[2], nalu->buf[3], nalu->buf[4]);
        dec_ret = pDecoder->DecodeOneFrame(nalu->buf, nalu->len, yuv_buf, &dec_out_w, &dec_out_h);
        if (dec_ret == OK) {
            fwrite(yuv_buf, 1, yuv_sz, fp_out);
        } else if ((dec_out_w!=yuv_w) || (dec_out_h!=yuv_h)) {
            yuv_w = dec_out_w;
            yuv_h = dec_out_h;
            ALOGD("decode sps! dec_out_w=%d, dec_out_h=%d", dec_out_w, dec_out_h);
            free(yuv_buf);
            yuv_sz = dec_out_w * dec_out_h * 3 >> 1;
            yuv_buf = (uint8_t *)malloc(yuv_sz);
        }
    }
    pDecoder->DestroyDecoder();

    free(nalu->buf);
    free(nalu);
    free(yuv_buf);

    fclose(fp_in);
    fclose(fp_out);

    ALOGI("--------------IttiamH264DecDemo end-------------------");
    return 0;
}