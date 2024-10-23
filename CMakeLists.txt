cmake_minimum_required(VERSION 3.10)
project(demo_test)

# 设置C++标准
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


# 设置库和头文件的输出目录
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/output/lib)
set(ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/output/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/output/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/output/lib)

# 设置头文件的输出目录
set(INCLUDE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/output/include)

include_directories(
        core/include
        deps/spdlog/include
        deps/libuv/include
)

# 包含core子目录
add_subdirectory( core )
add_subdirectory( api )
add_subdirectory( deps/libuv)
add_subdirectory( deps/spdlog )
add_subdirectory( deps/cpp_websocket )

# 添加可执行文件
add_executable(demo_test main.cpp)

add_definitions(-D MS_LITTLE_ENDIAN)
#add_definitions(-D MS_BIG_ENDIAN)
add_definitions(-D SPDLOG_COMPILED_LIB)
add_definitions(-D SPDLOG_ACTIVE_LEVEL=0)
add_definitions(-D WEBRTC_LINUX)
add_definitions(-D WEBRTC_POSIX)
add_definitions("-g")

## 链接core库和第三方库到可执行文件
target_link_libraries(${PROJECT_NAME} uv spdlog cpp_websocket CoreLib ApiLib)

# 包含core的头文件目录和第三方库的头文件目录
target_include_directories(${PROJECT_NAME} PRIVATE
        ${PROJECT_SOURCE_DIR}/api
        ${PROJECT_SOURCE_DIR}/core/include
        ${PROJECT_SOURCE_DIR}/deps/libuv/include
        ${PROJECT_SOURCE_DIR}/deps/spdlog/include
        ${PROJECT_SOURCE_DIR}/deps/cpp_websocket
        ${PROJECT_SOURCE_DIR}/deps/cpp_websocket/net
        ${PROJECT_SOURCE_DIR}/deps/cpp_websocket/net/http
        ${PROJECT_SOURCE_DIR}/deps/cpp_websocket/net/tcp
        ${PROJECT_SOURCE_DIR}/deps/cpp_websocket/utils
)

# 复制头文件到指定目录
file(COPY "core/include" DESTINATION ${INCLUDE_OUTPUT_PATH}/core)

file(GLOB_RECURSE API_HEADERS "api/*.h")
foreach(HEADER ${API_HEADERS})
    file(COPY ${HEADER} DESTINATION ${INCLUDE_OUTPUT_PATH}/api)
endforeach()

# 复制第三方库头文件
file(COPY "deps/spdlog/include/spdlog" DESTINATION ${INCLUDE_OUTPUT_PATH}/deps/spdlog)
file(COPY "deps/libuv/include" DESTINATION ${INCLUDE_OUTPUT_PATH}/deps/libuv)
