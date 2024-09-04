#ifndef TINY_MUDUO_TIMERQUEUE_H_
#define TINY_MUDUO_TIMERQUEUE_H_

#include <sys/timerfd.h>
#include <unistd.h>

#include <functional>
#include <memory>
#include <set>
#include <utility>
#include <vector>

#include "logging.h"
#include "noncopyable.h"
#include "timer.h"
#include "timestamp.h"

namespace tiny_muduo {

class EventLoop;
class Channel;

// 该类作为管理定时器的结构
class TimerQueue : public NonCopyAble {
   public:
    typedef std::function<void()> BasicFunc;

    TimerQueue(EventLoop* loop);
    ~TimerQueue();

    void ReadTimerFd() {
        uint64_t read_byte;
        ssize_t readn = ::read(timerfd_, &read_byte, sizeof(read_byte));
        if (readn != sizeof(read_byte)) {
            LOG_ERROR << "TimerQueue::ReadTimerFd read_size < 0";
        }
    }

    // 定时器读事件触发器
    void HandleRead() {
        ReadTimerFd();
        Timestamp expiration_time(Timestamp::Now());

        active_timers_.clear();
        // 在timers_容器中找到第一个其时间戳大于当前时间的定时器。
        // UINTPTR_MAX是一个很大的数，这里用来确保找到的是第一个时间戳大于当前时间的定时器。
        auto end = timers_.lower_bound(
            TimerPair(Timestamp::Now(), reinterpret_cast<Timer*>(UINTPTR_MAX)));
        // 超过当前时间的定时器是超时的 要处理掉
        active_timers_.insert(active_timers_.end(), timers_.begin(), end);
        timers_.erase(timers_.begin(), end);
        // 调用回调函数
        for (const auto& timerpair : active_timers_) {
            timerpair.second->Run();
        }

        ResetTimers();
    }

    // 对过期的定时器进行重置
    void ResetTimers() {
        for (auto& timerpair : active_timers_) {
            if ((timerpair.second)->repeat()) {
                // 可以重复使用的进行restart
                auto timer = timerpair.second;
                timer->Restart(Timestamp::Now());
                Insert(timer);
            } else {
                delete timerpair.second;
            }
        }
        // 如果重新插入了定时器，需要继续重置timerfd
        if (!timers_.empty()) {
            ResetTimer(timers_.begin()->second);
        }
    }

    bool Insert(Timer* timer) {
        bool reset_instantly = false;
        // 插入的定时器过期时间比timers上面最小的还小 需要重置timerfd
        if (timers_.empty() || timer->expiration() < timers_.begin()->first) {
            reset_instantly = true;
        }

        // 插入新节点
        timers_.emplace(std::move(TimerPair(timer->expiration(), timer)));

        return reset_instantly;
    }

    // 插入定时器 如果取代了最小的触发时间，需要重置
    // 在当前loop中运行 保证线程安全
    void AddTimerInLoop(Timer* timer) {
        bool reset_instantly = Insert(timer);
        if (reset_instantly) {
            ResetTimer(timer);
        }
    }

    void ResetTimer(Timer* timer);
    void AddTimer(Timestamp timestamp, BasicFunc&& cb, double interval);

   private:
    typedef std::pair<Timestamp, Timer*> TimerPair;
    typedef std::set<TimerPair> TimersSet;
    typedef std::vector<TimerPair> ActiveTimers;

    EventLoop* loop_;
    int timerfd_;
    std::unique_ptr<Channel> channel_;

    TimersSet timers_;
    ActiveTimers active_timers_;
};

}  // namespace tiny_muduo

#endif