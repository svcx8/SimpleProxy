#include "echo_server.hh"

#include "echo_conn.hh"

#include <dispatcher/epoller.hh>

#if defined WIN32
constexpr long flags = FD_READ | FD_CLOSE | FD_WRITE;
#endif

#if defined __unix__
constexpr long flags = EPOLLIN;
#endif

void EchoServer::OnReadable(SOCKET Socket) {
    SOCKET NewSocket = accept(Socket, nullptr, nullptr);
    if (NewSocket == SOCKET_ERROR) {
        throw NetEx();
    }
    LOG("NewSocket: %d", NewSocket);

    for (auto& P : EPoller::ReservedList) {
        if (P->AddSocket(NewSocket, flags) == -1) {
            LOG("[EchoServer] Failed to add event");
            CloseSocket(NewSocket);
            throw NetEx();
        }
        break;
    }
}