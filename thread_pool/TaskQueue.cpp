#include "TaskQueue.h"

TaskQueue::TaskQueue()//构造函数
{
    pthread_mutex_init(&m_mutex, NULL);
}

TaskQueue::~TaskQueue()//析构函数
{
    pthread_mutex_destroy(&m_mutex);
}

void TaskQueue::addTask(Task task)//添加任务
{
    pthread_mutex_lock(&m_mutex);
    m_taskQ.push(task);

    pthread_mutex_unlock(&m_mutex);
}

void TaskQueue::addTask(callback f, void *arg)//添加任务重载
{
    pthread_mutex_lock(&m_mutex);
    m_taskQ.push(Task(f,arg));

    pthread_mutex_unlock(&m_mutex);
}

Task TaskQueue::takeTast()//取一个任务
{
    Task t;
    pthread_mutex_lock(&m_mutex);
    if(!m_taskQ.empty())
    {
        t = m_taskQ.front();//去除队头任务
        m_taskQ.pop(); //删除队头任务
    }
    pthread_mutex_unlock(&m_mutex);
    return t;
}


