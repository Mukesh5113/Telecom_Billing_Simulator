#ifndef TBS_IO_JSON_HPP
#define TBS_IO_JSON_HPP

// Thin wrapper over RapidJSON so the rest of the codebase never touches
// RapidJSON types directly. Centralizes parsing, typed access, and errors.

#include <string>
#include <cstdio>
#include <stdexcept>
#include <vector>

#include "rapidjson/document.h"
#include "rapidjson/error/en.h"
#include "rapidjson/filereadstream.h"

#include "tbs/Money.hpp"

namespace tbs {
namespace json {

// Parse a JSON file into a RapidJSON Document. Throws std::runtime_error
// with a clear message (path + offset) on any parse/IO failure.
inline rapidjson::Document parseFile(const std::string& path) {
    std::FILE* fp =
#if defined(_WIN32)
        nullptr;
    if (fopen_s(&fp, path.c_str(), "rb") != 0) fp = nullptr;
#else
        std::fopen(path.c_str(), "rb");
#endif
    if (!fp) {
        throw std::runtime_error("json: cannot open file '" + path + "'");
    }

    std::vector<char> buffer(65536);
    rapidjson::FileReadStream stream(fp, buffer.data(),
                                     static_cast<std::size_t>(buffer.size()));
    rapidjson::Document doc;
    doc.ParseStream(stream);
    std::fclose(fp);

    if (doc.HasParseError()) {
        throw std::runtime_error(
            "json: parse error in '" + path + "' at offset " +
            std::to_string(doc.GetErrorOffset()) + ": " +
            rapidjson::GetParseError_En(doc.GetParseError()));
    }
    return doc;
}

inline std::string getString(const rapidjson::Value& v, const char* key,
                             const std::string& def = std::string()) {
    if (!v.HasMember(key) || !v[key].IsString()) return def;
    return std::string(v[key].GetString(), v[key].GetStringLength());
}

inline std::string requireString(const rapidjson::Value& v, const char* key) {
    if (!v.HasMember(key) || !v[key].IsString()) {
        throw std::runtime_error(std::string("json: missing string field '") + key + "'");
    }
    return std::string(v[key].GetString(), v[key].GetStringLength());
}

inline std::int64_t getInt64(const rapidjson::Value& v, const char* key,
                             std::int64_t def = 0) {
    if (!v.HasMember(key)) return def;
    const rapidjson::Value& f = v[key];
    if (f.IsInt64())  return f.GetInt64();
    if (f.IsInt())    return f.GetInt();
    if (f.IsUint())   return static_cast<std::int64_t>(f.GetUint());
    if (f.IsDouble()) return static_cast<std::int64_t>(f.GetDouble());
    if (f.IsString()) return std::stoll(f.GetString());
    return def;
}

inline int getInt(const rapidjson::Value& v, const char* key, int def = 0) {
    return static_cast<int>(getInt64(v, key, def));
}

// Money may appear as a string ("12.34") or a JSON number (12.34).
inline Money getMoney(const rapidjson::Value& v, const char* key,
                      Money def = Money(0)) {
    if (!v.HasMember(key)) return def;
    const rapidjson::Value& f = v[key];
    if (f.IsString()) return Money::parse(f.GetString());
    if (f.IsNumber()) {
        // Format with 2 decimals then parse to stay in integer-cents land.
        double d = f.GetDouble();
        char buf[64];
        std::snprintf(buf, sizeof(buf), "%.2f", d);
        return Money::parse(buf);
    }
    return def;
}

// Return a reference to a required array member.
inline const rapidjson::Value& requireArray(const rapidjson::Value& v,
                                            const char* key) {
    if (!v.HasMember(key) || !v[key].IsArray()) {
        throw std::runtime_error(std::string("json: missing array field '") + key + "'");
    }
    return v[key];
}

} // namespace json
} // namespace tbs

#endif // TBS_IO_JSON_HPP
