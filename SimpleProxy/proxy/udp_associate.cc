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

absl::Status UDPHandler::ReplyHandshake(ConnSocket* pair) {
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

    ClientSocket* udp_server_pair = new ClientSocket(udp_server_socket);
    udp_server_pair->poller_ = reinterpret_cast<ProxyPoller*>(EPoller::reserved_list_[EPoller::reserved_list_.size() - 1]);
    auto result = udp_server_pair->poller_->AddSocket(udp_server_socket, reinterpret_cast<uintptr_t>(udp_server_pair), EPOLLIN);
    pair->other_side_ = udp_server_pair;
    udp_server_pair->other_side_ = pair;

    if (!result.ok()) {
        ERROR("%s", udp_rsp.status().ToString().c_str());
        return result;
    }

    unsigned char socks5_handshake_ok[22];
    memcpy(socks5_handshake_ok, prebuild_socks5_handshake_ok_, prebuild_socks5_handshake_ok_len_);

    unsigned char* ptr = (unsigned char*)&udp_server_port;
    socks5_handshake_ok[prebuild_socks5_handshake_ok_len_ - 2] = ptr[1];
    socks5_handshake_ok[prebuild_socks5_handshake_ok_len_ - 1] = ptr[0];
    int send_len = send(pair->socket_, socks5_handshake_ok, prebuild_socks5_handshake_ok_len_, 0);
    if (send_len == SOCKET_ERROR) {
        return absl::InternalError(strerror(errno));
    }
    return absl::OkStatus();
}

void UDPHandler::Forward(ClientSocket* pair) {

}