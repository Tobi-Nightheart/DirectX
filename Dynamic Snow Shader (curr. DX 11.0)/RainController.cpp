#include "pch.h"
#include "RainController.h"
#include <minwinbase.h>
#include <memory>
#include <minwinbase.h>


RainController::RainController(ID3D11Device* d3d11Device, ID3D11DeviceContext* d3d11DeviceContext, ID3D11ShaderResourceView* tex, camera* c)
{
	rc_pDevice = d3d11Device;
	rc_pImmediateContext = d3d11DeviceContext;
	rc_pCamera = c;

	rc_FirstRun = true;
	rc_Age = 0.0f;
	rc_GameTime = 0.0f;
	rc_TimeStep = 0.0f;

	XMStoreFloat3(&rc_EyePosW, c->GetCameraPosition());
	XMVECTOR posw = c->GetCameraPosition();
	posw.y += 3;
	XMStoreFloat3(&rc_EmitPosW, posw);
	rc_EmitDirW = XMFLOAT3(0.0f, -1.0f, 0.0f);

	rc_MaxParticles = 10000;

	rc_texArraySRV = tex;
}


HRESULT RainController::Initialize()
{
	HRESULT hr;

	//init texture
	rc_randomTexSRV = CreateRandomTexture1DSRV();

	//init constant buffer
	D3D11_BUFFER_DESC buffer_desc;
	ZeroMemory(&buffer_desc, sizeof(buffer_desc));
	buffer_desc.Usage = D3D11_USAGE_DYNAMIC;
	buffer_desc.ByteWidth = 112;
	buffer_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	buffer_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	buffer_desc.MiscFlags = 0;

	hr = rc_pDevice->CreateBuffer(&buffer_desc, nullptr, &rc_cb0);


	if(FAILED(hr))
	{
		return hr;
	}

	//sampler
	D3D11_SAMPLER_DESC sampler_desc;
	ZeroMemory(&sampler_desc, sizeof(sampler_desc));
	sampler_desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	sampler_desc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	sampler_desc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	sampler_desc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	sampler_desc.MaxLOD = D3D11_FLOAT32_MAX;

	hr = rc_pDevice->CreateSamplerState(&sampler_desc, &rc_pSample0);

	if(FAILED(hr))
	{
		return hr;
	}

	//init depth states
	D3D11_DEPTH_STENCIL_DESC DS_desc;
	ZeroMemory(&DS_desc, sizeof(DS_desc));
	DS_desc.DepthEnable = true;
	DS_desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	DS_desc.DepthFunc = D3D11_COMPARISON_LESS;
	hr = rc_pDevice->CreateDepthStencilState(&DS_desc, &rc_pDepthStencilSolid);
	if (FAILED(hr))
	{
		return hr;
	}
	DS_desc.DepthEnable = false;
	DS_desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	hr = rc_pDevice->CreateDepthStencilState(&DS_desc, &rc_pDepthStencilDisable);
	if(FAILED(hr))
	{
		return hr;
	}
	DS_desc.DepthEnable = true;
	hr = rc_pDevice->CreateDepthStencilState(&DS_desc, &rc_pDepthStencilNoWrite);
	if (FAILED(hr))
	{
		return hr;
	}


	//load compile shaders
	ID3DBlob *VS_out, *PS, *GS_out, *GS_draw, *VS_draw,  *error;
	hr = D3DX11CompileFromFile("RainOutputShader.hlsl", 0, 0, "StreamOutVS", "vs_5_0", 0, 0, 0, &VS_out, &error, 0);
	if(error !=0)
	{
		OutputDebugStringA((char*)error->GetBufferPointer());
		error->Release();
		if (FAILED(hr)) 
		{
			return hr;
		}
	}

	hr = D3DX11CompileFromFile("RainOutputShader.hlsl", 0, 0, "StreamOutGS", "gs_5_0", 0, 0, 0, &GS_out, &error, 0);
	if (error != 0)
	{
		OutputDebugStringA((char*)error->GetBufferPointer());
		error->Release();
		if (FAILED(hr))
		{
			return hr;
		}
	}

	hr = D3DX11CompileFromFile("RainOutputShader.hlsl", 0, 0, "DrawVS", "vs_5_0", 0, 0, 0, &VS_draw, &error, 0);
	if (error != 0)
	{
		OutputDebugStringA((char*)error->GetBufferPointer());
		error->Release();
		if (FAILED(hr))
		{
			return hr;
		}
	}

	hr = D3DX11CompileFromFile("RainOutputShader.hlsl", 0, 0, "DrawGS", "gs_5_0", 0, 0, 0, &GS_draw, &error, 0);
	if (error != 0)
	{
		OutputDebugStringA((char*)error->GetBufferPointer());
		error->Release();
		if (FAILED(hr))
		{
			return hr;
		}
	}

	hr = D3DX11CompileFromFile("RainOutputShader.hlsl", 0, 0, "DrawPS", "ps_5_0", 0, 0, 0, &PS, &error, 0);
	if (error != 0)
	{
		OutputDebugStringA((char*)error->GetBufferPointer());
		error->Release();
		if (FAILED(hr))
		{
			return hr;
		}
	}

	//load shader objects
	hr = rc_pDevice->CreateVertexShader(VS_out->GetBufferPointer(), VS_out->GetBufferSize(), nullptr, &rc_pVSOUT);
	if (FAILED(hr))
	{
		return hr;
	}

	D3D11_SO_DECLARATION_ENTRY pDel[] =
	{
		{0,"POSITION", 0, 0, 3, 0},
		{0, "VELOCITY", 0, 0, 3, 0},
		{0, "SIZE", 0, 0, 2, 0},
		{0, "AGE", 0, 0, 1, 0},
		{0, "TYPE", 0, 0, 1, 0}
	};

	
	hr = rc_pDevice->CreateGeometryShaderWithStreamOutput(GS_out->GetBufferPointer(), GS_out->GetBufferSize(), pDel, 5, nullptr, 0, 0, nullptr, &rc_pGSOUT);
	if (FAILED(hr))
	{
		return hr;
	}

	//create draw shaders
	hr = rc_pDevice->CreateVertexShader(VS_draw->GetBufferPointer(), VS_draw->GetBufferSize(), nullptr, &rc_pVSDraw);
	if (FAILED(hr))
	{
		return hr;
	}

	hr = rc_pDevice->CreateGeometryShader(GS_draw->GetBufferPointer(), GS_draw->GetBufferSize(), nullptr, &rc_pGSDraw);
	if (FAILED(hr))
	{
		return hr;
	}

	hr = rc_pDevice->CreatePixelShader(PS->GetBufferPointer(), PS->GetBufferSize(), nullptr, &rc_pPS);
	if (FAILED(hr))
	{
		return hr;
	}

	D3D11_INPUT_ELEMENT_DESC iedesc_Particle[] = 
	{
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"VELOCITY", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"SIZE",     0, DXGI_FORMAT_R32G32_FLOAT,    0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"AGE",      0, DXGI_FORMAT_R32_FLOAT,       0, 32, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"TYPE",     0, DXGI_FORMAT_R32_UINT,        0, 36, D3D11_INPUT_PER_VERTEX_DATA, 0}
	};

	hr = rc_pDevice->CreateInputLayout(iedesc_Particle, ARRAYSIZE(iedesc_Particle), VS_out->GetBufferPointer(), VS_out->GetBufferSize(), &rc_pInputLayoutParticle);
	if (FAILED(hr))
	{
		return hr;
	}
	/*
	D3D11_INPUT_ELEMENT_DESC ie_desc_Position[] =
	{
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0}
	};
	hr = rc_pDevice->CreateInputLayout(ie_desc_Position, ARRAYSIZE(ie_desc_Position), VS_out->GetBufferPointer(), VS_out->GetBufferSize(), &rc_pInputLayoutPosition);
	if (FAILED(hr))
	{
		return hr;
	}
	*/
	//create vertex buffers
	D3D11_BUFFER_DESC vbd;
	vbd.Usage = D3D11_USAGE_DEFAULT;
	vbd.ByteWidth = sizeof(Particle) * 1;
	vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbd.CPUAccessFlags = 0;
	vbd.MiscFlags = 0;
	vbd.StructureByteStride = 0;

	Particle p;
	p.InitialPos = { 0.0f, 0.0f, 0.0f };
	p.InitialVel = { 0.0f, 0.0f, 0.0f };
	p.Size = { 0.0f, 0.0f };
	p.Age = 0.0f;
	p.Type = 0;

	D3D11_SUBRESOURCE_DATA vintData;
	vintData.pSysMem = &p;

	hr = rc_pDevice->CreateBuffer(&vbd, &vintData, &rc_InitVB);
	if (FAILED(hr))
	{
		return hr;
	}
	vbd.ByteWidth = sizeof(Particle)*rc_MaxParticles;
	vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER | D3D11_BIND_STREAM_OUTPUT;
	hr = rc_pDevice->CreateBuffer(&vbd, nullptr, &rc_DrawVB);
	if (FAILED(hr))
	{
		return hr;
	}
	hr = rc_pDevice->CreateBuffer(&vbd, nullptr, &rc_StreamOutVB);
	if (FAILED(hr))
	{
		return hr;
	}
	return S_OK;

}


