/**
 * Copyright (c) 2025 PWO Team. All rights reserved.
 * This code is confidential and intended solely for internal use by authorized personnel.
 * Any unauthorized reproduction, distribution, or disclosure — including publication or sharing outside the company —
 * is strictly prohibited and may result in legal action.
 */

#include <filesystem>
#include <fmt/format.h>
#include <chrono>
#include <thread>
#include <fmt/format.h>

#include <core/modulemanager.h>
#include <core/logger.h>

#include <utils/tools.h>
#include <script/lua.h>

Module::Module(ModuleManager* manager, const std::string& name, const std::string& path)
    : m_manager(manager), m_name(name), m_path(path)
{
    m_sandboxEnv = g_lua->newSandboxEnv();
}

void Module::loadDependencies()
{
    lua_State* L = g_lua->getLuaState();

    lua_getglobal(L, "dependencies");

    if (!g_lua->isTable(L, -1)) {
        lua_pop(L, 1);
        return;
    }

    lua_pushnil(L);
    while (lua_next(L, -2) != 0) {
        std::string moduleName = g_lua->getString(L, -1);
        m_dependencies.push_back(moduleName);
        lua_pop(L, 1);
    }
    lua_pop(L, 1);
}

bool Module::loadFiles()
{
    lua_State* L = g_lua->getLuaState();

    std::vector<std::filesystem::path> moduleFiles;
    std::filesystem::path modulePath = std::filesystem::path(m_path);

    std::filesystem::path constPath = modulePath / "const.lua";
    if (std::filesystem::is_regular_file(constPath))
        moduleFiles.push_back(modulePath / "const.lua");

    lua_getglobal(L, "files");

    if (!g_lua->isTable(L, -1)) {
        g_logger.error(fmt::format("[Module::loadFiles] ({:s}) Not found 'files' table in settings.lua", getName()));
        lua_pop(L, 1);
        return false;
    }

    lua_pushnil(L);
    while(lua_next(L, -2) != 0) {
        std::string file = g_lua->getString(L, -1);
        moduleFiles.push_back(modulePath / file);
        lua_pop(L, 1);
    }

    lua_pop(L, 1);

    for (auto it = moduleFiles.begin(); it != moduleFiles.end(); ++it) {
        if (!std::filesystem::is_regular_file(*it)) {
            g_logger.error(fmt::format("[Module::loadFiles] ({:s}) Not found file {:s}", getName(), it->filename().string()));
            continue;
        }

        const std::string scriptFile = it->string();

        lua_pushnil(L);
        lua_setglobal(L, "init");

        if (g_lua->loadFile(scriptFile) == -1) {
            g_logger.error(fmt::format("[Module::loadFiles] ({:s}) Failed to load {:s}", getName(), it->filename().string()));
            g_logger.trace(g_lua->getLastLuaError());
            continue;
        }

        lua_getglobal(L, "init");
        if (lua_isfunction(L, -1)) {
            if (lua_pcall(L, 0, 0, 0) != 0)
                LuaScript::reportError(nullptr, LuaScript::popString(L));
        } else {
            lua_pop(L, 1);
        }
    }

    return true;
}

