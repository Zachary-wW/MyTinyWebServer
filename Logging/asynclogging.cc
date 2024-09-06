#include "asynclogging.h"

#include <functional>
#include <memory>
#include <utility>

using namespace tiny_muduo;

void AsyncLogging::Append(const char* data, int len) {
    // 锁保持同步
    MutexLockGuard guard(mutex_);
    if (current_->writablebytes() >= len) {
        // 当前缓冲区能够放下，直接写入
        current_->Append(data, len);
    } else {
        // 如果当前缓冲区放不下

        // 将当前缓冲区加入BufferVector
        buffers_.emplace_back(std::move(current_));
        if (next_) {
            // 使用预备缓冲区填充当前缓冲区
            current_ = std::move(next_);
        } else {
            // 如果预备的也满了，重新申请
            current_.reset(new Buffer());
        }
        current_->Append(data, len);
        // 唤醒后端线程
        cond_.Notify();
    }
}

void AsyncLogging::Flush() { fflush(stdout); }

/**
 * 后端线程函数threadFunc，会构建1个LogFile对象，用于控制log文件创建、写日志数据，
 * 创建2个空闲缓冲区buffer1、buffer2，和一个待写缓冲队列buffersvector，
 * 分别用于替换当前缓冲currentBuffer_、空闲缓冲nextBuffer_、已满缓冲队列buffers_，
 * 从而避免在写文件过程中，锁住缓冲和队列，导致前端无法写数据到后端缓冲。
 */

void AsyncLogging::ThreadFunc() {
    latch_.CountDown();
    // buffer1 和 buffer2
    BufferPtr newbuffer_current(new Buffer());
    BufferPtr newbuffer_next(new Buffer());
    newbuffer_current->SetBufferZero();
    newbuffer_next->SetBufferZero();

    LogFilePtr log(new LogFile(filepath_));
    BufferVector buffersToWrite;
    // 缓冲区数组置为16个
    buffersToWrite.reserve(16);

    while (running_) {
        {
            MutexLockGuard guard(mutex_);
            // 说明没有一个数组被写满
            if (buffers_.empty()) {
                cond_.WaitForFewSeconds(
                    kBufferWriteTimeout);  // 等待条件变量 / timeout
            }

            buffers_.emplace_back(std::move(current_));
            buffersToWrite.swap(buffers_);
            current_ = std::move(newbuffer_current);
            if (!next_) {
                next_ = std::move(newbuffer_next);
            }
        }

        if (buffersToWrite.size() > 25) {
            char buf[256];
            snprintf(buf, sizeof buf,
                     "Dropped log messages at %s, %zd larger buffers\n",
                     Timestamp::Now().ToFormattedDefaultLogString().c_str(),
                     buffersToWrite.size() - 2);
            fputs(buf, stderr);
            log->Write(buf, static_cast<int>(strlen(buf)));
            // 只保留前两个
            buffersToWrite.erase(buffersToWrite.begin() + 2,
                                 buffersToWrite.end());
        }

        // 写入logfile中
        for (const auto& buffer : buffersToWrite) {
            log->Write(buffer->data(), buffer->len());
        }

        if (log->writtenbytes() >= kSingleFileMaximumSize) {
            log.reset(new LogFile(filepath_));
        }

        if (buffersToWrite.size() > 2) {
            buffersToWrite.resize(2);
        }

        // 归还buffer1 和 buffer2同时setzero
        if (!newbuffer_current) {
            newbuffer_current = std::move(buffersToWrite.back());
            buffersToWrite.pop_back();
            newbuffer_current->SetBufferZero();
        }
        if (!newbuffer_next) {
            newbuffer_current = std::move(buffersToWrite.back());
            buffersToWrite.pop_back();
            newbuffer_current->SetBufferZero();
        }

        buffersToWrite.clear();
        log->Flush();
    }
    log->Flush();
}
