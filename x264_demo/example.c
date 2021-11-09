/*****************************************************************************
 * example.c: libx264 API usage example
 *****************************************************************************
 * Copyright (C) 2014-2020 x264 project
 *
 * Authors: Anton Mitrofanov <BugMaster@narod.ru>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02111, USA.
 *
 * This program is also available under a commercial proprietary license.
 * For more information, contact us at licensing@x264.com.
 *****************************************************************************/

#include <stdint.h>
#include <stdio.h>
#include <x264.h>

#define FAIL_IF_ERROR( cond, ... )\
do\
{\
    if( cond )\
    {\
        fprintf( stderr, __VA_ARGS__ );\
        goto fail;\
    }\
} while( 0 )

int main( int argc, char **argv )
{
    int width, height;
    x264_param_t param;
    x264_picture_t pic;
    x264_picture_t pic_out;
    x264_t *h;
    int i_frame = 0;
    int i_frame_size;
    x264_nal_t *nal;
    int i_nal;

    FAIL_IF_ERROR( !(argc > 1), "Example usage: x264_demo 1280x720 <input.yuv >output.h264, csp use fixed I420(YYYYYYYY...UU...VV...\n" );
    FAIL_IF_ERROR( 2 != sscanf( argv[1], "%dx%d", &width, &height ), "resolution not specified or incorrect\n" );

    /* Get default params for preset/tuning */
    if( x264_param_default_preset( &param, "medium", NULL ) < 0 )
        goto fail;

    /* Configure non-default params */
    param.i_bitdepth = 8;
    param.i_csp = X264_CSP_I420;
    param.i_width  = width;
    param.i_height = height;
    param.b_vfr_input = 0;
    param.b_repeat_headers = 0;
    param.b_annexb = 1;

    /* Apply profile restrictions. */
    if( x264_param_apply_profile( &param, "baseline" ) < 0 )
        goto fail;

    param.rc.i_lookahead = 0;
    param.i_sync_lookahead = 0;
    param.i_bframe = 0;
    param.i_threads = 1;
    param.b_sliced_threads = 0;
    param.b_vfr_input = 0;
    param.rc.b_mb_tree = 0;

    param.i_keyint_max = 25;
    param.i_keyint_min = 25;

    if( x264_picture_alloc( &pic, param.i_csp, param.i_width, param.i_height ) < 0 )
        goto fail;
#undef fail
#define fail fail2

    h = x264_encoder_open( &param );
    if( !h )
        goto fail;
#undef fail
#define fail fail3

    FILE *in_fp = fopen(argv[2], "rb");
    FILE *out_fp = fopen(argv[3], "wb");

    if (in_fp==NULL || out_fp==NULL) {
        printf("open file error! %p, %p\n", in_fp, out_fp);
        return -1;
    }

    int luma_size = width * height;
    int chroma_size = luma_size / 4;

    int csd_sz = x264_encoder_headers( h, &nal, &i_nal );
    printf("csd_sz=%d\n", csd_sz);
    fwrite(nal->p_payload, csd_sz, 1, out_fp);

    /* Encode frames */
    for( ;; i_frame++ )
    {
        if( fread( pic.img.plane[0], 1, luma_size, in_fp ) != (unsigned)luma_size ) {
            break;
        }
        if( fread( pic.img.plane[1], 1, chroma_size, in_fp ) != (unsigned)chroma_size ) {
            break;
        }
        if( fread( pic.img.plane[2], 1, chroma_size, in_fp ) != (unsigned)chroma_size ) {
            break;
        }

        pic.i_pts = i_frame;
        i_frame_size = x264_encoder_encode( h, &nal, &i_nal, &pic, &pic_out );
        if( i_frame_size < 0 ) {
            goto fail;
        }
        else if( i_frame_size )
        {
            if( !fwrite( nal->p_payload, i_frame_size, 1, out_fp) )
                goto fail;
        }
        printf("[%d] sz=%d, keyframe=%d\n", i_frame, i_frame_size, pic_out.b_keyframe);
    }

    x264_encoder_close( h );
    x264_picture_clean( &pic );
    return 0;

#undef fail
fail3:
    x264_encoder_close( h );
fail2:
    x264_picture_clean( &pic );
fail:
    return -1;
}
