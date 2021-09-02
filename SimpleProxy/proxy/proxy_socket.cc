#include "proxy_socket.hh"

#include <algorithm>

void ProxySocket::AddPair(SocketPair* pair) {
    socket_list_.push_back(pair);
}

void ProxySocket::RemovePair(int s) {
    socket_list_.erase(std::remove_if(socket_list_.begin(), socket_list_.end(), [&](SocketPair* pair) {
                           return pair->this_side_ == s || pair->other_side_ == s;
                       }),
                       socket_list_.end());
}

int ProxySocket::GetSocket(int s) {
    for (auto& item : socket_list_) {
        if (item->this_side_ == s)
            return item->other_side_;
        if (item->other_side_ == s)
            return item->this_side_;
    }
    return 0;
}

SocketPair* ProxySocket::GetPointer(int s) {
    for (auto& item : socket_list_) {
        if (item->this_side_ == s || item->other_side_ == s)
            return item;
    }
    return nullptr;
}