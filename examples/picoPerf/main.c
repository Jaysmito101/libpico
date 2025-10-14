#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define PICO_IMPLEMENTATION
#include "pico/picoPerf.h"

void simulateMatrixMultiplication(int size)
{
    PICO_PERF_PUSH_SCOPE("MatrixMultiplication");
    
    double sum = 0.0;
    for (int i = 0; i < size; i++) {
        for (int j = 0; j < size; j++) {
            for (int k = 0; k < size; k++) {
                sum += sin((double)(i + j + k));
            }
        }
    }
    
    if (sum > 1e10) {
        printf("Matrix result: %f\n", sum);
    }
    
    PICO_PERF_POP_SCOPE();
}

void simulateFFT(int size)
{
    PICO_PERF_PUSH_SCOPE("FFT");
    
    double sum = 0.0;
    for (int i = 0; i < size * 1000; i++) {
        sum += cos((double)i) * sin((double)i);
    }
    
    if (sum > 1e10) {
        printf("FFT result: %f\n", sum);
    }
    
    PICO_PERF_POP_SCOPE();
}

void simulateDataProcessing(int iterations)
{
    PICO_PERF_PUSH_SCOPE("DataProcessing");
    
    for (int i = 0; i < iterations; i++) {
        PICO_PERF_PUSH_SCOPE("ProcessBatch");
        
        double sum = 0.0;
        for (int j = 0; j < 10000; j++) {
            sum += sqrt((double)(i * j + 1));
        }
        
        if (sum > 1e10) {
            printf("Batch result: %f\n", sum);
        }
        
        PICO_PERF_POP_SCOPE();
    }
    
    PICO_PERF_POP_SCOPE();
}

void simulateDatabaseQuery()
{
    PICO_PERF_PUSH_SCOPE("DatabaseQuery");
    
    PICO_PERF_PUSH_SCOPE("ConnectToDB");
    PICO_PERF_SLEEP(10);
    PICO_PERF_POP_SCOPE();
    
    PICO_PERF_PUSH_SCOPE("ExecuteQuery");
    PICO_PERF_SLEEP(25);
    PICO_PERF_POP_SCOPE();
    
    PICO_PERF_PUSH_SCOPE("FetchResults");
    PICO_PERF_SLEEP(15);
    PICO_PERF_POP_SCOPE();
    
    PICO_PERF_POP_SCOPE();
}

void simulateFileIO()
{
    PICO_PERF_PUSH_SCOPE("FileIO");
    
    PICO_PERF_PUSH_SCOPE("OpenFile");
    PICO_PERF_SLEEP(5);
    PICO_PERF_POP_SCOPE();
    
    for (int i = 0; i < 3; i++) {
        PICO_PERF_PUSH_SCOPE("ReadChunk");
        PICO_PERF_SLEEP(8);
        PICO_PERF_POP_SCOPE();
    }
    
    PICO_PERF_PUSH_SCOPE("CloseFile");
    PICO_PERF_SLEEP(3);
    PICO_PERF_POP_SCOPE();
    
    PICO_PERF_POP_SCOPE();
}

void simulateNetworkRequest()
{
    PICO_PERF_PUSH_SCOPE("NetworkRequest");
    
    PICO_PERF_PUSH_SCOPE("DNSLookup");
    PICO_PERF_SLEEP(20);
    PICO_PERF_POP_SCOPE();
    
    PICO_PERF_PUSH_SCOPE("TCPConnect");
    PICO_PERF_SLEEP(30);
    PICO_PERF_POP_SCOPE();
    
    PICO_PERF_PUSH_SCOPE("SendRequest");
    PICO_PERF_SLEEP(15);
    PICO_PERF_POP_SCOPE();
    
    PICO_PERF_PUSH_SCOPE("ReceiveResponse");
    PICO_PERF_SLEEP(40);
    PICO_PERF_POP_SCOPE();
    
    PICO_PERF_POP_SCOPE();
}

void complexNestedOperations()
{
    PICO_PERF_PUSH_SCOPE("ComplexNestedOperations");
    
    PICO_PERF_PUSH_SCOPE("Level1");
    simulateMatrixMultiplication(50);
    
    PICO_PERF_PUSH_SCOPE("Level2");
    simulateFFT(100);
    
    PICO_PERF_PUSH_SCOPE("Level3");
    simulateDataProcessing(5);
    PICO_PERF_POP_SCOPE(); // Level3
    
    PICO_PERF_POP_SCOPE(); // Level2
    PICO_PERF_POP_SCOPE(); // Level1
    
    PICO_PERF_POP_SCOPE(); // ComplexNestedOperations
}

