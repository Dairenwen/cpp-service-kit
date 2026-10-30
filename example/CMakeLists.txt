cmake_minimum_required(VERSION 3.20)

# 将最终产物放到项目的 build 目录
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    project(cpp_service_kit_example LANGUAGES CXX)
    set(CPP_KIT_BUILD_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../build")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Threads REQUIRED)
find_package(gflags CONFIG REQUIRED)
find_package(GTest REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(jsoncpp CONFIG REQUIRED)
find_package(Protobuf 3.12 REQUIRED)
find_path(HTTPLIB_INCLUDE_DIR httplib.h REQUIRED)
find_path(BRPC_INCLUDE_DIR brpc/server.h REQUIRED)
find_library(BRPC_LIBRARY NAMES brpc REQUIRED)
find_library(LEVELDB_LIBRARY NAMES leveldb REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(ZLIB REQUIRED)
find_package(Boost REQUIRED COMPONENTS system)
find_path(WEBSOCKETPP_INCLUDE_DIR websocketpp/server.hpp REQUIRED)

# 示例程序和库输出到 build/example
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CPP_KIT_BUILD_DIR}/example")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CPP_KIT_BUILD_DIR}/example")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CPP_KIT_BUILD_DIR}/example")

# ---------- GFlags：命令行参数和配置文件 ----------

# 编译 gflags/main.cc，生成 gflags_example。
add_executable(gflags_example gflags/main.cc)
target_link_libraries(gflags_example PRIVATE gflags)

# 将参数配置文件复制到可执行文件所在目录。
configure_file(gflags/main.conf "${CPP_KIT_BUILD_DIR}/example/main.conf" COPYONLY)


# ---------- GTest：四个独立的测试程序 ----------

# 基础断言：检查绝对值函数。
add_executable(gtest_simple gtest/simple/main.cc)
target_link_libraries(gtest_simple PRIVATE GTest::gtest Threads::Threads)

# 全局环境：在全部测试前准备数据，全部测试后清理数据。
add_executable(gtest_global gtest/global/main.cc)
target_link_libraries(gtest_global PRIVATE GTest::gtest Threads::Threads)

# 测试套件：演示整组测试的初始化与清理。
add_executable(gtest_suite gtest/suite/main.cc)
target_link_libraries(gtest_suite PRIVATE GTest::gtest Threads::Threads)

# 测试用例：为每个用例分别准备和清理数据。
add_executable(gtest_case gtest/case/main.cc)
target_link_libraries(gtest_case PRIVATE GTest::gtest Threads::Threads)

# ---------- Spdlog：两种日志输出演示 ----------

# 多输出端日志：分别设置控制台和文件的日志等级。
add_executable(spdlog_multi_sink spdlog/multi_sink/main.cc)
target_link_libraries(spdlog_multi_sink PRIVATE spdlog::spdlog Threads::Threads)

# 异步日志：通过后台线程写入日志文件。
add_executable(spdlog_async spdlog/async/main.cc)
target_link_libraries(spdlog_async PRIVATE spdlog::spdlog Threads::Threads)


# ---------- JsonCpp：序列化和反序列化 ----------

# 将 JSON 对象转换为字符串。
add_executable(jsoncpp_serialize jsoncpp/serialize/main.cc)
target_link_libraries(jsoncpp_serialize PRIVATE JsonCpp::JsonCpp)

# 将 JSON 字符串解析为对象。
add_executable(jsoncpp_unserialize jsoncpp/unserialize/main.cc)
target_link_libraries(jsoncpp_unserialize PRIVATE JsonCpp::JsonCpp)


# ---------- Protobuf：二进制序列化演示 ----------

