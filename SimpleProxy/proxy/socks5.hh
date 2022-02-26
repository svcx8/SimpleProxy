#ifndef SOCKS5_HEADER
#define SOCKS5_HEADER

#include <memory>
#include <mutex>

#include <absl/status/status.h>

#include "limiter/token_bucket.hh"
#include "misc/net.hh"

class Socks5Header {
public:
    unsigned char version_;
    unsigned char auth_method_count_;
    unsigned char* methods_;
    Socks5Header(void* buffer);
    bool Check();
};

class Socks5Command {
public:
    typedef union {
        struct sockaddr sockaddr_;
        struct sockaddr_in sockaddr_in_;
        struct sockaddr_in6 sockaddr_in6_;
    } ngx_sockaddr_t;
    unsigned char version_ = 0;
    unsigned char command_ = 0;
    unsigned char reserved_ = 0;
    unsigned char address_type_ = 0;
    sockaddr* sock_addr_ = nullptr;
    size_t sock_addr_len_ = 0;

    unsigned char* head_buffer_;
    Socks5Command(void* buffer);
    ~Socks5Command() {
        if (sock_addr_)
            delete sock_addr_;
    }
    absl::Status Check();

    static const char reply_success[];
    static const char reply_failure[];
};

#endif // socks5.hh