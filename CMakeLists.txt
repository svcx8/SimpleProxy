# CMakeList.txt: 顶层 CMake 项目文件，在此处执行全局配置
# 并包含子项目。
#
cmake_minimum_required (VERSION 3.8)

project ("SimpleProxy")

# 包含子项目。
add_subdirectory ("SimpleProxy/echo")
add_subdirectory ("SimpleProxy/dispatcher")
add_subdirectory ("SimpleProxy/proxy")
add_subdirectory ("SimpleProxy/_test")