# 保存这份 person.proto 生成的 C++ 文件。
set(PROTOBUF_SERIALIZE_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated/protobuf_serialize")
file(MAKE_DIRECTORY "${PROTOBUF_SERIALIZE_DIR}")

# 将序列化示例的 person.proto 转换为 person.pb.cc 和 person.pb.h。
# 协议文件更新后，构建时会重新生成对应代码。
add_custom_command(
    OUTPUT
        "${PROTOBUF_SERIALIZE_DIR}/person.pb.cc"
        "${PROTOBUF_SERIALIZE_DIR}/person.pb.h"
    COMMAND protobuf::protoc
        "--cpp_out=${PROTOBUF_SERIALIZE_DIR}"
        "--proto_path=${CMAKE_CURRENT_SOURCE_DIR}/protobuf/serialize"
        "${CMAKE_CURRENT_SOURCE_DIR}/protobuf/serialize/person.proto"
    DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/protobuf/serialize/person.proto"
        protobuf::protoc
    VERBATIM
)

# 将演示代码和协议生成代码一起编译，生成 protobuf_serialize。
add_executable(protobuf_serialize
    protobuf/serialize/main.cc
    "${PROTOBUF_SERIALIZE_DIR}/person.pb.cc"
)
target_include_directories(protobuf_serialize PRIVATE "${PROTOBUF_SERIALIZE_DIR}")
target_link_libraries(protobuf_serialize PRIVATE protobuf::libprotobuf)


# ---------- Protobuf：JSON 双向转换演示 ----------

# 为 JSON 示例单独保存生成代码，区分两份同名协议。
set(PROTOBUF_JSON_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated/protobuf_json")
file(MAKE_DIRECTORY "${PROTOBUF_JSON_DIR}")

# 将 JSON 示例的 person.proto 转换为 C++ 文件。
add_custom_command(
    OUTPUT
        "${PROTOBUF_JSON_DIR}/person.pb.cc"
        "${PROTOBUF_JSON_DIR}/person.pb.h"
    COMMAND protobuf::protoc
        --experimental_allow_proto3_optional
        "--cpp_out=${PROTOBUF_JSON_DIR}"
        "--proto_path=${CMAKE_CURRENT_SOURCE_DIR}/protobuf/json"
        "${CMAKE_CURRENT_SOURCE_DIR}/protobuf/json/person.proto"
    DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/protobuf/json/person.proto"
        protobuf::protoc
    VERBATIM
)

# 将 JSON 转换演示和协议生成代码一起编译，生成 protobuf_json。
add_executable(protobuf_json
    protobuf/json/main.cc
    "${PROTOBUF_JSON_DIR}/person.pb.cc"
)
target_include_directories(protobuf_json PRIVATE "${PROTOBUF_JSON_DIR}")
target_link_libraries(protobuf_json PRIVATE protobuf::libprotobuf)


# ---------- HTTP：普通服务、静态服务、客户端 ----------

# 普通 HTTP 服务：处理 /hi 和 /numbers/数字。
add_executable(httplib_server httplib/server/main.cc)
target_include_directories(httplib_server PRIVATE "${HTTPLIB_INCLUDE_DIR}")
target_link_libraries(httplib_server PRIVATE Threads::Threads)

# 静态 HTTP 服务：将 webroot 目录中的页面返回给客户端。
add_executable(httplib_static httplib/static/main.cc)
target_include_directories(httplib_static PRIVATE "${HTTPLIB_INCLUDE_DIR}")
target_link_libraries(httplib_static PRIVATE Threads::Threads)

# HTTP 客户端：向本机的普通 HTTP 服务发送请求。
add_executable(httplib_client httplib/client/main.cc)
target_include_directories(httplib_client PRIVATE "${HTTPLIB_INCLUDE_DIR}")
target_link_libraries(httplib_client PRIVATE Threads::Threads)

# 将静态页面复制到 build/example/webroot。
configure_file(
    httplib/static/webroot/index.html
    "${CPP_KIT_BUILD_DIR}/example/webroot/index.html"
    COPYONLY
)

# ---------- WebSocketpp：HTTP 欢迎页面和 WebSocket 消息回显 ----------

# 编译 websocketpp/main.cc，生成 build/example/websocketpp_server。
add_executable(websocketpp_server websocketpp/main.cc)
target_include_directories(websocketpp_server PRIVATE "${WEBSOCKETPP_INCLUDE_DIR}")
target_link_libraries(websocketpp_server PRIVATE Boost::system Threads::Threads)

# 将浏览器客户端页面复制到 build/example，供浏览器直接打开。
configure_file(
    websocketpp/client.html
    "${CPP_KIT_BUILD_DIR}/example/websocketpp_client.html"
    COPYONLY
)

# ---------- brpc：加法服务端和客户端 ----------


# 将 cal.proto 生成的 C++ 文件保存到 build/generated/brpc。
set(BRPC_PROTO_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated/brpc")
file(MAKE_DIRECTORY "${BRPC_PROTO_DIR}")
add_custom_command(
    OUTPUT
        "${BRPC_PROTO_DIR}/cal.pb.cc"
        "${BRPC_PROTO_DIR}/cal.pb.h"
    COMMAND protobuf::protoc
        --experimental_allow_proto3_optional
        "--cpp_out=${BRPC_PROTO_DIR}"
        "--proto_path=${CMAKE_CURRENT_SOURCE_DIR}/brpc"
        "${CMAKE_CURRENT_SOURCE_DIR}/brpc/cal.proto"
    DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/brpc/cal.proto"
        protobuf::protoc
    VERBATIM
)

# 协议代码编译一次，供服务端和客户端共同使用
add_library(brpc_proto STATIC "${BRPC_PROTO_DIR}/cal.pb.cc")
target_include_directories(brpc_proto PUBLIC "${BRPC_PROTO_DIR}")
target_link_libraries(brpc_proto PUBLIC protobuf::libprotobuf)

# 编译 brpc/server.cpp，生成 build/example/brpc_server
add_executable(brpc_server brpc/server.cpp)
target_include_directories(brpc_server PRIVATE "${BRPC_INCLUDE_DIR}")
target_link_libraries(brpc_server PRIVATE
    brpc_proto "${BRPC_LIBRARY}" protobuf::libprotobuf gflags
    "${LEVELDB_LIBRARY}" OpenSSL::SSL OpenSSL::Crypto ZLIB::ZLIB
    Threads::Threads ${CMAKE_DL_LIBS}
)

# 编译 brpc/client.cpp，生成 build/example/brpc_client
add_executable(brpc_client brpc/client.cpp)
target_include_directories(brpc_client PRIVATE "${BRPC_INCLUDE_DIR}")
target_link_libraries(brpc_client PRIVATE
    brpc_proto "${BRPC_LIBRARY}" protobuf::libprotobuf gflags
    "${LEVELDB_LIBRARY}" OpenSSL::SSL OpenSSL::Crypto ZLIB::ZLIB
    Threads::Threads ${CMAKE_DL_LIBS}
)

# 编译 brpc/async.cpp，生成 build/example/brpc_async
add_executable(brpc_async brpc/async.cpp)
target_include_directories(brpc_async PRIVATE "${BRPC_INCLUDE_DIR}")
target_link_libraries(brpc_async PRIVATE
    brpc_proto "${BRPC_LIBRARY}" protobuf::libprotobuf gflags
    "${LEVELDB_LIBRARY}" OpenSSL::SSL OpenSSL::Crypto ZLIB::ZLIB
    Threads::Threads ${CMAKE_DL_LIBS}
)

# 编译 brpc/http_server.cpp，生成 build/example/brpc_http_server
add_executable(brpc_http_server brpc/http_server.cpp)
target_include_directories(brpc_http_server PRIVATE "${BRPC_INCLUDE_DIR}")
target_link_libraries(brpc_http_server PRIVATE
    brpc_proto "${BRPC_LIBRARY}" protobuf::libprotobuf gflags
    "${LEVELDB_LIBRARY}" OpenSSL::SSL OpenSSL::Crypto ZLIB::ZLIB
    Threads::Threads ${CMAKE_DL_LIBS}
)

# 编译 brpc/http_client.cpp，生成 build/example/brpc_http_client
add_executable(brpc_http_client brpc/http_client.cpp)
target_include_directories(brpc_http_client PRIVATE "${BRPC_INCLUDE_DIR}")
target_link_libraries(brpc_http_client PRIVATE
    brpc_proto "${BRPC_LIBRARY}" protobuf::libprotobuf gflags
    "${LEVELDB_LIBRARY}" OpenSSL::SSL OpenSSL::Crypto ZLIB::ZLIB
    Threads::Threads ${CMAKE_DL_LIBS}
)

# ---------- FFmpeg：将视频封装为 HLS 点播文件 ----------

# 编译 ffmpeg/hls.cpp，生成 build/example/ffmpeg_hls。
find_path(AVFORMAT_INCLUDE_DIR libavformat/avformat.h REQUIRED)
find_library(AVFORMAT_LIBRARY NAMES avformat REQUIRED)
find_library(AVCODEC_LIBRARY NAMES avcodec REQUIRED)
find_library(AVUTIL_LIBRARY NAMES avutil REQUIRED)
add_executable(ffmpeg_hls ffmpeg/hls.cpp)
target_include_directories(ffmpeg_hls PRIVATE "${AVFORMAT_INCLUDE_DIR}")
target_link_libraries(ffmpeg_hls PRIVATE
    "${AVFORMAT_LIBRARY}" "${AVCODEC_LIBRARY}" "${AVUTIL_LIBRARY}"
)

