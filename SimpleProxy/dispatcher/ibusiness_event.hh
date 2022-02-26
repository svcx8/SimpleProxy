#ifndef IBUSINESS_EVENT_HEADER
#define IBUSINESS_EVENT_HEADER

class IPoller;

class IBusinessEvent {
public:
    virtual ~IBusinessEvent() {}
    virtual void OnAcceptable(int){};
    virtual void OnCloseable(int){};
    virtual void OnReadable(int){};
    virtual void OnWritable(int){};
    IPoller* poller_ = nullptr;
};

#endif // ibusiness_event.hh