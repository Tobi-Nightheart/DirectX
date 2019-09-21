#pragma once
#include "Camera.h"
#include "Model.h"
#include "GameTimer.h"

class SceneManager
{
private:
	Microsoft::WRL::ComPtr<ID3D11Device> sc_pDevice;
	Microsoft::WRL::ComPtr<ID3D11DeviceContext> sc_pContext;

	Model* sc_pPlane;
	Model* sc_pObject;

	DirectX::XMFLOAT3 vDirLight;
	DirectX::XMFLOAT4 cDirLight;
	DirectX::XMFLOAT4 cAmbLight;

	//Input

	//camera
	Camera sc_Camera;

	GameTimer* sc_pGT;

public:
	SceneManager(ID3D11Device* device, ID3D11DeviceContext* context, GameTimer* gt);
	HRESULT Initialize();
	void Render();
	~SceneManager();
};