#include "dns_resolver.hh"

#include <arpa/inet.h>
#include <netdb.h>

#include <shared_mutex>
#include <unordered_map>

#include "misc/configuration.hh"
#include "misc/logger.hh"

#include <cpr/cpr.h>
#include <rapidjson/document.h>
#include <rapidjson/error/en.h>

using namespace cpr;
using namespace rapidjson;

class LRUCache {
public:
    struct Node {
        std::string key_;
        sockaddr* val_;
        Node* prev_;
        Node* next_;
        Node() {}
        Node(std::string key, sockaddr* val) : key_(key), val_(val), prev_(nullptr), next_(nullptr) {}
    };

    LRUCache(int capacity) : capacity_(capacity) {
        head_ = new Node();
        tail_ = new Node();
        head_->next_ = tail_;
        tail_->prev_ = head_;
    }

    sockaddr* Get(std::string key) {
        std::shared_lock<std::shared_mutex> lk(mutex_);
        if (auto itor = cached_map_.find(key); itor != cached_map_.end()) {
            Node* temp = itor->second;
            MakeRecently(temp);
            return temp->val_;
        }
        return nullptr;
    }

    void Put(std::string key, sockaddr* value) {
        std::lock_guard<std::shared_mutex> lk(mutex_);
        if (auto itor = cached_map_.find(key); itor != cached_map_.end()) {
            Node* temp = itor->second;
            temp->val_ = value;
            MakeRecently(temp);
        } else {
            Node* cur = new Node(key, value);
            if (cached_map_.size() == capacity_) {
                Node* temp = DeleteCurrNode(tail_->prev_);
                delete temp->val_;
                temp->val_ = nullptr;
                cached_map_.erase(temp->key_);
            }
            MoveToHead(cur);
            cached_map_[key] = cur;
        }
    }

private:
    void MoveToHead(Node* cur) {
        Node* next = head_->next_;
        head_->next_ = cur;
        cur->prev_ = head_;
        next->prev_ = cur;
        cur->next_ = next;
    }

    Node* DeleteCurrNode(Node* cur) {
        cur->prev_->next_ = cur->next_;
        cur->next_->prev_ = cur->prev_;
        return cur;
    }

    void MakeRecently(Node* cur) {
        Node* temp = DeleteCurrNode(cur);
        MoveToHead(temp);
    }

    int capacity_;
    Node* head_;
    Node* tail_;
    std::unordered_map<std::string, Node*> cached_map_;
    mutable std::shared_mutex mutex_;
};

static LRUCache* cache_ = new LRUCache(512);

absl::StatusOr<sockaddr*> DNSResolver::Resolve(const std::string& domain) {
    auto res = cache_->Get(domain);
    if (res) {
        LOG("[Native] domain: %s cache hit", domain.c_str());
        return res;
    }

    else {
        LOG("[Native] domain: %s cache missing", domain.c_str());
        struct addrinfo hints;
        struct addrinfo* result;

        memset(&hints, 0, sizeof(struct addrinfo));
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_protocol = 0;

        if (getaddrinfo(domain.c_str(), nullptr, &hints, &result) != 0) {
            return absl::InternalError(absl::StrCat("Cannot resolve this domain: ", domain));
        }

        sockaddr* ia = new sockaddr();
        memcpy(ia, result->ai_addr, sizeof(sockaddr));
        freeaddrinfo(result);
        cache_->Put(domain, ia);
        LOG("[Native] result: %s", inet_ntoa(((sockaddr_in*)ia)->sin_addr));
        return ia;
    }
}

absl::StatusOr<sockaddr*> DNSResolver::ResolveDoH(const std::string& domain) {
    auto res = cache_->Get(domain);
    if (res) {
        LOG("[DoH] domain: %s cache hit", domain.c_str());
        return res;
    }

    else {
        LOG("[DoH] domain: %s cache missing", domain.c_str());
        Response r = Get(Url{ Configuration::doh_server_ },
                         Header{ { "accept", "application/dns-json" } },
                         Parameters{ { "type", "A" }, { "name", domain } },
                         VerifySsl(false));
        if (r.error.code != ErrorCode::OK) {
            return absl::InternalError(r.error.message);
        }
        Document doc;
        if (doc.Parse(r.text.c_str()).HasParseError()) {
            ERROR("Parse error: %s %zu", GetParseError_En(doc.GetParseError()), doc.GetErrorOffset());
            return absl::InternalError("Json parse error.");
        }
        if (auto itr = doc.FindMember("Answer"); itr != doc.MemberEnd() && itr->value.IsArray()) {
            const auto& answer = itr->value.GetArray();
            if (answer.Size() != 0) {
                for (const auto& record : answer) {
                    const auto& record_object = record.GetObject();
                    if (auto type = record_object.FindMember("type"); type != record_object.MemberEnd() && type->value.IsInt()) {
                        if (type->value.GetInt() == 1) {
                            if (auto data = record_object.FindMember("data"); data != record_object.MemberEnd() && data->value.IsString()) {
                                const char* ip = data->value.GetString();
                                sockaddr* sa = new sockaddr();
                                sa->sa_family = 2;
                                inet_pton(AF_INET, ip, &sa->sa_data[2]);
                                cache_->Put(domain, sa);
                                LOG("[DoH] result: %s", ip);
                                return sa;
                            }
                        }
                    }
                }
            }
        }
        return absl::InternalError(absl::StrCat("Cannot resolve this domain: ", domain));
    }
}