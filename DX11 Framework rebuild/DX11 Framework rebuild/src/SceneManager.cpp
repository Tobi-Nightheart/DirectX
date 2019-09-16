#include "pcH.h"
#include "SceneManager.h"

using namespace DirectX;

SceneManager::SceneManager(ID3D11Device* device, ID3D11DeviceContext* context, GameTimer* gt)
{
	sc_pDevice = device;
	sc_pContext = context;
	sc_pGT = gt;

	sc_pObject = nullptr;
	sc_pPlane = nullptr;
	sc_pCamera = nullptr;

	vDirLight = XMFLOAT3(5.0f, -7.0f, 3.0f);
	cDirLight = XMFLOAT4(1.0f, 0.5f, 0.5f, 0.8f);
	cAmbLight = XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
}

HRESULT SceneManager::Initialize()
{
	HRESULT hr = S_OK;

	sc_pCamera = new Camera();
	sc_pCamera->SetPosition(XMFLOAT3(0.0f, 0.0f, -5.0f));

	sc_pPlane = new Model(sc_pDevice.Get(), sc_pContext.Get(), (char*)"Resources/plane.obj", (char*)"assets/texture3.jpg", false);
	hr = sc_pPlane->LoadObjModel();
	if (FAILED(hr)) return hr;

	sc_pObject = new Model(sc_pDevice.Get(), sc_pContext.Get(), (char*)"Resources/cube.obj", (char*)"assets/texture1.jpg", false);
	hr = sc_pObject->LoadObjModel();
	if (FAILED(hr)) return hr;


	return S_OK;
}

void SceneManager::Render()
{
	XMMATRIX world, view, proj;
	world = XMMatrixIdentity();
	view = sc_pCamera->GetView();
	proj = sc_pCamera->GetProj();

	sc_pPlane->Draw(world, view, proj, cAmbLight, vDirLight, cDirLight);
	sc_pObject->Draw(world, view, proj, cAmbLight, vDirLight, cDirLight);

}

SceneManager::~SceneManager()
{
	if (sc_pObject)
	{
		delete sc_pObject;
		sc_pObject = nullptr;
	}
	if (sc_pPlane)
	{
		delete sc_pPlane;
		sc_pPlane = nullptr;
	}
	if (sc_pCamera)
	{
		delete sc_pCamera;
		sc_pCamera = nullptr;
	}
	if(sc_pGT) sc_pGT = nullptr;

}

