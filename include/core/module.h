/**
 * Copyright (c) 2025 PWO Team. All rights reserved.
 * This code is confidential and intended solely for internal use by authorized personnel.
 * Any unauthorized reproduction, distribution, or disclosure — including publication or sharing outside the company —
 * is strictly prohibited and may result in legal action.
 */

#ifndef CORE_MODULE_H
#define CORE_MODULE_H

#include <tuple>
#include <filesystem>
#include <unordered_map>

#include <script/lua.h>
#include <utils/types.h>

class ModuleManager;

class Module {
    public:
        Module(ModuleManager* manager, const std::string& name, const std::string& path);
        ~Module() = default;

        // non-copyable
        Module(const Module&) = delete;
        Module& operator=(const Module&) = delete;

        bool load();
        void unload();

        bool connect(const std::string& event, int32_t callback, const std::string& identifier = std::string());
        bool connectOnce(const std::string& event, int32_t callback, const std::string& identifier = std::string());

        void disconnect(const std::string& event, int32_t callback, bool clearList = true);
        void disconnect(const std::string& event, const std::string& identifier, bool clearList = true);

        void freeConnections();

        bool exportLua(const std::string& name, int32_t ref);

        std::string getName() { return m_name; }

        int getSandboxEnv() const { return m_sandboxEnv; }

        const std::vector<int32_t>& getEventCallback(const std::string& event);

        bool hasDependencies() { return !m_dependencies.empty(); }
        const StringVector& getDependencies() { return m_dependencies; }

    private:
        void loadDependencies();
        bool loadFiles();

        std::string m_name;
        std::string m_path;

        std::unordered_map<std::string, std::vector<int32_t>> m_eventCallbacks;
        std::unordered_map<std::string, std::unordered_map<std::string, int32_t>> m_identifiedEventCallbacks;
        std::unordered_map<std::string, StringVector> m_identifiedOnceConnects;

        std::vector<int32_t> m_onceConnects;

        StringVector m_dependencies;

        int m_sandboxEnv = -1;

        ModuleManager* m_manager = nullptr;

    friend class ModuleManager;
};

#endif
