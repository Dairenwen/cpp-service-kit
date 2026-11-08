# 在当前目录作用域内只加载一次，允许不同子目录分别引用组件。
include_guard()

# 先加载公共模块，再加载当前模块的依赖和静态库。
include("${CMAKE_CURRENT_LIST_DIR}/log.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/util.cmake")
# 在使用方机器上查找本组件的第三方头文件与库。
include("${CMAKE_CURRENT_LIST_DIR}/../dependencies/elastic.cmake")
# 导入安装时生成的目标，使用方可链接 BreezeKit::elastic。
include("${CMAKE_CURRENT_LIST_DIR}/../BreezeKit-elastic-targets.cmake")
