# 在当前目录作用域内只加载一次，避免重复执行依赖查找。
include_guard()

# 查找 log 模块使用的第三方依赖。
include("${CMAKE_CURRENT_LIST_DIR}/library.cmake")
# Threads::Threads 提供线程链接选项；spdlog::spdlog 提供日志接口和实现。
find_package(Threads REQUIRED)
find_package(spdlog CONFIG REQUIRED)
