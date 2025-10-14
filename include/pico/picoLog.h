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

#ifndef PICO_LOG_H
#define PICO_LOG_H

#ifdef PICO_LOG_DISABLE
#define PICO_LOG_IF_ENABLED(...)
#else
#define PICO_LOG_IF_ENABLED(...) __VA_ARGS__
#endif

#ifndef PICO_LOG_CONFIG_STACK_SIZE
#define PICO_LOG_CONFIG_STACK_SIZE 1024
#endif

#ifndef PICO_LOG_MAX_MESSAGE_LENGTH
#define PICO_LOG_MAX_MESSAGE_LENGTH 4096
#endif

#ifndef PICO_MALLOC
#define PICO_MALLOC(sz) malloc(sz)
#define PICO_FREE(ptr)  free(ptr)
#endif



#if defined(_MSC_VER)
#define PICO_LOG_FUNC __FUNCSIG__
#define PICO_LOG_FILE __FILE__
#define PICO_LOG_LINE __LINE__
#define PICO_LOG_FILENAME (strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1 : __FILE__)
#elif defined(__GNUC__) || defined(__MINGW32__) || defined(__MINGW64__)
#define PICO_LOG_FUNC __func__ 
#define PICO_LOG_FILE __FILE__
#define PICO_LOG_LINE __LINE__
#define PICO_LOG_FILENAME (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
#elif defined(__clang__) || defined(__APPLE__)
#define PICO_LOG_FUNC __func__ 
#define PICO_LOG_FILE __FILE__
#define PICO_LOG_LINE __LINE__
#define PICO_LOG_FILENAME (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
#else
#error "Unsupported compiler"
#endif


// To tag a translation unit do the following at the top of the file before including picolog:
// #define PICO_LOG_TAG "MyTag" 
// other wise it will try to figure out the tag from the file name

#ifndef PICO_LOG_TAG
#define PICO_LOG_TAG PICO_LOG_FILENAME
#endif


#define PICO_LOG(level, ...) PICO_LOG_IF_ENABLED(picoLog(level, PICO_LOG_TAG, PICO_LOG_FILE, PICO_LOG_FUNC, PICO_LOG_LINE, __VA_ARGS__))
#define PICO_DEBUG(...)      PICO_LOG(PICO_LOG_LEVEL_DEBUG, __VA_ARGS__)
#define PICO_VERBOSE(...)    PICO_LOG(PICO_LOG_LEVEL_VERBOSE, __VA_ARGS__)
#define PICO_INFO(...)       PICO_LOG(PICO_LOG_LEVEL_INFO, __VA_ARGS__)
#define PICO_WARN(...)       PICO_LOG(PICO_LOG_LEVEL_WARN, __VA_ARGS__)
#define PICO_ERROR(...)      PICO_LOG(PICO_LOG_LEVEL_ERROR, __VA_ARGS__)

#define PICO_LOG_INIT()        PICO_LOG_IF_ENABLED(picoLogContextCreate())
#define PICO_LOG_SHUTDOWN()    PICO_LOG_IF_ENABLED(picoLogShutdown())

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    PICO_LOG_LEVEL_DEBUG   = 0x01,
    PICO_LOG_LEVEL_VERBOSE = 0x02,
    PICO_LOG_LEVEL_INFO    = 0x04,
    PICO_LOG_LEVEL_WARN    = 0x08,
    PICO_LOG_LEVEL_ERROR   = 0x10,
    PICO_LOG_LEVEL_NONE    = 0x00,
    PICO_LOG_LEVEL_ALL     = 0x1F
} picoLogLevel;

typedef enum {
    PICO_LOG_TARGET_CONSOLE = 0x01,
    PICO_LOG_TARGET_FILE    = 0x02,
    PICO_LOG_TARGET_CUSTOM  = 0x04,
    PICO_LOG_TARGET_ALL     = 0x07
} picoLogTarget;

