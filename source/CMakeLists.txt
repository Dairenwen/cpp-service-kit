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
