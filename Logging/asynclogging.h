#ifndef TINY_MUDUO_ASYNCLOGGING_H_
#define TINY_MUDUO_ASYNCLOGGING_H_

#include <memory>
#include <vector>

#include "condition.h"
#include "latch.h"
#include "logfile.h"
#include "logging.h"
#include "logstream.h"
#include "mutex.h"
#include "noncopyable.h"
#include "thread.h"

namespace tiny_muduo {

static const double kBufferWriteTimeout = 3.0;
static const int64_t kSingleFileMaximumSize = 1024 * 1024 * 1024;

class AsyncLogging : public NonCopyAble {
   public:
    typedef FixedBuffer<kLargeSize> Buffer;
    typedef std::unique_ptr<Buffer> BufferPtr;
    typedef std::vector<BufferPtr> BufferVector;
    typedef std::unique_ptr<LogFile> LogFilePtr;

    AsyncLogging(const char* filepath = nullptr)
        : running_(false),
          filepath_(filepath),
          mutex_(),
          cond_(mutex_),
          latch_(1),
          thread_(std::bind(&AsyncLogging::ThreadFunc, this), "AsyncLogThread"),
          current_(new Buffer()),
          next_(new Buffer()) {}

    ~AsyncLogging() {
        if (running_) {
            Stop();
        }
    }

    void Stop() {
        running_ = false;
        cond_.Notify();
        thread_.Join();
    }

    void StartAsyncLogging() {
        running_ = true;
        thread_.StartThread();
        // 门阀latch_目的，在于让调用startthread()的线程等待线程函数启动完成
        latch_.Wait();
    }

    void Append(const char* data, int len);
    void Flush();
    void ThreadFunc();

   private:
    bool running_;
    const char* filepath_;
    MutexLock mutex_;
    Condition cond_;
    Latch latch_;
    Thread thread_;
    BufferPtr current_; // 当前使用的缓冲区，用于储存前端日志信息，如果不够则会使用预备缓冲区。
    BufferPtr next_; // 预备缓冲区，用于储存前端日志信息，在第一个缓冲区不够时使用

    // 已满缓冲区数组，用于储存前端日志信息，过一段时间或者缓冲区已满，就会将 Buffer 加入到 BufferVector 中。
    // 后端线程负责将 BufferVector 里的内容写入磁盘。
    BufferVector buffers_;
};

}  // namespace tiny_muduo

#endif
