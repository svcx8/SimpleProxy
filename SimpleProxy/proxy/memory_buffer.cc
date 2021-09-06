#include "memory_buffer.hh"

#include <misc/net.hh>
#include <misc/simple_pool.hh>

std::map<int, MemoryBuffer*> MemoryBuffer::buffer_array_;
static SimplePool<10, sizeof(MemoryBuffer)> memory_pool;
std::mutex MemoryBuffer::pool_mutex_;

MemoryBuffer* MemoryBuffer::GetPool(int s) {
    if (buffer_array_[s] == nullptr) {
        std::unique_lock<std::mutex> lock(pool_mutex_);
        buffer_array_[s] = static_cast<MemoryBuffer*>(memory_pool.Allocate());
    }
    return buffer_array_[s];
}

void MemoryBuffer::RemovePool(int s) {
    LOG("[--] Remove Pool: %04d", s);
    if (buffer_array_[s]) {
        memory_pool.Revert(buffer_array_[s]);
        buffer_array_[s] = nullptr;
    }
}

int MemoryBuffer::Transfer(int s) {
    int send_len = send(s, buffer_, Usage(), 0);
    if (send_len <= 0) {
        if (errno == EAGAIN) {
            return send_len;
        }
        throw NetEx();
    }
    end_ -= send_len;
    return send_len;
}