/**
 * Copyright (c) 2025 PWO Team. All rights reserved.
 * This code is confidential and intended solely for internal use by authorized personnel.
 * Any unauthorized reproduction, distribution, or disclosure — including publication or sharing outside the company —
 * is strictly prohibited and may result in legal action.
 */

#ifndef NETWORK_CONNECTION_H
#define NETWORK_CONNECTION_H

#include <includes.h>
#include <utils/types.h>
#include <network/networkmessage.h>

static constexpr int32_t CONNECTION_WRITE_TIMEOUT = 30;
static constexpr int32_t CONNECTION_READ_TIMEOUT = 30;

class Connection : public std::enable_shared_from_this<Connection>
{
    public:
        // non-copyable
        Connection(const Connection&) = delete;
        Connection& operator=(const Connection&) = delete;

        Connection(boost::asio::io_context& io_context) :
            m_readTimer(io_context),
            m_writeTimer(io_context),
            m_socket(io_context) {}
        ~Connection();

        void close();
        void accept();

        void send(OutputMessage& msg);

        uint32_t getIP();
        uint64_t getId() const { return m_id; }

    private:
        void parseHeader(const boost::system::error_code& error);
        void parsePacket(const boost::system::error_code& error);

        void onWriteOperation(const boost::system::error_code& error);

        static void handleTimeout(ConnectionWeakPtr connectionWeak, const boost::system::error_code& error);

        void closeSocket();
        void internalSend(OutputMessage& msg);

        boost::asio::ip::tcp::socket& getSocket() {
            return m_socket;
        }

        NetworkMessage m_msg;

        boost::asio::deadline_timer m_readTimer;
        boost::asio::deadline_timer m_writeTimer;

        std::recursive_mutex m_connectionLock;

        ProtocolSharedPtr m_protocol;

        boost::asio::ip::tcp::socket m_socket;

        bool m_closed = false;
        bool m_receivedFirst = false;

        uint64_t m_id = 0;

        friend class ConnectionManager;
        friend class Server;
};

#endif