void simulateServerRequest()
{
    PICO_PERF_PUSH_SCOPE("ServerRequest");
    
    simulateNetworkRequest();
    simulateDatabaseQuery();
    simulateFileIO();
    
    PICO_PERF_PUSH_SCOPE("ProcessResponse");
    PICO_PERF_SLEEP(12);
    PICO_PERF_POP_SCOPE();
    
    PICO_PERF_POP_SCOPE();
}

void dumpReportToFile(const char *filename, picoPerfReportFormat format)
{
    FILE *file = fopen(filename, "w");
    if (!file) {
        fprintf(stderr, "Failed to open file: %s\n", filename);
        return;
    }
    printf("Generating report: %s\n", filename);
    PICO_PERF_GET_REPORT(file, format);
    fflush(file);
    fclose(file);
}

int main()
{
    printf("Hello, Pico!\n");

    PICO_PERF_CREATE_CONTEXT();

    picoPerfTimeStamp ts = picoPerfGetCurrentTimestamp();
    printf("Current Timestamp: %04d-%02d-%02d %02d:%02d:%02d.%03d\n",
           ts.year, ts.month, ts.day, ts.hour, ts.minute, ts.second, ts.millisecond);
    
    picoPerfTime start = picoPerfNow();
    PICO_PERF_SLEEP(100);
    picoPerfTime end = picoPerfNow();
    
    printf("Frequency: %llu ticks/second\n", (unsigned long long)picoPerfFrequency());
    printf("Sleep 100ms duration:\n");
    printf("  - Seconds: %.6f\n", picoPerfDurationSeconds(start, end));
    printf("  - Milliseconds: %.3f\n", picoPerfDurationMilliseconds(start, end));
    printf("  - Microseconds: %.3f\n", picoPerfDurationMicroseconds(start, end));
    
    char durationStr[64];
    picoPerfFormatDuration(end - start, durationStr, sizeof(durationStr));
    printf("  - Formatted: %s\n\n", durationStr);

    PICO_PERF_BEGIN_RECORD();
    
    simulateMatrixMultiplication(100);
    simulateFFT(200);
    simulateDataProcessing(10);
    
    PICO_PERF_END_RECORD();

    PICO_PERF_BEGIN_RECORD();
    
    simulateDatabaseQuery();
    simulateFileIO();
    simulateNetworkRequest();
    simulateDatabaseQuery(); // Call again to test multiple calls
    
    PICO_PERF_END_RECORD();

    PICO_PERF_BEGIN_RECORD();
    
    complexNestedOperations();
    
    PICO_PERF_END_RECORD();

    PICO_PERF_BEGIN_RECORD();
    
    for (int i = 0; i < 3; i++) {
        PICO_PERF_PUSH_SCOPE("RequestHandler");
        simulateServerRequest();
        PICO_PERF_POP_SCOPE();
    }
    
    PICO_PERF_END_RECORD();

    PICO_PERF_BEGIN_RECORD();
    
    PICO_PERF_PUSH_SCOPE("MixedWorkload");
    PICO_PERF_PUSH_SCOPE("Phase1");
    simulateMatrixMultiplication(75);
    PICO_PERF_POP_SCOPE();
    
    PICO_PERF_PUSH_SCOPE("Phase2");
    simulateNetworkRequest();
    PICO_PERF_POP_SCOPE();
    
    PICO_PERF_PUSH_SCOPE("Phase3");
    PICO_PERF_PUSH_SCOPE("Subphase3.1");
    simulateDataProcessing(3);
    PICO_PERF_PUSH_SCOPE("Subphase3.2");
    simulateFFT(50);
    PICO_PERF_POP_N_SCOPES(2);
    
    PICO_PERF_POP_SCOPE();
    PICO_PERF_POP_SCOPE();
    
    PICO_PERF_END_RECORD();

    printf("Generating performance reports in all formats...\n");
    
    dumpReportToFile("perf_report.txt", PICO_PERF_REPORT_FORMAT_TEXT);
    dumpReportToFile("perf_report.csv", PICO_PERF_REPORT_FORMAT_CSV);
    dumpReportToFile("perf_report.json", PICO_PERF_REPORT_FORMAT_JSON);
    dumpReportToFile("perf_report.xml", PICO_PERF_REPORT_FORMAT_XML);
    
    PICO_PERF_GET_REPORT(stdout, PICO_PERF_REPORT_FORMAT_TEXT);

    PICO_PERF_DESTROY_CONTEXT();

    printf("Goodbye, Pico!\n");

    return 0;
}