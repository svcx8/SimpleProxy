#ifndef CONFIGURATION_HEADER
#define CONFIGURATION_HEADER

#include <misc/singleton.hh>

#include <string>

class Configuration : public Singleton<Configuration> {
public:
    Configuration();
    int port_ = 2333;
    bool enable_doh_ = false;
    std::string doh_server_{ "https://doh.pub/dns-query" };
};

#endif // configuration.hh