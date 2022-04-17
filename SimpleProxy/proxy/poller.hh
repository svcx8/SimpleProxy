#ifndef POLLER_HEADER
#define POLLER_HEADER

#include "dispatcher/epoller.hh"

#include "misc/logger.hh"

class ProxyPoller : public EPoller {
public:
    using EPoller::EPoller;
    absl::Status AddSocket(int s, uintptr_t socket_pair, long eventflags) {
        epoll_event _event;
        _event.data.u64 = socket_pair;
        _event.events = eventflags;
        SetNonBlocking(s);
        LOG("[AddSocket] [#%d] [%d] %ld pair: %lX", gettid(), s, eventflags, socket_pair);
        if (epoll_ctl(epoller_inst_, EPOLL_CTL_ADD, s, &_event) == -1) {
            LOG("[AddSocket] [%d] %s", s, strerror(errno));
            return absl::InternalError(strerror(errno));
        }
        return absl::OkStatus();
    }

    absl::Status ModSocket(int s, uintptr_t socket_pair, long eventflags) {
        epoll_event _event;
        _event.data.u64 = socket_pair;
        _event.events = eventflags;
        LOG("[ModSocket] [#%d] [%d] %ld", gettid(), s, eventflags);
        if (epoll_ctl(epoller_inst_, EPOLL_CTL_MOD, s, &_event) == -1) {
            LOG("[ModSocket] [%d] %s", s, strerror(errno));
            return absl::InternalError(strerror(errno));
        }
        return absl::OkStatus();
    }
};

#endif // poller.hh