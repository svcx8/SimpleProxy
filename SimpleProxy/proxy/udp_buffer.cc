#include "udp_buffer.hh"

#include "misc/net.hh"
#include "misc/simple_pool.hh"

#include "misc/logger.hh"

std::unordered_map<int, UDPBuffer*> UDPBuffer::buffer_array_;
static SimplePool<10, sizeof(UDPBuffer)> memory_pool;

UDPBuffer* UDPBuffer::GetPool(int s) {
    auto ptr = buffer_array_[s];
    if (ptr == nullptr) {
        ptr = reinterpret_cast<UDPBuffer*>(memory_pool.Allocate());
        buffer_array_[s] = ptr;
    }
    return ptr;
}

void UDPBuffer::RemovePool(int s) {
    if (auto itor = buffer_array_.find(s); itor != buffer_array_.end()) {
        memory_pool.Revert(buffer_array_[s]);
        buffer_array_.erase(itor);
    }
}

absl::Status UDPBuffer::Receive(int s) {
    socklen_t len = sizeof(client_addr_);
    int recv_len = recvfrom(s, &buffer_[22], UDPBuffer::buffer_size_ - 22, 0, (sockaddr*)&client_addr_, &len);
    if (recv_len > 0) {
        end_ = 22 + recv_len;
        LOG("UDPBuffer::Receive: recv_len: %d from: %s:%d",
            recv_len,
            inet_ntoa(reinterpret_cast<sockaddr_in*>(&client_addr_)->sin_addr),
            htons(reinterpret_cast<sockaddr_in*>(&client_addr_)->sin_port));
    }

    else {
        return absl::InternalError(strerror(errno));
    }

    return absl::OkStatus();
}

absl::Status UDPBuffer::Send(int s, void* client, int client_len, int offset) {
    int send_len = sendto(s, &buffer_[offset], Usage() - offset, 0, (sockaddr*)client, client_len);
    if (send_len > 0) {
        LOG("UDPBuffer::Send: send_len: %d to: %s:%d",
            send_len,
            inet_ntoa(reinterpret_cast<sockaddr_in*>(client)->sin_addr),
            htons(reinterpret_cast<sockaddr_in*>(client)->sin_port));
    }

    else {
        return absl::InternalError(strerror(errno));
    }
    return absl::OkStatus();
}