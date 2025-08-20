/**
 * Copyright (c) 2025 PWO Team. All rights reserved.
 * This code is confidential and intended solely for internal use by authorized personnel.
 * Any unauthorized reproduction, distribution, or disclosure — including publication or sharing outside the company —
 * is strictly prohibited and may result in legal action.
 */

#ifndef CORE_LOGGER_H
#define CORE_LOGGER_H

#include <list>
#include <core/log.h>

class Logger
{
    public:
        Logger();
        ~Logger();

        // non-copyable
        Logger(const Logger&) = delete;
        Logger& operator=(const Logger&) = delete;

        void writeLog(Log* log);

        void addLog(Log* log);

        void log(const std::string& message);
        void trace(const std::string& message);
        void debug(const std::string& message);
        void info(const std::string& message);
        void warning(const std::string& message);
        void error(const std::string& message);
        void fatal(const std::string& message);

        void printBacktrace(int level = 32);

    private:
        std::list<Log*> m_logList;
};

extern Logger g_logger;

#endif