#pragma once
#include "Model.h"
#include "GameTimer.h"
#include "camera.h"


class SnowTexture
{
private:

	//Device, Context pointers
	ID3D11Device* s_pDevice;
	ID3D11DeviceContext* s_pContext;
	camera* s_pCamera;
	GameTimer* s_pGameTimer;

	//Shaders
	ID3D11ComputeShader* s_pDeformationCS;
	ID3D11ComputeShader* s_pFillCS;
	ID3D11ComputeShader* s_pinitTexCS;
	ID3D11VertexShader* s_pSnowVS;
	ID3D11PixelShader* s_pSnowPS;

	//Tessellation Shaders
	ID3D11HullShader* s_pSnowHS;
	ID3D11DomainShader* s_pSnowDS;

	//Heightmap that stores the deformation 32 bit- 1024*1024 pixels
	ID3D11ShaderResourceView* s_pDeformationHeightMapSRV;
	ID3D11UnorderedAccessView* s_pDeformationHeightMapUAV;

	//Constant buffers Compute
	ID3D11Buffer* s_pDeformCB;
	ID3D11Buffer* s_pFillCB;

	//Constant buffers Render
	ID3D11Buffer* s_pTessellationCB;
	ID3D11Buffer* s_pMaterialCB;

	//Sampler
	ID3D11SamplerState* s_pLinearSampler;

	//Input Layout
	ID3D11InputLayout* s_pInputLayout;

	//Vertex Buffer
	ID3D11Buffer* s_pVertexBuffer;
	XMFLOAT3* s_aVerticies[10];

	//Object pointer
	Model* s_pObject;

	//Texture for the snow
	ID3D11ShaderResourceView* s_pMaterialSRV;
	ID3D11ShaderResourceView* s_pDeformedSRV;

	//Rasterizer
	ID3D11RasterizerState* s_pRaster;



	//Structs
	struct TessellationCB
	{
		XMMATRIX mWorld; //64
		XMMATRIX mWVP;	//64
		float scale;
		XMFLOAT3 padding;
		//->128 +16 bytes
	};

	struct MaterialCB
	{
		XMFLOAT4 MatAmbC; //16
		XMFLOAT4 MatDifC; //16
		XMFLOAT4 LightPos; //16
		XMFLOAT4 CamPos; //16
		XMFLOAT4 fBaseTextureRepeat; //16
		//->80bytes
	};

	struct DeformCB
	{
		XMFLOAT3 vWorldPos;
		float scale;
		XMFLOAT2 TexCoord;
		XMFLOAT2 Padding;
	};

	struct FillCB
	{
		float Fillrate;
		XMFLOAT3 Padding;
	};

public:
	SnowTexture(ID3D11Device* device, ID3D11DeviceContext* context, camera* c, GameTimer* gt);
	HRESULT Initialize();
	void Draw(XMMATRIX* world, XMMATRIX* view, XMMATRIX* proj);
	~SnowTexture();
	void FillSnow(ID3D11Device* device, ID3D11DeviceContext* context, GameTimer* gt, float rate);
	void CalculateDepression(ID3D11Device* device, ID3D11DeviceContext* context, GameTimer* gt, int num);
	void InitTex(ID3D11Device* device, ID3D11DeviceContext* context);


	//Getter and setter
	void SetPosArray(XMFLOAT3* pos, int num);
};
