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

#ifndef PICO_THREADS_H
#define PICO_THREADS_H

#ifndef PICO_MALLOC
#define PICO_MALLOC(sz) malloc(sz)
#define PICO_FREE(ptr)  free(ptr)
#endif

#if defined(_WIN32) || defined(_WIN64)
#define PICO_THREADS_WINDOWS
#elif defined(__unix__) || defined(__APPLE__)
#define PICO_THREADS_POSIX
#else
#error "Unsupported platform for picoThreads"
#endif

#define PICO_THREAD_INFINITE UINT32_MAX

#ifndef PICO_THREAD_NO_THREADPOOL

#ifndef PICO_THREAD_MAX_POOL_TASKS
#define PICO_THREAD_MAX_POOL_TASKS 65536
#endif

#ifndef PICO_THREAD_MAX_POOL_THREADS
#define PICO_THREAD_MAX_POOL_THREADS 64
#endif

#endif // PICO_THREAD_NO_THREADPOOL

#include <stdbool.h>
#include <stdint.h>

typedef struct picoThread_t picoThread_t;
typedef picoThread_t *picoThread;

typedef struct picoThreadMutex_t picoThreadMutex_t;
typedef picoThreadMutex_t *picoThreadMutex;

typedef struct picoThreadPool_t picoThreadPool_t;
typedef picoThreadPool_t *picoThreadPool;

typedef void (*picoThreadFunction)(void *arg);

typedef uint64_t picoThreadId;

// NOTE: The value returned by this function must be freed using picoThreadDestroy
// otherwise it will cause a memory leak. Freeing this object does NOT terminate the thread.
picoThread picoThreadCreate(picoThreadFunction function, void *arg);
void picoThreadDestroy(picoThread thread);
void picoThreadJoin(picoThread thread, uint32_t timeoutMilliseconds);
void picoThreadSleep(uint32_t milliseconds);
void picoThreadYield(void);
bool picoThreadJoinable(picoThread thread);
bool picoThreadIsCurrent(picoThread thread);
bool picoThreadIsAlive(picoThread thread);
picoThreadId picoThreadGetId(picoThread thread);
picoThreadId picoThreadGetCurrentId(void);

picoThreadMutex picoThreadMutexCreate(void);
void picoThreadMutexDestroy(picoThreadMutex mutex);
void picoThreadMutexLock(picoThreadMutex mutex, uint32_t timeoutMilliseconds);
bool picoThreadMutexTryLock(picoThreadMutex mutex);
void picoThreadMutexUnlock(picoThreadMutex mutex);

#ifndef PICO_THREAD_NO_THREADPOOL

picoThreadPool picoThreadPoolCreate(uint32_t threadCount);
void picoThreadPoolDestroy(picoThreadPool pool);
// This method will wait if the task queue is full
void picoThreadPoolAddTask(picoThreadPool pool, picoThreadFunction function, void *arg, uint32_t timeoutMilliseconds); 
void picoThreadPoolWaitAll(picoThreadPool pool);
// void picoThreadPoolForEach(picoThreadPool pool, void **items, uint32_t itemCount, void (*function)(void *item, void *userData), void *userData);
uint32_t picoThreadPoolGetThreadCount(picoThreadPool pool);
uint32_t picoThreadPoolGetPendingTaskCount(picoThreadPool pool);
uint32_t picoThreadPoolGetActiveThreadCount(picoThreadPool pool);

#endif // PICO_THREAD_NO_THREADPOOL

#if defined(PICO_IMPLEMENTATION) && !defined(PICO_THREADS_IMPLEMENTATION)
#define PICO_THREADS_IMPLEMENTATION
#endif

#ifdef PICO_THREADS_IMPLEMENTATION

#ifdef PICO_THREADS_WINDOWS

#include <Windows.h>
#include <process.h>

struct picoThread_t {
    HANDLE handle;
    unsigned int threadId;
    bool joinable;
    void *userData;
    bool isAlive;
    picoThreadFunction function;
};

struct picoThreadMutex_t {
    HANDLE mutex;
};

static unsigned __picoThreadFunctionWrapper(void *arg)
{
    picoThread thread = (picoThread)arg;
    thread->threadId  = GetCurrentThreadId();
    thread->isAlive   = true;
    if (thread && thread->userData && thread->function) {
        thread->function(thread->userData);
    }
    thread->isAlive = false;
    return 0;
}

