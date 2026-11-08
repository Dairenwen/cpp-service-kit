cmake_minimum_required(VERSION 3.20)

# 支持单独配置 source 目录。
# 从项目根目录构建时沿用顶层设置；直接配置 source 时单独声明项目和安装目录。
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    project(BreezeKitSource VERSION 0.1.0 LANGUAGES CXX)
    include(GNUInstallDirs)
    set(CPP_KIT_BUILD_DIR "${CMAKE_BINARY_DIR}")
endif()

# 依赖同时供安装后的组件配置复用，路径由各自的机器查找。
# 此处在构建环境查找全部模块的依赖。安装后的组件也复用这些脚本，
# 在使用方环境重新查找依赖，避免把构建机器上的库路径写进发布包。
include("${CMAKE_CURRENT_SOURCE_DIR}/../cmake/dependencies/log.cmake")
include("${CMAKE_CURRENT_SOURCE_DIR}/../cmake/dependencies/util.cmake")
include("${CMAKE_CURRENT_SOURCE_DIR}/../cmake/dependencies/rpc.cmake")
include("${CMAKE_CURRENT_SOURCE_DIR}/../cmake/dependencies/etcd.cmake")
include("${CMAKE_CURRENT_SOURCE_DIR}/../cmake/dependencies/mq.cmake")
include("${CMAKE_CURRENT_SOURCE_DIR}/../cmake/dependencies/fdfs.cmake")
include("${CMAKE_CURRENT_SOURCE_DIR}/../cmake/dependencies/redis.cmake")
include("${CMAKE_CURRENT_SOURCE_DIR}/../cmake/dependencies/odb.cmake")
include("${CMAKE_CURRENT_SOURCE_DIR}/../cmake/dependencies/elastic.cmake")
include("${CMAKE_CURRENT_SOURCE_DIR}/../cmake/dependencies/code.cmake")
include("${CMAKE_CURRENT_SOURCE_DIR}/../cmake/dependencies/trans.cmake")

# 每个模块独立生成静态库；公共日志和工具由其他模块链接。

