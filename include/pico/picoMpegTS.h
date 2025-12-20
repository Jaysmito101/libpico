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
 * - Jaysmito Mukherjee (jaysmito101@gmail.com)
 */

#ifndef PICO_MPEGTS_H
#define PICO_MPEGTS_H

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

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

typedef enum {
    PICO_MPEGTS_PACKET_TYPE_DEFAULT = 188,
    PICO_MPEGTS_PACKET_TYPE_M2TS    = 192,
    PICO_MPEGTS_PACKET_TYPE_DVB     = 204,
    PICO_MPEGTS_PACKET_TYPE_UNKNOWN = 0,
} picoMpegTSPacketType;

typedef enum {
    PICO_MPEGTS_PID_PAT            = 0x0000,
    PICO_MPEGTS_PID_CAT            = 0x0001,
    PICO_MPEGTS_PID_TSDT           = 0x0002,
    PICO_MPEGTS_PID_IPMP           = 0x0003,
    PICO_MPEGTS_PID_ASI            = 0x0004,
    PICO_MPEGTS_PID_RESERVED_START = 0x0005,
    PICO_MPEGTS_PID_RESERVED_END   = 0x000F,
    PICO_MPEGTS_PID_CUSTOM_START   = 0x0010,
    PICO_MPEGTS_PID_CUSTOM_END     = 0x1FFE,
    PICO_MPEGTS_PID_NULL_PACKET    = 0x1FFF,
} picoMpegTSPacketPID;

typedef enum {
    PICO_MPEGTS_ADAPTATION_FIELD_CONTROL_RESERVED        = 0x00,
    PICO_MPEGTS_ADAPTATION_FIELD_CONTROL_PAYLOAD_ONLY    = 0x01,
    PICO_MPEGTS_ADAPTATION_FIELD_CONTROL_ADAPTATION_ONLY = 0x02,
    PICO_MPEGTS_ADAPTATION_FIELD_CONTROL_BOTH            = 0x03,
} picoMpegTSAdaptionFieldControl;

typedef enum {
    PICO_MPEGTS_RESULT_SUCCESS = 0,
    PICO_MPEGTS_FILE_NOT_FOUND,
    PICO_MPEGTS_MALLOC_ERROR,
    PICO_MPEGTS_INVALID_DATA,
} picoMpegTSResult;

typedef struct picoMpegTS_t picoMpegTS_t;
typedef picoMpegTS_t *picoMpegTS;

typedef struct{
    bool discontinuityIndicator;
    bool randomAccessIndicator;
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
} picoMpegTSPacket_t ;
typedef picoMpegTSPacket_t *picoMpegTSPacket;

picoMpegTS picoMpegTSCreate(void);
void picoMpegTSDestroy(picoMpegTS mpegts);

picoMpegTSResult picoMpegTSAddPacket(picoMpegTS mpegts, const uint8_t *data);
picoMpegTSResult picoMpegTSAddBuffer(picoMpegTS mpegts, const uint8_t *buffer, size_t size);
picoMpegTSResult picoMpegTSAddFile(picoMpegTS mpegts, const char *filename);
picoMpegTSPacketType picoMpegTSDetectPacketType(const uint8_t *data, size_t size);
picoMpegTSPacketType picoMpegTSDetectPacketTypeFromFile(const char *filename);

picoMpegTSResult picoMpegTSParsePacket(const uint8_t *data, picoMpegTSPacket packetOut);
void picoMpegTSPacketDebugPrint(picoMpegTSPacket packet);

const char *picoMpegTSPacketTypeToString(picoMpegTSPacketType type);
const char *picoMpegTSResultToString(picoMpegTSResult result);
const char *picoMpegTSPIDToString(uint16_t pid);
const char *picoMpegTSAdaptionFieldControlToString(picoMpegTSAdaptionFieldControl afc);

#if defined(PICO_IMPLEMENTATION) && !defined(PICO_MPEGTS_IMPLEMENTATION)
#define PICO_MPEGTS_IMPLEMENTATION

struct picoMpegTS_t {
    uint8_t reserved;
};

// static bool __picoMpegTSIsLittleEndian(void)
// {
//     uint16_t test = 0x1;
//     return (*(uint8_t *)&test) == 0x1;
// }


