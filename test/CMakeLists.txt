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

# 编译测试程序并输出到 build/test。
add_executable(breeze_rpc_test
    BreezeRPC/client.cpp
    BreezeRPC/server.cpp
    "${BREEZE_TEST_PROTO_DIR}/cal.pb.cc"
)
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

# 使用 CTest 运行封装测试。
enable_testing()
add_test(NAME breeze_rpc_test COMMAND breeze_rpc_test)
add_test(NAME breeze_trans_test COMMAND breeze_trans_test)
