#include "utils.h"

char *memory_index_page = nullptr;

void bufferPrinter(char *buffer, const int bufferSize)
{
    if (buffer[bufferSize - 1] != '\0')
        buffer[bufferSize - 1] = '\0';
    printf("%s\n", buffer);
}
// 端口复用
void setAddrReuse(int socketfd)
{
    int opt = 1;
    int len = sizeof(opt);
    setsockopt(socketfd, SOL_SOCKET, SO_REUSEADDR, &opt, len);
}

void sendHello(int sockedfd)
{
    char buf[128];
    int ret = 0;
    sprintf(buf, "HTTP/1.1 200 OK\r\n");
    ret = send(sockedfd, buf, strlen(buf), MSG_NOSIGNAL);
    // MSG_NOSIGNAL表示在向关闭的链接写数据时，失败不会终端程序
    if (ret == -1)
        throw SocketClosed();

    sprintf(buf, "Content-type: text/html\r\n");
    ret = send(sockedfd, buf, strlen(buf), 0);
    if (ret == -1)
        throw SocketClosed();

    sprintf(buf, "\r\n");
    ret = send(sockedfd, buf, strlen(buf), 0);
    if (ret == -1)
        throw SocketClosed();

    sprintf(buf, "<P>Hello!\r\n");
    ret = send(sockedfd, buf, strlen(buf), 0);
    if (ret == -1)
        throw SocketClosed();

    // sleep(1);
    printf("send hello finish!\n");
}

void dispAddrInfo(struct sockaddr_in &addr)
{
    printf("client addr:%s, port:%d\n", inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));
}

void sprintAddrInfo(struct sockaddr_in &addr, std::string &str)
{
    char cs[128];
    sprintf(cs, "client addr:%s, port:%d\n", inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));
    str += cs;
}
// 获取文件或文件夹信息
bool fileExist(const char path[])
{
    struct stat staticFileState;            // stat结构体存储文件或文件夹的状态信息
    int ret = stat(path, &staticFileState); // stat() 函数获取指定路径的文件或文件夹的状态信息
    if (ret == -1)
    { // 没有对应的文件和文件夹或拒绝存取
        return false;
    }
    else
        return true;
}
// 载入指定页面文件到内存
void loadIndexTomemory(std::string file_path)
{
    struct stat staticFileState;
    int ret = stat(file_path.c_str(), &staticFileState);
    memory_index_page = new char[staticFileState.st_size + 1]; // staticFileState.st_size文件大小
    // memory_index_page大小为file_path的大小+1，开辟的内存肯定能存下file_path的所有内容
    FILE *resource = NULL;
    resource = fopen(file_path.c_str(), "r"); // 以只读方法打开url指定的文件。file_path.c_str()将file_path转为c风格方便放入c的函数
    char readBuf[256];
    memset(readBuf, 0, 256);
    fgets(readBuf, 256, resource); // fgets是从resource读取一行存在readBuf，最多读取256
    while (!feof(resource))
    {
        strcat(memory_index_page, readBuf);
        // char *strcat(char *dest, const char *src);将一个src连接到dest的末尾且以"\0"结尾
        fgets(readBuf, 256, resource);
    }
    fclose(resource);
}

// 设fd为非阻塞
int setFdNonblock(int fd)
{
    int old_options = fcntl(fd, F_GETFL);
    int new_options = old_options | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_options);
    return old_options;
}
// 向epoll中添加需要监听的文件描述符;可读事件配合电平触发
void addFdToEpoll_INLT(int epollfd, int addedfd)
{
    struct epoll_event epollEvent;
    memset(&epollEvent, 0, sizeof(epollEvent));
    epollEvent.data.fd = addedfd;
    epollEvent.events = EPOLLIN;//默认电平触发
    epoll_ctl(epollfd, EPOLL_CTL_ADD, addedfd, &epollEvent);
}
//上树+边缘触发
void addFdToEpoll_INET(int epollfd, int addedfd)
{
    struct epoll_event epollEvent;
    memset(&epollEvent, 0, sizeof(epollEvent));
    epollEvent.data.fd = addedfd;
    epollEvent.events = EPOLLIN | EPOLLET;// 可读事件 + 边缘触发模式
    epoll_ctl(epollfd, EPOLL_CTL_ADD, addedfd, &epollEvent);
}
//上树+边缘+单次触发
void addFdToEpoll_INETONESHOT(int epollfd, int addedfd)
{
    struct epoll_event epollEvent;
    memset(&epollEvent, 0, sizeof(epollEvent));
    epollEvent.data.fd = addedfd;
    epollEvent.events = EPOLLIN | EPOLLET | EPOLLONESHOT;
    epoll_ctl(epollfd, EPOLL_CTL_ADD, addedfd, &epollEvent);
}
//单次触发模式事件在触发后处于不被监视状态，EPOLL_CTL_MOD重新激活它们为被监视事件
void resetOneshot_INETONESHOT(int epollfd, int resetfd)
{
    struct epoll_event epollEvent;
    memset(&epollEvent, 0, sizeof(epollEvent));
    epollEvent.data.fd = resetfd;
    epollEvent.events = EPOLLIN | EPOLLET | EPOLLONESHOT;
    epoll_ctl(epollfd, EPOLL_CTL_ADD, resetfd, &epollEvent);
}

void deleteFdFromEpoll(int epollfd, int targetfd)
{
    epoll_ctl(epollfd, EPOLL_CTL_DEL, targetfd, nullptr);
}
//打印客户端的相关网络信息到屏幕上
void dispPeerConnection(int clientFd)
{
    struct sockaddr_in clientaddr;
    socklen_t clientaddrLength = sizeof(clientaddr);
    getpeername(clientFd, reinterpret_cast< struct sockaddr* >(&clientaddr), &clientaddrLength);
    dispAddrInfo (clientaddr);
}
//提取客户端的相关网络信息到str字符串中
void dispPeerConnection(int clientFd, std::string &str)
{
    struct sockaddr_in clientaddr;
    socklen_t clientaddrLength = sizeof(clientaddr);
    getpeername(clientFd, reinterpret_cast<sockaddr *>(&clientaddr), &clientaddrLength);
    sprintAddrInfo(clientaddr,str);
}
