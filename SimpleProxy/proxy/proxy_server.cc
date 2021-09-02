#include "proxy_server.hh"

#include <dispatcher/epoller.hh>

#if defined __unix__
constexpr long flags = EPOLLIN;
#endif

void ProxyServer::OnReadable(SOCKET Socket) {
    SOCKET NewSocket = accept(Socket, nullptr, nullptr);
    if (NewSocket == SOCKET_ERROR) {
        throw NetEx();
    }

    if (EPoller::reserved_list_[0]->AddSocket(NewSocket, flags) == -1) {
        LOG("[EchoServer] Failed to add event");
        CloseSocket(NewSocket);
        throw NetEx();
    }
    LOG("[ProxyServer] OnReadable: %d", NewSocket);
}