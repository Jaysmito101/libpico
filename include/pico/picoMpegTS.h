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
 * This has been purely implemented based of the ITU-T H.222.0 v9 (08/2023) specification.
 * Source: https://www.itu.int/rec/T-REC-H.222.0-202308-S/en
 * And some references from: https://tsduck.io/docs/mpegts-introduction.pdf
 * And also: https://github.com/FFmpeg/FFmpeg/blob/master/libavformat/mpegts.c
 * - Jaysmito Mukherjee (jaysmito101@gmail.com)
 */

#ifndef PICO_MPEGTS_H
#define PICO_MPEGTS_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define PICO_IMPLEMENTATION

#define PICO_MPEGTS_RETURN_ON_ERROR(resultVar)   \
    do {                                         \
        picoMpegTSResult res = resultVar;        \
        if (res != PICO_MPEGTS_RESULT_SUCCESS) { \
            return res;                          \
        }                                        \
    } while (0)

#ifndef PICO_INITIAL_PARSED_PACKETS_CAPACITY
#define PICO_INITIAL_PARSED_PACKETS_CAPACITY 1024
#endif

// Maximum number of PIDs (13-bit PID = 0x0000 to 0x1FFF)
#ifndef PICO_MPEGTS_MAX_PIDS
#define PICO_MPEGTS_MAX_PIDS 8192
#endif

#ifndef PICO_MPEGTS_MAX_DESCRIPTORS
#define PICO_MPEGTS_MAX_DESCRIPTORS 256 // maxmum descriptors to be parsed per section
#endif

#ifndef PICO_MPEGTS_MAX_TABLE_PAYLOAD_COUNT
#define PICO_MPEGTS_MAX_TABLE_PAYLOAD_COUNT 256 // maximum payloads to be stored per table
#endif

#ifndef PICO_MPEGTS_MAX_SECTIONS
#define PICO_MPEGTS_MAX_SECTIONS 256
#endif

#ifndef PICO_MPEGTS_TABLE_VERSION_AGE_THRESHOLD_SECONDS
#define PICO_MPEGTS_TABLE_VERSION_AGE_THRESHOLD_SECONDS 7200
#endif

#define PICO_MPEGTS_CC_UNINITIALIZED 42

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

#ifndef PICO_MPEGTS_LOG
#define PICO_MPEGTS_LOG(...) \
    do {                     \
        (void)0;             \
    } while (0)
#endif

#define PICO_MPEGTS_MAX_PID_COUNT              8192
#define PICO_MPEGTS_MAX_TABLE_COUNT            256
#define PICO_MPEGTS_MAX_DESCRIPTOR_DATA_LENGTH 256
#define PICO_MPEGTS_MAX_VERSIONS               32

typedef enum {
    PICO_MPEGTS_PACKET_TYPE_DEFAULT = 188,
    PICO_MPEGTS_PACKET_TYPE_M2TS    = 192,
    PICO_MPEGTS_PACKET_TYPE_DVB     = 204,
    PICO_MPEGTS_PACKET_TYPE_UNKNOWN = 0,
} picoMpegTSPacketType;

typedef enum {
    PICO_MPEGTS_FILTER_TYPE_PES     = 0,
    PICO_MPEGTS_FILTER_TYPE_SECTION = 1,
    PICO_MPEGTS_FILTER_TYPE_NULL    = 2,
} picoMpegTSFilterType;

typedef enum {
    PICO_MPEGTS_PID_PAT            = 0x0000, // Program Association Table
    PICO_MPEGTS_PID_CAT            = 0x0001, // Conditional Access Table
    PICO_MPEGTS_PID_TSDT           = 0x0002, // Transport Stream Description Table
    PICO_MPEGTS_PID_IPMP           = 0x0003, // IPMP Control Information
    PICO_MPEGTS_PID_ASI            = 0x0004, // Auxiliary Section Information
    PICO_MPEGTS_PID_RESERVED_START = 0x0005, // PIDs from 0x0005 to 0x000F are reserved
    PICO_MPEGTS_PID_RESERVED_END   = 0x000F,
    PICO_MPEGTS_PID_NIT            = 0x0010, // Network Information Table
    PICO_MPEGTS_PID_SDT_BAT        = 0x0011, // Service Description Table / Bouquet Association Table
    PICO_MPEGTS_PID_EIT            = 0x0012, // Event Information Table
    PICO_MPEGTS_PID_RST            = 0x0013, // Running Status Table
    PICO_MPEGTS_PID_TDT_TOT        = 0x0014, // Time and Date Table / Time Offset Table
    PICO_MPEGTS_PID_NET_SYNC       = 0x0015,
    PICO_MPEGTS_PID_RNT            = 0x0016, // RAR Notification Table
    PICO_MPEGTS_PID_LINK_LOCAL     = 0x001C,
    PICO_MPEGTS_PID_MEASUREMENT    = 0x001D,
    PICO_MPEGTS_PID_DIT            = 0x001E, // Discontinuity Information Table
    PICO_MPEGTS_PID_SIT            = 0x001F, // Selection Information Table
    PICO_MPEGTS_PID_CUSTOM_START   = 0x0020, // Custom PIDs can start from here (these may be assigned dynamically)
    PICO_MPEGTS_PID_CUSTOM_END     = 0x1FFE,
    PICO_MPEGTS_PID_NULL_PACKET    = 0x1FFF,
} picoMpegTSPacketPID;

// 0x0C -> 0x37 reserved
// 0x38 -> 0x3F defined in ISO/IEC 13818-6
// 0x43 -> 0x45 reserved
// 0x47 -> 0x49 reserved
// 0x4D reserved
typedef enum {
    PICO_MPEGTS_TABLE_ID_PAS                = 0x00, // Program Association Section
    PICO_MPEGTS_TABLE_ID_CAS                = 0x01, // Conditional Access Section
    PICO_MPEGTS_TABLE_ID_PMS                = 0x02, // Program Map Section
    PICO_MPEGTS_TABLE_ID_TSDS               = 0x03, // Transport Stream Description Section
    PICO_MPEGTS_TABLE_ID_ISDDS              = 0x04, // ISO/IEC 14496 Scene Description Section
    PICO_MPEGTS_TABLE_ID_ISODS              = 0x05, // ISO/IEC 14496 Object Descriptor Section
    PICO_MPEGTS_TABLE_ID_METAS              = 0x06, // Metadata Section
    PICO_MPEGTS_TABLE_ID_IPMP               = 0x07, // IPMP Control Information Section
    PICO_MPEGTS_TABLE_ID_ISOS               = 0x08, // ISO/IEC 14496 Section
    PICO_MPEGTS_TABLE_ID_GAUS               = 0x09, // ISO/IEC 23001-11 (Green access unit) section
    PICO_MPEGTS_TABLE_ID_QAUS               = 0x0A, // ISO/IEC 23001-10 (Quality access unit) section
    PICO_MPEGTS_TABLE_ID_MOAUS              = 0x0B, // ISO/IEC 23001-13 (Media Orchestration access unit) section
    PICO_MPEGTS_TABLE_ID_NISAN              = 0x40, // Network Information Section Actual Network
    PICO_MPEGTS_TABLE_ID_NISON              = 0x41, // Network Information Section Other Network
    PICO_MPEGTS_TABLE_ID_SDSATS             = 0x42, // Service Description Section Actual Transport Stream
    PICO_MPEGTS_TABLE_ID_SDSOTS             = 0x46, // Service Description Section Other Transport Stream
    PICO_MPEGTS_TABLE_ID_BAS                = 0x4A, // Bouquet Association Section
    PICO_MPEGTS_TABLE_ID_UNTS               = 0x4B, // Update Notification Table Section
    PICO_MPEGTS_TABLE_ID_DFIS               = 0x4C, // Downloadable Font Info Section
    PICO_MPEGTS_TABLE_ID_EISATSF            = 0x4E, // Event Information Section Actual Transport Stream, Present/Following
    PICO_MPEGTS_TABLE_ID_EISOTSF            = 0x4F, // Event Information Section Other Transport Stream, Present/Following
    PICO_MPEGTS_TABLE_ID_TDS                = 0x70, // Time Date Section
    PICO_MPEGTS_TABLE_ID_RSS                = 0x71, // Running Status Section
    PICO_MPEGTS_TABLE_ID_SS                 = 0x72, // Stuffing Section
    PICO_MPEGTS_TABLE_ID_TOS                = 0x73, // Time Offset Section
    PICO_MPEGTS_TABLE_ID_AIS                = 0x74, // Application Information Section
    PICO_MPEGTS_TABLE_ID_CS                 = 0x75, // Container Section
    PICO_MPEGTS_TABLE_ID_RCS                = 0x76, // Related Content Section
    PICO_MPEGTS_TABLE_ID_CIS                = 0x77, // Content Identifier Section
    PICO_MPEGTS_TABLE_ID_MPEFCS             = 0x78, // MPE-FEC Section
    PICO_MPEGTS_TABLE_ID_RPNS               = 0x79, // Resolution Provider Notification Section
    PICO_MPEGTS_TABLE_ID_MPEIFECS           = 0x7A, // MPE-IFEC Section
    PICO_MPEGTS_TABLE_ID_PMSGS              = 0x7B, // Protection Message Section
    PICO_MPEGTS_TABLE_ID_DIS                = 0x7E, // Discontinuity Information Section
    PICO_MPEGTS_TABLE_ID_SIS                = 0x7F, // Selection Information Section
    PICO_MPEGTS_TABLE_ID_USER_DEFINED_START = 0x80,
    PICO_MPEGTS_TABLE_ID_USER_DEFINED_END   = 0xFE,
    PICO_MPEGTS_TABLE_ID_FORBIDDEN          = 0xFF,
} picoMpegTSTableID;

typedef enum {
    PICO_MPEGTS_STREAM_TYPE_RESERVED                = 0x00,
    PICO_MPEGTS_STREAM_TYPE_MPEG1_VIDEO             = 0x01, // ISO/IEC 11172-2 Video
    PICO_MPEGTS_STREAM_TYPE_MPEG2_VIDEO             = 0x02, // ITU-T H.262 | ISO/IEC 13818-2 Video or ISO/IEC 11172-2 constrained parameter video stream
    PICO_MPEGTS_STREAM_TYPE_MPEG1_AUDIO             = 0x03, // ISO/IEC 11172-3 Audio
    PICO_MPEGTS_STREAM_TYPE_MPEG2_AUDIO             = 0x04, // ISO/IEC 13818-3 Audio
    PICO_MPEGTS_STREAM_TYPE_PRIVATE_SECTIONS        = 0x05, // ITU-T H.222.0 | ISO/IEC 13818-1 private_sections
    PICO_MPEGTS_STREAM_TYPE_PRIVATE_DATA            = 0x06, // ITU-T H.222.0 | ISO/IEC 13818-1 PES packets containing private data
    PICO_MPEGTS_STREAM_TYPE_MHEG                    = 0x07, // ISO/IEC 13522 MHEG
    PICO_MPEGTS_STREAM_TYPE_DSM_CC                  = 0x08, // ITU-T H.222.0 | ISO/IEC 13818-1 Annex A DSM-CC
    PICO_MPEGTS_STREAM_TYPE_H222_1                  = 0x09, // ITU-T H.222.1
    PICO_MPEGTS_STREAM_TYPE_DSMCC_TYPE_A            = 0x0A, // ISO/IEC 13818-6 type A
    PICO_MPEGTS_STREAM_TYPE_DSMCC_TYPE_B            = 0x0B, // ISO/IEC 13818-6 type B
    PICO_MPEGTS_STREAM_TYPE_DSMCC_TYPE_C            = 0x0C, // ISO/IEC 13818-6 type C
    PICO_MPEGTS_STREAM_TYPE_DSMCC_TYPE_D            = 0x0D, // ISO/IEC 13818-6 type D
    PICO_MPEGTS_STREAM_TYPE_H222_AUXILIARY          = 0x0E, // ITU-T H.222.0 | ISO/IEC 13818-1 auxiliary
    PICO_MPEGTS_STREAM_TYPE_AAC_ADTS                = 0x0F, // ISO/IEC 13818-7 Audio with ADTS transport syntax
    PICO_MPEGTS_STREAM_TYPE_MPEG4_VISUAL            = 0x10, // ISO/IEC 14496-2 Visual
    PICO_MPEGTS_STREAM_TYPE_AAC_LATM                = 0x11, // ISO/IEC 14496-3 Audio with the LATM transport syntax as defined in ISO/IEC 14496-3
    PICO_MPEGTS_STREAM_TYPE_MPEG4_PES               = 0x12, // ISO/IEC 14496-1 SL-packetized stream or FlexMux stream carried in PES packets
    PICO_MPEGTS_STREAM_TYPE_MPEG4_SECTIONS          = 0x13, // ISO/IEC 14496-1 SL-packetized stream or FlexMux stream carried in ISO/IEC 14496_sections
    PICO_MPEGTS_STREAM_TYPE_MPEG2_DSMCC_DATA        = 0x14, // ISO/IEC 13818-6 Synchronized Download Protocol
    PICO_MPEGTS_STREAM_TYPE_METADATA_PES            = 0x15, // Metadata carried in PES packets
    PICO_MPEGTS_STREAM_TYPE_METADATA_SECTIONS       = 0x16, // Metadata carried in metadata_sections
    PICO_MPEGTS_STREAM_TYPE_METADATA_DSMCC_DATA     = 0x17, // Metadata carried in ISO/IEC 13818-6 Data Carousel
    PICO_MPEGTS_STREAM_TYPE_METADATA_DSMCC_OBJECT   = 0x18, // Metadata carried in ISO/IEC 13818-6 Object Carousel
    PICO_MPEGTS_STREAM_TYPE_METADATA_DSMCC_DOWNLOAD = 0x19, // Metadata carried in ISO/IEC 13818-6 Synchronized Download Protocol
    PICO_MPEGTS_STREAM_TYPE_MPEG2_IPMP              = 0x1A, // IPMP stream (defined in ISO/IEC 13818-11, MPEG-2 IPMP)
    PICO_MPEGTS_STREAM_TYPE_H264                    = 0x1B, // AVC video stream conforming to one or more profiles defined in Annex A of ITU-T H.264 | ISO/IEC 14496-10
    PICO_MPEGTS_STREAM_TYPE_MPEG4_AUDIO_RAW         = 0x1C, // ISO/IEC 14496-3 Audio, without using any additional transport syntax
    PICO_MPEGTS_STREAM_TYPE_MPEG4_TEXT              = 0x1D, // ISO/IEC 14496-17 Text
    PICO_MPEGTS_STREAM_TYPE_MPEG4_AUXILIARY         = 0x1E, // Auxiliary video stream as defined in ISO/IEC 23002-3
    PICO_MPEGTS_STREAM_TYPE_H264_SVC                = 0x1F, // SVC video sub-bitstream of an AVC video stream conforming to one or more profiles defined in Annex G of ITU-T H.264 | ISO/IEC 14496-10
    PICO_MPEGTS_STREAM_TYPE_H264_MVC                = 0x20, // MVC video sub-bitstream of an AVC video stream conforming to one or more profiles defined in Annex H of ITU-T H.264 | ISO/IEC 14496-10
    PICO_MPEGTS_STREAM_TYPE_JPEG2000                = 0x21, // Video stream conforming to one or more profiles as defined in ITU-T T.800 | ISO/IEC 15444-1
    PICO_MPEGTS_STREAM_TYPE_MPEG2_3D_VIDEO          = 0x22, // Additional view ITU-T H.262 | ISO/IEC 13818-2 video stream for service-compatible stereoscopic 3D services
    PICO_MPEGTS_STREAM_TYPE_H264_STEREO_3D          = 0x23, // Additional view ITU-T H.264 | ISO/IEC 14496-10 video stream for service-compatible stereoscopic 3D services
    PICO_MPEGTS_STREAM_TYPE_HEVC                    = 0x24, // ITU-T H.265 and ISO/IEC 23008-2 video stream or an HEVC temporal video sub-bitstream
    PICO_MPEGTS_STREAM_TYPE_HEVC_SUBSET             = 0x25, // HEVC temporal video subset of an HEVC video stream conforming to one or more profiles defined in Annex A of ITU-T H.265 | ISO/IEC 23008-2
    PICO_MPEGTS_STREAM_TYPE_H264_MVCD               = 0x26, // MVCD video sub-bitstream of an AVC video stream conforming to one or more profiles defined in Annex I of ITU-T H.264 | ISO/IEC 14496-10
    PICO_MPEGTS_STREAM_TYPE_TIMELINE_METADATA       = 0x27, // Timeline and External Media Information Stream
    PICO_MPEGTS_STREAM_TYPE_HEVC_TILES              = 0x28, // HEVC enhancement sub-partition which includes TemporalId 0 of an HEVC video stream
    PICO_MPEGTS_STREAM_TYPE_HEVC_TILES_TEMPORAL     = 0x29, // HEVC temporal enhancement sub-partition of an HEVC video stream
    PICO_MPEGTS_STREAM_TYPE_HEVC_TILES_ENHANCEMENT  = 0x2A, // HEVC enhancement sub-partition of an HEVC video stream
    PICO_MPEGTS_STREAM_TYPE_HEVC_TILES_MAIN         = 0x2B, // HEVC enhancement sub-partition which includes TemporalId 0 of an HEVC video stream
    PICO_MPEGTS_STREAM_TYPE_GREEN_ACCESS_UNITS      = 0x2C, // Green access units carried in MPEG-2 sections
    PICO_MPEGTS_STREAM_TYPE_QUALITY_ACCESS_UNITS    = 0x2D, // Quality access units carried in sections
    PICO_MPEGTS_STREAM_TYPE_MEDIA_ORCHESTRATION     = 0x2E, // Media orchestration access units carried in sections
    PICO_MPEGTS_STREAM_TYPE_SUBSTREAM_FOR_MH3D      = 0x2F, // Substream containing HEVC or VVC for MH 3D or related sub-pictures
    PICO_MPEGTS_STREAM_TYPE_VVC                     = 0x33, // VVC video stream or a VVC temporal video sub-bitstream conforming to one or more profiles defined in Annex A of ITU-T H.266 | ISO/IEC 23090-3
    PICO_MPEGTS_STREAM_TYPE_VVC_SUBSET              = 0x34, // VVC temporal video subset of a VVC video stream conforming to one or more profiles defined in Annex A of ITU-T H.266 | ISO/IEC 23090-3
    PICO_MPEGTS_STREAM_TYPE_EVC                     = 0x35, // EVC video stream or an EVC temporal video sub-bitstream conforming to one or more profiles defined in ISO/IEC 23094-1
    PICO_MPEGTS_STREAM_TYPE_LCEVC                   = 0x36, // LCEVC video stream conforming to ISO/IEC 23094-2
    PICO_MPEGTS_STREAM_TYPE_LCEVC_WITH_HEVC         = 0x37, // Carriage of LCEVC within an HEVC video stream conforming to ISO/IEC 23094-2
    PICO_MPEGTS_STREAM_TYPE_LCEVC_WITH_VVC          = 0x38, // Carriage of LCEVC within a VVC video stream conforming to ISO/IEC 23094-2
    PICO_MPEGTS_STREAM_TYPE_CHINESE_VIDEO_STANDARD  = 0x42, // Chinese Video Standard
    PICO_MPEGTS_STREAM_TYPE_CAVS_VIDEO              = 0x93, // CAVS Video (Chinese Video Standard)
    PICO_MPEGTS_STREAM_TYPE_VC1                     = 0xEA, // VC-1 video stream
    PICO_MPEGTS_STREAM_TYPE_DIRAC                   = 0xD1, // Dirac video stream
    PICO_MPEGTS_STREAM_TYPE_AC3                     = 0x81, // AC-3 audio (ATSC)
    PICO_MPEGTS_STREAM_TYPE_DTS                     = 0x82, // DTS audio
    PICO_MPEGTS_STREAM_TYPE_TRUEHD                  = 0x83, // TrueHD audio (ATSC/Blu-ray)
    PICO_MPEGTS_STREAM_TYPE_EAC3                    = 0x87, // E-AC-3 audio (ATSC)
    PICO_MPEGTS_STREAM_TYPE_DTS_HD                  = 0x8A, // DTS-HD audio
    PICO_MPEGTS_STREAM_TYPE_SDDS                    = 0x94, // SDDS audio
    PICO_MPEGTS_STREAM_TYPE_USER_PRIVATE_START      = 0x80,
    PICO_MPEGTS_STREAM_TYPE_USER_PRIVATE_END        = 0xFF,
} picoMpegTSStreamType;

typedef enum {
    PICO_MPEGTS_DESCRIPTOR_TAG_RESERVED_0                      = 0,
    PICO_MPEGTS_DESCRIPTOR_TAG_FORBIDDEN                       = 1,
    PICO_MPEGTS_DESCRIPTOR_TAG_VIDEO_STREAM                    = 2,
    PICO_MPEGTS_DESCRIPTOR_TAG_AUDIO_STREAM                    = 3,
    PICO_MPEGTS_DESCRIPTOR_TAG_HIERARCHY                       = 4,
    PICO_MPEGTS_DESCRIPTOR_TAG_REGISTRATION                    = 5,
    PICO_MPEGTS_DESCRIPTOR_TAG_DATA_STREAM_ALIGNMENT           = 6,
    PICO_MPEGTS_DESCRIPTOR_TAG_TARGET_BACKGROUND_GRID          = 7,
    PICO_MPEGTS_DESCRIPTOR_TAG_VIDEO_WINDOW                    = 8,
    PICO_MPEGTS_DESCRIPTOR_TAG_CA                              = 9,
    PICO_MPEGTS_DESCRIPTOR_TAG_ISO_639_LANGUAGE                = 10,
    PICO_MPEGTS_DESCRIPTOR_TAG_SYSTEM_CLOCK                    = 11,
    PICO_MPEGTS_DESCRIPTOR_TAG_MULTIPLEX_BUFFER_UTILIZATION    = 12,
    PICO_MPEGTS_DESCRIPTOR_TAG_COPYRIGHT                       = 13,
    PICO_MPEGTS_DESCRIPTOR_TAG_MAXIMUM_BITRATE                 = 14,
    PICO_MPEGTS_DESCRIPTOR_TAG_PRIVATE_DATA_INDICATOR          = 15,
    PICO_MPEGTS_DESCRIPTOR_TAG_SMOOTHING_BUFFER                = 16,
    PICO_MPEGTS_DESCRIPTOR_TAG_STD                             = 17,
    PICO_MPEGTS_DESCRIPTOR_TAG_IBP                             = 18, // 19-26 defined in ISO/IEC 13818-6
    PICO_MPEGTS_DESCRIPTOR_TAG_MPEG4_VIDEO                     = 27,
    PICO_MPEGTS_DESCRIPTOR_TAG_MPEG4_AUDIO                     = 28,
    PICO_MPEGTS_DESCRIPTOR_TAG_IOD                             = 29,
    PICO_MPEGTS_DESCRIPTOR_TAG_SL                              = 30,
    PICO_MPEGTS_DESCRIPTOR_TAG_FMC                             = 31,
    PICO_MPEGTS_DESCRIPTOR_TAG_EXTERNAL_ES_ID                  = 32,
    PICO_MPEGTS_DESCRIPTOR_TAG_MUX_CODE                        = 33,
    PICO_MPEGTS_DESCRIPTOR_TAG_M4_MUX_BUFFER_SIZE              = 34,
    PICO_MPEGTS_DESCRIPTOR_TAG_MULTIPLEX_BUFFER                = 35,
    PICO_MPEGTS_DESCRIPTOR_TAG_CONTENT_LABELING                = 36,
    PICO_MPEGTS_DESCRIPTOR_TAG_METADATA_POINTER                = 37,
    PICO_MPEGTS_DESCRIPTOR_TAG_METADATA                        = 38,
    PICO_MPEGTS_DESCRIPTOR_TAG_METADATA_STD                    = 39,
    PICO_MPEGTS_DESCRIPTOR_TAG_AVC_VIDEO                       = 40,
    PICO_MPEGTS_DESCRIPTOR_TAG_IPMP                            = 41,
    PICO_MPEGTS_DESCRIPTOR_TAG_AVC_TIMING_AND_HRD              = 42,
    PICO_MPEGTS_DESCRIPTOR_TAG_MPEG2_AAC_AUDIO                 = 43,
    PICO_MPEGTS_DESCRIPTOR_TAG_M4_MUX_TIMING                   = 44,
    PICO_MPEGTS_DESCRIPTOR_TAG_MPEG4_TEXT                      = 45,
    PICO_MPEGTS_DESCRIPTOR_TAG_MPEG4_AUDIO_EXTENSION           = 46,
    PICO_MPEGTS_DESCRIPTOR_TAG_AUXILIARY_VIDEO_STREAM          = 47,
    PICO_MPEGTS_DESCRIPTOR_TAG_SVC_EXTENSION                   = 48,
    PICO_MPEGTS_DESCRIPTOR_TAG_MVC_EXTENSION                   = 49,
    PICO_MPEGTS_DESCRIPTOR_TAG_J2K_VIDEO                       = 50,
    PICO_MPEGTS_DESCRIPTOR_TAG_MVC_OPERATION_POINT             = 51,
    PICO_MPEGTS_DESCRIPTOR_TAG_MPEG2_STEREOSCOPIC_VIDEO_FORMAT = 52,
    PICO_MPEGTS_DESCRIPTOR_TAG_STEREOSCOPIC_PROGRAM_INFO       = 53,
    PICO_MPEGTS_DESCRIPTOR_TAG_STEREOSCOPIC_VIDEO_INFO         = 54,
    PICO_MPEGTS_DESCRIPTOR_TAG_TRANSPORT_PROFILE               = 55,
    PICO_MPEGTS_DESCRIPTOR_TAG_HEVC_VIDEO                      = 56,
    PICO_MPEGTS_DESCRIPTOR_TAG_VVC_VIDEO                       = 57,
    PICO_MPEGTS_DESCRIPTOR_TAG_EVC_VIDEO                       = 58,
    PICO_MPEGTS_DESCRIPTOR_TAG_EXTENSION                       = 63,
    PICO_MPEGTS_DESCRIPTOR_TAG_NETWORK_NAME                    = 0x40, // DVB Descriptors (ETSI EN 300 468)
    PICO_MPEGTS_DESCRIPTOR_TAG_SERVICE_LIST                    = 0x41,
    PICO_MPEGTS_DESCRIPTOR_TAG_STUFFING                        = 0x42,
    PICO_MPEGTS_DESCRIPTOR_TAG_SATELLITE_DELIVERY_SYSTEM       = 0x43,
    PICO_MPEGTS_DESCRIPTOR_TAG_CABLE_DELIVERY_SYSTEM           = 0x44,
    PICO_MPEGTS_DESCRIPTOR_TAG_VBI_DATA                        = 0x45,
    PICO_MPEGTS_DESCRIPTOR_TAG_VBI_TELETEXT                    = 0x46,
    PICO_MPEGTS_DESCRIPTOR_TAG_BOUQUET_NAME                    = 0x47,
    PICO_MPEGTS_DESCRIPTOR_TAG_SERVICE                         = 0x48,
    PICO_MPEGTS_DESCRIPTOR_TAG_COUNTRY_AVAILABILITY            = 0x49,
    PICO_MPEGTS_DESCRIPTOR_TAG_LINKAGE                         = 0x4A,
    PICO_MPEGTS_DESCRIPTOR_TAG_NVOD_REFERENCE                  = 0x4B,
    PICO_MPEGTS_DESCRIPTOR_TAG_TIME_SHIFTED_SERVICE            = 0x4C,
    PICO_MPEGTS_DESCRIPTOR_TAG_SHORT_EVENT                     = 0x4D,
    PICO_MPEGTS_DESCRIPTOR_TAG_EXTENDED_EVENT                  = 0x4E,
    PICO_MPEGTS_DESCRIPTOR_TAG_TIME_SHIFTED_EVENT              = 0x4F,
    PICO_MPEGTS_DESCRIPTOR_TAG_COMPONENT                       = 0x50,
    PICO_MPEGTS_DESCRIPTOR_TAG_MOSAIC                          = 0x51,
    PICO_MPEGTS_DESCRIPTOR_TAG_STREAM_IDENTIFIER               = 0x52,
    PICO_MPEGTS_DESCRIPTOR_TAG_CA_IDENTIFIER                   = 0x53,
    PICO_MPEGTS_DESCRIPTOR_TAG_CONTENT                         = 0x54,
    PICO_MPEGTS_DESCRIPTOR_TAG_PARENTAL_RATING                 = 0x55,
    PICO_MPEGTS_DESCRIPTOR_TAG_TELETEXT                        = 0x56,
    PICO_MPEGTS_DESCRIPTOR_TAG_TELEPHONE                       = 0x57,
    PICO_MPEGTS_DESCRIPTOR_TAG_LOCAL_TIME_OFFSET               = 0x58,
    PICO_MPEGTS_DESCRIPTOR_TAG_SUBTITLING                      = 0x59,
    PICO_MPEGTS_DESCRIPTOR_TAG_TERRESTRIAL_DELIVERY_SYSTEM     = 0x5A,
    PICO_MPEGTS_DESCRIPTOR_TAG_MULTILINGUAL_NETWORK_NAME       = 0x5B,
    PICO_MPEGTS_DESCRIPTOR_TAG_MULTILINGUAL_BOUQUET_NAME       = 0x5C,
    PICO_MPEGTS_DESCRIPTOR_TAG_MULTILINGUAL_SERVICE_NAME       = 0x5D,
    PICO_MPEGTS_DESCRIPTOR_TAG_MULTILINGUAL_COMPONENT          = 0x5E,
    PICO_MPEGTS_DESCRIPTOR_TAG_PRIVATE_DATA_SPECIFIER          = 0x5F,
    PICO_MPEGTS_DESCRIPTOR_TAG_SERVICE_MOVE                    = 0x60,
    PICO_MPEGTS_DESCRIPTOR_TAG_SHORT_SMOOTHING_BUFFER          = 0x61,
    PICO_MPEGTS_DESCRIPTOR_TAG_FREQUENCY_LIST                  = 0x62,
    PICO_MPEGTS_DESCRIPTOR_TAG_PARTIAL_TRANSPORT_STREAM        = 0x63,
    PICO_MPEGTS_DESCRIPTOR_TAG_DATA_BROADCAST                  = 0x64,
    PICO_MPEGTS_DESCRIPTOR_TAG_SCRAMBLING                      = 0x65,
    PICO_MPEGTS_DESCRIPTOR_TAG_DATA_BROADCAST_ID               = 0x66,
    PICO_MPEGTS_DESCRIPTOR_TAG_TRANSPORT_STREAM                = 0x67,
    PICO_MPEGTS_DESCRIPTOR_TAG_DSNG                            = 0x68,
    PICO_MPEGTS_DESCRIPTOR_TAG_PDC                             = 0x69,
    PICO_MPEGTS_DESCRIPTOR_TAG_AC3                             = 0x6A,
    PICO_MPEGTS_DESCRIPTOR_TAG_ANCILLARY_DATA                  = 0x6B,
    PICO_MPEGTS_DESCRIPTOR_TAG_CELL_LIST                       = 0x6C,
    PICO_MPEGTS_DESCRIPTOR_TAG_CELL_FREQUENCY_LINK             = 0x6D,
    PICO_MPEGTS_DESCRIPTOR_TAG_ANNOUNCEMENT_SUPPORT            = 0x6E,
    PICO_MPEGTS_DESCRIPTOR_TAG_APPLICATION_SIGNALLING          = 0x6F,
    PICO_MPEGTS_DESCRIPTOR_TAG_ADAPTATION_FIELD_DATA           = 0x70,
    PICO_MPEGTS_DESCRIPTOR_TAG_SERVICE_IDENTIFIER              = 0x71,
    PICO_MPEGTS_DESCRIPTOR_TAG_SERVICE_AVAILABILITY            = 0x72,
    PICO_MPEGTS_DESCRIPTOR_TAG_DEFAULT_AUTHORITY               = 0x73,
    PICO_MPEGTS_DESCRIPTOR_TAG_RELATED_CONTENT                 = 0x74,
    PICO_MPEGTS_DESCRIPTOR_TAG_TVA_ID                          = 0x75,
    PICO_MPEGTS_DESCRIPTOR_TAG_CONTENT_IDENTIFIER              = 0x76,
    PICO_MPEGTS_DESCRIPTOR_TAG_TIME_SLICE_FEC_IDENTIFIER       = 0x77,
    PICO_MPEGTS_DESCRIPTOR_TAG_ECM_REPETITION_RATE             = 0x78,
    PICO_MPEGTS_DESCRIPTOR_TAG_S2_SATELLITE_DELIVERY_SYSTEM    = 0x79,
    PICO_MPEGTS_DESCRIPTOR_TAG_ENHANCED_AC3                    = 0x7A,
    PICO_MPEGTS_DESCRIPTOR_TAG_DTS                             = 0x7B,
    PICO_MPEGTS_DESCRIPTOR_TAG_AAC                             = 0x7C,
    PICO_MPEGTS_DESCRIPTOR_TAG_XAIT_LOCATION                   = 0x7D,
    PICO_MPEGTS_DESCRIPTOR_TAG_FTA_CONTENT_MANAGEMENT          = 0x7E,
    PICO_MPEGTS_DESCRIPTOR_TAG_DVB_EXTENSION                   = 0x7F,
    PICO_MPEGTS_DESCRIPTOR_TAG_USER_PRIVATE_START              = 0x80,
    PICO_MPEGTS_DESCRIPTOR_TAG_USER_PRIVATE_END                = 255,
} picoMpegTSDescriptorTag;

typedef enum {
    PICO_MPEGTS_ADAPTATION_FIELD_CONTROL_RESERVED        = 0x00,
    PICO_MPEGTS_ADAPTATION_FIELD_CONTROL_PAYLOAD_ONLY    = 0x01,
    PICO_MPEGTS_ADAPTATION_FIELD_CONTROL_ADAPTATION_ONLY = 0x02,
    PICO_MPEGTS_ADAPTATION_FIELD_CONTROL_BOTH            = 0x03,
} picoMpegTSAdaptionFieldControl;

typedef enum {
    PICO_MPEGTS_RESULT_SUCCESS = 0,
    PICO_MPEGTS_RESULT_FILE_NOT_FOUND,
    PICO_MPEGTS_RESULT_MALLOC_ERROR,
    PICO_MPEGTS_RESULT_INVALID_DATA,
    PICO_MPEGTS_RESULT_INVALID_ARGUMENTS,
    PICO_MPEGTS_RESULT_UNKNOWN_PID_PACKET,
    PICO_MPEGTS_RESULT_TABLE_FULL,
    PICO_MPEGTS_RESULT_UNKNOWN_ERROR,
} picoMpegTSResult;

typedef enum {
    PICO_MPEGTS_SDT_RUNNING_STATUS_UNDEFINED         = 0,
    PICO_MPEGTS_SDT_RUNNING_STATUS_NOT_RUNNING       = 1,
    PICO_MPEGTS_SDT_RUNNING_STATUS_STARTS_IN_FEW_SEC = 2,
    PICO_MPEGTS_SDT_RUNNING_STATUS_PAUSING           = 3,
    PICO_MPEGTS_SDT_RUNNING_STATUS_RUNNING           = 4,
    PICO_MPEGTS_SDT_RUNNING_STATUS_SERVICE_OFF_AIR   = 5,
    PICO_MPEGTS_SDT_RUNNING_STATUS_RESERVED_6        = 6,
    PICO_MPEGTS_SDT_RUNNING_STATUS_RESERVED_7        = 7,
} picoMpegTSSDTRunningStatus;

typedef enum {
    PICO_MPEGTS_SERVICE_TYPE_DIGITAL_TELEVISION                 = 0x01,
    PICO_MPEGTS_SERVICE_TYPE_DIGITAL_RADIO_SOUND                = 0x02,
    PICO_MPEGTS_SERVICE_TYPE_TELETEXT                           = 0x03,
    PICO_MPEGTS_SERVICE_TYPE_NVOD_REFERENCE                     = 0x04,
    PICO_MPEGTS_SERVICE_TYPE_NVOD_TIME_SHIFTED                  = 0x05,
    PICO_MPEGTS_SERVICE_TYPE_MOSAIC                             = 0x06,
    PICO_MPEGTS_SERVICE_TYPE_FM_RADIO                           = 0x07,
    PICO_MPEGTS_SERVICE_TYPE_DVB_SRM                            = 0x08,
    PICO_MPEGTS_SERVICE_TYPE_ADVANCED_CODEC_DIGITAL_RADIO_SOUND = 0x0A,
    PICO_MPEGTS_SERVICE_TYPE_H264_AVC_MOSAIC                    = 0x0B,
    PICO_MPEGTS_SERVICE_TYPE_DATA_BROADCAST                     = 0x0C,
    PICO_MPEGTS_SERVICE_TYPE_DVB_MHP                            = 0x10,
    PICO_MPEGTS_SERVICE_TYPE_MPEG2_HD_DIGITAL_TELEVISION        = 0x11,
    PICO_MPEGTS_SERVICE_TYPE_H264_AVC_SD_DIGITAL_TELEVISION     = 0x16,
    PICO_MPEGTS_SERVICE_TYPE_H264_AVC_SD_NVOD_TIME_SHIFTED      = 0x17,
    PICO_MPEGTS_SERVICE_TYPE_H264_AVC_SD_NVOD_REFERENCE         = 0x18,
    PICO_MPEGTS_SERVICE_TYPE_H264_AVC_HD_DIGITAL_TELEVISION     = 0x19,
    PICO_MPEGTS_SERVICE_TYPE_H264_AVC_HD_NVOD_TIME_SHIFTED      = 0x1A,
    PICO_MPEGTS_SERVICE_TYPE_H264_AVC_HD_NVOD_REFERENCE         = 0x1B,
    PICO_MPEGTS_SERVICE_TYPE_H264_AVC_FCP_HD_DIGITAL_TELEVISION = 0x1C,
    PICO_MPEGTS_SERVICE_TYPE_H264_AVC_FCP_HD_NVOD_TIME_SHIFTED  = 0x1D,
    PICO_MPEGTS_SERVICE_TYPE_H264_AVC_FCP_HD_NVOD_REFERENCE     = 0x1E,
    PICO_MPEGTS_SERVICE_TYPE_HEVC_DIGITAL_TELEVISION            = 0x1F,
    PICO_MPEGTS_SERVICE_TYPE_HEVC_UHD_DIGITAL_TELEVISION        = 0x20,
    PICO_MPEGTS_SERVICE_TYPE_RESERVED_FF                        = 0xFF,
} picoMpegTSServiceType;

