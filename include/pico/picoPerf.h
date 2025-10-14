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

#ifndef PICO_PERF_H
#define PICO_PERF_H

#ifdef PICO_PERF_DISABLE
#define PICO_PERF_IF_ENABLED(...)
#else
#define PICO_PERF_IF_ENABLED(...) __VA_ARGS__
#endif

#ifndef PICO_PERF_MAX_SCOPES
#define PICO_PERF_MAX_SCOPES 1024 * 4
#endif

#ifndef PICO_PERF_MAX_RECORDS
#define PICO_PERF_MAX_RECORDS 16
#endif

#ifndef PICO_PERF_MAX_NAME_LENGTH
#define PICO_PERF_MAX_NAME_LENGTH 64
#endif

#if defined(_MSC_VER)
#define PICO_PERF_FUNC     __FUNCSIG__
#define PICO_PERF_FILE     __FILE__
#define PICO_PERF_LINE     __LINE__
#define PICO_PERF_FILENAME (strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1 : __FILE__)
#elif defined(__GNUC__) || defined(__MINGW32__) || defined(__MINGW64__)
#define PICO_PERF_FUNC     __func__
#define PICO_PERF_FILE     __FILE__
#define PICO_PERF_LINE     __LINE__
#define PICO_PERF_FILENAME (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
#elif defined(__clang__) || defined(__APPLE__)
#define PICO_PERF_FUNC     __func__
#define PICO_PERF_FILE     __FILE__
#define PICO_PERF_LINE     __LINE__
#define PICO_PERF_FILENAME (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
#else
#error "Unsupported compiler"
#endif

#define PICO_PERF_PUSH_SCOPE(name)     PICO_PERF_IF_ENABLED(picoPerfPushScope(name, PICO_PERF_FILE, PICO_PERF_FUNC, PICO_PERF_LINE))
#define PICO_PERF_POP_SCOPE()          PICO_PERF_IF_ENABLED(picoPerfPopScope(PICO_PERF_FILE, PICO_PERF_FUNC, PICO_PERF_LINE))
#define PICO_PERF_POP_N_SCOPES(n)      PICO_PERF_IF_ENABLED(picoPerfPopNScopes(n, PICO_PERF_FILE, PICO_PERF_FUNC, PICO_PERF_LINE))
#define PICO_PERF_BEGIN_RECORD()       PICO_PERF_IF_ENABLED(picoPerfBeginRecord())
#define PICO_PERF_END_RECORD()         PICO_PERF_IF_ENABLED(picoPerfEndRecord())
#define PICO_PERF_GET_REPORT(out, fmt) PICO_PERF_IF_ENABLED(picoPerfGetReport(out, fmt))
#define PICO_PERF_SLEEP(ms)            PICO_PERF_IF_ENABLED(picoPerfSleep(ms))
#define PICO_PERF_CREATE_CONTEXT()     PICO_PERF_IF_ENABLED(picoPerfCreateContext())
#define PICO_PERF_DESTROY_CONTEXT()    PICO_PERF_IF_ENABLED(picoPerfDestroyContext())

#ifndef PICO_MALLOC
#define PICO_MALLOC(sz) malloc(sz)
#define PICO_FREE(ptr)  free(ptr)
#endif

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

typedef enum {
    PICO_PERF_REPORT_FORMAT_TEXT,
    PICO_PERF_REPORT_FORMAT_CSV,
    PICO_PERF_REPORT_FORMAT_JSON,
    PICO_PERF_REPORT_FORMAT_XML,
} picoPerfReportFormat;

typedef struct {
    uint16_t year;
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
    uint16_t millisecond;
    uint32_t nanosecond;
} picoPerfTimeStamp;

typedef struct {
    const char *file;
    const char *function;
    uint32_t line;
} picoPerfCodeLocation_t;

typedef struct picoPerfContext_t picoPerfContext_t;
typedef picoPerfContext_t *picoPerfContext;

typedef uint64_t picoPerfTime;

bool picoPerfCreateContext();
void picoPerfDestroyContext();
picoPerfContext picoPerfGetContext();
void picoPerfSetContext(picoPerfContext context);

