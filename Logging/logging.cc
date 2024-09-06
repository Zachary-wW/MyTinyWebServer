#include "logging.h"

#include <utility>

using namespace tiny_muduo;
using namespace CurrentThread;

namespace CurrentThread {

// __thread变量每一个线程有一份独立实体，各个线程的值互不干扰。
// 可以用来修饰那些带有全局性且值可能变，但是又不值得用全局变量保护的变量。

__thread time_t t_lastsecond;
__thread char t_time[64];
__thread const int t_formattedtimeLength = 18;
__thread char t_errorbuf[512];

}  // namespace CurrentThread

const char* ErrorToString(int err) {
    // 根据错误码得到对应的错误描述 strerror_r更加安全  因为指定了buffer
    return strerror_r(err, t_errorbuf, sizeof(t_errorbuf));
}

// 日志级别转换成字符串的最大长度
static const int kLogLevelStringLength = 6;

// 默认输出到终端
static void DefaultOutput(const char* data, int len) {
    fwrite(data, len, sizeof(char), stdout);
}

// 默认刷新到终端
static void DefaultFlush() { fflush(stdout); }

// 两个全局函数指针，其执行的函数会确定日志信息的输出位置
Logger::OutputFunc g_Output = DefaultOutput;
Logger::FlushFunc g_Flush = DefaultFlush;
Logger::Level g_level = Logger::Level::INFO;  // 默认为INFO

// 设置输出和刷新函数
void SetOutputFunc(Logger::OutputFunc func) { g_Output = func; }
void SetFlushFunc(Logger::FlushFunc func) { g_Flush = func; }

// 获取和设置日志级别
Logger::Level LogLevel() { return g_level; }
void SetLogLevel(tiny_muduo::Logger::Level nowlevel) { g_level = nowlevel; }

//
Logger::Implement::Implement(SourceClass&& source, int line, Level level)
    : fileinfo_(std::move(source)), line_(line), level_(level) {
    // 实例化文件名、第几行、日志等级
    FormattedTime();
    CurrentThread::tid();
}

Logger::Implement::~Implement() {
    Finish();
    const LogStream::Buffer& buffer = stream_.buffer();
    g_Output(buffer.data(), buffer.len());
}

// 枚举转换成const char*
const char* Logger::Implement::GetLogLevel() const {
    switch (level_) {
        case DEBUG:
            return "DEBUG ";
        case INFO:
            return "INFO  ";
        case WARN:
            return "WARN  ";
        case ERROR:
            return "ERROR ";
        case FATAL:
            return "FATAL ";
    }
    return nullptr;
}

void Logger::Implement::FormattedTime() {
    Timestamp now = Timestamp::Now();
    time_t seconds =
        static_cast<time_t>(now.microseconds() / kMicrosecond2Second);
    int microseconds =
        static_cast<int>(now.microseconds() % kMicrosecond2Second);

    // 检查当前秒数是否与上次记录的秒数不同
    if (t_lastsecond != seconds) {
        struct tm tm_time;
        // 使用 localtime_r 函数（线程安全的 localtime 函数）将 seconds 转换为本地时间，并存储在 tm_time 中
        localtime_r(&seconds, &tm_time);
        // 使用 snprintf 函数将时间格式化为字符串
        snprintf(t_time, sizeof(t_time), "%4d%02d%02d %02d:%02d:%02d.",
                 tm_time.tm_year + 1900, tm_time.tm_mon + 1, tm_time.tm_mday,
                 tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec);
        t_lastsecond = seconds;
    }

    // 使用 snprintf 函数将微秒部分格式化为字符串，并存储在 buf 中。microlen 存储格式化后的字符串长度。
    char buf[32] = {0};
    int microlen = snprintf(buf, sizeof(buf), "%06d ", microseconds);

    stream_ << GeneralTemplate(t_time, t_formattedtimeLength) // 时间
            << GeneralTemplate(buf, microlen); // 微秒
    stream_ << GeneralTemplate(CurrentThread::t_formattedTid,
                               CurrentThread::t_formattedTidLength); // 格式化线程ID
    stream_ << GeneralTemplate(GetLogLevel(), kLogLevelStringLength); // 日志等级
}
