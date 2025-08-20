/**
 * Copyright (c) 2025 PWO Team. All rights reserved.
 * This code is confidential and intended solely for internal use by authorized personnel.
 * Any unauthorized reproduction, distribution, or disclosure — including publication or sharing outside the company —
 * is strictly prohibited and may result in legal action.
 */

#include <script/lua.h>

#include <core/module.h>
#include <core/modulemanager.h>
#include <core/logger.h>

#include <redis/pub.h>
#include <redis/sub.h>

#include <network/connectionmanager.h>

LuaScriptPtr g_lua = std::make_shared<LuaScript>();
LuaTablePtr g_config = nullptr;

LuaScript::LuaScript()
{
    m_luaState = luaL_newstate();
    luaL_openlibs(m_luaState);
}

LuaScript::~LuaScript()
{
    lua_close(m_luaState);
}

bool LuaScript::init()
{
	if (loadFile("config.lua") == -1) {
		g_logger.info("Failed to load config.lua");
		return false;
	}

	g_config = LuaTable::New(m_luaState);
	putGlobalOnStack(m_luaState, "_G");

	lua_pushnil(m_luaState);
	while (lua_next(m_luaState, -2) != 0) {
		lua_pushvalue(m_luaState, -2);
		lua_pushvalue(m_luaState, -2);
		lua_settable(m_luaState, -6);
		pop(m_luaState);
	}

	if (loadFile("lib/lib.lua") == -1) {
		g_logger.fatal("Failed to load lib/lib.lua");
		return false;
	}

	// Global Functions
	registerGlobalFunction("emit", LuaScript::luaEmit);

	// g_login
	registerTable("g_login");
	registerTableFunction("g_login", "getClient", LuaScript::luaLoginGetClient);

	// g_redis
	registerTable("g_redis");
	registerTableFunction("g_redis", "publish", LuaScript::luaRedisPublish);
	registerTableFunction("g_redis", "subscribe", LuaScript::luaRedisSubscribe);

	// Module
	registerClass("Module");
	registerStaticMethod("Module", "connect", LuaScript::luaModuleConnect);

	// Protocol
	registerClass("Protocol");
	registerStaticMethod("Protocol", "getId", LuaScript::luaProtocolGetId);
	registerStaticMethod("Protocol", "send", LuaScript::luaProtocolSend);
	registerStaticMethod("Protocol", "sendError", LuaScript::luaProtocolSendError);
	registerStaticMethod("Protocol", "sendLoadingMessage", LuaScript::luaProtocolSendLoadingMessage);

	// NetworkMessage
	registerClass("NetworkMessage");
	registerStaticMethod("NetworkMessage", "getU8", LuaScript::luaNetworkMessageGetU8);
	registerStaticMethod("NetworkMessage", "getU16", LuaScript::luaNetworkMessageGetU16);
	registerStaticMethod("NetworkMessage", "getU32", LuaScript::luaNetworkMessageGetU32);
	registerStaticMethod("NetworkMessage", "getU64", LuaScript::luaNetworkMessageGetU64);
	registerStaticMethod("NetworkMessage", "getString", LuaScript::luaNetworkMessageGetString);

	registerStaticMethod("NetworkMessage", "addU8", LuaScript::luaNetworkMessageAddU8);
	registerStaticMethod("NetworkMessage", "addU16", LuaScript::luaNetworkMessageAddU16);
	registerStaticMethod("NetworkMessage", "addU32", LuaScript::luaNetworkMessageAddU32);
	registerStaticMethod("NetworkMessage", "addU64", LuaScript::luaNetworkMessageAddU64);
	registerStaticMethod("NetworkMessage", "addString", LuaScript::luaNetworkMessageAddString);

	// OutputMessage
	registerClass("OutputMessage", "NetworkMessage", LuaScript::luaOutputMessageCreate);
	registerStaticMetaMethod("OutputMessage", "__gc", LuaScript::luaOutputMessageDelete);
	registerStaticMethod("OutputMessage", "delete", LuaScript::luaOutputMessageDelete);

	return true;
}

int32_t LuaScript::loadFile(const std::string& file)
{
	//loads file as a chunk at stack top
	int ret = luaL_loadfile(m_luaState, file.c_str());
	if (ret != 0) {
		m_lastLuaError = popString(m_luaState);
		return -1;
	}

	//check that it is loaded as a function
	if (!isFunction(m_luaState, -1)) {
		lua_pop(m_luaState, 1);
		return -1;
	}

	m_loadingFile = file;

	//execute it
	ret = protectedCall(m_luaState, 0, 0);
	if (ret != 0) {
		reportError(nullptr, popString(m_luaState));
		return -1;
	}

	return 0;
}

