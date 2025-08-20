/**
 * Copyright (c) 2025 PWO Team. All rights reserved.
 * This code is confidential and intended solely for internal use by authorized personnel.
 * Any unauthorized reproduction, distribution, or disclosure — including publication or sharing outside the company —
 * is strictly prohibited and may result in legal action.
 */

#include <fmt/format.h>

#include <redis/redis.h>
#include <redis/sub.h>

#include <core/modulemanager.h>
#include <core/logger.h>
#include <core/tasks.h>

RedisSubscriberPtr g_redisSubscriber = std::make_shared<RedisSubscriber>();

RedisSubscriber::~RedisSubscriber()
{
    if (m_context)
        redisFree(m_context);
}

bool RedisSubscriber::connect(const std::string& host, int port)
{
    m_context = redisConnect(host.c_str(), port);
    if (!m_context || m_context->err) {
        g_logger.error(fmt::format("[RedisSubscriber] Failed to estabilish connection. address: {:s}:{:d}", host, port));
        return false;
    }

    return true;
}

bool RedisSubscriber::subscribe(const std::string& channel)
{
    redisReply* reply = (redisReply*)redisCommand(m_context, "SUBSCRIBE %s", channel.c_str());
    if (reply) {
        freeReplyObject(reply);
        return true;
    } else {
        g_logger.error(fmt::format("[RedisSubscriber] Failed to subscribe to channel: {:s}", channel));
    }

    return false;
}

void RedisSubscriber::threadMain()
{
    const int redis_fd = (const int)m_context->fd;

	while (getState() != ThreadState::Terminated) {
		int ready = Redis::Select(redis_fd, 1);
		if (ready < 0) {
			break;
		} else if (ready == 0) {
			continue;
		}

		if (redisBufferRead(m_context) != REDIS_OK) {
			g_logger.error(fmt::format("[RedisSubscriber] redisBufferRead() failed: {:s}", m_context->errstr));
			break;
		}

		redisReply* reply = nullptr;
		while (redisGetReplyFromReader(m_context, (void**)&reply) == REDIS_OK && reply != nullptr) {
			if (reply->type == REDIS_REPLY_ARRAY && reply->elements == 3) {
				const std::string type = reply->element[0]->str;

				std::string channel;
				if (reply->element[1]->type == REDIS_REPLY_STRING && reply->element[1]->str)
						channel = reply->element[1]->str;

				if (channel.empty()) {
					g_logger.error(fmt::format("[RedisSubscriber] Not found channel. type: {:s}",type));
					continue;
				}

				if (type == "message" && reply->element[0]->type == REDIS_REPLY_STRING) {
					std::string message;

					if (reply->element[2]->type == REDIS_REPLY_STRING && reply->element[2]->str)
						message = reply->element[2]->str;

					if (message.empty()) {
						g_logger.error(fmt::format("[RedisSubscriber] Not found message. channel: {:s}", channel));
						continue;
					}

					g_dispatcher.addTask(createTask([this, channel, message]() {
						std::lock_guard<std::mutex> lock(m_mutex);
						g_modules->emitNoRet("onRedisMessage", channel.c_str(), std::tuple{ "message", message.c_str() });
					}));
				}
			}
			freeReplyObject(reply);
		}
	}
}
