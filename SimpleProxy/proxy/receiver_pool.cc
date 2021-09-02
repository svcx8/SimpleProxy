#include "receiver_pool.hh"

#include <misc/net.hh>

int ReceiverPool::buffer_size_ = 0;

void ReceiverPool::Clear(int s) {
    start_ = 0;
    end_ = 0;
    receiver_array_[s] = nullptr;
}

void ReceiverPool::Init(int pool_size, int buffer_size) {
    buffer_size_ = buffer_size;
    auto& buffer_array = GetInstance().buffer_array_;
    buffer_array.reserve(pool_size);
    for (int i = 0; i < pool_size; ++i) {
        auto new_pool = new ReceiverPool();
        new_pool->buffer_ = new unsigned char[buffer_size]();
        buffer_array.push_back(new_pool);
    }
}

ReceiverPool* ReceiverPool::GetBufferPoll() {
    for (auto& item : buffer_array_) {
        if (item->used_ == false) {
            item->used_ = true;
            return item;
        }
    }
    return nullptr;
}

ReceiverPool* ReceiverPool::GetPool(int s) {
    auto& receiver_array = GetInstance().receiver_array_;
    if (receiver_array[s] == nullptr) {
        receiver_array[s] = GetInstance().GetBufferPoll();
    }
    return receiver_array[s];
}

void ReceiverPool::Remove(int s) {
    auto& receiver_array = GetInstance().receiver_array_;
    LOG("Pool Remove: %d - %p", s, receiver_array[s]);
    if (receiver_array[s]) {
        receiver_array[s]->used_ = false;
        receiver_array[s] = nullptr;
    }
}

int ReceiverPool::Transfer(int s) {
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