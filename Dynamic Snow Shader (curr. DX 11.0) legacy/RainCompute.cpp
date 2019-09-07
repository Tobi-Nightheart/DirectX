#include "RainCompute.h"
#include <minwinbase.h>

static const int g_iNumRainGroupSize = 4;
static const int g_iRainGridSize = g_iNumRainGroupSize * 32;
static const int g_iHeightMapSize = 512;
const int g_iMaxRainDrop = g_iRainGridSize * g_iRainGridSize;


RainCompute::RainCompute(ID3D11Device* device, ID3D11DeviceContext* context, camera* camera, GameTimer* gameTimer)
{
	r_pDevice = device;
	r_pContext = context;
	r_pCamera = camera;
	r_pGameTimer = gameTimer;
	r_vBoundHalfSize = XMVectorSet(15.0f, 20.0f, 15.0f, 0.0f);
	r_fMaxWindVariance = 10.0f;
	//r_fStreakScale = 0.4f;
	r_fStreakScale = 1.0f;
	r_fDensity = 1.0f;
	r_fVerticalSpeed = -5.0f;
	r_vCurWindEffect = XMVectorSet(1.0f, 1.0f, 0.0f, 0.0f);

#pragma region NullInits
	initFLOAT = 0;


	r_pBlendDefault = nullptr;
	r_pBlendRender = nullptr;
	r_pCBDepth = nullptr;
	r_pCBRenderer = nullptr;
	r_pCBSimulate = nullptr;
	r_pCS = nullptr;
	r_pHeightMapDSV = nullptr;
	r_pHeightLayout = nullptr;
	r_pHeightMap2D = nullptr;
	r_pHeightMapSRV = nullptr;
	r_pInputLayout = nullptr;
	r_pRasterState = nullptr;
	r_pSampler0 = nullptr;
	r_pSimBuffer = nullptr;
	r_pSimBufferView = nullptr;
	r_pPS = nullptr;
	r_pNoiseSRV = nullptr;
	r_pStreakSRV = nullptr;
	r_pUAV = nullptr;
	r_pVS_Height = nullptr;
	r_pVS_Out = nullptr;
	//r_vBoundCenter = XMVectorZero();
	//r_vBoundHalfSize = XMVectorZero();
#pragma endregion

}

