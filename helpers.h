#pragma once
#define nameof(name) #name

#include <pthread.h>
#include <functional>

template <typename... TArgs>
void executeAndCheckReturnCode(int (*func)(TArgs...), TArgs... args)
{
    auto returnCode = func(args...);
    if (returnCode != 0)
    {
        throw std::runtime_error(std::string("Non-zero return code: ").append(std::to_string(returnCode)));
    }
}

timespec getAbsTime(int timeout)
{
    timespec res;
    clock_gettime(CLOCK_REALTIME, &res);
    res.tv_sec += timeout;
    return res;
}

void lock(pthread_mutex_t *mutex, int timeout)
{
    auto absTime = getAbsTime(timeout);
    executeAndCheckReturnCode(pthread_mutex_timedlock, mutex, (const timespec *)&absTime);
}

void wait(pthread_cond_t *cond, pthread_mutex_t *mutex, int timeout)
{
    auto absTime = getAbsTime(timeout);
    executeAndCheckReturnCode(pthread_cond_timedwait, cond, mutex, (const timespec *)&absTime);
}

void wait(pthread_barrier_t *barrier)
{
    auto returnCode = pthread_barrier_wait(barrier);
    if (returnCode != 0 && returnCode != PTHREAD_BARRIER_SERIAL_THREAD)
    {
        throw std::runtime_error(std::string("Non-zero return code: ").append(std::to_string(returnCode)));
    }
}

void unlock(pthread_mutex_t *mutex)
{
    executeAndCheckReturnCode(pthread_mutex_unlock, mutex);
}

void waitWhile(bool *value, pthread_cond_t *cond, pthread_mutex_t *mutex, int timeout)
{
    lock(mutex, timeout);
    while (*value)
    {
        wait(cond, mutex, timeout);
    }
}

void waitWhile(std::function<bool()> predicate, pthread_cond_t *cond, pthread_mutex_t *mutex, int timeout)
{
    lock(mutex, timeout);
    while (predicate())
    {
        wait(cond, mutex, timeout);
    }
}

void waitUntil(bool *value, pthread_cond_t *cond, pthread_mutex_t *mutex, int timeout)
{
    lock(mutex, timeout);
    while (!*value)
    {
        wait(cond, mutex, timeout);
    }
}

void waitUntilAndUnlock(bool *value, pthread_cond_t *cond, pthread_mutex_t *mutex, int timeout)
{
    waitUntil(value, cond, mutex, timeout);
    unlock(mutex);
}

void signal(pthread_cond_t *cond)
{
    executeAndCheckReturnCode(pthread_cond_signal, cond);
}

void signalAll(pthread_cond_t *cond)
{
    executeAndCheckReturnCode(pthread_cond_broadcast, cond);
}

void create(pthread_t *thread, void *(*routine)(void *), void *params)
{
    executeAndCheckReturnCode(pthread_create, thread, (const pthread_attr_t *)nullptr, routine, params);
}

void init(pthread_mutex_t *mutex)
{
    executeAndCheckReturnCode(pthread_mutex_init, mutex, (const pthread_mutexattr_t *)nullptr);
}

void init(pthread_cond_t *cond)
{
    executeAndCheckReturnCode(pthread_cond_init, cond, (const pthread_condattr_t *)nullptr);
}

void init(pthread_barrier_t *barrier, unsigned count)
{
    executeAndCheckReturnCode(pthread_barrier_init, barrier, (const pthread_barrierattr_t *)nullptr, count);
}

void destroy(pthread_mutex_t *mutex)
{
    executeAndCheckReturnCode(pthread_mutex_destroy, mutex);
}

void destroy(pthread_cond_t *cond)
{
    executeAndCheckReturnCode(pthread_cond_destroy, cond);
}

void destroy(pthread_barrier_t *barrier)
{
    executeAndCheckReturnCode(pthread_barrier_destroy, barrier);
}

void join(pthread_t thread, void **retVal = nullptr)
{
    executeAndCheckReturnCode(pthread_join, thread, retVal);
}