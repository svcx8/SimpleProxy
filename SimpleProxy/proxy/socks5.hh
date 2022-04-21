#ifndef SOCKS5_HEADER
#define SOCKS5_HEADER

#include <memory>
#include <mutex>

#include <absl/status/status.h>

#include "limiter/token_bucket.hh"
#include "misc/net.hh"

/*
    +----+----------+----------+
    |VER | NMETHODS | METHODS  |
    +----+----------+----------+
    | 1  |    1     | 1 to 255 |
    +----+----------+----------+
*/
class Socks5Header {
public:
    unsigned char version_;
    unsigned char auth_method_count_;
    unsigned char* methods_;
    Socks5Header(void* buffer);
    bool Check();
};

/*
    +----+-----+-------+------+----------+----------+
    |VER | CMD |  RSV  | ATYP | DST.ADDR | DST.PORT |
    +----+-----+-------+------+----------+----------+
    | 1  |  1  | X'00' |  1   | Variable |    2     |
    +----+-----+-------+------+----------+----------+

    o  VER    protocol version: X'05'
    o  CMD
        o  CONNECT X'01'
        o  BIND X'02'
        o  UDP ASSOCIATE X'03'
    o  RSV    RESERVED
    o  ATYP   address type of following address
        o  IP V4 address: X'01'
        o  DOMAINNAME: X'03'
        o  IP V6 address: X'04'
    o  DST.ADDR       desired destination address
    o  DST.PORT desired destination port in network octet
        order
*/

class Socks5Command {
public:
    int index_ = 0;
    unsigned char* head_buffer_;

    unsigned char version_ = 0;
    unsigned char command_ = 0;
    unsigned char reserved_ = 0;
    unsigned char address_type_ = 0;
    sockaddr* sock_addr_ = nullptr;
    size_t sock_addr_len_ = 0;

    Socks5Command(void* buffer);
    ~Socks5Command() {
        if (address_type_ != 3)
            delete sock_addr_;
    }
    absl::Status Check();

    static const char reply_success[];
    static const char reply_failure[];
};

/*
      +----+------+------+----------+----------+----------+
      |RSV | FRAG | ATYP | DST.ADDR | DST.PORT |   DATA   |
      +----+------+------+----------+----------+----------+
      | 2  |  1   |  1   | Variable |    2     | Variable |
      +----+------+------+----------+----------+----------+

    The fields in the UDP request header are:

      o  RSV  Reserved X'0000'
      o  FRAG    Current fragment number
      o  ATYP    address type of following addresses:
         o  IP V4 address: X'01' 4 bytes
         o  DOMAINNAME: X'03'
         o  IP V6 address: X'04' 16 bytes
      o  DST.ADDR       desired destination address
      o  DST.PORT       desired destination port
      o  DATA     user data
*/
class UDPPacket {
public:
    Socks5Command command_;
    unsigned char* user_data_ = nullptr;

    UDPPacket(unsigned char* buf) : command_(buf){};
    absl::Status Check();
};

#endif // socks5.hh