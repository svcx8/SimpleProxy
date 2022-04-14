#ifndef DNS_RESOLVER_HEADER
#define DNS_RESOLVER_HEADER

#include <netdb.h>

#include "misc/net.hh"

#include <string>

#include <absl/status/statusor.h>

namespace DNSResolver {
    absl::StatusOr<sockaddr*> Resolve(const std::string& domain);
    absl::StatusOr<sockaddr*> ResolveDoH(const std::string& domain);
} // namespace DNSResolver

#endif // dns_resolver.hh