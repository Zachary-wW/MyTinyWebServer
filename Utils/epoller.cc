#include "epoller.h"

#include <assert.h>
#include <string.h>
#include <sys/epoll.h>

#include <vector>

#include "channel.h"
#include "logging.h"

using namespace tiny_muduo;

// 将fd设置成FD_CLOEXEC，当fork的子进程使用execve()执行其他任务时，将之前的父进程fd关闭
Epoller::Epoller()
    : epollfd_(::epoll_create1(EPOLL_CLOEXEC)),
      events_(kDefaultEvents),  // 指定epoll_event的个数
      channelmap_() {}

Epoller::~Epoller() { ::close(epollfd_); }

void Epoller::Poll(Channels& channels) {
    int eventnums = EpollWait();  // 获取就绪fd_set
    FillActiveChannels(eventnums, channels);
}

// 获取监听结果
void Epoller::FillActiveChannels(int eventnums, Channels& channels) {
    for (int i = 0; i < eventnums; ++i) {
        // 通过单个epoll_event中的event_data联合体中的ptr
        Channel* ptr = static_cast<Channel*>(events_[i].data.ptr);
        // 设置监听的事件
        ptr->SetReceivedEvents(events_[i].events);
        // 加入active channels中
        channels.emplace_back(ptr);
    }
    if (eventnums == static_cast<int>(events_.size())) {
        events_.resize(eventnums * 2);
    }
}

// 从epoll中移除监听的channel
void Epoller::Remove(Channel* channel) {
    int fd = channel->fd();
    ChannelState state = channel->state();
    // 如果是kNew就错误了，因为还没有添加
    assert(state == kDeleted || state == kAdded);

    if (state == kAdded) {
        UpdateChannel(EPOLL_CTL_DEL, channel);
    }
    channel->SetChannelState(kNew);
    channelmap_.erase(fd);
    return;
}

// 更新epoll中channel状态
void Epoller::Update(Channel* channel) {
    int op = 0, events = channel->events();
    ChannelState state = channel->state();
    int fd = channel->fd();

    if (state == kNew || state == kDeleted) {
        if (state == kNew) {  // 如果channel是未被监听
            assert(channelmap_.find(fd) == channelmap_.end()); // 肯定找不到
            channelmap_[fd] = channel;  // 加入
        } else { // 如果已经被删除
            assert(channelmap_.find(fd) != channelmap_.end());// epoll中被删除 但map中还存在
            assert(channelmap_[fd] == channel); // key value对应的
        }
        op = EPOLL_CTL_ADD;
        channel->SetChannelState(kAdded);  // 更新channel状态 表示被监听
    } else {                               // 如果已经加入了
        assert(channelmap_.find(fd) != channelmap_.end());
        assert(channelmap_[fd] == channel);
        if (events == 0) {  // 没有事件即可删除
            op = EPOLL_CTL_DEL;
            channel->SetChannelState(kDeleted);
        } else {
            op = EPOLL_CTL_MOD;
        }
    }

    UpdateChannel(op, channel);  // 调用epoll_ctrl
}

void Epoller::UpdateChannel(int operation, Channel* channel) {
    struct epoll_event event;
    event.events = channel->events();
    event.data.ptr = static_cast<void*>(channel);

    if (epoll_ctl(epollfd_, operation, channel->fd(), &event) < 0) {
        LOG_ERROR << "Epoller::UpdataChannel epoll_ctl failed";
    }
    return;
}
