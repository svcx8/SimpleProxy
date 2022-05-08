#include "proxy_client.hh"

#include <thread>

#include "dispatcher/epoller.hh"
#include "memory_buffer.hh"
#include "misc/configuration.hh"
#include "misc/logger.hh"
#include "misc/net.hh"
#include "socket_pair.hh"
#include "socks5.hh"

#include <absl/status/status.h>

/*
    Level Triggered
    EPOLLIN Event

        Socket Receive Buffer           Is Empty            Low Level
        Socket Receive Buffer           Not Empty           High Level(Triggered)

    EPOLLOUT Event

        Socket Send Buffer              Is Full              Low Level
        Socket Send Buffer              Not Full             High Level(Triggered)
*/

void ProxyClient::OnReadable(uintptr_t s) {
    auto pair = reinterpret_cast<ClientSocket*>(s);

    if (pair->is_closed_) {
        ERROR("[%s] [#L%d] [t#%d] closed: %d", __FUNCTION__, __LINE__, gettid(), pair->socket_);
        pair->Close();
        return;
    }

    auto recv_from_server_pool = pair->buffer_;
    auto result = recv_from_server_pool->RecvFromServer(pair->socket_);

    if (result.ok()) {
        result.Update(recv_from_server_pool->ProxyClient_SendToClient(pair->other_side_->socket_));
    }

    if (!result.ok()) {
        if (result.code() != absl::StatusCode::kResourceExhausted) {
            pair->Close();
            return;
        } else {
            result = absl::OkStatus();
            // The buffer of client cannot receive more data, add to poller.
            // absl::Status ProxyConn::OnWritable(int s) will send remaining data.
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

void ProxyClient::OnWritable(uintptr_t s) {
    auto pair = reinterpret_cast<ClientSocket*>(s);

    if (pair->is_closed_) {
        ERROR("[%s] [#L%d] [t#%d] closed: %d", __FUNCTION__, __LINE__, gettid(), pair->socket_);
        pair->Close();
        return;
    }

    auto recv_from_client_pool = pair->other_side_->buffer_;
    if (reinterpret_cast<ConnSocket*>(pair->other_side_)->authentified_ == 2) {
        int socket_error = 0;
        socklen_t socket_error_len = sizeof(socket_error);
        if (getsockopt(pair->socket_, SOL_SOCKET, SO_ERROR, &socket_error, &socket_error_len) < 0) {
            return;
        }

        if (socket_error != 0) {
            send(pair->other_side_->socket_, Socks5Command::reply_failure, 10, 0);
            pair->Close();
            return;
        }

        pair->poller_->ModSocket(pair->socket_, s, EPOLLIN).IgnoreError();

        if (send(pair->other_side_->socket_, Socks5Command::reply_success, 10, 0) == SOCKET_ERROR) {
            pair->Close();
            return;
        }
        reinterpret_cast<ConnSocket*>(pair->other_side_)->authentified_++;
        return;
    }

    else {
        auto result = recv_from_client_pool->ProxyClient_SendToServer(pair->socket_);
        if (!result.ok()) {
            pair->Close();
            return;
        }
        // Remove EPOLLOUT event after the buffer is empty.
        if (recv_from_client_pool->Usage() == 0) {
            result.Update(pair->poller_->ModSocket(pair->socket_, s, EPOLLIN | EPOLLRDHUP));
            result.Update(pair->other_side_->poller_->AddSocket(pair->other_side_->socket_,
                                                                reinterpret_cast<uintptr_t>(pair->other_side_),
                                                                EPOLLIN | EPOLLRDHUP));
        }

        if (!result.ok()) {
            LOG("[%s] [#L%d] [t#%d] closed: %d", __FUNCTION__, __LINE__, gettid(), pair->socket_);
            pair->Close();
        }
    }
}

void ProxyClient::OnError(uintptr_t s) {
    auto pair = reinterpret_cast<ClientSocket*>(s);
    LOG("[%s] [#L%d] [t#%d] [%d] %s", __FUNCTION__, __LINE__, gettid(), pair->socket_, "OnError.");
}

void ProxyClient::OnClose(uintptr_t s) {
    auto pair = reinterpret_cast<ClientSocket*>(s);
    LOG("[%s] [#L%d] [t#%d] [%d] %s", __FUNCTION__, __LINE__, gettid(), pair->socket_, "OnClose.");
    pair->Close();
}