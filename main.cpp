#include <iostream>
#include <cstring>
#include <pthread.h>
#include "ThreadParams.cpp"

template <typename... TArgs>
void executeAndCheckReturnCode(int (*func)(TArgs...), TArgs... args)
{
    auto returnCode = func(args...);
    if (returnCode != 0)
    {
        throw std::runtime_error(std::string("Non-zero return code: ").append(std::to_string(returnCode)));
    }
}

int get_tid(pthread_mutex_t *mutex)
{
    if (mutex == nullptr)
    {
        throw new std::invalid_argument(nameof(mutex));
    }

    static int counter = 0;
    thread_local int *id = nullptr;
    if (id == nullptr)
    {
        int returnCode = pthread_mutex_lock(mutex);
        if (returnCode != 0)
        {
            throw std::runtime_error(std::string("pthread_mutex_lock returned ").append(std::to_string(returnCode)));
        }

        id = new int(++counter);

        returnCode = pthread_mutex_unlock(mutex);
        if (returnCode != 0)
        {
            throw std::runtime_error(std::string("pthread_mutex_unlock returned ").append(std::to_string(returnCode)));
        }
    }

    return *id;
}

timespec getAbsTime(int timeout)
{
    timespec res;
    clock_gettime(CLOCK_REALTIME, &res);
    res.tv_sec += timeout;
    return res;
}

void *producer_routine(void *args)
{
    auto params = (ProducerThreadParams *)args;
    auto threadIdMutexPtr = params->threadIdMutexPtr;
    auto canWorkPtr = params->canWorkPtr;
    auto canWorkCondPtr = params->canWorkCondPtr;
    auto canWorkMutexPtr = params->canWorkMutexPtr;
    auto mutexLockTimeout = params->mutexLockTimeout;
    auto sharedVariablePtr = params->sharedVariablePtr;
    auto consumedPtr = params->consumedPtr;
    auto consumedCondPtr = params->consumedCondPtr;
    auto consumedMutexPtr = params->consumedMutexPtr;
    get_tid(threadIdMutexPtr);

    auto absTime = getAbsTime(mutexLockTimeout);
    executeAndCheckReturnCode(pthread_mutex_timedlock, canWorkMutexPtr, (const timespec *)&absTime);
    while (!*canWorkPtr)
    {
        absTime = getAbsTime(mutexLockTimeout);
        executeAndCheckReturnCode(pthread_cond_timedwait, canWorkCondPtr, canWorkMutexPtr, (const timespec *)&absTime);
    }
    executeAndCheckReturnCode(pthread_mutex_unlock, canWorkMutexPtr);
    executeAndCheckReturnCode(pthread_cond_destroy, canWorkCondPtr);
    delete canWorkCondPtr;

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

        absTime = getAbsTime(mutexLockTimeout);
        input = end;
        executeAndCheckReturnCode(pthread_mutex_timedlock, consumedMutexPtr, (const timespec *)&absTime);
        while (!*consumedPtr)
        {
            absTime = getAbsTime(mutexLockTimeout);
            executeAndCheckReturnCode(pthread_cond_timedwait, consumedCondPtr, consumedMutexPtr, (const timespec *)&absTime);
        }
        (*sharedVariablePtr) = now;
        executeAndCheckReturnCode(pthread_cond_broadcast, consumedCondPtr);
        executeAndCheckReturnCode(pthread_mutex_unlock, consumedMutexPtr);
    }
    return nullptr;
}

void *consumer_routine(void *args)
{
    auto params = (ConsumerThreadParams *)args;
    auto threadIdMutexPtr = params->threadIdMutexPtr;
    // auto sharedVariablePtr = params->sharedVariablePtr;

    get_tid(threadIdMutexPtr);
    delete params;
    // notify about start
    // for every update issued by producer, read the value and add to sum
    // return pointer to result (for particular consumer)
    return nullptr;
}

void *consumer_interruptor_routine(void *args)
{
    auto params = (InterruptorThreadParams *)args;
    auto threadIdMutexPtr = params->threadIdMutexPtr;
    auto canWorkPtr = params->canWorkPtr;
    auto canWorkCondPtr = params->canWorkCondPtr;
    auto canWorkMutexPtr = params->canWorkMutexPtr;
    auto mutexLockTimeout = params->mutexLockTimeout;
    auto threadsCount = params->threadsCount;
    auto threads = params->threads;
    get_tid(threadIdMutexPtr);

    auto absTime = getAbsTime(mutexLockTimeout);
    executeAndCheckReturnCode(pthread_mutex_timedlock, canWorkMutexPtr, (const timespec *)&absTime);
    while (!*canWorkPtr)
    {
        absTime = getAbsTime(mutexLockTimeout);
        executeAndCheckReturnCode(pthread_cond_timedwait, canWorkCondPtr, canWorkMutexPtr, (const timespec *)&absTime);
    }
    executeAndCheckReturnCode(pthread_mutex_unlock, canWorkMutexPtr);
    executeAndCheckReturnCode(pthread_cond_destroy, canWorkCondPtr);
    delete canWorkCondPtr;

    while (true)
    {
        executeAndCheckReturnCode(pthread_cancel, threads[std::rand() % threadsCount]);

        absTime = getAbsTime(mutexLockTimeout);
        executeAndCheckReturnCode(pthread_mutex_timedlock, canWorkMutexPtr, (const timespec *)&absTime);
        auto shouldBreak = !*canWorkPtr;
        executeAndCheckReturnCode(pthread_mutex_unlock, canWorkMutexPtr);

        if (shouldBreak)
        {
            return nullptr;
        }
    }
}