typedef enum {
    PICO_LOG_FORMAT_DEFAULT      = 0x01, // [YYYY-MM-DD HH:MM:SS.mmm] [LEVEL] [TAG]: MESSAGE
    PICO_LOG_FORMAT_SHORT        = 0x02, // [LEVEL] [TAG]: MESSAGE
    PICO_LOG_FORMAT_MESSAGE_ONLY = 0x04, // MESSAGE
    PICO_LOG_FORMAT_VERBOSE      = 0x08, // [YYYY-MM-DD HH:MM:SS.mmm] [FILE:LINE] [FUNCTION] [LEVEL] [TAG]: MESSAGE
    PICO_LOG_FORMAT_JSON         = 0x10, // {"time": "YYYY-MM-DD HH:MM:SS.mmm", "file": "...", "line": ..., "function": "...", "level": "...", "tag": "...", "message": "..."}
} picoLogFormat;

typedef struct {
    uint32_t year;
    uint32_t month;
    uint32_t day;
    uint32_t hour;
    uint32_t minute;
    uint32_t second;
    uint32_t millisecond;
} picoLogTimeStamp_t;
typedef picoLogTimeStamp_t *picoLogTimeStamp;

typedef struct {
    const char *file;
    const char *function;
    uint32_t line;
} picoLogCodeLocation_t;
typedef picoLogCodeLocation_t *picoLogCodeLocation;

typedef void (*picoLogCustomLogger)(picoLogLevel level, const char *tag, const char *message, picoLogCodeLocation location, picoLogTimeStamp timestamp, void *userData);

typedef struct picoLogContext_t picoLogContext_t;
typedef picoLogContext_t *picoLogContext;

bool picoLogContextCreate();
void picoLogShutdown();
void picoLogPushLevel(picoLogLevel level);
void picoLogPopLevel();
void picoLogPushTagFilter(const char *tags);
void picoLogPopTagFilter();
void picoLogPushTarget(picoLogTarget target);
void picoLogPopTarget();
void picoLogPushFormat(picoLogFormat format);
void picoLogPopFormat();
void picoLogPushCustomLogger(picoLogCustomLogger logger, void *userData);
void picoLogPopCustomLogger();
void picoLogPushFileLogger(const char *filePath);
void picoLogPopFileLogger();
void picoLogPushFromEnvironment();

// Main logging function
void picoLog(picoLogLevel level, const char *tag, const char *file, const char *function, uint32_t line, const char *format, ...);

// For DLLs
picoLogContext picoLogGetContext();
void picoLogSetContext(picoLogContext context);

// Utility functions
const char *picoLogLevelToString(picoLogLevel level);
const char *picoLogFormatToString(picoLogFormat format);
const char *picoLogTargetToString(picoLogTarget target);
picoLogLevel picoLogStringToLevel(const char *levelStr);
picoLogFormat picoLogStringToFormat(const char *formatStr);
picoLogTarget picoLogStringToTarget(const char *targetStr);

#if defined(PICO_IMPLEMENTATION) && !defined(PICO_LOG_IMPLEMENTATION)
#define PICO_LOG_IMPLEMENTATION
#endif

#if defined(PICO_LOG_IMPLEMENTATION) && !defined(PICO_LOG_DISABLE)

#if defined(_WIN32) || defined(_WIN64)
#include <Windows.h>
#define PICO_LOG_MAX_PATH MAX_PATH
#else
#include <limits.h>
#include <sys/time.h>
#define PICO_LOG_MAX_PATH PATH_MAX
#endif

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

struct picoLogContext_t {
    picoLogLevel levelStack[PICO_LOG_CONFIG_STACK_SIZE];
    uint32_t levelStackTop;

    char tagFilterStack[PICO_LOG_CONFIG_STACK_SIZE][256];
    uint32_t tagFilterStackTop;

    picoLogTarget targetStack[PICO_LOG_CONFIG_STACK_SIZE];
    uint32_t targetStackTop;

    picoLogFormat formatStack[PICO_LOG_CONFIG_STACK_SIZE];
    uint32_t formatStackTop;

    picoLogCustomLogger customLoggerStack[PICO_LOG_CONFIG_STACK_SIZE];
    void *customLoggerUserDataStack[PICO_LOG_CONFIG_STACK_SIZE];
    uint32_t customLoggerStackTop;

    char logFilePaths[PICO_LOG_CONFIG_STACK_SIZE][PICO_LOG_MAX_PATH];
    uint32_t logFilesStackTop;
};

