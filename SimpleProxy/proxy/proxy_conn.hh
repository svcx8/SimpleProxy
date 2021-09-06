#ifndef PROXY_CONN_HEADER
#define PROXY_CONN_HEADER

#include "socks5.hh"

#include <dispatcher/ibusiness_event.hh>
#include <dispatcher/ipoller.hh>

class ProxyConn : public IBusinessEvent {
public:
    ~ProxyConn() {}

    void OnAcceptable(SOCKET) override{};
    void OnReadable(SOCKET) override;
    void OnWritable(SOCKET) override{};
    void OnCloseable(SOCKET) override{};

private:
    void HandShake(SocketPair* Pair);
    void CheckSocks5Handshake(SocketPair*);
};

#endif // proxy_conn.hh