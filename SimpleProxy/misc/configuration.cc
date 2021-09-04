#include "configuration.hh"

#include <misc/logger.hh>

#include <cstdio>

#include <rapidjson/document.h>
#include <rapidjson/error/en.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
using namespace rapidjson;

bool LoadJsonFromFile(Document& doc, const char* FilePath) {
    std::FILE* fp = std::fopen(FilePath, "r");
    if (fp) {
        std::fseek(fp, 0, SEEK_END);
        size_t len = std::ftell(fp);
        char* buf = (char*)std::malloc(len + 1);
        buf[len] = 0;
        if (buf == nullptr) {
            std::fclose(fp);
            LOG("Could not allocate memory");
            return false;
        }
        std::rewind(fp);
        size_t readb = std::fread(buf, 1, len, fp);
        std::fclose(fp);
        if (readb != len) {
            std::free(buf);
            LOG("Could not read the data");
            return false;
        }
        if (doc.Parse(buf).HasParseError()) {
            LOG("Parse error: %s %s %zu", FilePath, GetParseError_En(doc.GetParseError()), doc.GetErrorOffset());
            return false;
        }
    } else {
        LOG("Cannot load %s. %s", FilePath, std::strerror(errno));
        return false;
    }
    return true;
}

Configuration::Configuration() : enable_doh_(false) {
    Document doc;
    if (LoadJsonFromFile(doc, "proxy.json")) {
        if (auto itr = doc.FindMember("EnableDoH"); itr != doc.MemberEnd() && itr->value.IsBool()) {
            enable_doh_ = itr->value.GetBool();
        }

        if (auto itr = doc.FindMember("DoHServer"); itr != doc.MemberEnd() && itr->value.IsString()) {
            doh_server_ = itr->value.GetString();
        }
    }
}