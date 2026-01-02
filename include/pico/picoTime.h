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

// NOTE: This library isnt meant for timing for profiing. It is designed for general purpose timing needs.
//       If your goal is tracking performance, please use picoPerf.h or some other profiling library suited for that purpose.
//       This library provides basic timing utilities and timers for general use cases. As well as provides a way too
//       easily create timers with callbacks hooked up to the OS.
//       Also it is to be noted that, since this library uses OS level timers, the resolution and accuracy of the timers
//       is dependent on the underlying OS timer implementations and their limitations.
//       For instance, Windows timers have a minimum resolution of 1ms, so any timer set with interval less than that
//       will effectively be clamped to 1ms.

#ifndef PICO_TIME_H
#define PICO_TIME_H

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef PICO_MALLOC
#define PICO_MALLOC malloc
#define PICO_FREE   free
#endif

#ifndef PICO_REALLOC
#define PICO_REALLOC realloc
#endif

#ifndef PICO_ASSERT
#define PICO_ASSERT(cond) assert(cond)
#endif

typedef enum {
    PICO_TIME_ISO_CALENDAR_EXTENDED,          // YYYY-MM-DD
    PICO_TIME_ISO_CALENDAR_BASIC,             // YYYYMMDD
    PICO_TIME_ISO_ORDINAL_EXTENDED,           // YYYY-DDD
    PICO_TIME_ISO_ORDINAL_BASIC,              // YYYYDDD
    PICO_TIME_ISO_WEEK_EXTENDED,              // YYYY-Www-D
    PICO_TIME_ISO_WEEK_BASIC,                 // YYYYWwwD
    PICO_TIME_ISO_TIME_EXTENDED,              // hh:mm:ss
    PICO_TIME_ISO_TIME_BASIC,                 // hhmmss
    PICO_TIME_ISO_TIME_EXTENDED_FRAC,         // hh:mm:ss.sss (milliseconds)
    PICO_TIME_ISO_TIME_BASIC_FRAC,            // hhmmss.sss (milliseconds)
    PICO_TIME_ISO_DATETIME_EXTENDED,          // YYYY-MM-DDThh:mm:ss
    PICO_TIME_ISO_DATETIME_BASIC,             // YYYYMMDDThhmmss
    PICO_TIME_ISO_DATETIME_EXTENDED_FRAC,     // YYYY-MM-DDThh:mm:ss.sss
    PICO_TIME_ISO_DATETIME_BASIC_FRAC,        // YYYYMMDDThhmmss.sss
    PICO_TIME_ISO_DATETIME_EXTENDED_UTC,      // YYYY-MM-DDThh:mm:ssZ
    PICO_TIME_ISO_DATETIME_BASIC_UTC,         // YYYYMMDDThhmmssZ
    PICO_TIME_ISO_DATETIME_EXTENDED_FRAC_UTC, // YYYY-MM-DDThh:mm:ss.sssZ
    PICO_TIME_ISO_DATETIME_BASIC_FRAC_UTC,    // YYYYMMDDThhmmss.sssZ
    PICO_TIME_ISO_FORMAT_UNKNOWN
} picoTimeISOFormat;

typedef struct {
    uint16_t year;
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
    uint16_t millisecond;
    uint32_t nanosecond;
} picoTime_t;

typedef struct picoTimeTimer_t picoTimeTimer_t;
typedef picoTimeTimer_t *picoTimeTimer;

typedef void (*picoTimeTimerTriggerCallback)(picoTimeTimer_t *clock, void *userData);

// These are purely fro type safety and clarity.
typedef uint64_t picoTimeDurationNano_t;
typedef uint64_t picoTimeDurationMicro_t;
typedef uint64_t picoTimeDurationMilli_t;
typedef uint64_t picoTimeDurationSeconds_t;
typedef uint64_t picoTimeDurationMinutes_t;
typedef uint64_t picoTimeDurationHours_t;

picoTime_t picoTimeGetCurrent(void);

picoTime_t picoTimeFromNano(picoTimeDurationNano_t nanoseconds);
picoTime_t picoTimeFromMicro(picoTimeDurationMicro_t microseconds);
picoTime_t picoTimeFromMilli(picoTimeDurationMilli_t milliseconds);
picoTime_t picoTimeFromSeconds(picoTimeDurationSeconds_t seconds);
picoTime_t picoTimeFromMinutes(picoTimeDurationMinutes_t minutes);
picoTime_t picoTimeFromHours(picoTimeDurationHours_t hours);

picoTimeDurationNano_t picoTimeGetDurationNano(picoTime_t start, picoTime_t end);
picoTimeDurationMicro_t picoTimeGetDurationMicro(picoTime_t start, picoTime_t end);
picoTimeDurationMilli_t picoTimeGetDurationMilli(picoTime_t start, picoTime_t end);
picoTimeDurationSeconds_t picoTimeGetDurationSeconds(picoTime_t start, picoTime_t end);
picoTimeDurationMinutes_t picoTimeGetDurationMinutes(picoTime_t start, picoTime_t end);
picoTimeDurationHours_t picoTimeGetDurationHours(picoTime_t start, picoTime_t end);

picoTimeTimer picoTimeTimerCreate(void);
void picoTimeTimerDestroy(picoTimeTimer_t *clock);

void picoTimeTimerRestart(picoTimeTimer_t *clock);
void picoTimeTimerStop(picoTimeTimer_t *clock);
bool picoTimeTimerIsRunning(picoTimeTimer_t *clock);
void picoTimeTimerSetCallback(picoTimeTimer_t *clock, picoTimeTimerTriggerCallback callback);
void picoTimeTimerSetUserData(picoTimeTimer_t *clock, void *userData);
void picoTimeTimerSetRepeat(picoTimeTimer_t *clock, bool repeat);
void *picoTimeTimerGetUserData(picoTimeTimer_t *clock);
uint64_t picoTimeTimerGetTriggerCount(picoTimeTimer_t *clock);

void picoTimeTimerSetIntervalNano(picoTimeTimer_t *clock, picoTimeDurationNano_t nanoseconds);
void picoTimeTimerSetIntervalMicro(picoTimeTimer_t *clock, picoTimeDurationMicro_t microseconds);
void picoTimeTimerSetIntervalMilli(picoTimeTimer_t *clock, picoTimeDurationMilli_t milliseconds);
void picoTimeTimerSetIntervalSeconds(picoTimeTimer_t *clock, picoTimeDurationSeconds_t seconds);
void picoTimeTimerSetIntervalMinutes(picoTimeTimer_t *clock, picoTimeDurationMinutes_t minutes);

