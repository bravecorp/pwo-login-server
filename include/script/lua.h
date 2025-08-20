/**
 * Copyright (c) 2025 PWO Team. All rights reserved.
 * This code is confidential and intended solely for internal use by authorized personnel.
 * Any unauthorized reproduction, distribution, or disclosure — including publication or sharing outside the company —
 * is strictly prohibited and may result in legal action.
 */

#ifndef SCRIPT_LUA_H
#define SCRIPT_LUA_H

#include <fmt/format.h>
#include <functional>
#include <cassert>

#if __has_include("luajit/lua.hpp")
#include <luajit/lua.hpp>
#else
#include <luajit-2.1/lua.hpp>
#endif

#include <core/module.h>
#include <core/logger.h>

#include <network/protocol.h>
#include <network/networkmessage.h>
#include <network/outputmessage.h>

#include <utils/types.h>

class Module;
class Protocol;
class NetworkMessage;
class OutputMessage;

#define reportErrorFunc(L, a)  LuaScript::reportError(__FUNCTION__, a, L, true)

namespace LuaStack
{
	template<typename T>
	struct Push {};

	template<typename T>
	struct Pop {};
};

class LuaTable
{
	public:
		LuaTable(lua_State* L, int index = -1) : L(L)
		{
			if (!lua_istable(L, index))
				return;

			lua_pushvalue(L, index);
			ref = luaL_ref(L, LUA_REGISTRYINDEX);
		};

		~LuaTable()
		{
			if (L && ref != LUA_NOREF) {
				luaL_unref(L, LUA_REGISTRYINDEX, ref);
			}
		}

		static std::shared_ptr<LuaTable> New(lua_State* L){
			lua_newtable(L);
			return std::make_shared<LuaTable>(L);
		}

		template<typename T>
		T get(const std::string& key, T defaultValue = T())
		{
			lua_rawgeti(L, LUA_REGISTRYINDEX, ref);
			if (!lua_istable(L, -1)) {
				lua_pop(L, 1);
				return defaultValue;
			}

			lua_getfield(L, -1, key.c_str());
			if (lua_isnil(L, -1)) {
				lua_pop(L, 2);
				return defaultValue;
			}

			T value = LuaStack::Pop<T>::Value(L, -1);
			lua_pop(L, 1);
			return value;
		}

		template<typename K, typename V>
		void insert(K key, V value)
		{
			lua_rawgeti(L, LUA_REGISTRYINDEX, ref);
			if (!lua_istable(L, -1)) {
				lua_pop(L, 1);
				return;
			}

			LuaStack::Push<K>::Value(L, key);
			LuaStack::Push<V>::Value(L, value);
			lua_settable(L, -3);
			lua_pop(L, 1);
		}

		template<typename T>
		void forEach(const std::function<void(T)>& func)
		{
			lua_rawgeti(L, LUA_REGISTRYINDEX, ref);
			if (!lua_istable(L, -1)) {
				lua_pop(L, 1);
				return;
			}

			lua_pushnil(L);
			while (lua_next(L, -2) != 0) {
				T value = LuaStack::Pop<T>::Value(L, -1);
				func(value);
			}
			lua_pop(L, 1);
		}

		int size() const
		{
			int size = 0;

			lua_rawgeti(L, LUA_REGISTRYINDEX, ref);
			if (!lua_istable(L, -1)) {
				lua_pop(L, 1);
				return size;
			}

			lua_pushnil(L);
			while (lua_next(L, -2) != 0) {
				size++;
				lua_pop(L, 1);
			}
			lua_pop(L, 1);
			return size;
		}

		void putInStack() { lua_rawgeti(L, LUA_REGISTRYINDEX, ref); }

	private:
		lua_State* L = nullptr;
		int ref = LUA_NOREF;
};

class LuaScript
{
    public:
        LuaScript();
        ~LuaScript();

		bool init();

        int32_t loadFile(const std::string& file);

        static int protectedCall(lua_State* L, int nargs, int nresults);
        static int luaErrorHandler(lua_State* L);

        static void reportError(const char* function, const std::string& error_desc, lua_State* L = nullptr, bool stack_trace = false);

        void getRef(int ref) { lua_rawgeti(m_luaState, LUA_REGISTRYINDEX, ref); }
		void unref(int ref) { luaL_unref(m_luaState, LUA_REGISTRYINDEX, ref); }

        static int ref(lua_State* L);
        void pushThread();

