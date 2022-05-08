#include "socket_pair.hh"

#include <mutex>
#include <thread>
#include <unordered_map>

#include "dispatcher/epoller.hh"
#include "memory_buffer.hh"
#include "misc/logger.hh"

std::atomic<int> last_poller_index_ = 1;

// Client Poller reserved_list_[0] && reserved_list_[1]
// Conn Poller reserved_list_[2] && reserved_list_[3]
// set reserved_list_[0] && reserved_list_[2] as a group

ProxyPoller* AcquireConnPoller() {
    auto poller_ = reinterpret_cast<ProxyPoller*>(EPoller::reserved_list_[last_poller_index_ + 1]);
    last_poller_index_ = last_poller_index_ % 2 + 1;
    /*
        last_poller_index_ = 1, 2, 1, 2, 1, 2......
    */
    LOG("[SPM] [AcquireConnPoller] [t#%d] %d", gettid(), last_poller_index_.load());
    return poller_;
}

ProxyPoller* AcquireClientPoller() {
    auto poller_ = reinterpret_cast<ProxyPoller*>(EPoller::reserved_list_[last_poller_index_ - 1]);
    last_poller_index_ = last_poller_index_ % 2 + 1;
    /*
        last_poller_index_ = 1, 2, 1, 2, 1, 2......
    */
    LOG("[SPM] [AcquireClientPoller] [t#%d] %d", gettid(), last_poller_index_.load());
    return poller_;
}

void StartPurgeThread() {
    std::thread([]() {
        while (true) {
            std::this_thread::sleep_for(std::chrono::seconds(30));
            int count = 0;
            {
                // Check if the conn_list has closed connection
                std::lock_guard<std::mutex> lk(ConnSocket::list_mutex_);
                for (auto it = ConnSocket::list_.begin(); it != ConnSocket::list_.end();) {
                    if (it->second->is_closed_) {
                        close(it->second->socket_);
                        MemoryBuffer::RevertPool(it->second->buffer_);
                        delete it->second;
                        it = ConnSocket::list_.erase(it);
                        count++;
                    } else {
                        ++it;
                    }
                }
            }
            {
                // Check if the client_list has closed connection
                std::lock_guard<std::mutex> lk(ClientSocket::list_mutex_);
                for (auto it = ClientSocket::list_.begin(); it != ClientSocket::list_.end();) {
                    if (it->second->is_closed_) {
                        close(it->second->socket_);
                        MemoryBuffer::RevertPool(it->second->buffer_);
                        delete it->second;
                        it = ClientSocket::list_.erase(it);
                        count++;
                    } else {
                        ++it;
                    }
                }
            }

            LOG("[SPM] [t#%d] Prune %d pairs, left: %ld - %ld", gettid(), count, ConnSocket::list_.size(), ConnSocket::list_.size());
        }
    }).detach();
}

std::unordered_map<int, ConnSocket*> ConnSocket::list_;
std::mutex ConnSocket::list_mutex_;
void ConnSocket::AddToList(int s, ConnSocket* conn_socket) {
    std::lock_guard<std::mutex> lock(list_mutex_);
    list_[s] = conn_socket;
}

std::unordered_map<int, ClientSocket*> ClientSocket::list_;
std::mutex ClientSocket::list_mutex_;
void ClientSocket::AddToList(int s, ClientSocket* socket_) {
    std::lock_guard<std::mutex> lock(list_mutex_);
    list_[s] = socket_;
}

void ConnSocket::Close() {
    if (is_closed_ == false) {
        poller_->RemoveSocket(socket_).IgnoreError();
        is_closed_ = true;
    }
    if (other_side_ != nullptr && other_side_->is_closed_ == false) {
        other_side_->poller_->RemoveSocket(other_side_->socket_).IgnoreError();
        other_side_->is_closed_ = true;
    }
}

void ClientSocket::Close() {
    if (is_closed_ == false) {
        poller_->RemoveSocket(socket_).IgnoreError();
        is_closed_ = true;
    }
    if (other_side_ != nullptr && other_side_->is_closed_ == false) {
        other_side_->poller_->RemoveSocket(other_side_->socket_).IgnoreError();
        other_side_->is_closed_ = true;
    }
}