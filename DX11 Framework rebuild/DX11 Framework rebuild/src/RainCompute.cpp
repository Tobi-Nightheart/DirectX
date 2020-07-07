#include "RainCompute.h"
#include "WICTextureLoader.h"

//CHANGE because of duplication
static const int gNumRainGroupSize = 4;
static const int gRainGridSize = gNumRainGroupSize * 32;
static const int gHeightMapSize = 128;
const int gMaxRainDrop = gRainGridSize * gRainGridSize;

RainCompute::RainCompute(ID3D11Device* device, ID3D11DeviceContext* context, std::shared_ptr<Camera> camera, std::shared_ptr<GameTimer> gt)
{
	MyDevice = device;
	MyContext = context;
	MyCamera = camera;
	MyGameTimer = gt;
	MyBoundHalfSize = DirectX::XMVectorSet(15.0f, 20.0f, 15.0f, 0.0f);
	MyMaxWindVariance = 10.0f;
	MyStreakScale = 1.0f;
	MyDensity = 1.0f;
	MyVertSpeed = -5.0f;
	MyCurWindEffect = DirectX::XMVectorSet(1.0f, 1.0f, 0.0f, 0.0f);
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
	hr = MyDevice->CreateSamplerState(&sampler_desc, MySampler0.GetAddressOf());
	if (FAILED(hr)) return hr;

	D3D11_RASTERIZER_DESC raster_desc;
	ZeroMemory(&raster_desc, sizeof(raster_desc));
	raster_desc.CullMode = D3D11_CULL_NONE;
	raster_desc.FillMode = D3D11_FILL_SOLID;

	hr = MyDevice->CreateRasterizerState(&raster_desc, MyRasterState.GetAddressOf());
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
	hr = MyDevice->CreateVertexShader(VS->GetBufferPointer(), VS->GetBufferSize(), nullptr, MyVS_Out.GetAddressOf());
	if (FAILED(hr)) return hr;
	hr = MyDevice->CreateVertexShader(VSDepth->GetBufferPointer(), VSDepth->GetBufferSize(), nullptr, MyVS_Height.GetAddressOf());
	if (FAILED(hr)) return hr;
	hr = MyDevice->CreatePixelShader(PS->GetBufferPointer(), PS->GetBufferSize(), nullptr, MyPS.GetAddressOf());
	if (FAILED(hr)) return hr;
	hr = MyDevice->CreateComputeShader(CS->GetBufferPointer(), CS->GetBufferSize(), nullptr, MyCS.GetAddressOf());
	if (FAILED(hr)) return hr;

	//init depth resources for heightmap
	D3D11_TEXTURE2D_DESC t2d_desc;
	ZeroMemory(&t2d_desc, sizeof(t2d_desc));
	t2d_desc.Height = gHeightMapSize;
	t2d_desc.Width = gHeightMapSize;
	t2d_desc.MipLevels = 1;
	t2d_desc.ArraySize = 1;
	t2d_desc.Format = DXGI_FORMAT_R32_TYPELESS;
	t2d_desc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
	t2d_desc.Usage = D3D11_USAGE_DEFAULT;
	t2d_desc.SampleDesc.Count = 1;
	t2d_desc.SampleDesc.Quality = 0;
	hr = MyDevice->CreateTexture2D(&t2d_desc, nullptr, MyDepth.GetAddressOf());
	if (FAILED(hr)) return hr;

	D3D11_DEPTH_STENCIL_VIEW_DESC dsv_desc;
	ZeroMemory(&dsv_desc, sizeof(dsv_desc));
	dsv_desc.Format = DXGI_FORMAT_D32_FLOAT;
	dsv_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	hr = MyDevice->CreateDepthStencilView(MyDepth.Get(), &dsv_desc, MyHeightMapDSV.GetAddressOf());
	if (FAILED(hr)) return hr;

	D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc;
	ZeroMemory(&srv_desc, sizeof(srv_desc));
	srv_desc.Format = DXGI_FORMAT_R32_FLOAT;
	srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srv_desc.Texture2D.MipLevels = 1;
	srv_desc.Texture2D.MostDetailedMip = 0;
	hr = MyDevice->CreateShaderResourceView(MyDepth.Get(), &srv_desc, MyHeightMapSRV.GetAddressOf());
	if (FAILED(hr)) return hr;

	//init textures
	hr = CreateWICTextureFromFile(MyDevice.Get(), MyContext.Get(), L"assets/NoiseTexture.png", nullptr, MyNoiseSRV.GetAddressOf());
	if (FAILED(hr)) return hr;

	hr = CreateWICTextureFromFile(MyDevice.Get(), MyContext.Get(), L"assets/StreakTexture.png", nullptr, MyStreakSRV.GetAddressOf());
	if (FAILED(hr)) return hr;

	//vertexbuffer
	D3D11_BUFFER_DESC VB_desc;
	ZeroMemory(&VB_desc, sizeof(VB_desc));
	VB_desc.Usage = D3D11_USAGE_DEFAULT;
	VB_desc.BindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;
	VB_desc.StructureByteStride = sizeof(RainDrop);
	VB_desc.ByteWidth = gMaxRainDrop * sizeof(RainDrop);
	VB_desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;

	//init all values to low values for first update
	RainDrop arrInitSim[gMaxRainDrop];
	ZeroMemory(arrInitSim, sizeof(arrInitSim));
	for (int i = 0; i < gMaxRainDrop; i++) 
	{
		arrInitSim[i].Pos = DirectX::XMFLOAT3(0.0f, -10000.0f, 0.0f);
		arrInitSim[i].Vel = DirectX::XMFLOAT3(0.0f, -9.82f, 0.0f);
		arrInitSim[i].State = 0.0f;
	}

	D3D11_SUBRESOURCE_DATA InitData;
	InitData.pSysMem = arrInitSim;

	hr = MyDevice->CreateBuffer(&VB_desc, &InitData, MySimBuffer.GetAddressOf());
	if (FAILED(hr)) return hr;

	//init UAV CHANGE
	//D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc;
	ZeroMemory(&srv_desc, sizeof(srv_desc));
	srv_desc.ViewDimension = D3D11_SRV_DIMENSION_BUFFEREX;
	srv_desc.BufferEx.FirstElement = 0;	
	srv_desc.BufferEx.NumElements = gMaxRainDrop;
	srv_desc.Format = DXGI_FORMAT_UNKNOWN;
	hr = MyDevice->CreateShaderResourceView(MySimBuffer.Get(), &srv_desc, MySimBufferView.GetAddressOf());
	if (FAILED(hr)) return hr;

	D3D11_UNORDERED_ACCESS_VIEW_DESC uav_desc;
	ZeroMemory(&uav_desc, sizeof(uav_desc));
	uav_desc.Format = DXGI_FORMAT_UNKNOWN;
	uav_desc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
	uav_desc.Buffer.FirstElement = 0;
	uav_desc.Buffer.NumElements = gMaxRainDrop;

	hr = MyDevice->CreateUnorderedAccessView(MySimBuffer.Get(), &uav_desc, MyUAV.GetAddressOf());
	if (FAILED(hr)) return hr;

	//init CBs
	D3D11_BUFFER_DESC cb_desc;
	ZeroMemory(&cb_desc, sizeof(D3D11_BUFFER_DESC));
	cb_desc.Usage = D3D11_USAGE_DYNAMIC;
	cb_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	cb_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	cb_desc.ByteWidth = 64;

	hr = MyDevice->CreateBuffer(&cb_desc, nullptr, MyDepthCB.GetAddressOf());
	if (FAILED(hr)) return hr;
	
	cb_desc.ByteWidth = 112;

	hr = MyDevice->CreateBuffer(&cb_desc, nullptr, MySimulateCB.GetAddressOf());
	if (FAILED(hr)) return hr;

	cb_desc.ByteWidth = 96;

	hr = MyDevice->CreateBuffer(&cb_desc, nullptr, MyRendererCB.GetAddressOf());
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

	hr = MyDevice->CreateBlendState(&blend_desc, MyBlendRender.GetAddressOf());
	if (FAILED(hr)) return hr;

	//init input layout
	const D3D11_INPUT_ELEMENT_DESC layout[] =
	{
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
	};
	hr = MyDevice->CreateInputLayout(layout, ARRAYSIZE(layout), VSDepth->GetBufferPointer(), VSDepth->GetBufferSize(), MyInputLayout.GetAddressOf());
	if (FAILED(hr)) return hr;

	return S_OK;
}

