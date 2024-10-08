#include "eventloopthreadpool.h"

#include <memory>

#include "eventloopthread.h"

using namespace tiny_muduo;

EventLoopThreadPool::EventLoopThreadPool(EventLoop* loop)
    : base_loop_(loop), thread_nums_(0), next_(0) {}

// 栈变量 不用释放
EventLoopThreadPool::~EventLoopThreadPool() {}

void EventLoopThreadPool::StartLoop() {
    for (int i = 0; i < thread_nums_; ++i) {
        EventLoopThread* ptr = new EventLoopThread();
        threads_.emplace_back(std::unique_ptr<EventLoopThread>(ptr));
        loops_.emplace_back(ptr->StartLoop()); // 启动IO线程, 并将线程函数创建的EventLoop对象地址 插入loops_数组
    }
}

// round-robin(轮询)，从线程池获取下一个event loop
EventLoop* EventLoopThreadPool::NextLoop() {
    EventLoop* ret = base_loop_;
    if (!loops_.empty()) {
        ret = loops_[next_++];
        if (next_ == static_cast<int>(loops_.size())) next_ = 0;
    }

    return ret;
}
