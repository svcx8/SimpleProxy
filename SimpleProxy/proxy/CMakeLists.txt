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

# 添加依赖
find_package(CURL REQUIRED)
message(STATUS "CURL: ${CURL_INCLUDE_DIR}")

find_package(OpenSSL REQUIRED)
message(STATUS "OpenSSL: ${OPENSSL_INCLUDE_DIR}")

find_library(z-lib z)
message(STATUS "ZLIB: ${z-lib}")

# 添加头文件
include_directories("${PROJECT_SOURCE_DIR}/${CMAKE_PROJECT_NAME}"
    "${PROJECT_SOURCE_DIR}/${CMAKE_PROJECT_NAME}/third/cpr/include"
    "${PROJECT_SOURCE_DIR}/${CMAKE_PROJECT_NAME}/third/rapidjson/include"
    ${OPENSSL_INCLUDE_DIR}
    ${CURL_INCLUDE_DIR}
)

# 将源代码添加到此项目的可执行文件。
add_executable (Proxy
    "main.cc"
    "${PROJECT_SOURCE_DIR}/${CMAKE_PROJECT_NAME}/dispatcher/epoller.cc"
    "${PROJECT_SOURCE_DIR}/${CMAKE_PROJECT_NAME}/server/server.cc"
    "socks5.cc"
    "proxy_client.cc"
    "proxy_server.cc"
    "proxy_socket.cc"
    "proxy_conn.cc"
    "memory_buffer.cc"
    "dns_resolver.cc"
    "${PROJECT_SOURCE_DIR}/${CMAKE_PROJECT_NAME}/misc/configuration.cc"

    "${PROJECT_SOURCE_DIR}/${CMAKE_PROJECT_NAME}/third/cpr/cpr/auth.cpp"
    "${PROJECT_SOURCE_DIR}/${CMAKE_PROJECT_NAME}/third/cpr/cpr/bearer.cpp"
    "${PROJECT_SOURCE_DIR}/${CMAKE_PROJECT_NAME}/third/cpr/cpr/cookies.cpp"
    "${PROJECT_SOURCE_DIR}/${CMAKE_PROJECT_NAME}/third/cpr/cpr/cprtypes.cpp"
    "${PROJECT_SOURCE_DIR}/${CMAKE_PROJECT_NAME}/third/cpr/cpr/curl_container.cpp"
    "${PROJECT_SOURCE_DIR}/${CMAKE_PROJECT_NAME}/third/cpr/cpr/curlholder.cpp"
    "${PROJECT_SOURCE_DIR}/${CMAKE_PROJECT_NAME}/third/cpr/cpr/error.cpp"
    "${PROJECT_SOURCE_DIR}/${CMAKE_PROJECT_NAME}/third/cpr/cpr/multipart.cpp"
    "${PROJECT_SOURCE_DIR}/${CMAKE_PROJECT_NAME}/third/cpr/cpr/parameters.cpp"
    "${PROJECT_SOURCE_DIR}/${CMAKE_PROJECT_NAME}/third/cpr/cpr/payload.cpp"
    "${PROJECT_SOURCE_DIR}/${CMAKE_PROJECT_NAME}/third/cpr/cpr/proxies.cpp"
    "${PROJECT_SOURCE_DIR}/${CMAKE_PROJECT_NAME}/third/cpr/cpr/proxyauth.cpp"
    "${PROJECT_SOURCE_DIR}/${CMAKE_PROJECT_NAME}/third/cpr/cpr/redirect.cpp"
    "${PROJECT_SOURCE_DIR}/${CMAKE_PROJECT_NAME}/third/cpr/cpr/response.cpp"
    "${PROJECT_SOURCE_DIR}/${CMAKE_PROJECT_NAME}/third/cpr/cpr/session.cpp"
    "${PROJECT_SOURCE_DIR}/${CMAKE_PROJECT_NAME}/third/cpr/cpr/timeout.cpp"
    "${PROJECT_SOURCE_DIR}/${CMAKE_PROJECT_NAME}/third/cpr/cpr/unix_socket.cpp"
    "${PROJECT_SOURCE_DIR}/${CMAKE_PROJECT_NAME}/third/cpr/cpr/util.cpp"
)

# 添加静态库
if(WIN32)
    find_library(ws2-lib ws2_32 HINTS ${CMAKE_C_IMPLICIT_LINK_DIRECTORIES})
    target_link_libraries(Proxy ${ws2-lib})
elseif(ANDROID)
    message(STATUS "Platform: ${ANDROID_PLATFORM}")
    target_link_libraries(Proxy ${CURL_LIBRARY} ${z-lib} ${OPENSSL_LIBRARIES})
elseif(UNIX)
    message(STATUS "Platform: Linux")
    find_package(Threads REQUIRED)
    target_link_libraries(Proxy Threads::Threads ${OPENSSL_LIBRARIES} ${CURL_LIBRARY} ${z-lib})
endif()
# TODO: 如有需要，请添加测试并安装目标。