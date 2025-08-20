/**
 * Copyright (c) 2025 PWO Team. All rights reserved.
 * This code is confidential and intended solely for internal use by authorized personnel.
 * Any unauthorized reproduction, distribution, or disclosure — including publication or sharing outside the company —
 * is strictly prohibited and may result in legal action.
 */

#include <fmt/format.h>

#include <redis/pub.h>
#include <core/logger.h>

RedisPublisherPtr g_redisPublisher = std::make_shared<RedisPublisher>();

RedisPublisher::~RedisPublisher()
{
    if (m_context)
        redisFree(m_context);
}

bool RedisPublisher::connect(const std::string& host, int port)
{
    m_context = redisConnect(host.c_str(), port);
    if (!m_context || m_context->err) {
        g_logger.fatal(fmt::format("[RedisPublisher] Failed to estabilish connection. address: {:s}:{:d}", host, port));
        return false;
    }

    return true;
}

bool RedisPublisher::publish(const std::string& channel, const std::string& data)
{
    redisReply* reply = (redisReply*)redisCommand(m_context, "PUBLISH %s %s", channel.c_str(), data.c_str());
    if (reply) {
        freeReplyObject(reply);
        return true;
    } else {
        g_logger.fatal(fmt::format("[RedisPublisher] Failed to publish to channel {:s} data: {:s}", channel, data.c_str()));
    }

    return false;
}
