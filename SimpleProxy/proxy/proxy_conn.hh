#ifndef PROXY_CONN_HEADER
#define PROXY_CONN_HEADER

#include "dispatcher/ipoller.hh"
#include "socket_pair.hh"

class ProxyConn : public IBusinessEvent {
public:
    void OnReadable(int) override;
    void OnWritable(int) override;

private:
    absl::Status CheckSocks5Handshake(SocketPair*);
};

#endif // proxy_conn.hh