int LuaScript::protectedCall(lua_State* L, int nargs, int nresults)
{
	int error_index = lua_gettop(L) - nargs;
	lua_pushcfunction(L, luaErrorHandler);
	lua_insert(L, error_index);

	int ret = lua_pcall(L, nargs, nresults, error_index);
	lua_remove(L, error_index);
	return ret;
}

int LuaScript::luaErrorHandler(lua_State* L)
{
	const std::string& errorMessage = popString(L);
	pushString(L, LuaScript::getStackTrace(L, errorMessage));
	return 1;
}

void LuaScript::reportError(const char* function, const std::string& error_desc, lua_State* L /*= nullptr*/, bool stack_trace /*= false*/)
{
	std::string error = "Lua Script Error: ";
	if (function) {
		error = error + function + "(). ";
	}

	if (L && stack_trace) {
		error = error + getStackTrace(L, error_desc);
	} else {
		error = error + error_desc;
	}

	g_logger.error(error);
}

std::string LuaScript::getStackTrace(lua_State* L, const std::string& error_desc)
{
	lua_getglobal(L, "debug");
	if (!isTable(L, -1)) {
		lua_pop(L, 1);
		return error_desc;
	}

	lua_getfield(L, -1, "traceback");
	if (!isFunction(L, -1)) {
		lua_pop(L, 2);
		return error_desc;
	}

	lua_replace(L, -2);
	pushString(L, error_desc);
	lua_call(L, 1, 1);
	return popString(L);
}

int LuaScript::ref(lua_State* L)
{
    int ref = luaL_ref(L, LUA_REGISTRYINDEX);
    assert(ref != LUA_NOREF);
    assert(ref < 2147483647);
    return ref;
}

void LuaScript::pushThread()
{
	lua_pushthread(m_luaState);
    checkStack();
}

void LuaScript::pushFunction(lua_State* L, lua_CFunction function)
{
	lua_pushcfunction(L, function);
}

void LuaScript::setGlobal(lua_State* L, const std::string& name)
{
	lua_setglobal(L, name.c_str());
}

void LuaScript::putGlobalOnStack(lua_State* L, const std::string& name)
{
	lua_getglobal(L, name.c_str());
}

bool LuaScript::isGlobalFunction(lua_State* L, const std::string functionName)
{
	LuaScript::putGlobalOnStack(L, functionName);
	if (LuaScript::isFunction(L, -1)) {
		LuaScript::pop(L);
		return true;
	}
	LuaScript::pop(L);
	return false;
}

void LuaScript::registerTable(const std::string& tableName)
{
	putGlobalOnStack(m_luaState, tableName);
	if (LuaScript::isTable(m_luaState, -1)) {
		LuaScript::pop(m_luaState);
		return;
	}
	lua_newtable(m_luaState);
	setGlobal(m_luaState, tableName);
}

void LuaScript::registerTableFunction(const std::string& tableName, const std::string& functionName, lua_CFunction function)
{
	int luaTop = lua_gettop(m_luaState);

	putGlobalOnStack(m_luaState, tableName);
	assert(LuaScript::isTable(m_luaState, -1));

	pushString(m_luaState, functionName);
	lua_pushcfunction(m_luaState, function);
	lua_settable(m_luaState, -3);
	pop(m_luaState);

	assert(luaTop == lua_gettop(m_luaState));
}

