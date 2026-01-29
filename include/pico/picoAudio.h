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
 * picoAudio - Cross-platform Audio decoding library using OS-native APIs
 *
 * This library provides a simple API to decode audio files to raw PCM data.
 * It uses platform-specific APIs for decoding:
 *   - Windows: Media Foundation (IMFSourceReader)
 *   - macOS: AudioToolbox (ExtAudioFile)
 *   - Linux: not supported yet
 *
 */

#ifndef PICO_AUDIO_H
#define PICO_AUDIO_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#ifndef PICO_MALLOC
#define PICO_MALLOC malloc
#define PICO_FREE   free
#endif

#ifndef PICO_AUDIO_LOG
#define PICO_AUDIO_LOG(...) \
    do {                    \
        (void)0;            \
    } while (0)
#endif

typedef enum {
    PICO_AUDIO_RESULT_SUCCESS = 0,
    PICO_AUDIO_RESULT_ERROR_INVALID_ARGUMENT,
    PICO_AUDIO_RESULT_ERROR_FILE_NOT_FOUND,
    PICO_AUDIO_RESULT_ERROR_UNSUPPORTED_FORMAT,
    PICO_AUDIO_RESULT_ERROR_DECODER_INIT_FAILED,
    PICO_AUDIO_RESULT_ERROR_DECODE_FAILED,
    PICO_AUDIO_RESULT_ERROR_END_OF_FILE,
    PICO_AUDIO_RESULT_ERROR_MEMORY,
    PICO_AUDIO_RESULT_ERROR_UNSUPPORTED_PLATFORM,
    PICO_AUDIO_RESULT_ERROR_NOT_OPENED,
    PICO_AUDIO_RESULT_ERROR_SEEK_FAILED,
    PICO_AUDIO_RESULT_ERROR_UNKNOWN,
    PICO_AUDIO_RESULT_COUNT
} picoAudioResult;

typedef struct {
    // sample rate in Hertz
    uint32_t sampleRate;

    // number of audio channels, 1 for mono, 2 for stereo
    uint16_t channelCount;

    // bits per sample in output PCM data
    uint16_t bitsPerSample;

    // sample count
    uint64_t totalSamples;

    // duration in seconds (can also be inferred from totalSamples / sampleRate)
    double durationSeconds;
} picoAudioInfo_t;
typedef picoAudioInfo_t *picoAudioInfo;

typedef struct picoAudioDecoder_t picoAudioDecoder_t;
typedef picoAudioDecoder_t *picoAudioDecoder;

picoAudioDecoder picoAudioDecoderCreate(void);
void picoAudioDecoderDestroy(picoAudioDecoder decoder);

picoAudioResult picoAudioDecoderOpenFile(picoAudioDecoder decoder, const char *filePath);
picoAudioResult picoAudioDecoderOpenBuffer(picoAudioDecoder decoder, const uint8_t *buffer, size_t size);
picoAudioResult picoAudioDecoderGetAudioInfo(picoAudioDecoder decoder, picoAudioInfo info);
picoAudioResult picoAudioDecoderDecode(picoAudioDecoder decoder, int16_t *pcmBuffer, size_t maxSamples, size_t *samplesDecoded);
picoAudioResult picoAudioDecoderSeek(picoAudioDecoder decoder, uint64_t samplePosition);
bool picoAudioDecoderIsEOF(picoAudioDecoder decoder);

const char *picoAudioResultToString(picoAudioResult result);

#ifdef PICO_IMPLEMENTATION

// ----------- MFC based windows implementation -----------------

#if defined(_WIN32) || defined(_WIN64)

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <mfapi.h>
#include <mferror.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <propvarutil.h>
#include <windows.h>

// auto link necessary libraries so users dont have to manually link them
#pragma comment(lib, "mf.lib")
#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mfreadwrite.lib")
#pragma comment(lib, "mfuuid.lib")
#pragma comment(lib, "propsys.lib")
#pragma comment(lib, "shlwapi.lib")

#include <shlwapi.h>

struct picoAudioDecoder_t {
    IMFSourceReader *sourceReader;
    picoAudioInfo_t audioInfo;
    bool isOpened;
    bool isEOF;
    bool mfInitialized;
};

picoAudioDecoder picoAudioDecoderCreate(void)
{
    picoAudioDecoder decoder = (picoAudioDecoder)PICO_MALLOC(sizeof(picoAudioDecoder_t));
    if (!decoder) {
        return NULL;
    }
    memset(decoder, 0, sizeof(picoAudioDecoder_t));

    HRESULT hr = MFStartup(MF_VERSION, MFSTARTUP_NOSOCKET);
    if (FAILED(hr)) {
        PICO_AUDIO_LOG("Failed to initialize Media Foundation: 0x%08lx\n", hr);
        PICO_FREE(decoder);
        return NULL;
    }
    decoder->mfInitialized = true;

    return decoder;
}

