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
    SLICE_TYPE_I  = 15,
    SLICE_TYPE_P  = 16,
    SLICE_TYPE_B  = 17
} FrameType;

typedef enum {
    NALU_PRIORITY_DISPOSABLE = 0,
    NALU_PRIRITY_LOW         = 1,
    NALU_PRIORITY_HIGH       = 2,
    NALU_PRIORITY_HIGHEST    = 3
} NaluPriority;

#define PROFILE_H264_BASELINE             66
#define PROFILE_H264_CONSTRAINED_BASELINE 66 // with constraint_set1_flag=1
#define PROFILE_H264_MAIN                 77
#define PROFILE_H264_EXTENDED             88
#define PROFILE_H264_HIGH                 100
#define PROFILE_H264_HIGH_10              110
#define PROFILE_H264_HIGH_422             122
#define PROFILE_H264_HIGH_444             144


#define MAX_REFERENCE_PICTURES 16

enum {
  LIST_0 = 0,
  LIST_1 = 1,
  BI_PRED = 2,
  BI_PRED_L0 = 3,
  BI_PRED_L1 = 4
};

typedef struct {
    int startcodeprefix_len;         //! 4 for parameter sets and first slice in picture, 3 for everything else (suggested)
    unsigned len;                    //! Length of the NAL unit (Excluding the start code, which does not belong to the NALU)
    unsigned max_size;               //! Nal Unit Buffer size
    int forbidden_bit;               //! should be always FALSE
    int nal_reference_idc;           //! NALU_PRIORITY_xxxx
    int nal_unit_type;               //! NALU_TYPE_xxxx
    unsigned char *buf;              //! contains the first byte followed by the EBSP
    unsigned char slice_type;        //! nalu frame type (I/P/B)
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

typedef struct {
    unsigned first_mb_in_slice;         // ue(v)
    unsigned slice_type;                // ue(v)
    unsigned pic_parameter_set_id;      // ue(v)
    int frame_num;                      // u(v)
    //if (!frame_mbs_only_flag)
      Boolean field_pic_flag;           // u(1)
      //if (field_pic_flag)
        Boolean bottom_field_flag;      // u(1)
    //if (nal_unit_type == 5)
      unsigned idr_pic_id;              // ue(v)
    //if (pic_order_cnt_type == 0)
      int pic_order_cnt_lsb;            // u(v)
      //if (pic_order_present_flag && !field_pic_flag)
        int delta_pic_order_cnt_bottom; // se(v)
    //if (pic_order_cnt_type==1 && !delta_pic_order_always_zero_flag)
      //if (pic_order_present_flag && !field_pic_flag)
      int delta_pic_order_cnt[2];       // se(v),se(v)
    //if(redundant_pic_cnt_present_flag)
      unsigned redundant_pic_cnt;       // ue(v)
    //if (slice_type == B)
      Boolean direct_spatial_mv_pred_flag;          // u(1)
    //if (slice_type == P || slice_type == SP || slice_type == B)
      Boolean num_ref_idx_active_override_flag;     // u(1)
      //if (num_ref_idx_active_override_flag)
        unsigned num_ref_idx_l0_active_minus1;      // ue(v)
        //if (slice_type == B)
          unsigned num_ref_idx_l1_active_minus1;    // ue(v)

    //ref_pic_list_reordering()
      //if (slice_type != I && slice_type != SI)
        Boolean ref_pic_list_reordering_flag_l0;    // u(1)
        //if (ref_pic_list_reordering_flag_l0)
          //do {
            unsigned remapping_of_pic_nums_idc_l0[MAX_REFERENCE_PICTURES];  // ue(v)
            //if(remapping_of_pic_nums_idc_l0 == 0 || 1)
              unsigned abs_diff_pic_num_minus1_l0[MAX_REFERENCE_PICTURES];  // ue(v)
            //if(remapping_of_pic_nums_idc_l0 == 2)
              unsigned long_term_pic_idx_l0[MAX_REFERENCE_PICTURES];        // ue(v)
          //while (remapping_of_pic_nums_idc_l0[i] != 3)
      //if (slice_type == B)
        Boolean ref_pic_list_reordering_flag_l1;    // u(1)
        //if (ref_pic_list_reordering_flag_l1)
          //do {
            unsigned remapping_of_pic_nums_idc_l1[MAX_REFERENCE_PICTURES];  // ue(v)
            //if(remapping_of_pic_nums_idc_l1 == 0 || 1)
              unsigned abs_diff_pic_num_minus1_l1[MAX_REFERENCE_PICTURES];  // ue(v)
            //if(remapping_of_pic_nums_idc_l1 == 2)
              unsigned long_term_pic_idx_l1[MAX_REFERENCE_PICTURES];        // ue(v)
          //while (remapping_of_pic_nums_idc_l1[i] != 3)

    //if ((weighted_pred_flag && (slice_type==P || slice_type==SP)) || (weighted_bipred_idc==1 && slice_type==B))
    //  pred_weight_table()
        unsigned luma_log2_weight_denom;        // ue(v)
        unsigned chroma_log2_weight_denom;      // ue(v)
        //for ( i = 0; i <= num_ref_idx_l0_active_minus1; i++ )
          Boolean luma_weight_flag_l0;          // u(1)
          //if (luma_weight_flag_l0)
            //wp_weight[0][i][0];               // se(v)
            int wp_weight[2][MAX_REFERENCE_PICTURES][3];  //[list][index][component], copy from JM
            //wp_offset[0][i][0];               // se(v)
            int wp_offset[2][MAX_REFERENCE_PICTURES][3];  //[list][index][component]
          Boolean chroma_weight_flag_l0;        // u(1)
          //for (j=1; j<3; j++)
            //if (chroma_weight_flag_l0)
             //wp_weight[0][i][j];              // se(v)
             //wp_offset[0][i][j];              // se(v)
        //if (slice_type==B)
          //for (i=0; i<=num_ref_idx_l1_active_minus1; i++)
            Boolean luma_weight_flag_l1;        // u(1)
            //if (luma_weight_flag_l1)
              //wp_weight[1][i][0];             // se(v)
              //wp_offset[1][i][0];             // se(v)
            Boolean chroma_weight_flag_l1;      // u(1)
            //for (j=1; j<3; j++)
              //if (chroma_weight_flag_l1)
                //wp_weight[1][i][j];           // se(v)
                //wp_offset[1][i][j];           // se(v)

    //if (nal_ref_idc != 0)
    //  dec_ref_pic_marking()
        //if (nal_unit_type == 5)
          Boolean no_output_of_prior_pics_flag; // u(1)
          Boolean long_term_reference_flag;     // u(1)
        //else
          Boolean adaptive_ref_pic_marking_mode_flag;   // u(1)
          //if (adaptive_ref_pic_marking_mode_flag)
            //do {
              unsigned memory_management_control_operation;     // ue(v)
              //if (memory_management_control_operation == 1 || 3)
                unsigned difference_of_pic_nums_minus1;         // ue(v)
              //if (memory_management_control_operation == 2)
                unsigned long_term_pic_num;                     // ue(v)
              //if (memory_management_control_operation == 3 || 6)
                unsigned long_term_frame_idx;                   // ue(v)
              //if (memory_management_control_operation == 4)
                unsigned max_long_term_frame_idx_plus1;         // ue(v)
            //while (memory_management_control_operation != 0)

    //if (entropy_coding_mode_flag && slice_type != I && slice_type != SI)
      unsigned cabac_init_idc;            // ue(v)
    int slice_qp_delta;                 // se(v)
    //if (slice_type == SP || slice_type == SI)
      //if (slice_type == SP)
        Boolean sp_for_switch_flag;     // u(1)
      int slice_qs_delta;               // se(v)
    //if (deblocking_filter_control_present_flag)
      unsigned disable_deblocking_filter_idc;   // ue(v)
      //if (disable_deblocking_filter_idc != 1)
        int slice_alpha_c0_offset_div2; // se(v)
        int slice_beta_offset_div2;     // se(v)
    //if (num_slice_groups_minus1 > 0 && slice_group_map_type >= 3 && slice_group_map_type <= 5)
      int slice_group_change_cycle; // u(v)
} slice_header_t;

#define MAXIMUMVALUEOFcpb_cnt   32
typedef struct {
  unsigned  cpb_cnt_minus1;                                     // ue(v)
  unsigned  bit_rate_scale;                                     // u(4)
  unsigned  cpb_size_scale;                                     // u(4)
  //for (SchedSelIdx=0; SchedSelIdx<=cpb_cnt_minus1; SchedSelIdx++)
    unsigned  bit_rate_value_minus1[MAXIMUMVALUEOFcpb_cnt];         // ue(v)
    unsigned  cpb_size_value_minus1[MAXIMUMVALUEOFcpb_cnt];         // ue(v)
    unsigned  cbr_flag[MAXIMUMVALUEOFcpb_cnt];                      // u(1)
  unsigned  initial_cpb_removal_delay_length_minus1;            // u(5)
  unsigned  cpb_removal_delay_length_minus1;                    // u(5)
  unsigned  dpb_output_delay_length_minus1;                     // u(5)
  unsigned  time_offset_length;                                 // u(5)
} hrd_parameters_t;

typedef struct {
  Boolean      aspect_ratio_info_present_flag;                  // u(1)
    unsigned     aspect_ratio_idc;                              // u(8)
    // if (aspect_ratio_idc == Extended_SAR)
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
  Boolean      chroma_loc_info_present_flag;                    // u(1)
    unsigned     chroma_sample_loc_type_top_field;              // ue(v)
    unsigned     chroma_sample_loc_type_bottom_field;           // ue(v)
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
    unsigned     max_num_reorder_frames;                        // ue(v)
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

void EBSPtoRBSP(NALU_t *nalu)
{
    unsigned char* pb = nalu->buf;
    unsigned char* pb_end = nalu->buf + nalu->len;
    while (pb < pb_end) {
        if ((pb[0]==0) && (pb[1]==0) && (pb[2]==3)) {
            unsigned char* tmp = pb + 2;
            while (tmp < pb_end) {
                tmp[0] = tmp[1];
                tmp++;
            }
            pb_end[-1] = 0;
            pb_end--;
            nalu->len -= 1;
            pb = pb + 3;
        }
        pb++;
    }
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

/*!
 ************************************************************************
 * \brief
 *    calculate Ceil(Log2(uiVal))
 ************************************************************************
 */
unsigned CeilLog2( unsigned uiVal)
{
  unsigned uiTmp = uiVal-1;
  unsigned uiRet = 0;

  while( uiTmp != 0 )
  {
    uiTmp >>= 1;
    uiRet++;
  }
  return uiRet;
}

void ref_pic_list_reordering(slice_header_t *pSH, NALU_t *pNal, bs_t *pBS, FILE *pOutFp)
{
    //alloc_ref_pic_list_reordering_buffer(currSlice);
    // slice_type != I && slice_type != SI
    if (pNal->slice_type != SLICE_TYPE_I) {
        fprintf(pOutFp, "  --ref_pic_list_reordering()--\n");
        pSH->ref_pic_list_reordering_flag_l0 = (Boolean)bs_read1(pBS);
        fprintf(pOutFp, "  ref_pic_list_reordering_flag_l0:         %d\n", pSH->ref_pic_list_reordering_flag_l0);
        if (pSH->ref_pic_list_reordering_flag_l0) {
            int i = 0;
            int val;
            do {
                val = pSH->remapping_of_pic_nums_idc_l0[i] = bs_read_ue(pBS);
                fprintf(pOutFp, "    remapping_of_pic_nums_idc_l0[%d]:         %d\n", i, pSH->remapping_of_pic_nums_idc_l0[i]);
                if (val == 0 || val == 1) {
                    pSH->abs_diff_pic_num_minus1_l0[i] = bs_read_ue(pBS);
                    fprintf(pOutFp, "    abs_diff_pic_num_minus1_l0[%d]:           %d\n", i, pSH->abs_diff_pic_num_minus1_l0[i]);
                } else {
                    if (val == 2) {
                        pSH->long_term_pic_idx_l0[i] = bs_read_ue(pBS);
                        fprintf(pOutFp, "    long_term_pic_idx_l0[%d]:                 %d\n", i, pSH->long_term_pic_idx_l0[i]);
                    }
                }
                i++;
            } while (val != 3);
        }
        fprintf(pOutFp, "  --L0 end--\n");
    }

    if (pNal->slice_type == SLICE_TYPE_B) {
        pSH->ref_pic_list_reordering_flag_l1 = (Boolean)bs_read1(pBS);
        fprintf(pOutFp, "  ref_pic_list_reordering_flag_l1:         %d\n", pSH->ref_pic_list_reordering_flag_l1);
        if (pSH->ref_pic_list_reordering_flag_l1) {
            int i = 0;
            int val;
            do {
                val = pSH->remapping_of_pic_nums_idc_l1[i] = bs_read_ue(pBS);
                fprintf(pOutFp, "    remapping_of_pic_nums_idc_l1[%d]:         %d\n", i, pSH->remapping_of_pic_nums_idc_l1[i]);
                if (val == 0 || val == 1) {
                    pSH->abs_diff_pic_num_minus1_l1[i] = bs_read_ue(pBS);
                    fprintf(pOutFp, "    abs_diff_pic_num_minus1_l1[%d]:           %d\n", i, pSH->abs_diff_pic_num_minus1_l1[i]);
                } else {
                    if (val == 2) {
                        pSH->long_term_pic_idx_l1[i] = bs_read_ue(pBS);
                        fprintf(pOutFp, "    long_term_pic_idx_l1[%d]:                 %d\n", i, pSH->long_term_pic_idx_l1[i]);
                    }
                }
                i++;
            } while (val != 3);
        }
        fprintf(pOutFp, "  --L1 end--\n");
    }
}

/* copy from JM18.6 */
void reset_wp_params(slice_header_t *currSlice)
{
  int i,comp;
  int log_weight_denom;

  for (i=0; i<MAX_REFERENCE_PICTURES; i++)
  {
    for (comp=0; comp<3; comp++)
    {
      log_weight_denom = (comp == 0) ? currSlice->luma_log2_weight_denom : currSlice->chroma_log2_weight_denom;
      currSlice->wp_weight[0][i][comp] = 1 << log_weight_denom;
      currSlice->wp_weight[1][i][comp] = 1 << log_weight_denom;
    }
  }
}

void pred_weight_table(slice_header_t *pSH, NALU_t *pNal, bs_t *pBS, FILE *pOutFp)
{
    int list, i, j;
    int luma_def, chroma_def;
    int ref_count[2];

    ref_count[0] = pSH->num_ref_idx_l0_active_minus1 + 1;
    ref_count[1] = pSH->num_ref_idx_l1_active_minus1 + 1;

    fprintf(pOutFp, "--pred_weight_table()--\n");
    pSH->luma_log2_weight_denom = bs_read_ue(pBS);
    fprintf(pOutFp, "luma_log2_weight_denom:    %d\n", pSH->luma_log2_weight_denom);
    if (pSH->luma_log2_weight_denom > 7U) {
        fprintf(stderr, "error: pSH->luma_log2_weight_denom=%d is out of range!\n", pSH->luma_log2_weight_denom);
        pSH->luma_log2_weight_denom = 0;
    }
    luma_def = 1 << pSH->luma_log2_weight_denom;

    if (gCurSps.chroma_format_idc != 0) {
        pSH->chroma_log2_weight_denom = bs_read_ue(pBS);
        fprintf(pOutFp, "  chroma_log2_weight_denom:  %d\n", pSH->chroma_log2_weight_denom);
        if (pSH->chroma_log2_weight_denom > 7U) {
            fprintf(stderr, "error: pSH->chroma_log2_weight_denom=%d is out of range!\n", pSH->luma_log2_weight_denom);
            pSH->chroma_log2_weight_denom = 0;
        }
        chroma_def = 1 << pSH->chroma_log2_weight_denom;
    }

    reset_wp_params(pSH);

    for (i=0; i<ref_count[LIST_0]; i++) {
        //int luma_weight_flag_l0, chroma_weight_flag_l1;
        pSH->luma_weight_flag_l0 = (Boolean)bs_read1(pBS);
        fprintf(pOutFp, "  [LIST_0], idx=%d, luma_weight_flag_l0:    %d\n", i, pSH->luma_weight_flag_l0);
        if (pSH->luma_weight_flag_l0) {
            pSH->wp_weight[LIST_0][i][0] = bs_read_se(pBS);
            pSH->wp_offset[LIST_0][i][0] = bs_read_se(pBS);
            pSH->wp_offset[LIST_0][i][0] = pSH->wp_offset[LIST_0][i][0] << gCurSps.bit_depth_luma_minus8;
        } else {
            pSH->wp_weight[LIST_0][i][0] = 1 << pSH->luma_log2_weight_denom;
            pSH->wp_offset[LIST_0][i][0] = 0;
        }

        if (gCurSps.chroma_format_idc) {
            pSH->chroma_weight_flag_l0 = (Boolean)bs_read1(pBS);
            fprintf(pOutFp, "  [LIST_0], idx=%d, chroma_weight_flag_l0:  %d\n", i, pSH->chroma_weight_flag_l0);
            for (j=1; j<3; j++) {
                if (pSH->chroma_weight_flag_l0) {
                    pSH->wp_weight[LIST_0][i][j] = bs_read_se(pBS);
                    pSH->wp_offset[LIST_0][i][j] = bs_read_se(pBS);
                    pSH->wp_offset[LIST_0][i][0] = pSH->wp_offset[LIST_0][i][j] << gCurSps.bit_depth_chroma_minus8;
                } else {
                    pSH->wp_weight[LIST_0][i][j] = 1 << pSH->chroma_log2_weight_denom;
                    pSH->wp_offset[LIST_0][i][j] = 0;
                }
            }
        }
    }

    if (pNal->slice_type == SLICE_TYPE_B) {
        for (i=0; i<=ref_count[LIST_1]; i++) {
            pSH->luma_weight_flag_l1 = (Boolean)bs_read1(pBS);
            fprintf(pOutFp, "  [LIST_1], idx=%d, luma_weight_flag_l1:    %d\n", i, pSH->luma_weight_flag_l1);
            if (pSH->luma_weight_flag_l1) {
                pSH->wp_weight[LIST_1][i][0] = bs_read_se(pBS);
                pSH->wp_offset[LIST_1][i][0] = bs_read_se(pBS);
                pSH->wp_offset[LIST_1][i][0] = pSH->wp_offset[LIST_1][i][0] << gCurSps.bit_depth_luma_minus8;
            } else {
                pSH->wp_weight[LIST_1][i][0] = 1 << pSH->luma_log2_weight_denom;
                pSH->wp_offset[LIST_1][i][0] = 0;
            }

            if (gCurSps.chroma_format_idc) {
                pSH->chroma_weight_flag_l1 = (Boolean)bs_read1(pBS);
                fprintf(pOutFp, "  [LIST_1], idx=%d, chroma_weight_flag_l1:  %d\n", i, pSH->chroma_weight_flag_l1);
                for (j=1; j<3; j++) {
                    if (pSH->chroma_weight_flag_l1) {
                        pSH->wp_weight[LIST_1][i][j] = bs_read_se(pBS);
                        pSH->wp_offset[LIST_1][i][j] = bs_read_se(pBS);
                        pSH->wp_offset[LIST_1][i][j] = pSH->wp_offset[LIST_1][i][0] << gCurSps.bit_depth_luma_minus8;
                    } else {
                        pSH->wp_weight[LIST_1][i][j] = 1 << pSH->chroma_log2_weight_denom;
                        pSH->wp_offset[LIST_1][i][j] = 0;
                    }
                }
            }
        }
    }
}

void dec_ref_pic_marking(slice_header_t *pSH, NALU_t *pNal, bs_t *pBS, FILE *pOutFp)
{
    fprintf(pOutFp, "--dec_ref_pic_marking()--\n");
    if (pNal->nal_unit_type ==  NALU_TYPE_IDR) {
        pSH->no_output_of_prior_pics_flag = (Boolean)bs_read1(pBS);
        pSH->long_term_reference_flag = (Boolean)bs_read1(pBS);
        fprintf(pOutFp, "  no_output_of_prior_pics_flag:        %d\n", pSH->no_output_of_prior_pics_flag);
        fprintf(pOutFp, "  long_term_reference_flag:            %d\n", pSH->long_term_reference_flag);
    } else {
        pSH->adaptive_ref_pic_marking_mode_flag = (Boolean)bs_read1(pBS);
        fprintf(pOutFp, "  adaptive_ref_pic_marking_mode_flag:  %d\n", pSH->adaptive_ref_pic_marking_mode_flag);
        if (pSH->adaptive_ref_pic_marking_mode_flag) {
            int val;
            do {
                val = pSH->memory_management_control_operation = bs_read_ue(pBS);
                fprintf(pOutFp, "    memory_management_control_operation:   %d\n", pSH->memory_management_control_operation);
                if ((val==1) || (val==3)) {
                    pSH->difference_of_pic_nums_minus1 = bs_read_ue(pBS);
                    fprintf(pOutFp, "      difference_of_pic_nums_minus1:         %d\n", pSH->difference_of_pic_nums_minus1);
                }
                if (val == 2) {
                    pSH->long_term_pic_num = bs_read_ue(pBS);
                    fprintf(pOutFp, "      long_term_pic_num:                     %d\n", pSH->long_term_pic_num);
                }
                if ((val==3) || (val==6)) {
                    pSH->long_term_frame_idx = bs_read_ue(pBS);
                    fprintf(pOutFp, "      long_term_frame_idx:                   %d\n", pSH->long_term_frame_idx);
                }
                if (val == 4) {
                    pSH->max_long_term_frame_idx_plus1 = bs_read_ue(pBS);
                    fprintf(pOutFp, "      max_long_term_frame_idx_plus1:         %d\n", pSH->max_long_term_frame_idx_plus1);
                }
            } while (val != 0);
        }
    }
}

static int SliceHeaderParse(NALU_t * nal)
{
    bs_t s;
    static FILE *myout = fopen("slice_header.txt", "wb");
    static int nalu_idx = 0;

    bs_init(&s, nal->buf+1, nal->len - 1);

    if (nal->nal_unit_type == NALU_TYPE_SLICE || nal->nal_unit_type ==  NALU_TYPE_IDR)
    {
        slice_header_t sh;
        // parse slice header
        fprintf(myout, "\n------------------[nalu_idx=%d] slice header info-----------------\n", nalu_idx);

        sh.first_mb_in_slice = bs_read_ue(&s);
        fprintf(myout, "first_mb_in_slice:      %d\n", sh.first_mb_in_slice);
        sh.slice_type = bs_read_ue(&s);
        fprintf(myout, "slice_type:             %d\n", sh.slice_type);
        switch(sh.slice_type)
        {
        case 0: case 5: /* P */
            nal->slice_type = SLICE_TYPE_P;
            break;
        case 1: case 6: /* B */
            nal->slice_type = SLICE_TYPE_B;
            break;
        case 2: case 7: /* I */
            nal->slice_type = SLICE_TYPE_I;
            break;
        case 3: case 8: /* SP */
            nal->slice_type = SLICE_TYPE_P;
            break;
        case 4: case 9: /* SI */
            nal->slice_type = SLICE_TYPE_I;
            break;
        default:
            printf("unknown slice type! nalu_data[%#x,%#x,%#x,%#x]\n", nal->buf[0], nal->buf[1], nal->buf[2], nal->buf[3]);
            break;
        }
        sh.pic_parameter_set_id = bs_read_ue(&s);
        fprintf(myout, "pic_parameter_set_id:   %d\n", sh.pic_parameter_set_id);

        sh.frame_num = bs_read(&s, gCurSps.log2_max_frame_num_minus4 + 4);
        fprintf(myout, "frame_num:              %d\n", sh.frame_num);

        if (!gCurSps.frame_mbs_only_flag) {
            // field pic
            sh.field_pic_flag = (Boolean)bs_read1(&s);
            fprintf(myout, "  field_pic_flag:         %d\n", sh.field_pic_flag);
            if (sh.field_pic_flag) {
                sh.bottom_field_flag = (Boolean)bs_read1(&s);
                fprintf(myout, "    bottom_field_flag:      %d\n", sh.bottom_field_flag);
            }
        }

        if (nal->nal_unit_type == NALU_TYPE_IDR) {
            sh.idr_pic_id = bs_read_ue(&s);
            fprintf(myout, "  idr_pic_id:             %d\n", sh.idr_pic_id);
        }

        if (gCurSps.pic_order_cnt_type == 0) {
            sh.pic_order_cnt_lsb = bs_read(&s, gCurSps.log2_max_pic_order_cnt_lsb_minus4 + 4);
            fprintf(myout, "  pic_order_cnt_lsb:      %d\n", sh.pic_order_cnt_lsb);
            if (gCurPps.pic_order_present_flag && !sh.field_pic_flag) {
                sh.delta_pic_order_cnt_bottom = bs_read_se(&s);
                fprintf(myout, "    delta_pic_order_cnt_bottom: %d\n", sh.delta_pic_order_cnt_bottom);
            }
        }

        if (gCurSps.pic_order_cnt_type==1 && !gCurSps.delta_pic_order_always_zero_flag) {
            sh.delta_pic_order_cnt[0] = bs_read_se(&s);
            fprintf(myout, "  delta_pic_order_cnt[0]: %d\n", sh.delta_pic_order_cnt[0]);
            if (gCurPps.pic_order_present_flag && !sh.field_pic_flag) {
                sh.delta_pic_order_cnt[1] = bs_read_se(&s);
                fprintf(myout, "    delta_pic_order_cnt[1]: %d\n", sh.delta_pic_order_cnt[1]);
            }
        }

        if (gCurPps.redundant_pic_cnt_present_flag) {
            sh.redundant_pic_cnt = bs_read_ue(&s);
            fprintf(myout, "  redundant_pic_cnt:      %d\n", sh.redundant_pic_cnt);
        }

        if (nal->slice_type == SLICE_TYPE_B) {
            sh.direct_spatial_mv_pred_flag = (Boolean)bs_read1(&s);
            fprintf(myout, "  direct_spatial_mv_pred: %d\n", sh.direct_spatial_mv_pred_flag);
        }

        //if (slice_type == P || slice_type == SP || slice_type == B)
        if (nal->slice_type==SLICE_TYPE_P || nal->slice_type==SLICE_TYPE_B) {
            sh.num_ref_idx_active_override_flag = (Boolean)bs_read1(&s);
            fprintf(myout, "  num_ref_idx_active_override_flag: %d\n", sh.num_ref_idx_active_override_flag);
            if (sh.num_ref_idx_active_override_flag) {
                sh.num_ref_idx_l0_active_minus1 = bs_read_ue(&s);
                fprintf(myout, "    num_ref_idx_l0_active_minus1:     %d\n", sh.num_ref_idx_l0_active_minus1);
                if (nal->slice_type == SLICE_TYPE_B) {
                    sh.num_ref_idx_l1_active_minus1 = bs_read_ue(&s);
                    fprintf(myout, "      num_ref_idx_l1_active_minus1:     %d\n", sh.num_ref_idx_l1_active_minus1);
                }
            }
        }

        //ref_pic_list_reordering()
        ref_pic_list_reordering(&sh, nal, &s, myout);

        //if ((weighted_pred_flag && (slice_type==P || slice_type==SP)) || (weighted_bipred_idc==1 && slice_type==B))
        //  pred_weight_table()
        if ((gCurPps.weighted_pred_flag && (nal->slice_type==SLICE_TYPE_P)) || (gCurPps.weighted_bipred_idc==1 && (nal->slice_type==SLICE_TYPE_B)))
            pred_weight_table(&sh, nal, &s, myout);

        if (nal->nal_reference_idc != 0)
            dec_ref_pic_marking(&sh, nal, &s, myout);

        //if (entropy_coding_mode_flag && slice_type != I && slice_type != SI)
        if (gCurPps.entropy_coding_mode_flag && (nal->slice_type==SLICE_TYPE_B || nal->slice_type==SLICE_TYPE_P)) {
            sh.cabac_init_idc = bs_read_ue(&s);
            fprintf(myout, "  cabac_init_idc:         %d\n", sh.cabac_init_idc);
        }

        sh.slice_qp_delta = bs_read_se(&s);
        fprintf(myout, "slice_qp_delta:         %d\n", sh.slice_qp_delta);

        //if (sh.slice_type == SP || slice_type == SI)
        if ((sh.slice_type==3 || sh.slice_type==8) || (sh.slice_type==4 || sh.slice_type==9)) {
            if (sh.slice_type==3 || sh.slice_type==8) {
                sh.sp_for_switch_flag = (Boolean)bs_read1(&s);
                fprintf(myout, "    sp_for_switch_flag:     %d\n", sh.sp_for_switch_flag);
            }
            sh.slice_qs_delta = bs_read_se(&s);
            fprintf(myout, "  slice_qs_delta:         %d\n", sh.slice_qs_delta);
        }

        if (gCurPps.deblocking_filter_control_present_flag) {
            sh.disable_deblocking_filter_idc = bs_read_ue(&s);
            fprintf(myout, "  disable_deblocking_filter_idc:    %d\n", sh.disable_deblocking_filter_idc);
            if (sh.disable_deblocking_filter_idc != 1) {
                sh.slice_alpha_c0_offset_div2 = bs_read_se(&s);
                sh.slice_beta_offset_div2 = bs_read_se(&s);
                fprintf(myout, "    slice_alpha_c0_offset_div2:       %d\n", sh.slice_alpha_c0_offset_div2);
                fprintf(myout, "    slice_beta_offset_div2:           %d\n", sh.slice_beta_offset_div2);
            }
        }

        if (gCurPps.num_slice_groups_minus1>0 && gCurPps.slice_group_map_type>=3 && gCurPps.slice_group_map_type<=5) {
            int len = (gCurSps.pic_height_in_map_units_minus1+1)*(gCurSps.pic_width_in_mbs_minus1+1) / (gCurPps.slice_group_change_rate_minus1+1);
            if ((gCurSps.pic_height_in_map_units_minus1+1)*(gCurSps.pic_width_in_mbs_minus1+1) % (gCurPps.slice_group_change_rate_minus1+1)) {
                len += 1;
            }
            len = CeilLog2(len+1);
            sh.slice_group_change_cycle = bs_read(&s, len);
            fprintf(myout, "  slice_group_change_cycle:         %d\n", sh.slice_group_change_cycle);
        }

        nalu_idx++;
    }
    else
    {
        nal->slice_type = nal->nal_unit_type;
    }

    return 0;
}

static void hrd_parameters(bs_t* s, FILE* fp, hrd_parameters_t* hrd_param)
{
    hrd_param->cpb_cnt_minus1 = bs_read_ue(s);
    fprintf(fp, "    cpb_cnt_minus1:                           %u\n", hrd_param->cpb_cnt_minus1);
    hrd_param->bit_rate_scale = bs_read(s, 4);
    fprintf(fp, "    bit_rate_scale:                           %u\n", hrd_param->bit_rate_scale);
    hrd_param->cpb_size_scale = bs_read(s, 4);
    fprintf(fp, "    cpb_size_scale:                           %u\n", hrd_param->cpb_size_scale);
    for (int i=0; i<=hrd_param->cpb_cnt_minus1; i++) {
        hrd_param->bit_rate_value_minus1[i] = bs_read_ue(s);
        hrd_param->cpb_size_value_minus1[i] = bs_read_ue(s);
        hrd_param->cbr_flag[i] = bs_read1(s);
        fprintf(fp, "      bit_rate[%d]:                               %u\n", i, hrd_param->bit_rate_value_minus1[i]);
        fprintf(fp, "      cpb_size[%d]:                               %u\n", i, hrd_param->cpb_size_value_minus1[i]);
        fprintf(fp, "      cbr_flag[%d]:                               %u\n", i, hrd_param->cbr_flag[i]);
    }
    hrd_param->initial_cpb_removal_delay_length_minus1 = bs_read(s, 5);
    hrd_param->cpb_removal_delay_length_minus1 = bs_read(s, 5);
    hrd_param->dpb_output_delay_length_minus1 = bs_read(s, 5);
    hrd_param->time_offset_length = bs_read(s, 5);
    fprintf(fp, "    initial_cpb_removal_delay_length_minus1:  %u\n", hrd_param->initial_cpb_removal_delay_length_minus1);
    fprintf(fp, "    cpb_removal_delay_length_minus1:          %u\n", hrd_param->cpb_removal_delay_length_minus1);
    fprintf(fp, "    dpb_output_delay_length_minus1:           %u\n", hrd_param->dpb_output_delay_length_minus1);
    fprintf(fp, "    time_offset_length:                       %u\n", hrd_param->time_offset_length);
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
    //fprintf(myout, "profile_idc:                            %d\n", gCurSps.profile_idc);

    gCurSps.constrained_set0_flag = (Boolean)bs_read1(&s);
    //fprintf(myout, "constrained_set0_flag:                  %d\n", gCurSps.constrained_set0_flag);
    gCurSps.constrained_set1_flag = (Boolean)bs_read1(&s);
    //fprintf(myout, "constrained_set1_flag:                  %d\n", gCurSps.constrained_set1_flag);
    gCurSps.constrained_set2_flag = (Boolean)bs_read1(&s);
    //fprintf(myout, "constrained_set2_flag:                  %d\n", gCurSps.constrained_set2_flag);
    int reserved_zero = bs_read(&s, 5);
    //fprintf(myout, "reserved_zero:                          %#x\n", reserved_zero);
    //assert(reserved_zero == 0); // some higher profile bs will assert

    const char *profile_str = NULL;
    switch (gCurSps.profile_idc) {
        case PROFILE_H264_BASELINE:
            if (gCurSps.constrained_set1_flag)
                profile_str = "constrained_baseline";
            else
                profile_str = "baseline";
            break;
        case PROFILE_H264_MAIN:
            profile_str = "main";
            break;
        case PROFILE_H264_EXTENDED:
            profile_str = "extended";
            break;
        case PROFILE_H264_HIGH:
            profile_str = "high";
            break;
        case PROFILE_H264_HIGH_10:
            profile_str = "high_10";
            break;
        case PROFILE_H264_HIGH_422:
            profile_str = "high_422";
            break;
        case PROFILE_H264_HIGH_444:
            profile_str = "high_444";
            break;
        default:
            profile_str = "others";
            break;
    }
    fprintf(myout, "profile_idc:                            %d(%s)\n", gCurSps.profile_idc, profile_str);
    fprintf(myout, "constrained_set0_flag:                  %d\n", gCurSps.constrained_set0_flag);
    fprintf(myout, "constrained_set1_flag:                  %d\n", gCurSps.constrained_set1_flag);
    fprintf(myout, "constrained_set2_flag:                  %d\n", gCurSps.constrained_set2_flag);
    fprintf(myout, "reserved_zero:                          %#x\n", reserved_zero);

    gCurSps.level_idc = bs_read(&s, 8);
    fprintf(myout, "level_idc:                              %d\n", gCurSps.level_idc);
    gCurSps.seq_parameter_set_id = bs_read_ue(&s);
    fprintf(myout, "seq_parameter_set_id:                   %d\n", gCurSps.seq_parameter_set_id);

    gCurSps.chroma_format_idc = 1;
    gCurSps.bit_depth_luma_minus8 = 0;
    gCurSps.bit_depth_chroma_minus8 = 0;
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
        //fprintf (myout, "VUI sequence parameters present but not supported, ignored, proceeding to next NALU\n");
        vui_seq_parameters_t vui_param;
        memset(&vui_param, 0, sizeof(vui_param));
        vui_param.aspect_ratio_info_present_flag = (Boolean)bs_read1(&s);
        fprintf(myout, "  aspect_ratio_info_present_flag:          %d\n", vui_param.aspect_ratio_info_present_flag);
        if (vui_param.aspect_ratio_info_present_flag) {
            vui_param.aspect_ratio_idc = bs_read(&s, 8);
            fprintf(myout, "    aspect_ratio_idc:                         %d\n", vui_param.aspect_ratio_idc);
            if (vui_param.aspect_ratio_idc == 0xff) {
                vui_param.sar_width = bs_read(&s, 16);
                vui_param.sar_height = bs_read(&s, 16);
                fprintf(myout, "      sar_width:                                %u\n", vui_param.sar_width);
                fprintf(myout, "      sar_height:                               %u\n", vui_param.sar_height);
            }
        }
        vui_param.overscan_info_present_flag = (Boolean)bs_read1(&s);
        fprintf(myout, "  overscan_info_present_flag:              %d\n", vui_param.overscan_info_present_flag);
        if (vui_param.overscan_info_present_flag) {
            vui_param.overscan_appropriate_flag = (Boolean)bs_read1(&s);
            fprintf(myout, "    overscan_appropriate_flag:                %d\n", vui_param.overscan_appropriate_flag);
        }
        vui_param.video_signal_type_present_flag = (Boolean)bs_read1(&s);
        fprintf(myout, "  video_signal_type_present_flag:          %d\n", vui_param.video_signal_type_present_flag);
        if (vui_param.video_signal_type_present_flag) {
            vui_param.video_format = bs_read(&s, 3);
            fprintf(myout, "    video_format:                             %u\n", vui_param.video_format);
            vui_param.video_full_range_flag = (Boolean)bs_read1(&s);
            fprintf(myout, "    video_full_range_flag:                    %d\n", vui_param.video_full_range_flag);
            vui_param.colour_description_present_flag = (Boolean)bs_read1(&s);
            fprintf(myout, "    colour_description_present_flag:          %d\n", vui_param.colour_description_present_flag);
            if (vui_param.colour_description_present_flag) {
                vui_param.colour_primaries = bs_read(&s, 8);
                vui_param.transfer_characteristics = bs_read(&s, 8);
                vui_param.matrix_coefficients = bs_read(&s, 8);
                fprintf(myout, "      colour_primaries:                         %u\n", vui_param.colour_primaries);
                fprintf(myout, "      transfer_characteristics:                 %u\n", vui_param.transfer_characteristics);
                fprintf(myout, "      matrix_coefficients:                      %u\n", vui_param.matrix_coefficients);
            }
        }
        vui_param.chroma_loc_info_present_flag = (Boolean)bs_read1(&s);
        fprintf(myout, "  chroma_loc_info_present_flag:            %d\n", vui_param.chroma_loc_info_present_flag);
        if (vui_param.chroma_loc_info_present_flag) {
            vui_param.chroma_sample_loc_type_top_field = bs_read_ue(&s);
            vui_param.chroma_sample_loc_type_bottom_field = bs_read_ue(&s);
            fprintf(myout, "    chroma_sample_loc_type_top_field:         %u\n", vui_param.chroma_sample_loc_type_top_field);
            fprintf(myout, "    chroma_sample_loc_type_bottom_field:      %u\n", vui_param.chroma_sample_loc_type_bottom_field);
        }
        vui_param.timing_info_present_flag = (Boolean)bs_read1(&s);
        fprintf(myout, "  timing_info_present_flag:                %d\n", vui_param.timing_info_present_flag);
        if (vui_param.timing_info_present_flag) {
            vui_param.num_units_in_tick = bs_read(&s, 32);
            vui_param.time_scale = bs_read(&s, 32);
            vui_param.fixed_frame_rate_flag = (Boolean)bs_read1(&s);
            fprintf(myout, "    num_units_in_tick:                        %u\n", vui_param.num_units_in_tick);
            fprintf(myout, "    time_scale:                               %u\n", vui_param.time_scale);
            fprintf(myout, "    fixed_frame_rate_flag:                    %d\n", vui_param.fixed_frame_rate_flag);
        }
        vui_param.nal_hrd_parameters_present_flag = (Boolean)bs_read1(&s);
        fprintf(myout, "  nal_hrd_parameters_present_flag:         %d\n", vui_param.nal_hrd_parameters_present_flag);
        if (vui_param.nal_hrd_parameters_present_flag) {
            hrd_parameters(&s, myout, &vui_param.nal_hrd_parameters);
        }
        vui_param.vcl_hrd_parameters_present_flag = (Boolean)bs_read1(&s);
        fprintf(myout, "  vcl_hrd_parameters_present_flag:         %d\n", vui_param.vcl_hrd_parameters_present_flag);
        if (vui_param.vcl_hrd_parameters_present_flag) {
            hrd_parameters(&s, myout, &vui_param.vcl_hrd_parameters);
        }
        if (vui_param.nal_hrd_parameters_present_flag || vui_param.vcl_hrd_parameters_present_flag) {
            vui_param.low_delay_hrd_flag = (Boolean)bs_read1(&s);
            fprintf(myout, "    low_delay_hrd_flag:                       %d\n", vui_param.low_delay_hrd_flag);
        }
        vui_param.pic_struct_present_flag = (Boolean)bs_read1(&s);
        fprintf(myout, "  pic_struct_present_flag:                 %d\n", vui_param.pic_struct_present_flag);
        vui_param.bitstream_restriction_flag = (Boolean)bs_read1(&s);
        fprintf(myout, "  bitstream_restriction_flag:              %d\n", vui_param.bitstream_restriction_flag);
        if (vui_param.bitstream_restriction_flag) {
            vui_param.motion_vectors_over_pic_boundaries_flag = (Boolean)bs_read1(&s);
            vui_param.max_bytes_per_pic_denom = bs_read_ue(&s);
            vui_param.max_bits_per_mb_denom = bs_read_ue(&s);
            vui_param.log2_max_mv_length_horizontal = bs_read_ue(&s);
            vui_param.log2_max_mv_length_vertical = bs_read_ue(&s);
            vui_param.max_num_reorder_frames = bs_read_ue(&s);
            vui_param.max_dec_frame_buffering = bs_read_ue(&s);
            fprintf(myout, "    motion_vectors_over_pic_boundaries_flag:  %d\n", vui_param.motion_vectors_over_pic_boundaries_flag);
            fprintf(myout, "    max_bytes_per_pic_denom:                  %u\n", vui_param.max_bytes_per_pic_denom);
            fprintf(myout, "    max_bits_per_mb_denom:                    %u\n", vui_param.max_bits_per_mb_denom);
            fprintf(myout, "    log2_max_mv_length_horizontal:            %u\n", vui_param.log2_max_mv_length_horizontal);
            fprintf(myout, "    log2_max_mv_length_vertical:              %u\n", vui_param.log2_max_mv_length_vertical);
            fprintf(myout, "    max_num_reorder_frames:                   %u\n", vui_param.max_num_reorder_frames);
            fprintf(myout, "    max_dec_frame_buffering:                  %u\n", vui_param.max_dec_frame_buffering);
        }
    }
    gCurSps.Valid = TRUE;
    fprintf(myout, "-------------------------------------------------------\n\n");

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
int bitstream_parse(char *url){

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
    printf("------+---------+-- NALU Table -----+---------+------------+\n");
    printf("| NUM |    POS  |   IDC  | NAL_TYPE |   LEN   | SLICE_TYPE |\n");
    printf("+-----+---------+--------+----------+---------+------------+\n");

    while(!feof(h264bitstream))
    {
        int data_lenth;
        data_lenth=GetAnnexbNALU(nalu);
        EBSPtoRBSP(nalu);
        SliceHeaderParse(nalu);

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

        char slice_type_str[16]={0};
        switch(nalu->slice_type) {
            case SLICE_TYPE_I:sprintf(slice_type_str,"I");break;
            case SLICE_TYPE_P:sprintf(slice_type_str,"P");break;
            case SLICE_TYPE_B:sprintf(slice_type_str,"B");break;
            default:sprintf(slice_type_str,"");break;
        }

        fprintf(myout,"|%5d| %#8x| %7s| %9s| %8d| %10s |\n", nal_num, data_offset, idc_str, nal_type_str, nalu->len, slice_type_str);

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
        printf("error input! usage: %s input_file_name.h264\n", argv[0]);
        return -1;
    }
    bitstream_parse(argv[1]);

    return 0;
}
