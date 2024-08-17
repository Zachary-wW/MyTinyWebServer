#ifndef TINY_MUDUO_TCPCONNECTION_H_
#define TINY_MUDUO_TCPCONNECTION_H_

#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <memory>
#include <string>
#include <utility>

#include "buffer.h"
#include "callback.h"
#include "channel.h"
#include "httpcontent.h"
#include "noncopyable.h"

using std::string;

namespace tiny_muduo {

class EventLoop;

class TcpConnection : public std::enable_shared_from_this<TcpConnection>,
                      NonCopyAble {
   public:
    enum ConnectionState { kConnected, kDisconnected };

    TcpConnection(EventLoop* loop, int connfd);
    ~TcpConnection();

    void SetConnectionCallback(const ConnectionCallback& callback) {
        connection_callback_ = callback;
    }

    void SetConnectionCallback(ConnectionCallback&& callback) {
        connection_callback_ = std::move(callback);
    }

    void SetMessageCallback(const MessageCallback& callback) {
        message_callback_ = callback;
    }

    void SetMessageCallback(MessageCallback&& callback) {
        message_callback_ = std::move(callback);
    }

    void SetCloseCallback(const CloseCallback& callback) {
        close_callback_ = callback;
    }

    void SetCloseCallback(CloseCallback&& callback) {
        close_callback_ = std::move(callback);
    }
    // 连接建立时更改连接状态，注册读事件，注册连接回调函数
    void ConnectionEstablished() {
        state_ = kConnected;
        channel_->EnableReading();
        connection_callback_(shared_from_this(), &input_buffer_);
    }

    void Shutdown();
    bool IsShutdown() { return shutdown_state_; }
    void ConnectionDestructor();
    void HandleClose();
    void HandleMessage();
    void HandleWrite();
    void Send(Buffer* buffer);
    void Send(const string& str);
    void Send(const char* message, int len);
    void Send(const char* message) { Send(message, strlen(message)); }

    int fd() const { return fd_; }
    EventLoop* loop() const { return loop_; }
    HttpContent* GetHttpContent() { return &content_; }

   private:
    EventLoop* loop_; // subEventloop
    int fd_; // 已连接的socketfd
    ConnectionState state_;
    std::unique_ptr<Channel> channel_; // 封装fd_
    Buffer input_buffer_; // 接受缓冲区
    Buffer output_buffer_; // 发送缓冲区 由于TCP发送缓冲区有限
    HttpContent content_;
    bool shutdown_state_;

    ConnectionCallback connection_callback_;
    MessageCallback message_callback_;
    CloseCallback close_callback_;
};

// TODO
typedef std::shared_ptr<TcpConnection> TcpconnectionPtr;

}  // namespace tiny_muduo
#endif
