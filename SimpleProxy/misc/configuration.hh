#ifndef CONFIGURATION_HEADER
#define CONFIGURATION_HEADER

#include <misc/singleton.hh>

#include <string>

class Configuration : public Singleton<Configuration> {
public:
    Configuration();
    bool enable_doh_;
    std::string doh_server_{ "https://doh.pub/dns-query" };
};

#endif // configuration.hh