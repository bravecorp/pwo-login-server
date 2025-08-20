/**
 * Copyright (c) 2025 PWO Team. All rights reserved.
 * This code is confidential and intended solely for internal use by authorized personnel.
 * Any unauthorized reproduction, distribution, or disclosure — including publication or sharing outside the company —
 * is strictly prohibited and may result in legal action.
 */

#ifndef REDIS_PUB_H
#define REDIS_PUB_H

#include <hiredis/hiredis.h>

#include <utils/types.h>
#include <script/lua.h>

class RedisPublisher
{
    public:
        ~RedisPublisher();

        bool connect(const std::string& host, int port);
        bool publish(const std::string& channel, const std::string& data);

    private:
        redisContext* m_context = nullptr;
};

extern RedisPublisherPtr g_redisPublisher;

#endif
