#ifndef MITMCLIENT_HEADER
#define MITMCLIENT_HEADER

#include "dispatcher/ibusiness_event.hh"
#include "dispatcher/ipoller.hh"

class ProxyClient : public IBusinessEvent {
public:
    void OnReadable(int) override;
    void OnWritable(int) override;
};

#endif // ProxyClient.hh