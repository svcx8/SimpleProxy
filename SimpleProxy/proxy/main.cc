#include <thread>

#include "signal.h"

#include "dispatcher/epoller.hh"
#include "misc/configuration.hh"
#include "misc/logger.hh"
#include "poller.hh"
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
    auto result = Server::Start(Configuration::port_);
    if (!result.ok()) {
        ERROR("%.*s", (int)result.message().size(), result.message().data());
        return -1;
    }
    EPoller* server_poller = new EPoller(new ProxyServer(), 99);
    constexpr long flags = EPOLLIN;

    result = server_poller->AddSocket(Server::server_socket_, flags);
    if (!result.ok()) {
        ERROR("%.*s", (int)result.message().size(), result.message().data());
        return -1;
    }

    IPoller* client_poller_1 = new ProxyPoller(new ProxyClient(), 0);
    EPoller::reserved_list_.push_back(client_poller_1);

    IPoller* client_poller_2 = new ProxyPoller(new ProxyClient(), 1);
    EPoller::reserved_list_.push_back(client_poller_2);

    IPoller* conn_poller_1 = new ProxyPoller(new ProxyConn(), 2);
    EPoller::reserved_list_.push_back(conn_poller_1);

    IPoller* conn_poller_2 = new ProxyPoller(new ProxyConn(), 3);
    EPoller::reserved_list_.push_back(conn_poller_2);

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

    LOG("[#%d] server_poller", gettid());
    while (true) {
        server_poller->Poll();
    }
}