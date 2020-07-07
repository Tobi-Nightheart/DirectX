#pragma once
#include <memory>
#include "Camera.h"
#include "GameTimer.h"

class RainCompute
{
private:
	//Device and Context
	Microsoft::WRL::ComPtr<ID3D11Device> MyDevice;
	Microsoft::WRL::ComPtr<ID3D11DeviceContext> MyContext;

	//Constant buffers
	Microsoft::WRL::ComPtr<ID3D11Buffer> MyDepthCB;
	Microsoft::WRL::ComPtr<ID3D11Buffer> MySimulateCB;
	Microsoft::WRL::ComPtr<ID3D11Buffer> MyRendererCB;

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
	std::shared_ptr<Camera> MyCamera;
	std::shared_ptr<GameTimer> MyGameTimer;
	
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
	Microsoft::WRL::ComPtr<ID3D11Buffer> MySimBuffer;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> MySimBufferView;

	//Depth texture
	Microsoft::WRL::ComPtr<ID3D11Texture2D> MyDepth;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilView> MyHeightMapDSV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> MyHeightMapSRV;

	//noise and streak texture
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> MyNoiseSRV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> MyStreakSRV;

	//UAV
	Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> MyUAV;

	//blendStates
	Microsoft::WRL::ComPtr<ID3D11BlendState> MyBlendRender;
	Microsoft::WRL::ComPtr<ID3D11BlendState> MyBlendDefault;

	//Shader objects
	Microsoft::WRL::ComPtr<ID3D11VertexShader> MyVS_Height;
	Microsoft::WRL::ComPtr<ID3D11VertexShader> MyVS_Out;
	Microsoft::WRL::ComPtr<ID3D11PixelShader> MyPS;
	Microsoft::WRL::ComPtr<ID3D11ComputeShader> MyCS;

	//sample state
	Microsoft::WRL::ComPtr<ID3D11SamplerState> MySampler0;

	//rasterizer state
	Microsoft::WRL::ComPtr<ID3D11RasterizerState> MyRasterState;

	//InputLayout
	Microsoft::WRL::ComPtr<ID3D11InputLayout> MyInputLayout;
	Microsoft::WRL::ComPtr<ID3D11InputLayout> MyDepthIA;

	//Init float data
	float* MyInitFLOAT;

	//floats
	float MyMaxWindVariance;
	float MyVertSpeed;
	float MyStreakScale;
	float MyDensity;

	//vector
	DirectX::XMVECTOR MyBoundCenter;
	DirectX::XMVECTOR MyBoundHalfSize;
	DirectX::XMVECTOR MyCurWindEffect;

	//Matrices
	DirectX::XMMATRIX MyRainViewProj;
	DirectX::XMMATRIX MyRainView;
	DirectX::XMMATRIX MyRainProj;

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

	void SetDensity(float d) { MyDensity = d; }

	RainCompute operator= (RainCompute rhs) = delete;
};