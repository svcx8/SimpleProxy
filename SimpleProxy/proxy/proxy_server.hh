#ifndef PROXY_SERVER_HEADER
#define PROXY_SERVER_HEADER

#include "dispatcher/ibusiness_event.hh"

class ProxyServer : public IBusinessEvent {
public:
    void OnReadable(uintptr_t) override;
};

#endif // proxy_server.hh