/**
 * Copyright (c) 2025 PWO Team. All rights reserved.
 * This code is confidential and intended solely for internal use by authorized personnel.
 * Any unauthorized reproduction, distribution, or disclosure — including publication or sharing outside the company —
 * is strictly prohibited and may result in legal action.
 */

#ifndef UTILS_TYPES_H
#define UTILS_TYPES_H

#include <memory>
#include <string>
#include <vector>
#include <any>

class Connection;
class DBResult;
class Protocol;
class Server;
class ModuleManager;
class LuaScript;
class LuaTable;
class Redis;
class RedisPublisher;
class RedisSubscriber;
class NetworkMessage;
class OutputMessage;

using ConnectionSharedPtr = std::shared_ptr<Connection>;
using ConnectionWeakPtr = std::weak_ptr<Connection>;
using DBResultSharedPtr = std::shared_ptr<DBResult>;
using ProtocolSharedPtr = std::shared_ptr<Protocol>;
using ServerSharedPtr = std::shared_ptr<Server>;
using ModuleManagerPtr = std::shared_ptr<ModuleManager>;
using LuaScriptPtr = std::shared_ptr<LuaScript>;
using LuaTablePtr = std::shared_ptr<LuaTable>;
using RedisPublisherPtr = std::shared_ptr<RedisPublisher>;
using RedisPtr = std::shared_ptr<Redis>;
using RedisSubscriberPtr = std::shared_ptr<RedisSubscriber>;

using StringVector = std::vector<std::string>;
using AnyVector = std::vector<std::any>;

#endif
