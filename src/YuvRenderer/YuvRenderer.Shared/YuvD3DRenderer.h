#pragma once
#include <cstdint>
#include "ID3DRenderTarget.h"
#include <memory>
namespace YuvRenderer {
	public enum class YuvColorMode {
		Yuv420P
	};
	[Windows::Foundation::Metadata::WebHostHiddenAttribute]
	public ref class YuvD3DRenderer sealed
	{
	private:
		class DeviceResources;
		std::unique_ptr<DeviceResources> _deviceResources;
		std::unique_ptr<ID3DRenderTarget> _renderTarget;
		YuvColorMode _colorMode;
		std::uint32_t _srcWidth;
		std::uint32_t _srcHeight;
		YuvD3DRenderer(YuvColorMode colorMode, std::uint32_t srcWidth, std::uint32_t srcHeight, std::unique_ptr<ID3DRenderTarget>&& renderTarget);
		
	public:
		virtual ~YuvD3DRenderer();
		void Render(Windows::Storage::Streams::IBuffer^ ydata, Windows::Storage::Streams::IBuffer^ udata, Windows::Storage::Streams::IBuffer^ vdata, std::uint32_t yStride, std::uint32_t uvStride);
		void TrimAndRelease();
		static YuvD3DRenderer^ CreateForD3DImageSource(YuvColorMode colorMode, Windows::UI::Xaml::Media::Imaging::SurfaceImageSource^ imageSource, std::uint32_t imgSourceWidth, std::uint32_t imgSourceHeight, std::uint32_t srcWidth, std::uint32_t srcHeight);
				static YuvD3DRenderer^ CreateForSwapChainPanel(YuvColorMode colorMode, Windows::UI::Xaml::Controls::SwapChainPanel^ panel,  std::uint32_t srcWidth, std::uint32_t srcHeight);
	};
}

