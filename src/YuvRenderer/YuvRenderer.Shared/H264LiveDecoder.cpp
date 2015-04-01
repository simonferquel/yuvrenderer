#include "pch.h"
#include "H264LiveDecoder.h"
#include <robuffer.h>

using namespace YuvRenderer;
using namespace Microsoft::WRL;
using namespace Windows::Media::Core;
using namespace Windows::Media::MediaProperties;
using namespace Windows::Foundation;

YuvRenderer::H264LiveDecoder::H264LiveDecoder(YuvD3DRenderer ^ renderer) :
	_renderer(renderer),
	_sampleQueue(std::make_shared<SampleQueue>()),
	_currentFrameIndex(0)
{
	static std::once_flag once;
	std::call_once(once, []() {
		CoInitializeEx(nullptr, 0);
		MFStartup(MF_VERSION, 0);
	});
	auto hr = MFCreateDXGIDeviceManager(&_dxgiDeviceManageResetToken, &_dxgiDeviceManager);
	if (FAILED(hr)) {
		throw Platform::Exception::CreateException(hr);
	};
	ComPtr<IUnknown> deviceUnkown;
	renderer->getNV12RendringDevice().As(&deviceUnkown);
	hr = _dxgiDeviceManager->ResetDevice(deviceUnkown.Get(), _dxgiDeviceManageResetToken);
	if (FAILED(hr)) {
		throw Platform::Exception::CreateException(hr);
	};
	auto h264Properties = VideoEncodingProperties::CreateH264();
	h264Properties->Width = renderer->SrcWidth;
	h264Properties->Height = renderer->SrcHeight;
	_mediaStreamSource = ref new MediaStreamSource(ref new VideoStreamDescriptor(h264Properties));
	_mediaStreamSource->Starting += ref new TypedEventHandler<MediaStreamSource^, MediaStreamSourceStartingEventArgs^>([](MediaStreamSource^ source, MediaStreamSourceStartingEventArgs^ args) {
	});
	_mediaStreamSource->Closed += ref new TypedEventHandler<MediaStreamSource^, MediaStreamSourceClosedEventArgs^>([](MediaStreamSource^ source, MediaStreamSourceClosedEventArgs^ args) {
	});
	auto sampleQueue = _sampleQueue;
	_mediaStreamSource->SampleRequested += ref new TypedEventHandler<MediaStreamSource^, MediaStreamSourceSampleRequestedEventArgs^ >([sampleQueue](MediaStreamSource^ source, MediaStreamSourceSampleRequestedEventArgs^ args) {
		args->Request->Sample = sampleQueue->GetSample();
	});

	ComPtr<IMFAttributes> sourceReaderAttributes;
	hr = MFCreateAttributes(&sourceReaderAttributes, 3);
	if (FAILED(hr)) {
		throw Platform::Exception::CreateException(hr);
	};
	hr = sourceReaderAttributes->SetUINT32(MF_LOW_LATENCY, TRUE);
	if (FAILED(hr)) {
		throw Platform::Exception::CreateException(hr);
	};
	hr = sourceReaderAttributes->SetUINT32(MF_READWRITE_ENABLE_HARDWARE_TRANSFORMS, TRUE);
	if (FAILED(hr)) {
		throw Platform::Exception::CreateException(hr);
	};
	ComPtr<IUnknown> dxgiManagerUnk;
	_dxgiDeviceManager.As(&dxgiManagerUnk);
	hr = sourceReaderAttributes->SetUnknown(MF_SOURCE_READER_D3D_MANAGER, dxgiManagerUnk.Get());
	if (FAILED(hr)) {
		throw Platform::Exception::CreateException(hr);
	};
	ComPtr<IMFGetService> mfGetService;
	hr = reinterpret_cast<IInspectable*>(_mediaStreamSource)->QueryInterface<IMFGetService>(&mfGetService);
	if (FAILED(hr)) {
		throw Platform::Exception::CreateException(hr);
	};
	ComPtr<IMFMediaSource> mediaSource;
	hr = mfGetService->GetService(MF_MEDIASOURCE_SERVICE, _uuidof(IMFMediaSource), &mediaSource);
	if (FAILED(hr)) {
		throw Platform::Exception::CreateException(hr);
	};

	hr = MFCreateSourceReaderFromMediaSource(mediaSource.Get(), sourceReaderAttributes.Get(), &_mfSourceReader);
	if (FAILED(hr)) {
		throw Platform::Exception::CreateException(hr);
	};

	ComPtr<IMFMediaType> outputType;
	MFCreateMediaType(&outputType);
	outputType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
	outputType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_NV12);
	hr = _mfSourceReader->SetCurrentMediaType(MF_SOURCE_READER_FIRST_VIDEO_STREAM, nullptr, outputType.Get());
	if (FAILED(hr)) {
		throw Platform::Exception::CreateException(hr);
	};
}

