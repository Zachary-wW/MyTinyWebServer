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
    typedef std::vector<epoll_event> Events;
    typedef std::vector<Channel*> Channels;

    Epoller();
    ~Epoller();

    void Remove(Channel* channel_);
    void Poll(Channels& channels);
    int EpollWait() {
        return epoll_wait(epollfd_, &*events_.begin(),
                          static_cast<int>(events_.size()), -1);
    }

    void FillActiveChannels(int eventnums, Channels& channels);
    void Update(Channel* channel);
    void UpdateChannel(int operation, Channel* channel);

   private:
    // TODO
    typedef std::map<int, Channel*> ChannelMap; // Channel的映射，也帮忙保管所有注册在你这个Poller上的Channel

    int epollfd_;  // epoll create返回的句柄
    Events events_;
    ChannelMap channels_;
};

}  // namespace tiny_muduo
#endif
