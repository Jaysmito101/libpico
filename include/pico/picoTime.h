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

// Formats the given picoTime_t into a string.
// if the buffer is not large enough, the output will be truncated.
bool picoTimeFormat(const picoTime_t *time, char *buffer, size_t bufferSize);

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

#include <signal.h>
#include <pthread.h>
#include <time.h>

static void __picoTimeTimerCallback(union sigval sv)
{
    picoTimeTimer clock = (picoTimeTimer_t)sv.sival_ptr;
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

#endif

#endif // PICO_TIME_H
