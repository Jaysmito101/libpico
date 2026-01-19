#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define PICO_H264_LOG(...) printf(__VA_ARGS__)

#define PICO_IMPLEMENTATION
#include "pico/picoH264.h"

static picoH264SequenceParameterSet_t parsedSps[1024] = {0};
static size_t parsedSpsCount                          = 0;

static picoH264PictureParameterSet_t parsedPps[1024] = {0};
static size_t parsedPpsCount                         = 0;

static uint16_t idSliceIdMapping[1024][3] = {0};
static size_t idSliceIdMappingCount       = 0;

static uint8_t nalBuffer[1024 * 1024 * 16];        // 16 MB buffer for a single NAL unit
static uint8_t nalPayloadBuffer[1024 * 1024 * 16]; // 16 MB buffer for a single NAL unit

bool findSPSById(uint8_t spsId, picoH264SequenceParameterSet_t **spsOut)
{
    for (size_t i = 0; i < parsedSpsCount; i++) {
        if (parsedSps[i].seqParameterSetId == spsId) {
            *spsOut = &parsedSps[i];
            return true;
        }
    }
    return false;
}

bool findPPSById(uint8_t ppsId, picoH264PictureParameterSet_t **ppsOut)
{
    for (size_t i = 0; i < parsedPpsCount; i++) {
        if (parsedPps[i].picParameterSetId == ppsId) {
            *ppsOut = &parsedPps[i];
            return true;
        }
    }
    return false;
}

bool findPPSSPSBySliceId(uint16_t sliceId, picoH264PictureParameterSet_t **ppsOut, picoH264SequenceParameterSet_t **spsOut)
{
    for (size_t i = 0; i < idSliceIdMappingCount; i++) {
        if (idSliceIdMapping[i][2] == sliceId) {
            uint8_t ppsId = (uint8_t)idSliceIdMapping[i][0];
            uint8_t spsId = (uint8_t)idSliceIdMapping[i][1];

            if (!findPPSById(ppsId, ppsOut)) {
                return false;
            }
            if (!findSPSById(spsId, spsOut)) {
                return false;
            }
            return true;
        }
    }
    return false;
}

