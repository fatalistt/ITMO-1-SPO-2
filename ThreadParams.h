#include <stdexcept>
#include "helpers.h"

class ThreadParams
{
public:
    int mutexLockTimeout;
    pthread_barrier_t *threadsStartedBarrier;
    pthread_barrier_t *canWorkBarrier;
    bool shouldBreak;
    pthread_mutex_t *shouldBreakMutex;

protected:
    ThreadParams(int mutexLockTimeout, unsigned threadsCount) : mutexLockTimeout(mutexLockTimeout),
                                                                threadsStartedBarrier(new pthread_barrier_t()),
                                                                canWorkBarrier(new pthread_barrier_t()),
                                                                shouldBreak(false),
                                                                shouldBreakMutex(new pthread_mutex_t())
    {
        init(threadsStartedBarrier, threadsCount + 1);
        init(canWorkBarrier, threadsCount + 1);
        init(shouldBreakMutex);
    }

    ~ThreadParams()
    {
        destroy(shouldBreakMutex);
        destroy(canWorkBarrier);
        destroy(threadsStartedBarrier);

        delete shouldBreakMutex;
        delete canWorkBarrier;
        delete threadsStartedBarrier;
    }
};

class ConsumerThreadParams : public ThreadParams
{
public:
    const long sleepMilliseconds;
    const bool isDebugEnabled;
    const long *sharedVariable;
    bool *consumed;
    pthread_cond_t *consumedCond;
    pthread_mutex_t *consumedMutex;

    ConsumerThreadParams(
        int mutexLockTimeout,
        int threadsCount,
        long sleepMilliseconds,
        bool isDebugEnabled,
        const long *sharedVariable,
        bool *consumed,
        pthread_cond_t *consumedCond,
        pthread_mutex_t *consumedMutex) : ThreadParams(mutexLockTimeout, threadsCount),
                                          sleepMilliseconds(sleepMilliseconds),
                                          isDebugEnabled(isDebugEnabled),
                                          sharedVariable(sharedVariable),
                                          consumed(consumed),
                                          consumedCond(consumedCond),
                                          consumedMutex(consumedMutex)
    {
        if (sleepMilliseconds < 0)
        {
            throw std::invalid_argument(nameof(sleepMilliseconds));
        }
        if (sharedVariable == nullptr)
        {
            throw std::invalid_argument(nameof(sharedVariable));
        }
        if (consumedCond == nullptr)
        {
            throw std::invalid_argument(nameof(consumedCond));
        }
        if (consumedMutex == nullptr)
        {
            throw std::invalid_argument(nameof(consumedMutex));
        }
    }
};

class ProducerThreadParams : public ThreadParams
{
public:
    long *sharedVariable;
    bool *consumed;
    pthread_cond_t *consumedCond;
    pthread_mutex_t *consumedMutex;

    ProducerThreadParams(
        int mutexLockTimeout,
        int threadsCount,
        long *sharedVariable,
        bool *consumed,
        pthread_cond_t *consumedCond,
        pthread_mutex_t *consumedMutex) : ThreadParams(mutexLockTimeout, threadsCount),
                                          sharedVariable(sharedVariable),
                                          consumed(consumed),
                                          consumedCond(consumedCond),
                                          consumedMutex(consumedMutex)
    {
        if (sharedVariable == nullptr)
        {
            throw std::invalid_argument(nameof(sharedVariable));
        }
        if (consumedCond == nullptr)
        {
            throw std::invalid_argument(nameof(consumedCond));
        }
        if (consumedMutex == nullptr)
        {
            throw std::invalid_argument(nameof(consumedMutex));
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
        int thisThreadsCount,
        size_t threadsCount,
        const pthread_t threads[]) : ThreadParams(mutexLockTimeout, thisThreadsCount),
                                     threadsCount(threadsCount),
                                     threads(threads)
    {
        if (threadsCount <= 0)
        {
            throw std::invalid_argument(nameof(threadsCount));
        }
        if (threads == nullptr)
        {
            throw std::invalid_argument(nameof(threads));
        }
    }
};