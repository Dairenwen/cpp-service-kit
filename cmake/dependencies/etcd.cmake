# 在当前目录作用域内只加载一次，避免重复执行依赖查找。
include_guard()

# 查找 etcd 模块使用的第三方依赖。
include("${CMAKE_CURRENT_LIST_DIR}/library.cmake")
# 服务注册与发现复用日志、JSON 等工具，再查找 etcd 客户端及 Boost。
include("${CMAKE_CURRENT_LIST_DIR}/log.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/util.cmake")
find_package(etcd-cpp-api CONFIG REQUIRED)
find_package(Boost REQUIRED)
