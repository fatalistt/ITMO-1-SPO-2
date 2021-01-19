#include <pthread.h>
#include <atomic>
#include <cstring>
#include <iostream>
#include <memory>
#include <thread>
#include "helpers.h"
#include "ThreadParams.h"

int get_tid()
{
    static std::atomic_int counter = 0;
    thread_local std::unique_ptr<int> id = nullptr;
    if (id == nullptr)
    {
        id = std::make_unique<int>(++counter);
    }

    return *id;
}

void *producer_routine(void *args)
{
    auto params = (ProducerThreadParams *)args;
    get_tid();

    waitUntilAndUnlock(params->canWorkPtr, params->canWorkCondPtr, params->canWorkMutexPtr, params->mutexLockTimeout);

    std::string tmp;
    std::getline(std::cin, tmp);
    auto input = tmp.c_str();
    while (true)
    {
        errno = 0;
        char *end;
        auto now = std::strtol(input, &end, 10);
        if (input == end)
        {
            break;
        }

        if (errno == ERANGE)
        {
            throw std::invalid_argument("Overflow exception");
        }

        input = end;

        waitUntil(params->consumedPtr, params->consumedCondPtr, params->consumedMutexPtr, params->mutexLockTimeout);

        (*params->sharedVariablePtr) = now;
        (*params->consumedPtr) = false;
        signal(params->consumedCondPtr);

        unlock(params->consumedMutexPtr);
    }
    return nullptr;
}

void *consumer_routine(void *args)
{
    auto params = (ConsumerThreadParams *)args;
    pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, nullptr);

    lock(params->threadsStartedMutexPtr, params->mutexLockTimeout);
    ++(*params->threadsStartedPtr);
    unlock(params->threadsStartedMutexPtr);

    waitUntilAndUnlock(params->canWorkPtr, params->canWorkCondPtr, params->canWorkMutexPtr, params->mutexLockTimeout);

    long sum = 0;
    auto shouldBreak = false;
    for (;;)
    {
        waitWhile(
            [params, &shouldBreak]() {
                lock(params->canWorkMutexPtr, params->mutexLockTimeout);
                shouldBreak = !*params->canWorkPtr;
                unlock(params->canWorkMutexPtr);
                if (shouldBreak)
                {
                    return false;
                }
                return *params->consumed;
            },
            params->consumedCondPtr,
            params->consumedMutexPtr,
            params->mutexLockTimeout);

        if (!*params->consumed)
        {
            sum += *params->sharedVariablePtr;
            (*params->consumed) = true;
        }

        if (params->isDebugEnabled)
        {
            std::cout << "(" << get_tid() << ", " << sum << ")" << std::endl;
        }

        signalAll(params->consumedCondPtr);
        unlock(params->consumedMutexPtr);

        if (shouldBreak)
        {
            break;
        }

        if (params->sleepMilliseconds == 0)
        {
            std::this_thread::yield();
        }
        else
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(rand() % params->sleepMilliseconds + 1));
        }
    }

    pthread_exit((void *)sum);
}

void *consumer_interruptor_routine(void *args)
{
    auto params = (InterruptorThreadParams *)args;
    get_tid();

    waitUntilAndUnlock(params->canWorkPtr, params->canWorkCondPtr, params->canWorkMutexPtr, params->mutexLockTimeout);

    while (true)
    {
        executeAndCheckReturnCode(pthread_cancel, params->threads[std::rand() % params->threadsCount]);

        lock(params->canWorkMutexPtr, params->mutexLockTimeout);
        auto shouldBreak = !*params->canWorkPtr;
        unlock(params->canWorkMutexPtr);

        if (shouldBreak)
        {
            return nullptr;
        }
    }
}

