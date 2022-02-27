#ifndef CONFIGURATION_HEADER
#define CONFIGURATION_HEADER

#include <string>

class Configuration {
public:
    Configuration();

    static int port_;
    static bool enable_doh_;
    static std::string doh_server_;

    // uint64_t fill_period_ = 0;
    // uint64_t fill_tick_ = 0;
    // uint64_t capacity_ = 0;
};

#endif // configuration.hh