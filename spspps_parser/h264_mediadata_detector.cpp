/**
 * H.264 MediaData Detector
 *
 * Author: zjz1988314@163.com
 * Date:   2020-10-11
 * https://www.cnblogs.com/Dreaming-in-Gottingen/
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "bs_read.h"

typedef enum {
    NALU_TYPE_SLICE    = 1,
    NALU_TYPE_DPA      = 2,
    NALU_TYPE_DPB      = 3,
    NALU_TYPE_DPC      = 4,
    NALU_TYPE_IDR      = 5,
    NALU_TYPE_SEI      = 6,
    NALU_TYPE_SPS      = 7,
    NALU_TYPE_PPS      = 8,
    NALU_TYPE_AUD      = 9,
    NALU_TYPE_EOSEQ    = 10,
    NALU_TYPE_EOSTREAM = 11,
    NALU_TYPE_FILL     = 12,
} NaluType;

typedef enum {
    FRAME_TYPE_I  = 15,
    FRAME_TYPE_P  = 16,
    FRAME_TYPE_B  = 17
} FrameType;

typedef enum {
    NALU_PRIORITY_DISPOSABLE = 0,
    NALU_PRIRITY_LOW         = 1,
    NALU_PRIORITY_HIGH       = 2,
    NALU_PRIORITY_HIGHEST    = 3
} NaluPriority;


typedef struct {
    int startcodeprefix_len;         //! 4 for parameter sets and first slice in picture, 3 for everything else (suggested)
    unsigned len;                    //! Length of the NAL unit (Excluding the start code, which does not belong to the NALU)
    unsigned max_size;               //! Nal Unit Buffer size
    int forbidden_bit;               //! should be always FALSE
    int nal_reference_idc;           //! NALU_PRIORITY_xxxx
    int nal_unit_type;               //! NALU_TYPE_xxxx
    unsigned char *buf;              //! contains the first byte followed by the EBSP
    unsigned char frame_type;        //! nalu frame type (I/P/B)
} NALU_t;

//! Boolean Type
#ifdef FALSE
#  define Boolean unsigned int
#else
typedef enum {
  FALSE,
  TRUE
} Boolean;
#endif

#define MAXIMUMVALUEOFcpb_cnt   32
typedef struct {
  unsigned  cpb_cnt;                                            // ue(v)
  unsigned  bit_rate_scale;                                     // u(4)
  unsigned  cpb_size_scale;                                     // u(4)
  //for (SchedSelIdx=0; SchedSelIdx<=cpb_cnt; SchedSelIdx++)
    unsigned  bit_rate_value [MAXIMUMVALUEOFcpb_cnt];           // ue(v)
    unsigned  cpb_size_value[MAXIMUMVALUEOFcpb_cnt];            // ue(v)
    unsigned  vbr_cbr_flag[MAXIMUMVALUEOFcpb_cnt];              // u(1)
  unsigned  initial_cpb_removal_delay_length_minus1;            // u(5)
  unsigned  cpb_removal_delay_length_minus1;                    // u(5)
  unsigned  dpb_output_delay_length_minus1;                     // u(5)
  unsigned  time_offset_length;                                 // u(5)
} hrd_parameters_t;

typedef struct {
  Boolean      aspect_ratio_info_present_flag;                  // u(1)
    unsigned     aspect_ratio_idc;                              // u(8)
      unsigned     sar_width;                                   // u(16)
      unsigned     sar_height;                                  // u(16)
  Boolean      overscan_info_present_flag;                      // u(1)
    Boolean      overscan_appropriate_flag;                     // u(1)
  Boolean      video_signal_type_present_flag;                  // u(1)
    unsigned     video_format;                                  // u(3)
    Boolean      video_full_range_flag;                         // u(1)
    Boolean      colour_description_present_flag;               // u(1)
      unsigned     colour_primaries;                            // u(8)
      unsigned     transfer_characteristics;                    // u(8)
      unsigned     matrix_coefficients;                         // u(8)
  Boolean      chroma_location_info_present_flag;               // u(1)
    unsigned     chroma_location_frame;                         // ue(v)
    unsigned     chroma_location_field;                         // ue(v)
  Boolean      timing_info_present_flag;                        // u(1)
    unsigned     num_units_in_tick;                             // u(32)
    unsigned     time_scale;                                    // u(32)
    Boolean      fixed_frame_rate_flag;                         // u(1)
  Boolean      nal_hrd_parameters_present_flag;                 // u(1)
    hrd_parameters_t nal_hrd_parameters;                        // hrd_paramters_t
  Boolean      vcl_hrd_parameters_present_flag;                 // u(1)
    hrd_parameters_t vcl_hrd_parameters;                        // hrd_paramters_t
  // if ((nal_hrd_parameters_present_flag || (vcl_hrd_parameters_present_flag))
    Boolean      low_delay_hrd_flag;                            // u(1)
  Boolean      pic_struct_present_flag;                         // u(1)
  Boolean      bitstream_restriction_flag;                      // u(1)
    Boolean      motion_vectors_over_pic_boundaries_flag;       // u(1)
    unsigned     max_bytes_per_pic_denom;                       // ue(v)
    unsigned     max_bits_per_mb_denom;                         // ue(v)
    unsigned     log2_max_mv_length_vertical;                   // ue(v)
    unsigned     log2_max_mv_length_horizontal;                 // ue(v)
    unsigned     max_dec_frame_reordering;                      // ue(v)
    unsigned     max_dec_frame_buffering;                       // ue(v)
} vui_seq_parameters_t;

#define MAXnum_ref_frames_in_pic_order_cnt_cycle  256
typedef struct {
  Boolean   Valid;                  // indicates the parameter set is valid
  unsigned  profile_idc;                                        // u(8)
  Boolean   constrained_set0_flag;                              // u(1)
  Boolean   constrained_set1_flag;                              // u(1)
  Boolean   constrained_set2_flag;                              // u(1)

  unsigned  level_idc;                                          // u(8)
  unsigned  seq_parameter_set_id;                               // ue(v)
  // if( profile_idc == 100 || profile_idc == 110 ||
  //     profile_idc == 122 || profile_idc == 144 )
    unsigned chroma_format_idc;                                 // ue(v)
    // if( chroma_format_idc == 3 )
      Boolean residual_colour_transform_flag;                   // u(1)
    unsigned bit_depth_luma_minus8;                             // ue(v)
    unsigned bit_depth_chroma_minus8;                           // ue(v)
    Boolean qpprime_y_zero_transform_bypass_flag;               // u(1)
    Boolean seq_scaling_matrix_present_flag;                    // u(1)
    // if( seq_scaling_matrix_present_flag )
    //   for( i=0; i<8; i++)
      Boolean seq_scaling_list_present_flag[8];                 // u(1)

  unsigned  log2_max_frame_num_minus4;                          // ue(v)
  unsigned  pic_order_cnt_type;
  // if( pic_order_cnt_type == 0 )
    unsigned log2_max_pic_order_cnt_lsb_minus4;                 // ue(v)
  // else if( pic_order_cnt_type == 1 )
    Boolean delta_pic_order_always_zero_flag;                   // u(1)
    int     offset_for_non_ref_pic;                             // se(v)
    int     offset_for_top_to_bottom_field;                     // se(v)
    unsigned  num_ref_frames_in_pic_order_cnt_cycle;            // ue(v)
    // for( i=0; i<num_ref_frames_in_pic_order_cnt_cycle; i++ )
      int   offset_for_ref_frame[MAXnum_ref_frames_in_pic_order_cnt_cycle];   // se(v)
  unsigned  num_ref_frames;                                     // ue(v)
  Boolean   gaps_in_frame_num_value_allowed_flag;               // u(1)
  unsigned  pic_width_in_mbs_minus1;                            // ue(v)
  unsigned  pic_height_in_map_units_minus1;                     // ue(v)
  Boolean   frame_mbs_only_flag;                                // u(1)
  // if( !frame_mbs_only_flag )
    Boolean   mb_adaptive_frame_field_flag;                     // u(1)
  Boolean   direct_8x8_inference_flag;                          // u(1)
  Boolean   frame_cropping_flag;                                // u(1)
  // if( frame_cropping_flag )
    unsigned  frame_cropping_rect_left_offset;                  // ue(v)
    unsigned  frame_cropping_rect_right_offset;                 // ue(v)
    unsigned  frame_cropping_rect_top_offset;                   // ue(v)
    unsigned  frame_cropping_rect_bottom_offset;                // ue(v)
  Boolean   vui_parameters_present_flag;                        // u(1)
  // if( vui_parameters_present_flag )
    vui_seq_parameters_t vui_seq_parameters;                    //
} seq_parameter_set_rbsp_t;

#define MAXnum_slice_groups_minus1  8
typedef struct {
  Boolean   Valid;                  // indicates the parameter set is valid
  unsigned  pic_parameter_set_id;                               // ue(v)
  unsigned  seq_parameter_set_id;                               // ue(v)
  Boolean   entropy_coding_mode_flag;                           // u(1)
  // if( pic_order_cnt_type < 2 )  in the sequence parameter set
  Boolean      pic_order_present_flag;                          // u(1)
  unsigned  num_slice_groups_minus1;                            // ue(v)
  // if( num_slice_groups_minus1 > 0 )
    unsigned  slice_group_map_type;                             // ue(v)
    // if( slice_group_map_type == 0 )
      unsigned  run_length_minus1[MAXnum_slice_groups_minus1];  // ue(v)
    // else if( slice_group_map_type == 2 )
      unsigned  top_left[MAXnum_slice_groups_minus1];           // ue(v)
      unsigned  bottom_right[MAXnum_slice_groups_minus1];       // ue(v)
    // else if( slice_group_map_type == 3 || 4 || 5 )
      Boolean   slice_group_change_direction_flag;              // u(1)
      unsigned  slice_group_change_rate_minus1;                 // ue(v)
    // else if( slice_group_map_type == 6 )
      unsigned  num_slice_group_map_units_minus1;               // ue(v)
      unsigned  *slice_group_id;                                // complete MBAmap u(v)
  unsigned  num_ref_idx_l0_active_minus1;                       // ue(v)
  unsigned  num_ref_idx_l1_active_minus1;                       // ue(v)
  Boolean   weighted_pred_flag;                                 // u(1)
  unsigned  weighted_bipred_idc;                                // u(2)
  int       pic_init_qp_minus26;                                // se(v)
  int       pic_init_qs_minus26;                                // se(v)
  int       chroma_qp_index_offset;                             // se(v)
  Boolean   deblocking_filter_control_present_flag;             // u(1)
  Boolean   constrained_intra_pred_flag;                        // u(1)
  Boolean   redundant_pic_cnt_present_flag;                     // u(1)
} pic_parameter_set_rbsp_t;


FILE *h264bitstream = NULL;                // the bit stream file

seq_parameter_set_rbsp_t gCurSps;
pic_parameter_set_rbsp_t gCurPps;

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
int GetAnnexbNALU (NALU_t *nalu)
{
    int len3=0, len4=0;
    int pos = 0;
    int StartCodeFound, rewind;
    unsigned char *Buf = nalu->buf;

    nalu->startcodeprefix_len=3;

    // find current startcode
    if (3 != fread (Buf, 1, 3, h264bitstream)) {
        return 0;
    }
    len3 = FindStartCodeLen3 (Buf);
    if (len3 == 0) {
        if(1 != fread(Buf+3, 1, 1, h264bitstream)){
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
        if (feof (h264bitstream)){
            nalu->len = (pos-1)-nalu->startcodeprefix_len;
            memcpy (nalu->buf, &Buf[nalu->startcodeprefix_len], nalu->len);
            nalu->forbidden_bit = nalu->buf[0] & 0x80; //1 bit
            nalu->nal_reference_idc = nalu->buf[0] & 0x60; // 2 bit
            nalu->nal_unit_type = (nalu->buf[0]) & 0x1f;// 5 bit
            return pos-1;
        }
        Buf[pos++] = fgetc (h264bitstream);
        len4 = FindStartCodeLen4(&Buf[pos-4]);
        if(len4 != 1)
            len3 = FindStartCodeLen3(&Buf[pos-3]);
        StartCodeFound = (len3 == 1 || len4 == 1);
    }

    // Here, we have found another start code (and read length of startcode bytes more than we should
    // have.  Hence, go back in the file
    rewind = (len4 == 1)? -4 : -3;

    if (0 != fseek (h264bitstream, rewind, SEEK_CUR)){
        printf("GetAnnexbNALU: Cannot fseek in the bit stream file");
    }

    // Here the Start code, the complete NALU, and the next start code is in the Buf.
    // The size of Buf is pos, pos+rewind are the number of bytes excluding the next
    // start code, and (pos+rewind)-startcodeprefix_len is the size of the NALU excluding the start code

    nalu->len = (pos+rewind)-nalu->startcodeprefix_len;
    memcpy (nalu->buf, &Buf[nalu->startcodeprefix_len], nalu->len); // move backward for startcodeprefix_len
    nalu->forbidden_bit = nalu->buf[0] & 0x80; //1 bit
    nalu->nal_reference_idc = nalu->buf[0] & 0x60; // 2 bit
    nalu->nal_unit_type = (nalu->buf[0]) & 0x1f;// 5 bit

    return (pos+rewind);
}

static int GetFrameType(NALU_t * nal)
{
    bs_t s;
    int frame_type = 0;
    static FILE *myout = fopen("slice_header.txt", "wb");
    static int nalu_idx = 0;

    bs_init(&s, nal->buf+1, nal->len - 1);

    if (nal->nal_unit_type == NALU_TYPE_SLICE || nal->nal_unit_type ==  NALU_TYPE_IDR)
    {
        // decode slice header
        fprintf(myout, "------------------[nalu_idx=%d] slice header info-----------------\n", nalu_idx);

        /* read the first part of the header (only the pic_parameter_set_id) */
        int start_mb_nr = bs_read_ue(&s); // first mb nr in slice
        fprintf(myout, "SH: start_mb_nr=%d\n", start_mb_nr);
        /* picture/slice type */
        frame_type = bs_read_ue(&s);
        fprintf(myout, "SH: frame_type=%d\n", frame_type);
        switch(frame_type)
        {
        case 0: case 5: /* P */
            nal->frame_type = FRAME_TYPE_P;
            break;
        case 1: case 6: /* B */
            nal->frame_type = FRAME_TYPE_B;
            break;
        case 2: case 7: /* I */
            nal->frame_type = FRAME_TYPE_I;
            break;
        case 3: case 8: /* SP */
            nal->frame_type = FRAME_TYPE_P;
            break;
        case 4: case 9: /* SI */
            nal->frame_type = FRAME_TYPE_I;
            break;
        default:
            printf("unknown frame type! nalu_data[%#x,%#x,%#x,%#x]\n", nal->buf[0], nal->buf[1], nal->buf[2], nal->buf[3]);
            break;
        }
        int pps_id = bs_read_ue(&s);
        fprintf(myout, "SH: pps_id=%d\n", pps_id);

        /* read the scond part of the header (without the pic_parameter_set_id) */
        int frame_nr = bs_read(&s, gCurSps.log2_max_frame_num_minus4 + 4);
        fprintf(myout, "SH: frame_nr=%d\n", frame_nr);
        if (gCurSps.frame_mbs_only_flag) {
            // only support frame
        } else {
            // not support field
        }

        if (nal->nal_unit_type == NALU_TYPE_IDR) {
            int idr_pic_id = bs_read_ue(&s);
            fprintf(myout, "SH: idr_pic_id=%d\n", idr_pic_id);
        }

        if (gCurSps.pic_order_cnt_type == 0) {
            int pic_order_cnt_lsb = bs_read(&s, gCurSps.log2_max_pic_order_cnt_lsb_minus4 + 4);
            fprintf(myout, "SH: pic_order_cnt_lsb=%d\n", pic_order_cnt_lsb);
            if (gCurPps.pic_order_present_flag && gCurSps.frame_mbs_only_flag) {
                int delta_pic_order_cnt_bottom = bs_read_se(&s);
                fprintf(myout, "SH: delta_pic_order_cnt_bottom=%d\n", delta_pic_order_cnt_bottom);
            }
        }

        nalu_idx++;
    }
    else
    {
        nal->frame_type = nal->nal_unit_type;
    }

    return 0;
}

