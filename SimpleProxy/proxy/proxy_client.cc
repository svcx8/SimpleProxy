#include "proxy_client.hh"

#include <thread>

#include "dispatcher/epoller.hh"
#include "memory_buffer.hh"
#include "misc/configuration.hh"
#include "misc/logger.hh"
#include "misc/net.hh"
#include "socket_pair.hh"
#include "socks5.hh"

#include <absl/status/status.h>

/*
    There has some plans to use TokenBucket to limit the speed of download.

    1) if(token_bucket_->consure()) {
        sleep(token_bucket_->get_tick());
    }

    2) epoll_ctl(epoller_inst_, EPOLL_CTL_DEL, s, nullptr)

    3) create a thread to any connects

    4) send(token_bucket_->serving_);

    Unfortunately, if we are the server, plan 4) would be a good idea.
    But in this case, as a proxy, if we only limit the speed of upload,
    the speed you download from the remote server is not limited.
    In worst case, the file has been downloaded to buffer_pool_,
    but we still send to client at slow speed.

    In plan 1), we're not using multi-thread to handle per connect,
    so it will block the others.

    In plan 2) and 3), i don't know if it will cause high cpu usage.
*/

/*
    Level Triggered
    EPOLLIN Event

        Socket Receive Buffer           Is Empty            Low Level
        Socket Receive Buffer           Not Empty           High Level(Triggered)

    EPOLLOUT Event

        Socket Send Buffer              Is Full              Low Level
        Socket Send Buffer              Not Full             High Level(Triggered)
*/

void ProxyClient::OnReadable(uintptr_t s) {
    auto pair = reinterpret_cast<SocketPair*>(s);

    if (auto recv_from_server_pool = MemoryBuffer::GetPool(pair->client_socket_)) {
        auto result = recv_from_server_pool->RecvFromServer(pair->client_socket_); // [data race issue] When use this buffer_pool to recv data from server,
                                                                                   // the proxy_conn#155 uses the same buffer_pool to send to client.
                                                                                   // But i think this is a bug from ThreadSanitizer.

        if (result.ok()) {
            result.Update(recv_from_server_pool->ProxyClient_SendToClient(pair->conn_socket_));
        }

        if (!result.ok()) {
            if (result.code() != absl::StatusCode::kResourceExhausted) {
                SocketPairManager::RemovePair(pair); // TODO: [data race issue] When connection reset by client,
                                                     // the proxy_conn.cc#122 still reading from client,
                                                     // and the proxy_conn.cc#125 trying to send to proxy.
                return;
            } else {
                result = absl::OkStatus();
                // The buffer of client cannot receive more data, add to poller.
                // absl::Status ProxyConn::OnWritable(int s) will send remaining data.
                result.Update(poller_->RemoveSocket(pair->client_socket_));
                result.Update(pair->conn_poller_->ModSocket(pair->conn_socket_, s, EPOLLOUT));

                if (!result.ok()) {
                    // TODO
                }
            }
        }
    }
}

void ProxyClient::OnWritable(uintptr_t s) {
    auto pair = reinterpret_cast<SocketPair*>(s);

    if (auto recv_from_client_pool = MemoryBuffer::GetPool(pair->conn_socket_)) {
        if (pair->authentified_ == 2) {
            int socket_error = 0;
            socklen_t socket_error_len = sizeof(socket_error);
            if (getsockopt(pair->client_socket_, SOL_SOCKET, SO_ERROR, &socket_error, &socket_error_len) < 0) {
                return;
            }

            if (socket_error != 0) {
                send(pair->conn_socket_, Socks5Command::reply_failure, 10, 0);
                SocketPairManager::RemovePair(pair);
                return;
            }

            pair->client_poller_->ModSocket(pair->client_socket_, s, EPOLLIN).IgnoreError();

            if (send(pair->conn_socket_, Socks5Command::reply_success, 10, 0) == SOCKET_ERROR) {
                SocketPairManager::RemovePair(pair);
                return;
            }
            pair->authentified_++;
            return;
        }

        else {
            auto result = recv_from_client_pool->ProxyClient_SendToServer(pair->client_socket_);
            if (!result.ok()) {
                SocketPairManager::RemovePair(pair);
                return;
            }
            // Remove EPOLLOUT event after the buffer is empty.
            if (recv_from_client_pool->Usage() == 0) {
                result.Update(pair->client_poller_->ModSocket(pair->client_socket_, s, EPOLLIN));
                result.Update(pair->conn_poller_->AddSocket(pair->conn_socket_, s, EPOLLIN));
            }

            if (!result.ok()) {
                // TODO
            }
        }
    }
}