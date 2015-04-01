#include "pch.h"
#include "YuvD3DRenderer.h"
#include "SurfaceImageSourceRenderTarget.h"
#include <DirectXMath.h>
#include "YuvPixelShader.h"
#include "YuvPixelShaderNV12.h"
#include "YuvVertexShader.h"
#include <robuffer.h>
#include <SwapChainPanelRenderTarget.h>

using namespace YuvRenderer;
using namespace Microsoft::WRL;
struct YuvVertex {
	DirectX::XMFLOAT4 pos;
	DirectX::XMFLOAT2 uv;
};

struct YuvRenderer::YuvD3DRenderer::Yuv420PDeviceResources {



	ComPtr<ID3D11Device> _device;
	ComPtr<ID3D11DeviceContext> _deviceContext;
	ComPtr<ID3D11VertexShader> _vs;
	ComPtr<ID3D11PixelShader> _ps;
	ComPtr<ID3D11InputLayout> _inputLayout;
	ComPtr<ID3D11Buffer> _vertexBuffer;
	ComPtr<ID3D11Buffer> _vsConstantBuffer;
	ComPtr<ID3D11Texture2D> _ytex;
	ComPtr<ID3D11Texture2D> _utex;
	ComPtr<ID3D11Texture2D> _vtex;
	ComPtr<ID3D11ShaderResourceView> _ytexSRV;
	ComPtr<ID3D11ShaderResourceView> _utexSRV;
	ComPtr<ID3D11ShaderResourceView> _vtexSRV;
	ComPtr<ID3D11SamplerState> _texSampler;

	Yuv420PDeviceResources(std::uint32_t width, std::uint32_t height) {
		UINT creationFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT | D3D11_CREATE_DEVICE_VIDEO_SUPPORT;

#if defined(_DEBUG)     
		// If the project is in a debug build, enable debugging via SDK Layers. 
		creationFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif 

		const D3D_FEATURE_LEVEL featureLevels[] =
		{

			D3D_FEATURE_LEVEL_11_1,
			D3D_FEATURE_LEVEL_11_0,
			D3D_FEATURE_LEVEL_10_1,
			D3D_FEATURE_LEVEL_10_0,
			D3D_FEATURE_LEVEL_9_3,
			D3D_FEATURE_LEVEL_9_2,
			D3D_FEATURE_LEVEL_9_1,
		};
		auto hr = D3D11CreateDevice(
			NULL,
			D3D_DRIVER_TYPE_HARDWARE,
			NULL,
			creationFlags,
			featureLevels,
			ARRAYSIZE(featureLevels),
			D3D11_SDK_VERSION,
			&_device,
			NULL,
			&_deviceContext
			);
		if (FAILED(hr)) {
			throw Platform::Exception::CreateException(hr);
		};
		ComPtr<ID3D10Multithread> multiThreadControl;
		_device.As(&multiThreadControl);
		multiThreadControl->SetMultithreadProtected(TRUE);
		hr = _device->CreateVertexShader(YUV_RENDER_VERTEX_SHADER, ARRAYSIZE(YUV_RENDER_VERTEX_SHADER), nullptr, &_vs);
		if (FAILED(hr)) {
			throw Platform::Exception::CreateException(hr);
		};



		hr = _device->CreatePixelShader(YUV_RENDER_PIXEL_SHADER, ARRAYSIZE(YUV_RENDER_PIXEL_SHADER), nullptr, &_ps);
		if (FAILED(hr)) {
			throw Platform::Exception::CreateException(hr);
		};

		// init inputlayout

		D3D11_INPUT_ELEMENT_DESC elements[] = {
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, sizeof(DirectX::XMFLOAT4), D3D11_INPUT_PER_VERTEX_DATA, 0 },
		};

		hr = _device->CreateInputLayout(elements, ARRAYSIZE(elements), YUV_RENDER_VERTEX_SHADER, ARRAYSIZE(YUV_RENDER_VERTEX_SHADER), &_inputLayout);
		if (FAILED(hr)) {
			throw Platform::Exception::CreateException(hr);
		};

