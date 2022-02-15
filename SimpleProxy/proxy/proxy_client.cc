#include "proxy_client.hh"

#include <thread>

#include "dispatcher/epoller.hh"
#include "memory_buffer.hh"
#include "misc/configuration.hh"
#include "misc/logger.hh"
#include "proxy_socket.hh"

#include <absl/status/status.h>

/*
    ClientSocket->recv(); // Recv from server.
    ClientSocket->send(); // Send to server.
*/

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

// Receiving from Server. | Download | EPOLLIN
absl::Status ProxyClient::OnReadable(int s) {
    auto pair = ProxySocket::GetInstance().GetPointer(s);
    if (!pair)
        return absl::FailedPreconditionError("[" LINE_FILE "] Socket not found in ProxySocket::socket_list_.");

    if (auto buffer_pool = MemoryBuffer::GetPool(s)) {
        auto result = buffer_pool->Receive(s);

        if (result.ok()) {
            // auto wait_time = pair->token_bucket_.get()->Consume(*recv_res);
            // if (wait_time.count() > 0) {
            //     std::thread(&TokenBucket::GenerateToken, pair->token_bucket_.get(), poller_, pair->other_side_, wait_time).detach();
            //     return absl::OkStatus();
            // }
            result.Update(buffer_pool->Send(pair->this_side_));
        }

        if (result.code() == absl::StatusCode::kResourceExhausted) {
            result = absl::OkStatus();
            // The buffer of client cannot receive more data, add to poller.
            // absl::Status ProxyConn::OnWritable(int s) will send remaining data.
            result.Update(ProxySocket::GetConnPoller(pair)->ModSocket(pair->this_side_, EPOLLOUT));
            result.Update(poller_->RemoveSocket(pair->other_side_));
        }

        return result;
    } else {
        return absl::FailedPreconditionError("[" LINE_FILE "] GetPool return null");
    }
}

// Sending to Server. | Upload | EPOLLOUT
absl::Status ProxyClient::OnWritable(int s) {
    auto pair = ProxySocket::GetInstance().GetPointer(s);
    if (auto buffer_pool = MemoryBuffer::GetPool(pair->this_side_)) {
        if (pair->authentified_ == 2) {
            int socket_error = 0;
            socklen_t socket_error_len = sizeof(socket_error);
            if (getsockopt(pair->other_side_, SOL_SOCKET, SO_ERROR, &socket_error, &socket_error_len) < 0) {
                return absl::InternalError(strerror(errno));
            }

            if (socket_error != 0) {
                send(pair->this_side_, Socks5Command::reply_failure, 10, 0);
                ProxySocket::GetInstance().RemovePair(pair->this_side_);
                return absl::InternalError(strerror(errno));
            }

            auto result = poller_->ModSocket(pair->other_side_, EPOLLIN);

            if (send(pair->this_side_, Socks5Command::reply_success, 10, 0) == SOCKET_ERROR) {
                ProxySocket::GetInstance().RemovePair(pair->this_side_);
                return absl::InternalError(strerror(errno));
            }

            // pair->token_bucket_ = std::make_unique<TokenBucket>(Configuration::GetInstance().fill_period_,
            //                                                     Configuration::GetInstance().fill_tick_,
            //                                                     Configuration::GetInstance().capacity_);
            pair->authentified_++;
            return result;
        } else {
            auto result = buffer_pool->Send(pair->other_side_);
            if (!result.ok()) {
                return result;
            }
            // Remove EPOLLOUT event after the buffer is empty.
            if (buffer_pool->Usage() == 0) {
                result.Update(poller_->ModSocket(pair->this_side_, EPOLLIN));
                result.Update(ProxySocket::GetConnPoller(pair)->AddSocket(pair->other_side_, EPOLLIN));
            }
            return result;
        }
    } else {
        return absl::FailedPreconditionError("[" LINE_FILE "] GetPool return null");
    }
}