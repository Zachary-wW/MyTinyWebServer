#include "timer.h"

#include <utility>

using namespace tiny_muduo;

// 一次性定时器interval为0
Timer::Timer(Timestamp expiration__, BasicFunc&& cb, double interval = 0.0)
    : expiration_(expiration__),
      callback_(std::move(cb)),
      interval_(interval),
      repeat_(interval > 0.0) {}

void Timer::Restart(Timestamp now) {
    // 如果是重复定时事件，则继续添加定时事件，得到新事件到期事件 如果是一次性的返回空对象
    // expiration_ = Timestamp::AddTime(now, interval_);
    if (repeat_) {
        expiration_ = Timestamp::AddTime(now, interval_);
    } else {
        expiration_ = Timestamp();
    }
}