#ifndef TINY_MUDUO_ACCEPTOR_H_
#define TINY_MUDUO_ACCEPTOR_H_

#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#include <functional>
#include <memory>

#include "noncopyable.h"

namespace tiny_muduo {

class EventLoop;
class Address;
class Channel;

class Acceptor : public NonCopyAble {
   public:
    typedef std::function<void(int)> NewConnectionCallback;

    Acceptor(EventLoop* loop, const Address& address);
    ~Acceptor();

    void BindListenFd(const Address& address);
    void Listen();
    void NewConnection();

    int SetSockoptKeepAlive(int fd) {
        int option_val = 1;
        // setsockopt用于任意类型、任意状态套接口的设置选项值
        // SOL_SOCKET指在套接字级别上设置选项
        // SO_KEEPALIVE保持连接 TCP下socket不是listen or close && optional
        // val不为零
        return setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &option_val,
                          static_cast<socklen_t>(sizeof(option_val)));
    }

    int SetSockoptReuseAddr(int fd) {
        int option_val = 1;
        // SO_REUSEADDR打开或关闭地址复用功能（取决于optional val的值：~0/0）
        return setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &option_val,
                          static_cast<socklen_t>(sizeof(option_val)));
    }

    int SetSockoptTcpNoDelay(int fd) {
        int option_val = 1;
        // IPPROTO_IP:IP选项.IPv4套接口
        return setsockopt(fd, IPPROTO_TCP, SO_KEEPALIVE, &option_val,
                          static_cast<socklen_t>(sizeof(option_val)));
    }

    void SetNewConnectionCallback(const NewConnectionCallback& callback) {
        new_connection_callback_ = callback;
    }

    void SetNewConnectionCallback(NewConnectionCallback&& callback) {
        new_connection_callback_ = std::move(callback);
    }

   private:
    EventLoop* loop_;  // main EventLoop
    int listenfd_;     // 监听的fd
    int idlefd_;
    std::unique_ptr<Channel> channel_;  // 将listenfd及其感兴趣事件封装
    NewConnectionCallback new_connection_callback_;
};

}  // namespace tiny_muduo
#endif
