#include "echo_conn.hh"

constexpr long flags = EPOLLIN;

absl::Status EchoConn::OnReadable(int Socket) {
    char buffer[256];
    int RecvLen = recv(Socket, buffer, 256, 0);
    if (RecvLen > 0) {
        LOG("[EchoConn] OnReadable: %.*s", RecvLen, buffer);
        send(Socket, buffer, RecvLen, 0);
    }

    else if (RecvLen == 0) {
        OnCloseable(Socket);
    }

    else if (RecvLen < 0) {
        return absl::InternalError(strerror(errno));
    }

    return absl::OkStatus();
}

absl::Status EchoConn::OnWritable(int Socket) {
    LOG("[EchoConn] OnWritable");
    return absl::OkStatus();
}

absl::Status EchoConn::OnCloseable(int Socket) {
    LOG("[EchoConn] OnCloseable");
    close(Socket);
    return absl::OkStatus();
}