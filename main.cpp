#include <iostream>
#include <string>
#include <unistd.h>
#include <vector>

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
		void OnReceiveAudio(uint8_t* data, uint32_t len) override {
				SPDLOG_INFO("receive data:");
		};

		void OnReceiveEvent() override{

		}
};

int main() {
		SPDLOG_INFO("main Start ...");

		RtcTest test;

		auto iface_client1 = RTCApi::RtcFactory::CreateRtc(&test, "0.0.0.0", 9000);
		iface_client1->CreateRtpReceiverStream(333222111, "127.0.0.1", 7000, true);

		sleep(1);


		auto* buf
		    = "{\"header\":{},\"payload\":{\"preload\":true,\"bizType\":\"openDial\",\"speakerId\":\"225\",\"text\":\"以前有一对年轻男女骑车撞到村里的电线杆，当场毙命。一天，5岁的小志和妈妈经过那儿。小志突然：“妈妈，电线杆上有两个人。”妈妈牵着他的手快速走开说：“小孩子不要乱说！”这件事很快就传开了，后来，记者来采访小志，记者问：“在哪”。小志指指上头，记者抬头一看，电线杆上挂着个牌子，上写：交通安全，人人有责。 .\", \"isSlot\": 0, \"textType\":0}}";
		auto* send_buf = (uint8_t*)buf;
		auto send_len  = strlen(buf);

		for (int i=0;i<200;i++) {
				iface_client1->OnSendText(send_buf, send_len);

				sleep(300);
		}

//		iface_client1.reset();

		// SPDLOG_INFO(RTCUtils::Byte::BytesToHex(send_buf, send_len));


		sleep(100000);

		return 0;
}
