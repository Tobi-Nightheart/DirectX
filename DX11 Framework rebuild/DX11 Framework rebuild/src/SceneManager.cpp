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

	vDirLight = XMFLOAT3(10.0f, 10.0f, 0.0f);
	cDirLight = XMFLOAT4(0.45f, 0.5f, 0.45f, 0.1f);
	cAmbLight = XMFLOAT4(0.1f, 0.1f, 0.1f, 0.1f);
}

HRESULT SceneManager::Initialize()
{
	HRESULT hr = S_OK;

	sc_Camera.SetPosition(XMFLOAT3(0.0f, 10.0f, -25.0f));
	sc_Camera.LookAt(sc_Camera.GetPosition(), XMVectorSet(0, 0, 0, 1), XMVectorSet(0,1,0,0));

	sc_pPlane = new Model(sc_pDevice.Get(), sc_pContext.Get(), (char*)"Resources/plane.obj", (char*)"assets/texture3.jpg", false);
	hr = sc_pPlane->LoadObjModel();
	if (FAILED(hr)) return hr;
	
	sc_pObject = new Model(sc_pDevice.Get(), sc_pContext.Get(), (char*)"Resources/sphere.obj", (char*)"assets/texture1.jpg", false);
	hr = sc_pObject->LoadObjModel();
	if (FAILED(hr)) return hr;
	sc_pObject->SetPosition(0, 1, 0);


	return S_OK;
}

void SceneManager::Render()
{
	sc_Camera.UpdateViewMatrix();

	XMMATRIX world, view, proj, translate;
	world = XMMatrixIdentity();
	view = sc_Camera.GetView();
	proj = sc_Camera.GetProj();
	sc_pObject->IncYA(0.01f);
	sc_pPlane->Draw(world, view, proj, cAmbLight, vDirLight, cDirLight, sc_Camera);
	sc_pObject->Draw(world, view, proj, cAmbLight, vDirLight, cDirLight, sc_Camera);

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
	if(sc_pGT) sc_pGT = nullptr;

}

