#include "Timer.h"

//Timer::Node类
Timer::Node::Node(int _clientfd, int _timeout)
    : m_clientfd(_clientfd)
{
    setExpireTime(_timeout);
}

void Timer::Node::setExpireTime(int _timeout)
{
    gettimeofday(&m_curTime, nullptr);//函数获取当前时间存入m_curTime
    m_expireTime = m_curTime;
    m_expireTime.tv_sec += _timeout; 
}

void Timer::Node::resetExpireTime(int _timeout)
{
    setExpireTime(_timeout);
}


//Timer类
bool Timer::addfd(int clientFd)
    {
        MutexLockGuard mutexLockGuard(mutex);
        if(curSize >= maxSize) return false;//FIXME有逻辑问题
        if(hashMap.find(clientFd) == hashMap.end())//说明hashMap中没有clientFd,这是新fd
        {
            //自己写的
            // Node node(clientFd, timeout);
            // hashMap[clientFd] = timeList.end();
            // timeList.push_back(node);
            // struct epoll_event ev;
            // ev.data.fd = clientFd;
            // ev.events = EPOLLIN | EPOLLET;
            // int op = 0;
            // epoll_ctl(epollFd, op, clientFd, &ev);

            hashMap[clientFd] = timeList.insert(timeList.end(), Node(clientFd, timeout));
            addFdToEpoll_INETONESHOT(epollFd, clientFd);
            curSize++;
            #ifdef __PRINT_INFO_TO_DISP__
            printf("\nTimer - add client to keep alive:");
            dispPeerConnection( clientFd );
            #endif
            #ifdef __LOG_INFO__
            std::string logline("Timer - add client to keep alive: ");
            dispPeerConnection( clientFd, logline );
            loger->logInfo(logline);
            #endif
        }
        else
        {
            hashMap[ clientFd ] = timeList.insert( timeList.end(), Node( clientFd, timeout ) );
            resetOneshot_INETONESHOT(epollFd, clientFd);
            #ifdef __PRINT_INFO_TO_DISP__
            printf("\nTimer - flash client keep alive:");
            dispPeerConnection( clientFd );
            #endif
            #ifdef __LOG_INFO__
            std::string logline("Timer - flash client keep alive: ");
            dispPeerConnection( clientFd, logline );
            loger->logInfo(logline);
            #endif
        }
        return true;
    }

void Timer::deleteExpiredFd()
{
    TimeVal curtime;
    gettimeofday(&curtime, nullptr);
    MutexLockGuard mutexLockGuard(mutex);

    //因为旧链接在链表头，所以检查timeList.front()的Node
    while (curSize > 0 && curtime.tv_sec > timeList.front().expireTime().tv_sec)//超时条件
    {
        hashMap.erase( timeList.front().clientFd() );
        deleteFdFromEpoll(epollFd, timeList.front().clientFd());
        #ifdef __PRINT_INFO_TO_DISP__
        printf("\nTimer - delete expire keep alive:");
        dispPeerConnection( timeList.front().clientFd() );
        #endif
        #ifdef __LOG_INFO__
        std::string logline("Timer - delete expire keep alive: ");
        dispPeerConnection( timeList.front().clientFd(), logline );
        loger->logInfo(logline);
        #endif

        close(timeList.front().clientFd());
        timeList.pop_front();
        curSize--;

    }
}

void Timer::deleteFd(int clientFd)
{
    MutexLockGuard mutexLockGuard(mutex);
    if (hashMap.find(clientFd) != hashMap.end())
    {
        timeList.erase(hashMap[clientFd]);
        deleteFdFromEpoll(epollFd, clientFd);
        hashMap.erase(clientFd);
        curSize--;
        #ifdef __PRINT_INFO_TO_DISP__
        printf("\nTimer - delete client keep alive:");
        dispPeerConnection( clientFd );
        #endif
        #ifdef __LOG_INFO__
        std::string logline("Timer - delete client keep alive: ");
        dispPeerConnection( clientFd, logline );
        loger->logInfo(logline);
        #endif
    }
}

void Timer::forbidenFd(int clientFd)
{
    MutexLockGuard mutexLockGuard(mutex);
    if(hashMap.find(clientFd) != hashMap.end())
    {
        timeList.erase(hashMap[clientFd]);// 将该FD从时间链表中移除
        hashMap[clientFd] = timeList.end();// 但保留在哈希表中的查找信息
        #ifdef __PRINT_INFO_TO_DISP__
        printf("\nTimer - forbiden client, it's in timer but not be expire:");
        dispPeerConnection( clientFd );
        #endif
        #ifdef __LOG_INFO__
        std::string logline("Timer - forbiden client, it's in timer but not be expire: ");
        dispPeerConnection( clientFd, logline );
        loger->logInfo(logline);
        #endif
    }
    

}
