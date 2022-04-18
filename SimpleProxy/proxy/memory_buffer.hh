#ifndef MEMORY_BUFFER_HEADER
#define MEMORY_BUFFER_HEADER

#include <map>
#include <mutex>

#include <absl/status/statusor.h>

#include "misc/logger.hh"
#include "socket_pair.hh"

class MemoryBuffer {
public:
    /*
        FreeChunk* Allocate();

        class FreeChunk {
            public:
                FreeChunk* next_ = nullptr;
                char data_[size];
            };
        }

        static_cast<MemoryBuffer*>(Allocate());

        // ~~~I don't know why it is worked before, but this is definitely a BUG.~~~
        // Not a bug, just a feature.
    */
    constexpr static int buffer_size_ = 409600;
    unsigned char buffer_[buffer_size_]; // -> FreeChunk* next_, that's ok because no more read it.
    int start_;                          // -> store in &buffer_[sizeof(next_)]
    int end_;                            // -> store in &buffer_[sizeof(next_) + sizeof(start_)]

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
    static std::mutex allocate_mutex_;

    absl::Status Receive(int s);
    absl::Status Send(int s);
    static std::map<int, MemoryBuffer*> buffer_array_;
};

#endif // memory_buffer.hh