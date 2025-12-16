#include <math.h>
#include <stdio.h>
#include <stdlib.h>


#define PICO_LOG_TAG "SANDBOX" // if this is not defined, it will use the filename (main.c) as the tag

#define PICO_IMPLEMENTATION
#include "pico/picoLog.h"

void myCustomLogger(picoLogLevel level, const char *tag, const char *message,
                    picoLogCodeLocation location, picoLogTimeStamp timestamp, void *userData)
{

    (void)level;
    (void)tag;
    (void)location;
    (void)timestamp;

    int *callCount = (int *)userData;
    (*callCount)++;
    printf("[CUSTOM LOGGER #%d] %s\n", *callCount, message);
}

void moduleAdoWork(void)
{
    PICO_DEBUG("Module A: Starting work");
    PICO_INFO("Module A: Processing data with value: %d", 42);
    PICO_VERBOSE("Module A: Detailed processing step 1");
    PICO_VERBOSE("Module A: Detailed processing step 2");
    PICO_DEBUG("Module A: Work completed");
}

void moduleBdoWork(void)
{
    PICO_DEBUG("Module B: Starting work");
    PICO_INFO("Module B: Processing data with value: %f", 3.14159);
    PICO_WARN("Module B: Found potential issue - non-critical");
    PICO_DEBUG("Module B: Work completed");
}

void demonstrateErrorHandling(void)
{
    PICO_DEBUG("Testing error handling");

    int errorCode = -1;
    if (errorCode < 0) {
        PICO_ERROR("Failed to open resource: error code %d", errorCode);
    }

    const char *filename = "nonexistent.txt";
    PICO_ERROR("Could not load file: %s", filename);
}

int main(void)
{
    printf("Hello, Pico!\n");

    PICO_LOG_INIT();
    PICO_INFO("picoLog initialized successfully!");

    PICO_LOG_IF_ENABLED(picoLogPushFromEnvironment());
    PICO_INFO("Environment settings loaded (if any were set)");

    PICO_DEBUG("This is a DEBUG message - for detailed debugging info");
    PICO_VERBOSE("This is a VERBOSE message - for detailed execution flow");
    PICO_INFO("This is an INFO message - for general information");
    PICO_WARN("This is a WARN message - for warnings");
    PICO_ERROR("This is an ERROR message - for errors");

    picoLogPushFormat(PICO_LOG_FORMAT_SHORT);
    PICO_INFO("This uses SHORT format");
    picoLogPopFormat();

    picoLogPushFormat(PICO_LOG_FORMAT_MESSAGE_ONLY);
    PICO_INFO("This uses MESSAGE_ONLY format (no metadata)");
    picoLogPopFormat();

    picoLogPushFormat(PICO_LOG_FORMAT_VERBOSE);
    PICO_INFO("This uses VERBOSE format with all details");
    picoLogPopFormat();

    picoLogPushFormat(PICO_LOG_FORMAT_JSON);
    PICO_INFO("This uses JSON format - great for log parsers");
    picoLogPopFormat();

    PICO_DEBUG("You can see DEBUG");
    PICO_INFO("You can see INFO");
    PICO_WARN("You can see WARN");

    picoLogPushLevel(PICO_LOG_LEVEL_WARN | PICO_LOG_LEVEL_ERROR);
    PICO_DEBUG("You CANNOT see this DEBUG");
    PICO_INFO("You CANNOT see this INFO");
    PICO_WARN("You CAN see this WARN");
    PICO_ERROR("You CAN see this ERROR");
    picoLogPopLevel();

    PICO_INFO("INFO is visible again");

    PICO_INFO("This message is from SANDBOX tag");

    const char *logFile = "test_log.txt";
    picoLogPushFileLogger(logFile);
    picoLogPushTarget(PICO_LOG_TARGET_FILE | PICO_LOG_TARGET_CONSOLE);
    PICO_INFO("This message goes to both console AND file: %s", logFile);
    PICO_WARN("This warning is also logged to the file");
    picoLogPopTarget();
    picoLogPopFileLogger();
    PICO_INFO("File logging stopped - this goes to console only");

    int customLoggerCallCount = 0;
    picoLogPushCustomLogger(myCustomLogger, &customLoggerCallCount);
    picoLogPushTarget(PICO_LOG_TARGET_CUSTOM | PICO_LOG_TARGET_CONSOLE);
    PICO_INFO("This message goes to both console AND custom logger");
    PICO_WARN("Custom logger receives this too");
    picoLogPopTarget();
    picoLogPopCustomLogger();

    picoLogPushTarget(PICO_LOG_TARGET_CONSOLE);
    PICO_INFO("This goes to console only");
    picoLogPopTarget();

    int count        = 100;
    float pi         = 3.14159265359f;
    const char *name = "PicoLog";
    PICO_INFO("Integer: %d, Float: %.2f, String: %s", count, pi, name);
    PICO_DEBUG("Hexadecimal: 0x%X, Octal: %o", 255, 255);

    PICO_INFO("Level 0: Default format and level");

    picoLogPushFormat(PICO_LOG_FORMAT_SHORT);
    picoLogPushLevel(PICO_LOG_LEVEL_INFO | PICO_LOG_LEVEL_WARN | PICO_LOG_LEVEL_ERROR);
    PICO_INFO("Level 1: SHORT format, INFO+ only");
    PICO_DEBUG("This DEBUG won't show");

    picoLogPushFormat(PICO_LOG_FORMAT_MESSAGE_ONLY);
    PICO_INFO("Level 2: MESSAGE_ONLY format");

    picoLogPopFormat();
    PICO_INFO("Level 1: Back to SHORT format");

    picoLogPopFormat();
    picoLogPopLevel();
    PICO_INFO("Level 0: Back to default");

    picoLogLevel level = picoLogStringToLevel("ERROR");
    PICO_INFO("String 'ERROR' converted to level: %d", level);

    picoLogFormat format = picoLogStringToFormat("JSON");
    PICO_INFO("String 'JSON' converted to format: %d", format);

    picoLogTarget target = picoLogStringToTarget("CONSOLE");
    PICO_INFO("String 'CONSOLE' converted to target: %d", target);

    PICO_INFO("Application starting up...");

    picoLogPushLevel(PICO_LOG_LEVEL_INFO | PICO_LOG_LEVEL_WARN | PICO_LOG_LEVEL_ERROR);

    PICO_INFO("Loading configuration...");
    moduleAdoWork();

    PICO_INFO("Processing main tasks...");
    moduleBdoWork();

    PICO_INFO("Handling errors...");
    demonstrateErrorHandling();

    picoLogPopLevel();
    PICO_INFO("Application workflow completed");

    picoLogContext ctx = picoLogGetContext();
    PICO_INFO("Retrieved current context: %p", (void *)ctx);
    // In a real DLL scenario, you would pass this context to another module
    // and call picoLogSetContext() there

    PICO_INFO("All tests completed successfully!");
    PICO_WARN("Remember to check test_log.txt for file logging output");

    PICO_LOG_SHUTDOWN();

    printf("Goodbye, Pico!\n");
    return 0;
}
