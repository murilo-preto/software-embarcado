/*
 * lab1_1.c
 *
 *  Created on: 11/07/2013
 *      Author: Amaury
 */
#include <stdio.h>
#include <stdlib.h>

struct BITMAPFILEHEADER {
    char b;
    char m;
    unsigned long h_size; // size of file
    unsigned short h_reserved1;
    unsigned short h_reserved2;
    unsigned long h_offsetbits; // offset bits

    unsigned long i_size; // bitmap size
    unsigned long i_width; // width of bitmap
    unsigned long i_height; // height of bitmap
    unsigned short i_planes;
    unsigned short i_bitcount;
    unsigned long i_compression; // compression ratio (zero for no compression)
    unsigned long i_sizeimage; // size of image
    long i_xpelspermeter;
    long i_ypelspermeter;
    unsigned long i_colorsused;
    unsigned long i_colorsimportant;
};

struct SINGLE_PIXEL {
    unsigned char blue; // Blue level  0-255
    unsigned char green; // Green level 0-255
    unsigned char red; // Red level 0-255
};

int main() {
    unsigned char buffer[16];
    unsigned long int i = 0;
    unsigned long int S = 0;
    unsigned long int j;
    unsigned short x;

    struct BITMAPFILEHEADER source_head; // to store file header
    struct SINGLE_PIXEL source_pix[2][2]; // to store pixels

    source_head.h_size = 70; // size of file
    source_head.h_reserved1 = 0;
    source_head.h_reserved2 = 0;
    source_head.h_offsetbits = 54; // offset bits

    source_head.i_size = 40; // bitmap size
    source_head.i_width = 2; // width of bitmap
    source_head.i_height = 2; // height of bitmap
    source_head.i_planes = 1;
    source_head.i_bitcount = 24;
    source_head.i_compression = 0; // compression ratio (zero for no compression)
    source_head.i_sizeimage = 16; // size of image
    source_head.i_xpelspermeter = 2835;
    source_head.i_ypelspermeter = 2835;
    source_head.i_colorsused = 0;
    source_head.i_colorsimportant = 0;

    source_pix[0][0].blue = 0;
    source_pix[0][0].green = 0;
    source_pix[0][0].red = 0xff;

    source_pix[0][1].blue = 0xff;
    source_pix[0][1].green = 0xff;
    source_pix[0][1].red = 0xff;

    source_pix[1][0].blue = 0xff;
    source_pix[1][0].green = 0;
    source_pix[1][0].red = 0;

    source_pix[1][1].blue = 0;
    source_pix[1][1].green = 0xff;
    source_pix[1][1].red = 0;

    FILE *Dfp;
    Dfp = fopen("en2622.bmp", "w+b");
    buffer[0] = 'B';
    buffer[1] = 'M';
    source_head.b = 'B';
    source_head.m = 'M';
    fwrite(&source_head, sizeof(struct BITMAPFILEHEADER), 1, Dfp);

    S = source_head.i_width * source_head.i_height;

    for (i = 0; i < source_head.i_height; i++) {
        for (j = 0; j < source_head.i_width; j++) {
            fwrite(&source_pix[i][j], sizeof(struct SINGLE_PIXEL), 1, Dfp);
        }
        for (j = 0; j < sizeof(struct SINGLE_PIXEL) * source_head.i_width % 4; j++) {
            fwrite(buffer, 1, 1, Dfp);
        }
    }
    fclose(Dfp);

    Dfp = fopen("en2622.bmp", "rb");
    do {
        i = fread(&buffer, 1, 16, Dfp);
        for (S = 0; S < i; S++) {
            x = buffer[S];
            printf("%x ", x);
        }
        printf("\n");
    } while (i == 16);

    fclose(Dfp);
    return 0;
}