/**
 * Copyright (c) 2025 PWO Team. All rights reserved.
 * This code is confidential and intended solely for internal use by authorized personnel.
 * Any unauthorized reproduction, distribution, or disclosure — including publication or sharing outside the company —
 * is strictly prohibited and may result in legal action.
 */

#ifndef NETWORK_CONNECTIONMANAGER_H
#define NETWORK_CONNECTIONMANAGER_H

#include <network/connection.h>

class Protocol;

class ConnectionManager
{
    static uint64_t CONNECTION_ID_GENERATOR;

    public:
        ConnectionManager() = default;
        ~ConnectionManager() = default;

        // non-copyable
        ConnectionManager(const ConnectionManager&) = delete;
        ConnectionManager& operator=(const ConnectionManager&) = delete;

        ConnectionSharedPtr createConnection(boost::asio::io_context& io_context);
        void releaseConnection(const ConnectionSharedPtr& connection);
        void closeAll();

        ProtocolSharedPtr getProtocolById(uint64_t ip);

    protected:
        std::unordered_set<ConnectionSharedPtr> m_connections;
        std::mutex m_connectionManagerLock;
};

extern ConnectionManager g_connectionManager;

#endif
