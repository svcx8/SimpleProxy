#ifndef PROXY_SOCKET_HEADER
#define PROXY_SOCKET_HEADER

#include "socks5.hh"

#include <dispatcher/ipoller.hh>
#include <misc/singleton.hh>

#include <memory>
#include <mutex>
#include <vector>

class ProxySocket : public Singleton<ProxySocket> {
public:
    void AddPair(std::unique_ptr<SocketPair>&&);
    void RemovePair(int);
    SocketPair* GetPointer(int);
    static IPoller* GetConnPoller(SocketPair* pair);
    static IPoller* GetClientPoller(SocketPair* pair);

private:
    std::vector<std::unique_ptr<SocketPair>> socket_list_;
    static std::mutex list_mutex_;
    static int last_poller_index_;
    static std::mutex get_poller_mutex_;
};

#endif // proxy_socket.hh