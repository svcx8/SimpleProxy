#include "configuration.hh"

#include <cstdio>

#include <absl/status/statusor.h>
using absl::StatusOr;

#include <rapidjson/document.h>
#include <rapidjson/error/en.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
using namespace rapidjson;

#include "misc/logger.hh"

StatusOr<Document> LoadJsonFromFile(const char* FilePath) {
    std::FILE* fp = std::fopen(FilePath, "r");
    Document doc;
    if (fp) {
        std::fseek(fp, 0, SEEK_END);
        size_t len = std::ftell(fp);
        char* buf = (char*)std::malloc(len + 1);
        buf[len] = 0;
        if (buf == nullptr) {
            std::fclose(fp);
            return absl::InternalError("Could not allocate memory");
        }
        std::rewind(fp);
        size_t readb = std::fread(buf, 1, len, fp);
        std::fclose(fp);
        if (readb != len) {
            std::free(buf);
            return absl::InternalError("Could not read the data");
        }
        if (doc.Parse(buf).HasParseError()) {
            LOG("[%s] %s #%d\nFile: %s %s %zu", __FUNCTION__, __FILE__, __LINE__,
                FilePath, GetParseError_En(doc.GetParseError()), doc.GetErrorOffset());
            return absl::InternalError("Parse error");
        }
    } else {
        LOG("[%s] %s #%d\nCannot load: %s. %s", __FUNCTION__, __FILE__, __LINE__, FilePath, std::strerror(errno));
        return absl::NotFoundError("Cannot load.");
    }
    return doc;
}

int Configuration::port_ = 2333;
bool Configuration::enable_doh_ = false;
std::string Configuration::doh_server_{ "https://doh.pub/dns-query" };

Configuration::Configuration() {
    auto result = LoadJsonFromFile("proxy.json");
    if (result.ok()) {
        Document& doc = *result;
        if (auto itr = doc.FindMember("Port"); itr != doc.MemberEnd() && itr->value.IsInt()) {
            port_ = itr->value.GetInt();
        }

        if (auto itr = doc.FindMember("EnableDoH"); itr != doc.MemberEnd() && itr->value.IsBool()) {
            enable_doh_ = itr->value.GetBool();
        }

        if (auto itr = doc.FindMember("DoHServer"); itr != doc.MemberEnd() && itr->value.IsString()) {
            doh_server_ = itr->value.GetString();
        }

        // if (auto itr = doc.FindMember("BuckeFillPeriod"); itr != doc.MemberEnd() && itr->value.IsUint64()) {
        //     fill_period_ = itr->value.GetUint64();
        // }

        // if (auto itr = doc.FindMember("BuckeFillTick"); itr != doc.MemberEnd() && itr->value.IsUint64()) {
        //     fill_tick_ = itr->value.GetUint64();
        // }

        // if (auto itr = doc.FindMember("BucketCapacity"); itr != doc.MemberEnd() && itr->value.IsUint64()) {
        //     capacity_ = itr->value.GetUint64();
        // }
    }
}