typedef struct
{
    picoLogLevel level;
    const char *tag;
    const char *message;
    picoLogCodeLocation location;
    picoLogTimeStamp timestamp;
} picoLogEntry_t;
typedef picoLogEntry_t *picoLogEntry;

picoLogContext __picoLogGlobalContext = NULL;

static picoLogTimeStamp __picoLogGetCurrentTimestamp()
{
    static picoLogTimeStamp_t timestamp = {0};
#if defined(_WIN32) || defined(_WIN64)
    SYSTEMTIME st;
    GetSystemTime(&st);
    timestamp.year        = st.wYear;
    timestamp.month       = st.wMonth;
    timestamp.day         = st.wDay;
    timestamp.hour        = st.wHour;
    timestamp.minute      = st.wMinute;
    timestamp.second      = st.wSecond;
    timestamp.millisecond = st.wMilliseconds;
#elif defined(__unix__) || defined(__APPLE__)
    struct timeval tv;
    gettimeofday(&tv, NULL);
    struct tm *tm         = localtime(&tv.tv_sec);
    timestamp.year        = tm->tm_year + 1900;
    timestamp.month       = tm->tm_mon + 1;
    timestamp.day         = tm->tm_mday;
    timestamp.hour        = tm->tm_hour;
    timestamp.minute      = tm->tm_min;
    timestamp.second      = tm->tm_sec;
    timestamp.millisecond = tv.tv_usec / 1000;
#else
#error "Unsupported platform for time functions"
#endif
    return &timestamp;
}

static picoLogLevel __picoLogGetCurrentLevel()
{
    if (__picoLogGlobalContext == NULL || __picoLogGlobalContext->levelStackTop == 0) {
        return PICO_LOG_LEVEL_NONE;
    }
    return __picoLogGlobalContext->levelStack[__picoLogGlobalContext->levelStackTop - 1];
}

static const char *__picoLogGetCurrentTagFilter()
{
    if (__picoLogGlobalContext == NULL || __picoLogGlobalContext->tagFilterStackTop == 0) {
        return "";
    }
    return __picoLogGlobalContext->tagFilterStack[__picoLogGlobalContext->tagFilterStackTop - 1];
}

static picoLogTarget __picoLogGetCurrentTarget()
{
    if (__picoLogGlobalContext == NULL || __picoLogGlobalContext->targetStackTop == 0) {
        return PICO_LOG_TARGET_CONSOLE;
    }
    return __picoLogGlobalContext->targetStack[__picoLogGlobalContext->targetStackTop - 1];
}

static picoLogFormat __picoLogGetCurrentFormat()
{
    if (__picoLogGlobalContext == NULL || __picoLogGlobalContext->formatStackTop == 0) {
        return PICO_LOG_FORMAT_DEFAULT;
    }
    return __picoLogGlobalContext->formatStack[__picoLogGlobalContext->formatStackTop - 1];
}

static void __picoLogDispatchToCustomLoggers(picoLogEntry entry)
{
    if (__picoLogGlobalContext == NULL) {
        return;
    }

    if (!(__picoLogGetCurrentTarget() & PICO_LOG_TARGET_CUSTOM)) {
        return;
    }

    for (uint32_t i = 0; i < __picoLogGlobalContext->customLoggerStackTop; i++) {
        picoLogCustomLogger logger = __picoLogGlobalContext->customLoggerStack[i];
        void *userData             = __picoLogGlobalContext->customLoggerUserDataStack[i];
        if (logger != NULL) {
            logger(entry->level, entry->tag, entry->message, entry->location, entry->timestamp, userData);
        }
    }
}

static void __picoLogDispatchToFileLoggers(picoLogEntry entry)
{
    // loop through all file loggers and write the log entry to each file
    if (__picoLogGlobalContext == NULL) {
        return;
    }

    if (!(__picoLogGetCurrentTarget() & PICO_LOG_TARGET_FILE)) {
        return;
    }

    for (uint32_t i = 0; i < __picoLogGlobalContext->logFilesStackTop; i++) {
        const char *filePath = __picoLogGlobalContext->logFilePaths[i];
        if (filePath[0] == '\0') {
            continue;
        }
        FILE *file = fopen(filePath, "a+");
        if (file != NULL) {
            fprintf(file, "%s\n", entry->message);
            fclose(file);
        } else {
            PICO_ERROR("Failed to open log file: %s", filePath);
        }
    }
}

