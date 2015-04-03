#pragma once
#include "ID3DRenderTarget.h"
#include <windows.ui.xaml.media.dxinterop.h>
#include <wrl.h>
#include <d3d11_2.h>
#include <cstdint>

namespace YuvRenderer  {
	/// @brief Render target to a Surface Image Source
	class SurfaceImageSourceRenderTarget :
		public YuvRenderer::ID3DRenderTarget
	{
	private:
		Windows::UI::Xaml::Media::Imaging::SurfaceImageSource^ _imgSource;
		Microsoft::WRL::ComPtr<ISurfaceImageSourceNative> _imgSourceNative;
		ID3D11Device* _knownDevice;
		IDXGISurface* _knownSurface;
		Microsoft::WRL::ComPtr<ID3D11RenderTargetView> _rtvCache;
		float _aspectRatio;
		std::uint32_t _imgSourceWidth;
		std::uint32_t _imgSourceHeight;
	public:
		SurfaceImageSourceRenderTarget(Windows::UI::Xaml::Media::Imaging::SurfaceImageSource^ imageSource, std::uint32_t imgSourceWidth, std::uint32_t imgSourceHeight);
		virtual ~SurfaceImageSourceRenderTarget();

		virtual float aspectRatio() const;
		virtual void onBeginRender(ID3D11Device* device, ID3D11DeviceContext* deviceContext);
		virtual HRESULT onEndRender();

		virtual void TrimAndRelease() { _rtvCache = nullptr; };
	};
}

