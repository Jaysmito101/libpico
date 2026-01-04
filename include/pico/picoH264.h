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

// This is a interface for the user to provide a bitstream abstraction.
// This is to be used for cases when reading huge H.264 bitstreams from files
// or network streams which cannot be fully loaded into memory.
typedef struct {
    void *userData;

    picoH264BitstreamReadFunc read;
    picoH264BitstreamSeekFunc seek;
    picoH264BitstreamTellFunc tell;
} picoH264Bitsteam_t;
typedef picoH264Bitsteam_t *picoH264Bitstream;

// This is a core utility to help parse H.264 bitstreams from memory buffers.
// This is mostly to be used internally by the library, but is made public
// for advanced use cases such as parsing contents or portions that arent
// being parsed by the library directly at the moment.
typedef struct {
    const uint8_t *buffer;
    size_t size;
    size_t position;
    size_t bitPosition;
} picoH264BufferReader_t;
typedef picoH264BufferReader_t *picoH264BufferReader;

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

typedef enum {
    PICO_H264_NAL_REF_IDC_DISPOSABLE = 0,
    PICO_H264_NAL_REF_IDC_LOW        = 1,
    PICO_H264_NAL_REF_IDC_HIGH       = 2,
    PICO_H264_NAL_REF_IDC_HIGHEST    = 3
} picoH264NALRefIDC;

typedef enum {
    PICO_H264_SEI_MESSAGE_TYPE_BUFFERING_PERIOD                           = 0,
    PICO_H264_SEI_MESSAGE_TYPE_PIC_TIMING                                 = 1,
    PICO_H264_SEI_MESSAGE_TYPE_PAN_SCAN_RECT                              = 2,
    PICO_H264_SEI_MESSAGE_TYPE_FILLER_PAYLOAD                             = 3,
    PICO_H264_SEI_MESSAGE_TYPE_USER_DATA_REGISTERED_ITU_T_35              = 4,
    PICO_H264_SEI_MESSAGE_TYPE_USER_DATA_UNREGISTERED                     = 5,
    PICO_H264_SEI_MESSAGE_TYPE_RECOVERY_POINT                             = 6,
    PICO_H264_SEI_MESSAGE_TYPE_DEC_REF_PIC_MARKING_REPETITION             = 7,
    PICO_H264_SEI_MESSAGE_TYPE_SPARE_PIC                                  = 8,
    PICO_H264_SEI_MESSAGE_TYPE_SCENE_INFO                                 = 9,
    PICO_H264_SEI_MESSAGE_TYPE_SUB_SEQ_INFO                               = 10,
    PICO_H264_SEI_MESSAGE_TYPE_SUB_SEQ_LAYER_CHARACTERISTICS              = 11,
    PICO_H264_SEI_MESSAGE_TYPE_SUB_SEQ_CHARACTERISTICS                    = 12,
    PICO_H264_SEI_MESSAGE_TYPE_FILL_FRAME_FREEZE                          = 13,
    PICO_H264_SEI_MESSAGE_TYPE_FILL_FRAME_FREEZE_RELEASE                  = 14,
    PICO_H264_SEI_MESSAGE_TYPE_FULL_FRAME_SNAPSHOT                        = 15,
    PICO_H264_SEI_MESSAGE_TYPE_PROGRESSIVE_REFINEMENT_SEGMENT_START       = 16,
    PICO_H264_SEI_MESSAGE_TYPE_PROGRESSIVE_REFINEMENT_SEGMENT_END         = 17,
    PICO_H264_SEI_MESSAGE_TYPE_MOTION_CONSTRAINED_SLICE_GROUP_SET         = 18,
    PICO_H264_SEI_MESSAGE_TYPE_FILM_GRAIN_CHARACTERISTICS                 = 19,
    PICO_H264_SEI_MESSAGE_TYPE_DEBLOCKING_FILTER_DISPLAY_PREFERENCE       = 20,
    PICO_H264_SEI_MESSAGE_TYPE_STEREO_VIDEO_INFO                          = 21,
    PICO_H264_SEI_MESSAGE_TYPE_POST_FILTER_HINT                           = 22,
    PICO_H264_SEI_MESSAGE_TYPE_TONE_MAPPING_INFO                          = 23,
    PICO_H264_SEI_MESSAGE_TYPE_SCALABILITY_INFO                           = 24,
    PICO_H264_SEI_MESSAGE_TYPE_SUB_PIC_SCALABLE_LAYER                     = 25,
    PICO_H264_SEI_MESSAGE_TYPE_NON_REQUIRED_LAYER_REP                     = 26,
    PICO_H264_SEI_MESSAGE_TYPE_PRIORITY_LAYER_INFO                        = 27,
    PICO_H264_SEI_MESSAGE_TYPE_LAYERS_NOT_PRESENT                         = 28,
    PICO_H264_SEI_MESSAGE_TYPE_LAYER_DEPENDENCY_CHANGE                    = 29,
    PICO_H264_SEI_MESSAGE_TYPE_SCALABLE_NESTING                           = 30,
    PICO_H264_SEI_MESSAGE_TYPE_BASE_LAYER_TEMPORAL_HRD                    = 31,
    PICO_H264_SEI_MESSAGE_TYPE_QUALITY_LAYER_INTEGRITY_CHECK              = 32,
    PICO_H264_SEI_MESSAGE_TYPE_REDUNDANT_PIC_PROPERTY                     = 33,
    PICO_H264_SEI_MESSAGE_TYPE_TL0_DEP_REP_INDEX                          = 34,
    PICO_H264_SEI_MESSAGE_TYPE_TL_SWITCHING_POINT                         = 35,
    PICO_H264_SEI_MESSAGE_TYPE_PARALLEL_DECODING_INFO                     = 36,
    PICO_H264_SEI_MESSAGE_TYPE_MVC_SCALABLE_NESTING                       = 37,
    PICO_H264_SEI_MESSAGE_TYPE_VIEW_SCALABILITY_INFO                      = 38,
    PICO_H264_SEI_MESSAGE_TYPE_MULTIVIEW_SCENE_INFO                       = 39,
    PICO_H264_SEI_MESSAGE_TYPE_MULTIVIEW_ACQUISITION_INFO                 = 40,
    PICO_H264_SEI_MESSAGE_TYPE_NON_REQUIRED_VIEW_COMPONENT                = 41,
    PICO_H264_SEI_MESSAGE_TYPE_VIEW_DEPENDENCY_CHANGE                     = 42,
    PICO_H264_SEI_MESSAGE_TYPE_OPERATION_POINTS_NOT_PRESENT               = 43,
    PICO_H264_SEI_MESSAGE_TYPE_BASE_VIEW_TEMPORAL_HRD                     = 44,
    PICO_H264_SEI_MESSAGE_TYPE_FRAME_PACKING_ARRANGEMENT                  = 45,
    PICO_H264_SEI_MESSAGE_TYPE_MULTIVIEW_VIEW_POSITION                    = 46,
    PICO_H264_SEI_MESSAGE_TYPE_DISPLAY_ORIENTATION                        = 47,
    PICO_H264_SEI_MESSAGE_TYPE_MVCD_SCALABLE_NESTING                      = 48,
    PICO_H264_SEI_MESSAGE_TYPE_MVCD_VIEW_SCALABILITY_INFO                 = 49,
    PICO_H264_SEI_MESSAGE_TYPE_DEPTH_REPRESENTATION_INFO                  = 50,
    PICO_H264_SEI_MESSAGE_TYPE_THREE_DIMENSIONAL_REFERENCE_DISPLAYS_INFO  = 51,
    PICO_H264_SEI_MESSAGE_TYPE_DEPTH_TIMING                               = 52,
    PICO_H264_SEI_MESSAGE_TYPE_DEPTH_SAMPLING_INFO                        = 53,
    PICO_H264_SEI_MESSAGE_TYPE_CONSTRAINED_DEPTH_PARAMETER_SET_IDENTIFIER = 54,
    PICO_H264_SEI_MESSAGE_TYPE_GREEN_METADATA                             = 56,
    PICO_H264_SEI_MESSAGE_TYPE_MASTERING_DISPLAY_COLOUR_VOLUME            = 137,
    PICO_H264_SEI_MESSAGE_TYPE_COLOUR_REMAPPING_INFO                      = 142,
    PICO_H264_SEI_MESSAGE_TYPE_CONTENT_LIGHT_LEVEL_INFO                   = 144,
    PICO_H264_SEI_MESSAGE_TYPE_ALTERNATIVE_TRANSFER_CHARACTERISTICS       = 147,
    PICO_H264_SEI_MESSAGE_TYPE_AMBIENT_VIEWING_ENVIRONMENT                = 148,
    PICO_H264_SEI_MESSAGE_TYPE_CONTENT_COLOUR_VOLUME                      = 149,
    PICO_H264_SEI_MESSAGE_TYPE_EQUIRECTANGULAR_PROJECTION                 = 150,
    PICO_H264_SEI_MESSAGE_TYPE_CUBEMAP_PROJECTION                         = 151,
    PICO_H264_SEI_MESSAGE_TYPE_SPHERE_ROTATION                            = 154,
    PICO_H264_SEI_MESSAGE_TYPE_REGIONWISE_PACKING                         = 155,
    PICO_H264_SEI_MESSAGE_TYPE_OMNI_VIEWPORT                              = 156,
    PICO_H264_SEI_MESSAGE_TYPE_ALTERNATIVE_DEPTH_INFO                     = 181,
    PICO_H264_SEI_MESSAGE_TYPE_SEI_MANIFEST                               = 200,
    PICO_H264_SEI_MESSAGE_TYPE_SEI_PREFIX_INDICATION                      = 201,
    PICO_H264_SEI_MESSAGE_TYPE_ANNOTATED_REGIONS                          = 202,
    PICO_H264_SEI_MESSAGE_TYPE_SHUTTER_INTERVAL_INFO                      = 205,
    PICO_H264_SEI_MESSAGE_TYPE_NN_POST_FILTER_CHARACTERISTICS             = 210,
    PICO_H264_SEI_MESSAGE_TYPE_NN_POST_FILTER_ACTIVATION                  = 211,
    PICO_H264_SEI_MESSAGE_TYPE_PHASE_INDICATION                           = 212,
} picoH264SEIMessageType;

typedef enum {
    PICO_H264_SLICE_TYPE_P       = 0,
    PICO_H264_SLICE_TYPE_B       = 1,
    PICO_H264_SLICE_TYPE_I       = 2,
    PICO_H264_SLICE_TYPE_SP      = 3,
    PICO_H264_SLICE_TYPE_SI      = 4,
    PICO_H264_SLICE_TYPE_P_ONLY  = 5,
    PICO_H264_SLICE_TYPE_B_ONLY  = 6,
    PICO_H264_SLICE_TYPE_I_ONLY  = 7,
    PICO_H264_SLICE_TYPE_SP_ONLY = 8,
    PICO_H264_SLICE_TYPE_SI_ONLY = 9,
} picoH264SliceType;

