#ifndef PROXY_CONN_HEADER
#define PROXY_CONN_HEADER

#include "dispatcher/ipoller.hh"

class ProxyConn : public IBusinessEvent {
public:
    void OnReadable(uintptr_t) override;
    void OnWritable(uintptr_t) override;
};

#endif // proxy_conn.hh