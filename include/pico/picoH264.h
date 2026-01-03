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

#include "pico/picoH264.h"
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
    bool idrFlag;
    uint8_t priorityId;
    bool noInterLayerPredFlag;
    uint8_t dependencyId;
    uint8_t qualityId;
    uint8_t temporalId;
    bool useRefBasePicFlag;
    bool discardableFlag;
    bool outputFlag;
} picoH264NALUnitHeaderSVCExtension_t;
typedef picoH264NALUnitHeaderSVCExtension_t *picoH264NALUnitHeaderSVCExtension;

typedef struct {
    uint8_t viewId;
    bool depthFlag;
    bool nonIDRFlag;
    uint8_t temporalId;
    bool anchorPicFlag;
    bool interViewFlag;
} picoH264NALUnitHeader3DAVCExtension_t;
typedef picoH264NALUnitHeader3DAVCExtension_t *picoH264NALUnitHeader3DAVCExtension;

typedef struct {
    bool nonIdrFlag;
    uint8_t priorityId;
    uint16_t viewId;
    uint8_t temporalId;
    bool anchorPicFlag;
    bool interViewFlag;
} picoH264NALUnitHeaderMVCExtension_t;
typedef picoH264NALUnitHeaderMVCExtension_t *picoH264NALUnitHeaderMVCExtension;

typedef struct {
    bool isReferencePicture;
    picoH264NALUnitType nalUnitType;

    bool svcExtensionFlag;
    picoH264NALUnitHeaderSVCExtension_t svcExtension;

    bool avc3DExtensionFlag;
    picoH264NALUnitHeader3DAVCExtension_t avc3DExtension;

    bool mvcExtensionFlag;
    picoH264NALUnitHeaderMVCExtension_t mvcExtension;

    uint32_t zeroCount;

    uint32_t numBytesInNALHeader;
    size_t numBytesInNALUnit;
    size_t numBytesInPayload;
} picoH264NALUnitHeader_t;
typedef picoH264NALUnitHeader_t *picoH264NALUnitHeader;

picoH264Bitstream picoH264BitstreamFromBuffer(const uint8_t *buffer, size_t size);
void picoH264BitstreamDestroy(picoH264Bitstream bitstream);

bool picoH264FindNextNALUnit(picoH264Bitstream bitstream, size_t *nalUnitSizeOut);
bool picoH264ReadNALUnit(picoH264Bitstream bitstream, uint8_t *nalUnitBuffer, size_t nalUnitBufferSize, size_t nalUnitSizeOut);
bool picoH264ParseNALUnit(const uint8_t *nalUnitBuffer, size_t nalUnitSize, picoH264NALUnitHeader nalUnitHeaderOut, uint8_t *nalPayloadOut, size_t *nalPayloadSizeOut);

void picoH264NALUnitHeaderDebugPrint(picoH264NALUnitHeader nalUnitHeader);
const char *picoH264GetNALUnitTypeToString(picoH264NALUnitType nalUnitType);

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

static size_t __picoH264BitstreamBufferRead(void *userData, uint8_t *buffer, size_t size)
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

static bool __picoH264BitstreamBufferSeek(void *userData, int64_t offset, int origin)
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

static size_t __picoH264BitstreamBufferTell(void *userData)
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
            if (zeroCount > 3) {
                zeroCount = 3; // clamp to max 3 zero bytes
            }
            // go back to position just before the start code (if its a 4 byte start code we went back 3 bytes,
            // as then we get a uniform 2 zero bytes before the 01 byte)
            bitstream->seek(bitstream->userData, -((int64_t)zeroCount + 1), SEEK_CUR);
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
    bitstream->read     = __picoH264BitstreamBufferRead;
    bitstream->seek     = __picoH264BitstreamBufferSeek;
    bitstream->tell     = __picoH264BitstreamBufferTell;

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
    bitstream->read(bitstream->userData, NULL, 3); // skip the start code (at least 3 bytes)
    
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

