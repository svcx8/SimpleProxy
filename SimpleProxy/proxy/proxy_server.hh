#ifndef PROXY_SERVER_HEADER
#define PROXY_SERVER_HEADER

#include <dispatcher/ibusiness_event.hh>

class ProxyServer : public IBusinessEvent {
public:
    void OnAcceptable(SOCKET) override{};
    void OnReadable(SOCKET) override;
    void OnWritable(SOCKET) override{};
    void OnCloseable(SOCKET) override{};
};

#endif // proxy_server.hh