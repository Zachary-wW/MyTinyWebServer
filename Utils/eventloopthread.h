#ifndef TINY_MUDUO_EVENTLOOPTHREAD_H_
#define TINY_MUDUO_EVENTLOOPTHREAD_H_

#include "condition.h"
#include "mutex.h"
#include "noncopyable.h"
#include "thread.h"

namespace tiny_muduo {

class EventLoop;

class EventLoopThread : public NonCopyAble {
   public:
    EventLoopThread();
    ~EventLoopThread();

    void StartFunc();
    EventLoop* StartLoop();

   private:
    EventLoop* loop_; // 绑定的EventLoop对象指针
    Thread thread_;
    MutexLock mutex_;
    Condition cond_;
};

}  // namespace tiny_muduo
#endif
