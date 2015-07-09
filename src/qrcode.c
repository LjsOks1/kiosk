#include <stdio.h>
#include <qrencode.h>
#include <errno.h>
#include <png.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "qrcode.h"

#define INCHES_PER_METER (100.0/2.54)

enum imageType {
    PNG_TYPE,
    PNG32_TYPE,
    EPS_TYPE,
    SVG_TYPE,
    ANSI_TYPE,
    ANSI256_TYPE,
    ASCII_TYPE,
    ASCIIi_TYPE,
    UTF8_TYPE,
    ANSIUTF8_TYPE
};

static void 
fillRow(unsigned char *row, int size, const unsigned char color[])
{
    int i;

    for(i = 0; i< size; i++) {
        memcpy(row, color, 4);
        row += 4;
    }
}

void
my_png_write_data(png_structp png_ptr, png_bytep data, png_size_t length)
{
  /* with libpng15 next line causes pointer deference error; use libpng12 */
  struct mem_encode* p=(struct mem_encode*)png_get_io_ptr(png_ptr); /* was png_ptr->io_ptr */
  size_t nsize = p->size + length;

  /* allocate or grow buffer */
  if(p->buffer)
    p->buffer = realloc(p->buffer, nsize);
  else
    p->buffer = malloc(nsize);

  if(!p->buffer)
    png_error(png_ptr, "Write Error");

  /* copy new bytes to end of buffer */
  memcpy(p->buffer + p->size, data, length);
  p->size += length;
}

