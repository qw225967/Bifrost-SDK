# 定义一个库 target
add_library(CoreLib STATIC
        # 添加源文件
        # io
        src/io/network_thread.cpp
        src/io/udp_socket.cpp
        src/io/socket_interface.cpp
        src/io/packet_dispatcher_interface.cpp

        # rtc
        src/rtc/rtc_transport.cpp
        src/rtc/rtp_stream_receiver.cpp
        src/rtc/rtp_stream_sender.cpp
        src/rtc/rtp_packet.cpp
        src/rtc/rtp_stream.cpp
        src/rtc/nack_generator.cpp
        src/rtc/seq_manager.cpp
        src/rtc/data_calculator.cpp
        src/rtc/rtp_retransmission_buffer.cpp

        # rtcp
        src/rtc/rtcp/heart_beat_packet.cpp
        src/rtc/rtcp/rtcp_packet.cpp
        src/rtc/rtcp/feedback_rtp_item.cpp
        src/rtc/rtcp/nack_packet.cpp
        src/rtc/rtcp/feedback_packet.cpp
        src/rtc/rtcp/sender_report_packet.cpp
        src/rtc/rtcp/receiver_report_packet.cpp
        src/rtc/rtcp/compound_packet.cpp

        # utils
        src/utils/utils.cpp
        src/utils/copy_on_write_buffer.cc
        src/utils/timer_handler.cpp

        # 添加头文件
        #io
        include/rtc/rtcp/rtcp_packet.h
        include/rtc/rtc_common.h

        # rtcp
        include/rtc/rtcp/receiver_report_packet.h
        include/rtc/rtcp/sender_report_packet.h
        include/rtc/rtcp/feedback_packet.h
        include/rtc/rtcp/nack_packet.h
        include/rtc/rtcp/feedback_item.h
        include/rtc/rtcp/feedback_rtp_item.h
        include/rtc/rtc_transport.h
        include/rtc/rtp_stream_receiver.h
        include/rtc/rtp_stream_sender.h
        include/rtc/rtp_stream.h
        include/rtc/rtp_stream.h
        include/rtc/rtp_packet.h
        include/rtc/nack_generator.h
        include/utils/time_handler.h
        include/rtc/seq_manager.h
        include/rtc/data_calculator.h
        include/rtc/rtp_retransmission_buffer.h

        # utils
        include/utils/buffer.h
        include/utils/array_view.h
        include/utils/copy_on_write_buffer.h
        include/utils/ref_counted_base.h
        include/utils/ref_counted_object.h
        include/utils/ref_counter.h
        include/utils/scoped_refptr.h
        include/utils/type_traits.h
        include/rtc/rtcp/compound_packet.h
        include/io/net_io_factory.h
        src/io/net_io_factory.cpp
        include/io/tcp_buffer.h
        include/utils/cpp11_adaptor.h
        include/rtc/callback_queue_thread.h
        src/rtc/callback_queue_thread.cpp
)

# 设置库的属性
target_include_directories(CoreLib PUBLIC
        ${PROJECT_SOURCE_DIR}/core/include
        ${PROJECT_SOURCE_DIR}/deps/cpp_websocket
        ${PROJECT_SOURCE_DIR}/deps/cpp_websocket/net
        ${PROJECT_SOURCE_DIR}/deps/cpp_websocket/net/http
        ${PROJECT_SOURCE_DIR}/deps/cpp_websocket/net/tcp
        ${PROJECT_SOURCE_DIR}/deps/cpp_websocket/utils
)

# 定义库的接口
target_compile_definitions(CoreLib PUBLIC CORELIB_DEFINE)
target_link_libraries(CoreLib uv cpp_websocket)
