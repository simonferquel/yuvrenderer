#pragma once
#include <d3d11_2.h>
namespace YuvRenderer {
	/// @brief represents a D3D render target
	class ID3DRenderTarget {
	public:
		virtual ~ID3DRenderTarget() {}
		/// @brief get the aspect ration of the render target
		virtual float aspectRatio() const = 0;
		/// @brief setup the render target view of the device
		virtual void onBeginRender(ID3D11Device* device, ID3D11DeviceContext* deviceContext) = 0;
		/// @brief present the rendered scene
		virtual HRESULT onEndRender() = 0;
		/// @brief release resources allocated on the d3d device
		virtual void TrimAndRelease() = 0;
	};
}