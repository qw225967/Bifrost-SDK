cmake_minimum_required(VERSION 3.29)
project(demo_test)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(
        core/include
        deps/spdlog/include
        deps/libuv/include
        deps/websocket/include
)

# 包含core子目录
add_subdirectory( core )
add_subdirectory( api )
add_subdirectory( deps/libuv )
add_subdirectory( deps/spdlog )
add_subdirectory( deps/websocket )

# 添加可执行文件
add_executable(demo_test main.cpp
        api/rtc_interface.h
        api/rtc_factory.h
        api/rtc_factory.cpp)

add_definitions(-D MS_LITTLE_ENDIAN)
add_definitions(-D SPDLOG_COMPILED_LIB)
add_definitions(-D SPDLOG_ACTIVE_LEVEL=0)
add_definitions(-D WEBRTC_LINUX)
add_definitions(-D WEBRTC_POSIX)
add_definitions("-g")

# 链接core库和第三方库到可执行文件
target_link_libraries(${PROJECT_NAME} ApiLib CoreLib uv spdlog WebsocketLib)

# 包含core的头文件目录和第三方库的头文件目录
target_include_directories(${PROJECT_NAME} PRIVATE
        ${PROJECT_SOURCE_DIR}/api
        ${PROJECT_SOURCE_DIR}/core/include
        ${PROJECT_SOURCE_DIR}/deps/libuv/include
        ${PROJECT_SOURCE_DIR}/deps/spdlog/include
        ${PROJECT_SOURCE_DIR}/deps/websocket/include
)
