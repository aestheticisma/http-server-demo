
#include "buffer.h"

Buffer::Buffer(int initBuffSize): buffer_(initBuffSize), readPos_(0), writePos_(0) {}

size_t Buffer::WritableBytes() const {
    return buffer_.size() - writePos_;
}

size_t Buffer::ReadableBytes() const {
    return writePos_ - readPos_;
}

size_t Buffer::PrependableBytes() const {
    return readPos_;
}

const char* Buffer::Peek() const {
    return BeginPtr_() + readPos_;
}

void Buffer::EnsureWritable(size_t len) {
    if (WritableBytes() < len) {
        MakeSpace_(len);
    }
    assert(WritableBytes() >= len);
}

void Buffer::HasWritten(size_t len) {
    assert(len <= WritableBytes());
    writePos_ += len;
}

void Buffer::Retrieve(size_t len) {
    assert(len <= ReadableBytes());
    readPos_ += len;
}

void Buffer::RetrieveUtil(const char* end) {
    assert(Peek() <= end);
    Retrieve(end - Peek());
}

void Buffer::RetrieveAll() {
    bzero(&buffer_[0], buffer_.size());
    readPos_ = 0;
    writePos_ = 0;
}

std::string Buffer::RetrieveAllToStr() {
    std::string str(Peek(), ReadableBytes());
    RetrieveAll();
    return str;
}

char* Buffer::BeginWrite() {
    return BeginPtr_() + writePos_;
}

const char* Buffer::BeginWriteConst() const {
    return BeginPtr_() + writePos_;
}

void Buffer::Append(const char* str, size_t len) {
    assert(str);
    EnsureWritable(len);
    std::copy(str, str+len, BeginWrite());
    HasWritten(len);
}

void Buffer::Append(const void* data, size_t len) {
    assert(data);
    Append(static_cast<const char*>(data), len);
}

void Buffer::Append(const std::string& str) {
    Append(str.data(), str.length());
}

void Buffer::Append(const Buffer& buff) {
    Append(buff.Peek(), buff.ReadableBytes());
}

ssize_t Buffer::ReadFd(int fd, int* saveErrno) {
    char buff[65535];
    struct iovec iov[2];
    const size_t writeSize = WritableBytes();
    // 分散读 保证数据全部读完;
    // iov指向两块内存缓冲区，第一块为Buffer，第二块为buff[65535]
    // 使得数据一次性读完
    iov[0].iov_base = BeginPtr_() + writePos_;
    iov[0].iov_len = writeSize;
    iov[1].iov_base = buff;
    iov[1].iov_len = sizeof(buff);

    const ssize_t len = readv(fd, iov, 2);
    if (len < 0) {
        *saveErrno = errno;
    }
    // 读入的数据大小 小于 buffer可写空间大小
    else if (static_cast<size_t>(len) <= writeSize) {
        writePos_ += len;
    }
    // 第二块缓冲区也读入了数据，把其数据添加进Buffer
    else {
        writePos_ = buffer_.size();
        Append(buff, len - writeSize);
    }
    return len;
}

ssize_t Buffer::WriteFd(int fd, int* saveErrno) {
    size_t readSize = ReadableBytes();
    ssize_t len = write(fd, Peek(), readSize);
    if (len < 0) {
        *saveErrno = errno;
        return len;
    }
    readPos_ += len;
    return len;
}

char * Buffer::BeginPtr_() {
    return &*buffer_.begin();
}

const char* Buffer::BeginPtr_() const {
    return &*buffer_.begin();
}

void Buffer::MakeSpace_(size_t len) {
    // 可写空间 + 已读完空间 < len, resize
    if (WritableBytes() + PrependableBytes() < len) {
        buffer_.resize(writePos_ + len + 1);
    }
    // 覆盖Buffer中已读完的空间（整体前移）
    else {
        size_t readable = ReadableBytes();
        std::copy(BeginPtr_() + readPos_, BeginPtr_() + writePos_, BeginPtr_());
        readPos_ = 0;
        writePos_ = readPos_ + readable;
        assert(readable == ReadableBytes());
    }
}