static bool __picoH264ParseNALUnitHeaderSVCExtension(const uint8_t *nalUnitBuffer, size_t nalUnitSize, picoH264NALUnitHeaderSVCExtension nalUnitHeaderSVCExtensionOut)
{
    PICO_ASSERT(nalUnitBuffer != NULL);
    PICO_ASSERT(nalUnitSize > 0);
    PICO_ASSERT(nalUnitHeaderSVCExtensionOut != NULL);

    if (nalUnitSize < 3) {
        PICO_H264_LOG("picoH264ParseNALUnitHeaderSVCExtension: NAL unit too small to contain SVC extension\n");
        return false;
    }

    const uint8_t *ptr = nalUnitBuffer;

    // the first bit is the flag, which is already parsed in the main function, so ignore it,
    // the next 23 bits contain the SVC extension data
    uint8_t svcExtensionByte1 = *(ptr++);
    uint8_t svcExtensionByte2 = *(ptr++);
    uint8_t svcExtensionByte3 = *(ptr++);
    uint32_t bits = ((uint32_t)(svcExtensionByte1 & 0x7F) << 16) | ((uint32_t)svcExtensionByte2 << 8) | (uint32_t)svcExtensionByte3;

    nalUnitHeaderSVCExtensionOut->idrFlag                = ((bits >> 22) & 0x1) != 0;
    nalUnitHeaderSVCExtensionOut->priorityId             = (uint8_t)((bits >> 16) & 0x3F);
    nalUnitHeaderSVCExtensionOut->noInterLayerPredFlag   = ((bits >> 15) & 0x1) != 0;
    nalUnitHeaderSVCExtensionOut->dependencyId           = (uint8_t)((bits >> 12) & 0x7);
    nalUnitHeaderSVCExtensionOut->qualityId              = (uint8_t)((bits >> 8) & 0xF);
    nalUnitHeaderSVCExtensionOut->temporalId             = (uint8_t)((bits >> 5) & 0x7);
    nalUnitHeaderSVCExtensionOut->useRefBasePicFlag      = ((bits >> 4) & 0x1) != 0;
    nalUnitHeaderSVCExtensionOut->discardableFlag        = ((bits >> 3) & 0x1) != 0;
    nalUnitHeaderSVCExtensionOut->outputFlag             = ((bits >> 2) & 0x1) != 0;

    return true;
}

static bool __picoH264ParseNALUnitHeader3DAVCExtension(const uint8_t* nalUnitBuffer, size_t nalUnitSize, picoH264NALUnitHeader3DAVCExtension nalUnitHeader3DAVCExtensionOut)
{
    PICO_ASSERT(nalUnitBuffer != NULL);
    PICO_ASSERT(nalUnitSize > 0);
    PICO_ASSERT(nalUnitHeader3DAVCExtensionOut != NULL);

    if (nalUnitSize < 2) {
        PICO_H264_LOG("picoH264ParseNALUnitHeader3DAVCExtension: NAL unit too small to contain 3D AVC extension\n");
        return false;
    }

    const uint8_t *ptr = nalUnitBuffer;

    uint8_t avc3DExtensionByte1 = *(ptr++);
    uint8_t avc3DExtensionByte2 = *(ptr++);

    uint16_t bits = ((uint16_t)(avc3DExtensionByte1 & 0x7F) << 8) | (uint16_t)avc3DExtensionByte2;

    uint8_t viewIdx = (uint8_t)((bits >> 7) & 0xFF);
    uint8_t flags   = (uint8_t)(bits & 0x7F);

    nalUnitHeader3DAVCExtensionOut->viewId        = viewIdx;
    nalUnitHeader3DAVCExtensionOut->depthFlag     = (flags & 0x40) != 0; 
    nalUnitHeader3DAVCExtensionOut->nonIDRFlag    = (flags & 0x20) != 0;
    nalUnitHeader3DAVCExtensionOut->temporalId    = (uint8_t)((flags >> 2) & 0x07); 
    nalUnitHeader3DAVCExtensionOut->anchorPicFlag = (flags & 0x02) != 0; 
    nalUnitHeader3DAVCExtensionOut->interViewFlag = (flags & 0x01) != 0; 

    return true;
}

