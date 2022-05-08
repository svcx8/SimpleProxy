#include "proxy_conn.hh"

#include <memory>
#include <mutex>
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

void ErrorHandler(ConnSocket* pair) {
    if (pair) {
        if (pair->authentified_ == 1) {
            int res = send(pair->socket_, Socks5Command::reply_failure, 10, 0);
            if (res == SOCKET_ERROR) {
                ERROR("[ErrorHandler] %s", strerror(errno));
            }
        }

        pair->Close();
    }
}

#define ERRORLOG() ERROR("[%s] [#L%d] [t#%d] [%d] %s", __FUNCTION__, __LINE__, gettid(), pair->socket_, strerror(errno));

absl::Status CheckSocks5Handshake(ConnSocket* pair) {
    char head_buf[128]{};
    int recv_len = recv(pair->socket_, head_buf, 128, 0);
    if (recv_len <= 0) {
        ERRORLOG();
        ErrorHandler(pair);
        return absl::InternalError(strerror(errno));
    }

    if (pair->authentified_ == 0) {
        // First handshake, check valid socks5 header.
        Socks5Header head(head_buf);
        if (!head.Check()) {
            ERROR("[%s] [#L%d] [t#%d] [%d] check head: Unsupported version or methods.", __FUNCTION__, __LINE__, gettid(), pair->socket_);
            ErrorHandler(pair);
            return absl::InternalError("Unsupported version or methods.");
        }
        short int return_ver = 0x0005;
        if (send(pair->socket_, (char*)&return_ver, 2, 0) != 2) {
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
            ERROR("[%s] [#L%d] [t#%d] [%d] %s", __FUNCTION__, __LINE__, gettid(), pair->socket_, result.ToString().c_str());
            ErrorHandler(pair);
            return result;
        }

#ifndef NDEBUG
        if (command.address_type_ == 0x1) {
            char buffer[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &reinterpret_cast<sockaddr_in*>(command.sock_addr_)->sin_addr, buffer, sizeof(buffer));
            LOG("\taddress4: %s:%d\n", buffer, htons(reinterpret_cast<sockaddr_in*>(command.sock_addr_)->sin_port));
        }

        else if (command.address_type_ == 0x4) {
            char buffer[INET6_ADDRSTRLEN];
            inet_ntop(AF_INET6, &reinterpret_cast<sockaddr_in6*>(command.sock_addr_)->sin6_addr, buffer, sizeof(buffer));
            LOG("\taddress6: %s:%d\n", buffer, htons(reinterpret_cast<sockaddr_in6*>(command.sock_addr_)->sin6_port));
        }
#endif

        if (command.command_ == 0x1) {
            // TCP CONNECT
            int new_socket = socket(reinterpret_cast<sockaddr_in*>(command.sock_addr_)->sin_family,
                                    SOCK_STREAM | SOCK_NONBLOCK, IPPROTO_TCP);

            if (new_socket == SOCKET_ERROR) {
                ERRORLOG();
                ErrorHandler(pair);
                return absl::InternalError(strerror(errno));
            }

            // Connect to server.
            if (connect(new_socket, command.sock_addr_, command.sock_addr_len_) == SOCKET_ERROR) {
                if (errno != EINPROGRESS) {
                    ERRORLOG();
                    ErrorHandler(pair);
                    return absl::InternalError(strerror(errno));
                }
            }

            pair->authentified_++;
            ClientSocket* client_socket = new ClientSocket(new_socket);
            pair->other_side_ = client_socket;
            client_socket->other_side_ = pair;
            result.Update(client_socket->poller_->AddSocket(new_socket,
                                                                   reinterpret_cast<uintptr_t>(client_socket),
                                                                   EPOLLOUT | EPOLLRDHUP));
            if (!result.ok()) {
                delete client_socket;
                ERROR("[%s] [#L%d] [t#%d] [%d] %s", __FUNCTION__, __LINE__, gettid(), pair->socket_, result.ToString().c_str());
                ErrorHandler(pair);
                return absl::InternalError("[ProxyConn] Failed to add event.");
            }
        }

        else if (command.command_ == 0x3) {
            // UDP ASSOCIATE
            if (auto res = UDPHandler::ReplyHandshake(pair); res.ok()) {
                pair->authentified_ = 4;
            } else {
                ERROR("[%s] [#L%d] [t#%d] %s", __FUNCTION__, __LINE__, gettid(), res.ToString().c_str());
                ErrorHandler(pair);
                return absl::InternalError("[ProxyConn] Failed to reply handshake.");
            }
        }
    } // if (pair->authentified_ == 1)
    return absl::OkStatus();
}

