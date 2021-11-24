#ifndef PROXY_CONN_HEADER
#define PROXY_CONN_HEADER

#include "socks5.hh"

#include "dispatcher/ipoller.hh"

class ProxyConn : public IBusinessEvent {
public:
    absl::Status OnReadable(int) override;

private:
    absl::Status CheckSocks5Handshake(SocketPair*);
};

#endif // proxy_conn.hh