#include "socket_pair.hh"

#include <thread>

#include "dispatcher/epoller.hh"
#include "memory_buffer.hh"
#include "misc/logger.hh"

std::unordered_map<int, SocketPair*> SocketPairManager::socket_list_;
std::shared_mutex SocketPairManager::list_mutex_;
std::atomic<int> SocketPairManager::last_poller_index_ = 1;

SocketPair::~SocketPair() {
    MemoryBuffer::RemovePool(this);
}

void SocketPairManager::AddPair(int port, int s) {
    std::lock_guard<std::shared_mutex> lock(list_mutex_);
    auto pair = new SocketPair(port, s);

    LOG("[SPM] [t#%d] Add pair: %d - %d port: %d ", gettid(), pair->conn_socket_, pair->client_socket_, port);
    auto result = SocketPairManager::AcquireConnPoller(pair)->AddSocket(s,
                                                                        reinterpret_cast<uintptr_t>(pair),
                                                                        EPOLLIN);
    if (!result.ok()) {
        LOG("[SPM] [%d] Failed to add event", s);
        delete pair;
        return;
    }
    socket_list_[port] = pair;
}

void SocketPairManager::RemoveConnPair(SocketPair* pair) {
    LOG("[SPM] [t#%d] Remove Conn pair: %d - %d port: %d", gettid(), pair->conn_socket_, pair->client_socket_, pair->port_);

    close(pair->conn_socket_);
    pair->conn_socket_ = -233;

    if (pair->client_socket_ > 0) {
        close(pair->client_socket_);
        pair->client_socket_ = -233;
    }
}

void SocketPairManager::RemoveClientPair(SocketPair* pair) {
    LOG("[SPM] [t#%d] Remove Client pair: %d - %d port: %d", gettid(), pair->conn_socket_, pair->client_socket_, pair->port_);

    close(pair->client_socket_);
    pair->client_socket_ = -233;

    if (pair->conn_socket_ > 0) {
        close(pair->conn_socket_);
        pair->conn_socket_ = -233;
    }
}

// Client Poller reserved_list_[0] && reserved_list_[1]
// Conn Poller reserved_list_[2] && reserved_list_[3]
// set reserved_list_[0] && reserved_list_[2] as a group

ProxyPoller* SocketPairManager::AcquireConnPoller(SocketPair* pair) {
    pair->conn_poller_ = reinterpret_cast<ProxyPoller*>(EPoller::reserved_list_[last_poller_index_ + 1]);
    last_poller_index_ = last_poller_index_ % 2 + 1;
    /*
        last_poller_index_ = 1, 2, 1, 2, 1, 2......
    */
    LOG("[SPM] [AcquireConnPoller] [t#%d] %d %d poller: %p", gettid(), pair->conn_socket_, pair->client_socket_, pair->conn_poller_);
    return pair->conn_poller_;
}

ProxyPoller* SocketPairManager::AcquireClientPoller(SocketPair* pair) {
    pair->client_poller_ = reinterpret_cast<ProxyPoller*>(EPoller::reserved_list_[last_poller_index_ - 1]);
    last_poller_index_ = last_poller_index_ % 2 + 1;
    /*
        last_poller_index_ = 1, 2, 1, 2, 1, 2......
    */
    LOG("[SPM] [AcquireClientPoller] [t#%d] %d %d poller: %p", gettid(), pair->conn_socket_, pair->client_socket_, pair->client_poller_);
    return pair->client_poller_;
}

void SocketPairManager::StartPrugeThread() {
    std::thread([]() {
        while (true) {
            std::this_thread::sleep_for(std::chrono::seconds(30));
            std::lock_guard<std::shared_mutex> lock(list_mutex_);
            int count = 0;
            for (auto it = socket_list_.begin(); it != socket_list_.end();) {
                if (it->second->conn_socket_ == -233 && it->second->client_socket_ == -233) {
                    delete it->second;
                    it = socket_list_.erase(it);
                    count++;
                } else {
                    ++it;
                }
            }
            LOG("[SPM] [t#%d] Prune %d pairs, left: %ld", gettid(), count, socket_list_.size());
        }
    }).detach();
}