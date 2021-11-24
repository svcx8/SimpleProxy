#ifndef DNS_RESOLVER_HEADER
#define DNS_RESOLVER_HEADER

#include <netdb.h>

#include "misc/net.hh"

#include <string>

#include <absl/status/statusor.h>

namespace DNSResolver {
    absl::StatusOr<addrinfo*> Resolve(const char* domain);
    absl::StatusOr<sockaddr_in*> ResolveDoH(std::string& domain);
} // namespace DNSResolver

#endif // dns_resolver.hh