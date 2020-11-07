//TODO: make errors more descriptive, add more restrictions to png data formatting, optimize code for speed


/*
References:
  wikipedia.org/wiki/Portable_Network_Graphics
  libpng.org/pub/png
  zlib.net


Bit of background info:
-------------------------
PNG's are composed of a 8-byte signature, and then multiple chunks.
the names of these chunks tell you what type of information they're carrying

Chunks have different types. There's critical and ancillary chunks.
critical chunks are the important ones, they contain stuff like the end of the file and actual pixel information.
ancillary chunks can be useful, but aren't required for the PNG to work.

These Chunks are made up of four parts:
- A 4-byte value that represents the length of the information it has
- A 4-byte value that represents the type of chunk
- The actual information of the chunk, with a length determined by the first section
- And a 4-byte CRC which is used to check if the data was corrupted

There are four Critical Chunk types:
-IEND: Appears at end of file and holds no information.
-IHDR: Appears at start of file and holds useful information, such as dimensions
-IDAT: Holds pixel values, usually near end of file
-PLTE: In some cases, the IDAT chunk wont hold ACTUAL rgb values; instead, it'll hold numbers which correspond to an index of a list of colors. The PLTE chunk is that list.


Deeper Dive:
-----------------
The IHDR Chunk contains ALOT of juicy information. Heres its structure:
-A 4-byte value indicating the pictures width
-A 4-byte value indicating the pictures height
-A 1-byte value indicating the 'bit depth',
    *(essentially how many bits it takes to get represent one color channel)
-A 1-byte value indicating the color type.
    *(Possible types are 3 - Indexed (uses PLTE), 0 - Grayscale, 4 - Grayscale with transparency, 2 - RGB, and 6 - RGB with transparency)
-A 1-byte value indicating the compression method,
    *(as of now the only available option is deflate/inflate)
-A 1-byte value indicating the filter method.
    *(Again, theres only available option, called Adaptive filtering)
-A 1-byte value indicating the Interlace method.
    *(Interlacing allows the image to be transmitted over your shitty wifi better by sending information in passes, but does not affect the actual order of pixel values)

  ***(If you were to Write to a PNG, simply reverse this process)
In Order to actually GET the picture from a PNG, you need to:
    1. Combine the data from all the IDAT chunks (there can be multiple)

    2. Use the uncompress() function on the IDAT chunks to get the values we're going to work with

    3. With these values, you'll need to preform 'Adaptive filtering'.
       With this, at the start of each row of the PNG, there is a filter byte. The value of this byte determines how the values in its row are filtered:

       > If the bytes value is 0, the current pixel is unchanged.
       > If the bytes value is 1, the current pixel is equal to the current pixel minus the previous one.
       > If the bytes value is 2, the current pixel is equal to the current pixel minus the one above it.
       > If the bytes value is 3, the current pixel is equal to the current pixel minus the average of the previous and above pixels
       > If the bytes value is 4, the current pixel is equal to the current pixel minus the result of the 'Paeth function' when given the left, above, and upper-left pixels.

    4. Finally, if your image uses the PLTE chunk, replace all the numbers with their corresponding index.
       Otherwise, you should be left with values ordering top-left to bottom-right, with each value representing one color channel.
       If you group these numbers by color channels per pixel, you should have groups with x amount of values, each representing one pixel.
*/


#include <stdio.h>
#include <stdlib.h>
#include "zlib.h"


#define bytesToInt(a, b, c, d) (a << 24 | b << 16 | c << 8 | d)
#define compType(a, b) (a[0]==b[0] && a[1]==b[1] && a[2]==b[2] && a[3]==b[3])

// List of Critical Chunk Types
const unsigned char IEND_CHUNK[4] = {'I', 'E', 'N', 'D'};
const unsigned char IDAT_CHUNK[4] = {'I', 'D', 'A', 'T'};
const unsigned char IHDR_CHUNK[4] = {'I', 'H', 'D', 'R'};
const unsigned char PLTE_CHUNK[4] = {'P', 'L', 'T', 'E'};

