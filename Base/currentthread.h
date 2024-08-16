#ifndef TINY_MUDUO_CURRENTTHREAD_H_
#define TINY_MUDUO_CURRENTTHREAD_H_

#include <stdint.h>

namespace CurrentThread {

/*
__thread是一个关键字，被这个关键字修饰的全局变量会具备一个属性，
那就是该变量在每一个线程内都会有一个独立的实体。因为一般的全局变量都是
被同一个进程中的多个线程所共享，但是这里我们不希望这样
*/

extern __thread int t_cachedTid; // 保存tid缓冲，避免多次系统调用
extern __thread char t_formattedTid[32];
extern __thread int t_formattedTidLength;

void CacheTid();

inline int tid() {
  if (__builtin_expect(t_cachedTid == 0, 0)) {
    CacheTid();
  }
  return t_cachedTid;
}

inline const char* tid2string() { return t_formattedTid; } 
inline int tidstringlength() { return t_formattedTidLength; }

} // namespace CurrentThread

#endif