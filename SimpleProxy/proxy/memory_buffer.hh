#ifndef MEMORY_BUFFER_HEADER
#define MEMORY_BUFFER_HEADER

#include <mutex>
#include <unordered_map>

#include <absl/status/status.h>
#include <absl/status/statusor.h>

#include "misc/logger.hh"

class SocketPair;

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
            LOG("[L#%d] [%d] ERROR: sys: %s absl: %s", __LINE__, s, strerror(errno), res.ToString().c_str());
        }
        return res;
    }
    absl::Status RecvFromServer(int s) {
        auto&& res = Receive(s);
        if (!res.ok()) {
            LOG("[L#%d] [%d] ERROR: sys: %s absl: %s", __LINE__, s, strerror(errno), res.ToString().c_str());
        }
        return res;
    }

    absl::Status ProxyConn_SendToServer(int s) {
        auto&& res = Send(s);
        if (!res.ok()) {
            LOG("[L#%d] [%d] ERROR: sys: %s absl: %s", __LINE__, s, strerror(errno), res.ToString().c_str());
        }
        return res;
    }
    absl::Status ProxyClient_SendToServer(int s) {
        auto&& res = Send(s);
        if (!res.ok()) {
            LOG("[L#%d] [%d] ERROR: sys: %s absl: %s", __LINE__, s, strerror(errno), res.ToString().c_str());
        }
        return res;
    }

    absl::Status ProxyConn_SendToClient(int s) {
        auto&& res = Send(s);
        if (!res.ok()) {
            LOG("[L#%d] [%d] ERROR: sys: %s absl: %s", __LINE__, s, strerror(errno), res.ToString().c_str());
        }
        return res;
    }
    absl::Status ProxyClient_SendToClient(int s) {
        auto&& res = Send(s);
        if (!res.ok()) {
            LOG("[L#%d] [%d] ERROR: sys: %s absl: %s", __LINE__, s, strerror(errno), res.ToString().c_str());
        }
        return res;
    }

    static MemoryBuffer* AcquirePool();
    static void RevertPool(MemoryBuffer* buffer);

private:
    absl::Status Receive(int s);
    absl::Status Send(int s);
};

#endif // memory_buffer.hh