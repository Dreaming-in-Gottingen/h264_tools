#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>

enum PIXEL_FORMAT {
    YUV444 = 0,
    YUV420 = 1,
    NV21   = 2,
    NV12   = 3,
    MAX_FMT= 4,
};

static unsigned int GetTickCount()
{
    struct timeval tv;
    if (gettimeofday(&tv, NULL))
        return 0;
    return tv.tv_usec / 1000 + tv.tv_sec * 1000;
}

void convertRGBA2YUV(unsigned char* dst, unsigned char* src, int width, int height, int stride, enum PIXEL_FORMAT fmt)
{
#if 0 // low efficient because float calculate
    // BT709
    float Wr = 0.2126, Wg = 0.7152, Wb = 0.0722;
    // BT601
    //float Wr = 0.299, Wg = 0.587, Wb = 0.144;
    int pic_sz = width*height;
    int uv_cnt = 0;
    for (int i=0; i<height; i++) {
        for (int j=0; j<width; j++) {
            unsigned char R = *src++;
            unsigned char G = *src++;
            unsigned char B = *src++;
            unsigned char A = *src++;
            unsigned char Y = (unsigned char)(16 + 219.0 * (Wr*R + Wg*G + Wb*B) / 255);
            dst[i*width+j] = Y;
            if (fmt == YUV444) {
                dst[pic_sz + uv_cnt] = (unsigned char)(128 + 224.0 * (0.5 * (B - Y) / (1 - Wb)) / 255); // Cb
                dst[pic_sz*2 + uv_cnt] = (unsigned char)(128 + 224.0 * (0.5 * (R - Y) / (1 - Wr)) / 255); // Cr
                uv_cnt++;
            } else if (fmt == YUV420) {
                if (i%2==0 && j%2==0) {
                    dst[pic_sz + uv_cnt] = (unsigned char)(128 + 224.0 * (0.5 * (B - Y) / (1 - Wb)) / 255); // Cb
                    dst[pic_sz + (pic_sz>>2) + uv_cnt] = (unsigned char)(128 + 224.0 * (0.5 * (R - Y) / (1 - Wr)) / 255); // Cr
                    uv_cnt++;
                }
            } else if (fmt == NV21) {
                if (i%2==0 && j%2==0) {
                    dst[pic_sz + uv_cnt++] = (unsigned char)(128 + 224.0 * (0.5 * (R - Y) / (1 - Wr)) / 255); // Cr
                    dst[pic_sz + uv_cnt++] = (unsigned char)(128 + 224.0 * (0.5 * (B - Y) / (1 - Wb)) / 255); // Cb
                }
            } else if (fmt == NV12) {
                if (i%2==0 && j%2==0) {
                    dst[pic_sz + uv_cnt++] = (unsigned char)(128 + 224.0 * (0.5 * (B - Y) / (1 - Wb)) / 255); // Cb
                    dst[pic_sz + uv_cnt++] = (unsigned char)(128 + 224.0 * (0.5 * (R - Y) / (1 - Wr)) / 255); // Cr
                }
            }
        }
    }
#else
    // will delete padding on the right edge
    // rgba -> i420(yuv420p)
    int srcStride = 4 * stride;
    int dstStride = width;
    int dstVStride = height;

    unsigned char *dstY = dst;
    unsigned char *dstU = dstY + dstStride * dstVStride;
    unsigned char *dstV = dstU + (dstStride >> 1) * (dstVStride >> 1);

    const size_t redOffset = 0;
    const size_t greenOffset = 1;
    const size_t blueOffset  = 2;

    for (size_t y = 0; y < height; ++y) {
        for (size_t x = 0; x < width; ++x) {
            unsigned red   = src[redOffset];
            unsigned green = src[greenOffset];
            unsigned blue  = src[blueOffset];
            unsigned luma = ((red * 65 + green * 129 + blue * 25 + 128) >> 8) + 16;
            dstY[x] = luma;
            if ((x & 1) == 0 && (y & 1) == 0) {
                unsigned U =
                    ((-red * 38 - green * 74 + blue * 112 + 128) >> 8) + 128;

                unsigned V =
                    ((red * 112 - green * 94 - blue * 18 + 128) >> 8) + 128;

                dstU[x >> 1] = U;
                dstV[x >> 1] = V;
            }
            src += 4;
        }

        if ((y & 1) == 0) {
            dstU += dstStride >> 1;
            dstV += dstStride >> 1;
        }

        src += srcStride - 4 * width;
        dstY += dstStride;
    }
#endif
}

int main(int argc, char **argv)
{
    if (argc != 6) {
        printf("error input! usage like below:\n");
        printf("  usage: %s in_file out_file width height dst_pix_fmt(0-yuv444; 1-yuv420; 2-nv21; 3-nv12)\n", argv[0]);
        return -1;
    }

    char *in_rgba = argv[1];
    char *out_yuv = argv[2];

    FILE *in_fp = fopen(in_rgba, "rb");
    FILE *out_fp = fopen(out_yuv, "wb");
    if (in_fp==NULL || out_fp==NULL) {
        printf("fopen fail: in_fp=%p, out_fp=%p\n", in_fp, out_fp);
        return -1;
    }

    int width = atoi(argv[3]);
    int height = atoi(argv[4]);
    int stride = width;
    enum PIXEL_FORMAT pix_fmt = atoi(argv[5]);

    printf("dump argv info...\n");
    printf("  input_file=%s\n", in_rgba);
    printf("  output_file=%s\n", out_yuv);
    printf("  dimension=%d x %d\n", width, height);
    printf("  stride=%d\n", stride);
    printf("  dst pix_fmt=%d\n", pix_fmt);
    if (pix_fmt >= MAX_FMT) {
        printf("error pix_fmt(%d), which bigger than MAX_FMT! use NV21!", pix_fmt);
        pix_fmt = NV21;
    }

    int rgba_sz = width*height*4;
    int yuv_sz = width*height*3>>1;
    if (pix_fmt == YUV444)
        yuv_sz = width*height*3;
    unsigned char* src_buf = malloc(rgba_sz);
    unsigned char* dst_buf = malloc(yuv_sz);

    time_t rawtime_begin;
    time ( &rawtime_begin );
    printf("rgba -> yuv ...\n");
    unsigned int ticks_begin = GetTickCount();

    int rd_sz;
    while ((rd_sz=fread(src_buf, 1, rgba_sz, in_fp)) == rgba_sz) {
        convertRGBA2YUV(dst_buf, src_buf, width, height, stride, pix_fmt);
        fwrite(dst_buf, 1, yuv_sz, out_fp);
    }

    time_t rawtime_end;
    time ( &rawtime_end );
    unsigned int ticks_end = GetTickCount();
    printf("CSC finished!\n");
    printf("total_duration=%ld s, ticks_duration=%u ms\n", rawtime_end - rawtime_begin, ticks_end - ticks_begin);

    free(src_buf);
    free(dst_buf);
    fclose(in_fp);
    fclose(out_fp);

    return 0;
}
