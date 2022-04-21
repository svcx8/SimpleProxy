#include "memory_buffer.hh"

#include "misc/logger.hh"
#include "misc/net.hh"
#include "misc/simple_pool.hh"

std::unordered_map<int, MemoryBuffer*> MemoryBuffer::buffer_array_;
static SimplePool<10, sizeof(MemoryBuffer)> memory_pool;
std::mutex MemoryBuffer::allocate_mutex_;

MemoryBuffer* MemoryBuffer::GetPool(int s) {
    std::lock_guard<std::mutex> lock(allocate_mutex_);
    MemoryBuffer* ptr = nullptr;
    if (auto itor = buffer_array_.find(s); itor != buffer_array_.end()) {
        ptr = itor->second;
        buffer_array_[s] = ptr;
    }

    else {
        ptr = reinterpret_cast<MemoryBuffer*>(memory_pool.Allocate());
        buffer_array_[s] = ptr;
    }
    return ptr;
}

void MemoryBuffer::RemovePool(SocketPair* pair) {
    std::lock_guard<std::mutex> lock(allocate_mutex_);
    if (auto pool_1 = buffer_array_.find(pair->conn_socket_); pool_1 != buffer_array_.end()) {
        memory_pool.Revert(pool_1->second);
        buffer_array_.erase(pool_1);
    }

    if (auto pool_2 = buffer_array_.find(pair->client_socket_); pool_2 != buffer_array_.end()) {
        memory_pool.Revert(pool_2->second);
        buffer_array_.erase(pool_2);
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