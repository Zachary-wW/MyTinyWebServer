#ifndef TINY_MUDUO_LOGSTREAM_H_
#define TINY_MUDUO_LOGSTREAM_H_

#include <stdio.h>
#include <string.h>

#include <algorithm>
#include <string>

#include "noncopyable.h"

namespace tiny_muduo {

// 两种不同的缓冲大小
const int kSmallSize = 4096;         // 4KB
const int kLargeSize = 4096 * 1000;  // 4MB

/**
 * LogStream 类用于重载正文信息，一次信息大小是有限的，其使用的缓冲区的大小就是
 * kSmallBuffer。 而后端日志 AsyncLogging
 * 需要存储大量日志信息，其会使用的缓冲区大小更大，所以是 kLargeBuffer。
 */

static const int kNum2MaxStringLength = 48; // int转string的最大长度
static const char digits[] = {'9', '8', '7', '6', '5', '4', '3', '2', '1', '0',
                              '1', '2', '3', '4', '5', '6', '7', '8', '9'};

// 封装字符串和它的长度
class GeneralTemplate : public NonCopyAble {
   public:
    GeneralTemplate()
        : data_(nullptr), len_(0) {}  // 用于创建一个空的 GeneralTemplate 对象
    explicit GeneralTemplate(const char* data, int len)
        : data_(data), len_(len) {}

    const char* data_;
    int len_;
};

// FixedBuffer 类来实现这个存放日志信息的缓冲区
template <int SIZE>
class FixedBuffer : public NonCopyAble {
   public:
    FixedBuffer();
    ~FixedBuffer();

    static void CookieStart();
    static void CookieEnd();

    // 向缓冲区中添加数据
    void Append(const char* input_data, int length) {
        if (writablebytes() < length) {
            length = writablebytes();
        }
        // 将长为len的数据拷贝到cur_地址处
        memcpy(cur_, input_data, length);
        cur_ += length;
    }

    // 缓冲区置0
    void SetBufferZero() {
        memset(buf_, '\0', sizeof(buf_));
        cur_ = buf_;
    }

    void SetCookie(void (*cookie)()) { cookie_ = cookie; }
    // 增加长度
    void Add(int length) { cur_ += length; }
    // 返回缓冲区数据
    const char* data() const { return buf_; }
    // 获取当前缓冲区已使用长度
    int len() const { return static_cast<int>(cur_ - buf_); }
    // 尾部减去当前位置
    int writablebytes() const { return static_cast<int>(end() - cur_); } 
    char* current() const { return cur_; }
    // 获取缓冲区尾部
    const char* end() const { return buf_ + sizeof(buf_); }

   private:
    void (*cookie_)();
    char buf_[SIZE];
    char* cur_;
};

// LogStream 重载了一系列 operator<< 操作符，将不同格式数据转化为字符串，并存入 LogStream::buffer_。
class LogStream {
   public:
    typedef FixedBuffer<kSmallSize> Buffer;
    typedef LogStream Self;

    LogStream() {}
    ~LogStream() {}

    Buffer& buffer() { return buffer_; }

    // 将整数格式化为字符串
    // 没有使用snprintf 因为下面函数效率更高
    // Efficient Integer to String Conversions, by Matthew Wilson.
    template <typename T>
    void FormatInteger(T num) {
        if (buffer_.writablebytes() >= kNum2MaxStringLength) {
            char* buf = buffer_.current();
            char* now = buf;
            const char* zero = digits + 9;
            bool negative = num < 0;

            do {
                int remainder = static_cast<int>(num % 10);
                *(now++) = zero[remainder];
                num /= 10;
            } while (num != 0);

            if (negative) *(now++) = '-';
            *now = '\0';
            std::reverse(buf, now); // 将从 buf 到 now 的字符序列反转
            buffer_.Add(static_cast<int>(now - buf));
        }
    }

    Self& operator<<(short num) { return (*this) << static_cast<int>(num); }

    Self& operator<<(unsigned short num) {
        return (*this) << static_cast<unsigned int>(num);
    }

    Self& operator<<(int num) {
        FormatInteger(num);
        return *this;
    }

    Self& operator<<(unsigned int num) {
        FormatInteger(num);
        return *this;
    }

    Self& operator<<(long num) {
        FormatInteger(num);
        return *this;
    }

    Self& operator<<(unsigned long num) {
        FormatInteger(num);
        return *this;
    }

    Self& operator<<(long long num) {
        FormatInteger(num);
        return *this;
    }

    Self& operator<<(unsigned long long num) {
        FormatInteger(num);
        return *this;
    }

    Self& operator<<(const float& num) {
        return (*this) << static_cast<double>(num);
    }

    Self& operator<<(const double& num) {
        char buf[32];
        // %g 是一个通用的格式化选项，它根据数值的大小自动选择 %f 或 %e
        int len = snprintf(buf, sizeof(buf), "%g", num);
        buffer_.Append(buf, len);
        return *this;
    }

    Self& operator<<(bool boolean) { return (*this) << (boolean ? 1 : 0); }

    Self& operator<<(char chr) {
        buffer_.Append(&chr, 1);
        return *this;
    }

    Self& operator<<(const void* data) {
        return (*this) << static_cast<const char*>(data);
    }

    Self& operator<<(const char* data) {
        buffer_.Append(data, static_cast<int>(strlen(data)));
        return *this;
    }

    Self& operator<<(const GeneralTemplate& source) {
        buffer_.Append(source.data_, source.len_);
        return *this;
    }

    Self& operator<<(const std::string& str) {
        buffer_.Append(str.data(), static_cast<int>(str.size()));
        return *this;
    }

   private:
    Buffer buffer_;
};

}  // namespace tiny_muduo

#endif