typedef enum {
    PICO_MPEGTS_AUDIO_TYPE_UNDEFINED                  = 0x00,
    PICO_MPEGTS_AUDIO_TYPE_CLEAN_EFFECTS              = 0x01,
    PICO_MPEGTS_AUDIO_TYPE_HEARING_IMPAIRED           = 0x02,
    PICO_MPEGTS_AUDIO_TYPE_VISUAL_IMPAIRED_COMMENTARY = 0x03, // 0x04 - 0x7F User Private
    PICO_MPEGTS_AUDIO_TYPE_PRIMARY                    = 0x80,
    PICO_MPEGTS_AUDIO_TYPE_NATIVE                     = 0x81,
    PICO_MPEGTS_AUDIO_TYPE_EMERGENCY                  = 0x82,
    PICO_MPEGTS_AUDIO_TYPE_PRIMARY_COMMENTARY         = 0x83,
    PICO_MPEGTS_AUDIO_TYPE_ALTERNATE_COMMENTARY       = 0x84,
} picoMpegTSAudioType;

typedef enum {
    PICO_MPEGTS_CONTENT_NIBBLE_LEVEL_1_UNDEFINED         = 0x0,
    PICO_MPEGTS_CONTENT_NIBBLE_LEVEL_1_MOVIE_DRAMA       = 0x1,
    PICO_MPEGTS_CONTENT_NIBBLE_LEVEL_1_NEWS_CURRENT      = 0x2,
    PICO_MPEGTS_CONTENT_NIBBLE_LEVEL_1_SHOW_GAME         = 0x3,
    PICO_MPEGTS_CONTENT_NIBBLE_LEVEL_1_SPORTS            = 0x4,
    PICO_MPEGTS_CONTENT_NIBBLE_LEVEL_1_CHILDREN_YOUTH    = 0x5,
    PICO_MPEGTS_CONTENT_NIBBLE_LEVEL_1_MUSIC_BALLET      = 0x6,
    PICO_MPEGTS_CONTENT_NIBBLE_LEVEL_1_ARTS_CULTURE      = 0x7,
    PICO_MPEGTS_CONTENT_NIBBLE_LEVEL_1_SOCIAL_POLITICAL  = 0x8,
    PICO_MPEGTS_CONTENT_NIBBLE_LEVEL_1_EDUCATION_SCIENCE = 0x9,
    PICO_MPEGTS_CONTENT_NIBBLE_LEVEL_1_LEISURE_HOBBIES   = 0xA,
    PICO_MPEGTS_CONTENT_NIBBLE_LEVEL_1_SPECIAL           = 0xB,
    PICO_MPEGTS_CONTENT_NIBBLE_LEVEL_1_ADULT             = 0xC,
    PICO_MPEGTS_CONTENT_NIBBLE_LEVEL_1_RESERVED_E        = 0xE,
    PICO_MPEGTS_CONTENT_NIBBLE_LEVEL_1_USER_DEFINED      = 0xF,
} picoMpegTSContentNibbleLevel1;

typedef enum {
    PICO_MPEGTS_CONTENT_UNDEFINED = 0x00,

    // Movie/Drama (0x1)
    PICO_MPEGTS_CONTENT_MOVIE_DRAMA_GENERAL                  = 0x10,
    PICO_MPEGTS_CONTENT_MOVIE_DETECTIVE_THRILLER             = 0x11,
    PICO_MPEGTS_CONTENT_MOVIE_ADVENTURE_WESTERN_WAR          = 0x12,
    PICO_MPEGTS_CONTENT_MOVIE_SCIENCE_FICTION_FANTASY_HORROR = 0x13,
    PICO_MPEGTS_CONTENT_MOVIE_COMEDY                         = 0x14,
    PICO_MPEGTS_CONTENT_MOVIE_SOAP_MELODRAMA_FOLKLORIC       = 0x15,
    PICO_MPEGTS_CONTENT_MOVIE_ROMANCE                        = 0x16,
    PICO_MPEGTS_CONTENT_MOVIE_SERIOUS_CLASSICAL_RELIGIOUS    = 0x17,
    PICO_MPEGTS_CONTENT_MOVIE_ADULT_MOVIE_DRAMA              = 0x18,
    PICO_MPEGTS_CONTENT_MOVIE_USER_DEFINED                   = 0x1F,

    // News/Current affairs (0x2)
    PICO_MPEGTS_CONTENT_NEWS_GENERAL                     = 0x20,
    PICO_MPEGTS_CONTENT_NEWS_WEATHER_REPORT              = 0x21,
    PICO_MPEGTS_CONTENT_NEWS_MAGAZINE                    = 0x22,
    PICO_MPEGTS_CONTENT_NEWS_DOCUMENTARY                 = 0x23,
    PICO_MPEGTS_CONTENT_NEWS_DISCUSSION_INTERVIEW_DEBATE = 0x24,
    PICO_MPEGTS_CONTENT_NEWS_USER_DEFINED                = 0x2F,

    // Show/Game show (0x3)
    PICO_MPEGTS_CONTENT_SHOW_GENERAL                = 0x30,
    PICO_MPEGTS_CONTENT_SHOW_GAME_SHOW_QUIZ_CONTEST = 0x31,
    PICO_MPEGTS_CONTENT_SHOW_VARIETY_SHOW           = 0x32,
    PICO_MPEGTS_CONTENT_SHOW_TALK_SHOW              = 0x33,
    PICO_MPEGTS_CONTENT_SHOW_USER_DEFINED           = 0x3F,

    // Sports (0x4)
    PICO_MPEGTS_CONTENT_SPORTS_GENERAL         = 0x40,
    PICO_MPEGTS_CONTENT_SPORTS_SPECIAL_EVENTS  = 0x41,
    PICO_MPEGTS_CONTENT_SPORTS_MAGAZINES       = 0x42,
    PICO_MPEGTS_CONTENT_SPORTS_FOOTBALL_SOCCER = 0x43,
    PICO_MPEGTS_CONTENT_SPORTS_TENNIS_SQUASH   = 0x44,
    PICO_MPEGTS_CONTENT_SPORTS_TEAM_SPORTS     = 0x45,
    PICO_MPEGTS_CONTENT_SPORTS_ATHLETICS       = 0x46,
    PICO_MPEGTS_CONTENT_SPORTS_MOTOR_SPORT     = 0x47,
    PICO_MPEGTS_CONTENT_SPORTS_WATER_SPORT     = 0x48,
    PICO_MPEGTS_CONTENT_SPORTS_WINTER_SPORTS   = 0x49,
    PICO_MPEGTS_CONTENT_SPORTS_EQUESTRIAN      = 0x4A,
    PICO_MPEGTS_CONTENT_SPORTS_MARTIAL_SPORTS  = 0x4B,
    PICO_MPEGTS_CONTENT_SPORTS_USER_DEFINED    = 0x4F,

    // Children's/Youth programmes (0x5)
    PICO_MPEGTS_CONTENT_CHILDREN_GENERAL             = 0x50,
    PICO_MPEGTS_CONTENT_CHILDREN_PRE_SCHOOL          = 0x51,
    PICO_MPEGTS_CONTENT_CHILDREN_ENTERTAINMENT_6_14  = 0x52,
    PICO_MPEGTS_CONTENT_CHILDREN_ENTERTAINMENT_10_16 = 0x53,
    PICO_MPEGTS_CONTENT_CHILDREN_INFORMATIONAL       = 0x54,
    PICO_MPEGTS_CONTENT_CHILDREN_CARTOONS_PUPPETS    = 0x55,
    PICO_MPEGTS_CONTENT_CHILDREN_USER_DEFINED        = 0x5F,

    // Music/Ballet/Dance (0x6)
    PICO_MPEGTS_CONTENT_MUSIC_GENERAL          = 0x60,
    PICO_MPEGTS_CONTENT_MUSIC_ROCK_POP         = 0x61,
    PICO_MPEGTS_CONTENT_MUSIC_CLASSICAL        = 0x62,
    PICO_MPEGTS_CONTENT_MUSIC_FOLK_TRADITIONAL = 0x63,
    PICO_MPEGTS_CONTENT_MUSIC_JAZZ             = 0x64,
    PICO_MPEGTS_CONTENT_MUSIC_MUSICAL_OPERA    = 0x65,
    PICO_MPEGTS_CONTENT_MUSIC_BALLET           = 0x66,
    PICO_MPEGTS_CONTENT_MUSIC_USER_DEFINED     = 0x6F,

    // Arts/Culture (0x7)
    PICO_MPEGTS_CONTENT_ARTS_GENERAL                 = 0x70,
    PICO_MPEGTS_CONTENT_ARTS_PERFORMING_ARTS         = 0x71,
    PICO_MPEGTS_CONTENT_ARTS_FINE_ARTS               = 0x72,
    PICO_MPEGTS_CONTENT_ARTS_RELIGION                = 0x73,
    PICO_MPEGTS_CONTENT_ARTS_POPULAR_CULTURE         = 0x74,
    PICO_MPEGTS_CONTENT_ARTS_LITERATURE              = 0x75,
    PICO_MPEGTS_CONTENT_ARTS_FILM_CINEMA             = 0x76,
    PICO_MPEGTS_CONTENT_ARTS_EXPERIMENTAL_FILM_VIDEO = 0x77,
    PICO_MPEGTS_CONTENT_ARTS_BROADCASTING_PRESS      = 0x78,
    PICO_MPEGTS_CONTENT_ARTS_NEW_MEDIA               = 0x79,
    PICO_MPEGTS_CONTENT_ARTS_MAGAZINES               = 0x7A,
    PICO_MPEGTS_CONTENT_ARTS_FASHION                 = 0x7B,
    PICO_MPEGTS_CONTENT_ARTS_USER_DEFINED            = 0x7F,

    // Social/Political issues/Economics (0x8)
    PICO_MPEGTS_CONTENT_SOCIAL_GENERAL            = 0x80,
    PICO_MPEGTS_CONTENT_SOCIAL_MAGAZINES_REPORTS  = 0x81,
    PICO_MPEGTS_CONTENT_SOCIAL_ECONOMICS_ADVISORY = 0x82,
    PICO_MPEGTS_CONTENT_SOCIAL_REMARKABLE_PEOPLE  = 0x83,
    PICO_MPEGTS_CONTENT_SOCIAL_USER_DEFINED       = 0x8F,

    // Education/Science/Factual topics (0x9)
    PICO_MPEGTS_CONTENT_EDUCATION_GENERAL           = 0x90,
    PICO_MPEGTS_CONTENT_EDUCATION_NATURE_ANIMALS    = 0x91,
    PICO_MPEGTS_CONTENT_EDUCATION_TECHNOLOGY        = 0x92,
    PICO_MPEGTS_CONTENT_EDUCATION_MEDICINE          = 0x93,
    PICO_MPEGTS_CONTENT_EDUCATION_FOREIGN_COUNTRIES = 0x94,
    PICO_MPEGTS_CONTENT_EDUCATION_SOCIAL_SPIRITUAL  = 0x95,
    PICO_MPEGTS_CONTENT_EDUCATION_FURTHER_EDUCATION = 0x96,
    PICO_MPEGTS_CONTENT_EDUCATION_LANGUAGES         = 0x97,
    PICO_MPEGTS_CONTENT_EDUCATION_USER_DEFINED      = 0x9F,

    // Leisure hobbies (0xA)
    PICO_MPEGTS_CONTENT_LEISURE_GENERAL                = 0xA0,
    PICO_MPEGTS_CONTENT_LEISURE_TOURISM_TRAVEL         = 0xA1,
    PICO_MPEGTS_CONTENT_LEISURE_HANDICRAFT             = 0xA2,
    PICO_MPEGTS_CONTENT_LEISURE_MOTORING               = 0xA3,
    PICO_MPEGTS_CONTENT_LEISURE_FITNESS_HEALTH         = 0xA4,
    PICO_MPEGTS_CONTENT_LEISURE_COOKING                = 0xA5,
    PICO_MPEGTS_CONTENT_LEISURE_ADVERTISEMENT_SHOPPING = 0xA6,
    PICO_MPEGTS_CONTENT_LEISURE_GARDENING              = 0xA7,
    PICO_MPEGTS_CONTENT_LEISURE_USER_DEFINED           = 0xAF,

    // Special characteristics (0xB)
    PICO_MPEGTS_CONTENT_SPECIAL_ORIGINAL_LANGUAGE  = 0xB0,
    PICO_MPEGTS_CONTENT_SPECIAL_BLACK_AND_WHITE    = 0xB1,
    PICO_MPEGTS_CONTENT_SPECIAL_UNPUBLISHED        = 0xB2,
    PICO_MPEGTS_CONTENT_SPECIAL_LIVE_BROADCAST     = 0xB3,
    PICO_MPEGTS_CONTENT_SPECIAL_PLANO_STEREOSCOPIC = 0xB4,
    PICO_MPEGTS_CONTENT_SPECIAL_LOCAL_OR_REGIONAL  = 0xB5,
    PICO_MPEGTS_CONTENT_SPECIAL_USER_DEFINED       = 0xBF,

    // Adult (0xC)
    PICO_MPEGTS_CONTENT_ADULT_GENERAL      = 0xC0,
    PICO_MPEGTS_CONTENT_ADULT_USER_DEFINED = 0xCF,
} picoMpegTSContentNibble;

typedef enum {
    PICO_MPEGTS_COMPONENT_STREAM_CONTENT_RESERVED     = 0x0,
    PICO_MPEGTS_COMPONENT_STREAM_CONTENT_MPEG2_VIDEO  = 0x1,
    PICO_MPEGTS_COMPONENT_STREAM_CONTENT_MPEG1_AUDIO  = 0x2, // MPEG-1 Layer 2 audio
    PICO_MPEGTS_COMPONENT_STREAM_CONTENT_SUBTITLES    = 0x3,
    PICO_MPEGTS_COMPONENT_STREAM_CONTENT_AC3_AUDIO    = 0x4,
    PICO_MPEGTS_COMPONENT_STREAM_CONTENT_AVC_VIDEO    = 0x5,
    PICO_MPEGTS_COMPONENT_STREAM_CONTENT_HE_AAC_AUDIO = 0x6,
    PICO_MPEGTS_COMPONENT_STREAM_CONTENT_DTS_AUDIO    = 0x7,
    PICO_MPEGTS_COMPONENT_STREAM_CONTENT_DVB_SRM      = 0x8,
    PICO_MPEGTS_COMPONENT_STREAM_CONTENT_HEVC_VIDEO   = 0x9,
    PICO_MPEGTS_COMPONENT_STREAM_CONTENT_RESERVED_A   = 0xA,
    PICO_MPEGTS_COMPONENT_STREAM_CONTENT_RESERVED_B   = 0xB,
    PICO_MPEGTS_COMPONENT_STREAM_CONTENT_RESERVED_C   = 0xC,
    PICO_MPEGTS_COMPONENT_STREAM_CONTENT_RESERVED_D   = 0xD,
    PICO_MPEGTS_COMPONENT_STREAM_CONTENT_RESERVED_E   = 0xE,
    PICO_MPEGTS_COMPONENT_STREAM_CONTENT_EXTENSION    = 0xF, // Uses stream_content_ext for type
} picoMpegTSComponentStreamContent;

typedef enum {
    // Reserved (stream_content = 0x0)
    PICO_MPEGTS_COMPONENT_TYPE_RESERVED = 0x000,

    // MPEG-2 video (stream_content = 0x1, component_type 0x00-0x10)
    PICO_MPEGTS_COMPONENT_TYPE_MPEG2_VIDEO_RESERVED            = 0x100,
    PICO_MPEGTS_COMPONENT_TYPE_MPEG2_VIDEO_4_3_ASPECT_25HZ     = 0x101,
    PICO_MPEGTS_COMPONENT_TYPE_MPEG2_VIDEO_16_9_PAN_25HZ       = 0x102,
    PICO_MPEGTS_COMPONENT_TYPE_MPEG2_VIDEO_16_9_NO_PAN_25HZ    = 0x103,
    PICO_MPEGTS_COMPONENT_TYPE_MPEG2_VIDEO_GT_16_9_25HZ        = 0x104,
    PICO_MPEGTS_COMPONENT_TYPE_MPEG2_VIDEO_4_3_ASPECT_30HZ     = 0x105,
    PICO_MPEGTS_COMPONENT_TYPE_MPEG2_VIDEO_16_9_PAN_30HZ       = 0x106,
    PICO_MPEGTS_COMPONENT_TYPE_MPEG2_VIDEO_16_9_NO_PAN_30HZ    = 0x107,
    PICO_MPEGTS_COMPONENT_TYPE_MPEG2_VIDEO_GT_16_9_30HZ        = 0x108,
    PICO_MPEGTS_COMPONENT_TYPE_MPEG2_VIDEO_HD_4_3_ASPECT_25HZ  = 0x109,
    PICO_MPEGTS_COMPONENT_TYPE_MPEG2_VIDEO_HD_16_9_PAN_25HZ    = 0x10A,
    PICO_MPEGTS_COMPONENT_TYPE_MPEG2_VIDEO_HD_16_9_NO_PAN_25HZ = 0x10B,
    PICO_MPEGTS_COMPONENT_TYPE_MPEG2_VIDEO_HD_GT_16_9_25HZ     = 0x10C,
    PICO_MPEGTS_COMPONENT_TYPE_MPEG2_VIDEO_HD_4_3_ASPECT_30HZ  = 0x10D,
    PICO_MPEGTS_COMPONENT_TYPE_MPEG2_VIDEO_HD_16_9_PAN_30HZ    = 0x10E,
    PICO_MPEGTS_COMPONENT_TYPE_MPEG2_VIDEO_HD_16_9_NO_PAN_30HZ = 0x10F,
    PICO_MPEGTS_COMPONENT_TYPE_MPEG2_VIDEO_HD_GT_16_9_30HZ     = 0x110,

    // MPEG-1 Layer 2 audio (stream_content = 0x2, component_type 0x00-0x47)
    PICO_MPEGTS_COMPONENT_TYPE_MPEG1_AUDIO_RESERVED                 = 0x200,
    PICO_MPEGTS_COMPONENT_TYPE_MPEG1_AUDIO_SINGLE_MONO              = 0x201,
    PICO_MPEGTS_COMPONENT_TYPE_MPEG1_AUDIO_DUAL_MONO                = 0x202,
    PICO_MPEGTS_COMPONENT_TYPE_MPEG1_AUDIO_STEREO                   = 0x203,
    PICO_MPEGTS_COMPONENT_TYPE_MPEG1_AUDIO_MULTI_LINGUAL_MULTI_CHAN = 0x204,
    PICO_MPEGTS_COMPONENT_TYPE_MPEG1_AUDIO_SURROUND_SOUND           = 0x205,
    PICO_MPEGTS_COMPONENT_TYPE_MPEG1_AUDIO_VISUALLY_IMPAIRED        = 0x240,
    PICO_MPEGTS_COMPONENT_TYPE_MPEG1_AUDIO_HARD_OF_HEARING          = 0x241,
    PICO_MPEGTS_COMPONENT_TYPE_MPEG1_AUDIO_SUPPLEMENTARY            = 0x242,
    PICO_MPEGTS_COMPONENT_TYPE_MPEG1_AUDIO_RECEIVER_MIX             = 0x247,

    // Subtitling (stream_content = 0x3, component_type 0x00-0x24)
    PICO_MPEGTS_COMPONENT_TYPE_SUBTITLES_RESERVED                   = 0x300,
    PICO_MPEGTS_COMPONENT_TYPE_SUBTITLES_EBU_TELETEXT               = 0x301,
    PICO_MPEGTS_COMPONENT_TYPE_SUBTITLES_EBU_TELETEXT_ASSOC         = 0x302,
    PICO_MPEGTS_COMPONENT_TYPE_SUBTITLES_EBU_TELETEXT_VBI           = 0x303,
    PICO_MPEGTS_COMPONENT_TYPE_SUBTITLES_DVB_NORMAL_NO_AR           = 0x310,
    PICO_MPEGTS_COMPONENT_TYPE_SUBTITLES_DVB_NORMAL_4_3             = 0x311,
    PICO_MPEGTS_COMPONENT_TYPE_SUBTITLES_DVB_NORMAL_16_9            = 0x312,
    PICO_MPEGTS_COMPONENT_TYPE_SUBTITLES_DVB_NORMAL_2_21_1          = 0x313,
    PICO_MPEGTS_COMPONENT_TYPE_SUBTITLES_DVB_NORMAL_HD              = 0x314,
    PICO_MPEGTS_COMPONENT_TYPE_SUBTITLES_DVB_HARD_OF_HEARING_NO_AR  = 0x320,
    PICO_MPEGTS_COMPONENT_TYPE_SUBTITLES_DVB_HARD_OF_HEARING_4_3    = 0x321,
    PICO_MPEGTS_COMPONENT_TYPE_SUBTITLES_DVB_HARD_OF_HEARING_16_9   = 0x322,
    PICO_MPEGTS_COMPONENT_TYPE_SUBTITLES_DVB_HARD_OF_HEARING_2_21_1 = 0x323,
    PICO_MPEGTS_COMPONENT_TYPE_SUBTITLES_DVB_HARD_OF_HEARING_HD     = 0x324,

    // AC-3 audio (stream_content = 0x4)
    PICO_MPEGTS_COMPONENT_TYPE_AC3_RESERVED = 0x400,

    // H.264/AVC video (stream_content = 0x5, component_type 0x00-0x10)
    PICO_MPEGTS_COMPONENT_TYPE_AVC_VIDEO_RESERVED            = 0x500,
    PICO_MPEGTS_COMPONENT_TYPE_AVC_VIDEO_SD_4_3_25HZ         = 0x501,
    PICO_MPEGTS_COMPONENT_TYPE_AVC_VIDEO_SD_16_9_PAN_25HZ    = 0x502,
    PICO_MPEGTS_COMPONENT_TYPE_AVC_VIDEO_SD_16_9_NO_PAN_25HZ = 0x503,
    PICO_MPEGTS_COMPONENT_TYPE_AVC_VIDEO_SD_GT_16_9_25HZ     = 0x504,
    PICO_MPEGTS_COMPONENT_TYPE_AVC_VIDEO_SD_4_3_30HZ         = 0x505,
    PICO_MPEGTS_COMPONENT_TYPE_AVC_VIDEO_SD_16_9_PAN_30HZ    = 0x506,
    PICO_MPEGTS_COMPONENT_TYPE_AVC_VIDEO_SD_16_9_NO_PAN_30HZ = 0x507,
    PICO_MPEGTS_COMPONENT_TYPE_AVC_VIDEO_SD_GT_16_9_30HZ     = 0x508,
    PICO_MPEGTS_COMPONENT_TYPE_AVC_VIDEO_HD_4_3_25HZ         = 0x509,
    PICO_MPEGTS_COMPONENT_TYPE_AVC_VIDEO_HD_16_9_PAN_25HZ    = 0x50A,
    PICO_MPEGTS_COMPONENT_TYPE_AVC_VIDEO_HD_16_9_NO_PAN_25HZ = 0x50B,
    PICO_MPEGTS_COMPONENT_TYPE_AVC_VIDEO_HD_GT_16_9_25HZ     = 0x50C,
    PICO_MPEGTS_COMPONENT_TYPE_AVC_VIDEO_HD_4_3_30HZ         = 0x50D,
    PICO_MPEGTS_COMPONENT_TYPE_AVC_VIDEO_HD_16_9_PAN_30HZ    = 0x50E,
    PICO_MPEGTS_COMPONENT_TYPE_AVC_VIDEO_HD_16_9_NO_PAN_30HZ = 0x50F,
    PICO_MPEGTS_COMPONENT_TYPE_AVC_VIDEO_HD_GT_16_9_30HZ     = 0x510,

    // HE AAC audio (stream_content = 0x6, component_type 0x00-0x47)
    PICO_MPEGTS_COMPONENT_TYPE_HE_AAC_RESERVED                 = 0x600,
    PICO_MPEGTS_COMPONENT_TYPE_HE_AAC_SINGLE_MONO              = 0x601,
    PICO_MPEGTS_COMPONENT_TYPE_HE_AAC_DUAL_MONO                = 0x602,
    PICO_MPEGTS_COMPONENT_TYPE_HE_AAC_STEREO                   = 0x603,
    PICO_MPEGTS_COMPONENT_TYPE_HE_AAC_MULTI_LINGUAL_MULTI_CHAN = 0x604,
    PICO_MPEGTS_COMPONENT_TYPE_HE_AAC_SURROUND_SOUND           = 0x605,
    PICO_MPEGTS_COMPONENT_TYPE_HE_AAC_VISUALLY_IMPAIRED        = 0x640,
    PICO_MPEGTS_COMPONENT_TYPE_HE_AAC_HARD_OF_HEARING          = 0x641,
    PICO_MPEGTS_COMPONENT_TYPE_HE_AAC_SUPPLEMENTARY            = 0x642,
    PICO_MPEGTS_COMPONENT_TYPE_HE_AAC_HEV2_SINGLE_MONO         = 0x643,
    PICO_MPEGTS_COMPONENT_TYPE_HEV2_AAC_DUAL_MONO              = 0x644,
    PICO_MPEGTS_COMPONENT_TYPE_HEV2_AAC_STEREO                 = 0x645,
    PICO_MPEGTS_COMPONENT_TYPE_HE_AAC_RECEIVER_MIX             = 0x647,

    // DTS audio (stream_content = 0x7)
    PICO_MPEGTS_COMPONENT_TYPE_DTS_RESERVED = 0x700,

    // DVB SRM (stream_content = 0x8)
    PICO_MPEGTS_COMPONENT_TYPE_SRM_RESERVED = 0x800,

    // HEVC video (stream_content = 0x9, component_type 0x00-0x08)
    PICO_MPEGTS_COMPONENT_TYPE_HEVC_VIDEO_RESERVED         = 0x900,
    PICO_MPEGTS_COMPONENT_TYPE_HEVC_VIDEO_MAIN_HD_50HZ     = 0x901,
    PICO_MPEGTS_COMPONENT_TYPE_HEVC_VIDEO_MAIN_HD_60HZ     = 0x902,
    PICO_MPEGTS_COMPONENT_TYPE_HEVC_VIDEO_MAIN_10_HD_50HZ  = 0x903,
    PICO_MPEGTS_COMPONENT_TYPE_HEVC_VIDEO_MAIN_10_HD_60HZ  = 0x904,
    PICO_MPEGTS_COMPONENT_TYPE_HEVC_VIDEO_UHD_50HZ_SDR     = 0x905,
    PICO_MPEGTS_COMPONENT_TYPE_HEVC_VIDEO_UHD_60_120HZ_SDR = 0x906,
    PICO_MPEGTS_COMPONENT_TYPE_HEVC_VIDEO_UHD_50HZ_HDR     = 0x907,
    PICO_MPEGTS_COMPONENT_TYPE_HEVC_VIDEO_UHD_60_120HZ_HDR = 0x908,

    // Extension types (stream_content = 0xF, stream_content_ext varies)
    // Format: 0xFECC where E = stream_content_ext (0-F), CC = component_type
    PICO_MPEGTS_COMPONENT_TYPE_EXT_RESERVED = 0xF000,

    // AC-4 audio extension (stream_content_ext = 0x0)
    PICO_MPEGTS_COMPONENT_TYPE_AC4_MAIN_MONO                        = 0xF000,
    PICO_MPEGTS_COMPONENT_TYPE_AC4_MAIN_STEREO                      = 0xF001,
    PICO_MPEGTS_COMPONENT_TYPE_AC4_MAIN_STEREO_DIALOGUE_ENHANCEMENT = 0xF002,
    PICO_MPEGTS_COMPONENT_TYPE_AC4_MAIN_STEREO_VISUALLY_IMPAIRED    = 0xF003,
    PICO_MPEGTS_COMPONENT_TYPE_AC4_MAIN_STEREO_HARD_OF_HEARING      = 0xF004,
    PICO_MPEGTS_COMPONENT_TYPE_AC4_MAIN_MULTI_CHANNEL               = 0xF005,
    PICO_MPEGTS_COMPONENT_TYPE_AC4_BCAST_MIX_MONO                   = 0xF006,
    PICO_MPEGTS_COMPONENT_TYPE_AC4_BCAST_MIX_STEREO                 = 0xF007,
    PICO_MPEGTS_COMPONENT_TYPE_AC4_BCAST_MIX_STEREO_DIALOGUE        = 0xF008,
    PICO_MPEGTS_COMPONENT_TYPE_AC4_BCAST_MIX_STEREO_VI              = 0xF009,
    PICO_MPEGTS_COMPONENT_TYPE_AC4_BCAST_MIX_STEREO_HOH             = 0xF00A,
    PICO_MPEGTS_COMPONENT_TYPE_AC4_BCAST_MIX_MULTI_CHANNEL          = 0xF00B,
    PICO_MPEGTS_COMPONENT_TYPE_AC4_RECEIVER_MIX_MONO                = 0xF00C,
    PICO_MPEGTS_COMPONENT_TYPE_AC4_RECEIVER_MIX_STEREO              = 0xF00D,

    // NGA component type extension (stream_content_ext = 0xE)
    PICO_MPEGTS_COMPONENT_TYPE_NGA_CONTENT_LESS_16_9   = 0xFE00,
    PICO_MPEGTS_COMPONENT_TYPE_NGA_CONTENT_16_9        = 0xFE01,
    PICO_MPEGTS_COMPONENT_TYPE_NGA_CONTENT_GT_16_9     = 0xFE02,
    PICO_MPEGTS_COMPONENT_TYPE_NGA_STEREOSCOPIC_TAB    = 0xFE03,
    PICO_MPEGTS_COMPONENT_TYPE_NGA_HLG10_HDR           = 0xFE04,
    PICO_MPEGTS_COMPONENT_TYPE_NGA_HEVC_TEMPORAL_VIDEO = 0xFE05,
} picoMpegTSComponentType;

typedef struct picoMpegTS_t picoMpegTS_t;
typedef picoMpegTS_t *picoMpegTS;

typedef struct {
    // Also called the Legal Time Window flag. This is a 1-bit field
    // which when set to '1' indicates the presence of the ltw_offset field.
    bool ltwFlag;
    // This is a 1-bit field which when set to '1' indicates that
    // the value of the ltw_offset shall be valid.
    // A value of '0' indicates that the value in
    // the ltw_offset field is undefined.
    bool ltwValidFlag;
    // This is a 15-bit field, the value of which is defined
    // only if the ltw_valid flag has a value of '1'
    uint16_t ltwOffset;

    // This is a 1-bit field which when set to '1'
    // indicates the presence of the piecewise_rate field.
    bool piecewiseRateFlag;

    // The meaning of this 22-bit field is only defined
    // when both the ltw_flag and the ltw_valid_flag are set
    // to '1'. When defined, it is a positive integer specifying
    // a hypothetical bitrate R which is used to define the
    // end times of the Legal Time Windows of transport stream
    // packets of the same PID that follow this packet but do
    // not include the legal_time_window_offset field.
    uint32_t piecewiseRate;

    // This is a 1-bit flag which when set to '1' indicates that
    // the splice_type and DTS_next_AU fields are present.
    // A value of '0' indicates that neither splice_type nor
    // DTS_next_AU fields are present. This field shall be set
    // to '0' in transport stream packets in which the
    // splicing_point_flag is set to '0'. Once it is set
    // to '1' in a transport stream packet in which the
    // splice_countdown is positive, it shall be set to '1'
    // in all the subsequent transport stream packets of the
    // same PID that have the splicing_point_flag set to '1',
    // until the packet in which the splice_countdown reaches zero
    // (including this packet).
    bool seamlessSpliceFlag;

    // This is a 4-bit field. From the first occurrence of this
    // field onwards, it shall have the same value in all the
    // subsequent transport stream packets of the same PID in
    // which it is present, until the packet in which the splice_countdown
    // reaches zero (including this packet). If the elementary stream
    // carried in that PID is not a Rec. ITU-T H.262 | ISO/IEC13818-2 video
    // stream, then this field shall have the value '1111' (unspecified).
    uint8_t spliceType;
    uint64_t dtsNextAU;

    // This 1-bit field when set to '0' signals the presence of one or
    // several af_descriptor() constructs in the adaptation header.
    // When this flag is set to '1' it indicates that the
    // af_descriptor() is not present in the adaptation header.
    bool afDescriptorNotPresentFlag;
} picoMpegTSPacketAdaptionFieldExtension_t;
typedef picoMpegTSPacketAdaptionFieldExtension_t *picoMpegTSAdaptionFieldExtension;

typedef struct {
    // Clock reference base
    uint64_t base;

    // Clock reference extension
    uint16_t extension;
} picoMpegTSAdaptionFieldClockReference_t;
typedef picoMpegTSAdaptionFieldClockReference_t *picoMpegTSAdaptionFieldPCR_t;

typedef struct {
    // This is a 1-bit field which when set to '1' indicates
    // that the discontinuity state is true for the current
    // transport stream packet. When the discontinuity_indicator
    // is set to '0' or is not present, the discontinuity state is
    // false. The discontinuity indicator is used to indicate
    // two types of discontinuities, system time-base
    // discontinuities and continuity_counter discontinuities.
    bool discontinuityIndicator;

    // The random_access_indicator is a 1-bit field that indicates
    // that the current transport stream packet, and possibly
    // subsequent transport stream packets with the same PID,
    // contain some information to aid random access at this point.
    bool randomAccessIndicator;

    // The elementary_stream_priority_indicator is a 1-bit field.
    // It indicates, among packets with the same PID, the priority
    // of the elementary stream data carried within the payload
    // of this transport stream packet. A '1' indicates that the
    // payload has a higher priority than the payloads of other
    // transport stream packets.
    bool elementaryStreamPriorityIndicator;

    // The PCR_flag is a 1-bit flag. A value of '1' indicates
    // that the adaptation_field contains a PCR field coded in
    // two parts. A value of '0' indicates that the adaptation
    // field does not contain any PCR field.
    bool pcrFlag;

    // The program_clock_reference (PCR) is a 42-bit field coded in two parts.
    // The first part, program_clock_reference_base, is a 33-bit field
    // whose value is given by PCR_base(i), as given in 1.
    // The second part, program_clock_reference_extension, is a 9-bit field whose value
    // is given by PCR_ext(i), as given in 2. The PCR indicates the intended
    // time of arrival of the byte containing the last bit of the
    // program_clock_reference_base at the input of the system target decoder.
    // Equation 1: PCR_base(i) = ((system_clock_frequency * t(i))DIV 300) MOD 2^33
    // Equation 2: PCR_ext(i) = (system_clock_frequency * t(i)DIV 1) MOD 300
    picoMpegTSAdaptionFieldClockReference_t pcr;

    // The OPCR_flag is a 1-bit flag. A value of '1' indicates
    // that the adaptation_field contains an OPCR field coded in
    // two parts. A value of '0' indicates that the adaptation
    // field does not contain any OPCR field.
    bool opcrFlag;

    // The optional original program reference (OPCR) is a 42-bit field
    // coded in two parts. These two parts, the base and the extension, are coded
    // identically to the two corresponding parts of the PCR field.
    // The presence of the OPCR is indicated by the OPCR_flag.
    picoMpegTSAdaptionFieldClockReference_t opcr;

    // The splicing_point_flag is a 1-bit flag. When set to '1',
    // a splice_countdown field shall be present in this adaptation
    // field, specifying the occurrence of a splicing point.
    // A value of '0' indicates that a splice_countdown
    // field is not present in the adaptation field.
    bool splicingPointFlag;

    // The splice_countdown is an 8-bit field, representing a value
    // which may be positive or negative. A positive value specifies the
    // remaining number of transport stream packets, of the same PID,
    // following the associated transport stream packet until a splicing
    // point is reached. Duplicate transport stream packets and transport
    // stream packets which only contain adaptation fields are excluded.
    // The splicing point is located immediately after the last byte of the
    // transport stream packet in which the associated splice_countdown field
    // reaches zero. In the transport stream packet where the splice_countdown
    // reaches zero, the last data byte of the transport stream packet
    // payload shall be the last byte of a coded audio frame or a coded picture.
    // In the case of video, the corresponding access unit may or
    // may not be terminated by a sequence_end_code. Transport stream
    // packets with the same PID, which follow, may contain data from a different
    // elementary stream of the same type.
    uint8_t spliceCountdown;

    // The transport_private_data_flag is a 1-bit flag.
    // A value of'1' indicates that the adaptation field contains
    // one or more private_data bytes. A value of '0' indicates
    // the adaptation field does not contain any private_data bytes.
    bool transportPrivateDataFlag;

    // The transport_private_data_length is an 8-bit field specifying the number of
    // private_data bytes immediately following the transport private_data_length field.
    // The number of private_data bytes shall be such that private data
    // does not extend beyond the adaptation field.
    uint8_t transportPrivateDataLength;
    uint8_t transportPrivateData[183];

    // The adaptation_field_extension_flag is a 1-bit field
    // which when set to '1' indicates the presence of an adaptation
    // field extension. A value of '0' indicates that an adaptation
    // field extension is not present in the adaptation field.
    bool adaptationFieldExtensionFlag;
    picoMpegTSPacketAdaptionFieldExtension_t adaptationFieldExtension;
} picoMpegTSPacketAdaptationField_t;
typedef picoMpegTSPacketAdaptationField_t *picoMpegTSPacketAdaptationField;

