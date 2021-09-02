#ifndef EPOLLER_HEADER
#define EPOLLER_HEADER

#ifdef __unix__

#include <echo/echo_conn.hh>
#include <echo/echo_server.hh>

#include <proxy/proxy_client.hh>
#include <proxy/proxy_conn.hh>
#include <proxy/proxy_server.hh>

#include <sys/epoll.h>

#include <array>
#include <vector>

constexpr int MAX_EVENT_NUMBER = 64;

class EPoller : public IPoller {
public:
    enum PollType {
        tEchoConn,
        tEchoServer,
        tProxyClient,
        tProxyServer,
        tProxyConn
    };

    int id_ = 0;
    EPoller(PollType type, int _id);
    ~EPoller(){};

    int AddSocket(SOCKET Socket, long eventflags);
    // Just close the socket fd. It will automatically deregister itself from epoll.
    void RemoveSocket(SOCKET Socket);
    void RemoveCloseSocket(SOCKET Socket);

    void Poll();
    void HandleEvents(SOCKET Socket, uint32_t Event);
    std::array<epoll_event, MAX_EVENT_NUMBER> EventArray{};

    static int SetNonBlocking(int);
    static std::vector<IPoller*> reserved_list_; // SubReactor list

private:
    SOCKET epoller_inst_ = 0;
};

#endif

#endif // epoller.hh