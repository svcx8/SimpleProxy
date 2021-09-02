#ifndef PROXY_SOCKET_HEADER
#define PROXY_SOCKET_HEADER

#include "socks5.hh"

#include <misc/singleton.hh>

#include <vector>

class ProxySocket : public Singleton<ProxySocket> {
public:
    void AddPair(SocketPair*);
    void RemovePair(int);
    int GetSocket(int);
    SocketPair* GetPointer(int);

private:
    std::vector<SocketPair*> SocketList;
};

#endif // proxy_socket.hh