static int ParseAndDumpSPSInfo(NALU_t * nal)
{
    bs_t s;
    //FILE *myout = NULL;
    static FILE *myout = fopen("sps_info.txt", "wb");
    static int cnt;

    bs_init(&s, nal->buf+1, nal->len - 1);

    fprintf(myout, "---------------------SPS[%d] info--------------------------\n", cnt++);
    gCurSps.profile_idc = bs_read(&s, 8);
    fprintf(myout, "profile_idc:                            %d\n", gCurSps.profile_idc);

    gCurSps.constrained_set0_flag = (Boolean)bs_read1(&s);
    fprintf(myout, "constrained_set0_flag:                  %d\n", gCurSps.constrained_set0_flag);
    gCurSps.constrained_set1_flag = (Boolean)bs_read1(&s);
    fprintf(myout, "constrained_set1_flag:                  %d\n", gCurSps.constrained_set1_flag);
    gCurSps.constrained_set2_flag = (Boolean)bs_read1(&s);
    fprintf(myout, "constrained_set2_flag:                  %d\n", gCurSps.constrained_set2_flag);
    int reserved_zero = bs_read(&s, 5);
    fprintf(myout, "reserved_zero:                          %d\n", reserved_zero);
    //assert(reserved_zero == 0); // some bs will assert

    gCurSps.level_idc = bs_read(&s, 8);
    fprintf(myout, "level_idc:                              %d\n", gCurSps.level_idc);
    gCurSps.seq_parameter_set_id = bs_read_ue(&s);
    fprintf(myout, "seq_parameter_set_id:                   %d\n", gCurSps.seq_parameter_set_id);

    if (gCurSps.profile_idc==100 || gCurSps.profile_idc==110 || gCurSps.profile_idc==122 || gCurSps.profile_idc==144) {
        gCurSps.chroma_format_idc = bs_read_ue(&s);
        fprintf(myout, "  chroma_format_idc:                          %d\n", gCurSps.chroma_format_idc);
        if (gCurSps.chroma_format_idc == 3) {
            gCurSps.residual_colour_transform_flag = (Boolean)bs_read1(&s);
            fprintf(myout, "    residual_colour_transform_flag:                   %d\n", gCurSps.residual_colour_transform_flag);
        }
        gCurSps.bit_depth_luma_minus8 = bs_read_ue(&s);
        fprintf(myout, "  bit_depth_luma_minus8:                      %d\n", gCurSps.bit_depth_luma_minus8);
        gCurSps.bit_depth_chroma_minus8 = bs_read_ue(&s);
        fprintf(myout, "  bit_depth_chroma_minus8:                    %d\n", gCurSps.bit_depth_chroma_minus8);
        gCurSps.qpprime_y_zero_transform_bypass_flag = (Boolean)bs_read1(&s);
        fprintf(myout, "  qpprime_y_zero_transform_bypass_flag:       %d\n", gCurSps.qpprime_y_zero_transform_bypass_flag);
        gCurSps.seq_scaling_matrix_present_flag = (Boolean)bs_read1(&s);
        fprintf(myout, "  seq_scaling_matrix_present_flag:            %d\n", gCurSps.seq_scaling_matrix_present_flag);
        if (gCurSps.seq_scaling_matrix_present_flag) {
            for (int i=0; i<8; i++) {
                gCurSps.seq_scaling_list_present_flag[i] = (Boolean)bs_read1(&s);
                fprintf(myout, "    seq_scaling_list_present_flag:                %d\n", gCurSps.seq_scaling_list_present_flag[i]);
            }
        }
    }

    gCurSps.log2_max_frame_num_minus4 = bs_read_ue(&s);
    fprintf(myout, "log2_max_frame_num_minus4:              %d\n", gCurSps.log2_max_frame_num_minus4);

    gCurSps.pic_order_cnt_type = bs_read_ue(&s);
    fprintf(myout, "pic_order_cnt_type:                     %d\n", gCurSps.pic_order_cnt_type);
    if (gCurSps.pic_order_cnt_type == 0) {
        gCurSps.log2_max_pic_order_cnt_lsb_minus4 = bs_read_ue(&s);
        fprintf(myout, "  log2_max_pic_order_cnt_lsb_minus4:          %d\n", gCurSps.log2_max_pic_order_cnt_lsb_minus4);
    } else if (gCurSps.pic_order_cnt_type == 1) {
        gCurSps.delta_pic_order_always_zero_flag = (Boolean)bs_read1(&s);
        gCurSps.offset_for_non_ref_pic = bs_read_se(&s);
        gCurSps.offset_for_top_to_bottom_field = bs_read_se(&s);
        gCurSps.num_ref_frames_in_pic_order_cnt_cycle = bs_read_ue(&s);
        for (int i=0; i<gCurSps.num_ref_frames_in_pic_order_cnt_cycle; i++)
            gCurSps.offset_for_ref_frame[i] = bs_read_se(&s);
    }

    gCurSps.num_ref_frames = bs_read_ue(&s);
    fprintf(myout, "num_ref_frames:                         %d\n", gCurSps.num_ref_frames);
    gCurSps.gaps_in_frame_num_value_allowed_flag = (Boolean)bs_read1(&s);
    fprintf(myout, "gaps_in_frame_num_value_allowed_flag:   %d\n", gCurSps.gaps_in_frame_num_value_allowed_flag);

    gCurSps.pic_width_in_mbs_minus1 = bs_read_ue(&s);
    fprintf(myout, "pic_width_in_mbs_minus1:                %d\n", gCurSps.pic_width_in_mbs_minus1);
    gCurSps.pic_height_in_map_units_minus1 = bs_read_ue(&s);
    fprintf(myout, "pic_height_in_map_units_minus1:         %d\n", gCurSps.pic_height_in_map_units_minus1);

    gCurSps.frame_mbs_only_flag = (Boolean)bs_read1(&s);
    fprintf(myout, "frame_mbs_only_flag:                    %d\n", gCurSps.frame_mbs_only_flag);
    if (!gCurSps.frame_mbs_only_flag) {
        gCurSps.mb_adaptive_frame_field_flag = (Boolean)bs_read1(&s);
        fprintf(myout, "  mb_adaptive_frame_field_flag:               %d\n", gCurSps.mb_adaptive_frame_field_flag);
    }
    gCurSps.direct_8x8_inference_flag = (Boolean)bs_read1(&s);
    fprintf(myout, "direct_8x8_inference_flag:              %d\n", gCurSps.direct_8x8_inference_flag);

    gCurSps.frame_cropping_flag = (Boolean)bs_read1(&s);
    fprintf(myout, "frame_cropping_flag:                    %d\n", gCurSps.frame_cropping_flag);
    if (gCurSps.frame_cropping_flag) {
        gCurSps.frame_cropping_rect_left_offset = bs_read_ue(&s);
        fprintf(myout, "  frame_cropping_rect_left_offset:            %d\n", gCurSps.frame_cropping_rect_left_offset);
        gCurSps.frame_cropping_rect_right_offset = bs_read_ue(&s);
        fprintf(myout, "  frame_cropping_rect_right_offset:           %d\n", gCurSps.frame_cropping_rect_right_offset);
        gCurSps.frame_cropping_rect_top_offset = bs_read_ue(&s);
        fprintf(myout, "  frame_cropping_rect_top_offset:             %d\n", gCurSps.frame_cropping_rect_top_offset);
        gCurSps.frame_cropping_rect_bottom_offset = bs_read_ue(&s);
        fprintf(myout, "  frame_cropping_rect_bottom_offset:          %d\n", gCurSps.frame_cropping_rect_bottom_offset);
    }

    // vui
    gCurSps.vui_parameters_present_flag = (Boolean)bs_read1(&s);
    fprintf(myout, "vui_parameters_present_flag:            %d\n", gCurSps.vui_parameters_present_flag);
    if (gCurSps.vui_parameters_present_flag) {
        fprintf (myout, "VUI sequence parameters present but not supported, ignored, proceeding to next NALU\n");
    }
    gCurSps.Valid = TRUE;
    fprintf(myout, "-------------------------------------------------------\n");

    return 0;
}

