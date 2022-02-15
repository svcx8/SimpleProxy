#ifndef SOCKS5_HEADER
#define SOCKS5_HEADER

#include <memory>

#include <absl/status/status.h>

#include "limiter/token_bucket.hh"
#include "misc/net.hh"

class SocketPair {
public:
    int poller_index = 0;
    int this_side_ = 0;
    int other_side_ = 0;
    int authentified_ = 0;
    // std::unique_ptr<TokenBucket> token_bucket_;
};

class Socks5Header {
public:
    unsigned char version_;
    unsigned char auth_method_count_;
    unsigned char* methods_;
    Socks5Header(unsigned char* buffer);
    bool Check();
};

class Socks5Command {
public:
    typedef union {
        struct sockaddr sockaddr_;
        struct sockaddr_in sockaddr_in_;
        struct sockaddr_in6 sockaddr_in6_;
    } ngx_sockaddr_t;
    unsigned char version_;
    unsigned char command_;
    unsigned char reserved_;
    unsigned char address_type_;
    sockaddr* sock_addr_;
    size_t sock_addr_len_;

    unsigned char* head_buffer_;
    Socks5Command(unsigned char* buffer);
    ~Socks5Command() {
        if (sock_addr_)
            delete sock_addr_;
    }
    absl::Status Check();

    static const char reply_success[];
    static const char reply_failure[];
};

#endif // socks5.hh