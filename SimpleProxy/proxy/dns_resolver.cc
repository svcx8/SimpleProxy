#include "dns_resolver.hh"

#include <arpa/inet.h>

#include "misc/configuration.hh"
#include "misc/logger.hh"

#include <cpr/cpr.h>
#include <rapidjson/document.h>
#include <rapidjson/error/en.h>

using namespace cpr;
using namespace rapidjson;

absl::StatusOr<sockaddr*> DNSResolver::Resolve(const std::string& domain) {
    LOG("[Native] domain: %s", domain.c_str());
    struct addrinfo hints;
    struct addrinfo* result;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = 0;

    if (getaddrinfo(domain.c_str(), nullptr, &hints, &result) != 0) {
        return absl::InternalError("Cannot resolve this domain.");
    }

    sockaddr* ia = new sockaddr();
    memcpy(ia, result->ai_addr, sizeof(sockaddr));

    freeaddrinfo(result);
    return ia;
}

// curl -H "accept: application/dns-json" "https://doh.pub/dns-query?type=A&name=example.com"
absl::StatusOr<sockaddr_in*> DNSResolver::ResolveDoH(const std::string& domain) {
    LOG("[DoH] domain: %s", domain.c_str());
    Response r = Get(Url{ Configuration::doh_server_ },
                     Header{ { "accept", "application/json" } },
                     Parameters{ { "type", "A" }, { "name", domain } },
                     VerifySsl(false));
    if (r.error.code != ErrorCode::OK) {
        return absl::InternalError(r.error.message);
    }
    Document doc;
    if (doc.Parse(r.text.c_str()).HasParseError()) {
        LOG("Parse error: %s %zu", GetParseError_En(doc.GetParseError()), doc.GetErrorOffset());
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
                            sockaddr_in* sa = new sockaddr_in();
                            inet_pton(AF_INET, ip, &sa->sin_addr);
                            return sa;
                        }
                    }
                }
            }
        }
    }

    return absl::InternalError("Cannot resolve this domain.");
}