void RainCompute::Draw()
{
	DepthPassPrep();
	//DepthPass();
	Simulate();
	Render();
}

void RainCompute::DepthPassPrep()
{
	//add orthographic projection setup and prepare for height map render

	DirectX::XMMATRIX othoProj;
	DirectX::XMFLOAT3 BoundHalfSize;
	DirectX::XMStoreFloat3(&BoundHalfSize, MyBoundHalfSize);
	othoProj = DirectX::XMMatrixOrthographicLH(BoundHalfSize.x, BoundHalfSize.z, 1, 1000);

}

void RainCompute::Simulate()
{
	MyContext->CSSetShaderResources(0, 1, MyNoiseSRV.GetAddressOf());
	MyContext->CSSetShaderResources(1, 1, MyHeightMapSRV.GetAddressOf());
	MyContext->CSSetUnorderedAccessViews(0, 1, MyUAV.GetAddressOf(), 0);
	MyContext->CSSetShader(MyCS.Get(), nullptr, 0);
	
	SimulateCB simBuffer;
	simBuffer.HeightSpace = MyRainViewProj;
	DirectX::XMStoreFloat3(&simBuffer.BoundHalfSize, MyBoundCenter);
	simBuffer.deltaTime = MyGameTimer->DeltaTime();
	DirectX::XMStoreFloat3(&simBuffer.BoundHalfSize, MyBoundHalfSize);
	simBuffer.HeightMapSize = gHeightMapSize;
	DirectX::XMStoreFloat2(&simBuffer.WindForce, MyCurWindEffect);
	simBuffer.VertSpeed = MyVertSpeed;
	simBuffer.WindVariation = MyMaxWindVariance;
	MyContext->UpdateSubresource(MySimulateCB.Get(), 0, 0, &simBuffer, 0, 0);
	MyContext->CSSetConstantBuffers(0, 1, MySimulateCB.GetAddressOf());
	
	MyContext->Dispatch(4, 4, 1);

	MyContext->CSSetConstantBuffers(0, 0, nullptr);
	MyContext->CSSetShader(nullptr, 0, 0);
	MyContext->CSSetUnorderedAccessViews(0, 0, nullptr, 0);
	MyContext->CSSetShaderResources(0, 2, nullptr);
}

