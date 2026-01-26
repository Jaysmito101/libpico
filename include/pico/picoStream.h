/*
MIT License

Copyright (c) 2025 Jaysmito Mukherjee (jaysmito101@gmail.com)

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

#ifndef PICO_STREAM_H
#define PICO_STREAM_H

#ifndef PICO_MALLOC
#define PICO_MALLOC(sz) malloc(sz)
#define PICO_FREE(ptr)  free(ptr)
#endif

#ifndef PICO_STREAM_ENABLE_MAPPED
#if defined(_WIN32) || defined(__unix__) || defined(__APPLE__)
#define PICO_STREAM_ENABLE_MAPPED 1
#else
#define PICO_STREAM_ENABLE_MAPPED 0
#endif
#endif

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef struct picoStream_t picoStream_t;
typedef picoStream_t *picoStream;

typedef enum {
    PICO_STREAM_SEEK_SET = 0,
    PICO_STREAM_SEEK_CUR = 1,
    PICO_STREAM_SEEK_END = 2
} picoStreamSeekOrigin;

typedef enum picoStreamSourceType {
    PICO_STREAM_SOURCE_TYPE_CUSTOM,
    PICO_STREAM_SOURCE_TYPE_FILE,
    PICO_STREAM_SOURCE_TYPE_MEMORY,
#if PICO_STREAM_ENABLE_MAPPED
    PICO_STREAM_SOURCE_TYPE_MAPPED
#endif
} picoStreamSourceType;

typedef struct {
    void *userData;
    size_t (*read)(void *userData, void *buffer, size_t size);
    size_t (*write)(void *userData, const void *buffer, size_t size);
    int (*seek)(void *userData, int64_t offset, picoStreamSeekOrigin origin);
    int64_t (*tell)(void *userData);
    void (*flush)(void *userData);
    void (*destroy)(void *userData);
} picoStreamCustom_t;
typedef picoStreamCustom_t *picoStreamCustom;

picoStream picoStreamFromCustom(picoStreamCustom customStream, bool canRead, bool canWrite);
picoStream picoStreamFromFile(FILE *file, bool canRead, bool canWrite, bool ownFileHandle);
picoStream picoStreamFromFilePath(const char *filePath, bool canRead, bool canWrite);
picoStream picoStreamFromMemory(void *buffer, size_t size, bool canRead, bool canWrite, bool ownMemory);
#if PICO_STREAM_ENABLE_MAPPED
// read only memory mapped file
picoStream picoStreamFromFileMapped(const char *filePath);  
#endif
void picoStreamDestroy(picoStream stream);

size_t picoStreamRead(picoStream stream, void *buffer, size_t size);
size_t picoStreamWrite(picoStream stream, const void *buffer, size_t size);
int picoStreamSeek(picoStream stream, int64_t offset, picoStreamSeekOrigin origin);
int64_t picoStreamTell(picoStream stream);
bool picoStreamCanRead(picoStream stream);
bool picoStreamCanWrite(picoStream stream);

void picoStreamFlush(picoStream stream);
void picoStreamSetEndianess(picoStream stream, bool littleEndian);

void picoStreamReset(picoStream stream);

void* picoStreamGetUserData(picoStream stream);

uint8_t picoStreamReadU8(picoStream stream);
uint16_t picoStreamReadU16(picoStream stream);
uint32_t picoStreamReadU32(picoStream stream);
uint64_t picoStreamReadU64(picoStream stream);
int8_t picoStreamReadS8(picoStream stream);
int16_t picoStreamReadS16(picoStream stream);
int32_t picoStreamReadS32(picoStream stream);
int64_t picoStreamReadS64(picoStream stream);
float picoStreamReadF32(picoStream stream);
double picoStreamReadF64(picoStream stream);

void picoStreamWriteU8(picoStream stream, uint8_t value);
void picoStreamWriteU16(picoStream stream, uint16_t value);
void picoStreamWriteU32(picoStream stream, uint32_t value);
void picoStreamWriteU64(picoStream stream, uint64_t value);
void picoStreamWriteS8(picoStream stream, int8_t value);
void picoStreamWriteS16(picoStream stream, int16_t value);
void picoStreamWriteS32(picoStream stream, int32_t value);
void picoStreamWriteS64(picoStream stream, int64_t value);
void picoStreamWriteF32(picoStream stream, float value);
void picoStreamWriteF64(picoStream stream, double value);

// Read/Write null-terminated string. maxLength includes the null terminator.
size_t picoStreamReadString(picoStream stream, char *buffer, size_t maxLength);
void picoStreamWriteString(picoStream stream, const char *string);

size_t picoStreamReadLine(picoStream stream, char *buffer, size_t maxLength);
void picoStreamWriteLine(picoStream stream, const char *string);


bool picoStreamIsSystemLittleEndian(void);

#if defined(PICO_IMPLEMENTATION) && !defined(PICO_STREAM_IMPLEMENTATION)
#define PICO_STREAM_IMPLEMENTATION
#endif

#ifdef PICO_STREAM_IMPLEMENTATION

#if PICO_STREAM_ENABLE_MAPPED
#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#else
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#endif
#endif // PICO_STREAM_ENABLE_MAPPED


#define __PICO_STREAM_READ_IMPL(typeName, type)        \
    type picoStreamRead##typeName(picoStream stream)            \
    {                                                            \
        type value = (type)0;                                         \
        __picoStreamReadEndianess(stream, &value, sizeof(value)); \
        return value;                                           \
    }

#define __PICO_STREAM_WRITE_IMPL(typeName, type)       \
    void picoStreamWrite##typeName(picoStream stream, type value) \
    {                                                            \
        __picoStreamWriteEndianess(stream, &value, sizeof(value)); \
    }


typedef union {
    picoStreamCustom custom;
    FILE *file;
    struct {
        uint8_t *buffer;
        size_t size;
        size_t position;
    } memory;
#if PICO_STREAM_ENABLE_MAPPED
    struct {
        void *base;
        size_t size;
        size_t position;
#ifdef _WIN32
        void *fileHandle;
        void *mappingHandle;
#else
        int fd;
#endif
    } mapped;
#endif
} picoStreamSource_t;
typedef picoStreamSource_t *picoStreamSource;

typedef union {
    uint16_t u16;
    uint32_t u32;
    uint64_t u64;
    int16_t s16;
    int32_t s32;
    int64_t s64;
    float f32;
    double f64;
    uint8_t bytes[8];
} picoStreamEndianessConverter;

struct picoStream_t {
    picoStreamSource_t source;
    picoStreamSourceType type;

    bool canRead;
    bool canWrite;

    bool littleEndian;

    bool ownsMemory;
    bool ownsFile;
};


static void __picoStreamReadEndianess(picoStream stream, void *outValue, size_t size)
{
    if (!stream || !outValue || size == 0 || !stream->canRead) {
        return;
    }

    uint8_t buffer[8] = {0};
    size_t bytesRead  = picoStreamRead(stream, buffer, size);
    if (bytesRead != size) {
        memset(outValue, 0, size);
        return;
    }

    if (stream->littleEndian == picoStreamIsSystemLittleEndian()) {
        memcpy(outValue, buffer, size);
    } else {
        for (size_t i = 0; i < size; i++) {
            ((uint8_t *)outValue)[i] = buffer[size - 1 - i];
        }
    }
}

static void __picoStreamWriteEndianess(picoStream stream, const void *value, size_t size)
{
    if (!stream || !value || size == 0 || !stream->canWrite) {
        return;
    }

    uint8_t buffer[8] = {0};
    if (stream->littleEndian == picoStreamIsSystemLittleEndian()) {
        memcpy(buffer, value, size);
    } else {
        for (size_t i = 0; i < size; i++) {
            buffer[i] = ((const uint8_t *)value)[size - 1 - i];
        }
    }

    picoStreamWrite(stream, buffer, size);
}

picoStream picoStreamFromCustom(picoStreamCustom customStream, bool canRead, bool canWrite)
{
    if (!customStream || (!canRead && !canWrite)) {
        return NULL;
    }

    picoStream stream = (picoStream)PICO_MALLOC(sizeof(picoStream_t));
    if (!stream) {
        return NULL;
    }

    stream->source.custom = customStream;
    stream->type          = PICO_STREAM_SOURCE_TYPE_CUSTOM;
    stream->canRead       = canRead;
    stream->canWrite      = canWrite;
    stream->littleEndian  = true;
    stream->ownsMemory    = false;
    stream->ownsFile      = false;

    return stream;
}

picoStream picoStreamFromFile(FILE *file, bool canRead, bool canWrite, bool ownFileHandle)
{
    if (!file || (!canRead && !canWrite)) {
        return NULL;
    }

    picoStream stream = (picoStream)PICO_MALLOC(sizeof(picoStream_t));
    if (!stream) {
        return NULL;
    }

    stream->source.file  = file;
    stream->type         = PICO_STREAM_SOURCE_TYPE_FILE;
    stream->canRead      = canRead;
    stream->canWrite     = canWrite;
    stream->littleEndian = true;
    stream->ownsMemory   = false;
    stream->ownsFile     = ownFileHandle;

    return stream;
}

picoStream picoStreamFromFilePath(const char *filePath, bool canRead, bool canWrite)
{
    if (!filePath || (!canRead && !canWrite)) {
        return NULL;
    }

    const char *mode;
    if (canRead && canWrite) {
        mode = "wb+";
    } else if (canRead) {
        mode = "rb";
    } else if (canWrite) {
        mode = "wb+";
    } else {
        return NULL;
    }

    FILE *file = fopen(filePath, mode);
    if (!file) {
        return NULL;
    }

    picoStream stream = picoStreamFromFile(file, canRead, canWrite, true);
    if (!stream) {
        fclose(file);
        return NULL;
    }

    return stream;
}

picoStream picoStreamFromMemory(void *buffer, size_t size, bool canRead, bool canWrite, bool ownMemory)
{
    if ((!buffer && size > 0) || size == 0 || (!canRead && !canWrite)) {
        return NULL;
    }

    picoStream stream = (picoStream)PICO_MALLOC(sizeof(picoStream_t));
    if (!stream) {
        return NULL;
    }

    stream->source.memory.buffer   = (uint8_t *)buffer;
    stream->source.memory.size     = size;
    stream->source.memory.position = 0;
    stream->type                   = PICO_STREAM_SOURCE_TYPE_MEMORY;
    stream->canRead                = canRead;
    stream->canWrite               = canWrite;
    stream->littleEndian           = true;
    stream->ownsMemory             = ownMemory;
    stream->ownsFile               = false;

    return stream;
}

#if PICO_STREAM_ENABLE_MAPPED
picoStream picoStreamFromFileMapped(const char *filePath)
{
    if (!filePath) {
        return NULL;
    }

#ifdef _WIN32
    HANDLE fileHandle = CreateFileA(
        filePath,
        GENERIC_READ,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );
    if (fileHandle == INVALID_HANDLE_VALUE) {
        return NULL;
    }

    LARGE_INTEGER fileSize;
    if (!GetFileSizeEx(fileHandle, &fileSize)) {
        CloseHandle(fileHandle);
        return NULL;
    }

    if (fileSize.QuadPart == 0) {
        CloseHandle(fileHandle);
        return NULL;
    }

    HANDLE mappingHandle = CreateFileMappingA(
        fileHandle,
        NULL,
        PAGE_READONLY,
        0,
        0,
        NULL
    );
    if (!mappingHandle) {
        CloseHandle(fileHandle);
        return NULL;
    }

    void *mappedBase = MapViewOfFile(
        mappingHandle,
        FILE_MAP_READ,
        0,
        0,
        0
    );
    if (!mappedBase) {
        CloseHandle(mappingHandle);
        CloseHandle(fileHandle);
        return NULL;
    }

    picoStream stream = (picoStream)PICO_MALLOC(sizeof(picoStream_t));
    if (!stream) {
        UnmapViewOfFile(mappedBase);
        CloseHandle(mappingHandle);
        CloseHandle(fileHandle);
        return NULL;
    }

    stream->source.mapped.base          = mappedBase;
    stream->source.mapped.size          = (size_t)fileSize.QuadPart;
    stream->source.mapped.position      = 0;
    stream->source.mapped.fileHandle    = fileHandle;
    stream->source.mapped.mappingHandle = mappingHandle;

#else
    int fd = open(filePath, O_RDONLY);
    if (fd < 0) {
        return NULL;
    }

    struct stat st;
    if (fstat(fd, &st) < 0) {
        close(fd);
        return NULL;
    }

    if (st.st_size == 0) {
        close(fd);
        return NULL;
    }

    void *mappedBase = mmap(NULL, (size_t)st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (mappedBase == MAP_FAILED) {
        close(fd);
        return NULL;
    }

    picoStream stream = (picoStream)PICO_MALLOC(sizeof(picoStream_t));
    if (!stream) {
        munmap(mappedBase, (size_t)st.st_size);
        close(fd);
        return NULL;
    }

    stream->source.mapped.base     = mappedBase;
    stream->source.mapped.size     = (size_t)st.st_size;
    stream->source.mapped.position = 0;
    stream->source.mapped.fd       = fd;
#endif

    stream->type         = PICO_STREAM_SOURCE_TYPE_MAPPED;
    stream->canRead      = true;
    stream->canWrite     = false;  // mapped files are read-only
    stream->littleEndian = true;
    stream->ownsMemory   = false;
    stream->ownsFile     = true;   // we own the mapping

    return stream;
}
#endif // PICO_STREAM_ENABLE_MAPPED

void picoStreamDestroy(picoStream stream)
{
    if (!stream) {
        return;
    }

    if (stream->type == PICO_STREAM_SOURCE_TYPE_CUSTOM) {
        if (stream->source.custom && stream->source.custom->destroy) {
            stream->source.custom->destroy(stream->source.custom->userData);
        }
    }

    if (stream->type == PICO_STREAM_SOURCE_TYPE_FILE && stream->ownsFile && stream->source.file) {
        fclose(stream->source.file);
        stream->source.file = NULL;
    }

    if (stream->type == PICO_STREAM_SOURCE_TYPE_MEMORY && stream->ownsMemory && stream->source.memory.buffer) {
        PICO_FREE(stream->source.memory.buffer);
        stream->source.memory.buffer = NULL;
    }

#if PICO_STREAM_ENABLE_MAPPED
    if (stream->type == PICO_STREAM_SOURCE_TYPE_MAPPED && stream->ownsFile) {
#ifdef _WIN32
        if (stream->source.mapped.base) {
            UnmapViewOfFile(stream->source.mapped.base);
        }
        if (stream->source.mapped.mappingHandle) {
            CloseHandle(stream->source.mapped.mappingHandle);
        }
        if (stream->source.mapped.fileHandle) {
            CloseHandle(stream->source.mapped.fileHandle);
        }
#else
        if (stream->source.mapped.base && stream->source.mapped.size > 0) {
            munmap(stream->source.mapped.base, stream->source.mapped.size);
        }
        if (stream->source.mapped.fd >= 0) {
            close(stream->source.mapped.fd);
        }
#endif
    }
#endif // PICO_STREAM_ENABLE_MAPPED

    PICO_FREE(stream);
}

size_t picoStreamRead(picoStream stream, void *buffer, size_t size)
{
    if (!stream || !buffer || size == 0 || !stream->canRead) {
        return 0;
    }

    switch (stream->type) {
        case PICO_STREAM_SOURCE_TYPE_CUSTOM:
            if (stream->source.custom && stream->source.custom->read) {
                return stream->source.custom->read(stream->source.custom->userData, buffer, size);
            }
            break;

        case PICO_STREAM_SOURCE_TYPE_FILE:
            if (stream->source.file) {
                return fread(buffer, 1, size, stream->source.file);
            }
            break;

        case PICO_STREAM_SOURCE_TYPE_MEMORY:
            if (stream->source.memory.buffer) {
                size_t available = stream->source.memory.size - stream->source.memory.position;
                size_t toRead    = (size < available) ? size : available;
                if (toRead > 0) {
                    memcpy(buffer, stream->source.memory.buffer + stream->source.memory.position, toRead);
                    stream->source.memory.position += toRead;
                    return toRead;
                }
            }
            break;

#if PICO_STREAM_ENABLE_MAPPED
        case PICO_STREAM_SOURCE_TYPE_MAPPED:
            if (stream->source.mapped.base) {
                size_t available = stream->source.mapped.size - stream->source.mapped.position;
                size_t toRead = (size < available) ? size : available;
                if (toRead > 0) {
                    memcpy(buffer, (uint8_t *)stream->source.mapped.base + stream->source.mapped.position, toRead);
                    stream->source.mapped.position += toRead;
                    return toRead;
                }
            }
            break;
#endif

        default:
            break;
    }

    return 0;
}

size_t picoStreamWrite(picoStream stream, const void *buffer, size_t size)
{
    if (!stream || !buffer || size == 0 || !stream->canWrite) {
        return 0;
    }

    switch (stream->type) {
        case PICO_STREAM_SOURCE_TYPE_CUSTOM:
            if (stream->source.custom && stream->source.custom->write) {
                return stream->source.custom->write(stream->source.custom->userData, buffer, size);
            }
            break;

        case PICO_STREAM_SOURCE_TYPE_FILE:
            if (stream->source.file) {
                return fwrite(buffer, 1, size, stream->source.file);
            }
            break;

        case PICO_STREAM_SOURCE_TYPE_MEMORY:
            if (stream->source.memory.buffer) {
                size_t available = stream->source.memory.size - stream->source.memory.position;
                size_t toWrite   = (size < available) ? size : available;
                if (toWrite > 0) {
                    memcpy(stream->source.memory.buffer + stream->source.memory.position, buffer, toWrite);
                    stream->source.memory.position += toWrite;
                    return toWrite;
                }
            }
            break;

#if PICO_STREAM_ENABLE_MAPPED
        case PICO_STREAM_SOURCE_TYPE_MAPPED:
            break;
#endif

        default:
            break;
    }

    return 0;
}

int picoStreamSeek(picoStream stream, int64_t offset, picoStreamSeekOrigin origin)
{
    if (!stream) {
        return -1;
    }

    switch (stream->type) {
        case PICO_STREAM_SOURCE_TYPE_CUSTOM:
            if (stream->source.custom && stream->source.custom->seek) {
                return stream->source.custom->seek(stream->source.custom->userData, offset, origin);
            }
            break;

        case PICO_STREAM_SOURCE_TYPE_FILE:
            if (stream->source.file) {
                return fseek(stream->source.file, (long)offset, origin == PICO_STREAM_SEEK_SET ? SEEK_SET : (origin == PICO_STREAM_SEEK_CUR ? SEEK_CUR : SEEK_END));
            }
            break;

        case PICO_STREAM_SOURCE_TYPE_MEMORY:
            if (stream->source.memory.buffer) {
                size_t newPos = 0;
                switch (origin) {
                    case PICO_STREAM_SEEK_SET:
                        newPos = (offset >= 0) ? (size_t)offset : 0;
                        break;
                    case PICO_STREAM_SEEK_CUR:
                        if (offset < 0 && (size_t)(-offset) > stream->source.memory.position) {
                            newPos = 0;
                        } else {
                            newPos = stream->source.memory.position + offset;
                        }
                        break;
                    case PICO_STREAM_SEEK_END:
                        if (offset < 0 && (size_t)(-offset) > stream->source.memory.size) {
                            newPos = 0;
                        } else {
                            newPos = stream->source.memory.size + offset;
                        }
                        break;
                    default:
                        return -1;
                }
                if (newPos > stream->source.memory.size) {
                    return -1;
                }
                stream->source.memory.position = newPos;
                return 0;
            }
            break;

#if PICO_STREAM_ENABLE_MAPPED
        case PICO_STREAM_SOURCE_TYPE_MAPPED:
            if (stream->source.mapped.base) {
                size_t newPos = 0;
                switch (origin) {
                    case PICO_STREAM_SEEK_SET:
                        newPos = (offset >= 0) ? (size_t)offset : 0;
                        break;
                    case PICO_STREAM_SEEK_CUR:
                        if (offset < 0 && (size_t)(-offset) > stream->source.mapped.position) {
                            newPos = 0;
                        } else {
                            newPos = stream->source.mapped.position + offset;
                        }
                        break;
                    case PICO_STREAM_SEEK_END:
                        if (offset < 0 && (size_t)(-offset) > stream->source.mapped.size) {
                            newPos = 0;
                        } else {
                            newPos = stream->source.mapped.size + offset;
                        }
                        break;
                    default:
                        return -1;
                }
                if (newPos > stream->source.mapped.size) {
                    return -1;
                }
                stream->source.mapped.position = newPos;
                return 0;
            }
            break;
#endif

        default:
            break;
    }

    return -1;
}

int64_t picoStreamTell(picoStream stream)
{
    if (!stream) {
        return -1;
    }

    switch (stream->type) {
        case PICO_STREAM_SOURCE_TYPE_CUSTOM:
            if (stream->source.custom && stream->source.custom->tell) {
                return stream->source.custom->tell(stream->source.custom->userData);
            }
            break;

        case PICO_STREAM_SOURCE_TYPE_FILE:
            if (stream->source.file) {
                long pos = ftell(stream->source.file);
                return (pos >= 0) ? (int64_t)pos : -1;
            }
            break;

        case PICO_STREAM_SOURCE_TYPE_MEMORY:
            if (stream->source.memory.buffer) {
                return (int64_t)stream->source.memory.position;
            }
            break;

#if PICO_STREAM_ENABLE_MAPPED
        case PICO_STREAM_SOURCE_TYPE_MAPPED:
            if (stream->source.mapped.base) {
                return (int64_t)stream->source.mapped.position;
            }
            break;
#endif

        default:
            break;
    }

    return -1;
}

bool picoStreamCanRead(picoStream stream)
{
    if (!stream) {
        return false;
    }
    return stream->canRead;
}

bool picoStreamCanWrite(picoStream stream)
{
    if (!stream) {
        return false;
    }
    return stream->canWrite;
}

void picoStreamFlush(picoStream stream)
{
    if (!stream) {
        return;
    }

    switch (stream->type) {
        case PICO_STREAM_SOURCE_TYPE_CUSTOM:
            if (stream->source.custom && stream->source.custom->flush) {
                stream->source.custom->flush(stream->source.custom->userData);
            }
            break;

        case PICO_STREAM_SOURCE_TYPE_FILE:
            if (stream->source.file) {
                fflush(stream->source.file);
            }
            break;

        case PICO_STREAM_SOURCE_TYPE_MEMORY:
            // No-op for memory streams
            break;

#if PICO_STREAM_ENABLE_MAPPED
        case PICO_STREAM_SOURCE_TYPE_MAPPED:
            // No-op for mapped files (read-only)
            break;
#endif

        default:
            break;
    }
}

void picoStreamSetEndianess(picoStream stream, bool littleEndian)
{
    if (!stream) {
        return;
    }
    stream->littleEndian = littleEndian;
}

void picoStreamReset(picoStream stream)
{
    if (!stream) {
        return;
    }
    picoStreamSeek(stream, 0, PICO_STREAM_SEEK_SET);
}

void* picoStreamGetUserData(picoStream stream)
{
    if (!stream || stream->type != PICO_STREAM_SOURCE_TYPE_CUSTOM) {
        return NULL;
    }
    if (stream->source.custom) {
        return stream->source.custom->userData;
    }
    return NULL;
}


__PICO_STREAM_READ_IMPL(U8, uint8_t)
__PICO_STREAM_READ_IMPL(U16, uint16_t)
__PICO_STREAM_READ_IMPL(U32, uint32_t)
__PICO_STREAM_READ_IMPL(U64, uint64_t)
__PICO_STREAM_READ_IMPL(S8, int8_t)
__PICO_STREAM_READ_IMPL(S16, int16_t)
__PICO_STREAM_READ_IMPL(S32, int32_t)
__PICO_STREAM_READ_IMPL(S64, int64_t)
__PICO_STREAM_READ_IMPL(F32, float)
__PICO_STREAM_READ_IMPL(F64, double)

__PICO_STREAM_WRITE_IMPL(U8, uint8_t)
__PICO_STREAM_WRITE_IMPL(U16, uint16_t)
__PICO_STREAM_WRITE_IMPL(U32, uint32_t)
__PICO_STREAM_WRITE_IMPL(U64, uint64_t)
__PICO_STREAM_WRITE_IMPL(S8, int8_t)
__PICO_STREAM_WRITE_IMPL(S16, int16_t)
__PICO_STREAM_WRITE_IMPL(S32, int32_t)
__PICO_STREAM_WRITE_IMPL(S64, int64_t)
__PICO_STREAM_WRITE_IMPL(F32, float)
__PICO_STREAM_WRITE_IMPL(F64, double)

size_t picoStreamReadString(picoStream stream, char *buffer, size_t maxLength)
{
    if (!stream || !buffer || maxLength == 0 || !stream->canRead) {
        return 0;
    }

    size_t count = 0;
    while (count < maxLength - 1) {
        uint8_t ch;
        size_t bytesRead = picoStreamRead(stream, &ch, 1);
        if (bytesRead != 1 || ch == '\0') {
            break;
        }
        buffer[count++] = (char)ch;
    }
    buffer[count] = '\0';
    return count;
}

void picoStreamWriteString(picoStream stream, const char *string)
{
    if (!stream || !string || !stream->canWrite) {
        return;
    }

    size_t length = 0;
    while (string[length] != '\0') {
        length++;
    }
    picoStreamWrite(stream, string, length + 1); // include null terminator
}

size_t picoStreamReadLine(picoStream stream, char *buffer, size_t maxLength)
{
    if (!stream || !buffer || maxLength == 0 || !stream->canRead) {
        return 0;
    }

    size_t count = 0;
    while (count < maxLength - 1) {
        uint8_t ch;
        size_t bytesRead = picoStreamRead(stream, &ch, 1);
        if (bytesRead != 1 || ch == '\n') {
            break;
        }
        buffer[count++] = (char)ch;
    }
    buffer[count] = '\0';
    return count;
}

void picoStreamWriteLine(picoStream stream, const char *string)
{
    if (!stream || !string || !stream->canWrite) {
        return;
    }

    size_t length = 0;
    while (string[length] != '\0') {
        length++;
    }
    picoStreamWrite(stream, string, length);
    uint8_t newline = '\n';
    picoStreamWrite(stream, &newline, 1);
}


bool picoStreamIsSystemLittleEndian(void)
{
    uint16_t test = 0x1;
    return (*(uint8_t *)&test) == 0x1;
}

#endif // PICO_STREAM_IMPLEMENTATION

#endif // PICO_STREAM_H
