#include <stdio.h>
#include <stdbool.h>
#include <inttypes.h>

#define PICO_IMPLEMENTATION
#include "pico/picoTime.h"

void demonstrateBasicTime(void) {
    printf("--- Basic Time Retrieval and Formatting ---\n");
    
    picoTime_t now = picoTimeGetCurrent();
    char buffer[128];
    
    if (picoTimeFormat(&now, buffer, sizeof(buffer))) {
        printf("Current Time (Default Format): %s\n", buffer);
    }
    
    if (picoTimeFormatISO(&now, PICO_TIME_ISO_DATETIME_EXTENDED_FRAC_UTC, buffer, sizeof(buffer))) {
        printf("Current Time (ISO 8601 Extended Frac UTC): %s\n", buffer);
    }
    
    printf("Manual Access: %04u-%02u-%02u %02u:%02u:%02u.%03u\n",
           now.year, now.month, now.day, now.hour, now.minute, now.second, now.millisecond);
    printf("\n");
}

void demonstrateDurations(void) {
    printf("--- Duration Calculations ---\n");
    
    picoTime_t start = picoTimeGetCurrent();
    printf("Starting a task at: %02u:%02u:%02u.%03u\n", start.hour, start.minute, start.second, start.millisecond);
    
    picoTimeSleep(1500200); 
    
    picoTime_t end = picoTimeGetCurrent();
    printf("Finished task at: %02u:%02u:%02u.%03u\n", end.hour, end.minute, end.second, end.millisecond);
    
    picoTimeDurationMilli_t durationMs = picoTimeGetDurationMilli(start, end);
    picoTimeDurationSeconds_t durationS = picoTimeGetDurationSeconds(start, end);
    picoTimeDurationNano_t durationNs = picoTimeGetDurationNano(start, end);
    
    printf("Duration: %" PRIu64 " ms\n", durationMs);
    printf("Duration: %" PRIu64 " seconds (approx)\n", durationS);
    printf("Duration: %" PRIu64 " nanoseconds\n", durationNs);
    printf("\n");
}

void demonstrateISOParsing(void) {
    printf("--- ISO 8601 Parsing ---\n");
    
    const char *isoString = "2026-01-03T15:30:45.123Z";
    picoTime_t parsedTime;
    picoTimeISOFormat detectedFormat = PICO_TIME_ISO_FORMAT_UNKNOWN;
    
    if (picoTimeParseISO(isoString, &parsedTime, &detectedFormat)) {
        printf("Successfully parsed: %s, Format: %s\n", isoString, picoTimeISOFormatToString(detectedFormat));
        printf("Parsed Components: %04u-%02u-%02u %02u:%02u:%02u.%03u\n",
               parsedTime.year, parsedTime.month, parsedTime.day,
               parsedTime.hour, parsedTime.minute, parsedTime.second, parsedTime.millisecond);
    } else {
        printf("Failed to parse ISO string: %s\n", isoString);
    }
    printf("\n");
}

void timerCallback(picoTimeTimer timer, void *userData) {
    const char *name = (const char *)userData;
    uint64_t count = picoTimeTimerGetTriggerCount(timer);
    picoTime_t now = picoTimeGetCurrent();
    
    printf("[Timer %s] Triggered! Count: %" PRIu64 " at %02u:%02u:%02u.%03u\n",
           name, count, now.hour, now.minute, now.second, now.millisecond);
}

void demonstrateTimers(void) {
    printf("--- Timers (One-shot and Repeating) ---\n");
    
    picoTimeTimer repeatingTimer = picoTimeTimerCreate();
    picoTimeTimerSetIntervalMilli(repeatingTimer, 500);
    picoTimeTimerSetCallback(repeatingTimer, timerCallback);
    picoTimeTimerSetUserData(repeatingTimer, "Repeating-500ms");
    picoTimeTimerSetRepeat(repeatingTimer, true);
    
    printf("Starting repeating timer (500ms)...\n");
    picoTimeTimerRestart(repeatingTimer);
    
    picoTimeTimer oneShotTimer = picoTimeTimerCreate();
    picoTimeTimerSetIntervalSeconds(oneShotTimer, 2);
    picoTimeTimerSetCallback(oneShotTimer, timerCallback);
    picoTimeTimerSetUserData(oneShotTimer, "OneShot-2s");
    picoTimeTimerSetRepeat(oneShotTimer, false);
    
    printf("Starting one-shot timer (2s)...\n");
    picoTimeTimerRestart(oneShotTimer);
    
    printf("Main thread sleeping for 3 seconds while timers run...\n");
    picoTimeSleep(3000000);
    
    printf("Stopping timers...\n");
    picoTimeTimerStop(repeatingTimer);
    picoTimeTimerStop(oneShotTimer);
    
    picoTimeTimerDestroy(repeatingTimer);
    picoTimeTimerDestroy(oneShotTimer);
    printf("\n");
}
int main(void) {
    printf("Hello, Pico!\n");

    demonstrateBasicTime();
    demonstrateDurations();
    demonstrateISOParsing();
    demonstrateTimers();
    
    printf("Goodbye, Pico!\n");
    return 0;
}
