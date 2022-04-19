#ifndef LOGGER_HEADER
#define LOGGER_HEADER

#include <cstdio>

#define STRINGIZE2(x) #x
#define STRINGIZE(x) STRINGIZE2(x)
#define LINE_STRING STRINGIZE(__LINE__)

#define LINE_FILE "Line " LINE_STRING " of File " __FILE__

#ifdef NDEBUG
#define LOG(str, ...)
#else
#define LOG(str, ...)                         \
    do {                                      \
        std::printf(str "\n", ##__VA_ARGS__); \
    } while (0)
#endif // #ifdef NDEBUG

#define ERROR(str, ...)                       \
    do {                                      \
        std::printf(str "\n", ##__VA_ARGS__); \
    } while (0)

#define INFO(str, ...)                        \
    do {                                      \
        std::printf(str "\n", ##__VA_ARGS__); \
    } while (0)

#endif