picoMpegTSResult picoMpegTSParsePacket(const uint8_t *data, picoMpegTSPacket packet)
{
    PICO_ASSERT(packet != NULL);
    PICO_ASSERT(data != NULL);

    if (data[0] != 0x47) {
        return PICO_MPEGTS_INVALID_DATA;
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
}

picoMpegTS picoMpegTSCreate(void)
{
    picoMpegTS mpegts = (picoMpegTS)PICO_MALLOC(sizeof(picoMpegTS_t));
    if (!mpegts)
        return NULL;

    return mpegts;
}

void picoMpegTSDestroy(picoMpegTS mpegts)
{
    PICO_ASSERT(mpegts != NULL);
    PICO_FREE(mpegts);
}

// NOTE: Irrespective of type of packet we just use the first 188 bytes for parsing
// the rest of the portion isnt important for most use-cases and is
// out of the scope of this library for now
picoMpegTSResult picoMpegTSAddPacket(picoMpegTS mpegts, const uint8_t *data)
{
    PICO_ASSERT(mpegts != NULL);
    PICO_ASSERT(data != NULL);

    picoMpegTSPacket_t packet;
    picoMpegTSResult result = picoMpegTSParsePacket(data, &packet);
    if (result != PICO_MPEGTS_RESULT_SUCCESS) {
        return result;
    }

    picoMpegTSPacketDebugPrint(&packet);

    return PICO_MPEGTS_RESULT_SUCCESS;
}

picoMpegTSResult picoMpegTSAddBuffer(picoMpegTS mpegts, const uint8_t *buffer, size_t size)
{
    PICO_ASSERT(mpegts != NULL);
    PICO_ASSERT(buffer != NULL);

    picoMpegTSPacketType packetType = picoMpegTSDetectPacketType(buffer, size);
    if (packetType == PICO_MPEGTS_PACKET_TYPE_UNKNOWN) {
        return PICO_MPEGTS_INVALID_DATA;
    }

    size_t packetSize = (size_t)packetType;
    size_t offset     = 0;
    while (offset + packetSize <= size) {
        if (buffer[offset] != 0x47) {
            offset++;
            continue;
        }

        picoMpegTSResult result = picoMpegTSAddPacket(mpegts, &buffer[offset]);
        if (result != PICO_MPEGTS_RESULT_SUCCESS) {
            return result;
        }

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
        return PICO_MPEGTS_FILE_NOT_FOUND;
    }

    fseek(file, 0, SEEK_END);
    size_t fileSize = ftell(file);
    fseek(file, 0, SEEK_SET);
    uint8_t *buffer = (uint8_t *)PICO_MALLOC(fileSize);
    if (!buffer) {
        fclose(file);
        return PICO_MPEGTS_MALLOC_ERROR;
    }

    size_t bytesRead = fread(buffer, 1, fileSize, file);
    fclose(file);

    if (bytesRead != fileSize) {
        PICO_FREE(buffer);
        return PICO_MPEGTS_INVALID_DATA;
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
        case PICO_MPEGTS_FILE_NOT_FOUND:
            return "FILE_NOT_FOUND";
        case PICO_MPEGTS_MALLOC_ERROR:
            return "MEMORY_ALLOCATION_ERROR";
        case PICO_MPEGTS_INVALID_DATA:
            return "INVALID_DATA";
        default:
            return "UNKNOWN_ERROR";
    }
}

const char *picoMpegTSPIDToString(uint16_t pid)
{
    if (pid == PICO_MPEGTS_PID_PAT) {
        return "Program Association Table (PAT)";
    } else if (pid == PICO_MPEGTS_PID_CAT) {
        return "Conditional Access Table (CAT)";
    } else if (pid == PICO_MPEGTS_PID_TSDT) {
        return "Transport Stream Description Table (TSDT)";
    } else if (pid == PICO_MPEGTS_PID_IPMP) {
        return "IPMP Control Information";
    } else if (pid == PICO_MPEGTS_PID_ASI) {
        return "Adaptive Streaming Information (ASI)";
    } else if (pid >= PICO_MPEGTS_PID_RESERVED_START && pid <= PICO_MPEGTS_PID_RESERVED_END) {
        return "Reserved PID";
    } else if (pid >= PICO_MPEGTS_PID_CUSTOM_START && pid <= PICO_MPEGTS_PID_CUSTOM_END) {
        return "Custom PID";
    } else if (pid == PICO_MPEGTS_PID_NULL_PACKET) {
        return "Null Packet";
    } else {
        return "Unknown PID";
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

#endif // PICO_IMPLEMENTATION

#endif // PICO_MPEGTS_H
