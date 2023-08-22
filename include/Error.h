#ifndef __ERROR__
#define __ERROR__
#include <exception> //exception类包含了各类基本错误

class SocketClosed
{
public:
    SocketClosed(){}
    ~SocketClosed () throw() {}
    virtual const char* what() const _GLIBCXX_TXN_SAFE_DYN _GLIBCXX_NOTHROW
    {
        return "socket closed";
    }
};

#endif

/*使用throw SocketClosed();后，SocketClosed类怎么执行？
创建对象吗？然后呢？*/

/*答：当你使用 throw SocketClosed(); 语句时，会发生以下几件事情：
首先，会创建一个临时的 SocketClosed 对象。
这个对象是通过调用 SocketClosed 类的默认构造函数来创建的。
然后，这个临时对象会被抛出作为异常。
这会导致程序的控制流跳转到最近的 catch 语句，以便捕获和处理这个异常。
如果有一个 catch 语句能够捕获到这个异常（例如 catch (const SocketClosed& e)），
那么这个 catch 语句块中的代码就会被执行。
在这个 catch 语句块中，你可以访问这个临时对象，
并调用它的方法（例如 e.what()）来获取异常的信息。
当 catch 语句块中的代码执行完毕后，这个临时对象会被销毁。
这是通过调用 SocketClosed 类的析构函数来完成的。*/

