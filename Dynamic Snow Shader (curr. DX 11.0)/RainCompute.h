#pragma once
#include "camera.h"
#include "GameTimer.h"

class RainCompute
{
private:
	//Device pointer
	ID3D11Device* r_pDevice;
	ID3D11DeviceContext* r_pContext;
	
	//Constant buffers
	ID3D11Buffer* r_pCBDepth;
	ID3D11Buffer* r_pCBSimulate;
	ID3D11Buffer* r_pCBRenderer;

	struct HeightCB
	{
		XMMATRIX ToHeight;//64 bytes
	};

	struct RainCB
	{
		XMMATRIX HeightSpace;//64 bytes
		XMFLOAT3 BoundCenter; //12 bytes
		float deltaTime; //4 bytes
		XMFLOAT3 BoundHalfSize; //12 bytes
		float WindVariation; //4 bytes
		XMFLOAT2 WindForce; //8 bytes
		float VertSpeed; //4 bytes
		float HeightMapSize; //4 bytes
		//112
	};

	struct DrawCB
	{
		XMMATRIX ViewProj;//64 bytes
		XMFLOAT3 ViewDir;//12 bytes
		float scale;//4 bytes
		XMFLOAT4 AmbientColor;//16 bytes
		//96
	};

	HeightCB cb_height_values;
	RainCB cb_rain_values;
	DrawCB cb_draw_values;

	//utilities 
	camera* r_pCamera;
	GameTimer* r_pGameTimer;

	struct BoundBox
	{
		XMVECTOR Pos;
		XMVECTOR Dimension;
	};

	struct RainDrop
	{
		XMFLOAT3 Pos;
		XMFLOAT3 Vel;
		float State;
	};

	const int size=32*4;
	//vertex buffer	
	ID3D11Buffer* r_pSimBuffer;
	ID3D11ShaderResourceView* r_pSimBufferView;

	//depth texture
	ID3D11Texture2D* r_pHeightMap2D;
	ID3D11DepthStencilView* r_pHeightMapDSV;
	ID3D11ShaderResourceView* r_pHeightMapSRV;

	//noise and streak texture and SRV
	ID3D11ShaderResourceView* r_pNoiseSRV;
	ID3D11ShaderResourceView* r_pStreakSRV;	

	//unordered access view
	ID3D11UnorderedAccessView* r_pUAV;

	//blend states
	ID3D11BlendState* r_pBlendRender;
	ID3D11BlendState* r_pBlendDefault;

	//Shader objects
	ID3D11VertexShader* r_pVS_Height;
	ID3D11VertexShader* r_pVS_Out;
	ID3D11PixelShader* r_pPS;
	ID3D11ComputeShader* r_pCS;

	//Sample state
	ID3D11SamplerState* r_pSampler0;

	//Rasterizer State
	ID3D11RasterizerState* r_pRasterState;

	//Initialize float data
	float* initFLOAT;

	//Input Layout
	ID3D11InputLayout* r_pInputLayout;
	ID3D11InputLayout* r_pHeightLayout;

	//Float
	float r_fMaxWindVariance;
	float r_fVerticalSpeed;
	float r_fStreakScale;
	float r_fDensity;
	
	//Vectors
	XMVECTOR r_vBoundCenter;
	XMVECTOR r_vBoundHalfSize;
	XMVECTOR r_vCurWindEffect;

	//Matrices
	XMMATRIX r_mRainViewProj;
	XMMATRIX r_mRainView;
	XMMATRIX r_mRainProj;


public:
	RainCompute(ID3D11Device* device, ID3D11DeviceContext* context, camera* camera, GameTimer* gameTimer);
	HRESULT Initialize();
	void Draw();
	void HeighMapPass(ID3D11RenderTargetView* defaultRTV, ID3D11DepthStencilView* defaultDSV, D3D11_VIEWPORT* viewport);
	void HeightMapPrep();
	void PreRender();
	void Render();
	void UpdateTransforms();
	XMMATRIX GetView();
	XMMATRIX GetProj();
	~RainCompute();

	void SetDensity(float d);
	//diffrent randomness
	ID3D11ShaderResourceView* CreateRandomTexture1DSRV(ID3D11Device* device);
	float RandF();
	float RandF(float a, float b);
};

