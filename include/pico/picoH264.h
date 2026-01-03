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
 * Source: https://handle.itu.int/11.1002/1000/15935
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
    // idr_flag equal to 1 specifies that the current coded picture is an IDR picture when the value of dependency_id for the
    // NAL unit is equal to the maximum value of dependency_id in the coded picture. idr_flag equal to 0 specifies that the
    // current coded picture is not an IDR picture when the value of dependency_id for the NAL unit is equal to the maximum
    // value of dependency_id in the coded picture. The value of idr_flag shall be the same for all NAL units of a dependency
    // representation.
    bool idrFlag;

    // priority_id specifies a priority identifier for the NAL unit
    uint8_t priorityId;

    // no_inter_layer_pred_flag specifies whether inter-layer prediction may be used for decoding the coded slice. When
    // no_inter_layer_pred_flag is equal to 1, inter-layer prediction is not used for decoding the coded slice. When
    // no_inter_layer_pred_flag is equal to 0, inter-layer prediction may be used for decoding the coded slice as signalled in the
    // macroblock layer.
    // For prefix NAL units, no_inter_layer_pred_flag shall be equal to 1. When nal_unit_type is equal to 20 and quality_id is
    // greater than 0, no_inter_layer_pred_flag shall be equal to 0.
    // The variable MinNoInterLayerPredFlag is set equal to the minimum value of no_inter_layer_pred_flag for the slices of
    // the layer representation
    bool noInterLayerPredFlag;

    // dependency_id specifies a dependency identifier for the NAL unit. dependency_id shall be equal to 0 in prefix NAL units.  
    uint8_t dependencyId;

    // quality_id specifies a quality identifier for the NAL unit. quality_id shall be equal to 0 in prefix NAL units. The
    // assignment of values to quality_id is constrained by the sub-bitstream extraction process as specified in clause F.8.8.1.
    // The variable DQId is derived by
    // DQId = ( dependency_id << 4 ) + quality_id 
    // When nal_unit_type is equal to 20, the bitstream shall not contain data that result in DQId equal to 0
    uint8_t qualityId;

    // temporal_id specifies a temporal identifier for the NAL unit.
    // The value of temporal_id shall be the same for all prefix NAL units and coded slice in scalable extension NAL units of
    // an access unit. When an access unit contains any NAL unit with nal_unit_type equal to 5 or idr_flag equal to 1,
    // temporal_id shall be equal to 0
    uint8_t temporalId;

    // use_ref_base_pic_flag equal to 1 specifies that reference base pictures (when present) and decoded pictures (when
    // reference base pictures are not present) are used as reference pictures for inter prediction.
    // use_ref_base_pic_flag equal to 0 specifies that reference base pictures are not used as reference pictures for inter
    // prediction (i.e., only decoded pictures are used for inter prediction).
    // The values of use_ref_base_pic_flag shall be the same for all NAL units of a dependency representation
    bool useRefBasePicFlag;

    // discardable_flag equal to 1 specifies that the current NAL unit is not used for decoding dependency representations that
    // are part of the current coded picture or any subsequent coded picture in decoding order and have a greater value of
    // dependency_id than the current NAL unit. discardable_flag equal to 0 specifies that the current NAL unit may be used
    // for decoding dependency representations that are part of the current coded picture or any subsequent coded picture in
    // decoding order and have a greater value of dependency_id than the current NAL unit
    bool discardableFlag;

    // output_flag affects the decoded picture output and removal processes as specified in Annex C. The value of output_flag
    // shall be the same for all NAL units of a dependency representation. For any particular value of dependency_id, the value
    // of output_flag shall be the same for both fields of a complementary field pair.
    bool outputFlag;
} picoH264NALUnitHeaderSVCExtension_t;
typedef picoH264NALUnitHeaderSVCExtension_t *picoH264NALUnitHeaderSVCExtension;

typedef struct {
    // view_idx specifies the view oder index for the NAL unit.
    // view_id is inferred to be equal to view_id[ view_idx ], where view_id[ ] is present in the active sequence parameter set.
    // The variable VOIdx, representing the view order index of the view identified by view_id[ i ], is set equal to view_idx
    uint8_t viewId;

    // depth_flag equal to 1 indicates that the current NAL unit belongs to a depth view component, depth_flag equal to 0
    // indicates that the current NAL unit belongs to a texture view component
    bool depthFlag;


    // These are same as in MVC extension
    bool nonIDRFlag;
    uint8_t temporalId;
    bool anchorPicFlag;
    bool interViewFlag;
} picoH264NALUnitHeader3DAVCExtension_t;
typedef picoH264NALUnitHeader3DAVCExtension_t *picoH264NALUnitHeader3DAVCExtension;

