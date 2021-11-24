#ifdef __unix__

#include "epoller.hh"
#include "misc/logger.hh"

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

absl::Status EPoller::AddSocket(int s, long eventflags) {
    epoll_event _event;
    _event.data.fd = s;
    _event.events = eventflags;
    SetNonBlocking(s);
    int res = epoll_ctl(epoller_inst_, EPOLL_CTL_ADD, s, &_event);
    if (res == -1) {
        if (errno == EEXIST) {
            // I'm not sure I need to check the return value here.
            epoll_ctl(epoller_inst_, EPOLL_CTL_MOD, s, &_event);
        } else {
            return absl::InternalError(strerror(errno));
        }
    }
    return absl::OkStatus();
}

void EPoller::RemoveCloseSocket(int s) {
    close(s);
    // if (epoll_ctl(epoller_inst_, EPOLL_CTL_DEL, s, nullptr)) {
    //     LOG("[RemoveCloseSocket]: %s", strerror(errno)); // return Bad file descriptor
    // }
};

void EPoller::Poll() {
    int CompEventNum = epoll_wait(epoller_inst_, &event_array_[0], MAX_EVENT_NUMBER, 30000);
    for (int i = 0; i < CompEventNum; ++i) {
        HandleEvents(event_array_[i].data.fd, event_array_[i].events);
    }
}

void EPoller::HandleEvents(int s, uint32_t event) {
    if (event & EPOLLIN) {
        auto result = op_->OnReadable(s);
        if (!result.ok()) {
            ERROR("[EPoller::HandleEvents::OnReadable] %.*s", result.message().size(), result.message().data());
        }
    }

    else if (event & EPOLLOUT) {
        auto result = op_->OnWritable(s);
        if (!result.ok()) {
            ERROR("[EPoller::HandleEvents::OnWritable] %.*s", result.message().size(), result.message().data());
        }
    }
}

#endif // #ifdef __unix__