#include "epoller.h"

#include <assert.h>
#include <string.h>
#include <sys/epoll.h>

#include <vector>

#include "channel.h"

using namespace tiny_muduo;

Epoller::Epoller()
    : epollfd_(::epoll_create1(
          EPOLL_CLOEXEC)),  // 将fd设置成FD_CLOEXEC，当fork的子进程使用execve()执行其他任务时，将之前的父进程fd关闭
      events_(kDefaultEvents), // 指定epoll_event的个数
      channels_() {}

Epoller::~Epoller() { ::close(epollfd_); }

void Epoller::Poll(Channels& channels) {
    int eventnums = EpollWait(); // 获取就绪fd_set
    FillActiveChannels(eventnums, channels);
}

void Epoller::FillActiveChannels(int eventnums, Channels& channels) { // 获取监听结果
    for (int i = 0; i < eventnums; ++i) {
        Channel* ptr = static_cast<Channel*>(events_[i].data.ptr); // 通过单个epoll_event中的event_data联合体中的ptr
        ptr->SetReceivedEvents(events_[i].events); // 设置监听的事件
        channels.emplace_back(ptr); // 加入active channels中
    }
    if (eventnums == static_cast<int>(events_.size())) {
        events_.resize(eventnums * 2); // events_容量不够扩容
    }
}

void Epoller::Remove(Channel* channel) {
    int fd = channel->fd();
    ChannelState state = channel->state();
    assert(state == kDeleted || state == kAdded);

    if (state == kAdded) {
        UpdateChannel(EPOLL_CTL_DEL, channel);
    }
    channel->SetChannelState(kNew);
    channels_.erase(fd);
    return;
}

void Epoller::Update(Channel* channel) {
    int op = 0, events = channel->events();
    ChannelState state = channel->state();
    int fd = channel->fd();

    if (state == kNew || state == kDeleted) {
        if (state == kNew) { // 如果channel是新来的
            assert(channels_.find(fd) == channels_.end());
            channels_[fd] = channel; // 加入
        } else {
            assert(channels_.find(fd) != channels_.end());
            assert(channels_[fd] == channel);
        }
        op = EPOLL_CTL_ADD;
        channel->SetChannelState(kAdded); // 更新channel状态
    } else { // 如果已经加入了
        assert(channels_.find(fd) != channels_.end());
        assert(channels_[fd] == channel);
        if (events == 0) { // 没有事件即可删除
            op = EPOLL_CTL_DEL;
            channel->SetChannelState(kDeleted);
        } else {
            op = EPOLL_CTL_MOD;
        }
    }

    UpdateChannel(op, channel); // 调用epoll_ctrl
}

void Epoller::UpdateChannel(int operation, Channel* channel) {
    struct epoll_event event;
    event.events = channel->events();
    event.data.ptr = static_cast<void*>(channel);

    if (epoll_ctl(epollfd_, operation, channel->fd(), &event) < 0) {
        // printf("Epoller::UpdateChannel epoll_ctl SYS_ERR\n");
    }
    return;
}
