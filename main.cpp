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

		auto iface_client1 = RTCApi::RtcFactory::CreateRtc(&test, "0.0.0.0", 9000);
		iface_client1->CreateRtpSenderStream(123123, "127.0.0.1", 7000, true);


		sleep(100000);

    return 0;
}
