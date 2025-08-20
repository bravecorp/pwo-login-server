/**
 * Copyright (c) 2025 PWO Team. All rights reserved.
 * This code is confidential and intended solely for internal use by authorized personnel.
 * Any unauthorized reproduction, distribution, or disclosure — including publication or sharing outside the company —
 * is strictly prohibited and may result in legal action.
 */

#ifndef REDIS_H
#define REDIS_H

#include <utils/types.h>

class Redis
{
    public:
        ~Redis();

        bool connect();
        void joinThreads();

        static int Select(int fd, int timeout_sec = 1);
        static void PlatformInit();
        static void PlatformCleanup();
};

extern RedisPtr g_redis;

#endif
