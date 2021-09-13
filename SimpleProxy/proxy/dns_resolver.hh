#ifndef DNS_RESOLVER_HEADER
#define DNS_RESOLVER_HEADER

#include <netdb.h>

#include <string>

namespace DNSResolver {
    inline uint32_t Resolve(const char* domain) {
        auto result = gethostbyname(domain);
        return result ? *(uint32_t*)result->h_addr : 0;
    }
    uint32_t ResolveDoH(std::string& domain);
} // namespace DNSResolver

#endif // dns_resolver.hh