typedef struct {
    // non_idr_flag equal to 0 specifies that the current access unit is an IDR access unit.
    // The value of non_idr_flag shall be the same for all VCL NAL units of an access unit. When non_idr_flag is equal to 0
    // for a prefix NAL unit, the associated NAL unit shall have nal_unit_type equal to 5. When non_idr_flag is equal to 1 for
    // a prefix NAL unit, the associated NAL unit shall have nal_unit_type equal to 1.
    // When nal_unit_type is equal to 1 and the NAL unit is not immediately preceded by a NAL unit with nal_unit_type equal
    // to 14, non_idr_flag shall be inferred to be equal to 1. When nal_unit_type is equal to 5 and the NAL unit is not
    // immediately preceded by a NAL unit with nal_unit_type equal to 14, non_idr_flag shall be inferred to be equal to 0.
    // When nal_ref_idc is equal to 0, the value of non_idr_flag shall be equal to 1.
    bool nonIdrFlag;

    // priority_id specifies a priority identifier for the NAL unit. A lower value of priority_id specifies a higher priority. The
    // assignment of values to priority_id is constrained by the sub-bitstream extraction process as specified in clause G.8.5.3.
    // When nal_unit_type is equal to 1 or 5 and the NAL unit is not immediately preceded by a NAL unit with nal_unit_type
    // equal to 14, priority_id shall be inferred to be equal to 0.
    uint8_t priorityId;

    // view_id specifies a view identifier for the NAL unit. NAL units with the same value of view_id belong to the same view.
    // When nal_unit_type is equal to 1 or 5 and the NAL unit is not immediately preceded by a NAL unit with nal_unit_type
    // equal to 14, the value of view_id shall be inferred to be equal to 0. When the bitstream does contain NAL units with
    // nal_unit_type equal to 1 or 5 that are not immediately preceded by a NAL unit with nal_unit_type equal to 14, it shall not
    // contain data that result in a value of view_id for a view component of any non-base view that is equal to 0.
    // The variable VOIdx, representing the view order index of the view identified by view_id, is set equal to the value of i for
    // which the syntax element view_id[ i ] included in the referred subset sequence parameter set is equal to view_id
    uint16_t viewId;

    // temporal_id specifies a temporal identifier for the NAL unit.
    // When nal_unit_type is equal to 1 or 5 and the NAL unit is not immediately preceded by a NAL unit with nal_unit_type
    // equal to 14, temporal_id shall be inferred to be equal to the value of temporal_id for the non-base views in the same
    // access unit.
    // The value of temporal_id shall be the same for all prefix and coded slice MVC extension NAL units of an access unit.
    // When an access unit contains any NAL unit with nal_unit_type equal to 5 or non_idr_flag equal to 0, temporal_id shall
    // be equal to 0.
    uint8_t temporalId;

    // anchor_pic_flag equal to 1 specifies that the current access unit is an anchor access unit.
    // When nal_unit_type is equal to 1 or 5 and the NAL unit is not immediately preceded by a NAL unit with nal_unit_type
    // equal to 14, anchor_pic_flag shall be inferred to be equal to the value of anchor_pic_flag for the non-base views in the
    // same access unit.
    // When non_idr_flag is equal to 0, anchor_pic_flag shall be equal to 1.
    // When nal_ref_idc is equal to 0, the value of anchor_pic_flag shall be equal to 0.
    // The value of anchor_pic_flag shall be the same for all VCL NAL units of an access unit.
    bool anchorPicFlag;

    // inter_view_flag equal to 0 specifies that the current view component is not used for inter-view prediction by any other
    // view component in the current access unit. inter_view_flag equal to 1 specifies that the current view component may be
    // used for inter-view prediction by other view components in the current access unit.
    // When nal_unit_type is equal to 1 or 5 and the NAL unit is not immediately preceded by a NAL unit with nal_unit_type
    // equal to 14, inter_view_flag shall be inferred to be equal to 1.
    // The value of inter_view_flag shall be the same for all VCL NAL units of a view component.
    bool interViewFlag;
} picoH264NALUnitHeaderMVCExtension_t;
typedef picoH264NALUnitHeaderMVCExtension_t *picoH264NALUnitHeaderMVCExtension;

