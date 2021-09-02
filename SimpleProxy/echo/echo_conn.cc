#include "echo_conn.hh"

#include <dispatcher/ibusiness_event.hh>

#if defined WIN32
constexpr long flags = FD_READ | FD_CLOSE | FD_WRITE;
#endif

#if defined __unix__
constexpr long flags = EPOLLIN;
#endif

void EchoConn::OnReadable(SOCKET Socket) {
    char buffer[256];
    int RecvLen = recv(Socket, buffer, 256, 0);
    if (RecvLen > 0) {
        LOG("[EchoConn] OnReadable: %.*s", RecvLen, buffer);
        send(Socket, buffer, RecvLen, 0);
    }
#ifdef __unix__
    else if (RecvLen == 0) {
        OnCloseable(Socket);
    }
#endif
    else if (RecvLen <= 0) {
        throw NetEx();
    }
}

void EchoConn::OnWritable(SOCKET Socket) {
    LOG("[EchoConn] OnWritable");
}

void EchoConn::OnCloseable(SOCKET Socket) {
    LOG("[EchoConn] OnCloseable");
    Poller->RemoveCloseSocket(Socket);
}