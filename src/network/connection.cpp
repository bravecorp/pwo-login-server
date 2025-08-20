/**
 * Copyright (c) 2025 PWO Team. All rights reserved.
 * This code is confidential and intended solely for internal use by authorized personnel.
 * Any unauthorized reproduction, distribution, or disclosure — including publication or sharing outside the company —
 * is strictly prohibited and may result in legal action.
 */

#include "includes.h"

#include <network/connection.h>
#include <network/connectionmanager.h>
#include <network/outputmessage.h>
#include <network/protocol.h>

#include <core/server.h>
#include <core/logger.h>

void Connection::close()
{
    std::lock_guard<std::recursive_mutex> lockClass(m_connectionLock);
    g_connectionManager.releaseConnection(shared_from_this());

    if (m_closed)
        return;

    m_closed = true;

    closeSocket();
}

void Connection::closeSocket()
{
    if (m_socket.is_open()) {
        try {
            m_readTimer.cancel();
            m_writeTimer.cancel();
            boost::system::error_code error;
            m_socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, error);
            m_socket.close(error);
        } catch (boost::system::system_error& e) {
            g_logger.error("Network error: " + std::string(e.what()));
        }
    }
}

Connection::~Connection()
{
    closeSocket();
}

void Connection::accept()
{
    m_protocol = std::make_shared<Protocol>(shared_from_this());

    std::lock_guard<std::recursive_mutex> lockClass(m_connectionLock);
    try {
        m_readTimer.expires_from_now(boost::posix_time::seconds(CONNECTION_READ_TIMEOUT));
        m_readTimer.async_wait(std::bind(&Connection::handleTimeout, std::weak_ptr<Connection>(shared_from_this()), std::placeholders::_1));

        // Read size of the first packet
        boost::asio::async_read(m_socket,
                                boost::asio::buffer(m_msg.getBuffer(), NetworkMessage::HEADER_LENGTH),
                                std::bind(&Connection::parseHeader, shared_from_this(), std::placeholders::_1));
    } catch (boost::system::system_error& e) {
        g_logger.error("Network error: " + std::string(e.what()));
        close();
    }
}

void Connection::parseHeader(const boost::system::error_code& error)
{
    std::lock_guard<std::recursive_mutex> lockClass(m_connectionLock);
    m_readTimer.cancel();

    if (error) {
        close();
        return;
    }

    uint16_t size = m_msg.getLengthHeader();
    if (size == 0 || size >= NETWORKMESSAGE_MAXSIZE - 16) {
        close();
        return;
    }

    try {
        m_readTimer.expires_from_now(boost::posix_time::seconds(CONNECTION_READ_TIMEOUT));
        m_readTimer.async_wait(std::bind(&Connection::handleTimeout, std::weak_ptr<Connection>(shared_from_this()),
                                        std::placeholders::_1));

        // Read packet content
        m_msg.setLength(size + NetworkMessage::HEADER_LENGTH);
        boost::asio::async_read(m_socket, boost::asio::buffer(m_msg.getBodyBuffer(), size),
                                std::bind(&Connection::parsePacket, shared_from_this(), std::placeholders::_1));
    } catch (boost::system::system_error& e) {
        g_logger.error("Network error: " + std::string(e.what()));
        close();
    }
}

void Connection::parsePacket(const boost::system::error_code& error)
{
    std::lock_guard<std::recursive_mutex> lockClass(m_connectionLock);
    m_readTimer.cancel();

    if (error) {
        close();
        return;
    } else if (m_closed) {
        return;
    }

    //Check packet checksum
    uint32_t checksum;
    int32_t len = m_msg.getLength() - m_msg.getBufferPosition() - NetworkMessage::CHECKSUM_LENGTH;
    if (len > 0) {
        checksum = adlerChecksum(m_msg.getBuffer() + m_msg.getBufferPosition() + NetworkMessage::CHECKSUM_LENGTH, len);
    } else {
        checksum = 0;
    }

    uint32_t recvChecksum = m_msg.get<uint32_t>();
    if (recvChecksum != checksum) {
        // it might not have been the checksum, step back
        m_msg.skipBytes(-NetworkMessage::CHECKSUM_LENGTH);
    }

    if (m_receivedFirst) {
        m_protocol->parsePacket(m_msg);
    } else {
        m_receivedFirst = true;
        m_msg.skipBytes(1); // Skip protocol ID
        m_protocol->authenticate(m_msg);
    }

    try {
        m_readTimer.expires_from_now(boost::posix_time::seconds(CONNECTION_READ_TIMEOUT));
        m_readTimer.async_wait(std::bind(&Connection::handleTimeout, std::weak_ptr<Connection>(shared_from_this()),
            std::placeholders::_1));

        // Wait to the next packet
        boost::asio::async_read(m_socket, boost::asio::buffer(m_msg.getBuffer(), NetworkMessage::HEADER_LENGTH),
            std::bind(&Connection::parseHeader, shared_from_this(), std::placeholders::_1));
	} catch (boost::system::system_error& e) {
        g_logger.error("Network error: " + std::string(e.what()));
        close();
	}
}

void Connection::send(OutputMessage& msg)
{
    std::lock_guard<std::recursive_mutex> lockClass(m_connectionLock);
    internalSend(msg);
}

void Connection::internalSend(OutputMessage& msg)
{
    m_protocol->encryptMessage(msg);
    try {
        m_writeTimer.expires_from_now(boost::posix_time::seconds(CONNECTION_WRITE_TIMEOUT));
        m_writeTimer.async_wait(std::bind(&Connection::handleTimeout, std::weak_ptr<Connection>(shared_from_this()),
            std::placeholders::_1));

        boost::asio::async_write(m_socket,
            boost::asio::buffer(msg.getOutputBuffer(), msg.getLength()),
            std::bind(&Connection::onWriteOperation, shared_from_this(), std::placeholders::_1));
    } catch (boost::system::system_error& e) {
        g_logger.error("Network error: " + std::string(e.what()));
        close();
    }
}

uint32_t Connection::getIP()
{
    std::lock_guard<std::recursive_mutex> lockClass(m_connectionLock);

    // IP-address is expressed in network byte order
    boost::system::error_code error;
    const boost::asio::ip::tcp::endpoint endpoint = m_socket.remote_endpoint(error);
    if (error) {
        return 0;
    }

    auto bytes = endpoint.address().to_v4().to_bytes();
    uint32_t ip = (static_cast<uint32_t>(bytes[0]) << 24) |
        (static_cast<uint32_t>(bytes[1]) << 16) |
        (static_cast<uint32_t>(bytes[2]) << 8) |
        (static_cast<uint32_t>(bytes[3]));

    return ntohl(ip);
}

void Connection::onWriteOperation(const boost::system::error_code& error)
{
    std::lock_guard<std::recursive_mutex> lockClass(m_connectionLock);
    m_writeTimer.cancel();

    if (error) {
        close();
        return;
    }
}

void Connection::handleTimeout(ConnectionWeakPtr connectionWeak, const boost::system::error_code& error)
{
    if (error == boost::asio::error::operation_aborted) {
        return;
    }

    if (auto connection = connectionWeak.lock()) {
        connection->close();
    }
}
