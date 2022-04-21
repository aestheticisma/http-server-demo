
#include "log.h"

Log::Log() {
    lineCount_ = 0;
    isAsync_ = false;
    writeThread_ = nullptr;
    deque_ = nullptr;
    toDay_ = 0;
    fp_ = nullptr;
}

Log::~Log() {
    if (writeThread_ && writeThread_->joinable()) {
        while (!deque_->empty()) deque_->flush();
        deque_->Close();
        writeThread_->join();
    }
    if (fp_) {
        std::lock_guard<std::mutex> locker(mtx_);
        flush();
        fclose(fp_);
    }
    isOpen_ = false;
}

void Log::init(int level=1, const char* path, const char* suffix, int maxQueueSize) {
    isOpen_ = true;
    level_ = true;
    if (maxQueueSize > 0) {
        isAsync_ = true;
        if (!deque_) {
            std::unique_ptr<BlockDeque<std::string>> newDeque(new BlockDeque<std::string>(maxQueueSize));
            deque_ = move(newDeque);

            std::unique_ptr<std::thread> NewThread(new std::thread(FlushLogThread));
            writeThread_ = move(NewThread);
        }
    }
    else isAsync_ = false;

    lineCount_ = 0;

    time_t timer = time(nullptr);
    struct tm *sysTime = localtime(&timer);
    struct tm t = *sysTime;
    path_ = path;
    suffix_ = suffix;
    char fileName[LOG_NAME_LEN] = {0};
    snprintf(fileName, LOG_NAME_LEN - 1, "%s/%04d_%02d_%02d%s", 
            path_, t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, suffix_);
    toDay_ = t.tm_mday;

    {
        std::lock_guard<std::mutex> locker(mtx_);
        buff_.RetrieveAll();
        if (fp_) {
            flush();
            fclose(fp_);
        }

        fp_ = fopen(fileName, "a");
        if (fp_ == nullptr) {
            mkdir(path_, 0777);
            fp_ = fopen(fileName, "a");
        }
        assert(fp_ != nullptr);
    }
}

void Log::write(int level, const char* format, ...) {
    struct timeval now = {0, 0};
    gettimeofday(&now, nullptr);
    time_t tSec = now.tv_sec;
    struct tm *sysTime = localtime(&tSec);
    struct tm t = *sysTime;
    va_list vaList;

    // 打开新文件
    if (toDay_ != t.tm_mday || (lineCount_ && (lineCount_ % MAX_LINES == 0))) {
        std::unique_lock<std::mutex> locker(mtx_);

        locker.unlock();

        char newFile[LOG_NAME_LEN];
        char tail[36] = {0};
        snprintf(tail, 36, "%04d_%02d_%02d", t.tm_year+1900, t.tm_mon+1, t.tm_mday);

        if (toDay_ != t.tm_mday) {
            snprintf(newFile, LOG_NAME_LEN-72, "%s/%s%s", path_, tail, suffix_);
            toDay_ = t.tm_mday;
            lineCount_ = 0;
        }
        else snprintf(newFile, LOG_NAME_LEN-72, "%s/%s-%d%s", path_, tail, (lineCount_/MAX_LINES), suffix_);

        locker.lock();
        flush();
        fclose(fp_);
        fp_ = fopen(newFile, "a");
        assert(fp_ != nullptr);
    }
    // 继续写当前文件
    {
        std::unique_lock<std::mutex> locker(mtx_);
        ++lineCount_;
        char timeLines[128] = { 0 };
        snprintf(timeLines, 127, "%d-%02d-%02d %02d:%02d:%02d.%06ld ",
                    t.tm_year + 1900, t.tm_mon + 1, t.tm_mday,
                    t.tm_hour, t.tm_min, t.tm_sec, now.tv_usec);
        buff_.Append(timeLines, strlen(timeLines));
        // int n = snprintf(buff_.BeginWrite(), 128, "%d-%02d-%02d %02d:%02d:%02d.%06ld ",
        //             t.tm_year + 1900, t.tm_mon + 1, t.tm_mday,
        //             t.tm_hour, t.tm_min, t.tm_sec, now.tv_usec);
        // buff_.HasWritten(n);
        AppendLogLevelTitle_(level);

        va_start(vaList, format);
        char messages[128];
        vsnprintf(messages, 127, format, vaList);
        // int m = vsnprintf(buff_.BeginWrite(), buff_.WritableBytes(), format, vaList);
        va_end(vaList);

        // buff_.HasWritten(m);
        buff_.Append(messages, strlen(messages));
        buff_.Append("\n\0", 2);

        if (isAsync_ && deque_ && !deque_->full())
            deque_->push_back(buff_.RetrieveAllToStr());
        else
            fputs(buff_.Peek(), fp_);
        buff_.RetrieveAll();
    }
}

void Log::AppendLogLevelTitle_(int level) {
    switch (level) {
        case 0:
            buff_.Append("[debug]: ", 9);
            break;
        case 1:
            buff_.Append("[info] : ", 9);
            break;
        case 2:
            buff_.Append("[warn] : ", 9);
            break;
        case 3:
            buff_.Append("[error]: ", 9);
            break;
        default:
            buff_.Append("[info] : ", 9);
            break;
    }
}

void Log::AsyncWrite_() {
    std::string str = "";
    while (deque_->pop(str)) {
        std::lock_guard<std::mutex> locker(mtx_);
        fputs(str.c_str(), fp_);
    }
}

Log* Log::Instance() {
    static Log inst;
    return &inst;
}

void Log::FlushLogThread() {
    Log::Instance()->AsyncWrite_();
}

void Log::flush() {
    if (isAsync_) deque_->flush();
    fflush(fp_); // 清空文件缓冲区
}

int Log::GetLevel() {
    std::lock_guard<std::mutex> locker(mtx_);
    return level_;
}

void Log::SetLevel(int level) {
    std::lock_guard<std::mutex> locker(mtx_);
    level_ = level;
}
