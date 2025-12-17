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

void channelSender(void *arg)
{
    picoThreadChannel channel = (picoThreadChannel)arg;
    
    for (int i = 1; i <= 10; i++) {
        printf("[Sender %" PRIu64 "] Sending value: %d\n", 
               (uint64_t)picoThreadGetCurrentId(), i);
        
        if (picoThreadChannelSend(channel, &i)) {
            printf("[Sender %" PRIu64 "] Successfully sent: %d (pending: %u)\n",
                   (uint64_t)picoThreadGetCurrentId(), i,
                   picoThreadChannelGetPendingItemCount(channel));
        } else {
            printf("[Sender %" PRIu64 "] Failed to send: %d\n",
                   (uint64_t)picoThreadGetCurrentId(), i);
        }
        
        picoThreadSleep(100);
    }
    
    printf("[Sender %" PRIu64 "] Finished sending!\n", 
           (uint64_t)picoThreadGetCurrentId());
}

void channelReceiver(void *arg)
{
    picoThreadChannel channel = (picoThreadChannel)arg;
    
    for (int i = 0; i < 10; i++) {
        int value = 0;
        printf("[Receiver %" PRIu64 "] Waiting for value...\n",
               (uint64_t)picoThreadGetCurrentId());
        
        if (picoThreadChannelReceive(channel, &value, 5000)) {
            printf("[Receiver %" PRIu64 "] Received: %d (pending: %u)\n",
                   (uint64_t)picoThreadGetCurrentId(), value,
                   picoThreadChannelGetPendingItemCount(channel));
        } else {
            printf("[Receiver %" PRIu64 "] Timeout waiting for value\n",
                   (uint64_t)picoThreadGetCurrentId());
        }
        
        picoThreadSleep(150);
    }
    
    printf("[Receiver %" PRIu64 "] Finished receiving!\n",
           (uint64_t)picoThreadGetCurrentId());
}

void demonstrateBoundedChannel(void)
{
    printf("Bounded Channel Communication\n");
    
    const uint32_t capacity = 5;
    picoThreadChannel channel = picoThreadChannelCreateBounded(capacity, sizeof(int));
    
    if (!channel) {
        printf("Failed to create bounded channel!\n");
        return;
    }
    
    printf("Created bounded channel (capacity: %u, item size: %zu bytes)\n",
           picoThreadChannelGetCapacity(channel), sizeof(int));
    
    picoThread sender = picoThreadCreate(channelSender, channel);
    picoThread receiver = picoThreadCreate(channelReceiver, channel);
    
    picoThreadJoin(sender, PICO_THREAD_INFINITE);
    picoThreadJoin(receiver, PICO_THREAD_INFINITE);
    
    picoThreadDestroy(sender);
    picoThreadDestroy(receiver);
    
    printf("Final pending items: %u\n", picoThreadChannelGetPendingItemCount(channel));
    
    picoThreadChannelDestroy(channel);
    printf("Bounded channel demonstration completed!\n");
}

void unboundedChannelProducer(void *arg)
{
    picoThreadChannel channel = (picoThreadChannel)arg;
    
    for (int i = 1; i <= 20; i++) {
        printf("[Producer] Sending item %d\n", i);
        picoThreadChannelSend(channel, &i);
        picoThreadSleep(50);
    }
    
    // Send sentinel value to signal completion
    int sentinel = -1;
    picoThreadChannelSend(channel, &sentinel);
    printf("[Producer] Sent sentinel value, done!\n");
}

void unboundedChannelConsumer(void *arg)
{
    picoThreadChannel channel = (picoThreadChannel)arg;
    int itemsConsumed = 0;
    
    picoThreadSleep(500); // Start late to allow buffer to fill
    
    while (true) {
        int value = 0;
        
        if (picoThreadChannelTryReceive(channel, &value)) {
            if (value == -1) {
                printf("[Consumer] Received sentinel, stopping...\n");
                break;
            }
            itemsConsumed++;
            printf("[Consumer] Consumed: %d (total: %d, pending: %u)\n",
                   value, itemsConsumed, picoThreadChannelGetPendingItemCount(channel));
        } else {
            printf("[Consumer] No items available, waiting...\n");
            picoThreadSleep(100);
        }
    }
    
    printf("[Consumer] Consumed %d items total!\n", itemsConsumed);
}