		// create vertexBuffer
		YuvVertex vertices[] = {
			{ DirectX::XMFLOAT4(-1, 1, .5f, 1), DirectX::XMFLOAT2(0, 0) },
			{ DirectX::XMFLOAT4(1, 1, .5f, 1), DirectX::XMFLOAT2(1, 0) },
			{ DirectX::XMFLOAT4(-1, -1, .5f, 1), DirectX::XMFLOAT2(0, 1) },
			{ DirectX::XMFLOAT4(1, -1, .5f, 1), DirectX::XMFLOAT2(1, 1) },
		};
		D3D11_SUBRESOURCE_DATA verticesSD;
		verticesSD.pSysMem = vertices;
		verticesSD.SysMemPitch = 0;
		verticesSD.SysMemSlicePitch = 0;
		hr = _device->CreateBuffer(&CD3D11_BUFFER_DESC(sizeof(vertices), D3D11_BIND_VERTEX_BUFFER, D3D11_USAGE_IMMUTABLE, 0, 0, sizeof(YuvVertex)), &verticesSD, &_vertexBuffer);
		if (FAILED(hr)) {
			throw Platform::Exception::CreateException(hr);
		};
		hr = _device->CreateBuffer(&CD3D11_BUFFER_DESC(16, D3D11_BIND_CONSTANT_BUFFER, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE, 0, 0), nullptr, &_vsConstantBuffer);
		if (FAILED(hr)) {
			throw Platform::Exception::CreateException(hr);
		};
		D3D11_TEXTURE2D_DESC yTexDesc;
		yTexDesc.Width = width;
		yTexDesc.Height = height;
		yTexDesc.MipLevels = 1;
		yTexDesc.ArraySize = 1;
		yTexDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R8_UNORM;
		yTexDesc.SampleDesc.Count = 1;
		yTexDesc.SampleDesc.Quality = 0;
		yTexDesc.Usage = D3D11_USAGE::D3D11_USAGE_DYNAMIC;
		yTexDesc.BindFlags = D3D11_BIND_FLAG::D3D11_BIND_SHADER_RESOURCE;
		yTexDesc.CPUAccessFlags = D3D11_CPU_ACCESS_FLAG::D3D11_CPU_ACCESS_WRITE;
		yTexDesc.MiscFlags = 0;


		D3D11_TEXTURE2D_DESC uTexDesc = yTexDesc;
		uTexDesc.Width = width / 2;
		uTexDesc.Height = height / 2;
		D3D11_TEXTURE2D_DESC vTexDesc = uTexDesc;

		hr = _device->CreateTexture2D(&yTexDesc, nullptr, &_ytex);
		if (FAILED(hr)) {
			throw Platform::Exception::CreateException(hr);
		};
		hr = _device->CreateTexture2D(&uTexDesc, nullptr, &_utex);
		if (FAILED(hr)) {
			throw Platform::Exception::CreateException(hr);
		};
		hr = _device->CreateTexture2D(&vTexDesc, nullptr, &_vtex);
		if (FAILED(hr)) {
			throw Platform::Exception::CreateException(hr);
		};
		hr = _device->CreateShaderResourceView(_ytex.Get(), nullptr, &_ytexSRV);
		if (FAILED(hr)) {
			throw Platform::Exception::CreateException(hr);
		};
		hr = _device->CreateShaderResourceView(_utex.Get(), nullptr, &_utexSRV);
		if (FAILED(hr)) {
			throw Platform::Exception::CreateException(hr);
		};
		hr = _device->CreateShaderResourceView(_vtex.Get(), nullptr, &_vtexSRV);
		if (FAILED(hr)) {
			throw Platform::Exception::CreateException(hr);
		};



		

	}
	void apply() {
		_deviceContext->VSSetShader(_vs.Get(), nullptr, 0);
		_deviceContext->PSSetShader(_ps.Get(), nullptr, 0);

		ID3D11ShaderResourceView* resourceViews[] = { _ytexSRV.Get(), _utexSRV.Get(), _vtexSRV.Get() };
		_deviceContext->PSSetShaderResources(0, ARRAYSIZE(resourceViews), resourceViews);

		_deviceContext->IASetInputLayout(_inputLayout.Get());
		_deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
		UINT32 stride = sizeof(YuvVertex);
		UINT32 voffset = 0;
		_deviceContext->IASetVertexBuffers(0, 1, _vertexBuffer.GetAddressOf(), &stride, &voffset);

		_deviceContext->VSSetConstantBuffers(0, 1, _vsConstantBuffer.GetAddressOf());
	}
};