int main(int argc, char *argv[])
{
    if (argc < 2 || strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0) {
        printf("Usage: %s <input.h264>\n", argv[0]);
        printf("Parses an H.264 bitstream and prints NAL unit information.\n");
        return 0;
    }

    const char *h264DataFile = argv[1];
    FILE *file               = fopen(h264DataFile, "rb");
    if (!file) {
        printf("Error: Could not open H.264 data file: %s\n", h264DataFile);
        return 1;
    }
    fseek(file, 0, SEEK_END);
    long fileSize = ftell(file);
    fseek(file, 0, SEEK_SET);
    uint8_t *buffer = (uint8_t *)malloc(fileSize);
    fread(buffer, 1, fileSize, file);
    fclose(file);

    picoH264Bitstream bitstream = picoH264BitstreamFromBuffer(buffer, fileSize);
    if (!bitstream) {
        printf("Error: Could not create bitstream from buffer\n");
        free(buffer);
        return 1;
    }

    size_t nalUnitCount   = 0;
    size_t nalUnitSize    = 0;
    size_t nalPayloadSize = 0;

    static picoH264NALUnitHeader_t nalUnitHeader = {0};

    while (picoH264FindNextNALUnit(bitstream, &nalUnitSize)) {
        nalUnitCount++;
        printf("Found NAL Unit %zu at : %zu, size: %zu\n", nalUnitCount, bitstream->tell(bitstream->userData), nalUnitSize);

        if (!picoH264ReadNALUnit(bitstream, nalBuffer, sizeof(nalBuffer), nalUnitSize)) {
            printf("Error: Could not read NAL unit %zu\n", nalUnitCount);
            break;
        }

        if (!picoH264ParseNALUnit(nalBuffer, nalUnitSize, &nalUnitHeader, nalPayloadBuffer, &nalPayloadSize)) {
            printf("Error: Could not parse NAL unit %zu\n", nalUnitCount);
            break;
        }

        picoH264NALUnitHeaderDebugPrint(&nalUnitHeader);
        printf("NAL Unit %zu payload size: %zu bytes\n", nalUnitCount, nalPayloadSize);
        printf("--------------------------------------------------\n");

        switch (nalUnitHeader.nalUnitType) {
            case PICO_H264_NAL_UNIT_TYPE_AUD: {
                static picoH264AccessUnitDelimiter_t aud = {0};
                if (picoH264ParseAccessUnitDelimiter(nalPayloadBuffer, nalPayloadSize, &aud)) {
                    printf("Parsed AUD in NAL Unit %zu\n", nalUnitCount);
                    picoH264AccessUnitDelimiterDebugPrint(&aud);
                } else {
                    printf("Error: Could not parse AUD in NAL Unit %zu\n", nalUnitCount);
                }
                break;
            }
            case PICO_H264_NAL_UNIT_TYPE_SEI: {
                static picoH264SEIMessage_t messages[1024];
                size_t messageCount = 0;
                if (picoH264ParseSEIMessages(nalPayloadBuffer, nalPayloadSize, messages, sizeof(messages) / sizeof(messages[0]), &messageCount)) {
                    printf("Parsed %zu SEI messages in NAL Unit %zu\n", messageCount, nalUnitCount);
                    for (size_t i = 0; i < messageCount; i++) {
                        picoH264SEIMessageDebugPrint(&messages[i]);
                    }
                } else {
                    printf("Error: Could not parse SEI messages in NAL Unit %zu\n", nalUnitCount);
                }

                break;
            }
            case PICO_H264_NAL_UNIT_TYPE_SPS: {
                static picoH264SequenceParameterSet_t sps = {0};
                if (picoH264ParseSequenceParameterSet(nalPayloadBuffer, nalPayloadSize, &sps)) {
                    printf("Parsed SPS in NAL Unit %zu\n", nalUnitCount);
                    picoH264SequenceParameterSetDebugPrint(&sps);
                    if (parsedSpsCount < sizeof(parsedSps) / sizeof(parsedSps[0])) {
                        memcpy(&parsedSps[parsedSpsCount], &sps, sizeof(picoH264SequenceParameterSet_t));
                        parsedSpsCount++;
                    }
                } else {
                    printf("Error: Could not parse SPS in NAL Unit %zu\n", nalUnitCount);
                }
                break;
            }
            case PICO_H264_NAL_UNIT_TYPE_SUBSET_SPS: {
                static picoH264SubsetSequenceParameterSet_t subsetSps = {0};
                if (picoH264ParseSubsetSequenceParameterSet(nalPayloadBuffer, nalPayloadSize, &subsetSps)) {
                    printf("Parsed Subset SPS in NAL Unit %zu\n", nalUnitCount);
                    picoH264SubsetSequenceParameterSetDebugPrint(&subsetSps);
                } else {
                    printf("Error: Could not parse Subset SPS in NAL Unit %zu\n", nalUnitCount);
                }
                break;
            }
            case PICO_H264_NAL_UNIT_TYPE_SPS_EXT: {
                static picoH264SequenceParameterSetExtension_t spsExt = {0};
                if (picoH264ParseSequenceParameterSetExtension(nalPayloadBuffer, nalPayloadSize, &spsExt)) {
                    printf("Parsed SPS Extension in NAL Unit %zu\n", nalUnitCount);
                    picoH264SequenceParameterSetExtensionDebugPrint(&spsExt);
                } else {
                    printf("Error: Could not parse SPS Extension in NAL Unit %zu\n", nalUnitCount);
                }
                break;
            }
            case PICO_H264_NAL_UNIT_TYPE_PPS: {
                static picoH264PictureParameterSet_t pps = {0};
                uint8_t spsId                            = 0;

                if (picoH264PictureParameterSetParseSPSId(nalPayloadBuffer, nalPayloadSize, &spsId)) {
                    printf("PPS in NAL Unit %zu references SPS ID: %u\n", nalUnitCount, spsId);

                    picoH264SequenceParameterSet sps = NULL;
                    if (!findSPSById(spsId, &sps)) {
                        printf("Error: Could not find referenced SPS ID %u for PPS in NAL Unit %zu\n", spsId, nalUnitCount);
                        break;
                    }

                    if (picoH264ParsePictureParameterSet(nalPayloadBuffer, nalPayloadSize, sps, &pps)) {
                        printf("Parsed PPS in NAL Unit %zu\n", nalUnitCount);
                        picoH264PictureParameterSetDebugPrint(&pps);

                        if (parsedPpsCount < sizeof(parsedPps) / sizeof(parsedPps[0])) {
                            memcpy(&parsedPps[parsedPpsCount], &pps, sizeof(picoH264PictureParameterSet_t));
                            parsedPpsCount++;
                        }
                    } else {
                        printf("Error: Could not parse PPS in NAL Unit %zu\n", nalUnitCount);
                    }
                } else {
                    printf("Error: Could not get SPS ID from PPS in NAL Unit %zu\n", nalUnitCount);
                    break;
                }
                break;
            }
            case PICO_H264_NAL_UNIT_TYPE_CODED_SLICE_DATA_PART_A: {
                static picoH264SliceDataPartitionALayer_t slice = {0};
                uint8_t ppsId                                   = 0;
                uint8_t spsId                                   = 0;
                if (picoH264SliceHeaderParsePPSId(nalPayloadBuffer, nalPayloadSize, &ppsId)) {
                    printf("Slice in NAL Unit %zu references PPS ID: %u\n", nalUnitCount, ppsId);

                    picoH264PictureParameterSet pps = NULL;
                    if (!findPPSById(ppsId, &pps)) {
                        printf("Error: Could not find referenced PPS ID %u for Slice in NAL Unit %zu\n", ppsId, nalUnitCount);
                        break;
                    }
                    spsId                            = pps->seqParameterSetId;
                    picoH264SequenceParameterSet sps = NULL;
                    if (!findSPSById(spsId, &sps)) {
                        printf("Error: Could not find referenced SPS ID %u for Slice in NAL Unit %zu\n", spsId, nalUnitCount);
                        break;
                    }

                    if (picoH264ParseSliceDataPartitionALayer(nalPayloadBuffer, nalPayloadSize, &nalUnitHeader, sps, pps, &slice)) {
                        printf("Parsed Slice in NAL Unit %zu\n", nalUnitCount);
                        picoH264SliceDataPartitionALayerDebugPrint(&slice);

                        idSliceIdMapping[idSliceIdMappingCount][0] = ppsId;
                        idSliceIdMapping[idSliceIdMappingCount][1] = spsId;
                        idSliceIdMapping[idSliceIdMappingCount][2] = slice.sliceId;
                    } else {
                        printf("Error: Could not parse Slice in NAL Unit %zu\n", nalUnitCount);
                    }
                } else {
                    printf("Error: Could not get PPS ID from Slice in NAL Unit %zu\n", nalUnitCount);
                    break;
                }
                break;
            }
            case PICO_H264_NAL_UNIT_TYPE_CODED_SLICE_DATA_PART_B: {
                static picoH264SliceDataPartitionBLayer_t slice = {0};
                uint16_t sliceId                                = 0;
                if (picoH264SliceDataPartitionBCLayerParseSliceId(nalPayloadBuffer, nalPayloadSize, &sliceId)) {
                    printf("Slice in NAL Unit %zu has Slice ID: %u\n", nalUnitCount, sliceId);

                    picoH264PictureParameterSet pps  = NULL;
                    picoH264SequenceParameterSet sps = NULL;
                    if (!findPPSSPSBySliceId(sliceId, &pps, &sps)) {
                        printf("Error: Could not find referenced PPS/SPS for Slice ID %u in NAL Unit %zu\n", sliceId, nalUnitCount);
                        break;
                    }

                    if (picoH264ParseSliceDataPartitionBLayer(nalPayloadBuffer, nalPayloadSize, sps, pps, &slice)) {
                        printf("Parsed Slice in NAL Unit %zu\n", nalUnitCount);
                        picoH264SliceDataPartitionBLayerDebugPrint(&slice);
                    } else {
                        printf("Error: Could not parse Slice in NAL Unit %zu\n", nalUnitCount);
                    }
                } else {
                    printf("Error: Could not get Slice ID from Slice in NAL Unit %zu\n", nalUnitCount);
                    break;
                }
                break;
            }
            case PICO_H264_NAL_UNIT_TYPE_CODED_SLICE_DATA_PART_C: {
                static picoH264SliceDataPartitionCLayer_t slice = {0};
                uint16_t sliceId                                = 0;
                if (picoH264SliceDataPartitionBCLayerParseSliceId(nalPayloadBuffer, nalPayloadSize, &sliceId)) {
                    printf("Slice in NAL Unit %zu has Slice ID: %u\n", nalUnitCount, sliceId);

                    picoH264PictureParameterSet pps  = NULL;
                    picoH264SequenceParameterSet sps = NULL;
                    if (!findPPSSPSBySliceId(sliceId, &pps, &sps)) {
                        printf("Error: Could not find referenced PPS/SPS for Slice ID %u in NAL Unit %zu\n", sliceId, nalUnitCount);
                        break;
                    }

                    if (picoH264ParseSliceDataPartitionCLayer(nalPayloadBuffer, nalPayloadSize, sps, pps, &slice)) {
                        printf("Parsed Slice in NAL Unit %zu\n", nalUnitCount);
                        picoH264SliceDataPartitionCLayerDebugPrint(&slice);
                    } else {
                        printf("Error: Could not parse Slice in NAL Unit %zu\n", nalUnitCount);
                    }
                } else {
                    printf("Error: Could not get Slice ID from Slice in NAL Unit %zu\n", nalUnitCount);
                    break;
                }
                break;
            }
            case PICO_H264_NAL_UNIT_TYPE_CODED_SLICE_NON_IDR:
            case PICO_H264_NAL_UNIT_TYPE_CODED_SLICE_IDR:
            case PICO_H264_NAL_UNIT_TYPE_AUXILIARY_SLICE: {
                static picoH264SliceLayerWithoutPartitioning_t slice = {0};
                uint8_t ppsId                                        = 0;
                uint8_t spsId                                        = 0;
                if (picoH264SliceHeaderParsePPSId(nalPayloadBuffer, nalPayloadSize, &ppsId)) {
                    printf("Slice in NAL Unit %zu references PPS ID: %u\n", nalUnitCount, ppsId);

                    picoH264PictureParameterSet_t *pps = NULL;
                    if (!findPPSById(ppsId, &pps)) {
                        printf("Error: Could not find referenced PPS ID %u for Slice in NAL Unit %zu\n", ppsId, nalUnitCount);
                        break;
                    }
                    spsId                               = pps->seqParameterSetId;
                    picoH264SequenceParameterSet_t *sps = NULL;
                    if (!findSPSById(spsId, &sps)) {
                        printf("Error: Could not find referenced SPS ID %u for Slice in NAL Unit %zu\n", spsId, nalUnitCount);
                        break;
                    }

                    if (picoH264ParseSliceLayerWithoutPartitioning(nalPayloadBuffer, nalPayloadSize, &nalUnitHeader, sps, pps, &slice)) {
                        printf("Parsed Slice in NAL Unit %zu\n", nalUnitCount);
                        picoH264SliceLayerWithoutPartitioningDebugPrint(&slice);
                    } else {
                        printf("Error: Could not parse Slice in NAL Unit %zu\n", nalUnitCount);
                    }
                } else {
                    printf("Error: Could not get PPS ID from Slice in NAL Unit %zu\n", nalUnitCount);
                    break;
                }

                break;
            }
            default:
                printf("NAL Unit %zu is of type %u [No parser available]\n", nalUnitCount, nalUnitHeader.nalUnitType);
                break;
        }

        printf("==================================================\n");
    }

    picoH264BitstreamDestroy(bitstream);
    free(buffer);

    printf("Goodbye, Pico!\n");
    return 0;
}