void picoAudioDecoderDestroy(picoAudioDecoder decoder)
{
    if (!decoder) {
        return;
    }

    if (decoder->sourceReader) {
        decoder->sourceReader->lpVtbl->Release(decoder->sourceReader);
        decoder->sourceReader = NULL;
    }

    if (decoder->mfInitialized) {
        MFShutdown();
    }

    PICO_FREE(decoder);
}

static picoAudioResult __picoAudioConfigureSourceReader(picoAudioDecoder decoder)
{
    IMFMediaType *partialType = NULL;
    HRESULT hr                = MFCreateMediaType(&partialType);
    if (FAILED(hr)) {
        decoder->sourceReader->lpVtbl->Release(decoder->sourceReader);
        decoder->sourceReader = NULL;
        return PICO_AUDIO_RESULT_ERROR_DECODER_INIT_FAILED;
    }

    hr = partialType->lpVtbl->SetGUID(partialType, &MF_MT_MAJOR_TYPE, &MFMediaType_Audio);
    if (SUCCEEDED(hr)) {
        hr = partialType->lpVtbl->SetGUID(partialType, &MF_MT_SUBTYPE, &MFAudioFormat_PCM);
    }

    if (SUCCEEDED(hr)) {
        hr = decoder->sourceReader->lpVtbl->SetCurrentMediaType(
            decoder->sourceReader,
            (DWORD)MF_SOURCE_READER_FIRST_AUDIO_STREAM,
            NULL,
            partialType);
    }

    partialType->lpVtbl->Release(partialType);

    if (FAILED(hr)) {
        PICO_AUDIO_LOG("Failed to set PCM output format: 0x%08lx\n", hr);
        decoder->sourceReader->lpVtbl->Release(decoder->sourceReader);
        decoder->sourceReader = NULL;
        return PICO_AUDIO_RESULT_ERROR_UNSUPPORTED_FORMAT;
    }

    IMFMediaType *outputType = NULL;
    hr                       = decoder->sourceReader->lpVtbl->GetCurrentMediaType(
        decoder->sourceReader,
        (DWORD)MF_SOURCE_READER_FIRST_AUDIO_STREAM,
        &outputType);

    if (FAILED(hr)) {
        decoder->sourceReader->lpVtbl->Release(decoder->sourceReader);
        decoder->sourceReader = NULL;
        return PICO_AUDIO_RESULT_ERROR_DECODER_INIT_FAILED;
    }

    UINT32 sampleRate = 0, channelCount = 0, bitsPerSample = 0;
    outputType->lpVtbl->GetUINT32(outputType, &MF_MT_AUDIO_SAMPLES_PER_SECOND, &sampleRate);
    outputType->lpVtbl->GetUINT32(outputType, &MF_MT_AUDIO_NUM_CHANNELS, &channelCount);
    outputType->lpVtbl->GetUINT32(outputType, &MF_MT_AUDIO_BITS_PER_SAMPLE, &bitsPerSample);
    outputType->lpVtbl->Release(outputType);

    decoder->audioInfo.sampleRate    = sampleRate;
    decoder->audioInfo.channelCount  = (uint16_t)channelCount;
    decoder->audioInfo.bitsPerSample = (uint16_t)bitsPerSample;

    PROPVARIANT duration;
    PropVariantInit(&duration);
    hr = decoder->sourceReader->lpVtbl->GetPresentationAttribute(
        decoder->sourceReader, (DWORD)MF_SOURCE_READER_MEDIASOURCE, &MF_PD_DURATION, &duration);

    if (SUCCEEDED(hr) && duration.vt == VT_UI8) {
        decoder->audioInfo.durationSeconds = (double)duration.uhVal.QuadPart / 10000000.0;
        decoder->audioInfo.totalSamples    = (uint64_t)(decoder->audioInfo.durationSeconds * sampleRate);
    }
    PropVariantClear(&duration);

    decoder->isOpened = true;
    decoder->isEOF    = false;

    return PICO_AUDIO_RESULT_SUCCESS;
}

