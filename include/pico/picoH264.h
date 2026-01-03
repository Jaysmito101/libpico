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

typedef enum {
    PICO_H264_ASPECT_RATIO_IDC_UNSPECIFIED    = 0,
    PICO_H264_ASPECT_RATIO_IDC_SAR_1_1        = 1,
    PICO_H264_ASPECT_RATIO_IDC_SAR_12_11      = 2,
    PICO_H264_ASPECT_RATIO_IDC_SAR_10_11      = 3,
    PICO_H264_ASPECT_RATIO_IDC_SAR_16_11      = 4,
    PICO_H264_ASPECT_RATIO_IDC_SAR_40_33      = 5,
    PICO_H264_ASPECT_RATIO_IDC_SAR_24_11      = 6,
    PICO_H264_ASPECT_RATIO_IDC_SAR_20_11      = 7,
    PICO_H264_ASPECT_RATIO_IDC_SAR_32_11      = 8,
    PICO_H264_ASPECT_RATIO_IDC_SAR_80_33      = 9,
    PICO_H264_ASPECT_RATIO_IDC_SAR_18_11      = 10,
    PICO_H264_ASPECT_RATIO_IDC_SAR_15_11      = 11,
    PICO_H264_ASPECT_RATIO_IDC_SAR_64_33      = 12,
    PICO_H264_ASPECT_RATIO_IDC_SAR_160_99     = 13,
    PICO_H264_ASPECT_RATIO_IDC_SAR_4_3        = 14,
    PICO_H264_ASPECT_RATIO_IDC_SAR_3_2        = 15,
    PICO_H264_ASPECT_RATIO_IDC_SAR_2_1        = 16,
    PICO_H264_ASPECT_RATIO_IDC_RESERVED_START = 17,
    PICO_H264_ASPECT_RATIO_IDC_RESERVED_END   = 254,
    PICO_H264_ASPECT_RATIO_IDC_EXTENDED_SAR   = 255,
} picoH264AspectRatioIDC;

typedef enum {
    PICO_H264_VIDEO_FORMAT_COMPONENT   = 0,
    PICO_H264_VIDEO_FORMAT_PAL         = 1,
    PICO_H264_VIDEO_FORMAT_NTSC        = 2,
    PICO_H264_VIDEO_FORMAT_SECAM       = 3,
    PICO_H264_VIDEO_FORMAT_MAC         = 4,
    PICO_H264_VIDEO_FORMAT_UNSPECIFIED = 5,
    PICO_H264_VIDEO_FORMAT_RESERVED_6  = 6,
    PICO_H264_VIDEO_FORMAT_RESERVED_7  = 7,
} picoH264VideoFormat;

typedef enum {
    PICO_H264_PROFILE_IDC_CAVLC_444_INTRA               = 44,
    PICO_H264_PROFILE_IDC_BASELINE                      = 66, //  (Also Constrained Baseline with constraint_set1_flag = 1)
    PICO_H264_PROFILE_IDC_MAIN                          = 77,
    PICO_H264_PROFILE_IDC_EXTENDED                      = 88,
    PICO_H264_PROFILE_IDC_HIGH                          = 100, //  (Also Progressive/Constrained High with constraint flags)
    PICO_H264_PROFILE_IDC_HIGH_10                       = 110, // [cite: 41] (Also High 10 Intra with constraint_set3_flag = 1)
    PICO_H264_PROFILE_IDC_HIGH_422                      = 122, // [cite: 42] (Also High 4:2:2 Intra with constraint_set3_flag = 1)
    PICO_H264_PROFILE_IDC_HIGH_444_PREDICTIVE           = 244, // [cite: 42] (Also High 4:4:4 Intra with constraint_set3_flag = 1)
    PICO_H264_PROFILE_IDC_SCALABLE_BASELINE             = 83,  // [cite: 54] (Also Scalable Constrained Baseline with constraint_set5_flag = 1)
    PICO_H264_PROFILE_IDC_SCALABLE_HIGH                 = 86,  // [cite: 57] (Also Scalable Constrained High with constraint_set5_flag = 1)
    PICO_H264_PROFILE_IDC_MULTIVIEW_HIGH                = 118, // [cite: 67]
    PICO_H264_PROFILE_IDC_STEREO_HIGH                   = 128, // [cite: 67]
    PICO_H264_PROFILE_IDC_MFC_HIGH                      = 134, // [cite: 17] (Multi-resolution Frame Compatible)
    PICO_H264_PROFILE_IDC_MULTIVIEW_DEPTH_HIGH          = 138, // [cite: 71]
    PICO_H264_PROFILE_IDC_MFC_DEPTH_HIGH                = 135, // [cite: 72] (MFC Depth High)
    PICO_H264_PROFILE_IDC_ENHANCED_MULTIVIEW_DEPTH_HIGH = 139  // [cite: 17]
} picoH264ProfileIDC;

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

typedef struct {
    // cpb_cnt_minus1 plus 1 specifies the number of alternative CPB specifications in the bitstream. The value of
    // cpb_cnt_minus1 shall be in the range of 0 to 31, inclusive. When low_delay_hrd_flag is equal to 1, cpb_cnt_minus1 shall
    // be equal to 0. When cpb_cnt_minus1 is not present, it shall be inferred to be equal to 0
    uint32_t cpbCntMinus1;

    
    // bit_rate_scale (together with bit_rate_value_minus1[ SchedSelIdx ]) specifies the maximum input bit rate of the
    // SchedSelIdx-th CPB
    uint8_t bitRateScale;

    // cpb_size_scale (together with cpb_size_value_minus1[ SchedSelIdx ]) specifies the CPB size of the SchedSelIdx-th
    // CPB.
    uint8_t cpbSizeScale;

    // bit_rate_value_minus1[ SchedSelIdx ] (together with bit_rate_scale) specifies the maximum input bit rate for the
    // SchedSelIdx-th CPB. bit_rate_value_minus1[ SchedSelIdx ] shall be in the range of 0 to 232 − 2, inclusive. For any
    // SchedSelIdx > 0, bit_rate_value_minus1[ SchedSelIdx ] shall be greater than bit_rate_value_minus1[ SchedSelIdx − 1 ].
    // The bit rate in bits per second is given by:
    //     BitRate[ SchedSelIdx ] = ( bit_rate_value_minus1[ SchedSelIdx ] + 1 ) * 2^(6 + bit_rate_scale)
    uint32_t bitRateValueMinus1[256];

    // cpb_size_value_minus1[ SchedSelIdx ] is used together with cpb_size_scale to specify the SchedSelIdx-th CPB size.
    // cpb_size_value_minus1[ SchedSelIdx ] shall be in the range of 0 to 232 − 2, inclusive. For any SchedSelIdx greater than
    // 0, cpb_size_value_minus1[ SchedSelIdx ] shall be less than or equal to cpb_size_value_minus1[ SchedSelIdx −1 ].
    // The CPB size in bits is given by
    //     CpbSize[ SchedSelIdx ] = ( cpb_size_value_minus1[ SchedSelIdx ] + 1 ) * 2^(4 + cpb_size_scale)
    uint32_t cpbSizeValueMinus1[256];

    // cbr_flag[ SchedSelIdx ] equal to 0 specifies that to decode this bitstream by the HRD using the SchedSelIdx-th CPB
    // specification, the hypothetical stream delivery scheduler (HSS) operates in an intermittent bit rate mode.
    // cbr_flag[ SchedSelIdx ] equal to 1 specifies that the HSS operates in a constant bit rate (CBR) mode. When the
    // cbr_flag[ SchedSelIdx ] syntax element is not present, the value of cbr_flag shall be inferred to be equal to 0
    bool cbrFlag[256];

    // initial_cpb_removal_delay_length_minus1 specifies the length in bits of the initial_cpb_removal_delay[ SchedSelIdx ]
    // and initial_cpb_removal_delay_offset[ SchedSelIdx ] syntax elements of the buffering period SEI message. The length
    // of initial_cpb_removal_delay[ SchedSelIdx ] and of initial_cpb_removal_delay_offset[ SchedSelIdx ] is
    // initial_cpb_removal_delay_length_minus1 + 1. When the initial_cpb_removal_delay_length_minus1 syntax element is
    // present in more than one hrd_parameters( ) syntax structure within the VUI parameters syntax structure, the value of the
    // initial_cpb_removal_delay_length_minus1 parameters shall be equal in both hrd_parameters( ) syntax structures. When
    // the initial_cpb_removal_delay_length_minus1 syntax element is not present, it shall be inferred to be equal to 23.
    uint8_t initialCpbRemovalDelayLengthMinus1;

    // cpb_removal_delay_length_minus1 specifies the length in bits of the cpb_removal_delay syntax element. The length
    // of the cpb_removal_delay syntax element of the picture timing SEI message is cpb_removal_delay_length_minus1 + 1.
    // When the cpb_removal_delay_length_minus1 syntax element is present in more than one hrd_parameters( ) syntax
    // structure within the VUI parameters syntax structure, the value of the cpb_removal_delay_length_minus1 parameters
    // shall be equal in both hrd_parameters( ) syntax structures. When the cpb_removal_delay_length_minus1 syntax element
    // is not present, it shall be inferred to be equal to 23
    uint8_t cpbRemovalDelayLengthMinus1;

    // dpb_output_delay_length_minus1 specifies the length in bits of the dpb_output_delay syntax element. The length of
    // the dpb_output_delay syntax element of the picture timing SEI message is dpb_output_delay_length_minus1 + 1. When
    // the dpb_output_delay_length_minus1 syntax element is present in more than one hrd_parameters( ) syntax structure
    // within the VUI parameters syntax structure, the value of the dpb_output_delay_length_minus1 parameters shall be equal
    // in both hrd_parameters( ) syntax structures. When the dpb_output_delay_length_minus1 syntax element is not present, it
    // shall be inferred to be equal to 23.
    uint8_t dpbOutputDelayLengthMinus1;

    // time_offset_length greater than 0 specifies the length in bits of the time_offset syntax element. time_offset_length equal
    // to 0 specifies that the time_offset syntax element is not present. When the time_offset_length syntax element is present
    // in more than one hrd_parameters( ) syntax structure within the VUI parameters syntax structure, the value of the
    // time_offset_length parameters shall be equal in both hrd_parameters( ) syntax structures. When the time_offset_length
    // syntax element is not present, it shall be inferred to be equal to 24
    uint8_t timeOffsetLength;
} picoH264HypotheticalReferenceDecoder_t;
typedef picoH264HypotheticalReferenceDecoder_t *picoH264HypotheticalReferenceDecoder;

