/*
MIT License

Copyright (c) 2026 Jaysmito Mukherjee (jaysmito101@gmail.com)

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

/*
 * This has been purely implemented based of the ITU-T H.264 (V15) (08/2024) specification.
 * - Jaysmito Mukherjee (jaysmito101@gmail.com)
 */

// NOTE: It is important to keep in mind that this is NOT A DECODER. This library is only meant to parse H.264 bitstreams
//       and extract NAL units, slices, headers and other metadata from them. Actual decoding of video frames is outside the scope of this library.
//       This library can be used as a pre-processing step before feeding the data to an actual H.264 decoder.
//       The primary goal of this library is to provide a simple and lightweight way to parse H.264 streams to be used
//       together with decoder applications like Vulkan Video, DirectX Video, etc.

#ifndef PICO_H264_H
#define PICO_H264_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifndef PICO_MALLOC
#define PICO_MALLOC malloc
#define PICO_FREE   free
#endif

#ifndef PICO_REALLOC
#define PICO_REALLOC realloc
#endif

#ifndef PICO_ASSERT
#include <assert.h>
#define PICO_ASSERT(expr) assert(expr)
#endif

#ifndef PICO_H264_LOG
#define PICO_H264_LOG(...) \
    do {                   \
        (void)0;           \
    } while (0)
#endif

typedef size_t (*picoH264BitstreamReadFunc)(void *userData, uint8_t *buffer, size_t size);
typedef bool (*picoH264BitstreamSeekFunc)(void *userData, int64_t offset, int origin);
typedef size_t (*picoH264BitstreamTellFunc)(void *userData);

typedef struct {
    void *userData;

    picoH264BitstreamReadFunc read;
    picoH264BitstreamSeekFunc seek;
    picoH264BitstreamTellFunc tell;
} picoH264Bitsteam_t;
typedef picoH264Bitsteam_t *picoH264Bitstream;

typedef enum {
    PICO_H264_NAL_UNIT_TYPE_UNSPECIFIED             = 0,
    PICO_H264_NAL_UNIT_TYPE_CODED_SLICE_NON_IDR     = 1,
    PICO_H264_NAL_UNIT_TYPE_CODED_SLICE_DATA_PART_A = 2,
    PICO_H264_NAL_UNIT_TYPE_CODED_SLICE_DATA_PART_B = 3,
    PICO_H264_NAL_UNIT_TYPE_CODED_SLICE_DATA_PART_C = 4,
    PICO_H264_NAL_UNIT_TYPE_CODED_SLICE_IDR         = 5,
    PICO_H264_NAL_UNIT_TYPE_SEI                     = 6,
    PICO_H264_NAL_UNIT_TYPE_SPS                     = 7,
    PICO_H264_NAL_UNIT_TYPE_PPS                     = 8,
    PICO_H264_NAL_UNIT_TYPE_AUD                     = 9,
    PICO_H264_NAL_UNIT_TYPE_END_OF_SEQUENCE         = 10,
    PICO_H264_NAL_UNIT_TYPE_END_OF_STREAM           = 11,
    PICO_H264_NAL_UNIT_TYPE_FILLER_DATA             = 12,
    PICO_H264_NAL_UNIT_TYPE_SPS_EXT                 = 13,
    PICO_H264_NAL_UNIT_TYPE_PREFIX_NAL_UNIT         = 14,
    PICO_H264_NAL_UNIT_TYPE_SUBSET_SPS              = 15,
    PICO_H264_NAL_UNIT_TYPE_DEPTH_PARAMETER_SET     = 16,
    PICO_H264_NAL_UNIT_TYPE_RESERVED_17             = 17,
    PICO_H264_NAL_UNIT_TYPE_RESERVED_18             = 18,
    PICO_H264_NAL_UNIT_TYPE_AUXILIARY_SLICE         = 19,
    PICO_H264_NAL_UNIT_TYPE_SLICE_EXTENSION         = 20,
    PICO_H264_NAL_UNIT_TYPE_DEPTH_SLICE_EXTENSION   = 21,
    PICO_H264_NAL_UNIT_TYPE_RESERVED_22             = 22,
    PICO_H264_NAL_UNIT_TYPE_RESERVED_23             = 23,
    PICO_H264_NAL_UNIT_TYPE_UNSPECIFIED_24          = 24,
    PICO_H264_NAL_UNIT_TYPE_UNSPECIFIED_25          = 25,
    PICO_H264_NAL_UNIT_TYPE_UNSPECIFIED_26          = 26,
    PICO_H264_NAL_UNIT_TYPE_UNSPECIFIED_27          = 27,
    PICO_H264_NAL_UNIT_TYPE_UNSPECIFIED_28          = 28,
    PICO_H264_NAL_UNIT_TYPE_UNSPECIFIED_29          = 29,
    PICO_H264_NAL_UNIT_TYPE_UNSPECIFIED_30          = 30,
    PICO_H264_NAL_UNIT_TYPE_UNSPECIFIED_31          = 31,
} picoH264NALUnitType;