void ProxyConn::OnReadable(uintptr_t s) {
    auto pair = reinterpret_cast<ConnSocket*>(s);

    if (pair->is_closed_) {
        ERROR("[%s] [#L%d] [t#%d] closed: %d", __FUNCTION__, __LINE__, gettid(), pair->socket_);
        pair->Close();
        return;
    }

    if (pair->authentified_ < 2) {
        auto result = CheckSocks5Handshake(pair);
        if (!result.ok()) {
            // TODO
        }
    }

    else if (pair->authentified_ < 4) {
        // Receiving from Client, e.g. curl https://baidu.com
        auto recv_from_client_pool = pair->buffer_;
        auto result = recv_from_client_pool->ReceiveFromClient(pair->socket_);

        if (result.ok()) {
            result.Update(recv_from_client_pool->ProxyConn_SendToServer(pair->other_side_->socket_));
        }

        if (!result.ok()) {
            if (result.code() != absl::StatusCode::kResourceExhausted) {
                pair->Close();
                return;
            }

            else {
                result = absl::OkStatus();
                // The buffer of proxy server cannot receive more data, add to poller.
                // absl::Status ProxyClient::OnWritable(int s) will send remaining data.
                result.Update(pair->poller_->RemoveSocket(pair->socket_));
                result.Update(pair->other_side_->poller_->ModSocket(pair->other_side_->socket_,
                                                                            reinterpret_cast<uintptr_t>(pair->other_side_),
                                                                            EPOLLOUT | EPOLLRDHUP));

                if (!result.ok()) {
                    pair->Close();
                }
            }
        }
    }

    else if (pair->authentified_ == 4) {
        LOG("[ProxyConn] [t#%d] [%d] Remove UDP Associate.", gettid(), pair->other_side_->socket_);
        pair->poller_->RemoveSocket(pair->socket_).IgnoreError();
        close(pair->socket_);

        ProxyPoller* udp_poller = reinterpret_cast<ProxyPoller*>(EPoller::reserved_list_[EPoller::reserved_list_.size() - 1]);
        udp_poller->RemoveSocket(pair->other_side_->socket_).IgnoreError();
        close(pair->other_side_->socket_);
    }

    else {
        ERROR("[%s] [#L%d] [t#%d] [%d] %s", __FUNCTION__, __LINE__, gettid(), pair->socket_, "Invalid authentified.");
        pair->Close();
    }
}

void ProxyConn::OnWritable(uintptr_t s) {
    auto pair = reinterpret_cast<ConnSocket*>(s);

    if (pair->is_closed_) {
        ERROR("[%s] [#L%d] [t#%d] closed: %d", __FUNCTION__, __LINE__, gettid(), pair->socket_);
        pair->Close();
        return;
    }

    auto recv_from_server_pool = pair->other_side_->buffer_;
    auto result = recv_from_server_pool->ProxyConn_SendToClient(pair->socket_);
    if (!result.ok()) {
        pair->Close();
        return;
    }
    // If the buffer is empty, we can remove EPOLLOUT event.
    if (recv_from_server_pool->Usage() == 0) {
        result.Update(pair->poller_->ModSocket(pair->socket_, s, EPOLLIN | EPOLLRDHUP));
        result.Update(pair->other_side_->poller_->AddSocket(pair->other_side_->socket_,
                                                                    reinterpret_cast<uintptr_t>(pair->other_side_),
                                                                    EPOLLIN | EPOLLRDHUP));

        if (!result.ok()) {
            LOG("[ProxyConn] [t#%d] [%d] %s", gettid(), pair->socket_, result.ToString().c_str());
            pair->Close();
        }
    }
}

void ProxyConn::OnError(uintptr_t s) {
    auto pair = reinterpret_cast<ConnSocket*>(s);
    LOG("[%s] [#L%d] [t#%d] [%d] %s", __FUNCTION__, __LINE__, gettid(), pair->socket_, "OnError.");
}

void ProxyConn::OnClose(uintptr_t s) {
    auto pair = reinterpret_cast<ConnSocket*>(s);
    LOG("[%s] [#L%d] [t#%d] [%d] %s", __FUNCTION__, __LINE__, gettid(), pair->socket_, "OnClose.");
    pair->Close();
}