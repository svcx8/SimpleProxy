#include "proxy_server.hh"

#include "dispatcher/epoller.hh"
#include "misc/logger.hh"
#include "proxy_socket.hh"

#include <absl/memory/memory.h>

constexpr long flags = EPOLLIN;

absl::Status ProxyServer::OnReadable(int s) {
    int new_socket = accept(s, nullptr, nullptr);
    if (new_socket == SOCKET_ERROR) {
        return absl::InternalError(strerror(errno));
    }

    auto pair = absl::make_unique<SocketPair>();
    pair->this_side_ = new_socket;
    pair->authentified_ = 0;
    auto ptr = pair.get();
    ProxySocket::GetInstance().AddPair(std::move(pair));

    auto result = ProxySocket::GetConnPoller(ptr)->AddSocket(new_socket, flags);
    if (!result.ok()) {
        LOG("[ProxyServer] Failed to add event");
        close(new_socket);
        return result;
    }
    LOG("\n[ProxyServer] OnReadable: %d", new_socket);

    return absl::OkStatus();
}