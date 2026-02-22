#include <string>

class IpcCommandHandler;

class StatsService
{
public:
    explicit StatsService(IpcCommandHandler& handler);

    std::string fetchSession();
    std::string fetchEngine();
    std::string fetchShard();
    std::string fetchMarket();

private:
    IpcCommandHandler& m_handler;
};
