#include "pch.h"
#include "Skybox.h"





struct POS_COL_TEX_VERTEX//This will be added to and renamed in future tutorials
{
	XMFLOAT3	pos;
	XMFLOAT4	Col;
	XMFLOAT3	Texture0;
};

struct SKY_CB
{
	XMMATRIX WorldViewProjection; //64 bytes
};

Skybox::Skybox(ID3D11Device* device, ID3D11DeviceContext* context)
{
	s_pD3DDevice = device;
	s_pContext = context;
}

HRESULT Skybox::Initialize()
{
	HRESULT hr;

	POS_COL_TEX_VERTEX vertices[] =
	{
		//back face
		{XMFLOAT3(-1.0f, 1.0f, 1.0f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), XMFLOAT3(-1.0f, 1.0f, 1.0f) },
		{XMFLOAT3(-1.0f, -1.0f, 1.0f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), XMFLOAT3(-1.0f, -1.0f, 1.0f)},
		{XMFLOAT3(1.0f, 1.0f, 1.0f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), XMFLOAT3(1.0f, 1.0f, 1.0f)},
		{XMFLOAT3(1.0f, 1.0f, 1.0f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), XMFLOAT3(1.0f, 1.0f, 1.0f)},
		{XMFLOAT3(-1.0f, -1.0f, 1.0f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), XMFLOAT3(-1.0f, -1.0f, 1.0f)},
		{XMFLOAT3(1.0f, -1.0f, 1.0f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), XMFLOAT3(1.0f, -1.0f, 1.0f)},

		//front face
		{XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), XMFLOAT3(-1.0f, -1.0f, -1.0f)},
		{XMFLOAT3(-1.0f, 1.0f, -1.0f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), XMFLOAT3(-1.0f, 1.0f, -1.0f)},
		{XMFLOAT3(1.0f, 1.0f, -1.0f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), XMFLOAT3(1.0f, 1.0f, -1.0f)},
		{XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), XMFLOAT3(-1.0f, -1.0f, -1.0f)},
		{XMFLOAT3(1.0f, 1.0f, -1.0f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), XMFLOAT3(1.0f, 1.0f, -1.0f)},
		{XMFLOAT3(1.0f, -1.0f, -1.0f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), XMFLOAT3(1.0f, -1.0f, -1.0f)},

		//left face
		{XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), XMFLOAT3(-1.0f, -1.0f, -1.0f)},
		{XMFLOAT3(-1.0f, -1.0f, 1.0f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), XMFLOAT3(-1.0f, -1.0f, 1.0f)},
		{XMFLOAT3(-1.0f, 1.0f, -1.0f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), XMFLOAT3(-1.0f, 1.0f, -1.0f)},
		{XMFLOAT3(-1.0f, -1.0f, 1.0f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), XMFLOAT3(-1.0f, -1.0f, 1.0f)},
		{XMFLOAT3(-1.0f, 1.0f, 1.0f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), XMFLOAT3(-1.0f, 1.0f, 1.0f)},
		{XMFLOAT3(-1.0f, 1.0f, -1.0f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), XMFLOAT3(-1.0f, 1.0f, -1.0f)},

		//right face
		{XMFLOAT3(1.0f, -1.0f, 1.0f),  XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), XMFLOAT3(1.0f, -1.0f, 1.0f)},
		{XMFLOAT3(1.0f, -1.0f, -1.0f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), XMFLOAT3(1.0f, -1.0f, -1.0f)},
		{XMFLOAT3(1.0f, 1.0f, -1.0f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), XMFLOAT3(1.0f, 1.0f, -1.0f)},
		{XMFLOAT3(1.0f, 1.0f, 1.0f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), XMFLOAT3(1.0f, 1.0f, 1.0f)},
		{XMFLOAT3(1.0f, -1.0f, 1.0f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), XMFLOAT3(1.0f, -1.0f, 1.0f)},
		{XMFLOAT3(1.0f, 1.0f, -1.0f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), XMFLOAT3(1.0f, 1.0f, -1.0f)},

		//bottom face
		{XMFLOAT3(1.0f, -1.0f, -1.0f),  XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), XMFLOAT3(1.0f, -1.0f, -1.0f)},
		{XMFLOAT3(1.0f, -1.0f, 1.0f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), XMFLOAT3(1.0f, -1.0f, 1.0f)},
		{XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), XMFLOAT3(-1.0f, -1.0f, -1.0f)},
		{XMFLOAT3(1.0f, -1.0f, 1.0f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), XMFLOAT3(1.0f, -1.0f, 1.0f)},
		{XMFLOAT3(-1.0f, -1.0f, 1.0f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), XMFLOAT3(-1.0f, -1.0f, 1.0f)},
		{XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), XMFLOAT3(-1.0f, -1.0f, -1.0f)},

		//top face
		{XMFLOAT3(1.0f, 1.0f, 1.0f),  XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), XMFLOAT3(1.0f, 1.0f, 1.0f)},
		{XMFLOAT3(1.0f, 1.0f, -1.0f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), XMFLOAT3(1.0f, 1.0f, -1.0f)},
		{XMFLOAT3(-1.0f, 1.0f, -1.0f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), XMFLOAT3(-1.0f, 1.0f, -1.0f)},
		{XMFLOAT3(-1.0f, 1.0f, 1.0f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), XMFLOAT3(-1.0f, 1.0f, 1.0f)},
		{XMFLOAT3(1.0f, 1.0f, 1.0f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), XMFLOAT3(1.0f, 1.0f, 1.0f)},
		{XMFLOAT3(-1.0f, 1.0f, -1.0f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), XMFLOAT3(-1.0f, 1.0f, -1.0f)}
	};

	//Load Texture from file | change filepath if applicable
	hr = D3DX11CreateShaderResourceViewFromFile(s_pD3DDevice, "assets/skybox01.dds", NULL, NULL, &s_pTexture, NULL);
	if (FAILED(hr)) return hr;

	//Set up and create vertex buffer
	D3D11_BUFFER_DESC bufferDesc;
	ZeroMemory(&bufferDesc, sizeof(bufferDesc));
	bufferDesc.Usage = D3D11_USAGE_DYNAMIC;										//Allows use by CPU and GPU
	bufferDesc.ByteWidth = sizeof(vertices);                              //Set the total size of the buffer (in this case, 3 vertices)
	bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;							//Set the type of buffer to vertex buffer
	bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;							//Allow access by the CPU
	hr = s_pD3DDevice->CreateBuffer(&bufferDesc, NULL, &s_pVBuffer);		//Create the buffer

	if (FAILED(hr))//Return an error code if failed
	{
		return hr;
	}

	//setting up the constant buffer
	D3D11_BUFFER_DESC constant_buffer_desc;
	ZeroMemory(&constant_buffer_desc, sizeof(constant_buffer_desc));

	constant_buffer_desc.Usage = D3D11_USAGE_DEFAULT; //Can use UpdateSubresource() to update
	constant_buffer_desc.ByteWidth = 64; // MUST be a multiple of 16, calculate from CB struct
	constant_buffer_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER; // Use as a constant buffer

	hr = s_pD3DDevice->CreateBuffer(&constant_buffer_desc, NULL, &s_pConstantBuffer);

	if (FAILED(hr))//Return an error code if failed
	{
		return hr;
	}

	//initiate Sampler
	D3D11_SAMPLER_DESC sampler_desc;
	ZeroMemory(&sampler_desc, sizeof(sampler_desc));
	sampler_desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	sampler_desc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	sampler_desc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	sampler_desc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	sampler_desc.MaxLOD = D3D11_FLOAT32_MAX;

	s_pD3DDevice->CreateSamplerState(&sampler_desc, &s_pSampler0);

	if (FAILED(hr))//Return an error code if failed
	{
		return hr;
	}

	//initialize raster states
	D3D11_RASTERIZER_DESC rasterizer_desc;
	ZeroMemory(&rasterizer_desc, sizeof(rasterizer_desc));
	
	rasterizer_desc.FillMode = D3D11_FILL_SOLID;
	rasterizer_desc.CullMode = D3D11_CULL_BACK;
	hr = s_pD3DDevice->CreateRasterizerState(&rasterizer_desc, &s_pRasterSolid);
	if (FAILED(hr)) return hr;
	
	rasterizer_desc.FillMode = D3D11_FILL_SOLID;
	rasterizer_desc.CullMode = D3D11_CULL_FRONT;
	hr = s_pD3DDevice->CreateRasterizerState(&rasterizer_desc, &s_pRasterSkybox);
	if (FAILED(hr)) return hr;

	//initialize depth stencil states
	D3D11_DEPTH_STENCIL_DESC DS_desc;
	ZeroMemory(&DS_desc, sizeof(DS_desc));

	DS_desc.DepthEnable = true;
	DS_desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	DS_desc.DepthFunc = D3D11_COMPARISON_LESS;
	hr = s_pD3DDevice->CreateDepthStencilState(&DS_desc, &s_pDepthWriteSolid);
	if (FAILED(hr)) return hr;
	DS_desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	hr = s_pD3DDevice->CreateDepthStencilState(&DS_desc, &s_pDepthWriteSkybox);
	if (FAILED(hr)) return hr;


	//Copy the vertices into the buffer
	D3D11_MAPPED_SUBRESOURCE ms;

	//Lock the buffer to allow writing
	s_pContext->Map(s_pVBuffer, NULL, D3D11_MAP_WRITE_DISCARD, NULL, &ms);

	//Copy the data
	memcpy(ms.pData, vertices, sizeof(vertices));

	//Unlock the buffer
	s_pContext->Unmap(s_pVBuffer, NULL);

	//Load and compile the pixel and vertex shaders - use vs_5_0 to target DX11 hardware only
	ID3DBlob *VS, *PS, *error;
	hr = D3DX11CompileFromFile("sky_shader.hlsl", 0, 0, "VShader", "vs_5_0", 0, 0, 0, &VS, &error, 0);

	if (error != 0)//Check for shader compilation error
	{
		OutputDebugStringA((char*)error->GetBufferPointer());
		error->Release();
		if (FAILED(hr))//Don't fail if error is just a warning
		{
			return hr;
		}
	}

	hr = D3DX11CompileFromFile("sky_shader.hlsl", 0, 0, "PShader", "ps_5_0", 0, 0, 0, &PS, &error, 0);

	if (error != 0)//Check for shader compilation error
	{
		OutputDebugStringA((char*)error->GetBufferPointer());
		error->Release();
		if (FAILED(hr))//Don't fail if error is just a warning
		{
			return hr;
		}
	}

	//Create shader objects
	hr = s_pD3DDevice->CreateVertexShader(VS->GetBufferPointer(), VS->GetBufferSize(), NULL, &s_pVS);
	if (FAILED(hr))
	{
		return hr;
	}

	hr = s_pD3DDevice->CreatePixelShader(PS->GetBufferPointer(), PS->GetBufferSize(), NULL, &s_pPS);
	if (FAILED(hr))
	{
		return hr;
	}

	//Set the shader objects as active
	s_pContext->VSSetShader(s_pVS, 0, 0);
	s_pContext->PSSetShader(s_pPS, 0, 0);

	//Create and set the input layout object
	D3D11_INPUT_ELEMENT_DESC iedesc[] =
	{
		//Be very careful setting the correct dxgi format and D3D version
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,0,0,D3D11_INPUT_PER_VERTEX_DATA,0},
		//NOTE the spelling of COLOR. Again, be careful setting the correct dxgi format (using A32) and correct D3D version
		{"COLOR", 0,DXGI_FORMAT_R32G32B32A32_FLOAT,0,D3D11_APPEND_ALIGNED_ELEMENT,D3D11_INPUT_PER_VERTEX_DATA,0},
		//Texture coordinate definition
		{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0}
	};

	hr = s_pD3DDevice->CreateInputLayout(iedesc, ARRAYSIZE(iedesc), VS->GetBufferPointer(), VS->GetBufferSize(), &s_pInputLayout);
	if (FAILED(hr))
	{
		return hr;
	}

	s_pContext->IASetInputLayout(s_pInputLayout);



	return S_OK;
}

