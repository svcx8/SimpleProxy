#ifndef CONFIGURATION_HEADER
#define CONFIGURATION_HEADER

#include <string>

class Configuration {
public:
    static int port_;
    static bool enable_doh_;
    static std::string doh_server_;

    static void Init();
};

#endif // configuration.hh