		static void pushFunction(lua_State* L, lua_CFunction function);

		static void setGlobal(lua_State* L, const std::string& name);
		static void putGlobalOnStack(lua_State* L, const std::string& name);
		static bool isGlobalFunction(lua_State* L, const std::string functionName);

		void registerTable(const std::string& tableName);
		void registerTableFunction(const std::string& tableName, const std::string& functionName, lua_CFunction function);

		void registerClass(const std::string& className, const std::string& baseClass, lua_CFunction constructor = NULL);
		void registerClass(const std::string& className, lua_CFunction constructor = NULL) {
			registerClass(className, "", constructor);
		}

		// Binds

		// Global functions
		static int32_t luaEmit(lua_State* L);

		// g_login
		static int32_t luaLoginGetClient(lua_State* L);

		// g_redis
		static int32_t luaRedisPublish(lua_State* L);
		static int32_t luaRedisSubscribe(lua_State* L);

		// Module
		static int32_t luaModuleConnect(lua_State* L);

		// Protocol
		static int32_t luaProtocolGetId(lua_State* L);
		static int32_t luaProtocolSend(lua_State* L);
		static int32_t luaProtocolSendError(lua_State* L);
		static int32_t luaProtocolSendLoadingMessage(lua_State* L);

		// NetworkMessage
		static int32_t luaNetworkMessageGetU8(lua_State* L);
		static int32_t luaNetworkMessageGetU16(lua_State* L);
		static int32_t luaNetworkMessageGetU32(lua_State* L);
		static int32_t luaNetworkMessageGetU64(lua_State* L);
		static int32_t luaNetworkMessageGetString(lua_State* L);

		static int32_t luaNetworkMessageAddU8(lua_State* L);
		static int32_t luaNetworkMessageAddU16(lua_State* L);
		static int32_t luaNetworkMessageAddU32(lua_State* L);
		static int32_t luaNetworkMessageAddU64(lua_State* L);
		static int32_t luaNetworkMessageAddString(lua_State* L);

		// OutputMessage
		static int32_t luaOutputMessageCreate(lua_State* L);
		static int32_t luaOutputMessageDelete(lua_State* L);

		// End Bindings

        int newSandboxEnv();

        int getGlobalEnvironment();
		void setGlobalEnvironment(int env);
		void resetGlobalEnvironment();

        void getEnv(int index = -1);
    	void setEnv(int index = -2);

		static void pop(lua_State* L, int n = 1);
		int getTop() { return lua_gettop(m_luaState); }
		static int getTop(lua_State* L) { return lua_gettop(L); }

		int stackSize() { return getTop(); }
		void checkStack() { assert(getTop() <= 20); }
		void clearStack() { pop(m_luaState, stackSize()); }
		static void clearStack(lua_State* L) { pop(L, lua_gettop(L)); }

        bool hasIndex(int index) { return (stackSize() >= (index < 0 ? -index : index) && index != 0); }

        void setMetatable(int index = -2);
        static void setMetatable(lua_State* L, int32_t index, const std::string& name);

		void setField(const char* key, int index = -2);

        void putFunctionOnStack(int32_t functionId);
		void putFunctionOnStack(const std::string& function);
		void putFunctionOnStack(const char* function);

		static void startTrackStack(lua_State* L);
		static void endTrackStack(lua_State* L);

		void registerGlobalFunction(const std::string& name, lua_CFunction function);
		void registerStaticMethod(const std::string& className, const std::string& methodName, lua_CFunction method);
		void registerStaticMetaMethod(const std::string& className, const std::string& methodName, lua_CFunction func);

		template <typename T>
		static void pushTuple(lua_State* L, std::tuple<const char*, T> value) {
			LuaStack::Push<T>::Value(L, std::get<1>(value));
			lua_setfield(L, -2, std::get<0>(value));
		}

        void pushLuaResult(lua_State* luaState, int results, AnyVector& vec) {
			if (results > 0){
				for (int index = -1; index >= -results; index--) {
					if (isNumber(luaState, index)) {
						vec.push_back(getNumber<int>(luaState, index));

					} else if (isString(luaState, index)) {
						vec.push_back(getString(luaState, index));

					} else if (isBoolean(luaState, index)) {
						vec.push_back(getBoolean(luaState, index));

					} else {
						vec.push_back(nullptr);
					}
				}

				std::reverse(vec.begin(), vec.end());
				lua_pop(luaState, results);
			}
		}

