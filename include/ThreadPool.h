#ifndef __ThreadPool__
#define __ThreadPool__

#include <vector>
#include <list>
#include <atomic>
#include "Timer.h"
#include "Mutex.h"

class TaskData
{
public:
    TaskData(int _clientfd, Timer *_timer)
        : clientfd(_clientfd), timer(_timer) {}

    int clientfd;
    Timer *timer;
};

class ThreadPool
{
public:
    ThreadPool(void (*task)(TaskData),
               int threadNum = 4, int maxWorkListLen = 8);
    ~ThreadPool();

    // 返回线程池拥有的互斥锁
    MutexLock &getMutexLock()
    {
        return mutex;
    }
    //向工作队列中添加客户连接，并发唤醒信号
    void appendFd(TaskData taskData)
    {
        {
            MutexLockGuard mutexLockGuard(mutex);
            workList.push_back(taskData);
        }
        notifyOneThread();
        
    }

    void notifyOneThread()
    {
        cond.notify();
    }

private:
    // 标志线程池正在运行，设定为false之后池中的所有线程就退出事件循环
    std::atomic<bool> running;
    int threadNum;
    int maxWorkListLen;
    std::list<TaskData> workList;// 用于接受来自主线程已经接受的socketfd
    std::vector<pthread_t> threads;//线程队列
    MutexLock mutex;
    Condition cond;
    void (*task) (TaskData);//定义一个函数叫task，接受TaskData类对象参数

    static void *workerWarper(void* arg);//套一个静态成员函数的马甲包裹真正的工作函数worker()
    void worker();//线程工作函数，非静态成员函数不能作为pthread_create的工作函数
};

#endif