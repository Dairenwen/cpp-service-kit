cmake_minimum_required(VERSION 3.20)
project(cpp_service_kit LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 将三个目录的构建结果保存在同一个 build 目录。
set(CPP_KIT_BUILD_DIR "${CMAKE_BINARY_DIR}")
enable_testing()

# 顶层构建的中间文件放在 CMakeFiles，三个目录可独立使用 build 下的同名目录。
add_subdirectory(example "${CMAKE_BINARY_DIR}/CMakeFiles/example_build")
add_subdirectory(source "${CMAKE_BINARY_DIR}/CMakeFiles/source_build")
add_subdirectory(test "${CMAKE_BINARY_DIR}/CMakeFiles/test_build")
