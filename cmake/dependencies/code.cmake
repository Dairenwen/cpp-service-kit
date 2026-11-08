# 在当前目录作用域内只加载一次，避免重复执行依赖查找。
include_guard()

# 查找 code 模块使用的第三方依赖。
include("${CMAKE_CURRENT_LIST_DIR}/library.cmake")
# 邮件模块复用日志，通过 libcurl 的 SMTP 和 MIME 功能发送邮件。
include("${CMAKE_CURRENT_LIST_DIR}/log.cmake")
find_package(CURL REQUIRED)