// List of Ancillary Chunk Types
const unsigned char BKGD_CHUNK[4] = {'b', 'K', 'G', 'D'};
const unsigned char CHRM_CHUNK[4] = {'c', 'H', 'R', 'M'};
const unsigned char DSIG_CHUNK[4] = {'d', 'S', 'I', 'G'};
const unsigned char EXIF_CHUNK[4] = {'e', 'X', 'I', 'f'};
const unsigned char GAMA_CHUNK[4] = {'g', 'A', 'M', 'A'};
const unsigned char HIST_CHUNK[4] = {'h', 'I', 'S', 'T'};
const unsigned char ICCP_CHUNK[4] = {'i', 'C', 'C', 'P'};
const unsigned char ITXT_CHUNK[4] = {'i', 'T', 'X', 't'};
const unsigned char PHYS_CHUNK[4] = {'p', 'H', 'Y', 's'};
const unsigned char SBIT_CHUNK[4] = {'s', 'B', 'I', 'T'};
const unsigned char SPLT_CHUNK[4] = {'s', 'P', 'L', 'T'};
const unsigned char SRGB_CHUNK[4] = {'s', 'R', 'G', 'B'};
const unsigned char STER_CHUNK[4] = {'s', 'T', 'E', 'R'};
const unsigned char TEXT_CHUNK[4] = {'t', 'E', 'X', 't'};
const unsigned char TIME_CHUNK[4] = {'t', 'I', 'M', 'E'};
const unsigned char TRNS_CHUNK[4] = {'t', 'R', 'N', 'S'};
const unsigned char ZTXT_CHUNK[4] = {'z', 'T', 'X', 't'};

// An Example Of A Valid PNG Header
const unsigned char SIGNATURE[8] = {0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A};


typedef struct Pix {
    int RGBA[4];
} Pix;
typedef struct PLTE {
    Pix* indexes;
    int indexCount;
} PLTE;
typedef struct Chunk {
    int length;
    unsigned char type[4];
    unsigned char *data;
    unsigned char crc[4];
} Chunk;
typedef struct IHDR {
    int width;
    int height;
    int bitd;
    int colort;
    int compm;
    int filterm;
    int interlacem;
    int channels;
} IHDR;
typedef struct PNG {
    Chunk* chunks;
    int chunkLength;

    unsigned char* bytes;
    long int byteLength;

    IHDR iheader;
    PLTE palette;
    Pix* pixels;
} PNG;


PNG getPNGFromPath(char*);
PLTE getPaletteFromChunks(Chunk*, int);
Chunk* getChunksFromBytes(unsigned char*);

int hasValidCRC(Chunk*);
int getChunkCount(unsigned char*);
int hasValidBitDepth(int, int);
int hasValidSignature(unsigned char*);

void throwError(char*, int, int);
void getBytesFromPath(char*, long int*, unsigned char**);
unsigned char* getImgFromChunks(Chunk*, IHDR);
Pix* getPixelsFromImg(unsigned char*, IHDR, PLTE);



int main(int argc, char* argv[])
{
    int output_text = 1;
    PNG fpng = getPNGFromPath(argv[1]);

    if(output_text){
        printf("\n\nthere are %d chunks\n", fpng.chunkLength);
        printf("width: %d - height: %d\n", fpng.iheader.width, fpng.iheader.height);
        printf("color channels: %d\n", fpng.iheader.channels);
        printf("bit depth: %d\n", fpng.iheader.bitd);

        if(fpng.palette.indexCount != 0){
            printf("palette: ");
            for(int j = 0; j < fpng.palette.indexCount; j++){
                printf("(%d,%d,%d) ", fpng.palette.indexes[j].RGBA[0], fpng.palette.indexes[j].RGBA[1], fpng.palette.indexes[j].RGBA[2]);
            }
            printf("\n");
        }
        printf("\n#-#-#-#-#-#-#-#-#-#-#\n");

        for(int i = 0; i < fpng.chunkLength; i++){
            printf("value of type: %.4s\n", fpng.chunks[i].type);
            printf("value of data: %.4s\n", fpng.chunks[i].data);
            printf("value of length: %d\n", fpng.chunks[i].length);
            printf("value of crc: %.4s\n\n", fpng.chunks[i].crc);
            printf(i != fpng.chunkLength-1 ? "--------------------\n" : "\n");
        }
    }

    FILE *fp = fopen("C:\\Users\\mlfre\\OneDrive\\Desktop\\img_temp\\pixels.txt", "w");
    for(int i = 0; i < fpng.iheader.width*fpng.iheader.height; i++){

        if(fpng.palette.indexCount == 0){
            for(int k = 0; k < fpng.iheader.channels; k++){
                fprintf(fp, "%d,", fpng.pixels[i].RGBA[k]);
            }
        }
        else{
            fprintf(fp, "%d,", fpng.pixels[i].RGBA[0]);
            fprintf(fp, "%d,", fpng.pixels[i].RGBA[1]);
            fprintf(fp, "%d,", fpng.pixels[i].RGBA[2]);
        }
    }
    fprintf(fp, "\n");

    FILE *fp2 = fopen("C:\\Users\\mlfre\\OneDrive\\Desktop\\img_temp\\info.txt", "w");
    fprintf(fp2, "%d,", fpng.iheader.width);
    fprintf(fp2, "%d,", fpng.iheader.height);
    if(fpng.palette.indexCount == 0) fprintf(fp2, "%d\n", fpng.iheader.channels);
    else fprintf(fp2, "3");

    fclose(fp);
    fclose(fp2);
    return 0;
}

