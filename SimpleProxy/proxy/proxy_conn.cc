#include "proxy_conn.hh"

#include "proxy_client.hh"
#include "proxy_socket.hh"
#include "receiver_pool.hh"

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
        if (recv_len != 10) {
            throw MyEx("Invalid or Unsupported socks5 command.");
        }
        Socks5Command command((unsigned char*)head_buf);
        if (!command.Check()) {
            throw MyEx("Unsupported version or methods.");
        }

        pair->other_side_ = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, IPPROTO_TCP);
        if (pair->other_side_ == SOCKET_ERROR) {
            throw NetEx();
        }

        sockaddr_in proxy_to_server_addr;
        proxy_to_server_addr.sin_family = AF_INET;
        proxy_to_server_addr.sin_addr.s_addr = htonl(command.remote_address_);
        proxy_to_server_addr.sin_port = htons(command.port_);
        // Connect to server.
        if (connect(pair->other_side_, (sockaddr*)&proxy_to_server_addr, sizeof(proxy_to_server_addr)) == SOCKET_ERROR) {
            if (errno != EINPROGRESS) {
                throw NetEx();
            }
        }

        if (EPoller::reserved_list_[1]->AddSocket(pair->other_side_, flags) == -1) {
            LOG("[EchoServer] Failed to add event");
            CloseSocket(pair->other_side_);
            throw NetEx();
        }

        LOG("Server: %d | Port: %d", pair->other_side_, proxy_to_server_addr.sin_port);
    }
    pair->authentified_++;
}

void ProxyConn::OnReadable(SOCKET s) {
    auto ptr = ProxySocket::GetInstance().GetPointer(s);
    if (ptr == nullptr) {
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
        auto buffer_pool = ReceiverPool::GetPool(s);
        if (buffer_pool) {
            int recv_len = recv(s, (char*)buffer_pool->buffer_, ReceiverPool::buffer_size_, 0);
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
    ReceiverPool::Remove(ptr->this_side_);
    ReceiverPool::Remove(ptr->other_side_);
    EPoller::reserved_list_[0]->RemoveCloseSocket(ptr->this_side_);
    EPoller::reserved_list_[1]->RemoveCloseSocket(ptr->other_side_);
    ProxySocket::GetInstance().RemovePair(ptr->this_side_);
}