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

#define PICO_IMPLEMENTATION

#ifndef PICO_INITIAL_PARSED_PACKETS_CAPACITY
#define PICO_INITIAL_PARSED_PACKETS_CAPACITY 1024
#endif

#define PICO_MPEGTS_RETURN_ON_ERROR(resultVar)   \
    do {                                         \
        picoMpegTSResult res = resultVar;        \
        if (res != PICO_MPEGTS_RESULT_SUCCESS) { \
            return res;                          \
        }                                        \
    } while (0)

// Maximum number of PIDs (13-bit PID = 0x0000 to 0x1FFF)
#ifndef PICO_MPEGTS_MAX_PIDS
#define PICO_MPEGTS_MAX_PIDS 8192
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
    } while (0)
#endif

#define PICO_MPEGTS_MAX_PID_COUNT 8192

typedef enum {
    PICO_MPEGTS_PACKET_TYPE_DEFAULT = 188,
    PICO_MPEGTS_PACKET_TYPE_M2TS    = 192,
    PICO_MPEGTS_PACKET_TYPE_DVB     = 204,
    PICO_MPEGTS_PACKET_TYPE_UNKNOWN = 0,
} picoMpegTSPacketType;

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

// 0x04 -> 0x3F reserved
// 0x43 -> 0x45 reserved
// 0x47 -> 0x49 reserved
// 0x4D reserved
typedef enum {
    PICO_MPEGTS_TABLE_ID_PAS                = 0x00, // Program Association Section
    PICO_MPEGTS_TABLE_ID_CAS                = 0x01, // Conditional Access Section
    PICO_MPEGTS_TABLE_ID_PMS                = 0x02, // Program Map Section
    PICO_MPEGTS_TABLE_ID_TSDS               = 0x03, // Transport Stream Description Section
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
} picoMpegTSTableID;

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
    PICO_MPEGTS_RESULT_UNKNOWN_ERROR,
} picoMpegTSResult;

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

picoMpegTS picoMpegTSCreate(bool storeParsedPackets);
void picoMpegTSDestroy(picoMpegTS mpegts);
picoMpegTSResult picoMpegTSGetParsedPackets(picoMpegTS mpegts, picoMpegTSPacket *packetsOut, size_t *packetCountOut);

picoMpegTSResult picoMpegTSAddPacket(picoMpegTS mpegts, const uint8_t *data);
picoMpegTSResult picoMpegTSAddBuffer(picoMpegTS mpegts, const uint8_t *buffer, size_t size);
picoMpegTSResult picoMpegTSAddFile(picoMpegTS mpegts, const char *filename);
picoMpegTSPacketType picoMpegTSDetectPacketType(const uint8_t *data, size_t size);
picoMpegTSPacketType picoMpegTSDetectPacketTypeFromFile(const char *filename);

picoMpegTSResult picoMpegTSParsePacket(const uint8_t *data, picoMpegTSPacket packetOut);

void picoMpegTSPacketDebugPrint(picoMpegTSPacket packet);
void picoMpegTSPacketAdaptationFieldDebugPrint(picoMpegTSPacketAdaptationField adaptationField);
void picoMpegTSPacketAdaptationFieldExtensionDebugPrint(picoMpegTSAdaptionFieldExtension adaptationFieldExtension);

const char *picoMpegTSPacketTypeToString(picoMpegTSPacketType type);
const char *picoMpegTSResultToString(picoMpegTSResult result);
const char *picoMpegTSPIDToString(uint16_t pid);
const char *picoMpegTSTableIDToString(uint8_t tableID);
const char *picoMpegTSAdaptionFieldControlToString(picoMpegTSAdaptionFieldControl afc);

#if defined(PICO_IMPLEMENTATION) && !defined(PICO_MPEGTS_IMPLEMENTATION)
#define PICO_MPEGTS_IMPLEMENTATION

// the filter function
typedef struct picoMpegTSFilterContext_t picoMpegTSFilterContext_t;
typedef struct picoMpegTSFilterContext_t *picoMpegTSFilterContext;

