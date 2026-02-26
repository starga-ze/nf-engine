#pragma once

#include "util/ThreadManager.h"
#include "core/CoreControl.h"
#include "ipc/IpcCommandHandler.h"

#include <atomic>
#include <memory>

class IpcWorker;
class IpcReactor;

class IpcServer 
{
public:
    IpcServer(CoreControl* control, std::string socketPath,
            int worker, ThreadManager* threadManager);
    ~IpcServer();

    void start();
    void stop();

private:
    CoreControl* m_control;

    std::vector<std::unique_ptr<IpcWorker>> m_ipcWorkers;
    std::unique_ptr<IpcReactor> m_ipcReactor;
    std::unique_ptr<IpcCommandHandler> m_handler;
};

