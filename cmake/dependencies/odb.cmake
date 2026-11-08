# 在当前目录作用域内只加载一次，避免重复执行依赖查找。
include_guard()

# 查找 odb 模块使用的第三方依赖。
include("${CMAKE_CURRENT_LIST_DIR}/library.cmake")
# ODB 核心库提供数据库接口，odb-mysql 提供 MySQL 驱动。
find_package(Threads REQUIRED)
breeze_find_library(odb odb/database.hxx odb)
breeze_find_library(odbmysql odb/mysql/database.hxx odb-mysql)