static bool __picoH264ParseNALUnitHeaderMVCCExtension(const uint8_t* nalUnitBuffer, size_t nalUnitSize, picoH264NALUnitHeaderMVCExtension nalUnitHeaderMVCExtensionOut)
{
    PICO_ASSERT(nalUnitBuffer != NULL);
    PICO_ASSERT(nalUnitSize > 0);
    PICO_ASSERT(nalUnitHeaderMVCExtensionOut != NULL);

    if (nalUnitSize < 3) {
        PICO_H264_LOG("picoH264ParseNALUnitHeaderMVCCExtension: NAL unit too small to contain MVC extension\n");
        return false;
    }

    const uint8_t *ptr = nalUnitBuffer;

    uint8_t mvcExtensionByte1 = *(ptr++);
    uint8_t mvcExtensionByte2 = *(ptr++);
    uint8_t mvcExtensionByte3 = *(ptr++);

    uint32_t bits = ((uint32_t)mvcExtensionByte1 << 16) | ((uint32_t)mvcExtensionByte2 << 8) | (uint32_t)mvcExtensionByte3;

    nalUnitHeaderMVCExtensionOut->nonIdrFlag     = ((bits >> 23) & 0x1) != 0;
    nalUnitHeaderMVCExtensionOut->priorityId     = (uint8_t)((bits >> 17) & 0x3F);
    nalUnitHeaderMVCExtensionOut->viewId         = (uint16_t)((bits >> 7) & 0x3FF);
    nalUnitHeaderMVCExtensionOut->temporalId     = (uint8_t)((bits >> 4) & 0x7);
    nalUnitHeaderMVCExtensionOut->anchorPicFlag  = ((bits >> 3) & 0x1) != 0;
    nalUnitHeaderMVCExtensionOut->interViewFlag  = ((bits >> 2) & 0x1) != 0;

    return true;
}

