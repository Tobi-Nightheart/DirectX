#include "pch.h"
#include "SceneManager.h"
#include <minwinbase.h>
#include <minwinbase.h>


SceneManager::SceneManager(ID3D11RenderTargetView* defaultRTV, D3D11_VIEWPORT* viewport, ID3D11DepthStencilView* defaultDSV, GameTimer* gameTimer)
{
	sc_pDefaultRTV = defaultRTV;
	sc_pViewport = viewport;
	sc_pGameTimer = gameTimer;
	sc_pDefaultDSV = defaultDSV;
}


SceneManager::~SceneManager()
{
	

	SAFE_DELETE(sc_pPlane);
	SAFE_DELETE(sc_pCamera);
	SAFE_DELETE(sc_pCameraBirdsEye); //MAYBE remove this
	SAFE_DELETE(sc_pSkybox);
	SAFE_DELETE(sc_pText2D);
	SAFE_DELETE(sc_pRainCompute);	
	SAFE_DELETE(sc_pReflect);
	SAFE_DELETE(sc_pRoot_node);
	SAFE_DELETE(sc_pReflective);

	SAFE_ARRAY_DELETE(sc_pModel);
	SAFE_ARRAY_DELETE(sc_pNode_enemy);
	SAFE_ARRAY_DELETE(sc_pNode_obj);

	SAFE_RELEASE(sc_pSampler0);
	SAFE_RELEASE(sc_pBlendAlphaEnable);
	SAFE_RELEASE(sc_pBlendAlphaDisable);
	SAFE_RELEASE(sc_pRainTex);
	SAFE_RELEASE(sc_pTexture0);
	SAFE_RELEASE(sc_pTexture1);
	
	if(sc_pDefaultDSV)
	{
		sc_pDefaultDSV = nullptr;
	}

	if (sc_pDefaultRTV)
	{
		sc_pDefaultRTV = nullptr;
	}

	if(sc_pViewport)
	{
		sc_pViewport = nullptr;
	}
	

	sc_pInput = nullptr;
	if (sc_pDevice) sc_pDevice = nullptr;
	if (sc_pContext) sc_pContext = nullptr;
}

