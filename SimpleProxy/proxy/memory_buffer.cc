#include "memory_buffer.hh"

#include "misc/logger.hh"
#include "misc/net.hh"
#include "misc/simple_pool.hh"
#include "socket_pair.hh"

static SimplePool<10, sizeof(MemoryBuffer)> memory_pool;

MemoryBuffer* MemoryBuffer::AcquirePool() {
    return reinterpret_cast<MemoryBuffer*>(memory_pool.Allocate());
}

void MemoryBuffer::RevertPool(MemoryBuffer* buffer) {
    memory_pool.Revert(buffer);
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