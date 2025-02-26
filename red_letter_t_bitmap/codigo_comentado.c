/*
 * BMP Image Generator
 * 
 * This program creates a 16x16 pixel BMP image with a red letter 'T'
 * on a gray background. It demonstrates the structure of BMP files
 * and how to create them programmatically.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * BMP File Structure Definitions
 * These structures follow the standard BMP file format specification
 */
#pragma pack(push, 1)  /* Ensure structures are packed without padding */

/* BMP file header - 14 bytes total */
typedef struct {
    char signature[2];      /* "BM" - identifies the file as a BMP */
    unsigned int fileSize;  /* Total size of the BMP file in bytes */
    unsigned short reserved1;   /* Reserved, must be 0 */
    unsigned short reserved2;   /* Reserved, must be 0 */
    unsigned int dataOffset;    /* Offset to start of pixel data */
} BITMAPFILEHEADER;

/* BMP info header - 40 bytes total */
typedef struct {
    unsigned int headerSize;      /* Size of this header (40 bytes) */
    int width;                    /* Width of image in pixels */
    int height;                   /* Height of image in pixels */
    unsigned short planes;        /* Number of color planes (must be 1) */
    unsigned short bitsPerPixel;  /* Bits per pixel (24 for RGB) */
    unsigned int compression;     /* Compression method (0 for none) */
    unsigned int imageSize;       /* Size of raw image data */
    int xPixelsPerMeter;          /* Horizontal resolution (pixels/meter) */
    int yPixelsPerMeter;          /* Vertical resolution (pixels/meter) */
    unsigned int colorsUsed;      /* Number of colors in palette */
    unsigned int colorsImportant; /* Number of important colors */
} BITMAPINFOHEADER;

/* RGB pixel structure - 3 bytes */
typedef struct {
    unsigned char blue;   /* Blue component (0-255) */
    unsigned char green;  /* Green component (0-255) */
    unsigned char red;    /* Red component (0-255) */
} PIXEL;

#pragma pack(pop)  /* Restore default structure packing */

/*
 * Function: calculatePadding
 * -------------------------
 * Calculates padding bytes needed for BMP row alignment
 * Each row in a BMP file must be a multiple of 4 bytes
 *
 * width: width of the image in pixels
 *
 * returns: number of padding bytes needed per row
 */
int calculatePadding(int width) {
    return (4 - (width * sizeof(PIXEL)) % 4) % 4;
}

/*
 * Function: createBMPHeaders
 * -------------------------
 * Creates and initializes the BMP file and info headers
 *
 * fileHeader: pointer to file header structure to initialize
 * infoHeader: pointer to info header structure to initialize
 * width: width of the image in pixels
 * height: height of the image in pixels
 * dataSize: size of the pixel data including padding
 */
void createBMPHeaders(BITMAPFILEHEADER *fileHeader, BITMAPINFOHEADER *infoHeader, 
                      int width, int height, int dataSize) {
    /* Initialize file header */
    fileHeader->signature[0] = 'B';
    fileHeader->signature[1] = 'M';
    fileHeader->fileSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + dataSize;
    fileHeader->reserved1 = 0;
    fileHeader->reserved2 = 0;
    fileHeader->dataOffset = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
    
    /* Initialize info header */
    infoHeader->headerSize = sizeof(BITMAPINFOHEADER);
    infoHeader->width = width;
    infoHeader->height = height;
    infoHeader->planes = 1;
    infoHeader->bitsPerPixel = 24;  /* 24 bits for RGB (8 bits per channel) */
    infoHeader->compression = 0;    /* No compression */
    infoHeader->imageSize = dataSize;
    infoHeader->xPixelsPerMeter = 2835;  /* Standard resolution ~72 DPI */
    infoHeader->yPixelsPerMeter = 2835;
    infoHeader->colorsUsed = 0;
    infoHeader->colorsImportant = 0;
}

/*
 * Function: isPartOfLetterT
 * -------------------------
 * Determines if a pixel belongs to the letter 'T'
 *
 * x: x-coordinate of the pixel
 * y: y-coordinate of the pixel (BMP coordinate system)
 *
 * returns: 1 if the pixel is part of the 'T', 0 otherwise
 */
