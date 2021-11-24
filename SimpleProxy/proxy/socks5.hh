#ifndef SOCKS5_HEADER
#define SOCKS5_HEADER

#include "misc/net.hh"

#include <absl/status/status.h>

class SocketPair {
public:
    int poller_index = 0;
    int this_side_ = 0;
    int other_side_ = 0;
    int authentified_ = 0;
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
        struct sockaddr sockaddr;
        struct sockaddr_in sockaddr_in;
        struct sockaddr_in6 sockaddr_in6;
    } ngx_sockaddr_t;
    unsigned char version_;
    unsigned char command_;
    unsigned char reserved_;
    unsigned char address_type_;
    ngx_sockaddr_t address_struct_;

    unsigned char* head_buffer_;
    Socks5Command(unsigned char* buffer);
    absl::Status Check();

    static const char reply_success[];
    static const char reply_failure[];
};

#endif // socks5.hh