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

// Function to initialize the bitmap file header
void initialize_bitmap_header(struct BITMAPFILEHEADER* header) {
    header->h_size = 70 + (16 * 16 * 3); // size of file (adjusted for 16x16 image); // size of file
    header->h_reserved1 = 0;
    header->h_reserved2 = 0;
    header->h_offsetbits = 54; // offset bits

    header->i_size = 40; // bitmap size
    header->i_width = 16; // width of bitmap
    header->i_height = 16; // height of bitmap
    header->i_planes = 1;
    header->i_bitcount = 24;
    header->i_compression = 0; // compression ratio (zero for no compression)
    header->i_sizeimage = 16 * 16 * 3; // size of image
    header->i_xpelspermeter = 2835;
    header->i_ypelspermeter = 2835;
    header->i_colorsused = 0;
    header->i_colorsimportant = 0;
}

// Function to initialize the bitmap pixels
void initialize_bitmap_pixels(struct SINGLE_PIXEL pixels[16][16]) {
    // Set background to gray (RGB: 169, 169, 169)
    for (int i = 0; i < 16; i++) {
        for (int j = 0; j < 16; j++) {
            pixels[i][j].blue = 169;
            pixels[i][j].green = 169;
            pixels[i][j].red = 169;
        }
    }

    // Create the letter 'T' in red (RGB: 255, 0, 0)
    for (int i = 0; i < 16; i++) {
        pixels[7][i].blue = 0;
        pixels[7][i].green = 0;
        pixels[7][i].red = 255;  // Horizontal line of T

        pixels[i][7].blue = 0;
        pixels[i][7].green = 0;
        pixels[i][7].red = 255;  // Vertical line of T
    }
}
// Function to write the bitmap to a file
void write_bitmap_to_file(const char* filename, struct BITMAPFILEHEADER* header, struct SINGLE_PIXEL pixels[16][16]) {
    unsigned char buffer[16];
    FILE *Dfp = fopen(filename, "w+b");

    buffer[0] = 'B';
    buffer[1] = 'M';
    header->b = 'B';
    header->m = 'M';
    fwrite(header, sizeof(struct BITMAPFILEHEADER), 1, Dfp);

    for (unsigned long int i = 0; i < header->i_height; i++) {
        for (unsigned long int j = 0; j < header->i_width; j++) {
            fwrite(&pixels[i][j], sizeof(struct SINGLE_PIXEL), 1, Dfp);
        }
        for (unsigned long int j = 0; j < sizeof(struct SINGLE_PIXEL) * header->i_width % 4; j++) {
            fwrite(buffer, 1, 1, Dfp);
        }
    }
    fclose(Dfp);
}

// Function to read the bitmap from a file and print its content in hex format
void read_bitmap_from_file(const char* filename) {
    unsigned char buffer[16];
    FILE *Dfp = fopen(filename, "rb");
    unsigned long int i, S;
    unsigned short x;

    do {
        i = fread(&buffer, 1, 16, Dfp);
        for (S = 0; S < i; S++) {
            x = buffer[S];
            printf("%x ", x);
        }
        printf("\n");
    } while (i == 16);

    fclose(Dfp);
}

int main() {
    struct BITMAPFILEHEADER source_head; // to store file header
    struct SINGLE_PIXEL source_pix[16][16]; // to store pixels

    initialize_bitmap_header(&source_head); // Initialize header
    initialize_bitmap_pixels(source_pix); // Initialize pixels

    // Write bitmap to file
    write_bitmap_to_file("en2622.bmp", &source_head, source_pix);

    // Read bitmap from file and print content
    read_bitmap_from_file("en2622.bmp");

    return 0;
}