#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PICO_IMPLEMENTATION
#include "pico/picoStream.h"

static const uint8_t PNG_SIGNATURE[8] = {0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A};

typedef struct {
    uint32_t length;
    char type[5]; 
    uint32_t crc;
} PNGChunk;

void printUsage(const char *progName)
{
    printf("PNG Tags Parser - picoStream Demo\n");
    printf("Usage: %s <png_file>\n", progName);
}

bool verifyPNGSignature(picoStream stream)
{
    uint8_t signature[8];
    size_t bytesRead = picoStreamRead(stream, signature, 8);
    
    if (bytesRead != 8) {
        printf("Error: Could not read PNG signature\n");
        return false;
    }
    
    for (int i = 0; i < 8; i++) {
        if (signature[i] != PNG_SIGNATURE[i]) {
            printf("Error: Invalid PNG signature\n");
            return false;
        }
    }
    
    printf("Valid PNG signature detected\n\n");
    return true;
}

void parsePNGChunk(picoStream stream, PNGChunk *chunk)
{
    picoStreamSetEndianess(stream, false);
    
    chunk->length = picoStreamReadU32(stream);
    
    picoStreamRead(stream, chunk->type, 4);
    chunk->type[4] = '\0';
    
    if (chunk->length > 0) {
        picoStreamSeek(stream, chunk->length, PICO_STREAM_SEEK_CUR);
    }
    
    chunk->crc = picoStreamReadU32(stream);
}

void printChunkInfo(const PNGChunk *chunk, int chunkNum)
{
    printf("Chunk #%d:\n", chunkNum);
    printf("  Type:   %s", chunk->type);
    
    if (strcmp(chunk->type, "IHDR") == 0) {
        printf(" (Image Header)");
    } else if (strcmp(chunk->type, "PLTE") == 0) {
        printf(" (Palette)");
    } else if (strcmp(chunk->type, "IDAT") == 0) {
        printf(" (Image Data)");
    } else if (strcmp(chunk->type, "IEND") == 0) {
        printf(" (Image End)");
    } else if (strcmp(chunk->type, "tEXt") == 0) {
        printf(" (Text)");
    } else if (strcmp(chunk->type, "iTXt") == 0) {
        printf(" (International Text)");
    } else if (strcmp(chunk->type, "zTXt") == 0) {
        printf(" (Compressed Text)");
    } else if (strcmp(chunk->type, "tIME") == 0) {
        printf(" (Modification Time)");
    } else if (strcmp(chunk->type, "pHYs") == 0) {
        printf(" (Physical Dimensions)");
    } else if (strcmp(chunk->type, "gAMA") == 0) {
        printf(" (Gamma)");
    } else if (strcmp(chunk->type, "cHRM") == 0) {
        printf(" (Chromaticity)");
    } else if (strcmp(chunk->type, "sRGB") == 0) {
        printf(" (Standard RGB)");
    } else if (strcmp(chunk->type, "iCCP") == 0) {
        printf(" (ICC Profile)");
    }
    
    printf("\n");
    printf("  Length: %u bytes\n", chunk->length);
    printf("  CRC:    0x%08X\n", chunk->crc);
    
    bool isCritical = (chunk->type[0] & 0x20) == 0;
    printf("  Type:   %s\n", isCritical ? "Critical" : "Ancillary");
    
    printf("\n");
}

int main(int argc, char *argv[])
{
    printf("Hello, Pico!\n");

    if (argc < 2) {
        printUsage(argv[0]);
        return 1;
    }
    
    const char *filePath = argv[1];
    printf("Parsing PNG file: %s\n", filePath);
    
    picoStream stream = picoStreamFromFilePath(filePath, true, false);
    if (!stream) {
        printf("Error: Could not open file '%s'\n", filePath);
        return 1;
    }
    
    printf("Stream created successfully\n");
    printf("Can read:  %s\n", picoStreamCanRead(stream) ? "Yes" : "No");
    printf("Can write: %s\n", picoStreamCanWrite(stream) ? "No" : "Yes");
    printf("\n");
    
    if (!verifyPNGSignature(stream)) {
        picoStreamDestroy(stream);
        return 1;
    }
    
    printf("PNG Chunks\n");
    int chunkNum = 0;
    bool foundIEND = false;
    
    while (!foundIEND) {
        PNGChunk chunk;
        int64_t currentPos = picoStreamTell(stream);
        
        size_t bytesRead = picoStreamRead(stream, &chunk.length, 1);
        if (bytesRead == 0) {
            break; 
        }
        
        picoStreamSeek(stream, currentPos, PICO_STREAM_SEEK_SET);
        parsePNGChunk(stream, &chunk);
        
        chunkNum++;
        printChunkInfo(&chunk, chunkNum);
        
        if (strcmp(chunk.type, "IEND") == 0) {
            foundIEND = true;
        }
        
        if (chunkNum > 1000) {
            printf("Warning: Too many chunks, stopping parse\n");
            break;
        }
    }
    
    printf("Total chunks parsed: %d\n", chunkNum);
    
    picoStreamDestroy(stream);
    
    printf("Goodbye, Pico!\n");
    return 0;
}