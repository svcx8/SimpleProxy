#include "socks5.hh"

#include <cstring>

#include "dns_resolver.hh"
#include "misc/configuration.hh"
#include "misc/logger.hh"

Socks5Header::Socks5Header(void* buffer) {
    unsigned char* buf = reinterpret_cast<unsigned char*>(buffer);
    version_ = buf[0];
    auth_method_count_ = buf[1];
    methods_ = &buf[2];
}

bool Socks5Header::Check() {
    if (version_ != 5)
        return false;
    bool is_valid = false;
    for (int i = 0; i < auth_method_count_; ++i) {
        if (methods_[i] == 0) {
            is_valid = true;
            break;
        }
    }
    return is_valid;
}

Socks5Command::Socks5Command(void* buffer) : head_buffer_(reinterpret_cast<unsigned char*>(buffer)) {
    int index = 0;
    version_ = head_buffer_[index++];
    command_ = head_buffer_[index++];
    reserved_ = head_buffer_[index++];
    address_type_ = head_buffer_[index++];
}

absl::Status Socks5Command::Check() {
    int index = 4;
    if (address_type_ == 0x3) {
        int domain_len = head_buffer_[index++];
        std::string domain((char*)&head_buffer_[index], 0, domain_len);

        index += domain_len;
        address_type_ = 0x01;

        absl::StatusOr<sockaddr*> result;
        if (Configuration::enable_doh_) {
            result = DNSResolver::ResolveDoH(domain);

        } else {
            result = DNSResolver::Resolve(domain);
        }
        if (!result.ok()) {
            return result.status();
        }

        sock_addr_ = *result;
        sock_addr_len_ = sizeof(sockaddr_in);
        reinterpret_cast<sockaddr_in*>(sock_addr_)->sin_port = *(short int*)&head_buffer_[index];
    }

    else if (address_type_ == 0x01) {
        sockaddr_in* addr_info = new sockaddr_in();
        addr_info->sin_family = AF_INET;
        addr_info->sin_addr.s_addr = *(int32_t*)&head_buffer_[index];
        index += 4;
        addr_info->sin_port = *(short int*)&head_buffer_[index];
        sock_addr_ = reinterpret_cast<sockaddr*>(addr_info);
        sock_addr_len_ = sizeof(sockaddr_in);
    }

    else if (address_type_ == 0x04) {
        sockaddr_in6* addr_info6 = new sockaddr_in6();
        addr_info6->sin6_family = AF_INET6;
        std::memcpy(addr_info6->sin6_addr.s6_addr, &head_buffer_[index], 16);
        index += 16;
        addr_info6->sin6_port = *(short int*)&head_buffer_[index];
        sock_addr_ = reinterpret_cast<sockaddr*>(addr_info6);
        sock_addr_len_ = sizeof(sockaddr_in6);
    }

    else {
        LOG("wtf ATYP: %02X", address_type_);
        LOG("%02X %02X %02X %02X %02X %02X", head_buffer_[0], head_buffer_[1], head_buffer_[2], head_buffer_[3], head_buffer_[4], head_buffer_[5]);
        return absl::FailedPreconditionError("Unknown address type.");
    }

    if (version_ != 0x05) {
        return absl::FailedPreconditionError("Only support socks5.");
    }
    if (command_ != 1) {
        return absl::FailedPreconditionError("Only support tcp.");
    }
    return absl::OkStatus();
}

const char Socks5Command::reply_success[] = {
    0x5, 0x0, 0x0, 0x1, 0x0, 0x0, 0x0, 0x0, 0x2, 0x33
};

const char Socks5Command::reply_failure[] = {
    0x5, 0x1, 0x0, 0x1, 0x0, 0x0, 0x0, 0x0, 0x2, 0x33
};