struct YuvRenderer::YuvD3DRenderer::NV12DeviceResources {



	ComPtr<ID3D11Device> _device;
	ComPtr<ID3D11DeviceContext> _deviceContext;
	ComPtr<ID3D11VertexShader> _vs;
	ComPtr<ID3D11PixelShader> _ps;
	ComPtr<ID3D11InputLayout> _inputLayout;
	ComPtr<ID3D11Buffer> _vertexBuffer;
	ComPtr<ID3D11Buffer> _vsConstantBuffer;
	ComPtr<ID3D11Texture2D> _ytex;
	ComPtr<ID3D11Texture2D> _uvtex;
	ComPtr<ID3D11Texture2D> _nv12StagingTex;
	ComPtr<ID3D11ShaderResourceView> _ytexSRV;
	ComPtr<ID3D11ShaderResourceView> _uvtexSRV;
	ComPtr<ID3D11SamplerState> _texSampler;

	NV12DeviceResources(std::uint32_t width, std::uint32_t height) {
		UINT creationFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT | D3D11_CREATE_DEVICE_VIDEO_SUPPORT;

#if defined(_DEBUG)     
		// If the project is in a debug build, enable debugging via SDK Layers. 
		creationFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif 

		const D3D_FEATURE_LEVEL featureLevels[] =
		{

			D3D_FEATURE_LEVEL_11_1,
			D3D_FEATURE_LEVEL_11_0,
			D3D_FEATURE_LEVEL_10_1,
			D3D_FEATURE_LEVEL_10_0,
			D3D_FEATURE_LEVEL_9_3,
			D3D_FEATURE_LEVEL_9_2,
			D3D_FEATURE_LEVEL_9_1,
		};
		auto hr = D3D11CreateDevice(
			NULL,
			D3D_DRIVER_TYPE_HARDWARE,
			NULL,
			creationFlags,
			featureLevels,
			ARRAYSIZE(featureLevels),
			D3D11_SDK_VERSION,
			&_device,
			NULL,
			&_deviceContext
			);
		if (FAILED(hr)) {
			throw Platform::Exception::CreateException(hr);
		};
		ComPtr<ID3D10Multithread> multiThreadControl;
		_device.As(&multiThreadControl);
		multiThreadControl->SetMultithreadProtected(TRUE);
		hr = _device->CreateVertexShader(YUV_RENDER_VERTEX_SHADER, ARRAYSIZE(YUV_RENDER_VERTEX_SHADER), nullptr, &_vs);
		if (FAILED(hr)) {
			throw Platform::Exception::CreateException(hr);
		};



		hr = _device->CreatePixelShader(NV12_RENDER_PIXEL_SHADER, ARRAYSIZE(NV12_RENDER_PIXEL_SHADER), nullptr, &_ps);
		if (FAILED(hr)) {
			throw Platform::Exception::CreateException(hr);
		};

		// init inputlayout

		D3D11_INPUT_ELEMENT_DESC elements[] = {
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, sizeof(DirectX::XMFLOAT4), D3D11_INPUT_PER_VERTEX_DATA, 0 },
		};

		hr = _device->CreateInputLayout(elements, ARRAYSIZE(elements), YUV_RENDER_VERTEX_SHADER, ARRAYSIZE(YUV_RENDER_VERTEX_SHADER), &_inputLayout);
		if (FAILED(hr)) {
			throw Platform::Exception::CreateException(hr);
		};