typedef struct {
    // aspect_ratio_info_present_flag equal to 1 specifies that aspect_ratio_idc is present. aspect_ratio_info_present_flag
    // equal to 0 specifies that aspect_ratio_idc is not present
    bool aspectRatioInfoPresentFlag;

    // aspect_ratio_idc specifies the value of the sample aspect ratio of the luma samples. Table E-1 shows the meaning of the
    // code. When aspect_ratio_idc indicates Extended_SAR, the sample aspect ratio is represented by sar_width : sar_height.
    // When the aspect_ratio_idc syntax element is not present, aspect_ratio_idc value shall be inferred to be equal to 0.
    uint8_t aspectRatioIdc;

    // sar_width and sar_height shall be relatively prime or equal to 0. When aspect_ratio_idc is equal to 0 or sar_width is equal
    // to 0 or sar_height is equal to 0, the sample aspect ratio shall be considered unspecified by this Recommendation | International Standard

    // sar_width indicates the horizontal size of the sample aspect ratio (in arbitrary units).
    uint16_t sarWidth;
    // sar_height indicates the vertical size of the sample aspect ratio (in the same arbitrary units as sar_width).
    uint16_t sarHeight;

    // overscan_info_present_flag equal to 1 specifies that the overscan_appropriate_flag is present. When
    // overscan_info_present_flag is equal to 0 or is not present, the preferred display method for the video signal is unspecified
    bool overscanInfoPresentFlag;

    // overscan_appropriate_flag equal to 1 indicates that the cropped decoded pictures output are suitable for display using
    // overscan. overscan_appropriate_flag equal to 0 indicates that the cropped decoded pictures output contain visually
    // important information in the entire region out to the edges of the cropping rectangle of the picture, such that the cropped
    // decoded pictures output should not be displayed using overscan. Instead, they should be displayed using either an exact
    // match between the display area and the cropping rectangle, or using underscan. As used in this paragraph, the term
    // "overscan" refers to display processes in which some parts near the borders of the cropped decoded pictures are not visible
    // in the display area. The term "underscan" describes display processes in which the entire cropped decoded pictures are
    // visible in the display area, but they do not cover the entire display area. For display processes that neither use overscan
    // nor underscan, the display area exactly matches the area of the cropped decoded pictures
    bool overscanAppropriateFlag;

    // video_signal_type_present_flag equal to 1 specifies that video_format, video_full_range_flag and
    // colour_description_present_flag are present. video_signal_type_present_flag equal to 0, specify that video_format,
    // video_full_range_flag and colour_description_present_flag are not present
    bool videoSignalTypePresentFlag;

    // video_format indicates the representation of the pictures as specified in Table E-2, before being coded in accordance
    // with this Recommendation | International Standard. When the video_format syntax element is not present, video_format
    // value shall be inferred to be equal to 5
    picoH264VideoFormat videoFormat;

    // video_full_range_flag indicates the black level and range of the luma and chroma signals as derived from E′Y, E′PB, and
    // E′PR or E′R, E′G, and E′B real-valued component signals.
    // When the video_full_range_flag syntax element is not present, the value of video_full_range_flag shall be inferred to be
    // equal to 0.
    bool videoFullRangeFlag;

    // colour_description_present_flag equal to 1 specifies that colour_primaries, transfer_characteristics and
    // matrix_coefficients are present. colour_description_present_flag equal to 0 specifies that colour_primaries,
    // transfer_characteristics and matrix_coefficients are not present.
    bool colourDescriptionPresentFlag;

    // colour_primaries indicates the chromaticity coordinates of the source primaries as specified in Table E-3 in terms of the
    // CIE 1931 definition of x and y as specified by ISO 11664-1
    // When the colour_primaries syntax element is not present, the value of colour_primaries shall be inferred to be equal to 2
    // (the chromaticity is unspecified or is determined by the application).
    uint8_t colourPrimaries;

    // transfer_characteristics, as specified in Table E-4, either indicates the reference opto-electronic transfer characteristic
    // function of the source picture as a function of a source input linear optical intensity input Lc with a nominal real-valued
    // range of 0 to 1 or indicates the inverse of the reference electro-optical transfer characteristic function as a function of an
    // output linear optical intensity Lo with a nominal real-valued range of 0 to 1. For interpretation of entries in Table E-4 that
    // are expressed in terms of multiple curve segments parameterized by the variable L, see ISO 11664-2.
    // When the transfer_characteristics syntax element is not present, the value of transfer_characteristics shall be inferred to
    // be equal to 2 (the transfer characteristics are unspecified or are determined by the application). Values of
    // transfer_characteristics that are identified as reserved in Table E-4 are reserved for future use by ITU-T | ISO/IEC and
    // shall not be present in bitstreams conforming to this version of this Specification. Decoders shall interpret reserved values
    // of transfer_characteristics as equivalent to the value 2.
    uint8_t transferCharacteristics;

    // matrix_coefficients describes the matrix coefficients used in deriving luma and chroma signals from the green, blue, and
    // red, or Y, Z, and X primaries, as specified in Table E-5
    uint8_t matrixCoefficients;

    // chroma_loc_info_present_flag equal to 1 specifies that chroma_sample_loc_type_top_field and
    // chroma_sample_loc_type_bottom_field are present. chroma_loc_info_present_flag equal to 0 specifies that
    // chroma_sample_loc_type_top_field and chroma_sample_loc_type_bottom_field are not present.
    // When chroma_format_idc is not equal to 1, chroma_loc_info_present_flag should be equal to 0
    bool chromaLocInfoPresentFlag;

    // chroma_sample_loc_type_top_field and chroma_sample_loc_type_bottom_field specify the location of chroma
    // samples as follows:
    //     – If chroma_format_idc is equal to 1 (4:2:0 chroma format), chroma_sample_loc_type_top_field and
    //     chroma_sample_loc_type_bottom_field specify the location of chroma samples for the top field and the bottom field,
    //     respectively, as shown in Figure E-1.
    //     – Otherwise (chroma_format_idc is not equal to 1), the values of the syntax elements
    //     chroma_sample_loc_type_top_field and chroma_sample_loc_type_bottom_field shall be ignored. When
    //     chroma_format_idc is equal to 2 (4:2:2 chroma format) or 3 (4:4:4 chroma format), the location of chroma samples
    //     is specified in clause 6.2. When chroma_format_idc is equal to 0, there is no chroma sample array.
    // The value of chroma_sample_loc_type_top_field and chroma_sample_loc_type_bottom_field shall be in the range of 0
    // to 5, inclusive. When the chroma_sample_loc_type_top_field and chroma_sample_loc_type_bottom_field are not
    // present, the values of chroma_sample_loc_type_top_field and chroma_sample_loc_type_bottom_field shall be inferred
    // to be equal to 0.
    uint32_t chromaSampleLocTypeTopField;
    uint32_t chromaSampleLocTypeBottomField;

    // timing_info_present_flag equal to 1 specifies that num_units_in_tick, time_scale and fixed_frame_rate_flag are present
    // in the bitstream. timing_info_present_flag equal to 0 specifies that num_units_in_tick, time_scale and
    // fixed_frame_rate_flag are not present in the bitstream.
    bool timingInfoPresentFlag;

    // num_units_in_tick is the number of time units of a clock operating at the frequency time_scale Hz that corresponds to
    // one increment (called a clock tick) of a clock tick counter. num_units_in_tick shall be greater than 0. A clock tick is the
    // minimum interval of time that can be represented in the coded data. For example, when the frame rate of a video signal
    // is 30 000 / 1001 Hz, time_scale may be equal to 60 000 and num_units_in_tick may be equal to 1001. See Equation C-
    // 1 for the relationship between time units, clock ticks, and seconds.
    uint32_t numUnitsInTick;

    // time_scale is the number of time units that pass in one second. For example, a time coordinate system that measures time
    // using a 27 MHz clock has a time_scale of 27 000 000. time_scale shall be greater than 0.
    uint32_t timeScale;

    // fixed_frame_rate_flag equal to 1 indicates that the temporal distance between the HRD output times of any two
    // consecutive pictures in output order is constrained as follows. fixed_frame_rate_flag equal to 0 indicates that no such
    // constraints apply to the temporal distance between the HRD output times of any two consecutive pictures in output order.
    // When fixed_frame_rate_flag is not present, it shall be inferred to be equal to 0.
    bool fixedFrameRateFlag;
    
    // nal_hrd_parameters_present_flag equal to 1 specifies that NAL HRD parameters (pertaining to the Type II bitstream
    // conformance point) are present. nal_hrd_parameters_present_flag equal to 0 specifies that NAL HRD parameters are not
    // present.
    bool nalHrdParametersPresentFlag;
    picoH264HypotheticalReferenceDecoder_t nalHrdParameters;

    // vcl_hrd_parameters_present_flag equal to 1 specifies that VCL HRD parameters (pertaining to the Type I bitstream
    // conformance point) are present. vcl_hrd_parameters_present_flag equal to 0 specifies that VCL HRD parameters are not
    // present.
    bool vclHrdParametersPresentFlag;
    picoH264HypotheticalReferenceDecoder_t vclHrdParameters;

    // low_delay_hrd_flag specifies the HRD operational mode as specified in Annex C. When fixed_frame_rate_flag is equal
    // to 1, low_delay_hrd_flag shall be equal to 0. When low_delay_hrd_flag is not present, its value shall be inferred to be
    // equal to 1 − fixed_frame_rate_flag
    bool lowDelayHrdFlag;

    // pic_struct_present_flag equal to 1 specifies that picture timing SEI messages (clause D.2.3) are present that include the
    // pic_struct syntax element. pic_struct_present_flag equal to 0 specifies that the pic_struct syntax element is not present in
    // picture timing SEI messages. When pic_struct_present_flag is not present, its value shall be inferred to be equal to 0
    bool picStructPresentFlag;

    // bitstream_restriction_flag equal to 1, specifies that the following coded video sequence bitstream restriction parameters
    // are present. bitstream_restriction_flag equal to 0, specifies that the following coded video sequence bitstream restriction
    // parameters are not present.
    bool bitstreamRestrictionFlag;

    // motion_vectors_over_pic_boundaries_flag equal to 0 indicates that no sample outside the picture boundaries and no
    // sample at a fractional sample position for which the sample value is derived using one or more samples outside the picture
    // boundaries is used for inter prediction of any sample. motion_vectors_over_pic_boundaries_flag equal to 1 indicates that
    // one or more samples outside picture boundaries may be used in inter prediction. When the
    // motion_vectors_over_pic_boundaries_flag syntax element is not present, motion_vectors_over_pic_boundaries_flag
    // value shall be inferred to be equal to 1
    bool motionVectorsOverPicBoundariesFlag;

    // max_bytes_per_pic_denom indicates a number of bytes not exceeded by the sum of the sizes of the VCL NAL units
    // associated with any coded picture in the coded video sequence.
    // The number of bytes that represent a picture in the NAL unit stream is specified for this purpose as the total number of
    // bytes of VCL NAL unit data (i.e., the total of the NumBytesInNALunit variables for the VCL NAL units) for the picture.
    // The value of max_bytes_per_pic_denom shall be in the range of 0 to 16, inclusive.
    uint8_t maxBytesPerPicDenom;

    // max_bits_per_mb_denom indicates an upper bound for the number of coded bits of macroblock_layer( ) data for any
    // macroblock in any picture of the coded video sequence. The value of max_bits_per_mb_denom shall be in the range of 0
    // to 16, inclusiv
    uint8_t maxBitsPerMbDenom;

    // log2_max_mv_length_horizontal and log2_max_mv_length_vertical indicate the value range of a decoded horizontal
    // and vertical motion vector component, respectively, in ¼ luma sample units, for all pictures in the coded video sequence.
    // A value of n asserts that no value of a motion vector component shall exceed the range from −2n to 2n − 1, inclusive, in
    // units of ¼ luma sample displacement. The value of log2_max_mv_length_horizontal shall be in the range of 0 to 15,
    // inclusive. The value of log2_max_mv_length_vertical shall be in the range of 0 to 15, inclusive. When
    // log2_max_mv_length_horizontal is not present, the values of log2_max_mv_length_horizontal and
    // log2_max_mv_length_vertical shall be inferred to be equal to 15
    uint8_t log2MaxMvLengthHorizontal;
    uint32_t log2MaxMvLengthVertical;
    
    // max_num_reorder_frames indicates an upper bound for the number of frames buffers, in the decoded picture buffer
    // (DPB), that are required for storing frames, complementary field pairs, and non-paired fields before output. It is a
    // requirement of bitstream conformance that the maximum number of frames, complementary field pairs, or non-paired
    // fields that precede any frame, complementary field pair, or non-paired field in the coded video sequence in decoding
    // order and follow it in output order shall be less than or equal to max_num_reorder_frames. The value of
    // max_num_reorder_frames shall be in the range of 0 to max_dec_frame_buffering, inclusive. When the
    // max_num_reorder_frames syntax element is not present, the value of max_num_reorder_frames value shall be inferred
    // as follows:
    //     – If profile_idc is equal to 44, 86, 100, 110, 122, or 244 and constraint_set3_flag is equal to 1, the value of
    //     max_num_reorder_frames shall be inferred to be equal to 0.
    //     – Otherwise (profile_idc is not equal to 44, 86, 100, 110, 122, or 244 or constraint_set3_flag is equal to 0), the value
    //     of max_num_reorder_frames shall be inferred to be equal to MaxDpbFrames
    uint32_t numReorderFrames;

    // max_dec_frame_buffering specifies the required size of the HRD decoded picture buffer (DPB) in units of frame
    // buffers. It is a requirement of bitstream conformance that the coded video sequence shall not require a decoded picture
    // buffer with size of more than Max( 1, max_dec_frame_buffering ) frame buffers to enable the output of decoded pictures
    // at the output times specified by dpb_output_delay of the picture timing SEI messages. The value of
    // max_dec_frame_buffering shall be greater than or equal to max_num_ref_frames. An upper bound for the value of
    // max_dec_frame_buffering is specified by the level limits in clauses A.3.1, A.3.2, F.10.2.1, and G.10.2.
    // When the max_dec_frame_buffering syntax element is not present, the value of max_dec_frame_buffering shall be
    // inferred as follows:
    //     – If profile_idc is equal to 44, 86, 100, 110, 122, or 244 and constraint_set3_flag is equal to 1, the value of
    //     max_dec_frame_buffering shall be inferred to be equal to 0.
    //     – Otherwise (profile_idc is not equal to 44, 86, 100, 110, 122, or 244 or constraint_set3_flag is equal to 0), the value
    //     of max_dec_frame_buffering shall be inferred to be equal to MaxDpbFrames.
    uint32_t maxDecFrameBuffering;
} picoH264VideoUsabilityInformation_t;
typedef picoH264VideoUsabilityInformation_t *picoH264VideoUsabilityInformation;

