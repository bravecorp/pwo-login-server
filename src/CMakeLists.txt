set(loginserver_SRC
    # CORE
    ${CMAKE_CURRENT_LIST_DIR}/core/logger.cpp
    ${CMAKE_CURRENT_LIST_DIR}/core/module.cpp
    ${CMAKE_CURRENT_LIST_DIR}/core/modulemanager.cpp
    ${CMAKE_CURRENT_LIST_DIR}/core/server.cpp
    ${CMAKE_CURRENT_LIST_DIR}/core/signals.cpp
    ${CMAKE_CURRENT_LIST_DIR}/core/tasks.cpp

    # DATABASE
    ${CMAKE_CURRENT_LIST_DIR}/database/database.cpp
    ${CMAKE_CURRENT_LIST_DIR}/database/dbresult.cpp

    # NETWORK
    ${CMAKE_CURRENT_LIST_DIR}/network/connection.cpp
    ${CMAKE_CURRENT_LIST_DIR}/network/connectionmanager.cpp
    ${CMAKE_CURRENT_LIST_DIR}/network/networkmessage.cpp
    ${CMAKE_CURRENT_LIST_DIR}/network/protocol.cpp

    # REDIS
    ${CMAKE_CURRENT_LIST_DIR}/redis/pub.cpp
    ${CMAKE_CURRENT_LIST_DIR}/redis/redis.cpp
    ${CMAKE_CURRENT_LIST_DIR}/redis/sub.cpp

    # SCRIPT
    ${CMAKE_CURRENT_LIST_DIR}/script/lua.cpp

    # UTILS
    ${CMAKE_CURRENT_LIST_DIR}/utils/rsa.cpp
    ${CMAKE_CURRENT_LIST_DIR}/utils/tools.cpp
    ${CMAKE_CURRENT_LIST_DIR}/utils/xtea.cpp
    PARENT_SCOPE)
