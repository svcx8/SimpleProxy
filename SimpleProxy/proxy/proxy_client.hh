#ifndef MITMCLIENT_HEADER
#define MITMCLIENT_HEADER

#include "dispatcher/ibusiness_event.hh"
#include "dispatcher/ipoller.hh"

class ProxyClient : public IBusinessEvent {
public:
    void OnReadable(uintptr_t) override;
    void OnWritable(uintptr_t) override;
    void OnError(uintptr_t) override;
    void OnClose(uintptr_t) override;
};

#endif // ProxyClient.hh