#ifndef IPOLLER_HEADER
#define IPOLLER_HEADER

#include <absl/status/status.h>

#include "ibusiness_event.hh"

class IPoller {
public:
    IPoller(){};
    virtual ~IPoller(){};
    virtual absl::Status AddSocket(int s, long eventflags) = 0;
    virtual absl::Status ModSocket(int s, long eventflags) = 0;
    virtual absl::Status RemoveSocket(int s) = 0;
    // while (IsRunning) {
    virtual void Poll() = 0;
    // }
    void SetBusiness(IBusinessEvent* op) { op_ = op; }

protected:
    IBusinessEvent* op_ = nullptr;
};

#endif // ipoller.hh