picoPerfTimeStamp picoPerfGetCurrentTimestamp();
picoPerfTime picoPerfNow();
uint64_t picoPerfFrequency(); // ticks per second
double picoPerfDurationSeconds(picoPerfTime start, picoPerfTime end);
double picoPerfDurationMilliseconds(picoPerfTime start, picoPerfTime end);
double picoPerfDurationMicroseconds(picoPerfTime start, picoPerfTime end);
double picoPerfDurationNanoseconds(picoPerfTime start, picoPerfTime end);
void picoPerfFormatDuration(picoPerfTime duration, char *buffer, size_t bufferSize);
void picoPerfSleep(uint32_t milliseconds);

bool picoPerfBeginRecord();
void picoPerfEndRecord();

void picoPerfPushScope(const char *name, const char *file, const char *function, uint32_t line);
void picoPerfPopScope(const char *file, const char *function, uint32_t line);
void picoPerfPopNScopes(int count, const char *file, const char *function, uint32_t line); // -1 for all
void picoPerfGetReport(FILE *output, picoPerfReportFormat format);

#if defined(PICO_IMPLEMENTATION) && !defined(PICO_PERF_IMPLEMENTATION)
#define PICO_PERF_IMPLEMENTATION
#endif

#if defined(PICO_PERF_IMPLEMENTATION) && !defined(PICO_PERF_DISABLE)

#include <string.h>
#include <time.h>

#if defined(_WIN32) || defined(_WIN64)
#define PICO_PERF_MAX_PATH MAX_PATH
#include <Windows.h>
#elif defined(__unix__) || defined(__APPLE__)
#include <limits.h>
#include <sys/time.h>
#define PICO_PERF_MAX_PATH PATH_MAX
#else
#error "Unsupported platform for picoPerf"
#endif

typedef struct {
    char file[PICO_PERF_MAX_PATH];
    char function[PICO_PERF_MAX_PATH];
    uint32_t line;
} __picoPerfRecordLocation_t;

typedef struct {
    char name[PICO_PERF_MAX_NAME_LENGTH];
    char parentName[PICO_PERF_MAX_NAME_LENGTH];
    picoPerfTime startTime;
    picoPerfTime endTime;
    __picoPerfRecordLocation_t startLocation;
    __picoPerfRecordLocation_t endLocation;
    picoPerfTimeStamp startTimestamp;
    picoPerfTimeStamp endTimestamp;
    int scopeDepth;
} __picoPerfRecordItem_t;

typedef struct {
    picoPerfTime startTime;
    picoPerfTime endTime;
    __picoPerfRecordItem_t items[PICO_PERF_MAX_SCOPES];
    size_t itemCount;
} __picoPerfRecord_t;
typedef __picoPerfRecord_t *__picoPerfRecord;

struct picoPerfContext_t {
    __picoPerfRecord records;
    size_t recordHead;
    size_t recordCount;

    __picoPerfRecord_t currentRecord;
    __picoPerfRecordItem_t scopeStack[PICO_PERF_MAX_SCOPES];
    int scopeStackTop;

    bool recording;
};

static picoPerfContext __picoPerfGlobalContext = NULL;

static const char *__picoPerfEscapeString(const char *str)
{
    static char escaped[PICO_PERF_MAX_NAME_LENGTH * 2];
    size_t j = 0;
    for (size_t i = 0; str[i] != '\0' && j < sizeof(escaped) - 1; i++) {
        if (str[i] == '\"' || str[i] == '\\') {
            if (j < sizeof(escaped) - 2) {
                escaped[j++] = '\\';
                escaped[j++] = str[i];
            } else {
                break;
            }
        } else if (str[i] >= 0 && str[i] <= 0x1F) {
            if (j < sizeof(escaped) - 7) {
                snprintf(&escaped[j], 7, "\\u%04x", (unsigned char)str[i]);
                j += 6;
            } else {
                break;
            }
        } else {
            escaped[j++] = str[i];
        }
    }
    escaped[j] = '\0';
    return escaped;
}