typedef struct {
    // profile_idc and level_idc indicate the profile and level to which the coded video sequence conforms.
    uint8_t profileIdc;

    // constraint_set0_flag equal to 1 indicates that the coded video sequence obeys all constraints specified in clause A.2.1.
    bool constraintSet0Flag;

    // constraint_set1_flag eq;ual to 1 indicates that the coded video sequence obeys all constraints specified in clause A.2.2.
    bool constraintSet1Flag;

    // constraint_set2_flag equal to 1 indicates that the coded video sequence obeys all constraints specified in clause A.2.3.
    bool constraintSet2Flag;

    // constraint_set3_flag is specified as follows:
    // – If profile_idc is equal to 66, 77, or 88 and level_idc is equal to 11, constraint_set3_flag equal to 1 indicates that the
    // coded video sequence obeys all constraints specified in Annex A for level 1b and constraint_set3_flag equal to 0
    // indicates that the coded video sequence obeys all constraints specified in Annex A for level 1.1.
    // – Otherwise, if profile_idc is equal to 100 or 110, constraint_set3_flag equal to 1 indicates that the coded video
    // sequence obeys all constraints specified in Annex A for the High 10 Intra profile, and constraint_set3_flag equal to
    // 0 indicates that the coded video sequence may or may not obey these corresponding constraints.
    // – Otherwise, if profile_idc is equal to 122, constraint_set3_flag equal to 1 indicates that the coded video sequence
    // obeys all constraints specified in Annex A for the High 4:2:2 Intra profile, and constraint_set3_flag equal to 0
    // indicates that the coded video sequence may or may not obey these corresponding constraints.
    // – Otherwise, if profile_idc is equal to 44, constraint_set3_flag shall be equal to 1. When profile_idc is equal to 44, the
    // value of 0 for constraint_set3_flag is forbidden.
    // – Otherwise, if profile_idc is equal to 244, constraint_set3_flag equal to 1 indicates that the coded video sequence
    // obeys all constraints specified in Annex A for the High 4:4:4 Intra profile, and constraint_set3_flag equal to 0
    // indicates that the coded video sequence may or may not obey these corresponding constraints.
    // – Otherwise (profile_idc is equal to 66, 77, or 88 and level_idc is not equal to 11, or profile_idc is not equal to 66, 77,
    // 88, 100, 110, 122, 244, or 44), the value of 1 for constraint_set3_flag is reserved for future use by ITU-T | ISO/IEC.
    // constraint_set3_flag shall be equal to 0 for coded video sequences with profile_idc equal to 66, 77, or 88 and
    // level_idc not equal to 11 and for coded video sequences with profile_idc not equal to 66, 77, 88, 100, 110, 122, 244,
    // or 44 in bitstreams conforming to this Recommendation | International Standard. Decoders shall ignore the value of
    // constraint_set3_flag when profile_idc is equal to 66, 77, or 88 and level_idc is not equal to 11 or when profile_idc
    // is not equal to 66, 77, 88, 100, 110, 122, 244, or 44.
    bool constraintSet3Flag;

    // constraint_set4_flag is specified as follows:
    // – If profile_idc is equal to 77, 88, 100, or 110, constraint_set4_flag equal to 1 indicates that the value of
    // frame_mbs_only_flag is equal to 1. constraint_set4_flag equal to 0 indicates that the value of frame_mbs_only_flag
    // may or may not be equal to 1.
    // – Otherwise, if profile_idc is equal to 118, 128, or 134, constraint_set4_flag equal to 1 indicates that the coded video
    // sequence obeys all constraints specified in clause G.10.1.1. constraint_set4_flag equal to 0 indicates that the coded
    // video sequence may or may not obey the constraints specified in clause G.10.1.1.
    // – Otherwise (profile_idc is not equal to 77, 88, 100, 110, 118, 128, or 134), the value of 1 for constraint_set4_flag is
    // reserved for future use by ITU-T | ISO/IEC. constraint_set4_flag shall be equal to 0 for coded video sequences with
    // profile_idc not equal to 77, 88, 100, 110, 118, 128, or 134 in bitstreams conforming to this Recommendation |
    // International Standard. Decoders shall ignore the value of constraint_set4_flag when profile_idc is not equal to 77,
    // 88, 100, 110, 118, 128, or 134
    bool constraintSet4Flag;

    // constraint_set5_flag is specified as follows:
    // – If profile_idc is equal to 77, 88, or 100, constraint_set5_flag equal to 1 indicates that B slice types are not present in
    // the coded video sequence. constraint_set5_flag equal to 0 indicates that B slice types may or may not be present in
    // the coded video sequence.
    // – Otherwise, if profile_idc is equal to 118, constraint_set5_flag equal to 1 indicates that the coded video sequence
    // obeys all constraints specified in clause G.10.1.2 and constraint_set5_flag equal to 0 indicates that the coded video
    // sequence may or may not obey all constraints specified in clause G.10.1.2.
    // – Otherwise (profile_idc is not equal to 77, 88, 100, or 118), the value of 1 for constraint_set5_flag is reserved for
    // future use by ITU-T | ISO/IEC. constraint_set5_flag shall be equal to 0 when profile_idc is not equal to 77, 88, 100,
    // or 118 in bitstreams conforming to this Recommendation | International Standard. Decoders shall ignore the value of
    // constraint_set5_flag when profile_idc is not equal to 77, 88, 100, or 118
    bool constraintSet5Flag;

    // profile_idc and level_idc indicate the profile and level to which the coded video sequence conforms.
    uint8_t levelIdc;

    // seq_parameter_set_id identifies the sequence parameter set that is referred to by the picture parameter set. The value of
    // seq_parameter_set_id shall be in the range of 0 to 31, inclusive.
    uint32_t seqParameterSetId;

    // chroma_format_idc specifies the chroma sampling relative to the luma sampling. The value of
    // chroma_format_idc shall be in the range of 0 to 3, inclusive. When chroma_format_idc is not present, it shall be inferred
    // to be equal to 1 (4:2:0 chroma format).
    uint8_t chromaFormatIdc;

    // separate_colour_plane_flag equal to 1 specifies that the three colour components of the 4:4:4 chroma format are coded
    // separately. separate_colour_plane_flag equal to 0 specifies that the colour components are not coded separately. When
    // separate_colour_plane_flag is not present, it shall be inferred to be equal to 0. When separate_colour_plane_flag is equal
    // to 1, the primary coded picture consists of three separate components, each of which consists of coded samples of one
    // colour plane (Y, Cb or Cr) that each use the monochrome coding syntax. In this case, each colour plane is associated with
    // a specific colour_plane_id value.
    // Depending on the value of separate_colour_plane_flag, the value of the variable ChromaArrayType is assigned as
    // follows:
    // – If separate_colour_plane_flag is equal to 0, ChromaArrayType is set equal to chroma_format_idc.
    // – Otherwise (separate_colour_plane_flag is equal to 1), ChromaArrayType is set equal to 0
    bool separateColourPlaneFlag;

    // bit_depth_luma_minus8 specifies the bit depth of the samples of the luma array and the value of the luma quantization
    // parameter range offset QpBdOffsetY, as specified by
    //     BitDepthY = 8 + bit_depth_luma_minus8 (7-3)
    //     QpBdOffsetY = 6 * bit_depth_luma_minus8 (7-4)
    // When bit_depth_luma_minus8 is not present, it shall be inferred to be equal to 0. bit_depth_luma_minus8 shall be in the
    // range of 0 to 6, inclusive.
    uint64_t bitDepthLumaMinus8;

    // bit_depth_chroma_minus8 specifies the bit depth of the samples of the chroma arrays and the value of the chroma
    // quantization parameter range offset QpBdOffsetC, as specified by
    //     BitDepthC = 8 + bit_depth_chroma_minus8 (7-5)
    //     QpBdOffsetC = 6 * bit_depth_chroma_minus8 (7-6)
    // When bit_depth_chroma_minus8 is not present, it shall be inferred to be equal to 0. bit_depth_chroma_minus8 shall be
    // in the range of 0 to 6, inclusive.
    uint64_t bitDepthChromaMinus8;

    // qpprime_y_zero_transform_bypass_flag equal to 1 specifies that, when QP′Y is equal to 0, a transform bypass
    // operation for the transform coefficient decoding process and picture construction process prior to deblocking filter process
    // as specified in clause 8.5 shall be applied. qpprime_y_zero_transform_bypass_flag equal to 0 specifies that the transform
    // coefficient decoding process and picture construction process prior to deblocking filter process shall not use the transform
    // bypass operation. When qpprime_y_zero_transform_bypass_flag is not present, it shall be inferred to be equal to 0.
    bool qpprimeYZeroTransformBypassFlag;

    // seq_scaling_matrix_present_flag equal to 1 specifies that the flags seq_scaling_list_present_flag[ i ] for i = 0..7 or
    // i = 0..11 are present. seq_scaling_matrix_present_flag equal to 0 specifies that these flags are not present and the
    // sequence-level scaling list specified by Flat_4x4_16 shall be inferred for i = 0..5 and the sequence-level scaling list
    // specified by Flat_8x8_16 shall be inferred for i = 6..11. When seq_scaling_matrix_present_flag is not present, it shall be
    // inferred to be equal to 0.
    bool seqScalingMatrixPresentFlag;

    // seq_scaling_list_present_flag[ i ] equal to 1 specifies that the syntax structure for scaling list i is present in the sequence
    // parameter set. seq_scaling_list_present_flag[ i ] equal to 0 specifies that the syntax structure for scaling list i is not present
    // in the sequence parameter set and the scaling list fall-back rule set A specified in Table 7-2 shall be used to infer the
    // sequence-level scaling list for index i
    bool seqScalingListPresentFlag[12];

    int scalingList4x4[6][4 * 4];
    bool useDefaultScalingMatrix4x4Flag[6];
    int scalingList8x8[6][8 * 8];
    bool useDefaultScalingMatrix8x8Flag[6];

    // log2_max_frame_num_minus4 specifies the value of the variable MaxFrameNum that is used in frame_num related
    // derivations as follows:
    //     MaxFrameNum = 2^( log2_max_frame_num_minus4 + 4 )
    // The value of log2_max_frame_num_minus4 shall be in the range of 0 to 12, inclusive.
    uint16_t log2MaxFrameNumMinus4;

    // pic_order_cnt_type specifies the method to decode picture order count (as specified in clause 8.2.1). The value of
    // pic_order_cnt_type shall be in the range of 0 to 2, inclusive.
    // pic_order_cnt_type shall not be equal to 2 in a coded video sequence that contains any of the following:
    // – an access unit containing a non-reference frame followed immediately by an access unit containing a non-reference
    // picture,
    // – two access units each containing a field with the two fields together forming a complementary non-reference field
    // pair followed immediately by an access unit containing a non-reference picture,
    // – an access unit containing a non-reference field followed immediately by an access unit containing another non-
    // reference picture that does not form a complementary non-reference field pair with the first of the two access units.
    uint8_t picOrderCntType;

    // log2_max_pic_order_cnt_lsb_minus4 specifies the value of the variable MaxPicOrderCntLsb that is used in the
    // decoding process for picture order count as specified in clause 8.2.1 as follows:
    //     MaxPicOrderCntLsb = 2^( log2_max_pic_order_cnt_lsb_minus4 + 4 )
    // The value of log2_max_pic_order_cnt_lsb_minus4 shall be in the range of 0 to 12, inclusive.
    uint16_t log2MaxPicOrderCntLsbMinus4;

    // delta_pic_order_always_zero_flag equal to 1 specifies that delta_pic_order_cnt[ 0 ] and delta_pic_order_cnt[ 1 ] are
    // not present in the slice headers of the sequence and shall be inferred to be equal to 0. delta_pic_order_always_zero_flag
    // equal to 0 specifies that delta_pic_order_cnt[ 0 ] is present in the slice headers of the sequence and
    // delta_pic_order_cnt[ 1 ] may be present in the slice headers of the sequenc
    bool deltaPicOrderAlwaysZeroFlag;

    // offset_for_non_ref_pic is used to calculate the picture order count of a non-reference picture as specified in clause 8.2.1.
    // The value of offset_for_non_ref_pic shall be in the range of −2^31 + 1 to 2^31 − 1, inclusive.
    int32_t offsetForNonRefPic;

    // offset_for_top_to_bottom_field is used to calculate the picture order count of a bottom field as specified in clause 8.2.1.
    // The value of offset_for_top_to_bottom_field shall be in the range of −2^31 + 1 to 2^31 − 1, inclusive
    int32_t offsetForTopToBottomField;

    // num_ref_frames_in_pic_order_cnt_cycle is used in the decoding process for picture order count as specified in
    // clause 8.2.1. The value of num_ref_frames_in_pic_order_cnt_cycle shall be in the range of 0 to 255, inclusive
    uint8_t numRefFramesInPicOrderCntCycle;

    // offset_for_ref_frame[ i ] is an element of a list of num_ref_frames_in_pic_order_cnt_cycle values used in the decoding
    // process for picture order count as specified in clause 8.2.1. The value of offset_for_ref_frame[ i ] shall be in the range of
    // −2^31 + 1 to 2^31 − 1, inclusive.
    // When pic_order_cnt_type is equal to 1, the variable ExpectedDeltaPerPicOrderCntCycle is derived by
    //     ExpectedDeltaPerPicOrderCntCycle = 0
    //         for( i = 0; i < num_ref_frames_in_pic_order_cnt_cycle; i++ )
    //             ExpectedDeltaPerPicOrderCntCycle += offset_for_ref_frame[ i ]
    int32_t offsetForRefFrame[256];

    // max_num_ref_frames specifies the maximum number of short-term and long-term reference frames, complementary
    // reference field pairs, and non-paired reference fields that may be used by the decoding process for inter prediction of any
    // picture in the coded video sequence. max_num_ref_frames also determines the size of the sliding window operation as
    // specified in clause 8.2.5.3. The value of max_num_ref_frames shall be in the range of 0 to MaxDpbFrames inclusive
    uint32_t maxNumRefFrames;

    // gaps_in_frame_num_value_allowed_flag specifies the allowed values of frame_num as specified in clause 7.4.3 and
    // the decoding process in case of an inferred gap between values of frame_num
    bool gapsInFrameNumValueAllowedFlag;

    // pic_width_in_mbs_minus1 plus 1 specifies the width of each decoded picture in units of macroblocks.
    // The variable for the picture width in units of macroblocks is derived as
    //     PicWidthInMbs = pic_width_in_mbs_minus1 + 1
    // The variable for picture width for the luma component is derived as
    //     PicWidthInSamplesL = PicWidthInMbs * 16
    // The variable for picture width for the chroma components is derived as
    //     PicWidthInSamplesC = PicWidthInMbs * MbWidthC
    uint64_t picWidthInMbsMinus1;

    // pic_height_in_map_units_minus1 plus 1 specifies the height in slice group map units of a decoded frame or field.
    // The variables PicHeightInMapUnits and PicSizeInMapUnits are derived as
    //     PicHeightInMapUnits = pic_height_in_map_units_minus1 + 1
    //     PicSizeInMapUnits = PicWidthInMbs * PicHeightInMapUnits
    uint64_t picHeightInMapUnitsMinus1;

    // frame_mbs_only_flag equal to 0 specifies that coded pictures of the coded video sequence may either be coded fields or
    // coded frames. frame_mbs_only_flag equal to 1 specifies that every coded picture of the coded video sequence is a coded
    // frame containing only frame macroblocks.
    // The allowed range of values for pic_width_in_mbs_minus1, pic_height_in_map_units_minus1, and
    // frame_mbs_only_flag is specified by constraints in Annex A.
    // Depending on frame_mbs_only_flag, semantics are assigned to pic_height_in_map_units_minus1 as follows:
    //  – If frame_mbs_only_flag is equal to 0, pic_height_in_map_units_minus1 plus 1 is the height of a field in units of
    // macroblocks.
    //  – Otherwise (frame_mbs_only_flag is equal to 1), pic_height_in_map_units_minus1 plus 1 is the height of a frame in
    // units of macroblocks.
    // The variable FrameHeightInMbs is derived as
    //     FrameHeightInMbs = ( 2 − frame_mbs_only_flag ) * PicHeightInMapUnits
    bool frameMbsOnlyFlag;

    // mb_adaptive_frame_field_flag equal to 0 specifies no switching between frame and field macroblocks within a picture.
    // mb_adaptive_frame_field_flag equal to 1 specifies the possible use of switching between frame and field macroblocks
    // within frames. When mb_adaptive_frame_field_flag is not present, it shall be inferred to be equal to 0
    bool mbAdaptiveFrameFieldFlag;

    // direct_8x8_inference_flag specifies the method used in the derivation process for luma motion vectors for B_Skip,
    // B_Direct_16x16 and B_Direct_8x8 as specified in clause 8.4.1.2. When frame_mbs_only_flag is equal to 0,
    // direct_8x8_inference_flag shall be equal to 1
    bool direct8x8InferenceFlag;

    // frame_cropping_flag equal to 1 specifies that the frame cropping offset parameters follow next in the sequence
    // parameter set. frame_cropping_flag equal to 0 specifies that the frame cropping offset parameters are not present.
    bool frameCroppingFlag;

    // frame_crop_left_offset, frame_crop_right_offset, frame_crop_top_offset, frame_crop_bottom_offset specify the
    // samples of the pictures in the coded video sequence that are output from the decoding process, in terms of a rectangular
    // region specified in frame coordinates for output.
    // The variables CropUnitX and CropUnitY are derived as follows:
    //  – If ChromaArrayType is equal to 0, CropUnitX and CropUnitY are derived as:
    //      CropUnitX = 1 (7-19)
    //      CropUnitY = 2 − frame_mbs_only_flag (7-20)
    // – Otherwise (ChromaArrayType is equal to 1, 2, or 3), CropUnitX and CropUnitY are derived as:
    //      CropUnitX = SubWidthC (7-21)
    //      CropUnitY = SubHeightC * ( 2 − frame_mbs_only_flag ) (7-22)
    // The variables CroppedWidth and CroppedHeight are derived as follows:
    //      CroppedWidth = PicWidthInSamples −
    //      CropUnitX * ( frame_crop_left_offset + frame_crop_right_offset ) (7-23)
    //      CroppedHeight = 16 * FrameHeightInMbs −
    //      CropUnitY * ( frame_crop_top_offset + frame_crop_bottom_offset ) (7-24)
    // The frame cropping rectangle contains luma samples with horizontal frame coordinates from
    //      CropUnitX * frame_crop_left_offset to PicWidthInSamplesL − ( CropUnitX * frame_crop_right_offset + 1 ) and vertical
    // frame coordinates from CropUnitY * frame_crop_top_offset to ( 16 * FrameHeightInMbs ) −  ( CropUnitY * frame_crop_bottom_offset + 1 ), inclusive.
    //  The value of frame_crop_left_offset shall be in the range of 0 to ( PicWidthInSamplesL / CropUnitX ) − ( frame_crop_right_offset + 1 ), inclusive;
    // and the value of frame_crop_top_offset shall be in the range of 0 to ( 16 * FrameHeightInMbs / CropUnitY ) − ( frame_crop_bottom_offset + 1 ), inclusive.
    // When frame_cropping_flag is equal to 0, the values of frame_crop_left_offset, frame_crop_right_offset,
    // frame_crop_top_offset, and frame_crop_bottom_offset shall be inferred to be equal to 0.
    // When ChromaArrayType is not equal to 0, the corresponding specified samples of the two chroma arrays are the samples
    // having frame coordinates ( x / SubWidthC, y / SubHeightC ), where ( x, y ) are the frame coordinates of the specified
    // luma samples.
    uint64_t frameCropLeftOffset;
    uint64_t frameCropRightOffset;
    uint64_t frameCropTopOffset;
    uint64_t frameCropBottomOffset;

    // vui_parameters_present_flag equal to 1 specifies that the vui_parameters( ) syntax structure as specified in Annex E is
    // present. vui_parameters_present_flag equal to 0 specifies that the vui_parameters( ) syntax structure.
    uint64_t vuiParametersPresentFlag;
    picoH264VideoUsabilityInformation_t vui;
} picoH264SequenceParameterSet_t;
typedef picoH264SequenceParameterSet_t *picoH264SequenceParameterSet;

