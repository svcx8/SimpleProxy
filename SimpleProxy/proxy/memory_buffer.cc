#include "memory_buffer.hh"

#include <misc/net.hh>
#include <misc/simple_pool.hh>

std::map<int, MemoryBuffer*> MemoryBuffer::buffer_array_;
static SimplePool<2, sizeof(MemoryBuffer)> memory_pool;

MemoryBuffer* MemoryBuffer::GetPool(int s) {
    if (buffer_array_[s] == nullptr) {
        auto allocated = static_cast<MemoryBuffer*>(memory_pool.Allocate());
        buffer_array_[s] = allocated;
    }
    return buffer_array_[s];
}

void MemoryBuffer::RemovePool(int s) {
    LOG("[--] Remove Pool: %02d - %p", s, buffer_array_[s]);
    if (buffer_array_[s]) {
        LOG("max_recv: %d", buffer_array_[s]->max_recv_len_);
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