        template <typename Function, typename... T>
		void internalCallLuaField(Function function, int nresults, AnyVector& retVec, int sandboxEnv = -1, T&&... args){
			if (sandboxEnv > 0) {
				setGlobalEnvironment(sandboxEnv);
			}

			constexpr std::size_t argsCount = sizeof...(args);

			lua_createtable(m_luaState, 0, argsCount);
			(pushTuple(m_luaState, std::forward<T>(args)), ...);

			putFunctionOnStack(function);

			if (!isFunction(m_luaState, -1)) {
				lua_pop(m_luaState, 1);
				return;
			}

			lua_pushvalue(m_luaState, -2);

			if (lua_pcall(m_luaState, 1, nresults, 0) != 0) {
				LuaScript::reportError("internalCallLuaField", lua_tostring(m_luaState, -1), m_luaState, true);
				lua_pop(m_luaState, 2);
				return;
			}

			pushLuaResult(m_luaState, nresults, retVec);

			lua_pop(m_luaState, 1);

			if (sandboxEnv > 0) {
				resetGlobalEnvironment();
			}
		}

		template <typename Function>
		void internalCallLuaFieldRef(Function function, int nresults, AnyVector& retVec, int32_t tableRef, int sandboxEnv = -1) {
			if (sandboxEnv > 0) {
				setGlobalEnvironment(sandboxEnv);
			}

			getRef(tableRef);
			putFunctionOnStack(function);

			if (!isFunction(m_luaState, -1)) {
				lua_pop(m_luaState, 2);
				return;
			}

			lua_pushvalue(m_luaState, -2);

			if (lua_pcall(m_luaState, 1, nresults, 0) != 0) {
				LuaScript::reportError("internalCallLuaFieldRef", lua_tostring(m_luaState, -1), m_luaState, true);
				lua_pop(m_luaState, 2);
				return;
			}

			pushLuaResult(m_luaState, nresults, retVec);

			lua_pop(m_luaState, 1);

			if (sandboxEnv > 0) {
				resetGlobalEnvironment();
			}
		}

		template <typename Function>
		void internalCallLuaFieldNoRetRef(Function function, int32_t tableRef, int sandboxEnv = -1) {
			if (sandboxEnv > 0) {
				setGlobalEnvironment(sandboxEnv);
			}

			getRef(tableRef);
			putFunctionOnStack(function);

			if (!isFunction(m_luaState, -1)) {
				lua_pop(m_luaState, 2);
				return;
			}

			lua_pushvalue(m_luaState, -2);

			if (lua_pcall(m_luaState, 1, 0, 0) != 0) {
				LuaScript::reportError("internalCallLuaFieldNoRetRef", lua_tostring(m_luaState, -1), m_luaState, true);
				lua_pop(m_luaState, 2);
				return;
			}

			lua_pop(m_luaState, 1);

			if (sandboxEnv > 0) {
				resetGlobalEnvironment();
			}
		}

		template <typename Function, typename... T>
		void internalCallLuaFieldNoRet(Function function, int sandboxEnv = -1, T&&... args) {
			if (sandboxEnv > 0) {
				setGlobalEnvironment(sandboxEnv);
			}

			constexpr std::size_t argsCount = sizeof...(args);

			lua_createtable(m_luaState, 0, argsCount);
			(pushTuple(m_luaState, std::forward<T>(args)), ...);

			putFunctionOnStack(function);

			if (!isFunction(m_luaState, -1)) {
				lua_pop(m_luaState, 2);
				return;
			}

			lua_pushvalue(m_luaState, -2);

			if (lua_pcall(m_luaState, 1, 0, 0) != 0) {
				LuaScript::reportError("internalCallLuaFieldNoRet", lua_tostring(m_luaState, -1), m_luaState, true);
				g_logger.printBacktrace();
				lua_pop(m_luaState, 2);
				return;
			}

			lua_pop(m_luaState, 1);

			if (sandboxEnv > 0) {
				resetGlobalEnvironment();
			}
		}

        template <typename Function, typename... T>
		void callLuaField(Function function, int nresults, AnyVector& retVec, T&&... args){
			internalCallLuaField(function, nresults, retVec, -1, std::forward<T>(args)...);
		}

		// sandbox
		template <typename Function, typename... T>
		void callSandboxLuaField(Function function, int nresults, AnyVector& retVec, int sandboxEnv, T&&... args){
			internalCallLuaField(function, nresults, retVec, sandboxEnv, std::forward<T>(args)...);
		}