typedef struct {
    // The transport_error_indicator is a 1-bit flag.
    // When set to '1' it indicates that at least
    // 1 uncorrectable bit error exists in the associated
    // transport stream packet. This bit may be set to '1'
    // by entities external to the transport layer.
    // When set to '1' this bit shall not be reset
    // to '0' unless the bit value(s) in error have
    // been corrected.
    bool errorIndicator;

    // The payload_unit_start_indicator is a 1-bit flag
    // which has normative meaning for transport stream
    // packets that carry PES packets or transport stream
    // section data.
    bool payloadUnitStartIndicator;

    // The transport_priority is a 1-bit indicator. When set
    // to '1' it indicates that the associated packet is
    // of greater priority than other packets having the same
    // PID which do not have the bit set to '1'. The transport mechanism
    // can use this to prioritize its data within an
    // elementary stream. Depending on the application the
    // transport_priority field may be coded regardless of
    // the PID or within one PID only. This field may be
    // changed by channel-specific encoders or decoders.
    bool transportPriority;

    // The PID is a 13-bit field, indicating the type of the
    // data stored in the packet payload. PID value 0x0000 is reserved
    // for the program association table. PID value 0x0001
    // is reserved for the conditional access table.
    // PID value 0x0002 is reserved for the transport stream
    // description table, PID value 0x0003 is reserved for
    //  IPMP control information table and PID values
    //  0x0004-0x000F are reserved. PID value 0x1FFF is
    //  reserved for null packets.
    uint16_t pid;

    // This 2-bit field indicates the scrambling mode of
    // the transport stream packet payload. The transport stream
    // packet header, and the adaptation field when present,
    // shall not be scrambled. In the case of a null packet the
    // value of the transport_scrambling_control field
    // shall be set to '00'.
    uint8_t scramblingControl;

    // The continuity_counter is a 4-bit field incrementing
    // with each transport stream packet with the same PID.
    // The continuity_counter wraps around to 0 after its
    // maximum value. The continuity_counter shall not be
    // incremented when the adaptation_field_control of
    // the packet equals '00' or '10'.
    uint8_t continuityCounter;

    // This 2-bit field indicates whether this transport
    // stream packet header is followed by an adaptation
    // field and/or payload.
    picoMpegTSAdaptionFieldControl adaptionFieldControl;

    // Whether the adaptation field is present in this packet
    // derived from adaptionFieldControl
    bool hasAdaptationField;

    // Adaptation field of the packet
    picoMpegTSPacketAdaptationField_t adaptionField;

    // Payload data of the packet
    uint8_t payload[184];

    // Size of the payload data in bytes (0 - 184)
    uint8_t payloadSize;
} picoMpegTSPacket_t;
typedef picoMpegTSPacket_t *picoMpegTSPacket;

// NOTE: The order fo fileds has been rearranged for better memory alignment.
//       Also it is to be noted that all the fileds of this struct may not be
//       present in all PSI sections, so only a subset will be used for those
//       sections.
typedef struct {
    // This is a 12-bit field, the first two bits of which
    // shall be "00". It specifies the number of bytes of the
    // section, starting immediately following the
    // section_length field and including the CRC.
    uint16_t sectionLength;

    // Is the respective id depending upon which section it is.
    // examples: network id for NIT, transport stream id for SDT, program id for PMT etc.
    uint16_t id;

    // The table id of the given section.
    uint8_t tableId;

    // This 5-bit field is the version number of the sub_table.
    // The version_number shall be incremented by 1 when a change
    // in the information carried within the sub_table occurs.
    uint8_t versionNumber;

    // This 1-bit indicator, when set to "1" indicates that the
    // sub_table is the currently applicable sub_table.
    // When the bit is set to "0", it indicates that the sub_table
    // sent is not yet applicable and shall be the next sub_table to be valid.
    bool currentNextIndicator;

    // This 8-bit field gives the number of the section.
    // The section_number of the first section in the
    // sub_table shall be "0x00". The section_number shall be
    // incremented by 1 with each additional section with
    // the same table_id and id.
    uint8_t sectionNumber;

    // This 8-bit field specifies the number of the last
    // section (that is, the section with the highest section_number)
    // of the sub_table of which this section is part.
    uint8_t lastSectionNumber;
} picoMpegTSPSISectionHead_t;
typedef picoMpegTSPSISectionHead_t *picoMpegTSPSISectionHead;

typedef struct {
    // In program streams, the stream_id specifies the type and
    // number of the elementary stream as defined by the
    // stream_id Table. In transport streams, the stream_id may
    // be set to any valid value which correctly describes the
    // elementary stream type as defined in Table. In transport
    // streams, the elementary stream type is specified in the
    // program-specific information. For AVC video streams conforming
    // to one or more profiles defined in Annex G
    // of Rec. ITU-T H.264 | ISO/IEC 14496-10, all video sub-bitstreams
    // of the same AVC video stream shall have the same stream_id value.
    // For AVC video streams conforming to one or more profiles defined
    // in Annex H of Rec. ITU-T H.264 | ISO/IEC 14496-10, all MVC video
    // sub-bitstreams of the same AVC video stream shall have the same stream_id
    // value. For AVC video streams conforming to one or more
    // profiles defined in Annex I of Rec. ITU-T H.264 | ISO/IEC 14496-10,
    // all MVCD video sub-bitstreams of the same AVC video stream
    // shall have the same stream_id value.
    uint8_t streamId;

    // A 16-bit field specifying the number of bytes in the PES
    // packet following the last byte of the field.
    // A value of 0 indicates that the PES packet length is
    // neither specified nor bounded and is allowed only in PES packets
    // whose payload consists of bytes from a video elementary
    // stream contained in transport stream packets.
    uint16_t pesPacketLength;
} picoMpegTSPESHead_t;
typedef picoMpegTSPESHead_t *picoMpegTSPESHead;

// Time structures for DVB tables
typedef struct {
    // Modified Julian Date (16 bits) + UTC time (24 bits BCD: HH:MM:SS)
    int year;
    int month;
    int day;
    int hour;
    int minute;
    int second;
} picoMpegTSUTCTime_t;
typedef picoMpegTSUTCTime_t *picoMpegTSUTCTime;

typedef struct {
    // Duration in BCD format: 6 digits (HH:MM:SS)
    uint8_t hours;   // BCD encoded hours
    uint8_t minutes; // BCD encoded minutes
    uint8_t seconds; // BCD encoded seconds
} picoMpegTSDuration_t;
typedef picoMpegTSDuration_t *picoMpegTSDuration;

typedef struct {
    uint8_t languageCode[4];
    picoMpegTSAudioType audioType;
} picoMpegTSDescriptorISO639LanguageEntry_t;

typedef struct {
    picoMpegTSDescriptorISO639LanguageEntry_t entries[16];
    size_t entryCount;
} picoMpegTSDescriptorISO639Language_t;
typedef picoMpegTSDescriptorISO639Language_t *picoMpegTSDescriptorISO639Language;

typedef struct {
    picoMpegTSServiceType serviceType;
    uint8_t serviceProviderNameLength;
    uint8_t serviceProviderName[64];
    uint8_t serviceNameLength;
    uint8_t serviceName[64];
} picoMpegTSDescriptorService_t;
typedef picoMpegTSDescriptorService_t *picoMpegTSDescriptorService;

typedef struct {
    uint8_t componentTag;
} picoMpegTSDescriptorStreamIdentifier_t;
typedef picoMpegTSDescriptorStreamIdentifier_t *picoMpegTSDescriptorStreamIdentifier;

typedef struct {
    uint16_t caSystemId;
    uint16_t caPid;
    uint8_t privateData[64];
    size_t privateDataLength;
} picoMpegTSDescriptorCA_t;
typedef picoMpegTSDescriptorCA_t *picoMpegTSDescriptorCA;

typedef struct {
    uint8_t nibbleLevel1;
    uint8_t nibbleLevel2;
    uint8_t userByte;
} picoMpegTSDescriptorContentEntry_t;

typedef struct {
    picoMpegTSDescriptorContentEntry_t entries[16];
    size_t entryCount;
} picoMpegTSDescriptorContent_t;
typedef picoMpegTSDescriptorContent_t *picoMpegTSDescriptorContent;

typedef struct {
    // Stream content identifies the high-level type (video, audio, subtitles, etc.)
    uint8_t streamContent;

    // Stream content extension (only used when streamContent == 0xF)
    uint8_t streamContentExt;

    // Component type further specifies the stream characteristics
    uint8_t componentType;

    // Component tag links this descriptor to the stream in PMT
    uint8_t componentTag;

    // ISO 639 language code
    char languageCode[4];

    // Textual description of the component
    char text[256];
    size_t textLength;
} picoMpegTSDescriptorComponent_t;
typedef picoMpegTSDescriptorComponent_t *picoMpegTSDescriptorComponent;

typedef struct {
    // ISO 639 language code
    char languageCode[3];

    // Event name (short title)
    char eventName[64];
    uint8_t eventNameLength;

    // Event description text
    char text[64];
    uint8_t textLength;
} picoMpegTSDescriptorShortEvent_t;
typedef picoMpegTSDescriptorShortEvent_t *picoMpegTSDescriptorShortEvent;

typedef struct {
    uint16_t serviceId;
    picoMpegTSServiceType serviceType;
} picoMpegTSDescriptorServiceListEntry_t;

typedef struct {
    picoMpegTSDescriptorServiceListEntry_t entries[32];
    size_t entryCount;
} picoMpegTSDescriptorServiceList_t;
typedef picoMpegTSDescriptorServiceList_t *picoMpegTSDescriptorServiceList;

typedef struct {
    char name[256];
    uint8_t nameLength;
} picoMpegTSDescriptorNetworkName_t;
typedef picoMpegTSDescriptorNetworkName_t *picoMpegTSDescriptorNetworkName;

typedef struct {
    char countryCode[4];
    uint8_t rating;
} picoMpegTSDescriptorParentalRatingEntry_t;

typedef struct {
    picoMpegTSDescriptorParentalRatingEntry_t entries[16];
    size_t entryCount;
} picoMpegTSDescriptorParentalRating_t;
typedef picoMpegTSDescriptorParentalRating_t *picoMpegTSDescriptorParentalRating;

typedef struct {
    uint8_t data[PICO_MPEGTS_MAX_DESCRIPTOR_DATA_LENGTH];
    size_t dataLength;
    picoMpegTSDescriptorTag tag;
    bool isParsed;
    union {
        picoMpegTSDescriptorISO639Language_t iso639Language;
        picoMpegTSDescriptorService_t service;
        picoMpegTSDescriptorStreamIdentifier_t streamIdentifier;
        picoMpegTSDescriptorCA_t ca;
        picoMpegTSDescriptorContent_t content;
        picoMpegTSDescriptorComponent_t component;
        picoMpegTSDescriptorShortEvent_t shortEvent;
        picoMpegTSDescriptorServiceList_t serviceList;
        picoMpegTSDescriptorNetworkName_t networkName;
        picoMpegTSDescriptorParentalRating_t parentalRating;

    } parsed;
} picoMpegTSDescriptor_t;
typedef picoMpegTSDescriptor_t *picoMpegTSDescriptor;

typedef struct {
    picoMpegTSDescriptor descriptors;
    size_t count;
    size_t capacity;
} picoMpegTSDescriptorSet_t;
typedef picoMpegTSDescriptorSet_t *picoMpegTSDescriptorSet;

typedef struct {
    picoMpegTSDescriptorSet_t descriptorSet;

    struct {
        // This is a 16-bit field which serves as a label
        // for identification of this TS from any other
        // multiplex within the delivery system.
        uint16_t transportStreamId;

        // This 16-bit field gives the label identifying
        // the network_id of the originating delivery system.
        uint16_t originalNetworkId;

        picoMpegTSDescriptorSet_t descriptorSet;
    } transportStreams[PICO_MPEGTS_MAX_TABLE_PAYLOAD_COUNT];
    size_t transportStreamCount;
} picoMpegTSNetworkInformationTablePayload_t;
typedef picoMpegTSNetworkInformationTablePayload_t *picoMpegTSNetworkInformationTablePayload;

typedef struct {
    picoMpegTSDescriptorSet_t descriptorSet;

    struct {
        // This is a 16-bit field which serves as a label
        // for identification of this TS from any other
        // multiplex within the delivery system.
        uint16_t transportStreamId;

        // This 16-bit field gives the label identifying
        // the network_id of the originating delivery system.
        uint16_t originalNetworkId;

        picoMpegTSDescriptorSet_t descriptorSet;
    } services[PICO_MPEGTS_MAX_TABLE_PAYLOAD_COUNT];
    size_t serviceCount;
} picoMpegTSBoquetAssociationTablePayload_t;
typedef picoMpegTSBoquetAssociationTablePayload_t *picoMpegTSBoquetAssociationTablePayload;

typedef struct {
    // This 16-bit field gives the label identifying
    // the network_id of the originating delivery system.
    uint16_t originalNetworkId;

    struct {
        // This is a 16-bit field which serves as a label
        // for identification of this service within
        // the transport stream.
        uint16_t serviceId;

        // This is a 1-bit field which when set to "1"
        // indicates that EIT schedule information for the service
        // is present in the current TS, see ETSI TS 101 211 [i.1]
        // for information on maximum time interval between occurrences
        // of an EIT schedule sub_table). If the flag is set
        // to 0 then the EIT schedule information for the
        // service should not be present in the TS.
        bool eitScheduleFlag;

        // This is a 1-bit field which when set to "1" indicates
        // that EIT_present_following information for the
        // service is present in the current TS, see ETSI TS 101 211 [i.1]
        // for information on maximum time interval between occurrences
        // of an EIT present/following sub_table. If the flag is
        // set to 0 then the EIT present/following information for
        // the service should not be present in the TS.
        bool eitPresentFollowingFlag;

        // The running_status is a 3-bit field which indicates
        // the current running status of the service.
        picoMpegTSSDTRunningStatus runningStatus;

        // This 1-bit field, when set to "0" indicates that all
        // the component streams of the service are not scrambled.
        // When set to "1" it indicates that access to one or more
        // streams may be controlled by a CA system.
        bool freeCAMode;

        picoMpegTSDescriptorSet_t descriptorSet;
    } services[PICO_MPEGTS_MAX_TABLE_PAYLOAD_COUNT];
    size_t serviceCount;
} picoMpegTSServiceDescriptionTablePayload_t;
typedef picoMpegTSServiceDescriptionTablePayload_t *picoMpegTSServiceDescriptionTablePayload;

typedef struct {
    // This 16-bit field serves as a label to identify this TS
    // from any other multiplex within the delivery system.
    uint16_t transportStreamId;

    // This 16-bit field gives the label identifying the network_id
    // of the originating delivery system.
    uint16_t originalNetworkId;

    struct {
        // This 16-bit field contains the identification number
        // of the described event.
        uint16_t eventId;

        // This 40-bit field contains the start time of the event
        // in UTC and MJD.
        picoMpegTSUTCTime_t startTime;

        // A 24-bit field containing the duration of the event
        // in hours, minutes, seconds format: 6 digits, 4-bit BCD.
        picoMpegTSDuration_t duration;

        // This 3-bit field indicates the status of the event.
        picoMpegTSSDTRunningStatus runningStatus;

        // This 1-bit field, when set to "0" indicates that all
        // the component streams of the event are not scrambled.
        // When set to "1" it indicates that access to one or more
        // streams may be controlled by a CA system.
        bool freeCAMode;

        picoMpegTSDescriptorSet_t descriptorSet;
    } events[PICO_MPEGTS_MAX_TABLE_PAYLOAD_COUNT];
    size_t eventCount;
} picoMpegTSEventInformationTablePayload_t;
typedef picoMpegTSEventInformationTablePayload_t *picoMpegTSEventInformationTablePayload;

typedef struct {
    // This 40-bit field contains the current time and date
    // in UTC and MJD.
    picoMpegTSUTCTime_t utcTime;
} picoMpegTSTimeDateTablePayload_t;
typedef picoMpegTSTimeDateTablePayload_t *picoMpegTSTimeDateTablePayload;

typedef struct {
    // This 40-bit field contains the current time and date
    // in UTC and MJD.
    picoMpegTSUTCTime_t utcTime;

    // Descriptors providing local time offset information.
    picoMpegTSDescriptorSet_t descriptorSet;
} picoMpegTSTimeOffsetTablePayload_t;
typedef picoMpegTSTimeOffsetTablePayload_t *picoMpegTSTimeOffsetTablePayload;

typedef struct {
    struct {
        // This 16-bit field serves as a label for identification
        // of the TS, about which the RST informs, from any other
        // multiplex within the delivery system.
        uint16_t transportStreamId;

        // This 16-bit field gives the label identifying the
        // network_id of the originating delivery system.
        uint16_t originalNetworkId;

        // This is a 16-bit field which serves as a label to
        // identify this service from any other service within the TS.
        uint16_t serviceId;

        // This 16-bit field contains the identification number
        // of the related event.
        uint16_t eventId;

        // This 3-bit field indicates the status of the event.
        picoMpegTSSDTRunningStatus runningStatus;
    } statusEntries[PICO_MPEGTS_MAX_TABLE_PAYLOAD_COUNT];
    size_t statusEntryCount;
} picoMpegTSRunningStatusTablePayload_t;
typedef picoMpegTSRunningStatusTablePayload_t *picoMpegTSRunningStatusTablePayload;

typedef struct {
    // This 1-bit flag indicates the kind of transition in the TS.
    // When the bit is set to "1", it indicates that the transition
    // is due to a change of the originating source. When the bit
    // is set to "0", it indicates that the transition is due to
    // a change of the selection entity.
    bool transitionFlag;
} picoMpegTSDiscriminationInformationTablePayload_t;
typedef picoMpegTSDiscriminationInformationTablePayload_t *picoMpegTSDiscriminationInformationTablePayload;

typedef struct {
    // This 12-bit field gives the total length in bytes of the
    // transmission info descriptor loop.
    uint16_t transmissionInfoLoopLength;

    // Descriptors related to the service and event contained
    // in the partial TS.
    picoMpegTSDescriptorSet_t transmissionInfoDescriptorSet;

    struct {
        // This is a 16-bit field which serves as a label to
        // identify this service from any other service within
        // a TS.
        uint16_t serviceId;

        // This 3-bit field indicates the running status of the
        // event in the original stream.
        picoMpegTSSDTRunningStatus runningStatus;

        // Descriptors on the service and event contained in the
        // partial TS.
        picoMpegTSDescriptorSet_t descriptorSet;
    } services[PICO_MPEGTS_MAX_TABLE_PAYLOAD_COUNT];
    size_t serviceCount;
} picoMpegTSServiceInformationTablePayload_t;
typedef picoMpegTSServiceInformationTablePayload_t *picoMpegTSServiceInformationTablePayload;

typedef struct {
    struct {
        // Program_number is a 16-bit field specifying the program to which the
        // program_map_PID is applicable. When set to 0x0000, then the following
        // PID reference shall be the network PID.
        uint16_t programNumber;

        // When program_number == 0x0000, this is the network_PID (13 bits)
        // Otherwise, this is the program_map_PID (13 bits)
        uint16_t pid;
    } programs[PICO_MPEGTS_MAX_TABLE_PAYLOAD_COUNT];
    size_t programCount;
} picoMpegTSProgramAssociationSectionPayload_t;
typedef picoMpegTSProgramAssociationSectionPayload_t *picoMpegTSProgramAssociationSectionPayload;

typedef struct {
    picoMpegTSDescriptorSet_t descriptorSet;
} picoMpegTSConditionalAccessSectionPayload_t;
typedef picoMpegTSConditionalAccessSectionPayload_t *picoMpegTSConditionalAccessSectionPayload;

typedef struct {
    // This 13-bit field indicates the PID of the transport stream packets
    // which shall contain the PCR fields valid for the program specified
    // by program_number. If no PCR is associated with a program definition
    // for private streams, then this field shall take the value of 0x1FFF.
    uint16_t pcrPid;

    // Program-level descriptors
    picoMpegTSDescriptorSet_t programInfoDescriptorSet;

    struct {
        // This 8-bit field specifies the type of program element carried
        // within the packets with the PID whose value is specified by
        // the elementary_PID.
        picoMpegTSStreamType streamType;

        // This 13-bit field specifies the PID of the transport stream
        // packets which carry the associated program element.
        uint16_t elementaryPid;

        // ES-level descriptors for this stream
        picoMpegTSDescriptorSet_t esInfoDescriptorSet;
    } streams[PICO_MPEGTS_MAX_TABLE_PAYLOAD_COUNT];
    size_t streamCount;
} picoMpegTSProgramMapSectionPayload_t;
typedef picoMpegTSProgramMapSectionPayload_t *picoMpegTSProgramMapSectionPayload;

typedef struct {
    // Descriptors that apply to the entire transport stream
    picoMpegTSDescriptorSet_t descriptorSet;
} picoMpegTSTransportStreamDescriptionSectionPayload_t;
typedef picoMpegTSTransportStreamDescriptionSectionPayload_t *picoMpegTSTransportStreamDescriptionSectionPayload;

typedef struct {
    uint8_t metadataBytes[1024];
    size_t metadataByteCount;
} picoMpegTSMetadataSectionPayload_t;
typedef picoMpegTSMetadataSectionPayload_t *picoMpegTSMetadataSectionPayload;

typedef struct {
    uint8_t tableId;
    uint8_t versionNumber;
    uint64_t completedTimestamp;
    picoMpegTSPSISectionHead_t head;

    bool hasSection[PICO_MPEGTS_MAX_SECTIONS];

    union {
        picoMpegTSProgramAssociationSectionPayload_t pas;
        picoMpegTSConditionalAccessSectionPayload_t cas;
        picoMpegTSProgramMapSectionPayload_t pms;
        picoMpegTSTransportStreamDescriptionSectionPayload_t tsds;
        picoMpegTSMetadataSectionPayload_t metas;
        picoMpegTSNetworkInformationTablePayload_t nit;
        picoMpegTSBoquetAssociationTablePayload_t bat;
        picoMpegTSServiceDescriptionTablePayload_t sdt;
        picoMpegTSEventInformationTablePayload_t eit;
        picoMpegTSTimeDateTablePayload_t tdt;
        picoMpegTSTimeOffsetTablePayload_t tot;
        picoMpegTSRunningStatusTablePayload_t rst;
        picoMpegTSDiscriminationInformationTablePayload_t dit;
        picoMpegTSServiceInformationTablePayload_t sit;
    } data;
} picoMpegTSTable_t;
typedef picoMpegTSTable_t *picoMpegTSTable;

typedef struct {
    bool printParsedPackets;
    bool printCurrentTables;
    bool printParsedTables;
    bool printPartialTables;
} picoMpegTSDebugPrintInfo_t;
typedef picoMpegTSDebugPrintInfo_t *picoMpegTSDebugPrintInfo;

picoMpegTS picoMpegTSCreate(bool storeParsedPackets);
void picoMpegTSDestroy(picoMpegTS mpegts);
picoMpegTSResult picoMpegTSGetParsedPackets(picoMpegTS mpegts, picoMpegTSPacket *packetsOut, size_t *packetCountOut);

picoMpegTSResult picoMpegTSAddPacket(picoMpegTS mpegts, const uint8_t *data);
picoMpegTSResult picoMpegTSAddBuffer(picoMpegTS mpegts, const uint8_t *buffer, size_t size);
picoMpegTSResult picoMpegTSAddFile(picoMpegTS mpegts, const char *filename);
picoMpegTSPacketType picoMpegTSDetectPacketType(const uint8_t *data, size_t size);
picoMpegTSPacketType picoMpegTSDetectPacketTypeFromFile(const char *filename);

picoMpegTSResult picoMpegTSParsePacket(const uint8_t *data, picoMpegTSPacket packetOut);
picoMpegTSTable picoMpegTSGetTable(picoMpegTS mpegts, picoMpegTSTableID tableID);

void picoMpegTSDebugPrint(picoMpegTS mpegts, picoMpegTSDebugPrintInfo info);

void picoMpegTSPacketDebugPrint(picoMpegTSPacket packet);
void picoMpegTSPacketAdaptationFieldDebugPrint(picoMpegTSPacketAdaptationField adaptationField);
void picoMpegTSPacketAdaptationFieldExtensionDebugPrint(picoMpegTSAdaptionFieldExtension adaptationFieldExtension);
void picoMpegTsPsiSectionHeadDebugPrint(picoMpegTSPSISectionHead sectionHead);
void picoMpegTSTableDebugPrint(picoMpegTSTable table);

const char *picoMpegTSPacketTypeToString(picoMpegTSPacketType type);
const char *picoMpegTSResultToString(picoMpegTSResult result);
const char *picoMpegTSPIDToString(uint16_t pid);
const char *picoMpegTSTableIDToString(uint8_t tableID);
const char *picoMpegTSAdaptionFieldControlToString(picoMpegTSAdaptionFieldControl afc);
const char *picoMpegTSFilterTypeToString(picoMpegTSFilterType type);
const char *picoMpegTSSDTRunningStatusToString(picoMpegTSSDTRunningStatus status);
const char *picoMpegTSDescriptorTagToString(uint8_t tag);
const char *picoMpegTSServiceTypeToString(picoMpegTSServiceType type);
const char *picoMpegTSAudioTypeToString(picoMpegTSAudioType type);
const char *picoMpegTSContentNibbleLevel1ToString(uint8_t nibble);
const char *picoMpegTSContentNibbleToString(picoMpegTSContentNibble nibble);

#if defined(PICO_IMPLEMENTATION) && !defined(PICO_MPEGTS_IMPLEMENTATION)
#define PICO_MPEGTS_IMPLEMENTATION

// the filter function
typedef struct picoMpegTSFilterContext_t picoMpegTSFilterContext_t;
typedef struct picoMpegTSFilterContext_t *picoMpegTSFilterContext;

typedef picoMpegTSResult (*picoMpegTSFilterFunc)(picoMpegTS mpegts, picoMpegTSPacket packet, picoMpegTSFilterContext context);
typedef picoMpegTSResult (*picoMpegTSFilterHeadFunc)(picoMpegTS mpegts, picoMpegTSPacket packet, picoMpegTSFilterContext context);
typedef picoMpegTSResult (*picoMpegTSFilterBodyFunc)(picoMpegTS mpegts, picoMpegTSFilterContext context);
typedef void (*picoMpegTSFilterContextDestructorFunc)(picoMpegTSFilterContext context);
typedef void *(*picoMpegTSFilterContextConstructorFunc)(picoMpegTS mpegts, picoMpegTSFilterContext context);

struct picoMpegTSFilterContext_t {
    bool hasHead;
    union {
        picoMpegTSPSISectionHead_t psi;
        picoMpegTSPESHead_t pes;
    } head;
    uint16_t pid;

    uint8_t *payloadAccumulator;
    size_t payloadAccumulatorSize;
    size_t payloadAccumulatorCapacity;

    uint8_t lastContinuityCounter;
    bool continuityErrorDetected;
    size_t expectedPayloadSize;

    picoMpegTSFilterType filterType;
};

struct picoMpegTS_t {
    bool storeParsedPackets;
    picoMpegTSPacket parsedPackets;
    size_t parsedPacketCount;
    size_t parsedPacketCapacity;

    bool hasContinuityError;
    uint32_t ignoredPacketCount;

    picoMpegTSFilterContext pidFilters[PICO_MPEGTS_MAX_PID_COUNT];
    picoMpegTSTable tables[PICO_MPEGTS_MAX_TABLE_COUNT];
    picoMpegTSTable partialTables[PICO_MPEGTS_MAX_TABLE_COUNT][PICO_MPEGTS_MAX_VERSIONS];
    picoMpegTSTable parsedTables[PICO_MPEGTS_MAX_TABLE_COUNT][PICO_MPEGTS_MAX_VERSIONS];
};

static picoMpegTSResult __picoMpegTSDescriptorSetAdd(picoMpegTSDescriptorSet set, const picoMpegTSDescriptor descriptor)
{
    PICO_ASSERT(set != NULL);
    PICO_ASSERT(descriptor != NULL);

    if (set->count >= set->capacity) {
        size_t newCapacity                  = set->capacity == 0 ? 8 : set->capacity * 2;
        picoMpegTSDescriptor newDescriptors = (picoMpegTSDescriptor)PICO_REALLOC(set->descriptors, newCapacity * sizeof(picoMpegTSDescriptor_t));
        if (!newDescriptors) {
            return PICO_MPEGTS_RESULT_MALLOC_ERROR;
        }
        set->descriptors = newDescriptors;
        set->capacity    = newCapacity;
    }

    set->descriptors[set->count] = *descriptor;
    set->count++;
    return PICO_MPEGTS_RESULT_SUCCESS;
}

static void __picoMpegTSDescriptorSetClear(picoMpegTSDescriptorSet set)
{
    PICO_ASSERT(set != NULL);
    set->count = 0;
}

static void __picoMpegTSFilterContextFlushPayloadAccumulator(picoMpegTSFilterContext filterContext, size_t byteCount);
static picoMpegTSResult __picoMpegTSReplaceOrRegisterPSIFilter(picoMpegTS mpegts, uint16_t pid);
static picoMpegTSResult __picoMpegTSReplaceOrRegisterPESFilter(picoMpegTS mpegts, uint16_t pid);
static void __picoMpegTSDestroyFilterContext(picoMpegTS mpegts, picoMpegTSFilterContext context);

static bool __picoMpegTSDescriptorPayloadParseISO639Language(picoMpegTSDescriptor descriptor)
{
    PICO_ASSERT(descriptor != NULL);
    PICO_ASSERT(descriptor->tag == PICO_MPEGTS_DESCRIPTOR_TAG_ISO_639_LANGUAGE);

    if (descriptor->dataLength % 4 != 0) {
        return false;
    }

    descriptor->parsed.iso639Language.entryCount = descriptor->dataLength / 4;
    if (descriptor->parsed.iso639Language.entryCount > 16) {
        descriptor->parsed.iso639Language.entryCount = 16;
    }

    for (size_t i = 0; i < descriptor->parsed.iso639Language.entryCount; i++) {
        descriptor->parsed.iso639Language.entries[i].languageCode[0] = descriptor->data[i * 4 + 0];
        descriptor->parsed.iso639Language.entries[i].languageCode[1] = descriptor->data[i * 4 + 1];
        descriptor->parsed.iso639Language.entries[i].languageCode[2] = descriptor->data[i * 4 + 2];
        descriptor->parsed.iso639Language.entries[i].languageCode[3] = '\0';
        descriptor->parsed.iso639Language.entries[i].audioType       = (picoMpegTSAudioType)descriptor->data[i * 4 + 3];
    }

    return true;
}

static bool __picoMpegTSDescriptorPayloadParseService(picoMpegTSDescriptor descriptor)
{
    PICO_ASSERT(descriptor != NULL);
    PICO_ASSERT(descriptor->tag == PICO_MPEGTS_DESCRIPTOR_TAG_SERVICE);

    if (descriptor->dataLength < 2) {
        return false;
    }

    descriptor->parsed.service.serviceType               = descriptor->data[0];
    descriptor->parsed.service.serviceProviderNameLength = descriptor->data[1];

    if (descriptor->dataLength < 2 + descriptor->parsed.service.serviceProviderNameLength + 1) {
        return false;
    }

    if (descriptor->parsed.service.serviceProviderNameLength > 64) {
        return false;
    }

    memcpy(descriptor->parsed.service.serviceProviderName, &descriptor->data[2], descriptor->parsed.service.serviceProviderNameLength);
    descriptor->parsed.service.serviceProviderName[descriptor->parsed.service.serviceProviderNameLength] = '\0';

    descriptor->parsed.service.serviceNameLength = descriptor->data[2 + descriptor->parsed.service.serviceProviderNameLength];

    if (descriptor->dataLength < 2 + descriptor->parsed.service.serviceProviderNameLength + 1 + descriptor->parsed.service.serviceNameLength) {
        return false;
    }

    if (descriptor->parsed.service.serviceNameLength > 64) {
        return false;
    }

    memcpy(descriptor->parsed.service.serviceName, &descriptor->data[3 + descriptor->parsed.service.serviceProviderNameLength], descriptor->parsed.service.serviceNameLength);
    descriptor->parsed.service.serviceName[descriptor->parsed.service.serviceNameLength] = '\0';

    return true;
}

static bool __picoMpegTSDescriptorPayloadParseStreamIdentifier(picoMpegTSDescriptor descriptor)
{
    PICO_ASSERT(descriptor != NULL);
    PICO_ASSERT(descriptor->tag == PICO_MPEGTS_DESCRIPTOR_TAG_STREAM_IDENTIFIER);

    if (descriptor->dataLength != 1) {
        return false;
    }

    descriptor->parsed.streamIdentifier.componentTag = descriptor->data[0];
    return true;
}

static bool __picoMpegTSDescriptorPayloadParseCA(picoMpegTSDescriptor descriptor)
{
    PICO_ASSERT(descriptor != NULL);
    PICO_ASSERT(descriptor->tag == PICO_MPEGTS_DESCRIPTOR_TAG_CA);

    if (descriptor->dataLength < 4) {
        return false;
    }

    descriptor->parsed.ca.caSystemId = (uint16_t)(descriptor->data[0] << 8) | descriptor->data[1];
    descriptor->parsed.ca.caPid      = (uint16_t)((descriptor->data[2] & 0x1F) << 8) | descriptor->data[3];

    descriptor->parsed.ca.privateDataLength = descriptor->dataLength - 4;
    if (descriptor->parsed.ca.privateDataLength > 64) {
        descriptor->parsed.ca.privateDataLength = 64;
    }

    if (descriptor->parsed.ca.privateDataLength > 0) {
        memcpy(descriptor->parsed.ca.privateData, &descriptor->data[4], descriptor->parsed.ca.privateDataLength);
    }

    return true;
}

static bool __picoMpegTSDescriptorPayloadParseContent(picoMpegTSDescriptor descriptor)
{
    PICO_ASSERT(descriptor != NULL);
    PICO_ASSERT(descriptor->tag == PICO_MPEGTS_DESCRIPTOR_TAG_CONTENT);

    if (descriptor->dataLength % 2 != 0) {
        return false;
    }

    descriptor->parsed.content.entryCount = descriptor->dataLength / 2;
    if (descriptor->parsed.content.entryCount > 16) {
        descriptor->parsed.content.entryCount = 16;
    }

    for (size_t i = 0; i < descriptor->parsed.content.entryCount; i++) {
        descriptor->parsed.content.entries[i].nibbleLevel1 = (descriptor->data[i * 2] & 0xF0) >> 4;
        descriptor->parsed.content.entries[i].nibbleLevel2 = descriptor->data[i * 2] & 0x0F;
        descriptor->parsed.content.entries[i].userByte     = descriptor->data[i * 2 + 1];
    }

    return true;
}

static bool __picoMpegTSDescriptorPayloadParseComponent(picoMpegTSDescriptor descriptor)
{
    PICO_ASSERT(descriptor != NULL);
    PICO_ASSERT(descriptor->tag == PICO_MPEGTS_DESCRIPTOR_TAG_COMPONENT);

    if (descriptor->dataLength < 6) {
        return false;
    }

    descriptor->parsed.component.streamContent    = descriptor->data[0] & 0x0F;
    descriptor->parsed.component.streamContentExt = (descriptor->data[0] & 0xF0) >> 4;

    descriptor->parsed.component.componentType = descriptor->data[1];

    descriptor->parsed.component.componentTag = descriptor->data[2];

    descriptor->parsed.component.languageCode[0] = (char)descriptor->data[3];
    descriptor->parsed.component.languageCode[1] = (char)descriptor->data[4];
    descriptor->parsed.component.languageCode[2] = (char)descriptor->data[5];
    descriptor->parsed.component.languageCode[3] = '\0';

    descriptor->parsed.component.textLength = descriptor->dataLength - 6;
    if (descriptor->parsed.component.textLength > 255) {
        descriptor->parsed.component.textLength = 255;
    }

    if (descriptor->parsed.component.textLength > 0) {
        memcpy(descriptor->parsed.component.text, &descriptor->data[6], descriptor->parsed.component.textLength);
    }
    descriptor->parsed.component.text[descriptor->parsed.component.textLength] = '\0';

    return true;
}

static bool __picoMpegTSDescriptorPayloadParseShortEvent(picoMpegTSDescriptor descriptor)
{
    PICO_ASSERT(descriptor != NULL);
    PICO_ASSERT(descriptor->tag == PICO_MPEGTS_DESCRIPTOR_TAG_SHORT_EVENT);

    descriptor->parsed.shortEvent.languageCode[0] = (char)descriptor->data[0];
    descriptor->parsed.shortEvent.languageCode[1] = (char)descriptor->data[1];
    descriptor->parsed.shortEvent.languageCode[2] = (char)descriptor->data[2];

    uint8_t eventNameLength                       = descriptor->data[3];
    descriptor->parsed.shortEvent.eventNameLength = eventNameLength;

    if (4 + eventNameLength > descriptor->dataLength) {
        return false;
    }

    if (eventNameLength > 63) {
        eventNameLength                               = 63;
        descriptor->parsed.shortEvent.eventNameLength = 63;
    }

    if (eventNameLength > 0) {
        memcpy(descriptor->parsed.shortEvent.eventName, &descriptor->data[4], eventNameLength);
    }
    descriptor->parsed.shortEvent.eventName[eventNameLength] = '\0';
    size_t textLengthPos                                     = 4 + descriptor->parsed.shortEvent.eventNameLength;

    if (textLengthPos >= descriptor->dataLength) {
        return false;
    }

    uint8_t textLength                       = descriptor->data[textLengthPos];
    descriptor->parsed.shortEvent.textLength = textLength;

    if (textLengthPos + 1 + textLength > descriptor->dataLength) {
        return false;
    }

    if (textLength > 63) {
        textLength                               = 63;
        descriptor->parsed.shortEvent.textLength = 63;
    }

    if (textLength > 0) {
        memcpy(descriptor->parsed.shortEvent.text, &descriptor->data[textLengthPos + 1], textLength);
    }
    descriptor->parsed.shortEvent.text[textLength] = '\0';

    return true;
}

