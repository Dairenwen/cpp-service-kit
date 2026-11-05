cmake_minimum_required(VERSION 3.20)

# 单独构建时，将最终产物放到项目的 build 目录。
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    project(cpp_service_kit_source LANGUAGES CXX)
    set(CPP_KIT_BUILD_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../build")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Threads REQUIRED)
find_package(gflags CONFIG REQUIRED)
find_package(Protobuf 3.12 REQUIRED)
find_path(BRPC_INCLUDE_DIR brpc/server.h REQUIRED)
find_library(BRPC_LIBRARY NAMES brpc REQUIRED)
find_library(LEVELDB_LIBRARY NAMES leveldb REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(etcd-cpp-api CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(jsoncpp CONFIG REQUIRED)
find_package(Boost REQUIRED)
find_path(AMQPCPP_INCLUDE_DIR amqpcpp.h REQUIRED)
find_path(LIBEV_INCLUDE_DIR ev.h REQUIRED)
find_library(AMQPCPP_LIBRARY NAMES amqpcpp REQUIRED)
find_library(LIBEV_LIBRARY NAMES ev REQUIRED)
find_package(ZLIB REQUIRED)

# 封装库输出到 build/source
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CPP_KIT_BUILD_DIR}/source")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CPP_KIT_BUILD_DIR}/source")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CPP_KIT_BUILD_DIR}/source")

# 编译 BreezeRPC 封装
add_library(breeze_rpc STATIC BreezeRPC.cpp)
target_include_directories(breeze_rpc PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${BRPC_INCLUDE_DIR}"
)
target_link_libraries(breeze_rpc PUBLIC
    "${BRPC_LIBRARY}" protobuf::libprotobuf gflags
    "${LEVELDB_LIBRARY}" OpenSSL::SSL OpenSSL::Crypto ZLIB::ZLIB
    Threads::Threads ${CMAKE_DL_LIBS}
)

# 编译 BreezeEtcd 封装及其使用的日志、字符串工具。

add_library(breeze_etcd STATIC BreezeEtcd.cpp BreezeLog.cpp BreezeUtil.cpp)
target_include_directories(breeze_etcd PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${Boost_INCLUDE_DIRS}"
)
target_link_libraries(breeze_etcd PUBLIC
    etcd-cpp-api spdlog::spdlog JsonCpp::JsonCpp Threads::Threads
)

# 编译基于 AMQP-CPP 和 libev 的消息队列封装。

add_library(breeze_mq STATIC BreezeMQ.cpp BreezeLog.cpp)
target_include_directories(breeze_mq PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}" "${AMQPCPP_INCLUDE_DIR}" "${LIBEV_INCLUDE_DIR}"
)
target_link_libraries(breeze_mq PUBLIC "${AMQPCPP_LIBRARY}" "${LIBEV_LIBRARY}" spdlog::spdlog Threads::Threads)

# 编译FastDFS 客户端封装
find_path(FASTDFS_INCLUDE_DIR fastdfs/fdfs_client.h)
find_path(FASTCOMMON_INCLUDE_DIR fastcommon/logger.h)
find_library(FASTDFS_CLIENT_LIBRARY NAMES fdfsclient)
find_library(FASTCOMMON_LIBRARY NAMES fastcommon)
if(FASTDFS_INCLUDE_DIR AND FASTCOMMON_INCLUDE_DIR AND FASTDFS_CLIENT_LIBRARY AND FASTCOMMON_LIBRARY)
    add_library(breeze_fdfs STATIC BreezeFDFS.cpp BreezeLog.cpp)
    target_include_directories(breeze_fdfs PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}" "${FASTDFS_INCLUDE_DIR}" "${FASTCOMMON_INCLUDE_DIR}"
    )
    target_link_libraries(breeze_fdfs PUBLIC
        "${FASTDFS_CLIENT_LIBRARY}" "${FASTCOMMON_LIBRARY}" spdlog::spdlog Threads::Threads
    )
else()
    message(STATUS "FastDFS 客户端依赖未找到，跳过 FastDFS 封装")
endif()

# 编译 Redis++ 客户端封装。
find_path(REDIS_PLUS_PLUS_INCLUDE_DIR sw/redis++/redis.h)
find_library(REDIS_PLUS_PLUS_LIBRARY NAMES redis++)
find_library(HIREDIS_LIBRARY NAMES hiredis)
if(REDIS_PLUS_PLUS_INCLUDE_DIR AND REDIS_PLUS_PLUS_LIBRARY AND HIREDIS_LIBRARY)
    add_library(breeze_redis STATIC BreezeRedis.cpp)
    target_include_directories(breeze_redis PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}" "${REDIS_PLUS_PLUS_INCLUDE_DIR}"
    )
    target_link_libraries(breeze_redis PUBLIC
        "${REDIS_PLUS_PLUS_LIBRARY}" "${HIREDIS_LIBRARY}" Threads::Threads
    )
else()
    message(STATUS "Redis++ 或 hiredis 未找到，跳过 Redis 封装")
endif()

# 编译 ODB MySQL 连接池封装。
find_path(ODB_INCLUDE_DIR odb/mysql/database.hxx)
find_library(ODB_LIBRARY NAMES odb)
find_library(ODB_MYSQL_LIBRARY NAMES odb-mysql)
if(ODB_INCLUDE_DIR AND ODB_LIBRARY AND ODB_MYSQL_LIBRARY)
    add_library(breeze_odb STATIC BreezeODB.cpp)
    target_include_directories(breeze_odb PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}" "${ODB_INCLUDE_DIR}"
    )
    target_link_libraries(breeze_odb PUBLIC
        "${ODB_MYSQL_LIBRARY}" "${ODB_LIBRARY}" Threads::Threads
    )
else()
    message(STATUS "ODB 或 ODB MySQL 未找到，跳过 ODB 封装")
endif()
