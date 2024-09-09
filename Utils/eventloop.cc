#include "eventloop.h"

#include <pthread.h>
#include <signal.h>
#include <sys/eventfd.h>
#include <unistd.h>

#include <utility>

#include "channel.h"
#include "mutex.h"

using namespace tiny_muduo;

namespace {

class IgnoreSigPipe {
   public:
    IgnoreSigPipe() { ::signal(SIGPIPE, SIG_IGN); }
};

IgnoreSigPipe initObj;

}  // namespace

/*
EFD_CLOEXEC(2.6.27～) :
eventfd()返回一个文件描述符，如果该进程被fork的时候，这个文件描述符也会被复制过去，
这个时候就会有多个描述符指向同一个eventfd对象，如果设置了这个标志，则子进程在执行exec的时候，
会自动清除掉父进程的这个文件描述符。

EFD_NONBLOCK(2.6.27～):文件描述符会被设置为O_NONBLOCK,如果没有设置这个标志位，
read操作的时候将会阻塞直到计数器中有值，如果设置了这个这个标志位，计数器没有值得时候也会立刻返回-1。
 */

EventLoop::EventLoop()
    : running_(false),
      tid_(CurrentThread::tid()),
      epoller_(new Epoller()),
      // eventfd内核空间维护一个无符号64位整型计数器
      wakeup_fd_(::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC)),
      wakeup_channel_(new Channel(this, wakeup_fd_)),
      timer_queue_(new TimerQueue(this)),
      calling_functors_(false) {
    // 注册读事件处理函数
    wakeup_channel_->SetReadCallback(std::bind(&EventLoop::HandleRead, this));
    // 注册读事件，一旦监听到可读就调用HandleRead
    wakeup_channel_->EnableReading();
}

EventLoop::~EventLoop() {
    if (running_) running_ = false;
    wakeup_channel_->DisableAll();  // 清除所有感兴趣事件
    Remove(wakeup_channel_.get());  // 移除channel
    close(wakeup_fd_);
}

void EventLoop::Loop() {
    assert(IsInThreadLoop());
    running_ = true;
    while (running_) {
        active_channels_.clear();          // 清空元素 但是保留内存
        epoller_->Poll(active_channels_);  // 得到监听结果
        for (const auto& channel : active_channels_) {
            channel->HandleEvent();  // 对于每一个就绪的channel调用解决方法
        }
        DoToDoList();
    }
    running_ = false;
}

// 从 wakeup_fd_中读取数据，以便在事件循环中唤醒或处理待执行的任务
void EventLoop::HandleRead() {
    uint64_t read_one_byte = 1;
    ssize_t read_size =
        ::read(wakeup_fd_, &read_one_byte, sizeof(read_one_byte));
    (void)read_size;  // 防止未使用变量警告
    // 确保实际读取的字节数等于请求读取的字节数
    assert(read_size == sizeof(read_one_byte));
    return;
}

// 将func放入队列中，唤醒eventloop
/**
 执行当前EventLoop事件循环需要处理的回调操作 对于线程数 >=2 的情况 IO线程
 mainloop(mainReactor) 主要工作：
 (1) accept接收连接 => 将accept返回的connfd打包为Channel =>
 TcpServer::newConnection通过轮询将TcpConnection对象分配给subloop处理
 (2) mainloop调用queueInLoop将回调加入subloop（该回调需要subloop执行
 但subloop还在poller_->poll处阻塞） queueInLoop通过wakeup将subloop唤醒


 我们希望这个func能在某个EventLoop对象所绑定的线程上运行，但是由于当前CPU执行的线程不是
 我们期待的这个EventLoop线程，我们只能把这个可调用对象先存在这个EventLoop对象的数组成员
 todolist中
 **/

void EventLoop::QueueOneFunc(BasicFunc func) {
    {
        MutexLockGuard lock(mutex_);
        to_do_list_.emplace_back(std::move(func));
    }  // 加入队列时使用mutex保证线程安全

    // 使用calling_functors为了防止执行func时有新加入的func造成阻塞
    if (!IsInThreadLoop() || calling_functors_) {
        // TODO 封装函数wakeup
        // 为了避免没有数据可以读取，随便写入一个8字节数据唤醒eventloop
        uint64_t write_one_byte = 1;
        ssize_t write_size =
            ::write(wakeup_fd_, &write_one_byte, sizeof(write_one_byte));
        (void)write_size;
        assert(write_size == sizeof(write_one_byte));
    }
}

void EventLoop::RunOneFunc(BasicFunc func) {
    // 如果当前调用RunOneFunc的线程正好是EventLoop的运行线程，则直接执行此回调函数
    if (IsInThreadLoop()) {
        func();
    } else {
        // 在非当前EventLoop线程中执行，就需要唤醒EventLoop所在线程执行func
        QueueOneFunc(std::move(func));
    }
}

void EventLoop::DoToDoList() {
    // 如果没有该局部变量，todolist将会被锁住导致其他线程无法添加func造成服务器时间延迟
    ToDoList functors;
    calling_functors_ = true;
    {
        MutexLockGuard lock(mutex_);
        // 注意swap之后iterator还是指向原来的元素
        functors.swap(to_do_list_);
    }

    for (const auto& func : functors) {
        func();
    }
    calling_functors_ = false;
}