static int ParseAndDumpPPSInfo(NALU_t * nal)
{
    bs_t s;
    //FILE *myout = NULL;
    static FILE *myout = fopen("pps_info.txt", "wb");
    static int cnt;

    bs_init(&s, nal->buf+1, nal->len - 1);

    fprintf(myout, "---------------------PPS[%d] info--------------------------\n", cnt++);
    gCurPps.pic_parameter_set_id = bs_read_ue(&s);
    fprintf(myout, "pic_parameter_set_id:          %u\n", gCurPps.pic_parameter_set_id);
    gCurPps.seq_parameter_set_id = bs_read_ue(&s);
    fprintf(myout, "seq_parameter_set_id:          %u\n", gCurPps.seq_parameter_set_id);
    gCurPps.entropy_coding_mode_flag = (Boolean)bs_read1(&s);
    fprintf(myout, "entropy_coding_mode_flag:      %u\n", gCurPps.entropy_coding_mode_flag);
    gCurPps.pic_order_present_flag = (Boolean)bs_read1(&s);
    fprintf(myout, "pic_order_present_flag:        %u\n", gCurPps.pic_order_present_flag);
    gCurPps.num_slice_groups_minus1 = bs_read_ue(&s);
    fprintf(myout, "num_slice_groups_minus1:       %u\n", gCurPps.num_slice_groups_minus1);
    if (gCurPps.num_slice_groups_minus1 > 0) {
        gCurPps.slice_group_map_type = bs_read_ue(&s);
        unsigned type = gCurPps.slice_group_map_type;
        if (type == 0) {
            for (int i=0; i<MAXnum_slice_groups_minus1; i++)
                gCurPps.run_length_minus1[i] = bs_read_ue(&s);
        } else if (type == 2) {
            for (int i=0; i<MAXnum_slice_groups_minus1; i++) {
                gCurPps.top_left[i] = bs_read_ue(&s);
                gCurPps.bottom_right[i] = bs_read_ue(&s);
            }
        } else if (type==3 || type==4 || type==5) {
            gCurPps.slice_group_change_direction_flag = (Boolean)bs_read1(&s);
            gCurPps.slice_group_change_rate_minus1 = bs_read_ue(&s);
        } else if (type == 6) {
            gCurPps.num_slice_group_map_units_minus1 = bs_read_ue(&s);
            gCurPps.slice_group_id = NULL; // need to be Fixed
        }
    }
    gCurPps.num_ref_idx_l0_active_minus1 = bs_read_ue(&s);
    fprintf(myout, "num_ref_idx_l0_active_minus1:  %u\n", gCurPps.num_ref_idx_l0_active_minus1);
    gCurPps.num_ref_idx_l1_active_minus1 = bs_read_ue(&s);
    fprintf(myout, "num_ref_idx_l1_active_minus1:  %u\n", gCurPps.num_ref_idx_l1_active_minus1);
    gCurPps.weighted_pred_flag = (Boolean)bs_read1(&s);
    fprintf(myout, "weighted_pred_flag:            %u\n", gCurPps.weighted_pred_flag);
    gCurPps.weighted_bipred_idc = bs_read(&s, 2);
    fprintf(myout, "weighted_bipred_idc:           %u\n", gCurPps.weighted_bipred_idc);
    gCurPps.pic_init_qp_minus26 = bs_read_se(&s);
    fprintf(myout, "pic_init_qp_minus26:          %2d\n", gCurPps.pic_init_qp_minus26);
    gCurPps.pic_init_qs_minus26 = bs_read_se(&s);
    fprintf(myout, "pic_init_qs_minus26:          %2d\n", gCurPps.pic_init_qs_minus26);
    gCurPps.chroma_qp_index_offset = bs_read_se(&s);
    fprintf(myout, "chroma_qp_index_offset:       %2d\n", gCurPps.chroma_qp_index_offset);
    gCurPps.deblocking_filter_control_present_flag = (Boolean)bs_read1(&s);
    fprintf(myout, "deblocking_filter_control_present_flag:%u\n", gCurPps.deblocking_filter_control_present_flag);
    gCurPps.constrained_intra_pred_flag = (Boolean)bs_read1(&s);
    fprintf(myout, "constrained_intra_pred_flag:   %u\n", gCurPps.constrained_intra_pred_flag);
    gCurPps.redundant_pic_cnt_present_flag = (Boolean)bs_read1(&s);
    fprintf(myout, "redundant_pic_cnt_present_flag:%u\n", gCurPps.redundant_pic_cnt_present_flag);

    gCurPps.Valid = TRUE;
    fprintf(myout, "-------------------------------------------------------\n");

    return 0;
}

