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

		auto iface = RTCApi::RtcFactory::CreateRtc(&test);
		iface->CreateRtpReceiverStream(123123);

		sleep(100000);

    return 0;
}
