cmake_minimum_required(VERSION 3.20)

# 单独构建时，将最终产物放到项目的 build 目录。
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    project(cpp_service_kit_test LANGUAGES CXX)
    set(CPP_KIT_BUILD_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../build")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(GTest REQUIRED)
find_package(Protobuf 3.12 REQUIRED)

# 独立构建测试时，同时构建它依赖的 BreezeRPC 封装。
if(NOT TARGET breeze_rpc)
    add_subdirectory("../source" "${CMAKE_CURRENT_BINARY_DIR}/source_build")
endif()

# ---------- BreezeRPC 测试 ----------

# 生成测试服务使用的协议代码。
set(BREEZE_TEST_PROTO_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated/breeze_rpc_test")
file(MAKE_DIRECTORY "${BREEZE_TEST_PROTO_DIR}")
add_custom_command(
    OUTPUT "${BREEZE_TEST_PROTO_DIR}/cal.pb.cc" "${BREEZE_TEST_PROTO_DIR}/cal.pb.h"
    COMMAND protobuf::protoc
        "--cpp_out=${BREEZE_TEST_PROTO_DIR}"
        "--proto_path=${CMAKE_CURRENT_SOURCE_DIR}/BreezeRPC"
        "${CMAKE_CURRENT_SOURCE_DIR}/BreezeRPC/cal.proto"
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/BreezeRPC/cal.proto" protobuf::protoc
    VERBATIM
)
# 两个测试共用协议文件，确保并行构建时只生成一次。
add_custom_target(breeze_test_proto_generate
    DEPENDS "${BREEZE_TEST_PROTO_DIR}/cal.pb.cc" "${BREEZE_TEST_PROTO_DIR}/cal.pb.h"
)

# 编译测试程序并输出到 build/test。
add_executable(breeze_rpc_test
    BreezeRPC/client.cpp
    BreezeRPC/server.cpp
    "${BREEZE_TEST_PROTO_DIR}/cal.pb.cc"
)
add_dependencies(breeze_rpc_test breeze_test_proto_generate)
target_include_directories(breeze_rpc_test PRIVATE "${BREEZE_TEST_PROTO_DIR}")
target_link_libraries(breeze_rpc_test PRIVATE breeze_rpc GTest::gtest_main)
set_target_properties(breeze_rpc_test PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CPP_KIT_BUILD_DIR}/test"
)

# ---------- BreezeTrans 测试 ----------

# 查找测试使用的 FFmpeg、JsonCpp 和 Spdlog。
find_package(Threads REQUIRED)
find_package(jsoncpp CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_path(AVFORMAT_INCLUDE_DIR libavformat/avformat.h REQUIRED)
find_library(AVFORMAT_LIBRARY NAMES avformat REQUIRED)
find_library(AVCODEC_LIBRARY NAMES avcodec REQUIRED)
find_library(AVUTIL_LIBRARY NAMES avutil REQUIRED)

# 编译转换封装及测试代码，生成 build/test/breeze_trans_test。
add_executable(breeze_trans_test
    BreezeTrans/trans_test.cpp
    ../source/BreezeTrans.cpp
    ../source/BreezeUtil.cpp
    ../source/BreezeLog.cpp
)
target_include_directories(breeze_trans_test PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/../source" "${AVFORMAT_INCLUDE_DIR}"
)
target_compile_definitions(breeze_trans_test PRIVATE
    BREEZE_TEST_VIDEO="${CMAKE_CURRENT_SOURCE_DIR}/BreezeTrans/movie.mp4"
)
target_link_libraries(breeze_trans_test PRIVATE
    GTest::gtest_main JsonCpp::JsonCpp spdlog::spdlog Threads::Threads
    "${AVFORMAT_LIBRARY}" "${AVCODEC_LIBRARY}" "${AVUTIL_LIBRARY}"
)
set_target_properties(breeze_trans_test PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CPP_KIT_BUILD_DIR}/test"
)

# ---------- BreezeEtcd 测试 ----------

# 编译服务注册与发现测试，生成 build/test/breeze_etcd_test。
add_executable(breeze_etcd_test BreezeEtcd/etcd_test.cpp)
target_link_libraries(breeze_etcd_test PRIVATE breeze_etcd GTest::gtest_main)
set_target_properties(breeze_etcd_test PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CPP_KIT_BUILD_DIR}/test"
)

# 复用加法服务和协议，编译 etcd 与 RPC 联合测试。
add_executable(breeze_etcd_rpc_test
    BreezeEtcd/rpc_test.cpp
    BreezeRPC/server.cpp
    "${BREEZE_TEST_PROTO_DIR}/cal.pb.cc"
)
add_dependencies(breeze_etcd_rpc_test breeze_test_proto_generate)
target_include_directories(breeze_etcd_rpc_test PRIVATE "${BREEZE_TEST_PROTO_DIR}")
target_link_libraries(breeze_etcd_rpc_test PRIVATE breeze_etcd breeze_rpc GTest::gtest_main)
set_target_properties(breeze_etcd_rpc_test PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CPP_KIT_BUILD_DIR}/test"
)

