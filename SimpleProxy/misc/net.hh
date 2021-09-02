#ifndef NET_HEADER
#define NET_HEADER

#include "myexception.hh"

// https://docs.microsoft.com/en-us/windows/win32/winsock/complete-server-code
// https://docs.microsoft.com/en-us/windows/win32/winsock/complete-client-code
// _WIN32(WSAStartup())-->socket()——>bind()——>listen()——>accpet()——>recv() or send()——>closesocket()->WSACleanup()

#ifdef WIN32
#include <winsock2.h>
#include <ws2tcpip.h>

#include <windows.h>
#define CloseSocket closesocket
#endif
#include "logger.hh"
#ifdef __unix__
#include <arpa/inet.h>
#include <cstdint>
#include <errno.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

#define SOCKET int
#define FALSE 0
#define SOCKET_ERROR (-1)
#define SD_RECEIVE 0x00
#define SD_SEND 0x01
#define SD_BOTH 0x02
#define CloseSocket close
#endif

class NetException : public BaseException {
public:
#if defined WIN32
    char buffer_[128];
#endif
    NetException(const char* func, const char* file, int line) : BaseException(func, file, line) {
#if defined WIN32
        FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                       NULL, WSAGetLastError(),
                       MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US),
                       (LPSTR)&Buffer, 128, NULL);
        result_ = buffer_;
#endif

#if defined __unix__
        result_ = strerror(errno);
#endif
    }
};

#define NetEx() NetException(__FUNCTION__, __FILE__, __LINE__)

#if defined WIN32
inline void MyWinSocketSetup() {
    WSADATA wsaData;
    // Initialize Winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        throw NetEx();
    }
}
#endif

#endif // net.hpp