#ifdef __unix__

#include "epoller.hh"
#include <misc/logger.hh>

std::vector<IPoller*> EPoller::ReservedList;

EPoller::EPoller(PollType type, int _id) : Id(_id) {
    EPollInst = epoll_create1(0);
    switch (type) {
    case tEchoConn:
        Op = new EchoConn();
        Op->Poller = this;
        break;
    case tEchoServer:
        Op = new EchoServer();
        Op->Poller = this;
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
    auto res = epoll_ctl(EPollInst, EPOLL_CTL_ADD, Socket, &_event);
    if (res == -1) {
        if (errno == EEXIST) {
            return epoll_ctl(EPollInst, EPOLL_CTL_MOD, Socket, &_event);
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
    epoll_ctl(EPollInst, EPOLL_CTL_DEL, Socket, nullptr);
}

void EPoller::Poll() {
    int CompEventNum = epoll_wait(EPollInst, &EventArray[0], MAX_EVENT_NUMBER, -1);
    for (int i = 0; i < CompEventNum; ++i) {
        HandleEvents(EventArray[i].data.fd, EventArray[i].events);
    }
}

void EPoller::HandleEvents(SOCKET Socket, uint32_t Event) {
    if (Event & EPOLLIN) {
        // LOG("\t\t[#%d HandleEvents] OnReadable: %d", Id, Socket);
        Op->OnReadable(Socket);
    }

    else if (Event & EPOLLOUT) {
        // LOG("\t\t[#%d HandleEvents] OnWritable: %d", Id, Socket);
        Op->OnWritable(Socket);
    }
}

#endif // #ifdef __unix__