static void __picoPerfGetReportText(FILE *output)
{
    if (!__picoPerfGlobalContext || !output) {
        return;
    }

    fprintf(output, "picoPerf Performance Report\n");
    fprintf(output, "Total Records: %zu\n", __picoPerfGlobalContext->recordCount);

    for (size_t recordIdx = 0; recordIdx < __picoPerfGlobalContext->recordCount; recordIdx++) {
        __picoPerfRecord_t *record = &__picoPerfGlobalContext->records[recordIdx];
        fprintf(output, "--- Record %zu ---\n", recordIdx + 1);
        fprintf(output, "Items: %zu\n\n", record->itemCount);

        for (size_t itemIdx = 0; itemIdx < record->itemCount; itemIdx++) {
            __picoPerfRecordItem_t *item = &record->items[itemIdx];

            for (int i = 0; i < item->scopeDepth; i++) {
                fprintf(output, "  ");
            }

            char durationBuf[64];
            picoPerfFormatDuration(item->endTime - item->startTime, durationBuf, sizeof(durationBuf));

            fprintf(output, "[%zu] %s[>%s]: %s\n", itemIdx, item->name, item->parentName, durationBuf);

            for (int i = 0; i < item->scopeDepth; i++) {
                fprintf(output, "  ");
            }
            fprintf(output, "  Start: %s:%u in %s() at %04u-%02u-%02u %02u:%02u:%02u.%03u\n",
                    item->startLocation.file, item->startLocation.line, item->startLocation.function,
                    item->startTimestamp.year, item->startTimestamp.month, item->startTimestamp.day,
                    item->startTimestamp.hour, item->startTimestamp.minute, item->startTimestamp.second,
                    item->startTimestamp.millisecond);

            for (int i = 0; i < item->scopeDepth; i++) {
                fprintf(output, "  ");
            }
            fprintf(output, "  End:   %s:%u in %s() at %04u-%02u-%02u %02u:%02u:%02u.%03u\n",
                    item->endLocation.file, item->endLocation.line, item->endLocation.function,
                    item->endTimestamp.year, item->endTimestamp.month, item->endTimestamp.day,
                    item->endTimestamp.hour, item->endTimestamp.minute, item->endTimestamp.second,
                    item->endTimestamp.millisecond);

            for (int i = 0; i < item->scopeDepth; i++) {
                fprintf(output, "  ");
            }
            fprintf(output, "  Depth: %d\n", item->scopeDepth);

            fprintf(output, "\n");
        }
        fprintf(output, "\n");
    }
}

static void __picoPerfGetReportCSV(FILE *output)
{
    if (!__picoPerfGlobalContext || !output) {
        return;
    }

    fprintf(output, "RecordIndex,ItemIndex,Name,ParentName,ScopeDepth,StartTime,EndTime,DurationSeconds,DurationMilliseconds,DurationMicroseconds,DurationNanoseconds,");
    fprintf(output, "StartFile,StartFunction,StartLine,StartTimestamp,");
    fprintf(output, "EndFile,EndFunction,EndLine,EndTimestamp\n");

    for (size_t recordIdx = 0; recordIdx < __picoPerfGlobalContext->recordCount; recordIdx++) {
        __picoPerfRecord_t *record = &__picoPerfGlobalContext->records[recordIdx];

        for (size_t itemIdx = 0; itemIdx < record->itemCount; itemIdx++) {
            __picoPerfRecordItem_t *item = &record->items[itemIdx];

            double durationSec = picoPerfDurationSeconds(item->startTime, item->endTime);
            double durationMs  = picoPerfDurationMilliseconds(item->startTime, item->endTime);
            double durationUs  = picoPerfDurationMicroseconds(item->startTime, item->endTime);
            double durationNs  = picoPerfDurationNanoseconds(item->startTime, item->endTime);

            fprintf(output, "%zu,%zu,\"%s\",\"%s\",%d,%llu,%llu,%.9f,%.6f,%.3f,%.0f,",
                    recordIdx, itemIdx, __picoPerfEscapeString(item->name), __picoPerfEscapeString(item->parentName), item->scopeDepth,
                    (unsigned long long)item->startTime, (unsigned long long)item->endTime,
                    durationSec, durationMs, durationUs, durationNs);

            fprintf(output, "\"%s\",\"%s\",%u,\"%04u-%02u-%02u %02u:%02u:%02u.%03u\",",
                    __picoPerfEscapeString(item->startLocation.file), __picoPerfEscapeString(item->startLocation.function), item->startLocation.line,
                    item->startTimestamp.year, item->startTimestamp.month, item->startTimestamp.day,
                    item->startTimestamp.hour, item->startTimestamp.minute, item->startTimestamp.second,
                    item->startTimestamp.millisecond);

            fprintf(output, "\"%s\",\"%s\",%u,\"%04u-%02u-%02u %02u:%02u:%02u.%03u\"\n",
                    __picoPerfEscapeString(item->endLocation.file), __picoPerfEscapeString(item->endLocation.function), item->endLocation.line,
                    item->endTimestamp.year, item->endTimestamp.month, item->endTimestamp.day,
                    item->endTimestamp.hour, item->endTimestamp.minute, item->endTimestamp.second,
                    item->endTimestamp.millisecond);
        }
    }
}