void LuaScript::registerClass(const std::string& className, const std::string& baseClass, lua_CFunction constructor)
{
	// className = {}
	lua_newtable(m_luaState);
	lua_pushvalue(m_luaState, -1);
	lua_setglobal(m_luaState, className.c_str());
	int methods = lua_gettop(m_luaState);

	// methodsTable = {}
	lua_newtable(m_luaState);
	int methodsTable = lua_gettop(m_luaState);

	if (constructor) {
		// className.__call = constructor
		lua_pushcfunction(m_luaState, constructor);
		lua_setfield(m_luaState, methodsTable, "__call");
	}

	uint32_t parents = 0;
	if (!baseClass.empty()) {
		lua_getglobal(m_luaState, baseClass.c_str());
		lua_rawgeti(m_luaState, -1, 'p');
		parents = getNumber<uint32_t>(m_luaState, -1) + 1;
		lua_pop(m_luaState, 1);
		lua_setfield(m_luaState, methodsTable, "__index");
	}

	// setmetatable(className, methodsTable)
	lua_setmetatable(m_luaState, methods);

	// className.metatable = {}
	luaL_newmetatable(m_luaState, className.c_str());
	int metatable = lua_gettop(m_luaState);

	// className.metatable.__metatable = className
	lua_pushvalue(m_luaState, methods);
	lua_setfield(m_luaState, metatable, "__metatable");

	// className.metatable.__index = className
	lua_pushvalue(m_luaState, methods);
	lua_setfield(m_luaState, metatable, "__index");

	// className.metatable['h'] = hash
	lua_pushnumber(m_luaState, std::hash<std::string>()(className));
	lua_rawseti(m_luaState, metatable, 'h');

	// className.metatable['p'] = parents
	lua_pushnumber(m_luaState, parents);
	lua_rawseti(m_luaState, metatable, 'p');

	// pop className, className.metatable
	lua_pop(m_luaState, 2);
}


int32_t LuaScript::luaEmit(lua_State* L)
{
    // emit(event, tableRef, identifier)
    int stackTop = getTop(L);
    std::string identifier;

    if (stackTop > 2)
        identifier = LuaStack::Pop<std::string>::Value(L);

    int32_t tableRef = ref(L);
    std::string event = LuaStack::Pop<std::string>::Value(L);

    int ret = g_modules->luaEmit(event, tableRef, identifier);
    LuaStack::Push<int>::Value(L, ret);

    return getTop(L);
}

int32_t LuaScript::luaLoginGetClient(lua_State* L)
{
	// g_login.getClient(id)
	uint64_t id = LuaStack::Pop<uint64_t>::Value(L);
	ProtocolSharedPtr protocol = g_connectionManager.getProtocolById(id);
	if (protocol)
		LuaStack::Push<ProtocolSharedPtr>::Value(L, protocol);
	else
		lua_pushnil(L);
	return getTop(L);
}

int32_t LuaScript::luaRedisPublish(lua_State* L)
{
	// g_redis.publish(channel, data)
	std::string data = LuaStack::Pop<std::string>::Value(L);
    std::string channel = LuaStack::Pop<std::string>::Value(L);
	LuaStack::Push<bool>::Value(L, g_redisPublisher->publish(channel, data));
	return getTop(L);
}

int32_t LuaScript::luaRedisSubscribe(lua_State* L)
{
	// g_redis.subscribe(channel)
    std::string channel = LuaStack::Pop<std::string>::Value(L);
	LuaStack::Push<bool>::Value(L, g_redisSubscriber->subscribe(channel));
	return getTop(L);
}

int32_t LuaScript::luaModuleConnect(lua_State* L)
{
	// Module:connect(event, callback, [identifier])
	std::string identifier;
	if (getTop(L) > 3) {
		if (lua_type(L, 4) == LUA_TSTRING) {
			identifier = LuaStack::Pop<std::string>::Value(L);
		} else if (lua_type(L, 4) == LUA_TNUMBER) {
			identifier = std::to_string(LuaStack::Pop<int>::Value(L));
		}
	}

	int32_t callback = ref(L);
	std::string event = LuaStack::Pop<std::string>::Value(L);

	Module* module = LuaStack::Pop<Module>::Value(L);
	if (!module) {
		LuaStack::Push<bool>::Value(L, false);
		return getTop(L);
	}

	if (!module->connect(event, callback, identifier)) {
		LuaStack::Push<bool>::Value(L, false);
		return getTop(L);
	}

	auto disconnect = [](lua_State* L) -> int {
		Module* _module = static_cast<Module*>(lua_touserdata(L, lua_upvalueindex(1)));
		std::shared_ptr<std::string>* _event = static_cast<std::shared_ptr<std::string>*>(lua_touserdata(L, lua_upvalueindex(2)));
		std::shared_ptr<std::string>* _identifier = static_cast<std::shared_ptr<std::string>*>(lua_touserdata(L, lua_upvalueindex(3)));

		_module->disconnect(*_event->get(), *_identifier->get());
		LuaStack::Push<bool>::Value(L, true);
		return 1;
	};

	auto eventPtr = std::make_shared<std::string>(event);
	std::shared_ptr<std::string>* eventUserdata = static_cast<std::shared_ptr<std::string>*>(lua_newuserdata(L, sizeof(std::shared_ptr<std::string>)));
	new (eventUserdata) std::shared_ptr<std::string>(eventPtr);

	auto identifierPtr = std::make_shared<std::string>(identifier);
	std::shared_ptr<std::string>* identifierUserdata = static_cast<std::shared_ptr<std::string>*>(lua_newuserdata(L, sizeof(std::shared_ptr<std::string>)));
	new (identifierUserdata) std::shared_ptr<std::string>(identifierPtr);

	lua_pushlightuserdata(L, module);
	lua_pushlightuserdata(L, eventUserdata);
	lua_pushlightuserdata(L, identifierUserdata);

	lua_pushcclosure(L, disconnect, 3);
	return getTop(L);
}