void Skybox::Draw(XMMATRIX* view, XMMATRIX* projection, camera* cam)
{
	s_pContext->PSSetSamplers(0, 1, &s_pSampler0);
	s_pContext->PSSetShaderResources(0, 1, &s_pTexture);
	s_pContext->IASetInputLayout(s_pInputLayout);
	s_pContext->VSSetShader(s_pVS, 0, 0);
	s_pContext->PSSetShader(s_pPS, 0, 0);

	XMMATRIX world;
	world = XMMatrixScaling(3.0f, 3.0f, 3.0f);
	world = XMMatrixTranslation(cam->GetCameraPosition().x, cam->GetCameraPosition().y, cam->GetCameraPosition().z);
	
	SKY_CB cb0_values;
	cb0_values.WorldViewProjection = world * (*view) * (*projection);
	//upload the new values for the constant buffers
	s_pContext->UpdateSubresource(s_pConstantBuffer, 0, 0, &cb0_values, 0, 0);
	s_pContext->VSSetConstantBuffers(0, 1, &s_pConstantBuffer);


	//Set vertex buffer
	UINT stride = sizeof(POS_COL_TEX_VERTEX);
	UINT offset = 0;
	s_pContext->IASetVertexBuffers(0, 1, &s_pVBuffer, &stride, &offset);

	// Select which primitive type to use
	s_pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	//setting the raster state and depth stencil and resetting after draw
	s_pContext->RSSetState(s_pRasterSkybox);
	s_pContext->OMSetDepthStencilState(s_pDepthWriteSkybox, 0);

	//Draw the vertex buffer to the back buffer
	s_pContext->Draw(36, 0);

	s_pContext->RSSetState(s_pRasterSolid);
	s_pContext->OMSetDepthStencilState(s_pDepthWriteSolid, 0);
}


Skybox::~Skybox()
{
	if (s_pRasterSkybox) s_pRasterSkybox->Release();
	if (s_pRasterSolid) s_pRasterSolid->Release();
	if (s_pDepthWriteSkybox) s_pDepthWriteSkybox->Release();
	if (s_pDepthWriteSolid) s_pDepthWriteSolid->Release();

	if (s_pVS) s_pVS->Release();
	if (s_pPS) s_pPS->Release();
	if (s_pInputLayout) s_pInputLayout->Release();
	if (s_pConstantBuffer) s_pConstantBuffer->Release();
	if (s_pVBuffer) s_pVBuffer->Release();
	if (s_pTexture) s_pTexture->Release();
	if (s_pSampler0) s_pSampler0->Release();

	if (s_pD3DDevice) s_pD3DDevice = nullptr;
	if (s_pContext) s_pContext = nullptr;
}
