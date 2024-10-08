#include "condition.h"

#include <errno.h>
#include <stdint.h>
#include <sys/time.h>

using namespace tiny_muduo;

bool Condition::WaitForFewSeconds(double seconds) {
    struct timespec time;
    // 调用 clock_gettime 函数获取当前的单调时间（不受系统时间调整影响的时间）
    clock_gettime(CLOCK_MONOTONIC, &time);

    const int64_t kNanoSecondsPerSecond = 1000000000;
    int64_t nanoseconds = static_cast<int64_t>(seconds * kNanoSecondsPerSecond);

    time.tv_sec += static_cast<time_t>((time.tv_nsec + nanoseconds) /
                                       kNanoSecondsPerSecond);
    time.tv_nsec =
        static_cast<long>((time.tv_nsec + nanoseconds) % kNanoSecondsPerSecond);

    return ETIMEDOUT ==
           pthread_cond_timedwait(&cond_, mutex_.pthreadmutex(), &time);
}