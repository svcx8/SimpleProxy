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

    SocketPairManager::AddPair(client.sin_port, new_socket);
    LOG("\n[ProxyServer] OnReadable: %d", new_socket);
    return;
}