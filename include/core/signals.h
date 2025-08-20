/**
 * Copyright (c) 2025 PWO Team. All rights reserved.
 * This code is confidential and intended solely for internal use by authorized personnel.
 * Any unauthorized reproduction, distribution, or disclosure — including publication or sharing outside the company —
 * is strictly prohibited and may result in legal action.
 */

#ifndef CORE_SIGNALS_H
#define CORE_SIGNALS_H

#include <core/server.h>

class Signals
{
    boost::asio::signal_set m_set;
    ServerSharedPtr m_server;

    public:
        explicit Signals(boost::asio::io_context& io_context, ServerSharedPtr server);

    private:
        void asyncWait();
        void dispatchSignalHandler(int signal);

        void sigintHandler();
};

#endif
