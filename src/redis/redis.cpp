/**
 * Copyright (c) 2025 PWO Team. All rights reserved.
 * This code is confidential and intended solely for internal use by authorized personnel.
 * Any unauthorized reproduction, distribution, or disclosure — including publication or sharing outside the company —
 * is strictly prohibited and may result in legal action.
 */

#include <iostream>

#include <redis/redis.h>
#include <redis/pub.h>
#include <redis/sub.h>

#include <core/logger.h>
#include <core/tasks.h>

#include <script/lua.h>

#ifdef _WIN32
  #include <winsock2.h>
  #include <ws2tcpip.h>
  static WSADATA wsaData;
#else
  #include <sys/select.h>
  #include <unistd.h>
  #include <errno.h>
  #include <string.h>
#endif

RedisPtr g_redis = std::make_shared<Redis>();

Redis::~Redis()
{
	Redis::PlatformCleanup();
}

bool Redis::connect()
{
	Redis::PlatformInit();

	std::string host = g_config->get<std::string>("redisHost");
	int port = g_config->get<int>("redisPort");

	g_logger.info("Estabilishing subscriber connection...");
	if (!g_redisSubscriber->connect(host, port))
		return false;

	g_logger.info("Estabilishing publisher connection...");
	if (!g_redisPublisher->connect(host, port))
		return false;


	g_redisSubscriber->start();
	g_dispatcher.start();

	return true;
}

void Redis::joinThreads()
{
	g_redisSubscriber->join();
}

int Redis::Select(int fd, int timeout_sec)
{
	fd_set read_fds;
	FD_ZERO(&read_fds);
	FD_SET(fd, &read_fds);

	timeval timeout;
	timeout.tv_sec = timeout_sec;
	timeout.tv_usec = 0;

	int result = select(fd + 1, &read_fds, nullptr, nullptr, &timeout);

#ifdef _WIN32
	if (result == SOCKET_ERROR) {
		g_logger.error(fmt::format("select() failed: {:s}", WSAGetLastError()));
		return -1;
	}
#else
	if (result == -1) {
        g_logger.error(fmt::format("select() failed: {:s}", strerror(errno)));
		return -1;
	}
#endif

	return result;
}

void Redis::PlatformInit()
{
#ifdef _WIN32
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		g_logger.error("WSAStartup failed");
	}
#endif
}

void Redis::PlatformCleanup()
{
#ifdef _WIN32
	WSACleanup();
#endif
}
