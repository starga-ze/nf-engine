#pragma once

#include "core/CoreControl.h"
#include "ipc/IpcCommandHandler.h"

#include <atomic>
#include <string>
#include <memory>

class Epoll;   

class IpcServer 
{
public:
    explicit IpcServer(CoreControl& control, std::string socketPath);
    ~IpcServer();

    void start();
    void stop();

private:
    bool setupSocket();
    void cleanup();
    void handleAccept();

    std::unique_ptr<Epoll> m_ipcEpoll;

    std::atomic<bool> m_running{false};
    int m_serverFd{-1};

    CoreControl& m_control;
    std::string m_socketPath;

    std::unique_ptr<IpcCommandHandler> m_handler;
};

