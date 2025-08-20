/**
 * Copyright (c) 2025 PWO Team. All rights reserved.
 * This code is confidential and intended solely for internal use by authorized personnel.
 * Any unauthorized reproduction, distribution, or disclosure — including publication or sharing outside the company —
 * is strictly prohibited and may result in legal action.
 */

#ifndef CORE_LOG_H
#define CORE_LOG_H

enum class SeveretyLevel {
    Trace,
    Debug,
    Info,
    Warning,
    Error,
    Fatal,
};

class Log
{
    public:
        Log(SeveretyLevel severetyLevel, const std::string& message) :
            m_severetyLevel(severetyLevel), m_message(message) {}
        ~Log() = default;

        SeveretyLevel getSeveretyLevel() {
            return m_severetyLevel;
        }

        const std::string& getMessage() {
            return m_message;
        }

    private:
        SeveretyLevel m_severetyLevel;
        std::string m_message;
};

#endif
