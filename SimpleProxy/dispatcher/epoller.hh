#ifndef EPOLLER_HEADER
#define EPOLLER_HEADER

#ifdef __unix__

#include "ipoller.hh"
#include "proxy/socks5.hh"

#include <sys/epoll.h>

#include <array>
#include <vector>

constexpr int MAX_EVENT_NUMBER = 64;

class EPoller : public IPoller {
public:
    int id_ = 0;
    EPoller(IBusinessEvent* business, int _id);
    ~EPoller(){};

    absl::Status AddSocket(int s, long eventflags) override;
    void RemoveCloseSocket(int s) override;

    void Poll() override;
    void HandleEvents(int s, uint32_t event);
    std::array<epoll_event, MAX_EVENT_NUMBER> event_array_{};

    static int SetNonBlocking(int);
    static std::vector<IPoller*> reserved_list_; // SubReactor list

private:
    int epoller_inst_ = 0;
};

#endif // #ifdef __unix__

#endif // epoller.hh