typedef struct {
    // Indicates if this is an IDR (Instantaneous Decoder Refresh) picture.
    // True when this NAL represents an IDR at its maximum dependency level.
    // Must be consistent across all NAL units in the same dependency representation.
    bool idrFlag;

    // Priority identifier for the NAL unit (0-63). Lower values = higher priority.
    // Used by the sub-bitstream extraction process (see clause F.8.8.1).
    uint8_t priorityId;

    // Controls inter-layer prediction for this slice:
    // - true: Inter-layer prediction is NOT used (required for prefix NAL units)
    // - false: Inter-layer prediction MAY be used (required when quality_id > 0)
    bool noInterLayerPredFlag;

    // Dependency layer identifier (0-7). Must be 0 for prefix NAL units.
    // Combined with quality_id to form DQId = (dependency_id << 4) + quality_id
    uint8_t dependencyId;

    // Quality layer identifier (0-15). Must be 0 for prefix NAL units.
    // Higher values represent enhanced quality versions of the same spatial layer.
    // Combined with dependency_id to form DQId (see above).
    uint8_t qualityId;

    // Temporal layer identifier (0-7). Indicates the temporal sub-layer.
    // Must be 0 for IDR pictures (when idr_flag=1 or nal_unit_type=5).
    // Must be consistent across all NAL units in an access unit.
    uint8_t temporalId;

    // Controls reference picture selection for inter prediction:
    // - true: Use reference base pictures (or decoded pictures if unavailable)
    // - false: Use only decoded pictures (not reference base pictures)
    // Must be consistent across all NAL units in a dependency representation.
    bool useRefBasePicFlag;

    // Indicates if this NAL unit can be discarded without affecting higher dependency layers:
    // - true: Safe to discard; not needed by NAL units with higher dependency_id
    // - false: May be required by higher dependency layers
    bool discardableFlag;

    // Controls decoded picture output (see Annex C):
    // - true: Picture should be output after decoding
    // - false: Picture is for reference only, not for output
    // Must be consistent for all NAL units at the same dependency_id.
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
    picoH264NALRefIDC nalRefIDC;

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

typedef struct {
    // seq_parameter_set_id identifies the sequence parameter set associated with the sequence parameter set extension. The
    // value of seq_parameter_set_id shall be in the range of 0 to 31, inclusive
    uint8_t seqParameterSetId;

    // aux_format_idc equal to 0 indicates that there are no auxiliary coded pictures in the coded video sequence.
    // aux_format_idc equal to 1 indicates that exactly one auxiliary coded picture is present in each access unit of the coded
    // video sequence, and that for alpha blending purposes the decoded samples of the associated primary coded picture in each
    // access unit should be multiplied by the interpretation sample values of the auxiliary coded picture in the access unit in
    // the display process after output from the decoding process. aux_format_idc equal to 2 indicates that exactly one auxiliary
    // coded picture exists in each access unit of the coded video sequence, and that for alpha blending purposes the decoded
    // samples of the associated primary coded picture in each access unit should not be multiplied by the interpretation sample
    // values of the auxiliary coded picture in the access unit in the display process after output from the decoding process.
    // aux_format_idc equal to 3 indicates that exactly one auxiliary coded picture exists in each access unit of the coded video
    // sequence, and that the usage of the auxiliary coded pictures is unspecified. The value of aux_format_idc shall be in the
    // range of 0 to 3, inclusive. Values greater than 3 for aux_format_idc are reserved to indicate the presence of exactly one
    // auxiliary coded picture in each access unit of the coded video sequence for purposes to be specified in the future by ITU-T
    // | ISO/IEC. When aux_format_idc is not present, it shall be inferred to be equal to 0.
    uint8_t auxFormatIdc;

    // bit_depth_aux_minus8 specifies the bit depth of the samples of the sample array of the auxiliary coded picture.
    // bit_depth_aux_minus8 shall be in the range of 0 to 4, inclusive.
    uint8_t bitDepthAuxMinus8;

    // alpha_incr_flag equal to 0 indicates that the interpretation sample value for each decoded auxiliary coded picture sample
    // value is equal to the decoded auxiliary coded picture sample value for purposes of alpha blending. alpha_incr_flag equal
    // to 1 indicates that, for purposes of alpha blending, after decoding the auxiliary coded picture samples, any auxiliary coded
    // picture sample value that is greater than Min(alpha_opaque_value, alpha_transparent_value) should be increased by one
    // to obtain the interpretation sample value for the auxiliary coded picture sample, and any auxiliary coded picture sample
    // value that is less than or equal to Min(alpha_opaque_value, alpha_transparent_value) should be used without alteration
    // as the interpretation sample value for the decoded auxiliary coded picture sample value
    bool alphaIncrFlag;

    // alpha_opaque_value specifies the interpretation sample value of an auxiliary coded picture sample for which the
    // associated luma and chroma samples of the same access unit are considered opaque for purposes of alpha blending. The
    // number of bits used for the representation of the alpha_opaque_value syntax element is bit_depth_aux_minus8 + 9 bits.
    uint8_t alphaOpaqueValue;

    // alpha_transparent_value specifies the interpretation sample value of an auxiliary coded picture sample for which the
    // associated luma and chroma samples of the same access unit are considered transparent for purposes of alpha blending.
    // The number of bits used for the representation of the alpha_transparent_value syntax element is
    // bit_depth_aux_minus8 + 9 bits
    uint8_t alphaTransparentValue;

    // additional_extension_flag equal to 0 indicates that no additional data follows within the sequence parameter set
    // extension syntax structure prior to the RBSP trailing bits. The value of additional_extension_flag shall be equal to 0. The
    // value of 1 for additional_extension_flag is reserved for future use by ITU-T | ISO/IEC. Decoders shall ignore all data that
    // follows the value of 1 for additional_extension_flag in a sequence parameter set extension NAL unit
    bool additionalExtensionFlag;
} picoH264SequenceParameterSetExtension_t;
typedef picoH264SequenceParameterSetExtension_t *picoH264SequenceParameterSetExtension;

typedef struct {
    // vui_ext_dependency_id[ i ] and vui_ext_quality_id[ i ] indicate the maximum value of DQId for the i-th subset of coded
    // video sequences. The maximum value of DQId for the i-th subset of coded video sequences is derived by
    // vui_ext_dependency_id[ i ] + ( vui_ext_quality_id[ i ] << 4 )
    uint8_t vuiExtDependencyId;
    uint8_t vuiExtQualityId;

    // vui_ext_temporal_id[ i ] indicates the maximum value of temporal_id for the i-th subset of coded video sequences.
    // The SVC VUI parameters extension syntax structure shall not contain two or more information entries with identical
    // values of vui_ext_dependency_id[ i ], vui_ext_quality_id[ i ], and vui_ext_temporal_id[ i ].
    // The following syntax elements apply to the coded video sequences that are obtained by the invoking the sub-bitstream
    // extraction process as specified in clause F.8.8.1 with tIdTarget equal to vui_ext_temporal_id[ i ], dIdTarget equal to
    // vui_ext_dependency_id[ i ], and qIdTarget equal to vui_ext_quality_id[ i ] as the inputs and the i-th subset of coded video
    // sequences as the output.
    uint8_t vuiExtTemporalId;

    // vui_ext_timing_info_present_flag[ i ] equal to 1 specifies that vui_ext_num_units_in_tick[ i ], vui_ext_time_scale[ i ],
    // and vui_ext_fixed_frame_rate_flag[ i ] for the i-th subset of coded video sequences are present in the SVC VUI
    // parameters extension. vui_ext_timing_info_present_flag[ i ] equal to 0 specifies that vui_ext_num_units_in_tick[ i ],
    // vui_ext_time_scale[ i ], and vui_ext_fixed_frame_rate_flag[ i ] for the i-th subset of coded video sequences are not
    // present in the SVC VUI parameters extension.
    // The following syntax elements for the i-th subset of coded video sequences are specified using references to Annex E.
    // For these syntax elements the same semantics and constraints as the ones specified in Annex E apply, as if these syntax
    // elements vui_ext_num_units_in_tick[ i ], vui_ext_time_scale[ i ], vui_ext_fixed_frame_rate_flag[ i ],
    // vui_ext_nal_hrd_parameters_present_flag[ i ], vui_ext_vcl_hrd_parameters_present_flag[ i ],
    // vui_ext_low_delay_hrd_flag[ i ], and vui_ext_pic_struct_present_flag[ i ] were present as the syntax elements
    // num_units_in_tick, time_scale, fixed_frame_rate_flag, nal_hrd_parameters_present_flag,
    // vcl_hrd_parameters_present_flag, low_delay_hrd_flag, and pic_struct_present_flag, respectively, in the VUI parameters
    // of the active SVC sequence parameter sets for the i-th subset of coded video sequences.
    bool vuiExtTimingInfoPresentFlag;

    // vui_ext_num_units_in_tick[ i ] specifies the value of num_units_in_tick, as specified in clause E.2.1, for the i-th subset
    // of coded video sequences.
    uint32_t vuiExtNumUnitsInTick;

    // vui_ext_time_scale[ i ] specifies the value of time_scale, as specified in clause E.2.1, for the i-th subset of coded video sequences.
    uint32_t vuiExtTimeScale;

    // vui_ext_fixed_frame_rate_flag[ i ] specifies the value of fixed_frame_rate_flag, as specified in clause E.2.1, for the i-
    // th subset of coded video sequences.
    bool vuiExtFixedFrameRateFlag;

    // vui_ext_nal_hrd_parameters_present_flag[ i ] specifies the value of nal_hrd_parameters_present_flag, as specified in
    // clause E.2.1, for the i-th subset of coded video sequences.
    // When vui_ext_nal_hrd_parameters_present_flag[ i ] is equal to 1, NAL HRD parameters (clauses E.1.2 and E.2.2) for
    // the i-th subset of coded video sequences immediately follow the flag
    bool vuiExtNalHrdParametersPresentFlag;
    picoH264HypotheticalReferenceDecoder_t nalHrdParameters;

    // vui_ext_vcl_hrd_parameters_present_flag[ i ] specifies the value of vcl_hrd_parameters_present_flag, as specified in
    // clause E.2.1, for the i-th subset of coded video sequences.
    // When vui_ext_vcl_hrd_parameters_present_flag[ i ] is equal to 1, VCL HRD parameters (clauses E.1.2 and E.2.2) for
    // the i-th subset of coded video sequences immediately follow the fla
    bool vuiExtVclHrdParametersPresentFlag;
    picoH264HypotheticalReferenceDecoder_t vclHrdParameters;

    // vui_ext_low_delay_hrd_flag[ i ] specifies the value of low_delay_hrd_flag, as specified in clause E.2.1, for the i-th
    // subset of coded video sequences
    bool vuiExtLowDelayHrdFlag;

    // vui_ext_pic_struct_present_flag[ i ] specifies the value of pic_struct_present_flag, as specified in clause E.2.1, for the
    // i-th subset of coded video sequences
    bool vuiExtPicStructPresentFlag;
} picoH264SVCVUIParametersExtensionEntry_t;
typedef picoH264SVCVUIParametersExtensionEntry_t *picoH264SVCVUIParametersExtensionEntry;

typedef struct {
    // vui_ext_num_entries_minus1 plus 1 specifies the number of information entries that are present in the SVC VUI
    // parameters extension syntax structure. The value of vui_ext_num_entries_minus1 shall be in the range of 0 to 1023,
    // inclusive. Each information entry is associated with particular values of temporal_id, dependency_id, and quality_id and
    // may indicate timing information, NAL HRD parameters, VCL HRD parameters, and the presence of picture structure
    // information for a particular subset of coded video sequences as specified in the following
    uint32_t vuiExtNumEntriesMinus1;

    picoH264SVCVUIParametersExtensionEntry_t vuiExtEntries[1024];
} picoH264SVCVUIParametersExtension_t;
typedef picoH264SVCVUIParametersExtension_t *picoH264SVCVUIParametersExtension;

typedef struct {
    // inter_layer_deblocking_filter_control_present_flag equal to 1 specifies that a set of syntax elements controlling the
    // characteristics of the deblocking filter for inter-layer prediction is present in the slice header.
    // inter_layer_deblocking_filter_control_present_flag equal to 0 specifies that the set of syntax elements controlling the
    // characteristics of the deblocking filter for inter-layer prediction is not present in the slice headers and their inferred values
    // are in effect.
    bool interLayerDeblockingFilterControlPresentFlag;

    // extended_spatial_scalability_idc specifies the presence of syntax elements related to geometrical parameters for the
    // resampling processes. The value of extended_spatial_scalability_idc shall be in the range of 0 to 2, inclusive, and the
    // following applies:
    //     – If extended_spatial_scalability_idc is equal to 0, no geometrical parameters are present in the subset sequence
    //     parameter set and the slice headers referring to this subset sequence parameter set.
    //     – Otherwise, if extended_spatial_scalability_idc is equal to 1, geometrical parameters are present in the subset
    //     sequence parameter set, but not in the slice headers referring to this subset sequence parameter set.
    //     – Otherwise (extended_spatial_scalability_idc is equal to 2), geometrical parameters are not present in the subset
    //     sequence parameter set, but they are present in the slice headers with no_inter_layer_pred_flag equal to 0 and
    //     quality_id equal to 0 that refer to this subset sequence parameter set
    uint8_t extendedSpatialScalabilityIDC;

    // chroma_phase_x_plus1_flag specifies the horizontal phase shift of the chroma components in units of half luma samples
    // of a frame or layer frame. When chroma_phase_x_plus1_flag is not present, it shall be inferred to be equal to 1.
    // When ChromaArrayType is equal to 1 and chroma_sample_loc_type_top_field and
    // chroma_sample_loc_type_bottom_field are present, the following applies:
    //     – If chroma_phase_x_plus1_flag is equal to 0, chroma_sample_loc_type_top_field and
    //     chroma_sample_loc_type_bottom_field should be equal to 0, 2, or 4.
    //     – Otherwise (chroma_phase_x_plus1_flag is equal to 1), chroma_sample_loc_type_top_field and
    //     chroma_sample_loc_type_bottom_field should be equal to 1, 3, or 5.
    //     When ChromaArrayType is equal to 2, chroma_phase_x_plus1_flag should be equal to 1.
    bool chromaPhaseXPlus1Flag;

    // chroma_phase_y_plus1 specifies the vertical phase shift of the chroma components in units of half luma samples of a
    // frame or layer frame. When chroma_phase_y_plus1 is not present, it shall be inferred to be equal to 1. The value of
    // chroma_phase_y_plus1 shall be in the range of 0 to 2, inclusive.
    // When ChromaArrayType is equal to 1 and chroma_sample_loc_type_top_field and
    // chroma_sample_loc_type_bottom_field are present, the following applies:
    //     – If chroma_phase_y_plus1 is equal to 0, chroma_sample_loc_type_top_field and
    //     chroma_sample_loc_type_bottom_field should be equal to 2 or 3.
    //     – Otherwise, if chroma_phase_y_plus1 is equal to 1, chroma_sample_loc_type_top_field and
    //     chroma_sample_loc_type_bottom_field should be equal to 0 or 1.
    //     – Otherwise (chroma_phase_y_plus1 is equal to 2), chroma_sample_loc_type_top_field and
    //     chroma_sample_loc_type_bottom_field should be equal to 4 or 5.
    uint8_t chromaPhaseYPlus1;

    // seq_ref_layer_chroma_phase_x_plus1_flag specifies the horizontal phase shift of the chroma components in units of
    // half luma samples of a layer frame for the layer pictures that may be used for inter-layer prediction. When
    // seq_ref_layer_chroma_phase_x_plus1_flag is not present, it shall be inferred to be equal to chroma_phase_x_plus1_flag.
    bool seqRefLayerChromaPhaseXPlus1Flag;

    // seq_ref_layer_chroma_phase_y_plus1 specifies the vertical phase shift of the chroma components in units of half luma
    // samples of a layer frame for the layer pictures that may be used for inter-layer prediction. When
    // seq_ref_layer_chroma_phase_y_plus1 is not present, it shall be inferred to be equal to chroma_phase_y_plus1. The value
    // of seq_ref_layer_chroma_phase_y_plus1 shall be in the range of 0 to 2, inclusive.
    uint8_t seqRefLayerChromaPhaseYPlus1;

    // seq_scaled_ref_layer_left_offset specifies the horizontal offset between the upper-left luma sample of a resampled layer
    // picture used for inter-layer prediction and the upper-left luma sample of the current picture or current layer picture in
    // units of two luma samples. When seq_scaled_ref_layer_left_offset is not present, it shall be inferred to be equal to 0. The
    // value of seq_scaled_ref_layer_left_offset shall be in the range of −2^15 to 2^15 − 1, inclusive.
    int32_t seqScaledRefLayerLeftOffset;

    // seq_scaled_ref_layer_top_offset specifies the vertical offset between the upper-left luma sample of a resampled layer
    // picture used for inter-layer prediction and the upper-left luma sample of the current picture or current layer picture.
    // Depending on the value of frame_mbs_only_flag, the following applies:
    //     – If frame_mbs_only_flag is equal to 1, the vertical offset is specified in units of two luma samples.
    //     – Otherwise (frame_mbs_only_flag is equal to 0), the vertical offset is specified in units of four luma samples.
    //     When seq_scaled_ref_layer_top_offset is not present, it shall be inferred to be equal to 0. The value of
    //     seq_scaled_ref_layer_top_offset shall be in the range of −2^15 to 2^15 − 1, inclusive
    int32_t seqScaledRefLayerTopOffset;

    // seq_scaled_ref_layer_right_offset specifies the horizontal offset between the bottom-right luma sample of a resampled
    // layer picture used for inter-layer prediction and the bottom-right luma sample of the current picture or current layer picture
    // in units of two luma samples. When seq_scaled_ref_layer_right_offset is not present, it shall be inferred to be equal to 0.
    // The value of seq_scaled_ref_layer_right_offset shall be in the range of −2^15 to 2^15 − 1, inclusive
    int32_t seqScaledRefLayerRightOffset;

    // seq_scaled_ref_layer_bottom_offset specifies the vertical offset between the bottom-right luma sample of a resampled
    // layer picture used for inter-layer prediction and the bottom-right luma sample of the current picture or current layer
    // picture. Depending on the value of frame_mbs_only_flag, the following applies:
    //     – If frame_mbs_only_flag is equal to 1, the vertical offset is specified in units of two luma samples.
    //     – Otherwise (frame_mbs_only_flag is equal to 0), the vertical offset is specified in units of four luma samples.
    //     When seq_scaled_ref_layer_bottom_offset is not present, it shall be inferred to be equal to 0. The value of
    //     seq_scaled_ref_layer_bottom_offset shall be in the range of −2^15 to 2^15 − 1, inclusive.
    int32_t seqScaledRefLayerBottomOffset;

    // seq_tcoeff_level_prediction_flag specifies the presence of the syntax element adaptive_tcoeff_level_prediction_flag in
    // the subset sequence parameter set
    bool seqTcoeffLevelPredictionFlag;

    // adaptive_tcoeff_level_prediction_flag specifies the presence of tcoeff_level_prediction_flag in slice headers that refer
    // to the subset sequence parameter set. When adaptive_tcoeff_level_prediction_flag is not present, it shall be inferred to be
    // equal to 0
    bool adaptiveTcoeffLevelPredictionFlag;

    // slice_header_restriction_flag specifies the presence of syntax elements in slice headers that refer to the subset sequence
    // parameter set
    bool sliceHeaderRestrictionFlag;
} picoH264SPSSVCExtension_t;
typedef picoH264SPSSVCExtension_t *picoH264SPSSVCExtension;

typedef struct {
    // num_views_minus1 plus 1 specifies the maximum number of coded views in the coded video sequence. The value of
    // num_view_minus1 shall be in the range of 0 to 1023, inclusive
    uint16_t numViewsMinus1;

    // view_id[ i ] specifies the view_id of the view with VOIdx equal to i. The value of view_id[ i ] shall be in the range of 0
    // to 1023, inclusive
    uint16_t viewId[1024];

    // num_anchor_refs_l0[ i ] specifies the number of view components for inter-view prediction in the initial reference
    // picture list RefPicList0 (which is derived as specified in clause G.8.2.1) in decoding anchor view components with VOIdx
    // equal to i. The value of num_anchor_refs_l0[ i ] shall not be greater than Min( 15, num_views_minus1 ). The value of
    // num_anchor_refs_l0[ 0 ] shall be equal to 0
    uint8_t numAnchorRefsL0[1024];

    // anchor_ref_l0[ i ][ j ] specifies the view_id of the j-th view component for inter-view prediction in the initial reference
    // picture list RefPicList0 (which is derived as specified in clause G.8.2.1) in decoding anchor view components with VOIdx
    // equal to i. The value of anchor_ref_l0[ i ][ j ] shall be in the range of 0 to 1023, inclusive.
    uint16_t anchorRefL0[1024][16];

    // num_anchor_refs_l1[ i ] specifies the number of view components for inter-view prediction in the initial reference
    // picture list RefPicList1 (which is derived as specified in clause G.8.2.1) in decoding anchor view components with VOIdx
    // equal to i. The value of num_anchor_refs_l1[ i ] shall not be greater than Min( 15, num_views_minus1 ). The value of
    // num_anchor_refs_l1[ 0 ] shall be equal to 0
    uint8_t numAnchorRefsL1[1024];

    // anchor_ref_l1[ i ][ j ] specifies the view_id of the j-th view component for inter-view prediction in the initial reference
    // picture list RefPicList1 (which is derived as specified in clause G.8.2.1) in decoding an anchor view component with
    // VOIdx equal to i. The value of anchor_ref_l1[ i ][ j ] shall be in the range of 0 to 1023, inclusive.
    uint16_t anchorRefL1[1024][16];

    // num_non_anchor_refs_l0[ i ] specifies the number of view components for inter-view prediction in the initial reference
    // picture list RefPicList0 (which is derived as specified in clause G.8.2.1) in decoding non-anchor view components with
    // VOIdx equal to i. The value of num_non_anchor_refs_l0[ i ] shall not be greater than Min( 15, num_views_minus1 ). The
    // value of num_non_anchor_refs_l0[ 0 ] shall be equal to 0.
    uint8_t numNonAnchorRefsL0[1024];

    // non_anchor_ref_l0[ i ][ j ] specifies the view_id of the j-th view component for inter-view prediction in the initial
    // reference picture list RefPicList0 (which is derived as specified in clause G.8.2.1) in decoding non-anchor view
    // components with VOIdx equal to i. The value of non_anchor_ref_l0[ i ][ j ] shall be in the range of 0 to 1023, inclusive.
    uint16_t nonAnchorRefL0[1024][16];

    // num_non_anchor_refs_l1[ i ] specifies the number of view components for inter-view prediction in the initial reference
    // picture list RefPicList1 (which is derived as specified in clause G.8.2.1) in decoding non-anchor view components with
    // VOIdx equal to i. The value of num_non_anchor_refs_l1[ i ] shall not be greater than Min( 15, num_views_minus1 ). The
    // value of num_non_anchor_refs_l1[ 0 ] shall be equal to 0.
    uint8_t numNonAnchorRefsL1[1024];

    // non_anchor_ref_l1[ i ][ j ] specifies the view_id of the j-th view component for inter-view prediction in the initial
    // reference picture list RefPicList1 (which is derived as specified in clause G.8.2.1) in decoding non-anchor view
    // components with VOIdx equal to i. The value of non_anchor_ref_l1[ i ][ j ] shall be in the range of 0 to 1023, inclusive.
    // For any particular view with view_id equal to vId1 and VOIdx equal to vOIdx1 and another view with view_id equal to
    // vId2 and VOIdx equal to vOIdx2, when vId2 is equal to the value of one of non_anchor_ref_l0[ vOIdx1 ][ j ] for all j in
    // the range of 0 to num_non_anchor_refs_l0[ vOIdx1 ], exclusive, or one of non_anchor_ref_l1[ vOIdx1 ][ j ] for all j in
    // the range of 0 to num_non_anchor_refs_l1[ vOIdx1 ], exclusive, vId2 shall also be equal to the value of one of
    // anchor_ref_l0[ vOIdx1 ][ j ] for all j in the range of 0 to num_anchor_refs_l0[ vOIdx1 ], exclusive, or one of
    // anchor_ref_l1[ vOIdx1 ][ j ] for all j in the range of 0 to num_anchor_refs_l1[ vOIdx1 ], exclusive.
    uint16_t nonAnchorRefL1[1024][16];

    // num_level_values_signalled_minus1 plus 1 specifies the number of level values signalled for the coded video sequence.
    // The value of num_level_values_signalled_minus1 shall be in the range of 0 to 63, inclusive.
    uint8_t numLevelValuesSignalledMinus1;

    // level_idc[ i ] specifies the i-th level value signalled for the coded video sequence.
    uint8_t levelIDC[64];

    // num_applicable_ops_minus1[ i ] plus 1 specifies the number of operation points to which the level indicated by
    // level_idc[ i ] applies. The value of num_applicable_ops_minus1[ i ] shall be in the range of 0 to 1023, inclusive.
    uint16_t numApplicableOpsMinus1[64];

    // applicable_op_temporal_id[ i ][ j ] specifies the temporal_id of the j-th operation point to which the level indicated by
    // level_idc[ i ] applies.
    uint8_t applicableOpTemporalId[64][1024];

    // applicable_op_num_target_views_minus1[ i ][ j ] plus 1 specifies the number of target output views for the j-th
    // operation point to which the level indicated by level_idc[ i ] applies. The value of
    // applicable_op_num_target_views_minus1[ i ][ j ] shall be in the range of 0 to 1023, inclusive.
    uint16_t applicableOpNumTargetViewsMinus1[64][1024];

    // applicable_op_target_view_id[ i ][ j ][ k ] specifies the k-th target output view for the j-th operation point to which the
    // level indicated by level_idc[ i ] applies. The value of applicable_op_target_view_id[ i ][ j ][ k ] shall be in the range of 0
    // to 1023, inclusive
    // Let maxTId be the greatest temporal_id of all NAL units in the coded video sequence, and vId be view_id of any view in
    // the coded video sequence. There shall be one set of applicable_op_temporal_id[ i ][ j ],
    // applicable_op_num_target_views_minus1[ i ][ j ], and applicable_op_target_view_id[ i ][ j ][ k ], for any i and j and all
    // k for the i and j, in which applicable_op_temporal_id[ i ][ j ] is equal to maxTId,
    // applicable_op_num_target_views_minus1[ i ][ j ] is equal to 0, and applicable_op_target_view_id[ i ][ j ][ k ] is equal to vId
    uint16_t applicableOpTargetViewId[64][1024][1024];

    // applicable_op_num_views_minus1[ i ][ j ] plus 1 specifies the number of views required for decoding the target output
    // views corresponding to the j-th operation point to which the level indicated by level_idc[ i ] applies. The number of views
    // specified by applicable_op_num_views_minus1 includes the target output views and the views that the target output
    // views depend on as specified by the sub-bitstream extraction process in clause G.8.5 with tIdTarget equal to
    // applicable_op_temporal_id[ i ][ j ] and viewIdTargetList equal to the list of applicable_op_target_view_id[ i ][ j ][ k ] for
    // all k in the range of 0 to applicable_op_num_target_views_minus1[ i ][ j ], inclusive, as inputs. The value of
    // applicable_op_num_views_minus1[ i ][ j ] shall be in the range of 0 to 1023, inclusive.
    uint16_t applicableOpNumViewsMinus1[64][1024];

    // mfc_format_idc specifies the frame packing arrangement type for view components of the base view and the
    // corresponding frame packing arrangement type for view components in the non-base view. The semantics of
    // mfc_format_idc equal to 0 and 1 are specified by Table G-1.
    // In bitstreams conforming to this version of this Specification, the value of mfc_format_idc shall be equal to 0 or 1. Values
    // of mfc_format_idc in the range of 2..63 are reserved for future use by ITU-T | ISO/IEC. Decoders shall ignore the coded
    // video sequence when the value of mfc_format_idc is greater tha
    uint8_t mfcFormatIDC;

    // default_grid_position_flag equal to 0 specifies that the syntax elements view0_grid_position_x, view0_grid_position_y,
    // view1_grid_position_x, and view1_grid_position_y are present. default_grid_position_flag equal to 1 specifies that
    // view0_grid_position_x, view0_grid_position_y, view1_grid_position_x, and view1_grid_position_y are not present.
    bool defaultGridPositionFlag;

    // view0_grid_position_x has the same semantics as specified in clause D.2.26 for the frame0_grid_position_x syntax
    // element. The value of view0_grid_position_x shall be equal to 4, 8 or 12
    uint8_t view0GridPositionX;

    // view0_grid_position_y has the same semantics as specified in clause D.2.26 for the frame0_grid_position_y syntax
    // element. The value of view0_grid_position_y shall be equal to 4, 8 or 12
    uint8_t view0GridPositionY;

    // view1_grid_position_x has the same semantics as specified in clause D.2.26 for the frame1_grid_position_x syntax
    // element. The value of view1_grid_position_x shall be equal to 4, 8 or 12.
    uint8_t view1GridPositionX;

    // view1_grid_position_y has the same semantics as specified in clause D.2.26 for the frame1_grid_position_y syntax
    // element. The value of view1_grid_position_y shall be equal to 4, 8 or 12.
    uint8_t view1GridPositionY;

    // rpu_filter_enabled_flag equal to 1 specifies that a downsampling filter process and an upsampling filter process are used
    // to generate each colour component of an inter-view prediction reference. rpu_filter_enabled_flag equal to 0 specifies that
    // all sample values for each colour component of an inter-view prediction reference are set equal to 128
    bool rpuFilterEnabledFlag;

    // rpu_field_processing_flag equal to 0 specifies that each inter-view prediction reference with field_pic_flag equal to 0 is
    // processed as a frame when processed by the RPU. rpu_field_processing_flag equal to 1 specifies that each inter-view
    // prediction reference with field_pic_flag equal to 0 is processed as two fields when processed by the RPU. When not
    // present, the value of rpu_field_processing_flag is inferred to be equal to 0
    bool rpuFieldProcessingFlag;

} picoH264SPSMVCExtension_t;
typedef picoH264SPSMVCExtension_t *picoH264SPSMVCExtension;

typedef struct {
    // vui_mvc_temporal_id[ i ] indicates the maximum value of temporal_id for all VCL NAL units in the representation of
    // the i-th operation point
    uint8_t vuiMVCOpsTemporalId;

    // vui_mvc_num_target_output_views_minus1[ i ] plus one specifies the number of target output views for the i-th
    // operation point. The value of vui_mvc_num_target_output_views_minus1[ i ] shall be in the range of 0 to 1023, inclusive.
    uint16_t vuiMVCOpsNumTargetViewsMinus1;

    // vui_mvc_view_id[ i ][ j ] indicates the j-th target output view in the i-th operation point. The value of
    // vui_mvc_view_id[ i ] shall be in the range of 0 to 1023, inclusive.
    // The following syntax elements apply to the coded video sequence that is obtained by the sub-bitstream extraction process
    // as specified in clause G.8.5.3 with tIdTarget equal to vui_mvc_temporal_id[ i ] and viewIdTargetList containing
    // vui_mvc_view_id[ i ][ j ] for all j in the range of 0 to vui_mvc_num_target_output_views_minus1[ i ], inclusive, as the
    // inputs and the i-th sub-bitstream as the output
    uint16_t vuiMVCOpsTargetViewId[1024];

    // vui_mvc_timing_info_present_flag[ i ] equal to 1 specifies that vui_mvc_num_units_in_tick[ i ],
    // vui_mvc_time_scale[ i ], and vui_mvc_fixed_frame_rate_flag[ i ] for the i-th sub-bitstream are present in the MVC VUI
    // parameters extension. vui_mvc_timing_info_present_flag[ i ] equal to 0 specifies that vui_mvc_num_units_in_tick[ i ],
    // vui_mvc_time_scale[ i ], and vui_mvc_fixed_frame_rate_flag[ i ] for the i-th sub-bitstream are not present in the MVC
    // VUI parameters extension.
    // The following syntax elements for the i-th sub-bitstream are specified using references to Annex E. For these syntax
    // elements the same semantics and constraints as the ones specified in Annex E apply, as if these syntax elements
    // vui_mvc_num_units_in_tick[ i ], vui_mvc_time_scale[ i ], vui_mvc_fixed_frame_rate_flag[ i ],
    // vui_mvc_nal_hrd_parameters_present_flag[ i ], vui_mvc_vcl_hrd_parameters_present_flag[ i ],
    // vui_mvc_low_delay_hrd_flag[ i ], and vui_mvc_pic_struct_present_flag[ i ] were present as the syntax elements
    // num_units_in_tick, time_scale, fixed_frame_rate_flag, nal_hrd_parameters_present_flag,
    // vcl_hrd_parameters_present_flag, low_delay_hrd_flag, and pic_struct_present_flag, respectively, in the VUI parameters
    // of the active MVC sequence parameter sets for the i-th sub-bitstream.
    bool vuiMVCTimingInfoPresentFlag;

    // vui_mvc_num_units_in_tick[ i ] specifies the value of num_units_in_tick, as specified in clause E.2.1, for the i-th sub-
    // bitstream.
    uint32_t vuiMVCNumUnitsInTick;

    // vui_mvc_time_scale[ i ] specifies the value of time_scale, as specified in clause E.2.1, for the i-th sub-bitstream.
    uint32_t vuiMVCTimeScale;

    // vui_mvc_fixed_frame_rate_flag[ i ] specifies the value of fixed_frame_rate_flag, as specified in clause E.2.1, for the i-
    // th sub-bitstream.
    bool vuiMVCFixedFrameRateFlag;

    // vui_mvc_nal_hrd_parameters_present_flag[ i ] specifies the value of nal_hrd_parameters_present_flag, as specified
    // in clause E.2.1, for the i-th sub-bitstream.
    // When vui_mvc_nal_hrd_parameters_present_flag[ i ] is equal to 1, NAL HRD parameters (clauses E.1.2 and E.2.2) for
    // the i-th sub-bitstream immediately follow the flag.
    bool vuiMVCNalHrdParametersPresentFlag;
    picoH264HypotheticalReferenceDecoder_t vuiMVCNalHrdParameters;

    // vui_mvc_vcl_hrd_parameters_present_flag[ i ] specifies the value of vcl_hrd_parameters_present_flag, as specified in
    // clause E.2.1, for the i-th sub-bitstream.
    // When vui_mvc_vcl_hrd_parameters_present_flag[ i ] is equal to 1, VCL HRD parameters (clauses E.1.2 and E.2.2) for
    // the i-th sub-bitstream immediately follow the flag
    bool vuiMVCVclHrdParametersPresentFlag;
    picoH264HypotheticalReferenceDecoder_t vuiMVCVclHrdParameters;

    // vui_mvc_low_delay_hrd_flag[ i ] specifies the value of low_delay_hrd_flag, as specified in clause E.2.1, for the i-th
    // sub-bitstream.
    bool vuiMVCLowDelayHrdFlag;

    // vui_mvc_pic_struct_present_flag[ i ] specifies the value of pic_struct_present_flag, as specified in clause E.2.1, for the
    // i-th sub-bitstream
    bool vuiMVCPicStructPresentFlag;
} picoH264MVCVUIParametersExtensionOpsEntry_t;
typedef picoH264MVCVUIParametersExtensionOpsEntry_t *picoH264MVCVUIParametersExtensionOpsEntry;

typedef struct {
    // vui_mvc_num_ops_minus1 plus 1 specifies the number of operation points for which timing information, NAL HRD
    // parameters, VCL HRD parameters, and the pic_struct_present_flag may be present. The value of
    // vui_mvc_num_ops_minus1 shall be in the range of 0 to 1023, inclusive
    uint16_t vuiMVCNumOpsMinus1;
    picoH264MVCVUIParametersExtensionOpsEntry_t vuiMVCOpsEntries[1024];
} picoH264MVCVUIParametersExtension_t;
typedef picoH264MVCVUIParametersExtension_t *picoH264MVCVUIParametersExtension;

// NOTE: We only implement this partially, only the parts needed for SVC and MVC parsing
typedef struct {
    // the base SPS data
    picoH264SequenceParameterSet_t spsData;

    bool hasSvcExtension; // present if profile_idc is 83 or 86
    picoH264SPSSVCExtension_t svcExtension;

    // svc_vui_parameters_present_flag equal to 0 specifies that the syntax structure svc_vui_parameters_extension( ) is not
    // present. svc_vui_parameters_present_flag equal to 1 specifies that the syntax structure svc_vui_parameters_extension( )
    // is present
    bool svcVuiParametersPresentFlag;
    picoH264SVCVUIParametersExtension_t svcVuiParametersExtension;

    bool hasMvcExtension; // present if profile_idc is 118, 128, or 134
    picoH264SPSMVCExtension_t mvcExtension;

    // mvc_vui_parameters_present_flag equal to 0 specifies that the syntax structure mvc_vui_parameters_extension( ) is
    // not present. mvc_vui_parameters_present_flag equal to 1 specifies that the syntax structure
    // mvc_vui_parameters_extension( ) is present
    bool mvcVuiParametersPresentFlag;
    picoH264MVCVUIParametersExtension_t mvcVuiParametersExtension;

    // we don't currently parse the rest of the extensions.
    // can be added if needed, pull requests welcome!
} picoH264SubsetSequenceParameterSet_t;
typedef picoH264SubsetSequenceParameterSet_t *picoH264SubsetSequenceParameterSet;

typedef struct {
    // pic_parameter_set_id identifies the picture parameter set that is referred to in the slice header.
    // The value of pic_parameter_set_id shall be in the range of 0 to 255, inclusive.
    uint8_t picParameterSetId;

    // seq_parameter_set_id refers to the active sequence parameter set.
    // The value of seq_parameter_set_id shall be in the range of 0 to 31, inclusive.
    uint8_t seqParameterSetId;

    // entropy_coding_mode_flag selects the entropy decoding method to be applied for the syntax elements for which two
    // descriptors appear in the syntax tables as follows:
    //     – If entropy_coding_mode_flag is equal to 0, the method specified by the left descriptor in the syntax table is applied
    //     (Exp-Golomb coded, see clause 9.1 or CAVLC, see clause 9.2).
    //     – Otherwise (entropy_coding_mode_flag is equal to 1), the method specified by the right descriptor in the syntax table
    //     is applied (CABAC, see clause 9.3)
    bool entropyCodingModeFlag;

    // bottom_field_pic_order_in_frame_present_flag equal to 1 specifies that the syntax elements
    // delta_pic_order_cnt_bottom (when pic_order_cnt_type is equal to 0) or delta_pic_order_cnt[ 1 ] (when
    // pic_order_cnt_type is equal to 1), which are related to picture order counts for the bottom field of a coded frame, are
    // present in the slice headers for coded frames as specified in clause 7.3.3. bottom_field_pic_order_in_frame_present_flag
    // equal to 0 specifies that the syntax elements delta_pic_order_cnt_bottom and delta_pic_order_cnt[ 1 ] are not present in
    // the slice headers
    bool bottomFieldPicOrderInFramePresentFlag;

    // num_slice_groups_minus1 plus 1 specifies the number of slice groups for a picture.
    // When num_slice_groups_minus1 is equal to 0, all slices of the picture belong to the same slice group.
    // The allowed range of num_slice_groups_minus1 is specified in Annex A.
    uint32_t numSliceGroupsMinus1;

    // slice_group_map_type specifies how the mapping of slice group map units to slice groups is coded. The value of
    // slice_group_map_type shall be in the range of 0 to 6, inclusive.
    // slice_group_map_type equal to 0 specifies interleaved slice groups.
    // slice_group_map_type equal to 1 specifies a dispersed slice group mapping.
    // slice_group_map_type equal to 2 specifies one or more "foreground" slice groups and a "leftover" slice group.
    // slice_group_map_type values equal to 3, 4, and 5 specify changing slice groups. When num_slice_groups_minus1 is not
    // equal to 1, slice_group_map_type shall not be equal to 3, 4, or 5.
    // slice_group_map_type equal to 6 specifies an explicit assignment of a slice group to each slice group map unit
    uint8_t sliceGroupMapType;

    // run_length_minus1[i] is used to specify the number of consecutive slice group map units to be assigned
    // to the i-th slice group in raster scan order of slice group map units.
    // The value of run_length_minus1[i] shall be in the range of 0 to PicSizeInMapUnits − 1, inclusive.
    uint32_t runLengthMinus1[1024];

    // top_left[i] and bottom_right[i] specify the top-left and bottom-right corners of a rectangle, respectively.
    // top_left[i] and bottom_right[i] are slice group map unit positions in a raster scan of the picture.
    uint32_t topLeft[256];
    uint32_t bottomRight[256];

    // slice_group_change_direction_flag is used with slice_group_map_type to specify the refined map type
    // when slice_group_map_type is equal to 3, 4, or 5.
    bool sliceGroupChangeDirectionFlag;

    // slice_group_change_rate_minus1 is used to specify the variable SliceGroupChangeRate.
    uint32_t sliceGroupChangeRateMinus1;

    // pic_size_in_map_units_minus1 is used to specify the number of slice group map units in the picture.
    // pic_size_in_map_units_minus1 shall be equal to PicSizeInMapUnits − 1.
    uint32_t picSizeInMapUnitsMinus1;

    // slice_group_id[i] identifies a slice group of the i-th slice group map unit in raster scan order.
    // The length of the slice_group_id[i] syntax element is Ceil(Log2(num_slice_groups_minus1 + 1)) bits.
    // The value of slice_group_id[i] shall be in the range of 0 to num_slice_groups_minus1, inclusive.
    uint32_t sliceGroupId[1024]; // dynamically allocated array

    // num_ref_idx_l0_default_active_minus1 specifies how num_ref_idx_l0_active_minus1 is inferred for P, SP, and B slices
    // with num_ref_idx_active_override_flag equal to 0.
    // The value of num_ref_idx_l0_default_active_minus1 shall be in the range of 0 to 31, inclusive.
    uint8_t numRefIdxL0DefaultActiveMinus1;

    // num_ref_idx_l1_default_active_minus1 specifies how num_ref_idx_l1_active_minus1 is inferred for B slices
    // with num_ref_idx_active_override_flag equal to 0.
    // The value of num_ref_idx_l1_default_active_minus1 shall be in the range of 0 to 31, inclusive.
    uint8_t numRefIdxL1DefaultActiveMinus1;

    // weighted_pred_flag equal to 0 specifies that the default weighted prediction shall be applied to P and SP slices.
    // weighted_pred_flag equal to 1 specifies that explicit weighted prediction shall be applied to P and SP slices.
    bool weightedPredFlag;

    // weighted_bipred_idc equal to 0 specifies that the default weighted prediction shall be applied to B slices.
    // weighted_bipred_idc equal to 1 specifies that explicit weighted prediction shall be applied to B slices.
    // weighted_bipred_idc equal to 2 specifies that implicit weighted prediction shall be applied to B slices.
    // The value of weighted_bipred_idc shall be in the range of 0 to 2, inclusive.
    uint8_t weightedBipredIdc;

    // pic_init_qp_minus26 specifies the initial value minus 26 of SliceQPY for each slice.
    // The initial value is modified at the slice layer when a non-zero value of slice_qp_delta is decoded.
    // The value of pic_init_qp_minus26 shall be in the range of −(26 + QpBdOffsetY) to +25, inclusive.
    int8_t picInitQpMinus26;

    // pic_init_qs_minus26 specifies the initial value minus 26 of SliceQSY for all macroblocks in SP or SI slices.
    // The initial value is modified at the slice layer when a non-zero value of slice_qs_delta is decoded.
    // The value of pic_init_qs_minus26 shall be in the range of −26 to +25, inclusive.
    int8_t picInitQsMinus26;

    // chroma_qp_index_offset specifies the offset that shall be added to QPY and QSY for addressing the table of QPc values
    // for the Cb chroma component.
    // The value of chroma_qp_index_offset shall be in the range of −12 to +12, inclusive.
    int8_t chromaQpIndexOffset;

    // deblocking_filter_control_present_flag equal to 1 specifies that a set of syntax elements controlling the
    // characteristics of the deblocking filter is present in the slice header.
    // deblocking_filter_control_present_flag equal to 0 specifies that the set of syntax elements controlling
    // the characteristics of the deblocking filter is not present in the slice headers.
    bool deblockingFilterControlPresentFlag;

    // constrained_intra_pred_flag equal to 0 specifies that intra prediction allows usage of residual data and
    // decoded samples of neighbouring macroblocks coded using Inter macroblock prediction modes.
    // constrained_intra_pred_flag equal to 1 specifies constrained intra prediction, in which case prediction
    // of macroblocks coded using Intra macroblock prediction modes uses residual data and decoded samples
    // from I or SI macroblock types only.
    bool constrainedIntraPredFlag;

    // redundant_pic_cnt_present_flag equal to 0 specifies that the redundant_pic_cnt syntax element is not present
    // in slice headers, coded slice data partition B NAL units, and coded slice data partition C NAL units.
    // redundant_pic_cnt_present_flag equal to 1 specifies that the redundant_pic_cnt syntax element is present.
    bool redundantPicCntPresentFlag;

    // transform_8x8_mode_flag equal to 1 specifies that the 8x8 transform decoding process may be in use.
    // transform_8x8_mode_flag equal to 0 specifies that the 8x8 transform decoding process is not in use.
    bool transform8x8ModeFlag;

    // pic_scaling_matrix_present_flag equal to 1 specifies that parameters are present to modify the scaling lists
    // specified in the sequence parameter set.
    // pic_scaling_matrix_present_flag equal to 0 specifies that the scaling lists used for the picture shall be
    // inferred to be equal to those specified by the sequence parameter set.
    bool picScalingMatrixPresentFlag;

    // pic_scaling_list_present_flag[i] equal to 1 specifies that the scaling list syntax structure is present to specify
    // the scaling list for index i.
    // pic_scaling_list_present_flag[i] equal to 0 specifies that the syntax structure for scaling list i is not present.
    bool picScalingListPresentFlag[12]; // 6 for 4x4 + 6 for 8x8

    // ScalingList4x4 and ScalingList8x8 specify the scaling lists.
    uint8_t scalingList4x4[6][16];
    uint8_t scalingList8x8[6][64];

    // UseDefaultScalingMatrix4x4Flag and UseDefaultScalingMatrix8x8Flag specify whether default scaling matrices are used.
    bool useDefaultScalingMatrix4x4Flag[6];
    bool useDefaultScalingMatrix8x8Flag[6];

    // second_chroma_qp_index_offset specifies the offset that shall be added to QPY and QSY for addressing
    // the table of QPc values for the Cr chroma component.
    // The value of second_chroma_qp_index_offset shall be in the range of −12 to +12, inclusive.
    int8_t secondChromaQpIndexOffset;

} picoH264PictureParameterSet_t;
typedef picoH264PictureParameterSet_t *picoH264PictureParameterSet;

// NOTE: we do not currently parse the individual SEI message types.
//       as there are way too many of them, we can add specific parsers if needed
//       but currently we dont have any use cases for that, so keeping it simple.
//       pull requests welcome!
typedef struct {
    picoH264SEIMessageType payloadType;
    size_t payloadSize;
    uint8_t *payloadData; // pointer to payload data from the NAL unit buffer
} picoH264SEIMessage_t;
typedef picoH264SEIMessage_t *picoH264SEIMessage;

typedef struct {
    // primary_pic_type indicates that the slice_type values for all slices of the primary coded picture are members of the set
    // listed in Table 7-5 for the given value of primary_pic_type.
    uint8_t primaryPicType;
} picoH264AccessUnitDelimiter_t;
typedef picoH264AccessUnitDelimiter_t *picoH264AccessUnitDelimiter;

typedef struct {
    // modification_of_pic_nums_idc together with abs_diff_pic_num_minus1 or long_term_pic_num specifies which of the
    // reference pictures are re-mapped. The values of modification_of_pic_nums_idc are specified in Table 7-7. The value of
    // the first modification_of_pic_nums_idc that follows immediately after ref_pic_list_modification_flag_l0 or
    // ref_pic_list_modification_flag_l1 shall not be equal to 3.
    uint32_t modificationOfPicNumsIdc;

    // abs_diff_pic_num_minus1 plus 1 specifies the absolute difference between the picture number of the picture being
    // moved to the current index in the list and the picture number prediction value. abs_diff_pic_num_minus1 shall be in the
    // range of 0 to MaxPicNum − 1. The allowed values of abs_diff_pic_num_minus1 are further restricted as specified in
    // clause 8.2.4.3.1.
    uint32_t absDiffPicNumMinus1;

    // long_term_pic_num specifies the long-term picture number of the picture being moved to the current index in the list.
    // When decoding a coded frame, long_term_pic_num shall be equal to a LongTermPicNum assigned to one of the reference
    // frames or complementary reference field pairs marked as "used for long-term reference". When decoding a coded field,
    // long_term_pic_num shall be equal to a LongTermPicNum assigned to one of the reference fields marked as "used for
    // long-term reference".
    uint32_t longTermPicNum;

    // abs_diff_view_idx_minus1 plus 1 specifies the absolute difference between the reference view index to put to the current
    // index in the reference picture list and the prediction value of the reference view index
    // NOTE: only used for MVC
    uint32_t abs_diff_view_idx_minus1;
} picoH264RefPicListModificationEntry_t;
typedef picoH264RefPicListModificationEntry_t *picoH264RefPicListModificationEntry;

typedef struct {
    // ref_pic_list_modification_flag_l0 equal to 1 specifies that the syntax element modification_of_pic_nums_idc is present
    // for specifying reference picture list 0. ref_pic_list_modification_flag_l0 equal to 0 specifies that this syntax element is
    // not present.
    // When ref_pic_list_modification_flag_l0 is equal to 1, the number of times that modification_of_pic_nums_idc is not
    // equal to 3 following ref_pic_list_modification_flag_l0 shall not exceed num_ref_idx_l0_active_minus1 + 1.
    // When RefPicList0[ num_ref_idx_l0_active_minus1 ] in the initial reference picture list produced as specified in
    // clause 8.2.4.2 is equal to "no reference picture", ref_pic_list_modification_flag_l0 shall be equal to 1 and
    // modification_of_pic_nums_idc shall not be equal to 3 until RefPicList0[ num_ref_idx_l0_active_minus1 ] in the
    // modified list produced as specified in clause 8.2.4.3 is not equal to "no reference picture"
    bool refPicListModificationFlagL0;

    uint32_t numModificationsL0;
    picoH264RefPicListModificationEntry_t modificationsL0[128];

    // ref_pic_list_modification_flag_l1 equal to 1 specifies that the syntax element modification_of_pic_nums_idc is present
    // for specifying reference picture list 1. ref_pic_list_modification_flag_l1 equal to 0 specifies that this syntax element is
    // not present.
    // When ref_pic_list_modification_flag_l1 is equal to 1, the number of times that modification_of_pic_nums_idc is not
    // equal to 3 following ref_pic_list_modification_flag_l1 shall not exceed num_ref_idx_l1_active_minus1 + 1.
    // When decoding a slice with slice_type equal to 1 or 6 and RefPicList1[ num_ref_idx_l1_active_minus1 ] in the initial
    // reference picture list produced as specified in clause 8.2.4.2 is equal to "no reference picture",
    // ref_pic_list_modification_flag_l1 shall be equal to 1 and modification_of_pic_nums_idc shall not be equal to 3 until
    // RefPicList1[ num_ref_idx_l1_active_minus1 ] in the modified list produced as specified in clause 8.2.4.3 is not equal to
    // "no reference picture".
    bool refPicListModificationFlagL1;

    uint32_t numModificationsL1;
    picoH264RefPicListModificationEntry_t modificationsL1[128];
} picoH264RefPicListModification_t;
typedef picoH264RefPicListModification_t *picoH264RefPicListModification;

typedef struct {
    // luma_log2_weight_denom is the base 2 logarithm of the denominator for all luma weighting factors. The value of
    // luma_log2_weight_denom shall be in the range of 0 to 7, inclusive
    uint8_t lumaLog2WeightDenom;

    // chroma_log2_weight_denom is the base 2 logarithm of the denominator for all chroma weighting factors. The value of
    // chroma_log2_weight_denom shall be in the range of 0 to 7, inclusive.
    uint32_t chromaLog2WeightDenom;

    // luma_weight_l0_flag equal to 1 specifies that weighting factors for the luma component of list 0 prediction are present.
    // luma_weight_l0_flag equal to 0 specifies that these weighting factors are not present.
    bool lumaWeightL0Flag[32];

    // luma_weight_l0[ i ] is the weighting factor applied to the luma prediction value for list 0 prediction using RefPicList0[ i ].
    // When luma_weight_l0_flag is equal to 1, the value of luma_weight_l0[ i ] shall be in the range of −128 to 127, inclusive.
    // When luma_weight_l0_flag is equal to 0, luma_weight_l0[ i ] shall be inferred to be equal to 2
    // luma_log2_weight_denom for
    // RefPicList0[ i ].
    int8_t lumaWeightL0[32];

    // luma_offset_l0[ i ] is the additive offset applied to the luma prediction value for list 0 prediction using RefPicList0[ i ].
    // The value of luma_offset_l0[ i ] shall be in the range of −128 to 127, inclusive. When luma_weight_l0_flag is equal to 0,
    // luma_offset_l0[ i ] shall be inferred as equal to 0 for RefPicList0[ i ].
    int8_t lumaOffsetL0[32];

    // chroma_weight_l0_flag equal to 1 specifies that weighting factors for the chroma prediction values of list 0 prediction
    // are present. chroma_weight_l0_flag equal to 0 specifies that these weighting factors are not present
    bool chromaWeightL0Flag[32];

    // chroma_weight_l0[ i ][ j ] is the weighting factor applied to the chroma prediction values for list 0 prediction using
    // RefPicList0[ i ] with j equal to 0 for Cb and j equal to 1 for Cr. When chroma_weight_l0_flag is equal to 1, the value of
    // chroma_weight_l0[ i ][ j ] shall be in the range of −128 to 127, inclusive. When chroma_weight_l0_flag is equal to 0,
    // chroma_weight_l0[ i ][ j ] shall be inferred to be equal to 2^chroma_log2_weight_denom for RefPicList0[ i ].
    int8_t chromaWeightL0[32][2];

    // chroma_offset_l0[ i ][ j ] is the additive offset applied to the chroma prediction values for list 0 prediction using
    // RefPicList0[ i ] with j equal to 0 for Cb and j equal to 1 for Cr. The value of chroma_offset_l0[ i ][ j ] shall be in the range
    // of −128 to 127, inclusive. When chroma_weight_l0_flag is equal to 0, chroma_offset_l0[ i ][ j ] shall be inferred to be
    // equal to 0 for RefPicList0[ i ].
    int8_t chromaOffsetL0[32][2];

    // luma_weight_l1_flag, luma_weight_l1, luma_offset_l1, chroma_weight_l1_flag, chroma_weight_l1,
    // chroma_offset_l1 have the same semantics as luma_weight_l0_flag, luma_weight_l0, luma_offset_l0,
    // chroma_weight_l0_flag, chroma_weight_l0, chroma_offset_l0, respectively, with l0, list 0, and List0 replaced by l1,
    // list 1, and List1, respectively.
    bool lumaWeightL1Flag[32];
    int16_t lumaWeightL1[32];
    int16_t lumaOffsetL1[32];

    bool chromaWeightL1Flag[32];
    int16_t chromaWeightL1[32][2];
    int16_t chromaOffsetL1[32][2];
} picoH264PredWeightTable_t;
typedef picoH264PredWeightTable_t *picoH264PredWeightTable;

typedef struct {
    // memory_management_control_operation specifies a control operation to be applied to affect the reference picture
    // marking. The memory_management_control_operation syntax element is followed by data necessary for the operation
    // specified by the value of memory_management_control_operation. The values and control operations associated with
    // memory_management_control_operation are specified in Table 7-9. The memory_management_control_operation
    // syntax elements are processed by the decoding process in the order in which they appear in the slice header, and the
    // semantics constraints expressed for each memory_management_control_operation apply at the specific position in that
    // order at which that individual memory_management_control_operation is processed.
    // 0 => End memory_management_control_operation syntax element loop
    // 1 => Mark a short-term reference picture as "unused for reference"
    // 2 => Mark a long-term reference picture as "unused for reference"
    // 3 => Mark a short-term reference picture as "used for long-term reference" and assign a long-term frame index to it
    // 4 => Specify the maximum long-term frame index and mark all long-term reference pictures having long-term frame indices greater than the maximum value as "unused for reference"
    // 5 => Mark all reference pictures as "unused for reference" and set the MaxLongTermFrameIdx variable to "no long-term frame indices"
    // 6 => Mark the current picture as "used for long-term reference" and assign a long-term frame index to it
    uint32_t memoryManagementControlOperation;

    // difference_of_pic_nums_minus1 is used (with memory_management_control_operation equal to 3 or 1) to assign a
    // long-term frame index to a short-term reference picture or to mark a short-term reference picture as "unused for reference".
    // When the associated memory_management_control_operation is processed by the decoding process, the resulting picture
    // number derived from difference_of_pic_nums_minus1 shall be a picture number assigned to one of the reference pictures
    // marked as "used for reference" and not previously assigned to a long-term frame index.
    uint32_t differenceOfPicNumsMinus1;

    // long_term_pic_num is used (with memory_management_control_operation equal to 2) to mark a long-term reference
    // picture as "unused for reference". When the associated memory_management_control_operation is processed by the
    // decoding process, long_term_pic_num shall be equal to a long-term picture number assigned to one of the reference
    // pictures that is currently marked as "used for long-term reference"
    uint32_t longTermPicNum;

    // long_term_frame_idx is used (with memory_management_control_operation equal to 3 or 6) to assign a long-term
    // frame index to a picture. When the associated memory_management_control_operation is processed by the decoding
    // process, the value of long_term_frame_idx shall be in the range of 0 to MaxLongTermFrameIdx, inclusive.
    uint32_t longTermFrameIdx;

    // max_long_term_frame_idx_plus1 minus 1 specifies the maximum value of long-term frame index allowed for
    // long-term reference pictures (until receipt of another value of max_long_term_frame_idx_plus1). The value of
    // max_long_term_frame_idx_plus1 shall be in the range of 0 to max_num_ref_frames, inclusive.
    uint32_t maxLongTermFrameIdxPlus1;
} picoH264MMCOOperation_t;
typedef picoH264MMCOOperation_t *picoH264MMCOOperation;

typedef struct {
    // no_output_of_prior_pics_flag specifies how the previously-decoded pictures in the decoded picture buffer are treated
    // after decoding of an IDR picture. See Annex C. When the IDR picture is the first IDR picture in the bitstream, the value
    // of no_output_of_prior_pics_flag has no effect on the decoding process. When the IDR picture is not the first IDR picture
    // in the bitstream and the value of PicWidthInMbs, FrameHeightInMbs, or max_dec_frame_buffering derived from the
    // active sequence parameter set is different from the value of PicWidthInMbs, FrameHeightInMbs, or
    // max_dec_frame_buffering derived from the sequence parameter set active for the preceding picture,
    // no_output_of_prior_pics_flag equal to 1 may (but should not) be inferred by the decoder, regardless of the actual value
    // of no_output_of_prior_pics_flag.
    bool noOutputOfPriorPicsFlag;

    // long_term_reference_flag equal to 0 specifies that the MaxLongTermFrameIdx variable is set equal to "no long-term
    // frame indices" and that the IDR picture is marked as "used for short-term reference". long_term_reference_flag equal to 1
    // specifies that the MaxLongTermFrameIdx variable is set equal to 0 and that the current IDR picture is marked "used for
    // long-term reference" and is assigned LongTermFrameIdx equal to 0. When max_num_ref_frames is equal to 0,
    // long_term_reference_flag shall be equal to 0.
    bool longTermReferenceFlag;

    // adaptive_ref_pic_marking_mode_flag selects the reference picture marking mode of the currently decoded picture as
    // specified in Table 7-8. adaptive_ref_pic_marking_mode_flag shall be equal to 1 when the number of frames,
    // complementary field pairs, and non-paired fields that are currently marked as "used for long-term reference" is equal to
    // Max( max_num_ref_frames, 1 ).
    bool adaptiveRefPicMarkingModeFlag;

    uint32_t numMMCOOperations;
    picoH264MMCOOperation_t mmcoOperations[128];
} picoH264DecRefPicMarking_t;
typedef picoH264DecRefPicMarking_t *picoH264DecRefPicMarking;

typedef struct {
    // first_mb_in_slice specifies the address of the first macroblock in the slice. When arbitrary slice order is not allowed as
    // specified in Annex A, the value of first_mb_in_slice is constrained as follows:
    //     – If separate_colour_plane_flag is equal to 0, the value of first_mb_in_slice shall not be less than the value of
    //     first_mb_in_slice for any other slice of the current picture that precedes the current slice in decoding order.
    //     – Otherwise (separate_colour_plane_flag is equal to 1), the value of first_mb_in_slice shall not be less than the value
    //     of first_mb_in_slice for any other slice of the current picture that precedes the current slice in decoding order and
    //     has the same value of colour_plane_id.
    // The first macroblock address of the slice is derived as follows:
    //     – If MbaffFrameFlag is equal to 0, first_mb_in_slice is the macroblock address of the first macroblock in the slice,
    //     and first_mb_in_slice shall be in the range of 0 to PicSizeInMbs − 1, inclusive.
    //     – Otherwise (MbaffFrameFlag is equal to 1), first_mb_in_slice * 2 is the macroblock address of the first macroblock
    //     in the slice, which is the top macroblock of the first macroblock pair in the slice, and first_mb_in_slice shall be in
    //     the range of 0 to PicSizeInMbs / 2 − 1, inclusive
    uint32_t firstMbInSlice;

    // When slice_type has a value in the range 5..9, it is a requirement of bitstream conformance that all other slices of the
    // current coded picture shall have a value of slice_type equal to the current value of slice_type or equal to the current value
    // of slice_type minus 5
    picoH264SliceType sliceType;

    // pic_parameter_set_id specifies the picture parameter set in use. The value of pic_parameter_set_id shall be in the range
    // of 0 to 255, inclusive.
    uint8_t picParameterSetId;

    // colour_plane_id specifies the colour plane associated with the current slice RBSP when separate_colour_plane_flag is
    // equal to 1. The value of colour_plane_id shall be in the range of 0 to 2, inclusive. colour_plane_id equal to 0, 1, and 2
    // correspond to the Y, Cb, and Cr planes, respectively.
    uint8_t colourPlaneId;

    // frame_num is used as an identifier for pictures and shall be represented by log2_max_frame_num_minus4 + 4 bits in the bitstream
    uint32_t frameNum;

    // field_pic_flag equal to 1 specifies that the slice is a slice of a coded field. field_pic_flag equal to 0 specifies that the slice
    // is a slice of a coded frame. When field_pic_flag is not present it shall be inferred to be equal to 0.
    bool fieldPicFlag;

    // bottom_field_flag equal to 1 specifies that the slice is part of a coded bottom field. bottom_field_flag equal to 0 specifies
    // that the picture is a coded top field. When this syntax element is not present for the current slice, it shall be inferred to be
    // equal to 0.
    bool bottomFieldFlag;

    // idr_pic_id identifies an IDR picture. The values of idr_pic_id in all the slices of an IDR picture shall remain unchanged.
    // When two consecutive access units in decoding order are both IDR access units, the value of idr_pic_id in the slices of
    // the first such IDR access unit shall differ from the idr_pic_id in the second such IDR access unit. The value of idr_pic_id
    // shall be in the range of 0 to 65535, inclusive
    uint16_t idrPicId;

    // pic_order_cnt_lsb specifies the picture order count modulo MaxPicOrderCntLsb for the top field of a coded frame or
    // for a coded field. The length of the pic_order_cnt_lsb syntax element is log2_max_pic_order_cnt_lsb_minus4 + 4 bits.
    // The value of the pic_order_cnt_lsb shall be in the range of 0 to MaxPicOrderCntLsb − 1, inclusive
    uint32_t picOrderCntLsb;

    // delta_pic_order_cnt_bottom specifies the picture order count difference between the bottom field and the top field of a
    // coded frame as follows:
    //     – If the current picture includes a memory_management_control_operation equal to 5, the value of
    //     delta_pic_order_cnt_bottom shall be in the range of ( 1 − MaxPicOrderCntLsb ) to 2^31 − 1, inclusive.
    //     – Otherwise (the current picture does not include a memory_management_control_operation equal to 5), the value of
    //     delta_pic_order_cnt_bottom shall be in the range of −2^31 + 1 to 2^31 − 1, inclusive.
    //     When this syntax element is not present in the bitstream for the current slice, it shall be inferred to be equal to 0.
    int32_t deltaPicOrderCntBottom;

    // delta_pic_order_cnt[ 0 ] specifies the picture order count difference from the expected picture order count for the top
    // field of a coded frame or for a coded field as specified in clause 8.2.1. The value of delta_pic_order_cnt[ 0 ] shall be in
    // the range of −2^31 + 1 to 2^31 − 1, inclusive. When this syntax element is not present in the bitstream for the current slice, it
    // shall be inferred to be equal to 0.
    // delta_pic_order_cnt[ 1 ] specifies the picture order count difference from the expected picture order count for the bottom
    // field of a coded frame specified in clause 8.2.1. The value of delta_pic_order_cnt[ 1 ] shall be in the range of −2^31 + 1
    // to 2^31 − 1, inclusive. When this syntax element is not present in the bitstream for the current slice, it shall be inferred to
    // be equal to 0.
    int32_t deltaPicOrderCnt[2];

    // redundant_pic_cnt shall be equal to 0 for slices and slice data partitions belonging to the primary coded picture. The
    // value of redundant_pic_cnt shall be greater than 0 for coded slices or coded slice data partitions of a redundant coded
    // picture. When redundant_pic_cnt is not present in the bitstream, its value shall be inferred to be equal to 0. The value of
    // redundant_pic_cnt shall be in the range of 0 to 127, inclusive
    int8_t redundantPicCnt;

    // direct_spatial_mv_pred_flag specifies the method used in the decoding process to derive motion vectors and reference
    // indices for inter prediction as follows:
    //     – If direct_spatial_mv_pred_flag is equal to 1, the derivation process for luma motion vectors for B_Skip,
    //     B_Direct_16x16, and B_Direct_8x8 in clause 8.4.1.2 shall use spatial direct mode prediction as specified in
    //     clause 8.4.1.2.2.
    //     – Otherwise (direct_spatial_mv_pred_flag is equal to 0), the derivation process for luma motion vectors for B_Skip,
    //     B_Direct_16x16, and B_Direct_8x8 in clause 8.4.1.2 shall use temporal direct mode prediction as specified in
    //     clause 8.4.1.2.3
    bool directSpatialMvPredFlag;

    // num_ref_idx_active_override_flag equal to 1 specifies that the syntax element num_ref_idx_l0_active_minus1 is
    // present for P, SP, and B slices and that the syntax element num_ref_idx_l1_active_minus1 is present for B slices.
    // num_ref_idx_active_override_flag equal to 0 specifies that the syntax elements num_ref_idx_l0_active_minus1 and
    // num_ref_idx_l1_active_minus1 are not present.
    // When the current slice is a P, SP, or B slice and field_pic_flag is equal to 0 and the value of
    // num_ref_idx_l0_default_active_minus1 in the picture parameter set exceeds 15, num_ref_idx_active_override_flag shall
    // be equal to 1.
    // When the current slice is a B slice and field_pic_flag is equal to 0 and the value of
    // num_ref_idx_l1_default_active_minus1 in the picture parameter set exceeds 15, num_ref_idx_active_override_flag shall
    // be equal to 1.
    bool numRefIdxActiveOverrideFlag;

    // num_ref_idx_l0_active_minus1 specifies the maximum reference index for reference picture list 0 that shall be used to
    // decode the slice.
    // When the current slice is a P, SP, or B slice and num_ref_idx_l0_active_minus1 is not present,
    // num_ref_idx_l0_active_minus1 shall be inferred to be equal to num_ref_idx_l0_default_active_minus1.
    // The range of num_ref_idx_l0_active_minus1 is specified as follows:
    //     – If field_pic_flag is equal to 0, num_ref_idx_l0_active_minus1 shall be in the range of 0 to 15, inclusive. When
    //     MbaffFrameFlag is equal to 1, num_ref_idx_l0_active_minus1 is the maximum index value for the decoding of frame
    //     macroblocks and 2 * num_ref_idx_l0_active_minus1 + 1 is the maximum index value for the decoding of field
    //     macroblocks.
    //     – Otherwise (field_pic_flag is equal to 1), num_ref_idx_l0_active_minus1 shall be in the range of 0 to 31, inclusive.
    uint32_t numRefIdxL0ActiveMinus1;

    // num_ref_idx_l1_active_minus1 specifies the maximum reference index for reference picture list 1 that shall be used to
    // decode the slice.
    // When the current slice is a B slice and num_ref_idx_l1_active_minus1 is not present, num_ref_idx_l1_active_minus1
    // shall be inferred to be equal to num_ref_idx_l1_default_active_minus1
    uint32_t numRefIdxL1ActiveMinus1;

    picoH264RefPicListModification_t refPicListMvcModification;

    picoH264RefPicListModification_t refPicListModification;

    picoH264PredWeightTable_t predWeightTable;

    picoH264DecRefPicMarking_t decRefPicMarking;

    // cabac_init_idc specifies the index for determining the initialization table used in the initialization process for context
    // variables. The value of cabac_init_idc shall be in the range of 0 to 2, inclusive.
    uint32_t cabacInitIdc;

    // slice_qp_delta specifies the initial value of QPY to be used for all the macroblocks in the slice until modified by the value
    // of mb_qp_delta in the macroblock layer. The initial QPY quantization parameter for the slice is computed as
    // SliceQPY = 26 + pic_init_qp_minus26 + slice_qp_delta
    // The value of slice_qp_delta shall be limited such that SliceQPY is in the range of −QpBdOffsetY to +51, inclusive.
    int8_t sliceQpDelta;

    // sp_for_switch_flag specifies the decoding process to be used to decode P macroblocks in an SP slice as follows:
    //     – If sp_for_switch_flag is equal to 0, the P macroblocks in the SP slice shall be decoded using the SP decoding process
    //     for non-switching pictures as specified in clause 8.6.1.
    //     – Otherwise (sp_for_switch_flag is equal to 1), the P macroblocks in the SP slice shall be decoded using the SP and SI
    //     decoding process for switching pictures as specified in clause 8.6.2.
    bool spForSwitchFlag;

    // slice_qs_delta specifies the value of QSY for all the macroblocks in SP and SI slices. The QSY quantization parameter
    // for the slice is computed as
    //     QSY = 26 + pic_init_qs_minus26 + slice_qs_delta (7-33)
    // The value of slice_qs_delta shall be limited such that QSY is in the range of 0 to 51, inclusive. This value of QSY is used
    // for the decoding of all macroblocks in SI slices with mb_type equal to SI and all macroblocks in SP slices that are coded
    // in an Inter macroblock prediction mode.
    int8_t sliceQsDelta;

    // disable_deblocking_filter_idc specifies whether the operation of the deblocking filter shall be disabled across some
    // block edges of the slice and specifies for which edges the filtering is disabled. When disable_deblocking_filter_idc is not
    // present in the slice header, the value of disable_deblocking_filter_idc shall be inferred to be equal to 0.
    // The value of disable_deblocking_filter_idc shall be in the range of 0 to 2, inclusive.
    int8_t disableDeblockingFilterIdc;

    // slice_alpha_c0_offset_div2 specifies the offset used in accessing the α and tC0 deblocking filter tables for filtering
    // operations controlled by the macroblocks within the slice. From this value, the offset that shall be applied when addressing
    // these tables shall be computed as
    //     FilterOffsetA = slice_alpha_c0_offset_div2 << 1
    // The value of slice_alpha_c0_offset_div2 shall be in the range of −6 to +6, inclusive. When slice_alpha_c0_offset_div2 is
    // not present in the slice header, the value of slice_alpha_c0_offset_div2 shall be inferred to be equal to 0.
    int32_t sliceAlphaC0OffsetDiv2;

    // slice_beta_offset_div2 specifies the offset used in accessing the β deblocking filter table for filtering operations
    // controlled by the macroblocks within the slice. From this value, the offset that is applied when addressing the β table of
    // the deblocking filter shall be computed as
    //     FilterOffsetB = slice_beta_offset_div2 << 1
    // The value of slice_beta_offset_div2 shall be in the range of −6 to +6, inclusive. When slice_beta_offset_div2 is not
    // present in the slice header the value of slice_beta_offset_div2 shall be inferred to be equal to 0.
    int32_t sliceBetaOffsetDiv2;

    // slice_group_change_cycle is used to derive the number of slice group map units in slice group 0 when
    // slice_group_map_type is equal to 3, 4, or 5, as specified by
    //     MapUnitsInSliceGroup0 = Min( slice_group_change_cycle * SliceGroupChangeRate, PicSizeInMapUnits )
    // The value of slice_group_change_cycle is represented in the bitstream by the following number of bits
    //     Ceil( Log2( PicSizeInMapUnits ÷ SliceGroupChangeRate + 1 ) )
    // The value of slice_group_change_cycle shall be in the range of 0 to Ceil( PicSizeInMapUnits÷SliceGroupChangeRate ),
    // inclusive.
    uint32_t sliceGroupChangeCycle;
} picoH264SliceHeader_t;
typedef picoH264SliceHeader_t *picoH264SliceHeader;

typedef struct {
    // NOTE: we do not currently parse slice data
    //       as its not needed for the primary use case of
    //       this library (vulkan video / directx video decoding)
    // TODO: This is something that will probably be implemented in the future
    //       pull requests are welcome :)
    int dummy;
} picoH264SliceData_t;
typedef picoH264SliceData_t *picoH264SliceData;

typedef struct {
    picoH264SliceHeader_t header;
    picoH264SliceData_t data;
} picoH264SliceLayerWithoutPartitioning_t;
typedef picoH264SliceLayerWithoutPartitioning_t *picoH264SliceLayerWithoutPartitioning;

typedef struct {
    picoH264SliceHeader_t header;

    // slice_id identifies the slice associated with the slice data partition. The value of slice_id is constrained as follows:
    //     – If separate_colour_plane_flag is equal to 0, the following applies:
    //         – If arbitrary slice order is not allowed as specified in Annex A, the first slice of a coded picture, in decoding
    //         order, shall have slice_id equal to 0 and the value of slice_id shall be incremented by one for each subsequent
    //         slice of the coded picture in decoding order.
    //         – Otherwise (arbitrary slice order is allowed), each slice shall have a unique slice_id value within the set of slices
    //         of the coded picture.
    //     – Otherwise (separate_colour_plane_flag is equal to 1), the following applies:
    //         – If arbitrary slice order is not allowed as specified in Annex A, the first slice of a coded picture having each
    //         value of colour_plane_id, in decoding order, shall have slice_id equal to 0 and the value of slice_id shall be
    //         incremented by one for each subsequent slice of the coded picture having the same value of colour_plane_id,
    //         in decoding order.
    //     – Otherwise (arbitrary slice order is allowed) each slice shall have a unique slice_id value within each set of slices
    //     of the coded picture that have the same value of colour_plane_id.
    //     The range of slice_id is specified as follows:
    //         – If MbaffFrameFlag is equal to 0, slice_id shall be in the range of 0 to PicSizeInMbs − 1, inclusive.
    //         – Otherwise (MbaffFrameFlag is equal to 1), slice_id shall be in the range of 0 to PicSizeInMbs / 2 − 1, inclusive.
    uint16_t sliceId;
    picoH264SliceData_t data;
} picoH264SliceDataPartitionALayer_t;
typedef picoH264SliceDataPartitionALayer_t *picoH264SliceDataPartitionALayer;

typedef struct {
    // slice_id has the same semantics as specified for slice_data_partition_a_layer( ) syntax structure.
    uint16_t sliceId;

    // colour_plane_id specifies the colour plane associated with the current slice RBSP when separate_colour_plane_flag is
    // equal to 1. The value of colour_plane_id shall be in the range of 0 to 2, inclusive. colour_plane_id equal to 0, 1, and 2
    // correspond to the Y, Cb, and Cr planes, respectively.
    uint16_t colorPlaneId;

    // redundant_pic_cnt shall be equal to 0 for coded slices and coded slice data partitions belonging to the primary coded
    // picture. The redundant_pic_cnt shall be greater than 0 for coded slices and coded slice data partitions in redundant coded
    // pictures. When redundant_pic_cnt is not present, its value shall be inferred to be equal to 0. The value of
    // redundant_pic_cnt shall be in the range of 0 to 127, inclusive.
    // The presence of a slice data partition B RBSP is specified as follows:
    //     – If the syntax elements of a slice data partition A RBSP indicate the presence of any syntax elements of category 3 in
    //     the slice data for a slice, a slice data partition B RBSP shall be present having the same value of slice_id and
    //     redundant_pic_cnt as in the slice data partition A RBSP.
    //     – Otherwise (the syntax elements of a slice data partition A RBSP do not indicate the presence of any syntax elements
    //     of category 3 in the slice data for a slice), no slice data partition B RBSP shall be present having the same value of
    //     slice_id and redundant_pic_cnt as in the slice data partition A RBSP.
    uint16_t redundantPicCnt;

    picoH264SliceData_t data;
} picoH264SliceDataPartitionBLayer_t;
typedef picoH264SliceDataPartitionBLayer_t *picoH264SliceDataPartitionBLayer;

typedef struct {
    // slice_id has the same semantics as specified for slice_data_partition_a_layer( ) syntax structure.
    uint16_t sliceId;

    // colour_plane_id has the same semantics as specified for slice_data_partition_b_layer( ) syntax structure.
    uint16_t colorPlaneId;

    // redundant_pic_cnt has the same semantics as specified for slice_data_partition_b_layer( ) syntax structure.
    uint16_t redundantPicCnt;

    picoH264SliceData_t data;
} picoH264SliceDataPartitionCLayer_t;
typedef picoH264SliceDataPartitionCLayer_t *picoH264SliceDataPartitionCLayer;

picoH264Bitstream picoH264BitstreamFromBuffer(const uint8_t *buffer, size_t size);
void picoH264BitstreamDestroy(picoH264Bitstream bitstream);

void picoH264BufferReaderInit(picoH264BufferReader bufferReader, const uint8_t *buffer, size_t size);
// these functions are based of whats mentioned and repeatedly used in the H264 spec

// byte_aligned( ) is specified as follows:
//     – If the current position in the bitstream is on a byte boundary, i.e., the next bit in the bitstream is the first bit in a
//     byte, the return value of byte_aligned( ) is equal to TRUE.
//     – Otherwise, the return value of byte_aligned( ) is equal to FALSE
bool picoH264BufferReaderByteAligned(picoH264BufferReader bufferReader);

// more_data_in_byte_stream( ), which is used only in the byte stream NAL unit syntax structure specified in Annex B, is
// specified as follows:
//     – If more data follow in the byte stream, the return value of more_data_in_byte_stream( ) is equal to TRUE.
//     – Otherwise, the return value of more_data_in_byte_stream( ) is equal to FALSE
bool picoH264BufferReaderMoreDataInByteStream(picoH264BufferReader bufferReader);

// rbsp_trailing_bits( ) is specified as follows:
// rbsp_trailing_bits( ) { 
//      rbsp_stop_one_bit /* equal to 1 */
//      while( !byte_aligned( ) )
//      rbsp_alignment_zero_bit /* equal to 0 */
// }
void picoH264BufferReaderRBSPTrailingBits(picoH264BufferReader bufferReader);

// more_rbsp_data( ) is specified as follows:
//     – If there is no more data in the RBSP, the return value of more_rbsp_data( ) is equal to FALSE.
//     – Otherwise, the RBSP data is searched for the last (least significant, right-most) bit equal to 1 that is present in
//     the RBSP. Given the position of this bit, which is the first bit (rbsp_stop_one_bit) of the rbsp_trailing_bits( )
//     syntax structure, the following applies:
//     – If there is more data in an RBSP before the rbsp_trailing_bits( ) syntax structure, the return value of
//     more_rbsp_data( ) is equal to TRUE.
//     – Otherwise, the return value of more_rbsp_data( ) is equal to FALSE.
//     The method for enabling determination of whether there is more data in the RBSP is specified by the application (or
//     in Annex B for applications that use the byte stream format)
bool picoH264BufferReaderMoreRBSPData(picoH264BufferReader bufferReader);

// more_rbsp_trailing_data( ) is specified as follows:
//     – If there is more data in an RBSP, the return value of more_rbsp_trailing_data( ) is equal to TRUE.
//     – Otherwise, the return value of more_rbsp_trailing_data( ) is equal to FALSE.
bool picoH264BufferReaderMoreRBSPTrailingData(picoH264BufferReader bufferReader);

// next_bits( n ) provides the next bits in the bitstream for comparison purposes, without advancing the bitstream pointer.
// When used within the byte stream as specified in Annex B, next_bits( n ) returns a value of 0 if fewer than n bits remain
// within the byte stream.
// n shall be in the range of 0 to 64, inclusive.
uint64_t picoH264BufferReaderNextBits(picoH264BufferReader bufferReader, uint32_t n);

// read_bits( n ) reads the next n bits from the bitstream and advances the bitstream pointer by n bit positions. When n is
// equal to 0, read_bits( n ) is specified to return a value equal to 0 and to not advance the bitstream pointer.
// n shall be in the range of 0 to 64, inclusive.
uint64_t picoH264BufferReaderReadBits(picoH264BufferReader bufferReader, uint32_t n);

// ae(v): context-adaptive arithmetic entropy-coded syntax element. The parsing process for this descriptor is
// specified in clause 9.3
uint64_t picoH264BufferReaderAR(picoH264BufferReader bufferReader);

// b(8): byte having any pattern of bit string (8 bits). The parsing process for this descriptor is specified by the
// return value of the function read_bits( 8 )
uint8_t picoH264BufferReaderB(picoH264BufferReader bufferReader);

// ce(v): context-adaptive variable-length entropy-coded syntax element with the left bit first. The parsing process
// for this descriptor is specified in clause 9.2.
uint64_t picoH264BufferReaderCE(picoH264BufferReader bufferReader);

// f(n): fixed-pattern bit string using n bits written (from left to right) with the left bit first. The parsing process for
// this descriptor is specified by the return value of the function read_bits( n ).
uint64_t picoH264BufferReaderF(picoH264BufferReader bufferReader, uint32_t n);

// i(n): signed integer using n bits. When n is "v" in the syntax table, the number of bits varies in a manner dependent
// on the value of other syntax elements. The parsing process for this descriptor is specified by the return value of
// the function read_bits( n ) interpreted as a two's complement integer representation with most significant bit
// written first.
int64_t picoH264BufferReaderI(picoH264BufferReader bufferReader, uint32_t n);

// me(v): mapped Exp-Golomb-coded syntax element with the left bit first. The parsing process for this descriptor
// is specified in clause 9.1.
uint64_t picoH264BufferReaderME(picoH264BufferReader bufferReader);

// se(v): signed integer Exp-Golomb-coded syntax element with the left bit first. The parsing process for this
// descriptor is specified in clause 9.1.
int64_t picoH264BufferReaderSE(picoH264BufferReader bufferReader);

// te(v): truncated Exp-Golomb-coded syntax element with left bit first. The parsing process for this descriptor is
// specified in clause 9.1.
uint64_t picoH264BufferReaderTE(picoH264BufferReader bufferReader, uint32_t range);

// u(n): unsigned integer using n bits. When n is "v" in the syntax table, the number of bits varies in a manner
// dependent on the value of other syntax elements. The parsing process for this descriptor is specified by the return
// value of the function read_bits( n ) interpreted as a binary representation of an unsigned integer with most
// significant bit written first.
uint64_t picoH264BufferReaderU(picoH264BufferReader bufferReader, uint32_t n);

// ue(v): unsigned integer Exp-Golomb-coded syntax element with the left bit first. The parsing process for this
// descriptor is specified in clause 9.1
uint64_t picoH264BufferReaderUE(picoH264BufferReader bufferReader);

// find the next NAL unit in the bitstream, returns true if found, false if not found or error, forward the bitstream position to the start of the NAL unit
// NOTE: it move the cursor to the start of the start code prefix of the NAL unit (0x000001 or 0x00000001), to read the NAL unit itself, use picoH264ReadNALUnit after this function
bool picoH264FindNextNALUnit(picoH264Bitstream bitstream, size_t *nalUnitSizeOut);

// read the NAL unit from the bitstream into the provided buffer, returns true if successful, false if error (e.g. buffer too small)
bool picoH264ReadNALUnit(picoH264Bitstream bitstream, uint8_t *nalUnitBuffer, size_t nalUnitBufferSize, size_t nalUnitSizeOut);

// parse the NAL unit header and extract the NAL unit payload, returns true if successful, false if error (e.g. invalid NAL unit)
// it also expects to find the start code prefix at the start of nalUnitBuffer (0x000001 or 0x00000001)
bool picoH264ParseNALUnit(const uint8_t *nalUnitBuffer, size_t nalUnitSize, picoH264NALUnitHeader nalUnitHeaderOut, uint8_t *nalPayloadOut, size_t *nalPayloadSizeOut);

// the following functions print various structures to stdout for debugging purposes
void picoH264NALUnitHeaderDebugPrint(picoH264NALUnitHeader nalUnitHeader);
void picoH264SequenceParameterSetDebugPrint(picoH264SequenceParameterSet sps);
void picoH264SequenceParameterSetExtensionDebugPrint(picoH264SequenceParameterSetExtension spsExt);
void picoH264SubsetSequenceParameterSetDebugPrint(picoH264SubsetSequenceParameterSet spsSubset);
void picoH264PictureParameterSetDebugPrint(picoH264PictureParameterSet pps);
void picoH264SEIMessageDebugPrint(picoH264SEIMessage seiMessage);
void picoH264AccessUnitDelimiterDebugPrint(picoH264AccessUnitDelimiter aud);
void picoH264SliceHeaderDebugPrint(picoH264SliceHeader sliceHeader);
void picoH264SliceDataDebugPrint(picoH264SliceData sliceData);
void picoH264SliceLayerWithoutPartitioningDebugPrint(picoH264SliceLayerWithoutPartitioning sliceLayer);
void picoH264SliceDataPartitionALayerDebugPrint(picoH264SliceDataPartitionALayer sliceDataPartitionALayer);
void picoH264SliceDataPartitionBLayerDebugPrint(picoH264SliceDataPartitionBLayer sliceDataPartitionBLayer);
void picoH264SliceDataPartitionCLayerDebugPrint(picoH264SliceDataPartitionCLayer sliceDataPartitionCLayer);

// the following functions return string representations of various enums for debugging purposes
const char *picoH264NALRefIdcToString(picoH264NALRefIDC nalRefIdc);
const char *picoH264NALUnitTypeToString(picoH264NALUnitType nalUnitType);
const char *picoH264AspectRatioIDCToString(uint8_t idc);
const char *picoH264ProfileIdcToString(uint8_t profileIdc);
const char *picoH264VideoFormatToString(picoH264VideoFormat videoFormat);
const char *picoH264SEIMessageTypeToString(picoH264SEIMessageType seiMessageType);
const char *picoH264SliceTypeToString(picoH264SliceType sliceType);

// for testing purposes only
#define PICO_IMPLEMENTATION

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

const char *picoH264SliceTypeToString(picoH264SliceType sliceType)
{
    switch (sliceType) {
        case PICO_H264_SLICE_TYPE_P:
            return "P Slice";
        case PICO_H264_SLICE_TYPE_B:
            return "B Slice";
        case PICO_H264_SLICE_TYPE_I:
            return "I Slice";
        case PICO_H264_SLICE_TYPE_SP:
            return "SP Slice";
        case PICO_H264_SLICE_TYPE_SI:
            return "SI Slice";
        case PICO_H264_SLICE_TYPE_P_ONLY:
            return "P Only Slice";
        case PICO_H264_SLICE_TYPE_B_ONLY:
            return "B Only Slice";
        case PICO_H264_SLICE_TYPE_I_ONLY:
            return "I Only Slice";
        case PICO_H264_SLICE_TYPE_SP_ONLY:
            return "SP Only Slice";
        case PICO_H264_SLICE_TYPE_SI_ONLY:
            return "SI Only Slice";
        default:
            return "Unknown Slice Type";
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

void picoH264BufferReaderInit(picoH264BufferReader bufferReader, const uint8_t *buffer, size_t size)
{
    PICO_ASSERT(bufferReader != NULL);
    bufferReader->buffer      = buffer;
    bufferReader->size        = size;
    bufferReader->position    = 0;
    bufferReader->bitPosition = 0;
}

bool picoH264BufferReaderByteAligned(picoH264BufferReader bufferReader)
{
    PICO_ASSERT(bufferReader != NULL);
    return bufferReader->bitPosition == 0;
}

void picoH264BufferReaderRBSPTrailingBits(picoH264BufferReader bufferReader)
{
    PICO_ASSERT(bufferReader != NULL);

    uint8_t stopBit = (uint8_t)picoH264BufferReaderReadBits(bufferReader, 1);
    PICO_ASSERT(stopBit == 1);

    while (!picoH264BufferReaderByteAligned(bufferReader)) {
        uint8_t alignBit = (uint8_t)picoH264BufferReaderReadBits(bufferReader, 1);
        PICO_ASSERT(alignBit == 0);
    }
}

bool picoH264BufferReaderMoreDataInByteStream(picoH264BufferReader bufferReader)
{
    PICO_ASSERT(bufferReader != NULL);
    if (bufferReader->position < bufferReader->size) {
        return true;
    }
    return false;
}

bool picoH264BufferReaderMoreRBSPData(picoH264BufferReader bufferReader)
{
    PICO_ASSERT(bufferReader != NULL);

    

    return false;
}

bool picoH264BufferReaderMoreRBSPTrailingData(picoH264BufferReader bufferReader)
{
    PICO_ASSERT(bufferReader != NULL);
    size_t totalBitsRemaining = (bufferReader->size - bufferReader->position) * 8 - bufferReader->bitPosition;
    return totalBitsRemaining > 0;
}

uint64_t picoH264BufferReaderNextBits(picoH264BufferReader bufferReader, uint32_t n)
{
    PICO_ASSERT(bufferReader != NULL);
    PICO_ASSERT(n <= 64);

    if (n == 0) {
        return 0;
    }

    size_t savedPosition    = bufferReader->position;
    size_t savedBitPosition = bufferReader->bitPosition;

    size_t totalBitsRemaining = (bufferReader->size - bufferReader->position) * 8 - bufferReader->bitPosition;
    if (n > totalBitsRemaining) {
        return 0;
    }

    uint64_t result = picoH264BufferReaderReadBits(bufferReader, n);

    bufferReader->position    = savedPosition;
    bufferReader->bitPosition = savedBitPosition;

    return result;
}

uint64_t picoH264BufferReaderReadBits(picoH264BufferReader bufferReader, uint32_t n)
{
    PICO_ASSERT(bufferReader != NULL);
    PICO_ASSERT(n <= 64);

    if (n == 0) {
        return 0;
    }

    uint64_t result = 0;
    for (uint32_t i = 0; i < n; i++) {
        if (bufferReader->position >= bufferReader->size) {
            PICO_H264_LOG("picoH264BufferReaderReadBits: Attempted to read past end of buffer\n");
            break;
        }

        uint8_t currentByte = bufferReader->buffer[bufferReader->position];
        uint8_t bit         = (currentByte >> (7 - bufferReader->bitPosition)) & 1;
        result              = (result << 1) | bit;

        bufferReader->bitPosition++;
        if (bufferReader->bitPosition >= 8) {
            bufferReader->bitPosition = 0;
            bufferReader->position++;
        }
    }

    return result;
}

uint64_t picoH264BufferReaderAR(picoH264BufferReader bufferReader)
{
    PICO_ASSERT(bufferReader != NULL);
    PICO_H264_LOG("picoH264BufferReaderAR: CABAC parsing not implemented yet in buffer reader\n");
    return 0;
}

uint8_t picoH264BufferReaderB(picoH264BufferReader bufferReader)
{
    PICO_ASSERT(bufferReader != NULL);
    return (uint8_t)picoH264BufferReaderReadBits(bufferReader, 8);
}

uint64_t picoH264BufferReaderCE(picoH264BufferReader bufferReader)
{
    PICO_ASSERT(bufferReader != NULL);
    PICO_H264_LOG("picoH264BufferReaderCE: CAVLC parsing not implemented yet in buffer reader\n");
    return 0;
}

uint64_t picoH264BufferReaderF(picoH264BufferReader bufferReader, uint32_t n)
{
    PICO_ASSERT(bufferReader != NULL);
    return picoH264BufferReaderReadBits(bufferReader, n);
}

int64_t picoH264BufferReaderI(picoH264BufferReader bufferReader, uint32_t n)
{
    PICO_ASSERT(bufferReader != NULL);
    PICO_ASSERT(n <= 64);

    if (n == 0) {
        return 0;
    }

    uint64_t unsignedValue = picoH264BufferReaderReadBits(bufferReader, n);
    if (unsignedValue & (1ULL << (n - 1))) {
        return (int64_t)(unsignedValue | (~0ULL << n));
    } 
    
    return (int64_t)unsignedValue;
}

uint64_t picoH264BufferReaderME(picoH264BufferReader bufferReader)
{
    PICO_ASSERT(bufferReader != NULL);
    // NOTE: the exact use of this will depend on the mapping used, here we just read it as UE
    //       and the caller can apply the mapping as needed.
    return picoH264BufferReaderUE(bufferReader);
}

int64_t picoH264BufferReaderSE(picoH264BufferReader bufferReader)
{
    PICO_ASSERT(bufferReader != NULL);
    uint64_t codeNum = picoH264BufferReaderUE(bufferReader);

    switch (codeNum) {
        case 0: return 0;
        case 1: return 1;
        case 2: return -1;
        case 3: return 2;
        case 4: return -2;
        case 5: return 3;
        case 6: return -3;
        default: {
            int64_t value = (codeNum + 1) / 2;
            if (((codeNum + 1) & 1) == 0) {
                value = -value;
            }
            return value;
        }
    }
}

uint64_t picoH264BufferReaderTE(picoH264BufferReader bufferReader, uint32_t range)
{
    PICO_ASSERT(bufferReader != NULL);
    if (range == 1) {
        uint64_t bit = picoH264BufferReaderReadBits(bufferReader, 1);
        return 1 - bit;
    }

    return picoH264BufferReaderUE(bufferReader);
}

uint64_t picoH264BufferReaderU(picoH264BufferReader bufferReader, uint32_t n)
{
    PICO_ASSERT(bufferReader != NULL);
    return picoH264BufferReaderReadBits(bufferReader, n);
}

uint64_t picoH264BufferReaderUE(picoH264BufferReader bufferReader)
{
    PICO_ASSERT(bufferReader != NULL);
    int leadingZeroBits = 0;
    while (picoH264BufferReaderReadBits(bufferReader, 1) == 0) {
        leadingZeroBits++;
        if (leadingZeroBits > 31) {
            PICO_H264_LOG("picoH264BufferReaderUE: Too many leading zero bits\n");
            return 0;
        }
    }
    if (leadingZeroBits == 0) {
        return 0;
    }
    uint64_t suffix = picoH264BufferReaderReadBits(bufferReader, (uint32_t)leadingZeroBits);
    return ((1ULL << leadingZeroBits) - 1) + suffix;
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

    picoH264BufferReader_t br = {0};
    picoH264BufferReaderInit(&br, nalUnitBuffer, nalUnitSize);

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
    nalUnitHeaderOut->nalRefIDC = (picoH264NALRefIDC)((firstNALByte >> 5) & 0x03);
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

const char *picoH264NALRefIdcToString(picoH264NALRefIDC nalRefIdc)
{
    switch (nalRefIdc) {
        case PICO_H264_NAL_REF_IDC_DISPOSABLE:
            return "Disposable";
        case PICO_H264_NAL_REF_IDC_LOW:
            return "Low";
        case PICO_H264_NAL_REF_IDC_HIGH:
            return "High";
        case PICO_H264_NAL_REF_IDC_HIGHEST:
            return "Highest";
        default:
            return "Unknown NAL ref IDC";
    }
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

void picoH264SequenceParameterSetExtensionDebugPrint(picoH264SequenceParameterSetExtension spsExt)
{
    PICO_ASSERT(spsExt != NULL);

    PICO_H264_LOG("Sequence Parameter Set Extension:\n");
    PICO_H264_LOG("  seqParameterSetId: %u\n", (unsigned)spsExt->seqParameterSetId);
    PICO_H264_LOG("  auxFormatIdc: %u\n", (unsigned)spsExt->auxFormatIdc);
    if (spsExt->auxFormatIdc != 0) {
        PICO_H264_LOG("  bitDepthAuxMinus8: %u\n", (unsigned)spsExt->bitDepthAuxMinus8);
        PICO_H264_LOG("  alphaIncrFlag: %s\n", spsExt->alphaIncrFlag ? "true" : "false");
        PICO_H264_LOG("  alphaOpaqueValue: %u\n", (unsigned)spsExt->alphaOpaqueValue);
        PICO_H264_LOG("  alphaTransparentValue: %u\n", (unsigned)spsExt->alphaTransparentValue);
    }
    PICO_H264_LOG("  additionalExtensionFlag: %s\n", spsExt->additionalExtensionFlag ? "true" : "false");
}

void picoH264NALUnitHeaderDebugPrint(picoH264NALUnitHeader nalUnitHeader)
{
    PICO_ASSERT(nalUnitHeader != NULL);

    PICO_H264_LOG("NAL Unit Header:\n");
    PICO_H264_LOG("  nalRefIDC: %s (%u)\n", picoH264NALRefIdcToString(nalUnitHeader->nalRefIDC), (unsigned)nalUnitHeader->nalRefIDC);
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

static void __picoH264SPSSVCExtensionDebugPrint(const picoH264SPSSVCExtension_t *svcExt)
{
    if (!svcExt)
        return;

    PICO_H264_LOG("  SPS SVC Extension:\n");
    PICO_H264_LOG("    interLayerDeblockingFilterControlPresentFlag: %s\n", svcExt->interLayerDeblockingFilterControlPresentFlag ? "true" : "false");
    PICO_H264_LOG("    extendedSpatialScalabilityIDC: %u\n", (unsigned)svcExt->extendedSpatialScalabilityIDC);
    PICO_H264_LOG("    chromaPhaseXPlus1Flag: %s\n", svcExt->chromaPhaseXPlus1Flag ? "true" : "false");
    PICO_H264_LOG("    chromaPhaseYPlus1: %u\n", (unsigned)svcExt->chromaPhaseYPlus1);
    PICO_H264_LOG("    seqRefLayerChromaPhaseXPlus1Flag: %s\n", svcExt->seqRefLayerChromaPhaseXPlus1Flag ? "true" : "false");
    PICO_H264_LOG("    seqRefLayerChromaPhaseYPlus1: %u\n", (unsigned)svcExt->seqRefLayerChromaPhaseYPlus1);
    PICO_H264_LOG("    seqScaledRefLayerLeftOffset: %d\n", (int)svcExt->seqScaledRefLayerLeftOffset);
    PICO_H264_LOG("    seqScaledRefLayerTopOffset: %d\n", (int)svcExt->seqScaledRefLayerTopOffset);
    PICO_H264_LOG("    seqScaledRefLayerRightOffset: %d\n", (int)svcExt->seqScaledRefLayerRightOffset);
    PICO_H264_LOG("    seqScaledRefLayerBottomOffset: %d\n", (int)svcExt->seqScaledRefLayerBottomOffset);
    PICO_H264_LOG("    seqTcoeffLevelPredictionFlag: %s\n", svcExt->seqTcoeffLevelPredictionFlag ? "true" : "false");
    PICO_H264_LOG("    adaptiveTcoeffLevelPredictionFlag: %s\n", svcExt->adaptiveTcoeffLevelPredictionFlag ? "true" : "false");
    PICO_H264_LOG("    sliceHeaderRestrictionFlag: %s\n", svcExt->sliceHeaderRestrictionFlag ? "true" : "false");
}

static void __picoH264SVCVUIParametersExtensionDebugPrint(const picoH264SVCVUIParametersExtension_t *svcVui)
{
    if (!svcVui)
        return;

    PICO_H264_LOG("  SVC VUI Parameters Extension:\n");
    PICO_H264_LOG("    vuiExtNumEntriesMinus1: %u\n", (unsigned)svcVui->vuiExtNumEntriesMinus1);

    for (uint32_t i = 0; i < svcVui->vuiExtNumEntriesMinus1 && i < 1024; i++) {
        const picoH264SVCVUIParametersExtensionEntry_t *entry = &svcVui->vuiExtEntries[i];
        PICO_H264_LOG("    Entry[%u]:\n", (unsigned)i);
        PICO_H264_LOG("      vuiExtDependencyId: %u\n", (unsigned)entry->vuiExtDependencyId);
        PICO_H264_LOG("      vuiExtQualityId: %u\n", (unsigned)entry->vuiExtQualityId);
        PICO_H264_LOG("      vuiExtTemporalId: %u\n", (unsigned)entry->vuiExtTemporalId);
        PICO_H264_LOG("      vuiExtTimingInfoPresentFlag: %s\n", entry->vuiExtTimingInfoPresentFlag ? "true" : "false");

        if (entry->vuiExtTimingInfoPresentFlag) {
            PICO_H264_LOG("      vuiExtNumUnitsInTick: %u\n", (unsigned)entry->vuiExtNumUnitsInTick);
            PICO_H264_LOG("      vuiExtTimeScale: %u\n", (unsigned)entry->vuiExtTimeScale);
            PICO_H264_LOG("      vuiExtFixedFrameRateFlag: %s\n", entry->vuiExtFixedFrameRateFlag ? "true" : "false");
        }

        PICO_H264_LOG("      vuiExtNalHrdParametersPresentFlag: %s\n", entry->vuiExtNalHrdParametersPresentFlag ? "true" : "false");
        PICO_H264_LOG("      vuiExtVclHrdParametersPresentFlag: %s\n", entry->vuiExtVclHrdParametersPresentFlag ? "true" : "false");

        PICO_H264_LOG("      vuiExtLowDelayHrdFlag: %s\n", entry->vuiExtLowDelayHrdFlag ? "true" : "false");

        PICO_H264_LOG("      vuiExtPicStructPresentFlag: %s\n", entry->vuiExtPicStructPresentFlag ? "true" : "false");
    }
}

static void __picoH264SPSMVCExtensionDebugPrint(const picoH264SPSMVCExtension_t *mvcExt)
{
    if (!mvcExt)
        return;

    PICO_H264_LOG("  SPS MVC Extension:\n");
    PICO_H264_LOG("    numViewsMinus1: %u\n", (unsigned)mvcExt->numViewsMinus1);

    for (uint16_t i = 0; i < mvcExt->numViewsMinus1 && i < 1024; i++) {
        PICO_H264_LOG("    viewId[%u]: %u\n", (unsigned)i, (unsigned)mvcExt->viewId[i]);
    }

    PICO_H264_LOG("    Anchor References (showing first 5 views):\n");
    for (uint16_t i = 0; i < mvcExt->numViewsMinus1 && i < 5; i++) {
        PICO_H264_LOG("      View[%u] L0: count=%u\n", (unsigned)i, (unsigned)mvcExt->numAnchorRefsL0[i]);
        PICO_H264_LOG("      View[%u] L1: count=%u\n", (unsigned)i, (unsigned)mvcExt->numAnchorRefsL1[i]);
    }

    PICO_H264_LOG("    Non-Anchor References (showing first 5 views):\n");
    for (uint16_t i = 0; i < mvcExt->numViewsMinus1 && i < 5; i++) {
        PICO_H264_LOG("      View[%u] L0: count=%u\n", (unsigned)i, (unsigned)mvcExt->numNonAnchorRefsL0[i]);
        PICO_H264_LOG("      View[%u] L1: count=%u\n", (unsigned)i, (unsigned)mvcExt->numNonAnchorRefsL1[i]);
    }

    PICO_H264_LOG("    numLevelValuesSignalledMinus1: %u\n", (unsigned)mvcExt->numLevelValuesSignalledMinus1);

    for (uint8_t i = 0; i < mvcExt->numLevelValuesSignalledMinus1 && i < 64; i++) {
        PICO_H264_LOG("    Level[%u]:\n", (unsigned)i);
        PICO_H264_LOG("      levelIDC: %u\n", (unsigned)mvcExt->levelIDC[i]);
        PICO_H264_LOG("      numApplicableOpsMinus1: %u\n", (unsigned)mvcExt->numApplicableOpsMinus1[i]);
    }

    PICO_H264_LOG("    mfcFormatIDC: %u\n", (unsigned)mvcExt->mfcFormatIDC);
    PICO_H264_LOG("    defaultGridPositionFlag: %s\n", mvcExt->defaultGridPositionFlag ? "true" : "false");

    if (!mvcExt->defaultGridPositionFlag) {
        PICO_H264_LOG("    view0GridPositionX: %u\n", (unsigned)mvcExt->view0GridPositionX);
        PICO_H264_LOG("    view0GridPositionY: %u\n", (unsigned)mvcExt->view0GridPositionY);
        PICO_H264_LOG("    view1GridPositionX: %u\n", (unsigned)mvcExt->view1GridPositionX);
        PICO_H264_LOG("    view1GridPositionY: %u\n", (unsigned)mvcExt->view1GridPositionY);
    }

    PICO_H264_LOG("    rpuFilterEnabledFlag: %s\n", mvcExt->rpuFilterEnabledFlag ? "true" : "false");
    PICO_H264_LOG("    rpuFieldProcessingFlag: %s\n", mvcExt->rpuFieldProcessingFlag ? "true" : "false");
}

static void __picoH264MVCVUIParametersExtensionDebugPrint(const picoH264MVCVUIParametersExtension_t *mvcVui)
{
    if (!mvcVui)
        return;

    PICO_H264_LOG("  MVC VUI Parameters Extension:\n");
    PICO_H264_LOG("    vuiMVCNumOpsMinus1: %u\n", (unsigned)mvcVui->vuiMVCNumOpsMinus1);

    uint16_t numOpsToShow = mvcVui->vuiMVCNumOpsMinus1;
    for (uint16_t i = 0; i < numOpsToShow && i < 1024; i++) {
        const picoH264MVCVUIParametersExtensionOpsEntry_t *entry = &mvcVui->vuiMVCOpsEntries[i];
        PICO_H264_LOG("    Operation Point[%u]:\n", (unsigned)i);
        PICO_H264_LOG("      vuiMVCOpsTemporalId: %u\n", (unsigned)entry->vuiMVCOpsTemporalId);
        PICO_H264_LOG("      vuiMVCOpsNumTargetViewsMinus1: %u\n", (unsigned)entry->vuiMVCOpsNumTargetViewsMinus1);
        PICO_H264_LOG("      vuiMVCTimingInfoPresentFlag: %s\n", entry->vuiMVCTimingInfoPresentFlag ? "true" : "false");

        if (entry->vuiMVCTimingInfoPresentFlag) {
            PICO_H264_LOG("      vuiMVCNumUnitsInTick: %u\n", (unsigned)entry->vuiMVCNumUnitsInTick);
            PICO_H264_LOG("      vuiMVCTimeScale: %u\n", (unsigned)entry->vuiMVCTimeScale);
            PICO_H264_LOG("      vuiMVCFixedFrameRateFlag: %s\n", entry->vuiMVCFixedFrameRateFlag ? "true" : "false");
        }

        PICO_H264_LOG("      vuiMVCNalHrdParametersPresentFlag: %s\n", entry->vuiMVCNalHrdParametersPresentFlag ? "true" : "false");
        PICO_H264_LOG("      vuiMVCVclHrdParametersPresentFlag: %s\n", entry->vuiMVCVclHrdParametersPresentFlag ? "true" : "false");

        if (entry->vuiMVCNalHrdParametersPresentFlag || entry->vuiMVCVclHrdParametersPresentFlag) {
            PICO_H264_LOG("      vuiMVCLowDelayHrdFlag: %s\n", entry->vuiMVCLowDelayHrdFlag ? "true" : "false");
        }

        PICO_H264_LOG("      vuiMVCPicStructPresentFlag: %s\n", entry->vuiMVCPicStructPresentFlag ? "true" : "false");
    }
}

void picoH264SubsetSequenceParameterSetDebugPrint(picoH264SubsetSequenceParameterSet spsSubset)
{
    PICO_ASSERT(spsSubset != NULL);

    PICO_H264_LOG("Subset Sequence Parameter Set:\n");
    PICO_H264_LOG("  NOTE: This is a PARTIAL implementation per ITU-T H.264 specification.\n");
    PICO_H264_LOG("  NOTE: The following extensions are NOT parsed/present:\n");
    PICO_H264_LOG("    - seq_parameter_set_mvcd_extension() (for profile_idc 138 or 135)\n");
    PICO_H264_LOG("    - seq_parameter_set_3davc_extension() (for profile_idc 139)\n");
    PICO_H264_LOG("    - additional_extension2_flag and associated data\n");
    PICO_H264_LOG("\n");

    PICO_H264_LOG("  Base SPS Data:\n");
    picoH264SequenceParameterSetDebugPrint(&spsSubset->spsData);

    PICO_H264_LOG("\n  hasSvcExtension: %s\n", spsSubset->hasSvcExtension ? "true" : "false");
    if (spsSubset->hasSvcExtension) {
        __picoH264SPSSVCExtensionDebugPrint(&spsSubset->svcExtension);

        PICO_H264_LOG("  svcVuiParametersPresentFlag: %s\n", spsSubset->svcVuiParametersPresentFlag ? "true" : "false");
        if (spsSubset->svcVuiParametersPresentFlag) {
            __picoH264SVCVUIParametersExtensionDebugPrint(&spsSubset->svcVuiParametersExtension);
        }
    }

    PICO_H264_LOG("\n  hasMvcExtension: %s\n", spsSubset->hasMvcExtension ? "true" : "false");
    if (spsSubset->hasMvcExtension) {
        __picoH264SPSMVCExtensionDebugPrint(&spsSubset->mvcExtension);

        PICO_H264_LOG("  mvcVuiParametersPresentFlag: %s\n", spsSubset->mvcVuiParametersPresentFlag ? "true" : "false");
        if (spsSubset->mvcVuiParametersPresentFlag) {
            __picoH264MVCVUIParametersExtensionDebugPrint(&spsSubset->mvcVuiParametersExtension);
        }
    }
}

void picoH264PictureParameterSetDebugPrint(picoH264PictureParameterSet pps)
{
    PICO_ASSERT(pps != NULL);

    PICO_H264_LOG("Picture Parameter Set:\n");
    PICO_H264_LOG("  picParameterSetId: %u\n", (unsigned)pps->picParameterSetId);
    PICO_H264_LOG("  seqParameterSetId: %u\n", (unsigned)pps->seqParameterSetId);
    PICO_H264_LOG("  entropyCodingModeFlag: %s\n", pps->entropyCodingModeFlag ? "CABAC" : "Exp-Golomb/CAVLC");
    PICO_H264_LOG("  bottomFieldPicOrderInFramePresentFlag: %s\n", pps->bottomFieldPicOrderInFramePresentFlag ? "true" : "false");
    PICO_H264_LOG("  numSliceGroupsMinus1: %u\n", (unsigned)pps->numSliceGroupsMinus1);

    if (pps->numSliceGroupsMinus1 > 0) {
        PICO_H264_LOG("  sliceGroupMapType: %u\n", (unsigned)pps->sliceGroupMapType);

        if (pps->sliceGroupMapType == 0) {
            PICO_H264_LOG("    (interleaved slice groups)\n");
            for (uint32_t i = 0; i < pps->numSliceGroupsMinus1 && i < 256; i++) {
                PICO_H264_LOG("    runLengthMinus1[%u]: %u\n", (unsigned)i, (unsigned)pps->runLengthMinus1[i]);
            }
        } else if (pps->sliceGroupMapType == 1) {
            PICO_H264_LOG("    (dispersed slice group mapping)\n");
        } else if (pps->sliceGroupMapType == 2) {
            PICO_H264_LOG("    (foreground and leftover slice groups)\n");
            for (uint32_t i = 0; i < pps->numSliceGroupsMinus1 && i < 256; i++) {
                PICO_H264_LOG("    topLeft[%u]: %u, bottomRight[%u]: %u\n",
                              (unsigned)i, (unsigned)pps->topLeft[i],
                              (unsigned)i, (unsigned)pps->bottomRight[i]);
            }
        } else if (pps->sliceGroupMapType >= 3 && pps->sliceGroupMapType <= 5) {
            PICO_H264_LOG("    (changing slice groups, type %u)\n", (unsigned)pps->sliceGroupMapType);
            PICO_H264_LOG("    sliceGroupChangeDirectionFlag: %s\n", pps->sliceGroupChangeDirectionFlag ? "true" : "false");
            PICO_H264_LOG("    sliceGroupChangeRateMinus1: %u\n", (unsigned)pps->sliceGroupChangeRateMinus1);
        } else if (pps->sliceGroupMapType == 6) {
            PICO_H264_LOG("    (explicit slice group assignment)\n");
            PICO_H264_LOG("    picSizeInMapUnitsMinus1: %u\n", (unsigned)pps->picSizeInMapUnitsMinus1);
            for (uint32_t i = 0; i <= pps->picSizeInMapUnitsMinus1 && i < 1024; i++) {
                PICO_H264_LOG("    sliceGroupId[%u]: %u\n", (unsigned)i, (unsigned)pps->sliceGroupId[i]);
            }
        }
    }

    PICO_H264_LOG("  numRefIdxL0DefaultActiveMinus1: %u\n", (unsigned)pps->numRefIdxL0DefaultActiveMinus1);
    PICO_H264_LOG("  numRefIdxL1DefaultActiveMinus1: %u\n", (unsigned)pps->numRefIdxL1DefaultActiveMinus1);
    PICO_H264_LOG("  weightedPredFlag: %s\n", pps->weightedPredFlag ? "true" : "false");
    PICO_H264_LOG("  weightedBipredIdc: %u\n", (unsigned)pps->weightedBipredIdc);
    PICO_H264_LOG("  picInitQpMinus26: %d\n", (int)pps->picInitQpMinus26);
    PICO_H264_LOG("  picInitQsMinus26: %d\n", (int)pps->picInitQsMinus26);
    PICO_H264_LOG("  chromaQpIndexOffset: %d\n", (int)pps->chromaQpIndexOffset);
    PICO_H264_LOG("  deblockingFilterControlPresentFlag: %s\n", pps->deblockingFilterControlPresentFlag ? "true" : "false");
    PICO_H264_LOG("  constrainedIntraPredFlag: %s\n", pps->constrainedIntraPredFlag ? "true" : "false");
    PICO_H264_LOG("  redundantPicCntPresentFlag: %s\n", pps->redundantPicCntPresentFlag ? "true" : "false");
    PICO_H264_LOG("  transform8x8ModeFlag: %s\n", pps->transform8x8ModeFlag ? "true" : "false");
    PICO_H264_LOG("  picScalingMatrixPresentFlag: %s\n", pps->picScalingMatrixPresentFlag ? "true" : "false");

    if (pps->picScalingMatrixPresentFlag) {
        PICO_H264_LOG("  Scaling Lists:\n");
        for (uint32_t i = 0; i < 12; i++) {
            if (pps->picScalingListPresentFlag[i]) {
                PICO_H264_LOG("    picScalingListPresentFlag[%u]: true\n", (unsigned)i);
                PICO_H264_LOG("    useDefaultScalingMatrix%sFlag[%u]: %s\n",
                              (i < 6) ? "4x4" : "8x8",
                              (unsigned)(i < 6 ? i : i - 6),
                              (i < 6 ? pps->useDefaultScalingMatrix4x4Flag[i] : pps->useDefaultScalingMatrix8x8Flag[i - 6]) ? "true" : "false");
            }
        }
    }

    PICO_H264_LOG("  secondChromaQpIndexOffset: %d\n", (int)pps->secondChromaQpIndexOffset);
}

void picoH264SEIMessageDebugPrint(picoH264SEIMessage seiMessage)
{
    PICO_ASSERT(seiMessage != NULL);

    PICO_H264_LOG("SEI Message:\n");
    PICO_H264_LOG("  seiMessageType: %s (%u)\n", picoH264SEIMessageTypeToString(seiMessage->payloadType), (unsigned)seiMessage->payloadType);
    PICO_H264_LOG("  payloadSize: %u\n", (unsigned)seiMessage->payloadSize);
}

void picoH264AccessUnitDelimiterDebugPrint(picoH264AccessUnitDelimiter aud)
{
    PICO_ASSERT(aud != NULL);

    PICO_H264_LOG("Access Unit Delimiter:\n");
    PICO_H264_LOG("  primaryPicType: %u\n", aud->primaryPicType);
}

static void __picoH264RefPicListModificationDebugPrint(const picoH264RefPicListModification_t *refPicListModification)
{
    if (!refPicListModification)
        return;

    PICO_H264_LOG("  Ref Pic List Modification:\n");
    PICO_H264_LOG("    refPicListModificationFlagL0: %s\n", refPicListModification->refPicListModificationFlagL0 ? "true" : "false");
    if (refPicListModification->refPicListModificationFlagL0) {
        PICO_H264_LOG("    numModificationsL0: %u\n", (unsigned)refPicListModification->numModificationsL0);
        for (uint32_t i = 0; i < refPicListModification->numModificationsL0; i++) {
            PICO_H264_LOG("      modificationL0[%u]: modificationOfPicNumsIdc: %u, absDiffPicNumMinus1: %u, longTermPicNum: %u, abs_diff_view_idx_minus1: %u\n",
                          (unsigned)i, (unsigned)refPicListModification->modificationsL0[i].modificationOfPicNumsIdc,
                          (unsigned)refPicListModification->modificationsL0[i].absDiffPicNumMinus1,
                          (unsigned)refPicListModification->modificationsL0[i].longTermPicNum,
                          (unsigned)refPicListModification->modificationsL0[i].abs_diff_view_idx_minus1);
        }
    }
    PICO_H264_LOG("    refPicListModificationFlagL1: %s\n", refPicListModification->refPicListModificationFlagL1 ? "true" : "false");
    if (refPicListModification->refPicListModificationFlagL1) {
        PICO_H264_LOG("    numModificationsL1: %u\n", (unsigned)refPicListModification->numModificationsL1);
        for (uint32_t i = 0; i < refPicListModification->numModificationsL1; i++) {
            PICO_H264_LOG("      modificationL1[%u]: modificationOfPicNumsIdc: %u, absDiffPicNumMinus1: %u, longTermPicNum: %u, abs_diff_view_idx_minus1: %u\n",
                          (unsigned)i, (unsigned)refPicListModification->modificationsL1[i].modificationOfPicNumsIdc,
                          (unsigned)refPicListModification->modificationsL1[i].absDiffPicNumMinus1,
                          (unsigned)refPicListModification->modificationsL1[i].longTermPicNum,
                          (unsigned)refPicListModification->modificationsL1[i].abs_diff_view_idx_minus1);
        }
    }
}

static void __picoH264PredWeightTableDebugPrint(const picoH264PredWeightTable_t *predWeightTable)
{
    if (!predWeightTable)
        return;

    PICO_H264_LOG("  Pred Weight Table:\n");
    PICO_H264_LOG("    lumaLog2WeightDenom: %u\n", (unsigned)predWeightTable->lumaLog2WeightDenom);
    PICO_H264_LOG("    chromaLog2WeightDenom: %u\n", (unsigned)predWeightTable->chromaLog2WeightDenom);
    for (int i = 0; i < 32; i++) {
        if (predWeightTable->lumaWeightL0Flag[i]) {
            PICO_H264_LOG("    lumaWeightL0[%d]: %d, lumaOffsetL0[%d]: %d\n", i, (int)predWeightTable->lumaWeightL0[i], i, (int)predWeightTable->lumaOffsetL0[i]);
        }
        if (predWeightTable->chromaWeightL0Flag[i]) {
            PICO_H264_LOG("    chromaWeightL0[%d][0]: %d, chromaOffsetL0[%d][0]: %d\n", i, (int)predWeightTable->chromaWeightL0[i][0], i, (int)predWeightTable->chromaOffsetL0[i][0]);
            PICO_H264_LOG("    chromaWeightL0[%d][1]: %d, chromaOffsetL0[%d][1]: %d\n", i, (int)predWeightTable->chromaWeightL0[i][1], i, (int)predWeightTable->chromaOffsetL0[i][1]);
        }
        if (predWeightTable->lumaWeightL1Flag[i]) {
            PICO_H264_LOG("    lumaWeightL1[%d]: %d, lumaOffsetL1[%d]: %d\n", i, (int)predWeightTable->lumaWeightL1[i], i, (int)predWeightTable->lumaOffsetL1[i]);
        }
        if (predWeightTable->chromaWeightL1Flag[i]) {
            PICO_H264_LOG("    chromaWeightL1[%d][0]: %d, chromaOffsetL1[%d][0]: %d\n", i, (int)predWeightTable->chromaWeightL1[i][0], i, (int)predWeightTable->chromaOffsetL1[i][0]);
            PICO_H264_LOG("    chromaWeightL1[%d][1]: %d, chromaOffsetL1[%d][1]: %d\n", i, (int)predWeightTable->chromaWeightL1[i][1], i, (int)predWeightTable->chromaOffsetL1[i][1]);
        }
    }
}

static void __picoH264DecRefPicMarkingDebugPrint(const picoH264DecRefPicMarking_t *decRefPicMarking)
{
    if (!decRefPicMarking)
        return;

    PICO_H264_LOG("  Dec Ref Pic Marking:\n");
    PICO_H264_LOG("    noOutputOfPriorPicsFlag: %s\n", decRefPicMarking->noOutputOfPriorPicsFlag ? "true" : "false");
    PICO_H264_LOG("    longTermReferenceFlag: %s\n", decRefPicMarking->longTermReferenceFlag ? "true" : "false");
    PICO_H264_LOG("    adaptiveRefPicMarkingModeFlag: %s\n", decRefPicMarking->adaptiveRefPicMarkingModeFlag ? "true" : "false");
    if (decRefPicMarking->adaptiveRefPicMarkingModeFlag) {
        PICO_H264_LOG("    numMMCOOperations: %u\n", (unsigned)decRefPicMarking->numMMCOOperations);
        for (uint32_t i = 0; i < decRefPicMarking->numMMCOOperations; i++) {
            PICO_H264_LOG("      mmco[%u]: memoryManagementControlOperation: %u, differenceOfPicNumsMinus1: %u, longTermPicNum: %u, longTermFrameIdx: %u, maxLongTermFrameIdxPlus1: %u\n",
                          (unsigned)i, (unsigned)decRefPicMarking->mmcoOperations[i].memoryManagementControlOperation,
                          (unsigned)decRefPicMarking->mmcoOperations[i].differenceOfPicNumsMinus1,
                          (unsigned)decRefPicMarking->mmcoOperations[i].longTermPicNum,
                          (unsigned)decRefPicMarking->mmcoOperations[i].longTermFrameIdx,
                          (unsigned)decRefPicMarking->mmcoOperations[i].maxLongTermFrameIdxPlus1);
        }
    }
}

void picoH264SliceHeaderDebugPrint(picoH264SliceHeader sliceHeader)
{
    PICO_ASSERT(sliceHeader != NULL);

    PICO_H264_LOG("Slice Header:\n");
    PICO_H264_LOG("  firstMbInSlice: %u\n", (unsigned)sliceHeader->firstMbInSlice);
    PICO_H264_LOG("  sliceType: %s (%u)\n", picoH264SliceTypeToString(sliceHeader->sliceType), (unsigned)sliceHeader->sliceType);
    PICO_H264_LOG("  picParameterSetId: %u\n", (unsigned)sliceHeader->picParameterSetId);
    PICO_H264_LOG("  colourPlaneId: %u\n", (unsigned)sliceHeader->colourPlaneId);
    PICO_H264_LOG("  frameNum: %u\n", (unsigned)sliceHeader->frameNum);
    PICO_H264_LOG("  fieldPicFlag: %s\n", sliceHeader->fieldPicFlag ? "true" : "false");
    if (sliceHeader->fieldPicFlag) {
        PICO_H264_LOG("  bottomFieldFlag: %s\n", sliceHeader->bottomFieldFlag ? "true" : "false");
    }
    PICO_H264_LOG("  idrPicId: %u\n", (unsigned)sliceHeader->idrPicId);
    PICO_H264_LOG("  picOrderCntLsb: %u\n", (unsigned)sliceHeader->picOrderCntLsb);
    PICO_H264_LOG("  deltaPicOrderCntBottom: %d\n", (int)sliceHeader->deltaPicOrderCntBottom);
    PICO_H264_LOG("  deltaPicOrderCnt[0]: %d\n", (int)sliceHeader->deltaPicOrderCnt[0]);
    PICO_H264_LOG("  deltaPicOrderCnt[1]: %d\n", (int)sliceHeader->deltaPicOrderCnt[1]);
    PICO_H264_LOG("  redundantPicCnt: %d\n", (int)sliceHeader->redundantPicCnt);
    PICO_H264_LOG("  directSpatialMvPredFlag: %s\n", sliceHeader->directSpatialMvPredFlag ? "true" : "false");
    PICO_H264_LOG("  numRefIdxActiveOverrideFlag: %s\n", sliceHeader->numRefIdxActiveOverrideFlag ? "true" : "false");
    if (sliceHeader->numRefIdxActiveOverrideFlag) {
        PICO_H264_LOG("  numRefIdxL0ActiveMinus1: %u\n", (unsigned)sliceHeader->numRefIdxL0ActiveMinus1);
        PICO_H264_LOG("  numRefIdxL1ActiveMinus1: %u\n", (unsigned)sliceHeader->numRefIdxL1ActiveMinus1);
    }

    __picoH264RefPicListModificationDebugPrint(&sliceHeader->refPicListModification);
    __picoH264RefPicListModificationDebugPrint(&sliceHeader->refPicListMvcModification);
    __picoH264PredWeightTableDebugPrint(&sliceHeader->predWeightTable);
    __picoH264DecRefPicMarkingDebugPrint(&sliceHeader->decRefPicMarking);

    PICO_H264_LOG("  cabacInitIdc: %u\n", (unsigned)sliceHeader->cabacInitIdc);
    PICO_H264_LOG("  sliceQpDelta: %d\n", (int)sliceHeader->sliceQpDelta);
    PICO_H264_LOG("  spForSwitchFlag: %s\n", sliceHeader->spForSwitchFlag ? "true" : "false");
    PICO_H264_LOG("  sliceQsDelta: %d\n", (int)sliceHeader->sliceQsDelta);
    PICO_H264_LOG("  disableDeblockingFilterIdc: %d\n", (int)sliceHeader->disableDeblockingFilterIdc);
    PICO_H264_LOG("  sliceAlphaC0OffsetDiv2: %d\n", (int)sliceHeader->sliceAlphaC0OffsetDiv2);
    PICO_H264_LOG("  sliceBetaOffsetDiv2: %d\n", (int)sliceHeader->sliceBetaOffsetDiv2);
    PICO_H264_LOG("  sliceGroupChangeCycle: %u\n", (unsigned)sliceHeader->sliceGroupChangeCycle);
}

void picoH264SliceDataDebugPrint(picoH264SliceData sliceData)
{
    PICO_ASSERT(sliceData != NULL);
    PICO_H264_LOG("Slice Data: (not implemented)\n");
}

void picoH264SliceLayerWithoutPartitioningDebugPrint(picoH264SliceLayerWithoutPartitioning sliceLayer)
{
    PICO_ASSERT(sliceLayer != NULL);
    PICO_H264_LOG("Slice Layer Without Partitioning:\n");
    picoH264SliceHeaderDebugPrint(&sliceLayer->header);
    picoH264SliceDataDebugPrint(&sliceLayer->data);
}

void picoH264SliceDataPartitionALayerDebugPrint(picoH264SliceDataPartitionALayer sliceDataPartitionALayer)
{
    PICO_ASSERT(sliceDataPartitionALayer != NULL);
    PICO_H264_LOG("Slice Data Partition A Layer:\n");
    picoH264SliceHeaderDebugPrint(&sliceDataPartitionALayer->header);
    PICO_H264_LOG("  sliceId: %u\n", (unsigned)sliceDataPartitionALayer->sliceId);
    picoH264SliceDataDebugPrint(&sliceDataPartitionALayer->data);
}

void picoH264SliceDataPartitionBLayerDebugPrint(picoH264SliceDataPartitionBLayer sliceDataPartitionBLayer)
{
    PICO_ASSERT(sliceDataPartitionBLayer != NULL);
    PICO_H264_LOG("Slice Data Partition B Layer:\n");
    PICO_H264_LOG("  sliceId: %u\n", (unsigned)sliceDataPartitionBLayer->sliceId);
    PICO_H264_LOG("  colorPlaneId: %u\n", (unsigned)sliceDataPartitionBLayer->colorPlaneId);
    PICO_H264_LOG("  redundantPicCnt: %u\n", (unsigned)sliceDataPartitionBLayer->redundantPicCnt);
    picoH264SliceDataDebugPrint(&sliceDataPartitionBLayer->data);
}

void picoH264SliceDataPartitionCLayerDebugPrint(picoH264SliceDataPartitionCLayer sliceDataPartitionCLayer)
{
    PICO_ASSERT(sliceDataPartitionCLayer != NULL);
    PICO_H264_LOG("Slice Data Partition C Layer:\n");
    PICO_H264_LOG("  sliceId: %u\n", (unsigned)sliceDataPartitionCLayer->sliceId);
    PICO_H264_LOG("  colorPlaneId: %u\n", (unsigned)sliceDataPartitionCLayer->colorPlaneId);
    PICO_H264_LOG("  redundantPicCnt: %u\n", (unsigned)sliceDataPartitionCLayer->redundantPicCnt);
    picoH264SliceDataDebugPrint(&sliceDataPartitionCLayer->data);
}

const char *picoH264SEIMessageTypeToString(picoH264SEIMessageType seiMessageType)
{
    switch (seiMessageType) {
        case PICO_H264_SEI_MESSAGE_TYPE_BUFFERING_PERIOD:
            return "Buffering Period";
        case PICO_H264_SEI_MESSAGE_TYPE_PIC_TIMING:
            return "Picture Timing";
        case PICO_H264_SEI_MESSAGE_TYPE_PAN_SCAN_RECT:
            return "Pan Scan Rectangle";
        case PICO_H264_SEI_MESSAGE_TYPE_FILLER_PAYLOAD:
            return "Filler Payload";
        case PICO_H264_SEI_MESSAGE_TYPE_USER_DATA_REGISTERED_ITU_T_35:
            return "User Data Registered (ITU-T T.35)";
        case PICO_H264_SEI_MESSAGE_TYPE_USER_DATA_UNREGISTERED:
            return "User Data Unregistered";
        case PICO_H264_SEI_MESSAGE_TYPE_RECOVERY_POINT:
            return "Recovery Point";
        case PICO_H264_SEI_MESSAGE_TYPE_DEC_REF_PIC_MARKING_REPETITION:
            return "Decoded Reference Picture Marking Repetition";
        case PICO_H264_SEI_MESSAGE_TYPE_SPARE_PIC:
            return "Spare Picture";
        case PICO_H264_SEI_MESSAGE_TYPE_SCENE_INFO:
            return "Scene Information";
        case PICO_H264_SEI_MESSAGE_TYPE_SUB_SEQ_INFO:
            return "Sub-Sequence Information";
        case PICO_H264_SEI_MESSAGE_TYPE_SUB_SEQ_LAYER_CHARACTERISTICS:
            return "Sub-Sequence Layer Characteristics";
        case PICO_H264_SEI_MESSAGE_TYPE_SUB_SEQ_CHARACTERISTICS:
            return "Sub-Sequence Characteristics";
        case PICO_H264_SEI_MESSAGE_TYPE_FILL_FRAME_FREEZE:
            return "Fill Frame (Freeze)";
        case PICO_H264_SEI_MESSAGE_TYPE_FILL_FRAME_FREEZE_RELEASE:
            return "Fill Frame (Freeze Release)";
        case PICO_H264_SEI_MESSAGE_TYPE_FULL_FRAME_SNAPSHOT:
            return "Full Frame Snapshot";
        case PICO_H264_SEI_MESSAGE_TYPE_PROGRESSIVE_REFINEMENT_SEGMENT_START:
            return "Progressive Refinement Segment Start";
        case PICO_H264_SEI_MESSAGE_TYPE_PROGRESSIVE_REFINEMENT_SEGMENT_END:
            return "Progressive Refinement Segment End";
        case PICO_H264_SEI_MESSAGE_TYPE_MOTION_CONSTRAINED_SLICE_GROUP_SET:
            return "Motion Constrained Slice Group Set";
        case PICO_H264_SEI_MESSAGE_TYPE_FILM_GRAIN_CHARACTERISTICS:
            return "Film Grain Characteristics";
        case PICO_H264_SEI_MESSAGE_TYPE_DEBLOCKING_FILTER_DISPLAY_PREFERENCE:
            return "Deblocking Filter Display Preference";
        case PICO_H264_SEI_MESSAGE_TYPE_STEREO_VIDEO_INFO:
            return "Stereo Video Information";
        case PICO_H264_SEI_MESSAGE_TYPE_POST_FILTER_HINT:
            return "Post-Filter Hint";
        case PICO_H264_SEI_MESSAGE_TYPE_TONE_MAPPING_INFO:
            return "Tone Mapping Information";
        case PICO_H264_SEI_MESSAGE_TYPE_SCALABILITY_INFO:
            return "Scalability Information (Annex F)";
        case PICO_H264_SEI_MESSAGE_TYPE_SUB_PIC_SCALABLE_LAYER:
            return "Sub-Picture Scalable Layer (Annex F)";
        case PICO_H264_SEI_MESSAGE_TYPE_NON_REQUIRED_LAYER_REP:
            return "Non-Required Layer Representation (Annex F)";
        case PICO_H264_SEI_MESSAGE_TYPE_PRIORITY_LAYER_INFO:
            return "Priority Layer Information (Annex F)";
        case PICO_H264_SEI_MESSAGE_TYPE_LAYERS_NOT_PRESENT:
            return "Layers Not Present (Annex F)";
        case PICO_H264_SEI_MESSAGE_TYPE_LAYER_DEPENDENCY_CHANGE:
            return "Layer Dependency Change (Annex F)";
        case PICO_H264_SEI_MESSAGE_TYPE_SCALABLE_NESTING:
            return "Scalable Nesting (Annex F)";
        case PICO_H264_SEI_MESSAGE_TYPE_BASE_LAYER_TEMPORAL_HRD:
            return "Base Layer Temporal HRD (Annex F)";
        case PICO_H264_SEI_MESSAGE_TYPE_QUALITY_LAYER_INTEGRITY_CHECK:
            return "Quality Layer Integrity Check (Annex F)";
        case PICO_H264_SEI_MESSAGE_TYPE_REDUNDANT_PIC_PROPERTY:
            return "Redundant Picture Property (Annex F)";
        case PICO_H264_SEI_MESSAGE_TYPE_TL0_DEP_REP_INDEX:
            return "Temporal Layer 0 Dependency Representation Index (Annex F)";
        case PICO_H264_SEI_MESSAGE_TYPE_TL_SWITCHING_POINT:
            return "Temporal Layer Switching Point (Annex F)";
        case PICO_H264_SEI_MESSAGE_TYPE_PARALLEL_DECODING_INFO:
            return "Parallel Decoding Information (Annex G)";
        case PICO_H264_SEI_MESSAGE_TYPE_MVC_SCALABLE_NESTING:
            return "MVC Scalable Nesting (Annex G)";
        case PICO_H264_SEI_MESSAGE_TYPE_VIEW_SCALABILITY_INFO:
            return "View Scalability Information (Annex G)";
        case PICO_H264_SEI_MESSAGE_TYPE_MULTIVIEW_SCENE_INFO:
            return "Multiview Scene Information (Annex G)";
        case PICO_H264_SEI_MESSAGE_TYPE_MULTIVIEW_ACQUISITION_INFO:
            return "Multiview Acquisition Information (Annex G)";
        case PICO_H264_SEI_MESSAGE_TYPE_NON_REQUIRED_VIEW_COMPONENT:
            return "Non-Required View Component (Annex G)";
        case PICO_H264_SEI_MESSAGE_TYPE_VIEW_DEPENDENCY_CHANGE:
            return "View Dependency Change (Annex G)";
        case PICO_H264_SEI_MESSAGE_TYPE_OPERATION_POINTS_NOT_PRESENT:
            return "Operation Points Not Present (Annex G)";
        case PICO_H264_SEI_MESSAGE_TYPE_BASE_VIEW_TEMPORAL_HRD:
            return "Base View Temporal HRD (Annex G)";
        case PICO_H264_SEI_MESSAGE_TYPE_FRAME_PACKING_ARRANGEMENT:
            return "Frame Packing Arrangement";
        case PICO_H264_SEI_MESSAGE_TYPE_MULTIVIEW_VIEW_POSITION:
            return "Multiview View Position (Annex G)";
        case PICO_H264_SEI_MESSAGE_TYPE_DISPLAY_ORIENTATION:
            return "Display Orientation";
        case PICO_H264_SEI_MESSAGE_TYPE_MVCD_SCALABLE_NESTING:
            return "MVCD Scalable Nesting (Annex H)";
        case PICO_H264_SEI_MESSAGE_TYPE_MVCD_VIEW_SCALABILITY_INFO:
            return "MVCD View Scalability Information (Annex H)";
        case PICO_H264_SEI_MESSAGE_TYPE_DEPTH_REPRESENTATION_INFO:
            return "Depth Representation Information (Annex H)";
        case PICO_H264_SEI_MESSAGE_TYPE_THREE_DIMENSIONAL_REFERENCE_DISPLAYS_INFO:
            return "Three-Dimensional Reference Displays Information (Annex H)";
        case PICO_H264_SEI_MESSAGE_TYPE_DEPTH_TIMING:
            return "Depth Timing (Annex H)";
        case PICO_H264_SEI_MESSAGE_TYPE_DEPTH_SAMPLING_INFO:
            return "Depth Sampling Information (Annex H)";
        case PICO_H264_SEI_MESSAGE_TYPE_CONSTRAINED_DEPTH_PARAMETER_SET_IDENTIFIER:
            return "Constrained Depth Parameter Set Identifier (Annex I)";
        case PICO_H264_SEI_MESSAGE_TYPE_GREEN_METADATA:
            return "Green Metadata (ISO/IEC 23001-11)";
        case PICO_H264_SEI_MESSAGE_TYPE_MASTERING_DISPLAY_COLOUR_VOLUME:
            return "Mastering Display Colour Volume";
        case PICO_H264_SEI_MESSAGE_TYPE_COLOUR_REMAPPING_INFO:
            return "Colour Remapping Information";
        case PICO_H264_SEI_MESSAGE_TYPE_CONTENT_LIGHT_LEVEL_INFO:
            return "Content Light Level Information";
        case PICO_H264_SEI_MESSAGE_TYPE_ALTERNATIVE_TRANSFER_CHARACTERISTICS:
            return "Alternative Transfer Characteristics";
        case PICO_H264_SEI_MESSAGE_TYPE_AMBIENT_VIEWING_ENVIRONMENT:
            return "Ambient Viewing Environment";
        case PICO_H264_SEI_MESSAGE_TYPE_CONTENT_COLOUR_VOLUME:
            return "Content Colour Volume";
        case PICO_H264_SEI_MESSAGE_TYPE_EQUIRECTANGULAR_PROJECTION:
            return "Equirectangular Projection";
        case PICO_H264_SEI_MESSAGE_TYPE_CUBEMAP_PROJECTION:
            return "Cubemap Projection";
        case PICO_H264_SEI_MESSAGE_TYPE_SPHERE_ROTATION:
            return "Sphere Rotation";
        case PICO_H264_SEI_MESSAGE_TYPE_REGIONWISE_PACKING:
            return "Regionwise Packing";
        case PICO_H264_SEI_MESSAGE_TYPE_OMNI_VIEWPORT:
            return "Omni Viewport";
        case PICO_H264_SEI_MESSAGE_TYPE_ALTERNATIVE_DEPTH_INFO:
            return "Alternative Depth Information (Annex H)";
        case PICO_H264_SEI_MESSAGE_TYPE_SEI_MANIFEST:
            return "SEI Manifest";
        case PICO_H264_SEI_MESSAGE_TYPE_SEI_PREFIX_INDICATION:
            return "SEI Prefix Indication";
        case PICO_H264_SEI_MESSAGE_TYPE_ANNOTATED_REGIONS:
            return "Annotated Regions (ITU-T H.274 | ISO/IEC 23002-7)";
        case PICO_H264_SEI_MESSAGE_TYPE_SHUTTER_INTERVAL_INFO:
            return "Shutter Interval Information";
        case PICO_H264_SEI_MESSAGE_TYPE_NN_POST_FILTER_CHARACTERISTICS:
            return "Neural Network Post-Filter Characteristics (ITU-T H.274 | ISO/IEC 23002-7)";
        case PICO_H264_SEI_MESSAGE_TYPE_NN_POST_FILTER_ACTIVATION:
            return "Neural Network Post-Filter Activation (ITU-T H.274 | ISO/IEC 23002-7)";
        case PICO_H264_SEI_MESSAGE_TYPE_PHASE_INDICATION:
            return "Phase Indication (ITU-T H.274 | ISO/IEC 23002-7)";
        default:
            return "Unknown SEI Message Type";
    }
}

#endif // PICO_H264_IMPLEMENTATION

#endif // PICO_H264_H
