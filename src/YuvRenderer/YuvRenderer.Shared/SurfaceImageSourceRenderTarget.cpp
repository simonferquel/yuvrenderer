#include "pch.h"
#include "SurfaceImageSourceRenderTarget.h"
using namespace Microsoft::WRL;

YuvRenderer::SurfaceImageSourceRenderTarget::SurfaceImageSourceRenderTarget(
	Windows::UI::Xaml::Media::Imaging::SurfaceImageSource ^ imageSource, 
	std::uint32_t imgSourceWidth, 
	std::uint32_t imgSourceHeight)
	: _imgSource(imageSource), 
	_knownDevice(nullptr), 
	_knownSurface (nullptr), 
	_aspectRatio(static_cast<float>(imgSourceWidth)/static_cast<float>(imgSourceHeight)), 
	_imgSourceWidth(imgSourceWidth), 
	_imgSourceHeight(imgSourceHeight)
{
	reinterpret_cast<IInspectable*>(imageSource)->QueryInterface<ISurfaceImageSourceNative>(&_imgSourceNative);
}

YuvRenderer::SurfaceImageSourceRenderTarget::~SurfaceImageSourceRenderTarget()
{
}

float YuvRenderer::SurfaceImageSourceRenderTarget::aspectRatio() const
{
	return _aspectRatio;
}

void YuvRenderer::SurfaceImageSourceRenderTarget::onBeginRender(ID3D11Device * device, ID3D11DeviceContext * deviceContext)
{
	if (_knownDevice != device) {
		_knownDevice = device;
		ComPtr<IDXGIDevice> dxgiDevice;
		device->QueryInterface<IDXGIDevice>(&dxgiDevice);
		_imgSourceNative->SetDevice(dxgiDevice.Get());
	}
	ComPtr<IDXGISurface> surface;
	RECT updateRect;
	updateRect.top = 0;
	updateRect.left = 0;
	updateRect.right = _imgSourceWidth;
	updateRect.bottom = _imgSourceHeight;
	POINT offset;
	_imgSourceNative->BeginDraw(updateRect, &surface, &offset);
	if (surface.Get() != _knownSurface) {
		ComPtr<ID3D11Texture2D> tex;
		surface.As(&tex);
		device->CreateRenderTargetView(tex.Get(), nullptr, &_rtvCache);
	}
	D3D11_VIEWPORT vp;
	vp.MaxDepth = 1;
	vp.MinDepth = 0;
	vp.Height = static_cast<float>(_imgSourceHeight);
	vp.Width = static_cast<float>(_imgSourceWidth);
	vp.TopLeftX = static_cast<float>(offset.x);
	vp.TopLeftY = static_cast<float>(offset.y);
	deviceContext->OMSetRenderTargets(1, _rtvCache.GetAddressOf(), nullptr);
	deviceContext->RSSetViewports(1, &vp);
}

HRESULT YuvRenderer::SurfaceImageSourceRenderTarget::onEndRender()
{
	return _imgSourceNative->EndDraw();
}
