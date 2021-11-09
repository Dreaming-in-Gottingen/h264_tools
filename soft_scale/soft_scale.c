#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

int soft_scale(uint8_t *dst_buf, int dst_w, int dst_h, uint8_t *src_buf, int src_w, int src_h)
{
    int dst_uv_w = dst_w/2;
    int dst_uv_h = dst_h/2;

    int ratio_w = (src_w << 10) / dst_w;
    int ratio_h = (src_h << 10) / dst_h;

    uint8_t *src_uv_buf = src_buf + src_w*src_h;
    uint8_t *dst_uv_buf = dst_buf + dst_w*dst_h;

    int cur_w = 0;
    int cur_h = 0;
    int cur_size = 0;
    int cur_byte = 0;

    if (NULL == dst_buf || NULL == src_buf) {
        printf("dst or src is NULL!!!\n");
        return -1;
    }

    uint16_t i, j;
    for (i = 0; i < dst_h; i++) {
        cur_h = (ratio_h * i) >> 10;
        cur_size = cur_h * src_w;
        for (j = 0; j < dst_w; j++) {
            cur_w = (ratio_w * j) >> 10;
            *dst_buf++ = src_buf[cur_size + cur_w];
        }
    }
    for (i = 0; i < dst_uv_h; i++) {
        cur_h = (ratio_h * i) >> 10;
        cur_size = cur_h * (src_w / 2) * 2;
        for (j = 0; j < dst_uv_w; j++) {
            cur_w = (ratio_w * j) >> 10;
            cur_byte = cur_size + cur_w * 2;
            *dst_uv_buf++ = src_uv_buf[cur_byte];
            *dst_uv_buf++ = src_uv_buf[cur_byte + 1];
        }
    }

    return 0;
}

int main(int argc, char **argv)
{
    int ret = 0;
    if (argc != 9) {
        printf("error input with argc=%d! must be this: ./soft_scale -i in_file -ir widthxheight -o out_file -or widthxheight\n", argc);
        return -1;
    }

    char *src_yuv = argv[2];
    char *dst_yuv = argv[6];

    int src_width, src_height;
    int dst_width, dst_height;

    sscanf(argv[4], "%dx%d", &src_width, &src_height);
    printf("dump input params...\n");
    printf("input file info:\n");
    printf("    file      : %s\n", src_yuv);
    printf("    resolution: %d x %d\n", src_width, src_height);

    sscanf(argv[8], "%dx%d", &dst_width, &dst_height);
    printf("output file info:\n");
    printf("    file      : %s\n", dst_yuv);
    printf("    resolution: %d x %d\n", dst_width, dst_height);

    FILE *src_fp = fopen(src_yuv, "rb");
    FILE *dst_fp = fopen(dst_yuv, "wb");
    if (src_fp==NULL || dst_fp==NULL) {
        printf("fopen fail: in_fp=%p, out_fp=%p\n", src_fp, dst_fp);
        return -1;
    }

    int in_buf_sz = src_width * src_height * 3 >> 1;
    int out_buf_sz = dst_width * dst_height * 3 >> 1;
    uint8_t* src_buf = malloc(in_buf_sz);
    uint8_t* dst_buf = malloc(out_buf_sz);

    int rd_sz;
    while ((rd_sz=fread(src_buf, 1, in_buf_sz, src_fp)) == in_buf_sz) {
        ret = soft_scale(dst_buf, dst_width, dst_height, src_buf, src_width, src_height);
        if (ret != 0) {
            puts("error happened in soft_scale()!!!");
            break;
        }
        fwrite(dst_buf, 1, out_buf_sz, dst_fp);
    }

    free(src_buf);
    free(dst_buf);
    fclose(src_fp);
    fclose(dst_fp);

    return 0;
}
