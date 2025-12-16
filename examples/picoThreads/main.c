#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>

#define PICO_IMPLEMENTATION
#include "pico/picoThreads.h"

#define BUFFER_SIZE 5
#define TASK_COUNT 16

typedef struct {
    picoThreadMutex mutex;
    int counter;
    int targetCount;
} CounterData;

typedef struct {
    picoThreadMutex mutex;
    int balance;
    const char *name;
} BankAccount;

typedef struct {
    int workerId;
    int tasksCompleted;
} WorkerData;

typedef struct {
    picoThreadMutex mutex;
    int buffer[BUFFER_SIZE];
    int count;
    bool producerDone;
} ProducerConsumerData;


void simpleWorker(void *arg)
{
    int workerId = *(int *)arg;
    printf("[Worker %d] Thread started (ID: %" PRIu64 ")\n", workerId, (uint64_t)picoThreadGetCurrentId());

    for (int i = 1; i <= 5; i++) {
        printf("[Worker %d] Processing task %d/5...\n", workerId, i);
        picoThreadSleep(200);
    }

    printf("[Worker %d] Thread finished!\n", workerId);
}

void demonstrateBasicThreading(void)
{
    printf("Basic Thread Creation & Joining\n");

    int workerIds[3] = {1, 2, 3};
    picoThread threads[3];

    for (int i = 0; i < 3; i++) {
        threads[i] = picoThreadCreate(simpleWorker, &workerIds[i]);
        printf("Created thread %d (Alive: %s, Joinable: %s)\n",
               i + 1,
               picoThreadIsAlive(threads[i]) ? "Yes" : "No",
               picoThreadJoinable(threads[i]) ? "Yes" : "No");
    }

    printf("\nMain thread waiting for workers to complete...\n");

    for (int i = 0; i < 3; i++) {
        picoThreadJoin(threads[i], PICO_THREAD_INFINITE);
        printf("Thread %d joined (Alive: %s, Joinable: %s)\n",
               i + 1,
               picoThreadIsAlive(threads[i]) ? "Yes" : "No",
               picoThreadJoinable(threads[i]) ? "Yes" : "No");
        picoThreadDestroy(threads[i]);
    }

    printf("All workers completed!\n");
}

void counterWorker(void *arg)
{
    CounterData *data = (CounterData *)arg;

    while (true) {
        picoThreadMutexLock(data->mutex, PICO_THREAD_INFINITE);

        if (data->counter >= data->targetCount) {
            picoThreadMutexUnlock(data->mutex);
            break;
        }

        int currentValue = data->counter;
        data->counter++;

        picoThreadMutexUnlock(data->mutex);

        printf("[Thread %" PRIu64 "] Incremented counter: %d -> %d\n",
               (uint64_t)picoThreadGetCurrentId(), currentValue, currentValue + 1);

        picoThreadSleep(50);
    }
}

void demonstrateMutexSynchronization(void)
{
    printf("Mutex Synchronization\n");

    CounterData data;
    data.mutex       = picoThreadMutexCreate();
    data.counter     = 0;
    data.targetCount = 20;

    printf("Starting 3 threads to increment counter to %d...\n", data.targetCount);

    picoThread threads[3];
    for (int i = 0; i < 3; i++) {
        threads[i] = picoThreadCreate(counterWorker, &data);
    }

    for (int i = 0; i < 3; i++) {
        picoThreadJoin(threads[i], PICO_THREAD_INFINITE);
        picoThreadDestroy(threads[i]);
    }

    printf("Final counter value: %d (Expected: %d)\n", data.counter, data.targetCount);

    picoThreadMutexDestroy(data.mutex);
}


void bankTransaction(void *arg)
{
    BankAccount *account  = (BankAccount *)arg;
    picoThreadId threadId = picoThreadGetCurrentId();

    for (int attempt = 1; attempt <= 5; attempt++) {
        printf("[Thread %" PRIu64 "] Attempt %d: Trying to access account '%s'...\n",
               (uint64_t)threadId, attempt, account->name);

        if (picoThreadMutexTryLock(account->mutex)) {
            printf("[Thread %" PRIu64 "] SUCCESS! Acquired lock on account '%s'\n",
                   (uint64_t)threadId, account->name);

            int oldBalance = account->balance;
            picoThreadSleep(100); // Simulate processing time
            account->balance += 10;

            printf("[Thread %" PRIu64 "] Transaction complete: $%d -> $%d\n",
                   (uint64_t)threadId, oldBalance, account->balance);

            picoThreadMutexUnlock(account->mutex);
            break;
        } else {
            printf("[Thread %" PRIu64 "] FAILED! Account '%s' is busy, will retry...\n",
                   (uint64_t)threadId, account->name);
            picoThreadSleep(80);
        }
    }
}

