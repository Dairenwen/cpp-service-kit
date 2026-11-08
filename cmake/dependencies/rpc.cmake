# 在当前目录作用域内只加载一次，避免重复执行依赖查找。
include_guard()

# 查找 rpc 模块使用的第三方依赖。
include("${CMAKE_CURRENT_LIST_DIR}/library.cmake")
# brpc 的依赖包括线程、gflags、Protobuf、OpenSSL 和 zlib。
find_package(Threads REQUIRED)
find_package(gflags CONFIG REQUIRED)
find_package(Protobuf 3.12 REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(ZLIB REQUIRED)
# brpc 和 LevelDB 使用辅助函数创建可链接的 BreezeDep:: 目标。
breeze_find_library(brpc brpc/server.h brpc)
breeze_find_library(leveldb leveldb/db.h leveldb)
