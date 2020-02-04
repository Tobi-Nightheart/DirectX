#include "RainCompute.h"
#include "WICTextureLoader.h"

//CHANGE because of doublication
static const int g_iNumRainGroupSize = 4;
static const int g_iRainGridSize = g_iNumRainGroupSize * 32;
static const int g_iHeightMapSize = 512;
const int g_iMaxRainDrop = g_iRainGridSize * g_iRainGridSize;

RainCompute::RainCompute(ID3D11Device* device, ID3D11DeviceContext* context, std::shared_ptr<Camera> camera, std::shared_ptr<GameTimer> gt)
{
	r_pDevice = device;
	r_pContext = context;
	r_pCamera = camera;
	r_pGameTimer = gt;
	r_vBoundHalfSize = DirectX::XMVectorSet(15.0f, 20.0f, 15.0f, 0.0f);
	r_fMaxWindVariance = 10.0f;
	r_fStreakScale = 1.0f;
	r_fDensity = 1.0f;
	r_fVertSpeed = -5.0f;
	r_vCurWindEffect = DirectX::XMVectorSet(1.0f, 1.0f, 0.0f, 0.0f);
}

HRESULT RainCompute::Init()
{
	HRESULT hr;

	//init sampler & raster
	D3D11_SAMPLER_DESC sampler_desc;
	ZeroMemory(&sampler_desc, sizeof(sampler_desc));
	sampler_desc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	sampler_desc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	sampler_desc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	sampler_desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	hr = r_pDevice->CreateSamplerState(&sampler_desc, r_pSampler0.GetAddressOf());
	if (FAILED(hr)) return hr;

	D3D11_RASTERIZER_DESC raster_desc;
	ZeroMemory(&raster_desc, sizeof(raster_desc));
	raster_desc.CullMode = D3D11_CULL_NONE;
	raster_desc.FillMode = D3D11_FILL_SOLID;

	hr = r_pDevice->CreateRasterizerState(&raster_desc, r_pRasterState.GetAddressOf());
	if (FAILED(hr)) return hr;

	//init shader
	ID3DBlob *VSDepth, * VS, *PS, *CS, *error;

	hr = D3DCompileFromFile(L"opaque.hlsl", nullptr, nullptr, "OpaqueVS", "vs_5_0", 0, 0, &VSDepth, &error);
	if (error != 0) 
	{
		OutputDebugStringA((char*)error->GetBufferPointer());
		error->Release();
		if (FAILED(hr))
		{
			return hr;
		}
	}
	
	hr = D3DCompileFromFile(L"RainShader.hlsl", nullptr, nullptr, "VSRain", "vs_5_0", 0, 0, &VS, &error);
	if (error != 0)
	{
		OutputDebugStringA((char*)error->GetBufferPointer());
		error->Release();
		if (FAILED(hr))
			return hr;
	}
	
	hr = D3DCompileFromFile(L"RainShader.hlsl", nullptr, nullptr, "PSRain", "ps_5_0", 0, 0, &PS, &error);
	if (error != 0)
	{
		OutputDebugStringA((char*)error->GetBufferPointer());
		error->Release();
		if (FAILED(hr))
			return hr;
	}

	hr = D3DCompileFromFile(L"RainSimulationCS.hlsl", nullptr, nullptr, "SimulateRain", "cs_5_0", 0, 0, &CS, &error);
	if (error != 0)
	{
		OutputDebugStringA((char*)error->GetBufferPointer());
		error->Release();
		if (FAILED(hr))
			return hr;
	}

	//create shader objects
	hr = r_pDevice->CreateVertexShader(VS->GetBufferPointer(), VS->GetBufferSize(), nullptr, r_pVS_Out.GetAddressOf());
	if (FAILED(hr)) return hr;
	hr = r_pDevice->CreateVertexShader(VSDepth->GetBufferPointer(), VSDepth->GetBufferSize(), nullptr, r_pVS_Height.GetAddressOf());
	if (FAILED(hr)) return hr;
	hr = r_pDevice->CreatePixelShader(PS->GetBufferPointer(), PS->GetBufferSize(), nullptr, r_pPS.GetAddressOf());
	if (FAILED(hr)) return hr;
	hr = r_pDevice->CreateComputeShader(CS->GetBufferPointer(), CS->GetBufferSize(), nullptr, r_pCS.GetAddressOf());
	if (FAILED(hr)) return hr;

	//init depth resources for heightmap
	D3D11_TEXTURE2D_DESC t2d_desc;
	ZeroMemory(&t2d_desc, sizeof(t2d_desc));
	t2d_desc.Height = g_iHeightMapSize;
	t2d_desc.Width = g_iHeightMapSize;
	t2d_desc.MipLevels = 1;
	t2d_desc.ArraySize = 1;
	t2d_desc.Format = DXGI_FORMAT_R32_TYPELESS;
	t2d_desc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
	t2d_desc.Usage = D3D11_USAGE_DEFAULT;
	t2d_desc.SampleDesc.Count = 1;
	t2d_desc.SampleDesc.Quality = 0;
	hr = r_pDevice->CreateTexture2D(&t2d_desc, nullptr, r_pDepth.GetAddressOf());
	if (FAILED(hr)) return hr;

	D3D11_DEPTH_STENCIL_VIEW_DESC dsv_desc;
	ZeroMemory(&dsv_desc, sizeof(dsv_desc));
	dsv_desc.Format = DXGI_FORMAT_D32_FLOAT;
	dsv_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	hr = r_pDevice->CreateDepthStencilView(r_pDepth.Get(), &dsv_desc, r_pHeightMapDSV.GetAddressOf());
	if (FAILED(hr)) return hr;

	D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc;
	ZeroMemory(&srv_desc, sizeof(srv_desc));
	srv_desc.Format = DXGI_FORMAT_R32_FLOAT;
	srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srv_desc.Texture2D.MipLevels = 1;
	srv_desc.Texture2D.MostDetailedMip = 0;
	hr = r_pDevice->CreateShaderResourceView(r_pDepth.Get(), &srv_desc, r_pHeightMapSRV.GetAddressOf());
	if (FAILED(hr)) return hr;

	//init textures
	hr = CreateWICTextureFromFile(r_pDevice.Get(), r_pContext.Get(), L"assets/noise.png", nullptr, r_pNoiseSRV.GetAddressOf());
	if (FAILED(hr)) return hr;

	hr = CreateWICTextureFromFile(r_pDevice.Get(), r_pContext.Get(), L"assets/StreakTexture.png", nullptr, r_pStreakSRV.GetAddressOf());
	if (FAILED(hr)) return hr;

	//vertexbuffer
	D3D11_BUFFER_DESC VB_desc;
	ZeroMemory(&VB_desc, sizeof(VB_desc));
	VB_desc.Usage = D3D11_USAGE_DEFAULT;
	VB_desc.BindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;
	VB_desc.StructureByteStride = sizeof(RainDrop);
	VB_desc.ByteWidth = g_iMaxRainDrop * sizeof(RainDrop);
	VB_desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;

	//init all values to low values for first update
	RainDrop arrInitSim[g_iMaxRainDrop];
	ZeroMemory(arrInitSim, sizeof(arrInitSim));
	for (int i = 0; i < g_iMaxRainDrop; i++) 
	{
		arrInitSim[i].Pos = DirectX::XMFLOAT3(0.0f, -1000.0f, 0.0f);
		arrInitSim[i].Vel = DirectX::XMFLOAT3(0.0f, -9.82f, 0.0f);
		arrInitSim[i].State = 0.0f;
	}

	D3D11_SUBRESOURCE_DATA InitData;
	InitData.pSysMem = arrInitSim;

	hr = r_pDevice->CreateBuffer(&VB_desc, &InitData, r_pSimBuffer.GetAddressOf());
	if (FAILED(hr)) return hr;

	//init UAV CHANGE
	//D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc;
	ZeroMemory(&srv_desc, sizeof(srv_desc));
	srv_desc.ViewDimension = D3D11_SRV_DIMENSION_BUFFEREX;
	srv_desc.BufferEx.FirstElement = 0;	
	srv_desc.BufferEx.NumElements = g_iMaxRainDrop;
	srv_desc.Format = DXGI_FORMAT_UNKNOWN;
	hr = r_pDevice->CreateShaderResourceView(r_pSimBuffer.Get(), &srv_desc, r_pSimBufferView.GetAddressOf());
	if (FAILED(hr)) return hr;

	D3D11_UNORDERED_ACCESS_VIEW_DESC uav_desc;
	ZeroMemory(&uav_desc, sizeof(uav_desc));
	uav_desc.Format = DXGI_FORMAT_UNKNOWN;
	uav_desc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
	uav_desc.Buffer.FirstElement = 0;
	uav_desc.Buffer.NumElements = g_iMaxRainDrop;

	hr = r_pDevice->CreateUnorderedAccessView(r_pSimBuffer.Get(), &uav_desc, r_pUAV.GetAddressOf());
	if (FAILED(hr)) return hr;

	//init CBs
	D3D11_BUFFER_DESC cb_desc;
	ZeroMemory(&cb_desc, sizeof(D3D11_BUFFER_DESC));
	cb_desc.Usage = D3D11_USAGE_DYNAMIC;
	cb_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	cb_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	cb_desc.ByteWidth = 64;

	hr = r_pDevice->CreateBuffer(&cb_desc, nullptr, r_pDepthCB.GetAddressOf());
	if (FAILED(hr)) return hr;
	
	cb_desc.ByteWidth = 112;

	hr = r_pDevice->CreateBuffer(&cb_desc, nullptr, r_pSimulateCB.GetAddressOf());
	if (FAILED(hr)) return hr;

	cb_desc.ByteWidth = 96;

	hr = r_pDevice->CreateBuffer(&cb_desc, nullptr, r_pRendererCB.GetAddressOf());
	if (FAILED(hr)) return hr;

	//init blending
	D3D11_BLEND_DESC blend_desc;
	ZeroMemory(&blend_desc, sizeof(blend_desc));
	blend_desc.RenderTarget[0].BlendEnable = true;
	blend_desc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	blend_desc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	blend_desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	blend_desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_SRC_ALPHA;
	blend_desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
	blend_desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	blend_desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

	hr = r_pDevice->CreateBlendState(&blend_desc, r_pBlendRender.GetAddressOf());
	if (FAILED(hr)) return hr;

	//init input layout
	const D3D11_INPUT_ELEMENT_DESC layout[] =
	{
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
	};
	hr = r_pDevice->CreateInputLayout(layout, ARRAYSIZE(layout), VSDepth->GetBufferPointer(), VSDepth->GetBufferSize(), r_pInputLayout.GetAddressOf());
	if (FAILED(hr)) return hr;

	return S_OK;
}

void RainCompute::DepthPassPrep()
{

}

void RainCompute::DepthPass(ID3D11RenderTargetView& defaultRTV, ID3D11DepthStencilView& defaultDSV, D3D11_VIEWPORT& viewport)
{

}


void RainCompute::UpdateTransforms()
{

}

