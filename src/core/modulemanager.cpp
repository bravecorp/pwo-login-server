/**
 * Copyright (c) 2025 PWO Team. All rights reserved.
 * This code is confidential and intended solely for internal use by authorized personnel.
 * Any unauthorized reproduction, distribution, or disclosure — including publication or sharing outside the company —
 * is strictly prohibited and may result in legal action.
 */

#include <filesystem>
#include <fmt/format.h>

#include <core/modulemanager.h>
#include <core/logger.h>

#include <utils/tools.h>

ModuleManagerPtr g_modules = std::make_shared<ModuleManager>();

ModuleManager::~ModuleManager()
{
    for (auto module : m_modules) {
        delete module.second;
    }
}

bool ModuleManager::loadModules()
{
    const auto modulesPath = std::filesystem::current_path() / "modules";

    if (!std::filesystem::exists(modulesPath) || !std::filesystem::is_directory(modulesPath)) {
        g_logger.fatal("Can not load folder 'modules'");
        return false;
    }

    const auto modulesSettingsPath = modulesPath / "modules.lua";
    if (!std::filesystem::is_regular_file(modulesSettingsPath)) {
        g_logger.fatal("Not found modules/modules.lua");
        return false;
    }

    if (g_lua->loadFile(modulesSettingsPath.string()) == -1) {
        g_logger.error("Failed to load modules.lua");
        g_logger.trace(g_lua->getLastLuaError());
        return false;
    }

    lua_State* L = g_lua->getLuaState();

    lua_getglobal(L, "modules");

    if (!g_lua->isTable(L, -1)) {
        g_logger.fatal("Not found 'modules' table in modules/modules.lua");
        lua_pop(L, 1);
        return false;
    }

    std::vector<std::filesystem::path> paths;

    lua_pushnil(L);
    while(lua_next(L, -2) != 0) {
        std::string moduleName = g_lua->getString(L, -1);
        paths.push_back(modulesPath / moduleName);
        lua_pop(L, 1);
    }

    lua_pop(L, 1);

    for (auto it = paths.begin(); it != paths.end(); ++it) {
        if (!std::filesystem::is_directory(*it)) {
            g_logger.error(fmt::format("Not found module directory: {:s}", it->string()));
            continue;
        }

        std::string path = it->generic_string();

        size_t lastBar = path.find_last_of("/\\");
        std::string moduleName = path.substr(lastBar + 1);

        Module* newModule = new Module(this, moduleName, path);
        if (newModule->load())
            m_modules.insert(std::make_pair(moduleName, newModule));
        else
            delete newModule;
    }

    return true;
}

bool ModuleManager::isModuleLoaded(const std::string& name)
{
    return m_modules.find(name) != m_modules.end();
}

Module* ModuleManager::getModuleByName(const std::string& name)
{
    if (m_modules.find(name) != m_modules.end()) {
        return m_modules[name];
    }
    return nullptr;
}

void ModuleManager::removeAllConnectionsById(const std::string& identifier)
{
    for (auto it = m_identifiedModuleEvents.begin(); it != m_identifiedModuleEvents.end();) {
        auto& modules = it->second;

        for (auto& module : modules) {
            module->disconnect(it->first, identifier, false);
        }

        if (modules.size() == 0) {
            it = m_identifiedModuleEvents.erase(it);
        } else {
            ++it;
        }
    }
}

void ModuleManager::checkConnectOnce(Module* module, const std::string& event, int32_t callback)
{
    auto& onceConnects = module->m_onceConnects;
    auto it = std::find(onceConnects.begin(), onceConnects.end(), callback);

    if (it != onceConnects.end()) {
        onceConnects.erase(it);
        module->disconnect(event, callback);
    }
}

void ModuleManager::checkConnectOnce(Module* module, const std::string& event, const std::string& identifier)
{
    if (module->m_identifiedOnceConnects.find(event) != module->m_identifiedOnceConnects.end()) {
        StringVector& identifiers = module->m_identifiedOnceConnects[event];
        StringVector::iterator it = std::find(identifiers.begin(), identifiers.end(), identifier);

        if (it != identifiers.end()) {
            identifiers.erase(it);
            module->disconnect(event, identifier);
        }
    }
}