picoAudioResult picoAudioDecoderOpenFile(picoAudioDecoder decoder, const char *filePath)
{
    if (!decoder || !filePath) {
        return PICO_AUDIO_RESULT_ERROR_INVALID_ARGUMENT;
    }

    if (decoder->isOpened) {
        if (decoder->sourceReader) {
            decoder->sourceReader->lpVtbl->Release(decoder->sourceReader);
            decoder->sourceReader = NULL;
        }
        decoder->isOpened = false;
        decoder->isEOF    = false;
    }

    int wideLen = MultiByteToWideChar(CP_UTF8, 0, filePath, -1, NULL, 0);
    if (wideLen <= 0) {
        PICO_AUDIO_LOG("Failed to convert file path to wide string\n");
        return PICO_AUDIO_RESULT_ERROR_INVALID_ARGUMENT;
    }

    WCHAR *wideFilePath = (WCHAR *)PICO_MALLOC(wideLen * sizeof(WCHAR));
    if (!wideFilePath) {
        return PICO_AUDIO_RESULT_ERROR_MEMORY;
    }
    MultiByteToWideChar(CP_UTF8, 0, filePath, -1, wideFilePath, wideLen);

    DWORD fileAttribs = GetFileAttributesW(wideFilePath);
    if (fileAttribs == INVALID_FILE_ATTRIBUTES) {
        PICO_FREE(wideFilePath);
        return PICO_AUDIO_RESULT_ERROR_FILE_NOT_FOUND;
    }

    IMFAttributes *attributes = NULL;
    HRESULT hr                = MFCreateAttributes(&attributes, 1);
    if (FAILED(hr)) {
        PICO_FREE(wideFilePath);
        return PICO_AUDIO_RESULT_ERROR_DECODER_INIT_FAILED;
    }

    hr = MFCreateSourceReaderFromURL(wideFilePath, attributes, &decoder->sourceReader);
    attributes->lpVtbl->Release(attributes);
    PICO_FREE(wideFilePath);

    if (FAILED(hr)) {
        PICO_AUDIO_LOG("Failed to create source reader: 0x%08lx\n", hr);
        return PICO_AUDIO_RESULT_ERROR_DECODER_INIT_FAILED;
    }

    return __picoAudioConfigureSourceReader(decoder);
}

picoAudioResult picoAudioDecoderOpenBuffer(picoAudioDecoder decoder, const uint8_t *buffer, size_t size)
{
    if (!decoder || !buffer || size == 0) {
        return PICO_AUDIO_RESULT_ERROR_INVALID_ARGUMENT;
    }

    if (decoder->isOpened) {
        if (decoder->sourceReader) {
            decoder->sourceReader->lpVtbl->Release(decoder->sourceReader);
            decoder->sourceReader = NULL;
        }
        decoder->isOpened = false;
        decoder->isEOF    = false;
    }

    IStream *memStream = SHCreateMemStream(buffer, (UINT)size);
    if (!memStream) {
        PICO_AUDIO_LOG("Failed to create memory stream\n");
        return PICO_AUDIO_RESULT_ERROR_MEMORY;
    }

    IMFByteStream *byteStream = NULL;
    HRESULT hr                = MFCreateMFByteStreamOnStream(memStream, &byteStream);
    memStream->lpVtbl->Release(memStream);

    if (FAILED(hr)) {
        PICO_AUDIO_LOG("Failed to create MF byte stream: 0x%08lx\n", hr);
        return PICO_AUDIO_RESULT_ERROR_DECODER_INIT_FAILED;
    }

    IMFAttributes *attributes = NULL;
    hr                        = MFCreateAttributes(&attributes, 1);
    if (FAILED(hr)) {
        byteStream->lpVtbl->Release(byteStream);
        return PICO_AUDIO_RESULT_ERROR_DECODER_INIT_FAILED;
    }

    hr = MFCreateSourceReaderFromByteStream(byteStream, attributes, &decoder->sourceReader);
    attributes->lpVtbl->Release(attributes);
    byteStream->lpVtbl->Release(byteStream);

    if (FAILED(hr)) {
        PICO_AUDIO_LOG("Failed to create source reader from byte stream: 0x%08lx\n", hr);
        return PICO_AUDIO_RESULT_ERROR_DECODER_INIT_FAILED;
    }

    return __picoAudioConfigureSourceReader(decoder);
}

picoAudioResult picoAudioDecoderGetAudioInfo(picoAudioDecoder decoder, picoAudioInfo info)
{
    if (!decoder || !info) {
        return PICO_AUDIO_RESULT_ERROR_INVALID_ARGUMENT;
    }

    if (!decoder->isOpened) {
        return PICO_AUDIO_RESULT_ERROR_NOT_OPENED;
    }

    memcpy(info, &decoder->audioInfo, sizeof(picoAudioInfo_t));
    return PICO_AUDIO_RESULT_SUCCESS;
}

