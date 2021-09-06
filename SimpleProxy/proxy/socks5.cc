#include "socks5.hh"

#include <cstring>

#include "dns_resolver.hh"
#include <misc/configuration.hh>

Socks5Header::Socks5Header(unsigned char* buffer) {
    version_ = buffer[0];
    auth_method_count_ = buffer[1];
    methods_ = &buffer[2];
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

Socks5Command::Socks5Command(unsigned char* buffer) {
    int index = 0;
    version_ = buffer[index++];
    command_ = buffer[index++];
    reserved_ = buffer[index++];
    address_type_ = buffer[index++];

    if (address_type_ == 0x3) {
        int domain_len = buffer[index++];
        std::string domain((char*)&buffer[index], 0, domain_len);

        index += domain_len;
        address_type_ = 0x01;
        address_struct_.sockaddr_in.sin_family = AF_INET;
        address_struct_.sockaddr_in.sin_port = *(short int*)&buffer[index];

        if (Configuration::GetInstance().enable_doh_) {
            address_struct_.sockaddr_in.sin_addr.s_addr = DNSResolver::ResolveDoH(domain);
        } else {
            address_struct_.sockaddr_in.sin_addr.s_addr = DNSResolver::Resolve(domain.c_str());
        }
    }

    else if (address_type_ == 0x01) {
        address_struct_.sockaddr_in.sin_family = AF_INET;
        address_struct_.sockaddr_in.sin_addr.s_addr = *(int32_t*)&buffer[index];
        index += 4;
        address_struct_.sockaddr_in.sin_port = *(short int*)&buffer[index];
    }

    else if (address_type_ == 0x04) {
        address_struct_.sockaddr_in6.sin6_family = AF_INET6;
        std::memcpy(address_struct_.sockaddr_in6.sin6_addr.s6_addr, &buffer[index], 16);
        index += 16;
        address_struct_.sockaddr_in6.sin6_port = *(short int*)&buffer[index];
    }

    else {
        version_ = -1;
        LOG("index: %d | wtf ATYP: %02X", index, address_type_);
        LOG("%02X %02X %02X %02X %02X %02X", buffer[0], buffer[1], buffer[2], buffer[3], buffer[4], buffer[5]);
    }
}

bool Socks5Command::Check() {
    return version_ == 0x5 && command_ == 1; // Support ipv4&ipv6 and tcp only.
}

const char Socks5Command::reply_success[] = {
    0x5, 0x0, 0x0, 0x1, 0x0, 0x0, 0x0, 0x0, 0x2, 0x33
};

const char Socks5Command::reply_failure[] = {
    0x5, 0x1, 0x0, 0x1, 0x0, 0x0, 0x0, 0x0, 0x2, 0x33
};