#ifndef TINY_MUDUO_THREAD_H_
#define TINY_MUDUO_THREAD_H_

#include <pthread.h>
#include <stdio.h>
#include <sys/prctl.h>

#include <functional>
#include <string>

#include "latch.h"
#include "noncopyable.h"

using std::string;

namespace tiny_muduo {

static int thread_id_ = 1;

class Thread : public NonCopyAble {
   public:
    typedef std::function<void()> ThreadFunc;

    Thread(const ThreadFunc& func, const string& name = string());
    ~Thread();

    void StartThread();
    void Join() { ::pthread_join(pthread_id_, nullptr); }

   private:
    pthread_t pthread_id_; // 线程ID
    ThreadFunc func_; // 线程函数
    Latch latch_; // 线程同步，解决调用线程和新线程的同步问题的，初始化为1
    string name_; // 线程名称
};

// 新线程通用数据的封装
struct ThreadData {
    typedef tiny_muduo::Thread::ThreadFunc ThreadFunc;

    ThreadFunc func_;
    Latch* latch_;// 新线程的启动与调用线程的同步
    string name_;

    ThreadData(ThreadFunc& func, Latch* latch, const string& name)
        : func_(func), latch_(latch), name_(name) {}

    void RunOneFunc() {
        latch_->CountDown();
        latch_ = nullptr;
        char buf[32] = {0};
        // threadid自增确保每次调用时都能生成一个唯一的标识符
        snprintf(buf, sizeof(buf), "%d", (thread_id_++));
        // 调用prctl修改线程在内核中的名称
        ::prctl(PR_SET_NAME, name_.size() == 0
                                 ? ("WorkThread " + string(buf)).data()
                                 : name_.data());
        func_();
    }
};

}  // namespace tiny_muduo
#endif
