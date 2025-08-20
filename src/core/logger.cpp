/**
 * Copyright (c) 2025 PWO Team. All rights reserved.
 * This code is confidential and intended solely for internal use by authorized personnel.
 * Any unauthorized reproduction, distribution, or disclosure — including publication or sharing outside the company —
 * is strictly prohibited and may result in legal action.
 */

#include "includes.h"

#include <fmt/color.h>

#include <core/logger.h>

#include <utils/tools.h>

#if defined(_WIN32)
#include <windows.h>
#include <dbghelp.h>
#pragma comment(lib, "dbghelp.lib")
#else
#include <execinfo.h>
#include <cstdlib>
#endif

Logger g_logger;

Logger::~Logger()
{
    for (auto log : m_logList) {
        writeLog(log);
        delete log;
    }
}

Logger::Logger()
{

}

void Logger::addLog(Log* log)
{
    // if the list is empty we have to signal it
    bool do_signal = m_logList.empty();

    // add log to the queue
    m_logList.push_back(log);
}

void Logger::writeLog(Log* log)
{
    // TODO Write Log
}

void Logger::trace(const std::string& message)
{
    fmt::print(fg(fmt::color::forest_green) | fmt::emphasis::bold, "[TRACE]: {:s}\n", message);
    std::cout << std::flush;
    //addLog(new Log(SeveretyLevel::Trace, message));
}

void Logger::debug(const std::string& message)
{
    fmt::print(fg(fmt::color::dark_slate_blue) | fmt::emphasis::bold, "[DEBUG]: {:s}\n", message);
    std::cout << std::flush;
    //addLog(new Log(SeveretyLevel::Debug, message));
}

void Logger::info(const std::string& message)
{
    fmt::print(fmt::emphasis::bold, "[INFO]: {:s}\n", message);
    std::cout << std::flush;
    //addLog(new Log(SeveretyLevel::Info, message));
}

void Logger::warning(const std::string& message)
{
    fmt::print(fg(fmt::color::gold) | fmt::emphasis::bold, "[WARNING]: {:s}\n", message);
    std::cout << std::flush;
    //addLog(new Log(SeveretyLevel::Warning, message));
}

void Logger::error(const std::string& message)
{
    fmt::print(fg(fmt::color::crimson) | fmt::emphasis::bold, "[ERROR]: {:s}\n", message);
    std::cout << std::flush;
    //addLog(new Log(SeveretyLevel::Error, message));
}

void Logger::fatal(const std::string& message)
{
    fmt::print(fg(fmt::color::crimson) | fmt::emphasis::bold, "[FATAL]: {:s}\n", message);
    std::cout << std::flush;
    //addLog(new Log(SeveretyLevel::Fatal, message));
}

void Logger::printBacktrace(int level)
{
    std::stringstream ss;

#if defined(_WIN32)
    // Windows version
    void* stack[64];
    USHORT frames = CaptureStackBackTrace(0, level, stack, nullptr);

    HANDLE process = GetCurrentProcess();
    SymSetOptions(SYMOPT_DEFERRED_LOADS | SYMOPT_INCLUDE_32BIT_MODULES | SYMOPT_UNDNAME);
    SymInitialize(process, NULL, TRUE);

    SYMBOL_INFO* symbol = (SYMBOL_INFO*)calloc(sizeof(SYMBOL_INFO) + 256, 1);
    symbol->MaxNameLen = 255;
    symbol->SizeOfStruct = sizeof(SYMBOL_INFO);

    for (USHORT i = 1; i < frames; i++) {
        DWORD64 address = (DWORD64)(stack[i]);
        if (SymFromAddr(process, address, 0, symbol)) {
            ss << "#" << i - 1 << " " << demangleName(symbol->Name) << " - 0x"
                << std::hex << symbol->Address << std::dec << "\n";
        } else {
            ss << "#" << i - 1 << " ??? - 0x" << std::hex << address << std::dec << "\n";
        }
    }

    free(symbol);
#else
    // Linux/macOS version
    void* buffer[64];
    int nptrs = backtrace(buffer, level);
    char** strings = backtrace_symbols(buffer, nptrs);
    if (strings == nullptr) {
        ss << "Error: backtrace_symbols returned nullptr\n";
    }

    for (int i = 1; i < nptrs; i++) {
        std::string line = strings[i];
        std::size_t begin = line.find('(');
        std::size_t end = line.find('+', begin);
        if (begin != std::string::npos && end != std::string::npos && begin + 1 < end) {
            std::string mangled = line.substr(begin + 1, end - begin - 1);
            std::string demangled = demangleName(mangled.c_str());
            line.replace(begin + 1, end - begin - 1, demangled);
        }
        ss << "#" << i - 1 << " " << line << "\n";
    }
    free(strings);
#endif
    trace(ss.str());
}