static bool __picoMpegTSDescriptorPayloadParseServiceList(picoMpegTSDescriptor descriptor)
{
    PICO_ASSERT(descriptor != NULL);
    PICO_ASSERT(descriptor->tag == PICO_MPEGTS_DESCRIPTOR_TAG_SERVICE_LIST);

    if (descriptor->dataLength % 3 != 0) {
        return false;
    }

    descriptor->parsed.serviceList.entryCount = descriptor->dataLength / 3;
    if (descriptor->parsed.serviceList.entryCount > 32) {
        descriptor->parsed.serviceList.entryCount = 32;
    }

    for (size_t i = 0; i < descriptor->parsed.serviceList.entryCount; i++) {
        descriptor->parsed.serviceList.entries[i].serviceId   = (uint16_t)(descriptor->data[i * 3] << 8) | descriptor->data[i * 3 + 1];
        descriptor->parsed.serviceList.entries[i].serviceType = (picoMpegTSServiceType)descriptor->data[i * 3 + 2];
    }

    return true;
}

static bool __picoMpegTSDescriptorPayloadParseNetworkName(picoMpegTSDescriptor descriptor)
{
    PICO_ASSERT(descriptor != NULL);
    PICO_ASSERT(descriptor->tag == PICO_MPEGTS_DESCRIPTOR_TAG_NETWORK_NAME);

    descriptor->parsed.networkName.nameLength = (uint8_t)descriptor->dataLength;
    if (descriptor->parsed.networkName.nameLength > 255) {
        descriptor->parsed.networkName.nameLength = 255;
    }

    memcpy(descriptor->parsed.networkName.name, descriptor->data, descriptor->parsed.networkName.nameLength);
    descriptor->parsed.networkName.name[descriptor->parsed.networkName.nameLength] = '\0';

    return true;
}

static bool __picoMpegTSDescriptorPayloadParseParentalRating(picoMpegTSDescriptor descriptor)
{
    PICO_ASSERT(descriptor != NULL);
    PICO_ASSERT(descriptor->tag == PICO_MPEGTS_DESCRIPTOR_TAG_PARENTAL_RATING);

    if (descriptor->dataLength % 4 != 0) {
        return false;
    }

    descriptor->parsed.parentalRating.entryCount = descriptor->dataLength / 4;
    if (descriptor->parsed.parentalRating.entryCount > 16) {
        descriptor->parsed.parentalRating.entryCount = 16;
    }

    for (size_t i = 0; i < descriptor->parsed.parentalRating.entryCount; i++) {
        memcpy(descriptor->parsed.parentalRating.entries[i].countryCode, &descriptor->data[i * 4], 3);
        descriptor->parsed.parentalRating.entries[i].countryCode[3] = '\0';
        descriptor->parsed.parentalRating.entries[i].rating         = descriptor->data[i * 4 + 3];
    }

    return true;
}

static bool __picoMpegTSDescriptorPayloadParse(picoMpegTSDescriptor descriptor)
{
    PICO_ASSERT(descriptor != NULL);

    switch (descriptor->tag) {
        case PICO_MPEGTS_DESCRIPTOR_TAG_ISO_639_LANGUAGE:
            return __picoMpegTSDescriptorPayloadParseISO639Language(descriptor);
        case PICO_MPEGTS_DESCRIPTOR_TAG_SERVICE:
            return __picoMpegTSDescriptorPayloadParseService(descriptor);
        case PICO_MPEGTS_DESCRIPTOR_TAG_STREAM_IDENTIFIER:
            return __picoMpegTSDescriptorPayloadParseStreamIdentifier(descriptor);
        case PICO_MPEGTS_DESCRIPTOR_TAG_CA:
            return __picoMpegTSDescriptorPayloadParseCA(descriptor);
        case PICO_MPEGTS_DESCRIPTOR_TAG_CONTENT:
            return __picoMpegTSDescriptorPayloadParseContent(descriptor);
        case PICO_MPEGTS_DESCRIPTOR_TAG_COMPONENT:
            return __picoMpegTSDescriptorPayloadParseComponent(descriptor);
        case PICO_MPEGTS_DESCRIPTOR_TAG_SHORT_EVENT:
            return __picoMpegTSDescriptorPayloadParseShortEvent(descriptor);
        case PICO_MPEGTS_DESCRIPTOR_TAG_SERVICE_LIST:
            return __picoMpegTSDescriptorPayloadParseServiceList(descriptor);
        case PICO_MPEGTS_DESCRIPTOR_TAG_NETWORK_NAME:
            return __picoMpegTSDescriptorPayloadParseNetworkName(descriptor);
        case PICO_MPEGTS_DESCRIPTOR_TAG_PARENTAL_RATING:
            return __picoMpegTSDescriptorPayloadParseParentalRating(descriptor);
        default:
            return false;
    }
}

static picoMpegTSResult __picoMpegTSDescriptorParse(picoMpegTSDescriptor descriptor, const uint8_t *data, size_t dataSize, size_t *bytesConsumed)
{
    PICO_ASSERT(descriptor != NULL);
    PICO_ASSERT(data != NULL);
    PICO_ASSERT(bytesConsumed != NULL);

    // need at least tag (1 byte) + length (1 byte)
    if (dataSize < 2) {
        return PICO_MPEGTS_RESULT_INVALID_DATA;
    }

    uint8_t descriptorTag    = data[0];
    uint8_t descriptorLength = data[1];

    // check if we have enough data for this descriptors content
    if (2 + descriptorLength > dataSize) {
        PICO_MPEGTS_LOG("picoMpegTS: descriptor parse error - descriptor length exceeds bounds [%d/%zu]\n", descriptorLength, dataSize);
        return PICO_MPEGTS_RESULT_INVALID_DATA;
    }

    descriptor->tag        = (picoMpegTSDescriptorTag)descriptorTag;
    descriptor->dataLength = descriptorLength;

    size_t copyLength = descriptorLength;
    if (copyLength > PICO_MPEGTS_MAX_DESCRIPTOR_DATA_LENGTH) {
        copyLength = PICO_MPEGTS_MAX_DESCRIPTOR_DATA_LENGTH;
        PICO_MPEGTS_LOG("picoMpegTS: descriptor data truncated (%u > %d)\n",
                        descriptorLength, PICO_MPEGTS_MAX_DESCRIPTOR_DATA_LENGTH);
    }
    memcpy(descriptor->data, &data[2], copyLength);

    descriptor->isParsed = __picoMpegTSDescriptorPayloadParse(descriptor);

    *bytesConsumed = 2 + descriptorLength;
    return PICO_MPEGTS_RESULT_SUCCESS;
}

static picoMpegTSResult __picoMpegTSDescriptorSetParse(picoMpegTSDescriptorSet descriptorSet, picoMpegTSFilterContext filterContext, bool additive)
{
    PICO_ASSERT(descriptorSet != NULL);
    PICO_ASSERT(filterContext != NULL);

    const uint8_t *data = filterContext->payloadAccumulator;
    size_t dataSize     = filterContext->payloadAccumulatorSize;

    if (dataSize < 2) {
        return PICO_MPEGTS_RESULT_INVALID_DATA;
    }

    uint16_t descriptorsLength = ((uint16_t)(data[0] & 0x0F) << 8) | data[1];
    data += 2;
    dataSize -= 2;

    if (dataSize < descriptorsLength) {
        PICO_MPEGTS_LOG("picoMpegTS: descriptor set parse error - not enough data (%zu < %u)\n",
                        dataSize, descriptorsLength);
        return PICO_MPEGTS_RESULT_INVALID_DATA;
    }

    if (!additive) {
        __picoMpegTSDescriptorSetClear(descriptorSet);
    }

    size_t offset = 0;
    while (offset + 2 <= descriptorsLength) {
        picoMpegTSDescriptor_t descriptor;
        size_t bytesConsumed = 0;

        PICO_MPEGTS_RETURN_ON_ERROR(__picoMpegTSDescriptorParse(&descriptor, &data[offset], descriptorsLength - offset, &bytesConsumed));
        PICO_MPEGTS_RETURN_ON_ERROR(__picoMpegTSDescriptorSetAdd(descriptorSet, &descriptor));

        offset += bytesConsumed;
    }

    __picoMpegTSFilterContextFlushPayloadAccumulator(filterContext, 2 + offset);

    return PICO_MPEGTS_RESULT_SUCCESS;
}

static void __picoMpegTSDescriptorSetDestroy(picoMpegTSDescriptorSet set)
{
    if (!set)
        return;
    if (set->descriptors) {
        PICO_FREE(set->descriptors);
    }
    set->descriptors = NULL;
    set->count       = 0;
    set->capacity    = 0;
}

static void __picoMpegTSMJDToGregorian(uint16_t mjd, int *year, int *month, int *day)
{
    PICO_ASSERT(year != NULL);
    PICO_ASSERT(month != NULL);
    PICO_ASSERT(day != NULL);

    int y_prime, m_prime, k;

    y_prime = (int)((mjd - 15078.2) / 365.25);
    m_prime = (int)((mjd - 14956.1 - (int)(y_prime * 365.25)) / 30.6001);
    *day    = mjd - 14956 - (int)(y_prime * 365.25) - (int)(m_prime * 30.6001);
    if (m_prime == 14 || m_prime == 15) {
        k = 1;
    } else {
        k = 0;
    }
    *year  = y_prime + k + 1900;
    *month = m_prime - 1 - k * 12;
}

static int __picoMpegTSBCDToInteger(uint8_t bcd)
{
    return ((bcd >> 4) & 0xF) * 10 + (bcd & 0xF);
}

static picoMpegTSTable __picoMpegTSTableCreate(uint8_t tableId, uint8_t versionNumber)
{
    picoMpegTSTable table = (picoMpegTSTable)PICO_MALLOC(sizeof(picoMpegTSTable_t));
    if (!table)
        return NULL;
    memset(table, 0, sizeof(picoMpegTSTable_t));
    table->tableId       = tableId;
    table->versionNumber = versionNumber;
    return table;
}

static uint64_t __picoMpegTSGetCurrentTimestamp(void)
{
#ifdef _WIN32
    return (uint64_t)time(NULL);
#else
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return (uint64_t)ts.tv_sec;
#endif
}

static bool __picoMpegTSIsVersionNewer(picoMpegTSTable candidate, picoMpegTSTable current)
{
    if (candidate == NULL)
        return false;
    if (current == NULL)
        return true;

    uint64_t candidateTs = candidate->completedTimestamp;
    uint64_t currentTs   = current->completedTimestamp;

    if (candidateTs > currentTs && (candidateTs - currentTs) > PICO_MPEGTS_TABLE_VERSION_AGE_THRESHOLD_SECONDS) {
        return true;
    }
    if (currentTs > candidateTs && (currentTs - candidateTs) > PICO_MPEGTS_TABLE_VERSION_AGE_THRESHOLD_SECONDS) {
        return false;
    }

    return candidate->versionNumber > current->versionNumber;
}

static void __picoMpegTSTableDestroy(picoMpegTSTable table)
{
    PICO_ASSERT(table != NULL);

    switch (table->tableId) {
        case PICO_MPEGTS_TABLE_ID_PAS:
            // No descriptors to destroy
            break;

        case PICO_MPEGTS_TABLE_ID_CAS:
            __picoMpegTSDescriptorSetDestroy(&table->data.cas.descriptorSet);
            break;

        case PICO_MPEGTS_TABLE_ID_PMS:
            __picoMpegTSDescriptorSetDestroy(&table->data.pms.programInfoDescriptorSet);
            for (size_t i = 0; i < table->data.pms.streamCount; i++) {
                __picoMpegTSDescriptorSetDestroy(&table->data.pms.streams[i].esInfoDescriptorSet);
            }
            break;

        case PICO_MPEGTS_TABLE_ID_TSDS:
            __picoMpegTSDescriptorSetDestroy(&table->data.tsds.descriptorSet);
            break;

        case PICO_MPEGTS_TABLE_ID_METAS:
            // No descriptors to destroy
            break;

        case PICO_MPEGTS_TABLE_ID_NISAN:
        case PICO_MPEGTS_TABLE_ID_NISON:
            __picoMpegTSDescriptorSetDestroy(&table->data.nit.descriptorSet);
            for (size_t i = 0; i < table->data.nit.transportStreamCount; i++) {
                __picoMpegTSDescriptorSetDestroy(&table->data.nit.transportStreams[i].descriptorSet);
            }
            break;

        case PICO_MPEGTS_TABLE_ID_BAS:
            __picoMpegTSDescriptorSetDestroy(&table->data.bat.descriptorSet);
            for (size_t i = 0; i < table->data.bat.serviceCount; i++) {
                __picoMpegTSDescriptorSetDestroy(&table->data.bat.services[i].descriptorSet);
            }
            break;

        case PICO_MPEGTS_TABLE_ID_SDSATS:
        case PICO_MPEGTS_TABLE_ID_SDSOTS:
            for (size_t i = 0; i < table->data.sdt.serviceCount; i++) {
                __picoMpegTSDescriptorSetDestroy(&table->data.sdt.services[i].descriptorSet);
            }
            break;

        case PICO_MPEGTS_TABLE_ID_EISATSF:
        case PICO_MPEGTS_TABLE_ID_EISOTSF:
            for (size_t i = 0; i < table->data.eit.eventCount; i++) {
                __picoMpegTSDescriptorSetDestroy(&table->data.eit.events[i].descriptorSet);
            }
            break;

        case PICO_MPEGTS_TABLE_ID_TOS:
            __picoMpegTSDescriptorSetDestroy(&table->data.tot.descriptorSet);
            break;

        case PICO_MPEGTS_TABLE_ID_SIS:
            __picoMpegTSDescriptorSetDestroy(&table->data.sit.transmissionInfoDescriptorSet);
            for (size_t i = 0; i < table->data.sit.serviceCount; i++) {
                __picoMpegTSDescriptorSetDestroy(&table->data.sit.services[i].descriptorSet);
            }
            break;

        default:
            if ((table->tableId >= 0x50 && table->tableId <= 0x5F) ||
                (table->tableId >= 0x60 && table->tableId <= 0x6F)) {
                for (size_t i = 0; i < table->data.eit.eventCount; i++) {
                    __picoMpegTSDescriptorSetDestroy(&table->data.eit.events[i].descriptorSet);
                }
            }
            break;
    }

    PICO_FREE(table);
}

static picoMpegTSResult __picoMpegTSParseNIT(picoMpegTS mpegts, picoMpegTSNetworkInformationTablePayload table, picoMpegTSFilterContext filterContext)
{
    PICO_ASSERT(mpegts != NULL);
    PICO_ASSERT(table != NULL);
    PICO_ASSERT(filterContext != NULL);

    PICO_MPEGTS_RETURN_ON_ERROR(__picoMpegTSDescriptorSetParse(&table->descriptorSet, filterContext, true));

    uint16_t transportStreamLength = (filterContext->payloadAccumulator[0] & 0x0F) << 8 | filterContext->payloadAccumulator[1];
    size_t targetLength            = filterContext->payloadAccumulatorSize - transportStreamLength;
    __picoMpegTSFilterContextFlushPayloadAccumulator(filterContext, 2);

    while (filterContext->payloadAccumulatorSize > targetLength) {
        if (table->transportStreamCount == PICO_MPEGTS_MAX_TABLE_PAYLOAD_COUNT) {
            return PICO_MPEGTS_RESULT_TABLE_FULL;
        }

        uint16_t transportStreamId                                             = filterContext->payloadAccumulator[0] << 8 | filterContext->payloadAccumulator[1];
        table->transportStreams[table->transportStreamCount].transportStreamId = transportStreamId;

        uint16_t originalNetworkId                                             = filterContext->payloadAccumulator[2] << 8 | filterContext->payloadAccumulator[3];
        table->transportStreams[table->transportStreamCount].originalNetworkId = originalNetworkId;

        __picoMpegTSFilterContextFlushPayloadAccumulator(filterContext, 4);

        PICO_MPEGTS_RETURN_ON_ERROR(__picoMpegTSDescriptorSetParse(&table->transportStreams[table->transportStreamCount].descriptorSet, filterContext, true));

        table->transportStreamCount++;
    }

    // uint32_t crc32 = (filterContext->payloadAccumulator[0] << 24) | (filterContext->payloadAccumulator[1] << 16) | (filterContext->payloadAccumulator[2] << 8) | filterContext->payloadAccumulator[3];
    // NOTE: we do not do any CRC verification now.

    return PICO_MPEGTS_RESULT_SUCCESS;
}

static picoMpegTSResult __picoMpegTSParseBAT(picoMpegTS mpegts, picoMpegTSBoquetAssociationTablePayload table, picoMpegTSFilterContext filterContext)
{
    PICO_ASSERT(mpegts != NULL);
    PICO_ASSERT(table != NULL);
    PICO_ASSERT(filterContext != NULL);

    PICO_MPEGTS_RETURN_ON_ERROR(__picoMpegTSDescriptorSetParse(&table->descriptorSet, filterContext, true));

    uint16_t transportStreamLength = (filterContext->payloadAccumulator[0] & 0x0F) << 8 | filterContext->payloadAccumulator[1];
    size_t targetLength            = filterContext->payloadAccumulatorSize - transportStreamLength;
    __picoMpegTSFilterContextFlushPayloadAccumulator(filterContext, 2);

    while (filterContext->payloadAccumulatorSize > targetLength) {
        if (table->serviceCount == PICO_MPEGTS_MAX_TABLE_PAYLOAD_COUNT) {
            return PICO_MPEGTS_RESULT_TABLE_FULL;
        }

        uint16_t transportStreamId                             = filterContext->payloadAccumulator[0] << 8 | filterContext->payloadAccumulator[1];
        table->services[table->serviceCount].transportStreamId = transportStreamId;

        uint16_t originalNetworkId                             = filterContext->payloadAccumulator[2] << 8 | filterContext->payloadAccumulator[3];
        table->services[table->serviceCount].originalNetworkId = originalNetworkId;

        __picoMpegTSFilterContextFlushPayloadAccumulator(filterContext, 4);

        PICO_MPEGTS_RETURN_ON_ERROR(__picoMpegTSDescriptorSetParse(&table->services[table->serviceCount].descriptorSet, filterContext, true));

        table->serviceCount++;
    }

    // uint32_t crc32 = (filterContext->payloadAccumulator[0] << 24) | (filterContext->payloadAccumulator[1] << 16) | (filterContext->payloadAccumulator[2] << 8) | filterContext->payloadAccumulator[3];
    // NOTE: we do not do any CRC verification now.

    return PICO_MPEGTS_RESULT_SUCCESS;
}

static picoMpegTSResult __picoMpegTSParseSDT(picoMpegTS mpegts, picoMpegTSServiceDescriptionTablePayload table, picoMpegTSFilterContext filterContext)
{
    PICO_ASSERT(mpegts != NULL);
    PICO_ASSERT(table != NULL);
    PICO_ASSERT(filterContext != NULL);

    table->originalNetworkId = (uint16_t)(filterContext->payloadAccumulator[0] << 8) | filterContext->payloadAccumulator[1];

    __picoMpegTSFilterContextFlushPayloadAccumulator(filterContext, 3);

    size_t targetSize = filterContext->payloadAccumulatorSize - filterContext->expectedPayloadSize + 7; // 3 for the item above, 4 for the CRC32 at the end
    while (filterContext->payloadAccumulatorSize > targetSize) {
        if (table->serviceCount == PICO_MPEGTS_MAX_TABLE_PAYLOAD_COUNT) {
            return PICO_MPEGTS_RESULT_TABLE_FULL;
        }

        if (filterContext->payloadAccumulatorSize < 5) {
            break;
        }

        uint16_t serviceId                             = (uint16_t)(filterContext->payloadAccumulator[0] << 8) | filterContext->payloadAccumulator[1];
        table->services[table->serviceCount].serviceId = serviceId;

        uint8_t flags1                                               = filterContext->payloadAccumulator[2];
        table->services[table->serviceCount].eitScheduleFlag         = (flags1 & 0x02) != 0;
        table->services[table->serviceCount].eitPresentFollowingFlag = (flags1 & 0x01) != 0;

        uint8_t flags2 = filterContext->payloadAccumulator[3];

        table->services[table->serviceCount].runningStatus = (picoMpegTSSDTRunningStatus)((flags2 >> 5) & 0x07);
        table->services[table->serviceCount].freeCAMode    = (flags2 & 0x10) != 0;

        __picoMpegTSFilterContextFlushPayloadAccumulator(filterContext, 3);

        PICO_MPEGTS_RETURN_ON_ERROR(__picoMpegTSDescriptorSetParse(&table->services[table->serviceCount].descriptorSet, filterContext, true));

        table->serviceCount++;
    }

    // uint32_t crc32 = (filterContext->payloadAccumulator[0] << 24) | (filterContext->payloadAccumulator[1] << 16) |
    //                  (filterContext->payloadAccumulator[2] << 8) | filterContext->payloadAccumulator[3];

    return PICO_MPEGTS_RESULT_SUCCESS;
}

static picoMpegTSResult __picoMpegTSParseTDT(picoMpegTS mpegts, picoMpegTSTimeDateTablePayload table, picoMpegTSFilterContext filterContext)
{
    PICO_ASSERT(mpegts != NULL);
    PICO_ASSERT(table != NULL);
    PICO_ASSERT(filterContext != NULL);

    // NOTE: right now due to an issue, we arent parsing the UTC time
    // TODO: fix this
    PICO_MPEGTS_LOG("Time Date Table not implemented");

    return PICO_MPEGTS_RESULT_SUCCESS;
}

static picoMpegTSResult __picoMpegTSParseTOT(picoMpegTS mpegts, picoMpegTSTimeOffsetTablePayload table, picoMpegTSFilterContext filterContext)
{
    PICO_ASSERT(mpegts != NULL);
    PICO_ASSERT(table != NULL);
    PICO_ASSERT(filterContext != NULL);

    // NOTE: right now due to an issue, we arent parsing the UTC time
    PICO_MPEGTS_LOG("Time Offset Table only partially implemented");

    PICO_MPEGTS_RETURN_ON_ERROR(__picoMpegTSDescriptorSetParse(&table->descriptorSet, filterContext, false));

    return PICO_MPEGTS_RESULT_SUCCESS;
}

static picoMpegTSResult __picoMpegTSParseRST(picoMpegTS mpegts, picoMpegTSRunningStatusTablePayload table, picoMpegTSFilterContext filterContext)
{
    PICO_ASSERT(mpegts != NULL);
    PICO_ASSERT(table != NULL);
    PICO_ASSERT(filterContext != NULL);

    // TODO: not implemented right now
    PICO_MPEGTS_LOG("Running Status Table not implemented");

    return PICO_MPEGTS_RESULT_SUCCESS;
}

static picoMpegTSResult __picoMpegTSParseEIT(picoMpegTS mpegts, picoMpegTSEventInformationTablePayload table, picoMpegTSFilterContext filterContext)
{
    PICO_ASSERT(mpegts != NULL);
    PICO_ASSERT(table != NULL);
    PICO_ASSERT(filterContext != NULL);

    size_t targetSize        = filterContext->payloadAccumulatorSize - filterContext->expectedPayloadSize + 4;
    table->transportStreamId = (uint16_t)(filterContext->payloadAccumulator[0] << 8) | filterContext->payloadAccumulator[1];
    table->originalNetworkId = (uint16_t)(filterContext->payloadAccumulator[2] << 8) | filterContext->payloadAccumulator[3];

    __picoMpegTSFilterContextFlushPayloadAccumulator(filterContext, 6);

    while (filterContext->payloadAccumulatorSize > targetSize) {
        if (table->eventCount == PICO_MPEGTS_MAX_TABLE_PAYLOAD_COUNT) {
            return PICO_MPEGTS_RESULT_TABLE_FULL;
        }

        if (filterContext->payloadAccumulatorSize < (targetSize + 12)) {
            break;
        }

        table->events[table->eventCount].eventId = (uint16_t)(filterContext->payloadAccumulator[0] << 8) |
                                                   filterContext->payloadAccumulator[1];

        uint16_t mjd = (uint16_t)(filterContext->payloadAccumulator[2] << 8) | filterContext->payloadAccumulator[3];
        __picoMpegTSMJDToGregorian(mjd,
                                   &table->events[table->eventCount].startTime.year,
                                   &table->events[table->eventCount].startTime.month,
                                   &table->events[table->eventCount].startTime.day);

        table->events[table->eventCount].startTime.hour   = __picoMpegTSBCDToInteger(filterContext->payloadAccumulator[4]);
        table->events[table->eventCount].startTime.minute = __picoMpegTSBCDToInteger(filterContext->payloadAccumulator[5]);
        table->events[table->eventCount].startTime.second = __picoMpegTSBCDToInteger(filterContext->payloadAccumulator[6]);

        table->events[table->eventCount].duration.hours   = filterContext->payloadAccumulator[7];
        table->events[table->eventCount].duration.minutes = filterContext->payloadAccumulator[8];
        table->events[table->eventCount].duration.seconds = filterContext->payloadAccumulator[9];

        uint16_t flags                                 = (uint16_t)(filterContext->payloadAccumulator[10] << 8) | filterContext->payloadAccumulator[11];
        table->events[table->eventCount].runningStatus = (picoMpegTSSDTRunningStatus)((flags >> 13) & 0x07);
        table->events[table->eventCount].freeCAMode    = ((flags >> 12) & 0x01) != 0;

        __picoMpegTSFilterContextFlushPayloadAccumulator(filterContext, 10);
        PICO_MPEGTS_RETURN_ON_ERROR(__picoMpegTSDescriptorSetParse(&table->events[table->eventCount].descriptorSet, filterContext, true));

        table->eventCount++;
    }

    return PICO_MPEGTS_RESULT_SUCCESS;
}

static picoMpegTSResult __picoMpegTSParsePAT(picoMpegTS mpegts, picoMpegTSProgramAssociationSectionPayload table, picoMpegTSFilterContext filterContext)
{
    PICO_ASSERT(mpegts != NULL);
    PICO_ASSERT(table != NULL);
    PICO_ASSERT(filterContext != NULL);

    size_t targetSize = filterContext->payloadAccumulatorSize - filterContext->expectedPayloadSize + 4;
    while (filterContext->payloadAccumulatorSize > targetSize) {
        if (table->programCount == PICO_MPEGTS_MAX_TABLE_PAYLOAD_COUNT) {
            return PICO_MPEGTS_RESULT_TABLE_FULL;
        }

        uint16_t programNumber = (uint16_t)(filterContext->payloadAccumulator[0] << 8) |
                                 filterContext->payloadAccumulator[1];
        table->programs[table->programCount].programNumber = programNumber;

        uint16_t pid = ((uint16_t)(filterContext->payloadAccumulator[2] & 0x1F) << 8) |
                       filterContext->payloadAccumulator[3];
        table->programs[table->programCount].pid = pid;

        __picoMpegTSFilterContextFlushPayloadAccumulator(filterContext, 4);

        table->programCount++;
    }

    // uint32_t crc32 = (filterContext->payloadAccumulator[0] << 24) |
    //                  (filterContext->payloadAccumulator[1] << 16) |
    //                  (filterContext->payloadAccumulator[2] << 8) |
    //                  filterContext->payloadAccumulator[3];

    return PICO_MPEGTS_RESULT_SUCCESS;
}

static picoMpegTSResult __picoMpegTSParseCAT(picoMpegTS mpegts, picoMpegTSConditionalAccessSectionPayload table, picoMpegTSFilterContext filterContext)
{
    PICO_ASSERT(mpegts != NULL);
    PICO_ASSERT(table != NULL);
    PICO_ASSERT(filterContext != NULL);

    size_t targetSize = filterContext->payloadAccumulatorSize - filterContext->expectedPayloadSize + 4;
    while (filterContext->payloadAccumulatorSize > targetSize) {
        picoMpegTSDescriptor_t descriptor = {0};
        size_t bytesConsumed              = 0;

        PICO_MPEGTS_RETURN_ON_ERROR(__picoMpegTSDescriptorParse(&descriptor,
                                                                filterContext->payloadAccumulator,
                                                                filterContext->payloadAccumulatorSize - targetSize,
                                                                &bytesConsumed));

        PICO_MPEGTS_RETURN_ON_ERROR(__picoMpegTSDescriptorSetAdd(&table->descriptorSet, &descriptor));

        __picoMpegTSFilterContextFlushPayloadAccumulator(filterContext, bytesConsumed);
    }

    // uint32_t crc32 = (filterContext->payloadAccumulator[0] << 24) |
    //                  (filterContext->payloadAccumulator[1] << 16) |
    //                  (filterContext->payloadAccumulator[2] << 8) |
    //                  filterContext->payloadAccumulator[3];
    // NOTE: we do not do any CRC verification now.

    return PICO_MPEGTS_RESULT_SUCCESS;
}

static picoMpegTSResult __picoMpegTSParseTSDT(picoMpegTS mpegts, picoMpegTSTransportStreamDescriptionSectionPayload table, picoMpegTSFilterContext filterContext)
{
    PICO_ASSERT(mpegts != NULL);
    PICO_ASSERT(table != NULL);
    PICO_ASSERT(filterContext != NULL);

    size_t targetSize = filterContext->payloadAccumulatorSize - filterContext->expectedPayloadSize + 4;
    while (filterContext->payloadAccumulatorSize > targetSize) {
        picoMpegTSDescriptor_t descriptor = {0};
        size_t bytesConsumed              = 0;

        PICO_MPEGTS_RETURN_ON_ERROR(__picoMpegTSDescriptorParse(&descriptor,
                                                                filterContext->payloadAccumulator,
                                                                filterContext->payloadAccumulatorSize - targetSize,
                                                                &bytesConsumed));

        PICO_MPEGTS_RETURN_ON_ERROR(__picoMpegTSDescriptorSetAdd(&table->descriptorSet, &descriptor));

        __picoMpegTSFilterContextFlushPayloadAccumulator(filterContext, bytesConsumed);
    }

    // uint32_t crc32 = (filterContext->payloadAccumulator[0] << 24) |
    //                  (filterContext->payloadAccumulator[1] << 16) |
    //                  (filterContext->payloadAccumulator[2] << 8) |
    //                  filterContext->payloadAccumulator[3];
    // NOTE: we do not do any CRC verification now.

    return PICO_MPEGTS_RESULT_SUCCESS;
}

static picoMpegTSResult __picoMpegTSParsePMT(picoMpegTS mpegts, picoMpegTSProgramMapSectionPayload table, picoMpegTSFilterContext filterContext)
{
    PICO_ASSERT(mpegts != NULL);
    PICO_ASSERT(table != NULL);
    PICO_ASSERT(filterContext != NULL);

    table->pcrPid = ((uint16_t)(filterContext->payloadAccumulator[0] & 0x1F) << 8) |
                    filterContext->payloadAccumulator[1];

    size_t targetSize = filterContext->payloadAccumulatorSize - filterContext->expectedPayloadSize + 4;

    __picoMpegTSFilterContextFlushPayloadAccumulator(filterContext, 2);
    PICO_MPEGTS_RETURN_ON_ERROR(__picoMpegTSDescriptorSetParse(&table->programInfoDescriptorSet, filterContext, true));

    while (filterContext->payloadAccumulatorSize > targetSize) {
        if (table->streamCount == PICO_MPEGTS_MAX_TABLE_PAYLOAD_COUNT) {
            return PICO_MPEGTS_RESULT_TABLE_FULL;
        }

        table->streams[table->streamCount].streamType = (picoMpegTSStreamType)filterContext->payloadAccumulator[0];

        table->streams[table->streamCount].elementaryPid = ((uint16_t)(filterContext->payloadAccumulator[1] & 0x1F) << 8) |
                                                           filterContext->payloadAccumulator[2];

        __picoMpegTSFilterContextFlushPayloadAccumulator(filterContext, 3);

        PICO_MPEGTS_RETURN_ON_ERROR(__picoMpegTSDescriptorSetParse(&table->streams[table->streamCount].esInfoDescriptorSet, filterContext, true));

        table->streamCount++;
    }

    // uint32_t crc32 = (filterContext->payloadAccumulator[0] << 24) |
    //                  (filterContext->payloadAccumulator[1] << 16) |
    //                  (filterContext->payloadAccumulator[2] << 8) |
    //                  filterContext->payloadAccumulator[3];
    // NOTE: we do not do any CRC verification now.

    return PICO_MPEGTS_RESULT_SUCCESS;
}

static picoMpegTSResult __picoMpegTSOnTableReady(picoMpegTS mpegts, picoMpegTSTable oldTable, picoMpegTSTable newTable)
{
    PICO_ASSERT(mpegts != NULL);
    PICO_ASSERT(newTable != NULL);

    uint8_t tableId = newTable->tableId;

    // handle table-specific post-processing
    switch (tableId) {
        case PICO_MPEGTS_TABLE_ID_PAS:
            // clear filters for PIDs that were in old PAT but not in new PAT
            if (oldTable != NULL) {
                for (size_t i = 0; i < oldTable->data.pas.programCount; i++) {
                    uint16_t oldPid = oldTable->data.pas.programs[i].pid;
                    if (mpegts->pidFilters[oldPid] != NULL) {
                        __picoMpegTSDestroyFilterContext(mpegts, mpegts->pidFilters[oldPid]);
                        mpegts->pidFilters[oldPid] = NULL;
                    }
                }
            }

            // register filters for all PIDs in new PAT
            for (size_t i = 0; i < newTable->data.pas.programCount; i++) {
                uint16_t pid = newTable->data.pas.programs[i].pid;
                PICO_MPEGTS_RETURN_ON_ERROR(__picoMpegTSReplaceOrRegisterPSIFilter(mpegts, pid));
            }
            break;
        case PICO_MPEGTS_TABLE_ID_PMS:
            // clear filters for ES PIDs that were in old PMT but not in new PMT
            if (oldTable != NULL) {
                for (size_t i = 0; i < oldTable->data.pms.streamCount; i++) {
                    uint16_t oldPid = oldTable->data.pms.streams[i].elementaryPid;
                    if (mpegts->pidFilters[oldPid] != NULL) {
                        __picoMpegTSDestroyFilterContext(mpegts, mpegts->pidFilters[oldPid]);
                        mpegts->pidFilters[oldPid] = NULL;
                    }
                }
            }

            // register filters for all ES PIDs in new PMT
            for (size_t i = 0; i < newTable->data.pms.streamCount; i++) {
                uint16_t pid = newTable->data.pms.streams[i].elementaryPid;
                PICO_MPEGTS_RETURN_ON_ERROR(__picoMpegTSReplaceOrRegisterPESFilter(mpegts, pid));
            }
            break;

        default:
            // no post-processing needed for other tables
            break;
    }

    return PICO_MPEGTS_RESULT_SUCCESS;
}

static picoMpegTSResult __picoMpegTSTableAddSection(picoMpegTS mpegts, uint8_t tableId, picoMpegTSFilterContext filterContext)
{
    PICO_ASSERT(mpegts != NULL);
    PICO_ASSERT(filterContext != NULL);

    if (filterContext->filterType != PICO_MPEGTS_FILTER_TYPE_SECTION) {
        return PICO_MPEGTS_RESULT_INVALID_DATA;
    }

    picoMpegTSPSISectionHead head = &filterContext->head.psi;
    if (head->tableId != tableId) {
        return PICO_MPEGTS_RESULT_INVALID_DATA;
    }

    if (!head->currentNextIndicator) {
        PICO_MPEGTS_LOG("picoMpegTS: ignoring section %d.%d as its not current\n", tableId, head->sectionNumber);
        return PICO_MPEGTS_RESULT_SUCCESS;
    }

    uint8_t versionIndex = head->versionNumber % PICO_MPEGTS_MAX_VERSIONS;

    if (mpegts->parsedTables[tableId][versionIndex] != NULL &&
        mpegts->parsedTables[tableId][versionIndex]->versionNumber == head->versionNumber) {
        return PICO_MPEGTS_RESULT_SUCCESS;
    }

    if (mpegts->partialTables[tableId][versionIndex] == NULL) {
        mpegts->partialTables[tableId][versionIndex] = __picoMpegTSTableCreate(tableId, head->versionNumber);
    }

    picoMpegTSTable table = mpegts->partialTables[tableId][versionIndex];

    if (table->versionNumber != head->versionNumber) {
        __picoMpegTSTableDestroy(table);
        mpegts->partialTables[tableId][versionIndex] = __picoMpegTSTableCreate(tableId, head->versionNumber);
        table                                        = mpegts->partialTables[tableId][versionIndex];
    }

    if (table->hasSection[head->sectionNumber]) {
        return PICO_MPEGTS_RESULT_SUCCESS;
    }

    table->hasSection[head->sectionNumber] = true;
    table->head                            = *head;

    // PICO_MPEGTS_LOG("picoMpegTS: parsing section %d.%d v%d [%s] / [%s]\n",
    //                 tableId, head->sectionNumber, head->versionNumber,
    //                 picoMpegTSTableIDToString(tableId),
    //                 picoMpegTSPIDToString(filterContext->pid));

    switch (tableId) {
        case PICO_MPEGTS_TABLE_ID_PAS:
            PICO_MPEGTS_RETURN_ON_ERROR(__picoMpegTSParsePAT(mpegts, &table->data.pas, filterContext));
            break;

        case PICO_MPEGTS_TABLE_ID_CAS:
            PICO_MPEGTS_RETURN_ON_ERROR(__picoMpegTSParseCAT(mpegts, &table->data.cas, filterContext));
            break;

        case PICO_MPEGTS_TABLE_ID_PMS:
            PICO_MPEGTS_RETURN_ON_ERROR(__picoMpegTSParsePMT(mpegts, &table->data.pms, filterContext));
            break;

        case PICO_MPEGTS_TABLE_ID_TSDS:
            PICO_MPEGTS_RETURN_ON_ERROR(__picoMpegTSParseTSDT(mpegts, &table->data.tsds, filterContext));
            break;

        case PICO_MPEGTS_TABLE_ID_NISAN:
        case PICO_MPEGTS_TABLE_ID_NISON:
            PICO_MPEGTS_RETURN_ON_ERROR(__picoMpegTSParseNIT(mpegts, &table->data.nit, filterContext));
            break;

        case PICO_MPEGTS_TABLE_ID_BAS:
            PICO_MPEGTS_RETURN_ON_ERROR(__picoMpegTSParseBAT(mpegts, &table->data.bat, filterContext));
            break;

        case PICO_MPEGTS_TABLE_ID_SDSATS:
        case PICO_MPEGTS_TABLE_ID_SDSOTS:
            PICO_MPEGTS_RETURN_ON_ERROR(__picoMpegTSParseSDT(mpegts, &table->data.sdt, filterContext));
            break;

        case PICO_MPEGTS_TABLE_ID_EISATSF:
        case PICO_MPEGTS_TABLE_ID_EISOTSF:
            PICO_MPEGTS_RETURN_ON_ERROR(__picoMpegTSParseEIT(mpegts, &table->data.eit, filterContext));
            break;

        case PICO_MPEGTS_TABLE_ID_TDS:
            PICO_MPEGTS_RETURN_ON_ERROR(__picoMpegTSParseTDT(mpegts, &table->data.tdt, filterContext));
            break;

        case PICO_MPEGTS_TABLE_ID_RSS:
            PICO_MPEGTS_RETURN_ON_ERROR(__picoMpegTSParseRST(mpegts, &table->data.rst, filterContext));
            break;

        case PICO_MPEGTS_TABLE_ID_TOS:
            PICO_MPEGTS_RETURN_ON_ERROR(__picoMpegTSParseTOT(mpegts, &table->data.tot, filterContext));
            break;

        case PICO_MPEGTS_TABLE_ID_SIS:
            break;

        default:
            if ((tableId >= 0x50 && tableId <= 0x5F) || (tableId >= 0x60 && tableId <= 0x6F)) {
                PICO_MPEGTS_RETURN_ON_ERROR(__picoMpegTSParseEIT(mpegts, &table->data.eit, filterContext));
            }
            break;
    }

    bool allSectionsPresent = true;
    for (size_t i = 0; i <= table->head.lastSectionNumber; i++) {
        if (!table->hasSection[i]) {
            allSectionsPresent = false;
            break;
        }
    }

    if (allSectionsPresent) {
        table->completedTimestamp = __picoMpegTSGetCurrentTimestamp();

        if (mpegts->parsedTables[tableId][versionIndex] != NULL) {
            __picoMpegTSTableDestroy(mpegts->parsedTables[tableId][versionIndex]);
        }
        mpegts->parsedTables[tableId][versionIndex]  = table;
        mpegts->partialTables[tableId][versionIndex] = NULL;

        picoMpegTSTable latestTable = NULL;
        for (size_t v = 0; v < PICO_MPEGTS_MAX_VERSIONS; v++) {
            if (mpegts->parsedTables[tableId][v] != NULL) {
                if (__picoMpegTSIsVersionNewer(mpegts->parsedTables[tableId][v], latestTable)) {
                    latestTable = mpegts->parsedTables[tableId][v];
                }
            }
        }

        if (latestTable != NULL && latestTable != mpegts->tables[tableId]) {
            picoMpegTSTable oldTable = mpegts->tables[tableId];
            PICO_MPEGTS_RETURN_ON_ERROR(__picoMpegTSOnTableReady(mpegts, oldTable, latestTable));
            mpegts->tables[tableId] = latestTable;
        }
    }
    return PICO_MPEGTS_RESULT_SUCCESS;
}

