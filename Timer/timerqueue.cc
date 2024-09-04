#include "timerqueue.h"

#include <assert.h>
#include <sys/timerfd.h>

#include <cstring>

#include "channel.h"

using namespace tiny_muduo;

/**
 * linux2.6.25 版本新增了timerfd这个供用户程序使用的定时接口，
 * 这个接口基于文件描述符，当超时事件发生时，该文件描述符就变为可读。
 * 这种特性可以使我们在写服务器程序时，很方便的便把定时事件变成和其他I/O事件一样的处理方式，
 * 当时间到期后，就会触发读事件。我们调用响应的回调函数即可。
 */

// CLOCK_MONOTONIC绝对时间，获取的时间为系统最近一次重启到现在的时间，更改系统时间对其没影响

/**
 * 插入定时器流程

    EventLoop调用方法，加入一个定时器事件，会向里传入定时器回调函数，超时时间和间隔时间（为0.0则为一次性定时器），
    addTimer方法根据这些属性构造新的定时器。

    定时器队列内部插入此定时器，并判断这个定时器的超时时间是否比先前的都早。如果是最早触发的，
    就会调用resetTimerfd方法重新设置timerfd_的触发时间。内部会根据超时时间和现在时间计算出新的超时时间。

 */


TimerQueue::TimerQueue(EventLoop* loop)
    : loop_(loop),
      timerfd_(::timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC)),
      channel_(new Channel(loop_, timerfd_)) {
  channel_->SetReadCallback(std::bind(&TimerQueue::HandleRead, this));
  channel_->EnableReading();
}

TimerQueue::~TimerQueue() {
  channel_->DisableAll();
  loop_->Remove(channel_.get());
  close(timerfd_);

  for (const auto& timerpair : timers_) {
    // 删除所有定时器
    delete timerpair.second;
  } 
}

// 添加定时器
void TimerQueue::AddTimer(Timestamp timestamp, BasicFunc&& cb, double interval) {
  Timer* timer(new Timer(timestamp, std::move(cb), interval));
  loop_->RunOneFunc(std::bind(&TimerQueue::AddTimerInLoop, this, timer));
}

// 重置timerfd
void TimerQueue::ResetTimer(Timer* timer) {
  // itimerspec: POSIX.1b structure for timer start values and intervals
  struct itimerspec new_;
  struct itimerspec old_;

  memset(&new_, '\0', sizeof(new_));
  memset(&old_, '\0', sizeof(old_));

  // 剩下的时间
  int64_t micro_seconds_dif = timer->expiration().microseconds() - Timestamp::Now().microseconds();
  if (micro_seconds_dif < 100) {
    micro_seconds_dif = 100; // 如果差值小于100微秒，则将其设置为100微秒。这是为了避免定时器设置的时间过短而无法准确触发。
  }
 
  new_.it_value.tv_sec = static_cast<time_t>(
      micro_seconds_dif / kMicrosecond2Second);
  new_.it_value.tv_nsec = static_cast<long>(
      (micro_seconds_dif % kMicrosecond2Second) * 1000); // 将差值的纳秒部分（微秒的小数部分）
  int ret = ::timerfd_settime(timerfd_, 0, &new_, &old_);
  /**
   * 我们使用此函数启动或停止定时器。
    fd：timerfd_create()返回的文件描述符
    flags：0表示相对定时器，TFD_TIMER_ABSTIME表示绝对定时器
    new_value：设置超时事件，设置为0则表示停止定时器
    old_value：原来的超时时间，不使用可以置为NULL
   */
  assert(ret != -1);
  (void) ret;
}