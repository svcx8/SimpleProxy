#include "memory_buffer.hh"

#include "misc/logger.hh"
#include "misc/net.hh"
#include "misc/simple_pool.hh"
#include <absl/status/status.h>

std::map<int, MemoryBuffer*> MemoryBuffer::buffer_array_;
static SimplePool<10, sizeof(MemoryBuffer)> memory_pool;
std::mutex MemoryBuffer::allocate_mutex_;

MemoryBuffer* MemoryBuffer::GetPool(int s) {
    std::lock_guard<std::mutex> lock(allocate_mutex_);
    auto ptr = buffer_array_[s];
    if (ptr == nullptr) {
        ptr = reinterpret_cast<MemoryBuffer*>(memory_pool.Allocate());
        // LOG("Allocate MemoryBuffer: %p %d %d", ptr, ptr->start_, ptr->end_);
        buffer_array_[s] = ptr;
    }
    return ptr;
}

void MemoryBuffer::RemovePool(SocketPair* pair) {
    std::lock_guard<std::mutex> lock(allocate_mutex_);
    MemoryBuffer*& pool_1 = buffer_array_[pair->conn_socket_];
    if (pool_1) {
        pool_1->start_ = pool_1->end_ = 0;
        memory_pool.Revert(pool_1);
        pool_1 = nullptr;
    }

    MemoryBuffer*& pool_2 = buffer_array_[pair->client_socket_];
    if (pool_2) {
        pool_2->start_ = pool_2->end_ = 0;
        memory_pool.Revert(pool_2);
        pool_2 = nullptr;
    }
}

absl::Status MemoryBuffer::Receive(int s) {
    int recv_len = recv(s, &buffer_[end_], MemoryBuffer::buffer_size_ - end_, 0);
    if (recv_len > 0) {
        end_ += recv_len;
    }

    else if (recv_len == 0) {
        if (end_ == MemoryBuffer::buffer_size_) {
            return absl::ResourceExhaustedError("Buffer full.");
        }
        return absl::CancelledError("The socket was closed.");
    }

    else {
        if (errno == EAGAIN) {
            return absl::ResourceExhaustedError(strerror(errno));
        } else {
            return absl::InternalError(strerror(errno));
        }
    }

    return absl::OkStatus();
}

absl::Status MemoryBuffer::Send(int s) {
    int send_len = send(s, &buffer_[start_], Usage(), 0);
    if (send_len > 0) {
        start_ += send_len;
        if (start_ != 0 && start_ == end_) {
            start_ = end_ = 0;
        }
    }

    else if (send_len == 0) {
        return absl::CancelledError("The socket was closed.");
    }

    else if (send_len < 0) {
        if (errno == EAGAIN) {
            return absl::ResourceExhaustedError(strerror(errno));
        } else {
            return absl::InternalError(strerror(errno));
        }
    }
    return absl::OkStatus();
}