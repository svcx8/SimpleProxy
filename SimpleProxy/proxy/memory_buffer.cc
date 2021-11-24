#include "memory_buffer.hh"

#include "misc/logger.hh"
#include "misc/net.hh"
#include "misc/simple_pool.hh"

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

void MemoryBuffer::RemovePool(SocketPair* pair) {
    if (buffer_array_[pair->this_side_]) {
        memory_pool.Revert(buffer_array_[pair->this_side_]);
        buffer_array_[pair->this_side_] = nullptr;
    }

    if (buffer_array_[pair->other_side_]) {
        memory_pool.Revert(buffer_array_[pair->other_side_]);
        buffer_array_[pair->other_side_] = nullptr;
    }
}

absl::Status MemoryBuffer::Transfer(int s) {
    int send_len = send(s, buffer_, Usage(), 0);
    if (send_len == SOCKET_ERROR) {
        if (errno != EAGAIN) {
            return absl::InternalError(strerror(errno));
        }
    }
    end_ -= send_len;
    return absl::OkStatus();
}