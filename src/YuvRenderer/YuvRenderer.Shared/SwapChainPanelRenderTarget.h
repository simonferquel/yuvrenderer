#pragma once
#include "ID3DRenderTarget.h"
#include <windows.ui.xaml.media.dxinterop.h>
#include <atomic>
#include <wrl.h>
namespace YuvRenderer {

	/// @brief render target to a swapchainpanel
	class SwapChainPanelRenderTarget :
		public YuvRenderer::ID3DRenderTarget
	{
	private:
		std::atomic<float> _aspectRatio;
		Microsoft::WRL::ComPtr<IDXGISwapChain2> _swapChain;
		Microsoft::WRL::ComPtr<ID3D11RenderTargetView> _rtv;
		std::atomic<float> _targetWidth;
		std::atomic<float> _targetHeight;
		std::atomic<float> _inverseCompositionScaleX;
		std::atomic<float> _inverseCompositionScaleY;
		std::atomic<bool> _needResize;
		Windows::Foundation::EventRegistrationToken _resizeToken;
		Windows::Foundation::EventRegistrationToken _compositionScaleToken;

		ID3D11Device* _knownDevice;

		Windows::UI::Core::CoreDispatcher^ _dispatcher;
		Windows::UI::Xaml::Controls::SwapChainPanel^ _panel;
		Microsoft::WRL::ComPtr<ISwapChainPanelNative> _panelNative;
	public:
		SwapChainPanelRenderTarget(Windows::UI::Xaml::Controls::SwapChainPanel^ panel);
		virtual ~SwapChainPanelRenderTarget();

		// Inherited via ID3DRenderTarget
		virtual float aspectRatio() const override;
		virtual void onBeginRender(ID3D11Device * device, ID3D11DeviceContext * deviceContext) override;
		virtual HRESULT onEndRender() override;

		virtual void TrimAndRelease() {
			_panelNative->SetSwapChain(nullptr);
			_rtv = nullptr;
			_swapChain = nullptr;

		};
	};
}

