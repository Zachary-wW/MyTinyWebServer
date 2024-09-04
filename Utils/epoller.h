#ifndef TINY_MUDUO_EPOLLER_H_
#define TINY_MUDUO_EPOLLER_H_

#include <sys/epoll.h>

#include <map>
#include <vector>

#include "noncopyable.h"

static const int kDefaultEvents = 16;

namespace tiny_muduo {

class Channel;

class Epoller : public NonCopyAble {
   public:
    //  struct epoll_event
    // {
    //   uint32_t events;	/* Epoll events */
    //   epoll_data_t data;	/* User data variable */
    // } __EPOLL_PACKED;

    typedef std::vector<epoll_event> Events; // 事件集合
    typedef std::vector<Channel*> Channels;

    Epoller();
    ~Epoller();

    void Poll(Channels& channels);

    void Remove(Channel* channel);

    int EpollWait() {
      // 因为epoll wait第二个参数是一个epoll_event *类型
      // timeout参数中0会立即返回，小于0时将是永久阻塞
        return epoll_wait(epollfd_, &*events_.begin(),
                          static_cast<int>(events_.size()), -1);
    }

    void FillActiveChannels(int eventnums, Channels& channels);
    void Update(Channel* channel);
    void UpdateChannel(int operation, Channel* channel);

   private:
    // fd和Channel的映射，也帮忙保管所有注册在你这个Poller上的Channel
    typedef std::map<int, Channel*> ChannelMap;

    int epollfd_;  // epoll create返回的句柄
    Events events_;
    ChannelMap channelmap_;
};

}  // namespace tiny_muduo
#endif