# ---------- BreezeMQ 测试 ----------

# 编译消息发布、订阅和延迟消息测试
add_executable(breeze_mq_test BreezeMQ/mq_test.cpp)
target_link_libraries(breeze_mq_test PRIVATE breeze_mq GTest::gtest_main)
set_target_properties(breeze_mq_test PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CPP_KIT_BUILD_DIR}/test"
)

# ---------- BreezeRedis 测试 ----------

# 编译 Redis 连接和读写测试，生成 build/test/breeze_redis_test。

add_executable(breeze_redis_test BreezeRedis/redis_test.cpp)
target_link_libraries(breeze_redis_test PRIVATE breeze_redis GTest::gtest_main)
set_target_properties(breeze_redis_test PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CPP_KIT_BUILD_DIR}/test"
)


# ---------- FastDFS 封装集成测试 ----------

# 需要可访问的 Tracker 和 Storage；可用 FDFS_TRACKER 指定 Tracker 地址。
add_executable(breeze_fdfs_test BreezeFDFS/fdfs_test.cpp)
target_link_libraries(breeze_fdfs_test PRIVATE breeze_fdfs GTest::gtest)
set_target_properties(breeze_fdfs_test PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CPP_KIT_BUILD_DIR}/test"
)

# ---------- ODB MySQL 封装测试 ----------
find_program(ODB_COMPILER NAMES odb)

set(BREEZE_ODB_TEST_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated/breeze_odb_test")
file(MAKE_DIRECTORY "${BREEZE_ODB_TEST_DIR}")
add_custom_command(
    OUTPUT "${BREEZE_ODB_TEST_DIR}/test_record-odb.cxx" "${BREEZE_ODB_TEST_DIR}/test_record-odb.hxx"
    BYPRODUCTS "${BREEZE_ODB_TEST_DIR}/test_record-odb.ixx"
    COMMAND "${ODB_COMPILER}" -d mysql --std c++11 --generate-query
        --output-dir "${BREEZE_ODB_TEST_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/BreezeODB/test_record.hxx"
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/BreezeODB/test_record.hxx"
    VERBATIM
)
add_executable(breeze_odb_test BreezeODB/odb_test.cpp "${BREEZE_ODB_TEST_DIR}/test_record-odb.cxx")
target_include_directories(breeze_odb_test PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/BreezeODB" "${BREEZE_ODB_TEST_DIR}"
)
target_link_libraries(breeze_odb_test PRIVATE breeze_odb GTest::gtest_main)
set_target_properties(breeze_odb_test PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CPP_KIT_BUILD_DIR}/test")


# ---------- Elasticsearch 封装测试 ----------

# 包含本地请求构造测试和需要 Elasticsearch 服务的增删改查测试。
add_executable(breeze_elastic_test BreezeElastic/elastic_test.cpp)
target_link_libraries(breeze_elastic_test PRIVATE breeze_elastic GTest::gtest_main)
set_target_properties(breeze_elastic_test PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CPP_KIT_BUILD_DIR}/test"
)

# 使用 CTest 运行封装测试。
enable_testing()
add_test(NAME breeze_rpc_test COMMAND breeze_rpc_test)
add_test(NAME breeze_trans_test COMMAND breeze_trans_test)
add_test(NAME breeze_etcd_test COMMAND breeze_etcd_test)
set_tests_properties(breeze_etcd_test PROPERTIES TIMEOUT 45)
add_test(NAME breeze_etcd_rpc_test COMMAND breeze_etcd_rpc_test)
set_tests_properties(breeze_etcd_rpc_test PROPERTIES TIMEOUT 45)
add_test(NAME breeze_mq_direct_test COMMAND breeze_mq_test --gtest_filter=BreezeMQTest.DirectMessage)
add_test(NAME breeze_mq_delayed_test COMMAND breeze_mq_test --gtest_filter=BreezeMQTest.DelayedMessage)
set_tests_properties(breeze_mq_direct_test breeze_mq_delayed_test PROPERTIES TIMEOUT 15)
add_test(NAME breeze_redis_test COMMAND breeze_redis_test)
set_tests_properties(breeze_redis_test PROPERTIES TIMEOUT 15)
add_test(NAME breeze_fdfs_test COMMAND breeze_fdfs_test)
set_tests_properties(breeze_fdfs_test PROPERTIES TIMEOUT 90)
add_test(NAME breeze_odb_test COMMAND breeze_odb_test)
set_tests_properties(breeze_odb_test PROPERTIES TIMEOUT 15)
add_test(NAME breeze_elastic_test COMMAND breeze_elastic_test)
set_tests_properties(breeze_elastic_test PROPERTIES TIMEOUT 60)

