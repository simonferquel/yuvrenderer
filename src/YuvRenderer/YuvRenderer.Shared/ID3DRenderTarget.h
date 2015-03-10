#pragma once
#include <d3d11_2.h>
namespace YuvRenderer {
	class ID3DRenderTarget {
	public:
		virtual ~ID3DRenderTarget() {}
		virtual float aspectRatio() const = 0;
		virtual void onBeginRender(ID3D11Device* device, ID3D11DeviceContext* deviceContext) = 0;
		virtual HRESULT onEndRender() = 0;
		virtual void TrimAndRelease() = 0;
	};
}