#include "Core.h"
#include "util/Logger.h"
#include "db/DbConfig.h"
#include "shard/ShardWorker.h"

#include <csignal>
#include <chrono>
#include <thread>

std::atomic<bool> Core::m_running{true};

void Core::setFlag(bool enableDb) {
    m_enableDb = enableDb;

    if (m_enableDb) {
        LOG_INFO("Database enabled by startup option.");
    } else {
        LOG_INFO("Database disabled by startup option.");
    }
}

void Core::run() {
    if (not initializeRuntime()) {
        return;
    }

    if (not initializeTlsContext()) {
        return;
    }

    initializeIngress();
    initializeEgress();

    initializeClients();

    startThreads();

    waitForShutdown();
}

void Core::shutdown() {
    if (m_threadManager) {
        m_threadManager->stopAll();
    }

    LOG_INFO("All threads terminated successfully.");
    Logger::Shutdown();
}

bool Core::initializeRuntime() {
    ThreadManager::setName("main");
    Logger::Init("nf-server", "/var/log/nf/nf-server.log", 1048576 * 5, 100);

    handleSignal();

    m_shardWorkerThread = 4;

    m_clients = 100;
    
    m_tcpServerWorkerThread = 3;
    m_udpServerWorkerThread = 3;
    m_tlsServerWorkerThread = 3;

    m_tcpServerPort = 8000;
    m_udpServerPort = 8001;

    if (m_enableDb) {
        if (not initDatabase()) {
            LOG_FATAL("Database initialize failed.");
            return false;
        }
    }
    if (not initThreadManager()) {
        LOG_FATAL("ThreadManager initialize failed.");
        return false;
    }
    if (not initShardManager()) {
        LOG_FATAL("ShardManager initialize failed.");
        return false;
    }
    if (not initSessionManager()) {
        LOG_FATAL("SessionManager initialize failed.");
        return false;
    }
    return true;
}

bool Core::initializeTlsContext() {
    m_tlsContext = std::make_unique<TlsContext>();
    if (not m_tlsContext) {
        LOG_FATAL("Failed to allocate TLS context");
        return false;
    }

    if (not m_tlsContext->init("/etc/nf/cert/cert.pem", "/etc/nf/cert/key.pem")) {
        LOG_FATAL("Failed to initialize TLS context");
        return false;
    }

    LOG_INFO("Success to initialize TLS context");
    return true;
}

void Core::initializeIngress() {
    m_rxRouter = std::make_unique<RxRouter>(m_shardManager.get(), m_sessionManager.get());

    SSL_CTX *sslCtx = m_tlsContext->get();

    m_tlsServer = std::make_shared<TlsServer>(
            sslCtx,
            m_rxRouter.get(),
            m_tlsServerWorkerThread,
            m_threadManager.get()
    );

    m_tcpServer = std::make_unique<TcpServer>(
            m_tcpServerPort,
            m_rxRouter.get(),
            m_tcpServerWorkerThread,
            m_threadManager.get(),
            m_tlsServer
    );

    m_udpServer = std::make_unique<UdpServer>(
            m_udpServerPort,
            m_rxRouter.get(),
            m_udpServerWorkerThread,
            m_threadManager.get()
    );
}

void Core::initializeEgress() {
    m_txRouter = std::make_unique<TxRouter>(
            m_tlsServer.get(),
            m_tcpServer.get(),
            m_udpServer.get(),
            m_sessionManager.get()
    );

    for (size_t i = 0; i < m_shardManager->getWorkerCount(); ++i) {
        auto *worker = m_shardManager->getWorker(i);
        if (not worker) {
            LOG_FATAL("Shard Worker not init");
        }
        worker->setTxRouter(m_txRouter.get());
    }
}

void Core::startThreads() {
    
    m_threadManager->addThread("tls_reactor",
                               std::bind(&TlsServer::start, m_tlsServer.get()),
                               std::bind(&TlsServer::stopReact, m_tlsServer.get()));

    m_threadManager->addThread("udp_reactor",
                               std::bind(&UdpServer::start, m_udpServer.get()),
                               std::bind(&UdpServer::stop, m_udpServer.get()));
    
    m_threadManager->addThread("tcp_reactor",
                               std::bind(&TcpServer::start, m_tcpServer.get()),
                               std::bind(&TcpServer::stop, m_tcpServer.get()));

    m_threadManager->addThread("shard_manager",
                               std::bind(&ShardManager::start, m_shardManager.get()),
                               std::bind(&ShardManager::stop, m_shardManager.get()));

    m_threadManager->addThread("session_manager",
                               std::bind(&SessionManager::start, m_sessionManager.get()),
                               std::bind(&SessionManager::stop, m_sessionManager.get()));

    std::this_thread::sleep_for(std::chrono::milliseconds(3000));

    for (int i = 0; i < m_clients; ++i) {
        auto &client = m_clientList[i];
        m_threadManager->addThread("client_" + std::to_string(i + 1),
                std::bind(&Client::start, client.get()),
                std::bind(&Client::stop, client.get()));
    }
}

void Core::initializeClients() {
    m_clientList.clear();
    m_clientList.reserve(m_clients);
    for (int i = 0; i < m_clients; ++i) {
        m_clientList.emplace_back(std::make_unique<Client>(i + 1, m_udpServerPort, m_tcpServerPort));
    }
}

void Core::waitForShutdown() {
    while (m_running) {
        std::this_thread::sleep_for(std::chrono::seconds(3));
    }
}

void Core::handleSignal() {
    std::signal(SIGINT, Core::signalHandler);
    std::signal(SIGTERM, Core::signalHandler);
}

void Core::signalHandler(int /*signum*/) {
    m_running = false;
}

bool Core::initDatabase() {
    DbConfig db;
    db.server = "localhost,1433";
    db.database = "NFENGINE";
    db.user = "admin";
    db.password = "Admin123";
    db.poolSize = 4;

    m_dbManager = std::make_unique<DbManager>();
    CHECK_NULLPTR_RET_BOOL(m_dbManager, "DbManager");

    if (!m_dbManager->init(db)) {
        LOG_FATAL("Database Manager initialize failed.");
        return false;
    }
    return true;
}

bool Core::initThreadManager() {
    m_threadManager = std::make_unique<ThreadManager>();
    CHECK_NULLPTR_RET_BOOL(m_threadManager, "ThreadManager");
    return true;
}

bool Core::initShardManager() {
    m_shardManager = std::make_unique<ShardManager>(
            m_shardWorkerThread, m_threadManager.get(), m_dbManager.get());
    CHECK_NULLPTR_RET_BOOL(m_shardManager, "ShardManager");
    return true;
}

bool Core::initSessionManager() {
    m_sessionManager = std::make_unique<SessionManager>();
    CHECK_NULLPTR_RET_BOOL(m_sessionManager, "SessionManager");
    return true;
}