static void __picoLogDispatchToConsoleLoggers(picoLogEntry entry)
{
    if (__picoLogGlobalContext == NULL) {
        return;
    }

    if (!(__picoLogGetCurrentTarget() & PICO_LOG_TARGET_CONSOLE)) {
        return;
    }

    // color coding for different log levels
#if defined(_WIN32) || defined(_WIN64)
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    WORD color;

    switch (entry->level) {
        case PICO_LOG_LEVEL_DEBUG:
            color = FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_INTENSITY;
            break;
        case PICO_LOG_LEVEL_VERBOSE:
            color = FOREGROUND_BLUE | FOREGROUND_INTENSITY;
            break;
        case PICO_LOG_LEVEL_INFO:
            color = FOREGROUND_GREEN | FOREGROUND_INTENSITY;
            break;
        case PICO_LOG_LEVEL_WARN:
            color = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY;
            break;
        case PICO_LOG_LEVEL_ERROR:
            color = FOREGROUND_RED | FOREGROUND_INTENSITY;
            break;
        default:
            color = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
            break;
    }

    if (entry->level & PICO_LOG_LEVEL_ERROR) {
        SetConsoleTextAttribute(hConsole, color);
        fprintf(stderr, "%s\n", entry->message);
        SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
    } else {
        SetConsoleTextAttribute(hConsole, color);
        fprintf(stdout, "%s\n", entry->message);
        SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
    }

#elif defined(__unix__) || defined(__APPLE__)
    const char *colorReset   = "\033[0m";
    const char *colorRed     = "\033[31m";
    const char *colorYellow  = "\033[33m";
    const char *colorGreen   = "\033[32m";
    const char *colorCyan    = "\033[36m";
    const char *colorBlue    = "\033[34m";

    switch (entry->level) {
        case PICO_LOG_LEVEL_DEBUG:
            fprintf(stdout, "%s%s%s\n", colorCyan, entry->message, colorReset);
            break;
        case PICO_LOG_LEVEL_VERBOSE:
            fprintf(stdout, "%s%s%s\n", colorBlue, entry->message, colorReset);
            break;
        case PICO_LOG_LEVEL_INFO:
            fprintf(stdout, "%s%s%s\n", colorGreen, entry->message, colorReset);
            break;
        case PICO_LOG_LEVEL_WARN:
            fprintf(stdout, "%s%s%s\n", colorYellow, entry->message, colorReset);
            break;
        case PICO_LOG_LEVEL_ERROR:
            fprintf(stderr, "%s%s%s\n", colorRed, entry->message, colorReset);
            break;
        default:
            fprintf(stdout, "%s\n", entry->message);
            break;
    }
#else 
#warning "Unsupported platform for console colors"
    if (entry->level & PICO_LOG_LEVEL_ERROR) {
        fprintf(stderr, "%s", entry->message);
    } else {
        fprintf(stdout, "%s", entry->message);
    }
#endif
}