static picoMpegTSResult __picoMpegTSParsePacketAdaptationFieldExtenstion(const uint8_t *data, uint8_t dataSize, picoMpegTSAdaptionFieldExtension afExt)
{
    PICO_ASSERT(afExt != NULL);
    PICO_ASSERT(data != NULL);

    if (dataSize < 1) {
        return PICO_MPEGTS_RESULT_INVALID_DATA;
    }

    {
        uint8_t flags                     = data[0];
        afExt->ltwFlag                    = (flags & 0x80) != 0;
        afExt->piecewiseRateFlag          = (flags & 0x40) != 0;
        afExt->seamlessSpliceFlag         = (flags & 0x20) != 0;
        afExt->afDescriptorNotPresentFlag = (flags & 0x10) != 0;
        dataSize -= 1;
        data += 1;
    }

    if (afExt->ltwFlag) {
        if (dataSize < 2) {
            return PICO_MPEGTS_RESULT_INVALID_DATA;
        }
        {
            // 1 bit valid + 15 bits offset
            uint16_t ltw        = (uint16_t)(data[0] << 8) | data[1];
            afExt->ltwValidFlag = (ltw & 0x8000) != 0;
            afExt->ltwOffset    = ltw & 0x7FFF;
            data += 2;
            dataSize -= 2;
        }
    }

    if (afExt->piecewiseRateFlag) {
        if (dataSize < 3) {
            return PICO_MPEGTS_RESULT_INVALID_DATA;
        }
        {
            // 2 reserved + 22 bits rate
            uint32_t pwr         = (uint32_t)(data[0] << 16) | (uint32_t)(data[1] << 8) | data[2];
            afExt->piecewiseRate = pwr & 0x3FFFFF;
            data += 3;
            dataSize -= 3;
        }
    }

    if (afExt->seamlessSpliceFlag) {
        if (dataSize < 5) {
            return PICO_MPEGTS_RESULT_INVALID_DATA;
        }
        {
            // Splice_type (4 bits) | DTS_next_AU[32..30] (3 bits) | marker_bit (1 bit)
            // DTS_next_AU[29..15] (15 bits) | marker_bit (1 bit)
            // DTS_next_AU[14..0] (15 bits) | marker_bit (1 bit)
            uint8_t spliceType = (data[0] >> 4) & 0x0F;
            afExt->spliceType  = spliceType;

            uint64_t dtsHigh = (uint64_t)((data[0] >> 1) & 0x07) << 30;           // DTS_next_AU[32..30]
            uint64_t dtsMid  = (uint64_t)((data[1] << 7) | (data[2] >> 1)) << 15; // DTS_next_AU[29..15]
            uint64_t dtsLow  = (uint64_t)((data[3] << 7) | (data[4] >> 1));       // DTS_next_AU[14..0]
            afExt->dtsNextAU = dtsHigh | dtsMid | dtsLow;

            data += 5;
            dataSize -= 5;
        }
    }

    if (!afExt->afDescriptorNotPresentFlag) {
        // we skip af_descriptor() parsing for now
        PICO_MPEGTS_LOG("picoMpegTS: af_descriptor() parsing not implemented yet.\n");
    }

    return PICO_MPEGTS_RESULT_SUCCESS;
}

static picoMpegTSResult __picoMpegTSParsePacketAdaptationField(const uint8_t *data, uint8_t dataSize, picoMpegTSPacketAdaptationField af)
{
    PICO_ASSERT(af != NULL);
    PICO_ASSERT(data != NULL);

    if (dataSize < 1) {
        return PICO_MPEGTS_RESULT_INVALID_DATA;
    }

    {
        uint8_t flags                         = data[0];
        af->discontinuityIndicator            = (flags & 0x80) != 0;
        af->randomAccessIndicator             = (flags & 0x40) != 0;
        af->elementaryStreamPriorityIndicator = (flags & 0x20) != 0;
        af->pcrFlag                           = (flags & 0x10) != 0;
        af->opcrFlag                          = (flags & 0x08) != 0;
        af->splicingPointFlag                 = (flags & 0x04) != 0;
        af->transportPrivateDataFlag          = (flags & 0x02) != 0;
        af->adaptationFieldExtensionFlag      = (flags & 0x01) != 0;
        dataSize -= 1;
        data += 1;
    }

    if (af->pcrFlag) {
        if (dataSize < 6) {
            return PICO_MPEGTS_RESULT_INVALID_DATA;
        }
        // 33 bits base + 6 reserved + 9 bits extension
        af->pcr.base      = ((uint64_t)data[0] << 25) | ((uint64_t)data[1] << 17) | ((uint64_t)data[2] << 9) | ((uint64_t)data[3] << 1) | ((data[4] & 0x80) >> 7);
        af->pcr.extension = ((data[4] & 0x01) << 8) | data[5];
        data += 6;
        dataSize -= 6;
    }

    if (af->opcrFlag) {
        if (dataSize < 6) {
            return PICO_MPEGTS_RESULT_INVALID_DATA;
        }
        // 33 bits base + 6 reserved + 9 bits extension
        af->opcr.base      = ((uint64_t)data[0] << 25) | ((uint64_t)data[1] << 17) | ((uint64_t)data[2] << 9) | ((uint64_t)data[3] << 1) | ((data[4] & 0x80) >> 7);
        af->opcr.extension = ((data[4] & 0x01) << 8) | data[5];
        data += 6;
        dataSize -= 6;
    }

    if (af->splicingPointFlag) {
        if (dataSize < 1) {
            return PICO_MPEGTS_RESULT_INVALID_DATA;
        }
        af->spliceCountdown = data[0];
        data += 1;
        dataSize -= 1;
    }

    if (af->transportPrivateDataFlag) {
        if (dataSize < 1) {
            return PICO_MPEGTS_RESULT_INVALID_DATA;
        }
        af->transportPrivateDataLength = data[0];
        data += 1;
        dataSize -= 1;

        if (dataSize < af->transportPrivateDataLength) {
            return PICO_MPEGTS_RESULT_INVALID_DATA;
        }
        memcpy(af->transportPrivateData, data, af->transportPrivateDataLength);
        data += af->transportPrivateDataLength;
        dataSize -= af->transportPrivateDataLength;
    }

    if (af->adaptationFieldExtensionFlag) {
        if (dataSize < 1) {
            return PICO_MPEGTS_RESULT_INVALID_DATA;
        }
        uint8_t afExtLength = data[0];
        data += 1;
        dataSize -= 1;

        if (dataSize < afExtLength) {
            return PICO_MPEGTS_RESULT_INVALID_DATA;
        }
        PICO_MPEGTS_RETURN_ON_ERROR(
            __picoMpegTSParsePacketAdaptationFieldExtenstion(
                data,
                afExtLength,
                &af->adaptationFieldExtension));
        data += afExtLength;
        dataSize -= afExtLength;
    }

    // rest is stuffing bytes

    return PICO_MPEGTS_RESULT_SUCCESS;
}

static bool __picoMpegTSIsPIDCustom(uint16_t pid)
{
    return (pid >= PICO_MPEGTS_PID_CUSTOM_START && pid <= PICO_MPEGTS_PID_CUSTOM_END);
}

static picoMpegTSResult __picoMpegTSParseSectionHead(const uint8_t *data, picoMpegTSPSISectionHead sectionHeadOut)
{
    PICO_ASSERT(data != NULL);
    PICO_ASSERT(sectionHeadOut != NULL);

    memset(sectionHeadOut, 0, sizeof(picoMpegTSPSISectionHead_t));
    sectionHeadOut->tableId              = data[0];
    sectionHeadOut->sectionLength        = (uint16_t)((data[1] & 0x0F) << 8) | data[2];
    sectionHeadOut->id                   = (uint16_t)(data[3] << 8) | data[4];
    sectionHeadOut->versionNumber        = (data[5] >> 1) & 0x1F;
    sectionHeadOut->currentNextIndicator = (data[5] & 0x01) != 0;
    sectionHeadOut->sectionNumber        = data[6];
    sectionHeadOut->lastSectionNumber    = data[7];

    return PICO_MPEGTS_RESULT_SUCCESS;
}

static picoMpegTSResult __picoMpegTSParsePESHead(const char *data, picoMpegTSPESHead pesHeadOut)
{
    PICO_ASSERT(data != NULL);
    PICO_ASSERT(pesHeadOut != NULL);

    pesHeadOut->streamId        = (uint8_t)data[3];
    pesHeadOut->pesPacketLength = (uint16_t)((data[4] << 8) | data[5]);

    return PICO_MPEGTS_RESULT_SUCCESS;
}

static picoMpegTSFilterContext __picoMpegTSCreateFilterContext(picoMpegTS mpegts, picoMpegTSFilterType filterType, uint16_t pid)
{
    PICO_ASSERT(mpegts != NULL);

    picoMpegTSFilterContext context = (picoMpegTSFilterContext)PICO_MALLOC(sizeof(picoMpegTSFilterContext_t));
    if (!context) {
        return NULL;
    }

    memset(context, 0, sizeof(picoMpegTSFilterContext_t));

    context->pid                        = pid;
    context->payloadAccumulator         = NULL;
    context->payloadAccumulatorSize     = 0;
    context->payloadAccumulatorCapacity = 0;
    context->expectedPayloadSize        = 0;

    context->lastContinuityCounter   = 16; // invalid initial value
    context->continuityErrorDetected = false;

    context->filterType = filterType;
    context->hasHead    = false;

    return context;
}

static void __picoMpegTSDestroyFilterContext(picoMpegTS mpegts, picoMpegTSFilterContext context)
{
    PICO_ASSERT(mpegts != NULL);
    PICO_ASSERT(context != NULL);

    if (context->payloadAccumulator) {
        PICO_FREE(context->payloadAccumulator);
    }
    PICO_FREE(context);
}

// remove N bytes from the start of the payload accumulator, shifting the rest to the front
// if N = 0, remove all bytes
static void __picoMpegTSFilterContextFlushPayloadAccumulator(picoMpegTSFilterContext filterContext, size_t byteCount)
{
    PICO_ASSERT(filterContext != NULL);

    if (byteCount == 0 || byteCount >= filterContext->payloadAccumulatorSize) {
        // remove all
        filterContext->payloadAccumulatorSize = 0;
    } else {
        // shift remaining data to the front
        size_t remainingSize = filterContext->payloadAccumulatorSize - byteCount;
        memmove(filterContext->payloadAccumulator, &filterContext->payloadAccumulator[byteCount], remainingSize);
        filterContext->payloadAccumulatorSize = remainingSize;
    }
}

static picoMpegTSResult __picoMpegTSFilterContextPushData(
    picoMpegTSFilterContext filterContext,
    const uint8_t *data,
    size_t dataSize)
{
    PICO_ASSERT(filterContext != NULL);
    PICO_ASSERT(data != NULL);

    // ensure capacity
    size_t requiredCapacity = filterContext->payloadAccumulatorSize + dataSize;
    if (requiredCapacity > filterContext->payloadAccumulatorCapacity) {
        size_t newCapacity = filterContext->payloadAccumulatorCapacity == 0 ? 184 : filterContext->payloadAccumulatorCapacity;
        while (newCapacity < requiredCapacity) {
            newCapacity *= 2;
        }
        uint8_t *newAccumulator = (uint8_t *)PICO_REALLOC(filterContext->payloadAccumulator, newCapacity);
        if (!newAccumulator) {
            return PICO_MPEGTS_RESULT_MALLOC_ERROR;
        }
        filterContext->payloadAccumulator         = newAccumulator;
        filterContext->payloadAccumulatorCapacity = newCapacity;
    }

    // copy data
    memcpy(&filterContext->payloadAccumulator[filterContext->payloadAccumulatorSize], data, dataSize);
    filterContext->payloadAccumulatorSize += dataSize;

    return PICO_MPEGTS_RESULT_SUCCESS;
}

static picoMpegTSResult __picoMpegTSFilterContextFlush(picoMpegTS mpegts, picoMpegTSFilterContext filterContext, size_t flushPayloadSize)
{
    PICO_ASSERT(mpegts != NULL);
    PICO_ASSERT(filterContext != NULL);

    // time to actually deal with the data
    if (filterContext->payloadAccumulatorSize > filterContext->expectedPayloadSize && filterContext->hasHead) {
        if (filterContext->filterType == PICO_MPEGTS_FILTER_TYPE_SECTION) {
            uint8_t tableId = filterContext->head.psi.tableId;
            PICO_MPEGTS_RETURN_ON_ERROR(__picoMpegTSTableAddSection(mpegts, tableId, filterContext));
        } else {
            PICO_MPEGTS_LOG("PES filters are not yet implemented");
            return PICO_MPEGTS_RESULT_UNKNOWN_ERROR;
        }
    }

    // flush the accumulator
    __picoMpegTSFilterContextFlushPayloadAccumulator(filterContext, flushPayloadSize);
    filterContext->hasHead             = false;
    filterContext->expectedPayloadSize = 0;
    memset(&filterContext->head.psi, 0, sizeof(picoMpegTSPSISectionHead_t));

    return PICO_MPEGTS_RESULT_SUCCESS;
}

static picoMpegTSResult __picoMpegTSFilterContextApply(picoMpegTSFilterContext filterContext, picoMpegTS mpegts, picoMpegTSPacket packet)
{
    PICO_ASSERT(filterContext != NULL);
    PICO_ASSERT(mpegts != NULL);
    PICO_ASSERT(packet != NULL);

    // Validate continuity counter if enabled
    // Per ITU-T H.222.0: The continuity_counter shall not be incremented when
    // adaptation_field_control equals '00' (reserved) or '10' (adaptation field only, no payload)
    bool hasPayload = (packet->adaptionFieldControl == PICO_MPEGTS_ADAPTATION_FIELD_CONTROL_PAYLOAD_ONLY ||
                       packet->adaptionFieldControl == PICO_MPEGTS_ADAPTATION_FIELD_CONTROL_BOTH);
    if (hasPayload) {
        if (filterContext->lastContinuityCounter != 16) {

            uint8_t expectedCC = (filterContext->lastContinuityCounter + 1) & 0x0F; // it is 4-bit, wraps at 16
            if (packet->continuityCounter != expectedCC) {
                // check for duplicate packet (same CC is allowed for duplicates)
                if (packet->continuityCounter != filterContext->lastContinuityCounter) {
                    filterContext->continuityErrorDetected |= true;
                    mpegts->hasContinuityError |= true;
                    PICO_MPEGTS_LOG("Continuity counter error on PID 0x%04X: expected %u, got %u\n",
                                    packet->pid, expectedCC, packet->continuityCounter);
                }
            }
        }
        filterContext->lastContinuityCounter = packet->continuityCounter;
    }

    // append the packet payload to the accumulator
    if (packet->payloadSize > 0) {
        // if the current packet has pusi set to 1, then the first byte of the payload is the pointer field
        // and the data starts after that (first we need to just push the data before the pointer field, call body, if there is any data alread)
        // then push the rest of the data after the pointer field, the call head
        size_t payloadOffset = 0;
        if (packet->payloadUnitStartIndicator) {
            uint8_t pointerField = packet->payload[0];
            // first push data before pointer field
            if (pointerField > 0) {
                size_t prePointerSize = pointerField;
                if (prePointerSize > packet->payloadSize - 1) {
                    prePointerSize = packet->payloadSize - 1;
                }
                PICO_MPEGTS_RETURN_ON_ERROR(
                    __picoMpegTSFilterContextPushData(
                        filterContext,
                        &packet->payload[1],
                        prePointerSize));
            }

            PICO_MPEGTS_RETURN_ON_ERROR(__picoMpegTSFilterContextFlush(mpegts, filterContext, 0));

            // now move the offset to after the pointer field
            payloadOffset = 1 + pointerField;
            PICO_MPEGTS_RETURN_ON_ERROR(
                __picoMpegTSFilterContextPushData(
                    filterContext,
                    &packet->payload[payloadOffset],
                    packet->payloadSize - payloadOffset));

            if (filterContext->filterType == PICO_MPEGTS_FILTER_TYPE_SECTION) {
                PICO_MPEGTS_RETURN_ON_ERROR(
                    __picoMpegTSParseSectionHead(
                        filterContext->payloadAccumulator,
                        &filterContext->head.psi));
                __picoMpegTSFilterContextFlushPayloadAccumulator(filterContext, 8);
                filterContext->expectedPayloadSize = filterContext->head.psi.sectionLength - 5;
            } else {
                PICO_MPEGTS_RETURN_ON_ERROR(
                    __picoMpegTSParsePESHead(
                        (const char *)filterContext->payloadAccumulator,
                        &filterContext->head.pes));
                __picoMpegTSFilterContextFlushPayloadAccumulator(filterContext, 6);
                filterContext->expectedPayloadSize = filterContext->head.pes.pesPacketLength;
            }
            filterContext->hasHead = true;
        } else {
            // no pointer field, just push all data
            PICO_MPEGTS_RETURN_ON_ERROR(
                __picoMpegTSFilterContextPushData(
                    filterContext,
                    &packet->payload[payloadOffset],
                    packet->payloadSize - payloadOffset));
        }

        // if the expected payload size is set, check if we have enough data to call body
        if (filterContext->expectedPayloadSize > 0 &&
            filterContext->payloadAccumulatorSize >= filterContext->expectedPayloadSize) {
            PICO_MPEGTS_RETURN_ON_ERROR(__picoMpegTSFilterContextFlush(mpegts, filterContext, filterContext->expectedPayloadSize));
        }
    }

    return PICO_MPEGTS_RESULT_SUCCESS;
}

static picoMpegTSResult __picoMpegTSFilterFlushAllContexts(picoMpegTS mpegts)
{
    PICO_ASSERT(mpegts != NULL);

    for (size_t pid = 0; pid < PICO_MPEGTS_MAX_PID_COUNT; pid++) {
        picoMpegTSFilterContext filterContext = mpegts->pidFilters[pid];
        if (filterContext) {
            PICO_MPEGTS_RETURN_ON_ERROR(__picoMpegTSFilterContextFlush(mpegts, filterContext, 0));
        }
    }

    return PICO_MPEGTS_RESULT_SUCCESS;
}

static picoMpegTSResult __picoMpegTSRegisterFilter(picoMpegTS mpegts, picoMpegTSPacketPID pid, picoMpegTSFilterType filterType)
{
    PICO_ASSERT(mpegts != NULL);

    picoMpegTSFilterContext filterContext = __picoMpegTSCreateFilterContext(mpegts, filterType, pid);

    if (!filterContext) {
        PICO_MPEGTS_LOG("picoMpegTS: Failed to create (%s) filter context for PID 0x%04X\n",
                        picoMpegTSFilterTypeToString(PICO_MPEGTS_FILTER_TYPE_SECTION),
                        pid);
        return PICO_MPEGTS_RESULT_MALLOC_ERROR;
    }

    mpegts->pidFilters[pid] = filterContext;

    return PICO_MPEGTS_RESULT_SUCCESS;
}

static picoMpegTSResult __picoMpegTSReplaceOrRegisterPSIFilter(picoMpegTS mpegts, uint16_t pid)
{
    PICO_ASSERT(mpegts != NULL);

    if (mpegts->pidFilters[pid] != NULL) {
        picoMpegTSFilterContext oldFilter = mpegts->pidFilters[pid];
        PICO_MPEGTS_RETURN_ON_ERROR(__picoMpegTSFilterContextFlush(mpegts, oldFilter, 0));
        __picoMpegTSDestroyFilterContext(mpegts, oldFilter);
        mpegts->pidFilters[pid] = NULL;
    }
    return __picoMpegTSRegisterFilter(mpegts, pid, PICO_MPEGTS_FILTER_TYPE_SECTION);
}

static picoMpegTSResult __picoMpegTSReplaceOrRegisterPESFilter(picoMpegTS mpegts, uint16_t pid)
{
    PICO_ASSERT(mpegts != NULL);

    if (mpegts->pidFilters[pid] != NULL) {
        picoMpegTSFilterContext oldFilter = mpegts->pidFilters[pid];
        PICO_MPEGTS_RETURN_ON_ERROR(__picoMpegTSFilterContextFlush(mpegts, oldFilter, 0));
        __picoMpegTSDestroyFilterContext(mpegts, oldFilter);
        mpegts->pidFilters[pid] = NULL;
    }
    return __picoMpegTSRegisterFilter(mpegts, pid, PICO_MPEGTS_FILTER_TYPE_PES);
}

static picoMpegTSResult __picoMpegTSRegisterPSIFilters(picoMpegTS mpegts)
{
    PICO_ASSERT(mpegts != NULL);
    PICO_MPEGTS_RETURN_ON_ERROR(__picoMpegTSReplaceOrRegisterPSIFilter(mpegts, PICO_MPEGTS_PID_PAT));
    PICO_MPEGTS_RETURN_ON_ERROR(__picoMpegTSReplaceOrRegisterPSIFilter(mpegts, PICO_MPEGTS_PID_CAT));
    PICO_MPEGTS_RETURN_ON_ERROR(__picoMpegTSReplaceOrRegisterPSIFilter(mpegts, PICO_MPEGTS_PID_TSDT));
    PICO_MPEGTS_RETURN_ON_ERROR(__picoMpegTSReplaceOrRegisterPSIFilter(mpegts, PICO_MPEGTS_PID_NIT));
    PICO_MPEGTS_RETURN_ON_ERROR(__picoMpegTSReplaceOrRegisterPSIFilter(mpegts, PICO_MPEGTS_PID_SDT_BAT));
    PICO_MPEGTS_RETURN_ON_ERROR(__picoMpegTSReplaceOrRegisterPSIFilter(mpegts, PICO_MPEGTS_PID_EIT));
    PICO_MPEGTS_RETURN_ON_ERROR(__picoMpegTSReplaceOrRegisterPSIFilter(mpegts, PICO_MPEGTS_PID_RST));
    PICO_MPEGTS_RETURN_ON_ERROR(__picoMpegTSReplaceOrRegisterPSIFilter(mpegts, PICO_MPEGTS_PID_TDT_TOT));
    // The following two arent needed now, TODO: implement these
    // PICO_MPEGTS_RETURN_ON_ERROR(__picoMpegTSReplaceOrRegisterPSIFilter(mpegts, PICO_MPEGTS_PID_DIT));
    // PICO_MPEGTS_RETURN_ON_ERROR(__picoMpegTSReplaceOrRegisterPSIFilter(mpegts, PICO_MPEGTS_PID_SIT));

    return PICO_MPEGTS_RESULT_SUCCESS;
}

// ---------------------------------- Public functions implementation ----------------------

picoMpegTSResult picoMpegTSParsePacket(const uint8_t *data, picoMpegTSPacket packet)
{
    PICO_ASSERT(packet != NULL);
    PICO_ASSERT(data != NULL);

    if (data[0] != 0x47) {
        return PICO_MPEGTS_RESULT_INVALID_DATA;
    }

    uint16_t header                   = (uint16_t)(data[1] << 8) | data[2];
    packet->errorIndicator            = (header & 0x8000) != 0;
    packet->payloadUnitStartIndicator = (header & 0x4000) != 0;
    packet->transportPriority         = (header & 0x2000) != 0;
    packet->pid                       = header & 0x1FFF;
    uint8_t flags                     = data[3];
    packet->scramblingControl         = (flags & 0xC0) >> 6;
    packet->adaptionFieldControl      = (picoMpegTSAdaptionFieldControl)((flags & 0x30) >> 4);
    packet->continuityCounter         = flags & 0x0F;

    size_t payloadOffset = 4;
    if (packet->adaptionFieldControl == PICO_MPEGTS_ADAPTATION_FIELD_CONTROL_ADAPTATION_ONLY ||
        packet->adaptionFieldControl == PICO_MPEGTS_ADAPTATION_FIELD_CONTROL_BOTH) {
        uint8_t adaptationFieldLength = data[4];
        payloadOffset += 1 + adaptationFieldLength;
        if (adaptationFieldLength > 0) {
            PICO_MPEGTS_RETURN_ON_ERROR(
                __picoMpegTSParsePacketAdaptationField(
                    &data[5],
                    adaptationFieldLength,
                    &packet->adaptionField));
        }
    }

    if (packet->adaptionFieldControl == PICO_MPEGTS_ADAPTATION_FIELD_CONTROL_PAYLOAD_ONLY ||
        packet->adaptionFieldControl == PICO_MPEGTS_ADAPTATION_FIELD_CONTROL_BOTH) {
        size_t payloadSize = 188 - payloadOffset;
        if (payloadSize > sizeof(packet->payload)) {
            payloadSize = sizeof(packet->payload);
        }
        memcpy(packet->payload, &data[payloadOffset], payloadSize);
        packet->payloadSize = (uint8_t)payloadSize;
    } else {
        packet->payloadSize = 0;
    }
    return PICO_MPEGTS_RESULT_SUCCESS;
}

picoMpegTSTable picoMpegTSGetTable(picoMpegTS mpegts, picoMpegTSTableID tableID)
{
    PICO_ASSERT(mpegts != NULL);
    return mpegts->tables[tableID];
}

picoMpegTS picoMpegTSCreate(bool storeParsedPackets)
{
    picoMpegTS mpegts = (picoMpegTS)PICO_MALLOC(sizeof(picoMpegTS_t));
    if (!mpegts) {
        return NULL;
    }

    memset(mpegts, 0, sizeof(picoMpegTS_t));

    if (__picoMpegTSRegisterPSIFilters(mpegts) != PICO_MPEGTS_RESULT_SUCCESS) {
        PICO_FREE(mpegts);
        return NULL;
    }

    mpegts->storeParsedPackets = storeParsedPackets;
    if (storeParsedPackets) {
        mpegts->parsedPackets = (picoMpegTSPacket)PICO_MALLOC(sizeof(picoMpegTSPacket_t) * PICO_INITIAL_PARSED_PACKETS_CAPACITY);
        if (!mpegts->parsedPackets) {
            PICO_FREE(mpegts);
            return NULL;
        }
        mpegts->parsedPacketCount    = 0;
        mpegts->parsedPacketCapacity = PICO_INITIAL_PARSED_PACKETS_CAPACITY;
    }

    return mpegts;
}

void picoMpegTSDestroy(picoMpegTS mpegts)
{
    PICO_ASSERT(mpegts != NULL);
    if (mpegts->storeParsedPackets && mpegts->parsedPackets) {
        PICO_FREE(mpegts->parsedPackets);
    }

    for (size_t i = 0; i < PICO_MPEGTS_MAX_PID_COUNT; i++) {
        if (mpegts->pidFilters[i]) {
            __picoMpegTSDestroyFilterContext(mpegts, mpegts->pidFilters[i]);
        }
    }

    for (size_t i = 0; i < PICO_MPEGTS_MAX_TABLE_COUNT; i++) {
        for (size_t v = 0; v < PICO_MPEGTS_MAX_VERSIONS; v++) {
            if (mpegts->partialTables[i][v]) {
                __picoMpegTSTableDestroy(mpegts->partialTables[i][v]);
            }
            if (mpegts->parsedTables[i][v]) {
                __picoMpegTSTableDestroy(mpegts->parsedTables[i][v]);
            }
        }
    }

    PICO_FREE(mpegts);
}

picoMpegTSResult picoMpegTSGetParsedPackets(picoMpegTS mpegts, picoMpegTSPacket *packetsOut, size_t *packetCountOut)
{
    PICO_ASSERT(mpegts != NULL);
    PICO_ASSERT(packetsOut != NULL);
    PICO_ASSERT(packetCountOut != NULL);

    if (!mpegts->storeParsedPackets) {
        return PICO_MPEGTS_RESULT_INVALID_ARGUMENTS;
    }

    *packetsOut     = mpegts->parsedPackets;
    *packetCountOut = mpegts->parsedPacketCount;

    return PICO_MPEGTS_RESULT_SUCCESS;
}

void picoMpegTSDebugPrint(picoMpegTS mpegts, picoMpegTSDebugPrintInfo info)
{
    PICO_ASSERT(mpegts != NULL);

    PICO_MPEGTS_LOG("-----------------------------------------------------\n");
    if (info->printParsedPackets && mpegts->storeParsedPackets) {
        PICO_MPEGTS_LOG("Parsed Packets: %zu\n", mpegts->parsedPacketCount);
        for (size_t i = 0; i < mpegts->parsedPacketCount; i++) {
            picoMpegTSPacketDebugPrint(&mpegts->parsedPackets[i]);
        }
        PICO_MPEGTS_LOG("-----------------------------------------------------\n");
    }

    if (info->printCurrentTables) {
        PICO_MPEGTS_LOG("Current Tables:\n");
        for (size_t i = 0; i < PICO_MPEGTS_MAX_TABLE_COUNT; i++) {
            if (mpegts->tables[i]) {
                picoMpegTSTableDebugPrint(mpegts->tables[i]);
            }
        }
        PICO_MPEGTS_LOG("-----------------------------------------------------\n");
    }

    if (info->printParsedTables) {
        printf("Parsed Tables:\n");
        for (size_t i = 0; i < PICO_MPEGTS_MAX_TABLE_COUNT; i++) {
            for (size_t v = 0; v < PICO_MPEGTS_MAX_VERSIONS; v++) {
                if (mpegts->parsedTables[i][v]) {
                    printf("Parsed Table ID %zu (0x%02zX) Version %zu:\n", i, i, v);
                    picoMpegTSTableDebugPrint(mpegts->parsedTables[i][v]);
                }
            }
        }
        PICO_MPEGTS_LOG("-----------------------------------------------------\n");
    }

    if (info->printPartialTables) {
        printf("Partial Tables:\n");
        for (size_t i = 0; i < PICO_MPEGTS_MAX_TABLE_COUNT; i++) {
            for (size_t v = 0; v < PICO_MPEGTS_MAX_VERSIONS; v++) {
                if (mpegts->partialTables[i][v]) {
                    printf("Partial Table ID %zu (0x%02zX) Version %zu:\n", i, i, v);
                    picoMpegTSTableDebugPrint(mpegts->partialTables[i][v]);
                }
            }
        }
        PICO_MPEGTS_LOG("-----------------------------------------------------\n");
    }
}

// NOTE: Irrespective of type of packet we just use the first 188 bytes for parsing
// the rest of the portion isnt important for most use-cases and is
// out of the scope of this library for now
picoMpegTSResult picoMpegTSAddPacket(picoMpegTS mpegts, const uint8_t *data)
{
    PICO_ASSERT(mpegts != NULL);
    PICO_ASSERT(data != NULL);

    picoMpegTSPacket_t packet = {0};
    PICO_MPEGTS_RETURN_ON_ERROR(picoMpegTSParsePacket(data, &packet));

    // add packet to parsed packets if needed
    if (mpegts->storeParsedPackets) {
        if (mpegts->parsedPacketCount >= mpegts->parsedPacketCapacity) {
            size_t newCapacity         = mpegts->parsedPacketCapacity * 2;
            picoMpegTSPacket newBuffer = (picoMpegTSPacket)PICO_REALLOC(mpegts->parsedPackets, sizeof(picoMpegTSPacket_t) * newCapacity);
            if (!newBuffer) {
                return PICO_MPEGTS_RESULT_MALLOC_ERROR;
            }
            mpegts->parsedPackets        = newBuffer;
            mpegts->parsedPacketCapacity = newCapacity;
        }
        memcpy(&mpegts->parsedPackets[mpegts->parsedPacketCount++], &packet, sizeof(picoMpegTSPacket_t));
    }

    if (packet.pid == PICO_MPEGTS_PID_NULL_PACKET) {
        return PICO_MPEGTS_RESULT_SUCCESS;
    }

    picoMpegTSFilterContext filterContext = mpegts->pidFilters[packet.pid];

    // if there is no filter for this PID and it is not a custom PID then this is an error
    if (!filterContext && !__picoMpegTSIsPIDCustom(packet.pid)) {
        PICO_MPEGTS_LOG("picoMpegTS: Unknown PID: 0x%04X [%s]\n", packet.pid, picoMpegTSPIDToString(packet.pid));
        // TODO: Maybe just skip the packet?
        return PICO_MPEGTS_RESULT_UNKNOWN_PID_PACKET;
    }

    if (filterContext) {
        return __picoMpegTSFilterContextApply(filterContext, mpegts, &packet);
    }

    // if the packet is of custom PID without a filter, then we
    // only process it if it has payloadUnitStartIndicator set
    // at that case we cant register a pes filter
    // otherwise we just ignore the packet

    if (__picoMpegTSIsPIDCustom(packet.pid) && !packet.payloadUnitStartIndicator) {
        // TODO: not sure fi we need to handle this manually? as proper pes filters
        // should get registered by respective table updates (pmst/sdt/eit)
        return PICO_MPEGTS_RESULT_SUCCESS;
    }

    mpegts->ignoredPacketCount++;
    // PICO_MPEGTS_LOG("Ignoring packet with custom PID 0x%04X without filter.\n", packet.pid);

    return PICO_MPEGTS_RESULT_SUCCESS;
}

picoMpegTSResult picoMpegTSAddBuffer(picoMpegTS mpegts, const uint8_t *buffer, size_t size)
{
    PICO_ASSERT(mpegts != NULL);
    PICO_ASSERT(buffer != NULL);

    picoMpegTSPacketType packetType = picoMpegTSDetectPacketType(buffer, size);
    if (packetType == PICO_MPEGTS_PACKET_TYPE_UNKNOWN) {
        return PICO_MPEGTS_RESULT_INVALID_DATA;
    }

    size_t packetSize = (size_t)packetType;
    size_t offset     = 0;
    while (offset + packetSize <= size) {
        if (buffer[offset] != 0x47) {
            offset++;
            continue;
        }

        PICO_MPEGTS_RETURN_ON_ERROR(picoMpegTSAddPacket(mpegts, &buffer[offset]));

        offset += packetSize;
    }

    PICO_MPEGTS_RETURN_ON_ERROR(__picoMpegTSFilterFlushAllContexts(mpegts));

    return PICO_MPEGTS_RESULT_SUCCESS;
}

picoMpegTSResult picoMpegTSAddFile(picoMpegTS mpegts, const char *filename)
{
    PICO_ASSERT(mpegts != NULL);
    PICO_ASSERT(filename != NULL);

    FILE *file = fopen(filename, "rb");
    if (!file) {
        return PICO_MPEGTS_RESULT_FILE_NOT_FOUND;
    }

    fseek(file, 0, SEEK_END);
    size_t fileSize = ftell(file);
    fseek(file, 0, SEEK_SET);
    uint8_t *buffer = (uint8_t *)PICO_MALLOC(fileSize);
    if (!buffer) {
        fclose(file);
        return PICO_MPEGTS_RESULT_MALLOC_ERROR;
    }

    size_t bytesRead = fread(buffer, 1, fileSize, file);
    fclose(file);

    if (bytesRead != fileSize) {
        PICO_FREE(buffer);
        return PICO_MPEGTS_RESULT_INVALID_DATA;
    }
    return picoMpegTSAddBuffer(mpegts, buffer, fileSize);
}