static void __picoPerfGetReportJSON(FILE *output)
{
    if (!__picoPerfGlobalContext || !output) {
        return;
    }

    fprintf(output, "{\n");
    fprintf(output, "  \"totalRecords\": %zu,\n", __picoPerfGlobalContext->recordCount);
    fprintf(output, "  \"records\": [\n");

    for (size_t recordIdx = 0; recordIdx < __picoPerfGlobalContext->recordCount; recordIdx++) {
        __picoPerfRecord_t *record = &__picoPerfGlobalContext->records[recordIdx];

        fprintf(output, "    {\n");
        fprintf(output, "      \"recordIndex\": %zu,\n", recordIdx);
        fprintf(output, "      \"itemCount\": %zu,\n", record->itemCount);
        fprintf(output, "      \"items\": [\n");

        for (size_t itemIdx = 0; itemIdx < record->itemCount; itemIdx++) {
            __picoPerfRecordItem_t *item = &record->items[itemIdx];

            double durationSec = picoPerfDurationSeconds(item->startTime, item->endTime);
            double durationMs  = picoPerfDurationMilliseconds(item->startTime, item->endTime);
            double durationUs  = picoPerfDurationMicroseconds(item->startTime, item->endTime);
            double durationNs  = picoPerfDurationNanoseconds(item->startTime, item->endTime);

            fprintf(output, "        {\n");
            fprintf(output, "          \"itemIndex\": %zu,\n", itemIdx);
            fprintf(output, "          \"name\": \"%s\",\n", __picoPerfEscapeString(item->name));
            fprintf(output, "          \"parentName\": \"%s\",\n", __picoPerfEscapeString(item->parentName));
            fprintf(output, "          \"scopeDepth\": %d,\n", item->scopeDepth);
            fprintf(output, "          \"startTime\": %llu,\n", (unsigned long long)item->startTime);
            fprintf(output, "          \"endTime\": %llu,\n", (unsigned long long)item->endTime);
            fprintf(output, "          \"duration\": {\n");
            fprintf(output, "            \"seconds\": %.9f,\n", durationSec);
            fprintf(output, "            \"milliseconds\": %.6f,\n", durationMs);
            fprintf(output, "            \"microseconds\": %.3f,\n", durationUs);
            fprintf(output, "            \"nanoseconds\": %.0f\n", durationNs);
            fprintf(output, "          },\n");
            fprintf(output, "          \"start\": {\n");
            fprintf(output, "            \"file\": \"%s\",\n", __picoPerfEscapeString(item->startLocation.file));
            fprintf(output, "            \"function\": \"%s\",\n", __picoPerfEscapeString(item->startLocation.function));
            fprintf(output, "            \"line\": %u,\n", item->startLocation.line);
            fprintf(output, "            \"timestamp\": \"%04u-%02u-%02u %02u:%02u:%02u.%03u\"\n",
                    item->startTimestamp.year, item->startTimestamp.month, item->startTimestamp.day,
                    item->startTimestamp.hour, item->startTimestamp.minute, item->startTimestamp.second,
                    item->startTimestamp.millisecond);
            fprintf(output, "          },\n");
            fprintf(output, "          \"end\": {\n");
            fprintf(output, "            \"file\": \"%s\",\n", __picoPerfEscapeString(item->endLocation.file));
            fprintf(output, "            \"function\": \"%s\",\n", __picoPerfEscapeString(item->endLocation.function));
            fprintf(output, "            \"line\": %u,\n", item->endLocation.line);
            fprintf(output, "            \"timestamp\": \"%04u-%02u-%02u %02u:%02u:%02u.%03u\"\n",
                    item->endTimestamp.year, item->endTimestamp.month, item->endTimestamp.day,
                    item->endTimestamp.hour, item->endTimestamp.minute, item->endTimestamp.second,
                    item->endTimestamp.millisecond);
            fprintf(output, "          }\n");
            fprintf(output, "        }%s\n", (itemIdx < record->itemCount - 1) ? "," : "");
        }

        fprintf(output, "      ]\n");
        fprintf(output, "    }%s\n", (recordIdx < __picoPerfGlobalContext->recordCount - 1) ? "," : "");
    }

    fprintf(output, "  ]\n");
    fprintf(output, "}\n");
}

