#include "TcpReactor.h"

#include "protocol/tcp/TcpWorker.h"
#include "protocol/tls/TlsServer.h"

TcpReactor::TcpReactor(int port, TcpWorker* tcpWorker, std::shared_ptr<TlsServer> tlsServer)
{

}

TcpReactor::~TcpReactor()
{

}

void TcpReactor::start()
{

}

void TcpReactor::stop()
{

}
