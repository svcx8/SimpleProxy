#ifndef IBUSINESS_EVENT_HEADER
#define IBUSINESS_EVENT_HEADER

#include <misc/net.hh>

#include <cstdint>

class IPoller;

class IBusinessEvent {
public:
    virtual ~IBusinessEvent() {}
    virtual void OnAcceptable(SOCKET) = 0;
    virtual void OnCloseable(SOCKET) = 0;
    virtual void OnReadable(SOCKET) = 0;
    virtual void OnWritable(SOCKET) = 0;
    IPoller* poller_ = nullptr;
};

#endif // ibusiness_event.hh