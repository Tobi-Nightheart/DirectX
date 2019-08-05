#include "pch.h"
#include "SnowTexture.h"
#include <minwinbase.h>

//so we need to add a buffer storing vertices, that gets them from an object model
//how do we collect the data form dynamic objects?
//finally it will be helpful to install NVsight to debug the rain and snow texture shaders

SnowTexture::SnowTexture(ID3D11Device* device, ID3D11DeviceContext* context, camera* c, GameTimer* gt)
{
	s_pDevice = device;
	s_pContext = context;
	s_pCamera = c;
	s_pGameTimer = gt;

#pragma region NullInits
	s_pinitTexCS = nullptr;
	s_pVertexBuffer = nullptr;
	s_pTessellationCB = nullptr;
	s_pSnowVS = nullptr;
	s_pSnowDS = nullptr;
	s_pSnowHS = nullptr;
	s_pSnowPS = nullptr;
	s_pDeformCB = nullptr;
	s_pDeformationCS = nullptr;
	s_pDeformationHeightMapSRV = nullptr;
	s_pDeformationHeightMapUAV = nullptr;
	s_pDeformedTexSRV = nullptr;
	s_pFillCB = nullptr;
	s_pFillCS = nullptr;
	s_pInputLayout = nullptr;
	s_pLinearSampler = nullptr;
	s_pMaterialCB = nullptr;
	s_pMaterialTexSRV = nullptr;
	s_pObject = nullptr;
	s_pRaster = nullptr;
	s_aVerticies[0] = &XMFLOAT3(1.0f, 1.0f, 1.0f);
	
#pragma endregion


}

