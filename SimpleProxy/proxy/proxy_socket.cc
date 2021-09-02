#include "proxy_socket.hh"

#include <algorithm>

void ProxySocket::AddPair(SocketPair* Pair) {
    SocketList.push_back(Pair);
}

void ProxySocket::RemovePair(int Socket) {
    SocketList.erase(std::remove_if(SocketList.begin(), SocketList.end(), [&](SocketPair* Pair) {
                         return Pair->this_side_ == Socket || Pair->other_side_ == Socket;
                     }),
                     SocketList.end());
}

int ProxySocket::GetSocket(int Socket) {
    for (auto& Item : SocketList) {
        if (Item->this_side_ == Socket)
            return Item->other_side_;
        if (Item->other_side_ == Socket)
            return Item->this_side_;
    }
    return 0;
}

SocketPair* ProxySocket::GetPointer(int Socket) {
    for (auto& Item : SocketList) {
        if (Item->this_side_ == Socket || Item->other_side_ == Socket)
            return Item;
    }
    return nullptr;
}