void RainController::Draw(XMMATRIX* view, XMMATRIX* proj, camera* c, GameTimer* time)
{
	XMVECTOR CAMoffset;
	CAMoffset = c->GetCameraPosition();
	CAMoffset.x += c->GetLookAt().x;
	CAMoffset.y += 3;
	CAMoffset.z += c->GetLookAt().z;
	rc_pImmediateContext->IASetInputLayout(rc_pInputLayoutParticle);
	rc_pImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);

	rc_pImmediateContext->GSSetSamplers(0, 1, &rc_pSample0);

	UINT stride = sizeof(Particle);
	UINT offset[1] = { 0 };

	//check if first run
	if(rc_FirstRun)
	{
		rc_pImmediateContext->IASetVertexBuffers(0, 1, &rc_InitVB, &stride, offset);
	}else
	{
		rc_pImmediateContext->IASetVertexBuffers(0, 1, &rc_DrawVB, &stride, offset);
	}

	//update the current particles via stream out
	rc_pImmediateContext->SOSetTargets(1, &rc_StreamOutVB, offset);
	
	rc_pImmediateContext->VSSetShader(rc_pVSOUT, 0, 0);
	rc_pImmediateContext->GSSetShader(rc_pGSOUT, 0, 0);
	rc_pImmediateContext->PSSetShader(nullptr, nullptr, 0);

	XMMATRIX ViewProject = (*view) * (*proj);

	//Update constant buffer
	cbRain cb_rain_values;
	cb_rain_values.ViewProj = ViewProject;
	cb_rain_values.EyePosition = c->GetCameraPosition();
	cb_rain_values.GameTime = time->TotalTime();
	cb_rain_values.TimeStep = time->DeltaTime();
	XMStoreFloat3(&cb_rain_values.EmitPos , CAMoffset);
	cb_rain_values.EmitDirW = rc_EmitDirW;
	//Copy values
	D3D11_MAPPED_SUBRESOURCE mapped_subresource;
	HRESULT hr = this->rc_pImmediateContext->Map(rc_cb0, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped_subresource);
	memcpy(mapped_subresource.pData, &cb_rain_values, sizeof(cbRain));
	rc_pImmediateContext->Unmap(rc_cb0, 0);

	rc_pImmediateContext->VSSetConstantBuffers(0, 1, &rc_cb0);
	rc_pImmediateContext->OMSetDepthStencilState(rc_pDepthStencilDisable, 0);
	rc_pImmediateContext->VSSetShaderResources(0, 1, &rc_randomTexSRV);
	rc_pImmediateContext->GSSetShaderResources(0, 1, &rc_randomTexSRV);

	// first draw
	if(rc_FirstRun)
	{
		rc_pImmediateContext->Draw(1,0);
		rc_FirstRun = false;
	}
	else
	{
		rc_pImmediateContext->DrawAuto();
	}

	//done streaming out unbind the buffer
	ID3D11Buffer* bufferArray[1] = { 0 };
	rc_pImmediateContext->SOSetTargets(1, bufferArray, offset);

	//ping pong the vertex buffers
	std::swap(rc_DrawVB, rc_StreamOutVB);

	//set input layout
	rc_pImmediateContext->IASetVertexBuffers(0, 1, &rc_DrawVB, &stride, offset);

	//real draw setup
	rc_pImmediateContext->VSSetShader(rc_pVSDraw, 0, 0);
	rc_pImmediateContext->GSSetShader(rc_pGSDraw, 0, 0);
	rc_pImmediateContext->PSGetShader(&rc_pPS, 0, 0);
	rc_pImmediateContext->PSSetShaderResources(0, 1, &rc_texArraySRV);
	rc_pImmediateContext->OMSetDepthStencilState(rc_pDepthStencilNoWrite, 0);
	rc_pImmediateContext->DrawAuto();
	rc_pImmediateContext->OMSetDepthStencilState(rc_pDepthStencilSolid, 0);
	rc_pImmediateContext->GSSetShader(nullptr, 0, 0);
}