HRESULT SnowTexture::Initialize()
{
	HRESULT hr;

	s_pObject = new Model(s_pDevice, s_pContext);
	hr = s_pObject->LoadObjModel((char*)"Resources/snowplane.obj");
	if (FAILED(hr)) return hr;

	hr = D3DX11CreateShaderResourceViewFromFile(s_pDevice, "assets/snowtex.jpg", nullptr, nullptr, &s_pMaterialTexSRV, nullptr);
	if (FAILED(hr)) return hr;

	hr = D3DX11CreateShaderResourceViewFromFile(s_pDevice, "assets/texture1.jpg", nullptr, nullptr, &s_pDeformedTexSRV, nullptr);
	if (FAILED(hr)) return hr;

	//Creating height map
	D3D11_TEXTURE2D_DESC Texture_desc;
	ZeroMemory(&Texture_desc, sizeof(Texture_desc));
	Texture_desc.Height = 1024;
	Texture_desc.Width = 1024;
	Texture_desc.BindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;
	Texture_desc.Format = DXGI_FORMAT_R16G16_TYPELESS;
	Texture_desc.Usage = D3D11_USAGE_DEFAULT;
	Texture_desc.MipLevels = 1;
	Texture_desc.SampleDesc.Count = 1;
	Texture_desc.SampleDesc.Quality = 0;
	Texture_desc.ArraySize = 1;
	
	ID3D11Texture2D* HeightTexture;
	hr = s_pDevice->CreateTexture2D(&Texture_desc, nullptr, &HeightTexture);
	if (FAILED(hr)) return hr;

	D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc;
	ZeroMemory(&srv_desc, sizeof(srv_desc));
	srv_desc.Format = DXGI_FORMAT_R16G16_UNORM;
	srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srv_desc.Texture2D.MostDetailedMip = 0;
	srv_desc.Texture2D.MipLevels = 1;

	hr = s_pDevice->CreateShaderResourceView(HeightTexture, &srv_desc, &s_pDeformationHeightMapSRV);
	if (FAILED(hr)) return hr;

	D3D11_UNORDERED_ACCESS_VIEW_DESC uav_desc;
	ZeroMemory(&uav_desc, sizeof(uav_desc));
	uav_desc.Format = DXGI_FORMAT_R32_UINT;
	uav_desc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;

	hr = s_pDevice->CreateUnorderedAccessView(HeightTexture, &uav_desc, &s_pDeformationHeightMapUAV);
	if (FAILED(hr)) return hr;


	//Sampler init
	D3D11_SAMPLER_DESC sampler_desc;
	ZeroMemory(&sampler_desc, sizeof(sampler_desc));
	sampler_desc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	sampler_desc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	sampler_desc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	sampler_desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	hr = s_pDevice->CreateSamplerState(&sampler_desc, &s_pLinearSampler);
	if (FAILED(hr)) return hr;

	//Initializing constant buffers for compute
	//deformation cb
	D3D11_BUFFER_DESC cb_desc;
	ZeroMemory(&cb_desc, sizeof(cb_desc));
	cb_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	cb_desc.ByteWidth = 16;
	cb_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	cb_desc.Usage = D3D11_USAGE_DYNAMIC;

	hr = s_pDevice->CreateBuffer(&cb_desc, nullptr, &s_pDeformCB);
	if (FAILED(hr)) return hr;

	//fill cb
	cb_desc.ByteWidth = 16;

	hr = s_pDevice->CreateBuffer(&cb_desc, nullptr, &s_pFillCB);
	if (FAILED(hr)) return hr;

	//Initialize dynamic object point array
	XMFLOAT3 verticies[10] = 
	{
		XMFLOAT3(0.0f, 15.0f, 0.0f),
		XMFLOAT3(0.0f, 15.0f, 0.0f),
		XMFLOAT3(0.0f, 15.0f, 0.0f),
		XMFLOAT3(0.0f, 15.0f, 0.0f),
		XMFLOAT3(0.0f, 15.0f, 0.0f),

		XMFLOAT3(0.0f, 15.0f, 0.0f),
		XMFLOAT3(0.0f, 15.0f, 0.0f),
		XMFLOAT3(0.0f, 15.0f, 0.0f),
		XMFLOAT3(0.0f, 15.0f, 0.0f),
		XMFLOAT3(0.0f, 15.0f, 0.0f),
	};

	D3D11_SUBRESOURCE_DATA vinit_data;
	vinit_data.pSysMem = verticies;

	D3D11_BUFFER_DESC vb_desc;
	ZeroMemory(&vb_desc, sizeof(vb_desc));
	vb_desc.Usage = D3D11_USAGE_DYNAMIC;
	vb_desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vb_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	vb_desc.ByteWidth = sizeof(XMFLOAT3)*10;

	hr = s_pDevice->CreateBuffer(&vb_desc, &vinit_data, &s_pVertexBuffer);
	if (FAILED(hr)) return hr;

	//tessellation buffers
	D3D11_BUFFER_DESC buffer_desc;
	ZeroMemory(&buffer_desc, sizeof(buffer_desc));
	buffer_desc.Usage = D3D11_USAGE_DYNAMIC;
	buffer_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	buffer_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	buffer_desc.ByteWidth = 128+16;

	hr = s_pDevice->CreateBuffer(&buffer_desc, nullptr, &s_pTessellationCB);
	if (FAILED(hr)) return hr;

	buffer_desc.ByteWidth = 80;
	hr = s_pDevice->CreateBuffer(&buffer_desc, nullptr, &s_pMaterialCB);
	if (FAILED(hr)) return hr;


	//Initialize the shader stages (Vertex, Hull, Domain, Pixel)
	ID3DBlob *VS, *HS, *DS, *PS, *error;

	hr = D3DX11CompileFromFile("SnowShaders.hlsl", nullptr, nullptr, "SnowVS", "vs_5_0", 0, 0, nullptr, &VS, &error, nullptr);
	if(error!= nullptr)
	{
		OutputDebugStringA((char*)error->GetBufferPointer());
		error->Release();
		if (FAILED(hr)) return hr;
	}
	hr = D3DX11CompileFromFile("SnowShaders.hlsl", nullptr, nullptr, "SnowHS", "hs_5_0", 0, 0, nullptr, &HS, &error, nullptr);
	if(error!=nullptr)
	{
		OutputDebugStringA((char*)error->GetBufferPointer());
		error->Release();
		if (FAILED(hr)) return hr;
	}
	hr = D3DX11CompileFromFile("SnowShaders.hlsl", nullptr, nullptr, "DS", "ds_5_0", 0, 0, nullptr, &DS, &error, nullptr);
	if(error!=nullptr)
	{
		OutputDebugStringA((char*)error->GetBufferPointer());
		error->Release();
		if (FAILED(hr)) return hr;
	}
	hr = D3DX11CompileFromFile("SnowShaders.hlsl", nullptr, nullptr, "SnowPS", "ps_5_0", 0, 0, nullptr, &PS, &error, nullptr);
	if (error != nullptr)
	{
		OutputDebugStringA((char*)error->GetBufferPointer());
		error->Release();
		if (FAILED(hr)) return hr;
	}

	//create the shader stages
	hr = s_pDevice->CreateVertexShader(VS->GetBufferPointer(), VS->GetBufferSize(), nullptr, &s_pSnowVS);
	if (FAILED(hr)) return hr;
	hr =s_pDevice->CreateHullShader(HS->GetBufferPointer(), HS->GetBufferSize(), nullptr, &s_pSnowHS);
	if (FAILED(hr)) return hr;
	hr = s_pDevice->CreateDomainShader(DS->GetBufferPointer(), DS->GetBufferSize(), nullptr, &s_pSnowDS);
	if (FAILED(hr)) return hr;
	hr = s_pDevice->CreatePixelShader(PS->GetBufferPointer(), PS->GetBufferSize(), nullptr, &s_pSnowPS);
	if (FAILED(hr)) return hr;

	D3D11_INPUT_ELEMENT_DESC iedesc[] = 
	{
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
	};
	hr = s_pDevice->CreateInputLayout(iedesc, 3, VS->GetBufferPointer(), VS->GetBufferSize(), &s_pInputLayout);
	if (FAILED(hr)) return hr;

	D3D11_RASTERIZER_DESC raster_desc;
	ZeroMemory(&raster_desc, sizeof(raster_desc));
	raster_desc.FillMode = D3D11_FILL_WIREFRAME;
	raster_desc.CullMode = D3D11_CULL_NONE;

	hr = s_pDevice->CreateRasterizerState(&raster_desc, &s_pRaster);
	if (FAILED(hr)) return hr;

	//Compile Compute Shaders
	ID3DBlob *DeformCS, *FillCS, *initTexCS;

	hr = D3DX11CompileFromFile("DeformationCS.hlsl", nullptr, nullptr, "Deformation", "cs_5_0", 0, 0, nullptr, &DeformCS, &error, nullptr);
	if (error != nullptr) {
		OutputDebugStringA((char*)error->GetBufferPointer());
		error->Release();
		if (FAILED(hr)) return hr;
	}

	hr = D3DX11CompileFromFile("FillCS.hlsl", nullptr, nullptr, "FillShader", "cs_5_0", 0, 0, nullptr, &FillCS, &error, nullptr);
	if (error != nullptr) {
		OutputDebugStringA((char*)error->GetBufferPointer());
		error->Release();
		if (FAILED(hr)) return hr;
	}

	hr = D3DX11CompileFromFile("initTexCS.hlsl", nullptr, nullptr, "Init", "cs_5_0", 0, 0, nullptr, &initTexCS, &error, nullptr);
	if (error != nullptr) {
		OutputDebugStringA((char*)error->GetBufferPointer());
		error->Release();
		if (FAILED(hr)) return hr;
	}

	//Create compute shader stages
	hr = s_pDevice->CreateComputeShader(DeformCS->GetBufferPointer(), DeformCS->GetBufferSize(), nullptr, &s_pDeformationCS);
	if (FAILED(hr)) return hr;

	hr = s_pDevice->CreateComputeShader(FillCS->GetBufferPointer(), FillCS->GetBufferSize(), nullptr, &s_pFillCS);
	if (FAILED(hr)) return hr;

	hr = s_pDevice->CreateComputeShader(initTexCS->GetBufferPointer(), initTexCS->GetBufferSize(), nullptr, &s_pinitTexCS);
	if (FAILED(hr)) return hr;

	InitTex(s_pDevice, s_pContext);


	return S_OK;
}

