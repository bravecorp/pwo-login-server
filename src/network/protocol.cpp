/**
 * Copyright (c) 2025 PWO Team. All rights reserved.
 * This code is confidential and intended solely for internal use by authorized personnel.
 * Any unauthorized reproduction, distribution, or disclosure — including publication or sharing outside the company —
 * is strictly prohibited and may result in legal action.
 */

#include "includes.h"

#include <network/protocol.h>
#include <network/outputmessage.h>

#include <utils/rsa.h>
#include <utils/xtea.h>

#include <database/database.h>

#include <core/logger.h>
#include <core/modulemanager.h>

#include <script/lua.h>

#include <vector>

void Protocol::addMOTD(OutputMessage& msg)
{
    msg.addByte(Opcode::Motd);
    std::ostringstream ss;
    ss << g_config->get<int>("motdNumber", 0) << "\n";
    ss << g_config->get<std::string>("motdMessage");
    msg.addString(ss.str());
}

void Protocol::addSessionKey(OutputMessage& msg)
{
    uint32_t ticks = time(nullptr) / AUTHENTICATOR_PERIOD;
    msg.addByte(Opcode::SessionKey);
    msg.addString(m_account.email + "\n" + m_account.password + "\n\n" + std::to_string(ticks));
}

void Protocol::addCharacterList(OutputMessage& msg)
{
    msg.addByte(Opcode::CharacterList);

    auto characters = m_account.characters;
    msg.addByte(characters.size()); // number of chars
    for (auto character : characters) {
        msg.addString(character.name);
        msg.addString(character.instanceName);
        msg.addString(character.instanceId);
        msg.add<uint16_t>(character.level);
        msg.add<uint8_t>(static_cast<uint8_t>(character.autoReconnect));
    }

    //Add premium days
    msg.addByte(0);
    msg.addByte(m_account.premiumEnd > static_cast<uint64_t>(time(nullptr)) ? 1 : 0);
    msg.add<uint32_t>(m_account.premiumEnd);
}

void Protocol::authenticate(NetworkMessage& msg)
{
    msg.skipBytes(2); // operating system

    uint16_t version = msg.get<uint16_t>();

    msg.skipBytes(17);
    /*
     * Skipped bytes:
     * 4 bytes: protocolVersion
     * 12 bytes: dat, spr, pic signatures (4 bytes each)
     * 1 byte: 0
     */

    if (!g_RSA.decrypt(msg)) {
        disconnect();
        return;
    }

    uint32_t key[4];
    key[0] = msg.get<uint32_t>();
    key[1] = msg.get<uint32_t>();
    key[2] = msg.get<uint32_t>();
    key[3] = msg.get<uint32_t>();
    setXTEAKey(key);

    uint16_t versionMin = g_config->get<uint16_t>("versionMin");
    if (version < versionMin) {
        std::string versionStr = g_config->get<std::string>("versionStr");
        disconnectClient("Only clients with protocol " + versionStr + " allowed!");
        return;
    }

    std::string email = msg.getString();
    if (email.empty()) {
        disconnectClient("Invalid account email.");
        return;
    }

    std::string password = msg.getString();
    if (password.empty()) {
        disconnectClient("Invalid password.");
        return;
    }

    m_account = g_database.getAccount(email, password);
    if (!m_account.id) {
        disconnectClient("Invalid account email or password.");
        return;
    }

    OutputMessage output;

    addMOTD(output);
    addSessionKey(output);
    addCharacterList(output);

    send(output);
}

void Protocol::parsePacket(NetworkMessage& msg)
{
    if (!g_XTEA.decrypt(m_key, msg))
        return;

    uint8_t opcode = msg.getByte();
    if (opcode == Opcode::Ping) {
        m_lastPingTime = time(nullptr);
        return;
    }

    g_modules->emitNoRet("onReceiveNetworkMessage", std::to_string(opcode), std::tuple{"client", shared_from_this()}, std::tuple{"msg", &msg});
}

void Protocol::sendError(const std::string& message) const
{
    OutputMessage msg;
    msg.addByte(Opcode::Error);
    msg.addString(message);
    send(msg);
}

void Protocol::sendLoadingMessage(const std::string& message) const
{
    OutputMessage msg;
    msg.addByte(Opcode::LoadingMessage);
    msg.addString(message);
    send(msg);
}

void Protocol::disconnectClient(const std::string& message) const
{
    sendError(message);
    disconnect();
}

void Protocol::encryptMessage(OutputMessage& msg)
{
    msg.writeMessageLength();
    g_XTEA.encrypt(m_key, msg);
    msg.addCryptoHeader();
}

uint32_t Protocol::getId() const
{
    if (ConnectionSharedPtr connection = getConnection())
        return connection->getId();
    return 0;
}
