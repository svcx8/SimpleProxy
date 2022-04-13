#ifndef PROXY_SOCKET_HEADER
#define PROXY_SOCKET_HEADER

#include <map>
#include <memory>
#include <mutex>

#include "dispatcher/ipoller.hh"

class SocketPair {
public:
    ~SocketPair();
    SocketPair(int port, int s) : port_(port), this_side_(s) {}
    IPoller* this_poller_ = nullptr;
    IPoller* other_poller_ = nullptr;
    int port_ = 0;
    int this_side_ = 0;
    int other_side_ = 0;
    int authentified_ = 0;
    // std::unique_ptr<TokenBucket> token_bucket_;
};

class SocketPairManager {
    friend class SocketPair;

public:
    static void AddPair(int port, int s);
    static void RemovePair(SocketPair* pair);
    static SocketPair* GetPointer(int);

    static IPoller* GetConnPoller(SocketPair* pair);
    static IPoller* GetClientPoller(SocketPair* pair);

private:
    static std::map<int, SocketPair*> socket_list_; // Set the client port as index.
    static std::mutex list_mutex_;
    static int last_poller_index_;
};

#endif // socket_pair.hh