int32_t LuaScript::luaProtocolGetId(lua_State* L)
{
	// Protocol:getId()
	Protocol* protocol = LuaStack::Pop<Protocol>::Value(L);
	if (!protocol) {
		lua_pushnil(L);
		return LuaScript::getTop(L);
	}

	LuaStack::Push<uint32_t>::Value(L, protocol->getId());
	return LuaScript::getTop(L);
}

int32_t LuaScript::luaProtocolSend(lua_State* L)
{
	// Protocol:send(msg)
	OutputMessage* msg = LuaStack::Pop<OutputMessage>::Value(L);
	Protocol* protocol = LuaStack::Pop<Protocol>::Value(L);
	if (!protocol) {
		LuaStack::Push<bool>::Value(L, false);
		return LuaScript::getTop(L);
	}

	protocol->send(*msg);
	LuaStack::Push<bool>::Value(L, true);
	return LuaScript::getTop(L);
}

int32_t LuaScript::luaProtocolSendError(lua_State* L)
{
	// Protocol:sendError(error)
	std::string error = LuaStack::Pop<std::string>::Value(L);
	Protocol* protocol = LuaStack::Pop<Protocol>::Value(L);
	if (!protocol) {
		LuaStack::Push<bool>::Value(L, false);
		return LuaScript::getTop(L);
	}

	protocol->sendError(error);
	LuaStack::Push<bool>::Value(L, true);
	return LuaScript::getTop(L);
}

int32_t LuaScript::luaProtocolSendLoadingMessage(lua_State* L)
{
	// Protocol:sendLoadingMessage(message)
	std::string message = LuaStack::Pop<std::string>::Value(L);
	Protocol* protocol = LuaStack::Pop<Protocol>::Value(L);
	if (!protocol) {
		LuaStack::Push<bool>::Value(L, false);
		return LuaScript::getTop(L);
	}

	protocol->sendLoadingMessage(message);
	LuaStack::Push<bool>::Value(L, true);
	return LuaScript::getTop(L);
}

int32_t LuaScript::luaNetworkMessageGetU8(lua_State* L)
{
	// NetworkMessage:getU8()
	NetworkMessage* msg = LuaStack::Pop<NetworkMessage>::Value(L);
	if (!msg) {
		lua_pushnil(L);
		return LuaScript::getTop(L);
	}

	LuaStack::Push<uint8_t>::Value(L, msg->getByte());
	return LuaScript::getTop(L);
}

int32_t LuaScript::luaNetworkMessageGetU16(lua_State* L)
{
	// NetworkMessage:getU16()
	NetworkMessage* msg = LuaStack::Pop<NetworkMessage>::Value(L);
	if (!msg) {
		lua_pushnil(L);
		return LuaScript::getTop(L);
	}

	LuaStack::Push<uint16_t>::Value(L, msg->get<uint16_t>());
	return LuaScript::getTop(L);
}

int32_t LuaScript::luaNetworkMessageGetU32(lua_State* L)
{
	// NetworkMessage:getU32()
	NetworkMessage* msg = LuaStack::Pop<NetworkMessage>::Value(L);
	if (!msg) {
		lua_pushnil(L);
		return LuaScript::getTop(L);
	}

	LuaStack::Push<uint32_t>::Value(L, msg->get<uint32_t>());
	return LuaScript::getTop(L);
}

