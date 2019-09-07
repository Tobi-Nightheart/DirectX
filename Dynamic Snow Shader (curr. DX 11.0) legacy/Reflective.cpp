#pragma once
#include "pch.h"
#include "ObjFileModel.h"
#include "Reflective.h"
#include "Model.h"


Reflective::Reflective(ID3D11Device* d3d11Device, ID3D11DeviceContext* d3d11DeviceContext)
{
	r_px = 0.0f;
	r_py = 0.0f;
	r_pz = 0.0f;
	r_xAngle = 0.0f;
	r_yAngle = 0.0f;
	r_zAngle = 0.0f;
	r_Scale = 1.0f;
	r_pD3DDevice = d3d11Device;
	r_pImmediateContext = d3d11DeviceContext;
}

HRESULT Reflective::LoadObjModel(char* filename) {
	r_pObject = new ObjFileModel(filename, r_pD3DDevice, r_pImmediateContext);
	if (r_pObject->filename == "FILE NOT LOADED") { return S_FALSE; }

	HRESULT hr = S_OK;

	D3DX11CreateShaderResourceViewFromFile(r_pD3DDevice, "assets/skybox01.dds", NULL, NULL, &r_pTexture0, NULL);

	//load and compile shaders
	ID3DBlob *VS, *PS, *error;
	hr = D3DX11CompileFromFile("reflect_shader.hlsl", 0, 0, "ReflectVS", "vs_5_0", 0, 0, 0, &VS, &error, 0);
	if (error != 0)
	{
		OutputDebugStringA((char*)error->GetBufferPointer());
		error->Release();
		if (FAILED(hr))
			return hr;
	}

	hr = D3DX11CompileFromFile("reflect_shader.hlsl", 0, 0, "ReflectPS", "ps_5_0", 0, 0, 0, &PS, &error, 0);
	if (error != 0)
	{
		OutputDebugStringA((char*)error->GetBufferPointer());
		error->Release();
		if (FAILED(hr))
			return hr;
	}

	//create shader objects
	hr = r_pD3DDevice->CreateVertexShader(VS->GetBufferPointer(), VS->GetBufferSize(), NULL, &r_pVShader);
	if (FAILED(hr)) return hr;
	hr = r_pD3DDevice->CreatePixelShader(PS->GetBufferPointer(), PS->GetBufferSize(), NULL, &r_pPShader);
	if (FAILED(hr)) return hr;

	//set shader active
	r_pImmediateContext->VSSetShader(r_pVShader, 0, 0);
	r_pImmediateContext->PSSetShader(r_pPShader, 0, 0);

	D3D11_INPUT_ELEMENT_DESC iedesc[] =
	{
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0}

	};

	hr = r_pD3DDevice->CreateInputLayout(iedesc, ARRAYSIZE(iedesc), VS->GetBufferPointer(), VS->GetBufferSize(), &r_pInputLayout);
	if (FAILED(hr))
		return hr;
	r_pImmediateContext->IASetInputLayout(r_pInputLayout);

	D3D11_BUFFER_DESC constant_buffer_desc;
	ZeroMemory(&constant_buffer_desc, sizeof(constant_buffer_desc));

	constant_buffer_desc.Usage = D3D11_USAGE_DEFAULT;
	constant_buffer_desc.ByteWidth = 128;
	constant_buffer_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

	hr = r_pD3DDevice->CreateBuffer(&constant_buffer_desc, NULL, &r_pConstantBuffer);
	if (FAILED(hr)) return hr;

	constant_buffer_desc.ByteWidth = 144;

	hr = r_pD3DDevice->CreateBuffer(&constant_buffer_desc, nullptr, &r_pCBPixelIn);
	if (FAILED(hr)) return hr;

	CalculateModelCenterPoint();
	CalculateBoundingSphereRadius();
	

	D3D11_SAMPLER_DESC sampler_desc;
	ZeroMemory(&sampler_desc, sizeof(sampler_desc));
	sampler_desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	sampler_desc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	sampler_desc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	sampler_desc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	sampler_desc.MaxLOD = D3D11_FLOAT32_MAX;

	r_pD3DDevice->CreateSamplerState(&sampler_desc, &r_pSampler0);

	//Init resources for HeightMap pass
	hr = D3DX11CompileFromFile("shaders.hlsl", nullptr, nullptr, "VShaderOpaque", "vs_5_0", 0, 0, nullptr, &VS, &error, nullptr);
	if (error != 0)
	{
		OutputDebugStringA((char*)error->GetBufferPointer());
		error->Release();
		if (FAILED(hr))
			return hr;
	}

	hr = r_pD3DDevice->CreateVertexShader(VS->GetBufferPointer(), VS->GetBufferSize(), nullptr, &r_pVSOpaque);
	if (FAILED(hr)) return hr;

	D3D11_INPUT_ELEMENT_DESC opaque_desc[] =
	{
		{"POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
	};

	hr = r_pD3DDevice->CreateInputLayout(opaque_desc, ARRAYSIZE(opaque_desc), VS->GetBufferPointer(), VS->GetBufferSize(), &r_pIAOpaque);
	if (FAILED(hr)) return hr;

	return hr;

}

