#pragma once

#include <string>
#include <unordered_map>

class AuthService
{
public:
    struct LoginResult
    {
        bool success;
        std::string sessionId;
    };

    AuthService();

    LoginResult login(const std::string& id,
                      const std::string& password);

    bool validateSession(const std::string& sessionId);

    void logout(const std::string& sessionId);

private:
    struct Session
    {
        uint64_t expiresAt;
    };

    std::unordered_map<std::string, Session> m_sessions;

    std::string m_username = "admin";
    std::string m_passwordHash; // 로딩
    std::string m_salt;

    std::string hash(const std::string& pw,
                     const std::string& salt);

    uint64_t now();
};
