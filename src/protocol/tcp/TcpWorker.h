#pragma once

#include "ingress/RxRouter.h"
#include "util/ThreadManager.h"

class TcpWorker
{
public:
    TcpWorker(RxRouter* rxRouter, int worker, ThreadManager* threadManager);
    ~TcpWorker();

    void start();
    void stop();
};