picoMpegTSPacketType picoMpegTSDetectPacketType(const uint8_t *data, size_t size)
{
    // first find the first sync byte (0x47)
    size_t offset = 0;
    while (offset < size && data[offset] != 0x47) {
        offset++;
    }

    // we couldn't find a sync byte so the packet type is unknown
    if (offset >= size) {
        return PICO_MPEGTS_PACKET_TYPE_UNKNOWN;
    }

    size_t candidatePacketSize[] = {
        PICO_MPEGTS_PACKET_TYPE_DEFAULT,
        PICO_MPEGTS_PACKET_TYPE_M2TS,
        PICO_MPEGTS_PACKET_TYPE_DVB};
    size_t packetCounts[3] = {0, 0, 0};

    // loop through the data, deteting sync bytes at expected intervals, and counting valid packets
    // if no sync byte was found at any expected interval, we return unknown
    // if we find sync byte at any interval we move forward by that packet size and continue checking
    for (size_t i = offset; i < size;) {
        bool foundSync = false;
        for (size_t j = 0; j < sizeof(candidatePacketSize) / sizeof(candidatePacketSize[0]); j++) {
            size_t packetSize = candidatePacketSize[j];
            if (i + packetSize < size && data[i + packetSize] == 0x47) {
                packetCounts[j]++;
                i += packetSize;
                foundSync = true;
                break;
            }
        }
        if (!foundSync) {
            break;
        }
    }

    // determine which packet size had the most valid packets
    size_t maxCount = 0;
    size_t maxIndex = 0;
    for (size_t j = 0; j < sizeof(candidatePacketSize) / sizeof(candidatePacketSize[0]); j++) {
        if (packetCounts[j] > maxCount) {
            maxCount = packetCounts[j];
            maxIndex = j;
        }
    }

    if (maxCount == 0) {
        return PICO_MPEGTS_PACKET_TYPE_UNKNOWN;
    }

    return (picoMpegTSPacketType)candidatePacketSize[maxIndex];
}

picoMpegTSPacketType picoMpegTSDetectPacketTypeFromFile(const char *filename)
{
    FILE *file = fopen(filename, "rb");
    if (!file) {
        return PICO_MPEGTS_PACKET_TYPE_UNKNOWN;
    }

    // we only need to probe a small amount of data
    // to detect the packet type
    uint8_t buffer[4096];
    size_t bytesRead = fread(buffer, 1, sizeof(buffer), file);
    fclose(file);

    if (bytesRead == 0) {
        return PICO_MPEGTS_PACKET_TYPE_UNKNOWN;
    }

    return picoMpegTSDetectPacketType(buffer, bytesRead);
}

const char *picoMpegTSPacketTypeToString(picoMpegTSPacketType type)
{
    switch (type) {
        case PICO_MPEGTS_PACKET_TYPE_DEFAULT:
            return "MPEG-TS (188 bytes)";
        case PICO_MPEGTS_PACKET_TYPE_M2TS:
            return "M2TS (192 bytes)";
        case PICO_MPEGTS_PACKET_TYPE_DVB:
            return "DVB (204 bytes)";
        case PICO_MPEGTS_PACKET_TYPE_UNKNOWN:
        default:
            return "Unknown";
    }
}

const char *picoMpegTSResultToString(picoMpegTSResult result)
{
    switch (result) {
        case PICO_MPEGTS_RESULT_SUCCESS:
            return "OK";
        case PICO_MPEGTS_RESULT_FILE_NOT_FOUND:
            return "FILE_NOT_FOUND";
        case PICO_MPEGTS_RESULT_MALLOC_ERROR:
            return "MEMORY_ALLOCATION_ERROR";
        case PICO_MPEGTS_RESULT_INVALID_DATA:
            return "INVALID_DATA";
        case PICO_MPEGTS_RESULT_INVALID_ARGUMENTS:
            return "INVALID_ARGUMENTS";
        case PICO_MPEGTS_RESULT_UNKNOWN_PID_PACKET:
            return "UNKNOWN_PID_PACKET";
        case PICO_MPEGTS_RESULT_TABLE_FULL:
            return "TABLE_FULL";
        default:
            return "UNKNOWN_ERROR";
    }
}

const char *picoMpegTSPIDToString(uint16_t pid)
{
    switch (pid) {
        case PICO_MPEGTS_PID_PAT:
            return "Program Association Table (PAT)";
        case PICO_MPEGTS_PID_CAT:
            return "Conditional Access Table (CAT)";
        case PICO_MPEGTS_PID_TSDT:
            return "Transport Stream Description Table (TSDT)";
        case PICO_MPEGTS_PID_IPMP:
            return "IPMP Control Information";
        case PICO_MPEGTS_PID_ASI:
            return "Auxiliary Section Information (ASI)";
        case PICO_MPEGTS_PID_NIT:
            return "Network Information Table (NIT)";
        case PICO_MPEGTS_PID_SDT_BAT:
            return "Service Description Table / Bouquet Association Table (SDT/BAT)";
        case PICO_MPEGTS_PID_EIT:
            return "Event Information Table (EIT)";
        case PICO_MPEGTS_PID_RST:
            return "Running Status Table (RST)";
        case PICO_MPEGTS_PID_TDT_TOT:
            return "Time and Date Table / Time Offset Table (TDT/TOT)";
        case PICO_MPEGTS_PID_NET_SYNC:
            return "Network Sync";
        case PICO_MPEGTS_PID_RNT:
            return "RAR Notification Table (RNT)";
        case PICO_MPEGTS_PID_LINK_LOCAL:
            return "Link Local";
        case PICO_MPEGTS_PID_MEASUREMENT:
            return "Measurement";
        case PICO_MPEGTS_PID_DIT:
            return "Discontinuity Information Table (DIT)";
        case PICO_MPEGTS_PID_SIT:
            return "Selection Information Table (SIT)";
        case PICO_MPEGTS_PID_NULL_PACKET:
            return "Null Packet";
        default:
            if (pid >= PICO_MPEGTS_PID_RESERVED_START && pid <= PICO_MPEGTS_PID_RESERVED_END) {
                return "Reserved";
            }
            if (pid >= PICO_MPEGTS_PID_CUSTOM_START && pid <= PICO_MPEGTS_PID_CUSTOM_END) {
                return "Custom PID";
            }
            return "Unknown PID";
    }
}

const char *picoMpegTSTableIDToString(uint8_t tableID)
{
    switch (tableID) {
        case PICO_MPEGTS_TABLE_ID_PAS:
            return "Program Association Section (PAS)";
        case PICO_MPEGTS_TABLE_ID_CAS:
            return "Conditional Access Section (CAS)";
        case PICO_MPEGTS_TABLE_ID_PMS:
            return "Program Map Section (PMS)";
        case PICO_MPEGTS_TABLE_ID_TSDS:
            return "Transport Stream Description Section (TSDS)";
        case PICO_MPEGTS_TABLE_ID_ISDDS:
            return "ISO/IEC 14496 Scene Description Section (ISDDS)";
        case PICO_MPEGTS_TABLE_ID_ISODS:
            return "ISO/IEC 14496 Object Descriptor Section (ISODS)";
        case PICO_MPEGTS_TABLE_ID_METAS:
            return "Metadata Section (METAS)";
        case PICO_MPEGTS_TABLE_ID_IPMP:
            return "IPMP Control Information Section (IPMP)";
        case PICO_MPEGTS_TABLE_ID_ISOS:
            return "ISO/IEC 14496 Section (ISOS)";
        case PICO_MPEGTS_TABLE_ID_GAUS:
            return "ISO/IEC 23001-11 (Green access unit) section (GAUS)";
        case PICO_MPEGTS_TABLE_ID_QAUS:
            return "ISO/IEC 23001-10 (Quality access unit) section (QAUS)";
        case PICO_MPEGTS_TABLE_ID_MOAUS:
            return "ISO/IEC 23001-13 (Media Orchestration access unit) section (MOAUS)";
        case PICO_MPEGTS_TABLE_ID_NISAN:
            return "Network Information Section (Actual) (NIS - Actual)";
        case PICO_MPEGTS_TABLE_ID_NISON:
            return "Network Information Section (Other) (NIS - Other)";
        case PICO_MPEGTS_TABLE_ID_SDSATS:
            return "Service Description Section (Actual TS) (SDS - ATS)";
        case PICO_MPEGTS_TABLE_ID_SDSOTS:
            return "Service Description Section (Other TS) (SDS - OTS)";
        case PICO_MPEGTS_TABLE_ID_BAS:
            return "Bouquet Association Section (BAS)";
        case PICO_MPEGTS_TABLE_ID_UNTS:
            return "Update Notification Table Section (UNTS)";
        case PICO_MPEGTS_TABLE_ID_DFIS:
            return "Downloadable Font Info Section (DFIS)";
        case PICO_MPEGTS_TABLE_ID_EISATSF:
            return "Event Information Section (Actual TS, Present/Following) (EIS)";
        case PICO_MPEGTS_TABLE_ID_EISOTSF:
            return "Event Information Section (Other TS, Present/Following) (EIS)";
        case PICO_MPEGTS_TABLE_ID_TDS:
            return "Time Date Section (TDS)";
        case PICO_MPEGTS_TABLE_ID_RSS:
            return "Running Status Section (RSS)";
        case PICO_MPEGTS_TABLE_ID_SS:
            return "Stuffing Section (SS)";
        case PICO_MPEGTS_TABLE_ID_TOS:
            return "Time Offset Section (TOS)";
        case PICO_MPEGTS_TABLE_ID_AIS:
            return "Application Information Section (AIS)";
        case PICO_MPEGTS_TABLE_ID_CS:
            return "Container Section (CS)";
        case PICO_MPEGTS_TABLE_ID_RCS:
            return "Related Content Section (RCS)";
        case PICO_MPEGTS_TABLE_ID_CIS:
            return "Content Identifier Section (CIS)";
        case PICO_MPEGTS_TABLE_ID_MPEFCS:
            return "MPE-FEC Section (MPE-FEC)";
        case PICO_MPEGTS_TABLE_ID_RPNS:
            return "Resolution Provider Notification Section (RPNS)";
        case PICO_MPEGTS_TABLE_ID_MPEIFECS:
            return "MPE-IFEC Section (MPE-IFEC)";
        case PICO_MPEGTS_TABLE_ID_PMSGS:
            return "Protection Message Section (PMSGS)";
        case PICO_MPEGTS_TABLE_ID_DIS:
            return "Discontinuity Information Section (DIS)";
        case PICO_MPEGTS_TABLE_ID_SIS:
            return "Selection Information Section (SIS)";
        default:
            if (tableID >= 0x50 && tableID <= 0x5F) {
                return "Event Information Section (Actual TS, Schedule) (EIS)";
            }
            if (tableID >= 0x60 && tableID <= 0x6F) {
                return "Event Information Section (Other TS, Schedule) (EIS)";
            }
            if (tableID >= PICO_MPEGTS_TABLE_ID_USER_DEFINED_START && tableID <= PICO_MPEGTS_TABLE_ID_USER_DEFINED_END) {
                return "User Defined Table ID";
            }
            if (tableID >= 0x0C && tableID <= 0x37) {
                return "Reserved Table ID";
            }
            if (tableID >= 0x38 && tableID <= 0x3F) {
                return "Defined in ISO/IEC 13818-6";
            }
            if (tableID == PICO_MPEGTS_TABLE_ID_FORBIDDEN) {
                return "Forbidden";
            }
            return "Unknown Table ID";
    }
}

const char *picoMpegTSAdaptionFieldControlToString(picoMpegTSAdaptionFieldControl afc)
{
    switch (afc) {
        case PICO_MPEGTS_ADAPTATION_FIELD_CONTROL_RESERVED:
            return "Reserved";
        case PICO_MPEGTS_ADAPTATION_FIELD_CONTROL_PAYLOAD_ONLY:
            return "Payload Only";
        case PICO_MPEGTS_ADAPTATION_FIELD_CONTROL_ADAPTATION_ONLY:
            return "Adaptation Field Only";
        case PICO_MPEGTS_ADAPTATION_FIELD_CONTROL_BOTH:
            return "Adaptation Field and Payload";
        default:
            return "Unknown";
    }
}

const char *picoMpegTSFilterTypeToString(picoMpegTSFilterType filterType)
{
    switch (filterType) {
        case PICO_MPEGTS_FILTER_TYPE_PES:
            return "PES Filter";
        case PICO_MPEGTS_FILTER_TYPE_SECTION:
            return "Section Filter";
        case PICO_MPEGTS_FILTER_TYPE_NULL:
            return "Null Filter";
        default:
            return "Unknown Filter Type";
    }
}

const char *picoMpegTSSDTRunningStatusToString(picoMpegTSSDTRunningStatus status)
{
    switch (status) {
        case PICO_MPEGTS_SDT_RUNNING_STATUS_UNDEFINED:
            return "Undefined";
        case PICO_MPEGTS_SDT_RUNNING_STATUS_NOT_RUNNING:
            return "Not Running";
        case PICO_MPEGTS_SDT_RUNNING_STATUS_STARTS_IN_FEW_SEC:
            return "Starts in Few Seconds";
        case PICO_MPEGTS_SDT_RUNNING_STATUS_PAUSING:
            return "Pausing";
        case PICO_MPEGTS_SDT_RUNNING_STATUS_RUNNING:
            return "Running";
        case PICO_MPEGTS_SDT_RUNNING_STATUS_SERVICE_OFF_AIR:
            return "Service Off Air";
        case PICO_MPEGTS_SDT_RUNNING_STATUS_RESERVED_6:
            return "Reserved (6)";
        case PICO_MPEGTS_SDT_RUNNING_STATUS_RESERVED_7:
            return "Reserved (7)";
        default:
            return "Unknown Status";
    }
}

const char *picoMpegTSServiceTypeToString(picoMpegTSServiceType type)
{
    switch (type) {
        case PICO_MPEGTS_SERVICE_TYPE_DIGITAL_TELEVISION:
            return "Digital Television";
        case PICO_MPEGTS_SERVICE_TYPE_DIGITAL_RADIO_SOUND:
            return "Digital Radio Sound";
        case PICO_MPEGTS_SERVICE_TYPE_TELETEXT:
            return "Teletext";
        case PICO_MPEGTS_SERVICE_TYPE_NVOD_REFERENCE:
            return "NVOD Reference";
        case PICO_MPEGTS_SERVICE_TYPE_NVOD_TIME_SHIFTED:
            return "NVOD Time-Shifted";
        case PICO_MPEGTS_SERVICE_TYPE_MOSAIC:
            return "Mosaic";
        case PICO_MPEGTS_SERVICE_TYPE_FM_RADIO:
            return "FM Radio";
        case PICO_MPEGTS_SERVICE_TYPE_DVB_SRM:
            return "DVB SRM";
        case PICO_MPEGTS_SERVICE_TYPE_ADVANCED_CODEC_DIGITAL_RADIO_SOUND:
            return "Advanced Codec Digital Radio Sound";
        case PICO_MPEGTS_SERVICE_TYPE_H264_AVC_MOSAIC:
            return "H.264/AVC Mosaic";
        case PICO_MPEGTS_SERVICE_TYPE_DATA_BROADCAST:
            return "Data Broadcast";
        case PICO_MPEGTS_SERVICE_TYPE_DVB_MHP:
            return "DVB MHP";
        case PICO_MPEGTS_SERVICE_TYPE_MPEG2_HD_DIGITAL_TELEVISION:
            return "MPEG-2 HD Digital Television";
        case PICO_MPEGTS_SERVICE_TYPE_H264_AVC_SD_DIGITAL_TELEVISION:
            return "H.264/AVC SD Digital Television";
        case PICO_MPEGTS_SERVICE_TYPE_H264_AVC_SD_NVOD_TIME_SHIFTED:
            return "H.264/AVC SD NVOD Time-Shifted";
        case PICO_MPEGTS_SERVICE_TYPE_H264_AVC_SD_NVOD_REFERENCE:
            return "H.264/AVC SD NVOD Reference";
        case PICO_MPEGTS_SERVICE_TYPE_H264_AVC_HD_DIGITAL_TELEVISION:
            return "H.264/AVC HD Digital Television";
        case PICO_MPEGTS_SERVICE_TYPE_H264_AVC_HD_NVOD_TIME_SHIFTED:
            return "H.264/AVC HD NVOD Time-Shifted";
        case PICO_MPEGTS_SERVICE_TYPE_H264_AVC_HD_NVOD_REFERENCE:
            return "H.264/AVC HD NVOD Reference";
        case PICO_MPEGTS_SERVICE_TYPE_H264_AVC_FCP_HD_DIGITAL_TELEVISION:
            return "H.264/AVC FCP HD Digital Television";
        case PICO_MPEGTS_SERVICE_TYPE_H264_AVC_FCP_HD_NVOD_TIME_SHIFTED:
            return "H.264/AVC FCP HD NVOD Time-Shifted";
        case PICO_MPEGTS_SERVICE_TYPE_H264_AVC_FCP_HD_NVOD_REFERENCE:
            return "H.264/AVC FCP HD NVOD Reference";
        case PICO_MPEGTS_SERVICE_TYPE_HEVC_DIGITAL_TELEVISION:
            return "HEVC Digital Television";
        case PICO_MPEGTS_SERVICE_TYPE_HEVC_UHD_DIGITAL_TELEVISION:
            return "HEVC UHD Digital Television";
        case PICO_MPEGTS_SERVICE_TYPE_RESERVED_FF:
            return "Reserved (0xFF)";
        default:
            return "Reserved/Unknown Service Type";
    }
}

const char *picoMpegTSAudioTypeToString(picoMpegTSAudioType type)
{
    switch (type) {
        case PICO_MPEGTS_AUDIO_TYPE_UNDEFINED:
            return "Undefined";
        case PICO_MPEGTS_AUDIO_TYPE_CLEAN_EFFECTS:
            return "Clean effects";
        case PICO_MPEGTS_AUDIO_TYPE_HEARING_IMPAIRED:
            return "Hearing impaired";
        case PICO_MPEGTS_AUDIO_TYPE_VISUAL_IMPAIRED_COMMENTARY:
            return "Visual impaired commentary";
        case PICO_MPEGTS_AUDIO_TYPE_PRIMARY:
            return "Primary";
        case PICO_MPEGTS_AUDIO_TYPE_NATIVE:
            return "Native";
        case PICO_MPEGTS_AUDIO_TYPE_EMERGENCY:
            return "Emergency";
        case PICO_MPEGTS_AUDIO_TYPE_PRIMARY_COMMENTARY:
            return "Primary commentary";
        case PICO_MPEGTS_AUDIO_TYPE_ALTERNATE_COMMENTARY:
            return "Alternate commentary";
        default:
            if (type >= 0x04 && type <= 0x7F) {
                return "User Private";
            }
            return "Reserved";
    }
}

const char *picoMpegTSContentNibbleLevel1ToString(uint8_t nibble)
{
    switch ((picoMpegTSContentNibbleLevel1)nibble) {
        case PICO_MPEGTS_CONTENT_NIBBLE_LEVEL_1_UNDEFINED:
            return "Undefined content";
        case PICO_MPEGTS_CONTENT_NIBBLE_LEVEL_1_MOVIE_DRAMA:
            return "Movie/Drama";
        case PICO_MPEGTS_CONTENT_NIBBLE_LEVEL_1_NEWS_CURRENT:
            return "News/Current affairs";
        case PICO_MPEGTS_CONTENT_NIBBLE_LEVEL_1_SHOW_GAME:
            return "Show/Game show";
        case PICO_MPEGTS_CONTENT_NIBBLE_LEVEL_1_SPORTS:
            return "Sports";
        case PICO_MPEGTS_CONTENT_NIBBLE_LEVEL_1_CHILDREN_YOUTH:
            return "Children's/Youth programmes";
        case PICO_MPEGTS_CONTENT_NIBBLE_LEVEL_1_MUSIC_BALLET:
            return "Music/Ballet/Dance";
        case PICO_MPEGTS_CONTENT_NIBBLE_LEVEL_1_ARTS_CULTURE:
            return "Arts/Culture (without music)";
        case PICO_MPEGTS_CONTENT_NIBBLE_LEVEL_1_SOCIAL_POLITICAL:
            return "Social/Political issues/Economics";
        case PICO_MPEGTS_CONTENT_NIBBLE_LEVEL_1_EDUCATION_SCIENCE:
            return "Education/Science/Factual topics";
        case PICO_MPEGTS_CONTENT_NIBBLE_LEVEL_1_LEISURE_HOBBIES:
            return "Leisure hobbies";
        case PICO_MPEGTS_CONTENT_NIBBLE_LEVEL_1_SPECIAL:
            return "Special characteristics";
        case PICO_MPEGTS_CONTENT_NIBBLE_LEVEL_1_ADULT:
            return "Adult";
        case PICO_MPEGTS_CONTENT_NIBBLE_LEVEL_1_USER_DEFINED:
            return "User defined";
        default:
            return "Reserved for future use";
    }
}

const char *picoMpegTSContentNibbleToString(picoMpegTSContentNibble nibble)
{
    switch (nibble) {
        case PICO_MPEGTS_CONTENT_UNDEFINED:
            return "Undefined content";

        // Movie/Drama
        case PICO_MPEGTS_CONTENT_MOVIE_DRAMA_GENERAL:
            return "movie/drama (general)";
        case PICO_MPEGTS_CONTENT_MOVIE_DETECTIVE_THRILLER:
            return "detective/thriller";
        case PICO_MPEGTS_CONTENT_MOVIE_ADVENTURE_WESTERN_WAR:
            return "adventure/western/war";
        case PICO_MPEGTS_CONTENT_MOVIE_SCIENCE_FICTION_FANTASY_HORROR:
            return "science fiction/fantasy/horror";
        case PICO_MPEGTS_CONTENT_MOVIE_COMEDY:
            return "comedy";
        case PICO_MPEGTS_CONTENT_MOVIE_SOAP_MELODRAMA_FOLKLORIC:
            return "soap/melodrama/folkloric";
        case PICO_MPEGTS_CONTENT_MOVIE_ROMANCE:
            return "romance";
        case PICO_MPEGTS_CONTENT_MOVIE_SERIOUS_CLASSICAL_RELIGIOUS:
            return "serious/classical/religious/historical movie/drama";
        case PICO_MPEGTS_CONTENT_MOVIE_ADULT_MOVIE_DRAMA:
            return "adult movie/drama";
        case PICO_MPEGTS_CONTENT_MOVIE_USER_DEFINED:
            return "movie user defined";

        // News/Current affairs
        case PICO_MPEGTS_CONTENT_NEWS_GENERAL:
            return "news/current affairs (general)";
        case PICO_MPEGTS_CONTENT_NEWS_WEATHER_REPORT:
            return "news/weather report";
        case PICO_MPEGTS_CONTENT_NEWS_MAGAZINE:
            return "news magazine";
        case PICO_MPEGTS_CONTENT_NEWS_DOCUMENTARY:
            return "documentary";
        case PICO_MPEGTS_CONTENT_NEWS_DISCUSSION_INTERVIEW_DEBATE:
            return "discussion/interview/debate";
        case PICO_MPEGTS_CONTENT_NEWS_USER_DEFINED:
            return "news user defined";

        // Show/Game show
        case PICO_MPEGTS_CONTENT_SHOW_GENERAL:
            return "show/game show (general)";
        case PICO_MPEGTS_CONTENT_SHOW_GAME_SHOW_QUIZ_CONTEST:
            return "game show/quiz/contest";
        case PICO_MPEGTS_CONTENT_SHOW_VARIETY_SHOW:
            return "variety show";
        case PICO_MPEGTS_CONTENT_SHOW_TALK_SHOW:
            return "talk show";
        case PICO_MPEGTS_CONTENT_SHOW_USER_DEFINED:
            return "show user defined";

        // Sports
        case PICO_MPEGTS_CONTENT_SPORTS_GENERAL:
            return "sports (general)";
        case PICO_MPEGTS_CONTENT_SPORTS_SPECIAL_EVENTS:
            return "special events (Olympic Games, World Cup, etc.)";
        case PICO_MPEGTS_CONTENT_SPORTS_MAGAZINES:
            return "sports magazines";
        case PICO_MPEGTS_CONTENT_SPORTS_FOOTBALL_SOCCER:
            return "football/soccer";
        case PICO_MPEGTS_CONTENT_SPORTS_TENNIS_SQUASH:
            return "tennis/squash";
        case PICO_MPEGTS_CONTENT_SPORTS_TEAM_SPORTS:
            return "team sports (excluding football)";
        case PICO_MPEGTS_CONTENT_SPORTS_ATHLETICS:
            return "athletics";
        case PICO_MPEGTS_CONTENT_SPORTS_MOTOR_SPORT:
            return "motor sport";
        case PICO_MPEGTS_CONTENT_SPORTS_WATER_SPORT:
            return "water sport";
        case PICO_MPEGTS_CONTENT_SPORTS_WINTER_SPORTS:
            return "winter sports";
        case PICO_MPEGTS_CONTENT_SPORTS_EQUESTRIAN:
            return "equestrian";
        case PICO_MPEGTS_CONTENT_SPORTS_MARTIAL_SPORTS:
            return "martial sports";
        case PICO_MPEGTS_CONTENT_SPORTS_USER_DEFINED:
            return "sports user defined";

        // Children's/Youth programmes
        case PICO_MPEGTS_CONTENT_CHILDREN_GENERAL:
            return "children's/youth programmes (general)";
        case PICO_MPEGTS_CONTENT_CHILDREN_PRE_SCHOOL:
            return "pre-school children's programmes";
        case PICO_MPEGTS_CONTENT_CHILDREN_ENTERTAINMENT_6_14:
            return "entertainment programmes for 6 to 14";
        case PICO_MPEGTS_CONTENT_CHILDREN_ENTERTAINMENT_10_16:
            return "entertainment programmes for 10 to 16";
        case PICO_MPEGTS_CONTENT_CHILDREN_INFORMATIONAL:
            return "informational/educational/school programmes";
        case PICO_MPEGTS_CONTENT_CHILDREN_CARTOONS_PUPPETS:
            return "cartoons/puppets";
        case PICO_MPEGTS_CONTENT_CHILDREN_USER_DEFINED:
            return "children user defined";

        // Music/Ballet/Dance
        case PICO_MPEGTS_CONTENT_MUSIC_GENERAL:
            return "music/ballet/dance (general)";
        case PICO_MPEGTS_CONTENT_MUSIC_ROCK_POP:
            return "rock/pop";
        case PICO_MPEGTS_CONTENT_MUSIC_CLASSICAL:
            return "serious music/classical music";
        case PICO_MPEGTS_CONTENT_MUSIC_FOLK_TRADITIONAL:
            return "folk/traditional music";
        case PICO_MPEGTS_CONTENT_MUSIC_JAZZ:
            return "jazz";
        case PICO_MPEGTS_CONTENT_MUSIC_MUSICAL_OPERA:
            return "musical/opera";
        case PICO_MPEGTS_CONTENT_MUSIC_BALLET:
            return "ballet";
        case PICO_MPEGTS_CONTENT_MUSIC_USER_DEFINED:
            return "music user defined";

        // Arts/Culture
        case PICO_MPEGTS_CONTENT_ARTS_GENERAL:
            return "arts/culture (without music, general)";
        case PICO_MPEGTS_CONTENT_ARTS_PERFORMING_ARTS:
            return "performing arts";
        case PICO_MPEGTS_CONTENT_ARTS_FINE_ARTS:
            return "fine arts";
        case PICO_MPEGTS_CONTENT_ARTS_RELIGION:
            return "religion";
        case PICO_MPEGTS_CONTENT_ARTS_POPULAR_CULTURE:
            return "popular culture/traditional arts";
        case PICO_MPEGTS_CONTENT_ARTS_LITERATURE:
            return "literature";
        case PICO_MPEGTS_CONTENT_ARTS_FILM_CINEMA:
            return "film/cinema";
        case PICO_MPEGTS_CONTENT_ARTS_EXPERIMENTAL_FILM_VIDEO:
            return "experimental film/video";
        case PICO_MPEGTS_CONTENT_ARTS_BROADCASTING_PRESS:
            return "broadcasting/press";
        case PICO_MPEGTS_CONTENT_ARTS_NEW_MEDIA:
            return "new media";
        case PICO_MPEGTS_CONTENT_ARTS_MAGAZINES:
            return "arts/culture magazines";
        case PICO_MPEGTS_CONTENT_ARTS_FASHION:
            return "fashion";
        case PICO_MPEGTS_CONTENT_ARTS_USER_DEFINED:
            return "arts user defined";

        // Social/Political issues/Economics
        case PICO_MPEGTS_CONTENT_SOCIAL_GENERAL:
            return "social/political issues/economics (general)";
        case PICO_MPEGTS_CONTENT_SOCIAL_MAGAZINES_REPORTS:
            return "magazines/reports/documentary";
        case PICO_MPEGTS_CONTENT_SOCIAL_ECONOMICS_ADVISORY:
            return "economics/social advisory";
        case PICO_MPEGTS_CONTENT_SOCIAL_REMARKABLE_PEOPLE:
            return "remarkable people";
        case PICO_MPEGTS_CONTENT_SOCIAL_USER_DEFINED:
            return "social user defined";

        // Education/Science/Factual topics
        case PICO_MPEGTS_CONTENT_EDUCATION_GENERAL:
            return "education/science/factual topics (general)";
        case PICO_MPEGTS_CONTENT_EDUCATION_NATURE_ANIMALS:
            return "nature/animals/environment";
        case PICO_MPEGTS_CONTENT_EDUCATION_TECHNOLOGY:
            return "technology/natural sciences";
        case PICO_MPEGTS_CONTENT_EDUCATION_MEDICINE:
            return "medicine/physiology/psychology";
        case PICO_MPEGTS_CONTENT_EDUCATION_FOREIGN_COUNTRIES:
            return "foreign countries/expeditions";
        case PICO_MPEGTS_CONTENT_EDUCATION_SOCIAL_SPIRITUAL:
            return "social/spiritual sciences";
        case PICO_MPEGTS_CONTENT_EDUCATION_FURTHER_EDUCATION:
            return "further education";
        case PICO_MPEGTS_CONTENT_EDUCATION_LANGUAGES:
            return "languages";
        case PICO_MPEGTS_CONTENT_EDUCATION_USER_DEFINED:
            return "education user defined";

        // Leisure hobbies
        case PICO_MPEGTS_CONTENT_LEISURE_GENERAL:
            return "leisure hobbies (general)";
        case PICO_MPEGTS_CONTENT_LEISURE_TOURISM_TRAVEL:
            return "tourism/travel";
        case PICO_MPEGTS_CONTENT_LEISURE_HANDICRAFT:
            return "handicraft";
        case PICO_MPEGTS_CONTENT_LEISURE_MOTORING:
            return "motoring";
        case PICO_MPEGTS_CONTENT_LEISURE_FITNESS_HEALTH:
            return "fitness and health";
        case PICO_MPEGTS_CONTENT_LEISURE_COOKING:
            return "cooking";
        case PICO_MPEGTS_CONTENT_LEISURE_ADVERTISEMENT_SHOPPING:
            return "advertisement/shopping";
        case PICO_MPEGTS_CONTENT_LEISURE_GARDENING:
            return "gardening";
        case PICO_MPEGTS_CONTENT_LEISURE_USER_DEFINED:
            return "leisure user defined";

        // Special characteristics
        case PICO_MPEGTS_CONTENT_SPECIAL_ORIGINAL_LANGUAGE:
            return "original language";
        case PICO_MPEGTS_CONTENT_SPECIAL_BLACK_AND_WHITE:
            return "black and white";
        case PICO_MPEGTS_CONTENT_SPECIAL_UNPUBLISHED:
            return "unpublished";
        case PICO_MPEGTS_CONTENT_SPECIAL_LIVE_BROADCAST:
            return "live broadcast";
        case PICO_MPEGTS_CONTENT_SPECIAL_PLANO_STEREOSCOPIC:
            return "plano-stereoscopic";
        case PICO_MPEGTS_CONTENT_SPECIAL_LOCAL_OR_REGIONAL:
            return "local or regional";
        case PICO_MPEGTS_CONTENT_SPECIAL_USER_DEFINED:
            return "special user defined";

        // Adult
        case PICO_MPEGTS_CONTENT_ADULT_GENERAL:
            return "adult (general)";
        case PICO_MPEGTS_CONTENT_ADULT_USER_DEFINED:
            return "adult user defined";

        default:
            return "reserved or unknown content";
    }
}

const char *picoMpegTSStreamTypeToString(uint8_t streamType)
{
    switch (streamType) {
        case PICO_MPEGTS_STREAM_TYPE_RESERVED:
            return "Reserved";
        case PICO_MPEGTS_STREAM_TYPE_MPEG1_VIDEO:
            return "MPEG-1 Video";
        case PICO_MPEGTS_STREAM_TYPE_MPEG2_VIDEO:
            return "MPEG-2 Video";
        case PICO_MPEGTS_STREAM_TYPE_MPEG1_AUDIO:
            return "MPEG-1 Audio";
        case PICO_MPEGTS_STREAM_TYPE_MPEG2_AUDIO:
            return "MPEG-2 Audio";
        case PICO_MPEGTS_STREAM_TYPE_PRIVATE_SECTIONS:
            return "Private Sections";
        case PICO_MPEGTS_STREAM_TYPE_PRIVATE_DATA:
            return "Private Data";
        case PICO_MPEGTS_STREAM_TYPE_MHEG:
            return "MHEG";
        case PICO_MPEGTS_STREAM_TYPE_DSM_CC:
            return "DSM-CC";
        case PICO_MPEGTS_STREAM_TYPE_H222_1:
            return "ITU-T H.222.1";
        case PICO_MPEGTS_STREAM_TYPE_DSMCC_TYPE_A:
            return "DSM-CC Type A";
        case PICO_MPEGTS_STREAM_TYPE_DSMCC_TYPE_B:
            return "DSM-CC Type B";
        case PICO_MPEGTS_STREAM_TYPE_DSMCC_TYPE_C:
            return "DSM-CC Type C";
        case PICO_MPEGTS_STREAM_TYPE_DSMCC_TYPE_D:
            return "DSM-CC Type D";
        case PICO_MPEGTS_STREAM_TYPE_H222_AUXILIARY:
            return "H.222 Auxiliary";
        case PICO_MPEGTS_STREAM_TYPE_AAC_ADTS:
            return "AAC Audio (ADTS)";
        case PICO_MPEGTS_STREAM_TYPE_MPEG4_VISUAL:
            return "MPEG-4 Visual";
        case PICO_MPEGTS_STREAM_TYPE_AAC_LATM:
            return "AAC Audio (LATM)";
        case PICO_MPEGTS_STREAM_TYPE_MPEG4_PES:
            return "MPEG-4 SL/FlexMux (PES)";
        case PICO_MPEGTS_STREAM_TYPE_MPEG4_SECTIONS:
            return "MPEG-4 SL/FlexMux (Sections)";
        case PICO_MPEGTS_STREAM_TYPE_MPEG2_DSMCC_DATA:
            return "MPEG-2 DSM-CC (Synchronized Download)";
        case PICO_MPEGTS_STREAM_TYPE_METADATA_PES:
            return "Metadata (PES)";
        case PICO_MPEGTS_STREAM_TYPE_METADATA_SECTIONS:
            return "Metadata (Sections)";
        case PICO_MPEGTS_STREAM_TYPE_METADATA_DSMCC_DATA:
            return "Metadata (DSM-CC Data Carousel)";
        case PICO_MPEGTS_STREAM_TYPE_METADATA_DSMCC_OBJECT:
            return "Metadata (DSM-CC Object Carousel)";
        case PICO_MPEGTS_STREAM_TYPE_METADATA_DSMCC_DOWNLOAD:
            return "Metadata (DSM-CC Synchronized Download)";
        case PICO_MPEGTS_STREAM_TYPE_MPEG2_IPMP:
            return "MPEG-2 IPMP";
        case PICO_MPEGTS_STREAM_TYPE_H264:
            return "H.264/AVC Video";
        case PICO_MPEGTS_STREAM_TYPE_MPEG4_AUDIO_RAW:
            return "MPEG-4 Audio (Raw)";
        case PICO_MPEGTS_STREAM_TYPE_MPEG4_TEXT:
            return "MPEG-4 Text";
        case PICO_MPEGTS_STREAM_TYPE_MPEG4_AUXILIARY:
            return "MPEG-4 Auxiliary Video";
        case PICO_MPEGTS_STREAM_TYPE_H264_SVC:
            return "H.264/AVC SVC";
        case PICO_MPEGTS_STREAM_TYPE_H264_MVC:
            return "H.264/AVC MVC";
        case PICO_MPEGTS_STREAM_TYPE_JPEG2000:
            return "JPEG 2000 Video";
        case PICO_MPEGTS_STREAM_TYPE_MPEG2_3D_VIDEO:
            return "MPEG-2 3D Video";
        case PICO_MPEGTS_STREAM_TYPE_H264_STEREO_3D:
            return "H.264/AVC Stereo 3D";
        case PICO_MPEGTS_STREAM_TYPE_HEVC:
            return "H.265/HEVC Video";
        case PICO_MPEGTS_STREAM_TYPE_HEVC_SUBSET:
            return "H.265/HEVC Temporal Subset";
        case PICO_MPEGTS_STREAM_TYPE_H264_MVCD:
            return "H.264/AVC MVCD";
        case PICO_MPEGTS_STREAM_TYPE_TIMELINE_METADATA:
            return "Timeline and External Media";
        case PICO_MPEGTS_STREAM_TYPE_HEVC_TILES:
            return "H.265/HEVC Tiles";
        case PICO_MPEGTS_STREAM_TYPE_HEVC_TILES_TEMPORAL:
            return "H.265/HEVC Temporal Tiles";
        case PICO_MPEGTS_STREAM_TYPE_HEVC_TILES_ENHANCEMENT:
            return "H.265/HEVC Enhancement Tiles";
        case PICO_MPEGTS_STREAM_TYPE_HEVC_TILES_MAIN:
            return "H.265/HEVC Main Tiles";
        case PICO_MPEGTS_STREAM_TYPE_GREEN_ACCESS_UNITS:
            return "Green Access Units";
        case PICO_MPEGTS_STREAM_TYPE_QUALITY_ACCESS_UNITS:
            return "Quality Access Units";
        case PICO_MPEGTS_STREAM_TYPE_MEDIA_ORCHESTRATION:
            return "Media Orchestration";
        case PICO_MPEGTS_STREAM_TYPE_SUBSTREAM_FOR_MH3D:
            return "MH 3D Substream";
        case PICO_MPEGTS_STREAM_TYPE_VVC:
            return "H.266/VVC Video";
        case PICO_MPEGTS_STREAM_TYPE_VVC_SUBSET:
            return "H.266/VVC Temporal Subset";
        case PICO_MPEGTS_STREAM_TYPE_EVC:
            return "EVC Video";
        case PICO_MPEGTS_STREAM_TYPE_LCEVC:
            return "LCEVC Video";
        case PICO_MPEGTS_STREAM_TYPE_LCEVC_WITH_HEVC:
            return "LCEVC with HEVC";
        case PICO_MPEGTS_STREAM_TYPE_LCEVC_WITH_VVC:
            return "LCEVC with VVC";
        case PICO_MPEGTS_STREAM_TYPE_CHINESE_VIDEO_STANDARD:
            return "Chinese Video Standard";
        case PICO_MPEGTS_STREAM_TYPE_AC3:
            return "AC-3 Audio";
        case PICO_MPEGTS_STREAM_TYPE_DTS:
            return "DTS Audio";
        case PICO_MPEGTS_STREAM_TYPE_TRUEHD:
            return "TrueHD Audio";
        case PICO_MPEGTS_STREAM_TYPE_EAC3:
            return "E-AC-3 Audio";
        case PICO_MPEGTS_STREAM_TYPE_DTS_HD:
            return "DTS-HD Audio";
        case PICO_MPEGTS_STREAM_TYPE_CAVS_VIDEO:
            return "CAVS Video";
        case PICO_MPEGTS_STREAM_TYPE_SDDS:
            return "SDDS Audio";
        case PICO_MPEGTS_STREAM_TYPE_DIRAC:
            return "Dirac Video";
        case PICO_MPEGTS_STREAM_TYPE_VC1:
            return "VC-1 Video";
        default:
            if (streamType >= PICO_MPEGTS_STREAM_TYPE_USER_PRIVATE_START &&
                streamType <= PICO_MPEGTS_STREAM_TYPE_USER_PRIVATE_END) {
                return "User Private";
            }
            return "Unknown Stream Type";
    }
}

