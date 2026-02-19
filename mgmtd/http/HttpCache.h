#pragma once
#include <string>
#include <unordered_map>
#include <optional>

class HttpCache {
public:
    struct Entry {
        std::string contentType;
        std::string body;
    };

    explicit HttpCache(
        const std::string& baseDir);

    std::optional<Entry>
    get(const std::string& path) const;

private:
    std::unordered_map<
        std::string,
        Entry> m_cache;

    static std::string
    loadFile(const std::string& path);
};

