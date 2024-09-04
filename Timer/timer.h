#ifndef TINY_MUDUO_TIMER_H_
#define TINY_MUDUO_TIMER_H_

#include <functional>

#include "noncopyable.h"
#include "timestamp.h"

namespace tiny_muduo {

class Timer : public NonCopyAble {
   public:
    typedef std::function<void()> BasicFunc;

    Timer(Timestamp timestamp, BasicFunc&& cb, double interval);

    void Restart(Timestamp now);

    void Run() const { callback_(); }

    Timestamp expiration() const { return expiration_; }
    bool repeat() const { return repeat_; }

   private:
    Timestamp expiration_; // 下一次超时时间
    BasicFunc callback_; // 到时间后的回调函数
    double interval_; // 超时时间间隔
    bool repeat_; // false代表一次性定时器，true就是可以重复的
};

}  // namespace tiny_muduo

#endif