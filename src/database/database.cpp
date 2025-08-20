/**
 * Copyright (c) 2025 PWO Team. All rights reserved.
 * This code is confidential and intended solely for internal use by authorized personnel.
 * Any unauthorized reproduction, distribution, or disclosure — including publication or sharing outside the company —
 * is strictly prohibited and may result in legal action.
 */

#include "includes.h"

#include <database/database.h>
#include <core/logger.h>
#include <script/lua.h>
#include <utils/tools.h>

Database g_database;

Database::~Database()
{
    if (m_handle) {
        mysql_close(m_handle);
    }
}

void Database::connect()
{
    m_handle = mysql_init(nullptr);
    if (!m_handle) {
        throw std::runtime_error("Failed to initialize MySQL connection handle.");
    }

    bool reconnect = true;
    mysql_options(m_handle, MYSQL_OPT_RECONNECT, &reconnect);

    bool result = mysql_real_connect(m_handle,
                g_config->get<const char*>("mysqlHost"),
                g_config->get<const char*>("mysqlUser"),
                g_config->get<const char*>("mysqlPass"),
                g_config->get<const char*>("mysqlDatabase"),
                g_config->get<int>("mysqlPort"),
                g_config->get<const char*>("mysqlSock"), 0);

    if (!result) {
        throw std::runtime_error(std::string(mysql_error(m_handle)));
    }
}

DBResultSharedPtr Database::storeQuery(const std::string& query)
{
    m_databaseLock.lock();
    if (mysql_real_query(m_handle, query.c_str(), query.length()) != 0) {
        g_logger.error("[mysql_real_query]: " + std::string(mysql_error(m_handle)));
        m_databaseLock.unlock();
        return nullptr;
    }

    MYSQL_RES* result = mysql_store_result(m_handle);
    if (!result) {
        g_logger.error("[mysql_store_result]: " + std::string(mysql_error(m_handle)));
        m_databaseLock.unlock();
        return nullptr;
    }
    m_databaseLock.unlock();

    DBResultSharedPtr res = std::make_shared<DBResult>(result);
    return res->hasNext() ? res : nullptr;
}

std::string Database::escapeString(const std::string& string) const
{
    // the worst case is 2n + 1
    size_t length = string.length();
    size_t maxLength = (length * 2) + 1;

    std::string escaped;
    escaped.reserve(maxLength + 2);
    escaped.push_back('\'');

    if (length != 0) {
        char* output = new char[maxLength];
        mysql_real_escape_string(m_handle, output, string.c_str(), length);
        escaped.append(output);
        delete[] output;
    }

    escaped.push_back('\'');
    return escaped;
}

Account Database::getAccount(const std::string& email, const std::string& password)
{
    Account account;

    auto accountInfo = getAccountInfo(email, password);
    if (!accountInfo) {
        return account;
    }

    account.email = email;
    account.password = password;
    account.id = accountInfo->getNumber<uint16_t>("id");
    account.premiumEnd = accountInfo->getNumber<uint64_t>("premium_ends_at");
    account.characters = getCharacterList(account.id);

    return account;
}

DBResultSharedPtr Database::getAccountInfo(const std::string& email, const std::string& password)
{
    std::string hashPass = transformToSHA1(g_config->get<std::string>("encryptionSalt") + password);
    auto result = storeQuery("SELECT `id`, `email`, `password`, `premium_ends_at` FROM `accounts` WHERE `email` = " + escapeString(email) + " AND password = " + escapeString(hashPass));
    return result;
}

std::vector<Character> Database::getCharacterList(uint16_t accountId)
{
    std::vector<Character> characters;

    auto result = storeQuery("SELECT `name`, `account_id`, `level`, `instance_id`, `instance_name`, `auto_reconnect` FROM `players` WHERE `account_id` = '" + std::to_string(accountId) + "'");
    if (result) {
        while(result->hasNext()) {
            Character character;
            character.name = result->getString("name");
            character.instanceName = result->getString("instance_name");
            character.instanceId = result->getString("instance_id");
            character.level = result->getNumber<uint16_t>("level");
            character.autoReconnect = static_cast<bool>(result->getNumber<int>("auto_reconnect"));
            characters.push_back(character);
            result->next();
        }
    }
    return characters;
}