		template <typename Function>
		void callLuaFieldRef(Function function, int nresults, AnyVector& retVec, int32_t tableRef){
			internalCallLuaFieldRef(function, nresults, retVec, tableRef, -1);
		}

		// sandbox
		template <typename Function>
		void callSandboxLuaFieldRef(Function function, int nresults, AnyVector& retVec, int32_t tableRef, int sandboxEnv){
			internalCallLuaFieldRef(function, nresults, retVec, tableRef, sandboxEnv);
		}

		// no return
		template <typename Function, typename... T>
		void callLuaFieldNoRet(Function function, T&&... args){
			internalCallLuaFieldNoRet(function, -1, std::forward<T>(args)...);
		}

		// no return sandbox
		template <typename Function, typename... T>
		void callSandboxLuaFieldNoRet(Function function, int sandboxEnv, T&&... args){
			internalCallLuaFieldNoRet(function, sandboxEnv, std::forward<T>(args)...);
		}

		// no return ref
		template <typename Function>
		void callLuaFieldNoRetRef(Function function, int32_t tableRef){
			internalCallLuaFieldNoRetRef(function, tableRef);
		}

		// no return ref sandbox
		template <typename Function>
		void callSandboxLuaFieldNoRetRef(Function function, int32_t tableRef, int sandboxEnv){
			internalCallLuaFieldNoRetRef(function, tableRef, sandboxEnv);
		}

        static bool isTable(lua_State* L, int32_t arg) {
			return lua_istable(L, arg);
		}

        static bool isFunction(lua_State* L, int32_t arg) {
			return lua_isfunction(L, arg);
		}

        static bool isBoolean(lua_State* L, int32_t arg) {
			return lua_isboolean(L, arg);
		}

        static bool isUserdata(lua_State* L, int32_t arg) {
			return lua_isuserdata(L, arg) != 0;
		}

        static bool isString(lua_State* L, int32_t arg) {
			return lua_isstring(L, arg) != 0;
		}

        static bool isNumber(lua_State* L, int32_t arg) {
			return lua_type(L, arg) == LUA_TNUMBER;
		}

        static bool getBoolean(lua_State* L, int32_t arg) {
			return lua_toboolean(L, arg) != 0;
		}

        template<typename T>
		static typename std::enable_if<std::is_enum<T>::value, T>::type getNumber(lua_State* L, int32_t arg) {
			return static_cast<T>(static_cast<int64_t>(lua_tonumber(L, arg)));
		}

		template<typename T>
		static typename std::enable_if<std::is_integral<T>::value && std::is_unsigned<T>::value, T>::type getNumber(lua_State* L, int32_t arg) {
			double num = lua_tonumber(L, arg);
			if (num < static_cast<double>(std::numeric_limits<T>::lowest()) || num > static_cast<double>(std::numeric_limits<T>::max())) {
				reportErrorFunc(L, fmt::format("Argument {} has out-of-range value for {}: {}", arg, typeid(T).name(), num));
			}

			return static_cast<T>(num);
		}

		template<typename T>
		static typename std::enable_if<(std::is_integral<T>::value && (std::is_signed<T>::value) || std::is_floating_point<T>::value), T>::type getNumber(lua_State* L, int32_t arg) {
			double num = lua_tonumber(L, arg);
			if (num < static_cast<double>(std::numeric_limits<T>::lowest()) || num > static_cast<double>(std::numeric_limits<T>::max())) {
				reportErrorFunc(L, fmt::format("Argument {} has out-of-range value for {}: {}", arg, typeid(T).name(), num));
			}

			return static_cast<T>(num);
		}

		template<typename T>
		static T getNumber(lua_State *L, int32_t arg, T defaultValue) {
			const auto parameters = lua_gettop(L);
			if (parameters == 0 || arg > parameters) {
				return defaultValue;
			}
			return getNumber<T>(L, arg);
		}

        template<class T>
		static void pushUserdata(lua_State* L, T* value) {
			T** userdata = static_cast<T**>(lua_newuserdata(L, sizeof(T*)));
			*userdata = value;
		}

		template<class T>
		static T* getUserdata(lua_State* L, int32_t arg) {
			T** userdata = getRawUserdata<T>(L, arg);
			if (!userdata) {
				return nullptr;
			}
			return *userdata;
		}

		template<class T>
		static T** getRawUserdata(lua_State* L, int32_t arg) {
			return static_cast<T**>(lua_touserdata(L, arg));
		}