int32_t LuaScript::luaNetworkMessageGetU64(lua_State* L)
{
	// NetworkMessage:getU64()
	NetworkMessage* msg = LuaStack::Pop<NetworkMessage>::Value(L);
	if (!msg) {
		lua_pushnil(L);
		return LuaScript::getTop(L);
	}

	LuaStack::Push<uint64_t>::Value(L, msg->get<uint64_t>());
	return LuaScript::getTop(L);
}

int32_t LuaScript::luaNetworkMessageGetString(lua_State* L)
{
	// NetworkMessage:getString()
	NetworkMessage* msg = LuaStack::Pop<NetworkMessage>::Value(L);
	if (!msg) {
		lua_pushnil(L);
		return LuaScript::getTop(L);
	}

	LuaStack::Push<std::string>::Value(L, msg->getString());
	return LuaScript::getTop(L);
}

int32_t LuaScript::luaNetworkMessageAddU8(lua_State* L)
{
	// NetworkMessage:addU8(value)
	uint8_t value = LuaStack::Pop<uint8_t>::Value(L);
	NetworkMessage* msg = LuaStack::Pop<NetworkMessage>::Value(L);
	if (!msg) {
		LuaStack::Push<bool>::Value(L, false);
		return LuaScript::getTop(L);
	}

	msg->addByte(value);
	LuaStack::Push<bool>::Value(L, true);
	return LuaScript::getTop(L);
}

int32_t LuaScript::luaNetworkMessageAddU16(lua_State* L)
{
	// NetworkMessage:addU16(value)
	uint16_t value = LuaStack::Pop<uint16_t>::Value(L);
	NetworkMessage* msg = LuaStack::Pop<NetworkMessage>::Value(L);
	if (!msg) {
		LuaStack::Push<bool>::Value(L, false);
		return LuaScript::getTop(L);
	}

	msg->add<uint16_t>(value);
	LuaStack::Push<bool>::Value(L, true);
	return LuaScript::getTop(L);
}

int32_t LuaScript::luaNetworkMessageAddU32(lua_State* L)
{
	// NetworkMessage:addU32(value)
	uint32_t value = LuaStack::Pop<uint32_t>::Value(L);
	NetworkMessage* msg = LuaStack::Pop<NetworkMessage>::Value(L);
	if (!msg) {
		LuaStack::Push<bool>::Value(L, false);
		return LuaScript::getTop(L);
	}

	msg->add<uint32_t>(value);
	LuaStack::Push<bool>::Value(L, true);
	return LuaScript::getTop(L);
}

int32_t LuaScript::luaNetworkMessageAddU64(lua_State* L)
{
	// NetworkMessage:addU64(value)
	uint64_t value = LuaStack::Pop<uint64_t>::Value(L);
	NetworkMessage* msg = LuaStack::Pop<NetworkMessage>::Value(L);
	if (!msg) {
		LuaStack::Push<bool>::Value(L, false);
		return LuaScript::getTop(L);
	}

	msg->add<uint64_t>(value);
	LuaStack::Push<bool>::Value(L, true);
	return LuaScript::getTop(L);
}

int32_t LuaScript::luaNetworkMessageAddString(lua_State* L)
{
	// NetworkMessage:addString(value)
	std::string value = LuaStack::Pop<std::string>::Value(L);
	NetworkMessage* msg = LuaStack::Pop<NetworkMessage>::Value(L);
	if (!msg) {
		LuaStack::Push<bool>::Value(L, false);
		return LuaScript::getTop(L);
	}

	msg->addString(value);
	LuaStack::Push<bool>::Value(L, true);
	return LuaScript::getTop(L);
}

int32_t LuaScript::luaOutputMessageCreate(lua_State* L)
{
	// OutputMessage()
	LuaStack::Push<OutputMessage>::Value(L, new OutputMessage);
	return LuaScript::getTop(L) - 1;
}

int32_t LuaScript::luaOutputMessageDelete(lua_State* L)
{
	OutputMessage** outputPtr = getRawUserdata<OutputMessage>(L, 1);
	if (outputPtr && *outputPtr) {
		delete *outputPtr;
		*outputPtr = nullptr;
	}
	return LuaScript::getTop(L);
}

int LuaScript::newSandboxEnv()
{
	lua_newtable(m_luaState); // pushes the new environment table
	lua_newtable(m_luaState); // pushes the new environment metatable

	getRef(getGlobalEnvironment()); // pushes the global environment
	setField("__index"); // sets metatable __index to the global environment
	setMetatable(); // assigns environment metatable
	return ref(m_luaState); // return a reference to the environment table
}

