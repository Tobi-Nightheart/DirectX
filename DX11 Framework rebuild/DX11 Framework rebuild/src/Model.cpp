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


HRESULT Model::LoadObjModel()
{
	m_pObject = new ObjFileModel(ObjectName, m_pDevice.Get(), m_pContext.Get());
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

	hr = D3DCompileFromFile(L"Shader/opaque.hlsl", NULL, NULL, "OpaqueVS", "vs_5_0", 0, 0, &opaque, &error);
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

void Model::DepthPass(XMMATRIX& world, XMMATRIX& view, XMMATRIX& projection)
{
	MODEL_CB CB;
	CB.WVP = world * view * projection;
	CB.World = world;
	m_pContext->UpdateSubresource(m_pModelCB.Get(), 0, nullptr, &CB, 0, 0);
	m_pContext->VSSetConstantBuffers(0, 1, m_pModelCB.GetAddressOf());

	m_pContext->IASetInputLayout(m_pInputLayout.Get());
	m_pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	m_pContext->RSSetState(m_pRaster.Get());
	m_pContext->PSSetShader(nullptr, nullptr, 0);

	m_pObject->Draw();
}

XMMATRIX Model::GetWorld()
{
	XMMATRIX world;
	world = XMMatrixScaling(m_scale, m_scale, m_scale);
	world = XMMatrixRotationRollPitchYaw(m_xA, m_yA, m_zA);
	world = XMMatrixTranslationFromVector(m_Position);
	return world;
}

XMVECTOR Model::GetBoundingSphereWSPosition()
{
	return XMVector3Transform(m_BSC_Pos, GetWorld());
}

ObjFileModel* Model::GetObject()
{
	return m_pObject;
}

void Model::LookAt(float x, float z)
{
	XMFLOAT3 position;
	XMStoreFloat3(&position, m_Position);
	float dx, dz;
	dx = x - position.x;
	dz = z - position.z;

	m_yA = (float) atan2(dx, dz) * (float)(180.0 / XM_PI);
}

void Model::MoveForward(float d)
{

}

void Model::CalculateModelCenterPoint()
{
	float min_x = 0.0f, min_y = 0.0f, min_z = 0.0f, max_x = 0.0f, max_y = 0.0f, max_z = 0.0f;
	for (int i = 0; i < (int)m_pObject->numVerts; i++)
	{
		if (min_x > m_pObject->vertices[i].Pos.x)
			min_x = m_pObject->vertices[i].Pos.x;
		if (min_y > m_pObject->vertices[i].Pos.y)
			min_y = m_pObject->vertices[i].Pos.y;
		if (min_z > m_pObject->vertices[i].Pos.z)
			min_z= m_pObject->vertices[i].Pos.z;
		if (max_x < m_pObject->vertices[i].Pos.x)
			max_x = m_pObject->vertices[i].Pos.x;
		if (max_y < m_pObject->vertices[i].Pos.y)
			max_y = m_pObject->vertices[i].Pos.y;
		if (max_z < m_pObject->vertices[i].Pos.z)
			max_z = m_pObject->vertices[i].Pos.z;
	}
	m_BSC_Pos = XMVectorSet((min_x + max_x) / 2, (min_y + max_y) / 2, (min_z + max_z) / 2, 0.0f);
}

void Model::CalculateBoundingSphereRadius()
{
	float maxdist = 0.0f;
	float curr;
	XMFLOAT3 center;
	XMStoreFloat3(&center, m_BSC_Pos);
	for (int i = 0; i < (int)m_pObject->numVerts; i++)
	{
		curr = (float) (pow(center.x - m_pObject->vertices[i].Pos.x, 2) + pow(center.y - m_pObject->vertices[i].Pos.y, 2) + pow(center.z - m_pObject->vertices[i].Pos.z, 2));
		if (curr > maxdist) maxdist = curr;
	}
	m_BSC_radius = (float) (sqrt(maxdist) * m_scale);
}

#pragma region GetterSetterIncrementerMethods
void Model::SetTexture(ID3D11ShaderResourceView* tex)
{
	m_pTexture0 = tex;
}

void Model::SetSampler(ID3D11SamplerState* sampler)
{
	m_pSampler0 = sampler;
}

void Model::SetDeviceAndContext(ID3D11Device* Device, ID3D11DeviceContext* Context)
{
	m_pDevice = Device;
	m_pContext = Context;
}

void Model::ChangePos(DirectX::XMVECTOR delta, bool isPoint)
{
	if (isPoint)
		m_Position = delta;
	else m_Position += delta;
}

void Model::IncXA(float a)
{
	m_xA += a;
}

void Model::IncYA(float a)
{
	m_yA += a;
}

void Model::IncZA(float a)
{
	m_zA += a;
}



void Model::SetXA(float a)
{
	m_xA = a;
}

void Model::SetYA(float a)
{
	m_yA = a;
}

void Model::SetZA(float a)
{
	m_zA = a;
}


DirectX::XMVECTOR Model::GetPosition()
{
	return m_Position;
}

float Model::GetXA()
{
	return m_xA;
}

float Model::GetYA()
{
	return m_yA;
}

float Model::GetZA()
{
	return m_zA;
}

float Model::GetScale()
{
	return m_scale;
}

void Model::IncScale(float num)
{
	m_scale += num;
}

void Model::SetScale(float scale)
{
	m_scale = scale;
}

float Model::GetBoundingSphereRadius()
{
	return m_BSC_radius * m_scale;
}

#pragma endregion

Model::~Model()
{
	delete m_pObject;
	ObjectName = nullptr;
	TextureName = nullptr;
}