static int ParseAndDumpSEIInfo(NALU_t * nal)
{
    return 0;
}

/**
 * Analysis H.264 Bitstream
 * @param url    Location of input H.264 bitstream file.
 */
int simplest_h264_parser(char *url){

    NALU_t *nalu;
    int buffersize = 1000000;   // max bs size: 1MB

    //FILE *myout=fopen("output_log.txt","wb+");
    FILE *myout=stdout;

    h264bitstream=fopen(url, "rb");
    if (h264bitstream==NULL){
        printf("Open file error\n");
        return -1;
    }

    nalu = (NALU_t*)calloc (1, sizeof (NALU_t));
    if (nalu == NULL){
        printf("Alloc NALU fail!\n");
        return -1;
    }

    nalu->max_size=buffersize;
    nalu->buf = (unsigned char*)calloc (buffersize, sizeof (char));
    if (nalu->buf == NULL) {
        free(nalu);
        printf ("Alloc NALU buf fail!\n");
        return 0;
    }

    int data_offset=0;
    int nal_num=0;
    printf("------+---------+-- NALU Table -----+---------+-------+\n");
    printf("| NUM |    POS  |   IDC  | NAL_TYPE |   LEN   | FRAME |\n");
    printf("+-----+---------+--------+----------+---------+-------+\n");

    while(!feof(h264bitstream))
    {
        int data_lenth;
        data_lenth=GetAnnexbNALU(nalu);
        GetFrameType(nalu);

        char nal_type_str[16]={0};
        switch(nalu->nal_unit_type) {
            case NALU_TYPE_SLICE:sprintf(nal_type_str,"SLICE");break;
            case NALU_TYPE_DPA:sprintf(nal_type_str,"DPA");break;
            case NALU_TYPE_DPB:sprintf(nal_type_str,"DPB");break;
            case NALU_TYPE_DPC:sprintf(nal_type_str,"DPC");break;
            case NALU_TYPE_IDR:sprintf(nal_type_str,"IDR");break;
            case NALU_TYPE_SEI:sprintf(nal_type_str,"SEI");break;
            case NALU_TYPE_SPS:sprintf(nal_type_str,"SPS");break;
            case NALU_TYPE_PPS:sprintf(nal_type_str,"PPS");break;
            case NALU_TYPE_AUD:sprintf(nal_type_str,"AUD");break;
            case NALU_TYPE_EOSEQ:sprintf(nal_type_str,"EOSEQ");break;
            case NALU_TYPE_EOSTREAM:sprintf(nal_type_str,"EOSTREAM");break;
            case NALU_TYPE_FILL:sprintf(nal_type_str,"FILL");break;
        }

        char idc_str[16]={0};
        switch(nalu->nal_reference_idc>>5) {
            case NALU_PRIORITY_DISPOSABLE:sprintf(idc_str,"DISPOS");break;
            case NALU_PRIRITY_LOW:sprintf(idc_str,"LOW");break;
            case NALU_PRIORITY_HIGH:sprintf(idc_str,"HIGH");break;
            case NALU_PRIORITY_HIGHEST:sprintf(idc_str,"HIGHEST");break;
        }

        char frame_type_str[16]={0};
        switch(nalu->frame_type) {
            case FRAME_TYPE_I:sprintf(frame_type_str,"I");break;
            case FRAME_TYPE_P:sprintf(frame_type_str,"P");break;
            case FRAME_TYPE_B:sprintf(frame_type_str,"B");break;
            default:sprintf(frame_type_str,"");break;
        }

        fprintf(myout,"|%5d| %#8x| %7s| %9s| %8d| %5s |\n", nal_num, data_offset, idc_str, nal_type_str, nalu->len, frame_type_str);

        data_offset = data_offset + data_lenth;
        nal_num++;

        // parse sps & pps & sei
        switch(nalu->nal_unit_type) {
            case NALU_TYPE_SPS:
                ParseAndDumpSPSInfo(nalu);
                break;
            case NALU_TYPE_PPS:
                ParseAndDumpPPSInfo(nalu);
                break;
            case NALU_TYPE_SEI:
                ParseAndDumpSEIInfo(nalu);
                break;
        }
    }

    if (nalu) {
        if (nalu->buf) {
            free(nalu->buf);
            nalu->buf=NULL;
        }
        free(nalu);
    }
    fclose(h264bitstream);

    return 0;
}

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        puts("error input! usage: cmd url");
        return -1;
    }
    simplest_h264_parser(argv[1]);

    return 0;
}
