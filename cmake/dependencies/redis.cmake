# 在当前目录作用域内只加载一次，避免重复执行依赖查找。
include_guard()

# 查找 redis 模块使用的第三方依赖。
include("${CMAKE_CURRENT_LIST_DIR}/library.cmake")
# redis-plus-plus 使用 hiredis 通信，连接池使用线程支持。
find_package(Threads REQUIRED)
breeze_find_library(redispp sw/redis++/redis.h redis++)
breeze_find_library(hiredis hiredis/hiredis.h hiredis)