HRESULT SceneManager::Initialize(ID3D11Device* device, ID3D11DeviceContext* context, Input* input)
{
	HRESULT hr = S_OK;
	sc_pDevice = device;
	sc_pContext = context;
	sc_pInput = input;

	sc_pText2D = new Text2D("assets/font1Alpha.png", sc_pDevice, sc_pContext);
	//initialize scene nodes
	sc_pRoot_node = new scene_node();
	sc_pNode_enemy = new scene_node[5];
	sc_pNode_obj = new scene_node[5];

	//initialize textures
	string a = "assets/texture1.jpg";
	string b = "assets/texture2.jpg";
	string d = "assets/texture3.jpg";
	hr = D3DX11CreateShaderResourceViewFromFile(sc_pDevice, a.c_str(), nullptr, nullptr, &sc_pTexture0, nullptr);
	if (FAILED(hr)) return hr;
	hr = D3DX11CreateShaderResourceViewFromFile(sc_pDevice, b.c_str(), nullptr, nullptr, &sc_pTexture1, nullptr);
	if (FAILED(hr)) return hr;
	hr = D3DX11CreateShaderResourceViewFromFile(sc_pDevice, d.c_str(), nullptr, nullptr, &sc_pTexturePlane, nullptr);
	if (FAILED(hr)) return hr;
	string c = "assets/raindrop.dds";
	hr = D3DX11CreateShaderResourceViewFromFile(sc_pDevice, c.c_str(), nullptr, nullptr, &sc_pRainTex, nullptr);
	if (FAILED(hr)) return hr;
	
	//initialize skybox
	sc_pSkybox = new Skybox(sc_pDevice, sc_pContext);
	sc_pSkybox->Initialize();
	//init plane
	sc_pPlane=new Model(sc_pDevice, sc_pContext);
	hr = sc_pPlane->LoadObjModel((char*)"Resources/plane.obj");
	if (FAILED(hr)) return hr;
	sc_pPlane->SetTexture(sc_pTexturePlane);

	//sc_pModel 1-4 are objects and 5-10 are enemies
	sc_pModel = new Model[10];
	for(int i = 0; i<5; i++)
	{
		sc_pModel[i].SetDevice(sc_pDevice, sc_pContext);
		sc_pModel[i].LoadObjModel((char*)"Resources/cube.obj");
		sc_pModel[i].SetTexture(sc_pTexture0);
		sc_pNode_obj[i].SetModel(&sc_pModel[i]);
	}

	for(int i = 5; i<10; i++)
	{
		sc_pModel[i].SetDevice(sc_pDevice, sc_pContext);
		sc_pModel[i].LoadObjModel((char*)"Resources/sphere.obj");
		sc_pModel[i].SetTexture(sc_pTexture1);
		sc_pNode_enemy[i-5].SetModel(&sc_pModel[i]);
	}
	//placing the enemies
	sc_pNode_enemy[0].SetX(  1.0f, sc_pRoot_node);
	sc_pNode_enemy[0].SetZ(  5.0f, sc_pRoot_node);
	sc_pNode_enemy[1].SetX( 12.0f, sc_pRoot_node);
	sc_pNode_enemy[1].SetZ( -6.0f, sc_pRoot_node);
	sc_pNode_enemy[2].SetX( -6.0, sc_pRoot_node);
	sc_pNode_enemy[2].SetZ(  0.0f, sc_pRoot_node);
	sc_pNode_enemy[3].SetX(  9.0f, sc_pRoot_node);
	sc_pNode_enemy[3].SetZ( -2.0f, sc_pRoot_node);
	sc_pNode_enemy[4].SetX( -6.0f, sc_pRoot_node);
	sc_pNode_enemy[4].SetZ( -7.0f, sc_pRoot_node);
	//placing the objects
	sc_pNode_obj[0].SetX(1.0f, sc_pRoot_node);
	sc_pNode_obj[0].SetZ(3.0f, sc_pRoot_node);
	sc_pNode_obj[1].SetX(10.0f, sc_pRoot_node);
	sc_pNode_obj[1].SetZ(-6.0f, sc_pRoot_node);
	sc_pNode_obj[2].SetX(-4.0, sc_pRoot_node);
	sc_pNode_obj[2].SetZ(0.0f, sc_pRoot_node);
	sc_pNode_obj[3].SetX(7.0f, sc_pRoot_node);
	sc_pNode_obj[3].SetZ(-2.0f, sc_pRoot_node);
	sc_pNode_obj[4].SetX(-4.0f, sc_pRoot_node);
	sc_pNode_obj[4].SetZ(-7.0f, sc_pRoot_node);
	//add them too child nodes
	for(int i = 0; i<5; i++)
	{
		if (i == 0)
		{
			sc_pRoot_node->addChildNode(&sc_pNode_obj[0]);
			sc_pRoot_node->addChildNode(&sc_pNode_enemy[0]);
		}
		else 
		{
			sc_pNode_obj[i-1].addChildNode(&sc_pNode_obj[i]);
			sc_pRoot_node->addChildNode(&sc_pNode_enemy[i]);

		}
	}
	
	
	//init blend state
	D3D11_BLEND_DESC blend_desc;
	ZeroMemory(&blend_desc, sizeof(blend_desc));
	blend_desc.AlphaToCoverageEnable = false;
	blend_desc.IndependentBlendEnable = false;
	blend_desc.RenderTarget[0].BlendEnable = true;
	blend_desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	blend_desc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	blend_desc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	blend_desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	blend_desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
	blend_desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
	blend_desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
	hr = sc_pDevice->CreateBlendState(&blend_desc, &sc_pBlendAlphaEnable);
	if (FAILED(hr)) return hr;

	blend_desc.RenderTarget[0].BlendEnable = false;
	hr = sc_pDevice->CreateBlendState(&blend_desc, &sc_pBlendAlphaDisable);
	if (FAILED(hr)) return hr;
	
	sc_pReflect = new Reflective(sc_pDevice, sc_pContext);
	sc_pReflect->LoadObjModel((char*)"Resources/sphere.obj");




	sc_pCamera = new camera(0.0f, 1.0f, -5.0f, 0.0f, 0, sc_pRoot_node, true);
	sc_pCameraBirdsEye = new camera(0.0f, 20.0f, 0.0f, 0.0f, -90.0f, sc_pRoot_node, false);
	

	sc_pRainCompute = new RainCompute(sc_pDevice, sc_pContext, sc_pCamera, sc_pGameTimer);
	sc_pRainCompute->Initialize();

	sc_directional_light_shines_from = XMVectorSet(2.0f, 3.0f, 1.0f, 0.0f);
	sc_directional_light_colour = { .5f,.5f, .5f, 0.0f };
	
	sc_ambient_light_colour = { .1f,.1f, .1f, 0.0f };

	sc_pSnowTexture = new SnowTexture(sc_pDevice, sc_pContext, sc_pCamera, sc_pGameTimer);
	sc_pSnowTexture->Initialize();

	return S_OK;
}