picoAudioResult picoAudioDecoderDecode(picoAudioDecoder decoder, int16_t *pcmBuffer, size_t maxSamples, size_t *samplesDecoded)
{
    if (!decoder || !pcmBuffer || !samplesDecoded) {
        return PICO_AUDIO_RESULT_ERROR_INVALID_ARGUMENT;
    }

    if (!decoder->isOpened) {
        return PICO_AUDIO_RESULT_ERROR_NOT_OPENED;
    }

    if (decoder->isEOF) {
        *samplesDecoded = 0;
        return PICO_AUDIO_RESULT_ERROR_END_OF_FILE;
    }

    *samplesDecoded     = 0;
    size_t bytesNeeded  = maxSamples * sizeof(int16_t);
    size_t bytesWritten = 0;

    while (bytesWritten < bytesNeeded) {
        DWORD flags       = 0;
        IMFSample *sample = NULL;

        HRESULT hr = decoder->sourceReader->lpVtbl->ReadSample(
            decoder->sourceReader,
            (DWORD)MF_SOURCE_READER_FIRST_AUDIO_STREAM,
            0,
            NULL,
            &flags,
            NULL,
            &sample);

        if (FAILED(hr)) {
            PICO_AUDIO_LOG("ReadSample failed: 0x%08lx\n", hr);
            return PICO_AUDIO_RESULT_ERROR_DECODE_FAILED;
        }

        if (flags & MF_SOURCE_READERF_ENDOFSTREAM) {
            decoder->isEOF = true;
            if (sample) {
                sample->lpVtbl->Release(sample);
            }
            break;
        }

        if (!sample) {
            continue;
        }

        IMFMediaBuffer *mediaBuffer = NULL;
        hr                          = sample->lpVtbl->ConvertToContiguousBuffer(sample, &mediaBuffer);

        if (SUCCEEDED(hr)) {
            BYTE *audioData       = NULL;
            DWORD audioDataLength = 0;

            hr = mediaBuffer->lpVtbl->Lock(mediaBuffer, &audioData, NULL, &audioDataLength);
            if (SUCCEEDED(hr)) {
                size_t bytesToCopy = audioDataLength;
                if (bytesWritten + bytesToCopy > bytesNeeded) {
                    bytesToCopy = bytesNeeded - bytesWritten;
                }

                memcpy((uint8_t *)pcmBuffer + bytesWritten, audioData, bytesToCopy);
                bytesWritten += bytesToCopy;

                mediaBuffer->lpVtbl->Unlock(mediaBuffer);
            }

            mediaBuffer->lpVtbl->Release(mediaBuffer);
        }

        sample->lpVtbl->Release(sample);
    }

    *samplesDecoded = bytesWritten / sizeof(int16_t);

    if (*samplesDecoded == 0 && decoder->isEOF) {
        return PICO_AUDIO_RESULT_ERROR_END_OF_FILE;
    }

    return PICO_AUDIO_RESULT_SUCCESS;
}

picoAudioResult picoAudioDecoderSeek(picoAudioDecoder decoder, uint64_t samplePosition)
{
    if (!decoder) {
        return PICO_AUDIO_RESULT_ERROR_INVALID_ARGUMENT;
    }

    if (!decoder->isOpened) {
        return PICO_AUDIO_RESULT_ERROR_NOT_OPENED;
    }

    LONGLONG position = (LONGLONG)((double)samplePosition / decoder->audioInfo.sampleRate * 10000000.0);

    PROPVARIANT posVar;
    PropVariantInit(&posVar);
    posVar.vt            = VT_I8;
    posVar.hVal.QuadPart = position;

    HRESULT hr = decoder->sourceReader->lpVtbl->SetCurrentPosition(decoder->sourceReader, &GUID_NULL, &posVar);

    PropVariantClear(&posVar);

    if (FAILED(hr)) {
        PICO_AUDIO_LOG("Seek failed: 0x%08lx\n", hr);
        return PICO_AUDIO_RESULT_ERROR_SEEK_FAILED;
    }

    decoder->isEOF = false;
    return PICO_AUDIO_RESULT_SUCCESS;
}

bool picoAudioDecoderIsEOF(picoAudioDecoder decoder)
{
    if (!decoder) {
        return true;
    }
    return decoder->isEOF;
}

//  -------------- AudioToolbox macOS implementation -----------------

#elif defined(__APPLE__)

#include <AudioToolbox/AudioToolbox.h>

struct picoAudioDecoder_t {
    ExtAudioFileRef audioFile;
    AudioFileID audioFileID;
    picoAudioInfo_t audioInfo;
    AudioStreamBasicDescription outputFormat;
    bool isOpened;
    bool isEOF;
    bool fromBuffer;

    const uint8_t *bufferData;
    size_t bufferSize;
    size_t bufferPosition;
};

picoAudioDecoder picoAudioDecoderCreate(void)
{
    picoAudioDecoder decoder = (picoAudioDecoder)PICO_MALLOC(sizeof(picoAudioDecoder_t));
    if (!decoder) {
        return NULL;
    }
    memset(decoder, 0, sizeof(picoAudioDecoder_t));
    return decoder;
}

void picoAudioDecoderDestroy(picoAudioDecoder decoder)
{
    if (!decoder) {
        return;
    }

    if (decoder->audioFile) {
        ExtAudioFileDispose(decoder->audioFile);
        decoder->audioFile = NULL;
    }

    if (decoder->audioFileID) {
        AudioFileClose(decoder->audioFileID);
        decoder->audioFileID = NULL;
    }

    PICO_FREE(decoder);
}