const char *picoMpegTSDescriptorTagToString(uint8_t tag)
{
    switch (tag) {
        case PICO_MPEGTS_DESCRIPTOR_TAG_VIDEO_STREAM:
            return "Video Stream Descriptor";
        case PICO_MPEGTS_DESCRIPTOR_TAG_AUDIO_STREAM:
            return "Audio Stream Descriptor";
        case PICO_MPEGTS_DESCRIPTOR_TAG_HIERARCHY:
            return "Hierarchy Descriptor";
        case PICO_MPEGTS_DESCRIPTOR_TAG_REGISTRATION:
            return "Registration Descriptor";
        case PICO_MPEGTS_DESCRIPTOR_TAG_DATA_STREAM_ALIGNMENT:
            return "Data Stream Alignment Descriptor";
        case PICO_MPEGTS_DESCRIPTOR_TAG_TARGET_BACKGROUND_GRID:
            return "Target Background Grid Descriptor";
        case PICO_MPEGTS_DESCRIPTOR_TAG_VIDEO_WINDOW:
            return "Video Window Descriptor";
        case PICO_MPEGTS_DESCRIPTOR_TAG_CA:
            return "CA Descriptor";
        case PICO_MPEGTS_DESCRIPTOR_TAG_ISO_639_LANGUAGE:
            return "ISO 639 Language Descriptor";
        case PICO_MPEGTS_DESCRIPTOR_TAG_SYSTEM_CLOCK:
            return "System Clock Descriptor";
        case PICO_MPEGTS_DESCRIPTOR_TAG_MULTIPLEX_BUFFER_UTILIZATION:
            return "Multiplex Buffer Utilization Descriptor";
        case PICO_MPEGTS_DESCRIPTOR_TAG_COPYRIGHT:
            return "Copyright Descriptor";
        case PICO_MPEGTS_DESCRIPTOR_TAG_MAXIMUM_BITRATE:
            return "Maximum Bitrate Descriptor";
        case PICO_MPEGTS_DESCRIPTOR_TAG_PRIVATE_DATA_INDICATOR:
            return "Private Data Indicator Descriptor";
        case PICO_MPEGTS_DESCRIPTOR_TAG_SMOOTHING_BUFFER:
            return "Smoothing Buffer Descriptor";
        case PICO_MPEGTS_DESCRIPTOR_TAG_STD:
            return "STD Descriptor";
        case PICO_MPEGTS_DESCRIPTOR_TAG_IBP:
            return "IBP Descriptor";
        case PICO_MPEGTS_DESCRIPTOR_TAG_MPEG4_VIDEO:
            return "MPEG-4 Video Descriptor";
        case PICO_MPEGTS_DESCRIPTOR_TAG_MPEG4_AUDIO:
            return "MPEG-4 Audio Descriptor";
        case PICO_MPEGTS_DESCRIPTOR_TAG_IOD:
            return "IOD Descriptor";
        case PICO_MPEGTS_DESCRIPTOR_TAG_SL:
            return "SL Descriptor";
        case PICO_MPEGTS_DESCRIPTOR_TAG_FMC:
            return "FMC Descriptor";
        case PICO_MPEGTS_DESCRIPTOR_TAG_EXTERNAL_ES_ID:
            return "External ES ID Descriptor";
        case PICO_MPEGTS_DESCRIPTOR_TAG_MUX_CODE:
            return "MuxCode Descriptor";
        case PICO_MPEGTS_DESCRIPTOR_TAG_M4_MUX_BUFFER_SIZE:
            return "M4MuxBufferSize Descriptor";
        case PICO_MPEGTS_DESCRIPTOR_TAG_MULTIPLEX_BUFFER:
            return "MultiplexBuffer Descriptor";
        case PICO_MPEGTS_DESCRIPTOR_TAG_CONTENT_LABELING:
            return "Content Labeling Descriptor";
        case PICO_MPEGTS_DESCRIPTOR_TAG_METADATA_POINTER:
            return "Metadata Pointer Descriptor";
        case PICO_MPEGTS_DESCRIPTOR_TAG_METADATA:
            return "Metadata Descriptor";
        case PICO_MPEGTS_DESCRIPTOR_TAG_METADATA_STD:
            return "Metadata STD Descriptor";
        case PICO_MPEGTS_DESCRIPTOR_TAG_AVC_VIDEO:
            return "AVC Video Descriptor";
        case PICO_MPEGTS_DESCRIPTOR_TAG_IPMP:
            return "IPMP Descriptor";
        case PICO_MPEGTS_DESCRIPTOR_TAG_AVC_TIMING_AND_HRD:
            return "AVC Timing and HRD Descriptor";
        case PICO_MPEGTS_DESCRIPTOR_TAG_MPEG2_AAC_AUDIO:
            return "MPEG-2 AAC Audio Descriptor";
        case PICO_MPEGTS_DESCRIPTOR_TAG_M4_MUX_TIMING:
            return "M4MuxTiming Descriptor";
        case PICO_MPEGTS_DESCRIPTOR_TAG_MPEG4_TEXT:
            return "MPEG-4 Text Descriptor";
        case PICO_MPEGTS_DESCRIPTOR_TAG_MPEG4_AUDIO_EXTENSION:
            return "MPEG-4 Audio Extension Descriptor";
        case PICO_MPEGTS_DESCRIPTOR_TAG_AUXILIARY_VIDEO_STREAM:
            return "Auxiliary Video Stream Descriptor";
        case PICO_MPEGTS_DESCRIPTOR_TAG_SVC_EXTENSION:
            return "SVC Extension Descriptor";
        case PICO_MPEGTS_DESCRIPTOR_TAG_MVC_EXTENSION:
            return "MVC Extension Descriptor";
        case PICO_MPEGTS_DESCRIPTOR_TAG_J2K_VIDEO:
            return "J2K Video Descriptor";
        case PICO_MPEGTS_DESCRIPTOR_TAG_MVC_OPERATION_POINT:
            return "MVC Operation Point Descriptor";
        case PICO_MPEGTS_DESCRIPTOR_TAG_MPEG2_STEREOSCOPIC_VIDEO_FORMAT:
            return "MPEG2 Stereoscopic Video Format Descriptor";
        case PICO_MPEGTS_DESCRIPTOR_TAG_STEREOSCOPIC_PROGRAM_INFO:
            return "Stereoscopic Program Info Descriptor";
        case PICO_MPEGTS_DESCRIPTOR_TAG_STEREOSCOPIC_VIDEO_INFO:
            return "Stereoscopic Video Info Descriptor";
        case PICO_MPEGTS_DESCRIPTOR_TAG_TRANSPORT_PROFILE:
            return "Transport Profile Descriptor";
        case PICO_MPEGTS_DESCRIPTOR_TAG_HEVC_VIDEO:
            return "HEVC Video Descriptor";
        case PICO_MPEGTS_DESCRIPTOR_TAG_VVC_VIDEO:
            return "VVC Video Descriptor";
        case PICO_MPEGTS_DESCRIPTOR_TAG_EVC_VIDEO:
            return "EVC Video Descriptor";
        case PICO_MPEGTS_DESCRIPTOR_TAG_EXTENSION:
            return "Extension Descriptor";
        // DVB Descriptors (ETSI EN 300 468)
        case PICO_MPEGTS_DESCRIPTOR_TAG_NETWORK_NAME:
            return "Network Name Descriptor";
        case PICO_MPEGTS_DESCRIPTOR_TAG_SERVICE_LIST:
            return "Service List Descriptor";
        case PICO_MPEGTS_DESCRIPTOR_TAG_STUFFING:
            return "Stuffing Descriptor";
        case PICO_MPEGTS_DESCRIPTOR_TAG_SATELLITE_DELIVERY_SYSTEM:
            return "Satellite Delivery System Descriptor";
        case PICO_MPEGTS_DESCRIPTOR_TAG_CABLE_DELIVERY_SYSTEM:
            return "Cable Delivery System Descriptor";
        case PICO_MPEGTS_DESCRIPTOR_TAG_VBI_DATA:
            return "VBI Data Descriptor";
        case PICO_MPEGTS_DESCRIPTOR_TAG_VBI_TELETEXT:
            return "VBI Teletext Descriptor";
        case PICO_MPEGTS_DESCRIPTOR_TAG_BOUQUET_NAME:
            return "Bouquet Name Descriptor";
        case PICO_MPEGTS_DESCRIPTOR_TAG_SERVICE:
            return "Service Descriptor";
        case PICO_MPEGTS_DESCRIPTOR_TAG_COUNTRY_AVAILABILITY:
            return "Country Availability Descriptor";
        case PICO_MPEGTS_DESCRIPTOR_TAG_LINKAGE:
            return "Linkage Descriptor";
        case PICO_MPEGTS_DESCRIPTOR_TAG_NVOD_REFERENCE:
            return "NVOD Reference Descriptor";
        case PICO_MPEGTS_DESCRIPTOR_TAG_TIME_SHIFTED_SERVICE:
            return "Time Shifted Service Descriptor";
        case PICO_MPEGTS_DESCRIPTOR_TAG_SHORT_EVENT:
            return "Short Event Descriptor";
        case PICO_MPEGTS_DESCRIPTOR_TAG_EXTENDED_EVENT:
            return "Extended Event Descriptor";
        case PICO_MPEGTS_DESCRIPTOR_TAG_TIME_SHIFTED_EVENT:
            return "Time Shifted Event Descriptor";
        case PICO_MPEGTS_DESCRIPTOR_TAG_COMPONENT:
            return "Component Descriptor";
        case PICO_MPEGTS_DESCRIPTOR_TAG_MOSAIC:
            return "Mosaic Descriptor";
        case PICO_MPEGTS_DESCRIPTOR_TAG_STREAM_IDENTIFIER:
            return "Stream Identifier Descriptor";
        case PICO_MPEGTS_DESCRIPTOR_TAG_CA_IDENTIFIER:
            return "CA Identifier Descriptor";
        case PICO_MPEGTS_DESCRIPTOR_TAG_CONTENT:
            return "Content Descriptor";
        case PICO_MPEGTS_DESCRIPTOR_TAG_PARENTAL_RATING:
            return "Parental Rating Descriptor";
        case PICO_MPEGTS_DESCRIPTOR_TAG_TELETEXT:
            return "Teletext Descriptor";
        case PICO_MPEGTS_DESCRIPTOR_TAG_TELEPHONE:
            return "Telephone Descriptor";
        case PICO_MPEGTS_DESCRIPTOR_TAG_LOCAL_TIME_OFFSET:
            return "Local Time Offset Descriptor";
        case PICO_MPEGTS_DESCRIPTOR_TAG_SUBTITLING:
            return "Subtitling Descriptor";
        case PICO_MPEGTS_DESCRIPTOR_TAG_TERRESTRIAL_DELIVERY_SYSTEM:
            return "Terrestrial Delivery System Descriptor";
        case PICO_MPEGTS_DESCRIPTOR_TAG_MULTILINGUAL_NETWORK_NAME:
            return "Multilingual Network Name Descriptor";
        case PICO_MPEGTS_DESCRIPTOR_TAG_MULTILINGUAL_BOUQUET_NAME:
            return "Multilingual Bouquet Name Descriptor";
        case PICO_MPEGTS_DESCRIPTOR_TAG_MULTILINGUAL_SERVICE_NAME:
            return "Multilingual Service Name Descriptor";
        case PICO_MPEGTS_DESCRIPTOR_TAG_MULTILINGUAL_COMPONENT:
            return "Multilingual Component Descriptor";
        case PICO_MPEGTS_DESCRIPTOR_TAG_PRIVATE_DATA_SPECIFIER:
            return "Private Data Specifier Descriptor";
        case PICO_MPEGTS_DESCRIPTOR_TAG_SERVICE_MOVE:
            return "Service Move Descriptor";
        case PICO_MPEGTS_DESCRIPTOR_TAG_SHORT_SMOOTHING_BUFFER:
            return "Short Smoothing Buffer Descriptor";
        case PICO_MPEGTS_DESCRIPTOR_TAG_FREQUENCY_LIST:
            return "Frequency List Descriptor";
        case PICO_MPEGTS_DESCRIPTOR_TAG_PARTIAL_TRANSPORT_STREAM:
            return "Partial Transport Stream Descriptor";
        case PICO_MPEGTS_DESCRIPTOR_TAG_DATA_BROADCAST:
            return "Data Broadcast Descriptor";
        case PICO_MPEGTS_DESCRIPTOR_TAG_SCRAMBLING:
            return "Scrambling Descriptor";
        case PICO_MPEGTS_DESCRIPTOR_TAG_DATA_BROADCAST_ID:
            return "Data Broadcast Id Descriptor";
        case PICO_MPEGTS_DESCRIPTOR_TAG_TRANSPORT_STREAM:
            return "Transport Stream Descriptor";
        case PICO_MPEGTS_DESCRIPTOR_TAG_DSNG:
            return "DSNG Descriptor";
        case PICO_MPEGTS_DESCRIPTOR_TAG_PDC:
            return "PDC Descriptor";
        case PICO_MPEGTS_DESCRIPTOR_TAG_AC3:
            return "AC-3 Descriptor";
        case PICO_MPEGTS_DESCRIPTOR_TAG_ANCILLARY_DATA:
            return "Ancillary Data Descriptor";
        case PICO_MPEGTS_DESCRIPTOR_TAG_CELL_LIST:
            return "Cell List Descriptor";
        case PICO_MPEGTS_DESCRIPTOR_TAG_CELL_FREQUENCY_LINK:
            return "Cell Frequency Link Descriptor";
        case PICO_MPEGTS_DESCRIPTOR_TAG_ANNOUNCEMENT_SUPPORT:
            return "Announcement Support Descriptor";
        case PICO_MPEGTS_DESCRIPTOR_TAG_APPLICATION_SIGNALLING:
            return "Application Signalling Descriptor";
        case PICO_MPEGTS_DESCRIPTOR_TAG_ADAPTATION_FIELD_DATA:
            return "Adaptation Field Data Descriptor";
        case PICO_MPEGTS_DESCRIPTOR_TAG_SERVICE_IDENTIFIER:
            return "Service Identifier Descriptor";
        case PICO_MPEGTS_DESCRIPTOR_TAG_SERVICE_AVAILABILITY:
            return "Service Availability Descriptor";
        case PICO_MPEGTS_DESCRIPTOR_TAG_DEFAULT_AUTHORITY:
            return "Default Authority Descriptor";
        case PICO_MPEGTS_DESCRIPTOR_TAG_RELATED_CONTENT:
            return "Related Content Descriptor";
        case PICO_MPEGTS_DESCRIPTOR_TAG_TVA_ID:
            return "TVA ID Descriptor";
        case PICO_MPEGTS_DESCRIPTOR_TAG_CONTENT_IDENTIFIER:
            return "Content Identifier Descriptor";
        case PICO_MPEGTS_DESCRIPTOR_TAG_TIME_SLICE_FEC_IDENTIFIER:
            return "Time Slice FEC Identifier Descriptor";
        case PICO_MPEGTS_DESCRIPTOR_TAG_ECM_REPETITION_RATE:
            return "ECM Repetition Rate Descriptor";
        case PICO_MPEGTS_DESCRIPTOR_TAG_S2_SATELLITE_DELIVERY_SYSTEM:
            return "S2 Satellite Delivery System Descriptor";
        case PICO_MPEGTS_DESCRIPTOR_TAG_ENHANCED_AC3:
            return "Enhanced AC-3 Descriptor";
        case PICO_MPEGTS_DESCRIPTOR_TAG_DTS:
            return "DTS Descriptor";
        case PICO_MPEGTS_DESCRIPTOR_TAG_AAC:
            return "AAC Descriptor";
        case PICO_MPEGTS_DESCRIPTOR_TAG_XAIT_LOCATION:
            return "XAIT Location Descriptor";
        case PICO_MPEGTS_DESCRIPTOR_TAG_FTA_CONTENT_MANAGEMENT:
            return "FTA Content Management Descriptor";
        case PICO_MPEGTS_DESCRIPTOR_TAG_DVB_EXTENSION:
            return "DVB Extension Descriptor";
        default:
            if (tag >= PICO_MPEGTS_DESCRIPTOR_TAG_USER_PRIVATE_START && tag <= PICO_MPEGTS_DESCRIPTOR_TAG_USER_PRIVATE_END) {
                return "User Private Descriptor";
            }
            if (tag >= 19 && tag <= 26) {
                return "Defined in ISO/IEC 13818-6";
            }
            return "Reserved/Unknown Descriptor";
    }
}

const char *picoMpegTSComponentStreamContentToString(uint8_t streamContent)
{
    switch (streamContent) {
        case PICO_MPEGTS_COMPONENT_STREAM_CONTENT_RESERVED:
            return "Reserved";
        case PICO_MPEGTS_COMPONENT_STREAM_CONTENT_MPEG2_VIDEO:
            return "MPEG-2 Video";
        case PICO_MPEGTS_COMPONENT_STREAM_CONTENT_MPEG1_AUDIO:
            return "MPEG-1 Layer 2 Audio";
        case PICO_MPEGTS_COMPONENT_STREAM_CONTENT_SUBTITLES:
            return "Subtitles";
        case PICO_MPEGTS_COMPONENT_STREAM_CONTENT_AC3_AUDIO:
            return "AC-3 Audio";
        case PICO_MPEGTS_COMPONENT_STREAM_CONTENT_AVC_VIDEO:
            return "H.264/AVC Video";
        case PICO_MPEGTS_COMPONENT_STREAM_CONTENT_HE_AAC_AUDIO:
            return "HE AAC Audio";
        case PICO_MPEGTS_COMPONENT_STREAM_CONTENT_DTS_AUDIO:
            return "DTS Audio";
        case PICO_MPEGTS_COMPONENT_STREAM_CONTENT_DVB_SRM:
            return "DVB SRM Data";
        case PICO_MPEGTS_COMPONENT_STREAM_CONTENT_HEVC_VIDEO:
            return "HEVC Video";
        case PICO_MPEGTS_COMPONENT_STREAM_CONTENT_EXTENSION:
            return "Extension";
        default:
            return "Reserved";
    }
}

const char *picoMpegTSComponentTypeToString(uint16_t componentType)
{
    switch (componentType) {
        // Reserved
        case PICO_MPEGTS_COMPONENT_TYPE_RESERVED:
            return "Reserved";

        // MPEG-2 video
        case PICO_MPEGTS_COMPONENT_TYPE_MPEG2_VIDEO_RESERVED:
            return "MPEG-2 Video, reserved";
        case PICO_MPEGTS_COMPONENT_TYPE_MPEG2_VIDEO_4_3_ASPECT_25HZ:
            return "MPEG-2 Video, 4:3 aspect ratio, 25 Hz";
        case PICO_MPEGTS_COMPONENT_TYPE_MPEG2_VIDEO_16_9_PAN_25HZ:
            return "MPEG-2 Video, 16:9 pan vectors, 25 Hz";
        case PICO_MPEGTS_COMPONENT_TYPE_MPEG2_VIDEO_16_9_NO_PAN_25HZ:
            return "MPEG-2 Video, 16:9 no pan, 25 Hz";
        case PICO_MPEGTS_COMPONENT_TYPE_MPEG2_VIDEO_GT_16_9_25HZ:
            return "MPEG-2 Video, >16:9 aspect ratio, 25 Hz";
        case PICO_MPEGTS_COMPONENT_TYPE_MPEG2_VIDEO_4_3_ASPECT_30HZ:
            return "MPEG-2 Video, 4:3 aspect ratio, 30 Hz";
        case PICO_MPEGTS_COMPONENT_TYPE_MPEG2_VIDEO_16_9_PAN_30HZ:
            return "MPEG-2 Video, 16:9 pan vectors, 30 Hz";
        case PICO_MPEGTS_COMPONENT_TYPE_MPEG2_VIDEO_16_9_NO_PAN_30HZ:
            return "MPEG-2 Video, 16:9 no pan, 30 Hz";
        case PICO_MPEGTS_COMPONENT_TYPE_MPEG2_VIDEO_GT_16_9_30HZ:
            return "MPEG-2 Video, >16:9 aspect ratio, 30 Hz";
        case PICO_MPEGTS_COMPONENT_TYPE_MPEG2_VIDEO_HD_4_3_ASPECT_25HZ:
            return "MPEG-2 HD Video, 4:3 aspect ratio, 25 Hz";
        case PICO_MPEGTS_COMPONENT_TYPE_MPEG2_VIDEO_HD_16_9_PAN_25HZ:
            return "MPEG-2 HD Video, 16:9 pan vectors, 25 Hz";
        case PICO_MPEGTS_COMPONENT_TYPE_MPEG2_VIDEO_HD_16_9_NO_PAN_25HZ:
            return "MPEG-2 HD Video, 16:9 no pan, 25 Hz";
        case PICO_MPEGTS_COMPONENT_TYPE_MPEG2_VIDEO_HD_GT_16_9_25HZ:
            return "MPEG-2 HD Video, >16:9 aspect ratio, 25 Hz";
        case PICO_MPEGTS_COMPONENT_TYPE_MPEG2_VIDEO_HD_4_3_ASPECT_30HZ:
            return "MPEG-2 HD Video, 4:3 aspect ratio, 30 Hz";
        case PICO_MPEGTS_COMPONENT_TYPE_MPEG2_VIDEO_HD_16_9_PAN_30HZ:
            return "MPEG-2 HD Video, 16:9 pan vectors, 30 Hz";
        case PICO_MPEGTS_COMPONENT_TYPE_MPEG2_VIDEO_HD_16_9_NO_PAN_30HZ:
            return "MPEG-2 HD Video, 16:9 no pan, 30 Hz";
        case PICO_MPEGTS_COMPONENT_TYPE_MPEG2_VIDEO_HD_GT_16_9_30HZ:
            return "MPEG-2 HD Video, >16:9 aspect ratio, 30 Hz";

        // MPEG-1 Layer 2 audio
        case PICO_MPEGTS_COMPONENT_TYPE_MPEG1_AUDIO_RESERVED:
            return "MPEG-1 Layer 2 Audio, reserved";
        case PICO_MPEGTS_COMPONENT_TYPE_MPEG1_AUDIO_SINGLE_MONO:
            return "MPEG-1 Layer 2 Audio, single mono";
        case PICO_MPEGTS_COMPONENT_TYPE_MPEG1_AUDIO_DUAL_MONO:
            return "MPEG-1 Layer 2 Audio, dual mono";
        case PICO_MPEGTS_COMPONENT_TYPE_MPEG1_AUDIO_STEREO:
            return "MPEG-1 Layer 2 Audio, stereo";
        case PICO_MPEGTS_COMPONENT_TYPE_MPEG1_AUDIO_MULTI_LINGUAL_MULTI_CHAN:
            return "MPEG-1 Layer 2 Audio, multilingual/multichannel";
        case PICO_MPEGTS_COMPONENT_TYPE_MPEG1_AUDIO_SURROUND_SOUND:
            return "MPEG-1 Layer 2 Audio, surround sound";
        case PICO_MPEGTS_COMPONENT_TYPE_MPEG1_AUDIO_VISUALLY_IMPAIRED:
            return "MPEG-1 Layer 2 Audio, for visually impaired";
        case PICO_MPEGTS_COMPONENT_TYPE_MPEG1_AUDIO_HARD_OF_HEARING:
            return "MPEG-1 Layer 2 Audio, for hard of hearing";
        case PICO_MPEGTS_COMPONENT_TYPE_MPEG1_AUDIO_SUPPLEMENTARY:
            return "MPEG-1 Layer 2 Audio, supplementary";
        case PICO_MPEGTS_COMPONENT_TYPE_MPEG1_AUDIO_RECEIVER_MIX:
            return "MPEG-1 Layer 2 Audio, receiver mix";

        // Subtitles
        case PICO_MPEGTS_COMPONENT_TYPE_SUBTITLES_RESERVED:
            return "Subtitles, reserved";
        case PICO_MPEGTS_COMPONENT_TYPE_SUBTITLES_EBU_TELETEXT:
            return "EBU Teletext subtitles";
        case PICO_MPEGTS_COMPONENT_TYPE_SUBTITLES_EBU_TELETEXT_ASSOC:
            return "EBU Teletext subtitles, associated";
        case PICO_MPEGTS_COMPONENT_TYPE_SUBTITLES_EBU_TELETEXT_VBI:
            return "EBU Teletext subtitles, VBI";
        case PICO_MPEGTS_COMPONENT_TYPE_SUBTITLES_DVB_NORMAL_NO_AR:
            return "DVB subtitles, normal, no aspect ratio";
        case PICO_MPEGTS_COMPONENT_TYPE_SUBTITLES_DVB_NORMAL_4_3:
            return "DVB subtitles, normal, 4:3";
        case PICO_MPEGTS_COMPONENT_TYPE_SUBTITLES_DVB_NORMAL_16_9:
            return "DVB subtitles, normal, 16:9";
        case PICO_MPEGTS_COMPONENT_TYPE_SUBTITLES_DVB_NORMAL_2_21_1:
            return "DVB subtitles, normal, 2.21:1";
        case PICO_MPEGTS_COMPONENT_TYPE_SUBTITLES_DVB_NORMAL_HD:
            return "DVB subtitles, normal, HD";
        case PICO_MPEGTS_COMPONENT_TYPE_SUBTITLES_DVB_HARD_OF_HEARING_NO_AR:
            return "DVB subtitles, for hard of hearing, no aspect ratio";
        case PICO_MPEGTS_COMPONENT_TYPE_SUBTITLES_DVB_HARD_OF_HEARING_4_3:
            return "DVB subtitles, for hard of hearing, 4:3";
        case PICO_MPEGTS_COMPONENT_TYPE_SUBTITLES_DVB_HARD_OF_HEARING_16_9:
            return "DVB subtitles, for hard of hearing, 16:9";
        case PICO_MPEGTS_COMPONENT_TYPE_SUBTITLES_DVB_HARD_OF_HEARING_2_21_1:
            return "DVB subtitles, for hard of hearing, 2.21:1";
        case PICO_MPEGTS_COMPONENT_TYPE_SUBTITLES_DVB_HARD_OF_HEARING_HD:
            return "DVB subtitles, for hard of hearing, HD";

        // AC-3 audio
        case PICO_MPEGTS_COMPONENT_TYPE_AC3_RESERVED:
            return "AC-3 Audio, reserved";

        // H.264/AVC video
        case PICO_MPEGTS_COMPONENT_TYPE_AVC_VIDEO_RESERVED:
            return "H.264/AVC Video, reserved";
        case PICO_MPEGTS_COMPONENT_TYPE_AVC_VIDEO_SD_4_3_25HZ:
            return "H.264/AVC SD Video, 4:3 aspect ratio, 25 Hz";
        case PICO_MPEGTS_COMPONENT_TYPE_AVC_VIDEO_SD_16_9_PAN_25HZ:
            return "H.264/AVC SD Video, 16:9 pan vectors, 25 Hz";
        case PICO_MPEGTS_COMPONENT_TYPE_AVC_VIDEO_SD_16_9_NO_PAN_25HZ:
            return "H.264/AVC SD Video, 16:9 no pan, 25 Hz";
        case PICO_MPEGTS_COMPONENT_TYPE_AVC_VIDEO_SD_GT_16_9_25HZ:
            return "H.264/AVC SD Video, >16:9 aspect ratio, 25 Hz";
        case PICO_MPEGTS_COMPONENT_TYPE_AVC_VIDEO_SD_4_3_30HZ:
            return "H.264/AVC SD Video, 4:3 aspect ratio, 30 Hz";
        case PICO_MPEGTS_COMPONENT_TYPE_AVC_VIDEO_SD_16_9_PAN_30HZ:
            return "H.264/AVC SD Video, 16:9 pan vectors, 30 Hz";
        case PICO_MPEGTS_COMPONENT_TYPE_AVC_VIDEO_SD_16_9_NO_PAN_30HZ:
            return "H.264/AVC SD Video, 16:9 no pan, 30 Hz";
        case PICO_MPEGTS_COMPONENT_TYPE_AVC_VIDEO_SD_GT_16_9_30HZ:
            return "H.264/AVC SD Video, >16:9 aspect ratio, 30 Hz";
        case PICO_MPEGTS_COMPONENT_TYPE_AVC_VIDEO_HD_4_3_25HZ:
            return "H.264/AVC HD Video, 4:3 aspect ratio, 25 Hz";
        case PICO_MPEGTS_COMPONENT_TYPE_AVC_VIDEO_HD_16_9_PAN_25HZ:
            return "H.264/AVC HD Video, 16:9 pan vectors, 25 Hz";
        case PICO_MPEGTS_COMPONENT_TYPE_AVC_VIDEO_HD_16_9_NO_PAN_25HZ:
            return "H.264/AVC HD Video, 16:9 no pan, 25 Hz";
        case PICO_MPEGTS_COMPONENT_TYPE_AVC_VIDEO_HD_GT_16_9_25HZ:
            return "H.264/AVC HD Video, >16:9 aspect ratio, 25 Hz";
        case PICO_MPEGTS_COMPONENT_TYPE_AVC_VIDEO_HD_4_3_30HZ:
            return "H.264/AVC HD Video, 4:3 aspect ratio, 30 Hz";
        case PICO_MPEGTS_COMPONENT_TYPE_AVC_VIDEO_HD_16_9_PAN_30HZ:
            return "H.264/AVC HD Video, 16:9 pan vectors, 30 Hz";
        case PICO_MPEGTS_COMPONENT_TYPE_AVC_VIDEO_HD_16_9_NO_PAN_30HZ:
            return "H.264/AVC HD Video, 16:9 no pan, 30 Hz";
        case PICO_MPEGTS_COMPONENT_TYPE_AVC_VIDEO_HD_GT_16_9_30HZ:
            return "H.264/AVC HD Video, >16:9 aspect ratio, 30 Hz";

        // HE AAC audio
        case PICO_MPEGTS_COMPONENT_TYPE_HE_AAC_RESERVED:
            return "HE AAC Audio, reserved";
        case PICO_MPEGTS_COMPONENT_TYPE_HE_AAC_SINGLE_MONO:
            return "HE AAC Audio, single mono";
        case PICO_MPEGTS_COMPONENT_TYPE_HE_AAC_DUAL_MONO:
            return "HE AAC Audio, dual mono";
        case PICO_MPEGTS_COMPONENT_TYPE_HE_AAC_STEREO:
            return "HE AAC Audio, stereo";
        case PICO_MPEGTS_COMPONENT_TYPE_HE_AAC_MULTI_LINGUAL_MULTI_CHAN:
            return "HE AAC Audio, multilingual/multichannel";
        case PICO_MPEGTS_COMPONENT_TYPE_HE_AAC_SURROUND_SOUND:
            return "HE AAC Audio, surround sound";
        case PICO_MPEGTS_COMPONENT_TYPE_HE_AAC_VISUALLY_IMPAIRED:
            return "HE AAC Audio, for visually impaired";
        case PICO_MPEGTS_COMPONENT_TYPE_HE_AAC_HARD_OF_HEARING:
            return "HE AAC Audio, for hard of hearing";
        case PICO_MPEGTS_COMPONENT_TYPE_HE_AAC_SUPPLEMENTARY:
            return "HE AAC Audio, supplementary";
        case PICO_MPEGTS_COMPONENT_TYPE_HE_AAC_HEV2_SINGLE_MONO:
            return "HE AAC v2 Audio, single mono";
        case PICO_MPEGTS_COMPONENT_TYPE_HEV2_AAC_DUAL_MONO:
            return "HE AAC v2 Audio, dual mono";
        case PICO_MPEGTS_COMPONENT_TYPE_HEV2_AAC_STEREO:
            return "HE AAC v2 Audio, stereo";
        case PICO_MPEGTS_COMPONENT_TYPE_HE_AAC_RECEIVER_MIX:
            return "HE AAC Audio, receiver mix";

        // DTS audio
        case PICO_MPEGTS_COMPONENT_TYPE_DTS_RESERVED:
            return "DTS Audio, reserved";

        // DVB SRM
        case PICO_MPEGTS_COMPONENT_TYPE_SRM_RESERVED:
            return "DVB SRM Data, reserved";

        // HEVC video
        case PICO_MPEGTS_COMPONENT_TYPE_HEVC_VIDEO_RESERVED:
            return "HEVC Video, reserved";
        case PICO_MPEGTS_COMPONENT_TYPE_HEVC_VIDEO_MAIN_HD_50HZ:
            return "HEVC Main Profile HD, 50 Hz";
        case PICO_MPEGTS_COMPONENT_TYPE_HEVC_VIDEO_MAIN_HD_60HZ:
            return "HEVC Main Profile HD, 60 Hz";
        case PICO_MPEGTS_COMPONENT_TYPE_HEVC_VIDEO_MAIN_10_HD_50HZ:
            return "HEVC Main 10 Profile HD, 50 Hz";
        case PICO_MPEGTS_COMPONENT_TYPE_HEVC_VIDEO_MAIN_10_HD_60HZ:
            return "HEVC Main 10 Profile HD, 60 Hz";
        case PICO_MPEGTS_COMPONENT_TYPE_HEVC_VIDEO_UHD_50HZ_SDR:
            return "HEVC UHD Video, 50 Hz SDR";
        case PICO_MPEGTS_COMPONENT_TYPE_HEVC_VIDEO_UHD_60_120HZ_SDR:
            return "HEVC UHD Video, 60/120 Hz SDR";
        case PICO_MPEGTS_COMPONENT_TYPE_HEVC_VIDEO_UHD_50HZ_HDR:
            return "HEVC UHD Video, 50 Hz HDR";
        case PICO_MPEGTS_COMPONENT_TYPE_HEVC_VIDEO_UHD_60_120HZ_HDR:
            return "HEVC UHD Video, 60/120 Hz HDR";

        // AC-4 extension (stream_content_ext = 0x0)
        case PICO_MPEGTS_COMPONENT_TYPE_AC4_MAIN_MONO:
            return "AC-4 Main audio, mono";
        case PICO_MPEGTS_COMPONENT_TYPE_AC4_MAIN_STEREO:
            return "AC-4 Main audio, stereo";
        case PICO_MPEGTS_COMPONENT_TYPE_AC4_MAIN_STEREO_DIALOGUE_ENHANCEMENT:
            return "AC-4 Main audio, stereo, dialogue enhancement";
        case PICO_MPEGTS_COMPONENT_TYPE_AC4_MAIN_STEREO_VISUALLY_IMPAIRED:
            return "AC-4 Main audio, stereo, for visually impaired";
        case PICO_MPEGTS_COMPONENT_TYPE_AC4_MAIN_STEREO_HARD_OF_HEARING:
            return "AC-4 Main audio, stereo, for hard of hearing";
        case PICO_MPEGTS_COMPONENT_TYPE_AC4_MAIN_MULTI_CHANNEL:
            return "AC-4 Main audio, multichannel";
        case PICO_MPEGTS_COMPONENT_TYPE_AC4_BCAST_MIX_MONO:
            return "AC-4 Broadcast-mix audio, mono";
        case PICO_MPEGTS_COMPONENT_TYPE_AC4_BCAST_MIX_STEREO:
            return "AC-4 Broadcast-mix audio, stereo";
        case PICO_MPEGTS_COMPONENT_TYPE_AC4_BCAST_MIX_STEREO_DIALOGUE:
            return "AC-4 Broadcast-mix audio, stereo, dialogue enhancement";
        case PICO_MPEGTS_COMPONENT_TYPE_AC4_BCAST_MIX_STEREO_VI:
            return "AC-4 Broadcast-mix audio, stereo, for visually impaired";
        case PICO_MPEGTS_COMPONENT_TYPE_AC4_BCAST_MIX_STEREO_HOH:
            return "AC-4 Broadcast-mix audio, stereo, for hard of hearing";
        case PICO_MPEGTS_COMPONENT_TYPE_AC4_BCAST_MIX_MULTI_CHANNEL:
            return "AC-4 Broadcast-mix audio, multichannel";
        case PICO_MPEGTS_COMPONENT_TYPE_AC4_RECEIVER_MIX_MONO:
            return "AC-4 Receiver-mix audio, mono";
        case PICO_MPEGTS_COMPONENT_TYPE_AC4_RECEIVER_MIX_STEREO:
            return "AC-4 Receiver-mix audio, stereo";

        // NGA component type extension (stream_content_ext = 0xE)
        case PICO_MPEGTS_COMPONENT_TYPE_NGA_CONTENT_LESS_16_9:
            return "NGA Video, less than 16:9 aspect ratio";
        case PICO_MPEGTS_COMPONENT_TYPE_NGA_CONTENT_16_9:
            return "NGA Video, 16:9 aspect ratio";
        case PICO_MPEGTS_COMPONENT_TYPE_NGA_CONTENT_GT_16_9:
            return "NGA Video, greater than 16:9 aspect ratio";
        case PICO_MPEGTS_COMPONENT_TYPE_NGA_STEREOSCOPIC_TAB:
            return "NGA Video, stereoscopic top-and-bottom";
        case PICO_MPEGTS_COMPONENT_TYPE_NGA_HLG10_HDR:
            return "NGA Video, HLG10 HDR";
        case PICO_MPEGTS_COMPONENT_TYPE_NGA_HEVC_TEMPORAL_VIDEO:
            return "NGA HEVC temporal video subset";

        default:
            return "Unknown Component Type";
    }
}

