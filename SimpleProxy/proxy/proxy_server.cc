#include "proxy_server.hh"

#include <absl/memory/memory.h>

#include "dispatcher/epoller.hh"
#include "misc/logger.hh"
#include "misc/net.hh"
#include "socket_pair.hh"

void ProxyServer::OnReadable(int s) {
    sockaddr_in client;
    socklen_t len = sizeof(client);
    int new_socket = accept(s, (sockaddr*)&client, &len);
    if (new_socket == SOCKET_ERROR) {
        ERROR("[ProxyServer] Failee to accept: %s", strerror(errno));
        return;
    }

    auto pair = std::make_shared<SocketPair>(client.sin_port);
    pair->this_side_ = new_socket;
    pair->authentified_ = 0;

    auto result = SocketPairManager::GetConnPoller(pair.get())->AddSocket(new_socket, EPOLLIN);
    SocketPairManager::AddPair(client.sin_port, std::move(pair)); // If ConnPoller::OnReadable() is called before this line, the socket pair will not found.
    if (!result.ok()) {
        LOG("[ProxyServer] Failed to add event");
        close(new_socket);
        SocketPairManager::RemovePair(pair.get());
        return;
    }
    LOG("\n[ProxyServer] OnReadable: %d", new_socket);
    return;
}