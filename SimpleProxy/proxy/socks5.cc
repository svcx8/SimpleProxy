#include "socks5.hh"

#include <misc/logger.hh>

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
    unsigned char* ptr = (unsigned char*)&remote_address_;
    version_ = buffer[index++];
    command_ = buffer[index++];
    reserved_ = buffer[index++];
    address_type_ = buffer[index++];
    // Remote address.
    ptr[0] = buffer[index + 3];
    ptr[1] = buffer[index + 2];
    ptr[2] = buffer[index + 1];
    ptr[3] = buffer[index];
    index += 4;
    short src = *(short*)&buffer[index];
    port_ = ((src >> 8) & 0xFF) | ((src & 0xFF) << 8);
    LOG("[!!] remote_address_: %d.%d.%d.%d:%d", ptr[3], ptr[2], ptr[1], ptr[0], port_);
}

bool Socks5Command::Check() {
    return version_ == 0x5 && command_ == 1 && address_type_ == 1; // Support ipv4 and tcp only.
}

const char Socks5Command::reply_success[] = {
    0x5, 0x0, 0x0, 0x1, 0x0, 0x0, 0x0, 0x0, 0x2, 0x33
};

const char Socks5Command::reply_failure[] = {
    0x5, 0x1, 0x0, 0x1, 0x0, 0x0, 0x0, 0x0, 0x2, 0x33
};