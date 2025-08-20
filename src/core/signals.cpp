/**
 * Copyright (c) 2025 PWO Team. All rights reserved.
 * This code is confidential and intended solely for internal use by authorized personnel.
 * Any unauthorized reproduction, distribution, or disclosure — including publication or sharing outside the company —
 * is strictly prohibited and may result in legal action.
 */

#include "includes.h"

#include <core/signals.h>
#include <core/logger.h>
#include <core/tasks.h>

#include <database/database.h>
#include <network/connectionmanager.h>

#include <redis/redis.h>

Signals::Signals(boost::asio::io_context& io_context, ServerSharedPtr server) :
    m_set(io_context),
    m_server(server)
{
    m_set.add(SIGINT);

    asyncWait();
}

void Signals::asyncWait()
{
    m_set.async_wait([this] (boost::system::error_code err, int signal) {
        if (err) {
            g_logger.error("Signal handling error: " + err.message());
            return;
        }
        dispatchSignalHandler(signal);
        asyncWait();
    });
}

void Signals::dispatchSignalHandler(int signal)
{
    switch(signal) {
        case SIGINT: //Shuts the server down
            sigintHandler();
            break;
        default:
            break;
    }
}

void Signals::sigintHandler()
{
    g_logger.info("Gracefully stopping...");
    m_server.get()->close();
    g_dispatcher.join();
    g_redis->joinThreads();
    g_connectionManager.closeAll();
}
