/**
 * Copyright (c) 2025 PWO Team. All rights reserved.
 * This code is confidential and intended solely for internal use by authorized personnel.
 * Any unauthorized reproduction, distribution, or disclosure — including publication or sharing outside the company —
 * is strictly prohibited and may result in legal action.
 */

#include "includes.h"

#include <network/connectionmanager.h>
#include <network/protocol.h>

uint64_t ConnectionManager::CONNECTION_ID_GENERATOR = 0;
ConnectionManager g_connectionManager;

ConnectionSharedPtr ConnectionManager::createConnection(boost::asio::io_context& io_context)
{
    std::lock_guard<std::mutex> lockClass(m_connectionManagerLock);

    auto connection = std::make_shared<Connection>(io_context);
    connection->m_id = ++CONNECTION_ID_GENERATOR;
    m_connections.insert(connection);
    return connection;
}

void ConnectionManager::releaseConnection(const ConnectionSharedPtr& connection)
{
    std::lock_guard<std::mutex> lockClass(m_connectionManagerLock);
    m_connections.erase(connection);
}

void ConnectionManager::closeAll()
{
    std::lock_guard<std::mutex> lockClass(m_connectionManagerLock);

    for (const auto& connection : m_connections) {
        try {
            boost::system::error_code error;
            connection->m_socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, error);
            connection->m_socket.close(error);
        } catch (boost::system::system_error&) {
        }
    }
    m_connections.clear();
}

ProtocolSharedPtr ConnectionManager::getProtocolById(uint64_t id)
{
    for (ConnectionSharedPtr connection : m_connections) {
        if (connection->m_id == id)
            return connection->m_protocol;
    }
    return nullptr;
}