picoTimeDurationNano_t picoTimeTimerElapsedNano(picoTimeTimer_t *clock);
picoTimeDurationMicro_t picoTimeTimerElapsedMicro(picoTimeTimer_t *clock);
picoTimeDurationMilli_t picoTimeTimerElapsedMilli(picoTimeTimer_t *clock);
picoTimeDurationSeconds_t picoTimeTimerElapsedSeconds(picoTimeTimer_t *clock);
picoTimeDurationMinutes_t picoTimeTimerElapsedMinutes(picoTimeTimer_t *clock);
picoTimeDurationHours_t picoTimeTimerElapsedHours(picoTimeTimer_t *clock);

void picoTimeSleep(picoTimeDurationMicro_t microseconds);

const char *picoTimeISOFormatToString(picoTimeISOFormat format);

// Formats the given picoTime_t into a string.
// if the buffer is not large enough, the output will be truncated.
bool picoTimeFormat(const picoTime_t *time, char *buffer, size_t bufferSize);
bool picoTimeFormatISO(const picoTime_t *time, picoTimeISOFormat format, char *buffer, size_t bufferSize);

// Parses the given ISO formatted string into a picoTime_t, also it automatically detects the format of the string.
// if the string is not a valid ISO formatted string, the function will return false.
bool picoTimeParseISO(const char *isoString, picoTime_t *outTime, picoTimeISOFormat *format);

#define PICO_IMPLEMENTATION

#if defined(PICO_IMPLEMENTATION) && !defined(PICO_TIME_IMPLEMENTATION)
#define PICO_TIME_IMPLEMENTATION
#endif

#if defined(PICO_TIME_IMPLEMENTATION)

#if defined(_WIN32) || defined(_WIN64)
#include <Windows.h>
#else
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#endif

struct picoTimeTimer_t {
    picoTime_t startTime;
    picoTimeDurationNano_t tickInterval;
    picoTimeTimerTriggerCallback callback;
    bool isRunning;
    bool repeat;
    void *userData;
    uint64_t triggerCount;
#if defined(_WIN32) || defined(_WIN64)
    HANDLE timerHandle;
    HANDLE timerQueueHandle;
#else
    // POSIX timer implementation can be added here in future.
    timer_t timerHandle;
    void *timerQueueHandle;
#endif
};

static bool __picoTimeRecreateTimer(picoTimeTimer_t *clock, bool start);