static const char *__picoLogFormatMessage(picoLogEntry entry)
{
    static char formattedMessage[PICO_LOG_MAX_MESSAGE_LENGTH * 2] = {0};
    picoLogFormat format = __picoLogGetCurrentFormat();
    switch (format) {
        case PICO_LOG_FORMAT_DEFAULT:
            snprintf(formattedMessage, sizeof(formattedMessage), "[%04u-%02u-%02u %02u:%02u:%02u.%03u] [%s] [%s]: %s",
                     entry->timestamp->year, entry->timestamp->month, entry->timestamp->day,
                     entry->timestamp->hour, entry->timestamp->minute, entry->timestamp->second, entry->timestamp->millisecond,
                     picoLogLevelToString(entry->level),
                     entry->tag ? entry->tag : "NO_TAG",
                     entry->message);
            break;
        case PICO_LOG_FORMAT_SHORT:
            snprintf(formattedMessage, sizeof(formattedMessage), "[%s] [%s]: %s",
                     picoLogLevelToString(entry->level),
                     entry->tag ? entry->tag : "NO_TAG",
                     entry->message);
            break;
        case PICO_LOG_FORMAT_MESSAGE_ONLY:
            snprintf(formattedMessage, sizeof(formattedMessage), "%s", entry->message);
            break;
        case PICO_LOG_FORMAT_VERBOSE:
            snprintf(formattedMessage, sizeof(formattedMessage), "[%04u-%02u-%02u %02u:%02u:%02u.%03u] [%s:%u] [%s] [%s] [%s]: %s",
                     entry->timestamp->year, entry->timestamp->month, entry->timestamp->day,
                     entry->timestamp->hour, entry->timestamp->minute, entry->timestamp->second, entry->timestamp->millisecond,
                     entry->location ? entry->location->file : "NO_FILE",
                     entry->location ? entry->location->line : 0,
                     entry->location ? entry->location->function : "NO_FUNCTION",
                     picoLogLevelToString(entry->level),
                     entry->tag ? entry->tag : "NO_TAG",
                     entry->message);
            break;
        case PICO_LOG_FORMAT_JSON:
            snprintf(formattedMessage, sizeof(formattedMessage),
                     "{\"time\": \"%04u-%02u-%02u %02u:%02u:%02u.%03u\", \"file\": \"%s\", \"line\": %u, \"function\": \"%s\", \"level\": \"%s\", \"tag\": \"%s\", \"message\": \"%s\"}",
                     entry->timestamp->year, entry->timestamp->month, entry->timestamp->day,
                     entry->timestamp->hour, entry->timestamp->minute, entry->timestamp->second, entry->timestamp->millisecond,
                     entry->location ? entry->location->file : "NO_FILE",
                     entry->location ? entry->location->line : 0,
                     entry->location ? entry->location->function : "NO_FUNCTION",
                     picoLogLevelToString(entry->level),
                     entry->tag ? entry->tag : "NO_TAG",
                     entry->message);
            break;
        default:
            snprintf(formattedMessage, sizeof(formattedMessage), "%s", entry->message);
            break;
    }

    return formattedMessage;
}

bool picoLogContextCreate()
{
    if (__picoLogGlobalContext != NULL) {
        PICO_WARN("picoLogContextCreate called but context already exists");
        return true;
    }
    __picoLogGlobalContext = (picoLogContext)PICO_MALLOC(sizeof(picoLogContext_t));
    if (__picoLogGlobalContext == NULL) {
        return false;
    }
    memset(__picoLogGlobalContext, 0, sizeof(picoLogContext_t));

    // Initialize stacks
    __picoLogGlobalContext->levelStack[0] = PICO_LOG_LEVEL_ALL;
    __picoLogGlobalContext->levelStackTop = 1;

    __picoLogGlobalContext->tagFilterStack[0][0] = '\0'; // Empty string means no filter
    __picoLogGlobalContext->tagFilterStackTop    = 1;

    __picoLogGlobalContext->targetStack[0] = PICO_LOG_TARGET_CONSOLE;
    __picoLogGlobalContext->targetStackTop = 1;

    __picoLogGlobalContext->formatStack[0] = PICO_LOG_FORMAT_DEFAULT;
    __picoLogGlobalContext->formatStackTop = 1;

    return true;
}

void picoLogShutdown()
{
    if (__picoLogGlobalContext == NULL) {
        PICO_WARN("picoLogShutdown called but context is NULL");
        return;
    }

    PICO_FREE(__picoLogGlobalContext);
    __picoLogGlobalContext = NULL;
}

picoLogContext picoLogGetContext()
{
    return __picoLogGlobalContext;
}

void picoLogSetContext(picoLogContext context)
{
    if (__picoLogGlobalContext != NULL) {
        PICO_WARN("picoLogSetContext called but context already exists");
        return;
    }

    __picoLogGlobalContext = context;
}

void picoLogPushLevel(picoLogLevel level)
{
    if (__picoLogGlobalContext == NULL) {
        PICO_WARN("picoLogPushLevel called but context is NULL");
        return;
    }
    if (__picoLogGlobalContext->levelStackTop >= PICO_LOG_CONFIG_STACK_SIZE) {
        PICO_ERROR("picoLogPushLevel stack overflow");
        return;
    }
    __picoLogGlobalContext->levelStack[__picoLogGlobalContext->levelStackTop++] = level;
}

