#include "echo_conn.hh"
#include "echo_server.hh"

#include <thread>

#include <dispatcher/epoller.hh>
#include <misc/net.hh>
#include <server/server.hh>

int main() {
#if defined WIN32
    system("chcp 65001");
    MyWinSocketSetup();
#endif

    try {
        const char* IP = "127.0.0.1";
        int Port = 2345;
        Server::GetInstance().Start(Port);

#if defined __unix__
        IPoller* ServerPoller = new EPoller(EPoller::tEchoServer, 1);
        constexpr long flags = EPOLLIN;
#endif

        ServerPoller->AddSocket(Server::GetInstance().server_socket_, flags);

        IPoller* ConnPoller = new EPoller(EPoller::tEchoConn, 2);
        EPoller::ReservedList.push_back(ConnPoller);

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
    } catch (BaseException& ex) {
        LOG("Exception: %s\n[%s] [%s] Line: #%d", ex.result_, ex.file_, ex.function_, ex.line_);
    }
}