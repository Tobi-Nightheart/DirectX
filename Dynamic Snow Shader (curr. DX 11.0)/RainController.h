#pragma once
#include "GameTimer.h"

class RainController
{
private:
	ID3D11Device* rc_pDevice;
	ID3D11DeviceContext* rc_pImmediateContext;
	
	ID3D11VertexShader* rc_pVSOUT;
	ID3D11PixelShader* rc_pPS;
	ID3D11GeometryShader* rc_pGSOUT;
	ID3D11VertexShader* rc_pVSDraw;
	ID3D11GeometryShader* rc_pGSDraw;
	ID3D11SamplerState* rc_pSample0;
	ID3D11DepthStencilState* rc_pDepthStencilSolid;
	ID3D11DepthStencilState* rc_pDepthStencilNoWrite;
	ID3D11DepthStencilState* rc_pDepthStencilDisable;
	ID3D11InputLayout* rc_pInputLayoutPosition;
	ID3D11InputLayout* rc_pInputLayoutParticle;

	ID3D11ShaderResourceView* rc_texArraySRV;
	ID3D11ShaderResourceView* rc_randomTexSRV;

	UINT rc_MaxParticles;
	bool rc_FirstRun;

	float rc_Age;
	float rc_TimeStep;
	float rc_GameTime;

	//ParticleEffect* rc_FX;
	XMFLOAT3 rc_EyePosW;
	XMFLOAT3 rc_EmitPosW;
	XMFLOAT3 rc_EmitDirW;

	ID3D11Buffer* rc_InitVB;
	ID3D11Buffer* rc_DrawVB;
	ID3D11Buffer* rc_StreamOutVB;
	ID3D11Buffer* rc_cb0;

	camera* rc_pCamera;

	struct cbRain
	{
		XMMATRIX ViewProj;
		XMVECTOR EyePosition;
		XMFLOAT3 EmitPos;
		float TimeStep;
		XMFLOAT3 EmitDirW;
		float GameTime;
		//size 108
	};

	struct Particle
	{
		XMFLOAT3 InitialPos;
		XMFLOAT3 InitialVel;
		XMFLOAT2 Size;
		float Age;
		unsigned int Type;
	};

public:
	RainController(ID3D11Device* d3d11Device, ID3D11DeviceContext* d3d11DeviceContext, ID3D11ShaderResourceView* tex, camera* c);
	HRESULT Initialize();
	void Draw(XMMATRIX* view, XMMATRIX* proj, camera* c, GameTimer* time);
	void Update(float dt, float gameTime);
	~RainController();
	void Reset();
	//random generation helper functions
	ID3D11ShaderResourceView* CreateRandomTexture1DSRV();
	//returns float in [0,1]
	float RandF();
	//returns float in [a, b]
	float RandF(float a, float b);

	XMVECTOR RandomUnitVec3();

};