void picoLogPopLevel()
{
    if (__picoLogGlobalContext == NULL) {
        PICO_WARN("picoLogPopLevel called but context is NULL");
        return;
    }
    if (__picoLogGlobalContext->levelStackTop == 0) {
        PICO_ERROR("picoLogPopLevel stack underflow");
        return;
    }
    __picoLogGlobalContext->levelStackTop--;
}

void picoLogPushTagFilter(const char *tags)
{
    if (__picoLogGlobalContext == NULL) {
        PICO_WARN("picoLogPushTagFilter called but context is NULL");
        return;
    }
    if (__picoLogGlobalContext->tagFilterStackTop >= PICO_LOG_CONFIG_STACK_SIZE) {
        PICO_ERROR("picoLogPushTagFilter stack overflow");
        return;
    }
    strncpy(__picoLogGlobalContext->tagFilterStack[__picoLogGlobalContext->tagFilterStackTop++], tags, 255);
    __picoLogGlobalContext->tagFilterStack[__picoLogGlobalContext->tagFilterStackTop - 1][255] = '\0';
}

void picoLogPopTagFilter()
{
    if (__picoLogGlobalContext == NULL) {
        PICO_WARN("picoLogPopTagFilter called but context is NULL");
        return;
    }
    if (__picoLogGlobalContext->tagFilterStackTop == 0) {
        PICO_ERROR("picoLogPopTagFilter stack underflow");
        return;
    }
    __picoLogGlobalContext->tagFilterStackTop--;
}

void picoLogPushTarget(picoLogTarget target)
{
    if (__picoLogGlobalContext == NULL) {
        PICO_WARN("picoLogPushTarget called but context is NULL");
        return;
    }
    if (__picoLogGlobalContext->targetStackTop >= PICO_LOG_CONFIG_STACK_SIZE) {
        PICO_ERROR("picoLogPushTarget stack overflow");
        return;
    }
    __picoLogGlobalContext->targetStack[__picoLogGlobalContext->targetStackTop++] = target;
}

void picoLogPopTarget()
{
    if (__picoLogGlobalContext == NULL) {
        PICO_WARN("picoLogPopTarget called but context is NULL");
        return;
    }
    if (__picoLogGlobalContext->targetStackTop == 0) {
        PICO_ERROR("picoLogPopTarget stack underflow");
        return;
    }
    __picoLogGlobalContext->targetStackTop--;
}

void picoLogPushFormat(picoLogFormat format)
{
    if (__picoLogGlobalContext == NULL) {
        PICO_WARN("picoLogPushFormat called but context is NULL");
        return;
    }
    if (__picoLogGlobalContext->formatStackTop >= PICO_LOG_CONFIG_STACK_SIZE) {
        PICO_ERROR("picoLogPushFormat stack overflow");
        return;
    }
    __picoLogGlobalContext->formatStack[__picoLogGlobalContext->formatStackTop++] = format;
}

void picoLogPopFormat()
{
    if (__picoLogGlobalContext == NULL) {
        PICO_WARN("picoLogPopFormat called but context is NULL");
        return;
    }
    if (__picoLogGlobalContext->formatStackTop == 0) {
        PICO_ERROR("picoLogPopFormat stack underflow");
        return;
    }
    __picoLogGlobalContext->formatStackTop--;
}

void picoLogPushCustomLogger(picoLogCustomLogger logger, void *userData)
{
    if (__picoLogGlobalContext == NULL) {
        PICO_WARN("picoLogPushCustomLogger called but context is NULL");
        return;
    }
    if (__picoLogGlobalContext->customLoggerStackTop >= PICO_LOG_CONFIG_STACK_SIZE) {
        PICO_ERROR("picoLogPushCustomLogger stack overflow");
        return;
    }
    __picoLogGlobalContext->customLoggerStack[__picoLogGlobalContext->customLoggerStackTop]         = logger;
    __picoLogGlobalContext->customLoggerUserDataStack[__picoLogGlobalContext->customLoggerStackTop] = userData;
    __picoLogGlobalContext->customLoggerStackTop++;
}

