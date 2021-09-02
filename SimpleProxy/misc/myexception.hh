#ifndef MYEXCEPTION_HEADER
#define MYEXCEPTION_HEADER

class BaseException {
public:
    BaseException(const char* func, const char* file, int line)
        : function_(func), file_(file), line_(line), result_(nullptr){};
    BaseException(const char* func, const char* file, int line, const char* result)
        : function_(func), file_(file), line_(line), result_(result){};
    int line_;
    const char* function_;
    const char* file_;
    const char* result_;
};

#define MyEx(result) BaseException(__FUNCTION__, __FILE__, __LINE__, result)

#endif // myexception.hh