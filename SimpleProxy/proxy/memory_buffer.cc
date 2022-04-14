#include "memory_buffer.hh"

#include "misc/logger.hh"
#include "misc/net.hh"
#include "misc/simple_pool.hh"
#include <absl/status/status.h>

std::map<int, MemoryBuffer*> MemoryBuffer::buffer_array_;
static SimplePool<10, sizeof(MemoryBuffer)> memory_pool;
static std::mutex allocate_mutex{};

MemoryBuffer* MemoryBuffer::GetPool(int s) {
    std::lock_guard<std::mutex> lock(allocate_mutex);
    if (buffer_array_[s] == nullptr) {
        buffer_array_[s] = static_cast<MemoryBuffer*>(memory_pool.Allocate());
    }
    return buffer_array_[s];
}

void MemoryBuffer::RemovePool(SocketPair* pair) {
    std::lock_guard<std::mutex> lock(allocate_mutex);
    MemoryBuffer*& pool_1 = buffer_array_[pair->this_side_];
    if (pool_1) {
        pool_1->start_ = pool_1->end_ = 0;
        memory_pool.Revert(pool_1);
        pool_1 = nullptr;
    }

    MemoryBuffer*& pool_2 = buffer_array_[pair->other_side_];
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
            LOG("[" LINE_FILE "] [%d] buffer full, %s", s, strerror(errno));
            return absl::ResourceExhaustedError("Buffer full.");
        }
        return absl::CancelledError("The socket was closed.");
    }

    else {
        if (errno == EAGAIN) {
            LOG("[" LINE_FILE "] [%d] EAGAIN: %s", s, strerror(errno));
            return absl::ResourceExhaustedError(strerror(errno));
        } else {
            LOG("[" LINE_FILE "] [%d] ERROR: %s", s, strerror(errno));
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
            LOG("[" LINE_FILE "] [%d] EAGAIN: %s", s, strerror(errno));
            return absl::ResourceExhaustedError(strerror(errno));
        } else {
            LOG("[" LINE_FILE "] [%d] ERROR: %s", s, strerror(errno));
            return absl::InternalError(strerror(errno));
        }
    }
    return absl::OkStatus();
}