void Reflective::Draw(XMMATRIX* view, XMMATRIX* projection, XMFLOAT4 AmbC, XMVECTOR DirV, XMFLOAT4 DirC) {
	
	XMMATRIX world;
	world = XMMatrixScaling(r_Scale, r_Scale, r_Scale);
	world = XMMatrixRotationZ(r_zAngle);
	world = XMMatrixRotationY(r_yAngle);
	world = XMMatrixRotationX(r_xAngle);
	world = XMMatrixTranslation(r_px, r_py, r_pz);

	
	REFLECTIVE_CONSTANT_BUFFER reflective_cb_values;
	reflective_cb_values.WorldViewProjection = world * (*view) * (*projection);
	reflective_cb_values.WorldView = world * (*view);
	r_pImmediateContext->UpdateSubresource(r_pConstantBuffer, 0, 0, &reflective_cb_values, 0, 0);
	
	r_pImmediateContext->VSSetConstantBuffers(0, 1, &r_pConstantBuffer);
	
	Light_CB light_cb;
	light_cb.world = world;
	light_cb.ambientdown = { .2f, .2f, .2f, 0.0f };
	light_cb.ambientrange = { .3f, .3f, .3f, 0.0f };
	light_cb.DirLightColor = DirC;
	light_cb.DirToLight = -DirV;

	r_pImmediateContext->PSSetConstantBuffers(0, 1, &r_pCBPixelIn);
	r_pImmediateContext->UpdateSubresource(r_pCBPixelIn, 0, 0, &light_cb, 0, 0);

	//Setting the vs, ps, and input layout
	r_pImmediateContext->VSSetShader(r_pVShader, 0, 0);
	r_pImmediateContext->PSSetShader(r_pPShader, 0, 0);
	r_pImmediateContext->IASetInputLayout(r_pInputLayout);
	r_pImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	r_pImmediateContext->PSSetShaderResources(0, 1, &r_pTexture0);
	r_pImmediateContext->PSSetSamplers(0, 1, &r_pSampler0);

	r_pObject->Draw();
}

void Reflective::DrawOpaque(XMMATRIX* view, XMMATRIX* projection)
{
	XMMATRIX world;
	world = XMMatrixScaling(r_Scale, r_Scale, r_Scale);
	world = XMMatrixRotationZ(r_zAngle);
	world = XMMatrixRotationY(r_yAngle);
	world = XMMatrixRotationX(r_xAngle);
	world = XMMatrixTranslation(r_px, r_py, r_pz);

	REFLECTIVE_CONSTANT_BUFFER CB;
	CB.WorldViewProjection = world * (*view)* (*projection);
	CB.WorldView = world * (*view);

	r_pImmediateContext->VSSetConstantBuffers(0, 1, &r_pConstantBuffer);
	r_pImmediateContext->UpdateSubresource(r_pConstantBuffer, 0, nullptr, &CB, 0, 0);

	//Set the appropriate shaders
	r_pImmediateContext->VSSetShader(r_pVSOpaque, nullptr, 0);
	r_pImmediateContext->IASetInputLayout(r_pIAOpaque);
	r_pImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	r_pImmediateContext->PSSetShader(nullptr, nullptr, 0);

	r_pObject->Draw();
}

XMVECTOR Reflective::GetBoundingSphereWorldSpacePosition()
{
	XMMATRIX world;
	world = XMMatrixScaling(r_Scale, r_Scale, r_Scale);
	world = XMMatrixRotationRollPitchYaw(r_xAngle, r_yAngle, r_zAngle);
	world = XMMatrixTranslation(r_px, r_py, r_pz);

	XMVECTOR offset = XMVectorSet(r_bounding_sphere_center_x, r_bounding_sphere_center_y, r_bounding_sphere_center_z, 0.0f);
	offset = XMVector3Transform(offset, world);
	return offset;
}

bool Reflective::CheckCollision(Model * model)
{
	//check if they are the same model
	//if (model == this)
	//	return false;

	XMVECTOR homeModel = GetBoundingSphereWorldSpacePosition();
	XMVECTOR foreignModel = model->GetBoundingSphereWorldSpacePosition();

	float homeX, homeY, homeZ, foreignX, foreignY, foreignZ;
	homeX = XMVectorGetX(homeModel);
	homeY = XMVectorGetY(homeModel);
	homeZ = XMVectorGetZ(homeModel);
	foreignX = XMVectorGetX(foreignModel);
	foreignY = XMVectorGetY(foreignModel);
	foreignZ = XMVectorGetZ(foreignModel);

	float distsquared = pow(homeX - foreignX, 2) + pow(homeY - foreignY, 2) + pow(homeZ - foreignZ, 2);
	float collisiondist = pow(this->GetBoundingSphereRadius(), 2) + pow(model->GetBoundingSphereRadius(), 2);
	if (distsquared <= collisiondist) {
		return true;
	}
	else return false;

}