int LuaScript::getGlobalEnvironment()
{
	if (m_globalEnv != -1) {
		return m_globalEnv;
	}

	lua_getglobal(m_luaState, "_G");
	m_globalEnv = luaL_ref(m_luaState, LUA_REGISTRYINDEX);

	return m_globalEnv;
}

void LuaScript::setGlobalEnvironment(int env)
{
	pushThread();
	getRef(env);
	assert(isTable(m_luaState, -1));
	setEnv();
	pop(m_luaState);
}

void LuaScript::resetGlobalEnvironment()
{
	int globalEnvIndex = getGlobalEnvironment();
	setGlobalEnvironment(globalEnvIndex);
}

void LuaScript::getEnv(int index)
{
    assert(hasIndex(index));
    lua_getfenv(m_luaState, index);
}

void LuaScript::setEnv(int index)
{
    assert(hasIndex(index));
    lua_setfenv(m_luaState, index);
}

void LuaScript::pop(lua_State* L, int n)
{
	if(n > 0) {
        lua_pop(L, n);
    }
}

void LuaScript::setMetatable(int index)
{
    assert(hasIndex(index));
    lua_setmetatable(m_luaState, index);
}

void LuaScript::setMetatable(lua_State* L, int32_t index, const std::string& name)
{
	luaL_getmetatable(L, name.c_str());
	lua_setmetatable(L, index - 1);
}

void LuaScript::setField(const char* key, int index)
{
    assert(hasIndex(index));
    assert(isUserdata(m_luaState, index) || isTable(m_luaState, index));
    lua_setfield(m_luaState, index, key);
}

void LuaScript::putFunctionOnStack(int32_t functionId)
{
	lua_rawgeti(m_luaState, LUA_REGISTRYINDEX, functionId);
}

void LuaScript::putFunctionOnStack(const std::string& function)
{
	lua_getglobal(m_luaState, function.c_str());
}

void LuaScript::putFunctionOnStack(const char* function)
{
	lua_getglobal(m_luaState, function);
}

static int LUA_STACK_TRACK_START = 0;
void LuaScript::startTrackStack(lua_State* L)
{
	LUA_STACK_TRACK_START = lua_gettop(L);
}

void LuaScript::endTrackStack(lua_State* L)
{
	int luaTop = lua_gettop(L);
	assert(LUA_STACK_TRACK_START == luaTop);
}

void LuaScript::registerGlobalFunction(const std::string& name, lua_CFunction function)
{
	LuaScript::pushFunction(m_luaState, function);
	LuaScript::setGlobal(m_luaState, name);
}

void LuaScript::registerStaticMethod(const std::string& className, const std::string& methodName, lua_CFunction method)
{
	int top = lua_gettop(m_luaState);

	putGlobalOnStack(m_luaState, className);
	if (!isTable(m_luaState, -1)) {
		pop(m_luaState);
		assert(top == lua_gettop(m_luaState));
		return;
	}

	lua_pushcfunction(m_luaState, method);
	lua_setfield(m_luaState, -2, methodName.c_str());

	pop(m_luaState);
	assert(top == lua_gettop(m_luaState));
}

void LuaScript::registerStaticMetaMethod(const std::string& className, const std::string& methodName, lua_CFunction func)
{
	// className.metatable.methodName = func
	luaL_getmetatable(m_luaState, className.c_str());
	lua_pushcfunction(m_luaState, func);
	lua_setfield(m_luaState, -2, methodName.c_str());

	// pop className.metatable
	lua_pop(m_luaState, 1);
}

void LuaScript::pushBoolean(lua_State* L, bool value)
{
	lua_pushboolean(L, value ? 1 : 0);
}

void LuaScript::pushNil(lua_State* L)
{
	lua_pushnil(L);
}

void LuaScript::pushString(lua_State* L, const std::string& value)
{
	lua_pushlstring(L, value.c_str(), value.length());
}

std::string LuaScript::getString(lua_State* L, int32_t arg)
{
	size_t len;
	const char* c_str = lua_tolstring(L, arg, &len);
	if (!c_str || len == 0) {
		return std::string();
	}
	return std::string(c_str, len);
}

std::string LuaScript::popString(lua_State* L)
{
	if (lua_gettop(L) == 0) {
		return std::string();
	}

	std::string str(getString(L, -1));
	lua_pop(L, 1);
	return str;
}
