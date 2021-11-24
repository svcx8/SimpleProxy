#ifndef MEMORY_BUFFER_HEADER
#define MEMORY_BUFFER_HEADER

#include "proxy_socket.hh"

#include <map>
#include <mutex>

class MemoryBuffer {
public:
    constexpr static int buffer_size_ = 409600;
    unsigned char buffer_[buffer_size_];
    int start_ = 0;
    int end_ = 0;
    int max_recv_len_ = 0;

    int Usage() {
        return end_ - start_;
    }
    absl::Status Transfer(int s);
    static MemoryBuffer* GetPool(int s);
    static void RemovePool(SocketPair* pair);

private:
    static std::map<int, MemoryBuffer*> buffer_array_;
    static std::mutex pool_mutex_;
};

#endif // memory_buffer.hh