picoAudioResult picoAudioDecoderOpenFile(picoAudioDecoder decoder, const char *filePath)
{
    if (!decoder || !filePath) {
        return PICO_AUDIO_RESULT_ERROR_INVALID_ARGUMENT;
    }

    if (decoder->isOpened) {
        if (decoder->audioFile) {
            ExtAudioFileDispose(decoder->audioFile);
            decoder->audioFile = NULL;
        }
        decoder->isOpened = false;
        decoder->isEOF    = false;
    }

    CFStringRef pathString = CFStringCreateWithCString(kCFAllocatorDefault, filePath, kCFStringEncodingUTF8);
    if (!pathString) {
        return PICO_AUDIO_RESULT_ERROR_MEMORY;
    }

    CFURLRef fileURL = CFURLCreateWithFileSystemPath(kCFAllocatorDefault, pathString, kCFURLPOSIXPathStyle, false);
    CFRelease(pathString);

    if (!fileURL) {
        return PICO_AUDIO_RESULT_ERROR_INVALID_ARGUMENT;
    }

    OSStatus status = ExtAudioFileOpenURL(fileURL, &decoder->audioFile);
    CFRelease(fileURL);

    if (status != noErr) {
        PICO_AUDIO_LOG("Failed to open audio file: %d\n", (int)status);
        if (status == fnfErr || status == -43) {
            return PICO_AUDIO_RESULT_ERROR_FILE_NOT_FOUND;
        }
        return PICO_AUDIO_RESULT_ERROR_DECODER_INIT_FAILED;
    }

    AudioStreamBasicDescription inputFormat;
    UInt32 size = sizeof(inputFormat);
    status      = ExtAudioFileGetProperty(decoder->audioFile, kExtAudioFileProperty_FileDataFormat, &size, &inputFormat);

    if (status != noErr) {
        ExtAudioFileDispose(decoder->audioFile);
        decoder->audioFile = NULL;
        return PICO_AUDIO_RESULT_ERROR_DECODER_INIT_FAILED;
    }

    decoder->outputFormat.mSampleRate       = inputFormat.mSampleRate;
    decoder->outputFormat.mFormatID         = kAudioFormatLinearPCM;
    decoder->outputFormat.mFormatFlags      = kAudioFormatFlagIsSignedInteger | kAudioFormatFlagIsPacked;
    decoder->outputFormat.mBitsPerChannel   = 16;
    decoder->outputFormat.mChannelsPerFrame = inputFormat.mChannelsPerFrame;
    decoder->outputFormat.mBytesPerFrame    = 2 * inputFormat.mChannelsPerFrame;
    decoder->outputFormat.mFramesPerPacket  = 1;
    decoder->outputFormat.mBytesPerPacket   = decoder->outputFormat.mBytesPerFrame;

    status = ExtAudioFileSetProperty(decoder->audioFile, kExtAudioFileProperty_ClientDataFormat,
                                     sizeof(decoder->outputFormat), &decoder->outputFormat);

    if (status != noErr) {
        PICO_AUDIO_LOG("Failed to set output format: %d\n", (int)status);
        ExtAudioFileDispose(decoder->audioFile);
        decoder->audioFile = NULL;
        return PICO_AUDIO_RESULT_ERROR_UNSUPPORTED_FORMAT;
    }

    SInt64 totalFrames = 0;
    size               = sizeof(totalFrames);
    status             = ExtAudioFileGetProperty(decoder->audioFile, kExtAudioFileProperty_FileLengthFrames, &size, &totalFrames);

    if (status != noErr) {
        totalFrames = 0;
    }

    decoder->audioInfo.sampleRate      = (uint32_t)decoder->outputFormat.mSampleRate;
    decoder->audioInfo.channelCount    = (uint16_t)decoder->outputFormat.mChannelsPerFrame;
    decoder->audioInfo.bitsPerSample   = 16;
    decoder->audioInfo.totalSamples    = (uint64_t)totalFrames * decoder->audioInfo.channelCount;
    decoder->audioInfo.durationSeconds = (double)totalFrames / decoder->audioInfo.sampleRate;

    decoder->isOpened = true;
    decoder->isEOF    = false;

    return PICO_AUDIO_RESULT_SUCCESS;
}

static OSStatus picoAudioReadProc(void *inClientData, SInt64 inPosition, UInt32 requestCount,
                                  void *buffer, UInt32 *actualCount)
{
    picoAudioDecoder decoder = (picoAudioDecoder)inClientData;
    if (!decoder || !decoder->bufferData) {
        return kAudioFileUnspecifiedError;
    }

    if (inPosition < 0 || (size_t)inPosition >= decoder->bufferSize) {
        *actualCount = 0;
        return kAudioFileEndOfFileError;
    }

    size_t available = decoder->bufferSize - (size_t)inPosition;
    size_t toRead    = (requestCount < available) ? requestCount : available;

    memcpy(buffer, decoder->bufferData + inPosition, toRead);
    *actualCount = (UInt32)toRead;

    return noErr;
}