		// create vertexBuffer
		YuvVertex vertices[] = {
			{ DirectX::XMFLOAT4(-1, 1, .5f, 1), DirectX::XMFLOAT2(0, 0) },
			{ DirectX::XMFLOAT4(1, 1, .5f, 1), DirectX::XMFLOAT2(1, 0) },
			{ DirectX::XMFLOAT4(-1, -1, .5f, 1), DirectX::XMFLOAT2(0, 1) },
			{ DirectX::XMFLOAT4(1, -1, .5f, 1), DirectX::XMFLOAT2(1, 1) },
		};
		D3D11_SUBRESOURCE_DATA verticesSD;
		verticesSD.pSysMem = vertices;
		verticesSD.SysMemPitch = 0;
		verticesSD.SysMemSlicePitch = 0;
		hr = _device->CreateBuffer(&CD3D11_BUFFER_DESC(sizeof(vertices), D3D11_BIND_VERTEX_BUFFER, D3D11_USAGE_IMMUTABLE, 0, 0, sizeof(YuvVertex)), &verticesSD, &_vertexBuffer);
		if (FAILED(hr)) {
			throw Platform::Exception::CreateException(hr);
		};
		hr = _device->CreateBuffer(&CD3D11_BUFFER_DESC(16, D3D11_BIND_CONSTANT_BUFFER, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE, 0, 0), nullptr, &_vsConstantBuffer);
		if (FAILED(hr)) {
			throw Platform::Exception::CreateException(hr);
		};
		D3D11_TEXTURE2D_DESC yTexDesc;
		yTexDesc.Width = width;
		yTexDesc.Height = height;
		yTexDesc.MipLevels = 1;
		yTexDesc.ArraySize = 1;
		yTexDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R8_UNORM;
		yTexDesc.SampleDesc.Count = 1;
		yTexDesc.SampleDesc.Quality = 0;
		yTexDesc.Usage = D3D11_USAGE::D3D11_USAGE_DYNAMIC;
		yTexDesc.BindFlags = D3D11_BIND_FLAG::D3D11_BIND_SHADER_RESOURCE;
		yTexDesc.CPUAccessFlags = D3D11_CPU_ACCESS_FLAG::D3D11_CPU_ACCESS_WRITE;
		yTexDesc.MiscFlags = 0;

		D3D11_TEXTURE2D_DESC nv12StatingTexDesc = yTexDesc;
		nv12StatingTexDesc.BindFlags = 0;
		nv12StatingTexDesc.Usage = D3D11_USAGE_STAGING;
		nv12StatingTexDesc.ArraySize = 1;
		nv12StatingTexDesc.Format = DXGI_FORMAT::DXGI_FORMAT_NV12;
		nv12StatingTexDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;


		D3D11_TEXTURE2D_DESC uvTexDesc = yTexDesc;
		uvTexDesc.Width = width / 2;
		uvTexDesc.Height = height / 2;
		uvTexDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R8G8_UNORM;

		hr = _device->CreateTexture2D(&yTexDesc, nullptr, &_ytex);
		if (FAILED(hr)) {
			throw Platform::Exception::CreateException(hr);
		};
		hr = _device->CreateTexture2D(&uvTexDesc, nullptr, &_uvtex);
		if (FAILED(hr)) {
			throw Platform::Exception::CreateException(hr);
		};

		hr = _device->CreateTexture2D(&nv12StatingTexDesc, nullptr, &_nv12StagingTex);
		if (FAILED(hr)) {
			throw Platform::Exception::CreateException(hr);
		};
		
		hr = _device->CreateShaderResourceView(_ytex.Get(), nullptr, &_ytexSRV);
		if (FAILED(hr)) {
			throw Platform::Exception::CreateException(hr);
		};
		hr = _device->CreateShaderResourceView(_uvtex.Get(), nullptr, &_uvtexSRV);
		if (FAILED(hr)) {
			throw Platform::Exception::CreateException(hr);
		};
		





	}
	void apply() {
		_deviceContext->VSSetShader(_vs.Get(), nullptr, 0);
		_deviceContext->PSSetShader(_ps.Get(), nullptr, 0);

		ID3D11ShaderResourceView* resourceViews[] = { _ytexSRV.Get(), _uvtexSRV.Get() };
		_deviceContext->PSSetShaderResources(0, ARRAYSIZE(resourceViews), resourceViews);

		_deviceContext->IASetInputLayout(_inputLayout.Get());
		_deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
		UINT32 stride = sizeof(YuvVertex);
		UINT32 voffset = 0;
		_deviceContext->IASetVertexBuffers(0, 1, _vertexBuffer.GetAddressOf(), &stride, &voffset);

		_deviceContext->VSSetConstantBuffers(0, 1, _vsConstantBuffer.GetAddressOf());
	}
};