picoThread picoThreadCreate(picoThreadFunction function, void *arg)
{
    picoThread thread = (picoThread)PICO_MALLOC(sizeof(picoThread_t));
    if (!thread) {
        return NULL;
    }

    thread->joinable = true;
    thread->userData = arg;
    thread->function = function;
    thread->isAlive  = false;
    thread->handle   = (HANDLE)_beginthreadex(
        NULL,
        0,
        (unsigned(__stdcall *)(void *))__picoThreadFunctionWrapper,
        thread,
        0,
        &thread->threadId);

    return thread;
}

void picoThreadDestroy(picoThread thread)
{
    if (!thread) {
        return;
    }

    if (thread->joinable && thread->isAlive) {
        picoThreadJoin(thread, INFINITE);
    }

    PICO_FREE(thread);
}

void picoThreadJoin(picoThread thread, uint32_t timeoutMilliseconds)
{
    if (!thread || !thread->joinable) {
        return;
    }

    WaitForSingleObject((HANDLE)thread->handle, timeoutMilliseconds);

    thread->joinable = false;
}

void picoThreadSleep(uint32_t milliseconds)
{
    Sleep(milliseconds);
}

void picoThreadYield(void)
{
    SwitchToThread();
}

bool picoThreadJoinable(picoThread thread)
{
    if (!thread) {
        return false;
    }
    return thread->joinable;
}

bool picoThreadIsCurrent(picoThread thread)
{
    if (!thread) {
        return false;
    }
    return GetCurrentThreadId() == thread->threadId;
}

bool picoThreadIsAlive(picoThread thread)
{
    if (!thread) {
        return false;
    }
    return thread->isAlive;
}

picoThreadId picoThreadGetId(picoThread thread)
{
    if (!thread) {
        return 0;
    }
    return (picoThreadId)thread->threadId;
}

picoThreadId picoThreadGetCurrentId(void)
{
    return (picoThreadId)GetCurrentThreadId();
}

picoThreadMutex picoThreadMutexCreate(void)
{
    picoThreadMutex mutex = (picoThreadMutex)PICO_MALLOC(sizeof(picoThreadMutex_t));
    if (!mutex) {
        return NULL;
    }
    mutex->mutex = CreateMutex(NULL, FALSE, NULL);
    if (!mutex->mutex) {
        PICO_FREE(mutex);
        return NULL;
    }
    return mutex;
}

void picoThreadMutexDestroy(picoThreadMutex mutex)
{
    if (!mutex) {
        return;
    }
    CloseHandle(mutex->mutex);
    PICO_FREE(mutex);
}

void picoThreadMutexLock(picoThreadMutex mutex, uint32_t timeoutMilliseconds)
{
    if (!mutex) {
        return;
    }
    WaitForSingleObject(mutex->mutex, timeoutMilliseconds);
}

bool picoThreadMutexTryLock(picoThreadMutex mutex)
{
    if (!mutex) {
        return false;
    }
    DWORD result = WaitForSingleObject(mutex->mutex, 0);
    return (result == WAIT_OBJECT_0);
}

void picoThreadMutexUnlock(picoThreadMutex mutex)
{
    if (!mutex) {
        return;
    }
    ReleaseMutex(mutex->mutex);
}

#endif // PICO_THREADS_WINDOWS

#ifdef PICO_THREADS_POSIX

#include <pthread.h>
#include <time.h>
#include <unistd.h>


struct picoThread_t {
    pthread_t handle;
    bool joinable;
    void *userData;
    bool isAlive;
    picoThreadFunction function;
};

struct picoThreadMutex_t {
    pthread_mutex_t mutex;
};

static void* __picoThreadFunctionWrapper(void *arg)
{
    picoThread thread = (picoThread)arg;
    thread->isAlive   = true;
    if (thread && thread->userData && thread->function) {
        thread->function(thread->userData);
    }
    thread->isAlive = false;
    return NULL;
}

picoThread picoThreadCreate(picoThreadFunction function, void *arg)
{
    picoThread thread = (picoThread)PICO_MALLOC(sizeof(picoThread_t));
    if (!thread) {
        return NULL;
    }

    thread->joinable = true;
    thread->userData = arg;
    thread->function = function;
    thread->isAlive  = false;

    int result = pthread_create(&thread->handle, NULL, __picoThreadFunctionWrapper, thread);
    if (result != 0) {
        PICO_FREE(thread);
        return NULL;
    }

    return thread;
}

void picoThreadDestroy(picoThread thread)
{
    if (!thread) {
        return;
    }

    if (thread->joinable && thread->isAlive) {
        picoThreadJoin(thread, PICO_THREAD_INFINITE);
    }

    PICO_FREE(thread);
}