typedef struct {
    // nal_ref_idc not equal to 0 specifies that the content of the NAL unit contains a sequence parameter set, a sequence
    // parameter set extension, a subset sequence parameter set, a picture parameter set, a slice of a reference picture, a slice
    // data partition of a reference picture, or a prefix NAL unit preceding a slice of a reference picture.
    // For coded video sequences conforming to one or more of the profiles specified in Annex A that are decoded using the
    // decoding process specified in clauses 2 to 9, nal_ref_idc equal to 0 for a NAL unit containing a slice or slice data partition
    // indicates that the slice or slice data partition is part of a non-reference picture.
    // nal_ref_idc shall not be equal to 0 for sequence parameter set or sequence parameter set extension or subset sequence
    // parameter set or picture parameter set NAL units. When nal_ref_idc is equal to 0 for one NAL unit with nal_unit_type in
    // the range of 1 to 4, inclusive, of a particular picture, it shall be equal to 0 for all NAL units with nal_unit_type in the range
    // of 1 to 4, inclusive, of the picture.
    // nal_ref_idc shall not be equal to 0 for NAL units with nal_unit_type equal to 5.
    // nal_ref_idc shall be equal to 0 for all NAL units having nal_unit_type equal to 6, 9, 10, 11, or 12.
    bool isReferencePicture;


    // specifies the type of RBSP data structure contained in the NAL unit
    picoH264NALUnitType nalUnitType;

    // When svc_extension_flag is not present, the value of svc_extension_flag is inferred to be equal to 0.
    // The value of svc_extension_flag shall be equal to 1 for coded video sequences conforming to one or more profiles
    // specified in Annex F. Decoders conforming to one or more profiles specified in Annex F shall ignore (remove from the
    // bitstream and discard) NAL units for which nal_unit_type is equal to 14 or 20 and for which svc_extension_flag is equal
    // to 0.
    // The value of svc_extension_flag shall be equal to 0 for coded video sequences conforming to one or more profiles
    // specified in Annex G. Decoders conforming to one or more profiles specified in Annex G shall ignore (remove from the
    // bitstream and discard) NAL units for which nal_unit_type is equal to 14 or 20 and for which svc_extension_flag is equal
    // to 1.
    // The value of svc_extension_flag shall be equal to 0 for coded video sequences conforming to one or more profiles
    // specified in Annex H. Decoders conforming to one or more profiles specified in Annex H shall ignore (remove from the
    // bitstream and discard) NAL units for which nal_unit_type is equal to 14, 20, or 21 and for which svc_extension_flag is
    // equal to 1.
    // The value of svc_extension_flag shall be equal to 0 for coded video sequences conforming to one or more profiles
    // specified in Annex I. Decoders conforming to one or more profiles specified in Annex I shall ignore (remove from the
    // bitstream and discard) NAL units for which nal_unit_type is equal to 14 or 20 and for which svc_extension_flag is equal
    // to 1.
    bool svcExtensionFlag;
    picoH264NALUnitHeaderSVCExtension_t svcExtension;

    // When avc_3d_extension_flag is not present, the value of avc_3d_extension_flag is inferred to be equal to 0.
    // The value of DepthFlag is specified as follows:
    // DepthFlag = ( nal_unit_type ! = 21 ) ? 0 : ( avc_3d_extension_flag ? depth_flag : 1 ) (7-2)
    // The value of avc_3d_extension_flag shall be equal to 0 for coded video sequences conforming to one or more profiles
    // specified in Annex H. Decoders conforming to one or more profiles specified in Annex H shall ignore (remove from the
    // bitstream and discard) NAL units for which nal_unit_type is equal to 21 and for which avc_3d_extension_flag is equal
    // to 1
    bool avc3DExtensionFlag;
    picoH264NALUnitHeader3DAVCExtension_t avc3DExtension;

    bool mvcExtensionFlag;
    picoH264NALUnitHeaderMVCExtension_t mvcExtension;

    // either 2 or 3 depending on whether the start code prefix is 3 or 4 bytes
    // for (00 00 01) or (00 00 00 01)
    // to get start code prefix size, use (zeroCount + 1)
    uint32_t zeroCount;

    uint32_t numBytesInNALHeader;
    size_t numBytesInNALUnit;
    size_t numBytesInPayload;
} picoH264NALUnitHeader_t;
typedef picoH264NALUnitHeader_t *picoH264NALUnitHeader;

// typedef struct {

// } picoH264SequenceParameterSet_t;
// typedef picoH264SequenceParameterSet_t *picoH264SequenceParameterSet;

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
