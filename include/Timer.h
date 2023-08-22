#ifndef __TIMER__
#define __TIMER__
#include <sys/time.h>
#include <unordered_map>
#include <list>
#include <memory>
#include "utils.h"
#include "AsyncLoger.h"
#include "Mutex.h"
#include <sys/epoll.h>

using TimeVal = struct timeval;//timeval结构体有tv_sec秒+tv_usec毫秒

class Timer
{
private:

    class Node
    {
    private:
        int m_clientfd;//要处理的fd,上树、下树、免清理、激活等操作
        TimeVal m_curTime;//加入时间
        TimeVal m_expireTime;//过期时间 = 加入时间 + _timeout
    public:
        Node(int _clientfd, int _timeout);//_timeout表示长链接限时，默认10秒

        const TimeVal& curTime()const {return m_curTime;}
        const TimeVal& expireTime()const {return m_expireTime;}
        const int clientFd() const {return m_clientfd;}

        void setExpireTime(int _timeout);
        void resetExpireTime(int _timeout);
    };

    // 保存节点的链表，快失效的在链表头，新的在链表尾部
    //保存了Node元素，其中含有fd,fd加入epoll时间和规定的过期时间
    std::list<Node> timeList;
    //保存客户端套接字文件描述符到时间节点的映射关系
    //hashMap的元素组成为：key:clientFd。value:Node(clientFd, timeout)在list<Node>中的索引iterator
    std::unordered_map<int, std::list<Node>::iterator> hashMap;
    //添加数据时的写锁
    MutexLock mutex;
    //节点超出时间
    int timeout;
    //当前在时间链表timeList中存在的元素数
    int curSize;
    //timeList最大元素数量
    int maxSize;
    //epoll树
    int epollFd;
    //日志记录
    std::shared_ptr<AsyncLoger> loger;
    
public:
    Timer(int _epollfd, int _timeout, int _maxSize = 1000)
    :epollFd(_epollfd), timeout(_timeout), maxSize(_maxSize), curSize(0)
    { AsyncLoger::getInstance(loger);}
    // 添加需要长链接的客户端，添加成功返回true，失败返回false
    bool addfd(int clientFd);
    //检查处理过期的长链接
    void deleteExpiredFd();
    //删除指定的长链接
    void deleteFd(int clientFd);
    //屏蔽某个长链接不受时间限制检查(从timeList中拿出不受检测即可)
    void forbidenFd(int clientFd);
};

#endif 