void picoThreadJoin(picoThread thread, uint32_t timeoutMilliseconds)
{
    if (!thread || !thread->joinable) {
        return;
    }

    if (timeoutMilliseconds == PICO_THREAD_INFINITE) {
        pthread_join(thread->handle, NULL);
    } else {
        // pthread doesn't have built-in timeout, so we poll
        uint32_t elapsed            = 0;
        const uint32_t pollInterval = 10; // 10ms poll interval

        while (thread->isAlive && elapsed < timeoutMilliseconds) {
            picoThreadSleep(pollInterval);
            elapsed += pollInterval;
        }

        if (!thread->isAlive) {
            pthread_join(thread->handle, NULL);
        }
    }

    thread->joinable = false;
}

void picoThreadSleep(uint32_t milliseconds)
{
    struct timespec ts;
    ts.tv_sec  = milliseconds / 1000;
    ts.tv_nsec = (milliseconds % 1000) * 1000000;
    nanosleep(&ts, NULL);
}

void picoThreadYield()
{
    sched_yield();
}

bool picoThreadJoinable(picoThread thread)
{
    if (!thread) {
        return false;
    }
    return thread->joinable;
}

bool picoThreadIsCurrent(picoThread thread)
{
    if (!thread) {
        return false;
    }
    return pthread_equal(pthread_self(), thread->handle);
}

bool picoThreadIsAlive(picoThread thread)
{
    if (!thread) {
        return false;
    }
    return thread->isAlive;
}

picoThreadId picoThreadGetId(picoThread thread)
{
    if (!thread) {
        return 0;
    }
    return (picoThreadId)thread->handle;
}

picoThreadId picoThreadGetCurrentId()
{
    return (picoThreadId)pthread_self();
}

picoThreadMutex picoThreadMutexCreate()
{
    picoThreadMutex mutex = (picoThreadMutex)PICO_MALLOC(sizeof(picoThreadMutex_t));
    if (!mutex) {
        return NULL;
    }

    int result = pthread_mutex_init(&mutex->mutex, NULL);
    if (result != 0) {
        PICO_FREE(mutex);
        return NULL;
    }

    return mutex;
}

void picoThreadMutexDestroy(picoThreadMutex mutex)
{
    if (!mutex) {
        return;
    }
    pthread_mutex_destroy(&mutex->mutex);
    PICO_FREE(mutex);
}

void picoThreadMutexLock(picoThreadMutex mutex, uint32_t timeoutMilliseconds)
{
    if (!mutex) {
        return;
    }

    if (timeoutMilliseconds == PICO_THREAD_INFINITE) {
        pthread_mutex_lock(&mutex->mutex);
    } else {
        // pthread doesn't have timeout lock in all POSIX versions, so we use trylock with polling
        uint32_t elapsed            = 0;
        const uint32_t pollInterval = 1; // 1ms poll interval

        while (elapsed < timeoutMilliseconds) {
            if (pthread_mutex_trylock(&mutex->mutex) == 0) {
                return;
            }
            picoThreadSleep(pollInterval);
            elapsed += pollInterval;
        }
    }
}

bool picoThreadMutexTryLock(picoThreadMutex mutex)
{
    if (!mutex) {
        return false;
    }
    return pthread_mutex_trylock(&mutex->mutex) == 0;
}

void picoThreadMutexUnlock(picoThreadMutex mutex)
{
    if (!mutex) {
        return;
    }
    pthread_mutex_unlock(&mutex->mutex);
}

#endif // PICO_THREADS_POSIX

#ifndef PICO_THREAD_NO_THREADPOOL

typedef struct {
    picoThreadPool pool;
    bool running;
    uint32_t index;
} picoThreadPoolWorkerArg_t;
typedef picoThreadPoolWorkerArg_t *picoThreadPoolWorkerArg;

struct picoThreadPool_t {
    picoThread threads[PICO_THREAD_MAX_POOL_THREADS];
    picoThreadFunction tasks[PICO_THREAD_MAX_POOL_TASKS];
    void *taskArgs[PICO_THREAD_MAX_POOL_TASKS];
    bool busy[PICO_THREAD_MAX_POOL_THREADS];
    picoThreadPoolWorkerArg_t workerArgs[PICO_THREAD_MAX_POOL_THREADS];

    uint32_t threadCount;
    uint32_t taskCount;
    picoThreadMutex mutex;
};

