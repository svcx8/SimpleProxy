#include "proxy_server.hh"

#include "proxy_socket.hh"
#include <dispatcher/epoller.hh>

#if defined __unix__
constexpr long flags = EPOLLIN;
#endif

void ProxyServer::OnReadable(SOCKET s) {
    SOCKET NewSocket = accept(s, nullptr, nullptr);
    if (NewSocket == SOCKET_ERROR) {
        throw NetEx();
    }

    auto pair = std::make_unique<SocketPair>();
    pair->this_side_ = NewSocket;
    pair->authentified_ = 0;
    auto ptr = pair.get();
    ProxySocket::GetInstance().AddPair(std::move(pair));

    if (ProxySocket::GetConnPoller(ptr)->AddSocket(NewSocket, flags) == -1) {
        LOG("[EchoServer] Failed to add event");
        CloseSocket(NewSocket);
        throw NetEx();
    }
    LOG("[ProxyServer] OnReadable: %d", NewSocket);
}