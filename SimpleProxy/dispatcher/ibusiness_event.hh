#ifndef IBUSINESS_EVENT_HEADER
#define IBUSINESS_EVENT_HEADER

#include <cstdint>

#include <absl/status/status.h>

class IPoller;

class IBusinessEvent {
public:
    virtual ~IBusinessEvent() {}
    virtual absl::Status OnAcceptable(int) { return absl::OkStatus(); };
    virtual absl::Status OnCloseable(int) { return absl::OkStatus(); };
    virtual absl::Status OnReadable(int) { return absl::OkStatus(); };
    virtual absl::Status OnWritable(int) { return absl::OkStatus(); };
    IPoller* poller_ = nullptr;
};

#endif // ibusiness_event.hh