#ifndef TINY_MUDUO_CHANNEL_H_
#define TINY_MUDUO_CHANNEL_H_

#include <sys/epoll.h>

#include <utility>
#include <memory>

#include "eventloop.h"
#include "callback.h"
#include "noncopyable.h"
#include "logging.h"

namespace tiny_muduo {

enum ChannelState {
  kNew,
  kAdded,
  kDeleted
};

class Channel : public NonCopyAble {
 public:
  typedef std::function<void()> ErrorCallback;
 
  Channel(EventLoop* loop, const int& fd);
  ~Channel();

  void HandleEvent();
  void HandleEventWithGuard();

  // 用来匹配右值
  void SetReadCallback(ReadCallback&& callback) { 
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
  
  void SetErrorCallback(ErrorCallback&& callback) {
    error_callback_ = std::move(callback);
  }
  
  void SetErrorCallback(const ErrorCallback& callback) {
    error_callback_ = callback;
  }  
 
  void Tie(const std::shared_ptr<void>& ptr) {
  	tied_ = true;
  	tie_ = ptr;
  } 

  void EnableReading() {
    // EPOLLPRI表示有优先级数据可读（如信号）
    events_ |= (EPOLLIN | EPOLLPRI); 
    // 注册到事件监听器（本质上调用epoll ctrl）
    Update();  
  }

  void EnableWriting() {
    events_ |= EPOLLOUT;
    Update();
  }

  void DisableAll() {
    events_ = 0;
    Update(); 
  }

  void DisableWriting() {
    events_ &= ~EPOLLOUT;
    Update();  
  }

  void Update() {
    loop_->Update(this);
  }

  void SetReceivedEvents(int events__) {
    recv_events_ = events__;
  }

  void SetChannelState(ChannelState state__) {
    state_ = state__;
  }

  int fd() const { return fd_; } 
  int events() const { return events_; }
  int recv_events() const { return recv_events_; }
  ChannelState state() const { return state_; }
  
  bool IsWriting() const { return events_ & EPOLLOUT; }
  bool IsReading() const { return events_ & (EPOLLIN | EPOLLPRI); }

 private:
  EventLoop* loop_; // channel（fd）对应的loop
  int fd_; // 一个channel内部封装一个fd
  int events_;      // 感兴趣的事件集合
  int recv_events_; // 事件监视器返回的事件集合，即真正发生的事件集合
  std::weak_ptr<void> tie_;
  bool tied_;
  int errno_;
  
  ChannelState state_;
  ReadCallback read_callback_;
  WriteCallback write_callback_;  
  ErrorCallback error_callback_;
};

}
#endif
