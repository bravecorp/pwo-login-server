cmake_minimum_required(VERSION 3.14)

set(CMAKE_DISABLE_SOURCE_CHANGES ON)
set(CMAKE_DISABLE_IN_SOURCE_BUILD ON)

project(loginserver CXX)

add_subdirectory(src)
add_executable(loginserver ${loginserver_SRC} src/main.cpp)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

set(INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include")

set_target_properties(loginserver PROPERTIES CXX_STANDARD 17)
set_target_properties(loginserver PROPERTIES CXX_STANDARD_REQUIRED ON)

if (NOT WIN32)
    add_compile_options(-Wall -Werror -pipe -fvisibility=hidden)
endif ()

# Find packages.
find_package(CryptoPP QUIET)
if (CryptoPP_FOUND)  # vcpkg-provided cmake package called CryptoPP
    set(Crypto++_LIBRARIES "cryptopp-static")
else()
    find_package(Crypto++ REQUIRED MODULE)
endif ()

find_package(fmt 6.1.2 REQUIRED)

find_package(unofficial-libmariadb CONFIG QUIET)
if (unofficial-libmariadb_FOUND)
    set(MYSQL_CLIENT_LIBS "libmariadb")
else ()
    find_package(MySQL REQUIRED)
endif ()

find_package(Threads REQUIRED)
find_package(Hiredis 0.14.1 REQUIRED)
find_package(LuaJIT)
find_package(Boost 1.74.0 REQUIRED COMPONENTS system)

include_directories(
    ${INCLUDE_DIR}
    ${Boost_INCLUDE_DIRS}
    ${Crypto++_INCLUDE_DIR}
    ${LUA_INCLUDE_DIR}
    ${MYSQL_INCLUDE_DIR}
    ${HIREDIS_INCLUDE_DIRS}
)

target_link_libraries(loginserver PRIVATE
    Boost::system
    fmt::fmt
    ${HIREDIS_LIBRARIES}
    ${CMAKE_THREAD_LIBS_INIT}
    ${Crypto++_LIBRARIES}
    ${LUA_LIBRARIES}
    ${MYSQL_CLIENT_LIBS}
    ${ZLIB_LIBRARY}
)

 ### INTERPROCEDURAL_OPTIMIZATION ###
cmake_policy(SET CMP0069 NEW)
include(CheckIPOSupported)
check_ipo_supported(RESULT result OUTPUT error)
if (result)
    message(STATUS "IPO / LTO enabled")
    set_target_properties(loginserver PROPERTIES INTERPROCEDURAL_OPTIMIZATION True)
else ()
    message(STATUS "IPO / LTO not supported: <${error}>")
endif ()
### END INTERPROCEDURAL_OPTIMIZATION ###
