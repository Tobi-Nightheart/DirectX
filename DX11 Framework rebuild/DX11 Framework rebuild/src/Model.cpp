#pragma once
#include "pcH.h"
#include "Model.h"

Model::Model()
{
	m_Position = XMVectorZero();
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
	m_pIADepth = nullptr;
	m_pInputLayout = nullptr;
	m_pRaster = nullptr;
	m_pModelCB = nullptr;
	m_pLightCB = nullptr;

	m_pObject = nullptr;
	m_BSC_Pos = XMVectorZero();
	m_BSC_radius = 0.0f;
	TextureName = "Assets/Textures/default.jpg";
	ObjectName = "Assets/Resources/cube.obj";
	IsReflective = false;
}

Model::Model(ID3D11Device* device, ID3D11DeviceContext* context, char* ObjectName, char* TextureName, bool IsReflective)
{
	m_Position = XMVectorZero();
	m_xA = 0.0f;
	m_yA = 0.0f;
	m_zA = 0.0f;
	m_scale = 1.0f;
	m_pDevice = device;
	m_pContext = device;
	m_pTexture0 = nullptr;
	m_pSampler0 = nullptr;
	m_pVShaderDepth = nullptr;
	m_pVShader = nullptr;
	m_pPShader = nullptr;
	m_pIADepth = nullptr;
	m_pInputLayout = nullptr;
	m_pRaster = nullptr;
	m_pModelCB = nullptr;
	m_pLightCB = nullptr;

	m_pObject = nullptr;
	m_BSC_Pos = XMVectorZero();
	m_BSC_radius = 0.0f;
	this.ObjectName = ObjectName;
	this.TextureName = TextureName;
	this.IsReflective = IsReflective;
}
