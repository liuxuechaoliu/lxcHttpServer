#include "AsyncLoger.h"

/*只在类的头文件中声明了静态成员，而没有在源文件中进行定义，
那么编译器在链接时会找不到这些静态成员的定义，导致链接错误。
所以通常在类的头文件中声明静态成员，在源文件中定义它们，
以确保链接过程能够正确完成。*/
std::shared_ptr<AsyncLoger> AsyncLoger::instance;
MutexLock AsyncLoger::singletonMutex;

Buffer::Buffer(size_t)
{
    dataPtr = new char[bufferSize];
}

Buffer::~Buffer()
{
    delete[] dataPtr;
}

void Buffer::append(const char* logLine, size_t len)
{
    memcpy(dataPtr+end, logLine, len);
    end += len;
}

bool Buffer::appendAvailable(size_t strLen)
{
    if(end + strLen + DATE_STRING_LEGTH >= bufferSize) return false;
    else return true;
}

bool Buffer::empty() const
{
    return end == 0;
}

int Buffer::writeToFile(int fd)
{
    int ret = write(fd, dataPtr, end);
    if(ret == -1) return -1;
    else
    {
        end = 0;
        return 1;
    }
}

AsyncLoger::AsyncLoger(std::string logName = "LogFile")
:logFileFd(-1), conditionVar(3), running(true)
{
    // 构造文件名，日期取的是日志系统进行构造时的时间
    char tmpBuf[BUFLEN];
    sprintCurTime(tmpBuf, DATE_STRING_LEGTH);
    std::string fileName = logName + tmpBuf + ".log";
    // 就算打开失败也不要要在构造函数中抛出异常。O_CREAT表示不存在，则创建
    logFileFd = open(fileName.c_str(), O_WRONLY|O_CREAT, 777);
    //初始化空缓冲区
    for(int i =0; i < 4; i++)
    {
        emptyBuffers.push_back(std::unique_ptr<Buffer>(new Buffer(LOGBUFFERSIZE)));
    }
    //指定起始缓冲区，把emptyBuffers最后一个元素取(删除)出来给curBuffer
    curBuffer = std::move(emptyBuffers.back());
    emptyBuffers.pop_back();
    //启动后端线程执行threadFun()
    pthread_create(&logBackGroundthreadId, nullptr, threadWrap, this);
}

AsyncLoger::~AsyncLoger()
{
    running = false;
    conditionVar.notifyAll();
    pthread_join(logBackGroundthreadId,nullptr);
    if(logFileFd != -1) close(logFileFd);
}

void AsyncLoger::Deleter(AsyncLoger * ptr)
{
    delete ptr;
}

void AsyncLoger::threadFun()
{
    std::vector<std::unique_ptr<Buffer>> writeQueue;
    writeQueue.reserve(4);
    while (running)
    {
        {
            MutexLockGuard mutexLockGuard(mutex);
            conditionVar.waitForTime(mutex);
            //尝试写入缓冲区
            if(curBuffer)
            {
                writeQueue.push_back(std::move(curBuffer));
                if(!emptyBuffers.empty())
                {
                    curBuffer = std::move(emptyBuffers.back());
                    emptyBuffers.pop_back();
                }
            }
            
            //
            while(!toWriteBuffers.empty())
            {
                writeQueue.push_back(std::move(toWriteBuffers.back()));
                toWriteBuffers.pop();
            }
        }

        while (!writeQueue.empty())
        {
            writeQueue.back()->writeToFile(logFileFd);
            {
                MutexLockGuard mutexLockGuard(mutex);
                emptyBuffers.push_back(std::move(writeQueue.back()));
            }
            writeQueue.pop_back();
        }
    }
}

void *AsyncLoger::threadWrap(void * args)
{
    AsyncLoger* asyncLoger = reinterpret_cast<AsyncLoger*>(args);
    asyncLoger->threadFun();
}

void AsyncLoger::getInstance(std::shared_ptr<AsyncLoger> &sPtr)
{
    {
        if(!instance)
        {
            MutexLockGuard mutexLockGuard(singletonMutex);
            instance = std::shared_ptr<AsyncLoger>(new AsyncLoger("RunTimeLog"), AsyncLoger::Deleter);
        }
        sPtr = instance;
    }

}

int AsyncLoger::logInfo(std::string &logline)
{
    size_t logSize = logline.size();
    {
        MutexLockGuard mutexLockGuard(mutex);

        if(curBuffer == nullptr) return -1;

        if(logSize > LOGBUFFERSIZE/2)
        {
            memcpy(&logline[LOGBUFFERSIZE/2 - 17], "Log Info Too Long", 17);
            logSize = LOGBUFFERSIZE/2;
        }

        if(curBuffer->appendAvailable(logSize + DATE_STRING_LEGTH))
        {
            //添加时间
            char dataStr[DATE_STRING_LEGTH];
            sprintCurTime(dataStr, DATE_STRING_LEGTH);
            dataStr[DATE_STRING_LEGTH -1] = ' ';
            curBuffer->append(dataStr, DATE_STRING_LEGTH);
            //添加信息
            curBuffer->append(logline.c_str(), logSize);
        }

        else
        {
            toWriteBuffers.push(std::move(curBuffer));
            conditionVar.notifyAll();
            if(!emptyBuffers.empty())
            {
                curBuffer = std::move(emptyBuffers.back());
                emptyBuffers.pop_back();
                char dataStr[DATE_STRING_LEGTH];
                sprintCurTime(dataStr, DATE_STRING_LEGTH);
                dataStr[DATE_STRING_LEGTH -1] = ' ';
                curBuffer->append(dataStr, DATE_STRING_LEGTH);
                //添加信息
                curBuffer->append(logline.c_str(), logSize);
            }
        }
    }
    
}

int AsyncLoger::logInfoTempVar(std::string logline)
{
    logInfo(logline);
}

void sprintCurTime(char* str, int len)
{
    time_t time(0);
    strftime(str, len, "%Y%m%d%H%M%S", localtime(&time));
}