static SInt64 picoAudioGetSizeProc(void *inClientData)
{
    picoAudioDecoder decoder = (picoAudioDecoder)inClientData;
    if (!decoder) {
        return 0;
    }
    return (SInt64)decoder->bufferSize;
}

picoAudioResult picoAudioDecoderOpenBuffer(picoAudioDecoder decoder, const uint8_t *buffer, size_t size)
{
    if (!decoder || !buffer || size == 0) {
        return PICO_AUDIO_RESULT_ERROR_INVALID_ARGUMENT;
    }

    if (decoder->isOpened) {
        if (decoder->audioFile) {
            ExtAudioFileDispose(decoder->audioFile);
            decoder->audioFile = NULL;
        }
        if (decoder->audioFileID) {
            AudioFileClose(decoder->audioFileID);
            decoder->audioFileID = NULL;
        }
        decoder->isOpened = false;
        decoder->isEOF    = false;
    }

    decoder->bufferData     = buffer;
    decoder->bufferSize     = size;
    decoder->bufferPosition = 0;
    decoder->fromBuffer     = true;

    AudioFileTypeID typeHints[] = {
        kAudioFileAAC_ADTSType,
        kAudioFileM4AType,
        kAudioFileMP3Type,
        kAudioFileCAFType,
        kAudioFileWAVEType,
        kAudioFileAIFFType,
        0                      
    };

    OSStatus status = -1;
    for (int i = 0; typeHints[i] != 0 && status != noErr; i++) {
        status = AudioFileOpenWithCallbacks(
            decoder,
            picoAudioReadProc,
            NULL,
            picoAudioGetSizeProc,
            NULL,
            typeHints[i],
            &decoder->audioFileID);
    }

    if (status != noErr) {
        status = AudioFileOpenWithCallbacks(
            decoder,
            picoAudioReadProc,
            NULL,
            picoAudioGetSizeProc,
            NULL,
            0,
            &decoder->audioFileID);
    }

    if (status != noErr) {
        PICO_AUDIO_LOG("Failed to open audio from buffer: %d\n", (int)status);
        decoder->bufferData = NULL;
        decoder->bufferSize = 0;
        decoder->fromBuffer = false;
        return PICO_AUDIO_RESULT_ERROR_UNSUPPORTED_FORMAT;
    }

    status = ExtAudioFileWrapAudioFileID(decoder->audioFileID, false, &decoder->audioFile);
    if (status != noErr) {
        PICO_AUDIO_LOG("Failed to wrap AudioFileID: %d\n", (int)status);
        AudioFileClose(decoder->audioFileID);
        decoder->audioFileID = NULL;
        decoder->bufferData  = NULL;
        decoder->bufferSize  = 0;
        decoder->fromBuffer  = false;
        return PICO_AUDIO_RESULT_ERROR_DECODER_INIT_FAILED;
    }

    AudioStreamBasicDescription inputFormat;
    UInt32 propSize = sizeof(inputFormat);
    status          = ExtAudioFileGetProperty(decoder->audioFile, kExtAudioFileProperty_FileDataFormat,
                                              &propSize, &inputFormat);
    if (status != noErr) {
        ExtAudioFileDispose(decoder->audioFile);
        AudioFileClose(decoder->audioFileID);
        decoder->audioFile   = NULL;
        decoder->audioFileID = NULL;
        decoder->bufferData  = NULL;
        decoder->bufferSize  = 0;
        decoder->fromBuffer  = false;
        return PICO_AUDIO_RESULT_ERROR_DECODER_INIT_FAILED;
    }

    decoder->outputFormat.mSampleRate       = inputFormat.mSampleRate;
    decoder->outputFormat.mFormatID         = kAudioFormatLinearPCM;
    decoder->outputFormat.mFormatFlags      = kAudioFormatFlagIsSignedInteger | kAudioFormatFlagIsPacked;
    decoder->outputFormat.mBitsPerChannel   = 16;
    decoder->outputFormat.mChannelsPerFrame = inputFormat.mChannelsPerFrame;
    decoder->outputFormat.mBytesPerFrame    = 2 * inputFormat.mChannelsPerFrame;
    decoder->outputFormat.mFramesPerPacket  = 1;
    decoder->outputFormat.mBytesPerPacket   = decoder->outputFormat.mBytesPerFrame;

    status = ExtAudioFileSetProperty(decoder->audioFile, kExtAudioFileProperty_ClientDataFormat,
                                     sizeof(decoder->outputFormat), &decoder->outputFormat);
    if (status != noErr) {
        PICO_AUDIO_LOG("Failed to set output format: %d\n", (int)status);
        ExtAudioFileDispose(decoder->audioFile);
        AudioFileClose(decoder->audioFileID);
        decoder->audioFile   = NULL;
        decoder->audioFileID = NULL;
        decoder->bufferData  = NULL;
        decoder->bufferSize  = 0;
        decoder->fromBuffer  = false;
        return PICO_AUDIO_RESULT_ERROR_UNSUPPORTED_FORMAT;
    }

    SInt64 totalFrames = 0;
    propSize           = sizeof(totalFrames);
    status             = ExtAudioFileGetProperty(decoder->audioFile, kExtAudioFileProperty_FileLengthFrames,
                                                 &propSize, &totalFrames);
    if (status != noErr) {
        totalFrames = 0;
    }

    decoder->audioInfo.sampleRate      = (uint32_t)decoder->outputFormat.mSampleRate;
    decoder->audioInfo.channelCount    = (uint16_t)decoder->outputFormat.mChannelsPerFrame;
    decoder->audioInfo.bitsPerSample   = 16;
    decoder->audioInfo.totalSamples    = (uint64_t)totalFrames * decoder->audioInfo.channelCount;
    decoder->audioInfo.durationSeconds = (double)totalFrames / decoder->audioInfo.sampleRate;

    decoder->isOpened = true;
    decoder->isEOF    = false;

    return PICO_AUDIO_RESULT_SUCCESS;
}

