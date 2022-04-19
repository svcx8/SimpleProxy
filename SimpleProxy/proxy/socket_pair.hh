#ifndef PROXY_SOCKET_HEADER
#define PROXY_SOCKET_HEADER

#include <sys/socket.h>

#include <atomic>
#include <memory>
#include <shared_mutex>
#include <unordered_map>

#include "poller.hh"

class SocketPair {
public:
    ~SocketPair();
    SocketPair(int port, int s) : port_(port), conn_socket_(s) {}
    ProxyPoller* conn_poller_ = nullptr;
    ProxyPoller* client_poller_ = nullptr;
    int port_ = 0;
    int conn_socket_ = 0;
    int client_socket_ = 0;
    int authentified_ = 0;
    sockaddr_storage tcp_auth_addr_;
    int tcp_auth_addr_len_ = 0;
};

class SocketPairManager {
    friend class SocketPair;

public:
    static void AddPair(int port, int s);
    static void RemovePair(SocketPair* pair);
    static SocketPair* GetPointer(int);

    static ProxyPoller* AcquireConnPoller(SocketPair* pair);
    static ProxyPoller* AcquireClientPoller(SocketPair* pair);

private:
    static std::unordered_map<int, SocketPair*> socket_list_; // Set the client port as index.
    static std::shared_mutex list_mutex_;
    static std::atomic<int> last_poller_index_;
};

#endif // socket_pair.hh