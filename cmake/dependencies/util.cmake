# 在当前目录作用域内只加载一次，避免重复执行依赖查找。
include_guard()

# 查找 util 模块使用的第三方依赖。
include("${CMAKE_CURRENT_LIST_DIR}/library.cmake")
# 工具实现使用公共日志；加载日志依赖后再查找 JSON 和 UUID 使用的 Boost。
include("${CMAKE_CURRENT_LIST_DIR}/log.cmake")
find_package(jsoncpp CONFIG REQUIRED)
find_package(Boost REQUIRED)
