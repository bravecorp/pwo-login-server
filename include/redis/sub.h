/**
 * Copyright (c) 2025 PWO Team. All rights reserved.
 * This code is confidential and intended solely for internal use by authorized personnel.
 * Any unauthorized reproduction, distribution, or disclosure — including publication or sharing outside the company —
 * is strictly prohibited and may result in legal action.
 */

#ifndef REDIS_SUB_H
#define REDIS_SUB_H

#include <mutex>
#include <hiredis/hiredis.h>

#include <core/threadholder.h>
#include <script/lua.h>

class RedisSubscriber : public ThreadHolder<RedisSubscriber>
{
    public:
        ~RedisSubscriber();

        bool connect(const std::string& host, int port);
        bool subscribe(const std::string& channel);

        void threadMain();

    private:
        std::mutex m_mutex;
        redisContext* m_context = nullptr;
};

extern RedisSubscriberPtr g_redisSubscriber;

#endif
