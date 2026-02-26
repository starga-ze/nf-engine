#include "IpcServer.h"

#include "ipc/IpcWorker.h"
#include "ipc/IpcReactor.h"

IpcServer::IpcServer(CoreControl* control, std::string socketPath, 
        int worker, ThreadManager* threadManager)
    : m_control(control)
{
    m_handler = std::make_unique<IpcCommandHandler>(control);

    for (int i = 0; i < worker; ++i)
    {
        m_ipcWorkers.emplace_back(std::make_unique<IpcWorker>(m_handler.get(), threadManager, i));
    }
    m_ipcReactor = std::make_unique<IpcReactor>(std::move(socketPath), m_ipcWorkers);

    for (auto& worker : m_ipcWorkers)
    {
        worker->setReactor(m_ipcReactor.get());
    }
}

IpcServer::~IpcServer()
{
    stop();
}


void IpcServer::start()
{
    for (auto& worker : m_ipcWorkers)
    {
        worker->start();
    }
    m_ipcReactor->start();
}

void IpcServer::stop()
{
    m_ipcReactor->stop();
    for (auto & worker : m_ipcWorkers)
    {
        worker->stop();
    }
}
