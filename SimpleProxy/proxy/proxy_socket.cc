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

    auto itor = std::find_if(socket_list_.begin(), socket_list_.end(),
                             [&](std::unique_ptr<SocketPair> const& pair) {
                                 return pair->this_side_ == s || pair->other_side_ == s;
                             });

    if (itor != socket_list_.end()) {
        close(itor->get()->this_side_);
        close(itor->get()->other_side_);
        MemoryBuffer::RemovePool(itor->get());
        socket_list_.erase(itor);
    }
}

SocketPair* ProxySocket::GetPointer(int s) {
    std::unique_lock<std::mutex> lock(list_mutex_);
    for (const auto& item : socket_list_) {
        if (item->this_side_ == s || item->other_side_ == s)
            return item.get();
    }
    ERROR("[ProxySocket] The socket %d pair not found.", s);
    return nullptr;
}

// Client Poller reserved_list_[0] && reserved_list_[1]
// Conn Poller reserved_list_[2] && reserved_list_[3]
// set reserved_list_[0] && reserved_list_[2] as a group

IPoller* ProxySocket::GetConnPoller(SocketPair* pair) {
    if (pair->poller_index == 0) {
        IPoller* ptr = nullptr;
        pair->poller_index = last_poller_index_;
        LOG("[ProxySocket::GetConnPoller] last_poller_index_: %d", last_poller_index_);

        ptr = EPoller::reserved_list_[last_poller_index_ + 1];
        last_poller_index_ = last_poller_index_ % 2 + 1;
        /*
            last_poller_index_ = 1, 2, 1, 2, 1, 2......
        */
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
        LOG("[ProxySocket::GetClientPoller] last_poller_index_: %d", last_poller_index_);

        ptr = EPoller::reserved_list_[last_poller_index_ - 1];
        last_poller_index_ = last_poller_index_ % 2 + 1;
        /*
            last_poller_index_ = 1, 2, 1, 2, 1, 2......
        */
        return ptr;
    }

    else {
        return EPoller::reserved_list_[pair->poller_index - 1];
    }
}