picoH264Bitstream picoH264BitstreamFromBuffer(const uint8_t *buffer, size_t size);
void picoH264BitstreamDestroy(picoH264Bitstream bitstream);

bool picoH264FindNextNALUnit(picoH264Bitstream bitstream, size_t *nalUnitSizeOut);
bool picoH264ReadNALUnit(picoH264Bitstream bitstream, uint8_t *nalUnitBuffer, size_t nalUnitBufferSize, size_t nalUnitSizeOut);
bool picoH264ParseNALUnit(const uint8_t *nalUnitBuffer, size_t nalUnitSize, picoH264NALUnitHeader nalUnitHeaderOut, uint8_t *nalPayloadOut, size_t *nalPayloadSizeOut);

void picoH264NALUnitHeaderDebugPrint(picoH264NALUnitHeader nalUnitHeader);
void picoH264SequenceParameterSetDebugPrint(picoH264SequenceParameterSet sps);

const char *picoH264NALUnitTypeToString(picoH264NALUnitType nalUnitType);
const char *picoH264AspectRatioIDCToString(uint8_t idc);
const char *picoH264ProfileIdcToString(uint8_t profileIdc);
const char *picoH264VideoFormatToString(picoH264VideoFormat videoFormat);

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

const char *picoH264AspectRatioIDCToString(uint8_t idc)
{
    switch (idc) {
        case PICO_H264_ASPECT_RATIO_IDC_UNSPECIFIED:
            return "Unspecified";
        case PICO_H264_ASPECT_RATIO_IDC_SAR_1_1:
            return "1:1 (square)";
        case PICO_H264_ASPECT_RATIO_IDC_SAR_12_11:
            return "12:11";
        case PICO_H264_ASPECT_RATIO_IDC_SAR_10_11:
            return "10:11";
        case PICO_H264_ASPECT_RATIO_IDC_SAR_16_11:
            return "16:11";
        case PICO_H264_ASPECT_RATIO_IDC_SAR_40_33:
            return "40:33";
        case PICO_H264_ASPECT_RATIO_IDC_SAR_24_11:
            return "24:11";
        case PICO_H264_ASPECT_RATIO_IDC_SAR_20_11:
            return "20:11";
        case PICO_H264_ASPECT_RATIO_IDC_SAR_32_11:
            return "32:11";
        case PICO_H264_ASPECT_RATIO_IDC_SAR_80_33:
            return "80:33";
        case PICO_H264_ASPECT_RATIO_IDC_SAR_18_11:
            return "18:11";
        case PICO_H264_ASPECT_RATIO_IDC_SAR_15_11:
            return "15:11";
        case PICO_H264_ASPECT_RATIO_IDC_SAR_64_33:
            return "64:33";
        case PICO_H264_ASPECT_RATIO_IDC_SAR_160_99:
            return "160:99";
        case PICO_H264_ASPECT_RATIO_IDC_SAR_4_3:
            return "4:3";
        case PICO_H264_ASPECT_RATIO_IDC_SAR_3_2:
            return "3:2";
        case PICO_H264_ASPECT_RATIO_IDC_SAR_2_1:
            return "2:1";
        case PICO_H264_ASPECT_RATIO_IDC_EXTENDED_SAR:
            return "Extended_SAR";
        default:
            if ((int)idc >= 17 && (int)idc <= 254)
                return "Reserved";
            return "Unknown";
    }
}

