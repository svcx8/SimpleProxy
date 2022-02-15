#include "proxy_conn.hh"

#include <absl/status/status.h>
#include <linux/eventpoll.h>
#include <memory>
#include <string_view>

#include "memory_buffer.hh"
#include "misc/configuration.hh"
#include "misc/logger.hh"
#include "proxy_client.hh"
#include "proxy_socket.hh"

/*
    ConnSocket->recv(); // Recv from client.
    ConnSocket->send(); // Send to client.
*/

void ErrorHandler(SocketPair* pair) {
    if (pair->authentified_ == 1)
        send(pair->this_side_, Socks5Command::reply_failure, 10, 0);
    ProxySocket::GetInstance().RemovePair(pair->this_side_);
}

absl::Status ProxyConn::CheckSocks5Handshake(SocketPair* pair) {
    char head_buf[256];
    int recv_len = recv(pair->this_side_, head_buf, 256, 0);
    if (recv_len == SOCKET_ERROR) {
        ErrorHandler(pair);
        return absl::InternalError(strerror(errno));
    }

    if (pair->authentified_ == 0) {
        // First handshake, check valid socks5 header.
        Socks5Header head((unsigned char*)head_buf);
        if (!head.Check()) {
            ErrorHandler(pair);
            return absl::InternalError("Unsupported version or methods.");
        }
        short int return_ver = 0x0005;
        if (send(pair->this_side_, (char*)&return_ver, 2, 0) != 2) {
            ErrorHandler(pair);
            return absl::InternalError(strerror(errno));
        }
    }

    else if (pair->authentified_ == 1) {
        // Second handshake, recv command.
        Socks5Command command((unsigned char*)head_buf);
        auto result = command.Check();
        if (!result.ok()) {
            ErrorHandler(pair);
            return result;
        }

#ifndef NDEBUG
        if (command.address_type_ == 0x1) {
            char buffer[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &reinterpret_cast<sockaddr_in*>(command.sock_addr_)->sin_addr, buffer, sizeof(buffer));
            printf("\taddress4: %s:%d\n", buffer, htons(reinterpret_cast<sockaddr_in*>(command.sock_addr_)->sin_port));
            pair->other_side_ = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, IPPROTO_TCP);
        }

        else if (command.address_type_ == 0x4) {
            char buffer[INET6_ADDRSTRLEN];
            inet_ntop(AF_INET6, &reinterpret_cast<sockaddr_in6*>(command.sock_addr_)->sin6_addr, buffer, sizeof(buffer));
            printf("\taddress6: %s:%d\n", buffer, htons(reinterpret_cast<sockaddr_in6*>(command.sock_addr_)->sin6_port));
        }
#endif

        pair->other_side_ = socket(reinterpret_cast<sockaddr_in*>(command.sock_addr_)->sin_family, SOCK_STREAM | SOCK_NONBLOCK, IPPROTO_TCP);

        if (pair->other_side_ == SOCKET_ERROR) {
            ErrorHandler(pair);
            return absl::InternalError(strerror(errno));
        }

        // Connect to server.
        if (connect(pair->other_side_, command.sock_addr_, command.sock_addr_len_) == SOCKET_ERROR) {
            if (errno != EINPROGRESS) {
                ErrorHandler(pair);
                return absl::InternalError(strerror(errno));
            }
        }

        result = ProxySocket::GetClientPoller(pair)->AddSocket(pair->other_side_, EPOLLOUT);
        if (!result.ok()) {
            ErrorHandler(pair);
            return absl::InternalError("[ProxyConn] Failed to add event.");
        }

        LOG("[ProxyConn] Server: %d | Type: %02X", pair->other_side_, command.address_type_);
    }

    pair->authentified_++;
    return absl::OkStatus();
}

absl::Status ProxyConn::OnReadable(int s) {
    auto pair = ProxySocket::GetInstance().GetPointer(s);
    if (!pair) {
        return absl::FailedPreconditionError("[" LINE_FILE "] Socket not found in ProxySocket::socket_list_.");
    }

    if (pair->authentified_ < 2) {
        return CheckSocks5Handshake(pair);
    }

    else {
        // Receiving from Client, e.g. curl https://baidu.com
        if (auto buffer_pool = MemoryBuffer::GetPool(s)) {
            auto result = buffer_pool->Receive(s);

            if (result.ok()) {
                result.Update(buffer_pool->Send(pair->other_side_));
            }

            if (result.code() == absl::StatusCode::kResourceExhausted) {
                result = absl::OkStatus();
                // The buffer of proxy server cannot receive more data, add to poller.
                // absl::Status ProxyClient::OnWritable(int s) will send remaining data.
                result.Update(ProxySocket::GetClientPoller(pair)->ModSocket(pair->other_side_, EPOLLOUT));
                result.Update(poller_->RemoveSocket(pair->this_side_));
            }

            return result;
        } else {
            return absl::FailedPreconditionError("[" LINE_FILE "] GetPool return null");
        }
    }
}

absl::Status ProxyConn::OnWritable(int s) {
    auto pair = ProxySocket::GetInstance().GetPointer(s);
    if (auto buffer_pool = MemoryBuffer::GetPool(pair->other_side_)) {
        auto result = buffer_pool->Send(pair->this_side_);
        if (!result.ok()) {
            return result;
        }
        // If the buffer is empty, we can remove EPOLLOUT event.
        if (buffer_pool->Usage() == 0 /*&& pair->token_bucket_->blocked_ == false*/) {
            result.Update(poller_->ModSocket(pair->this_side_, EPOLLIN));
            result.Update(ProxySocket::GetClientPoller(pair)->AddSocket(pair->other_side_, EPOLLIN));
        }
        return result;
    } else {
        return absl::FailedPreconditionError("[" LINE_FILE "] GetPool return null");
    }
}