void SceneManager::Render(GameTimer* gameTimer, string fps)
{
	//Dispatch the compute shaders
	sc_pSnowTexture->FillSnow(sc_pDevice, sc_pContext, sc_pGameTimer, 0.002f);


	XMMATRIX world, projection, view;
	world = XMMatrixIdentity();
	
	sc_pReflect->SetZ(5.0f);
	sc_pReflect->SetX(5.0f);
	XMMATRIX plane = XMMatrixIdentity();
	plane = XMMatrixTranslation(0, -5, 0);
	XMMATRIX snow = XMMatrixIdentity();
	snow = XMMatrixTranslation(0, -3, 0);
	//HeightMap Render Pass
	sc_pRainCompute->HeightMapPrep();
	projection = sc_pRainCompute->GetProj();
	view = sc_pRainCompute->GetView();
	sc_pPlane->DrawOpaque(&plane, &view, &projection);
	sc_pRoot_node->executeOpaque(&world, &view, &projection);
	sc_pReflect->DrawOpaque(&view, &projection);
	sc_pRainCompute->HeighMapPass(sc_pDefaultRTV, sc_pDefaultDSV, sc_pViewport);
	
	//Draw the scene in camera view
	sc_pInput->ReadInputStates();
	//put into its own input player input class
	sc_pInput->KeyboardInput(sc_pCamera, sc_pCameraBirdsEye, sc_pRainCompute, gameTimer->DeltaTime());
	camera* drawcam;
	if (sc_pCamera->GetActive()) drawcam = sc_pCamera;
	else
	{
		drawcam = sc_pCameraBirdsEye;
	}
	
	projection = drawcam->GetProjMatrix();
	
	view = drawcam->GetViewMatrix();

	sc_pSkybox->Draw(&view, &projection, drawcam);
	
	sc_pPlane->Draw(&plane, &view, &projection, sc_ambient_light_colour, sc_directional_light_shines_from, sc_directional_light_colour);
	sc_pRoot_node->execute(&world, &view, &projection, sc_ambient_light_colour, sc_directional_light_shines_from, sc_directional_light_colour);

	sc_pReflect->Draw(&view, &projection, sc_ambient_light_colour, sc_directional_light_shines_from, sc_directional_light_colour);
	
	for(int i = 0; i<5; i++)
	{
		sc_pNode_enemy[i].LookAtAZ(sc_pCamera->GetX(), sc_pCamera->GetZ());
		sc_pNode_enemy[i].MoveForward(0.5f, sc_pRoot_node, gameTimer->DeltaTime());
		sc_pNode_enemy[i].FluctuateHeight(.0005f, sc_pRoot_node, gameTimer->TotalTime());
		sc_pSnowTexture->SetPosArray(sc_pNode_enemy[i].GetWorldDeformPosition(), i);
	}

	//Calculate the deformation
	for (int i = 0; i < 5; i++) {
		sc_pSnowTexture->CalculateDepression(sc_pDevice, sc_pContext, sc_pGameTimer, i);
	}
	
	sc_pSnowTexture->Draw(&snow, &view, &projection);
	sc_pRainCompute->Draw();

	sc_pContext->OMSetBlendState(sc_pBlendAlphaEnable, nullptr, 0xffffffff);
	sc_pText2D->AddText(fps, -1.0f, 1.0f, .05f);
	sc_pText2D->RenderText();
	sc_pContext->OMSetBlendState(sc_pBlendAlphaDisable, nullptr, 0xffffffff);

}

void SceneManager::SetViewport(D3D11_VIEWPORT* viewport)
{
	sc_pViewport = viewport;
}

void SceneManager::SetRenderTarget(ID3D11RenderTargetView* defaultRTV)
{
	sc_pDefaultRTV = defaultRTV;
}