bool picoH264ParseNALUnit(const uint8_t *nalUnitBuffer, size_t nalUnitSize, picoH264NALUnitHeader nalUnitHeaderOut, uint8_t *nalPayloadOut, size_t *nalPayloadSizeOut)
{
    PICO_ASSERT(nalUnitBuffer != NULL);
    PICO_ASSERT(nalUnitSize > 0);
    PICO_ASSERT(nalUnitHeaderOut != NULL);

    (void)nalPayloadOut;
    (void)nalPayloadSizeOut;

    nalUnitHeaderOut->numBytesInNALUnit   = nalUnitSize;
    nalUnitHeaderOut->numBytesInNALHeader = 0;

    const uint8_t *nalUnitBufferEnd = nalUnitBuffer + nalUnitSize;

    nalUnitHeaderOut->zeroCount = 0;
    while (nalUnitBuffer < nalUnitBufferEnd) {
        if (*nalUnitBuffer == 0x00) {
            nalUnitHeaderOut->zeroCount++;
            nalUnitBuffer++;
        } else if (*nalUnitBuffer == 0x01 && nalUnitHeaderOut->zeroCount >= 2) {
            if (nalUnitHeaderOut->zeroCount > 3) {
                // this means we are dealing with an invalid start code
                // this should have been dealt with in the NAL unit finder already
                // so encountering this case here means the input is malformed
                PICO_H264_LOG("picoH264ParseNALUnitHeader: Invalid start code in NAL unit, more than 3 zero bytes\n");
                return false;
            }
            // Found start code (00 00 01) or (00 00 00 01)
            nalUnitBuffer++; // move past the 01 byte
            break;
        } else {
            // Invalid start code
            PICO_H264_LOG("picoH264ParseNALUnitHeader: Invalid start code in NAL unit with less than 2 zero bytes\n");
            return false;
        }
    }

    if (nalUnitBuffer >= nalUnitBufferEnd) {
        PICO_H264_LOG("picoH264ParseNALUnitHeader: NAL unit too small to contain header\n");
        return false;
    }

    uint8_t firstNALByte = *(nalUnitBuffer++);
    // check for the first forbidden zero bit
    if ((firstNALByte & 0x80) != 0) {
        PICO_H264_LOG("picoH264ParseNALUnitHeader: Forbidden zero bit is not zero\n");
        return false;
    }
    // the next two bits indicate if this is a reference picture
    nalUnitHeaderOut->isReferencePicture = (firstNALByte & 0x60) != 0;
    // the last 5 bits indicate the NAL unit type
    nalUnitHeaderOut->nalUnitType = (picoH264NALUnitType)(firstNALByte & 0x1F);
    nalUnitHeaderOut->numBytesInNALHeader += 1;

    if (nalUnitBuffer >= nalUnitBufferEnd) {
        PICO_H264_LOG("picoH264ParseNALUnitHeader: NAL unit too small to contain header\n");
        return false;
    }

    // initialie the flags to false
    nalUnitHeaderOut->svcExtensionFlag   = false;
    nalUnitHeaderOut->avc3DExtensionFlag = false;
    nalUnitHeaderOut->mvcExtensionFlag   = false;

    // now check the codition according to the spec Rec. ITU-T H.264 (V15) (08/2024), Page 45, section 7.3.1
    // nal unit type 14, 20, 21 indicate the presence of extensions
    if (nalUnitHeaderOut->nalUnitType == PICO_H264_NAL_UNIT_TYPE_PREFIX_NAL_UNIT || nalUnitHeaderOut->nalUnitType == PICO_H264_NAL_UNIT_TYPE_SLICE_EXTENSION || nalUnitHeaderOut->nalUnitType == PICO_H264_NAL_UNIT_TYPE_DEPTH_SLICE_EXTENSION) {
        uint8_t extensionByte = *nalUnitBuffer;
        if (nalUnitHeaderOut->nalUnitType != PICO_H264_NAL_UNIT_TYPE_DEPTH_SLICE_EXTENSION) {
            // svc_extension_flag is present for NAL unit types 14 and 20 (the bit is 1 for SVC)
            nalUnitHeaderOut->svcExtensionFlag = (extensionByte & 0x80) != 0;
        } else {
            nalUnitHeaderOut->avc3DExtensionFlag = (extensionByte & 0x80) != 0;
        }

        if (nalUnitHeaderOut->svcExtensionFlag) {
            if (!__picoH264ParseNALUnitHeaderSVCExtension(nalUnitBuffer, nalUnitSize - nalUnitHeaderOut->numBytesInNALHeader, &nalUnitHeaderOut->svcExtension)) {
                PICO_H264_LOG("picoH264ParseNALUnitHeader: Failed to parse SVC extension\n");
                return false;
            }
            nalUnitHeaderOut->numBytesInNALHeader += 3; 
        } else if (nalUnitHeaderOut->avc3DExtensionFlag) {
            if (!__picoH264ParseNALUnitHeader3DAVCExtension(nalUnitBuffer, nalUnitSize - nalUnitHeaderOut->numBytesInNALHeader, &nalUnitHeaderOut->avc3DExtension)) {
                PICO_H264_LOG("picoH264ParseNALUnitHeader: Failed to parse 3D AVC extension\n");
                return false;
            }
            nalUnitHeaderOut->numBytesInNALHeader += 2; 
        } else {
            if (!__picoH264ParseNALUnitHeaderMVCCExtension(nalUnitBuffer, nalUnitSize - nalUnitHeaderOut->numBytesInNALHeader, &nalUnitHeaderOut->mvcExtension)) {
                PICO_H264_LOG("picoH264ParseNALUnitHeader: Failed to parse MVC extension\n");
                return false;
            }
            nalUnitHeaderOut->mvcExtensionFlag = true;
            nalUnitHeaderOut->numBytesInNALHeader += 3;
        }
    }
    size_t startCodeSize = nalUnitHeaderOut->zeroCount + 1; // zero bytes + 01 byte
    nalUnitHeaderOut->numBytesInPayload = nalUnitSize - nalUnitHeaderOut->numBytesInNALHeader - startCodeSize;

    if (nalPayloadOut && nalPayloadSizeOut) {
        size_t payloadIndex = 0;
        while (nalUnitBuffer < nalUnitBufferEnd && payloadIndex < nalUnitHeaderOut->numBytesInPayload) {
            // handle emulation prevention three byte (0x03)
            if (nalUnitBuffer + 2 < nalUnitBufferEnd &&
                nalUnitBuffer[0] == 0x00 &&
                nalUnitBuffer[1] == 0x00 &&
                nalUnitBuffer[2] == 0x03) {
                nalPayloadOut[payloadIndex++] = *(nalUnitBuffer++);
                nalPayloadOut[payloadIndex++] = *(nalUnitBuffer++);

                // skip the emulation prevention byte
                nalUnitBuffer++; 
                continue;
            }
            nalPayloadOut[payloadIndex++] = *(nalUnitBuffer++);
        }

        // NOTE: there might be a bug with this??
        *nalPayloadSizeOut = payloadIndex;
    }

    return true;
}

const char *picoH264GetNALUnitTypeToString(picoH264NALUnitType nalUnitType)
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

