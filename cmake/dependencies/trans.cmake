# 在当前目录作用域内只加载一次，避免重复执行依赖查找。
include_guard()

# 查找 trans 模块使用的第三方依赖。
include("${CMAKE_CURRENT_LIST_DIR}/library.cmake")
# 转码模块复用日志和工具，FFmpeg 各库分别提供容器、编解码和基础工具。
include("${CMAKE_CURRENT_LIST_DIR}/log.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/util.cmake")
breeze_find_library(avformat libavformat/avformat.h avformat)
breeze_find_library(avcodec libavcodec/avcodec.h avcodec)
breeze_find_library(avutil libavutil/avutil.h avutil)
