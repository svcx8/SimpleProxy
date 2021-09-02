#ifdef __unix__

#include "epoller.hh"

#include <fcntl.h>

std::vector<IPoller*> EPoller::reserved_list_;

EPoller::EPoller(PollType type, int _id) : id_(_id) {
    epoller_inst_ = epoll_create1(0);
    switch (type) {
    case tEchoConn:
        op_ = new EchoConn();
        op_->poller_ = this;
        break;
    case tEchoServer:
        op_ = new EchoServer();
        op_->poller_ = this;
        break;
    case tProxyClient:
        op_ = new ProxyClient();
        op_->poller_ = this;
        break;
    case tProxyServer:
        op_ = new ProxyServer();
        op_->poller_ = this;
        break;
    case tProxyConn:
        op_ = new ProxyConn();
        op_->poller_ = this;
        break;
    default:
        LOG("PollType Not Matched.");
        throw MyEx("PollType Not Matched.");
    }
}

int EPoller::SetNonBlocking(int fd) {
    int OldOpt = fcntl(fd, F_GETFL);
    int NewOpt = OldOpt | O_NONBLOCK;
    fcntl(fd, F_SETFL, NewOpt);
    return OldOpt;
}

int EPoller::AddSocket(SOCKET Socket, long eventflags) {
    epoll_event _event;
    _event.data.fd = Socket;
    _event.events = eventflags;
    SetNonBlocking(Socket);
    auto res = epoll_ctl(epoller_inst_, EPOLL_CTL_ADD, Socket, &_event);
    if (res == -1) {
        if (errno == EEXIST) {
            return epoll_ctl(epoller_inst_, EPOLL_CTL_MOD, Socket, &_event);
        } else {
            throw NetEx();
        }
    }
    return res;
}

void EPoller::RemoveCloseSocket(SOCKET Socket) {
    CloseSocket(Socket);
};

void EPoller::RemoveSocket(SOCKET Socket) {
    epoll_ctl(epoller_inst_, EPOLL_CTL_DEL, Socket, nullptr);
}

void EPoller::Poll() {
    int CompEventNum = epoll_wait(epoller_inst_, &EventArray[0], MAX_EVENT_NUMBER, -1);
    for (int i = 0; i < CompEventNum; ++i) {
        HandleEvents(EventArray[i].data.fd, EventArray[i].events);
    }
}

void EPoller::HandleEvents(SOCKET Socket, uint32_t Event) {
    if (Event & EPOLLIN) {
        // LOG("\t\t[#%d HandleEvents] OnReadable: %d", Id, Socket);
        op_->OnReadable(Socket);
    }

    else if (Event & EPOLLOUT) {
        // LOG("\t\t[#%d HandleEvents] OnWritable: %d", Id, Socket);
        op_->OnWritable(Socket);
    }
}

#endif // #ifdef __unix__