/**
 * Copyright (c) 2025 PWO Team. All rights reserved.
 * This code is confidential and intended solely for internal use by authorized personnel.
 * Any unauthorized reproduction, distribution, or disclosure — including publication or sharing outside the company —
 * is strictly prohibited and may result in legal action.
 */

#include "includes.h"
#include "definitions.h"

#include <core/server.h>
#include <core/logger.h>
#include <core/signals.h>
#include <core/modulemanager.h>

#include <redis/redis.h>

#include <utils/rsa.h>

#include <database/database.h>

[[noreturn]] void badAllocationHandler() {
    // Use functions that only use stack allocation
    puts("Allocation failed, server out of memory.\nTry to compile in 64 bits mode.\n");
    getchar();
    exit(-1);
}

bool mainLoader();

int main(int argc, char* argv[]) {
    // Setup bad allocation handler
    std::set_new_handler(badAllocationHandler);

    if (mainLoader()) {
        std::string host = g_config->get<std::string>("host");
        int port = g_config->get<int>("port");

        boost::asio::io_context io_context;
        ServerSharedPtr server = std::make_shared<Server>(io_context);
        Signals signals(io_context, server);
        server.get()->open(host, port);
    } else {
        g_logger.fatal("The login server IS NOT online!");
    }

    return 0;
}

bool mainLoader() {

#ifdef _WIN32
    SetConsoleTitle((LPCWSTR)(SERVER_NAME));
#endif
    g_logger.info("Starting " + std::string(SERVER_NAME) + " Version " + SERVER_VERSION);
    g_logger.info("Compiled with " + std::string(BOOST_COMPILER));
    g_logger.info("Compiled on " + std::string(__DATE__) + ' ' + __TIME__);

#if defined(__amd64__) || defined(_M_X64)
    g_logger.info("Platform x64");
#elif defined(__i386__) || defined(_M_IX86) || defined(_X86_)
    g_logger.info("Platform x86");
#elif defined(__arm__)
    g_logger.info("Platform ARM");
#else
    g_logger.info("Platform unknown");
#endif

    g_logger.info("Loading key.pem");
    try {
        g_RSA.loadPEM();
    } catch(const std::exception& e) {
        g_logger.error("Failed to load key.pem: " + std::string(e.what()));
        return false;
    }

    g_logger.info("Loading lua");
    if (!g_lua->init())
        return false;

    g_logger.info("Establishing database connection...");
    try {
        g_database.connect();
        g_logger.info("MySQL " + g_database.getVersion());
    } catch (const std::exception& e) {
        g_logger.error("Failed to connect to database: " + std::string(e.what()));
        return false;
    }

    g_logger.info("Loading redis");
    if (!g_redis->connect())
        return false;

    g_logger.info("Loading modules");
	if (!g_modules->loadModules())
        return false;

    return true;
}
