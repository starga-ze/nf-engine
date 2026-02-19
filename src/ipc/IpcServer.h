#pragma once

#include <atomic>
#include <string>
#include <memory>

class Epoll;   

class IpcServer 
{
public:
    explicit IpcServer(std::string socketPath);
    ~IpcServer();

    void start();
    void stop();

private:
    bool setupSocket();
    void cleanup();
    void handleAccept();

    std::unique_ptr<Epoll> m_ipcEpoll;

    std::string m_socketPath;
    std::atomic<bool> m_running{false};
    int m_serverFd{-1};

};

