# 在当前目录作用域内只加载一次，避免重复执行依赖查找。
include_guard()

# 查找 elastic 模块使用的第三方依赖。
include("${CMAKE_CURRENT_LIST_DIR}/library.cmake")
# ES 模块复用日志和 JSON 工具，使用 elasticlient 与 CPR 发送请求。
include("${CMAKE_CURRENT_LIST_DIR}/log.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/util.cmake")
breeze_find_library(elasticlient elasticlient/client.h elasticlient)
breeze_find_library(cpr cpr/response.h cpr)