typedef struct {
    bool isReferencePicture;
    picoH264NALUnitType nalUnitType;

} picoH264NALUnitHeader_t;
typedef picoH264NALUnitHeader_t *picoH264NALUnitHeader;

picoH264Bitstream picoH264BitstreamFromBuffer(const uint8_t *buffer, size_t size);
void picoH264BitstreamDestroy(picoH264Bitstream bitstream);

bool picoH264FindNextNALUnit(picoH264Bitstream bitstream, size_t *nalUnitSizeOut);
bool picoH264ReadNALUnit(picoH264Bitstream bitstream, uint8_t *nalUnitBuffer, size_t nalUnitBufferSize, size_t nalUnitSizeOut);
bool picoH264ParseNALUnitHeader(const uint8_t *nalUnitBuffer, size_t nalUnitSize, picoH264NALUnitHeader nalUnitHeaderOut);

const char* picoH264GetNALUnitTypeToString(picoH264NALUnitType nalUnitType);

#define PICO_IMPLEMENTATION // for testing purposes only

#if defined(PICO_IMPLEMENTATION) && !defined(PICO_H264_IMPLEMENTATION)
#define PICO_H264_IMPLEMENTATION
#endif

#ifdef PICO_H264_IMPLEMENTATION

typedef struct {
    const uint8_t *buffer;
    size_t size;
    size_t position;
} __picoH264BitstreamBufferContext_t;
typedef __picoH264BitstreamBufferContext_t *__picoH264BitstreamBufferContext;

static size_t __picoH264BufferRead(void *userData, uint8_t *buffer, size_t size)
{
    __picoH264BitstreamBufferContext context = (__picoH264BitstreamBufferContext)userData;
    PICO_ASSERT(context != NULL);

    size_t bytesAvailable = context->size - context->position;
    size_t bytesToRead    = (size < bytesAvailable) ? size : bytesAvailable;

    if (bytesToRead > 0 && buffer) {
        memcpy(buffer, context->buffer + context->position, bytesToRead);
    }

    context->position += bytesToRead;

    return bytesToRead;
}

static bool __picoH264BufferSeek(void *userData, int64_t offset, int origin)
{
    __picoH264BitstreamBufferContext context = (__picoH264BitstreamBufferContext)userData;
    PICO_ASSERT(context != NULL);

    size_t newPosition = 0;

    switch (origin) {
        case SEEK_SET:
            if (offset < 0 || (size_t)offset > context->size) {
                return false;
            }
            newPosition = (size_t)offset;
            break;
        case SEEK_CUR:
            if ((offset < 0 && (size_t)(-offset) > context->position) ||
                (offset > 0 && context->position + (size_t)offset > context->size)) {
                return false;
            }
            newPosition = context->position + (size_t)offset;
            break;
        case SEEK_END:
            if (offset > 0 || (size_t)(-offset) > context->size) {
                return false;
            }
            newPosition = context->size + (size_t)offset;
            break;
        default:
            return false;
    }

    context->position = newPosition;
    return true;
}

static size_t __picoH264BufferTell(void *userData)
{
    __picoH264BitstreamBufferContext context = (__picoH264BitstreamBufferContext)userData;
    PICO_ASSERT(context != NULL);

    return context->position;
}

