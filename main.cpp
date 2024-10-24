#include <unistd.h>

#include <iostream>

#include "api/rtc_factory.h"
#include "spdlog/spdlog.h"

class RtcTest : public RTCApi::RtcInterface::DataCallBackObserver {
public:
		~RtcTest() override = default;
		/**
		 * @brief 音频数据回调
		 * @details 该函数携带完整一帧音频数据
		 * @return 无
		 */
		void OnReceiveAudio(uint8_t* data, uint32_t len) override{};
};

int main() {
		SPDLOG_INFO("main Start ...");

		RtcTest test;

		auto iface_client = RTCApi::RtcFactory::CreateRtc(&test, "0.0.0.0", 9000);
		iface_client->CreateRtpReceiverStream(123123, "127.0.0.1", 9000);
		iface_client->CreateRtpSenderStream(123123, "101.42.42.53", 9001);

		char* data = "hello world";

		for (int i = 0; i < 100000; i++) {
				iface_client->OnSendAudio(123123, reinterpret_cast<uint8_t*>(data),
				                   strlen(data));
				SPDLOG_INFO("send packet");

				usleep(100 * 1000);
	}

		sleep(100000);

    return 0;
}
