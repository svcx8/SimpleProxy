#include "proxy_conn.hh"

#include <memory>
#include <string_view>

#include "dispatcher/epoller.hh"
#include "memory_buffer.hh"
#include "misc/configuration.hh"
#include "misc/logger.hh"
#include "misc/net.hh"
#include "proxy/socket_pair.hh"
#include "proxy_client.hh"
#include "socks5.hh"

#include <absl/status/status.h>

void ErrorHandler(SocketPair* pair) {
    if (pair->authentified_ == 1) {
        int res = send(pair->conn_socket_, Socks5Command::reply_failure, 10, 0);
        if (res == SOCKET_ERROR) {
            LOG("[ErrorHandler] %s", strerror(errno));
        }
    }

    SocketPairManager::RemovePair(pair);
}

absl::Status CheckSocks5Handshake(SocketPair* pair) {
    char head_buf[256];
    int recv_len = recv(pair->conn_socket_, head_buf, 256, 0);
    if (recv_len == SOCKET_ERROR) {
        ErrorHandler(pair);
        return absl::InternalError(strerror(errno));
    }

    if (pair->authentified_ == 0) {
        // First handshake, check valid socks5 header.
        Socks5Header head(head_buf);
        if (!head.Check()) {
            ErrorHandler(pair);
            return absl::InternalError("Unsupported version or methods.");
        }
        short int return_ver = 0x0005;
        if (send(pair->conn_socket_, (char*)&return_ver, 2, 0) != 2) {
            ErrorHandler(pair);
            return absl::InternalError(strerror(errno));
        }
        pair->authentified_++;
    }

    else if (pair->authentified_ == 1) {
        // Second handshake, recv command.
        Socks5Command command(head_buf);
        auto result = command.Check();
        if (!result.ok()) {
            LOG("[CheckSocks5Handshake] [%d] check command: %.*s", pair->conn_socket_, (int)result.message().size(), result.message().data());
            ErrorHandler(pair);
            return result;
        }

#ifndef NDEBUG
        if (command.address_type_ == 0x1) {
            char buffer[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &reinterpret_cast<sockaddr_in*>(command.sock_addr_)->sin_addr, buffer, sizeof(buffer));
            LOG("\taddress4: %s:%d\n", buffer, htons(reinterpret_cast<sockaddr_in*>(command.sock_addr_)->sin_port));
            pair->client_socket_ = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, IPPROTO_TCP);
        }

        else if (command.address_type_ == 0x4) {
            char buffer[INET6_ADDRSTRLEN];
            inet_ntop(AF_INET6, &reinterpret_cast<sockaddr_in6*>(command.sock_addr_)->sin6_addr, buffer, sizeof(buffer));
            LOG("\taddress6: %s:%d\n", buffer, htons(reinterpret_cast<sockaddr_in6*>(command.sock_addr_)->sin6_port));
        }
#endif

        pair->client_socket_ = socket(reinterpret_cast<sockaddr_in*>(command.sock_addr_)->sin_family,
                                      SOCK_STREAM | SOCK_NONBLOCK, IPPROTO_TCP);

        if (pair->client_socket_ == SOCKET_ERROR) {
            ErrorHandler(pair);
            return absl::InternalError(strerror(errno));
        }

        // Connect to server.
        if (connect(pair->client_socket_, command.sock_addr_, command.sock_addr_len_) == SOCKET_ERROR) {
            if (errno != EINPROGRESS) {
                ErrorHandler(pair);
                return absl::InternalError(strerror(errno));
            }
        }

        pair->authentified_++;
        result = SocketPairManager::AcquireClientPoller(pair)->AddSocket(pair->client_socket_,
                                                                         reinterpret_cast<uintptr_t>(pair),
                                                                         EPOLLOUT);
        if (!result.ok()) {
            ErrorHandler(pair);
            return absl::InternalError("[ProxyConn] Failed to add event.");
        }
    }
    return absl::OkStatus();
}

void ProxyConn::OnReadable(uintptr_t s) {
    auto pair = reinterpret_cast<SocketPair*>(s);

    if (pair->authentified_ < 2) {
        auto result = CheckSocks5Handshake(pair);
        if (!result.ok()) {
            LOG("[ProxyConn] [%d] check handshake step %d : %.*s", pair->authentified_, pair->conn_socket_, (int)result.message().size(), result.message().data());
        }
    }

    else {
        // Receiving from Client, e.g. curl https://baidu.com
        if (auto recv_from_client_pool = MemoryBuffer::GetPool(pair->conn_socket_)) {
            auto result = recv_from_client_pool->ReceiveFromClient(pair->conn_socket_);

            if (result.ok()) {
                result.Update(recv_from_client_pool->ProxyConn_SendToServer(pair->client_socket_));
            }

            if (!result.ok()) {
                if (result.code() != absl::StatusCode::kResourceExhausted) {
                    SocketPairManager::RemovePair(pair);
                    return;
                }

                else {
                    result = absl::OkStatus();
                    // The buffer of proxy server cannot receive more data, add to poller.
                    // absl::Status ProxyClient::OnWritable(int s) will send remaining data.
                    result.Update(poller_->RemoveSocket(pair->conn_socket_));
                    result.Update(pair->client_poller_->ModSocket(pair->client_socket_, s, EPOLLOUT));

                    if (!result.ok()) {
                        // TODO
                    }
                }
            }
        }
    }
}

void ProxyConn::OnWritable(uintptr_t s) {
    auto pair = reinterpret_cast<SocketPair*>(s);

    if (auto recv_from_server_pool = MemoryBuffer::GetPool(pair->client_socket_)) {
        auto result = recv_from_server_pool->ProxyConn_SendToClient(pair->conn_socket_);
        if (!result.ok()) {
            SocketPairManager::RemovePair(pair);
            return;
        }
        // If the buffer is empty, we can remove EPOLLOUT event.
        if (recv_from_server_pool->Usage() == 0) {
            result.Update(pair->conn_poller_->ModSocket(pair->conn_socket_, s, EPOLLIN));
            result.Update(pair->client_poller_->AddSocket(pair->client_socket_, s, EPOLLIN));

            if (!result.ok()) {
                // TODO
            }
        }
    }
}