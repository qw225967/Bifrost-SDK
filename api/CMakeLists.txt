# 定义一个库 target
add_library(ApiLib STATIC
        # 添加源文件
        rtc_factory.cpp
)

# 设置库的属性
target_include_directories(ApiLib PUBLIC
        ${PROJECT_SOURCE_DIR}/api
)

# 定义库的接口
target_compile_definitions(ApiLib PUBLIC APILIB_DEFINE)
target_link_libraries(ApiLib CoreLib)
