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

FILE *h264bitstream = NULL;                // the bit stream file


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

    bs_init(&s, nal->buf+1, nal->len - 1);

    if (nal->nal_unit_type == NALU_TYPE_SLICE || nal->nal_unit_type ==  NALU_TYPE_IDR)
    {
        /* i_first_mb */
        bs_read_ue(&s);
        /* picture type */
        frame_type = bs_read_ue(&s);
        switch(frame_type)
        {
        case 0: case 5: /* P */
            nal->frame_type = FRAME_TYPE_P;
            break;
        case 1: case 6: /* B */
            nal->frame_type = FRAME_TYPE_B;
            break;
        case 3: case 8: /* SP */
            nal->frame_type = FRAME_TYPE_P;
            break;
        case 2: case 7: /* I */
            nal->frame_type = FRAME_TYPE_I;
            break;
        case 4: case 9: /* SI */
            nal->frame_type = FRAME_TYPE_I;
            break;
        default:
            printf("unknown frame type! nalu_data[%#x,%#x,%#x,%#x]\n", nal->buf[0], nal->buf[1], nal->buf[2], nal->buf[3]);
            break;
        }
    }
    else
    {
        nal->frame_type = nal->nal_unit_type;
    }

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

    h264bitstream=fopen(url, "rb+");
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
        puts("error input! fmt: cmd url");
        return -1;
    }
    simplest_h264_parser(argv[1]);

    return 0;
}