typedef picoMpegTSResult (*picoMpegTSFilterFunc)(picoMpegTS mpegts, picoMpegTSPacket packet, picoMpegTSFilterContext context);
typedef void (*picoMpegTSFilterContextDestructorFunc)(picoMpegTSFilterContext context);

struct picoMpegTSFilterContext_t {
    picoMpegTSFilterFunc filterFunc;
    picoMpegTSFilterContextDestructorFunc destructorFunc;
    void *userContext;

    uint8_t *payloadAccumulator;
    size_t payloadAccumulatorSize;
    size_t payloadAccumulatorCapacity;

    uint8_t lastContinuityCounter;
    bool continuityErrorDetected;
};

struct picoMpegTS_t {
    bool storeParsedPackets;
    picoMpegTSPacket parsedPackets;
    size_t parsedPacketCount;
    size_t parsedPacketCapacity;

    bool hasContinuityError;
    uint32_t ignoredPacketCount;

    picoMpegTSFilterContext pidFilters[PICO_MPEGTS_MAX_PID_COUNT];
};

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

static picoMpegTSFilterContext __picoMpegTSCreateFilterContext(
    picoMpegTS mpegts,
    picoMpegTSFilterFunc filterFunc,
    picoMpegTSFilterContextDestructorFunc destructorFunc,
    void *userContext)
{
    PICO_ASSERT(mpegts != NULL);
    PICO_ASSERT(filterFunc != NULL);

    picoMpegTSFilterContext context = (picoMpegTSFilterContext)PICO_MALLOC(sizeof(picoMpegTSFilterContext_t));
    if (!context) {
        return NULL;
    }

    memset(context, 0, sizeof(picoMpegTSFilterContext_t));
    context->filterFunc     = filterFunc;
    context->destructorFunc = destructorFunc;
    context->userContext    = userContext;

    context->payloadAccumulator         = NULL;
    context->payloadAccumulatorSize     = 0;
    context->payloadAccumulatorCapacity = 0;

    context->lastContinuityCounter   = 16; // invalid initial value
    context->continuityErrorDetected = false;

    return context;
}

static picoMpegTSFilterContext __picoMpegTSCreatePESFilterContext(picoMpegTS mpegts, uint16_t pid)
{
    PICO_ASSERT(mpegts != NULL);
    PICO_ASSERT(pid < PICO_MPEGTS_MAX_PID_COUNT);

    // TODO : Implement PES filter context creation

    return NULL;
}

