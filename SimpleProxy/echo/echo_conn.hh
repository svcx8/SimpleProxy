#ifndef ECHO_CONN_HEADER
#define ECHO_CONN_HEADER

#include "dispatcher/ipoller.hh"

class EchoConn : public IBusinessEvent {
public:
    absl::Status OnAcceptable(int) override{};
    absl::Status OnCloseable(int) override;
    absl::Status OnReadable(int) override;
    absl::Status OnWritable(int) override;
};

#endif // echo_conn.hh