YuvRenderer::YuvD3DRenderer::YuvD3DRenderer( std::uint32_t srcWidth, std::uint32_t srcHeight, std::unique_ptr<ID3DRenderTarget>&& renderTarget, StretchMode stretchMode)
	: _renderTarget(std::move(renderTarget)),
	_srcWidth(srcWidth),
	_srcHeight(srcHeight),
	_stretchMode(stretchMode)
{
}

Microsoft::WRL::ComPtr<ID3D11Device> YuvRenderer::YuvD3DRenderer::getNV12RendringDevice()
{
	if (_nv12DeviceResources == nullptr) {
		_nv12DeviceResources.reset(new NV12DeviceResources(_srcWidth, _srcHeight));
	}
	return _nv12DeviceResources->_device;
}

void YuvRenderer::YuvD3DRenderer::RenderNV12(ID3D11Texture2D * tex, uint32 subResourceIndex)
{
	if (_nv12DeviceResources == nullptr) {
		_nv12DeviceResources.reset(new NV12DeviceResources(_srcWidth, _srcHeight));
	}
	_nv12DeviceResources->_deviceContext->CopySubresourceRegion(_nv12DeviceResources->_nv12StagingTex.Get(), 0, 0, 0, 0, tex, subResourceIndex, nullptr);
	
	D3D11_MAPPED_SUBRESOURCE stagingMappedResource;
	auto hr = _nv12DeviceResources->_deviceContext->Map(_nv12DeviceResources->_nv12StagingTex.Get(), 0, D3D11_MAP_READ, 0, &stagingMappedResource);
	RenderNV12(Platform::ArrayReference<byte>((byte*)stagingMappedResource.pData, stagingMappedResource.RowPitch* (_srcHeight + _srcHeight / 2)), stagingMappedResource.RowPitch);
	_nv12DeviceResources->_deviceContext->Unmap(_nv12DeviceResources->_nv12StagingTex.Get(), 0);


}

YuvRenderer::YuvD3DRenderer::~YuvD3DRenderer()
{
}

void fillFactors(StretchMode stretchMode, float factor, float aspectRatioFactorToSource[]) {
	switch (stretchMode) {
	case StretchMode::UniformToFill:
		aspectRatioFactorToSource[0] = factor > 1 ? 1.0f : 1 / factor;
		aspectRatioFactorToSource[1] = factor > 1 ? factor : 1.0f;
		break;
	case StretchMode::Fill:
		aspectRatioFactorToSource[0] = 1;
		aspectRatioFactorToSource[1] = 1;
		break;
	case StretchMode::Uniform:
		aspectRatioFactorToSource[0] = factor > 1 ? 1 / factor : 1.0f;
		aspectRatioFactorToSource[1] = factor > 1 ? 1.0f : factor;
		break;
			
	}
}

