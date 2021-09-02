#include "receiver_pool.hh"

#include <misc/net.hh>

int ReceiverPool::buffer_size_ = 0;

void ReceiverPool::Clear(int Socket) {
    start_ = 0;
    end_ = 0;
    receiver_array_[Socket] = nullptr;
}

void ReceiverPool::Init(int PoolSize, int BufferSize) {
    buffer_size_ = BufferSize;
    auto& buffer_array = GetInstance().buffer_array_;
    buffer_array.reserve(PoolSize);
    for (int i = 0; i < PoolSize; ++i) {
        auto new_pool = new ReceiverPool();
        new_pool->buffer_ = new unsigned char[BufferSize]();
        buffer_array.push_back(new_pool);
    }
}

ReceiverPool* ReceiverPool::GetBufferPoll() {
    for (auto& Item : buffer_array_) {
        if (Item->used_ == false) {
            Item->used_ = true;
            return Item;
        }
    }
    return nullptr;
}

ReceiverPool* ReceiverPool::GetPool(int Socket) {
    auto& receiver_array = GetInstance().receiver_array_;
    if (receiver_array[Socket] == nullptr) {
        receiver_array[Socket] = GetInstance().GetBufferPoll();
    }
    return receiver_array[Socket];
}

void ReceiverPool::Remove(int Socket) {
    auto& receiver_array = GetInstance().receiver_array_;
    LOG("Pool Remove: %d - %p", Socket, receiver_array[Socket]);
    if (receiver_array[Socket]) {
        receiver_array[Socket]->used_ = false;
        receiver_array[Socket] = nullptr;
    }
}

int ReceiverPool::Transfer(int Socket) {
    int send_len = send(Socket, buffer_, Usage(), 0);
    if (send_len <= 0) {
        if (errno == EAGAIN) {
            return send_len;
        }
        throw NetEx();
    }
    end_ -= send_len;
    return send_len;
}