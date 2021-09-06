#ifndef MITMCLIENT_HEADER
#define MITMCLIENT_HEADER

#include <dispatcher/ibusiness_event.hh>
#include <dispatcher/ipoller.hh>

class ProxyClient : public IBusinessEvent {
public:
    void OnAcceptable(SOCKET) override{};
    void OnCloseable(SOCKET) override{};
    void OnReadable(SOCKET) override;
    void OnWritable(SOCKET) override;
};

#endif // ProxyClient.hh