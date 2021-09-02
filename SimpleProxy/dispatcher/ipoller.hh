#ifndef IPOLLER_HEADER
#define IPOLLER_HEADER

#include "ibusiness_event.hh"

class IPoller {
public:
    IPoller(){};
    virtual ~IPoller(){};
    virtual int AddSocket(int s, long eventflags) = 0;
    virtual void RemoveSocket(int s) = 0;
    virtual void RemoveCloseSocket(int s) = 0;
    // while (IsRunning) {
    virtual void Poll() = 0;
    // }
    void SetBusiness(IBusinessEvent* op) { op_ = op; }

protected:
    IBusinessEvent* op_ = nullptr;
};

#endif // ipoller.hh