int getChunkCount(unsigned char* bytes)
{
    int count = 0;
    int length = 0;
    int next_seg = 7;
    unsigned char type[4];

    while(1){
        length = bytesToInt(bytes[next_seg+1], bytes[next_seg+2],
                            bytes[next_seg+3], bytes[next_seg+4]);

        type[0] = bytes[next_seg+5];
        type[1] = bytes[next_seg+6];
        type[2] = bytes[next_seg+7];
        type[3] = bytes[next_seg+8];

        if(compType(type, IEND_CHUNK)) break;
        next_seg+=length+12;
        count++;

    }

    return count+1;
}

void throwError(char* message, int condition, int status)
{
    if(condition){
        fprintf(stderr, message);
        exit(status);
    }
}

void getBytesFromPath(char* path, long int* length, unsigned char** dest)
{
    FILE *fp = fopen(path, "rb");
    throwError("ERROR: could not open file\n\n", fp == NULL, EXIT_FAILURE);

    fseek(fp, 0, SEEK_END);
    *length = ftell(fp);

    rewind(fp);
    *dest = (unsigned char *)malloc(*length+1);
    fread(*dest, *length, 1, fp);
    fclose(fp);

}

PNG getPNGFromPath(char* path)
{
    PNG new_png;
    int channels[7] = {1, 0, 3, 1, 2, 0, 4};

    getBytesFromPath(path, &new_png.byteLength, &new_png.bytes);
    new_png.chunkLength = getChunkCount(new_png.bytes);
    new_png.chunks = getChunksFromBytes(new_png.bytes);

    new_png.iheader.width = bytesToInt(new_png.chunks[0].data[0], new_png.chunks[0].data[1],
                               new_png.chunks[0].data[2], new_png.chunks[0].data[3]);
    new_png.iheader.height = bytesToInt(new_png.chunks[0].data[4], new_png.chunks[0].data[5],
                                new_png.chunks[0].data[6], new_png.chunks[0].data[7]);

    new_png.iheader.bitd = new_png.chunks[0].data[8];
    new_png.iheader.colort = new_png.chunks[0].data[9];
    new_png.iheader.compm = new_png.chunks[0].data[10];

    new_png.iheader.filterm = new_png.chunks[0].data[11];
    new_png.iheader.interlacem = new_png.chunks[0].data[12];
    new_png.iheader.channels = channels[new_png.iheader.colort];

    throwError("ERROR: PNG has an invalid bit depth\n\n", !hasValidBitDepth(new_png.iheader.bitd, new_png.iheader.colort), EXIT_FAILURE);
    throwError("ERROR: PNG has an invalid CRC\n\n", !hasValidCRC(new_png.chunks), EXIT_FAILURE);
    throwError("ERROR: PNG has an invalid signature\n\n", !hasValidSignature(new_png.bytes), EXIT_FAILURE);

    new_png.palette = getPaletteFromChunks(new_png.chunks, new_png.chunkLength);
    unsigned char* temp = getImgFromChunks(new_png.chunks, new_png.iheader);
    new_png.pixels = getPixelsFromImg(temp, new_png.iheader, new_png.palette);

    free(temp);
    return new_png;
}

