#include "server.hh"

void Server::Start(int Port) {
    server_socket_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (server_socket_ == SOCKET_ERROR) {
        throw NetEx();
    }
    LOG("[Server::Start] Socket: %d Port: %d", (int)server_socket_, Port);
    sockaddr_in ServerAddr = {};
    ServerAddr.sin_family = AF_INET;
    // INADDR_LOOPBACK = 127.0.0.1; INADDR_ANY = 0.0.0.0;
    ServerAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    ServerAddr.sin_port = htons(Port);

    if (bind(server_socket_, (sockaddr*)&ServerAddr, sizeof(ServerAddr)) == SOCKET_ERROR) {
        throw NetEx();
    }
    if (listen(server_socket_, 5) == SOCKET_ERROR) {
        throw NetEx();
    }
    // No longer need server socket
    // LOG("Close Server Socket");
    // CloseSocket(server_socket_);
}