void start_threads(ThreadParams *params)
{
    auto absTime = getAbsTime(params->mutexLockTimeout);
    executeAndCheckReturnCode(pthread_mutex_timedlock, params->canWorkMutexPtr, (const timespec *)&absTime);
    *(params->canWorkPtr) = true;
    executeAndCheckReturnCode(pthread_mutex_unlock, params->canWorkMutexPtr);
    executeAndCheckReturnCode(pthread_cond_broadcast, params->canWorkCondPtr);
}

int run_threads(int count, long sleepMs, bool isDebugEnabled)
{
    pthread_t producer;

    pthread_t interruptor;
    auto consumers = new pthread_t[count];
    auto sharedVar = 0L;
    const int timeout = 300;
    auto consumed = true;
    pthread_cond_t consumedCondPtr;
    pthread_mutex_t consumedMutexPtr;
    ProducerThreadParams producerParams(
        timeout,
        &sharedVar,
        &consumed,
        &consumedCondPtr,
        &consumedMutexPtr);
    InterruptorThreadParams interruptorParams(
        timeout,
        count,
        consumers);
    ConsumerThreadParams consumersParams(
        timeout,
        sleepMs,
        isDebugEnabled,
        &sharedVar,
        &consumed,
        &consumedCondPtr,
        &consumedMutexPtr);

    init(&consumedCondPtr);
    init(&consumedMutexPtr);
    create(&producer, producer_routine, (void *)&producerParams);
    create(&interruptor, consumer_interruptor_routine, (void *)&interruptorParams);

    for (auto i = 0; i < count; ++i)
    {
        create(&consumers[i], consumer_routine, (void *)&consumersParams);
    }

    for (;;)
    {
        lock(consumersParams.threadsStartedMutexPtr, timeout);
        auto started = *consumersParams.threadsStartedPtr;
        unlock(consumersParams.threadsStartedMutexPtr);
        if (started == count)
        {
            break;
        }
    }

    start_threads(&interruptorParams);
    start_threads(&producerParams);
    start_threads(&consumersParams);

    join(producer);

    lock(interruptorParams.canWorkMutexPtr, timeout);
    *(interruptorParams.canWorkPtr) = false;
    unlock(interruptorParams.canWorkMutexPtr);

    join(interruptor);

    lock(consumersParams.canWorkMutexPtr, timeout);
    *(consumersParams.canWorkPtr) = false;
    unlock(consumersParams.canWorkMutexPtr);

    waitUntil(consumersParams.consumed, consumersParams.consumedCondPtr, consumersParams.consumedMutexPtr, consumersParams.mutexLockTimeout);
    signalAll(consumersParams.consumedCondPtr);
    unlock(consumersParams.consumedMutexPtr);

    long totalSum = 0;
    for (auto i = 0; i < count; ++i)
    {
        long sum;
        join(consumers[i], (void **)&sum);
        totalSum += sum;
    }

    destroy(&consumedCondPtr);
    destroy(&consumedMutexPtr);

    delete[] consumers;

    return (int)totalSum;
}

int argToInt(char *argv, std::string message, int min = 0, int max = std::numeric_limits<int>::max())
{
    char *tmp;
    auto n = std::strtol(argv, &tmp, 10);
    if (errno == ERANGE || n < min || n > max)
    {
        throw std::invalid_argument(message);
    }

    return (int)n;
}

int main(int argc, char *argv[])
{
    if (argc != 3 && argc != 4)
    {
        std::cout << "Usage: posix (N) (MS) [-debug]" << std::endl;
        return 1;
    }

    auto n = argToInt(argv[1], "1 <= N <= 1000", 1, 1000);
    auto sleepMs = argToInt(argv[2], "0 <= MS <= 1000000", 0, 1000000);
    bool isDebugEnabled = false;
    if (argc == 4)
    {
        if (std::strncmp("-debug", argv[3], std::strlen("-debug")) != 0)
        {
            throw std::invalid_argument("Invalid argument. '-debug' expected.");
        }
        isDebugEnabled = true;
    }

    std::cout << run_threads(n, sleepMs, isDebugEnabled) << std::endl;
    return 0;
}