void SnowTexture::Draw(XMMATRIX* world, XMMATRIX* view, XMMATRIX* proj, bool raster)
{
	//set input layout
	s_pContext->IASetInputLayout(s_pInputLayout);

	//Set shader stages
	s_pContext->VSSetShader(s_pSnowVS, nullptr, 0);
	s_pContext->HSSetShader(s_pSnowHS, nullptr, 0);
	s_pContext->DSSetShader(s_pSnowDS, nullptr, 0);
	s_pContext->PSSetShader(s_pSnowPS, nullptr, 0);
	
	//set shader constant buffers
	//VS
	s_pContext->VSSetConstantBuffers(0, 1, &s_pTessellationCB);
	s_pContext->VSSetConstantBuffers(1, 1, &s_pMaterialCB);
	//HS + DS
	s_pContext->HSSetConstantBuffers(0, 1, &s_pTessellationCB);
	s_pContext->DSSetConstantBuffers(0, 1, &s_pTessellationCB);
	s_pContext->DSSetConstantBuffers(1, 1, &s_pMaterialCB);

	//set shader resources
	//for DS
	s_pContext->DSSetShaderResources(2, 1, &s_pDeformationHeightMapSRV);
	//for PS
	s_pContext->PSSetShaderResources(0, 1, &s_pMaterialTexSRV);
	s_pContext->PSSetShaderResources(1, 1, &s_pDeformedTexSRV);

	//set input topology
	s_pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST);
	
	//set sampler states
	s_pContext->DSSetSamplers(0, 1, &s_pLinearSampler);
	s_pContext->PSSetSamplers(0, 1, &s_pLinearSampler);

	//update constant buffers
	D3D11_MAPPED_SUBRESOURCE source;
	s_pContext->Map(s_pTessellationCB, 0, D3D11_MAP_WRITE_DISCARD, 0, &source);
	TessellationCB* TSCB = (TessellationCB*) source.pData;
	TSCB->mWorld = *world;
	TSCB->mWVP = (*world) * (*view) * (*proj);
	s_pContext->Unmap(s_pTessellationCB, 0);

	s_pContext->Map(s_pMaterialCB, 0, D3D11_MAP_WRITE_DISCARD, 0, &source);
	MaterialCB* MCB = (MaterialCB*)source.pData;
	MCB->CamPos = XMFLOAT4(s_pCamera->GetCameraPosition().x, s_pCamera->GetCameraPosition().y, s_pCamera->GetCameraPosition().z, s_pCamera->GetCameraPosition().w);
	MCB->LightPos = XMFLOAT4(2.0f, 3.0f, 1.0f, 0.0f);
	MCB->MatAmbC = XMFLOAT4(0.25f, 0.25f, 0.25f, 1.0f);
	MCB->MatDifC = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
	MCB->fBaseTextureRepeat = XMFLOAT4(1.0f, 0, 0, 0);
	s_pContext->Unmap(s_pMaterialCB, 0);

	
	//check if wireframe is used
	if (raster) 
	{
		//set wireframe rasterizer
		ID3D11RasterizerState* default_raster;
		s_pContext->RSGetState(&default_raster);
		s_pContext->RSSetState(s_pRaster);

		//draw the object
		ObjFileModel* obj;
		obj = s_pObject->GetObjectA();
		obj->Draw();

		//restore rasterizer state
		s_pContext->RSSetState(default_raster);
		default_raster->Release();
	}
	else
	{
		ObjFileModel* obj;
		obj = s_pObject->GetObjectA();
		obj->Draw();
	}
	
	
	//clean up tessellation 
	s_pContext->HSSetShader(nullptr, nullptr, 0);
	s_pContext->DSSetShader(nullptr, nullptr, 0);

	//clean up draw shaders
	s_pContext->CSSetShader(nullptr, nullptr, 0);

	s_pContext->VSSetShader(nullptr, nullptr, 0);
	s_pContext->GSSetShader(nullptr, nullptr, 0);
	s_pContext->PSSetShader(nullptr, nullptr, 0);
	
	//clean SRVs
	ID3D11ShaderResourceView* arrSRV[1] = { nullptr };
	s_pContext->DSSetShaderResources(2, 1, arrSRV);
	s_pContext->PSSetShaderResources(0, 1, arrSRV);
	s_pContext->PSSetShaderResources(1, 1, arrSRV);
}