static void __picoPerfGetReportXML(FILE *output)
{
    if (!__picoPerfGlobalContext || !output) {
        return;
    }

    fprintf(output, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
    fprintf(output, "<PicoPerfReport>\n");
    fprintf(output, "  <Summary>\n");
    fprintf(output, "    <TotalRecords>%zu</TotalRecords>\n", __picoPerfGlobalContext->recordCount);
    fprintf(output, "  </Summary>\n");
    fprintf(output, "  <Records>\n");

    for (size_t recordIdx = 0; recordIdx < __picoPerfGlobalContext->recordCount; recordIdx++) {
        __picoPerfRecord_t *record = &__picoPerfGlobalContext->records[recordIdx];

        fprintf(output, "    <Record index=\"%zu\">\n", recordIdx);
        fprintf(output, "      <ItemCount>%zu</ItemCount>\n", record->itemCount);
        fprintf(output, "      <Items>\n");

        for (size_t itemIdx = 0; itemIdx < record->itemCount; itemIdx++) {
            __picoPerfRecordItem_t *item = &record->items[itemIdx];

            double durationSec = picoPerfDurationSeconds(item->startTime, item->endTime);
            double durationMs  = picoPerfDurationMilliseconds(item->startTime, item->endTime);
            double durationUs  = picoPerfDurationMicroseconds(item->startTime, item->endTime);
            double durationNs  = picoPerfDurationNanoseconds(item->startTime, item->endTime);

            fprintf(output, "        <Item index=\"%zu\">\n", itemIdx);
            fprintf(output, "          <Name>%s</Name>\n", item->name);
            fprintf(output, "          <ParentName>%s</ParentName>\n", item->parentName);
            fprintf(output, "          <ScopeDepth>%d</ScopeDepth>\n", item->scopeDepth);
            fprintf(output, "          <StartTime>%llu</StartTime>\n", (unsigned long long)item->startTime);
            fprintf(output, "          <EndTime>%llu</EndTime>\n", (unsigned long long)item->endTime);
            fprintf(output, "          <Duration>\n");
            fprintf(output, "            <Seconds>%.9f</Seconds>\n", durationSec);
            fprintf(output, "            <Milliseconds>%.6f</Milliseconds>\n", durationMs);
            fprintf(output, "            <Microseconds>%.3f</Microseconds>\n", durationUs);
            fprintf(output, "            <Nanoseconds>%.0f</Nanoseconds>\n", durationNs);
            fprintf(output, "          </Duration>\n");
            fprintf(output, "          <Start>\n");
            fprintf(output, "            <File>%s</File>\n", item->startLocation.file);
            fprintf(output, "            <Function>%s</Function>\n", item->startLocation.function);
            fprintf(output, "            <Line>%u</Line>\n", item->startLocation.line);
            fprintf(output, "            <Timestamp>%04u-%02u-%02u %02u:%02u:%02u.%03u</Timestamp>\n",
                    item->startTimestamp.year, item->startTimestamp.month, item->startTimestamp.day,
                    item->startTimestamp.hour, item->startTimestamp.minute, item->startTimestamp.second,
                    item->startTimestamp.millisecond);
            fprintf(output, "          </Start>\n");
            fprintf(output, "          <End>\n");
            fprintf(output, "            <File>%s</File>\n", item->endLocation.file);
            fprintf(output, "            <Function>%s</Function>\n", item->endLocation.function);
            fprintf(output, "            <Line>%u</Line>\n", item->endLocation.line);
            fprintf(output, "            <Timestamp>%04u-%02u-%02u %02u:%02u:%02u.%03u</Timestamp>\n",
                    item->endTimestamp.year, item->endTimestamp.month, item->endTimestamp.day,
                    item->endTimestamp.hour, item->endTimestamp.minute, item->endTimestamp.second,
                    item->endTimestamp.millisecond);
            fprintf(output, "          </End>\n");
            fprintf(output, "        </Item>\n");
        }

        fprintf(output, "      </Items>\n");
        fprintf(output, "    </Record>\n");
    }

    fprintf(output, "  </Records>\n");
    fprintf(output, "</PicoPerfReport>\n");
}

bool picoPerfCreateContext()
{
    if (__picoPerfGlobalContext != NULL) {
        return false;
    }

    __picoPerfGlobalContext = (picoPerfContext)PICO_MALLOC(sizeof(picoPerfContext_t));
    if (!__picoPerfGlobalContext) {
        return false;
    }
    memset(__picoPerfGlobalContext, 0, sizeof(picoPerfContext_t));

    __picoPerfGlobalContext->records = (__picoPerfRecord)PICO_MALLOC(sizeof(__picoPerfRecord_t) * PICO_PERF_MAX_RECORDS);
    if (!__picoPerfGlobalContext->records) {
        PICO_FREE(__picoPerfGlobalContext);
        __picoPerfGlobalContext = NULL;
        return false;
    }
    memset(__picoPerfGlobalContext->records, 0, sizeof(__picoPerfRecord_t) * PICO_PERF_MAX_RECORDS);
    return true;
}

void picoPerfDestroyContext()
{
    if (!__picoPerfGlobalContext) {
        return;
    }

    PICO_FREE(__picoPerfGlobalContext->records);
    PICO_FREE(__picoPerfGlobalContext);
    __picoPerfGlobalContext = NULL;
}

picoPerfContext picoPerfGetContext()
{
    return __picoPerfGlobalContext;
}

void picoPerfSetContext(picoPerfContext context)
{
    if (__picoPerfGlobalContext != NULL) {
        return;
    }

    __picoPerfGlobalContext = context;
}

picoPerfTimeStamp picoPerfGetCurrentTimestamp()
{
    picoPerfTimeStamp ts = {0};
#if defined(_WIN32) || defined(_WIN64)
    SYSTEMTIME st;
    GetLocalTime(&st);
    ts.year        = st.wYear;
    ts.month       = (uint8_t)st.wMonth;
    ts.day         = (uint8_t)st.wDay;
    ts.hour        = (uint8_t)st.wHour;
    ts.minute      = (uint8_t)st.wMinute;
    ts.second      = (uint8_t)st.wSecond;
    ts.millisecond = (uint16_t)st.wMilliseconds;
    ts.nanosecond  = 0; // Windows does not provide nanosecond precision
#elif defined(__unix__) || defined(__APPLE__)
    struct timeval tv;
    gettimeofday(&tv, NULL);
    struct tm *tm  = localtime(&tv.tv_sec);
    ts.year        = tm->tm_year + 1900;
    ts.month       = tm->tm_mon + 1;
    ts.day         = tm->tm_mday;
    ts.hour        = tm->tm_hour;
    ts.minute      = tm->tm_min;
    ts.second      = tm->tm_sec;
    ts.millisecond = tv.tv_usec / 1000;
    ts.nanosecond  = (tv.tv_usec % 1000) * 1000;
#else
#error "Unsupported platform for time functions"
#endif

    return ts;
}

picoPerfTime picoPerfNow()
{
#if defined(_WIN32) || defined(_WIN64)
    LARGE_INTEGER counter;
    QueryPerformanceCounter(&counter);
    return (picoPerfTime)counter.QuadPart;
#elif defined(__unix__) || defined(__APPLE__)
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (picoPerfTime)ts.tv_sec * 1000000000LL + (picoPerfTime)ts.tv_nsec;
#else
#error "Unsupported platform for picoPerfNow"
#endif
}

uint64_t picoPerfFrequency()
{
#if defined(_WIN32) || defined(_WIN64)
    LARGE_INTEGER freq;
    QueryPerformanceFrequency(&freq);
    return (uint64_t)freq.QuadPart;
#elif defined(__unix__) || defined(__APPLE__)
    return 1000000000LL; // Nanoseconds
#else
#error "Unsupported platform for picoPerfFrequency"
#endif
}

double picoPerfDurationSeconds(picoPerfTime start, picoPerfTime end)
{
    uint64_t freq = picoPerfFrequency();
    if (freq == 0) {
        return 0.0;
    }
    return (double)(end - start) / (double)freq;
}

double picoPerfDurationMilliseconds(picoPerfTime start, picoPerfTime end)
{
    return picoPerfDurationSeconds(start, end) * 1000.0;
}

double picoPerfDurationMicroseconds(picoPerfTime start, picoPerfTime end)
{
    return picoPerfDurationSeconds(start, end) * 1000000.0;
}

double picoPerfDurationNanoseconds(picoPerfTime start, picoPerfTime end)
{
    return picoPerfDurationSeconds(start, end) * 1000000000.0;
}

void picoPerfFormatDuration(picoPerfTime duration, char *buffer, size_t bufferSize)
{
    if (bufferSize == 0) {
        return;
    }

    uint64_t freq = picoPerfFrequency();
    if (freq == 0) {
        snprintf(buffer, bufferSize, "0s");
        return;
    }

    double seconds = (double)duration / (double)freq;

    if (seconds >= 1.0) {
        snprintf(buffer, bufferSize, "%.3fs", seconds);
    } else if (seconds >= 0.001) {
        snprintf(buffer, bufferSize, "%.3fms", seconds * 1000.0);
    } else if (seconds >= 0.000001) {
        snprintf(buffer, bufferSize, "%.3fµs", seconds * 1000000.0);
    } else {
        snprintf(buffer, bufferSize, "%.3fns", seconds * 1000000000.0);
    }
}

void picoPerfSleep(uint32_t milliseconds)
{
#if defined(_WIN32) || defined(_WIN64)
    Sleep(milliseconds);
#elif defined(__unix__) || defined(__APPLE__)
    struct timespec ts;
    ts.tv_sec  = milliseconds / 1000;
    ts.tv_nsec = (milliseconds % 1000) * 1000000;
    nanosleep(&ts, NULL);
#else
#error "Unsupported platform for picoPerfSleep"
#endif
}

bool picoPerfBeginRecord()
{
    if (!__picoPerfGlobalContext || __picoPerfGlobalContext->recording) {
        return false;
    }

    if (__picoPerfGlobalContext->recordCount >= PICO_PERF_MAX_RECORDS) {
        return false;
    }

    __picoPerfGlobalContext->recording     = true;
    __picoPerfGlobalContext->scopeStackTop = 0;

    memset(&__picoPerfGlobalContext->records[__picoPerfGlobalContext->recordHead], 0, sizeof(__picoPerfRecord_t));
    memset(&__picoPerfGlobalContext->currentRecord, 0, sizeof(__picoPerfRecord_t));

    return true;
}

void picoPerfEndRecord()
{
    if (!__picoPerfGlobalContext || !__picoPerfGlobalContext->recording) {
        return;
    }

    __picoPerfGlobalContext->recording                                    = false;
    __picoPerfGlobalContext->records[__picoPerfGlobalContext->recordHead] = __picoPerfGlobalContext->currentRecord;
    __picoPerfGlobalContext->recordHead                                   = (__picoPerfGlobalContext->recordHead + 1) % PICO_PERF_MAX_RECORDS;

    if (__picoPerfGlobalContext->recordCount < PICO_PERF_MAX_RECORDS) {
        __picoPerfGlobalContext->recordCount++;
    }
}

void picoPerfPushScope(const char *name, const char *file, const char *function, uint32_t line)
{
    if (!__picoPerfGlobalContext || !__picoPerfGlobalContext->recording) {
        return;
    }

    if (__picoPerfGlobalContext->scopeStackTop >= PICO_PERF_MAX_SCOPES) {
        return;
    }

    __picoPerfRecordItem_t *item = &__picoPerfGlobalContext->scopeStack[__picoPerfGlobalContext->scopeStackTop];
    memset(item, 0, sizeof(__picoPerfRecordItem_t));

    strncpy(item->name, name, PICO_PERF_MAX_NAME_LENGTH - 1);
    item->startTime          = picoPerfNow();
    item->startLocation.line = line;
    strncpy(item->startLocation.file, file ? file : "unknown", PICO_PERF_MAX_PATH - 1);
    strncpy(item->startLocation.function, function ? function : "unknown", PICO_PERF_MAX_PATH - 1);
    item->startTimestamp = picoPerfGetCurrentTimestamp();
    item->scopeDepth     = __picoPerfGlobalContext->scopeStackTop;

    if (__picoPerfGlobalContext->scopeStackTop > 0) {
        __picoPerfRecordItem_t *parentItem = &__picoPerfGlobalContext->scopeStack[__picoPerfGlobalContext->scopeStackTop - 1];
        strncpy(item->parentName, parentItem->name, PICO_PERF_MAX_NAME_LENGTH - 1);
    } else {
        snprintf(item->parentName, PICO_PERF_MAX_NAME_LENGTH - 1, "ROOT");
    }

    __picoPerfGlobalContext->scopeStackTop++;
}

void picoPerfPopScope(const char *file, const char *function, uint32_t line)
{
    if (!__picoPerfGlobalContext || !__picoPerfGlobalContext->recording) {
        return;
    }

    if (__picoPerfGlobalContext->scopeStackTop <= 0) {
        return;
    }

    if (__picoPerfGlobalContext->currentRecord.itemCount >= PICO_PERF_MAX_SCOPES) {
        return;
    }

    __picoPerfGlobalContext->scopeStackTop--;

    __picoPerfRecordItem_t *stackItem = &__picoPerfGlobalContext->scopeStack[__picoPerfGlobalContext->scopeStackTop];

    stackItem->endTime          = picoPerfNow();
    stackItem->endLocation.line = line;
    strncpy(stackItem->endLocation.file, file ? file : "unknown", PICO_PERF_MAX_PATH - 1);
    strncpy(stackItem->endLocation.function, function ? function : "unknown", PICO_PERF_MAX_PATH - 1);
    stackItem->endTimestamp = picoPerfGetCurrentTimestamp();

    __picoPerfRecordItem_t *recordItem = &__picoPerfGlobalContext->currentRecord.items[__picoPerfGlobalContext->currentRecord.itemCount];
    memcpy(recordItem, stackItem, sizeof(__picoPerfRecordItem_t));
    __picoPerfGlobalContext->currentRecord.itemCount++;
}

void picoPerfPopNScopes(int count, const char *file, const char *function, uint32_t line)
{
    if (!__picoPerfGlobalContext || !__picoPerfGlobalContext->recording) {
        return;
    }

    if (count < 0) {
        count = __picoPerfGlobalContext->scopeStackTop;
    }

    for (int i = 0; i < count; i++) {
        picoPerfPopScope(file, function, line);
    }
}

void picoPerfGetReport(FILE *output, picoPerfReportFormat format)
{
    if (!__picoPerfGlobalContext || !output) {
        return;
    }

    switch (format) {
        case PICO_PERF_REPORT_FORMAT_TEXT:
            __picoPerfGetReportText(output);
            break;
        case PICO_PERF_REPORT_FORMAT_CSV:
            __picoPerfGetReportCSV(output);
            break;
        case PICO_PERF_REPORT_FORMAT_JSON:
            __picoPerfGetReportJSON(output);
            break;
        case PICO_PERF_REPORT_FORMAT_XML:
            __picoPerfGetReportXML(output);
            break;
        default:
            __picoPerfGetReportText(output);
            break;
    }
}

#endif // PICO_PERF_IMPLEMENTATION

#endif // PICO_PERF_H