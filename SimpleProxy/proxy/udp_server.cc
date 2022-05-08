#include "udp_server.hh"

#include <netdb.h>
#include <sys/socket.h>
#include <thread>

#include "misc/net.hh"
#include "socket_pair.hh"
#include "udp_associate.hh"
#include "udp_buffer.hh"

void UDPServer::OnReadable(uintptr_t s) {
    ClientSocket* pair = reinterpret_cast<ClientSocket*>(s);
    auto pool = UDPBuffer::GetPool(pair->socket_);
    auto res = pool->Receive(pair->socket_);
    if (res.ok()) {
        if (pair->tcp_auth_addr_.ss_family == 0) {
            memcpy(&pair->tcp_auth_addr_, &pool->client_addr_, sizeof(pool->client_addr_));
            if (pair->tcp_auth_addr_.ss_family == AF_INET) {
                pair->tcp_auth_addr_len_ = sizeof(struct sockaddr_in);
            } else {
                pair->tcp_auth_addr_len_ = sizeof(struct sockaddr_in6);
            }
            LOG("First init. port: %d %d host: %s %s",
                ((sockaddr_in*)&pair->tcp_auth_addr_)->sin_port, ((sockaddr_in*)&pool->client_addr_)->sin_port,
                inet_ntoa(((sockaddr_in*)&pair->tcp_auth_addr_)->sin_addr), inet_ntoa(((sockaddr_in*)&pool->client_addr_)->sin_addr));
        }

        auto AddrEqual = [](sockaddr_storage* lhs, sockaddr_storage* rhs) {
            return memcmp(lhs, rhs, sizeof(sockaddr_storage)) == 0;
        };

        if (AddrEqual(&pair->tcp_auth_addr_, &pool->client_addr_)) {
            // Received from client
            // For compatibility.
            pool->buffer_[22] = 0x05;
            pool->buffer_[23] = 0x01;
            UDPPacket packet(&pool->buffer_[22]);
            res.Update(packet.Check());
            if (res.ok()) {
                res.Update(pool->Send(pair->socket_, packet.command_.sock_addr_, packet.command_.sock_addr_len_, 22 + packet.command_.index_));
                if (res.ok()) {
                    LOG("[UDPHandler::ReadFrom] ToRemote: %s:%d",
                        inet_ntoa(reinterpret_cast<sockaddr_in*>(packet.command_.sock_addr_)->sin_addr),
                        ntohs(reinterpret_cast<sockaddr_in*>(packet.command_.sock_addr_)->sin_port));
                    return;
                }
            }
        }

        else {
            // Received from remote
            int index = 0;
            int port = 0;
            char host[NI_MAXHOST];
            int distance = 0;

            if (pool->client_addr_.ss_family == AF_INET) {
                index = 16 - 4;
                port = reinterpret_cast<sockaddr_in*>(&pool->client_addr_)->sin_port;
                inet_ntop(AF_INET, &reinterpret_cast<sockaddr_in*>(&pool->client_addr_)->sin_addr, host, NI_MAXHOST);
                memcpy(&pool->buffer_[index + 4], &reinterpret_cast<sockaddr_in*>(&pool->client_addr_)->sin_addr, 4);

                unsigned char* ptr = (unsigned char*)&port;
                pool->buffer_[index + 4 + 4] = ptr[1];
                pool->buffer_[index + 4 + 4 + 1] = ptr[0];
                distance = 16 - 4;
            } else {
                port = reinterpret_cast<sockaddr_in6*>(&pool->client_addr_)->sin6_port;
                inet_ntop(AF_INET6, &reinterpret_cast<sockaddr_in6*>(&pool->client_addr_)->sin6_addr, host, NI_MAXHOST);
                memcpy(&pool->buffer_[index + 4], &reinterpret_cast<sockaddr_in6*>(&pool->client_addr_)->sin6_addr, 16);

                unsigned char* ptr = (unsigned char*)&port;
                pool->buffer_[index + 4 + 16] = ptr[1];
                pool->buffer_[index + 4 + 16 + 1] = ptr[0];
            }
            pool->buffer_[index++] = 0x05;
            pool->buffer_[index++] = 0x00;
            pool->buffer_[index++] = 0x00;
            pool->buffer_[index++] = 0x01;

            res.Update(pool->Send(pair->socket_, reinterpret_cast<sockaddr*>(&pair->tcp_auth_addr_), pair->tcp_auth_addr_len_, distance));
            if (res.ok()) {
                struct sockaddr_in sin;
                socklen_t len = sizeof(sin);
                if (getsockname(pair->socket_, (struct sockaddr*)&sin, &len) == -1) {
                    return;
                }
                inet_ntop(AF_INET, &sin.sin_addr, host, NI_MAXHOST);
                LOG("[UDPHandler::ReadFrom] ToClient: %s:%d %02X %02X %02X %02X",
                    host,
                    ntohs(sin.sin_port),
                    pool->buffer_[distance],
                    pool->buffer_[distance + 1],
                    pool->buffer_[distance + 2],
                    pool->buffer_[distance + 3]);
                return;
            }
        }
    }
    ERROR("[UDPHandler::ReadFrom] Error: %s %d - %d", res.ToString().c_str(), pair->other_side_->socket_, pair->socket_);
}