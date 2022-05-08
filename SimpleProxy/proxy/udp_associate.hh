#ifndef UDP_ASSOCIATE_HEADER
#define UDP_ASSOCIATE_HEADER

#include "dispatcher/ibusiness_event.hh"

#include <condition_variable>
#include <cstring>
#include <mutex>
#include <unordered_map>

#include "proxy/socket_pair.hh"
#include "proxy/socks5.hh"
#include "server/server.hh"

#include <absl/status/status.h>

class UDPHandler {
public:
    char socks5_handshake_ok_[22];
    // 0x05, 0x00, 0x00, 0x01, { IP ADDRESS 16 bytes }, { PORT 2 bytes }
    int socks5_handshake_ok_len;

    static void Forward(ClientSocket* pair);

    static bool Init4() {
        return Init(2); // AF_INET
    }
    static bool Init6() {
        return Init(10); // AF_INET6
    }
    static absl::Status ReplyHandshake(ConnSocket* pair);

    static uint64_t server_ip_;
    static unsigned char prebuild_socks5_handshake_ok_[22];
    static int prebuild_socks5_handshake_ok_len_;

private:
    std::condition_variable exit_cond_;
    std::mutex mutex_;
    static bool Init(int af);
};

#endif // udp_associate.hh