void YuvRenderer::YuvD3DRenderer::RenderYuv420P(Windows::Storage::Streams::IBuffer ^ ydata, Windows::Storage::Streams::IBuffer ^ udata, Windows::Storage::Streams::IBuffer ^ vdata, std::uint32_t yStride, std::uint32_t uvStride)
{
	if (_yuv420PDeviceResources == nullptr) {
		_yuv420PDeviceResources.reset(new Yuv420PDeviceResources(_srcWidth, _srcHeight));
	}
	D3D11_MAPPED_SUBRESOURCE ytexMapped;
	D3D11_MAPPED_SUBRESOURCE utexMapped;
	D3D11_MAPPED_SUBRESOURCE vtexMapped;
	auto hr = _yuv420PDeviceResources->_deviceContext->Map(_yuv420PDeviceResources->_ytex.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &ytexMapped);
	if (FAILED(hr)) {
		throw Platform::Exception::CreateException(hr);
	};

	hr = _yuv420PDeviceResources->_deviceContext->Map(_yuv420PDeviceResources->_utex.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &utexMapped);
	if (FAILED(hr)) {
		throw Platform::Exception::CreateException(hr);
	};

	hr = _yuv420PDeviceResources->_deviceContext->Map(_yuv420PDeviceResources->_vtex.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &vtexMapped);
	if (FAILED(hr)) {
		throw Platform::Exception::CreateException(hr);
	};

	ComPtr<Windows::Storage::Streams::IBufferByteAccess> rawYData;
	reinterpret_cast<IInspectable*>(ydata)->QueryInterface<Windows::Storage::Streams::IBufferByteAccess>(&rawYData);
	byte* rawYBytes;
	rawYData->Buffer(&rawYBytes);
	ComPtr<Windows::Storage::Streams::IBufferByteAccess> rawUData;
	reinterpret_cast<IInspectable*>(udata)->QueryInterface<Windows::Storage::Streams::IBufferByteAccess>(&rawUData);
	byte* rawUBytes;
	rawUData->Buffer(&rawUBytes);
	ComPtr<Windows::Storage::Streams::IBufferByteAccess> rawVData;
	reinterpret_cast<IInspectable*>(vdata)->QueryInterface<Windows::Storage::Streams::IBufferByteAccess>(&rawVData);
	byte* rawVBytes;
	rawVData->Buffer(&rawVBytes);

	for (uint32_t y = 0; y < _srcHeight; ++y) {
		memcpy(static_cast<uint8_t*>(ytexMapped.pData) + (y*ytexMapped.RowPitch), rawYBytes + (y*yStride), _srcWidth);
	}
	for (uint32_t y = 0; y < _srcHeight / 2; ++y) {
		memcpy(static_cast<uint8_t*>(utexMapped.pData) + (y*utexMapped.RowPitch), rawUBytes + (y*uvStride), _srcWidth / 2);

		memcpy(static_cast<uint8_t*>(vtexMapped.pData) + (y*vtexMapped.RowPitch), rawVBytes + (y*uvStride), _srcWidth / 2);
	}

	_yuv420PDeviceResources->_deviceContext->Unmap(_yuv420PDeviceResources->_vtex.Get(), 0);
	_yuv420PDeviceResources->_deviceContext->Unmap(_yuv420PDeviceResources->_utex.Get(), 0);
	_yuv420PDeviceResources->_deviceContext->Unmap(_yuv420PDeviceResources->_ytex.Get(), 0);

	_renderTarget->onBeginRender(_yuv420PDeviceResources->_device.Get(), _yuv420PDeviceResources->_deviceContext.Get());
	auto factor = _renderTarget->aspectRatio() / (static_cast<float>(_srcWidth) / static_cast<float>(_srcHeight));
	float factors[2];
	fillFactors(_stretchMode, factor, factors);
	D3D11_MAPPED_SUBRESOURCE scaleBuffer;
	_yuv420PDeviceResources->_deviceContext->Map(_yuv420PDeviceResources->_vsConstantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &scaleBuffer);
	memcpy(scaleBuffer.pData, factors, sizeof(factors));
	_yuv420PDeviceResources->_deviceContext->Unmap(_yuv420PDeviceResources->_vsConstantBuffer.Get(), 0);
	_yuv420PDeviceResources->apply();
	_yuv420PDeviceResources->_deviceContext->Draw(4, 0);
	auto presentResult = _renderTarget->onEndRender();
	if (presentResult == DXGI_ERROR_DEVICE_RESET || presentResult == DXGI_ERROR_DEVICE_REMOVED) {
		_yuv420PDeviceResources = nullptr;
	}



}

