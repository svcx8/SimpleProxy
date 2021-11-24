#include "proxy_client.hh"

#include "dispatcher/epoller.hh"
#include "memory_buffer.hh"
#include "misc/logger.hh"
#include "proxy_socket.hh"

/*
    ClientSocket->recv(); // Recv from server.
    ClientSocket->send(); // Send to server.
*/

void ClosePair(SocketPair* pair) {
    LOG("[ProxyClient] OnCloseable: %d - %d", pair->this_side_, pair->other_side_);
    MemoryBuffer::RemovePool(pair);
    ProxySocket::GetInstance().RemovePair(pair->this_side_);
    close(pair->this_side_);
    close(pair->other_side_);
}

absl::Status ProxyClient::OnReadable(int s) {
    auto pair = ProxySocket::GetInstance().GetPointer(s);
    if (!pair)
        return absl::FailedPreconditionError("[" LINE_FILE "] Socket not found in ProxySocket::socket_list_.");

    // Receiving from Server.
    auto buffer_pool = MemoryBuffer::GetPool(s);
    if (buffer_pool) {
        int recv_len = recv(s, (char*)buffer_pool->buffer_, MemoryBuffer::buffer_size_, 0);
        if (recv_len > buffer_pool->max_recv_len_) {
            buffer_pool->max_recv_len_ = recv_len;
        }
        if (recv_len > 0) {
            buffer_pool->end_ += recv_len;
            // If send() return EAGAIN, register into epoll.
            auto result = buffer_pool->Transfer(pair->this_side_);
            if (result.ok()) {
                result = poller_->AddSocket(s, EPOLLIN | EPOLLOUT);
            }

            return result;
        }

        else if (recv_len == 0) {
            ProxySocket::ClosePair(pair);
            return absl::OkStatus();
        }

        else /* if (recv_len < 0)*/ {
            return absl::InternalError(strerror(errno));
        }
    } else {
        return absl::FailedPreconditionError("[" LINE_FILE "] GetPool return null");
    }
}

// connect to remote server success.
absl::Status ProxyClient::OnWritable(int s) {
    LOG("[ProxyClient] OnWritable: %d", s);
    auto pair = ProxySocket::GetInstance().GetPointer(s);
    if (pair->authentified_ < 3) {
        if (send(pair->this_side_, Socks5Command::reply_success, 10, 0) == SOCKET_ERROR) {
            ProxySocket::ClosePair(pair);
            return absl::InternalError(strerror(errno));
        }
        pair->authentified_++;
    } else {
        auto buffer_pool = MemoryBuffer::GetPool(s);
        buffer_pool->Transfer(pair->this_side_);
    }
    poller_->AddSocket(s, EPOLLIN);
    return absl::OkStatus();
}