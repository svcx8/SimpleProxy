#include "proxy_server.hh"

#include "dispatcher/epoller.hh"
#include "misc/logger.hh"
#include "misc/net.hh"
#include "socket_pair.hh"

void ProxyServer::OnReadable(uintptr_t s) {
    sockaddr_in client;
    socklen_t len = sizeof(client);
    int new_socket = accept(s, (sockaddr*)&client, &len);
    if (new_socket == SOCKET_ERROR) {
        ERROR("[ProxyServer] Failed to accept: %s", strerror(errno));
        return;
    }

    ConnSocket* conn_socket = new ConnSocket(new_socket);
    auto result = conn_socket->poller_->AddSocket(new_socket,
                                                       reinterpret_cast<uintptr_t>(conn_socket),
                                                       EPOLLIN | EPOLLRDHUP);
    if (!result.ok()) {
        delete conn_socket;
        LOG("[ProxyServer] [%d] Failed to add event", new_socket);
        return;
    }

    LOG("\n[ProxyServer] OnReadable: %d", new_socket);
    return;
}