#include <string>

class IpcCommandHandler;

class StatsService
{
public:
    explicit StatsService(IpcCommandHandler& handler);

    std::string fetchSession();
    std::string fetchEngine();

private:
    IpcCommandHandler& m_handler;
};
