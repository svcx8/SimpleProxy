#ifndef CONFIGURATION_HEADER
#define CONFIGURATION_HEADER

#include "singleton.hh"

#include <string>

class Configuration : public Singleton<Configuration> {
    friend Singleton;

public:
    int port_ = 2333;
    bool enable_doh_ = false;
    std::string doh_server_{ "https://doh.pub/dns-query" };

    // uint64_t fill_period_ = 0;
    // uint64_t fill_tick_ = 0;
    // uint64_t capacity_ = 0;

protected:
    Configuration();
};

#endif // configuration.hh