void YuvRenderer::YuvD3DRenderer::RenderYuv420P(const Platform::Array<byte>^ ydata, const Platform::Array<byte>^ udata, const Platform::Array<byte>^ vdata, std::uint32_t yStride, std::uint32_t uvStride)
{
	if (_yuv420PDeviceResources == nullptr) {
		_yuv420PDeviceResources.reset(new Yuv420PDeviceResources(_srcWidth, _srcHeight));
	}
	D3D11_MAPPED_SUBRESOURCE ytexMapped;
	D3D11_MAPPED_SUBRESOURCE utexMapped;
	D3D11_MAPPED_SUBRESOURCE vtexMapped;
	auto hr = _yuv420PDeviceResources->_deviceContext->Map(_yuv420PDeviceResources->_ytex.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &ytexMapped);
	if (FAILED(hr)) {
		throw Platform::Exception::CreateException(hr);
	};

	hr = _yuv420PDeviceResources->_deviceContext->Map(_yuv420PDeviceResources->_utex.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &utexMapped);
	if (FAILED(hr)) {
		throw Platform::Exception::CreateException(hr);
	};

	hr = _yuv420PDeviceResources->_deviceContext->Map(_yuv420PDeviceResources->_vtex.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &vtexMapped);
	if (FAILED(hr)) {
		throw Platform::Exception::CreateException(hr);
	};

	
	

	for (uint32_t y = 0; y < _srcHeight; ++y) {
		memcpy(static_cast<uint8_t*>(ytexMapped.pData) + (y*ytexMapped.RowPitch), ydata->Data + (y*yStride), _srcWidth);
	}
	for (uint32_t y = 0; y < _srcHeight / 2; ++y) {
		memcpy(static_cast<uint8_t*>(utexMapped.pData) + (y*utexMapped.RowPitch), udata->Data + (y*uvStride), _srcWidth / 2);

		memcpy(static_cast<uint8_t*>(vtexMapped.pData) + (y*vtexMapped.RowPitch), vdata->Data + (y*uvStride), _srcWidth / 2);
	}

	_yuv420PDeviceResources->_deviceContext->Unmap(_yuv420PDeviceResources->_vtex.Get(), 0);
	_yuv420PDeviceResources->_deviceContext->Unmap(_yuv420PDeviceResources->_utex.Get(), 0);
	_yuv420PDeviceResources->_deviceContext->Unmap(_yuv420PDeviceResources->_ytex.Get(), 0);

	_renderTarget->onBeginRender(_yuv420PDeviceResources->_device.Get(), _yuv420PDeviceResources->_deviceContext.Get());
	auto factor = _renderTarget->aspectRatio() / (static_cast<float>(_srcWidth) / static_cast<float>(_srcHeight));
	float factors[2];
	fillFactors(_stretchMode, factor, factors);
	D3D11_MAPPED_SUBRESOURCE scaleBuffer;
	_yuv420PDeviceResources->_deviceContext->Map(_yuv420PDeviceResources->_vsConstantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &scaleBuffer);
	memcpy(scaleBuffer.pData, factors, sizeof(factors));
	_yuv420PDeviceResources->_deviceContext->Unmap(_yuv420PDeviceResources->_vsConstantBuffer.Get(), 0);
	_yuv420PDeviceResources->apply();
	_yuv420PDeviceResources->_deviceContext->Draw(4, 0);
	auto presentResult = _renderTarget->onEndRender();
	if (presentResult == DXGI_ERROR_DEVICE_RESET || presentResult == DXGI_ERROR_DEVICE_REMOVED) {
		_yuv420PDeviceResources = nullptr;
	}

}

