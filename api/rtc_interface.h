/*******************************************************
 * @author      : dog head
 * @date        : Created in 24-10-22 下午3:45
 * @mail        : fangruiqian.frq@alibaba-inc.com
 * @project     : TEMP-PROJECT-FLAG
 * @file        : rtc_interface.h
 * @description : TODO
 *******************************************************/

#ifndef RTC_INTERFACE_H
#define RTC_INTERFACE_H

#include <cstdint> // uint8_t, etc

namespace RTCApi {
		class RtcInterface {
		public:
				class DataCallBackObserver {
				public:
						virtual ~DataCallBackObserver() = default;
						/**
						 * @brief 音频数据回调
						 * @details 该函数携带完整一帧音频数据
						 * @return 无
						 */
						virtual void OnReceiveAudio(uint8_t* data, uint32_t len) = 0;
				};

		public:
				virtual ~RtcInterface() = default;

		public:
				// 1.流相关接口
				/**
				 * @brief 创建发送流
				 * @details 可以创建对应的文本流、音频流
				 * @return 返回创建是否成功
				 */
				virtual bool CreateRtpSenderStream(uint32_t ssrc) = 0;

				/**
				 * @brief 删除发送流
				 * @details 根据ssrc删除对应流
				 * @return 返回删除是否成功
				 */
				virtual bool DeleteRtpSenderStream(uint32_t ssrc) = 0;

				/**
				 * @brief 创建接收流
				 * @details 可以创建对应的文本流、音频流
				 * @return 返回删除是否成功
				 */
				virtual bool CreateRtpReceiverStream(uint32_t ssrc) = 0;

				/**
				 * @brief 删除接收流
				 * @details 根据ssrc删除对应流
				 * @return 返回创建是否成功
				 */
				virtual bool DeleteRtpReceiverStream(uint32_t ssrc) = 0;

				virtual void OnSendPacket(uint32_t ssrc, uint8_t* data, uint32_t len) = 0;
		};
} // namespace RTCInterface

#endif // RTC_INTERFACE_H
