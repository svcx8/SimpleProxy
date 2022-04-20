#include "server.hh"

#include "misc/logger.hh"
#include <linux/in.h>
#include <tuple>

absl::StatusOr<int> Server::Create(int port, int type, int protocol) {
    Server* server = new Server();
    int server_socket = socket(AF_INET, type, protocol);
    if (server_socket == SOCKET_ERROR) {
        return absl::InternalError(strerror(errno));
    }

    LOG("[Server::Create] socket: %d port: %d protocol: %d", server_socket, port, protocol);

    sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    // INADDR_LOOPBACK = 127.0.0.1; INADDR_ANY = 0.0.0.0;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(port);

    if (bind(server_socket, (sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        return absl::InternalError(strerror(errno));
    }

    if (protocol != IPPROTO_UDP) {
        if (listen(server_socket, 5) == SOCKET_ERROR) {
            return absl::InternalError(strerror(errno));
        }
    }
    return server_socket;
}