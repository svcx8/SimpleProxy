#ifndef ECHO_SERVER_HEADER
#define ECHO_SERVER_HEADER

#include <dispatcher/ipoller.hh>

class EchoServer : public IBusinessEvent {
public:
    void OnAcceptable(SOCKET) override{};
    void OnCloseable(SOCKET) override{};
    void OnReadable(SOCKET) override;
    void OnWritable(SOCKET) override{};
};

#endif // echo_server.hh