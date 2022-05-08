#ifndef PROXY_SOCKET_HEADER
#define PROXY_SOCKET_HEADER

#include <sys/socket.h>

#include <atomic>
#include <memory>
#include <shared_mutex>
#include <unordered_map>

#include "memory_buffer.hh"
#include "poller.hh"

ProxyPoller* AcquireConnPoller();
ProxyPoller* AcquireClientPoller();
void StartPurgeThread();

class SocketBase {
public:
    SocketBase(int s, ProxyPoller* poller, MemoryBuffer* buffer) : socket_(s), poller_(poller), buffer_(buffer){};
    virtual void Close() = 0;

    // delete copy and move constructor and assignment operator
    SocketBase(const SocketBase&) = delete;
    SocketBase& operator=(const SocketBase&) = delete;
    SocketBase(SocketBase&&) = delete;
    SocketBase& operator=(SocketBase&&) = delete;

    int socket_ = -233;
    int is_closed_ = false;
    ProxyPoller* poller_ = nullptr;
    MemoryBuffer* buffer_ = nullptr;
    SocketBase* other_side_ = nullptr;
};

class ConnSocket : public SocketBase {
public:
    ConnSocket(int s) : SocketBase(s, AcquireConnPoller(), MemoryBuffer::AcquirePool()) {
        AddToList(s, this);
    }
    virtual void Close() override;

    // delete copy and move constructor and assignment operator
    ConnSocket(const ConnSocket&) = delete;
    ConnSocket& operator=(const ConnSocket&) = delete;
    ConnSocket(ConnSocket&&) = delete;
    ConnSocket& operator=(ConnSocket&&) = delete;

    static void AddToList(int s, ConnSocket* conn_socket);
    static std::unordered_map<int, ConnSocket*> list_;
    static std::mutex list_mutex_;

    int authentified_ = 0;
};

class ClientSocket : public SocketBase {
public:
    ClientSocket(int s) : SocketBase(s, AcquireClientPoller(), MemoryBuffer::AcquirePool()) {
        AddToList(s, this);
    }
    virtual void Close() override;
    // delete copy and move constructor and assignment operator
    ClientSocket(const ClientSocket&) = delete;
    ClientSocket& operator=(const ClientSocket&) = delete;
    ClientSocket(ClientSocket&&) = delete;
    ClientSocket& operator=(ClientSocket&&) = delete;

    static void AddToList(int s, ClientSocket* client_socket);
    static std::unordered_map<int, ClientSocket*> list_;
    static std::mutex list_mutex_;

    sockaddr_storage tcp_auth_addr_;
    int tcp_auth_addr_len_ = 0;
};

#endif // socket_pair.hh