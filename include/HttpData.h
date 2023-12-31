#ifndef __HTTPDATA__
#define __HTTPDATA__

#include <sys/socket.h>
#include <string.h>
#include <exception>
#include <stdio.h>
#include <string>
#include <vector>
#include <utility>
#include <memory>
#include <unordered_map>
#include "Error.h"

using std::string; using std::vector;
using std::shared_ptr;

extern const string NULLINFO;

// 专门用于处理和客户机http数据的交互
const int HTTPDATA_BUFFERSIZE = 1024;//存储 HTTP 请求或响应数据的缓冲区的大小 // FIXME 这个值可以调整得更大一些，调试时使用的是较小值64
const int LINE_BUFFERSIZE = 256;//这个常量定义了用于存储 HTTP 报文中一行文本的缓冲区的大小。

// 存储资源的根目录路径
const char RESOURCE_ROOT[] = "www";

class HttpData{
public:
    HttpData(int clientSocket);
    enum RequestMethod{ UNSUPPORT, GET, POST };
    // 解析从客户机发来的所有信息
    void parseData();
    const string& getUrl() const { return url; }
    const string& getRequestMethod_string() const { return requestMethod_string; }
    const int getRequestMethod() const { return requestMethod; }
    const string& getVersion() const { return version; }
    const string& getUserAgent() const { return userAgent ? *userAgent : NULLINFO; }
    const string& getUrlResourceType() const { return urlResourceType; }
    int getContentLength() const { return contentLength ? numeralContentLength() : 0; }
    int getParamCount() const { return clientParamData ? (*clientParamData).size() : 0; }
    int getClientSocket() const { return clientSocket; }
    const shared_ptr< std::unordered_map< string, string > > getClientParamData() const { return clientParamData; }
    bool keepConnection() const { return m_keepConnection; }
    bool badRequest() const { return m_badRequest; }

private:
    // 根据CRLF标志符，将套接字中的数据分行取出。读取成功的最后\r\n会被替换成\0
    string parseOneLine(); // 一次性从recv中读出一大段数据后续在来处理代替利用recv每次只从缓冲区取一个字符出来。
                           //理论上系统调用会有不少开销，以此减少对recv的调用次数

    // get requestmethod, url and http verison
    void parseStartLine();
    // 解析完请求行之后，解析剩余的请求头部信息,所有行保存在不同变量中
    void parseHeader();
    // 读取body中的POST数据
    void parseBodyParamData();
    // 读取url中的GET的参数，并把键值对存在(*clientParamData)哈希表中
    void parseUrlDataParamList();
    // 解析字符串中的 key1=value1&key=value2& 参数，将键值对放入哈希表中。目前需要手动在输入字符的最后添加一个&
    void parseStringParamTo_htable_clientParamData(string & );
    // 将content-length字符串转换为数字
    int numeralContentLength() const;
    // 从套接字clientSocket中取出一段数据(最大1024)放入dataBuffer，设置readIndex(0)和dataEndIndex(size-1)
    void readRawDataFromSocket();
    //lxc如果读取位大于未被读取数据位或者缓冲区大小
    inline bool dataBufferEmpty(){ return readIndex > dataEndIndex || readIndex >= HTTPDATA_BUFFERSIZE; }
    // 获得一行中请求头的名称(key)
    string getHeaderLineName( string &s );

    // 根据指定字符分割字符串
    void stringSplit(const string& s, vector<string>& tokens, const string& delimiters = " ");
    //通过请求头的第一个字符串是GET还是POST还是其它判断RequestMethod类型
    void getRequestMethod( const vector<string>& headerInfo );

    int clientSocket;
    char dataBuffer[ HTTPDATA_BUFFERSIZE ];//读取缓冲区
    int readIndex, dataEndIndex; // readIndex指向未被读取的数据的第一个字符，dataEndIndex指向数据的最后位置

    char prev;//在循环中记录上一个字符

    // data
    RequestMethod requestMethod;
    string requestMethod_string, url, urlResourceType, version;
    shared_ptr<string> host, userAgent, accept, acceptLanguage, 
                       acceptEncoding, connection, upgradeInsecurceRequests, 
                       contentType, contentLength; // 弄成堆上对象，因为有可能为空。用智能指针来管理
    shared_ptr< std::unordered_map< string, string > > clientParamData;  // 利用哈希表保存post/get中的键值对
    bool m_keepConnection, m_badRequest;
};

#endif

/*GET请求头：
GET url?key1=value1&key=value2 HTTP/1.1
*/

/*POST请求头：
POST url HTTP/1.1
*/
