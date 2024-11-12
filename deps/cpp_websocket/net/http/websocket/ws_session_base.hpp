#ifndef WS_SESSION_BASE_HPP
#define WS_SESSION_BASE_HPP
#include "websocket_pub.hpp"
#include "websocket_frame.hpp"
#include "utils/data_buffer.hpp"
#include <stdint.h>
#include <string>
#include <vector>

namespace cpp_streamer
{
class WebSocketSessionBase
{
public:
    WebSocketSessionBase();
    virtual ~WebSocketSessionBase();

public:
    void AsyncWriteText(const std::string& text);
    void AsyncWriteData(const uint8_t* data, size_t len);

protected:
    int HandleFrame(DataBuffer& data);
    void SendClose(uint16_t code, const char *reason);
    void SendPingFrame(int64_t now_ms);

protected:
    virtual void HandleWsData(uint8_t* data, size_t len, int op_code) = 0;
    virtual void SendWsFrame(const uint8_t* data, size_t len, uint8_t op_code) = 0;
    virtual void HandleWsClose(uint8_t* data, size_t len) = 0;

private:
    void HandleWsPing();

protected:
    std::unique_ptr<WebSocketFrame> frame_;
    std::vector<std::shared_ptr<DataBuffer>> recv_buffer_vec_;
    int last_op_code_           = 1;
    int die_count_              = 0;
    int64_t last_recv_pong_ms_  = -1;
    int64_t last_send_ping_ms_  = -1;
    bool is_connected_          = false;
    bool close_                 = false;
};
}

#endif