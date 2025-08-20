/**
 * Copyright (c) 2025 PWO Team. All rights reserved.
 * This code is confidential and intended solely for internal use by authorized personnel.
 * Any unauthorized reproduction, distribution, or disclosure — including publication or sharing outside the company —
 * is strictly prohibited and may result in legal action.
 */

#ifndef CORE_MODULEMANAGER_H
#define CORE_MODULEMANAGER_H

#include <core/module.h>
#include <script/lua.h>
#include <utils/tools.h>

static std::string EMPTY_IDENTIFIER;

class ModuleManager
{
    public:
        ModuleManager() = default;
        ~ModuleManager();

        // non-copyable
        ModuleManager(const ModuleManager&) = delete;
        ModuleManager& operator=(const ModuleManager&) = delete;

        template<typename... T>
        void emitNoRet(const std::string& event, const std::string& identifier = std::string(), T&&... args) {
            if (identifier.empty()) {
                if (m_moduleEvents.find(event) != m_moduleEvents.end()) {
                    auto& modules = m_moduleEvents[event];

                    for (auto& module : modules) {
                        for (int32_t callback : module->getEventCallback(event)) {
                            g_lua->callSandboxLuaFieldNoRet(callback, module->getSandboxEnv(), std::forward<T>(args)...);
                            checkConnectOnce(module, event, callback);
                        }
                    }
                }

            } else {
                auto& modules = m_identifiedModuleEvents[event];
                int32_t callback { 0 };

                for (auto& module : modules) {
                    auto& eventMap = module->m_identifiedEventCallbacks[event];

                    if (eventMap.find(identifier) != eventMap.end()) {
                        if (eventMap[identifier] != callback) {
                            callback = eventMap[identifier];
                            g_lua->callSandboxLuaFieldNoRet(callback, module->getSandboxEnv(), std::forward<T>(args)...);
                            checkConnectOnce(module, event, identifier);
                        }
                    }
                }
            }
        }

        int luaEmit(const std::string& event, int32_t tableRef, const std::string& identifier = std::string()) {
            int ret = 0;
            std::vector<std::any> vecRet;
            lua_State* L = g_lua->getLuaState();

            if (identifier.empty()) {
                if (m_moduleEvents.find(event) != m_moduleEvents.end()) {
                    auto& modules = m_moduleEvents[event];

                    for (auto& module : modules) {
                        for (int32_t callback : module->getEventCallback(event)) {
                            vecRet.clear();

                            g_lua->callSandboxLuaFieldRef(callback, 1, vecRet, tableRef, module->getSandboxEnv());

                            if (vecRet.size() > 0) {
                                int luaValue = anyCast<int>(vecRet, 0, 0);
                                ret = std::min<int>(luaValue, ret);
                            }

                            checkConnectOnce(module, event, callback);
                        }
                    }
                }

            } else {
                auto& modules = m_identifiedModuleEvents[event];
                int32_t callback { 0 };

                for (auto& module : modules) {
                    auto& eventMap = module->m_identifiedEventCallbacks[event];

                    if (eventMap.find(identifier) != eventMap.end()) {
                        if (eventMap[identifier] != callback) {
                            callback = eventMap[identifier];
                            vecRet.clear();
                            g_lua->callSandboxLuaFieldRef(callback, 1, vecRet, tableRef, module->getSandboxEnv());

                            if (vecRet.size() > 0) {
                                int luaValue = anyCast<int>(vecRet, 0, 0);
                                ret = std::min<int>(luaValue, ret);
                            }

                            checkConnectOnce(module, event, identifier);
                        }
                    }
                }
            }

            luaL_unref(L, LUA_REGISTRYINDEX, tableRef);
            return ret;
        }

        template<typename... T>
        void emit(const std::string& event, int nresults, std::vector<std::any>& vecRet, const std::string& identifier = std::string(), T&&... args) {
            if (identifier.empty()) {
                if (m_moduleEvents.find(event) != m_moduleEvents.end()) {
                    auto& modules = m_moduleEvents[event];

                    for (auto& module : modules) {
                        for (int32_t callback : module->getEventCallback(event)) {
                            g_lua->callSandboxLuaField(callback, nresults, vecRet, module->getSandboxEnv(), std::forward<T>(args)...);
                            checkConnectOnce(module, event, callback);
                        }
                    }
                }

            } else {
                auto& modules = m_identifiedModuleEvents[event];
                int32_t callback { 0 };

                for (auto& module : modules) {
                    auto& eventMap = module->m_identifiedEventCallbacks[event];

                    if (eventMap.find(identifier) != eventMap.end()) {
                        if (eventMap[identifier] != callback) {
                            callback = eventMap[identifier];
                            g_lua->callSandboxLuaField(callback, nresults, vecRet, module->getSandboxEnv(), std::forward<T>(args)...);
                            checkConnectOnce(module, event, identifier);
                        }
                    }
                }
            }
        }

        void removeAllConnectionsById(const std::string& identifier);
        bool loadModules();

        void checkConnectOnce(Module* module, const std::string& event, int32_t callback);
        void checkConnectOnce(Module* module, const std::string& event, const std::string& identifier);

        bool isModuleLoaded(const std::string& name);
        Module* getModuleByName(const std::string& name);

    private:
        std::unordered_map<Module*, std::unordered_map<std::string, int32_t>> m_moduleExports;
        std::unordered_map<std::string, std::vector<Module*>> m_moduleEvents;
        std::unordered_map<std::string, std::vector<Module*>> m_identifiedModuleEvents;
        std::unordered_map<std::string, Module*> m_modules;

    friend class Module;
};

extern ModuleManagerPtr g_modules;

#endif
