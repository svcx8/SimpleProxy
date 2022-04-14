#include "socket_pair.hh"

#include "dispatcher/epoller.hh"
#include "memory_buffer.hh"
#include "misc/logger.hh"

std::map<int, SocketPair*> SocketPairManager::socket_list_;
std::mutex SocketPairManager::list_mutex_;
int SocketPairManager::last_poller_index_ = 1;

SocketPair::~SocketPair() {
    MemoryBuffer::RemovePool(this);
}

void SocketPairManager::AddPair(int port, int s) {
    std::lock_guard<std::mutex> lock(list_mutex_);
    auto pair = new SocketPair(port, s);
    socket_list_[port] = pair;
    auto result = SocketPairManager::GetConnPoller(pair)->AddSocket(s, EPOLLIN);
    if (!result.ok()) {
        LOG("[SocketPairManager] Failed to add event");
        close(s);
        SocketPairManager::RemovePair(pair);
        return;
    }
}

void SocketPairManager::RemovePair(SocketPair* pair) {
    std::lock_guard<std::mutex> lock(list_mutex_);
    if (pair != nullptr) {
        pair->this_poller_->RemoveSocket(pair->this_side_).IgnoreError();
        close(pair->this_side_);

        if (pair->other_poller_) {
            pair->other_poller_->RemoveSocket(pair->other_side_).IgnoreError();
            close(pair->other_side_);
        }

        SocketPairManager::socket_list_.erase(pair->port_);
        delete pair;
    }
}

SocketPair* SocketPairManager::GetPointer(int s) {
    std::lock_guard<std::mutex> lock(list_mutex_);
    for (const auto& item : socket_list_) {
        if (item.second->this_side_ == s || item.second->other_side_ == s)
            return item.second;
    }
    LOG("[SocketPairManager] The socket %d pair not found.", s);
    return {};
}

// Client Poller reserved_list_[0] && reserved_list_[1]
// Conn Poller reserved_list_[2] && reserved_list_[3]
// set reserved_list_[0] && reserved_list_[2] as a group

IPoller* SocketPairManager::GetConnPoller(SocketPair* pair) {
    if (pair->this_poller_ == nullptr) {
        LOG("[SocketPairManager::GetConnPoller] last_poller_index_: %d %d", pair->this_side_, last_poller_index_);
        pair->this_poller_ = EPoller::reserved_list_[last_poller_index_ + 1];
        last_poller_index_ = last_poller_index_ % 2 + 1;
        /*
            last_poller_index_ = 1, 2, 1, 2, 1, 2......
        */
        return pair->this_poller_;
    }

    else {
        return pair->this_poller_;
    }
}

IPoller* SocketPairManager::GetClientPoller(SocketPair* pair) {
    if (pair->other_poller_ == nullptr) {
        LOG("[SocketPairManager::GetClientPoller] last_poller_index_: %d %d", pair->other_side_, last_poller_index_);
        pair->other_poller_ = EPoller::reserved_list_[last_poller_index_ - 1];
        last_poller_index_ = last_poller_index_ % 2 + 1;
        /*
            last_poller_index_ = 1, 2, 1, 2, 1, 2......
        */
        return pair->other_poller_;
    }

    else {
        return pair->other_poller_;
    }
}