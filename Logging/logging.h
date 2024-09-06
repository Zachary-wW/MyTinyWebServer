#ifndef TINY_MUDUO_LOGGING_H_
#define TINY_MUDUO_LOGGING_H_

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>

#include <cstring>

#include "currentthread.h"
#include "logstream.h"
#include "noncopyable.h"
#include "timestamp.h"

namespace tiny_muduo {

class SourceClass {
   public:
    SourceClass(const char* data)
        : data_(data), len_(static_cast<int>(strlen(data_))) {
        // 参数 str 所指向的字符串中搜索最后一次出现字符 c
        // 如果找到，forward_slash 将指向该字符；如果没有找到，forward_slash 将为 nullptr
        const char* forward_slash = strrchr(data, '/');
        if (forward_slash) {
            // 如果找到了 / 字符，这行代码将 data_ 更新为指向 / 之后的第一个字符，即文件名的开始部分。
            data_ = forward_slash + 1;
            // 减去从原始 data 指针到新 data_ 指针之间的字符数。这样做是为了得到文件名的长度，而不是整个路径的长度。
            len_ -= static_cast<int>((data_ - data));
        }
    }

    const char* data_;
    int len_;
};

class Logger : public NonCopyAble {
   public:
    // 日志级别
    enum Level { DEBUG, INFO, WARN, ERROR, FATAL };

    Logger(const char* file_, int line, Level level)
        : implement_(file_, line, level) {}

    LogStream& stream() { return implement_.stream(); }

    // 定义输出和刷新的函数指针
    typedef void (*OutputFunc)(const char* data, int len);
    typedef void (*FlushFunc)();

   private:
    class Implement : public NonCopyAble {
       public:
        typedef Logger::Level Level;

        Implement(SourceClass&& source, int line, Level level);
        ~Implement();

        void FormattedTime();
        const char* GetLogLevel() const;

        void Finish() {
            stream_ << " - " << GeneralTemplate(fileinfo_.data_, fileinfo_.len_)
                    << ':' << line_ << '\n';
        }

        LogStream& stream() { return stream_; }

       private:
        SourceClass fileinfo_;
        int line_;
        Level level_;

        LogStream stream_;
    };

    Implement implement_;
};

}  // namespace tiny_muduo

tiny_muduo::Logger::Level LogLevel();
void SetLogLevel(tiny_muduo::Logger::Level nowlevel);
const char* ErrorToString(int err);

// 宏函数
// 每个宏定义都会构造出一个 Logger 的临时对象，然后输出相关信息
#define LOG_DEBUG                                \
    if (LogLevel() <= tiny_muduo::Logger::DEBUG) \
    tiny_muduo::Logger(__FILE__, __LINE__, tiny_muduo::Logger::DEBUG).stream()
#define LOG_INFO                                \
    if (LogLevel() <= tiny_muduo::Logger::INFO) \
    tiny_muduo::Logger(__FILE__, __LINE__, tiny_muduo::Logger::INFO).stream()
#define LOG_WARN \
    tiny_muduo::Logger(__FILE__, __LINE__, tiny_muduo::Logger::WARN).stream()
#define LOG_ERROR \
    tiny_muduo::Logger(__FILE__, __LINE__, tiny_muduo::Logger::ERROR).stream()
#define LOG_FATAL \
    tiny_muduo::Logger(__FILE__, __LINE__, tiny_muduo::Logger::FATAL).stream()

#endif
