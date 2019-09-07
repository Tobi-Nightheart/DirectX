#pragma once
#include "pcH.h"
#include "Model.h"

using namespace DirectX;

Model::Model()
{
	m_Position = DirectX::XMVectorZero();
	m_xA = 0.0f;
	m_yA = 0.0f;
	m_zA = 0.0f;
	m_scale = 1.0f;
	m_pDevice = nullptr;
	m_pContext = nullptr;
	m_pTexture0 = nullptr;
	m_pSampler0 = nullptr;
	m_pVShaderDepth = nullptr;
	m_pVShader = nullptr;
	m_pPShader = nullptr;
	//m_pIADepth = nullptr;
	m_pInputLayout = nullptr;
	m_pRaster = nullptr;
	m_pModelCB = nullptr;
	m_pLightCB = nullptr;

	m_pObject = nullptr;
	m_BSC_Pos = DirectX::XMVectorZero();
	m_BSC_radius = 0.0f;
	TextureName = (char*) "Assets/Textures/default.jpg";
	ObjectName = (char*) "Assets/Resources/cube.obj";
	IsReflective = false;
}

Model::Model(ID3D11Device* device, ID3D11DeviceContext* context, char* ObjName, char* TexName, bool Reflective)
{
	m_Position = DirectX::XMVectorZero();
	m_xA = 0.0f;
	m_yA = 0.0f;
	m_zA = 0.0f;
	m_scale = 1.0f;
	m_pDevice = device;
	m_pContext = context;
	m_pTexture0 = nullptr;
	m_pSampler0 = nullptr;
	m_pVShaderDepth = nullptr;
	m_pVShader = nullptr;
	m_pPShader = nullptr;
	//m_pIADepth = nullptr;
	m_pInputLayout = nullptr;
	m_pRaster = nullptr;
	m_pModelCB = nullptr;
	m_pLightCB = nullptr;

	m_pObject = nullptr;
	m_BSC_Pos = DirectX::XMVectorZero();
	m_BSC_radius = 0.0f;
	ObjectName = ObjName;
	TextureName = TexName;
	IsReflective = Reflective;
}


HRESULT Model::LoadObjModel(char* filename)
{
	m_pObject = new ObjFileModel(filename, m_pDevice.Get(), m_pContext.Get());
	if (m_pObject->filename == "FILE NOT LOADED") return S_FALSE;

	HRESULT hr = S_OK;

	//load and compile shaders
	ID3DBlob* VS, * PS, *opaque, * error;
	hr = D3DCompileFromFile(L"Shader/model.hlsl", NULL, NULL, "VShader", "vs_5_0", 0, 0, &VS, &error);
	if (error != 0)
	{
		OutputDebugStringA((char*)error->GetBufferPointer());
		error->Release();
		if (FAILED(hr))
			return hr;
	}

	hr = D3DCompileFromFile(L"Shader/model.hlsl", NULL, NULL, "PShader", "ps_5_0", 0, 0, &PS, &error);
	if (error != 0)
	{
		OutputDebugStringA((char*)error->GetBufferPointer());
		error->Release();
		if (FAILED(hr))
			return hr;
	}

	hr = D3DCompileFromFile(L"Shader/opaque.hlsl", NULL, NULL, NULL, "vs_5_0", 0, 0, &opaque, &error);
	if (error != 0)
	{
		OutputDebugStringA((char*)error->GetBufferPointer());
		error->Release();
		if (FAILED(hr))
			return hr;
	}


	//create shader objects
	hr = m_pDevice->CreateVertexShader(VS->GetBufferPointer(), VS->GetBufferSize(), nullptr, &m_pVShader);
	if (FAILED(hr)) return hr;
	hr = m_pDevice->CreatePixelShader(PS->GetBufferPointer(), PS->GetBufferSize(), nullptr, &m_pPShader);
	if (FAILED(hr)) return hr;
	hr = m_pDevice->CreateVertexShader(opaque->GetBufferPointer(), opaque->GetBufferSize(), nullptr, &m_pVShaderDepth);
	if (FAILED(hr)) return hr;

	//create the IA layout
	D3D11_INPUT_ELEMENT_DESC iedesc[] =
	{
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 20, D3D11_INPUT_PER_VERTEX_DATA, 0},
	};

	hr = m_pDevice->CreateInputLayout(iedesc, ARRAYSIZE(iedesc), VS->GetBufferPointer(), VS->GetBufferSize(), &m_pInputLayout);
	if (FAILED(hr)) return hr;

	//constant buffers creation
	D3D11_BUFFER_DESC bufferDesc;
	ZeroMemory(&bufferDesc, sizeof(D3D11_BUFFER_DESC));

	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	bufferDesc.ByteWidth = sizeof(MODEL_CB);
	bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

	hr = m_pDevice->CreateBuffer(&bufferDesc, NULL, &m_pModelCB);
	if (FAILED(hr)) return hr;

	bufferDesc.ByteWidth = 80;

	hr = m_pDevice->CreateBuffer(&bufferDesc, NULL, &m_pLightCB);
	if (FAILED(hr)) return hr;

	//create sampler
	D3D11_SAMPLER_DESC samplerDesc;
	ZeroMemory(&samplerDesc, sizeof(D3D11_SAMPLER_DESC));

	samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

	hr = m_pDevice->CreateSamplerState(&samplerDesc, &m_pSampler0);
	if (FAILED(hr)) return hr;

	//create rasterizer
	D3D11_RASTERIZER_DESC rasterDesc;
	ZeroMemory(&rasterDesc, sizeof(D3D11_RASTERIZER_DESC));

	rasterDesc.CullMode = D3D11_CULL_BACK;
	rasterDesc.FillMode = D3D11_FILL_SOLID;

	hr = m_pDevice->CreateRasterizerState(&rasterDesc, &m_pRaster);
	if (FAILED(hr)) return hr;

	CalculateModelCenterPoint();
	CalculateBoundingSphereRadius();

	return hr;
}

void Model::Draw(XMMATRIX& world, XMMATRIX& view, XMMATRIX& projection, XMFLOAT4& AmbColor, XMFLOAT3& DirVector, XMFLOAT4& DirColor)
{
	//Update constant buffers
	MODEL_CB modelCBValues;
	modelCBValues.World = world;
	modelCBValues.WVP = world * view * projection;
	m_pContext->UpdateSubresource(m_pModelCB.Get(), 0, 0, &modelCBValues, 0, 0);
	m_pContext->VSSetConstantBuffers(0, 1, &m_pModelCB);
	
	LIGHT_CB lightCBValues;
	lightCBValues.EyePos = XMFLOAT3(0.0f, 0.0f, 0.0f);
	lightCBValues.DirToLight = DirVector;
	lightCBValues.DirLightColor = DirColor;
	lightCBValues.Ambientdown = AmbColor;
	lightCBValues.AmbientRange = XMFLOAT4(0.2f, 0.2f, 0.2f, 0.2f);
	m_pContext->UpdateSubresource(m_pLightCB.Get(), 0, 0, &lightCBValues, 0, 0);
	m_pContext->PSSetConstantBuffers(1, 1, &m_pLightCB);

	m_pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	m_pContext->IASetInputLayout(m_pInputLayout.Get());
	m_pContext->VSSetShader(m_pVShader.Get(), nullptr, 0);
	m_pContext->RSSetState(m_pRaster.Get());
	m_pContext->PSSetShader(m_pPShader.Get(), nullptr, 0);
	m_pContext->PSSetSamplers(0, 1, m_pSampler0.GetAddressOf());

	m_pObject->Draw();
	
}