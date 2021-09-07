#ifndef LOGGER_HEADER
#define LOGGER_HEADER

#include <cstdio>
#include <iostream>
using std::cout;
using std::endl;

#ifdef NDEBUG
#define LOG(str, ...)
#else
#define LOG(str, ...)                         \
    do {                                      \
        std::printf(str "\n", ##__VA_ARGS__); \
    } while (0)
#endif // #ifdef NDEBUG

#define ERROR(str, ...)                         \
    do {                                      \
        std::printf(str "\n", ##__VA_ARGS__); \
    } while (0)

#endif