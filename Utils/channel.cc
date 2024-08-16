#include "channel.h"

#include <sys/epoll.h>

using namespace tiny_muduo;

Channel::Channel(EventLoop* loop, const int& fd)
    : loop_(loop), fd_(fd), events_(0), recv_events_(0), state_(kNew) {}

Channel::~Channel() {}

void Channel::HandleEvent() {
    if (recv_events_ & (EPOLLIN | EPOLLPRI |
                        EPOLLRDHUP)) {  // EPOLLRDHUP表示对端关闭连接或发送了FIN
        if (read_callback_) {  // 如果回调函数存在
            read_callback_();
        }
    }

    if (recv_events_ & EPOLLOUT) {
        if (write_callback_) {
            write_callback_();
        }
    }
}