const char *picoH264ProfileIdcToString(uint8_t profileIdc)
{
    switch (profileIdc) {
        case PICO_H264_PROFILE_IDC_BASELINE:
            return "Baseline Profile";
        case PICO_H264_PROFILE_IDC_MAIN:
            return "Main Profile";
        case PICO_H264_PROFILE_IDC_EXTENDED:
            return "Extended Profile";
        case PICO_H264_PROFILE_IDC_HIGH:
            return "High Profile";
        case PICO_H264_PROFILE_IDC_HIGH_10:
            return "High 10 Profile";
        case PICO_H264_PROFILE_IDC_HIGH_422:
            return "High 4:2:2 Profile";
        case PICO_H264_PROFILE_IDC_HIGH_444_PREDICTIVE:
            return "High 4:4:4 Profile";
        case PICO_H264_PROFILE_IDC_STEREO_HIGH:
            return "Stereo High Profile";
        case PICO_H264_PROFILE_IDC_MULTIVIEW_HIGH:
            return "Multiview High Profile";
        case PICO_H264_PROFILE_IDC_MULTIVIEW_DEPTH_HIGH:
            return "Multiview Depth High Profile";
        case PICO_H264_PROFILE_IDC_ENHANCED_MULTIVIEW_DEPTH_HIGH:
            return "Enhanced Multiview Depth High Profile";
        case PICO_H264_PROFILE_IDC_CAVLC_444_INTRA:
            return "CAVLC 4:4:4 Intra Profile";
        case PICO_H264_PROFILE_IDC_SCALABLE_BASELINE:
            return "Scalable Baseline Profile";
        case PICO_H264_PROFILE_IDC_SCALABLE_HIGH:
            return "Scalable High Profile";
        case PICO_H264_PROFILE_IDC_MFC_HIGH:
            return "MFC High Profile";
        case PICO_H264_PROFILE_IDC_MFC_DEPTH_HIGH:
            return "MFC Depth High Profile";
        default:
            if (profileIdc >= 1 && profileIdc <= 65)
                return "Reserved (Profile specific)";
            return "Unknown Profile";
    }
}

