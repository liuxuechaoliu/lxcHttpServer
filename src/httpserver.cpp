#include <iostream>
#include <stdio.h>
#include <string>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <pthread.h>
#include <list>
#include <vector>
#include <exception>
#include <unistd.h>
#include <signal.h>
#include "ThreadPool.h"
#include "utils.h"
#include "HttpData.h"
#include "Responser.h"
#include "Timer.h"
#include "Error.h"
#include "AsyncLoger.h"

using std::string;
using std::exception;
using std::runtime_error;
using std::cout;
using std::endl;

const int BUFFERSIZE = 1024;
//长链接的超时时间 默认15秒
const int TIMEOUT = 15;

//统一信号源管道
static int pipeline[2];

void signalHandler(int sig)
{
    int saveError = errno;
    int msg = sig;
    write(pipeline[1], reinterpret_cast<char*> (&msg), 1);
    errno = saveError;
}

void addSingal(int sig)
{
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = signalHandler;
    sa.sa_flags |= SA_RESTART;
    sigfillset(&sa.sa_mask);
    sigemptyset(&sa.sa_mask);
    sigaction(sig, &sa, nullptr);
}

void httpserver(TaskData taskData)
{
    int clientSocketFd = (taskData.clientfd);
    std::shared_ptr<AsyncLoger> loger;
    AsyncLoger::getInstance(loger);
    try
    {
        HttpData httpData(clientSocketFd);
        httpData.parseData();
        if(httpData.badRequest())
        {
            return;
        }
        if(httpData.getRequestMethod() == HttpData::UNSUPPORT)
        {
            return;
        }

        Responser responser(httpData);
        if(!fileExist(httpData.getUrl().c_str()) && httpData.getUrlResourceType() != "memory"){responser.sendNotFound(); return;}

        if(httpData.getRequestMethod() == HttpData::RequestMethod::GET)
        {
            if(httpData.getUrlResourceType() == "cgi") responser.executeCGI();
            else if(httpData.getUrlResourceType() == "memory") responser.sendMemoryPage();
            else responser.sendStaticFileToClient();
        } 
        if(httpData.getRequestMethod() == HttpData::RequestMethod::POST)
        {
            if(httpData.getUrlResourceType() == "cgi") responser.executeCGI();
            else responser.sendNotFound();
        }

        if(httpData.keepConnection())
        {
            taskData.timer->addfd(clientSocketFd);
        }
        else
        {
            close(clientSocketFd);
        }
    }
    catch(const std::exception& e)
    {
        taskData.timer->deleteFd(clientSocketFd);
        close(clientSocketFd);
    }
}


int main()
{
    string ip = "127.0.0.1";
    int port = 12345;
    std::shared_ptr<AsyncLoger> loger;
    AsyncLoger::getInstance(loger);

    int serverSocketfd = socket(AF_INET, SOCK_STREAM, 0);
    if(serverSocketfd = -1) throw runtime_error("socket create failed\n");
    printf("socket create success\n");
    loger->logInfoTempVar("socket create success\n");

    setAddrReuse(serverSocketfd);//使用setsockopt()
    setFdNonblock(serverSocketfd);//使用fcntl()

    struct sockaddr_in ipaddr;
    ipaddr.sin_family = AF_INET;
    ipaddr.sin_port = htons(port);
    ipaddr.sin_addr.s_addr = inet_addr(ip.c_str());

    int ret = bind(serverSocketfd, reinterpret_cast<struct sockaddr*>(&ipaddr), sizeof(ipaddr));
    if(ret == -1) throw runtime_error("bind failed\n");
    printf("bind ipaddr create success\n");
    loger->logInfoTempVar("bind ipaddr create success\n");

    ret = listen(serverSocketfd, 20);
    if(ret == -1) throw runtime_error("call listen failsed\n");
    printf("listen create success\n");
    loger->logInfoTempVar("listen create success\n");

    ret = pipe(pipeline);
    if(ret == -1)throw runtime_error("pipeline create failed\n");
    printf("pipeline create success\n");
    loger->logInfoTempVar("pipeline create success\n");

    struct sockaddr_in clientaddr;
    socklen_t clientaddrLength = sizeof(clientaddr);

    int epollFd = epoll_create(5);
    addFdToEpoll_INET(epollFd, serverSocketfd);
    addFdToEpoll_INLT(epollFd, pipeline[0]);
    struct epoll_event epollEvents[5];
    ThreadPool threadPool(httpserver, 4);
    printf("ThreadPool create success\n");
    loger->logInfoTempVar("ThreadPool create success\n");

    loadIndexTomemory("www/index.html");
    printf("load index page to memory success\n");
    loger->logInfoTempVar("load index page to memory success\n");

    addSingal(SIGALRM);
    setFdNonblock(pipeline[1]);
    alarm(1);
    Timer timer(epollFd, TIMEOUT);
    char signals[128];

    while (true)
    {
        ret = epoll_wait(epollFd, epollEvents, 5, -1);
        for(int i = 0; i < ret; i++)
        {
            if(epollEvents[i].data.fd == serverSocketfd)
            {
                int clientSocketFd = -1;
                while ((clientSocketFd = accept(serverSocketfd, reinterpret_cast<struct sockaddr*>(&clientaddr), &clientaddrLength) ) != -1);
                {
                    threadPool.appendFd(TaskData(clientSocketFd, &timer));//可看到timer更像一种epoll元素的公共属性包括epollfd和TIMEOUT，而不包括每次处理得套接字本身
                }
            }
            if(epollEvents[i].data.fd != pipeline[0])
            {
                timer.forbidenFd(epollEvents[i].data.fd);
                threadPool.appendFd(TaskData(epollEvents[i].data.fd, &timer));

            }
            else
            {
                int sigNums = read(pipeline[0], signals, 128);
                for(int i = 0; i < sigNums; i++)
                {
                    if(signals[i] == SIGALRM)
                    {
                        timer.deleteExpiredFd();
                        alarm(1);
                    }
                    else continue;
                }
            }
        }
    }

    return 0;
}