int isPartOfLetterT(int x, int y) {
    /* Horizontal bar of the 'T' (top 3 rows) */
    if (y <= 2 && x >= 3 && x <= 12) {
        return 1;
    }
    /* Vertical stem of the 'T' */
    else if (y > 2 && x >= 7 && x <= 8) {
        return 1;
    }
    return 0;
}

/*
 * Function: writeImageData
 * -------------------------
 * Writes the pixel data for the image to the BMP file
 *
 * file: pointer to the output file
 * width: width of the image in pixels
 * height: height of the image in pixels
 */
void writeImageData(FILE *file, int width, int height) {
    /* Define colors */
    const PIXEL GRAY = {192, 192, 192};  /* Gray background */
    const PIXEL RED = {0, 0, 255};       /* Red for the letter 'T' (BGR format) */
    
    /* Calculate padding bytes needed for row alignment */
    int paddingBytes = calculatePadding(width);
    unsigned char padding[4] = {0};  /* Zero-filled padding */
    
    /* Write pixel data row by row (bottom to top for BMP) */
    for (int y = height - 1; y >= 0; y--) {
        for (int x = 0; x < width; x++) {
            PIXEL pixel = GRAY;  /* Default to gray background */
            
            /* Check if this pixel is part of the letter 'T' */
            if (isPartOfLetterT(x, y)) {
                pixel = RED;
            }
            
            /* Write the pixel to the file */
            fwrite(&pixel, sizeof(PIXEL), 1, file);
        }
        
        /* Add padding at the end of each row if needed */
        if (paddingBytes > 0) {
            fwrite(padding, paddingBytes, 1, file);
        }
    }
}

/*
 * Function: displayFileHex
 * -------------------------
 * Reads and displays the contents of a file in hexadecimal format
 *
 * filename: name of the file to display
 */
void displayFileHex(const char *filename) {
    FILE *file = fopen(filename, "rb");
    if (!file) {
        printf("Could not open %s for reading\n", filename);
        return;
    }
    
    unsigned char buffer[16];
    size_t bytesRead;
    printf("\nFile content (hex):\n");
    
    /* Read and display 16 bytes at a time */
    while ((bytesRead = fread(buffer, 1, 16, file)) > 0) {
        for (size_t i = 0; i < bytesRead; i++) {
            printf("%02x ", buffer[i]);
        }
        printf("\n");
    }
    
    fclose(file);
}

/*
 * Main function
 */
int main() {
    /* Define image dimensions */
    const int WIDTH = 16;
    const int HEIGHT = 16;
    
    /* Calculate file size parameters */
    int paddingBytes = calculatePadding(WIDTH);
    int dataSize = HEIGHT * (WIDTH * sizeof(PIXEL) + paddingBytes);
    
    /* Initialize header structures */
    BITMAPFILEHEADER fileHeader;
    BITMAPINFOHEADER infoHeader;
    createBMPHeaders(&fileHeader, &infoHeader, WIDTH, HEIGHT, dataSize);
    
    /* Create and open the output BMP file */
    const char *outputFilename = "letter_T.bmp";
    FILE *bmpFile = fopen(outputFilename, "wb");
    if (!bmpFile) {
        fprintf(stderr, "Error: Could not create output file\n");
        return EXIT_FAILURE;
    }
    
    /* Write headers to the file */
    fwrite(&fileHeader, sizeof(BITMAPFILEHEADER), 1, bmpFile);
    fwrite(&infoHeader, sizeof(BITMAPINFOHEADER), 1, bmpFile);
    
    /* Write pixel data */
    writeImageData(bmpFile, WIDTH, HEIGHT);
    
    /* Close the file */
    fclose(bmpFile);
    
    /* Display success message */
    printf("BMP file '%s' created successfully.\n", outputFilename);
    printf("Image dimensions: %d x %d pixels\n", WIDTH, HEIGHT);
    
    /* Display file contents in hex format */
    displayFileHex(outputFilename);
    
    return EXIT_SUCCESS;
}