void RainController::Update(float dt, float gameTime)
{
	rc_GameTime = gameTime;
	rc_TimeStep = dt;

	rc_Age += dt;
}


RainController::~RainController()
{
	if (rc_InitVB) rc_InitVB->Release();
	if (rc_DrawVB) rc_DrawVB->Release();
	if (rc_StreamOutVB) rc_StreamOutVB->Release();
	if (rc_cb0) rc_cb0->Release();
	if (rc_pVSOUT) rc_pVSOUT->Release();
	if (rc_pGSOUT) rc_pGSOUT->Release();
	if (rc_pVSDraw) rc_pVSDraw->Release();
	if (rc_pPS) rc_pPS->Release();
	if (rc_pGSDraw) rc_pGSDraw->Release();
	if (rc_texArraySRV) rc_texArraySRV = nullptr;
	if (rc_randomTexSRV) rc_randomTexSRV->Release();
	if (rc_pInputLayoutParticle) rc_pInputLayoutParticle->Release();
	if (rc_pInputLayoutPosition) rc_pInputLayoutPosition->Release();
	if (rc_pDepthStencilDisable) rc_pDepthStencilDisable->Release();
	if (rc_pDepthStencilSolid) rc_pDepthStencilSolid->Release();
	if (rc_pDepthStencilNoWrite) rc_pDepthStencilNoWrite->Release();
	if (rc_pSample0) rc_pSample0->Release();
	if (rc_pCamera) rc_pCamera = nullptr;
}