void picoMpegTsPsiSectionHeadDebugPrint(picoMpegTSPSISectionHead sectionHead)
{
    PICO_ASSERT(sectionHead != NULL);
    PICO_MPEGTS_LOG("PSI Section Head:\n");
    PICO_MPEGTS_LOG("  Table ID: 0x%02X (%s)\n", sectionHead->tableId, picoMpegTSTableIDToString(sectionHead->tableId));
    PICO_MPEGTS_LOG("  Section Length: %u\n", sectionHead->sectionLength);
    PICO_MPEGTS_LOG("  ID: %u\n", sectionHead->id);
    PICO_MPEGTS_LOG("  Version Number: %u\n", sectionHead->versionNumber);
    PICO_MPEGTS_LOG("  Current Next Indicator: %s\n", sectionHead->currentNextIndicator ? "true" : "false");
    PICO_MPEGTS_LOG("  Section Number: %u\n", sectionHead->sectionNumber);
    PICO_MPEGTS_LOG("  Last Section Number: %u\n", sectionHead->lastSectionNumber);
}

void picoMpegTSPacketAdaptationFieldExtensionDebugPrint(picoMpegTSAdaptionFieldExtension afExt)
{
    PICO_ASSERT(afExt != NULL);
    PICO_MPEGTS_LOG("Adaptation Field Extension:\n");
    PICO_MPEGTS_LOG("  LTW Flag: %s\n", afExt->ltwFlag ? "true" : "false");
    if (afExt->ltwFlag) {
        PICO_MPEGTS_LOG("    LTW Valid Flag: %s\n", afExt->ltwValidFlag ? "true" : "false");
        if (afExt->ltwValidFlag) {
            PICO_MPEGTS_LOG("    LTW Offset: %u\n", afExt->ltwOffset);
        }
    }
    PICO_MPEGTS_LOG("  Piecewise Rate Flag: %s\n", afExt->piecewiseRateFlag ? "true" : "false");
    if (afExt->piecewiseRateFlag) {
        PICO_MPEGTS_LOG("    Piecewise Rate: %u\n", afExt->piecewiseRate);
    }
    PICO_MPEGTS_LOG("  Seamless Splice Flag: %s\n", afExt->seamlessSpliceFlag ? "true" : "false");
    if (afExt->seamlessSpliceFlag) {
        PICO_MPEGTS_LOG("    Splice Type: %u\n", afExt->spliceType);
        PICO_MPEGTS_LOG("    DTS Next AU: %llx\n", afExt->dtsNextAU);
    }
    PICO_MPEGTS_LOG("  AF Descriptor Not Present Flag: %s\n", afExt->afDescriptorNotPresentFlag ? "true" : "false");
}

void picoMpegTSPacketAdaptationFieldDebugPrint(picoMpegTSPacketAdaptationField af)
{
    PICO_ASSERT(af != NULL);
    PICO_MPEGTS_LOG("Adaptation Field:\n");
    PICO_MPEGTS_LOG("  Discontinuity Indicator: %s\n", af->discontinuityIndicator ? "true" : "false");
    PICO_MPEGTS_LOG("  Random Access Indicator: %s\n", af->randomAccessIndicator ? "true" : "false");
    PICO_MPEGTS_LOG("  Elementary Stream Priority Indicator: %s\n", af->elementaryStreamPriorityIndicator ? "true" : "false");
    PICO_MPEGTS_LOG("  PCR Flag: %s\n", af->pcrFlag ? "true" : "false");
    if (af->pcrFlag) {
        PICO_MPEGTS_LOG("    PCR Base: %llu\n", af->pcr.base);
        PICO_MPEGTS_LOG("    PCR Extension: %u\n", af->pcr.extension);
    }
    PICO_MPEGTS_LOG("  OPCR Flag: %s\n", af->opcrFlag ? "true" : "false");
    if (af->opcrFlag) {
        PICO_MPEGTS_LOG("    OPCR Base: %llu\n", af->opcr.base);
        PICO_MPEGTS_LOG("    OPCR Extension: %u\n", af->opcr.extension);
    }
    PICO_MPEGTS_LOG("  Splicing Point Flag: %s\n", af->splicingPointFlag ? "true" : "false");
    if (af->splicingPointFlag) {
        PICO_MPEGTS_LOG("    Splice Countdown: %u\n", af->spliceCountdown);
    }
    PICO_MPEGTS_LOG("  Transport Private Data Flag: %s\n", af->transportPrivateDataFlag ? "true" : "false");
    if (af->transportPrivateDataFlag) {
        PICO_MPEGTS_LOG("    Transport Private Data Length: %u\n", af->transportPrivateDataLength);
    }
    PICO_MPEGTS_LOG("  Adaptation Field Extension Flag: %s\n", af->adaptationFieldExtensionFlag ? "true" : "false");
}

void picoMpegTSPacketDebugPrint(picoMpegTSPacket packet)
{
    PICO_ASSERT(packet != NULL);
    PICO_MPEGTS_LOG("MPEG-TS Packet:\n");
    PICO_MPEGTS_LOG("  PID: %s [0x%04X]\n", picoMpegTSPIDToString(packet->pid), packet->pid);
    PICO_MPEGTS_LOG("  Error Indicator: %s\n", packet->errorIndicator ? "true" : "false");
    PICO_MPEGTS_LOG("  Payload Unit Start Indicator: %s\n", packet->payloadUnitStartIndicator ? "true" : "false");
    PICO_MPEGTS_LOG("  Transport Priority: %s\n", packet->transportPriority ? "true" : "false");
    PICO_MPEGTS_LOG("  Scrambling Control: %u\n", packet->scramblingControl);
    PICO_MPEGTS_LOG("  Continuity Counter: %u\n", packet->continuityCounter);
    PICO_MPEGTS_LOG("  Adaptation Field Control: %s\n", picoMpegTSAdaptionFieldControlToString(packet->adaptionFieldControl));
    PICO_MPEGTS_LOG("  Payload Size: %u bytes\n", packet->payloadSize);
    if (packet->adaptionFieldControl == PICO_MPEGTS_ADAPTATION_FIELD_CONTROL_ADAPTATION_ONLY ||
        packet->adaptionFieldControl == PICO_MPEGTS_ADAPTATION_FIELD_CONTROL_BOTH) {
        picoMpegTSPacketAdaptationFieldDebugPrint(&packet->adaptionField);
    }
}

static void __picoMpegTSDescriptorPayloadISO639LanguageDebugPrint(picoMpegTSDescriptor descriptor, int indent)
{
    (void)indent;
    if (descriptor == NULL) {
        return;
    }
    PICO_MPEGTS_LOG("%*sISO 639 Language Descriptor:\n", indent, "");
    for (size_t i = 0; i < descriptor->parsed.iso639Language.entryCount; i++) {
        PICO_MPEGTS_LOG("%*sEntry [%zu]:\n", indent + 2, "", i);
        PICO_MPEGTS_LOG("%*sLanguage\t: %s\n", indent + 4, "", descriptor->parsed.iso639Language.entries[i].languageCode);
        PICO_MPEGTS_LOG("%*sAudio Type\t: %s [0x%02X]\n", indent + 4, "",
                        picoMpegTSAudioTypeToString(descriptor->parsed.iso639Language.entries[i].audioType),
                        descriptor->parsed.iso639Language.entries[i].audioType);
    }
}

static void __picoMpegTSDescriptorPayloadServiceDebugPrint(picoMpegTSDescriptor descriptor, int indent)
{
    (void)indent;
    if (descriptor == NULL) {
        return;
    }
    PICO_MPEGTS_LOG("%*sService Descriptor:\n", indent, "");
    PICO_MPEGTS_LOG("%*sService Type\t: %s [0x%02X]\n", indent + 2, "",
                    picoMpegTSServiceTypeToString(descriptor->parsed.service.serviceType),
                    descriptor->parsed.service.serviceType);
    PICO_MPEGTS_LOG("%*sProvider Name\t: %s\n", indent + 2, "", descriptor->parsed.service.serviceProviderName);
    PICO_MPEGTS_LOG("%*sService Name\t: %s\n", indent + 2, "", descriptor->parsed.service.serviceName);
}

static void __picoMpegTSDescriptorPayloadStreamIdentifierDebugPrint(picoMpegTSDescriptor descriptor, int indent)
{
    (void)indent;
    if (descriptor == NULL) {
        return;
    }
    PICO_MPEGTS_LOG("%*sStream Identifier Descriptor:\n", indent, "");
    PICO_MPEGTS_LOG("%*sComponent Tag\t: 0x%02X\n", indent + 2, "", descriptor->parsed.streamIdentifier.componentTag);
}

static void __picoMpegTSDescriptorPayloadCADebugPrint(picoMpegTSDescriptor descriptor, int indent)
{
    (void)indent;
    if (descriptor == NULL) {
        return;
    }
    PICO_MPEGTS_LOG("%*sCA Descriptor:\n", indent, "");
    PICO_MPEGTS_LOG("%*sCA System ID\t: 0x%04X\n", indent + 2, "", descriptor->parsed.ca.caSystemId);
    PICO_MPEGTS_LOG("%*sCA PID\t: 0x%04X\n", indent + 2, "", descriptor->parsed.ca.caPid);
    PICO_MPEGTS_LOG("%*sPrivate Data Length : %zu\n", indent + 2, "", descriptor->parsed.ca.privateDataLength);
    if (descriptor->parsed.ca.privateDataLength > 0) {
        PICO_MPEGTS_LOG("%*sPrivate Data\t: ", indent + 2, "");
        for (size_t i = 0; i < descriptor->parsed.ca.privateDataLength; i++) {
            PICO_MPEGTS_LOG("%02X ", descriptor->parsed.ca.privateData[i]);
        }
        PICO_MPEGTS_LOG("\n");
    }
}

static void __picoMpegTSDescriptorPayloadContentDebugPrint(picoMpegTSDescriptor descriptor, int indent)
{
    (void)indent;
    if (descriptor == NULL) {
        return;
    }
    PICO_MPEGTS_LOG("%*sContent Descriptor:\n", indent, "");
    for (size_t i = 0; i < descriptor->parsed.content.entryCount; i++) {
        PICO_MPEGTS_LOG("%*sEntry [%zu]:\n", indent + 2, "", i);
        PICO_MPEGTS_LOG("%*sLevel 1\t: %s [0x%X]\n", indent + 4, "",
                        picoMpegTSContentNibbleLevel1ToString(descriptor->parsed.content.entries[i].nibbleLevel1),
                        descriptor->parsed.content.entries[i].nibbleLevel1);
        PICO_MPEGTS_LOG("%*sLevel 2\t: %s [0x%X]\n", indent + 4, "",
                        picoMpegTSContentNibbleToString((picoMpegTSContentNibble)((descriptor->parsed.content.entries[i].nibbleLevel1 << 4) | descriptor->parsed.content.entries[i].nibbleLevel2)),
                        descriptor->parsed.content.entries[i].nibbleLevel2);
        PICO_MPEGTS_LOG("%*sUser Byte\t: 0x%02X\n", indent + 4, "", descriptor->parsed.content.entries[i].userByte);
    }
}

static void __picoMpegTSDescriptorPayloadComponentDebugPrint(picoMpegTSDescriptor descriptor, int indent)
{
    (void)indent;
    if (descriptor == NULL) {
        return;
    }
    PICO_MPEGTS_LOG("%*sComponent Descriptor:\n", indent, "");
    PICO_MPEGTS_LOG("%*sStream Content\t: %s [0x%X]\n", indent + 2, "",
                    picoMpegTSComponentStreamContentToString(descriptor->parsed.component.streamContent),
                    descriptor->parsed.component.streamContent);
    if (descriptor->parsed.component.streamContent == 0xF) {
        PICO_MPEGTS_LOG("%*sStream Content Ext\t: 0x%X\n", indent + 2, "", descriptor->parsed.component.streamContentExt);
    }

    uint16_t combinedType = (descriptor->parsed.component.streamContent == 0xF)
                                ? (uint16_t)(0xF000 | (descriptor->parsed.component.streamContentExt << 8) | descriptor->parsed.component.componentType)
                                : (uint16_t)((descriptor->parsed.component.streamContent << 8) | descriptor->parsed.component.componentType);
    (void)combinedType; // Suppress unused variable warning when logging is disabled

    PICO_MPEGTS_LOG("%*sComponent Type\t: %s [0x%02X]\n", indent + 2, "",
                    picoMpegTSComponentTypeToString(combinedType),
                    descriptor->parsed.component.componentType);
    PICO_MPEGTS_LOG("%*sComponent Tag\t: 0x%02X\n", indent + 2, "", descriptor->parsed.component.componentTag);
    PICO_MPEGTS_LOG("%*sLanguage Code\t: %s\n", indent + 2, "", descriptor->parsed.component.languageCode);
    if (descriptor->parsed.component.textLength > 0) {
        PICO_MPEGTS_LOG("%*sText\t: %s\n", indent + 2, "", descriptor->parsed.component.text);
    }
}

static void __picoMpegTSDescriptorPayloadShortEventDebugPrint(picoMpegTSDescriptor descriptor, int indent)
{
    (void)indent;
    if (descriptor == NULL) {
        return;
    }
    PICO_MPEGTS_LOG("%*sShort Event Descriptor:\n", indent, "");
    PICO_MPEGTS_LOG("%*sLanguage Code\t: %.3s\n", indent + 2, "", descriptor->parsed.shortEvent.languageCode);
    if (descriptor->parsed.shortEvent.eventNameLength > 0) {
        PICO_MPEGTS_LOG("%*sEvent Name\t: %s\n", indent + 2, "", descriptor->parsed.shortEvent.eventName);
    }
    if (descriptor->parsed.shortEvent.textLength > 0) {
        PICO_MPEGTS_LOG("%*sEvent Text\t: %s\n", indent + 2, "", descriptor->parsed.shortEvent.text);
    }
}

static void __picoMpegTSDescriptorPayloadServiceListDebugPrint(picoMpegTSDescriptor descriptor, int indent)
{
    (void)indent;
    if (descriptor == NULL) {
        return;
    }
    PICO_MPEGTS_LOG("%*sService List Descriptor:\n", indent, "");
    for (size_t i = 0; i < descriptor->parsed.serviceList.entryCount; i++) {
        PICO_MPEGTS_LOG("%*sEntry [%zu]:\n", indent + 2, "", i);
        PICO_MPEGTS_LOG("%*sService ID\t: 0x%04X\n", indent + 4, "", descriptor->parsed.serviceList.entries[i].serviceId);
        PICO_MPEGTS_LOG("%*sService Type\t: %s [0x%02X]\n", indent + 4, "",
                        picoMpegTSServiceTypeToString(descriptor->parsed.serviceList.entries[i].serviceType),
                        descriptor->parsed.serviceList.entries[i].serviceType);
    }
}

static void __picoMpegTSDescriptorPayloadNetworkNameDebugPrint(picoMpegTSDescriptor descriptor, int indent)
{
    (void)indent;
    if (descriptor == NULL) {
        return;
    }
    PICO_MPEGTS_LOG("%*sNetwork Name Descriptor:\n", indent, "");
    PICO_MPEGTS_LOG("%*sNetwork Name\t: %s\n", indent + 2, "", descriptor->parsed.networkName.name);
}

static void __picoMpegTSDescriptorPayloadParentalRatingDebugPrint(picoMpegTSDescriptor descriptor, int indent)
{
    (void)indent;
    if (descriptor == NULL) {
        return;
    }
    PICO_MPEGTS_LOG("%*sParental Rating Descriptor:\n", indent, "");
    for (size_t i = 0; i < descriptor->parsed.parentalRating.entryCount; i++) {
        PICO_MPEGTS_LOG("%*sEntry [%zu]:\n", indent + 2, "", i);
        PICO_MPEGTS_LOG("%*sCountry Code\t: %.3s\n", indent + 4, "", descriptor->parsed.parentalRating.entries[i].countryCode);

        uint8_t rating = descriptor->parsed.parentalRating.entries[i].rating;
        if (rating == 0x00) {
            PICO_MPEGTS_LOG("%*sRating\t: Undefined [0x00]\n", indent + 4, "");
        } else if (rating >= 0x01 && rating <= 0x0F) {
            PICO_MPEGTS_LOG("%*sRating\t: Minimum Age %u [0x%02X]\n", indent + 4, "", rating + 3, rating);
        } else {
            PICO_MPEGTS_LOG("%*sRating\t: Defined by broadcaster [0x%02X]\n", indent + 4, "", rating);
        }
    }
}

static void __picoMpegTSDescriptorPayloadDebugPrint(picoMpegTSDescriptor descriptor, int indent)
{
    (void)indent;
    if (descriptor == NULL) {
        return;
    }
    switch (descriptor->tag) {
        case PICO_MPEGTS_DESCRIPTOR_TAG_ISO_639_LANGUAGE:
            __picoMpegTSDescriptorPayloadISO639LanguageDebugPrint(descriptor, indent);
            break;
        case PICO_MPEGTS_DESCRIPTOR_TAG_SERVICE:
            __picoMpegTSDescriptorPayloadServiceDebugPrint(descriptor, indent);
            break;
        case PICO_MPEGTS_DESCRIPTOR_TAG_STREAM_IDENTIFIER:
            __picoMpegTSDescriptorPayloadStreamIdentifierDebugPrint(descriptor, indent);
            break;
        case PICO_MPEGTS_DESCRIPTOR_TAG_CA:
            __picoMpegTSDescriptorPayloadCADebugPrint(descriptor, indent);
            break;
        case PICO_MPEGTS_DESCRIPTOR_TAG_CONTENT:
            __picoMpegTSDescriptorPayloadContentDebugPrint(descriptor, indent);
            break;
        case PICO_MPEGTS_DESCRIPTOR_TAG_COMPONENT:
            __picoMpegTSDescriptorPayloadComponentDebugPrint(descriptor, indent);
            break;
        case PICO_MPEGTS_DESCRIPTOR_TAG_SHORT_EVENT:
            __picoMpegTSDescriptorPayloadShortEventDebugPrint(descriptor, indent);
            break;
        case PICO_MPEGTS_DESCRIPTOR_TAG_SERVICE_LIST:
            __picoMpegTSDescriptorPayloadServiceListDebugPrint(descriptor, indent);
            break;
        case PICO_MPEGTS_DESCRIPTOR_TAG_NETWORK_NAME:
            __picoMpegTSDescriptorPayloadNetworkNameDebugPrint(descriptor, indent);
            break;
        case PICO_MPEGTS_DESCRIPTOR_TAG_PARENTAL_RATING:
            __picoMpegTSDescriptorPayloadParentalRatingDebugPrint(descriptor, indent);
            break;
        default:
            PICO_MPEGTS_LOG("%*sDescriptor Payload: \n", indent, "");
            for (size_t i = 0; i < descriptor->dataLength; i++) {
                PICO_MPEGTS_LOG("%02X ", descriptor->data[i]);
            }
            PICO_MPEGTS_LOG("\n");
            break;
    }
}

static void __picoMpegTSDescriptorDebugPrint(picoMpegTSDescriptor descriptor, int indent)
{
    if (descriptor == NULL) {
        return;
    }
    PICO_MPEGTS_LOG("%*sDescriptor: Tag=0x%02X (%s), Length=%zu, Parsed=%s\n",
                    indent, "", descriptor->tag,
                    picoMpegTSDescriptorTagToString(descriptor->tag),
                    descriptor->dataLength,
                    descriptor->isParsed ? "true" : "false");
    if (descriptor->isParsed) {
        __picoMpegTSDescriptorPayloadDebugPrint(descriptor, indent + 2);
    }
}

static void __picoMpegTSDescriptorSetDebugPrint(picoMpegTSDescriptorSet set, int indent)
{
    if (set == NULL || set->count == 0) {
        return;
    }
    for (size_t i = 0; i < set->count; i++) {
        __picoMpegTSDescriptorDebugPrint(&set->descriptors[i], indent);
    }
}

static void __picoMpegTSUTCTimeDebugPrint(const picoMpegTSUTCTime_t *time, const char *label, int indent)
{
    (void)indent;
    (void)label;
    if (time == NULL)
        return;
    PICO_MPEGTS_LOG("%*s%s: Date: %04u-%02u-%02u, Time: %02u:%02u:%02u\n",
                    indent, "", label, time->year, time->month, time->day, time->hour, time->minute, time->second);
}

static void __picoMpegTSDurationDebugPrint(const picoMpegTSDuration_t *duration, const char *label, int indent)
{
    (void)indent;
    (void)label;
    if (duration == NULL)
        return;
    PICO_MPEGTS_LOG("%*s%s: %02X:%02X:%02X\n",
                    indent, "", label, duration->hours, duration->minutes, duration->seconds);
}

static void __picoMpegTSPASDebugPrint(const picoMpegTSProgramAssociationSectionPayload_t *pas)
{
    if (pas == NULL)
        return;
    PICO_MPEGTS_LOG("  Program Association Section (PAS):\n");
    PICO_MPEGTS_LOG("    Program Count: %zu\n", pas->programCount);
    for (size_t i = 0; i < pas->programCount; i++) {
        if (pas->programs[i].programNumber == 0x0000) {
            PICO_MPEGTS_LOG("    [%zu] Network PID: 0x%04X\n", i, pas->programs[i].pid);
        } else {
            PICO_MPEGTS_LOG("    [%zu] Program Number: %u, PMT PID: 0x%04X\n",
                            i, pas->programs[i].programNumber, pas->programs[i].pid);
        }
    }
}

static void __picoMpegTSCASDebugPrint(const picoMpegTSConditionalAccessSectionPayload_t *cas)
{
    if (cas == NULL)
        return;
    PICO_MPEGTS_LOG("  Conditional Access Section (CAS):\n");
    PICO_MPEGTS_LOG("    Descriptor Count: %zu\n", cas->descriptorSet.count);
    __picoMpegTSDescriptorSetDebugPrint((picoMpegTSDescriptorSet)&cas->descriptorSet, 4);
}

static void __picoMpegTSPMSDebugPrint(const picoMpegTSProgramMapSectionPayload_t *pms)
{
    if (pms == NULL)
        return;
    PICO_MPEGTS_LOG("  Program Map Section (PMS):\n");
    PICO_MPEGTS_LOG("    PCR PID: 0x%04X\n", pms->pcrPid);
    PICO_MPEGTS_LOG("    Program Info Descriptors: %zu\n", pms->programInfoDescriptorSet.count);
    __picoMpegTSDescriptorSetDebugPrint((picoMpegTSDescriptorSet)&pms->programInfoDescriptorSet, 4);
    PICO_MPEGTS_LOG("    Stream Count: %zu\n", pms->streamCount);
    for (size_t i = 0; i < pms->streamCount; i++) {
        PICO_MPEGTS_LOG("    [%zu] Stream Type: %s [0x%02X], Elementary PID: 0x%04X, ES Descriptors: %zu\n",
                        i, picoMpegTSStreamTypeToString(pms->streams[i].streamType), pms->streams[i].streamType, pms->streams[i].elementaryPid,
                        pms->streams[i].esInfoDescriptorSet.count);
        __picoMpegTSDescriptorSetDebugPrint((picoMpegTSDescriptorSet)&pms->streams[i].esInfoDescriptorSet, 6);
    }
}

static void __picoMpegTSTSDSDebugPrint(const picoMpegTSTransportStreamDescriptionSectionPayload_t *tsds)
{
    if (tsds == NULL)
        return;
    PICO_MPEGTS_LOG("  Transport Stream Description Section (TSDS):\n");
    PICO_MPEGTS_LOG("    Descriptor Count: %zu\n", tsds->descriptorSet.count);
    __picoMpegTSDescriptorSetDebugPrint((picoMpegTSDescriptorSet)&tsds->descriptorSet, 4);
}

static void __picoMpegTSMETASDebugPrint(const picoMpegTSMetadataSectionPayload_t *metas)
{
    if (metas == NULL)
        return;
    PICO_MPEGTS_LOG("  Metadata Section (METAS):\n");
    PICO_MPEGTS_LOG("    Metadata Byte Count: %zu\n", metas->metadataByteCount);
}

static void __picoMpegTSNITDebugPrint(const picoMpegTSNetworkInformationTablePayload_t *nit)
{
    if (nit == NULL)
        return;
    PICO_MPEGTS_LOG("  Network Information Table (NIT):\n");
    PICO_MPEGTS_LOG("    Network Descriptors: %zu\n", nit->descriptorSet.count);
    __picoMpegTSDescriptorSetDebugPrint((picoMpegTSDescriptorSet)&nit->descriptorSet, 4);
    PICO_MPEGTS_LOG("    Transport Stream Count: %zu\n", nit->transportStreamCount);
    for (size_t i = 0; i < nit->transportStreamCount; i++) {
        PICO_MPEGTS_LOG("    [%zu] TS ID: 0x%04X, Original Network ID: 0x%04X, Descriptors: %zu\n",
                        i, nit->transportStreams[i].transportStreamId,
                        nit->transportStreams[i].originalNetworkId,
                        nit->transportStreams[i].descriptorSet.count);
        __picoMpegTSDescriptorSetDebugPrint((picoMpegTSDescriptorSet)&nit->transportStreams[i].descriptorSet, 6);
    }
}

static void __picoMpegTSBATDebugPrint(const picoMpegTSBoquetAssociationTablePayload_t *bat)
{
    if (bat == NULL)
        return;
    PICO_MPEGTS_LOG("  Bouquet Association Table (BAT):\n");
    PICO_MPEGTS_LOG("    Bouquet Descriptors: %zu\n", bat->descriptorSet.count);
    __picoMpegTSDescriptorSetDebugPrint((picoMpegTSDescriptorSet)&bat->descriptorSet, 4);
    PICO_MPEGTS_LOG("    Service Count: %zu\n", bat->serviceCount);
    for (size_t i = 0; i < bat->serviceCount; i++) {
        PICO_MPEGTS_LOG("    [%zu] TS ID: 0x%04X, Original Network ID: 0x%04X, Descriptors: %zu\n",
                        i, bat->services[i].transportStreamId,
                        bat->services[i].originalNetworkId,
                        bat->services[i].descriptorSet.count);
        __picoMpegTSDescriptorSetDebugPrint((picoMpegTSDescriptorSet)&bat->services[i].descriptorSet, 6);
    }
}

static void __picoMpegTSSDTDebugPrint(const picoMpegTSServiceDescriptionTablePayload_t *sdt)
{
    if (sdt == NULL)
        return;
    PICO_MPEGTS_LOG("  Service Description Table (SDT):\n");
    PICO_MPEGTS_LOG("    Original Network ID: 0x%04X\n", sdt->originalNetworkId);
    PICO_MPEGTS_LOG("    Service Count: %zu\n", sdt->serviceCount);
    for (size_t i = 0; i < sdt->serviceCount; i++) {
        PICO_MPEGTS_LOG("    [%zu] Service ID: 0x%04X\n", i, sdt->services[i].serviceId);
        PICO_MPEGTS_LOG("      EIT Schedule Flag: %s\n", sdt->services[i].eitScheduleFlag ? "true" : "false");
        PICO_MPEGTS_LOG("      EIT Present/Following Flag: %s\n", sdt->services[i].eitPresentFollowingFlag ? "true" : "false");
        PICO_MPEGTS_LOG("      Running Status: %s\n", picoMpegTSSDTRunningStatusToString(sdt->services[i].runningStatus));
        PICO_MPEGTS_LOG("      Free CA Mode: %s\n", sdt->services[i].freeCAMode ? "true" : "false");
        PICO_MPEGTS_LOG("      Descriptors: %zu\n", sdt->services[i].descriptorSet.count);
        __picoMpegTSDescriptorSetDebugPrint((picoMpegTSDescriptorSet)&sdt->services[i].descriptorSet, 6);
    }
}

static void __picoMpegTSEITDebugPrint(const picoMpegTSEventInformationTablePayload_t *eit)
{
    if (eit == NULL)
        return;
    PICO_MPEGTS_LOG("  Event Information Table (EIT):\n");
    PICO_MPEGTS_LOG("    Transport Stream ID: 0x%04X\n", eit->transportStreamId);
    PICO_MPEGTS_LOG("    Original Network ID: 0x%04X\n", eit->originalNetworkId);
    PICO_MPEGTS_LOG("    Event Count: %zu\n", eit->eventCount);
    for (size_t i = 0; i < eit->eventCount; i++) {
        PICO_MPEGTS_LOG("    [%zu] Event ID: 0x%04X\n", i, eit->events[i].eventId);
        __picoMpegTSUTCTimeDebugPrint(&eit->events[i].startTime, "Start Time", 6);
        __picoMpegTSDurationDebugPrint(&eit->events[i].duration, "Duration", 6);
        PICO_MPEGTS_LOG("      Running Status: %s\n", picoMpegTSSDTRunningStatusToString(eit->events[i].runningStatus));
        PICO_MPEGTS_LOG("      Free CA Mode: %s\n", eit->events[i].freeCAMode ? "true" : "false");
        PICO_MPEGTS_LOG("      Descriptors: %zu\n", eit->events[i].descriptorSet.count);
        __picoMpegTSDescriptorSetDebugPrint((picoMpegTSDescriptorSet)&eit->events[i].descriptorSet, 6);
    }
}

static void __picoMpegTSTDTDebugPrint(const picoMpegTSTimeDateTablePayload_t *tdt)
{
    if (tdt == NULL)
        return;
    PICO_MPEGTS_LOG("  Time Date Table (TDT):\n");
    __picoMpegTSUTCTimeDebugPrint(&tdt->utcTime, "UTC Time", 4);
}

static void __picoMpegTSTOTDebugPrint(const picoMpegTSTimeOffsetTablePayload_t *tot)
{
    if (tot == NULL)
        return;
    PICO_MPEGTS_LOG("  Time Offset Table (TOT):\n");
    __picoMpegTSUTCTimeDebugPrint(&tot->utcTime, "UTC Time", 4);
    PICO_MPEGTS_LOG("    Descriptor Count: %zu\n", tot->descriptorSet.count);
    __picoMpegTSDescriptorSetDebugPrint((picoMpegTSDescriptorSet)&tot->descriptorSet, 4);
}

static void __picoMpegTSRSTDebugPrint(const picoMpegTSRunningStatusTablePayload_t *rst)
{
    if (rst == NULL)
        return;
    PICO_MPEGTS_LOG("  Running Status Table (RST):\n");
    PICO_MPEGTS_LOG("    Status Entry Count: %zu\n", rst->statusEntryCount);
    for (size_t i = 0; i < rst->statusEntryCount; i++) {
        PICO_MPEGTS_LOG("    [%zu] TS ID: 0x%04X, Original Network ID: 0x%04X, Service ID: 0x%04X, Event ID: 0x%04X, Running Status: %s\n",
                        i, rst->statusEntries[i].transportStreamId,
                        rst->statusEntries[i].originalNetworkId,
                        rst->statusEntries[i].serviceId,
                        rst->statusEntries[i].eventId,
                        picoMpegTSSDTRunningStatusToString(rst->statusEntries[i].runningStatus));
    }
}

static void __picoMpegTSDITDebugPrint(const picoMpegTSDiscriminationInformationTablePayload_t *dit)
{
    if (dit == NULL)
        return;
    PICO_MPEGTS_LOG("  Discontinuity Information Table (DIT):\n");
    PICO_MPEGTS_LOG("    Transition Flag: %s\n", dit->transitionFlag ? "true" : "false");
}

static void __picoMpegTSSITDebugPrint(const picoMpegTSServiceInformationTablePayload_t *sit)
{
    if (sit == NULL)
        return;
    PICO_MPEGTS_LOG("  Selection Information Table (SIT):\n");
    PICO_MPEGTS_LOG("    Transmission Info Loop Length: %u\n", sit->transmissionInfoLoopLength);
    PICO_MPEGTS_LOG("    Transmission Info Descriptors: %zu\n", sit->transmissionInfoDescriptorSet.count);
    __picoMpegTSDescriptorSetDebugPrint((picoMpegTSDescriptorSet)&sit->transmissionInfoDescriptorSet, 4);
    PICO_MPEGTS_LOG("    Service Count: %zu\n", sit->serviceCount);
    for (size_t i = 0; i < sit->serviceCount; i++) {
        PICO_MPEGTS_LOG("    [%zu] Service ID: 0x%04X, Running Status: %s\n",
                        i, sit->services[i].serviceId,
                        picoMpegTSSDTRunningStatusToString(sit->services[i].runningStatus));
        PICO_MPEGTS_LOG("      Descriptors: %zu\n", sit->services[i].descriptorSet.count);
        __picoMpegTSDescriptorSetDebugPrint((picoMpegTSDescriptorSet)&sit->services[i].descriptorSet, 6);
    }
}

void picoMpegTSTableDebugPrint(picoMpegTSTable table)
{
    if (table == NULL) {
        PICO_MPEGTS_LOG("Table: NULL\n");
        return;
    }

    PICO_MPEGTS_LOG("MPEG-TS Table:\n");
    PICO_MPEGTS_LOG("  Table ID: 0x%02X (%s)\n", table->tableId, picoMpegTSTableIDToString(table->tableId));
    PICO_MPEGTS_LOG("  Version Number: %u [%x]\n", table->versionNumber, table->versionNumber);

    int sectionCount = 0;
    for (int i = 0; i < PICO_MPEGTS_MAX_SECTIONS; i++) {
        if (table->hasSection[i])
            sectionCount++;
    }
    PICO_MPEGTS_LOG("  Sections Present: %d/%d\n", sectionCount, table->head.lastSectionNumber + 1);
    (void)sectionCount;

    switch (table->tableId) {
        case PICO_MPEGTS_TABLE_ID_PAS:
            __picoMpegTSPASDebugPrint(&table->data.pas);
            break;

        case PICO_MPEGTS_TABLE_ID_CAS:
            __picoMpegTSCASDebugPrint(&table->data.cas);
            break;

        case PICO_MPEGTS_TABLE_ID_PMS:
            __picoMpegTSPMSDebugPrint(&table->data.pms);
            break;

        case PICO_MPEGTS_TABLE_ID_TSDS:
            __picoMpegTSTSDSDebugPrint(&table->data.tsds);
            break;

        case PICO_MPEGTS_TABLE_ID_METAS:
            __picoMpegTSMETASDebugPrint(&table->data.metas);
            break;

        case PICO_MPEGTS_TABLE_ID_NISAN:
        case PICO_MPEGTS_TABLE_ID_NISON:
            __picoMpegTSNITDebugPrint(&table->data.nit);
            break;

        case PICO_MPEGTS_TABLE_ID_BAS:
            __picoMpegTSBATDebugPrint(&table->data.bat);
            break;

        case PICO_MPEGTS_TABLE_ID_SDSATS:
        case PICO_MPEGTS_TABLE_ID_SDSOTS:
            __picoMpegTSSDTDebugPrint(&table->data.sdt);
            break;

        case PICO_MPEGTS_TABLE_ID_EISATSF:
        case PICO_MPEGTS_TABLE_ID_EISOTSF:
            __picoMpegTSEITDebugPrint(&table->data.eit);
            break;

        case PICO_MPEGTS_TABLE_ID_TDS:
            __picoMpegTSTDTDebugPrint(&table->data.tdt);
            break;

        case PICO_MPEGTS_TABLE_ID_TOS:
            __picoMpegTSTOTDebugPrint(&table->data.tot);
            break;

        case PICO_MPEGTS_TABLE_ID_RSS:
            __picoMpegTSRSTDebugPrint(&table->data.rst);
            break;

        case PICO_MPEGTS_TABLE_ID_DIS:
            __picoMpegTSDITDebugPrint(&table->data.dit);
            break;

        case PICO_MPEGTS_TABLE_ID_SIS:
            __picoMpegTSSITDebugPrint(&table->data.sit);
            break;

        default:
            if ((table->tableId >= 0x50 && table->tableId <= 0x5F) ||
                (table->tableId >= 0x60 && table->tableId <= 0x6F)) {
                __picoMpegTSEITDebugPrint(&table->data.eit);
            } else {
                PICO_MPEGTS_LOG("  (Unknown or unsupported table type)\n");
            }
            break;
    }
}

#endif // PICO_IMPLEMENTATION

#endif // PICO_MPEGTS_H