picoAudioResult picoAudioDecoderGetAudioInfo(picoAudioDecoder decoder, picoAudioInfo info)
{
    if (!decoder || !info) {
        return PICO_AUDIO_RESULT_ERROR_INVALID_ARGUMENT;
    }

    if (!decoder->isOpened) {
        return PICO_AUDIO_RESULT_ERROR_NOT_OPENED;
    }

    memcpy(info, &decoder->audioInfo, sizeof(picoAudioInfo_t));
    return PICO_AUDIO_RESULT_SUCCESS;
}

picoAudioResult picoAudioDecoderDecode(picoAudioDecoder decoder, int16_t *pcmBuffer,
                                       size_t maxSamples, size_t *samplesDecoded)
{
    if (!decoder || !pcmBuffer || !samplesDecoded) {
        return PICO_AUDIO_RESULT_ERROR_INVALID_ARGUMENT;
    }

    if (!decoder->isOpened) {
        return PICO_AUDIO_RESULT_ERROR_NOT_OPENED;
    }

    if (decoder->isEOF) {
        *samplesDecoded = 0;
        return PICO_AUDIO_RESULT_ERROR_END_OF_FILE;
    }

    UInt32 framesToRead = (UInt32)(maxSamples / decoder->audioInfo.channelCount);

    AudioBufferList bufferList;
    bufferList.mNumberBuffers              = 1;
    bufferList.mBuffers[0].mNumberChannels = decoder->audioInfo.channelCount;
    bufferList.mBuffers[0].mDataByteSize   = framesToRead * decoder->outputFormat.mBytesPerFrame;
    bufferList.mBuffers[0].mData           = pcmBuffer;

    UInt32 framesRead = framesToRead;
    OSStatus status   = ExtAudioFileRead(decoder->audioFile, &framesRead, &bufferList);

    if (status != noErr) {
        PICO_AUDIO_LOG("ExtAudioFileRead failed: %d\n", (int)status);
        return PICO_AUDIO_RESULT_ERROR_DECODE_FAILED;
    }

    *samplesDecoded = framesRead * decoder->audioInfo.channelCount;

    if (framesRead == 0) {
        decoder->isEOF = true;
        return PICO_AUDIO_RESULT_ERROR_END_OF_FILE;
    }

    return PICO_AUDIO_RESULT_SUCCESS;
}

picoAudioResult picoAudioDecoderSeek(picoAudioDecoder decoder, uint64_t samplePosition)
{
    if (!decoder) {
        return PICO_AUDIO_RESULT_ERROR_INVALID_ARGUMENT;
    }

    if (!decoder->isOpened) {
        return PICO_AUDIO_RESULT_ERROR_NOT_OPENED;
    }

    SInt64 framePosition = (SInt64)(samplePosition / decoder->audioInfo.channelCount);

    OSStatus status = ExtAudioFileSeek(decoder->audioFile, framePosition);

    if (status != noErr) {
        PICO_AUDIO_LOG("Seek failed: %d\n", (int)status);
        return PICO_AUDIO_RESULT_ERROR_SEEK_FAILED;
    }

    decoder->isEOF = false;
    return PICO_AUDIO_RESULT_SUCCESS;
}

bool picoAudioDecoderIsEOF(picoAudioDecoder decoder)
{
    if (!decoder) {
        return true;
    }
    return decoder->isEOF;
}

// ----------------- Linux --------------------------------------

