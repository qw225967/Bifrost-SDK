cmake_minimum_required(VERSION 3.10)

project(cpp_websocket)

set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -std=c++14 -g -Wno-deprecated -Wno-deprecated-declarations -Wall -fexceptions -frtti -D__STDC_FORMAT_MACROS -fPIC")

add_definitions(-D ENABLE_OPEN_SSL)

include_directories(
    ${PROJECT_SOURCE_DIR}
    ${PROJECT_SOURCE_DIR}/net
    ${PROJECT_SOURCE_DIR}/net/http
    ${PROJECT_SOURCE_DIR}/net/tcp
    ${PROJECT_SOURCE_DIR}/utils
    /opt/homebrew/Cellar/openssl@3/3.4.0/include
)

add_library(${PROJECT_NAME} STATIC 
        ${PROJECT_SOURCE_DIR}/net/http/http_client.cpp
        ${PROJECT_SOURCE_DIR}/net/http/websocket/websocket_client.cpp
        ${PROJECT_SOURCE_DIR}/net/http/websocket/ws_session_base.cpp
        ${PROJECT_SOURCE_DIR}/net/http/websocket/websocket_frame.cpp
        ${PROJECT_SOURCE_DIR}/net/http/websocket/websocket_pub.cpp
        ${PROJECT_SOURCE_DIR}/utils/byte_crypto.cpp
        ${PROJECT_SOURCE_DIR}/utils/base64.cpp
)

# 查找名为 "mylib" 的库
find_library(SSL_LIBRARY
        NAMES ssl
        /opt/homebrew/Cellar/openssl@3/3.4.0/lib
        NO_DEFAULT_PATH)

target_link_libraries(
        ${PROJECT_NAME}
        /opt/homebrew/Cellar/openssl@3/3.4.0/lib/libssl.a
        /opt/homebrew/Cellar/openssl@3/3.4.0/lib/libcrypto.a
)