void YuvRenderer::H264LiveDecoder::StartRenderingLoop()
{
	concurrency::create_task([this]() {
		while (true) {
			ComPtr<IMFSample> sample;
			DWORD actualStreamIndex;
			DWORD streamFlags;
			long long timeStamp;
			HRESULT hr;
			hr = _mfSourceReader->ReadSample(MF_SOURCE_READER_FIRST_VIDEO_STREAM, 0, &actualStreamIndex, &streamFlags, &timeStamp, &sample);
			if (!SUCCEEDED(hr)) {

				return;
			}
			if (streamFlags & MF_SOURCE_READERF_ENDOFSTREAM) {
				return;
			}


			DWORD bufferCount;
			hr = sample->GetBufferCount(&bufferCount);
			ComPtr<IMFMediaBuffer> buffer;
			hr = sample->GetBufferByIndex(0, &buffer);
			ComPtr<IMFDXGIBuffer> dxgiBuffer;
			hr = buffer.As(&dxgiBuffer);
			ComPtr<ID3D11Texture2D> tex;
			uint32 subResourceIndex;
			dxgiBuffer->GetResource(__uuidof(ID3D11Texture2D), &tex);
			dxgiBuffer->GetSubresourceIndex(&subResourceIndex);
			_renderer->RenderNV12(tex.Get(), subResourceIndex);
		}
	});
}

void YuvRenderer::H264LiveDecoder::StopRenderingLoop()
{
	_sampleQueue->SubmitSample(nullptr);
}

void YuvRenderer::H264LiveDecoder::SubmitH264Frame(const Platform::Array<byte>^ data)
{
	auto buffer = ref new Windows::Storage::Streams::Buffer(data->Length);
	buffer->Length = data->Length;
	ComPtr<Windows::Storage::Streams::IBufferByteAccess> rawBuf;
	reinterpret_cast<IInspectable*>(buffer)->QueryInterface<Windows::Storage::Streams::IBufferByteAccess>(&rawBuf);
	byte* rawBytes;
	rawBuf->Buffer(&rawBytes);
	memcpy(rawBytes, data->Data, data->Length);
	TimeSpan ts;
	ts.Duration = _currentFrameIndex++;
	auto sample = MediaStreamSample::CreateFromBuffer(buffer, ts);
	_sampleQueue->SubmitSample(sample);
}

void YuvRenderer::SampleQueue::SubmitSample(Windows::Media::Core::MediaStreamSample ^ sample)
{
	std::unique_lock<std::mutex> l(_mutex);
	while (_samples.size() >= _maxFramesAhead) {
		_cvProduce.wait(l);
	}
	_samples.push(sample);
	_cv.notify_one();
}

Windows::Media::Core::MediaStreamSample ^ YuvRenderer::SampleQueue::GetSample()
{
	std::unique_lock<std::mutex> l(_mutex);
	while (_samples.size() == 0) {
		_cv.wait(l);
	}
	auto sample = _samples.front();
	_samples.pop();
	_cvProduce.notify_one();
	return sample;
}