void RainController::Reset()
{
	rc_FirstRun = true;
	rc_Age = 0.0f;
}

ID3D11ShaderResourceView* RainController::CreateRandomTexture1DSRV()
{
	// create random data
	XMFLOAT4 randomValues[1024];

	for(int i = 0; i < 1024; i++)
	{
		randomValues[i].x = RandF(-1.0f, 1.0f);
		randomValues[i].y = RandF(-1.0f, 1.0f);
		randomValues[i].z = RandF(-1.0f, 1.0f);
		randomValues[i].w = RandF(-1.0f, 1.0f);
	}

	D3D11_SUBRESOURCE_DATA initData;
	initData.pSysMem = randomValues;
	initData.SysMemPitch = 1024 * sizeof(XMFLOAT4);
	initData.SysMemSlicePitch = 0;

	//create texture data
	D3D11_TEXTURE1D_DESC texDesc;
	texDesc.Width = 1024;
	texDesc.MipLevels = 1;
	texDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	texDesc.Usage = D3D11_USAGE_IMMUTABLE;
	texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	texDesc.CPUAccessFlags = 0;
	texDesc.MiscFlags = 0;
	texDesc.ArraySize = 1;

	ID3D11Texture1D* randomTex = 0;
	
	rc_pDevice->CreateTexture1D(&texDesc, &initData, &randomTex);

	//create the shader resource view
	D3D11_SHADER_RESOURCE_VIEW_DESC viewDesc;
	viewDesc.Format = texDesc.Format;
	viewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE1D;
	viewDesc.Texture1D.MipLevels = texDesc.MipLevels;
	viewDesc.Texture1D.MostDetailedMip = 0;

	ID3D11ShaderResourceView* randomTexSRV = 0;
	rc_pDevice->CreateShaderResourceView(randomTex, &viewDesc, &randomTexSRV);

	randomTex->Release();

	return randomTexSRV;

}

float RainController::RandF()
{
	return (float)(rand()) / (float)RAND_MAX;
}

float RainController::RandF(float a, float b)
{
	return a + RandF()*(b - a);
}

XMVECTOR RainController::RandomUnitVec3()
{
	XMVECTOR One = XMVectorSet(1.0f, 1.0f, 1.0f, 1.0f);
	XMVECTOR Zero = XMVectorZero();

	while(true)
	{
		XMVECTOR v = XMVectorSet(
			RandF(-1.0, 1.0f),
			RandF(-1.0, 1.0),
			RandF(-1.0f, 1.0),
			0.0
		);

		if (XMVector3Greater(XMVector3Length(v), One)) continue;
		return XMVector3Normalize(v);
	}
}
