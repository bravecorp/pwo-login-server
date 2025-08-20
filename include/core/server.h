/**
 * Copyright (c) 2025 PWO Team. All rights reserved.
 * This code is confidential and intended solely for internal use by authorized personnel.
 * Any unauthorized reproduction, distribution, or disclosure — including publication or sharing outside the company —
 * is strictly prohibited and may result in legal action.
 */

#ifndef CORE_SERVER_H
#define CORE_SERVER_H

#include <network/connection.h>

class Server : public std::enable_shared_from_this<Server>
{
    public:
        explicit Server(boost::asio::io_context& io_context) : m_io_context(io_context) {}
        ~Server();

        // non-copyable
        Server(const Server&) = delete;
        Server& operator=(const Server&) = delete;

        void open(const std::string& ip, int32_t port);
        void close();

        void onAccept(ConnectionSharedPtr connection, const boost::system::error_code& error);

    protected:
        void accept();

        boost::asio::io_context& m_io_context;
        std::unique_ptr<boost::asio::ip::tcp::acceptor> m_acceptor;
};

#endif
