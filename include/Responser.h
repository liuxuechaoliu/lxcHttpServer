#ifndef __RESPONSER__
#define __RESPONSER__

#include <string>
#include <exception>
#include <unistd.h>
#include <sys/stat.h>
#include "sys/wait.h"
#include "HttpData.h"
#include "utils.h"

extern const int PAGE_BUFFER_SIZE;//2048

class Responser
{
private:
    void firstLine_200(char[]);
    void header_keepAlive(char[]);
    void header_htmlContentType(char[]);
    void header_pngContentType(char[]);
    void header_contentLength(char[], size_t);
    void header_body(char[]);
    void serverStaticFile();
    int cgiContentLength();

    const HttpData &httpData;
    int clientSocket;
    bool isClose;
public:
    Responser(const HttpData &_httpData)
    :httpData(_httpData), clientSocket(httpData.getClientSocket()), isClose(false){}
    ~Responser();
    void sendStaticFileToClient();
    void executeCGI();
    void sendForBidden();
    void sendNotFound();
    void sendMemoryPage();
    void closePeerConnection();
};

#endif // !__RESPONSER__