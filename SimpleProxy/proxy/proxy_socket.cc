#include "proxy_socket.hh"

#include "dispatcher/epoller.hh"
#include "memory_buffer.hh"
#include "misc/logger.hh"

#include <algorithm>
#include <mutex>

std::mutex ProxySocket::list_mutex_;
std::mutex ProxySocket::get_poller_mutex_;
int ProxySocket::last_poller_index_ = 1;

void ProxySocket::AddPair(std::unique_ptr<SocketPair>&& pair) {
    std::unique_lock<std::mutex> lock(list_mutex_);
    socket_list_.push_back(std::move(pair));
}

void ProxySocket::RemovePair(int s) {
    std::unique_lock<std::mutex> lock(list_mutex_);
    socket_list_.erase(std::remove_if(socket_list_.begin(), socket_list_.end(), [&](std::unique_ptr<SocketPair> const& pair) {
                           return pair->this_side_ == s;
                       }),
                       socket_list_.end());
}

SocketPair* ProxySocket::GetPointer(int s) {
    std::unique_lock<std::mutex> lock(list_mutex_);
    for (const auto& item : socket_list_) {
        if (item->this_side_ == s || item->other_side_ == s)
            return item.get();
    }
    ERROR("[%s] The socket %d pair not found.", __FUNCTION__, s);
    return nullptr;
}

// Client Poller reserved_list_[0] && reserved_list_[1]
// Conn Poller reserved_list_[2] && reserved_list_[3]
// set reserved_list_[0] && reserved_list_[2] as a group

IPoller* ProxySocket::GetConnPoller(SocketPair* pair) {
    if (pair->poller_index == 0) {
        IPoller* ptr = nullptr;
        pair->poller_index = last_poller_index_;
        if (last_poller_index_ == 1) {
            ptr = EPoller::reserved_list_[2];
            ++last_poller_index_;
        } else {
            ptr = EPoller::reserved_list_[3];
            last_poller_index_ = 1;
        }
        return ptr;
    }

    else {
        return EPoller::reserved_list_[pair->poller_index + 1];
    }
}

IPoller* ProxySocket::GetClientPoller(SocketPair* pair) {
    if (pair->poller_index == 0) {
        IPoller* ptr = nullptr;
        pair->poller_index = last_poller_index_;
        if (last_poller_index_ == 1) {
            ptr = EPoller::reserved_list_[0];
            ++last_poller_index_;
        } else {
            ptr = EPoller::reserved_list_[1];
            last_poller_index_ = 1;
        }
        return ptr;
    }

    else {
        return EPoller::reserved_list_[pair->poller_index - 1];
    }
}

void ProxySocket::ClosePair(SocketPair* pair) {
    LOG("[ProxySocket] ClosePair: %d - %d", pair->this_side_, pair->other_side_);
    MemoryBuffer::RemovePool(pair);
    ProxySocket::GetInstance().RemovePair(pair->this_side_);
    close(pair->this_side_);
    close(pair->other_side_);
}