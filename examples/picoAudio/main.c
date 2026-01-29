#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PICO_AUDIO_LOG(...) printf(__VA_ARGS__)

#define PICO_IMPLEMENTATION
#include "pico/picoAudio.h"

static void writeWavHeader(FILE *file, uint32_t sampleRate, uint16_t channels,
                           uint16_t bitsPerSample, uint32_t dataSize)
{
    uint32_t byteRate   = sampleRate * channels * bitsPerSample / 8;
    uint16_t blockAlign = channels * bitsPerSample / 8;
    uint32_t chunkSize  = 36 + dataSize;

    fwrite("RIFF", 1, 4, file);
    fwrite(&chunkSize, 4, 1, file);
    fwrite("WAVE", 1, 4, file);

    fwrite("fmt ", 1, 4, file);
    uint32_t fmtChunkSize = 16;
    fwrite(&fmtChunkSize, 4, 1, file);
    uint16_t audioFormat = 1;
    fwrite(&audioFormat, 2, 1, file);
    fwrite(&channels, 2, 1, file);
    fwrite(&sampleRate, 4, 1, file);
    fwrite(&byteRate, 4, 1, file);
    fwrite(&blockAlign, 2, 1, file);
    fwrite(&bitsPerSample, 2, 1, file);

    fwrite("data", 1, 4, file);
    fwrite(&dataSize, 4, 1, file);
}

static void printUsage(const char *programName)
{
    printf("Usage: %s <input_audio_file> [-o output.wav] [-p output.pcm]\n\n", programName);
    printf("Options:\n");
    printf("  -o <file>   Save decoded audio as WAV file\n");
    printf("  -p <file>   Save decoded audio as raw PCM file\n");
    printf("\nSupported formats: AAC (ADTS), M4A, MP3, WAV, AIFF, CAF\n");
}

int main(int argc, char *argv[])
{
    printf("Hello, Pico!\n");

    if (argc < 2) {
        printUsage(argv[0]);
        return 0;
    }

    const char *inputFile = NULL;
    const char *wavOutput = NULL;
    const char *pcmOutput = NULL;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) {
            wavOutput = argv[++i];
        } else if (strcmp(argv[i], "-p") == 0 && i + 1 < argc) {
            pcmOutput = argv[++i];
        } else if (argv[i][0] != '-') {
            inputFile = argv[i];
        }
    }

    if (!inputFile) {
        printf("fatal: No input file specified.\n\n");
        printUsage(argv[0]);
        return 1;
    }

    printf("Decoding: %s\n", inputFile);
    if (wavOutput) {
        printf("WAV Output: %s\n", wavOutput);
    }

    if (pcmOutput) {
        printf("PCM Output: %s\n", pcmOutput);
    }

    printf("\n");

    picoAudioDecoder decoder = picoAudioDecoderCreate();
    if (!decoder) {
        printf("fatal: Failed to create decoder!\n");
        return 1;
    }

    picoAudioResult result = picoAudioDecoderOpenFile(decoder, inputFile);
    if (result != PICO_AUDIO_RESULT_SUCCESS) {
        printf("fatal: %s\n", picoAudioResultToString(result));
        picoAudioDecoderDestroy(decoder);
        return 1;
    }

    picoAudioInfo_t audioInfo;
    result = picoAudioDecoderGetAudioInfo(decoder, &audioInfo);
    if (result != PICO_AUDIO_RESULT_SUCCESS) {
        printf("fatal: %s\n", picoAudioResultToString(result));
        picoAudioDecoderDestroy(decoder);
        return 1;
    }

    printf("Sample Rate:     %u Hz\n", audioInfo.sampleRate);
    printf("Channels:        %u\n", audioInfo.channelCount);
    printf("Bits per Sample: %u bit\n", audioInfo.bitsPerSample);
    printf("Total Samples:   %" PRIu64 "\n", audioInfo.totalSamples);
    printf("Duration:        %.2f seconds (%.2f minutes)\n", audioInfo.durationSeconds, audioInfo.durationSeconds / 60.0);
    printf("\n");

    if (wavOutput || pcmOutput) {
        FILE *wavFile = wavOutput ? fopen(wavOutput, "wb") : NULL;
        FILE *pcmFile = pcmOutput ? fopen(pcmOutput, "wb") : NULL;

        if (wavOutput && !wavFile) {
            printf("fatal: Cannot create WAV output file!\n");
            picoAudioDecoderDestroy(decoder);
            return 1;
        }
        if (pcmOutput && !pcmFile) {
            printf("fatal: Cannot create PCM output file!\n");
            if (wavFile) {
                fclose(wavFile);
            }
            picoAudioDecoderDestroy(decoder);
            return 1;
        }

        if (wavFile) {
            uint8_t placeholder[44] = {0};
            fwrite(placeholder, 1, 44, wavFile);
        }

        size_t bytesPerSample = audioInfo.bitsPerSample / 8;
        size_t bufferSize     = audioInfo.sampleRate * audioInfo.channelCount;
        uint8_t *pcmBuffer    = (uint8_t *)malloc(bufferSize * bytesPerSample);
        if (!pcmBuffer) {
            printf("fatal: Failed to allocate buffer!\n");
            if (wavFile)
                fclose(wavFile);
            if (pcmFile)
                fclose(pcmFile);
            picoAudioDecoderDestroy(decoder);
            return 1;
        }

        size_t totalSamples   = 0;
        uint32_t totalBytes   = 0;
        size_t samplesDecoded = 0;

        fflush(stdout);

        while (!picoAudioDecoderIsEOF(decoder)) {
            result = picoAudioDecoderDecode(decoder, pcmBuffer, bufferSize, &samplesDecoded);

            if (result == PICO_AUDIO_RESULT_ERROR_END_OF_FILE) {
                break;
            }

            if (result != PICO_AUDIO_RESULT_SUCCESS) {
                printf("\nfatal: %s\n", picoAudioResultToString(result));
                break;
            }

            if (samplesDecoded > 0) {
                size_t bytesDecoded = samplesDecoded * bytesPerSample;
                if (wavFile)
                    fwrite(pcmBuffer, 1, bytesDecoded, wavFile);
                if (pcmFile)
                    fwrite(pcmBuffer, 1, bytesDecoded, pcmFile);
                totalSamples += samplesDecoded;
                totalBytes += (uint32_t)bytesDecoded;
            }
        }

        if (wavFile) {
            fseek(wavFile, 0, SEEK_SET);
            writeWavHeader(wavFile,
                           audioInfo.sampleRate,
                           audioInfo.channelCount,
                           audioInfo.bitsPerSample,
                           totalBytes);
            fclose(wavFile);
        }
        if (pcmFile) {
            fclose(pcmFile);
        }

        free(pcmBuffer);

        printf("Total samples decoded: %zu\n", totalSamples);
        printf("Output size: %u bytes (%.2f MB)\n",
               totalBytes, totalBytes / (1024.0 * 1024.0));
        printf("Decoded duration: %.2f seconds\n", (double)totalSamples / (double)audioInfo.channelCount / (double)audioInfo.sampleRate);
    }

    picoAudioDecoderDestroy(decoder);

    printf("Goodbye, Pico!\n");
    return 0;
}
