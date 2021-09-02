#include "proxy_client.hh"

#include "proxy_socket.hh"
#include "receiver_pool.hh"

#include <dispatcher/epoller.hh>

/*
    ClientSocket->recv(); // Recv from server.
    ClientSocket->send(); // Send to server.
*/

void ProxyClient::OnReadable(SOCKET s) {
    auto ptr = ProxySocket::GetInstance().GetPointer(s);
    // Receiving from Server.
    auto buffer_pool = ReceiverPool::GetPool(s);
    int recv_len = recv(s, (char*)buffer_pool->buffer_, ReceiverPool::buffer_size_, 0);
    if (recv_len > 0) {
        buffer_pool->end_ += recv_len;
        // If send() return EAGAIN, register into epoll.
        if (buffer_pool->Transfer(ptr->this_side_) == -1) {
            poller_->AddSocket(s, EPOLLIN | EPOLLOUT);
        }
    }

#if defined __unix__
    else if (recv_len == 0) {
        OnCloseable(s);
    }
#endif
    else if (recv_len <= 0) {
        throw NetEx();
    }
}

// connect to remote server success.
void ProxyClient::OnWritable(SOCKET s) {
    LOG("[ProxyClient] OnWritable: %d", s);
    auto pair = ProxySocket::GetInstance().GetPointer(s);
    if (pair->authentified_ < 3) {
        if (send(pair->this_side_, Socks5Command::reply_success, 10, 0) == SOCKET_ERROR) {
            throw NetEx();
        }
        pair->authentified_++;
    } else {
        auto buffer_pool = ReceiverPool::GetPool(s);
        buffer_pool->Transfer(pair->this_side_);
    }
    poller_->AddSocket(s, EPOLLIN);
}

void ProxyClient::OnCloseable(SOCKET s) {
    auto ptr = ProxySocket::GetInstance().GetPointer(s);
    ReceiverPool::Remove(ptr->this_side_);
    ReceiverPool::Remove(ptr->other_side_);
    EPoller::reserved_list_[0]->RemoveCloseSocket(ptr->this_side_);
    EPoller::reserved_list_[1]->RemoveCloseSocket(ptr->other_side_);
    ProxySocket::GetInstance().RemovePair(ptr->this_side_);
}