		// Shared Ptr
		template<class T>
		static void pushSharedPtr(lua_State* L, T value) {
			new (lua_newuserdata(L, sizeof(T))) T(std::move(value));
		}

		template<class T>
		static std::shared_ptr<T>& getSharedPtr(lua_State* L, int32_t arg) {
			return *static_cast<std::shared_ptr<T>*>(lua_touserdata(L, arg));
		}

		static void pushBoolean(lua_State* L, bool value);
		static void pushNil(lua_State* L);

        static void pushString(lua_State* L, const std::string& value);
        static std::string getString(lua_State* L, int32_t arg);
        static std::string popString(lua_State* L);

        const std::string& getLastLuaError() const {
			return m_lastLuaError;
		}

        const std::string& getLoadingFile() const {
			return m_loadingFile;
		}

        lua_State* getLuaState() { return m_luaState; }

    private:
        static std::string getStackTrace(lua_State* L, const std::string& error_desc);

        int32_t m_globalEnv = -1;

        std::string m_lastLuaError;
        std::string m_loadingFile;

        lua_State* m_luaState = nullptr;
};

extern LuaScriptPtr g_lua;
extern LuaTablePtr g_config;

template<>
struct LuaStack::Push<float>
{
	static void Value(lua_State* L, float value) {
		lua_pushnumber(L, static_cast<lua_Number>(value));
	}
};

template<>
struct LuaStack::Pop<float>
{
	static float Value(lua_State* L, int index = -1) {
		float value = static_cast<float>(lua_tonumber(L, index));
		lua_pop(L, 1);
		return value;
	}
};

template<>
struct LuaStack::Push<double>
{
	static void Value(lua_State* L, double value) {
		lua_pushnumber(L, static_cast<lua_Number>(value));
	}
};

template<>
struct LuaStack::Pop<double>
{
	static double Value(lua_State* L, int index = -1) {
		double value = static_cast<int>(lua_tonumber(L, index));
		lua_pop(L, 1);
		return value;
	}
};

template<>
struct LuaStack::Push<uint8_t>
{
	static void Value(lua_State* L, uint8_t value) {
		lua_pushnumber(L, static_cast<lua_Number>(value));
	}
};

template<>
struct LuaStack::Pop<uint8_t>
{
	static uint8_t Value(lua_State* L, int index = -1) {
		uint8_t value = static_cast<uint8_t>(lua_tonumber(L, index));
		lua_pop(L, 1);
		return value;
	}
};

template<>
struct LuaStack::Push<uint16_t>
{
	static void Value(lua_State* L, uint16_t value) {
		lua_pushnumber(L, static_cast<lua_Number>(value));
	}
};

template<>
struct LuaStack::Pop<uint16_t>
{
	static uint16_t Value(lua_State* L, int index = -1) {
		uint16_t value = static_cast<uint16_t>(lua_tonumber(L, index));
		lua_pop(L, 1);
		return value;
	}
};

template<>
struct LuaStack::Push<uint32_t>
{
	static void Value(lua_State* L, uint32_t value) {
		lua_pushnumber(L, static_cast<lua_Number>(value));
	}
};

template<>
struct LuaStack::Pop<uint32_t>
{
	static uint32_t Value(lua_State* L, int index = -1) {
		uint32_t value = static_cast<int>(lua_tonumber(L, index));
		lua_pop(L, 1);
		return value;
	}
};

template<>
struct LuaStack::Push<uint64_t>
{
	static void Value(lua_State* L, uint64_t value) {
		lua_pushnumber(L, static_cast<lua_Number>(value));
	}
};

template<>
struct LuaStack::Pop<uint64_t>
{
	static uint64_t Value(lua_State* L, int index = -1) {
		uint64_t value = static_cast<int>(lua_tonumber(L, index));
		lua_pop(L, 1);
		return value;
	}
};

template<>
struct LuaStack::Push<int>
{
	static void Value(lua_State* L, int value) {
		lua_pushnumber(L, static_cast<lua_Number>(value));
	}
};

template<>
struct LuaStack::Pop<int>
{
	static int Value(lua_State* L, int index = -1) {
		int value = static_cast<int>(lua_tonumber(L, index));
		lua_pop(L, 1);
		return value;
	}
};

template<>
struct LuaStack::Push<bool>
{
	static void Value(lua_State* L, bool value) {
		lua_pushboolean(L, value ? 1 : 0);
	}
};