HRESULT RainCompute::Initialize()
{
	HRESULT hr;

	//init sampler state
	D3D11_SAMPLER_DESC sampler_desc;
	ZeroMemory(&sampler_desc, sizeof(sampler_desc));
	sampler_desc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	sampler_desc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	sampler_desc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	sampler_desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	hr = r_pDevice->CreateSamplerState(&sampler_desc, &r_pSampler0);
	if (FAILED(hr)) return hr;

	D3D11_RASTERIZER_DESC rasterizer_desc;
	ZeroMemory(&rasterizer_desc, sizeof(rasterizer_desc));
	rasterizer_desc.CullMode = D3D11_CULL_NONE;
	rasterizer_desc.FillMode = D3D11_FILL_SOLID;

	hr = r_pDevice->CreateRasterizerState(&rasterizer_desc, &r_pRasterState);
	if (FAILED(hr)) return hr;
	const char	c_szName[] = "this is not my fault";
	r_pRasterState->SetPrivateData(WKPDID_D3DDebugObjectName, sizeof(c_szName) - 1, c_szName);

	//init vertex buffer
	D3D11_BUFFER_DESC vertexbuffer_desc;
	ZeroMemory(&vertexbuffer_desc, sizeof(vertexbuffer_desc));
	vertexbuffer_desc.Usage = D3D11_USAGE_DEFAULT;
	vertexbuffer_desc.BindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;
	vertexbuffer_desc.StructureByteStride = sizeof(RainDrop);
	vertexbuffer_desc.ByteWidth = g_iMaxRainDrop * sizeof(RainDrop);
	vertexbuffer_desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;

	//init all values in buffer to low values for first update
	RainDrop arrInitSim[g_iMaxRainDrop];
	ZeroMemory(arrInitSim, sizeof(arrInitSim));
	for(int i = 0; i < g_iMaxRainDrop; i++)
	{
		arrInitSim[i].Pos = XMFLOAT3(0.0f, -1000.0f, 0.0f);
		arrInitSim[i].Vel = XMFLOAT3(0.0f, -9.82f, 0.0f);
		arrInitSim[i].State = 0.0f;
	}

	D3D11_SUBRESOURCE_DATA InitData;
	InitData.pSysMem = arrInitSim;

	hr = r_pDevice->CreateBuffer(&vertexbuffer_desc, &InitData, &r_pSimBuffer);
	if (FAILED(hr)) return hr;

	//create shader resource view for compute
	D3D11_SHADER_RESOURCE_VIEW_DESC descView;
	ZeroMemory(&descView, sizeof(descView));
	descView.ViewDimension = D3D11_SRV_DIMENSION_BUFFEREX;
	descView.BufferEx.FirstElement = 0;
	descView.Format = DXGI_FORMAT_UNKNOWN;
	descView.BufferEx.NumElements = g_iMaxRainDrop;
	hr = r_pDevice->CreateShaderResourceView(r_pSimBuffer, &descView, &r_pSimBufferView);
	if (FAILED(hr)) return hr;

	//init UAV
	D3D11_UNORDERED_ACCESS_VIEW_DESC uav_desc;
	ZeroMemory(&uav_desc, sizeof(uav_desc));
	uav_desc.Format = DXGI_FORMAT_UNKNOWN;
	uav_desc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
	uav_desc.Buffer.FirstElement = 0;
	uav_desc.Buffer.NumElements = g_iMaxRainDrop;

	hr = r_pDevice->CreateUnorderedAccessView(r_pSimBuffer, &uav_desc, &r_pUAV);
	if (FAILED(hr)) return hr;

	//init depth stencil view
	D3D11_TEXTURE2D_DESC texture2d_desc;
	ZeroMemory(&texture2d_desc, sizeof(texture2d_desc));
	texture2d_desc.Height = g_iHeightMapSize;
	texture2d_desc.Width = g_iHeightMapSize;
	texture2d_desc.MipLevels = 1;
	texture2d_desc.ArraySize = 1;
	texture2d_desc.Format = DXGI_FORMAT_R32_TYPELESS;
	texture2d_desc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
	texture2d_desc.Usage = D3D11_USAGE_DEFAULT;
	texture2d_desc.SampleDesc.Count = 1;
	texture2d_desc.SampleDesc.Quality = 0;
	hr = r_pDevice->CreateTexture2D(&texture2d_desc, nullptr, &r_pHeightMap2D);
	if (FAILED(hr)) return hr;

	D3D11_DEPTH_STENCIL_VIEW_DESC dsv_desc;
	ZeroMemory(&dsv_desc, sizeof(dsv_desc));
	dsv_desc.Format = DXGI_FORMAT_D32_FLOAT;
	dsv_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;

	hr = r_pDevice->CreateDepthStencilView(r_pHeightMap2D, &dsv_desc, &r_pHeightMapDSV);
	if(FAILED(hr))	return hr;
	

	//init shaderresourceview for sampling during simulation
	D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc;
	ZeroMemory(&srv_desc, sizeof(srv_desc));
	srv_desc.Format = DXGI_FORMAT_R32_FLOAT;
	srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srv_desc.Texture2D.MipLevels = 1;
	srv_desc.Texture2D.MostDetailedMip = 0;
	hr = r_pDevice->CreateShaderResourceView(r_pHeightMap2D, &srv_desc, &r_pHeightMapSRV);
	if (FAILED(hr)) return hr;

	//init CBDepth
	D3D11_BUFFER_DESC cb_desc;
	ZeroMemory(&cb_desc, sizeof(cb_desc));
	cb_desc.Usage = D3D11_USAGE_DYNAMIC;
	cb_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	cb_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	cb_desc.ByteWidth = 64;

	hr = r_pDevice->CreateBuffer(&cb_desc, nullptr, &r_pCBDepth);
	if (FAILED(hr)) return hr;

	//init CBSimulate
	cb_desc.ByteWidth = 112;

	hr = r_pDevice->CreateBuffer(&cb_desc, nullptr, &r_pCBSimulate);
	if (FAILED(hr)) return hr;

	//init CBRenderer
	cb_desc.ByteWidth = 96;
	
	hr = r_pDevice->CreateBuffer(&cb_desc, nullptr, &r_pCBRenderer);
	if (FAILED(hr)) return hr;

	//init alpha blending (may be scrapped from snow)
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

	hr = r_pDevice->CreateBlendState(&blend_desc, &r_pBlendRender);
	if (FAILED(hr)) return hr;

	//load noise and rain streak texture
	hr = D3DX11CreateShaderResourceViewFromFile(r_pDevice, "assets/noise.png", nullptr, nullptr, &r_pNoiseSRV, nullptr);
	//hr = D3DX11CreateShaderResourceViewFromFile(r_pDevice, "assets/WhiteNoiseDithering.png", nullptr, nullptr, &r_pNoiseSRV, nullptr);
	//hr = D3DX11CreateShaderResourceViewFromFile(r_pDevice,"assets/texture3.jpg", nullptr, nullptr, &r_pNoiseSRV, nullptr);
	//hr = D3DX11CreateShaderResourceViewFromFile(r_pDevice, "assets/noise.dds", nullptr, nullptr, &r_pNoiseSRV, nullptr);

	if (FAILED(hr)) return hr;
	//hr = D3DX11CreateShaderResourceViewFromFile(r_pDevice, "assets/texture0.jpg", nullptr, nullptr, &r_pStreakSRV, nullptr);
	hr = D3DX11CreateShaderResourceViewFromFile(r_pDevice, "assets/StreakTexture.png",  nullptr, nullptr, &r_pStreakSRV, nullptr);
	if (FAILED(hr)) return hr;

	//compile shader

	ID3DBlob *HVS, *OVS, *CS, *PS, *error;	

	hr = D3DX11CompileFromFile("HeightMapShader.hlsl", nullptr, nullptr, "HeightMapVS", "vs_5_0", 0, 0, nullptr, &HVS, &error, nullptr);
	if (error != 0)
	{
		OutputDebugStringA((char*)error->GetBufferPointer());
		error->Release();
		if (FAILED(hr))
		{
			return hr;
		}
	}

	hr = D3DX11CompileFromFile("RainShaders.hlsl", nullptr, nullptr, "VS_Rain", "vs_5_0", 0, 0, nullptr, &OVS, &error, nullptr);
	if (error != 0)
	{
		OutputDebugStringA((char*)error->GetBufferPointer());
		error->Release();
		if (FAILED(hr))
		{
			return hr;
		}
	}

	hr = D3DX11CompileFromFile("RainShaders.hlsl", nullptr, nullptr, "PS_Rain", "ps_5_0", 0, 0, nullptr, &PS, &error, nullptr);
	if (error != 0)
	{
		OutputDebugStringA((char*)error->GetBufferPointer());
		error->Release();
		if (FAILED(hr))
		{
			return hr;
		}
	}

	hr = D3DX11CompileFromFile("RainSimulationCS.hlsl", nullptr, nullptr, "SimulateRain", "cs_5_0", 0, 0, nullptr, &CS, &error, nullptr);
	if (error != 0)
	{
		OutputDebugStringA((char*)error->GetBufferPointer());
		error->Release();
		if (FAILED(hr))
		{
			return hr;
		}
	}

	//load shader
	hr = r_pDevice->CreateVertexShader(HVS->GetBufferPointer(), HVS->GetBufferSize(), nullptr, &r_pVS_Height);
	if (FAILED(hr)) return hr;

	hr = r_pDevice->CreateVertexShader(OVS->GetBufferPointer(), OVS->GetBufferSize(), nullptr, &r_pVS_Out);
	if (FAILED(hr)) return hr;

	hr = r_pDevice->CreateComputeShader(CS->GetBufferPointer(), CS->GetBufferSize(), nullptr, &r_pCS);
	if (FAILED(hr)) return hr;

	hr = r_pDevice->CreatePixelShader(PS->GetBufferPointer(), PS->GetBufferSize(), nullptr, &r_pPS);
	if (FAILED(hr)) return hr;

	//init input layout
	const D3D11_INPUT_ELEMENT_DESC layout[]=
	{
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
	};
	hr = r_pDevice->CreateInputLayout(layout, ARRAYSIZE(layout), HVS->GetBufferPointer(), HVS->GetBufferSize(), &r_pInputLayout);
	if (FAILED(hr)) return hr;
	

	return S_OK;
}