#else

// for now this is just a place holder, I do plan to
// later implement this via libfaad2 in the future

struct picoAudioDecoder_t {
    bool isOpened;
    bool isEOF;
    picoAudioInfo_t audioInfo;
};

picoAudioDecoder picoAudioDecoderCreate(void)
{
    PICO_AUDIO_LOG("picoAudio: Platform not supported. Returning stub decoder.\n");
    picoAudioDecoder decoder = (picoAudioDecoder)PICO_MALLOC(sizeof(picoAudioDecoder_t));
    if (!decoder) {
        return NULL;
    }
    memset(decoder, 0, sizeof(picoAudioDecoder_t));
    return decoder;
}

void picoAudioDecoderDestroy(picoAudioDecoder decoder)
{
    if (decoder) {
        PICO_FREE(decoder);
    }
}

picoAudioResult picoAudioDecoderOpenFile(picoAudioDecoder decoder, const char *filePath)
{
    (void)decoder;
    (void)filePath;
    return PICO_AUDIO_RESULT_ERROR_UNSUPPORTED_PLATFORM;
}

picoAudioResult picoAudioDecoderOpenBuffer(picoAudioDecoder decoder, const uint8_t *buffer, size_t size)
{
    (void)decoder;
    (void)buffer;
    (void)size;
    return PICO_AUDIO_RESULT_ERROR_UNSUPPORTED_PLATFORM;
}

picoAudioResult picoAudioDecoderGetAudioInfo(picoAudioDecoder decoder, picoAudioInfo info)
{
    (void)decoder;
    (void)info;
    return PICO_AUDIO_RESULT_ERROR_UNSUPPORTED_PLATFORM;
}

picoAudioResult picoAudioDecoderDecode(picoAudioDecoder decoder, int16_t *pcmBuffer,
                                       size_t maxSamples, size_t *samplesDecoded)
{
    (void)decoder;
    (void)pcmBuffer;
    (void)maxSamples;
    (void)samplesDecoded;
    return PICO_AUDIO_RESULT_ERROR_UNSUPPORTED_PLATFORM;
}

picoAudioResult picoAudioDecoderSeek(picoAudioDecoder decoder, uint64_t samplePosition)
{
    (void)decoder;
    (void)samplePosition;
    return PICO_AUDIO_RESULT_ERROR_UNSUPPORTED_PLATFORM;
}

bool picoAudioDecoderIsEOF(picoAudioDecoder decoder)
{
    (void)decoder;
    return true;
}

#endif

const char *picoAudioResultToString(picoAudioResult result)
{
    switch (result) {
        case PICO_AUDIO_RESULT_SUCCESS:
            return "PICO_AUDIO_RESULT_SUCCESS";
        case PICO_AUDIO_RESULT_ERROR_INVALID_ARGUMENT:
            return "PICO_AUDIO_RESULT_ERROR_INVALID_ARGUMENT";
        case PICO_AUDIO_RESULT_ERROR_FILE_NOT_FOUND:
            return "PICO_AUDIO_RESULT_ERROR_FILE_NOT_FOUND";
        case PICO_AUDIO_RESULT_ERROR_UNSUPPORTED_FORMAT:
            return "PICO_AUDIO_RESULT_ERROR_UNSUPPORTED_FORMAT";
        case PICO_AUDIO_RESULT_ERROR_DECODER_INIT_FAILED:
            return "PICO_AUDIO_RESULT_ERROR_DECODER_INIT_FAILED";
        case PICO_AUDIO_RESULT_ERROR_DECODE_FAILED:
            return "PICO_AUDIO_RESULT_ERROR_DECODE_FAILED";
        case PICO_AUDIO_RESULT_ERROR_END_OF_FILE:
            return "PICO_AUDIO_RESULT_ERROR_END_OF_FILE";
        case PICO_AUDIO_RESULT_ERROR_MEMORY:
            return "PICO_AUDIO_RESULT_ERROR_MEMORY";
        case PICO_AUDIO_RESULT_ERROR_UNSUPPORTED_PLATFORM:
            return "PICO_AUDIO_RESULT_ERROR_UNSUPPORTED_PLATFORM";
        case PICO_AUDIO_RESULT_ERROR_NOT_OPENED:
            return "PICO_AUDIO_RESULT_ERROR_NOT_OPENED";
        case PICO_AUDIO_RESULT_ERROR_SEEK_FAILED:
            return "PICO_AUDIO_RESULT_ERROR_SEEK_FAILED";
        case PICO_AUDIO_RESULT_ERROR_UNKNOWN:
            return "PICO_AUDIO_RESULT_ERROR_UNKNOWN";
        default:
            return "PICO_AUDIO_RESULT_UNKNOWN";
    }
}

#endif // PICO_IMPLEMENTATION

#endif // PICO_AUDIO_H
