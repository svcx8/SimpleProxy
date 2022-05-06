#include "udp_associate.hh"

#include <ifaddrs.h>
#include <netdb.h>
#include <sys/socket.h>
#include <thread>

#include "dispatcher/epoller.hh"
#include "misc/configuration.hh"
#include "misc/logger.hh"
#include "misc/net.hh"
#include "proxy/poller.hh"
#include "socket_pair.hh"
#include "socks5.hh"
#include "udp_buffer.hh"

uint64_t UDPHandler::server_ip_ = 0;
unsigned char UDPHandler::prebuild_socks5_handshake_ok_[22] = {
    0x05, 0x00, 0x00, 0x01
};
int UDPHandler::prebuild_socks5_handshake_ok_len_ = 0;

bool UDPHandler::Init(int af) {
    ifaddrs *ifaddr, *ifa;
    char host[NI_MAXHOST];

    if (getifaddrs(&ifaddr) == -1) {
        return false;
    }

    for (ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == nullptr)
            continue;
        int name_info = getnameinfo(ifa->ifa_addr, sizeof(struct sockaddr_in), host, NI_MAXHOST, nullptr, 0, NI_NUMERICHOST);
        if (((strcmp(ifa->ifa_name, "eth0") == 0) || (strcmp(ifa->ifa_name, "wlan0") == 0)) && (ifa->ifa_addr->sa_family == af)) {
            if (name_info != 0) {
                ERROR("getnameinfo() failed: %s", gai_strerror(name_info));
                return false;
            }
            break;
        }
    }

    if (inet_pton(af, host, &prebuild_socks5_handshake_ok_[4]) == -1) {
        return false;
    }

    INFO("[UDPServer] Server IP: %s", host);

    prebuild_socks5_handshake_ok_len_ = af == AF_INET6 ? 22 : 10;

    freeifaddrs(ifaddr);
    return true;
}

absl::Status UDPHandler::ReplyHandshake(SocketPair* pair) {
    auto udp_rsp = Server::CreateUDPServer(0);
    if (!udp_rsp.ok()) {
        ERROR("%s", udp_rsp.status().ToString().c_str());
        return udp_rsp.status();
    }

    int udp_server_socket = *udp_rsp;
    struct sockaddr_in sin;
    socklen_t len = sizeof(sin);
    if (getsockname(udp_server_socket, (struct sockaddr*)&sin, &len) == -1) {
        return absl::InternalError(strerror(errno));
    }
    int udp_server_port = ntohs(sin.sin_port);

    ProxyPoller* udp_server_poller = reinterpret_cast<ProxyPoller*>(EPoller::reserved_list_[EPoller::reserved_list_.size() - 1]);
    auto result = udp_server_poller->AddSocket(udp_server_socket, reinterpret_cast<uintptr_t>(pair), EPOLLIN);
    if (!result.ok()) {
        ERROR("%s", udp_rsp.status().ToString().c_str());
        return result;
    }

    unsigned char socks5_handshake_ok[22];
    memcpy(socks5_handshake_ok, prebuild_socks5_handshake_ok_, prebuild_socks5_handshake_ok_len_);

    unsigned char* ptr = (unsigned char*)&udp_server_port;
    socks5_handshake_ok[prebuild_socks5_handshake_ok_len_ - 2] = ptr[1];
    socks5_handshake_ok[prebuild_socks5_handshake_ok_len_ - 1] = ptr[0];
    int send_len = send(pair->conn_socket_, socks5_handshake_ok, prebuild_socks5_handshake_ok_len_, 0);
    if (send_len == SOCKET_ERROR) {
        return absl::InternalError(strerror(errno));
    }
    pair->client_socket_ = udp_server_socket;
    return absl::OkStatus();
}

void UDPHandler::Forward(SocketPair* pair) {
    auto pool = UDPBuffer::GetPool(pair->client_socket_);
    auto res = pool->Receive(pair->client_socket_);
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
                res.Update(pool->Send(pair->client_socket_, packet.command_.sock_addr_, packet.command_.sock_addr_len_, 22 + packet.command_.index_));
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

            res.Update(pool->Send(pair->client_socket_, reinterpret_cast<sockaddr*>(&pair->tcp_auth_addr_), pair->tcp_auth_addr_len_, distance));
            if (res.ok()) {
                struct sockaddr_in sin;
                socklen_t len = sizeof(sin);
                if (getsockname(pair->client_socket_, (struct sockaddr*)&sin, &len) == -1) {
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
    ERROR("[UDPHandler::ReadFrom] Error: %s %d - %d", res.ToString().c_str(), pair->conn_socket_, pair->client_socket_);
}