void picoLogPopCustomLogger()
{
    if (__picoLogGlobalContext == NULL) {
        PICO_WARN("picoLogPopCustomLogger called but context is NULL");
        return;
    }
    if (__picoLogGlobalContext->customLoggerStackTop == 0) {
        PICO_ERROR("picoLogPopCustomLogger stack underflow");
        return;
    }
    __picoLogGlobalContext->customLoggerStackTop--;
}

void picoLogPushFileLogger(const char *filePath)
{
    if (__picoLogGlobalContext == NULL) {
        PICO_WARN("picoLogPushFileLogger called but context is NULL");
        return;
    }
    if (__picoLogGlobalContext->logFilesStackTop >= PICO_LOG_CONFIG_STACK_SIZE) {
        PICO_ERROR("picoLogPushFileLogger stack overflow");
        return;
    }
    strncpy(__picoLogGlobalContext->logFilePaths[__picoLogGlobalContext->logFilesStackTop++], filePath, PICO_LOG_MAX_PATH - 1);
    __picoLogGlobalContext->logFilePaths[__picoLogGlobalContext->logFilesStackTop - 1][PICO_LOG_MAX_PATH - 1] = '\0';
}

void picoLogPopFileLogger()
{
    if (__picoLogGlobalContext == NULL) {
        PICO_WARN("picoLogPopFileLogger called but context is NULL");
        return;
    }
    if (__picoLogGlobalContext->logFilesStackTop == 0) {
        PICO_ERROR("picoLogPopFileLogger stack underflow");
        return;
    }
    __picoLogGlobalContext->logFilesStackTop--;
}

void picoLogPushFromEnvironment()
{
    if (__picoLogGlobalContext == NULL) {
        PICO_WARN("picoLogPushFromEnvironment called but context is NULL");
        return;
    }

    const char *levelStr  = getenv("PICO_LOG_LEVEL");
    const char *targetStr = getenv("PICO_LOG_TARGET");
    const char *formatStr = getenv("PICO_LOG_FORMAT");
    const char *tagStr    = getenv("PICO_LOG_TAG_FILTER");
    const char *fileStr   = getenv("PICO_LOG_FILE");

    if (levelStr != NULL) {
        picoLogLevel level = picoLogStringToLevel(levelStr);
        picoLogPushLevel(level);
    }

    if (targetStr != NULL) {
        picoLogTarget target = picoLogStringToTarget(targetStr);
        picoLogPushTarget(target);
    }

    if (formatStr != NULL) {
        picoLogFormat format = picoLogStringToFormat(formatStr);
        picoLogPushFormat(format);
    }

    if (tagStr != NULL) {
        picoLogPushTagFilter(tagStr);
    }

    if (fileStr != NULL) {
        picoLogPushFileLogger(fileStr);
    }
}

void picoLog(picoLogLevel level, const char *tag, const char *file, const char *function, uint32_t line, const char *format, ...)
{
    if (__picoLogGlobalContext == NULL) {
        return;
    }

    static char messageBuffer[PICO_LOG_MAX_MESSAGE_LENGTH];

    if (!(level & __picoLogGetCurrentLevel())) {
        return;
    }

    const char *currentTagFilter = __picoLogGetCurrentTagFilter();
    if (currentTagFilter[0] != '\0' && tag != NULL && strstr(currentTagFilter, tag) == NULL) {
        return;
    }

    va_list args;
    va_start(args, format);
    vsnprintf(messageBuffer, PICO_LOG_MAX_MESSAGE_LENGTH, format, args);
    va_end(args);

    picoLogEntry_t entry           = {0};
    entry.level                    = level;
    entry.tag                      = tag;
    entry.message                  = messageBuffer;
    picoLogCodeLocation_t location = {file, function, line};
    entry.location                 = &location;
    entry.timestamp                = __picoLogGetCurrentTimestamp();
    entry.message                  = __picoLogFormatMessage(&entry);

    __picoLogDispatchToCustomLoggers(&entry);
    __picoLogDispatchToFileLoggers(&entry);
    __picoLogDispatchToConsoleLoggers(&entry);

    // Clear message buffer for next use
    messageBuffer[0] = '\0';
}

