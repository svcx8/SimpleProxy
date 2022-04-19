#include "udp_server.hh"

#include <sys/socket.h>
#include <thread>

#include "misc/net.hh"
#include "proxy/socket_pair.hh"
#include "proxy/udp_associate.hh"

void UDPServer::OnReadable(uintptr_t s) {
    SocketPair* pair = reinterpret_cast<SocketPair*>(s);
    UDPHandler::Forward(pair);
}