#ifndef __ASYNCLOGER__
#define __ASYNCLOGER__

// #define _ASYNCLOGER_TEST_

#include <fcntl.h>
#include <string>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <algorithm>
#include <vector>
#include <queue>
#include <atomic>
#include <memory>
#include "Mutex.h"

#define BUFLEN 256//用于存放文件名
//每个缓冲区currBffer大小为1M
static const int LOGBUFFERSIZE = 1 * 1024 * 1024;

//有效信息固定14个char
static const int DATE_STRING_LEGTH = 15;

//把如 "20220101123059"(表示2022年1月1日12点30分59秒),并存储在char[]
void sprintCurTime(char * , int);

class Buffer
{
private:
    size_t bufferSize, end;
    char* dataPtr;
public:
    //创造dataPtr指针指向char[bufferSize]，end为追加信息的位置
    Buffer(size_t);
    //deletedataPtr
    ~Buffer();
    // 向buffer中的end位置添加字符串信息
    void append(const char*, size_t len);
    // 测试。判断Buffer中是否还有位置存放strlen个元素
    bool appendAvailable(size_t strLen);
    //判断是否有内容
    bool empty() const;
    //buffer中内容写入fd(日志文件)重置end
    int writeToFile(int fd);
};

class AsyncLoger
{
private:
    AsyncLoger(std::string);
    ~AsyncLoger();
    //单例模式
    static std::shared_ptr<AsyncLoger> instance;
    static MutexLock singletonMutex;//静态锁专门用来锁getInstance()
    AsyncLoger& operator=(const AsyncLoger&) = delete;
    AsyncLoger(const AsyncLoger&) = delete;
    //智能指针定义时的自定义的删除器,指定当std::shared_ptr管理的对象不再被引用时 如何释放它
    static void Deleter(AsyncLoger*);
    //后端写入线程
    void threadFun();
    //threadFun马甲
    static void* threadWrap(void*);
    int logFileFd;
    pthread_t logBackGroundthreadId;
    MutexLock mutex;
    ConditionTimeWait conditionVar;
    //双缓冲区
    std::unique_ptr<Buffer> curBuffer;
    std::vector<std::unique_ptr<Buffer>> emptyBuffers;
    std::queue<std::unique_ptr<Buffer>> toWriteBuffers;
    std::atomic<bool> running;


public:
    //单例模式
    static void getInstance(std::shared_ptr<AsyncLoger> & sPtr);
    // 用于前端线程提交日志信息
    int logInfo(std::string&);
    int logInfoTempVar(std::string);
};


#endif