Chunk* getChunksFromBytes(unsigned char* bytes)
{
    int next_seg = 7;
    int chunks = 0;
    Chunk* chunk_array = (Chunk*) malloc(sizeof(Chunk)*getChunkCount(bytes));

    while(1){
        chunk_array[chunks].length = bytesToInt(bytes[next_seg+1], bytes[next_seg+2],
                                      bytes[next_seg+3], bytes[next_seg+4]);

        chunk_array[chunks].type[0] = bytes[next_seg+5];
        chunk_array[chunks].type[1] = bytes[next_seg+6];
        chunk_array[chunks].type[2] = bytes[next_seg+7];
        chunk_array[chunks].type[3] = bytes[next_seg+8];

        chunk_array[chunks].data = malloc(chunk_array[chunks].length);
        if(chunk_array[chunks].length > 0){
            for(int cnt = 0; cnt < chunk_array[chunks].length; cnt++){
                chunk_array[chunks].data[cnt] = bytes[next_seg+9+cnt];
            }
        }
        else chunk_array[chunks].data = NULL;

        chunk_array[chunks].crc[0] = bytes[next_seg+chunk_array[chunks].length+9];
        chunk_array[chunks].crc[1] = bytes[next_seg+chunk_array[chunks].length+10];
        chunk_array[chunks].crc[2] = bytes[next_seg+chunk_array[chunks].length+11];
        chunk_array[chunks].crc[3] = bytes[next_seg+chunk_array[chunks].length+12];

        if(compType(chunk_array[chunks].type, IEND_CHUNK)) break;
        next_seg+=chunk_array[chunks].length+12;
        chunks++;

    }

    return chunk_array;
}

int hasValidSignature(unsigned char* bytes)
{
    int equal = 1;
    for(int i = 0; i < 8; i++) equal = bytes[i] == SIGNATURE[i] ? equal : 0;
    return equal;
}

int hasValidBitDepth(int bitd, int colort)
{
    if (bitd == 8) return 1;
    switch(colort){
        case 0:
            return bitd == 1 || bitd == 2 || bitd == 4 || bitd == 16;
        case 3:
            return bitd == 1 || bitd == 2 || bitd == 4;
        case 2:
        case 4:
        case 6:
            return bitd == 16;
    }

    return 0;
}

int hasValidCRC(Chunk* chunks)
{
    int count = 0;

    uLong type_crc;
    uLong chunk_crc;
    uLong actual_crc;

    while(!compType(chunks[count].type, IEND_CHUNK))
    {
        type_crc = crc32(0, chunks[count].type, 4);
        chunk_crc = crc32(type_crc, chunks[count].data, chunks[count].length);
        actual_crc = bytesToInt(chunks[count].crc[0], chunks[count].crc[1],
                                chunks[count].crc[2], chunks[count].crc[3]);

        if(actual_crc != chunk_crc) return 0;

        count++;
    }

    return 1;
}

PLTE getPaletteFromChunks(Chunk* chunks, int chunkCount)
{
    PLTE img_palette;

    for(int chk = 0; chk < chunkCount; chk++){
        if(compType(chunks[chk].type, PLTE_CHUNK)){

            img_palette.indexCount = chunks[chk].length/3;
            img_palette.indexes = (Pix*) malloc(sizeof(Pix)*img_palette.indexCount);

            for(int i = 0; i < img_palette.indexCount; i++){
                img_palette.indexes[i].RGBA[0] = chunks[chk].data[(i*3)];
                img_palette.indexes[i].RGBA[1] = chunks[chk].data[(i*3)+1];
                img_palette.indexes[i].RGBA[2] = chunks[chk].data[(i*3)+2];
                img_palette.indexes[i].RGBA[3] = 0;
            }
            break;
        }

        else if(compType(chunks[chk].type, IEND_CHUNK)){
            img_palette.indexes = NULL;
            img_palette.indexCount = 0;
        }
    }

    return img_palette;
}

