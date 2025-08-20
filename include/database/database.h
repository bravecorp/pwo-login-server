/**
 * Copyright (c) 2025 PWO Team. All rights reserved.
 * This code is confidential and intended solely for internal use by authorized personnel.
 * Any unauthorized reproduction, distribution, or disclosure — including publication or sharing outside the company —
 * is strictly prohibited and may result in legal action.
 */

#ifndef DATABASE_H
#define DATABASE_H

#include <database/dbresult.h>

#include <utils/types.h>

struct Character {
    std::string name;
    std::string instanceName;
    std::string instanceId;
    uint16_t level;
    bool autoReconnect;
};

struct Account {
    uint16_t id = 0;
    std::string email;
    std::string password;
    uint64_t premiumEnd;
    std::vector<Character> characters;
};

class Database
{
    public:
        Database() = default;
        ~Database();

        // non-copyable
        Database(const Database&) = delete;
        Database& operator=(const Database&) = delete;

        void connect();
        void disconnect();
        DBResultSharedPtr storeQuery(const std::string& query);

        std::string getVersion() const {
            return mysql_get_client_info();
        }

        std::string escapeString(const std::string& string) const;

        Account getAccount(const std::string& email, const std::string& password);

    private:
        DBResultSharedPtr getAccountInfo(const std::string& email, const std::string& password);
        std::vector<Character> getCharacterList(uint16_t accountId);

        MYSQL* m_handle = nullptr;
        std::recursive_mutex m_databaseLock;
};

extern Database g_database;

#endif