static bool __picoH264FindNextNALUnit(picoH264Bitstream bitstream)
{
    PICO_ASSERT(bitstream != NULL);
    PICO_ASSERT(bitstream->read != NULL);
    PICO_ASSERT(bitstream->seek != NULL);
    PICO_ASSERT(bitstream->tell != NULL);

    uint8_t byte     = 0;
    int zeroCount    = 0;
    size_t readBytes = 0;

    // scan byte wise looking for start code prefix (00 00 01) or (00 00 00 01)
    // when the terminating 01 is read and we have previously seen at least
    // two 00 bytes the stream is positioned after the start code so we can return true.
    while (1) {
        readBytes = bitstream->read(bitstream->userData, &byte, 1);
        if (readBytes == 0) {
            // eof reached
            return false;
        }

        if (byte == 0x00) {
            if (zeroCount < 3)
                zeroCount++;
            continue;
        }

        if (byte == 0x01 && zeroCount >= 2) {
            // go back to position just before the start code (if its a 4 byte start code we went back 3 bytes,
            // as then we get a uniform 2 zero bytes before the 01 byte)
            bitstream->seek(bitstream->userData, -((int64_t)(3)), SEEK_CUR);
            return true; // Found start code (00 00 01) or (00 00 00 01)
        }

        zeroCount = 0;
    }
}

picoH264Bitstream picoH264BitstreamFromBuffer(const uint8_t *buffer, size_t size)
{
    PICO_ASSERT(buffer != NULL);
    PICO_ASSERT(size > 0);

    picoH264Bitstream bitstream = (picoH264Bitstream)PICO_MALLOC(sizeof(picoH264Bitsteam_t));
    if (!bitstream) {
        PICO_H264_LOG("picoH264BitstreamFromBuffer: Failed to allocate memory for bitstream\n");
        return NULL;
    }

    __picoH264BitstreamBufferContext context;
    context = (__picoH264BitstreamBufferContext)PICO_MALLOC(sizeof(__picoH264BitstreamBufferContext_t));
    if (!context) {
        PICO_H264_LOG("picoH264BitstreamFromBuffer: Failed to allocate memory for bitstream context\n");
        PICO_FREE(bitstream);
        return NULL;
    }

    context->buffer   = buffer;
    context->size     = size;
    context->position = 0;

    bitstream->userData = context;
    bitstream->read     = __picoH264BufferRead;
    bitstream->seek     = __picoH264BufferSeek;
    bitstream->tell     = __picoH264BufferTell;

    return bitstream;
}

void picoH264BitstreamDestroy(picoH264Bitstream bitstream)
{
    PICO_ASSERT(bitstream != NULL);
    if (bitstream->userData) {
        PICO_FREE(bitstream->userData);
    }
    PICO_FREE(bitstream);
}

bool picoH264FindNextNALUnit(picoH264Bitstream bitstream, size_t *nalUnitSizeOut)
{
    PICO_ASSERT(bitstream != NULL);

    if (!__picoH264FindNextNALUnit(bitstream)) {
        // no more NAL units found
        return false;
    }

    // if we did find a NAL unit, we are now positioned just before the start code
    // so now we can try to find the next start code to determine the size of this NAL unit
    size_t nalStartPos = bitstream->tell(bitstream->userData);
    // skip the start code
    uint8_t startCode[3];
    bitstream->read(bitstream->userData, startCode, 3);
    if (!(startCode[0] == 0x00 && startCode[1] == 0x00 && startCode[2] == 0x01)) {
        // NOTE: ideally this case should not happen as we already verified the
        // start code in __picoH264FindNextNALUnit
        PICO_H264_LOG("picoH264FindNextNALUnit: Invalid start code\n");
        return false; // invalid start code
    }

    // try to find the next NAL unit
    (void)__picoH264FindNextNALUnit(bitstream);

    size_t nalEndPos   = bitstream->tell(bitstream->userData);
    size_t nalUnitSize = nalEndPos - nalStartPos;
    if (nalUnitSizeOut) {
        *nalUnitSizeOut = nalUnitSize;
    }

    // rewind to the start of this NAL unit for further processing
    bitstream->seek(bitstream->userData, (int64_t)nalStartPos, SEEK_SET);
    return true;
}

bool picoH264ReadNALUnit(picoH264Bitstream bitstream, uint8_t *nalUnitBuffer, size_t nalUnitBufferSize, size_t nalUnitSizeOut)
{
    PICO_ASSERT(bitstream != NULL);
    PICO_ASSERT(nalUnitBuffer != NULL);
    PICO_ASSERT(nalUnitBufferSize >= nalUnitSizeOut);

    size_t bytesRead = bitstream->read(bitstream->userData, nalUnitBuffer, nalUnitSizeOut);
    if (bytesRead != nalUnitSizeOut) {
        PICO_H264_LOG("picoH264ReadNALUnit: Failed to read complete NAL unit\n");
        return false;
    }

    return true;
}