bool Module::load()
{
    int64_t lastTime = OTSYS_TIME();
    lua_State* L = g_lua->getLuaState();

    g_lua->setGlobalEnvironment(m_sandboxEnv);

    // push module class
    g_lua->pushUserdata<Module>(L, this);
    g_lua->setMetatable(L, -1, "Module");
    lua_setglobal(L, "module");

    // load module files
    const auto dir = m_path.c_str();
    if (!std::filesystem::exists(dir) || !std::filesystem::is_directory(dir)) {
        g_logger.error(fmt::format("[Module::load] ({:s}) Can not load folder '{:s}.'", getName(), m_path));
        return false;
    }

    std::filesystem::path modulePath = std::filesystem::path(m_path);
    std::filesystem::path settingsPath = modulePath / "settings.lua";

    if (!std::filesystem::is_regular_file(settingsPath)) {
        g_logger.error(fmt::format("[Module::load] ({:s}) Not found settings.lua", getName()));
        return false;
    }

    if (g_lua->loadFile(settingsPath.string()) == -1) {
        g_logger.error(fmt::format("[Module::load] ({:s}) Failed to load settings.lua", getName()));
        g_logger.trace(g_lua->getLastLuaError());
        return false;
    }

    if (hasDependencies()) {
        for (const std::string& moduleName : m_dependencies) {
            if (!m_manager->isModuleLoaded(moduleName)) {
                g_logger.error(fmt::format("[Module::load] ({:s}) The dependency {:s} is not loaded.", getName(), moduleName));
                return false;
            }
        }
    }

    if (!loadFiles())
        return false;

    g_logger.info(fmt::format("[Module] {:s} loaded ({:.2f}ms)", m_name, double(OTSYS_TIME() - lastTime)));

    g_lua->resetGlobalEnvironment();

    m_manager->emitNoRet("onLoadModule", getName());
    return true;
}

void Module::unload()
{
    g_lua->callSandboxLuaFieldNoRet("terminate", m_sandboxEnv);
    freeConnections();
}

bool Module::connect(const std::string& event, int32_t callback, const std::string& identifier)
{
    if (identifier.empty()){
        auto& callbacks = m_eventCallbacks[event];
        callbacks.push_back(callback);

        auto& modules = m_manager->m_moduleEvents[event];
        if (std::find(modules.begin(), modules.end(), this) == modules.end()) {
            modules.push_back(this);
        }

    } else {
        auto& eventMap = m_identifiedEventCallbacks[event];

        if (eventMap.find(identifier) != eventMap.end()) {
            g_logger.error(fmt::format("[Module::{:s}] Error when trying to connect already connected event with identifier {:s}.\n", m_name, identifier));
            return false;
        }

        eventMap.insert(std::make_pair(identifier, callback));

        auto& modules = m_manager->m_identifiedModuleEvents[event];
        if (std::find(modules.begin(), modules.end(), this) == modules.end()) {
            modules.push_back(this);
        }
    }

    return true;
}

bool Module::connectOnce(const std::string& event, int32_t callback, const std::string& identifier)
{
    bool success = connect(event, callback, identifier);
    if (success) {
        if (identifier.empty()) {
            m_onceConnects.push_back(callback);
        } else {
            StringVector& connects = m_identifiedOnceConnects[event];
            connects.push_back(identifier);
        }
    }
    return success;
}

void Module::disconnect(const std::string& event, int32_t callback, bool clearList)
{
    auto& moduleEvents = m_manager->m_moduleEvents;
    if (m_eventCallbacks.find(event) != m_eventCallbacks.end()) {
        lua_State* L = g_lua->getLuaState();

        auto& callbacks = m_eventCallbacks[event];

        callbacks.erase(std::remove_if(callbacks.begin(), callbacks.end(), [callback](int32_t callbackIt) {
            return callbackIt == callback;
        }), callbacks.end());


        if (callbacks.empty()) {
            auto& modules = moduleEvents[event];

            modules.erase(std::remove_if(modules.begin(), modules.end(), [this](Module* moduleIt) {
                return moduleIt == this;
            }), modules.end());

            luaL_unref(L, LUA_REGISTRYINDEX, callback);
            m_eventCallbacks.erase(event);
        }
    }

    if (clearList && moduleEvents.find(event) != moduleEvents.end()) {
        if (moduleEvents[event].size() == 0) {
            moduleEvents.erase(event);
        }
    }
}

