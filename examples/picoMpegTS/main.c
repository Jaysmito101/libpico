#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PICO_MPEGTS_LOG(...) printf(__VA_ARGS__)
#define PICO_IMPLEMENTATION
#include "pico/picoMpegTS.h"

void printUsage(const char *progName)
{
    printf("MPEG-TS Parser and Validator - picoMpegTS Demo\n");
    printf("Usage: %s <input.ts>\n", progName);
}

int main(int argc, char **argv)
{
    printf("Hello, Pico!\n");

    if (argc < 2) {
        printUsage(argv[0]);
        return 1;
    }

    const char* input_filename = argv[1];
    printf("Input file: %s\n", input_filename);

    printf("Detecting MPEG-TS packet type...\n");
    picoMpegTSPacketType packetType = picoMpegTSDetectPacketTypeFromFile(input_filename);
    printf("Detected packet type: %s\n", picoMpegTSPacketTypeToString(packetType));

    if (packetType == PICO_MPEGTS_PACKET_TYPE_UNKNOWN) {
        printf("Error: Unknown or invalid MPEG-TS packet type.\n");
        return 1;
    }

    printf("Validating TS file...\n");
    picoMpegTS mpegts = picoMpegTSCreate(true);
    if (!mpegts) {
        printf("Error: Failed to create picoMpegTS context.\n");
        return 1;
    }

    printf("Processing file...\n");
    picoMpegTSResult result = picoMpegTSAddFile(mpegts, input_filename);
    if (result != PICO_MPEGTS_RESULT_SUCCESS) {
        printf("Error adding file: %s\n", picoMpegTSResultToString(result));
        picoMpegTSDestroy(mpegts);
        return 1;
    }
    printf("File processed successfully.\n");

    size_t pesPacketCount = 0;
    picoMpegTSPESPacket* pesPackets = picoMpegTSGetPESPackets(mpegts, &pesPacketCount);
    printf("\nSummary:\n");
    printf("  Total PES Packets Found: %zu\n", pesPacketCount);

    size_t videoCount = 0;
    size_t audioCount = 0;
    size_t otherCount = 0;

    for (size_t i = 0; i < pesPacketCount; i++) {
        picoMpegTSPESPacket packet = pesPackets[i];
        if (picoMpegTSIsStreamIDVideo(packet->head.streamId)) {
            videoCount++;
        } else if (picoMpegTSIsStreamIDAudio(packet->head.streamId)) {
            audioCount++;
        } else {
            otherCount++;
        }
    }

    printf("  Video Packets: %zu\n", videoCount);
    printf("  Audio Packets: %zu\n", audioCount);
    printf("  Other Packets: %zu\n", otherCount);

    if (pesPacketCount > 0) {
        printf("\nFirst 5 PES Packets:\n");
        for (size_t i = 0; i < (pesPacketCount < 5 ? pesPacketCount : 5); i++) {
            picoMpegTSPESPacket packet = pesPackets[i];
            printf("  [%zu] StreamID: 0x%02X (%s), Length: %zu\n", 
                   i, 
                   packet->head.streamId, 
                   picoMpegTSPESStreamIDToString(packet->head.streamId), 
                   packet->dataLength);
        }
    }

    picoMpegTSDebugPrintInfo_t info = { .printPESPackets = false, .printCurrentTables = true };
    picoMpegTSDebugPrint(mpegts, &info);

    picoMpegTSDestroy(mpegts);

    printf("Goodbye, Pico!\n");
    return 0;
}
