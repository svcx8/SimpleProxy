#ifndef SERVER_HEADER
#define SERVER_HEADER

#include <tuple>

#include "misc/net.hh"

#include <absl/status/statusor.h>

class Server {
public:
    static absl::StatusOr<std::tuple<int, int>> CreateTCPServer(int Port) {
        return Create(Port, SOCK_STREAM | O_NONBLOCK, IPPROTO_TCP);
    }
    static absl::StatusOr<std::tuple<int, int>> CreateUDPServer(int Port) {
        return Create(Port, SOCK_DGRAM | O_NONBLOCK, IPPROTO_UDP);
    }

private:
    static absl::StatusOr<std::tuple<int, int>> Create(int port, int type, int protocol);
};

#endif // server.hh