const char *picoH264VideoFormatToString(picoH264VideoFormat videoFormat)
{
    switch (videoFormat) {
        case PICO_H264_VIDEO_FORMAT_COMPONENT:
            return "Component";
        case PICO_H264_VIDEO_FORMAT_PAL:
            return "PAL";
        case PICO_H264_VIDEO_FORMAT_NTSC:
            return "NTSC";
        case PICO_H264_VIDEO_FORMAT_SECAM:
            return "SECAM";
        case PICO_H264_VIDEO_FORMAT_MAC:
            return "MAC";
        case PICO_H264_VIDEO_FORMAT_UNSPECIFIED:
            return "Unspecified";
        case PICO_H264_VIDEO_FORMAT_RESERVED_6:
            return "Reserved 6";
        case PICO_H264_VIDEO_FORMAT_RESERVED_7:
            return "Reserved 7";
        default:
            return "Unknown";
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
    uint32_t bits             = ((uint32_t)(svcExtensionByte1 & 0x7F) << 16) | ((uint32_t)svcExtensionByte2 << 8) | (uint32_t)svcExtensionByte3;

    nalUnitHeaderSVCExtensionOut->idrFlag              = ((bits >> 22) & 0x1) != 0;
    nalUnitHeaderSVCExtensionOut->priorityId           = (uint8_t)((bits >> 16) & 0x3F);
    nalUnitHeaderSVCExtensionOut->noInterLayerPredFlag = ((bits >> 15) & 0x1) != 0;
    nalUnitHeaderSVCExtensionOut->dependencyId         = (uint8_t)((bits >> 12) & 0x7);
    nalUnitHeaderSVCExtensionOut->qualityId            = (uint8_t)((bits >> 8) & 0xF);
    nalUnitHeaderSVCExtensionOut->temporalId           = (uint8_t)((bits >> 5) & 0x7);
    nalUnitHeaderSVCExtensionOut->useRefBasePicFlag    = ((bits >> 4) & 0x1) != 0;
    nalUnitHeaderSVCExtensionOut->discardableFlag      = ((bits >> 3) & 0x1) != 0;
    nalUnitHeaderSVCExtensionOut->outputFlag           = ((bits >> 2) & 0x1) != 0;

    return true;
}

static bool __picoH264ParseNALUnitHeader3DAVCExtension(const uint8_t *nalUnitBuffer, size_t nalUnitSize, picoH264NALUnitHeader3DAVCExtension nalUnitHeader3DAVCExtensionOut)
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

static bool __picoH264ParseNALUnitHeaderMVCCExtension(const uint8_t *nalUnitBuffer, size_t nalUnitSize, picoH264NALUnitHeaderMVCExtension nalUnitHeaderMVCExtensionOut)
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

    nalUnitHeaderMVCExtensionOut->nonIdrFlag    = ((bits >> 23) & 0x1) != 0;
    nalUnitHeaderMVCExtensionOut->priorityId    = (uint8_t)((bits >> 17) & 0x3F);
    nalUnitHeaderMVCExtensionOut->viewId        = (uint16_t)((bits >> 7) & 0x3FF);
    nalUnitHeaderMVCExtensionOut->temporalId    = (uint8_t)((bits >> 4) & 0x7);
    nalUnitHeaderMVCExtensionOut->anchorPicFlag = ((bits >> 3) & 0x1) != 0;
    nalUnitHeaderMVCExtensionOut->interViewFlag = ((bits >> 2) & 0x1) != 0;

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
    size_t startCodeSize                = nalUnitHeaderOut->zeroCount + 1; // zero bytes + 01 byte
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

const char *picoH264NALUnitTypeToString(picoH264NALUnitType nalUnitType)
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

static void __picoH264VideoUsabilityInformationDebugPrint(const picoH264VideoUsabilityInformation_t *vui)
{
    if (!vui)
        return;

    PICO_H264_LOG("  VUI Parameters:\n");
    PICO_H264_LOG("    aspectRatioInfoPresentFlag: %s\n", vui->aspectRatioInfoPresentFlag ? "true" : "false");
    if (vui->aspectRatioInfoPresentFlag) {
        PICO_H264_LOG("    aspectRatioIdc: %s (%u)\n", picoH264AspectRatioIDCToString(vui->aspectRatioIdc), (unsigned)vui->aspectRatioIdc);
        if (vui->aspectRatioIdc == PICO_H264_ASPECT_RATIO_IDC_EXTENDED_SAR) {
            PICO_H264_LOG("    sarWidth: %u\n", (unsigned)vui->sarWidth);
            PICO_H264_LOG("    sarHeight: %u\n", (unsigned)vui->sarHeight);
        }
    }
    PICO_H264_LOG("    overscanInfoPresentFlag: %s\n", vui->overscanInfoPresentFlag ? "true" : "false");
    if (vui->overscanInfoPresentFlag) {
        PICO_H264_LOG("    overscanAppropriateFlag: %s\n", vui->overscanAppropriateFlag ? "true" : "false");
    }
    PICO_H264_LOG("    videoSignalTypePresentFlag: %s\n", vui->videoSignalTypePresentFlag ? "true" : "false");
    if (vui->videoSignalTypePresentFlag) {
        PICO_H264_LOG("    videoFormat: %s (%u)\n", picoH264VideoFormatToString(vui->videoFormat), (unsigned)vui->videoFormat);
        PICO_H264_LOG("    videoFullRangeFlag: %s\n", vui->videoFullRangeFlag ? "true" : "false");
        PICO_H264_LOG("    colourDescriptionPresentFlag: %s\n", vui->colourDescriptionPresentFlag ? "true" : "false");
        if (vui->colourDescriptionPresentFlag) {
            PICO_H264_LOG("    colourPrimaries: %u\n", (unsigned)vui->colourPrimaries);
            PICO_H264_LOG("    transferCharacteristics: %u\n", (unsigned)vui->transferCharacteristics);
            PICO_H264_LOG("    matrixCoefficients: %u\n", (unsigned)vui->matrixCoefficients);
        }
    }
    PICO_H264_LOG("    chromaLocInfoPresentFlag: %s\n", vui->chromaLocInfoPresentFlag ? "true" : "false");
    if (vui->chromaLocInfoPresentFlag) {
        PICO_H264_LOG("    chromaSampleLocTypeTopField: %u\n", (unsigned)vui->chromaSampleLocTypeTopField);
        PICO_H264_LOG("    chromaSampleLocTypeBottomField: %u\n", (unsigned)vui->chromaSampleLocTypeBottomField);
    }
    PICO_H264_LOG("    timingInfoPresentFlag: %s\n", vui->timingInfoPresentFlag ? "true" : "false");
    if (vui->timingInfoPresentFlag) {
        PICO_H264_LOG("    numUnitsInTick: %u\n", (unsigned)vui->numUnitsInTick);
        PICO_H264_LOG("    timeScale: %u\n", (unsigned)vui->timeScale);
        PICO_H264_LOG("    fixedFrameRateFlag: %s\n", vui->fixedFrameRateFlag ? "true" : "false");
    }
    PICO_H264_LOG("    nalHrdParametersPresentFlag: %s\n", vui->nalHrdParametersPresentFlag ? "true" : "false");
    PICO_H264_LOG("    vclHrdParametersPresentFlag: %s\n", vui->vclHrdParametersPresentFlag ? "true" : "false");
    if (vui->nalHrdParametersPresentFlag || vui->vclHrdParametersPresentFlag) {
        PICO_H264_LOG("    lowDelayHrdFlag: %s\n", vui->lowDelayHrdFlag ? "true" : "false");
    }
    PICO_H264_LOG("    picStructPresentFlag: %s\n", vui->picStructPresentFlag ? "true" : "false");
    PICO_H264_LOG("    bitstreamRestrictionFlag: %s\n", vui->bitstreamRestrictionFlag ? "true" : "false");
    if (vui->bitstreamRestrictionFlag) {
        PICO_H264_LOG("    motionVectorsOverPicBoundariesFlag: %s\n", vui->motionVectorsOverPicBoundariesFlag ? "true" : "false");
        PICO_H264_LOG("    maxBytesPerPicDenom: %u\n", (unsigned)vui->maxBytesPerPicDenom);
        PICO_H264_LOG("    maxBitsPerMbDenom: %u\n", (unsigned)vui->maxBitsPerMbDenom);
        PICO_H264_LOG("    log2MaxMvLengthHorizontal: %u\n", (unsigned)vui->log2MaxMvLengthHorizontal);
        PICO_H264_LOG("    log2MaxMvLengthVertical: %u\n", (unsigned)vui->log2MaxMvLengthVertical);
        PICO_H264_LOG("    numReorderFrames: %u\n", (unsigned)vui->numReorderFrames);
        PICO_H264_LOG("    maxDecFrameBuffering: %u\n", (unsigned)vui->maxDecFrameBuffering);
    }
}

void picoH264SequenceParameterSetDebugPrint(picoH264SequenceParameterSet sps)
{
    PICO_ASSERT(sps != NULL);

    PICO_H264_LOG("Sequence Parameter Set:\n");
    PICO_H264_LOG("  profileIdc: %s (%u)\n", picoH264ProfileIdcToString(sps->profileIdc), (unsigned)sps->profileIdc);
    PICO_H264_LOG("  constraintSet0Flag: %s\n", sps->constraintSet0Flag ? "true" : "false");
    PICO_H264_LOG("  constraintSet1Flag: %s\n", sps->constraintSet1Flag ? "true" : "false");
    PICO_H264_LOG("  constraintSet2Flag: %s\n", sps->constraintSet2Flag ? "true" : "false");
    PICO_H264_LOG("  constraintSet3Flag: %s\n", sps->constraintSet3Flag ? "true" : "false");
    PICO_H264_LOG("  constraintSet4Flag: %s\n", sps->constraintSet4Flag ? "true" : "false");
    PICO_H264_LOG("  constraintSet5Flag: %s\n", sps->constraintSet5Flag ? "true" : "false");
    PICO_H264_LOG("  levelIdc: %u\n", (unsigned)sps->levelIdc);
    PICO_H264_LOG("  seqParameterSetId: %u\n", (unsigned)sps->seqParameterSetId);

    if (sps->profileIdc == 100 || sps->profileIdc == 110 || sps->profileIdc == 122 || sps->profileIdc == 244 || sps->profileIdc == 44 || sps->profileIdc == 83 || sps->profileIdc == 86 || sps->profileIdc == 118 || sps->profileIdc == 128 || sps->profileIdc == 138 || sps->profileIdc == 139 || sps->profileIdc == 134 || sps->profileIdc == 135) {
        PICO_H264_LOG("  chromaFormatIdc: %u\n", (unsigned)sps->chromaFormatIdc);
        if (sps->chromaFormatIdc == 3) {
            PICO_H264_LOG("  separateColourPlaneFlag: %s\n", sps->separateColourPlaneFlag ? "true" : "false");
        }
        PICO_H264_LOG("  bitDepthLumaMinus8: %llu\n", (unsigned long long)sps->bitDepthLumaMinus8);
        PICO_H264_LOG("  bitDepthChromaMinus8: %llu\n", (unsigned long long)sps->bitDepthChromaMinus8);
        PICO_H264_LOG("  qpprimeYZeroTransformBypassFlag: %s\n", sps->qpprimeYZeroTransformBypassFlag ? "true" : "false");
        PICO_H264_LOG("  seqScalingMatrixPresentFlag: %s\n", sps->seqScalingMatrixPresentFlag ? "true" : "false");
    }

    PICO_H264_LOG("  log2MaxFrameNumMinus4: %u\n", (unsigned)sps->log2MaxFrameNumMinus4);
    PICO_H264_LOG("  picOrderCntType: %u\n", (unsigned)sps->picOrderCntType);
    if (sps->picOrderCntType == 0) {
        PICO_H264_LOG("  log2MaxPicOrderCntLsbMinus4: %u\n", (unsigned)sps->log2MaxPicOrderCntLsbMinus4);
    } else if (sps->picOrderCntType == 1) {
        PICO_H264_LOG("  deltaPicOrderAlwaysZeroFlag: %s\n", sps->deltaPicOrderAlwaysZeroFlag ? "true" : "false");
        PICO_H264_LOG("  offsetForNonRefPic: %d\n", (int)sps->offsetForNonRefPic);
        PICO_H264_LOG("  offsetForTopToBottomField: %d\n", (int)sps->offsetForTopToBottomField);
        PICO_H264_LOG("  numRefFramesInPicOrderCntCycle: %u\n", (unsigned)sps->numRefFramesInPicOrderCntCycle);
        for (uint32_t i = 0; i < sps->numRefFramesInPicOrderCntCycle; i++) {
            PICO_H264_LOG("    offsetForRefFrame[%u]: %d\n", (unsigned)i, (int)sps->offsetForRefFrame[i]);
        }
    }

    PICO_H264_LOG("  maxNumRefFrames: %u\n", (unsigned)sps->maxNumRefFrames);
    PICO_H264_LOG("  gapsInFrameNumValueAllowedFlag: %s\n", sps->gapsInFrameNumValueAllowedFlag ? "true" : "false");
    PICO_H264_LOG("  picWidthInMbsMinus1: %llu\n", (unsigned long long)sps->picWidthInMbsMinus1);
    PICO_H264_LOG("  picHeightInMapUnitsMinus1: %llu\n", (unsigned long long)sps->picHeightInMapUnitsMinus1);
    PICO_H264_LOG("  frameMbsOnlyFlag: %s\n", sps->frameMbsOnlyFlag ? "true" : "false");
    if (!sps->frameMbsOnlyFlag) {
        PICO_H264_LOG("  mbAdaptiveFrameFieldFlag: %s\n", sps->mbAdaptiveFrameFieldFlag ? "true" : "false");
    }
    PICO_H264_LOG("  direct8x8InferenceFlag: %s\n", sps->direct8x8InferenceFlag ? "true" : "false");
    PICO_H264_LOG("  frameCroppingFlag: %s\n", sps->frameCroppingFlag ? "true" : "false");
    if (sps->frameCroppingFlag) {
        PICO_H264_LOG("    frameCropLeftOffset: %llu\n", (unsigned long long)sps->frameCropLeftOffset);
        PICO_H264_LOG("    frameCropRightOffset: %llu\n", (unsigned long long)sps->frameCropRightOffset);
        PICO_H264_LOG("    frameCropTopOffset: %llu\n", (unsigned long long)sps->frameCropTopOffset);
        PICO_H264_LOG("    frameCropBottomOffset: %llu\n", (unsigned long long)sps->frameCropBottomOffset);
    }
    PICO_H264_LOG("  vuiParametersPresentFlag: %llu\n", (unsigned long long)sps->vuiParametersPresentFlag);
    if (sps->vuiParametersPresentFlag) {
        __picoH264VideoUsabilityInformationDebugPrint(&sps->vui);
    }
}

void picoH264NALUnitHeaderDebugPrint(picoH264NALUnitHeader nalUnitHeader)
{
    PICO_ASSERT(nalUnitHeader != NULL);

    PICO_H264_LOG("NAL Unit Header:\n");
    PICO_H264_LOG("  isReferencePicture: %s\n", nalUnitHeader->isReferencePicture ? "true" : "false");
    PICO_H264_LOG("  nalUnitType: %s (%u)\n", picoH264NALUnitTypeToString(nalUnitHeader->nalUnitType), (unsigned)nalUnitHeader->nalUnitType);
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