SnowTexture::~SnowTexture()
{
}

void SnowTexture::FillSnow(ID3D11Device * device, ID3D11DeviceContext * context, GameTimer * gt, float rate)
{
	//Update the fill rate for the snow
	D3D11_MAPPED_SUBRESOURCE mapped;
	context->Map(s_pFillCB, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
	FillCB* fillCB = (FillCB*)mapped.pData;

	fillCB->Fillrate = rate;
	context->Unmap(s_pFillCB, 0);
	context->CSSetConstantBuffers(0, 1, &s_pFillCB);

	//set the UAV
	ID3D11UnorderedAccessView* arrUAV[1] = { s_pDeformationHeightMapUAV };
	context->CSSetUnorderedAccessViews(0, 1, arrUAV, nullptr);

	//set the fill shader
	context->CSSetShader(s_pFillCS, nullptr, 0);

	//Dispatch 32x, 32y, 1 threads to fill the entire texture
	context->Dispatch(32, 32, 1);

	//Clean up
	context->CSSetShader(nullptr, nullptr, 0);
	arrUAV[0] = nullptr;
	context->CSSetUnorderedAccessViews(0, 1, arrUAV, nullptr);
	ID3D11Buffer* arrBuff[1] = { nullptr };
	context->CSSetConstantBuffers(0, 1, arrBuff);
	
}

void SnowTexture::CalculateDepression(ID3D11Device * device, ID3D11DeviceContext * context, GameTimer * gt , int num)
{
	D3D11_MAPPED_SUBRESOURCE mapped;
	context->Map(s_pDeformCB, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
	DeformCB* dCB = (DeformCB*)mapped.pData;

	dCB->vWorldPos = *s_aVerticies[num];
	dCB->TexCoord.x = 0.0f;
	dCB->TexCoord.y = 0.0f;
	dCB->scale = 2;
	context->Unmap(s_pDeformCB, 0);
	context->CSSetConstantBuffers(0, 1, &s_pDeformCB);

	//set UAV
	ID3D11UnorderedAccessView* arrUAV[1] = { s_pDeformationHeightMapUAV };
	context->CSSetUnorderedAccessViews(0, 1, arrUAV, nullptr);

	//set defromation shader
	context->CSSetShader(s_pDeformationCS, nullptr, 0);

	context->Dispatch(32, 32, 1);

	//clean up
	context->CSSetShader(nullptr, nullptr, 0);
	arrUAV[0] = nullptr;
	context->CSSetUnorderedAccessViews(0, 1, arrUAV, nullptr);
	ID3D11Buffer* arrBuff[1] =  { nullptr };
	context->CSSetConstantBuffers(0, 1, arrBuff);
}

void SnowTexture::InitTex(ID3D11Device* device, ID3D11DeviceContext* context) {

	//set UAV
	ID3D11UnorderedAccessView* arrUAV[1] = { s_pDeformationHeightMapUAV };
	context->CSSetUnorderedAccessViews(0, 1, arrUAV, nullptr);

	//set initialization shader
	context->CSSetShader(s_pinitTexCS, nullptr, 0);

	context->Dispatch(32, 32, 1);

	//clean up
	context->CSSetShader(nullptr, nullptr, 0);
	arrUAV[0] = nullptr;
	context->CSSetUnorderedAccessViews(0, 1, arrUAV, nullptr);
}

void SnowTexture::SetPosArray(XMFLOAT3* pos, int num)
{
	s_aVerticies[num] = pos;
}
