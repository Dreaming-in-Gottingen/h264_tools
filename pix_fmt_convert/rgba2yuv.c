#include <stdio.h>
#include <stdlib.h>

enum PIXEL_FORMAT {
    YUV444 = 0,
    YUV420 = 1,
    NV21   = 2,
    NV12   = 3,
    MAX_FMT= 4,
};

void convertRGBA2YUV(unsigned char* dst, unsigned char* src, int width, int height, enum PIXEL_FORMAT fmt)
{
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
}

int main(int argc, char **argv)
{
    if (argc != 6) {
        printf("error input! must be this: cmd in_file out_file width height dst_pix_fmt\n");
        printf("                           dst_pix_fmt: 0-yuv444; 1-yuv420; 2-nv21; 3-nv12\n");
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
    enum PIXEL_FORMAT pix_fmt = atoi(argv[5]);

    printf("argv info: input_file=%s, output_file=%s, size=%dx%d, pix_fmt=%d\n", in_rgba, out_yuv, width, height, pix_fmt);
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

    int rd_sz;
    while ((rd_sz=fread(src_buf, 1, rgba_sz, in_fp)) == rgba_sz) {
        convertRGBA2YUV(dst_buf, src_buf, width, height, pix_fmt);
        fwrite(dst_buf, 1, yuv_sz, out_fp);
    }

    free(src_buf);
    free(dst_buf);
    fclose(in_fp);
    fclose(out_fp);

    return 0;
}
