# CMakeList.txt: Proxy 的 CMake 项目，在此处包括源代码并定义
# 项目特定的逻辑。
#
cmake_minimum_required (VERSION 3.8)

# 文件写出位置
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}\\BinOutput\\${CMAKE_BUILD_TYPE})
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -fno-rtti -fno-exceptions -D_LIBCPP_DISABLE_VISIBILITY_ANNOTATIONS -msse")

# 添加依赖
set(CMAKE_PREFIX_PATH "${CMAKE_PREFIX_PATH}/${CMAKE_LIBRARY_ARCHITECTURE}")
message(STATUS CMAKE_PREFIX_PATH: ${CMAKE_PREFIX_PATH})

find_package(CURL REQUIRED)
message(STATUS "CURL: ${CURL_INCLUDE_DIR}")

find_package(OpenSSL REQUIRED)
message(STATUS "OpenSSL: ${OPENSSL_INCLUDE_DIR}")

find_library(z-lib z)
message(STATUS "ZLIB: ${z-lib}")

find_package(absl REQUIRED)

# 添加头文件
include_directories("${PROJECT_SOURCE_DIR}/${CMAKE_PROJECT_NAME}"
    "${PROJECT_SOURCE_DIR}/${CMAKE_PROJECT_NAME}/third/cpr/include"
    "${PROJECT_SOURCE_DIR}/${CMAKE_PROJECT_NAME}/third/rapidjson/include"
    ${OPENSSL_INCLUDE_DIR}
    ${CURL_INCLUDE_DIR}
)

# 将源代码添加到此项目的可执行文件。
aux_source_directory(. program_src)
aux_source_directory(${PROJECT_SOURCE_DIR}/${CMAKE_PROJECT_NAME}/server server_src)
aux_source_directory(${PROJECT_SOURCE_DIR}/${CMAKE_PROJECT_NAME}/limiter limiter_src)
aux_source_directory(${PROJECT_SOURCE_DIR}/${CMAKE_PROJECT_NAME}/misc misc_src)
aux_source_directory(${PROJECT_SOURCE_DIR}/${CMAKE_PROJECT_NAME}/third/cpr/cpr/ cpr_src)

add_executable (Proxy
    ${program_src}
    ${server_src}
    ${limiter_src}
    ${misc_src}
    ${cpr_src}
)

# 添加静态库
if(ANDROID)
    message(STATUS "Platform: ${ANDROID_PLATFORM}")
    target_link_libraries(Proxy Dispatcher ${OPENSSL_LIBRARIES} ${CURL_LIBRARY} ${z-lib} absl::status absl::statusor)
elseif(UNIX)
    message(STATUS "Platform: Linux")
    find_package(Threads REQUIRED)
    target_link_libraries(Proxy Threads::Threads ${OPENSSL_LIBRARIES} ${CURL_LIBRARY} ${z-lib})
endif()
# TODO: 如有需要，请添加测试并安装目标。