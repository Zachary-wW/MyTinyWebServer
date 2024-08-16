#ifndef TINY_MUDUO_EVENTLOOP_H_
#define TINY_MUDUO_EVENTLOOP_H_

#include <assert.h>
#include <pthread.h>
#include <stdint.h>
#include <sys/eventfd.h>
#include <unistd.h>

#include <functional>
#include <memory>
#include <vector>

#include "currentthread.h"
#include "epoller.h"
#include "mutex.h"
#include "noncopyable.h"

namespace tiny_muduo {

class Epoller;
class Channel;

class EventLoop : public NonCopyAble {
   public:
    typedef std::vector<Channel*> Channels;
    typedef std::function<void()> BasicFunc; 
    typedef std::vector<BasicFunc> ToDoList;

    EventLoop();
    ~EventLoop();

    bool IsInThreadLoop() { return CurrentThread::tid() == tid_; }
    void Update(Channel* channel) { epoller_->Update(channel); }
    void Remove(Channel* channel) { epoller_->Remove(channel); }

    void Loop();
    void HandleRead();
    void QueueOneFunc(BasicFunc func);
    void RunOneFunc(BasicFunc func);
    void DoToDoList();

   private:
    pid_t tid_;
    std::unique_ptr<Epoller> epoller_;
    int wakeup_fd_;
    std::unique_ptr<Channel> wakeup_channel_;
    bool calling_functors_; // 标志当前loop是否有需要执行的回调操作
    Channels active_channels_;
    ToDoList to_do_list_;

    MutexLock mutex_;
};

}  // namespace tiny_muduo
#endif
