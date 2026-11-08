# 这个辅助函数在同一次 CMake 配置中只定义一次。
include_guard(GLOBAL)

# 对没有 CMake 配置的第三方库，在使用方机器上查找头文件和库文件。
# 参数依次为目标简称、用于查找的头文件和库名，例如 redispp、sw/redis++/redis.h、redis++。
function(breeze_find_library name header library)
    # 多个组件使用同一依赖时复用已有目标，避免重复创建。
    if(TARGET BreezeDep::${name})
        return()
    endif()
    # 在当前机器查找头文件所在目录和库文件；REQUIRED 使缺失依赖时显示配置错误。
    find_path(BREEZE_${name}_INCLUDE_DIR NAMES "${header}" REQUIRED)
    find_library(BREEZE_${name}_LIBRARY NAMES "${library}" REQUIRED)
    # IMPORTED 表示使用现有库；GLOBAL 让其他目录也能使用该依赖目标。
    add_library(BreezeDep::${name} UNKNOWN IMPORTED GLOBAL)
    # 记录库的实际路径和公开头文件目录，链接目标时由 CMake 传递给使用方。
    set_target_properties(BreezeDep::${name} PROPERTIES
        IMPORTED_LOCATION "${BREEZE_${name}_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${BREEZE_${name}_INCLUDE_DIR}")
endfunction()