template<>
struct LuaStack::Pop<bool>
{
	static bool Value(lua_State* L, int index = -1) {
		bool value = lua_toboolean(L, index) != 0;
		lua_pop(L, 1);
		return value;
	}
};

template<>
struct LuaStack::Push<const char>
{
	static void Value(lua_State* L, const char* value) {
		lua_pushstring(L, value);
	}
};

template<>
struct LuaStack::Push<const char*>
{
	static void Value(lua_State* L, const char* value) {
		LuaStack::Push<const char>::Value(L, value);
	}
};

template<>
struct LuaStack::Pop<const char>
{
	static const char* Value(lua_State* L, int index = -1) {
		const char* value = lua_tostring(L, index);
		lua_pop(L, 1);
		return value;
	}
};

template<>
struct LuaStack::Pop<const char*>
{
	static const char* Value(lua_State* L, int index = -1) {
		return LuaStack::Pop<const char>::Value(L, index);
	}
};

template<>
struct LuaStack::Push<std::string>
{
	static void Value(lua_State* L, const std::string& value) {
		lua_pushstring(L, value.c_str());
	}
};

template<>
struct LuaStack::Pop<std::string>
{
	static std::string Value(lua_State* L, int index = -1) {
		const char* value = lua_tostring(L, index);
		lua_pop(L, 1);
		return value ? std::string(value) : std::string();
	}
};

template<>
struct LuaStack::Push<std::nullptr_t>
{
	static void Value(lua_State* L, std::nullptr_t value) {
		lua_pushnil(L);
	}
};

template<>
struct LuaStack::Pop<std::nullptr_t>
{
	static uint64_t Value(lua_State* L, int index = -1) {
		uint64_t value = static_cast<int>(lua_tonumber(L, index));
		lua_pop(L, 1);
		return value;
	}
};

template<>
struct LuaStack::Pop<Module>
{
	static Module* Value(lua_State* L, int index = -1) {
		Module* value = LuaScript::getUserdata<Module>(L, index);
		lua_pop(L, 1);
		return value;
	}
};

template<>
struct LuaStack::Push<Module>
{
	static void Value(lua_State* L, Module* value) {
		LuaScript::pushUserdata<Module>(L, value);
		LuaScript::setMetatable(L, -1, "Module");
	}
};

template<>
struct LuaStack::Pop<Protocol>
{
	static Protocol* Value(lua_State* L, int index = -1) {
		Protocol* value = LuaScript::getUserdata<Protocol>(L, index);
		lua_pop(L, 1);
		return value;
	}
};

template<>
struct LuaStack::Push<ProtocolSharedPtr>
{
	static void Value(lua_State* L, ProtocolSharedPtr value) {
		LuaScript::pushSharedPtr<ProtocolSharedPtr>(L, value);
		LuaScript::setMetatable(L, -1, "Protocol");
	}
};

template<>
struct LuaStack::Push<Protocol>
{
	static void Value(lua_State* L, Protocol* value) {
		LuaScript::pushUserdata<Protocol>(L, value);
		LuaScript::setMetatable(L, -1, "Protocol");
	}
};

template<>
struct LuaStack::Push<Protocol*>
{
	static void Value(lua_State* L, Protocol* value) {
		LuaStack::Push<Protocol>::Value(L, value);
	}
};

template<>
struct LuaStack::Pop<NetworkMessage>
{
	static NetworkMessage* Value(lua_State* L, int index = -1) {
		NetworkMessage* value = LuaScript::getUserdata<NetworkMessage>(L, index);
		lua_pop(L, 1);
		return value;
	}
};

template<>
struct LuaStack::Push<NetworkMessage*>
{
	static void Value(lua_State* L, NetworkMessage* value) {
		LuaScript::pushUserdata<NetworkMessage>(L, value);
		LuaScript::setMetatable(L, -1, "NetworkMessage");
	}
};

template<>
struct LuaStack::Pop<OutputMessage>
{
	static OutputMessage* Value(lua_State* L, int index = -1) {
		OutputMessage* value = LuaScript::getUserdata<OutputMessage>(L, index);
		lua_pop(L, 1);
		return value;
	}
};

template<>
struct LuaStack::Push<OutputMessage>
{
	static void Value(lua_State* L, OutputMessage* value) {
		LuaScript::pushUserdata<OutputMessage>(L, value);
		LuaScript::setMetatable(L, -1, "OutputMessage");
	}
};

#endif