const char* picoH264GetNALUnitTypeToString(picoH264NALUnitType nalUnitType)
{
    switch (nalUnitType) {
        case PICO_H264_NAL_UNIT_TYPE_UNSPECIFIED:
            return "Unspecified";
        case PICO_H264_NAL_UNIT_TYPE_CODED_SLICE_NON_IDR:
            return "Coded slice of a non-IDR picture";
        case PICO_H264_NAL_UNIT_TYPE_CODED_SLICE_DATA_PART_A:
            return "Coded slice data partition A";
        case PICO_H264_NAL_UNIT_TYPE_CODED_SLICE_DATA_PART_B:
            return "Coded slice data partition B";
        case PICO_H264_NAL_UNIT_TYPE_CODED_SLICE_DATA_PART_C:
            return "Coded slice data partition C";
        case PICO_H264_NAL_UNIT_TYPE_CODED_SLICE_IDR:
            return "Coded slice of an IDR picture";
        case PICO_H264_NAL_UNIT_TYPE_SEI:
            return "Supplemental enhancement information (SEI)";
        case PICO_H264_NAL_UNIT_TYPE_SPS:
            return "Sequence parameter set (SPS)";
        case PICO_H264_NAL_UNIT_TYPE_PPS:
            return "Picture parameter set (PPS)";
        case PICO_H264_NAL_UNIT_TYPE_AUD:
            return "Access unit delimiter (AUD)";
        case PICO_H264_NAL_UNIT_TYPE_END_OF_SEQUENCE:
            return "End of sequence";
        case PICO_H264_NAL_UNIT_TYPE_END_OF_STREAM:
            return "End of stream";
        case PICO_H264_NAL_UNIT_TYPE_FILLER_DATA:
            return "Filler data";
        case PICO_H264_NAL_UNIT_TYPE_SPS_EXT:
            return "Sequence parameter set extension";
        case PICO_H264_NAL_UNIT_TYPE_PREFIX_NAL_UNIT:
            return "Prefix NAL unit";
        case PICO_H264_NAL_UNIT_TYPE_SUBSET_SPS:
            return "Subset sequence parameter set";
        case PICO_H264_NAL_UNIT_TYPE_DEPTH_PARAMETER_SET:
            return "Depth parameter set";
        case PICO_H264_NAL_UNIT_TYPE_RESERVED_17:
            return "Reserved (17)";
        case PICO_H264_NAL_UNIT_TYPE_RESERVED_18:
            return "Reserved (18)";
        case PICO_H264_NAL_UNIT_TYPE_AUXILIARY_SLICE:
            return "Coded slice of an auxiliary coded picture without partitioning";
        case PICO_H264_NAL_UNIT_TYPE_SLICE_EXTENSION:
            return "Coded slice extension";
        case PICO_H264_NAL_UNIT_TYPE_DEPTH_SLICE_EXTENSION:
            return "Coded slice extension for a depth view component";
        case PICO_H264_NAL_UNIT_TYPE_RESERVED_22:
            return "Reserved (22)";
        case PICO_H264_NAL_UNIT_TYPE_RESERVED_23:
            return "Reserved (23)";
        case PICO_H264_NAL_UNIT_TYPE_UNSPECIFIED_24:
            return "Unspecified (24)";
        case PICO_H264_NAL_UNIT_TYPE_UNSPECIFIED_25:
            return "Unspecified (25)";
        case PICO_H264_NAL_UNIT_TYPE_UNSPECIFIED_26:
            return "Unspecified (26)";
        case PICO_H264_NAL_UNIT_TYPE_UNSPECIFIED_27:
            return "Unspecified (27)";
        case PICO_H264_NAL_UNIT_TYPE_UNSPECIFIED_28:
            return "Unspecified (28)";
        case PICO_H264_NAL_UNIT_TYPE_UNSPECIFIED_29:
            return "Unspecified (29)";
        case PICO_H264_NAL_UNIT_TYPE_UNSPECIFIED_30:
            return "Unspecified (30)";
        case PICO_H264_NAL_UNIT_TYPE_UNSPECIFIED_31:
            return "Unspecified (31)";
        default:
            return "Unknown NAL unit type";
    }
}

#endif // PICO_H264_IMPLEMENTATION

#endif // PICO_H264_H
