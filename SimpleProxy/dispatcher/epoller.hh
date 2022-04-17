#ifndef EPOLLER_HEADER
#define EPOLLER_HEADER

#ifdef __linux__

#include <sys/epoll.h>

#include <array>
// #include <string>
#include <vector>

#include "ipoller.hh"

constexpr int MAX_EVENT_NUMBER = 64;

class EPoller : public IPoller {
public:
    int id_ = 0;

    EPoller(IBusinessEvent* business, int _id);
    ~EPoller(){};

    absl::Status AddSocket(int s, long eventflags) override;
    absl::Status ModSocket(int s, long eventflags) override;
    absl::Status RemoveSocket(int s) override;

    void Poll() override;
    void HandleEvents(uintptr_t s, uint32_t event);

    std::array<epoll_event, MAX_EVENT_NUMBER> event_array_{};

    static int SetNonBlocking(int);
    static std::vector<IPoller*> reserved_list_; // SubReactor list

protected:
    int epoller_inst_ = 0;
};

#endif // #ifdef __linux__

#endif // epoller.hh