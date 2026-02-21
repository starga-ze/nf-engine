#include "HttpCache.h"
#include <fstream>
#include <sstream>

static bool hasExtension(const std::string& path, const std::string& ext)
{
    if (path.size() < ext.size())
        return false;

    return path.compare(path.size() - ext.size(), ext.size(), ext) == 0;
}

HttpCache::HttpCache(const std::string& baseDir) :
    m_baseDir(baseDir)
{

}

std::string HttpCache::loadFile(const std::string& path)
{
    std::ifstream file(path, std::ios::binary);
    if (!file)
        throw std::runtime_error("Failed to open file: " + path);

    std::ostringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

std::optional<HttpCache::Entry> HttpCache::get(const std::string& path) const
{
    std::string filePath;
    std::string contentType = "text/plain";

    if (path == "/")
    {
        filePath = m_baseDir + "/index.html";
        contentType = "text/html";
    }
    else
    {
        filePath = m_baseDir + path;

        if (hasExtension(path, ".html"))
        {
            contentType = "text/html";
        }
        else if (hasExtension(path, ".js"))
        {
            contentType = "application/javascript";
        }
        else if (hasExtension(path, ".css"))
        {
            contentType = "text/css";

        }
    }
    try
    {
        std::string body = loadFile(filePath);

        Entry entry;
        entry.contentType = contentType;
        entry.body = std::move(body);

        return entry;
    }
    catch (...)
    {
        return std::nullopt;
    }
}