static void __picoMpegTSDestroyFilterContext(picoMpegTS mpegts, picoMpegTSFilterContext context)
{
    PICO_ASSERT(mpegts != NULL);
    PICO_ASSERT(context != NULL);

    if (context->destructorFunc) {
        context->destructorFunc(context);
    }

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

static picoMpegTSResult __picoMpegTSFilterContextApply(picoMpegTSFilterContext filterContext, picoMpegTS mpegts, picoMpegTSPacket packet)
{
    PICO_ASSERT(filterContext != NULL);
    PICO_ASSERT(mpegts != NULL);
    PICO_ASSERT(packet != NULL);

    // append the packet payload to the accumulator
    if (packet->payloadSize > 0) {
        // ensure capacity
        size_t requiredCapacity = filterContext->payloadAccumulatorSize + packet->payloadSize;
        if (requiredCapacity > filterContext->payloadAccumulatorCapacity) {
            size_t newCapacity = filterContext->payloadAccumulatorCapacity == 0 ? 256 : filterContext->payloadAccumulatorCapacity;
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
        // copy payload
        memcpy(&filterContext->payloadAccumulator[filterContext->payloadAccumulatorSize], packet->payload, packet->payloadSize);
        filterContext->payloadAccumulatorSize += packet->payloadSize;
    }

    // // Validate continuity counter if enabled
    // // Per ITU-T H.222.0: The continuity_counter shall not be incremented when
    // // adaptation_field_control equals '00' (reserved) or '10' (adaptation field only, no payload)
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

    return filterContext->filterFunc(mpegts, packet, filterContext);
}

// ---------------------------------- Filter Api Functions ---------------------------------

// ------------- NAT Filter Functions ---------------

static void __picoMpegTSFilterContextNATDestructor(picoMpegTSFilterContext context)
{
    PICO_ASSERT(context != NULL);
    // nothing to do for now
}

static picoMpegTSResult __picoMpegTSFilterNAT(picoMpegTS mpegts, picoMpegTSPacket packet, picoMpegTSFilterContext context)
{
    PICO_ASSERT(mpegts != NULL);
    PICO_ASSERT(packet != NULL);
    PICO_ASSERT(context != NULL);

    return PICO_MPEGTS_RESULT_SUCCESS;
}

static picoMpegTSResult __picoMpegTSFilterCreateNAT(picoMpegTS mpegts, uint16_t pid)
{
    PICO_ASSERT(mpegts != NULL);
    PICO_ASSERT(pid < PICO_MPEGTS_MAX_PID_COUNT);

    if (mpegts->pidFilters[pid] != NULL) {
        // already exists
        return PICO_MPEGTS_RESULT_SUCCESS;
    }

    picoMpegTSFilterContext natFilterContext = __picoMpegTSCreateFilterContext(
        mpegts,
        __picoMpegTSFilterNAT,
        __picoMpegTSFilterContextNATDestructor,
        NULL);
    if (!natFilterContext) {
        PICO_MPEGTS_LOG("picoMpegTS: Failed to create NAT filter context for PID 0x%04X\n", pid);
        return PICO_MPEGTS_RESULT_MALLOC_ERROR;
    }

    mpegts->pidFilters[pid] = natFilterContext;

    return PICO_MPEGTS_RESULT_SUCCESS;
}

static picoMpegTSResult __picoMpegTSRegisterPSIFilters(picoMpegTS mpegts)
{
    PICO_ASSERT(mpegts != NULL);

    // register NAT filter
    PICO_MPEGTS_RETURN_ON_ERROR(__picoMpegTSFilterCreateNAT(mpegts, PICO_MPEGTS_PID_PAT));
        

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

picoMpegTS picoMpegTSCreate(bool storeParsedPackets)
{
    picoMpegTS mpegts = (picoMpegTS)PICO_MALLOC(sizeof(picoMpegTS_t));
    if (!mpegts) {
        return NULL;
    }

    memset(mpegts, 0, sizeof(picoMpegTS_t));

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
    // free all the filter contexts
    for (size_t i = 0; i < PICO_MPEGTS_MAX_PID_COUNT; i++) {
        if (mpegts->pidFilters[i]) {
            __picoMpegTSDestroyFilterContext(mpegts, mpegts->pidFilters[i]);
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

    picoMpegTSFilterContext filterContext = mpegts->pidFilters[packet.pid];

    // if there is no filter for this PID and it is not a custom PID then this is an error
    if (!filterContext && !__picoMpegTSIsPIDCustom(packet.pid)) {
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
        mpegts->pidFilters[packet.pid] = __picoMpegTSCreatePESFilterContext(mpegts, packet.pid);
        if (!mpegts->pidFilters[packet.pid]) {
            return PICO_MPEGTS_RESULT_MALLOC_ERROR;
        }
        return __picoMpegTSFilterContextApply(mpegts->pidFilters[packet.pid], mpegts, &packet);
    }

    mpegts->ignoredPacketCount++;
    PICO_MPEGTS_LOG("Ignoring packet with custom PID 0x%04X without filter.\n", packet.pid);

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
            if (tableID >= PICO_MPEGTS_TABLE_ID_USER_DEFINED_START && tableID <= PICO_MPEGTS_TABLE_ID_USER_DEFINED_END) {
                return "User Defined Table ID";
            }
            return "Reserved/Unknown Table ID";
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

#endif // PICO_IMPLEMENTATION

#endif // PICO_MPEGTS_H
