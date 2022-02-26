#ifndef ECHO_SERVER_HEADER
#define ECHO_SERVER_HEADER

#include "dispatcher/ipoller.hh"

class EchoServer : public IBusinessEvent {
public:
    void OnAcceptable(int) override{};
    void OnCloseable(int) override{};
    void OnReadable(int) override;
    void OnWritable(int) override{};
};

#endif // echo_server.hh