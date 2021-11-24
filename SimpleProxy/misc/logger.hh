#ifndef LOGGER_HEADER
#define LOGGER_HEADER

#include <cstdio>
#include <iostream>
using std::cout;
using std::endl;

#define DEBUG2(x, y) "Line " #y " of File " x
#define DEBUG1(x, y) DEBUG2(x, y)
#define LINE_FILE DEBUG1(__FILE__, __LINE__)

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

#endif