# 以下模块遵循相同写法：
# add_library(... STATIC ...) 把源码编译为 .a 静态库。
# EXPORT_NAME 决定安装后的名称，例如 breeze_log 对应 BreezeKit::log。
# PUBLIC cxx_std_17 将 C++17 要求传递给链接此模块的项目。
# BUILD_INTERFACE 指源码构建时的头文件目录；INSTALL_INTERFACE 指安装前缀下的 include。
# PUBLIC 链接项会传递给使用方，帮助使用方链接静态库所需的依赖。
# 日志模块提供项目共用的 spdlog 日志及线程支持。
add_library(breeze_log STATIC BreezeLog.cpp)
set_target_properties(breeze_log PROPERTIES EXPORT_NAME log)
target_compile_features(breeze_log PUBLIC cxx_std_17)
target_include_directories(breeze_log PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
target_link_libraries(breeze_log PUBLIC spdlog::spdlog Threads::Threads)

# 工具模块使用日志、JsonCpp 和 Boost；其他需要 JSON 等工具的模块可直接链接它。
add_library(breeze_util STATIC BreezeUtil.cpp)
set_target_properties(breeze_util PROPERTIES EXPORT_NAME util)
target_compile_features(breeze_util PUBLIC cxx_std_17)
target_include_directories(breeze_util PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
target_link_libraries(breeze_util PUBLIC breeze_log JsonCpp::JsonCpp Boost::boost)

# RPC 模块连接 brpc、Protobuf 和 gflags；LevelDB、OpenSSL、zlib 等提供底层支持。
add_library(breeze_rpc STATIC BreezeRPC.cpp)
set_target_properties(breeze_rpc PROPERTIES EXPORT_NAME rpc)
target_compile_features(breeze_rpc PUBLIC cxx_std_17)
target_include_directories(breeze_rpc PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
target_link_libraries(breeze_rpc PUBLIC BreezeDep::brpc protobuf::libprotobuf gflags BreezeDep::leveldb OpenSSL::SSL OpenSSL::Crypto ZLIB::ZLIB Threads::Threads ${CMAKE_DL_LIBS})

# etcd 模块复用日志和工具，通过 etcd-cpp-api 与注册中心通信。
add_library(breeze_etcd STATIC BreezeEtcd.cpp)
set_target_properties(breeze_etcd PROPERTIES EXPORT_NAME etcd)
target_compile_features(breeze_etcd PUBLIC cxx_std_17)
target_include_directories(breeze_etcd PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
target_link_libraries(breeze_etcd PUBLIC breeze_log breeze_util etcd-cpp-api Boost::boost)

# 消息队列模块使用 AMQP-CPP、libev 事件循环和公共日志。
add_library(breeze_mq STATIC BreezeMQ.cpp)
set_target_properties(breeze_mq PROPERTIES EXPORT_NAME mq)
target_compile_features(breeze_mq PUBLIC cxx_std_17)
target_include_directories(breeze_mq PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
target_link_libraries(breeze_mq PUBLIC breeze_log BreezeDep::amqpcpp BreezeDep::ev)

# FastDFS 模块使用 fdfsclient、fastcommon 和公共日志。
add_library(breeze_fdfs STATIC BreezeFDFS.cpp)
set_target_properties(breeze_fdfs PROPERTIES EXPORT_NAME fdfs)
target_compile_features(breeze_fdfs PUBLIC cxx_std_17)
target_include_directories(breeze_fdfs PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
target_link_libraries(breeze_fdfs PUBLIC breeze_log BreezeDep::fdfsclient BreezeDep::fastcommon)

# Redis 模块使用 redis-plus-plus 和 hiredis；线程库支持客户端连接池。
add_library(breeze_redis STATIC BreezeRedis.cpp)
set_target_properties(breeze_redis PROPERTIES EXPORT_NAME redis)
target_compile_features(breeze_redis PUBLIC cxx_std_17)
target_include_directories(breeze_redis PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
target_link_libraries(breeze_redis PUBLIC BreezeDep::redispp BreezeDep::hiredis Threads::Threads)

# ODB 模块使用 ODB 核心库和 MySQL 驱动，创建数据库连接池。
add_library(breeze_odb STATIC BreezeODB.cpp)
set_target_properties(breeze_odb PROPERTIES EXPORT_NAME odb)
target_compile_features(breeze_odb PUBLIC cxx_std_17)
target_include_directories(breeze_odb PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
target_link_libraries(breeze_odb PUBLIC BreezeDep::odbmysql BreezeDep::odb Threads::Threads)

# Elasticsearch 模块复用日志、JSON 工具，并连接 elasticlient 和 CPR。
add_library(breeze_elastic STATIC BreezeElastic.cpp)
set_target_properties(breeze_elastic PROPERTIES EXPORT_NAME elastic)
target_compile_features(breeze_elastic PUBLIC cxx_std_17)
target_include_directories(breeze_elastic PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
target_link_libraries(breeze_elastic PUBLIC breeze_log breeze_util BreezeDep::elasticlient BreezeDep::cpr)

# 邮件模块使用 libcurl 和公共日志，发送验证码邮件。
add_library(breeze_code STATIC BreezeCode.cpp)
set_target_properties(breeze_code PROPERTIES EXPORT_NAME code)
target_compile_features(breeze_code PUBLIC cxx_std_17)
target_include_directories(breeze_code PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
target_link_libraries(breeze_code PUBLIC breeze_log CURL::libcurl)

# 转码模块复用日志和工具，并连接 FFmpeg 的封装、编解码和工具库。
add_library(breeze_trans STATIC BreezeTrans.cpp)
set_target_properties(breeze_trans PROPERTIES EXPORT_NAME trans)
target_compile_features(breeze_trans PUBLIC cxx_std_17)
target_include_directories(breeze_trans PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
target_link_libraries(breeze_trans PUBLIC breeze_log breeze_util BreezeDep::avformat BreezeDep::avcodec BreezeDep::avutil)

# 分别导出模块，使用方只需加载自己选择的组件。
# 每个模块的两条 install 命令分别完成：
# 1. 将 .a 安装到 CMAKE_INSTALL_LIBDIR，并登记到该模块的导出集合。
# 2. 生成对应 targets.cmake，使用 BreezeKit:: 命名空间供外部项目链接。
# 分模块导出使 find_package(... COMPONENTS redis) 等调用只加载所需组件。
install(TARGETS breeze_log EXPORT BreezeKit-log-targets
    ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}")
install(EXPORT BreezeKit-log-targets NAMESPACE BreezeKit::
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/BreezeKit")
install(TARGETS breeze_util EXPORT BreezeKit-util-targets
    ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}")
install(EXPORT BreezeKit-util-targets NAMESPACE BreezeKit::
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/BreezeKit")
install(TARGETS breeze_rpc EXPORT BreezeKit-rpc-targets
    ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}")
install(EXPORT BreezeKit-rpc-targets NAMESPACE BreezeKit::
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/BreezeKit")
install(TARGETS breeze_etcd EXPORT BreezeKit-etcd-targets
    ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}")
install(EXPORT BreezeKit-etcd-targets NAMESPACE BreezeKit::
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/BreezeKit")
install(TARGETS breeze_mq EXPORT BreezeKit-mq-targets
    ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}")
install(EXPORT BreezeKit-mq-targets NAMESPACE BreezeKit::
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/BreezeKit")
install(TARGETS breeze_fdfs EXPORT BreezeKit-fdfs-targets
    ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}")
install(EXPORT BreezeKit-fdfs-targets NAMESPACE BreezeKit::
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/BreezeKit")
install(TARGETS breeze_redis EXPORT BreezeKit-redis-targets
    ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}")
install(EXPORT BreezeKit-redis-targets NAMESPACE BreezeKit::
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/BreezeKit")
install(TARGETS breeze_odb EXPORT BreezeKit-odb-targets
    ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}")
install(EXPORT BreezeKit-odb-targets NAMESPACE BreezeKit::
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/BreezeKit")
install(TARGETS breeze_elastic EXPORT BreezeKit-elastic-targets
    ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}")
install(EXPORT BreezeKit-elastic-targets NAMESPACE BreezeKit::
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/BreezeKit")
install(TARGETS breeze_code EXPORT BreezeKit-code-targets
    ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}")
install(EXPORT BreezeKit-code-targets NAMESPACE BreezeKit::
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/BreezeKit")
install(TARGETS breeze_trans EXPORT BreezeKit-trans-targets
    ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}")
install(EXPORT BreezeKit-trans-targets NAMESPACE BreezeKit::
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/BreezeKit")

# 公开头文件使用 BreezeKit/ 前缀。
# 安装全部公开头文件到 include/BreezeKit，使用方可写 #include <BreezeKit/BreezeLog.hpp>。
install(FILES
    BreezeLog.hpp
    BreezeUtil.hpp
    BreezeRPC.hpp
    BreezeEtcd.hpp
    BreezeMQ.hpp
    BreezeFDFS.hpp
    BreezeRedis.hpp
    BreezeODB.hpp
    BreezeElastic.hpp
    BreezeCode.hpp
    BreezeTrans.hpp
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/BreezeKit")
