#ifndef PROXY_SOCKET_HEADER
#define PROXY_SOCKET_HEADER

#include "socks5.hh"

#include <misc/singleton.hh>

#include <memory>
#include <vector>

class ProxySocket : public Singleton<ProxySocket> {
public:
    void AddPair(std::unique_ptr<SocketPair>&&);
    void RemovePair(int);
    int GetSocket(int);
    SocketPair* GetPointer(int);

private:
    std::vector<std::unique_ptr<SocketPair>> socket_list_;
};

#endif // proxy_socket.hh