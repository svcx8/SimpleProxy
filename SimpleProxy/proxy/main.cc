#include "proxy_client.hh"
#include "proxy_conn.hh"
#include "proxy_server.hh"

#include "dispatcher/epoller.hh"
#include "misc/configuration.hh"
#include "misc/logger.hh"
#include "server/server.hh"

#include <thread>

/*
    There has 2 type of sockets:
    1: Proxy Recv/To Client
    2: Proxy Recv/To Server
    and:
    1: Recv From Client will Send To Server
    2: Recv From Server will Send To Client
*/

int main() {
    auto result = Server::GetInstance().Start(Configuration::GetInstance().port_);
    if (!result.ok()) {
        ERROR("%.*s", result.message().size(), result.message().data());
        return -1;
    }
    IPoller* server_poller = new EPoller(new ProxyServer(), 99);
    constexpr long flags = EPOLLIN;

    result = server_poller->AddSocket(Server::GetInstance().server_socket_, flags);
    if (!result.ok()) {
        ERROR("%.*s", result.message().size(), result.message().data());
        return -1;
    }

    IPoller* client_poller_1 = new EPoller(new ProxyClient(), 0);
    EPoller::reserved_list_.push_back(client_poller_1);

    IPoller* client_poller_2 = new EPoller(new ProxyClient(), 1);
    EPoller::reserved_list_.push_back(client_poller_2);

    IPoller* conn_poller_1 = new EPoller(new ProxyConn(), 2);
    EPoller::reserved_list_.push_back(conn_poller_1);

    IPoller* conn_poller_2 = new EPoller(new ProxyConn(), 3);
    EPoller::reserved_list_.push_back(conn_poller_2);

    std::thread([&] {
        LOG("[++] ConnPoller1 Start");
        while (true) {
            conn_poller_1->Poll();
        }
    }).detach();

    std::thread([&] {
        LOG("[++] ConnPoller2 Start");
        while (true) {
            conn_poller_2->Poll();
        }
    }).detach();

    std::thread([&] {
        LOG("[++] ClientPoller1 Start");
        while (true) {
            client_poller_1->Poll();
        }
    }).detach();

    std::thread([&] {
        LOG("[++] ClientPoller2 Start");
        while (true) {
            client_poller_2->Poll();
        }
    }).detach();

    LOG("[++] ServerPoller Start");
    while (true) {
        server_poller->Poll();
    }
}