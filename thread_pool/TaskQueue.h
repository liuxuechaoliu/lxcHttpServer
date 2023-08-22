#ifndef _TASKQUEUE_H_
#define _TASKQUEUE_H_

#include <queue>
using namespace std;
#include <pthread.h>

using callback = void (*) (void* arg);//callback fun; fun为返回值为void*,参数为void* arg的函数 

struct Task
{
    Task()//构造函数
    {
        function = nullptr;
        arg = nullptr;
    }
    Task(callback f, void* arg)//构造函数
    {
        function = f;
        this->arg = arg;
    }
    callback function;
    void * arg;
};


class TaskQueue
{
public:
    TaskQueue();
    ~TaskQueue();

    //添加任务
    void addTask(Task task);
    void addTask(callback f, void* arg);
    //取一个任务
    Task takeTast();
    //获取当前任务个数
    inline int taskNumber()
    {
        return m_taskQ.size();
    }

private:
    pthread_mutex_t m_mutex;
    queue<Task> m_taskQ;
};

#endif //  _TASKQUEUE_H_