static void __picoH264PrintSVCExtension(const picoH264NALUnitHeaderSVCExtension_t *svc)
{
    if (!svc)
        return;

    PICO_H264_LOG("  SVC Extension:\n");
    PICO_H264_LOG("    idrFlag: %s\n", svc->idrFlag ? "true" : "false");
    PICO_H264_LOG("    priorityId: %u\n", (unsigned)svc->priorityId);
    PICO_H264_LOG("    noInterLayerPredFlag: %s\n", svc->noInterLayerPredFlag ? "true" : "false");
    PICO_H264_LOG("    dependencyId: %u\n", (unsigned)svc->dependencyId);
    PICO_H264_LOG("    qualityId: %u\n", (unsigned)svc->qualityId);
    PICO_H264_LOG("    temporalId: %u\n", (unsigned)svc->temporalId);
    PICO_H264_LOG("    useRefBasePicFlag: %s\n", svc->useRefBasePicFlag ? "true" : "false");
    PICO_H264_LOG("    discardableFlag: %s\n", svc->discardableFlag ? "true" : "false");
    PICO_H264_LOG("    outputFlag: %s\n", svc->outputFlag ? "true" : "false");
}

static void __picoH264Print3DAVCExtension(const picoH264NALUnitHeader3DAVCExtension_t *ext)
{
    if (!ext)
        return;

    PICO_H264_LOG("  AVC 3D Extension:\n");
    PICO_H264_LOG("    viewId: %u\n", (unsigned)ext->viewId);
    PICO_H264_LOG("    depthFlag: %s\n", ext->depthFlag ? "true" : "false");
    PICO_H264_LOG("    nonIDRFlag: %s\n", ext->nonIDRFlag ? "true" : "false");
    PICO_H264_LOG("    temporalId: %u\n", (unsigned)ext->temporalId);
    PICO_H264_LOG("    anchorPicFlag: %s\n", ext->anchorPicFlag ? "true" : "false");
    PICO_H264_LOG("    interViewFlag: %s\n", ext->interViewFlag ? "true" : "false");
}

static void __picoH264PrintMVCExtension(const picoH264NALUnitHeaderMVCExtension_t *ext)
{
    if (!ext)
        return;

    PICO_H264_LOG("  MVC Extension:\n");
    PICO_H264_LOG("    nonIdrFlag: %s\n", ext->nonIdrFlag ? "true" : "false");
    PICO_H264_LOG("    priorityId: %u\n", (unsigned)ext->priorityId);
    PICO_H264_LOG("    viewId: %u\n", (unsigned)ext->viewId);
    PICO_H264_LOG("    temporalId: %u\n", (unsigned)ext->temporalId);
    PICO_H264_LOG("    anchorPicFlag: %s\n", ext->anchorPicFlag ? "true" : "false");
    PICO_H264_LOG("    interViewFlag: %s\n", ext->interViewFlag ? "true" : "false");
}

void picoH264NALUnitHeaderDebugPrint(picoH264NALUnitHeader nalUnitHeader)
{
    PICO_ASSERT(nalUnitHeader != NULL);

    PICO_H264_LOG("NAL Unit Header:\n");
    PICO_H264_LOG("  isReferencePicture: %s\n", nalUnitHeader->isReferencePicture ? "true" : "false");
    PICO_H264_LOG("  nalUnitType: %s (%u)\n", picoH264GetNALUnitTypeToString(nalUnitHeader->nalUnitType), (unsigned)nalUnitHeader->nalUnitType);
    PICO_H264_LOG("  numBytesInNALHeader: %u\n", (unsigned)nalUnitHeader->numBytesInNALHeader);
    PICO_H264_LOG("  numBytesInNALUnit: %zu\n", nalUnitHeader->numBytesInNALUnit);
    PICO_H264_LOG("  numBytesInPayload: %zu\n", nalUnitHeader->numBytesInPayload);

    PICO_H264_LOG("  svcExtensionFlag: %s\n", nalUnitHeader->svcExtensionFlag ? "true" : "false");
    PICO_H264_LOG("  avc3DExtensionFlag: %s\n", nalUnitHeader->avc3DExtensionFlag ? "true" : "false");
    PICO_H264_LOG("  mvcExtensionFlag: %s\n", nalUnitHeader->mvcExtensionFlag ? "true" : "false");

    if (nalUnitHeader->svcExtensionFlag) {
        __picoH264PrintSVCExtension(&nalUnitHeader->svcExtension);
    }

    if (nalUnitHeader->avc3DExtensionFlag) {
        __picoH264Print3DAVCExtension(&nalUnitHeader->avc3DExtension);
    }

    if (nalUnitHeader->mvcExtensionFlag) {
        __picoH264PrintMVCExtension(&nalUnitHeader->mvcExtension);
    }
}

#endif // PICO_H264_IMPLEMENTATION

#endif // PICO_H264_H
