# CMakeList.txt: Proxy 的 CMake 项目，在此处包括源代码并定义
# 项目特定的逻辑。
#
cmake_minimum_required (VERSION 3.8)

# 文件写出位置
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}\\BinOutput\\${CMAKE_BUILD_TYPE})
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++1z -fno-rtti -D_LIBCPP_DISABLE_VISIBILITY_ANNOTATIONS")

if(MSVC)
    add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")
endif()

# 添加头文件
include_directories("${PROJECT_SOURCE_DIR}/${CMAKE_PROJECT_NAME}")

# 将源代码添加到此项目的可执行文件。
add_executable (Proxy
    "main.cc"
    "receiver_pool.cc"
    "${PROJECT_SOURCE_DIR}/${CMAKE_PROJECT_NAME}/dispatcher/epoller.cc"
    "${PROJECT_SOURCE_DIR}/${CMAKE_PROJECT_NAME}/server/server.cc"
    "socks5.cc"
    "proxy_client.cc"
    "proxy_server.cc"
    "proxy_socket.cc"
    "proxy_conn.cc"
    "${PROJECT_SOURCE_DIR}/${CMAKE_PROJECT_NAME}/echo/echo_conn.cc"
    "${PROJECT_SOURCE_DIR}/${CMAKE_PROJECT_NAME}/echo/echo_server.cc"
)

# 添加静态库
if(WIN32)
    find_library(ws2-lib ws2_32 HINTS ${CMAKE_C_IMPLICIT_LINK_DIRECTORIES})
    target_link_libraries(Proxy ${ws2-lib})
endif()

if(ANDROID)
    find_library(log-lib log)
    target_link_libraries(Proxy ${log-lib})
endif()

if(UNIX)
    find_package(Threads REQUIRED)
    target_link_libraries(Proxy Threads::Threads)
endif()
# TODO: 如有需要，请添加测试并安装目标。