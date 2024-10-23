# 定义一个库 target
add_library(WebsocketLib STATIC
        src/ws_common.cpp
        src/ws_util.cpp
        src/ws_encode.cpp
        src/ws_sha1.cpp
        src/ws_membuf.cpp
        # 添加其他源文件
)

# 设置库的属性
target_include_directories(WebsocketLib PUBLIC
        ${PROJECT_SOURCE_DIR}/deps/websocket/include
)

# 定义库的接口
target_compile_definitions(WebsocketLib PUBLIC WEBSOCKETLIB_DEFINE)