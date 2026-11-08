# 在当前目录作用域内只加载一次，避免重复执行依赖查找。
include_guard()

# 查找 mq 模块使用的第三方依赖。
include("${CMAKE_CURRENT_LIST_DIR}/library.cmake")
# 消息队列复用公共日志；AMQP-CPP 提供协议，libev 提供事件循环。
include("${CMAKE_CURRENT_LIST_DIR}/log.cmake")
breeze_find_library(amqpcpp amqpcpp.h amqpcpp)
breeze_find_library(ev ev.h ev)
