#ifndef SOCKS5_HEADER
#define SOCKS5_HEADER

class SocketPair {
public:
    int this_side_;
    int other_side_;
    int authentified_;
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
    unsigned char version_;
    unsigned char command_;
    unsigned char reserved_;
    unsigned char address_type_;
    int remote_address_;
    short int port_;
    Socks5Command(unsigned char* buffer);
    bool Check();
    static const char reply_success[];
    static const char reply_failure[];
};

#endif // socks5.hh