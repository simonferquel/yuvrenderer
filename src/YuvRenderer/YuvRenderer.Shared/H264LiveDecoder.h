#pragma once
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include "YuvD3DRenderer.h"
#include <mutex>
#include <condition_variable>
#include <memory>
#include <queue>

namespace YuvRenderer {

	/** @brief represent a blocking producer / consumer queue for passing samples from the caller, to the decoding thread.
	*/
	class SampleQueue {
	private:
		/// @brief max number of frames pending decoding
		std::uint32_t _maxFramesAhead;
		std::mutex _mutex;
		std::condition_variable _cv;
		std::condition_variable _cvProduce;
		std::queue<Windows::Media::Core::MediaStreamSample^> _samples;
	public:
		SampleQueue() : _maxFramesAhead(5) {}
		/// @brief submit a media sample
		void SubmitSample(Windows::Media::Core::MediaStreamSample^ sample);
		/// @brief consume a media sample
		Windows::Media::Core::MediaStreamSample^ GetSample();
	};
	/// @brief H264 decoder accepting frames one by one
	[Windows::Foundation::Metadata::WebHostHiddenAttribute]
	public ref class H264LiveDecoder sealed
	{
	private:
		long _currentFrameIndex;
		std::shared_ptr<SampleQueue> _sampleQueue;
		uint32 _dxgiDeviceManageResetToken;
		Microsoft::WRL::ComPtr<IMFDXGIDeviceManager> _dxgiDeviceManager;
		Microsoft::WRL::ComPtr<IMFSourceReader> _mfSourceReader;
		Windows::Media::Core::MediaStreamSource^ _mediaStreamSource;
		YuvD3DRenderer^ _renderer;
	public:
		/// @brief create an instance of the H264 Live Decoder
		/// @param renderer renderer with the same size as the H264 video stream
		H264LiveDecoder(YuvD3DRenderer^ renderer);
		/// @brief start the rendering thread
		void StartRenderingLoop();
		/// @brief send the stop message to the the rendering thread
		void StopRenderingLoop();
		/// @brief submit a sample in form of a byte array.
		/// @param data is recopied in a Media Foundation buffer. So it can be an ArrayReference to a native temp buffer
		void SubmitH264Frame(const Platform::Array<byte>^ data);
	};

}