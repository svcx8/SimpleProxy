#ifndef ECHO_CONN_HEADER
#define ECHO_CONN_HEADER

#include <dispatcher/ipoller.hh>

class EchoConn : public IBusinessEvent {
public:
    void OnAcceptable(SOCKET) override{};
    void OnCloseable(SOCKET) override;
    void OnReadable(SOCKET) override;
    void OnWritable(SOCKET) override;
};

#endif // echo_conn.hh