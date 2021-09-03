#include "proxy_conn.hh"

#include "memory_buffer.hh"
#include "proxy_client.hh"
#include "proxy_socket.hh"

#include <dispatcher/epoller.hh>

#if defined __unix__
constexpr long flags = EPOLLIN | EPOLLOUT;
#endif

/*
    ConnSocket->recv(); // Recv from client.
    ConnSocket->send(); // Send to client.
*/

void ProxyConn::CheckSocks5Handshake(SocketPair* pair) {
    char head_buf[256];
    try {
        int recv_len = recv(pair->this_side_, head_buf, 256, 0);
        if (recv_len == SOCKET_ERROR) {
            throw NetEx();
        }

        if (pair->authentified_ == 0) {
            // First handshake, check valid socks5 header.
            Socks5Header head((unsigned char*)head_buf);
            if (!head.Check()) {
                throw MyEx("Unsupported version or methods.");
            }
            short int return_ver = 0x0005;
            if (send(pair->this_side_, (char*)&return_ver, 2, 0) != 2) {
                throw NetEx();
            }
        }

        else if (pair->authentified_ == 1) {
            // Second handshake, recv command.
            Socks5Command command((unsigned char*)head_buf);
            if (!command.Check()) {
                throw MyEx("Unsupported version or methods.");
            }

            if (command.address_type_ == 0x1) {
                char buffer[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, &command.address_struct_.sockaddr_in.sin_addr, buffer, sizeof(buffer));
                printf("address: %s:%d\n", buffer, htons(command.address_struct_.sockaddr_in.sin_port));
                pair->other_side_ = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, IPPROTO_TCP);
            } else if (command.address_type_ == 0x4) {
                char buffer[INET6_ADDRSTRLEN];
                inet_ntop(AF_INET6, &command.address_struct_.sockaddr_in6.sin6_addr, buffer, sizeof(buffer));
                printf("address: %s:%d\n", buffer, htons(command.address_struct_.sockaddr_in6.sin6_port));
                pair->other_side_ = socket(AF_INET6, SOCK_STREAM | SOCK_NONBLOCK, IPPROTO_TCP);
            }
            if (pair->other_side_ == SOCKET_ERROR) {
                throw NetEx();
            }

            // Connect to server.
            if (connect(pair->other_side_, (sockaddr*)&command.address_struct_, sizeof(command.address_struct_)) == SOCKET_ERROR) {
                if (errno != EINPROGRESS) {
                    throw NetEx();
                }
            }

            if (EPoller::reserved_list_[1]->AddSocket(pair->other_side_, flags) == -1) {
                LOG("[EchoServer] Failed to add event");
                CloseSocket(pair->other_side_);
                throw NetEx();
            }

            LOG("Server: %d | Type: %02X", pair->other_side_, command.address_type_);
        }
    } catch (BaseException& ex) {
        if (pair->authentified_ == 1)
            send(pair->this_side_, Socks5Command::reply_failure, 10, 0);
        EPoller::reserved_list_[0]->RemoveCloseSocket(pair->this_side_);
        ProxySocket::GetInstance().RemovePair(pair->this_side_);
        throw BaseException(ex.function_, ex.file_, ex.line_, ex.result_);
    }
    pair->authentified_++;
}

void ProxyConn::OnReadable(SOCKET s) {
    auto ptr = ProxySocket::GetInstance().GetPointer(s);
    if (ptr == nullptr) {
        // Memory leaked.
        SocketPair* pair = new SocketPair();
        pair->this_side_ = s;
        pair->authentified_ = 0;
        ProxySocket::GetInstance().AddPair(pair);
        ptr = pair;
    }
    if (ptr->authentified_ < 3) {
        CheckSocks5Handshake(ptr);
    }

    else {
        // Receiving from Client, e.g. curl https://baidu.com
        auto buffer_pool = MemoryBuffer::GetPool(s);
        if (buffer_pool) {
            int recv_len = recv(s, (char*)buffer_pool->buffer_, MemoryBuffer::buffer_size_, 0);
            if (recv_len > buffer_pool->max_recv_len_) {
                buffer_pool->max_recv_len_ = recv_len;
            }
            if (recv_len > 0) {
                buffer_pool->end_ += recv_len;
                buffer_pool->Transfer(ptr->other_side_);
            }

#if defined __unix__
            else if (recv_len == 0) {
                OnCloseable(s);
            }
#endif
            else if (recv_len <= 0) {
                throw NetEx();
            }
        } else {
            LOG("GetPool return null");
        }
    }
}

void ProxyConn::OnCloseable(SOCKET s) {
    auto ptr = ProxySocket::GetInstance().GetPointer(s);
    LOG("[ProxyConn] OnCloseable: %d", s);
    MemoryBuffer::RemovePool(ptr->this_side_);
    MemoryBuffer::RemovePool(ptr->other_side_);
    EPoller::reserved_list_[0]->RemoveCloseSocket(ptr->this_side_);
    EPoller::reserved_list_[1]->RemoveCloseSocket(ptr->other_side_);
    ProxySocket::GetInstance().RemovePair(ptr->this_side_);
}