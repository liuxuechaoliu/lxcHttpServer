#ifndef __MUTEX__
#define __MUTEX__
#include <pthread.h>

// 互斥锁类
class MutexLock
{
private:
    pthread_mutex_t mutex_;

public:
    MutexLock() // 定义锁
    {
        pthread_mutex_init(&mutex_, NULL);
    }
    ~MutexLock() // 销毁锁
    {
        pthread_mutex_destroy(&mutex_);
    }
    void lock() // 上锁
    {
        pthread_mutex_lock(&mutex_);
    }
    void unlock() // 开锁
    {
        pthread_mutex_unlock(&mutex_);
    }
    pthread_mutex_t *getMutex()
    {
        return &mutex_;
    }
};

/*将 MutexLockGuard 对象定义为局部变量，
在其所在的作用域结束时，会自动调用析构函数释放互斥锁*/
class MutexLockGuard
{
private:
    MutexLock &mutex_;

public:
    explicit MutexLockGuard(MutexLock &mutex) : mutex_(mutex)
    {
        mutex_.lock();
    }
    ~MutexLockGuard()
    {
        mutex_.unlock();
    }
};

class Condition
{
private:
    pthread_cond_t cond_;

public:
    explicit Condition()
    {
        pthread_cond_init(&cond_, NULL);
    }
    ~Condition()
    {
        pthread_cond_destroy(&cond_);
    }
    void wait(MutexLock &mutex_)
    {
        pthread_cond_wait(&cond_, mutex_.getMutex());
    }
    void notify()
    {
        pthread_cond_signal(&cond_);
    }
    void notifyAll()
    {
        pthread_cond_broadcast(&cond_);
        // 线程库函数调用，向所有正在等待条件变量 cond_ 的线程发送唤醒信号。
    }
};

// 等待时间默认为1秒钟
// 构造是创造条件，析构时销毁条件
class ConditionTimeWait
{
private:
    int waitSecond;
    pthread_cond_t cond_;

public:
    explicit ConditionTimeWait(int _waitSecond = 1) : waitSecond(_waitSecond)
    {
        pthread_cond_init(&cond_, NULL);
    }
    ~ConditionTimeWait()
    {
        pthread_cond_destroy(&cond_);
    }

    /*int pthread_cond_timedwait(*cond,*mutex,*timespec)接受的
    timespec是一个绝对时间，即为要停止等待的真正时间，因此要
    先获得当前时间，再加上等待的时间变为停止等待的绝对时间timespec再放入函数*/
    void waitForTime(MutexLock &mutex)
    {
        struct timespec abstime;
        clock_gettime(CLOCK_REALTIME, &abstime);

        const int64_t kNanoSecondsPerSecond = 1000000000;//纳秒和秒的换算比
        int64_t nanoseconds = waitSecond * kNanoSecondsPerSecond;//总共的等待的纳秒时间

        abstime.tv_sec += static_cast<long>((abstime.tv_nsec + nanoseconds) / kNanoSecondsPerSecond);
        abstime.tv_nsec = static_cast<long>((abstime.tv_nsec + nanoseconds) % kNanoSecondsPerSecond);

        pthread_cond_timedwait(&cond_, mutex.getMutex(), &abstime);
    }

    void notify()
    {
        pthread_cond_signal(&cond_);
    }
    void notifyAll()
    {
        pthread_cond_broadcast(&cond_);
        // 线程库函数调用，向所有正在等待条件变量 cond_ 的线程发送唤醒信号。
    }
};

#endif