unsigned char* getImgFromChunks(Chunk* chunks, IHDR img_info)
{
    int count = 0;
    int type_count = 0;
    int idat_count = 0;
    uLongf compressed_size = 0;
    uLongf uncompressed_size = img_info.height*(1+((img_info.bitd*img_info.channels*img_info.width+7)>>3));
    unsigned char* compressed_idat;
    unsigned char* uncompressed_idat = (unsigned char*) malloc(uncompressed_size);


    while(1){
        if (compType(chunks[count].type, IDAT_CHUNK)){
            for(; compType(chunks[count+idat_count].type, IDAT_CHUNK); idat_count++){
                compressed_size += chunks[count+idat_count].length;
            }

            compressed_idat = (unsigned char*) malloc(compressed_size);
            for(int j = 0; j < idat_count; j++){
                for(int i = 0; i < chunks[count+j].length; i++){
                    compressed_idat[type_count] = chunks[count+j].data[i];
                    type_count++;
                }
            }
            break;
        }
        count++;
    }

    int ret = uncompress(uncompressed_idat, &uncompressed_size, compressed_idat, compressed_size);
    free(compressed_idat);

    return ret == 0 ? uncompressed_idat : '\0';
}

int PaethPredictor(int a, int b, int c)
{
    int pa = abs(b - c);
    int pb = abs(a - c);
    int pc = abs(a + b - (2*c));

    if(pa <= pb && pa <= pc) return a;
    else if(pb <= pc) return b;
    return c;
}

Pix* getPixelsFromImg(unsigned char* img, IHDR img_info, PLTE color_indexes)
{
    int stride = ((img_info.bitd * img_info.channels * img_info.width + 7) >> 3);
    int* pixels = (int*) malloc(sizeof(int)*img_info.height*stride);
    Pix* rgba_pixels = (Pix*) malloc(sizeof(Pix)*img_info.width*img_info.height);

    int i = 0;
    int current_pixel = 0;
    int filter_type;

    int filt_a;
    int filt_b;
    int filt_c;

    int bpp = stride/img_info.width;

    for(int r = 0; r < img_info.height; r++){
        filter_type = img[i];
        i++;
        for(int c = 0; c < stride; c++){

            filt_a = c >= bpp ? pixels[r * stride + c - bpp] : 0;
            filt_b = r > 0 ? pixels[(r - 1) * stride + c] : 0;
            filt_c = r > 0 && c >= bpp ? pixels[(r - 1) * stride + c - bpp] : 0;

            switch(filter_type){
                case 0:
                    pixels[current_pixel] = img[i] & 0xff;
                    break;
                case 1:
                    pixels[current_pixel] = (img[i] + filt_a) & 0xff;
                    break;
                case 2:
                    pixels[current_pixel] = (img[i] + filt_b) & 0xff;
                    break;
                case 3:
                    pixels[current_pixel] = (img[i] + filt_a + filt_c) & 0xff;
                    break;
                case 4:
                    pixels[current_pixel] = (img[i] + PaethPredictor(filt_a, filt_b, filt_c)) & 0xff;
                    break;
            }
            current_pixel++;
            i++;
        }
    }

    for(int pixl = 0; pixl < img_info.width*img_info.height; pixl++){
        rgba_pixels[pixl].RGBA[0] = 0;
        rgba_pixels[pixl].RGBA[1] = 0;
        rgba_pixels[pixl].RGBA[2] = 0;
        rgba_pixels[pixl].RGBA[3] = 0;

        if(color_indexes.indexCount == 0){
            for(int chnl = 0; chnl < img_info.channels; chnl++){
                rgba_pixels[pixl].RGBA[chnl] = pixels[(pixl*img_info.channels)+chnl];
            }
        }
        else{
            rgba_pixels[pixl].RGBA[0] = color_indexes.indexes[pixels[pixl]].RGBA[0];
            rgba_pixels[pixl].RGBA[1] = color_indexes.indexes[pixels[pixl]].RGBA[1];
            rgba_pixels[pixl].RGBA[2] = color_indexes.indexes[pixels[pixl]].RGBA[2];
        }
    }

    free(pixels);
    return rgba_pixels;
}