void demonstrateUnboundedChannel(void)
{
    printf("Unbounded Channel Communication\n");
    
    picoThreadChannel channel = picoThreadChannelCreateUnbounded(sizeof(int));
    
    if (!channel) {
        printf("Failed to create unbounded channel!\n");
        return;
    }
    
    printf("Created unbounded channel (item size: %zu bytes)\n", sizeof(int));
    
    picoThread producer = picoThreadCreate(unboundedChannelProducer, channel);
    picoThread consumer = picoThreadCreate(unboundedChannelConsumer, channel);
    
    picoThreadJoin(producer, PICO_THREAD_INFINITE);
    picoThreadJoin(consumer, PICO_THREAD_INFINITE);
    
    picoThreadDestroy(producer);
    picoThreadDestroy(consumer);
    
    picoThreadChannelDestroy(channel);
    printf("Unbounded channel demonstration completed!\n");
}

typedef struct {
    int workerId;
    char message[64];
    uint64_t timestamp;
} WorkMessage;

void multiChannelWorker(void *arg)
{
    picoThreadChannel channel = (picoThreadChannel)arg;
    int workerId = (int)(picoThreadGetCurrentId() % 1000);
    
    for (int i = 0; i < 5; i++) {
        WorkMessage msg;
        msg.workerId = workerId;
        msg.timestamp = (uint64_t)(i * 100);
        snprintf(msg.message, sizeof(msg.message), "Task %d from worker %d", i + 1, workerId);
        
        printf("[Worker %d] Sending: %s\n", workerId, msg.message);
        picoThreadChannelSend(channel, &msg);
        
        picoThreadSleep(100 + (workerId % 3) * 50);
    }
}

void multiChannelCollector(void *arg)
{
    picoThreadChannel channel = (picoThreadChannel)arg;
    int totalReceived = 0;
    int consecutiveTimeouts = 0;
    
    while (consecutiveTimeouts < 3) {
        WorkMessage msg;
        
        if (picoThreadChannelReceive(channel, &msg, 500)) {
            totalReceived++;
            consecutiveTimeouts = 0;
            printf("[Collector] Received from worker %d: %s (timestamp: %" PRIu64 ")\n",
                   msg.workerId, msg.message, msg.timestamp);
        } else {
            consecutiveTimeouts++;
            printf("[Collector] Timeout %d/3, pending: %u\n",
                   consecutiveTimeouts, picoThreadChannelGetPendingItemCount(channel));
        }
    }
    
    printf("[Collector] Collected %d messages total!\n", totalReceived);
}

void demonstrateMultipleProducers(void)
{
    printf("Multiple Producers, Single Consumer Pattern\n");
    
    picoThreadChannel channel = picoThreadChannelCreateBounded(10, sizeof(WorkMessage));
    
    if (!channel) {
        printf("Failed to create channel!\n");
        return;
    }
    
    printf("Created channel for WorkMessage (size: %zu bytes)\n", sizeof(WorkMessage));
    
    const int workerCount = 4;
    picoThread workers[4];
    
    picoThread collector = picoThreadCreate(multiChannelCollector, channel);
    
    for (int i = 0; i < workerCount; i++) {
        workers[i] = picoThreadCreate(multiChannelWorker, channel);
        picoThreadSleep(50);
    }
    
    for (int i = 0; i < workerCount; i++) {
        picoThreadJoin(workers[i], PICO_THREAD_INFINITE);
        picoThreadDestroy(workers[i]);
    }
    
    picoThreadJoin(collector, PICO_THREAD_INFINITE);
    picoThreadDestroy(collector);
    
    picoThreadChannelDestroy(channel);
    printf("Multiple producers demonstration completed!\n");
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
    demonstrateBoundedChannel();
    demonstrateUnboundedChannel();
    demonstrateMultipleProducers();

    printf("Goodbye, Pico!\n");
    return 0;
}
