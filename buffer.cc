#include "buffer.h"

#include <sys/uio.h>

using namespace tiny_muduo;

int Buffer::ReadFd(int fd) {
  char extrabuf[65536] = {0};
  struct iovec iv[2]; // Structure for scatter/gather I/O.
  const int writable = writablebytes();
  iv[0].iov_base = beginwrite();
  iv[0].iov_len = writable;
  iv[1].iov_base = extrabuf;
  iv[1].iov_len = sizeof(extrabuf);
  
  const int iovcnt = (writable < static_cast<int>(sizeof(extrabuf)) ? 2 : 1);
  int readn = readv(fd, iv, iovcnt); // 将fd中的数据写到iv中 返回读取的字节数

  if (readn < 0) {
    assert(readn >= 0);
  }
  else if (readn <= writable) {
    write_index_ += readn;
  } else {
    write_index_ = buffer_.size();
    Append(extrabuf, readn - writable);
  }

  return readn;
}