const char *picoLogLevelToString(picoLogLevel level)
{
    switch (level) {
        case PICO_LOG_LEVEL_DEBUG:
            return "DEBUG";
        case PICO_LOG_LEVEL_VERBOSE:
            return "VERBOSE";
        case PICO_LOG_LEVEL_INFO:
            return "INFO";
        case PICO_LOG_LEVEL_WARN:
            return "WARN";
        case PICO_LOG_LEVEL_ERROR:
            return "ERROR";
        case PICO_LOG_LEVEL_NONE:
            return "NONE";
        case PICO_LOG_LEVEL_ALL:
            return "ALL";
        default:
            return "UNKNOWN";
    }
}

const char *picoLogFormatToString(picoLogFormat format)
{
    switch (format) {
        case PICO_LOG_FORMAT_DEFAULT:
            return "DEFAULT";
        case PICO_LOG_FORMAT_SHORT:
            return "SHORT";
        case PICO_LOG_FORMAT_MESSAGE_ONLY:
            return "MESSAGE_ONLY";
        case PICO_LOG_FORMAT_VERBOSE:
            return "VERBOSE";
        case PICO_LOG_FORMAT_JSON:
            return "JSON";
        default:
            return "UNKNOWN";
    }
}

const char *picoLogTargetToString(picoLogTarget target)
{
    switch (target) {
        case PICO_LOG_TARGET_CONSOLE:
            return "CONSOLE";
        case PICO_LOG_TARGET_FILE:
            return "FILE";
        case PICO_LOG_TARGET_CUSTOM:
            return "CUSTOM";
        case PICO_LOG_TARGET_ALL:
            return "ALL";
        default:
            return "UNKNOWN";
    }
}

picoLogLevel picoLogStringToLevel(const char *levelStr)
{
    if (strcmp(levelStr, "DEBUG") == 0) {
        return PICO_LOG_LEVEL_DEBUG;
    } else if (strcmp(levelStr, "VERBOSE") == 0) {
        return PICO_LOG_LEVEL_VERBOSE;
    } else if (strcmp(levelStr, "INFO") == 0) {
        return PICO_LOG_LEVEL_INFO;
    } else if (strcmp(levelStr, "WARN") == 0) {
        return PICO_LOG_LEVEL_WARN;
    } else if (strcmp(levelStr, "ERROR") == 0) {
        return PICO_LOG_LEVEL_ERROR;
    } else if (strcmp(levelStr, "NONE") == 0) {
        return PICO_LOG_LEVEL_NONE;
    } else if (strcmp(levelStr, "ALL") == 0) {
        return PICO_LOG_LEVEL_ALL;
    } else {
        return PICO_LOG_LEVEL_NONE; // Default to NONE for unknown strings
    }
}

picoLogFormat picoLogStringToFormat(const char *formatStr)
{
    if (strcmp(formatStr, "DEFAULT") == 0) {
        return PICO_LOG_FORMAT_DEFAULT;
    } else if (strcmp(formatStr, "SHORT") == 0) {
        return PICO_LOG_FORMAT_SHORT;
    } else if (strcmp(formatStr, "MESSAGE_ONLY") == 0) {
        return PICO_LOG_FORMAT_MESSAGE_ONLY;
    } else if (strcmp(formatStr, "VERBOSE") == 0) {
        return PICO_LOG_FORMAT_VERBOSE;
    } else if (strcmp(formatStr, "JSON") == 0) {
        return PICO_LOG_FORMAT_JSON;
    } else {
        return PICO_LOG_FORMAT_DEFAULT; // Default to DEFAULT for unknown strings
    }
}

picoLogTarget picoLogStringToTarget(const char *targetStr)
{
    if (strcmp(targetStr, "CONSOLE") == 0) {
        return PICO_LOG_TARGET_CONSOLE;
    } else if (strcmp(targetStr, "FILE") == 0) {
        return PICO_LOG_TARGET_FILE;
    } else if (strcmp(targetStr, "CUSTOM") == 0) {
        return PICO_LOG_TARGET_CUSTOM;
    } else if (strcmp(targetStr, "ALL") == 0) {
        return PICO_LOG_TARGET_ALL;
    } else {
        return PICO_LOG_TARGET_CONSOLE; // Default to CONSOLE for unknown strings
    }
}

#endif // PICO_LOG_IMPLEMENTATION

#endif // PICO_LOG_H