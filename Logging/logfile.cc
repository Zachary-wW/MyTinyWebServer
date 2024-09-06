#include "logfile.h"

using namespace tiny_muduo;

LogFile::LogFile(const char* filepath = nullptr)
    // e means file descriptor will be closed if you use any of the exec functions
    : fp_(::fopen(filepath, "ae")),
      written_bytes_(0),
      lastwrite_(0),
      lastflush_(0) {
    if (!fp_) {
        std::string DefaultPath = std::move(
            "./LogFiles/LogFile_" +
            Timestamp::Now().Timestamp::ToFormattedDefaultLogString() + ".log");
        fp_ = ::fopen(DefaultPath.data(), "ae");
    }
}

LogFile::~LogFile() {
    Flush();
    fclose(fp_);
}

void LogFile::Write(const char* data, int len) {
    int pos = 0;
    while (pos != len) {
        pos += static_cast<int>(
            // 它是非线程安全的 fwrite 函数版本，用于将 data 指针加上当前位置 pos 处的数据写入文件指针 fp_ 指向的文件。
            // sizeof(char) 指定每个元素的大小，len - pos 指定要写入的元素数量。
            fwrite_unlocked(data + pos, sizeof(char), len - pos, fp_));
    }
    time_t now = ::time(nullptr);
    if (len != 0) {
        lastwrite_ = now;
        written_bytes_ += len;
    }
    // 检查自最后一次写入以来是否超过了预定的刷新间隔 kFlushInterval。
    if (lastwrite_ - lastflush_ > kFlushInterval) {
        Flush();
        lastflush_ = now;
    }
}
