#include "eventloopthread.h"

#include <pthread.h>

#include <functional>

#include "condition.h"
#include "eventloop.h"
#include "mutex.h"

using namespace tiny_muduo;

EventLoopThread::EventLoopThread()
    : loop_(nullptr),
      thread_(std::bind(&EventLoopThread::StartFunc, this)),
      mutex_(),
      cond_(mutex_) {}

EventLoopThread::~EventLoopThread() {}

/* 启动IO线程函数中的loop循环, 返回IO线程中创建的EventLoop对象地址(栈空间) */
EventLoop* EventLoopThread::StartLoop() {
    thread_.StartThread();
    EventLoop* loop = nullptr;
    {
        MutexLockGuard lock(mutex_);
        while (loop_ == nullptr) {
            cond_.Wait(); // 同步等待线程函数完成初始化工作, 唤醒等待在此处的调用线程
        }
        loop = loop_;
    }
    return loop;
}

// IO线程函数, 创建EventLoop局部对象, 运行loop循环
void EventLoopThread::StartFunc() {
    EventLoop loop;

    {
        MutexLockGuard lock(mutex_);
        loop_ = &loop;
        cond_.Notify(); // 唤醒等待线程
    }

    loop_->Loop();
    {
        MutexLockGuard lock(mutex_);
        loop_ = nullptr;
    }
}
