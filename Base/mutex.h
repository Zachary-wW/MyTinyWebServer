#ifndef TINY_MUDUO_MUTEX_H_
#define TINY_MUDUO_MUTEX_H_

#include <pthread.h>

#include "noncopyable.h"

namespace tiny_muduo {

class MutexLock : public NonCopyAble {
   public:
    MutexLock() { pthread_mutex_init(&mutex_, nullptr); }

    ~MutexLock() { pthread_mutex_destroy(&mutex_); }

    // 获取锁
    pthread_mutex_t* pthreadmutex() { return &mutex_; } 
    // 加锁
    bool Lock() { return pthread_mutex_lock(&mutex_) == 0; }
    // 释放锁
    bool Unlock() { return pthread_mutex_unlock(&mutex_) == 0; }
    // FIXME：为什么和pthreadmutex一样？
    pthread_mutex_t* mutex() { return &mutex_; }

   private:
    pthread_mutex_t mutex_;
};

// RAII思想，使得mutex能够自动释放锁
class MutexLockGuard {
   public:
    explicit MutexLockGuard(MutexLock& mutex) : mutex_(mutex) { mutex_.Lock(); }
    ~MutexLockGuard() { mutex_.Unlock(); }

   private:
    MutexLock& mutex_;
};

}  // namespace tiny_muduo
#endif