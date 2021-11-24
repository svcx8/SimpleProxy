#include "proxy_client.hh"

#include "dispatcher/epoller.hh"
#include "memory_buffer.hh"
#include "misc/logger.hh"
#include "proxy_socket.hh"

/*
    ClientSocket->recv(); // Recv from server.
    ClientSocket->send(); // Send to server.
*/

absl::Status ProxyClient::OnReadable(int s) {
    auto pair = ProxySocket::GetInstance().GetPointer(s);
    if (!pair)
        return absl::FailedPreconditionError("[" LINE_FILE "] Socket not found in ProxySocket::socket_list_.");

    // Receiving from Server.
    auto buffer_pool = MemoryBuffer::GetPool(s);
    if (buffer_pool) {
        int recv_len = recv(s, (char*)buffer_pool->buffer_, MemoryBuffer::buffer_size_, 0);
        if (recv_len > 0) {
            buffer_pool->end_ += recv_len;
            auto result = buffer_pool->Transfer(pair->this_side_);
            if (result.ok()) {
                result = poller_->AddSocket(s, EPOLLIN | EPOLLOUT);
            }
            return result;
        }

        else if (recv_len == 0) {
            ProxySocket::GetInstance().RemovePair(pair->this_side_);
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
    auto pair = ProxySocket::GetInstance().GetPointer(s);
    if (pair->authentified_ < 3) {
        if (send(pair->this_side_, Socks5Command::reply_success, 10, 0) == SOCKET_ERROR) {
            ProxySocket::GetInstance().RemovePair(pair->this_side_);
            return absl::InternalError(strerror(errno));
        }
        pair->authentified_++;
    } else {
        auto buffer_pool = MemoryBuffer::GetPool(s);
        auto result = buffer_pool->Transfer(pair->this_side_);
        if (!result.ok()) {
            return result;
        }
    }
    return poller_->AddSocket(s, EPOLLIN);
}