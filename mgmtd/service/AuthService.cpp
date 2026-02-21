#include "AuthService.h"
#include "util/Logger.h"

#include <openssl/sha.h>
#include <iomanip>
#include <sstream>
#include <chrono>
#include <random>
#include <fstream>
#include <nlohmann/json.hpp>

static constexpr uint64_t SESSION_TTL_SEC = 1800;

using json = nlohmann::json;

AuthService::AuthService()
{
    std::ifstream f(std::string(PROJECT_ROOT) +
                    "/database/mgmtd-startup-config.json");

    if (!f.is_open())
    {
        LOG_FATAL("startup-config.json not found");
        return;
    }
    
    try
    {
        json j;
        f >> j;

        m_username     = j["admin"]["username"];
        m_passwordHash = j["admin"]["password_hash"];
        m_salt         = j["admin"]["salt"];
    }
    catch (const std::exception& e)
    {
        LOG_FATAL("JSON parse error: {}", e.what());
    }
}

uint64_t AuthService::now()
{
    return std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
}

std::string AuthService::hash(const std::string& pw,
                              const std::string& salt)
{
    std::string input = pw + salt;

    unsigned char digest[SHA256_DIGEST_LENGTH];
    SHA256((unsigned char*)input.c_str(),
           input.size(),
           digest);

    std::ostringstream oss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i)
        oss << std::hex << std::setw(2)
            << std::setfill('0') << (int)digest[i];

    return oss.str();
}

AuthService::LoginResult AuthService::login(const std::string& id,
                   const std::string& password)
{
    if (id != m_username)
        return {false, ""};

    if (hash(password, m_salt) != m_passwordHash)
        return {false, ""};

    /*
    if (id != "admin" || password != "admin")
        return {false, ""};
    */

    std::string sessionId = std::to_string(now()) + "_sess";

    m_sessions[sessionId] = {
        now() + SESSION_TTL_SEC
    };

    return {true, sessionId};
}

bool AuthService::validateSession(const std::string& sessionId)
{
    LOG_DEBUG("Validate session. map size={}, session id={}", m_sessions.size(), sessionId);

    auto it = m_sessions.find(sessionId);
    if (it == m_sessions.end())
        return false;

    if (now() > it->second.expiresAt)
    {
        m_sessions.erase(it);
        return false;
    }

    return true;
}

void AuthService::logout(const std::string& sessionId)
{
    m_sessions.erase(sessionId);
}