void RainCompute::Render()
{
	MyContext->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	MyContext->OMSetBlendState(MyBlendRender.Get(), 0, 0);
	MyContext->PSSetShaderResources(0, 1, MyStreakSRV.GetAddressOf());
	MyContext->IASetInputLayout(nullptr);
	MyContext->IASetVertexBuffers(0, 1, nullptr, 0, 0);

	RendererCB RenderBuffer;
	RenderBuffer.ViewProj = MyRainViewProj;
	RenderBuffer.ViewDir = MyCamera->GetLook3f();
	RenderBuffer.AmbientColor = DirectX::XMFLOAT4(0.1f, 0.1f, 0.1f, 1.0f);
	RenderBuffer.scale = MyStreakScale;
	MyContext->UpdateSubresource(MyRendererCB.Get(), 0, 0, &RenderBuffer, 0, 0);
	MyContext->VSSetConstantBuffers(0, 1, MyRendererCB.GetAddressOf());

	MyContext->Draw(gMaxRainDrop*6, 0);

	MyContext->OMSetBlendState(MyBlendDefault.Get(), 0, 0);
	MyContext->PSSetShaderResources(0, 1, nullptr);
	MyContext->VSSetConstantBuffers(0, 1, nullptr);
}

void RainCompute::DepthPass(ID3D11RenderTargetView& defaultRTV, ID3D11DepthStencilView& defaultDSV, D3D11_VIEWPORT& viewport)
{
	//render the height map
}



void RainCompute::UpdateTransforms()
{
	//Update the transform of the bounding box according to camera movement
	MyBoundCenter = MyCamera->GetPosition();
}

