#include "proxy_conn.hh"

#include <memory>
#include <string_view>

#include "dispatcher/epoller.hh"
#include "memory_buffer.hh"
#include "misc/configuration.hh"
#include "misc/logger.hh"
#include "misc/net.hh"
#include "proxy/poller.hh"
#include "proxy/socket_pair.hh"
#include "proxy/udp_associate.hh"
#include "proxy_client.hh"
#include "socks5.hh"

#include <absl/status/status.h>

void ErrorHandler(SocketPair* pair) {
    if (pair->authentified_ == 1) {
        int res = send(pair->conn_socket_, Socks5Command::reply_failure, 10, 0);
        if (res == SOCKET_ERROR) {
            ERROR("[ErrorHandler] %s", strerror(errno));
        }
    }

    SocketPairManager::RemovePair(pair);
}

#define ERRORLOG() ERROR("[%s] [#L%d] [t#%d] [%d] %s", __FUNCTION__, __LINE__, gettid(), pair->conn_socket_, strerror(errno))

absl::Status CheckSocks5Handshake(SocketPair* pair) {
    char head_buf[128]{};
    int recv_len = recv(pair->conn_socket_, head_buf, 128, 0);
    if (recv_len == SOCKET_ERROR) {
        ErrorHandler(pair);
        ERRORLOG();
        return absl::InternalError(strerror(errno));
    }

    if (pair->authentified_ == 0) {
        // First handshake, check valid socks5 header.
        Socks5Header head(head_buf);
        if (!head.Check()) {
            ErrorHandler(pair);
            ERROR("[%s] [#L%d] [t#%d] [%d] check head: Unsupported version or methods.", __FUNCTION__, __LINE__, gettid(), pair->conn_socket_);
            return absl::InternalError("Unsupported version or methods.");
        }
        short int return_ver = 0x0005;
        if (send(pair->conn_socket_, (char*)&return_ver, 2, 0) != 2) {
            ERRORLOG();
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
            ERROR("[%s] [#L%d] [t#%d] [%d] %s", __FUNCTION__, __LINE__, gettid(), pair->conn_socket_, result.ToString().c_str());
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

        if (command.command_ == 0x1) {
            // TCP CONNECT
            pair->client_socket_ = socket(reinterpret_cast<sockaddr_in*>(command.sock_addr_)->sin_family,
                                          SOCK_STREAM | SOCK_NONBLOCK, IPPROTO_TCP);

            if (pair->client_socket_ == SOCKET_ERROR) {
                ERRORLOG();
                ErrorHandler(pair);
                return absl::InternalError(strerror(errno));
            }

            // Connect to server.
            if (connect(pair->client_socket_, command.sock_addr_, command.sock_addr_len_) == SOCKET_ERROR) {
                if (errno != EINPROGRESS) {
                    ERRORLOG();
                    ErrorHandler(pair);
                    return absl::InternalError(strerror(errno));
                }
            }

            pair->authentified_++;
            result.Update(SocketPairManager::AcquireClientPoller(pair)->AddSocket(pair->client_socket_,
                                                                                  reinterpret_cast<uintptr_t>(pair),
                                                                                  EPOLLOUT));
            if (!result.ok()) {
                ErrorHandler(pair);
                ERROR("[%s] [#L%d] [t#%d] [%d] %s", __FUNCTION__, __LINE__, gettid(), pair->conn_socket_, result.ToString().c_str());
                return absl::InternalError("[ProxyConn] Failed to add event.");
            }
        }

        else if (command.command_ == 0x3) {
            // UDP ASSOCIATE
            if (auto res = UDPHandler::ReplyHandshake(pair); res.ok()) {
                pair->authentified_ = 4;
            } else {
                ErrorHandler(pair);
                ERROR("[%s] [#L%d] [t#%d] %s", __FUNCTION__, __LINE__, gettid(), res.ToString().c_str());
                return absl::InternalError("[ProxyConn] Failed to reply handshake.");
            }
        }
    } // if (pair->authentified_ == 1)
    return absl::OkStatus();
}

void ProxyConn::OnReadable(uintptr_t s) {
    auto pair = reinterpret_cast<SocketPair*>(s);

    if (pair->authentified_ < 2) {
        auto result = CheckSocks5Handshake(pair);
        if (!result.ok()) {
            // TODO
        }
    }

    else if (pair->authentified_ < 4) {
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

    else {
        if (pair->authentified_ == 4) {
            LOG("[ProxyConn] [t#%d] [%d] Remove UDP Associate.", gettid(), pair->client_socket_);
            pair->conn_poller_->RemoveSocket(pair->conn_socket_).IgnoreError();
            close(pair->conn_socket_);

            ProxyPoller* udp_poller = reinterpret_cast<ProxyPoller*>(EPoller::reserved_list_[EPoller::reserved_list_.size() - 1]);
            udp_poller->RemoveSocket(pair->client_socket_).IgnoreError();
            close(pair->client_socket_);
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