void demonstrateTryLock(void)
{
    printf("TryLock & Non-blocking Operations\n");

    BankAccount account;
    account.mutex   = picoThreadMutexCreate();
    account.balance = 100;
    account.name    = "Savings";

    printf("Initial balance: $%d\n", account.balance);
    printf("Starting 4 threads attempting concurrent transactions...\n");

    picoThread threads[4];
    for (int i = 0; i < 4; i++) {
        threads[i] = picoThreadCreate(bankTransaction, &account);
        picoThreadSleep(20); // Stagger thread creation
    }

    for (int i = 0; i < 4; i++) {
        picoThreadJoin(threads[i], PICO_THREAD_INFINITE);
        picoThreadDestroy(threads[i]);
    }

    printf("Final balance: $%d\n", account.balance);

    picoThreadMutexDestroy(account.mutex);
}

void identityWorker(void *arg)
{
    picoThread *selfThread = (picoThread *)arg;
    picoThreadId id        = picoThreadGetCurrentId();

    printf("[Worker] My thread ID: %" PRIu64 "\n", id);
    printf("[Worker] Am I the current thread? %s\n",
           picoThreadIsCurrent(*selfThread) ? "YES" : "NO");
    printf("[Worker] My thread ID from handle: %" PRIu64 "\n",
           (uint64_t)picoThreadGetId(*selfThread));

    picoThreadSleep(500);

    printf("[Worker] Still alive!\n");
}

void demonstrateThreadIdentity(void)
{
    printf("Thread Identity & Current Thread Checks\n");

    printf("[Main] Main thread ID: %" PRIu64 "\n", (uint64_t)picoThreadGetCurrentId());

    picoThread worker = picoThreadCreate(identityWorker, &worker);

    picoThreadSleep(100); // Let worker start

    printf("[Main] Worker thread ID: %" PRIu64 "\n", (uint64_t)picoThreadGetId(worker));
    printf("[Main] Is worker the current thread? %s\n",
           picoThreadIsCurrent(worker) ? "YES" : "NO");
    printf("[Main] Is worker alive? %s\n",
           picoThreadIsAlive(worker) ? "YES" : "NO");

    picoThreadJoin(worker, PICO_THREAD_INFINITE);

    printf("[Main] Is worker still alive? %s\n",
           picoThreadIsAlive(worker) ? "YES" : "NO");

    picoThreadDestroy(worker);
}

void cooperativeWorker(void *arg)
{
    int workerId = *(int *)arg;

    for (int i = 0; i < 10; i++) {
        printf("[Worker %d] Iteration %d\n", workerId, i + 1);

        if (i % 3 == 0) {
            printf("[Worker %d] Yielding CPU to other threads...\n", workerId);
            picoThreadYield();
        } else {
            picoThreadSleep(50);
        }
    }
}

void demonstrateYieldAndSleep(void)
{
    printf("Thread Yielding & Sleep\n");

    int workerIds[2] = {1, 2};
    picoThread threads[2];

    printf("Starting 2 cooperative workers...\n");

    for (int i = 0; i < 2; i++) {
        threads[i] = picoThreadCreate(cooperativeWorker, &workerIds[i]);
    }

    for (int i = 0; i < 2; i++) {
        picoThreadJoin(threads[i], PICO_THREAD_INFINITE);
        picoThreadDestroy(threads[i]);
    }

    printf("Cooperative workers completed!\n");
}


void producer(void *arg)
{
    ProducerConsumerData *data = (ProducerConsumerData *)arg;

    for (int i = 1; i <= 15; i++) {
        while (true) {
            picoThreadMutexLock(data->mutex, PICO_THREAD_INFINITE);

            if (data->count < BUFFER_SIZE) {
                data->buffer[data->count] = i;
                data->count++;
                printf("[Producer] Produced item %d (Buffer: %d/%d)\n",
                       i, data->count, BUFFER_SIZE);
                picoThreadMutexUnlock(data->mutex);
                break;
            }

            picoThreadMutexUnlock(data->mutex);
            printf("[Producer] Buffer full, waiting...\n");
            picoThreadSleep(100);
        }

        picoThreadSleep(150);
    }

    picoThreadMutexLock(data->mutex, PICO_THREAD_INFINITE);
    data->producerDone = true;
    picoThreadMutexUnlock(data->mutex);

    printf("[Producer] All items produced!\n");
}

