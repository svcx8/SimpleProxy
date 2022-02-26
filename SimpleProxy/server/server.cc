#include "server.hh"

#include "misc/logger.hh"

absl::Status Server::Start(int Port) {
    server_socket_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (server_socket_ == SOCKET_ERROR) {
        return absl::InternalError(strerror(errno));
    }
    LOG("[Server::Start] Socket: %d Port: %d", (int)server_socket_, Port);
    sockaddr_in ServerAddr = {};
    ServerAddr.sin_family = AF_INET;
    // INADDR_LOOPBACK = 127.0.0.1; INADDR_ANY = 0.0.0.0;
    ServerAddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    ServerAddr.sin_port = htons(Port);

    if (bind(server_socket_, (sockaddr*)&ServerAddr, sizeof(ServerAddr)) == SOCKET_ERROR) {
        return absl::InternalError(strerror(errno));
    }
    if (listen(server_socket_, 5) == SOCKET_ERROR) {
        return absl::InternalError(strerror(errno));
    }

    return absl::OkStatus();
    // No longer need server socket
    // LOG("Close Server Socket");
    // close(server_socket_);
}