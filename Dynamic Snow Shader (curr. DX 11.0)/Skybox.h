#pragma once
class Skybox
{
private:

	ID3D11Device* s_pD3DDevice;
	ID3D11DeviceContext* s_pContext;

	ID3D11VertexShader* s_pVS;
	ID3D11PixelShader* s_pPS;
	ID3D11InputLayout* s_pInputLayout;
	ID3D11Buffer* s_pConstantBuffer;
	ID3D11Buffer* s_pVBuffer;
	ID3D11ShaderResourceView* s_pTexture;
	ID3D11SamplerState* s_pSampler0;

	//Skybox specific
	ID3D11RasterizerState* s_pRasterSolid = 0;
	ID3D11RasterizerState* s_pRasterSkybox = 0;
	ID3D11DepthStencilState* s_pDepthWriteSolid = 0;
	ID3D11DepthStencilState* s_pDepthWriteSkybox = 0;
public:
	Skybox(ID3D11Device* device, ID3D11DeviceContext* context);
	HRESULT Initialize();
	void Draw(XMMATRIX* view, XMMATRIX* projection, camera* cam);
	~Skybox();
};

