#ifndef PROXY_SERVER_HEADER
#define PROXY_SERVER_HEADER

#include "dispatcher/ibusiness_event.hh"

class ProxyServer : public IBusinessEvent {
public:
    absl::Status OnReadable(int) override;
};

#endif // proxy_server.hh