int run_threads(int count, long sleepMs, bool isDebugEnabled)
{
    auto threads = new pthread_t[count];
    auto sharedVar = 0L;
    const int timeout = 300;

    pthread_mutex_t threadIdMutex;
    executeAndCheckReturnCode(pthread_mutex_init, &threadIdMutex, (const pthread_mutexattr_t *)nullptr);

    pthread_t producer;
    auto producerParams = new ProducerThreadParams(
        &threadIdMutex,
        new bool(false),
        new pthread_cond_t(),
        new pthread_mutex_t(),
        timeout,
        &sharedVar,
        new bool(true),
        new pthread_cond_t(),
        new pthread_mutex_t());
    executeAndCheckReturnCode(pthread_cond_init, producerParams->canWorkCondPtr, (const pthread_condattr_t *)nullptr);
    executeAndCheckReturnCode(pthread_mutex_init, producerParams->canWorkMutexPtr, (const pthread_mutexattr_t *)nullptr);
    executeAndCheckReturnCode(pthread_cond_init, producerParams->consumedCondPtr, (const pthread_condattr_t *)nullptr);
    executeAndCheckReturnCode(pthread_mutex_init, producerParams->consumedMutexPtr, (const pthread_mutexattr_t *)nullptr);
    executeAndCheckReturnCode(pthread_create, &producer, (const pthread_attr_t *)nullptr, producer_routine, (void *)producerParams);

    pthread_t interruptor;
    auto interruptorParams = new InterruptorThreadParams(
        &threadIdMutex,
        new bool(false),
        new pthread_cond_t(),
        new pthread_mutex_t(),
        timeout,
        count,
        threads);
    executeAndCheckReturnCode(pthread_cond_init, interruptorParams->canWorkCondPtr, (const pthread_condattr_t *)nullptr);
    executeAndCheckReturnCode(pthread_mutex_init, interruptorParams->canWorkMutexPtr, (const pthread_mutexattr_t *)nullptr);
    executeAndCheckReturnCode(pthread_create, &interruptor, (const pthread_attr_t *)nullptr, consumer_interruptor_routine, (void *)interruptorParams);

    auto consumersStartedCount = 0;
    auto consumersParams = new ConsumerThreadParams(
        &threadIdMutex,
        new bool(false),
        new pthread_cond_t(),
        new pthread_mutex_t(),
        timeout,
        sleepMs,
        isDebugEnabled,
        &sharedVar,
        new pthread_mutex_t(),
        &consumersStartedCount);
    executeAndCheckReturnCode(pthread_cond_init, consumersParams->canWorkCondPtr, (const pthread_condattr_t *)nullptr);
    executeAndCheckReturnCode(pthread_mutex_init, consumersParams->canWorkMutexPtr, (const pthread_mutexattr_t *)nullptr);
    executeAndCheckReturnCode(pthread_mutex_init, consumersParams->consumersMutexPtr, (const pthread_mutexattr_t *)nullptr);
    for (auto i = 0; i < count; ++i)
    {
        executeAndCheckReturnCode(pthread_create, &threads[i], (const pthread_attr_t *)nullptr, consumer_routine, (void *)consumersParams);
    }
    executeAndCheckReturnCode(pthread_join, producer, (void **)nullptr);

    auto absTime = getAbsTime(timeout);
    executeAndCheckReturnCode(pthread_mutex_timedlock, interruptorParams->canWorkMutexPtr, (const timespec *)&absTime);
    *(interruptorParams->canWorkPtr) = false;
    executeAndCheckReturnCode(pthread_mutex_unlock, interruptorParams->canWorkMutexPtr);

    executeAndCheckReturnCode(pthread_join, interruptor, (void **)nullptr);

    absTime = getAbsTime(timeout);
    executeAndCheckReturnCode(pthread_mutex_timedlock, consumersParams->canWorkMutexPtr, (const timespec *)&absTime);
    *(consumersParams->canWorkPtr) = false;
    executeAndCheckReturnCode(pthread_mutex_unlock, consumersParams->canWorkMutexPtr);

    int totalSum = 0;
    for (auto i = 0; i < count; ++i)
    {
        int sum;
        executeAndCheckReturnCode(pthread_join, threads[i], (void **)&sum);
        totalSum += sum;
    }

    executeAndCheckReturnCode(pthread_cond_destroy, consumersParams->canWorkCondPtr);
    executeAndCheckReturnCode(pthread_mutex_destroy, consumersParams->canWorkMutexPtr);
    executeAndCheckReturnCode(pthread_mutex_destroy, consumersParams->consumersMutexPtr);

    executeAndCheckReturnCode(pthread_mutex_destroy, &threadIdMutex);
    return totalSum;
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
    if (argc >= 3)
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