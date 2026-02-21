#include <string>

class IpcCommandHandler;

class StatsService
{
public:
    explicit StatsService(IpcCommandHandler& handler);

    std::string fetchSession();
    std::string fetchEngine();
    std::string fetchShard();

private:
    IpcCommandHandler& m_handler;
};
