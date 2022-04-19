#ifndef UDP_SERVER_HEADER
#define UDP_SERVER_HEADER

#include "dispatcher/ibusiness_event.hh"

class UDPServer : public IBusinessEvent {
public:
    void OnReadable(uintptr_t) override;
};

#endif // udp_server.hh