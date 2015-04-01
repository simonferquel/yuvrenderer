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

	class SampleQueue {
	private:
		std::int32_t _maxFramesAhead;
		std::mutex _mutex;
		std::condition_variable _cv;
		std::condition_variable _cvProduce;
		std::queue<Windows::Media::Core::MediaStreamSample^> _samples;
	public:
		SampleQueue() : _maxFramesAhead(5) {}
		void SubmitSample(Windows::Media::Core::MediaStreamSample^ sample);
		Windows::Media::Core::MediaStreamSample^ GetSample();
	};

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
		H264LiveDecoder(YuvD3DRenderer^ renderer);
		void StartRenderingLoop();
		void StopRenderingLoop();
		void SubmitH264Frame(const Platform::Array<byte>^ data);
	};

}