static void __picoThreadPoolWorker(void *arg)
{
    picoThreadPoolWorkerArg workerArg = (picoThreadPoolWorkerArg)arg;

    while (workerArg->running) {
        picoThreadFunction task = NULL;
        void *taskArg           = NULL;

        picoThreadMutexLock(workerArg->pool->mutex, PICO_THREAD_INFINITE);
        if (workerArg->pool->taskCount > 0) {
            task    = workerArg->pool->tasks[workerArg->pool->taskCount - 1];
            taskArg = workerArg->pool->taskArgs[workerArg->pool->taskCount - 1];
            workerArg->pool->taskCount--;
            workerArg->pool->busy[workerArg->index] = true;
        } else {
            workerArg->pool->busy[workerArg->index] = false;
        }
        picoThreadMutexUnlock(workerArg->pool->mutex);

        if (task) {
            task(taskArg);
        } else {
            picoThreadSleep(10); // Sleep briefly if no task
        }
    }

}

picoThreadPool picoThreadPoolCreate(uint32_t threadCount)
{
    if (threadCount == 0 || threadCount > PICO_THREAD_MAX_POOL_THREADS) {
        return NULL;
    }

    picoThreadPool pool = (picoThreadPool)PICO_MALLOC(sizeof(picoThreadPool_t));
    if (!pool) {
        return NULL;
    }

    pool->threadCount = threadCount;
    pool->taskCount   = 0;
    pool->mutex       = picoThreadMutexCreate();
    if (!pool->mutex) {
        PICO_FREE(pool);
        return NULL;
    }

    for (uint32_t i = 0; i < threadCount; i++) {
        pool->workerArgs[i].pool  = pool;
        pool->workerArgs[i].index = i;
        pool->workerArgs[i].running = true;
        pool->threads[i] = picoThreadCreate(__picoThreadPoolWorker, &pool->workerArgs[i]);
        pool->busy[i]    = false;
    }

    return pool;
}

void picoThreadPoolDestroy(picoThreadPool pool)
{
    if (!pool) {
        return;
    }

    picoThreadPoolWaitAll(pool);

    for (uint32_t i = 0; i < pool->threadCount; i++) {
        pool->workerArgs[i].running = false;
        picoThreadDestroy(pool->threads[i]);
    }

    picoThreadMutexDestroy(pool->mutex);
    PICO_FREE(pool);
}

void picoThreadPoolAddTask(picoThreadPool pool, picoThreadFunction function, void *arg, uint32_t timeoutMilliseconds)
{
    if (!pool || !function) {
        return;
    }

    uint32_t elapsed            = 0;
    const uint32_t pollInterval = 10; // 10ms poll interval
    while (elapsed < timeoutMilliseconds) {
        picoThreadMutexLock(pool->mutex, timeoutMilliseconds);
        if (pool->taskCount < PICO_THREAD_MAX_POOL_TASKS) {
            pool->tasks[pool->taskCount]    = function;
            pool->taskArgs[pool->taskCount] = arg;
            pool->taskCount++;
            picoThreadMutexUnlock(pool->mutex);
            return;
        }
        picoThreadMutexUnlock(pool->mutex);
        picoThreadSleep(pollInterval);
        elapsed += pollInterval;
    }
}

void picoThreadPoolWaitAll(picoThreadPool pool)
{
    if (!pool) {
        return;
    }

    while (true) {
        picoThreadMutexLock(pool->mutex, PICO_THREAD_INFINITE);
        bool allDone = (pool->taskCount == 0);
        for (uint32_t i = 0; i < pool->threadCount; i++) {
            if (pool->busy[i]) {
                allDone = false;
                break;
            }
        }
        picoThreadMutexUnlock(pool->mutex);

        if (allDone) {
            break;
        }

        picoThreadSleep(10);
    }
}

uint32_t picoThreadPoolGetThreadCount(picoThreadPool pool)
{
    if (!pool) {
        return 0;
    }
    return pool->threadCount;
}

uint32_t picoThreadPoolGetPendingTaskCount(picoThreadPool pool)
{
    if (!pool) {
        return 0;
    }
    return pool->taskCount;
}

uint32_t picoThreadPoolGetActiveThreadCount(picoThreadPool pool)
{
    if (!pool) {
        return 0;
    }
    uint32_t activeCount = 0;
    picoThreadMutexLock(pool->mutex, PICO_THREAD_INFINITE);
    for (uint32_t i = 0; i < pool->threadCount; i++) {
        if (pool->busy[i]) {
            activeCount++;
        }
    }
    picoThreadMutexUnlock(pool->mutex);
    return activeCount;
}

#endif // PICO_THREAD_NO_THREADPOOL

#endif // PICO_THREADS_IMPLEMENTATION

#endif // PICO_THREADS_H
