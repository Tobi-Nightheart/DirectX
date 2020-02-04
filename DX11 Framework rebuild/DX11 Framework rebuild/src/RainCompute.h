#pragma once
#include <memory>
#include "Camera.h"
#include "GameTimer.h"

class RainCompute
{
private:
	//Device and Context
	Microsoft::WRL::ComPtr<ID3D11Device> r_pDevice;
	Microsoft::WRL::ComPtr<ID3D11DeviceContext> r_pContext;

	//Constant buffers
	Microsoft::WRL::ComPtr<ID3D11Buffer> r_pDepthCB;
	Microsoft::WRL::ComPtr<ID3D11Buffer> r_pSimulateCB;
	Microsoft::WRL::ComPtr<ID3D11Buffer> r_pRendererCB;

	struct DepthCB //64 bytes
	{
		DirectX::XMMATRIX ToHeight;
	};

	struct SimulateCB //112 bytes 
	{
		DirectX::XMMATRIX HeightSpace;
		DirectX::XMFLOAT3 BoundCenter;
		float deltaTime;
		DirectX::XMFLOAT3 BoundHalfSize;
		float WindVariation;
		DirectX::XMFLOAT2 WindForce;
		float VertSpeed;
		float HeightMapSize;
	};

	struct RendererCB //96 bytes
	{
		DirectX::XMMATRIX ViewProj;
		DirectX::XMFLOAT3 ViewDir;
		float scale;
		DirectX::XMFLOAT4 AmbientColor;
	};

	//CHANGE
	//DepthCB cb_height_values;
	//SimulateCB cb_simulate_value;
	//RendererCB cb_draw_value;

	//utilities
	std::shared_ptr<Camera> r_pCamera;
	std::shared_ptr<GameTimer> r_pGameTimer;
	
	struct BoundBox //CHANGE
	{
		DirectX::XMVECTOR Pos;
		DirectX::XMVECTOR Dimension;
	};

	struct RainDrop
	{
		DirectX::XMFLOAT3 Pos;
		DirectX::XMFLOAT3 Vel;
		float State;
	};

	const int DimSize = 32 * 4;

	//VertexBuffer
	Microsoft::WRL::ComPtr<ID3D11Buffer> r_pSimBuffer;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> r_pSimBufferView;

	//Depth texture
	Microsoft::WRL::ComPtr<ID3D11Texture2D> r_pDepth;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilView> r_pHeightMapDSV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> r_pHeightMapSRV;

	//noise and streak texture
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> r_pNoiseSRV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> r_pStreakSRV;

	//UAV
	Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> r_pUAV;

	//blendStates
	Microsoft::WRL::ComPtr<ID3D11BlendState> r_pBlendRender;
	Microsoft::WRL::ComPtr<ID3D11BlendState> r_pBlendDefault;

	//Shader objects
	Microsoft::WRL::ComPtr<ID3D11VertexShader> r_pVS_Height;
	Microsoft::WRL::ComPtr<ID3D11VertexShader> r_pVS_Out;
	Microsoft::WRL::ComPtr<ID3D11PixelShader> r_pPS;
	Microsoft::WRL::ComPtr<ID3D11ComputeShader> r_pCS;

	//sample state
	Microsoft::WRL::ComPtr<ID3D11SamplerState> r_pSampler0;

	//rasterizer state
	Microsoft::WRL::ComPtr<ID3D11RasterizerState> r_pRasterState;

	//InputLayout
	Microsoft::WRL::ComPtr<ID3D11InputLayout> r_pInputLayout;
	Microsoft::WRL::ComPtr<ID3D11InputLayout> r_pDepthIA;

	//Init float data
	float* initFLOAT;

	//floats
	float r_fMaxWindVariance;
	float r_fVertSpeed;
	float r_fStreakScale;
	float r_fDensity;

	//vector
	DirectX::XMVECTOR r_vBoundCenter;
	DirectX::XMVECTOR r_vBoundHalfSize;
	DirectX::XMVECTOR r_vCurWindEffect;

	//Matrices
	DirectX::XMMATRIX r_mRainViewProj;
	DirectX::XMMATRIX r_mRainView;
	DirectX::XMMATRIX r_mRainProj;

public:
	RainCompute(ID3D11Device* device, ID3D11DeviceContext* context, std::shared_ptr<Camera> camera, std::shared_ptr<GameTimer> gt);
	HRESULT Init();
	void Draw();
	void DepthPass(ID3D11RenderTargetView& defaultRTV, ID3D11DepthStencilView& defaultDSV, D3D11_VIEWPORT& viewport);
	void DepthPassPrep();
	void SetDepthCB(DirectX::XMMATRIX world);
	void Simulate();
	void Render();
	void UpdateTransforms();
	DirectX::XMMATRIX GetView();
	DirectX::XMMATRIX GetProj();
	~RainCompute();

	void SetDensity(float d) { r_fDensity = d; }

	RainCompute operator= (RainCompute rhs) = delete;
};