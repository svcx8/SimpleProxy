#ifdef __unix__

#include "epoller.hh"

#include <fcntl.h>

std::vector<IPoller*> EPoller::reserved_list_;

EPoller::EPoller(IBusinessEvent* business, int _id) : id_(_id) {
    epoller_inst_ = epoll_create1(0);
    op_ = business;
    op_->poller_ = this;
}

int EPoller::SetNonBlocking(int fd) {
    int old_opt = fcntl(fd, F_GETFL);
    int new_opt = old_opt | O_NONBLOCK;
    return fcntl(fd, F_SETFL, new_opt);
}

int EPoller::AddSocket(int s, long eventflags) {
    epoll_event _event;
    _event.data.fd = s;
    _event.events = eventflags;
    SetNonBlocking(s);
    int res = epoll_ctl(epoller_inst_, EPOLL_CTL_ADD, s, &_event);
    if (res == -1) {
        if (errno == EEXIST) {
            return epoll_ctl(epoller_inst_, EPOLL_CTL_MOD, s, &_event);
        } else {
            throw NetEx();
        }
    }
    return res;
}

void EPoller::RemoveCloseSocket(int s) {
    CloseSocket(s);
};

void EPoller::RemoveSocket(int s) {
    epoll_ctl(epoller_inst_, EPOLL_CTL_DEL, s, nullptr);
}

void EPoller::Poll() {
    int CompEventNum = epoll_wait(epoller_inst_, &event_array_[0], MAX_EVENT_NUMBER, -1);
    for (int i = 0; i < CompEventNum; ++i) {
        HandleEvents(event_array_[i].data.fd, event_array_[i].events);
    }
}

void EPoller::HandleEvents(int s, uint32_t event) {
    if (event & EPOLLIN) {
        // LOG("\t\t[#%d HandleEvents] OnReadable: %d", Id, s);
        op_->OnReadable(s);
    }

    else if (event & EPOLLOUT) {
        // LOG("\t\t[#%d HandleEvents] OnWritable: %d", Id, s);
        op_->OnWritable(s);
    }
}

#endif // #ifdef __unix__