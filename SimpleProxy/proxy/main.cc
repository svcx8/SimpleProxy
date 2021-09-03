#include "proxy_client.hh"
#include "proxy_conn.hh"
#include "proxy_server.hh"

#include <dispatcher/epoller.hh>
#include <server/server.hh>

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
#if defined WIN32
    system("chcp 65001");
    MyWinSocketSetup();
#endif

    try {
        constexpr int port = 2333;
        Server::GetInstance().Start(port);

        IPoller* poller = new EPoller(new ProxyServer(), 1);
        constexpr long flags = EPOLLIN;

        poller->AddSocket(Server::GetInstance().server_socket_, flags);

        IPoller* ConnPoller = new EPoller(new ProxyConn(), 3);
        EPoller::reserved_list_.push_back(ConnPoller);

        IPoller* ClientPoller = new EPoller(new ProxyClient(), 4);
        EPoller::reserved_list_.push_back(ClientPoller);

        std::thread([&] {
            LOG("ConnPoller Start");
            while (true) {
                try {
                    ConnPoller->Poll();
                } catch (BaseException& ex) {
                    LOG("Exception: %s\n[%s] [%s] Line: #%d", ex.result_, ex.file_, ex.function_, ex.line_);
                }
            }
        }).detach();

        std::thread([&] {
            while (true) {
                try {
                    ClientPoller->Poll();
                } catch (BaseException& ex) {
                    LOG("Exception: %s\n[%s] [%s] Line: #%d", ex.result_, ex.file_, ex.function_, ex.line_);
                }
            }
        }).detach();

        while (true) {
            try {
                poller->Poll();
            } catch (BaseException& ex) {
                LOG("Exception: %s\n[%s] [%s] Line: #%d", ex.result_, ex.file_, ex.function_, ex.line_);
            }
        }
    } catch (BaseException& ex) {
        LOG("Exception: %s\n[%s] [%s] Line: #%d", ex.result_, ex.file_, ex.function_, ex.line_);
    }
}