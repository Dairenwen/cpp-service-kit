# 声明项目使用的 CMake 最低版本
cmake_minimum_required(VERSION 3.20)
project(BreezeKit VERSION 0.1.0 LANGUAGES CXX)

# 示例和测试按需构建；发布包只包含库和公开文件
option(BREEZE_BUILD_EXAMPLES "构建示例程序" OFF)
option(BREEZE_BUILD_TESTS "构建测试程序" OFF)
# 所有目标使用 C++17，且使用标准语法；CPP_KIT_BUILD_DIR 供示例和测试设置输出位置。
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CPP_KIT_BUILD_DIR "${CMAKE_BINARY_DIR}")

# GNUInstallDirs 提供 lib、include、share 等安装目录变量。
# CMakePackageConfigHelpers 用于生成可被其他项目 find_package 查找的配置。
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
add_subdirectory(source)

# 设置 -DBREEZE_BUILD_EXAMPLES=ON 时加入示例；编译中间文件保存在构建目录。
if(BREEZE_BUILD_EXAMPLES)
    add_subdirectory(example "${CMAKE_BINARY_DIR}/CMakeFiles/example_build")
endif()
# 设置 -DBREEZE_BUILD_TESTS=ON 时加入测试，并向 CTest 注册已有测试。
if(BREEZE_BUILD_TESTS)
    enable_testing()
    add_subdirectory(test "${CMAKE_BINARY_DIR}/CMakeFiles/test_build")
endif()

# 生成供 find_package 使用的配置和版本文件。配置文件相对于安装前缀放在 lib/cmake/BreezeKit（具体 lib 目录由平台决定）。
set(BREEZE_CMAKE_DIR "${CMAKE_INSTALL_LIBDIR}/cmake/BreezeKit")
# 将 .in 模板中的 @PACKAGE_INIT@ 展开，生成适合安装和移动的包配置。
configure_package_config_file(cmake/BreezeKitConfig.cmake.in
    "${CMAKE_CURRENT_BINARY_DIR}/BreezeKitConfig.cmake"
    INSTALL_DESTINATION "${BREEZE_CMAKE_DIR}")
# 写入版本检查文件：使用方指定版本时，以相同主版本作为兼容条件。
write_basic_package_version_file("${CMAKE_CURRENT_BINARY_DIR}/BreezeKitConfigVersion.cmake"
    VERSION "${PROJECT_VERSION}" COMPATIBILITY SameMajorVersion)
# 安装包配置、组件入口、依赖查找脚本，以及使用说明和许可证。
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/BreezeKitConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/BreezeKitConfigVersion.cmake"
    DESTINATION "${BREEZE_CMAKE_DIR}")
install(DIRECTORY cmake/dependencies cmake/components DESTINATION "${BREEZE_CMAKE_DIR}")
install(FILES README.md LICENSE DESTINATION "${CMAKE_INSTALL_DATADIR}/doc/BreezeKit")

# 根据上述安装规则生成 tar.gz 压缩包。TGZ 生成 .tar.gz；执行 cpack 时按 install 规则收集文件。
set(CPACK_GENERATOR TGZ)
set(CPACK_PACKAGE_NAME BreezeKit)
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
include(CPack)
