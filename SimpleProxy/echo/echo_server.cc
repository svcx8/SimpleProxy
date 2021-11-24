#include "echo_server.hh"

#include "echo_conn.hh"

#include "dispatcher/epoller.hh"

constexpr long flags = EPOLLIN;

absl::Status EchoServer::OnReadable(int Socket) {
    int new_socket = accept(Socket, nullptr, nullptr);
    if (new_socket == SOCKET_ERROR) {
        return absl::InternalError(strerror(errno));
    }
    LOG("new_socket: %d", new_socket);

    for (auto& P : EPoller::reserved_list_) {
        auto result = P->AddSocket(new_socket, flags);
        if (!result.ok()) {
            LOG("[EchoServer] Failed to add event");
            close(new_socket);
            return absl::InternalError(strerror(errno));
        }
        // Only try to add to the first poller.
        break;
    }

    return absl::OkStatus();
}