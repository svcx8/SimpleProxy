#include "socket_pair.hh"

#include "dispatcher/epoller.hh"
#include "memory_buffer.hh"
#include "misc/logger.hh"
#include <stdint.h>

std::unordered_map<int, SocketPair*> SocketPairManager::socket_list_;
std::mutex SocketPairManager::list_mutex_;
std::atomic<int> SocketPairManager::last_poller_index_ = 1;

SocketPair::~SocketPair() {
    MemoryBuffer::RemovePool(this);
}

void SocketPairManager::AddPair(int port, int s) {
    auto pair = new SocketPair(port, s);
    socket_list_[port] = pair;
    LOG("[AddPair] [t#%d] port: %d socket: %d pair: %p", gettid(), port, s, pair);
    auto result = SocketPairManager::AcquireConnPoller(pair)->AddSocket(s,
                                                                        reinterpret_cast<uintptr_t>(pair),
                                                                        EPOLLIN);
    if (!result.ok()) {
        LOG("[SPM] [%d] Failed to add event", s);
        SocketPairManager::RemovePair(pair);
        return;
    }
}

void SocketPairManager::RemovePair(SocketPair* pair) {
    std::lock_guard<std::mutex> lock(list_mutex_);
    if (pair != nullptr && pair->conn_socket_) {
        LOG("[SPM] [t#%d] Remove pair: %d - %d", gettid(), pair->conn_socket_, pair->client_socket_);

        pair->conn_poller_->RemoveSocket(pair->conn_socket_).IgnoreError();
        close(pair->conn_socket_);

        if (pair->client_poller_) {
            pair->client_poller_->RemoveSocket(pair->client_socket_).IgnoreError();
            close(pair->client_socket_);
        }

        SocketPairManager::socket_list_.erase(pair->port_);
        pair->conn_socket_ = 0;
        delete pair;
    }
}

// Client Poller reserved_list_[0] && reserved_list_[1]
// Conn Poller reserved_list_[2] && reserved_list_[3]
// set reserved_list_[0] && reserved_list_[2] as a group

ProxyPoller* SocketPairManager::AcquireConnPoller(SocketPair* pair) {
    LOG("[SPM] [AcquireConnPoller] %d %d idx: %d", pair->conn_socket_, pair->client_socket_, last_poller_index_.load());
    pair->conn_poller_ = reinterpret_cast<ProxyPoller*>(EPoller::reserved_list_[last_poller_index_ + 1]);
    last_poller_index_ = last_poller_index_ % 2 + 1;
    /*
        last_poller_index_ = 1, 2, 1, 2, 1, 2......
    */
    return pair->conn_poller_;
}

ProxyPoller* SocketPairManager::AcquireClientPoller(SocketPair* pair) {
    LOG("[SPM] [AcquireClientPoller] %d %d idx: %d", pair->conn_socket_, pair->client_socket_, last_poller_index_.load());
    pair->client_poller_ = reinterpret_cast<ProxyPoller*>(EPoller::reserved_list_[last_poller_index_ - 1]);
    last_poller_index_ = last_poller_index_ % 2 + 1;
    /*
        last_poller_index_ = 1, 2, 1, 2, 1, 2......
    */
    return pair->client_poller_;
}