void consumer(void *arg)
{
    ProducerConsumerData *data = (ProducerConsumerData *)arg;
    int consumedCount          = 0;

    while (true) {
        picoThreadMutexLock(data->mutex, PICO_THREAD_INFINITE);

        if (data->count > 0) {
            int item = data->buffer[data->count - 1];
            data->count--;
            consumedCount++;
            printf("[Consumer] Consumed item %d (Buffer: %d/%d)\n",
                   item, data->count, BUFFER_SIZE);
            picoThreadMutexUnlock(data->mutex);
            picoThreadSleep(200);
        } else if (data->producerDone) {
            picoThreadMutexUnlock(data->mutex);
            break;
        } else {
            picoThreadMutexUnlock(data->mutex);
            printf("[Consumer] Buffer empty, waiting...\n");
            picoThreadSleep(100);
        }
    }

    printf("[Consumer] Consumed %d items total!\n", consumedCount);
}

void demonstrateProducerConsumer(void)
{
    printf("Producer-Consumer Pattern\n");

    ProducerConsumerData data;
    data.mutex        = picoThreadMutexCreate();
    data.count        = 0;
    data.producerDone = false;

    printf("Starting producer-consumer simulation...\n");

    picoThread producerThread = picoThreadCreate(producer, &data);
    picoThread consumerThread = picoThreadCreate(consumer, &data);

    picoThreadJoin(producerThread, PICO_THREAD_INFINITE);
    picoThreadJoin(consumerThread, PICO_THREAD_INFINITE);

    picoThreadDestroy(producerThread);
    picoThreadDestroy(consumerThread);

    picoThreadMutexDestroy(data.mutex);

    printf("Producer-Consumer simulation completed!\n");
}

typedef struct {
    int taskId;
    int processingTime;
    picoThreadMutex outputMutex;
} PoolTaskData;

void poolTask(void *arg)
{
    PoolTaskData *data = (PoolTaskData *)arg;
    
    picoThreadMutexLock(data->outputMutex, PICO_THREAD_INFINITE);
    printf("[Task %d] Started on thread %" PRIu64 "\n", 
           data->taskId, (uint64_t)picoThreadGetCurrentId());
    picoThreadMutexUnlock(data->outputMutex);
    
    // Simulate work
    picoThreadSleep(data->processingTime);
    
    picoThreadMutexLock(data->outputMutex, PICO_THREAD_INFINITE);
    printf("[Task %d] Completed after %dms\n", 
           data->taskId, data->processingTime);
    picoThreadMutexUnlock(data->outputMutex);
}

void demonstrateThreadPool(void)
{
    printf("Thread Pool\n");
    
    const uint32_t poolSize = 4;
    
    
    printf("Creating thread pool with %u threads...\n", poolSize);
    picoThreadPool pool = picoThreadPoolCreate(poolSize);
    
    if (!pool) {
        printf("Failed to create thread pool!\n");
        return;
    }
    
    printf("Thread pool created with %u threads\n", 
           picoThreadPoolGetThreadCount(pool));
    
    // Create a shared mutex for synchronized output
    picoThreadMutex outputMutex = picoThreadMutexCreate();
    
    // Create task data
    PoolTaskData tasks[TASK_COUNT];
    for (uint32_t i = 0; i < TASK_COUNT; i++) {
        tasks[i].taskId = i + 1;
        tasks[i].processingTime = 100 + (i * 50) % 300; // Varying processing times
        tasks[i].outputMutex = outputMutex;
    }
    
    printf("Adding %u tasks to the pool...\n", TASK_COUNT);
    for (uint32_t i = 0; i < TASK_COUNT; i++) {
        picoThreadPoolAddTask(pool, poolTask, &tasks[i], PICO_THREAD_INFINITE);
        printf("Added task %d (pending: %u, active: %u)\n", 
               i + 1,
               picoThreadPoolGetPendingTaskCount(pool),
               picoThreadPoolGetActiveThreadCount(pool));
        picoThreadSleep(50); // Stagger task submission slightly
    }
    
    printf("\nWaiting for all tasks to complete...\n");
    printf("Pending tasks: %u, Active threads: %u\n",
           picoThreadPoolGetPendingTaskCount(pool),
           picoThreadPoolGetActiveThreadCount(pool));
    
    picoThreadPoolWaitAll(pool);
    
    printf("\nAll tasks completed!\n");
    printf("Final state - Pending: %u, Active: %u\n",
           picoThreadPoolGetPendingTaskCount(pool),
           picoThreadPoolGetActiveThreadCount(pool));
    
    picoThreadMutexDestroy(outputMutex);
    picoThreadPoolDestroy(pool);
    
    printf("Thread pool destroyed successfully!\n");
}

int main(void)
{
    printf("Hello, Pico!\n");

    demonstrateBasicThreading();
    demonstrateMutexSynchronization();
    demonstrateTryLock();
    demonstrateThreadIdentity();
    demonstrateYieldAndSleep();
    demonstrateProducerConsumer();
    demonstrateThreadPool();

    printf("Goodbye, Pico!\n");
    return 0;
}
