#ifndef SERVER_HEADER
#define SERVER_HEADER

#include "misc/net.hh"
#include "misc/singleton.hh"

#include <absl/status/status.h>

class Server : public Singleton<Server> {
public:
    int server_socket_;
    int Send(void* buffer, std::size_t size) {
        return send(server_socket_, (char*)buffer, size, 0);
    }
    absl::Status Start(int Port);
};

#endif // server.hh