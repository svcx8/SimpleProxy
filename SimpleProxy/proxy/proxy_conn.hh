#ifndef PROXY_CONN_HEADER
#define PROXY_CONN_HEADER

#include "socks5.hh"

#include "dispatcher/ipoller.hh"
#include "misc/singleton.hh"

class ProxyConn : public IBusinessEvent {
public:
    absl::Status OnReadable(int) override;
    absl::Status OnWritable(int) override;

private:
    absl::Status CheckSocks5Handshake(SocketPair*);
};

#endif // proxy_conn.hh