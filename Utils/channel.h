#ifndef TINY_MUDUO_CHANNEL_H_
#define TINY_MUDUO_CHANNEL_H_

#include <sys/epoll.h>

#include <utility>

#include "callback.h"
#include "eventloop.h"
#include "noncopyable.h"

namespace tiny_muduo {

enum ChannelState { kNew, kAdded, kDeleted };

class Channel : public NonCopyAble {
   public:
    Channel(EventLoop* loop, const int& fd);
    ~Channel();

    void HandleEvent();

    // TODO
    void SetReadCallback(ReadCallback&& callback) {  // 用来匹配右值
        read_callback_ = std::move(callback);
    }

    void SetReadCallback(const ReadCallback& callback) {
        read_callback_ = callback;
    }

    void SetWriteCallback(WriteCallback&& callback) {
        write_callback_ = std::move(callback);
    }

    void SetWriteCallback(const WriteCallback& callback) {
        write_callback_ = callback;
    }

    void EnableReading() {
        events_ |= (EPOLLIN | EPOLLPRI); // EPOLLPRI表示有优先级数据可读（如信号）
        Update(); // 注册到事件监听器（本质上调用epoll ctrl）
    }

    void EnableWriting() {
        events_ |= EPOLLOUT;
        Update();
    }

    void DisableAll() {
        events_ = 0;
        Update();
    }

    // TODO
    void DisableWriting() {
        events_ &= ~EPOLLOUT;
        Update();
    }

    void Update() { loop_->Update(this); }

    void SetReceivedEvents(int revt) { recv_events_ = revt; }

    void SetChannelState(ChannelState state) { state_ = state; }

    int fd() const { return fd_; }
    int events() const { return events_; }
    int recv_events() const { return recv_events_; }
    ChannelState state() const { return state_; }

    bool IsWriting() { return events_ & EPOLLOUT; }
    bool IsReading() { return events_ & (EPOLLIN | EPOLLPRI); }

   private:
    EventLoop* loop_; // channel（fd）对应的loop
    int fd_;      // 一个channel内部封装一个fd
    int events_;  // 感兴趣的事件集合
    int recv_events_;  // 事件监视器返回的事件集合，即真正发生的事件集合

    ChannelState state_;
    // TODO
    // 读写事件处理回调函数
    ReadCallback read_callback_;
    WriteCallback write_callback_;
};

}  // namespace tiny_muduo
#endif
