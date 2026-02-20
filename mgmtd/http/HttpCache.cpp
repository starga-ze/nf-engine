#include "HttpCache.h"
#include <fstream>
#include <sstream>

std::string
HttpCache::loadFile(
    const std::string& path)
{
    std::ifstream file(path,
                       std::ios::binary);
    if (!file)
        throw std::runtime_error(
            "Failed to open file: "
            + path);

    std::ostringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

HttpCache::HttpCache(
    const std::string& baseDir)
{
    m_cache["/"] = {
        "text/html",
        loadFile(baseDir
            + "/index.html")
    };

    m_cache["/app.js"] = {
        "application/javascript",
        loadFile(baseDir
            + "/app.js")
    };

    m_cache["/style.css"] = {
        "text/css",
        loadFile(baseDir
            + "/style.css")
    };
}

std::optional<HttpCache::Entry>
HttpCache::get(
    const std::string& path) const
{
    auto it =
        m_cache.find(path);
    if (it == m_cache.end())
        return std::nullopt;
    return it->second;
}

