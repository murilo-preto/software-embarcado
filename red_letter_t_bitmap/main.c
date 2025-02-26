#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#pragma pack(push, 1)
typedef struct {
    char signature[2];
    unsigned int fileSize;
    unsigned short reserved1;
    unsigned short reserved2;
    unsigned int dataOffset;
} BITMAPFILEHEADER;

typedef struct {
    unsigned int headerSize;
    int width;
    int height;
    unsigned short planes;
    unsigned short bitsPerPixel;
    unsigned int compression;
    unsigned int imageSize;
    int xPixelsPerMeter;
    int yPixelsPerMeter;
    unsigned int colorsUsed;
    unsigned int colorsImportant;
} BITMAPINFOHEADER;

typedef struct {
    unsigned char blue;
    unsigned char green;
    unsigned char red;
} PIXEL;
#pragma pack(pop)

int calculatePadding(int width) {
    return (4 - (width * sizeof(PIXEL)) % 4) % 4;
}

int main() {
    const int WIDTH = 16;
    const int HEIGHT = 16;

    const PIXEL GRAY = { 192, 192, 192 };
    const PIXEL RED = { 0, 0, 255 };

    int paddingBytes = calculatePadding(WIDTH);
    int dataSize = HEIGHT * (WIDTH * sizeof(PIXEL) + paddingBytes);
    int fileSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + dataSize;

    BITMAPFILEHEADER fileHeader = {
        .signature = {'B', 'M'},
        .fileSize = fileSize,
        .reserved1 = 0,
        .reserved2 = 0,
        .dataOffset = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER)
    };

    BITMAPINFOHEADER infoHeader = {
        .headerSize = sizeof(BITMAPINFOHEADER),
        .width = WIDTH,
        .height = HEIGHT,
        .planes = 1,
        .bitsPerPixel = 24,
        .compression = 0,
        .imageSize = dataSize,
        .xPixelsPerMeter = 2835,
        .yPixelsPerMeter = 2835,
        .colorsUsed = 0,
        .colorsImportant = 0
    };

    FILE* bmpFile = fopen("letter_T.bmp", "wb");
    if (!bmpFile) {
        fprintf(stderr, "Error: Could not create output file\n");
        return EXIT_FAILURE;
    }

    fwrite(&fileHeader, sizeof(BITMAPFILEHEADER), 1, bmpFile);
    fwrite(&infoHeader, sizeof(BITMAPINFOHEADER), 1, bmpFile);

    unsigned char padding[4] = { 0 };

    for (int y = HEIGHT - 1; y >= 0; y--) {
        for (int x = 0; x < WIDTH; x++) {
            PIXEL pixel = GRAY;

            if (y <= 2 &&
                x >= 3 && x <= 12) {
                pixel = RED;
            }
            else if (y > 2 &&
                x >= 7 && x <= 8) {
                pixel = RED;
            }

            fwrite(&pixel, sizeof(PIXEL), 1, bmpFile);
        }

        if (paddingBytes > 0) {
            fwrite(padding, paddingBytes, 1, bmpFile);
        }
    }

    fclose(bmpFile);

    printf("BMP file 'letter_T.bmp' created successfully.\n");
    printf("Image dimensions: %d x %d pixels\n", WIDTH, HEIGHT);

    FILE* readFile = fopen("letter_T.bmp", "rb");
    if (readFile) {
        unsigned char buffer[16];
        size_t bytesRead;
        printf("\nFile content (hex):\n");

        while ((bytesRead = fread(buffer, 1, 16, readFile)) > 0) {
            for (size_t i = 0; i < bytesRead; i++) {
                printf("%02x ", buffer[i]);
            }
            printf("\n");
        }

        fclose(readFile);
    }

    return EXIT_SUCCESS;
}