static int 
writePNG(const QRcode *qrcode,const char* outfile, enum imageType type)
{
    static FILE *fp;
    int size = 12;
    int margin = 3;
    int dpi = 72;
    unsigned char fg_color[4] = {0, 0, 0, 255};
    unsigned char bg_color[4] = {255, 255, 255, 255};

    png_structp png_ptr;
    png_infop info_ptr;
    png_colorp palette = NULL;
    png_byte alpha_values[2];
    unsigned char *row, *p, *q;
    int x, y, xx, yy, bit;
    int realwidth;

    realwidth = (qrcode->width + margin * 2) * size;
    if(type == PNG_TYPE) {
        row = (unsigned char *)malloc((realwidth + 7) / 8);
    } else if(type == PNG32_TYPE) {
        row = (unsigned char *)malloc(realwidth * 4);
    } else {
        fprintf(stderr, "Internal error.\n");
        exit(EXIT_FAILURE);
    }
    if(row == NULL) {
        fprintf(stderr, "Failed to allocate memory.\n");
        exit(EXIT_FAILURE);
    }
    fp = fopen(outfile, "wb");
    if(fp == NULL) {
       fprintf(stderr, "Failed to create file: %s\n", outfile);
       perror(NULL);
       exit(EXIT_FAILURE);
    }

    png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if(png_ptr == NULL) {
        fprintf(stderr, "Failed to initialize PNG writer.\n");
        exit(EXIT_FAILURE);
    }

    info_ptr = png_create_info_struct(png_ptr);
    if(info_ptr == NULL) {
        fprintf(stderr, "Failed to initialize PNG write.\n");
        exit(EXIT_FAILURE);
    }

    if(setjmp(png_jmpbuf(png_ptr))) {
        png_destroy_write_struct(&png_ptr, &info_ptr);
        fprintf(stderr, "Failed to write PNG image.\n");
        exit(EXIT_FAILURE);
    }

    if(type == PNG_TYPE) {
        palette = (png_colorp) malloc(sizeof(png_color) * 2);
        if(palette == NULL) {
            fprintf(stderr, "Failed to allocate memory.\n");
            exit(EXIT_FAILURE);
        }
        palette[0].red   = fg_color[0];
        palette[0].green = fg_color[1];
        palette[0].blue  = fg_color[2];
        palette[1].red   = bg_color[0];
        palette[1].green = bg_color[1];
        palette[1].blue  = bg_color[2];
        alpha_values[0] = fg_color[3];
        alpha_values[1] = bg_color[3];
        png_set_PLTE(png_ptr, info_ptr, palette, 2);
        png_set_tRNS(png_ptr, info_ptr, alpha_values, 2, NULL);
    }

    png_init_io(png_ptr, fp);
//    png_set_write_fn(png_ptr,*state,my_png_write_data,NULL);
    if(type == PNG_TYPE) {
        png_set_IHDR(png_ptr, info_ptr,
                realwidth, realwidth,
                1,
                PNG_COLOR_TYPE_PALETTE,
                PNG_INTERLACE_NONE,
                PNG_COMPRESSION_TYPE_DEFAULT,
                PNG_FILTER_TYPE_DEFAULT);
    } else {
        png_set_IHDR(png_ptr, info_ptr,
                realwidth, realwidth,
                8,
                PNG_COLOR_TYPE_RGB_ALPHA,
                PNG_INTERLACE_NONE,
                PNG_COMPRESSION_TYPE_DEFAULT,
                PNG_FILTER_TYPE_DEFAULT);
    }
    png_set_pHYs(png_ptr, info_ptr,
            dpi * INCHES_PER_METER,
            dpi * INCHES_PER_METER,
            PNG_RESOLUTION_METER);
    png_write_info(png_ptr, info_ptr);

    if(type == PNG_TYPE) {
    /* top margin */
        memset(row, 0xff, (realwidth + 7) / 8);
        for(y=0; y<margin * size; y++) {
            png_write_row(png_ptr, row);
        }

        /* data */
        p = qrcode->data;
        for(y=0; y<qrcode->width; y++) {
            memset(row, 0xff, (realwidth + 7) / 8);
            q = row;
            q += margin * size / 8;
            bit = 7 - (margin * size % 8);
            for(x=0; x<qrcode->width; x++) {
                for(xx=0; xx<size; xx++) {
                    *q ^= (*p & 1) << bit;
                    bit--;
                    if(bit < 0) {
                        q++;
                        bit = 7;
                    }
                }
                p++;
            }
            for(yy=0; yy<size; yy++) {
                png_write_row(png_ptr, row);
            }
        }
        /* bottom margin */
        memset(row, 0xff, (realwidth + 7) / 8);
        for(y=0; y<margin * size; y++) {
            png_write_row(png_ptr, row);
        }
    } else {
    /* top margin */
        fillRow(row, realwidth, bg_color);
        for(y=0; y<margin * size; y++) {
            png_write_row(png_ptr, row);
        }

        /* data */
        p = qrcode->data;
        for(y=0; y<qrcode->width; y++) {
            fillRow(row, realwidth, bg_color);
            for(x=0; x<qrcode->width; x++) {
                for(xx=0; xx<size; xx++) {
                    if(*p & 1) {
                        memcpy(&row[((margin + x) * size + xx) * 4], fg_color, 4);
                    }
                }
                p++;
            }
            for(yy=0; yy<size; yy++) {
                png_write_row(png_ptr, row);
            }
        }
        /* bottom margin */
        fillRow(row, realwidth, bg_color);
        for(y=0; y<margin * size; y++) {
            png_write_row(png_ptr, row);
        }
    }

    png_write_end(png_ptr, info_ptr);
    png_destroy_write_struct(&png_ptr, &info_ptr);

    fclose(fp);
    free(row);
    free(palette);
    

    return 0;
}


 
int
generate_qr_code(char* string)
{
    QRcode *qrcode;
/*  This should be defined in recorderbin after record stopped.
    struct mem_encode state;

    state.buffer=NULL;
    state.size=0;
*/
    qrcode = QRcode_encodeString(string, 4, QR_ECLEVEL_H, QR_MODE_8, 1);

    if(qrcode == NULL) {
       printf("An error has occured (%d).\n", errno);
       if(errno == EINVAL){
          printf("Invalid input object\n");
       }  
       else if(errno == ENOMEM) {
          printf("Unable to allocate memory for input objects.\n");
       }
       else if(errno == ERANGE) {
          printf("Input data is too large.\n");
       }
       return 1;
    }
 
    writePNG(qrcode,"lastqr.png",PNG_TYPE);
    QRcode_free(qrcode);
    return 0;
}
