#include "HttpData.h"

const string NULLINFO("NULL");

HttpData::HttpData(int clientSocket):prev(0), 
                                     clientSocket(clientSocket),
                                     readIndex(HTTPDATA_BUFFERSIZE),
                                     dataEndIndex(HTTPDATA_BUFFERSIZE - 1),
                                     m_badRequest(false),
                                     requestMethod(UNSUPPORT){
    memset(dataBuffer, 0, HTTPDATA_BUFFERSIZE);
}

void HttpData::parseData()
{
    parseStartLine();
    if(m_badRequest || requestMethod == UNSUPPORT) return;
    parseHeader();
    if(requestMethod == POST) parseBodyParamData();
}

string HttpData::parseOneLine()
{
    if(dataBufferEmpty()) readRawDataFromSocket();
    char lineBuffer[LINE_BUFFERSIZE];
    int index = 0;
    while (readIndex <= dataEndIndex)
    {
        lineBuffer[index] = dataBuffer[readIndex];
        index++;
        readIndex++;
        if(prev == '\r' && dataBuffer[readIndex-1] == '\n'){prev = 0; break;}
        prev = dataBuffer[readIndex-1];

        if(readIndex > dataEndIndex) readRawDataFromSocket();
    }
    lineBuffer[index - 2]  = '\0';
    return string(lineBuffer);
}

// 判断请求类别(GET/POST)/ 读取url中参数存入哈希/ 设置url为www+url/ 获取urlResourceType(如com)/ 读取HTTP版本
void HttpData::parseStartLine()
{
    string firstLine(std::move(parseOneLine()));
    vector<string> headerInfo;
    stringSplit(firstLine, headerInfo, " ");
    getRequestMethod(headerInfo);
    /* 如果请求无法识别就不进行后续的url处理，即一旦被标记为badRequest或者UNSUPPORT，
    那么httpData中的大部分数据都将不会生成。因此在要继续访问httpData前需要进行一次检查*/
    if(m_badRequest || requestMethod == UNSUPPORT) return;

    url = headerInfo[1];
    if(url == "/") url = "/index.html";
    parseUrlDataParamList();

    // 计算url的后缀类型(比如com)
    auto suffixStartIndex = url.find_last_of('.');
    if(suffixStartIndex == string::npos) urlResourceType = NULLINFO;
    else urlResourceType = url.substr(suffixStartIndex + 1);

    url = RESOURCE_ROOT + url;

    version = headerInfo[2];
    
}

void HttpData::parseHeader()
{
    string line;
    string headName;
    while (true)
    {
        line = parseOneLine();
        if(line == "") break;
        headName = getHeaderLineName(line);
        if(headName == "HOST") host = std::make_shared<string>(std::move(line));
        else if(headName == "User-Agent") userAgent = std::make_shared<string>(std::move(line));
        else if(headName == "Accept") accept = std::make_shared<string>(std::move(line));
        else if(headName == "Accept-Language") acceptLanguage = std::make_shared<string>(std::move(line));
        else if(headName == "Connection")
        {
            connection = std::make_shared<string>(std::move(line));
            if((*connection).size()>= 13 && (*connection).substr(12) == "keep-alive" ) m_keepConnection = true;
        } 
        else if (headName == "Upgrade-Insecure-Requests") upgradeInsecurceRequests = std::make_shared<string>(std::move(line));
        else if(headName == "Content_Type") contentType = std::make_shared<string>(std::move(line));
        else if(headName == "Content-Length") contentLength = std::make_shared<string>(std::move(line));
    }   
    

}

string HttpData::getHeaderLineName(string &s)
{
    int i = 0;
    while (s[i] != '=') i++;
    return s.substr(0, i);
}


void HttpData::parseBodyParamData()
{
    int contentLength = numeralContentLength();
    char* data = new char[contentLength + 2];

    int index = 0;
    while (readIndex <= dataEndIndex)
    {
        data[index] = dataBuffer[readIndex];
        index++;
        readIndex++;
        if(index == contentLength) break;
        if(readIndex > dataEndIndex) readRawDataFromSocket();
    }
    // fixme 目前只考虑了application/x-www-form-urlencoded的post数据格式
    data[contentLength] = '&';
    data[contentLength + 1] = 0;
    string s_data(data);
    parseStringParamTo_htable_clientParamData(s_data);
    delete[] data;
}
// 分开url为url和paramStr.读取url中的GET的参数paramStr，并把键值对存在(*clientParamData)哈希表中
void HttpData::parseUrlDataParamList()
{
    int i = 0;
    while(i < url.size() && url[i] != '?') i++;
    if(i = url.size() || i + 1 > url.size()) return;//没有参数
    string paramStr = url.substr(i + 1);
    url = url.substr(0, i);;
    paramStr.push_back('&');
    parseStringParamTo_htable_clientParamData(paramStr);
}

void HttpData::parseStringParamTo_htable_clientParamData(string & str)
{
    if(clientParamData == nullptr) clientParamData = std::make_shared< std::unordered_map<string, string>>();
    int cur = 0, prev = 0;//用于分割&
    int start = 0, mid = 0, end =0;//用于分割=
    while (cur < str.size())
    {
        while (str[cur] != '&') cur++;
        start = prev;
        mid = start;
        while(str[mid] != '=') mid++;
        end = cur-1;
        if(end = mid) (*clientParamData)[str.substr(start, mid - start)] = "";
        else (*clientParamData)[str.substr(start, mid - start)] = str.substr(mid + 1, end - mid);
        cur++;
        prev = cur;
    }
    
}

int HttpData::numeralContentLength() const
{
    int i = 0;
    while((*contentLength)[i] != ' ') i++;
    i++;
    return std::stoi((*contentLength).substr( i, (*contentLength).size() - i ));
}

// 从套接字clientSocket中取出一段数据(最大1024)放入dataBuffer，设置readIndex(0)和dataEndIndex(size-1)
void HttpData::readRawDataFromSocket()
{
    int dataNum = recv(clientSocket, dataBuffer, HTTPDATA_BUFFERSIZE, 0);
    if(dataNum < 0) throw std::runtime_error("recv error!\n");
    if(dataNum == 0) throw SocketClosed();
    readIndex = 0;
    dataEndIndex = dataNum -1;
}

void HttpData::stringSplit(const string &s, vector<string> &tokens, const string &delimiters)
{
    string::size_type lastPos = s.find_first_not_of(delimiters, 0);
    string::size_type pos = s.find_first_of(delimiters, lastPos);
    while (string::npos != pos || string::npos != lastPos)
    {
        tokens.push_back(s.substr(lastPos, pos-lastPos));
        lastPos = s.find_first_not_of(delimiters, pos);
        pos = s.find_first_of(delimiters, lastPos);
    }
}

void HttpData::getRequestMethod(const vector<string> &headerInfo)
{
    if(headerInfo.size() == 0)
    {
        requestMethod = RequestMethod::UNSUPPORT;
        requestMethod_string = "UNSUPPORT";
        m_badRequest = true;
        return;
    }
    if(headerInfo[0] == "GET") { requestMethod = RequestMethod::GET; requestMethod_string = "GET";}
    else if(headerInfo[0] == "POST") { requestMethod = RequestMethod::POST; requestMethod_string = "POST";}
    else {requestMethod = RequestMethod::UNSUPPORT; requestMethod_string = "UNSUPPORT";}
}
