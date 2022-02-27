#ifndef MEMORY_BUFFER_HEADER
#define MEMORY_BUFFER_HEADER

#include <map>
#include <mutex>

#include <absl/status/statusor.h>

#include "misc/logger.hh"
#include "socket_pair.hh"

class MemoryBuffer {
public:
    constexpr static int buffer_size_ = 409600;
    unsigned char buffer_[buffer_size_];
    int start_ = 0;
    int end_ = 0;

    int Usage() {
        return end_ - start_;
    }
    absl::Status Receive(int s);
    absl::Status Send(int s);
    static MemoryBuffer* GetPool(int s);
    static void RemovePool(SocketPair* pair);

private:
    static std::map<int, MemoryBuffer*> buffer_array_;
};

#endif // memory_buffer.hh