void YuvRenderer::YuvD3DRenderer::RenderNV12(const Platform::Array<byte>^ data, std::uint32_t stride)
{
	if (_nv12DeviceResources == nullptr) {
		_nv12DeviceResources.reset(new NV12DeviceResources(_srcWidth, _srcHeight));
	}
	D3D11_MAPPED_SUBRESOURCE ytexMapped;
	D3D11_MAPPED_SUBRESOURCE uvtexMapped;
	auto hr = _nv12DeviceResources->_deviceContext->Map(_nv12DeviceResources->_ytex.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &ytexMapped);
	if (FAILED(hr)) {
		throw Platform::Exception::CreateException(hr);
	};

	hr = _nv12DeviceResources->_deviceContext->Map(_nv12DeviceResources->_uvtex.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &uvtexMapped);
	if (FAILED(hr)) {
		throw Platform::Exception::CreateException(hr);
	};





	if (ytexMapped.RowPitch == stride) {
		memcpy(static_cast<uint8_t*>(ytexMapped.pData), data->Data, stride * (_srcHeight));
	}
	else {
		for (uint32_t y = 0; y < _srcHeight; ++y) {
			memcpy(static_cast<uint8_t*>(ytexMapped.pData) + (y*ytexMapped.RowPitch), data->Data + (y*stride), _srcWidth);
		}
	}
	if (uvtexMapped.RowPitch == stride) {
		memcpy(static_cast<uint8_t*>(uvtexMapped.pData), data->Data+ stride * (_srcHeight), stride * (_srcHeight/2));
	}
	else {
		for (uint32_t y = 0; y < _srcHeight / 2; ++y) {
			memcpy(static_cast<uint8_t*>(uvtexMapped.pData) + (y*uvtexMapped.RowPitch), data->Data + ((_srcHeight + y)*stride), _srcWidth);
		}
	}

	_nv12DeviceResources->_deviceContext->Unmap(_nv12DeviceResources->_uvtex.Get(), 0);
	_nv12DeviceResources->_deviceContext->Unmap(_nv12DeviceResources->_ytex.Get(), 0);

	_renderTarget->onBeginRender(_nv12DeviceResources->_device.Get(), _nv12DeviceResources->_deviceContext.Get());
	auto factor = _renderTarget->aspectRatio() / (static_cast<float>(_srcWidth) / static_cast<float>(_srcHeight));
	float factors[2];
	fillFactors(_stretchMode, factor, factors);
	D3D11_MAPPED_SUBRESOURCE scaleBuffer;
	_nv12DeviceResources->_deviceContext->Map(_nv12DeviceResources->_vsConstantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &scaleBuffer);
	memcpy(scaleBuffer.pData, factors, sizeof(factors));
	_nv12DeviceResources->_deviceContext->Unmap(_nv12DeviceResources->_vsConstantBuffer.Get(), 0);
	_nv12DeviceResources->apply();
	_nv12DeviceResources->_deviceContext->Draw(4, 0);
	auto presentResult = _renderTarget->onEndRender();
	if (presentResult == DXGI_ERROR_DEVICE_RESET || presentResult == DXGI_ERROR_DEVICE_REMOVED) {
		_nv12DeviceResources = nullptr;
	}
}

void YuvRenderer::YuvD3DRenderer::TrimAndRelease()
{
	if (_yuv420PDeviceResources != nullptr) {
		ComPtr<IDXGIDevice3> trimableDevice;
		_yuv420PDeviceResources->_device.As(&trimableDevice);
		trimableDevice->Trim();
		_renderTarget->TrimAndRelease();
		_yuv420PDeviceResources = nullptr;
	}
}

YuvD3DRenderer^ YuvRenderer::YuvD3DRenderer::CreateForD3DImageSource(Windows::UI::Xaml::Media::Imaging::SurfaceImageSource ^ imageSource, std::uint32_t imgSourceWidth, std::uint32_t imgSourceHeight, std::uint32_t srcWidth, std::uint32_t srcHeight, StretchMode stretchMode)
{
	return ref new YuvD3DRenderer( srcWidth, srcHeight, std::make_unique< SurfaceImageSourceRenderTarget>(imageSource, imgSourceWidth, imgSourceHeight), stretchMode);

}

YuvD3DRenderer ^ YuvRenderer::YuvD3DRenderer::CreateForSwapChainPanel( Windows::UI::Xaml::Controls::SwapChainPanel^ panel, std::uint32_t srcWidth, std::uint32_t srcHeight, StretchMode stretchMode)
{
	return ref new YuvD3DRenderer(srcWidth, srcHeight, std::make_unique< SwapChainPanelRenderTarget>(panel), stretchMode);
}
