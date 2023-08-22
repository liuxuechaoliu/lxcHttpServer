#include "ThreadPool.h"


ThreadPool::ThreadPool(void (*task)(TaskData), int threadNum, int maxWorkListLen)
    :task(task), threadNum(threadNum), maxWorkListLen(maxWorkListLen),
    threads(threadNum), running(true)
{
    for(int i = 0; i < threadNum; i++)
    {
        pthread_create(&threads[i], nullptr, workerWarper, this);
    }
}

void * ThreadPool::workerWarper(void * arg)
{
    ThreadPool* threadpool = static_cast<ThreadPool*> (arg);
    threadpool->worker();
    return nullptr;
}

void ThreadPool::worker()
{
    while (running == true)
    {
        TaskData taskdata(-1, nullptr);
        {
            MutexLockGuard mutexLockGuard(mutex);
            while (workList.empty() && running)
            {
                cond.wait(mutex);
            }
            /*pthread_cond_wait 通常会在一个循环中使用，因为它可能会被假唤醒。
                当 pthread_cond_wait 被假唤醒时，它会返回，但条件并没有满足。
                因此，我们需要在一个循环中检查条件是否满足，
                如果不满足循环，则再次调用 pthread_cond_wait 等待条件满足。*/
            if(!running) return;
            taskdata = workList.front();
            workList.pop_front();
        }
        task(taskdata);
    }
    
}

ThreadPool::~ThreadPool()
{
    running = false;
    cond.notifyAll();
    for(pthread_t thread : threads)
    {
        pthread_join(thread, nullptr);
    }
}


