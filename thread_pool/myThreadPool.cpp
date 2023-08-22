#include "myThreadPool.h"
#include <string.h>//c语言库，memset，memcpy，
#include <string>
#include <unistd.h>
#include <iostream>
using namespace std;

ThreadPool::ThreadPool(int min, int max)
{
    
    do
    {
        taskQ = new TaskQueue;//实例化任务队列
        if(taskQ == nullptr)
        {
            cout<<"new taskQ fail"<<endl;
            break;
        }
        threadIDs = new pthread_t[max];
        if(threadIDs == nullptr)
        {
            cout<<"new threadIDs fail"<<endl;
            break;
        }
        memset(threadIDs, 0, sizeof(pthread_t)*max);

        minNum = min;
        maxNum = max;
        busyNum = 0;
        liveNum = min;
        exitNum = 0;

        //初始化锁和条件
        if(pthread_mutex_init(&mutexPool, NULL) != 0 ||
           pthread_cond_init(&notEmpty, NULL) != 0)
        {
            cout<<"mutex or condition init fail"<<endl;
            break;
        }

        shutdown = false;

        //创建管理者线程
        pthread_create(&managerID, NULL, manager, this);
        //创建工作线程
        for (int i = 0; i < min; i++)
        {
            pthread_create(&threadIDs[i], NULL, worker, this);
        }
        return;//此处返回空，while(0)之后的内容是创建失败要执行的内容

    } while (0);

    //以下是如果创建失败要执行的内容
    if(threadIDs) delete[] threadIDs;
    if(taskQ) delete taskQ;
}

ThreadPool::~ThreadPool()
{
    //关闭线程池
    shutdown = true;

    //阻塞回收管理者线程
    pthread_join(managerID, NULL);

    //唤醒阻塞的消费者线程，并回收
    for (int i = 0; i < liveNum; i++)
    {
        pthread_cond_signal(&notEmpty);
        /* code */
    }
    if(taskQ)
    {
        delete taskQ;
    }
    if(threadIDs)
    {
        delete[] threadIDs;
    }
    pthread_mutex_destroy(&mutexPool);
    pthread_cond_destroy(&notEmpty);
    

}

void ThreadPool::addTask(Task task)
{
    if (shutdown)//若线程池被关闭，则直接返回
    {
        return;
    }

    //添加任务
    taskQ->addTask(task);
    pthread_cond_signal(&notEmpty);
}

int ThreadPool::getBusyNum()
{
    pthread_mutex_lock(&mutexPool);
    int busyNum = this->busyNum;
    pthread_mutex_unlock(&mutexPool);
    return busyNum;
}

int ThreadPool::getAliveNum()
{
    pthread_mutex_lock(&mutexPool);
    int liveNum = this->liveNum;
    pthread_mutex_unlock(&mutexPool);
    return liveNum;
}

void *ThreadPool::worker(void *arg)
{
    /*因为worker是静态函数，
    访问ThreadPool属性要引入ThreadPool实例对象指针this(arg)*/
    ThreadPool* pool = static_cast<ThreadPool*>(arg);
    while (true)//一直检测干活
    {
        pthread_mutex_lock(&pool->mutexPool);
        //当前队列是否为空
        while (pool->taskQ->taskNumber() == 0 && !pool->shutdown)
        {
            //条件阻塞工作线程，因为任务队列为空所以，同时解锁
            pthread_cond_wait(&pool->notEmpty, &pool->mutexPool);
            //判断是否销毁线程
            if(pool->exitNum > 0)
            {
                pool->exitNum--;
                if(pool->liveNum > pool->minNum)
                {
                    pool->liveNum--;
                    pthread_mutex_unlock(&pool->mutexPool);
                    pool->threadExit();
                }
            }
        }

        //判断线程池是否关闭
        if(pool->shutdown)
        {
            pthread_mutex_unlock(&pool->mutexPool);
            pool->threadExit();
        }

        //从任务队列中去取一个任务
        Task task = pool->taskQ->takeTast();

        pool->busyNum++;
        pthread_mutex_unlock(&pool->mutexPool);

        cout<<"thread"<<to_string(pthread_self())<<" start work"<<endl;
        task.function(task.arg);//执行任务
        int* p = (int*)task.arg;
        delete p;
        task.arg = nullptr;
        cout<<"thread "<<to_string(pthread_self())<<" end work"<<endl;
        
        pthread_mutex_lock(&pool->mutexPool);
        pool->busyNum--;
        pthread_mutex_unlock(&pool->mutexPool);
    }
    return nullptr;
}

void *ThreadPool::manager(void *arg)
{
    ThreadPool* pool = static_cast<ThreadPool*>(arg);
    while (pool->shutdown)
    {
        //每隔3s检测一次
        sleep(3);

        pthread_mutex_lock(&pool->mutexPool);
        int queueSize = pool->taskQ->taskNumber();
        int busyNum = pool->busyNum;
        int liveNum = pool->liveNum;
        pthread_mutex_unlock(&pool->mutexPool);

        //添加线程
        if(queueSize > liveNum && liveNum < pool->maxNum)
        {
            pthread_mutex_lock(&pool->mutexPool);
            int counter = 0;
            for (int i = 0; i < pool->maxNum && counter < NUMBER && 
                 pool->liveNum < pool->maxNum; i++)
            {
                if(pool->threadIDs[i] == 0)
                {
                    pthread_create(&pool->threadIDs[i], NULL, worker, pool);
                    counter++;
                    pool->liveNum++;
                }
            }
            pthread_mutex_unlock(&pool->mutexPool);
        }

        //销毁线程
        //忙的线程*2 < 存活的线程数 && 存活的线程数>最小线程数
        if (pool->busyNum*2<pool->liveNum && pool->liveNum>pool->minNum)
        {
            pthread_mutex_lock(&pool->mutexPool);
            pool->exitNum = pool->NUMBER;//退出数为2
            pthread_mutex_unlock(&pool->mutexPool);
            for (int i = 0; i < pool->NUMBER; i++)
            {
                pthread_cond_signal(&pool->notEmpty);
                //唤醒阻塞的工作线程，因为pool->exitNum不为0
                //工作线程会判定exitNum后自杀
            }
        }
    }
    return nullptr;
}

void ThreadPool::threadExit()//线程退出，threadIDs[i]置0
{
    pthread_t tid = pthread_self();
    for(int i = 0; i < maxNum; i++)
    {
        if(threadIDs[i] == tid)
        {
            threadIDs[i] = 0;
            cout<<"threadExit() called "<< to_string(tid)<< exit<< endl;
            break;
        }
    }
    pthread_exit(NULL);
}
