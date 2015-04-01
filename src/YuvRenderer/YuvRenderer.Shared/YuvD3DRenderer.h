#pragma once
#include <cstdint>
#include "ID3DRenderTarget.h"
#include <memory>
namespace YuvRenderer {
	public enum  class StretchMode
	{
		UniformToFill,
		Uniform,
		Fill
	};
	[Windows::Foundation::Metadata::WebHostHiddenAttribute]
	public ref class YuvD3DRenderer sealed
	{
	private:
		class Yuv420PDeviceResources;
		class NV12DeviceResources;
		std::unique_ptr<Yuv420PDeviceResources> _yuv420PDeviceResources;
		std::unique_ptr<NV12DeviceResources> _nv12DeviceResources;
		std::unique_ptr<ID3DRenderTarget> _renderTarget;
		std::uint32_t _srcWidth;
		std::uint32_t _srcHeight;
		StretchMode _stretchMode;
		YuvD3DRenderer(std::uint32_t srcWidth, std::uint32_t srcHeight, std::unique_ptr<ID3DRenderTarget>&& renderTarget, StretchMode stretchMode);
	internal:
		Microsoft::WRL::ComPtr<ID3D11Device> getNV12RendringDevice();
		void RenderNV12(ID3D11Texture2D* tex, uint32 subResourceIndex);
	public:
		virtual ~YuvD3DRenderer();
		property std::uint32_t SrcWidth {std::uint32_t get() { return _srcWidth; }}
		property std::uint32_t SrcHeight {std::uint32_t get() { return _srcHeight; }}
		void RenderYuv420P(Windows::Storage::Streams::IBuffer^ ydata, Windows::Storage::Streams::IBuffer^ udata, Windows::Storage::Streams::IBuffer^ vdata, std::uint32_t yStride, std::uint32_t uvStride);
		void RenderYuv420P(const Platform::Array<byte>^ ydata, const Platform::Array<byte>^ udata, const Platform::Array<byte>^ vdata, std::uint32_t yStride, std::uint32_t uvStride);
		void RenderNV12(const Platform::Array<byte>^ data, std::uint32_t stride);
		void TrimAndRelease();
		static YuvD3DRenderer^ CreateForD3DImageSource(Windows::UI::Xaml::Media::Imaging::SurfaceImageSource^ imageSource, std::uint32_t imgSourceWidth, std::uint32_t imgSourceHeight, std::uint32_t srcWidth, std::uint32_t srcHeight, StretchMode stretchMode);
		static YuvD3DRenderer^ CreateForSwapChainPanel(Windows::UI::Xaml::Controls::SwapChainPanel^ panel,  std::uint32_t srcWidth, std::uint32_t srcHeight, StretchMode stretchMode);
	};
}

