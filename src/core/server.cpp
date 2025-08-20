/**
 * Copyright (c) 2025 PWO Team. All rights reserved.
 * This code is confidential and intended solely for internal use by authorized personnel.
 * Any unauthorized reproduction, distribution, or disclosure — including publication or sharing outside the company —
 * is strictly prohibited and may result in legal action.
 */

#include "includes.h"

#include <network/outputmessage.h>
#include <network/connectionmanager.h>

#include <core/server.h>
#include <core/logger.h>

Server::~Server()
{
    close();
}

void Server::accept()
{
    if (!m_acceptor) {
        return;
    }

    auto connection = g_connectionManager.createConnection(m_io_context);
    m_acceptor->async_accept(connection->getSocket(), std::bind(&Server::onAccept, shared_from_this(), connection, std::placeholders::_1));
}

void Server::onAccept(ConnectionSharedPtr connection, const boost::system::error_code& error)
{
    if (!error) {
        auto remote_ip = connection->getIP();
        if (remote_ip != 0) {
            connection->accept();
        } else {
            connection->close();
        }

        accept();
    }
}

void Server::open(const std::string& ip, int32_t port)
{
    try {
        boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::make_address_v4(ip), static_cast<unsigned short>(port));

        m_acceptor.reset(new boost::asio::ip::tcp::acceptor(m_io_context, endpoint));
        m_acceptor->set_option(boost::asio::ip::tcp::no_delay(true));

        g_logger.info("Listening on tcp://" + ip + ":" + std::to_string(port));

        accept();
        m_io_context.run();
    } catch (boost::system::system_error& e) {
        g_logger.info("Failed to bind at address tcp://" + ip + ":" + std::to_string(port) + ": " + e.what());
    }
}

void Server::close()
{
    if (m_acceptor && m_acceptor->is_open()) {
        boost::system::error_code error;
        m_acceptor->close(error);
    }
    m_io_context.stop();
}