void RainCompute::Draw()
{
	Render();	
}

void RainCompute::HeighMapPass(ID3D11RenderTargetView* defaultRTV, ID3D11DepthStencilView* defaultDSV, D3D11_VIEWPORT* viewport)
{
	PreRender();
	r_pContext->OMSetRenderTargets(1, &defaultRTV, defaultDSV);
	r_pContext->RSSetViewports(1, viewport);
}

void RainCompute::HeightMapPrep()
{
	D3D11_VIEWPORT vp[1] = { {0, 0, g_iHeightMapSize, g_iHeightMapSize, 0.0f, 1.0f} };
	r_pContext->RSSetViewports(1, vp);

	UpdateTransforms();

	//clear height map
	r_pContext->ClearDepthStencilView(r_pHeightMapDSV, D3D11_CLEAR_DEPTH, 1.0f, 0);

	//set the height map for rendering
	ID3D11RenderTargetView* nullView = nullptr;
	r_pContext->OMSetRenderTargets(1, &nullView, r_pHeightMapDSV);
	//fill the height generation matrix
	D3D11_MAPPED_SUBRESOURCE mapped;
	r_pContext->Map(r_pCBDepth, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
	HeightCB* heightConstants = (HeightCB*)mapped.pData;
	XMMATRIX forpointer = XMMatrixTranspose(r_mRainViewProj);
	heightConstants->ToHeight = forpointer;
	r_pContext->Unmap(r_pCBDepth, 0);
	r_pContext->VSSetConstantBuffers(0, 1, &r_pCBDepth);

	//set vertex layout
	r_pContext->IASetInputLayout(r_pInputLayout);

	//set shaders
	r_pContext->VSSetShader(r_pVS_Height, nullptr, 0);
	r_pContext->PSSetShader(nullptr, nullptr, 0);
}	

void RainCompute::PreRender()
{
	//Unbind the height map
	ID3D11RenderTargetView* nullView = nullptr;
	r_pContext->OMSetRenderTargets(1, &nullView, nullptr);

	//Constants
	D3D11_MAPPED_SUBRESOURCE mapped;
	r_pContext->Map(r_pCBSimulate, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
	RainCB* pSimulationConstants = (RainCB*)mapped.pData;

	pSimulationConstants->HeightSpace = XMMatrixTranspose(r_mRainViewProj);
	pSimulationConstants->BoundCenter.x = r_vBoundCenter.x;
	pSimulationConstants->BoundCenter.y = r_vBoundCenter.y;
	pSimulationConstants->BoundCenter.z = r_vBoundCenter.z;
	pSimulationConstants->deltaTime = r_pGameTimer->DeltaTime();
	pSimulationConstants->WindVariation = 0.02f;
	pSimulationConstants->BoundHalfSize.x = r_vBoundHalfSize.x;
	pSimulationConstants->BoundHalfSize.y = r_vBoundHalfSize.y;
	pSimulationConstants->BoundHalfSize.z = r_vBoundHalfSize.z;

	//update the wind effect
	float randX = (float)(rand() % 101)*0.02f - 1.0f;
	r_vCurWindEffect.x += randX * r_fMaxWindVariance * r_pGameTimer->DeltaTime();
	if (r_vCurWindEffect.x > r_fMaxWindVariance)
		r_vCurWindEffect.x = r_fMaxWindVariance;
	else if (r_vCurWindEffect.x < -r_fMaxWindVariance)
		r_vCurWindEffect.x = -r_fMaxWindVariance;

	float randY = (float)(rand() % 101) * 0.02f - 1.0f;
	r_vCurWindEffect.y += randY * r_fMaxWindVariance * r_pGameTimer->DeltaTime();
	if (r_vCurWindEffect.y > r_fMaxWindVariance)
		r_vCurWindEffect.y = r_fMaxWindVariance;
	else if (r_vCurWindEffect.y < -r_fMaxWindVariance)
		r_vCurWindEffect.y = -r_fMaxWindVariance;

	pSimulationConstants->WindForce.x = r_vCurWindEffect.x;
	pSimulationConstants->WindForce.y = r_vCurWindEffect.y;
	pSimulationConstants->VertSpeed = r_fVerticalSpeed + (float)(rand() % 101)*0.002f - 1.0f;
	pSimulationConstants->HeightMapSize = (float)g_iHeightMapSize;
	r_pContext->Unmap(r_pCBSimulate, 0);
	r_pContext->CSSetConstantBuffers(0, 1, &r_pCBSimulate);

	//Output
	ID3D11UnorderedAccessView* arrUAV[1] = {r_pUAV};
	r_pContext->CSSetUnorderedAccessViews(0, 1, arrUAV, nullptr);

	//Input
	ID3D11ShaderResourceView* arrViews[2] = { r_pNoiseSRV, r_pHeightMapSRV };
	r_pContext->CSSetShaderResources(0, 2, arrViews);

	//Shader
	r_pContext->CSSetShader(r_pCS, nullptr, 0);

	//Execute the compute shader
	//Dispatch groups of 32x32 threads to simulate all the rain drops
	//r_pContext->Dispatch(g_iNumRainGroupSize, g_iNumRainGroupSize, 1);
	r_pContext->Dispatch(32, 32, 1);

	//clean the compute shader data
	r_pContext->CSSetShader(nullptr, nullptr, 0);
	ZeroMemory(arrViews, sizeof(arrViews));
	r_pContext->CSSetShaderResources(0, 2, arrViews);
	arrUAV[0] = nullptr;
	r_pContext->CSSetUnorderedAccessViews(0, 1, arrUAV, nullptr);
}

void RainCompute::Render()
{
	//grab the old raster state
	ID3D11RasterizerState* default_state;
	r_pContext->RSGetState(&default_state);

	//set the output view
	r_pContext->VSSetShaderResources(0, 1, &r_pSimBufferView);
	r_pContext->PSSetShaderResources(0, 1, &r_pStreakSRV);
	
	//update the constant buffer for draw
	D3D11_MAPPED_SUBRESOURCE mapped;
	r_pContext->Map(r_pCBRenderer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
	DrawCB* pDrawConsts = (DrawCB*)mapped.pData;
	XMMATRIX mViewProj = XMMatrixMultiply(r_pCamera->GetViewMatrix(), r_pCamera->GetProjMatrix());
	pDrawConsts->ViewProj = XMMatrixTranspose(mViewProj);
	XMVECTOR dir = XMVector3Normalize(r_pCamera->GetLookAt());
	XMStoreFloat3(&pDrawConsts->ViewDir, dir);
	pDrawConsts->scale = r_fStreakScale;
	pDrawConsts->AmbientColor = XMFLOAT4(0.4f, 0.4f, 0.4f, 0.5f);
	r_pContext->Unmap(r_pCBRenderer, 0);
	
	//blend state setting
	r_pContext->OMSetBlendState(r_pBlendRender, nullptr, 0xffffffff);
	
	//set shaders
	r_pContext->VSSetShader(r_pVS_Out, nullptr, 0);
	r_pContext->GSSetShader(nullptr, nullptr, 0);
	r_pContext->PSSetShader(r_pPS, nullptr, 0);

	//setting the input layout and vertex buffer to null and primitive topology
	r_pContext->IASetInputLayout(nullptr);
	r_pContext->IASetVertexBuffers(0, 0, nullptr, nullptr, nullptr);
	r_pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	//set the shader data
	r_pContext->VSSetConstantBuffers(0, 1, &r_pCBRenderer);
	r_pContext->PSSetConstantBuffers(0, 1, &r_pCBRenderer);

	r_pContext->PSSetSamplers(0, 1, &r_pSampler0);
	//CHANGE LATER
	r_pContext->RSSetState(r_pRasterState);

	//call draw with amount of raindrops six times
	int TotalDrops = (int)((float)g_iMaxRainDrop* r_fDensity);
	r_pContext->Draw(TotalDrops * 6, 0);//Draw the whole buffer

	//Clean up
	r_pContext->VSSetShader(nullptr, nullptr, 0);
	r_pContext->PSSetShader(nullptr, nullptr, 0);
	ID3D11ShaderResourceView* arrSRV[1];
	arrSRV[0] = NULL;
	r_pContext->VSSetShaderResources(0, 1, arrSRV);
	r_pContext->PSSetShaderResources(0, 1, arrSRV);

	//reset blend state
	r_pContext->OMSetBlendState(r_pBlendDefault, nullptr, 0xffffffff);
	r_pContext->RSSetState(default_state);
}

void RainCompute::UpdateTransforms()
{
	 
	//Change simulation data based upon the scene camera
	XMVECTOR vCamPos = r_pCamera->GetCameraPosition();
	XMVECTOR vCamDir = r_pCamera->GetLookAt();
	vCamDir.y = 0.0f;
	vCamDir = XMVector3Normalize(vCamDir);
	XMVECTOR vOffset = XMVectorSet(vCamDir.x * r_vBoundHalfSize.x, 0.0f, vCamDir.z * r_vBoundHalfSize.z, 1.0f);
	r_vBoundCenter = vCamPos + vOffset * 0.8f; //keep 20 percent behind the camera
	//r_vBoundCenter = vCamPos + vOffset * 0.2f;

	
	//build the view matrix for the rain volume
	XMVECTOR vEye = r_vBoundCenter;
	vEye.y +=  r_vBoundHalfSize.y;
	XMVECTOR vAt = r_vBoundCenter;
	XMVECTOR vUp = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
	r_mRainView = XMMatrixLookAtLH(vEye, vAt, vUp);
	/*
	XMVECTOR vEye = XMVectorZero();
	vEye.y += r_vBoundHalfSize.y;
	XMVECTOR vAt = XMVectorZero();
	XMVECTOR vUp = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
	r_mRainView = XMMatrixLookAtLH(vEye, vAt, vUp);
*/
	//build the projection matrix
	r_mRainProj = XMMatrixOrthographicOffCenterLH(-r_vBoundHalfSize.x, r_vBoundHalfSize.x, -r_vBoundHalfSize.y, r_vBoundHalfSize.y, 0.0f, 2.0f* r_vBoundHalfSize.z);

	//calculate the the transformation matrix
	r_mRainViewProj = XMMatrixMultiply(r_mRainView, r_mRainProj);
}

XMMATRIX RainCompute::GetView()
{
	return r_mRainView;
}

XMMATRIX RainCompute::GetProj()
{
	return r_mRainProj;
}


RainCompute::~RainCompute()
{
	if (r_pHeightLayout) r_pHeightLayout->Release();
	if (r_pVS_Height) r_pVS_Height->Release();
	if (r_pCBDepth) r_pCBDepth->Release();
	if (r_pCS) r_pCS->Release();
	if (r_pCBSimulate) r_pCBSimulate->Release();
	if (r_pNoiseSRV) r_pNoiseSRV->Release();
	if (r_pHeightMapSRV) r_pHeightMapSRV->Release();
	if (r_pHeightMapDSV) r_pHeightMapDSV->Release();
	if (r_pUAV) r_pUAV->Release();
	if (r_pSimBufferView) r_pSimBufferView->Release();
	if (r_pStreakSRV) r_pStreakSRV->Release();
	if (r_pSimBuffer) r_pSimBuffer->Release();
	if (r_pCBRenderer) r_pCBRenderer->Release();
	if (r_pBlendRender) r_pBlendRender->Release();
	if (r_pBlendDefault) r_pBlendDefault->Release();
	if (r_pVS_Out) r_pVS_Out->Release();
	if (r_pPS) r_pPS->Release();
	if (r_pContext) r_pContext = nullptr;
	if (r_pDevice) r_pDevice = nullptr;
}

void RainCompute::SetDensity(float d)
{
	r_fDensity = d;
}

ID3D11ShaderResourceView* RainCompute::CreateRandomTexture1DSRV(ID3D11Device* device)
{
	//Create random data
	XMFLOAT4 randomValues[1024];
	for (int i = 0; i < 1024; i++)
	{
		randomValues[i].x = RandF(-1.0f, 1.0f);
		randomValues[i].y = RandF(-1.0f, 1.0f);
		randomValues[i].z = RandF(-1.0f, 1.0f);
		randomValues[i].w = RandF(-1.0f, 1.0f);
	}

	D3D11_SUBRESOURCE_DATA init;
	init.pSysMem = randomValues;
	init.SysMemPitch = 1024 * sizeof(XMFLOAT4);
	init.SysMemSlicePitch = 0;

	//create texture
	D3D11_TEXTURE1D_DESC  texDesc;
	texDesc.Width = 1024;
	texDesc.MipLevels = 1;
	texDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	texDesc.Usage = D3D11_USAGE_IMMUTABLE;
	texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	texDesc.CPUAccessFlags = 0;
	texDesc.MiscFlags = 0;
	texDesc.ArraySize = 1;

	ID3D11Texture1D* randomTex = 0;
	device->CreateTexture1D(&texDesc, &init, &randomTex);

	//create the resource view
	D3D11_SHADER_RESOURCE_VIEW_DESC view;
	view.Format = texDesc.Format;
	view.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE1D;
	view.Texture1D.MipLevels = texDesc.MipLevels;
	view.Texture1D.MostDetailedMip = 0;

	ID3D11ShaderResourceView* randomTexSRV = 0;
	device->CreateShaderResourceView(randomTex, &view, &randomTexSRV);

	randomTex->Release();

	return randomTexSRV;
}

//Returns a random value [0,1]
float RainCompute::RandF()
{
	return (float)(rand()) / (float)RAND_MAX;
}

float RainCompute::RandF(float a, float b) 
{
	return a + RandF() * (b - a);
}