void Reflective::CalculateModelCenterPoint()
{
	float min_x = 0;
	float min_y = 0;
	float min_z = 0;
	float max_x = 0;
	float max_y = 0;
	float max_z = 0;
	for (int i = 0; i < (int) r_pObject->numverts; i++) {
		if (min_x > r_pObject->vertices[i].Pos.x)
			min_x = r_pObject->vertices[i].Pos.x;
		if (min_y > r_pObject->vertices[i].Pos.x)
			min_y = r_pObject->vertices[i].Pos.x;
		if (min_z > r_pObject->vertices[i].Pos.x)
			min_z = r_pObject->vertices[i].Pos.x;
		if (max_x < r_pObject->vertices[i].Pos.x)
			max_x = r_pObject->vertices[i].Pos.x;
		if (max_y < r_pObject->vertices[i].Pos.x)
			max_y = r_pObject->vertices[i].Pos.x;
		if (max_z < r_pObject->vertices[i].Pos.x)
			max_z = r_pObject->vertices[i].Pos.x;
	}
	r_bounding_sphere_center_x = (min_x + max_x) / 2;
	r_bounding_sphere_center_y = (min_y + max_y) / 2;
	r_bounding_sphere_center_z = (min_z + max_z) / 2;
}

void Reflective::CalculateBoundingSphereRadius()
{
	float maxdist = 0.0f;
	float curr;
	for (int i = 0; i < (int) r_pObject->numverts; i++) {
		curr = pow(r_bounding_sphere_center_x - r_pObject->vertices[i].Pos.x, 2) + pow(r_bounding_sphere_center_y - r_pObject->vertices[i].Pos.y, 2) + pow(r_bounding_sphere_center_z - r_pObject->vertices[i].Pos.z, 2);
		if (curr > maxdist) maxdist = curr;
	}
	r_bounding_sphere_radius = sqrt(maxdist);
}

void Reflective::IncX(float num)
{
	r_px += num;
}

void Reflective::IncY(float num)
{
	r_py += num;
}

void Reflective::IncZ(float num)
{
	r_pz += num;
}

void Reflective::IncXAngle(float num)
{
	r_xAngle += num;
}

void Reflective::IncYAngle(float num)
{
	r_yAngle += num;
}

void Reflective::IncZAngle(float num)
{
	r_zAngle += num;
}

void Reflective::IncScale(float num)
{
	r_Scale += num;
}

void Reflective::SetX(float x)
{
	r_px = x;
}

void Reflective::SetY(float y)
{
	r_py = y;
}

void Reflective::SetZ(float z)
{
	r_pz = z;
}

void Reflective::SetXAngle(float xAngle)
{
	r_xAngle = xAngle;
}

void Reflective::SetYAngle(float yAngle)
{
	r_yAngle = yAngle;
}

void Reflective::SetZAngle(float zAngle)
{
	r_zAngle = zAngle;
}

void Reflective::SetScale(float scale)
{
	r_Scale = scale;
}

float Reflective::GetX()
{
	return r_px;
}

float Reflective::GetY()
{
	return r_py;
}

float Reflective::GetZ()
{
	return r_pz;
}

float Reflective::GetXAngle()
{
	return r_xAngle;
}

float Reflective::GetYAngle()
{
	return r_yAngle;
}

float Reflective::GetZAngle()
{
	return r_zAngle;
}

float Reflective::GetScale()
{
	return r_Scale;
}

float Reflective::GetBoundingSphereRadius()
{
	return r_bounding_sphere_radius * r_Scale;
}

XMMATRIX Reflective::GetWorld()
{
	XMMATRIX world;
	world = XMMatrixScaling(r_Scale, r_Scale, r_Scale);
	world = XMMatrixRotationZ(r_zAngle);
	world = XMMatrixRotationY(r_yAngle);
	world = XMMatrixRotationX(r_xAngle);
	world = XMMatrixTranslation(r_px, r_py, r_pz);
	return world;
}



Reflective::~Reflective()
{
	if (r_pTexture0) r_pTexture0->Release();
	if (r_pConstantBuffer) r_pConstantBuffer->Release();
	if (r_pInputLayout) r_pInputLayout->Release();
	if (r_pVShader) r_pVShader->Release();
	if (r_pPShader) r_pPShader->Release();
	delete r_pObject;
	if (r_pCBPixelIn) r_pCBPixelIn->Release();
	if (r_pD3DDevice) r_pD3DDevice = nullptr;
	if (r_pImmediateContext) r_pImmediateContext = nullptr;
	if (r_pSampler0) r_pSampler0->Release();
}