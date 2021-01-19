#include <stdexcept>
#define nameof(name) #name

class ThreadParams
{
public:
    pthread_mutex_t *threadIdMutexPtr;
    bool *canWorkPtr;
    pthread_cond_t *canWorkCondPtr;
    pthread_mutex_t *canWorkMutexPtr;
    int mutexLockTimeout;

protected:
    ThreadParams(
        pthread_mutex_t *threadIdMutexPtr,
        bool *canWorkPtr,
        pthread_cond_t *canWorkCondPtr,
        pthread_mutex_t *canWorkMutexPtr,
        const int mutexLockTimeout) : threadIdMutexPtr(threadIdMutexPtr),
                                      canWorkPtr(canWorkPtr),
                                      canWorkCondPtr(canWorkCondPtr),
                                      canWorkMutexPtr(canWorkMutexPtr),
                                      mutexLockTimeout(mutexLockTimeout)
    {
        if (threadIdMutexPtr == nullptr)
        {
            throw std::invalid_argument(nameof(threadIdMutexPtr));
        }
        if (canWorkPtr == nullptr)
        {
            throw std::invalid_argument(nameof(canWorkPtr));
        }
        if (canWorkCondPtr == nullptr)
        {
            throw std::invalid_argument(nameof(canWorkCondPtr));
        }
        if (canWorkMutexPtr == nullptr)
        {
            throw std::invalid_argument(nameof(canWorkMutexPtr));
        }
    }
};

class ConsumerThreadParams : public ThreadParams
{
public:
    const long sleepMilliseconds;
    const bool isDebugEnabled;
    const long *sharedVariablePtr;
    pthread_mutex_t *consumersMutexPtr;
    int *threadStartedCountPtr;

    ConsumerThreadParams(
        pthread_mutex_t *threadIdMutexPtr,
        bool *canWorkPtr,
        pthread_cond_t *canWorkCondPtr,
        pthread_mutex_t *canWorkMutexPtr,
        const int mutexLockTimeout,
        long sleepMilliseconds,
        bool isDebugEnabled,
        const long *sharedVariablePtr,
        pthread_mutex_t *consumersMutexPtr,
        int *threadStartedCountPtr) : ThreadParams(threadIdMutexPtr, canWorkPtr, canWorkCondPtr, canWorkMutexPtr, mutexLockTimeout),
                                      sleepMilliseconds(sleepMilliseconds),
                                      isDebugEnabled(isDebugEnabled),
                                      sharedVariablePtr(sharedVariablePtr),
                                      consumersMutexPtr(consumersMutexPtr),
                                      threadStartedCountPtr(threadStartedCountPtr)

    {
        if (sleepMilliseconds < 0)
        {
            throw std::invalid_argument(nameof(sleepMilliseconds));
        }
        if (sharedVariablePtr == nullptr)
        {
            throw std::invalid_argument(nameof(sharedVariablePtr));
        }
        if (consumersMutexPtr == nullptr)
        {
            throw std::invalid_argument(nameof(consumersMutexPtr));
        }
        if (threadStartedCountPtr == nullptr)
        {
            throw std::invalid_argument(nameof(threadStartedCountPtr));
        }
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
        pthread_mutex_t *threadIdMutexPtr,
        bool *canWorkPtr,
        pthread_cond_t *canWorkCondPtr,
        pthread_mutex_t *canWorkMutexPtr,
        const int mutexLockTimeout,
        long *sharedVariablePtr,
        bool *consumedPtr,
        pthread_cond_t *consumedCondPtr,
        pthread_mutex_t *consumedMutexPtr) : ThreadParams(threadIdMutexPtr, canWorkPtr, canWorkCondPtr, canWorkMutexPtr, mutexLockTimeout),
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
    pthread_mutex_t *mutex_ptr;

    InterruptorThreadParams(
        pthread_mutex_t *threadIdMutexPtr,
        bool *canWorkPtr,
        pthread_cond_t *canWorkCondPtr,
        pthread_mutex_t *canWorkMutexPtr,
        const int mutexLockTimeout,
        size_t threadsCount,
        const pthread_t *threads) : ThreadParams(threadIdMutexPtr, canWorkPtr, canWorkCondPtr, canWorkMutexPtr, mutexLockTimeout),
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
        if (mutex_ptr == nullptr)
        {
            throw std::invalid_argument(nameof(mutex_ptr));
        }
    }
};