void Module::disconnect(const std::string& event, const std::string& identifier, bool clearList)
{
    if (m_identifiedEventCallbacks.find(event) == m_identifiedEventCallbacks.end()) {
        return;
    }

    auto& identifiedModuleEvents = m_manager->m_identifiedModuleEvents;
    auto& eventMap = m_identifiedEventCallbacks[event];

    if (eventMap.find(identifier) != eventMap.end()) {
        lua_State* L = g_lua->getLuaState();

        int32_t callback = eventMap[identifier];
        auto& modules = identifiedModuleEvents[event];

        luaL_unref(L, LUA_REGISTRYINDEX, callback);
        eventMap.erase(identifier);

        if (eventMap.empty()) {
            modules.erase(std::remove_if(modules.begin(), modules.end(), [this](Module* moduleIt) {
                return moduleIt == this;
            }), modules.end());
        }
    }

    if (clearList && identifiedModuleEvents.find(event) != identifiedModuleEvents.end()) {
        if (identifiedModuleEvents[event].size() == 0) {
            identifiedModuleEvents.erase(event);
        }
    }
}

void Module::freeConnections()
{
    std::unordered_map<std::string, std::vector<int32_t>> events;

    for (const auto& [event, callbacks] : m_eventCallbacks) {
        auto& callbacksIds = events[event];

        for (int32_t callbackId : callbacks) {
            callbacksIds.push_back(callbackId);
        }
    }

    for (const auto& [event, callbacks] : events) {
        for (int32_t callback : callbacks) {
            disconnect(event, callback);
        }
    }

    events.clear();

    std::unordered_map<std::string, std::vector<std::string>> identifiedEvents;
    for (const auto&[event, eventMap] : m_identifiedEventCallbacks) {
        std::vector<std::string> identifiers;
        for (const auto&[identifier, callback] : eventMap) {
            identifiers.push_back(identifier);
        }
        identifiedEvents.insert(std::make_pair(event, identifiers));
    }

    for (const auto& [event, identifiers] : identifiedEvents) {
        for (const auto& identifier : identifiers) {
            disconnect(event, identifier);
        }
    }
    identifiedEvents.clear();

    auto& moduleExports = m_manager->m_moduleExports;
    if (moduleExports.find(this) != moduleExports.end()) {
        moduleExports[this].clear();
    }
}

bool Module::exportLua(const std::string& name, int32_t ref)
{
    auto& moduleExports = m_manager->m_moduleExports;

    if (moduleExports.find(this) == moduleExports.end()) {
        moduleExports.insert(std::make_pair(this, std::unordered_map<std::string, int32_t>()));
    }

    auto& exports = moduleExports[this];

    lua_State* L = g_lua->getLuaState();

    if (exports.find(name) == exports.end()) {
        lua_getglobal(L, "g_modules");

        if (!lua_istable(L, -1)) {
            lua_pop(L, 1);
            luaL_unref(L, LUA_REGISTRYINDEX, ref);

            LuaScript::reportErrorFunc(L, "Failed to export, the g_modules table don't exist.");
            return false;
        }

        lua_getfield(L, -1, m_name.c_str());

        if (!lua_istable(L, -1)) {
            lua_pop(L, 1);

            lua_newtable(L);

            g_lua->getRef(ref);
            lua_setfield(L, -2, name.c_str());

            lua_setfield(L, -2, m_name.c_str());
            lua_pop(L, 2);

            exports.insert(std::make_pair(name, ref));
            return true;
        }

        g_lua->getRef(ref);
        lua_setfield(L, -2, name.c_str());

        lua_pop(L, 2);

        exports.insert(std::make_pair(name, ref));
        return true;

    } else {
        luaL_unref(L, LUA_REGISTRYINDEX, ref);
        g_lua->reportErrorFunc(L, fmt::format("Error: The name {:s} is already reserved.", name));
    }

    return false;
}

const std::vector<int32_t>& Module::getEventCallback(const std::string& event)
{
    if (m_eventCallbacks.find(event) != m_eventCallbacks.end())
        return m_eventCallbacks[event];

    static std::vector<int32_t> emptyVector;
    return emptyVector;
}