static bool __picoTimeIsLeapYear(uint16_t year)
{
    return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

static uint32_t __picoTimeDaysInMonth(uint8_t month, uint16_t year)
{
    PICO_ASSERT(month >= 1 && month <= 12);

    static const uint32_t daysInMonth[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if (month < 1 || month > 12) {
        return 0;
    }
    if (month == 2 && __picoTimeIsLeapYear(year)) {
        return 29;
    }
    return daysInMonth[month - 1];
}

static uint64_t __picoTimeDaysSinceEpoch(uint16_t year, uint8_t month, uint8_t day)
{
    PICO_ASSERT(month >= 1 && month <= 12);
    PICO_ASSERT(day >= 1 && day <= __picoTimeDaysInMonth(month, year));

    uint64_t days = 0;
    for (uint16_t y = 1; y < year; y++) {
        days += __picoTimeIsLeapYear(y) ? 366 : 365;
    }

    for (uint8_t m = 1; m < month; m++) {
        days += __picoTimeDaysInMonth(m, year);
    }

    days += day;
    return days;
}

static picoTimeDurationNano_t __picoTimeToNano(picoTime_t time)
{
    uint64_t totalDays = __picoTimeDaysSinceEpoch(time.year, time.month, time.day);

    picoTimeDurationNano_t nano = 0;
    nano += totalDays * 86400ULL * 1000000000ULL;
    nano += (picoTimeDurationNano_t)time.hour * 3600ULL * 1000000000ULL;
    nano += (picoTimeDurationNano_t)time.minute * 60ULL * 1000000000ULL;
    nano += (picoTimeDurationNano_t)time.second * 1000000000ULL;
    nano += (picoTimeDurationNano_t)time.millisecond * 1000000ULL;
    nano += (picoTimeDurationNano_t)time.nanosecond;

    return nano;
}

picoTime_t picoTimeGetCurrent(void)
{
    picoTime_t time = {0};
#if defined(_WIN32) || defined(_WIN64)
    SYSTEMTIME st;
    GetLocalTime(&st);
    time.year        = st.wYear;
    time.month       = (uint8_t)st.wMonth;
    time.day         = (uint8_t)st.wDay;
    time.hour        = (uint8_t)st.wHour;
    time.minute      = (uint8_t)st.wMinute;
    time.second      = (uint8_t)st.wSecond;
    time.millisecond = st.wMilliseconds;
    time.nanosecond  = 0; // Windows does not provide nanosecond precision
#else
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    struct tm *tm    = localtime(&ts.tv_sec);
    time.year        = (uint16_t)(tm->tm_year + 1900);
    time.month       = (uint8_t)(tm->tm_mon + 1);
    time.day         = (uint8_t)tm->tm_mday;
    time.hour        = (uint8_t)tm->tm_hour;
    time.minute      = (uint8_t)tm->tm_min;
    time.second      = (uint8_t)tm->tm_sec;
    time.millisecond = (uint16_t)(ts.tv_nsec / 1000000);
    time.nanosecond  = (uint32_t)ts.tv_nsec % 1000000;
#endif
    return time;
}

picoTime_t picoTimeFromNano(picoTimeDurationNano_t nanoseconds)
{
    picoTime_t time = {0};

    time.nanosecond = (uint32_t)(nanoseconds % 1000000000ULL);

    uint64_t totalSeconds = nanoseconds / 1000000000ULL;
    time.millisecond      = (uint16_t)((nanoseconds / 1000000ULL) % 1000ULL);
    time.second           = (uint8_t)(totalSeconds % 60);
    totalSeconds /= 60;
    time.minute = (uint8_t)(totalSeconds % 60);
    totalSeconds /= 60;
    time.hour = (uint8_t)(totalSeconds % 24);
    totalSeconds /= 24;
    uint64_t totalDays = totalSeconds;
    uint16_t year      = 1;
    while (totalDays > 0) {
        uint32_t daysInYear = __picoTimeIsLeapYear(year) ? 366 : 365;
        if (totalDays < daysInYear) {
            break;
        }
        totalDays -= daysInYear;
        year++;
    }
    time.year     = year;
    uint8_t month = 1;
    while (totalDays > 0 && month <= 12) {
        uint32_t daysInMonth = __picoTimeDaysInMonth(month, year);
        if (totalDays < daysInMonth) {
            break;
        }
        totalDays -= daysInMonth;
        month++;
    }
    time.month = month;
    time.day   = (uint8_t)(totalDays + 1);

    return time;
}

picoTime_t picoTimeFromMicro(picoTimeDurationMicro_t microseconds)
{
    return picoTimeFromNano(microseconds * 1000ULL);
}

picoTime_t picoTimeFromMilli(picoTimeDurationMilli_t milliseconds)
{
    return picoTimeFromNano(milliseconds * 1000000ULL);
}

picoTime_t picoTimeFromSeconds(picoTimeDurationSeconds_t seconds)
{
    return picoTimeFromNano(seconds * 1000000000ULL);
}

picoTime_t picoTimeFromMinutes(picoTimeDurationMinutes_t minutes)
{
    return picoTimeFromNano(minutes * 60ULL * 1000000000ULL);
}

picoTime_t picoTimeFromHours(picoTimeDurationHours_t hours)
{
    return picoTimeFromNano(hours * 3600ULL * 1000000000ULL);
}

picoTimeDurationNano_t picoTimeGetDurationNano(picoTime_t start, picoTime_t end)
{
    picoTimeDurationNano_t startNano = __picoTimeToNano(start);
    picoTimeDurationNano_t endNano   = __picoTimeToNano(end);
    if (endNano >= startNano) {
        return endNano - startNano;
    }
    return 0;
}

picoTimeDurationMicro_t picoTimeGetDurationMicro(picoTime_t start, picoTime_t end)
{
    return picoTimeGetDurationNano(start, end) / 1000ULL;
}

picoTimeDurationMilli_t picoTimeGetDurationMilli(picoTime_t start, picoTime_t end)
{
    return picoTimeGetDurationNano(start, end) / 1000000ULL;
}

picoTimeDurationSeconds_t picoTimeGetDurationSeconds(picoTime_t start, picoTime_t end)
{
    return picoTimeGetDurationNano(start, end) / 1000000000ULL;
}

picoTimeDurationMinutes_t picoTimeGetDurationMinutes(picoTime_t start, picoTime_t end)
{
    return picoTimeGetDurationNano(start, end) / (60ULL * 1000000000ULL);
}

picoTimeDurationHours_t picoTimeGetDurationHours(picoTime_t start, picoTime_t end)
{
    return picoTimeGetDurationNano(start, end) / (3600ULL * 1000000000ULL);
}

void picoTimeTimerStop(picoTimeTimer_t *clock)
{
    PICO_ASSERT(clock != NULL);

    __picoTimeRecreateTimer(clock, false);
}

bool picoTimeTimerIsRunning(picoTimeTimer_t *clock)
{
    PICO_ASSERT(clock != NULL);

    return clock->isRunning;
}

void picoTimeTimerRestart(picoTimeTimer_t *clock)
{
    PICO_ASSERT(clock != NULL);

    clock->startTime = picoTimeGetCurrent();
    __picoTimeRecreateTimer(clock, true);
}

void picoTimeTimerSetCallback(picoTimeTimer_t *clock, picoTimeTimerTriggerCallback callback)
{
    PICO_ASSERT(clock != NULL);

    clock->callback = callback;
}

void picoTimeTimerSetUserData(picoTimeTimer_t *clock, void *userData)
{
    PICO_ASSERT(clock != NULL);

    clock->userData = userData;
}

void picoTimeTimerSetRepeat(picoTimeTimer_t *clock, bool repeat)
{
    PICO_ASSERT(clock != NULL);

    clock->repeat = repeat;
    __picoTimeRecreateTimer(clock, clock->isRunning);
}

void *picoTimeTimerGetUserData(picoTimeTimer_t *clock)
{
    PICO_ASSERT(clock != NULL);

    return clock->userData;
}

void picoTimeTimerSetIntervalNano(picoTimeTimer_t *clock, picoTimeDurationNano_t nanoseconds)
{
    PICO_ASSERT(clock != NULL);

    clock->tickInterval = nanoseconds;
    __picoTimeRecreateTimer(clock, clock->isRunning);
}

void picoTimeTimerSetIntervalMicro(picoTimeTimer_t *clock, picoTimeDurationMicro_t microseconds)
{
    picoTimeTimerSetIntervalNano(clock, microseconds * 1000ULL);
}

void picoTimeTimerSetIntervalMilli(picoTimeTimer_t *clock, picoTimeDurationMilli_t milliseconds)
{
    picoTimeTimerSetIntervalNano(clock, milliseconds * 1000000ULL);
}

void picoTimeTimerSetIntervalSeconds(picoTimeTimer_t *clock, picoTimeDurationSeconds_t seconds)
{
    picoTimeTimerSetIntervalNano(clock, seconds * 1000000000ULL);
}

void picoTimeTimerSetIntervalMinutes(picoTimeTimer_t *clock, picoTimeDurationMinutes_t minutes)
{
    picoTimeTimerSetIntervalNano(clock, minutes * 60ULL * 1000000000ULL);
}

picoTimeDurationNano_t picoTimeTimerElapsedNano(picoTimeTimer_t *clock)
{
    PICO_ASSERT(clock != NULL);

    picoTime_t current = picoTimeGetCurrent();
    return picoTimeGetDurationNano(clock->startTime, current);
}

picoTimeDurationMicro_t picoTimeTimerElapsedMicro(picoTimeTimer_t *clock)
{
    return picoTimeTimerElapsedNano(clock) / 1000ULL;
}

picoTimeDurationMilli_t picoTimeTimerElapsedMilli(picoTimeTimer_t *clock)
{
    return picoTimeTimerElapsedNano(clock) / 1000000ULL;
}

picoTimeDurationSeconds_t picoTimeTimerElapsedSeconds(picoTimeTimer_t *clock)
{
    return picoTimeTimerElapsedNano(clock) / 1000000000ULL;
}

picoTimeDurationMinutes_t picoTimeTimerElapsedMinutes(picoTimeTimer_t *clock)
{
    return picoTimeTimerElapsedNano(clock) / (60ULL * 1000000000ULL);
}

picoTimeDurationHours_t picoTimeTimerElapsedHours(picoTimeTimer_t *clock)
{
    return picoTimeTimerElapsedNano(clock) / (3600ULL * 1000000000ULL);
}

uint64_t picoTimeTimerGetTriggerCount(picoTimeTimer_t *clock)
{
    PICO_ASSERT(clock != NULL);

    return clock->triggerCount;
}

#if defined(_WIN32) || defined(_WIN64)
static void CALLBACK __picoTimeTimerCallback(PVOID lpParameter, BOOLEAN timerOrWaitFired)
{
    (void)timerOrWaitFired;
    picoTimeTimer clock = (picoTimeTimer)lpParameter;
    PICO_ASSERT(clock != NULL);

    if (!clock->isRunning) {
        return;
    }

    clock->triggerCount++;

    if (clock->callback != NULL) {
        clock->callback(clock, clock->userData);
    }

    if (!clock->repeat) {
        clock->isRunning = false;
    }
}

static bool __picoTimeRecreateTimer(picoTimeTimer_t *clock, bool start)
{
    PICO_ASSERT(clock != NULL);

    if (clock->timerHandle != NULL) {
        DeleteTimerQueueTimer(
            clock->timerQueueHandle,
            clock->timerHandle,
            NULL);
        clock->timerHandle = NULL;
    }
    clock->isRunning = false;

    if (clock->tickInterval == 0) {
        return false;
    }

    if (start) {

        DWORD intervalMs = (DWORD)(clock->tickInterval / 1000000ULL);
        if (intervalMs == 0) {
            intervalMs = 1;
        }

        DWORD dueTime = intervalMs;

        DWORD period     = clock->repeat ? intervalMs : 0;
        clock->isRunning = true;

        HANDLE timerHandle = NULL;
        BOOL result        = CreateTimerQueueTimer(
            &timerHandle,
            clock->timerQueueHandle,
            __picoTimeTimerCallback,
            clock,
            dueTime,
            period,
            WT_EXECUTEDEFAULT);

        if (!result) {
            return false;
        }

        clock->timerHandle = timerHandle;
    }

    return true;
}

picoTimeTimer picoTimeTimerCreate(void)
{
    picoTimeTimer clock = (picoTimeTimer)PICO_MALLOC(sizeof(picoTimeTimer_t));
    if (clock == NULL) {
        return NULL;
    }
    memset(clock, 0, sizeof(picoTimeTimer_t));

    clock->timerQueueHandle = CreateTimerQueue();
    if (clock->timerQueueHandle == NULL) {
        PICO_FREE(clock);
        return NULL;
    }

    clock->timerHandle = NULL;
    clock->isRunning   = false;
    clock->startTime   = picoTimeGetCurrent();

    return clock;
}

void picoTimeTimerDestroy(picoTimeTimer_t *clock)
{
    PICO_ASSERT(clock != NULL);
    clock->isRunning = false;

    if (clock->timerHandle != NULL) {
        DeleteTimerQueueTimer(
            clock->timerQueueHandle,
            clock->timerHandle,
            INVALID_HANDLE_VALUE);
    }

    if (clock->timerQueueHandle != NULL) {
        DeleteTimerQueue(clock->timerQueueHandle);
    }
    PICO_FREE(clock);
}

#else // Unix/Linux/macOS

#include <pthread.h>
#include <signal.h>
#include <time.h>

static void __picoTimeTimerCallback(union sigval sv)
{
    picoTimeTimer clock = (picoTimeTimer)sv.sival_ptr;
    PICO_ASSERT(clock != NULL);

    if (!clock->isRunning) {
        return;
    }

    clock->triggerCount++;

    if (clock->callback != NULL) {
        clock->callback(clock, clock->userData);
    }

    if (!clock->repeat) {
        clock->isRunning = false;
    }
}

static bool __picoTimeRecreateTimer(picoTimeTimer_t *clock, bool start)
{
    PICO_ASSERT(clock != NULL);

    if (clock->timerHandle != NULL) {
        timer_delete((timer_t)clock->timerHandle);
        clock->timerHandle = NULL;
    }
    clock->isRunning = false;

    if (clock->tickInterval == 0) {
        return false;
    }

    if (start) {
        struct sigevent sev;
        struct itimerspec its;
        timer_t timerId;

        memset(&sev, 0, sizeof(sev));
        sev.sigev_notify            = SIGEV_THREAD;
        sev.sigev_notify_function   = __picoTimeTimerCallback;
        sev.sigev_notify_attributes = NULL;
        sev.sigev_value.sival_ptr   = (void *)clock;

        if (timer_create(CLOCK_MONOTONIC, &sev, &timerId) == -1) {
            return false;
        }

        uint64_t seconds     = clock->tickInterval / 1000000000ULL;
        uint64_t nanoseconds = clock->tickInterval % 1000000000ULL;

        its.it_value.tv_sec  = (time_t)seconds;
        its.it_value.tv_nsec = (long)nanoseconds;

        if (clock->repeat) {
            its.it_interval.tv_sec  = (time_t)seconds;
            its.it_interval.tv_nsec = (long)nanoseconds;
        } else {
            its.it_interval.tv_sec  = 0;
            its.it_interval.tv_nsec = 0;
        }

        if (timer_settime(timerId, 0, &its, NULL) == -1) {
            timer_delete(timerId);
            return false;
        }

        clock->isRunning   = true;
        clock->timerHandle = timerId;
    }

    return true;
}

picoTimeTimer picoTimeTimerCreate(void)
{
    picoTimeTimer clock = (picoTimeTimer)PICO_MALLOC(sizeof(picoTimeTimer_t));
    if (clock == NULL) {
        return NULL;
    }
    memset(clock, 0, sizeof(picoTimeTimer_t));

    clock->timerHandle      = NULL;
    clock->timerQueueHandle = NULL;
    clock->isRunning        = false;
    clock->startTime        = picoTimeGetCurrent();

    return clock;
}

void picoTimeTimerDestroy(picoTimeTimer_t *clock)
{
    PICO_ASSERT(clock != NULL);

    clock->isRunning = false;

    if (clock->timerHandle != NULL) {
        timer_delete(clock->timerHandle);
        clock->timerHandle = NULL;
    }

    PICO_FREE(clock);
}

#endif

void picoTimeSleep(picoTimeDurationMicro_t microseconds)
{
#if defined(_WIN32) || defined(_WIN64)
    DWORD ms = (DWORD)(microseconds / 1000);
    if (ms == 0 && microseconds > 0) {
        ms = 1;
    }
    Sleep(ms);
#else
    if (microseconds >= 1000000) {
        unsigned int secs = (unsigned int)(microseconds / 1000000);
        useconds_t usecs  = (useconds_t)(microseconds % 1000000);
        sleep(secs);
        if (usecs > 0) {
            usleep(usecs);
        }
    } else {
        usleep((useconds_t)microseconds);
    }
#endif
}

bool picoTimeFormat(const picoTime_t *time, char *buffer, size_t bufferSize)
{
    if (time == NULL || buffer == NULL || bufferSize == 0) {
        return false;
    }

    int written = snprintf(
        buffer,
        bufferSize,
        "%04u-%02u-%02u %02u:%02u:%02u.%03u%03u",
        time->year,
        time->month,
        time->day,
        time->hour,
        time->minute,
        time->second,
        time->millisecond,
        time->nanosecond / 1000);

    return (written > 0 && (size_t)written < bufferSize);
}

static uint16_t __picoTimeGetDayOfYear(uint16_t year, uint8_t month, uint8_t day)
{
    uint16_t dayOfYear = 0;
    for (uint8_t m = 1; m < month; m++) {
        dayOfYear += (uint16_t)__picoTimeDaysInMonth(m, year);
    }
    dayOfYear += day;
    return dayOfYear;
}

static uint8_t __picoTimeGetDayOfWeek(uint16_t year, uint8_t month, uint8_t day)
{
    uint16_t y = year;
    uint8_t m  = month;
    if (m < 3) {
        m += 12;
        y--;
    }

    uint16_t k = y % 100;
    uint16_t j = y / 100;

    int h = (day + (13 * (m + 1)) / 5 + k + k / 4 + j / 4 - 2 * j) % 7;

    int dow = ((h + 6) % 7);
    return (uint8_t)dow;
}

static void __picoTimeGetISOWeek(uint16_t year, uint8_t month, uint8_t day,
                                 uint16_t *outISOYear, uint8_t *outWeek, uint8_t *outDayOfWeek)
{
    uint8_t dow    = __picoTimeGetDayOfWeek(year, month, day);
    uint8_t isoDow = (dow == 0) ? 7 : dow;

    uint16_t dayOfYear = __picoTimeGetDayOfYear(year, month, day);

    int thursdayDayOfYear = (int)dayOfYear - (int)isoDow + 4;

    uint16_t isoYear = year;

    if (thursdayDayOfYear < 1) {
        isoYear                 = year - 1;
        uint16_t daysInPrevYear = __picoTimeIsLeapYear(isoYear) ? 366 : 365;
        thursdayDayOfYear += (int)daysInPrevYear;
    } else {
        uint16_t daysInYear = __picoTimeIsLeapYear(year) ? 366 : 365;
        if (thursdayDayOfYear > (int)daysInYear) {
            isoYear = year + 1;
            thursdayDayOfYear -= (int)daysInYear;
        }
    }

    uint8_t jan4Dow    = __picoTimeGetDayOfWeek(isoYear, 1, 4);
    uint8_t isoJan4Dow = (jan4Dow == 0) ? 7 : jan4Dow;
    int week1Start     = 4 - isoJan4Dow + 1; // Day of year when week 1 starts

    int weekNum = (thursdayDayOfYear - week1Start) / 7 + 1;

    *outISOYear   = isoYear;
    *outWeek      = (uint8_t)weekNum;
    *outDayOfWeek = isoDow;
}

static bool __picoTimeFromOrdinal(uint16_t year, uint16_t dayOfYear,
                                  uint8_t *outMonth, uint8_t *outDay)
{
    uint16_t daysInYear = __picoTimeIsLeapYear(year) ? 366 : 365;
    if (dayOfYear < 1 || dayOfYear > daysInYear) {
        return false;
    }

    uint16_t remaining = dayOfYear;
    for (uint8_t m = 1; m <= 12; m++) {
        uint32_t daysInMonth = __picoTimeDaysInMonth(m, year);
        if (remaining <= daysInMonth) {
            *outMonth = m;
            *outDay   = (uint8_t)remaining;
            return true;
        }
        remaining -= (uint16_t)daysInMonth;
    }
    return false;
}

static bool __picoTimeFromISOWeek(uint16_t isoYear, uint8_t week, uint8_t dayOfWeek,
                                  uint16_t *outYear, uint8_t *outMonth, uint8_t *outDay)
{
    if (week < 1 || week > 53 || dayOfWeek < 1 || dayOfWeek > 7) {
        return false;
    }

    uint8_t jan4Dow    = __picoTimeGetDayOfWeek(isoYear, 1, 4);
    uint8_t isoJan4Dow = (jan4Dow == 0) ? 7 : jan4Dow;

    int week1MondayDoy = 4 - isoJan4Dow + 1;
    int targetDoy      = week1MondayDoy + (week - 1) * 7 + (dayOfWeek - 1);

    uint16_t year = isoYear;

    if (targetDoy < 1) {
        year                    = isoYear - 1;
        uint16_t daysInPrevYear = __picoTimeIsLeapYear(year) ? 366 : 365;
        targetDoy += (int)daysInPrevYear;
    } else {
        uint16_t daysInYear = __picoTimeIsLeapYear(isoYear) ? 366 : 365;
        if (targetDoy > (int)daysInYear) {
            year = isoYear + 1;
            targetDoy -= (int)daysInYear;
        }
    }

    uint8_t month, day;
    if (!__picoTimeFromOrdinal(year, (uint16_t)targetDoy, &month, &day)) {
        return false;
    }

    *outYear  = year;
    *outMonth = month;
    *outDay   = day;
    return true;
}

static bool __picoTimeParseDigits(const char **str, int numDigits, int *outValue)
{
    int value = 0;
    for (int i = 0; i < numDigits; i++) {
        char c = (*str)[i];
        if (c < '0' || c > '9') {
            return false;
        }
        value = value * 10 + (c - '0');
    }
    *outValue = value;
    *str += numDigits;
    return true;
}

// NOTE: this is still a work in progress and may not cover all edge cases.
// Suggestions for improvement are welcome.
static picoTimeISOFormat __picoTimeDetectISOFormat(const char *isoString)
{
    if (isoString == NULL || *isoString == '\0') {
        return PICO_TIME_ISO_FORMAT_UNKNOWN;
    }

    size_t len    = strlen(isoString);
    bool hasT     = (strchr(isoString, 'T') != NULL);
    bool hasColon = (strchr(isoString, ':') != NULL);
    bool hasDash  = (strchr(isoString, '-') != NULL);
    bool hasW     = (strchr(isoString, 'W') != NULL);
    bool hasDot   = (strchr(isoString, '.') != NULL);
    bool hasZ     = (isoString[len - 1] == 'Z');

    if (!hasT && hasColon && len <= 12) {
        if (hasDot) {
            return hasColon ? PICO_TIME_ISO_TIME_EXTENDED_FRAC : PICO_TIME_ISO_TIME_BASIC_FRAC;
        }
        return hasColon ? PICO_TIME_ISO_TIME_EXTENDED : PICO_TIME_ISO_TIME_BASIC;
    }

    if (!hasT && !hasDash && len >= 6 && len <= 10) {
        if (hasDot) {
            return PICO_TIME_ISO_TIME_BASIC_FRAC;
        }
        if (len == 6) {
            return PICO_TIME_ISO_TIME_BASIC;
        }
    }

    if (hasW) {
        if (hasT) {
            return hasDash ? PICO_TIME_ISO_DATETIME_EXTENDED : PICO_TIME_ISO_DATETIME_BASIC;
        }
        return hasDash ? PICO_TIME_ISO_WEEK_EXTENDED : PICO_TIME_ISO_WEEK_BASIC;
    }

    if (hasT) {
        bool extendedDate = (isoString[4] == '-');
        bool extendedTime = hasColon;
        bool frac         = hasDot;

        if (extendedDate && extendedTime) {
            if (hasZ) {
                return frac ? PICO_TIME_ISO_DATETIME_EXTENDED_FRAC_UTC : PICO_TIME_ISO_DATETIME_EXTENDED_UTC;
            }
            return frac ? PICO_TIME_ISO_DATETIME_EXTENDED_FRAC : PICO_TIME_ISO_DATETIME_EXTENDED;
        } else {
            if (hasZ) {
                return frac ? PICO_TIME_ISO_DATETIME_BASIC_FRAC_UTC : PICO_TIME_ISO_DATETIME_BASIC_UTC;
            }
            return frac ? PICO_TIME_ISO_DATETIME_BASIC_FRAC : PICO_TIME_ISO_DATETIME_BASIC;
        }
    }

    if ((hasDash && len == 8) || (!hasDash && len == 7)) {
        return hasDash ? PICO_TIME_ISO_ORDINAL_EXTENDED : PICO_TIME_ISO_ORDINAL_BASIC;
    }

    if ((hasDash && len == 10) || (!hasDash && len == 8)) {
        return hasDash ? PICO_TIME_ISO_CALENDAR_EXTENDED : PICO_TIME_ISO_CALENDAR_BASIC;
    }

    return PICO_TIME_ISO_FORMAT_UNKNOWN;
}

bool picoTimeFormatISO(const picoTime_t *time, picoTimeISOFormat format, char *buffer, size_t bufferSize)
{
    if (time == NULL || buffer == NULL || bufferSize == 0 || format >= PICO_TIME_ISO_FORMAT_UNKNOWN) {
        return false;
    }

    int written = 0;
    uint16_t isoYear;
    uint8_t isoWeek, isoDayOfWeek;
    uint16_t dayOfYear;

    switch (format) {
        case PICO_TIME_ISO_CALENDAR_EXTENDED:
            written = snprintf(buffer, bufferSize, "%04u-%02u-%02u",
                               time->year, time->month, time->day);
            break;

        case PICO_TIME_ISO_CALENDAR_BASIC:
            written = snprintf(buffer, bufferSize, "%04u%02u%02u",
                               time->year, time->month, time->day);
            break;

        case PICO_TIME_ISO_ORDINAL_EXTENDED:
            dayOfYear = __picoTimeGetDayOfYear(time->year, time->month, time->day);
            written   = snprintf(buffer, bufferSize, "%04u-%03u",
                                 time->year, dayOfYear);
            break;

        case PICO_TIME_ISO_ORDINAL_BASIC:
            dayOfYear = __picoTimeGetDayOfYear(time->year, time->month, time->day);
            written   = snprintf(buffer, bufferSize, "%04u%03u",
                                 time->year, dayOfYear);
            break;

        case PICO_TIME_ISO_WEEK_EXTENDED:
            __picoTimeGetISOWeek(time->year, time->month, time->day, &isoYear, &isoWeek, &isoDayOfWeek);
            written = snprintf(buffer, bufferSize, "%04u-W%02u-%u",
                               isoYear, isoWeek, isoDayOfWeek);
            break;

        case PICO_TIME_ISO_WEEK_BASIC:
            __picoTimeGetISOWeek(time->year, time->month, time->day, &isoYear, &isoWeek, &isoDayOfWeek);
            written = snprintf(buffer, bufferSize, "%04uW%02u%u",
                               isoYear, isoWeek, isoDayOfWeek);
            break;

        case PICO_TIME_ISO_TIME_EXTENDED:
            written = snprintf(buffer, bufferSize, "%02u:%02u:%02u",
                               time->hour, time->minute, time->second);
            break;

        case PICO_TIME_ISO_TIME_BASIC:
            written = snprintf(buffer, bufferSize, "%02u%02u%02u",
                               time->hour, time->minute, time->second);
            break;

        case PICO_TIME_ISO_TIME_EXTENDED_FRAC:
            written = snprintf(buffer, bufferSize, "%02u:%02u:%02u.%03u",
                               time->hour, time->minute, time->second, time->millisecond);
            break;

        case PICO_TIME_ISO_TIME_BASIC_FRAC:
            written = snprintf(buffer, bufferSize, "%02u%02u%02u.%03u",
                               time->hour, time->minute, time->second, time->millisecond);
            break;

        case PICO_TIME_ISO_DATETIME_EXTENDED:
            written = snprintf(buffer, bufferSize, "%04u-%02u-%02uT%02u:%02u:%02u",
                               time->year, time->month, time->day,
                               time->hour, time->minute, time->second);
            break;

        case PICO_TIME_ISO_DATETIME_BASIC:
            written = snprintf(buffer, bufferSize, "%04u%02u%02uT%02u%02u%02u",
                               time->year, time->month, time->day,
                               time->hour, time->minute, time->second);
            break;

        case PICO_TIME_ISO_DATETIME_EXTENDED_FRAC:
            written = snprintf(buffer, bufferSize, "%04u-%02u-%02uT%02u:%02u:%02u.%03u",
                               time->year, time->month, time->day,
                               time->hour, time->minute, time->second, time->millisecond);
            break;

        case PICO_TIME_ISO_DATETIME_BASIC_FRAC:
            written = snprintf(buffer, bufferSize, "%04u%02u%02uT%02u%02u%02u.%03u",
                               time->year, time->month, time->day,
                               time->hour, time->minute, time->second, time->millisecond);
            break;

        case PICO_TIME_ISO_DATETIME_EXTENDED_UTC:
            written = snprintf(buffer, bufferSize, "%04u-%02u-%02uT%02u:%02u:%02uZ",
                               time->year, time->month, time->day,
                               time->hour, time->minute, time->second);
            break;

        case PICO_TIME_ISO_DATETIME_BASIC_UTC:
            written = snprintf(buffer, bufferSize, "%04u%02u%02uT%02u%02u%02uZ",
                               time->year, time->month, time->day,
                               time->hour, time->minute, time->second);
            break;

        case PICO_TIME_ISO_DATETIME_EXTENDED_FRAC_UTC:
            written = snprintf(buffer, bufferSize, "%04u-%02u-%02uT%02u:%02u:%02u.%03uZ",
                               time->year, time->month, time->day,
                               time->hour, time->minute, time->second, time->millisecond);
            break;

        case PICO_TIME_ISO_DATETIME_BASIC_FRAC_UTC:
            written = snprintf(buffer, bufferSize, "%04u%02u%02uT%02u%02u%02u.%03uZ",
                               time->year, time->month, time->day,
                               time->hour, time->minute, time->second, time->millisecond);
            break;

        default:
            return false;
    }

    return (written > 0 && (size_t)written < bufferSize);
}

bool picoTimeParseISO(const char *isoString, picoTime_t *outTime, picoTimeISOFormat *format)
{
    if (isoString == NULL || outTime == NULL) {
        return false;
    }

    picoTimeISOFormat detectedFormat = PICO_TIME_ISO_FORMAT_UNKNOWN;
    detectedFormat                   = __picoTimeDetectISOFormat(isoString);
    if (detectedFormat >= PICO_TIME_ISO_FORMAT_UNKNOWN) {
        return false;
    }
    if (format != NULL) {
        *format = detectedFormat;
    }

    memset(outTime, 0, sizeof(picoTime_t));
    outTime->year  = 1;
    outTime->month = 1;
    outTime->day   = 1;

    const char *p = isoString;
    int val;
    uint16_t year = 1;
    uint8_t month = 1, day = 1;
    uint8_t hour = 0, minute = 0, second = 0;
    uint16_t millisecond = 0;

    switch (detectedFormat) {
        case PICO_TIME_ISO_CALENDAR_EXTENDED:
            if (!__picoTimeParseDigits(&p, 4, &val))
                return false;
            year = (uint16_t)val;
            if (*p++ != '-')
                return false;
            if (!__picoTimeParseDigits(&p, 2, &val))
                return false;
            month = (uint8_t)val;
            if (*p++ != '-')
                return false;
            if (!__picoTimeParseDigits(&p, 2, &val))
                return false;
            day = (uint8_t)val;
            break;

        case PICO_TIME_ISO_CALENDAR_BASIC:
            if (!__picoTimeParseDigits(&p, 4, &val))
                return false;
            year = (uint16_t)val;
            if (!__picoTimeParseDigits(&p, 2, &val))
                return false;
            month = (uint8_t)val;
            if (!__picoTimeParseDigits(&p, 2, &val))
                return false;
            day = (uint8_t)val;
            break;

        case PICO_TIME_ISO_ORDINAL_EXTENDED: {
            uint16_t dayOfYear;
            if (!__picoTimeParseDigits(&p, 4, &val))
                return false;
            year = (uint16_t)val;
            if (*p++ != '-')
                return false;
            if (!__picoTimeParseDigits(&p, 3, &val))
                return false;
            dayOfYear = (uint16_t)val;
            if (!__picoTimeFromOrdinal(year, dayOfYear, &month, &day))
                return false;
        } break;

        case PICO_TIME_ISO_ORDINAL_BASIC: {
            uint16_t dayOfYear;
            if (!__picoTimeParseDigits(&p, 4, &val))
                return false;
            year = (uint16_t)val;
            if (!__picoTimeParseDigits(&p, 3, &val))
                return false;
            dayOfYear = (uint16_t)val;
            if (!__picoTimeFromOrdinal(year, dayOfYear, &month, &day))
                return false;
        } break;

        case PICO_TIME_ISO_WEEK_EXTENDED: {
            uint16_t isoYear;
            uint8_t week, dayOfWeek;
            if (!__picoTimeParseDigits(&p, 4, &val))
                return false;
            isoYear = (uint16_t)val;
            if (*p++ != '-')
                return false;
            if (*p++ != 'W')
                return false;
            if (!__picoTimeParseDigits(&p, 2, &val))
                return false;
            week = (uint8_t)val;
            if (*p++ != '-')
                return false;
            if (!__picoTimeParseDigits(&p, 1, &val))
                return false;
            dayOfWeek = (uint8_t)val;
            if (!__picoTimeFromISOWeek(isoYear, week, dayOfWeek, &year, &month, &day))
                return false;
        } break;

        case PICO_TIME_ISO_WEEK_BASIC: {
            uint16_t isoYear;
            uint8_t week, dayOfWeek;
            if (!__picoTimeParseDigits(&p, 4, &val))
                return false;
            isoYear = (uint16_t)val;
            if (*p++ != 'W')
                return false;
            if (!__picoTimeParseDigits(&p, 2, &val))
                return false;
            week = (uint8_t)val;
            if (!__picoTimeParseDigits(&p, 1, &val))
                return false;
            dayOfWeek = (uint8_t)val;
            if (!__picoTimeFromISOWeek(isoYear, week, dayOfWeek, &year, &month, &day))
                return false;
        } break;

        case PICO_TIME_ISO_TIME_EXTENDED:
            if (!__picoTimeParseDigits(&p, 2, &val))
                return false;
            hour = (uint8_t)val;
            if (*p++ != ':')
                return false;
            if (!__picoTimeParseDigits(&p, 2, &val))
                return false;
            minute = (uint8_t)val;
            if (*p++ != ':')
                return false;
            if (!__picoTimeParseDigits(&p, 2, &val))
                return false;
            second = (uint8_t)val;
            break;

        case PICO_TIME_ISO_TIME_BASIC:
            if (!__picoTimeParseDigits(&p, 2, &val))
                return false;
            hour = (uint8_t)val;
            if (!__picoTimeParseDigits(&p, 2, &val))
                return false;
            minute = (uint8_t)val;
            if (!__picoTimeParseDigits(&p, 2, &val))
                return false;
            second = (uint8_t)val;
            break;

        case PICO_TIME_ISO_TIME_EXTENDED_FRAC:
            if (!__picoTimeParseDigits(&p, 2, &val))
                return false;
            hour = (uint8_t)val;
            if (*p++ != ':')
                return false;
            if (!__picoTimeParseDigits(&p, 2, &val))
                return false;
            minute = (uint8_t)val;
            if (*p++ != ':')
                return false;
            if (!__picoTimeParseDigits(&p, 2, &val))
                return false;
            second = (uint8_t)val;
            if (*p++ != '.')
                return false;
            if (!__picoTimeParseDigits(&p, 3, &val))
                return false;
            millisecond = (uint16_t)val;
            break;

        case PICO_TIME_ISO_TIME_BASIC_FRAC:
            if (!__picoTimeParseDigits(&p, 2, &val))
                return false;
            hour = (uint8_t)val;
            if (!__picoTimeParseDigits(&p, 2, &val))
                return false;
            minute = (uint8_t)val;
            if (!__picoTimeParseDigits(&p, 2, &val))
                return false;
            second = (uint8_t)val;
            if (*p++ != '.')
                return false;
            if (!__picoTimeParseDigits(&p, 3, &val))
                return false;
            millisecond = (uint16_t)val;
            break;

        case PICO_TIME_ISO_DATETIME_EXTENDED:
        case PICO_TIME_ISO_DATETIME_EXTENDED_UTC:
            if (!__picoTimeParseDigits(&p, 4, &val))
                return false;
            year = (uint16_t)val;
            if (*p++ != '-')
                return false;
            if (!__picoTimeParseDigits(&p, 2, &val))
                return false;
            month = (uint8_t)val;
            if (*p++ != '-')
                return false;
            if (!__picoTimeParseDigits(&p, 2, &val))
                return false;
            day = (uint8_t)val;
            if (*p++ != 'T')
                return false;
            if (!__picoTimeParseDigits(&p, 2, &val))
                return false;
            hour = (uint8_t)val;
            if (*p++ != ':')
                return false;
            if (!__picoTimeParseDigits(&p, 2, &val))
                return false;
            minute = (uint8_t)val;
            if (*p++ != ':')
                return false;
            if (!__picoTimeParseDigits(&p, 2, &val))
                return false;
            second = (uint8_t)val;
            // Optional Z
            break;

        case PICO_TIME_ISO_DATETIME_BASIC:
        case PICO_TIME_ISO_DATETIME_BASIC_UTC:
            if (!__picoTimeParseDigits(&p, 4, &val))
                return false;
            year = (uint16_t)val;
            if (!__picoTimeParseDigits(&p, 2, &val))
                return false;
            month = (uint8_t)val;
            if (!__picoTimeParseDigits(&p, 2, &val))
                return false;
            day = (uint8_t)val;
            if (*p++ != 'T')
                return false;
            if (!__picoTimeParseDigits(&p, 2, &val))
                return false;
            hour = (uint8_t)val;
            if (!__picoTimeParseDigits(&p, 2, &val))
                return false;
            minute = (uint8_t)val;
            if (!__picoTimeParseDigits(&p, 2, &val))
                return false;
            second = (uint8_t)val;
            break;

        case PICO_TIME_ISO_DATETIME_EXTENDED_FRAC:
        case PICO_TIME_ISO_DATETIME_EXTENDED_FRAC_UTC:
            if (!__picoTimeParseDigits(&p, 4, &val))
                return false;
            year = (uint16_t)val;
            if (*p++ != '-')
                return false;
            if (!__picoTimeParseDigits(&p, 2, &val))
                return false;
            month = (uint8_t)val;
            if (*p++ != '-')
                return false;
            if (!__picoTimeParseDigits(&p, 2, &val))
                return false;
            day = (uint8_t)val;
            if (*p++ != 'T')
                return false;
            if (!__picoTimeParseDigits(&p, 2, &val))
                return false;
            hour = (uint8_t)val;
            if (*p++ != ':')
                return false;
            if (!__picoTimeParseDigits(&p, 2, &val))
                return false;
            minute = (uint8_t)val;
            if (*p++ != ':')
                return false;
            if (!__picoTimeParseDigits(&p, 2, &val))
                return false;
            second = (uint8_t)val;
            if (*p++ != '.')
                return false;
            if (!__picoTimeParseDigits(&p, 3, &val))
                return false;
            millisecond = (uint16_t)val;
            break;

        case PICO_TIME_ISO_DATETIME_BASIC_FRAC:
        case PICO_TIME_ISO_DATETIME_BASIC_FRAC_UTC:
            if (!__picoTimeParseDigits(&p, 4, &val))
                return false;
            year = (uint16_t)val;
            if (!__picoTimeParseDigits(&p, 2, &val))
                return false;
            month = (uint8_t)val;
            if (!__picoTimeParseDigits(&p, 2, &val))
                return false;
            day = (uint8_t)val;
            if (*p++ != 'T')
                return false;
            if (!__picoTimeParseDigits(&p, 2, &val))
                return false;
            hour = (uint8_t)val;
            if (!__picoTimeParseDigits(&p, 2, &val))
                return false;
            minute = (uint8_t)val;
            if (!__picoTimeParseDigits(&p, 2, &val))
                return false;
            second = (uint8_t)val;
            if (*p++ != '.')
                return false;
            if (!__picoTimeParseDigits(&p, 3, &val))
                return false;
            millisecond = (uint16_t)val;
            break;

        default:
            return false;
    }

    if (month < 1 || month > 12)
        return false;
    if (day < 1 || day > __picoTimeDaysInMonth(month, year))
        return false;
    if (hour > 23)
        return false;
    if (minute > 59)
        return false;
    if (second > 59)
        return false;
    if (millisecond > 999)
        return false;

    outTime->year        = year;
    outTime->month       = month;
    outTime->day         = day;
    outTime->hour        = hour;
    outTime->minute      = minute;
    outTime->second      = second;
    outTime->millisecond = millisecond;
    outTime->nanosecond  = 0;

    return true;
}

const char *picoTimeISOFormatToString(picoTimeISOFormat format)
{
    switch (format) {
        case PICO_TIME_ISO_CALENDAR_EXTENDED:
            return "Calendar Extended (YYYY-MM-DD)";
        case PICO_TIME_ISO_CALENDAR_BASIC:
            return "Calendar Basic (YYYYMMDD)";
        case PICO_TIME_ISO_ORDINAL_EXTENDED:
            return "Ordinal Extended (YYYY-DDD)";
        case PICO_TIME_ISO_ORDINAL_BASIC:
            return "Ordinal Basic (YYYYDDD)";
        case PICO_TIME_ISO_WEEK_EXTENDED:
            return "Week Extended (YYYY-Www-D)";
        case PICO_TIME_ISO_WEEK_BASIC:
            return "Week Basic (YYYYWwwD)";
        case PICO_TIME_ISO_TIME_EXTENDED:
            return "Time Extended (hh:mm:ss)";
        case PICO_TIME_ISO_TIME_BASIC:
            return "Time Basic (hhmmss)";
        case PICO_TIME_ISO_TIME_EXTENDED_FRAC:
            return "Time Extended with Fraction (hh:mm:ss.sss)";
        case PICO_TIME_ISO_TIME_BASIC_FRAC:
            return "Time Basic with Fraction (hhmmss.sss)";
        case PICO_TIME_ISO_DATETIME_EXTENDED:
            return "DateTime Extended (YYYY-MM-DDThh:mm:ss)";
        case PICO_TIME_ISO_DATETIME_BASIC:
            return "DateTime Basic (YYYYMMDDThhmmss)";
        case PICO_TIME_ISO_DATETIME_EXTENDED_FRAC:
            return "DateTime Extended with Fraction (YYYY-MM-DDThh:mm:ss.sss)";
        case PICO_TIME_ISO_DATETIME_BASIC_FRAC:
            return "DateTime Basic with Fraction (YYYYMMDDThhmmss.sss)";
        case PICO_TIME_ISO_DATETIME_EXTENDED_UTC:
            return "DateTime Extended UTC (YYYY-MM-DDThh:mm:ssZ)";
        case PICO_TIME_ISO_DATETIME_BASIC_UTC:
            return "DateTime Basic UTC (YYYYMMDDThhmmssZ)";
        case PICO_TIME_ISO_DATETIME_EXTENDED_FRAC_UTC:
            return "DateTime Extended with Fraction UTC (YYYY-MM-DDThh:mm:ss.sssZ)";
        case PICO_TIME_ISO_DATETIME_BASIC_FRAC_UTC:
            return "DateTime Basic with Fraction UTC (YYYYMMDDThhmmss.sssZ)";
        default:
            return "Unknown Format";
    }
}

#endif

#endif // PICO_TIME_H
