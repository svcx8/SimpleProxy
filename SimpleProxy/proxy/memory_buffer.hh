#ifndef MEMORY_BUFFER_HEADER
#define MEMORY_BUFFER_HEADER

#include <map>
#include <mutex>

#include <absl/status/statusor.h>

#include "misc/logger.hh"
#include "socket_pair.hh"

class MemoryBuffer {
public:
    int Usage() {
        return end_ - start_;
    }

    absl::Status ReceiveFromClient(int s) {
        auto&& res = Receive(s);
        if (!res.ok()) {
            LOG("[" LINE_FILE "] [%d] ERROR: %s", s, strerror(errno));
        }
        return res;
    }
    absl::Status RecvFromServer(int s) {
        auto&& res = Receive(s);
        if (!res.ok()) {
            LOG("[" LINE_FILE "] [%d] ERROR: %s", s, strerror(errno));
        }
        return res;
    }

    absl::Status ProxyConn_SendToServer(int s) {
        auto&& res = Send(s);
        if (!res.ok()) {
            LOG("[" LINE_FILE "] [%d] ERROR: %s", s, strerror(errno));
        }
        return res;
    }
    absl::Status ProxyClient_SendToServer(int s) {
        auto&& res = Send(s);
        if (!res.ok()) {
            LOG("[" LINE_FILE "] [%d] ERROR: %s", s, strerror(errno));
        }
        return res;
    }

    absl::Status ProxyConn_SendToClient(int s) {
        auto&& res = Send(s);
        if (!res.ok()) {
            LOG("[" LINE_FILE "] [%d] ERROR: %s", s, strerror(errno));
        }
        return res;
    }
    absl::Status ProxyClient_SendToClient(int s) {
        auto&& res = Send(s);
        if (!res.ok()) {
            LOG("[" LINE_FILE "] [%d] ERROR: %s", s, strerror(errno));
        }
        return res;
    }

    static MemoryBuffer* GetPool(int s);
    static void RemovePool(SocketPair* pair);

private:
    int start_ = 0;
    int end_ = 0;

    constexpr static int buffer_size_ = 409600;
    unsigned char buffer_[buffer_size_];

    static std::mutex allocate_mutex_;

    absl::Status Receive(int s);
    absl::Status Send(int s);
    static std::map<int, MemoryBuffer*> buffer_array_;
};

#endif // memory_buffer.hh