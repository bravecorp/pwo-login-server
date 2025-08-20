/**
 * Copyright (c) 2025 PWO Team. All rights reserved.
 * This code is confidential and intended solely for internal use by authorized personnel.
 * Any unauthorized reproduction, distribution, or disclosure — including publication or sharing outside the company —
 * is strictly prohibited and may result in legal action.
 */

#ifndef NETWORK_PROTOCOL_H
#define NETWORK_PROTOCOL_H

static constexpr auto AUTHENTICATOR_PERIOD = 30U;

#include <utils/types.h>
#include <network/connection.h>
#include <database/database.h>

enum Opcode {
    Authenticate = 1,
    Error = 2,
    Motd = 3,
    CharacterList = 4,
    SessionKey = 5,
    Ping = 6,
    LoadingMessage = 7,
};

class Protocol : public std::enable_shared_from_this<Protocol>
{
    public:
        explicit Protocol(ConnectionSharedPtr connection) : m_connection(connection) {}
        ~Protocol() = default;

        // non-copyable
        Protocol(const Protocol&) = delete;
        Protocol& operator=(const Protocol&) = delete;

        void encryptMessage(OutputMessage& msg);
        void authenticate(NetworkMessage& msg);
        void parsePacket(NetworkMessage& msg);

        ConnectionSharedPtr getConnection() const {
            return m_connection.lock();
        }

        uint32_t getId() const;

        void send(OutputMessage& msg) const {
            if (auto m_connection = getConnection()) {
                m_connection->send(msg);
            }
        }

        void sendError(const std::string& message) const;
        void sendLoadingMessage(const std::string& message) const;
        void disconnectClient(const std::string& message) const;

    private:
        void addMOTD(OutputMessage& msg);
        void addSessionKey(OutputMessage& msg);
        void addCharacterList(OutputMessage& msg);
        void disconnect() const {
            if (auto m_connection = getConnection()) {
                m_connection->close();
            }
        }
        void setXTEAKey(const uint32_t* key) {
            memcpy(this->m_key, key, sizeof(*key) * 4);
        }

        friend class Connection;

    private:
        time_t m_lastPingTime = 0;
        uint32_t m_key[4] = {};
        Account m_account;
        const ConnectionWeakPtr m_connection;
};

#endif
