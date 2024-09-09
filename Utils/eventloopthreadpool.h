#ifndef TINY_MUDUO_EVENTLOOPTHREADPOOL_H_
#define TINY_MUDUO_EVENTLOOPTHREADPOOL_H_

#include <memory>
#include <vector>

#include "noncopyable.h"

namespace tiny_muduo {

class EventLoopThread;
class EventLoop;

class EventLoopThreadPool : public NonCopyAble {
   public:
    typedef std::vector<std::unique_ptr<EventLoopThread>> Thread;
    typedef std::vector<EventLoop*> Loop;

    EventLoopThreadPool(EventLoop* loop);
    ~EventLoopThreadPool();

    void SetThreadNums(int thread_nums) { thread_nums_ = thread_nums; }

    void StartLoop();
    EventLoop* NextLoop();

   private:
    EventLoop* base_loop_; // main创建的loop
    Thread threads_; // IO线程vector
    Loop loops_; // EventLoop列表, 指向的是EventLoopThread线程函数创建的EventLoop对象

    int thread_nums_;
    int next_; // 新连接到来，所选择的EventLoopThread下标
};

}  // namespace tiny_muduo
#endif
