#ifndef SERVER_HEADER
#define SERVER_HEADER

#include "misc/net.hh"

#include <absl/status/status.h>

class Server {
public:
    static int server_socket_;
    static int Send(void* buffer, std::size_t size) {
        return send(server_socket_, (char*)buffer, size, 0);
    }
    static absl::Status Start(int Port);
};

#endif // server.hh