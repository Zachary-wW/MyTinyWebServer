#ifndef TINY_MUDUO_BUFFER_H_
#define TINY_MUDUO_BUFFER_H_

#include <vector>
#include <algorithm>
#include <string>
#include <assert.h>

using std::string;

namespace tiny_muduo {

// kPrePendIndex 即为 初始的 prependable bytes
static const int kPrePendIndex = 8; // 使用static限制变量仅在本文件中访问

class Buffer {
 public:
  Buffer() : buffer_(1024), read_index_(kPrePendIndex), write_index_(kPrePendIndex) {
  }
  ~Buffer() {}

  int ReadFd(int fd);
  
  char* begin() { return &*buffer_.begin(); }; // 先解引用再获取地址 获取buffer第一个元素的地址
  const char* begin() const { return &*buffer_.begin(); };

  char* beginread() { return begin() + read_index_; } 
  const char* beginread() const { return begin() + read_index_; }

  char* beginwrite() { return begin() + write_index_; }
  const char* beginwrite() const { return begin() + write_index_; }

  void Append(const char* message, int len) {
    MakeSureEnoughStorage(len);
    std::copy(message, message + len, beginwrite());
    write_index_ += len;
  }

  void Append(const string& message) {
    Append(message.data(), message.size()); // str.data(): Return const pointer to contents.
  }

  void Retrieve(int len) {
    if (len + read_index_ != write_index_) { //  如果说数据可供读取 长度限制在RetrieveAsString函数
      read_index_ = read_index_ + len; // 更新read index
    } else { // 刚好读取完写入的数据 将write index归位
      write_index_ = kPrePendIndex; 
      read_index_ = write_index_;
    }
  }

  void RetrieveUntilIndex(const char* index) {
    assert(beginwrite() >= index);
    read_index_ += index - beginread();
  }

  void RetrieveAll() { // 和retrieve中正好读到write index的情况一样
    write_index_ = kPrePendIndex;
    read_index_ = write_index_;
  }

  string RetrieveAsString(int len) {
    assert(read_index_ + len <= write_index_);
    string ret = std::move(PeekAsString(len));
    Retrieve(len); // 本质上更新index
    return ret;
  }

  string RetrieveAllAsString() {
    string ret = std::move(PeekAllAsString());
    RetrieveAll();
    return ret;
  }

  const char* Peek() const {
    return beginread();
  }

  char* Peek() {
    return beginread();
  }

  string PeekAsString(int len) {
    return string(beginread(), beginread() + len);
  }

  string PeekAllAsString() { // 获取从read index 到 write index之间的数据
    return string(beginread(), beginwrite()); 
  }
  
  int readablebytes() const { return write_index_ - read_index_; }
  int writablebytes() const { return buffer_.capacity() - write_index_; } 
  int prependablebytes() const { return read_index_; }

  void MakeSureEnoughStorage(int len) {
    if (writablebytes() >= len) return; // 容量够大不用管
    if (writablebytes() + prependablebytes() >= kPrePendIndex + len) { // 前面有空闲空间，前移
      std::copy(beginread(), beginwrite(), begin() + kPrePendIndex);
      write_index_ = kPrePendIndex + readablebytes();
      read_index_ = kPrePendIndex;
    } else { // 不够扩容
      buffer_.resize(buffer_.size() + len);
    }
  }

 private:
  std::vector<char> buffer_;
  int read_index_;
  int write_index_;
};

}
#endif
