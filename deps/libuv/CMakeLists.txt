# 定义一个库 target
add_library(uv STATIC
        src/fs-poll.c
        src/idna.c
        src/inet.c
        src/random.c
        src/strscpy.c
        src/strtok.c
        src/thread-common.c
        src/threadpool.c
        src/timer.c
        src/uv-common.c
        src/uv-data-getter-setters.c
        src/version.c
        src/unix/darwin.c
        src/unix/async.c
        src/unix/bsd-ifaddrs.c
        src/unix/core.c
        src/unix/darwin-proctitle.c
        src/unix/dl.c
        src/unix/fs.c
        src/unix/fsevents.c
        src/unix/getaddrinfo.c
        src/unix/getnameinfo.c
        src/unix/kqueue.c
        src/unix/loop-watcher.c
        src/unix/loop.c
        src/unix/pipe.c
        src/unix/poll.c
        src/unix/process.c
        src/unix/proctitle.c
        src/unix/random-devurandom.c
        src/unix/random-getentropy.c
        src/unix/signal.c
        src/unix/stream.c
        src/unix/sysinfo-loadavg.c
        src/unix/tcp.c
        src/unix/thread.c
        src/unix/tty.c
        src/unix/udp.c
        # 添加其他源文件
)

# 设置库的属性
target_include_directories(uv PUBLIC
        ${PROJECT_SOURCE_DIR}/deps/libuv/include
        ${PROJECT_SOURCE_DIR}/deps/libuv/src
)

# 定义库的接口
target_compile_definitions(uv PUBLIC UV_DEFINE)