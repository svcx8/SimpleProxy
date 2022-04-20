#include <thread>

#include "proxy/udp_server.hh"
#include "signal.h"

#include "dispatcher/epoller.hh"
#include "misc/configuration.hh"
#include "misc/logger.hh"
#include "poller.hh"
#include "proxy/udp_associate.hh"
#include "proxy_client.hh"
#include "proxy_conn.hh"
#include "proxy_server.hh"
#include "server/server.hh"

/*
    There has 2 type of sockets:
    1: Proxy Recv/To Client
    2: Proxy Recv/To Server
    and:
    1: Recv From Client will Send To Server
    2: Recv From Server will Send To Client
*/

void signal_callback_handler(int signum) {
    // Just ignore the SIGPIPE signal.
    // (gdb) handle SIGPIPE nostop noprint pass
}

int main() {
    signal(SIGPIPE, signal_callback_handler);

    Configuration::Init();
    // ********** Create TCP Listener **********
    auto tcp_rsp = Server::CreateTCPServer(Configuration::port_);
    if (!tcp_rsp.ok()) {
        ERROR("%s", tcp_rsp.status().ToString().c_str());
        return -1;
    }
    
    int tcp_server_socket = tcp_rsp.value();

    EPoller* tcp_server_poller = new EPoller(new ProxyServer());
    constexpr long flags = EPOLLIN;

    auto result = tcp_server_poller->AddSocket(tcp_server_socket, flags);
    if (!result.ok()) {
        ERROR("%s", tcp_rsp.status().ToString().c_str());
        return -1;
    }
    // ********** Create TCP Listener **********

    // ********** Create UDP Listener **********
    if (UDPHandler::Init4() == false && UDPHandler::Init6() == false) {
        ERROR("[main] Init UDP failed.");
    }
    ProxyPoller* udp_server_poller = new ProxyPoller(new UDPServer());
    // ********** Create UDP Listener **********

    IPoller* client_poller_1 = new ProxyPoller(new ProxyClient());
    EPoller::reserved_list_.push_back(client_poller_1);

    IPoller* client_poller_2 = new ProxyPoller(new ProxyClient());
    EPoller::reserved_list_.push_back(client_poller_2);

    IPoller* conn_poller_1 = new ProxyPoller(new ProxyConn());
    EPoller::reserved_list_.push_back(conn_poller_1);

    IPoller* conn_poller_2 = new ProxyPoller(new ProxyConn());
    EPoller::reserved_list_.push_back(conn_poller_2);

    EPoller::reserved_list_.push_back(udp_server_poller);

    std::thread([&] {
        LOG("[#%d] conn_poller_1", gettid());
        while (true) {
            conn_poller_1->Poll();
        }
    }).detach();

    std::thread([&] {
        LOG("[#%d] conn_poller_2", gettid());
        while (true) {
            conn_poller_2->Poll();
        }
    }).detach();

    std::thread([&] {
        LOG("[#%d] client_poller_1", gettid());
        while (true) {
            client_poller_1->Poll();
        }
    }).detach();

    std::thread([&] {
        LOG("[#%d] client_poller_2", gettid());
        while (true) {
            client_poller_2->Poll();
        }
    }).detach();

    std::thread([&] {
        LOG("[#%d] udp_poller", gettid());
        while (true) {
            udp_server_poller->Poll();
        }
    }).detach();

    LOG("[#%d] server_poller", gettid());
    while (true) {
        tcp_server_poller->Poll();
    }
}