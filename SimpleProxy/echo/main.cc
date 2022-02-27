#include "echo_conn.hh"
#include "echo_server.hh"

#include <thread>

#include "dispatcher/epoller.hh"
#include <misc/net.hh>
#include <server/server.hh>

int main() {
    const char* IP = "127.0.0.1";
    int Port = 2345;
    Server::Start(Port);

    IPoller* ServerPoller = new EPoller(new EchoServer(), 1);
    constexpr long flags = EPOLLIN;

    ServerPoller->AddSocket(Server::server_socket_, flags);

    IPoller* ConnPoller = new EPoller(new EchoConn(), 2);
    EPoller::reserved_list_.push_back(ConnPoller);

    // SubReactor
    std::thread([&] {
        LOG("ConnPoller Start");
        while (true) {
            ConnPoller->Poll();
        }
    }).detach();

    // Acceptor
    std::thread([&] {
        LOG("ServerPoller Start");
        while (true) {
            ServerPoller->Poll();
        }
    }).join();
}