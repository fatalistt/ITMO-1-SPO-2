#include <stdexcept>
#include "helpers.h"

class ThreadParams
{
public:
    int mutexLockTimeout;
    bool *canWorkPtr;
    pthread_cond_t *canWorkCondPtr;
    pthread_mutex_t *canWorkMutexPtr;

protected:
    ThreadParams(const int mutexLockTimeout) : mutexLockTimeout(mutexLockTimeout)
    {
        canWorkPtr = new bool(false);
        canWorkCondPtr = new pthread_cond_t();
        canWorkMutexPtr = new pthread_mutex_t();

        init(canWorkCondPtr);
        init(canWorkMutexPtr);
    }

    ~ThreadParams()
    {
        destroy(canWorkMutexPtr);
        destroy(canWorkCondPtr);

        delete canWorkMutexPtr;
        delete canWorkCondPtr;
        delete canWorkPtr;
    }
};

class ConsumerThreadParams : public ThreadParams
{
public:
    const long sleepMilliseconds;
    const bool isDebugEnabled;
    pthread_mutex_t *threadsStartedMutexPtr;
    int *threadsStartedPtr;
    const long *sharedVariablePtr;
    bool *consumed;
    pthread_cond_t *consumedCondPtr;
    pthread_mutex_t *consumedMutexPtr;

    ConsumerThreadParams(
        const int mutexLockTimeout,
        long sleepMilliseconds,
        bool isDebugEnabled,
        const long *sharedVariablePtr,
        bool *consumed,
        pthread_cond_t *consumedCondPtr,
        pthread_mutex_t *consumedMutexPtr) : ThreadParams(mutexLockTimeout),
                                             sleepMilliseconds(sleepMilliseconds),
                                             isDebugEnabled(isDebugEnabled),
                                             sharedVariablePtr(sharedVariablePtr),
                                             consumed(consumed),
                                             consumedCondPtr(consumedCondPtr),
                                             consumedMutexPtr(consumedMutexPtr)

    {
        if (sleepMilliseconds < 0)
        {
            throw std::invalid_argument(nameof(sleepMilliseconds));
        }
        if (sharedVariablePtr == nullptr)
        {
            throw std::invalid_argument(nameof(sharedVariablePtr));
        }
        if (consumedCondPtr == nullptr)
        {
            throw std::invalid_argument(nameof(consumedCondPtr));
        }
        if (consumedMutexPtr == nullptr)
        {
            throw std::invalid_argument(nameof(consumedMutexPtr));
        }

        threadsStartedMutexPtr = new pthread_mutex_t();
        threadsStartedPtr = new int(0);

        init(threadsStartedMutexPtr);
    }

    ~ConsumerThreadParams()
    {
        destroy(threadsStartedMutexPtr);    

        delete threadsStartedPtr;
        delete threadsStartedMutexPtr;
    }
};

class ProducerThreadParams : public ThreadParams
{
public:
    long *sharedVariablePtr;
    bool *consumedPtr;
    pthread_cond_t *consumedCondPtr;
    pthread_mutex_t *consumedMutexPtr;

    ProducerThreadParams(
        const int mutexLockTimeout,
        long *sharedVariablePtr,
        bool *consumedPtr,
        pthread_cond_t *consumedCondPtr,
        pthread_mutex_t *consumedMutexPtr) : ThreadParams(mutexLockTimeout),
                                             sharedVariablePtr(sharedVariablePtr),
                                             consumedPtr(consumedPtr),
                                             consumedCondPtr(consumedCondPtr),
                                             consumedMutexPtr(consumedMutexPtr)
    {
        if (sharedVariablePtr == nullptr)
        {
            throw std::invalid_argument(nameof(sharedVariablePtr));
        }
        if (consumedCondPtr == nullptr)
        {
            throw std::invalid_argument(nameof(consumedCondPtr));
        }
        if (consumedMutexPtr == nullptr)
        {
            throw std::invalid_argument(nameof(consumedMutexPtr));
        }
    }
};

class InterruptorThreadParams : public ThreadParams
{
public:
    const size_t threadsCount;
    const pthread_t *threads;

    InterruptorThreadParams(
        const int mutexLockTimeout,
        size_t threadsCount,
        const pthread_t *threads) : ThreadParams(mutexLockTimeout),
                                    threadsCount(threadsCount),
                                    threads(threads)
    {
        if (threadsCount <= 0)
        {
            throw std::invalid_argument(nameof(threadsCount));
        }
        if (threads == nullptr)
        {
            throw std::invalid_argument(nameof(sharedVariablePtr));
        }
    }
};