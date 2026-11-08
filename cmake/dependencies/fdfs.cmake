# 在当前目录作用域内只加载一次，避免重复执行依赖查找。
include_guard()

# 查找 fdfs 模块使用的第三方依赖。
include("${CMAKE_CURRENT_LIST_DIR}/library.cmake")
# FastDFS 客户端依赖 fdfsclient 和 fastcommon，并使用项目公共日志。
include("${CMAKE_CURRENT_LIST_DIR}/log.cmake")
breeze_find_library(fdfsclient fastdfs/fdfs_client.h fdfsclient)
breeze_find_library(fastcommon fastcommon/logger.h fastcommon)
