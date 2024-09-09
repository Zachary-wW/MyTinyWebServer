#ifndef TINY_MUDUO_LATCH_H_
#define TINY_MUDUO_LATCH_H_

#include "condition.h"
#include "mutex.h"
#include "noncopyable.h"

namespace tiny_muduo {

// 作为线程同步
// 内部持有一个向下计数的计数器count_，构造时给定一个初值，代表需要等待的线程数。
// 每个线程完成一个任务，count_减1，当count_值减到0时，代表所有线程已经完成了所有任务，
// 在CountDownLatch上等待的线程就可以继续执行了
class Latch : public NonCopyAble {
   public:
    explicit Latch(int count) : count_(count), mutex_(), cond_(mutex_) {}

    void CountDown() {
        MutexLockGuard lock(mutex_);
        --count_;
        if (count_ == 0) {
            cond_.NotifyAll();
        }
    }

    void Wait() {
        MutexLockGuard lock(mutex_);
        while (count_ > 0) {
            cond_.Wait();
        }
    }

   private:
    int count_;
    MutexLock mutex_;
    Condition cond_;
};

}  // namespace tiny_muduo
#endif