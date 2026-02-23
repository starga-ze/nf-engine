#pragma once

#include <vector>
#include <memory>
#include <atomic>
#include <string>

class ThreadManager;

class DbManager;

class ShardWorker;

class Event;

class Action;

struct MarketMeta
{
    uint8_t id;
    std::string alias;
};

class ShardManager 
{
public:
    ShardManager(size_t workerCount, ThreadManager *threadManager, DbManager *dbmanager);

    ~ShardManager();

    void start();

    void stop();

    void dispatch(size_t shardIdx, std::unique_ptr <Event> event);

    void commit(size_t shardIdx, std::unique_ptr <Action> action);

    size_t getWorkerCount() const;

    ShardWorker *getWorker(size_t shardIdx) const;

    const std::vector<MarketMeta>& getMarkets() const;

private:
    void initMarkets();
    void initWorkers();

    void startWorkers();

    const size_t m_workerCount;
    ThreadManager *m_threadManager;
    DbManager *m_dbManager;

    std::atomic<bool> m_running{false};

    std::vector <std::unique_ptr<ShardWorker>> m_workers;

    std::vector<MarketMeta> m_markets;
};

