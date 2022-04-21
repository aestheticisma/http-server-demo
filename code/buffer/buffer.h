
#ifndef BUFFER_H
#define BUFFER_H
#include <iostream>
#include <cstring>
#include <vector>
#include <atomic>
#include <assert.h>
#include <unistd.h> // write
#include <sys/uio.h> // iovec readv

class Buffer {
public:
    Buffer(int initBuffSize = 1024);
    ~Buffer() = default;

    size_t WritableBytes() const; // 当前Buffer可写字节数;
    size_t ReadableBytes() const; // 当前Buffer可读字节数;
    size_t PrependableBytes() const; // 当前Buffer已读字节数;

    const char* Peek() const; // 当前读指针位置;
    void EnsureWritable(size_t len); // 保证Buffer有空间可写，没有则resize buffer;
    void HasWritten(size_t len); // 移动写光标位置;

    void Retrieve(size_t len); // 移动读光标位置;
    void RetrieveUtil(const char* end); //移动读光标到 end 处;
    void RetrieveAll(); // 读当前Buffer全部可读值 (即清空Buffer)
    std::string RetrieveAllToStr(); // // 读当前Buffer全部可读值;

    // 返回可写的位置
    char* BeginWrite();
    const char* BeginWriteConst() const;

    void Append(const char* str, size_t len); // 向Buffer中添加数据（基方法）
    void Append(const void* data, size_t len);
    void Append(const std::string& str);
    void Append(const Buffer& buff);

    ssize_t ReadFd(int fd, int* Errno); // 从fd中读数据至Buffer中
    ssize_t WriteFd(int fd, int* Errno); // 将Buffer数据写入fd中

private:
    char* BeginPtr_();
    const char* BeginPtr_() const;
    void MakeSpace_(size_t len);

    std::vector<char> buffer_;
    std::atomic<size_t> readPos_;
    std::atomic<size_t> writePos_;
};

#endif // BUFFER_H