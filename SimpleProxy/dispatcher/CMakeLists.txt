# CMakeList.txt: Dispatcher 的 CMake 项目，在此处包括源代码并定义
# 项目特定的逻辑。
#
cmake_minimum_required (VERSION 3.8)

# 文件写出位置
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}\\BinOutput\\${CMAKE_BUILD_TYPE})
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -fno-rtti -fno-exceptions -D_LIBCPP_DISABLE_VISIBILITY_ANNOTATIONS")

# 添加依赖
set(CMAKE_PREFIX_PATH "${CMAKE_PREFIX_PATH}/${CMAKE_LIBRARY_ARCHITECTURE}")
message(STATUS CMAKE_PREFIX_PATH: ${CMAKE_PREFIX_PATH})

# 添加头文件
include_directories(
    "${PROJECT_SOURCE_DIR}/${CMAKE_PROJECT_NAME}"
)

# 将源代码添加到此项目的可执行文件。
aux_source_directory(. program_src)

add_library (
    Dispatcher
    STATIC
    ${program_src}
)

# 添加静态库
find_package(absl REQUIRED)
target_link_libraries(Dispatcher absl::status absl::statusor)

# TODO: 如有需要，请添加测试并安装目标。