#include "dns_resolver.hh"

#include "misc/configuration.hh"
#include "misc/logger.hh"

#include <arpa/inet.h>

#include <cpr/cpr.h>
#include <rapidjson/document.h>
#include <rapidjson/error/en.h>

using namespace cpr;
using namespace rapidjson;

#define GetObjectA GetObject

// curl -H "accept: application/dns-json" "https://doh.pub/dns-query?type=A&name=example.com"
absl::StatusOr<uint32_t> DNSResolver::ResolveDoH(std::string& domain) {
    LOG("[DoH] domain: %s", domain.c_str());
    Response r = Get(Url{ Configuration::GetInstance().doh_server_ },
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
                            sockaddr_in sa;
                            inet_pton(AF_INET, ip, &(sa.sin_addr));
                            return sa.sin_addr.s_addr;
                        }
                    }
                }
            }
        }
    }

    return absl::InternalError("Cannot resolve this domain.");
}