#pragma once
#include "scene_node.h"
#include "camera.h"
#include "Model.h"
#include "Skybox.h"
#include "Reflective.h"
#include "Text2D.h"
#include "Input.h"
#include "RainCompute.h"
#include  "SnowTexture.h"

class SceneManager
{
private:
	ID3D11Device* sc_pDevice;
	ID3D11DeviceContext* sc_pContext;
	//plane
	Model* sc_pPlane;


	//Nodes
	scene_node* sc_pRoot_node;
	scene_node* sc_pNode_enemy;
	scene_node* sc_pNode_obj;
	scene_node* sc_pReflective;

	//Lighting
	XMVECTOR sc_directional_light_shines_from;
	XMFLOAT4 sc_directional_light_colour;
	XMFLOAT4 sc_ambient_light_colour;

	//Model
	Model* sc_pModel;

	//Skybox
	Skybox* sc_pSkybox;

	//reflective
	Reflective* sc_pReflect;

	

	//2d text
	Text2D* sc_pText2D;
	ID3D11BlendState* sc_pBlendAlphaEnable;
	ID3D11BlendState* sc_pBlendAlphaDisable;

	//Texture pointer
	ID3D11ShaderResourceView* sc_pTexture0; 
	ID3D11ShaderResourceView* sc_pTexture1;
	ID3D11ShaderResourceView* sc_pTexturePlane;
	ID3D11SamplerState* sc_pSampler0; //sampler state for texture

	//Input
	Input* sc_pInput;
	
	//camera
	camera* sc_pCamera;
	camera* sc_pCameraBirdsEye;

	GameTimer* sc_pGameTimer;

	//for rain compute shader
	//rain
	ID3D11ShaderResourceView* sc_pRainTex;
	ID3D11RenderTargetView* sc_pDefaultRTV;
	D3D11_VIEWPORT* sc_pViewport;
	RainCompute* sc_pRainCompute;
	ID3D11DepthStencilView* sc_pDefaultDSV;

	//for the 3d texture
	SnowTexture* sc_pSnowTexture;

public:
	SceneManager(ID3D11RenderTargetView* defaultRTV, D3D11_VIEWPORT* viewport, ID3D11DepthStencilView* defaultDSV, GameTimer* gameTimer);
	~SceneManager();
	HRESULT Initialize(ID3D11Device* device, ID3D11DeviceContext* context, Input* input);
	void Render(GameTimer* gameTimer, string fps);
	void SetViewport(D3D11_VIEWPORT* viewport);
	void SetRenderTarget(ID3D11RenderTargetView* defaultRTV);
};

