#include "proxy_client.hh"
#include "proxy_conn.hh"
#include "proxy_server.hh"

#include <dispatcher/epoller.hh>
#include <server/server.hh>

#include <proxy/dns_resolver.hh>

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
    try {
        // // DoH testing
        // std::string domain("baidu.com");
        // auto result = DNSResolver::ResolveDoH(domain);
        // unsigned char* ptr = (unsigned char*)&result;
        // LOG("%d.%d.%d.%d", ptr[0], ptr[1], ptr[2], ptr[3]);

        // // DNS testing
        // result = DNSResolver::Resolve("cn.bing.com");
        // ptr = (unsigned char*)&result;
        // LOG("%d.%d.%d.%d", ptr[0], ptr[1], ptr[2], ptr[3]);

        constexpr int port = 2333;
        Server::GetInstance().Start(port);

        IPoller* server_poller = new EPoller(new ProxyServer(), 1);
        constexpr long flags = EPOLLIN;

        server_poller->AddSocket(Server::GetInstance().server_socket_, flags);

        IPoller* conn_poller = new EPoller(new ProxyConn(), 3);
        EPoller::reserved_list_.push_back(conn_poller);

        IPoller* client_poller = new EPoller(new ProxyClient(), 4);
        EPoller::reserved_list_.push_back(client_poller);

        std::thread([&] {
            LOG("[++] ConnPoller Start");
            while (true) {
                try {
                    conn_poller->Poll();
                } catch (BaseException& ex) {
                    LOG("Exception: %s\n[%s] [%s] Line: #%d", ex.result_, ex.file_, ex.function_, ex.line_);
                }
            }
        }).detach();

        std::thread([&] {
            LOG("[++] ClientPoller Start");
            while (true) {
                try {
                    client_poller->Poll();
                } catch (BaseException& ex) {
                    LOG("Exception: %s\n[%s] [%s] Line: #%d", ex.result_, ex.file_, ex.function_, ex.line_);
                }
            }
        }).detach();

        LOG("[++] ServerPoller Start");
        while (true) {
            try {
                server_poller->Poll();
            } catch (BaseException& ex) {
                LOG("Exception: %s\n[%s] [%s] Line: #%d", ex.result_, ex.file_, ex.function_, ex.line_);
            }
        }
    } catch (BaseException& ex) {
        LOG("Exception: %s\n[%s] [%s] Line: #%d", ex.result_, ex.file_, ex.function_, ex.line_);
    }
}