#pragma once
#include "ObjFileModel.h"

class Model
{
private:
	struct MODEL_CB
	{
		DirectX::XMMATRIX WVP;
		DirectX::XMMATRIX World;
	};

	struct REFLECTIVE_CB
	{
		DirectX::XMMATRIX WVP;
		DirectX::XMMATRIX WorldView;
	};

	struct LIGHT_CB
	{
		DirectX::XMFLOAT3 EyePos;
		float pack1;
		DirectX::XMFLOAT3 DirToLight;
		float pack2;
		DirectX::XMFLOAT4 DirLightColor;
		DirectX::XMFLOAT4 Ambientdown;
		DirectX::XMFLOAT4 AmbientRange;
	};

	Microsoft::WRL::ComPtr<ID3D11Device> m_pDevice;
	Microsoft::WRL::ComPtr<ID3D11DeviceContext> m_pContext;

	ObjFileModel* m_pObject;

	//Rendering Pipeline Objects
	Microsoft::WRL::ComPtr<ID3D11VertexShader> m_pVShader;
	Microsoft::WRL::ComPtr<ID3D11PixelShader> m_pPShader;
	Microsoft::WRL::ComPtr<ID3D11InputLayout>m_pInputLayout;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_pTexture0;
	Microsoft::WRL::ComPtr<ID3D11SamplerState> m_pSampler0;
	Microsoft::WRL::ComPtr<ID3D11RasterizerState> m_pRaster;
	
	Microsoft::WRL::ComPtr<ID3D11VertexShader> m_pVShaderDepth;
	//Microsoft::WRL::ComPtr<ID3D11InputLayout> m_pIADepth; #remove in next version

	//Microsoft::WRL::ComPtr<CLSID_WICImagingFactory> m_pImagingFactory;

	//Constant Buffers
	Microsoft::WRL::ComPtr<ID3D11Buffer> m_pModelCB;
	Microsoft::WRL::ComPtr<ID3D11Buffer> m_pLightCB;

	//Collision BSC shorthand for Bounding sphere Radius
	DirectX::XMVECTOR m_BSC_Pos;
	float m_BSC_radius;

	//Positioning, angles and scale
	DirectX::XMVECTOR m_Position;
	float m_xA, m_yA, m_zA, m_scale;
	 
	//what about these pointers??
	char* ObjectName;
	char* TextureName;
	bool IsReflective;

	void CalculateModelCenterPoint();
	void CalculateBoundingSphereRadius();

public:
	Model();
	Model(ID3D11Device* device, ID3D11DeviceContext* context, char* ObjName, char* TexName, bool Reflective);
	~Model();
	HRESULT LoadObjModel();
	void Draw(DirectX::XMMATRIX& world, DirectX::XMMATRIX& view, DirectX::XMMATRIX& projection, DirectX::XMFLOAT4& AmbColor, DirectX::XMFLOAT3& DirVector, DirectX::XMFLOAT4& DirC);
	void DepthPass(DirectX::XMMATRIX& world, DirectX::XMMATRIX& view, DirectX::XMMATRIX& projection);

	
	bool CheckCollision(Model* model); // #remove in next version
	ObjFileModel* GetObject();
	//Faces the model towards a point on the XZ-Plane
	void LookAt(float x, float z); // #remove in next version
	void MoveForward(float d); // #remove in next version

#pragma region GetterSetterIncrementerMethods
	void SetTexture(ID3D11ShaderResourceView* tex);
	void SetSampler(ID3D11SamplerState* sampler);
	void SetDeviceAndContext(ID3D11Device* Device, ID3D11DeviceContext* Context);
	//change to a point set point to true, else it adds the vector to position
	void ChangePos(DirectX::XMVECTOR point, bool isPoint);
	void SetPosition(float x, float y, float z);
	void SetPosition(DirectX::XMVECTOR newPos);
	DirectX::XMVECTOR GetPosition();
	DirectX::XMFLOAT3 GetPosition3f();
	
	float GetXA();
	void IncXA(float a);
	void SetXA(float a);
	float GetYA();
	void IncYA(float a);
	void SetYA(float a);
	float GetZA();
	void IncZA(float a);
	void SetZA(float a);
	
	float GetScale();
	void IncScale(float v);
	void SetScale(float s);

	float GetBoundingSphereRadius();
	DirectX::XMMATRIX GetWorld();
	DirectX::XMVECTOR GetBoundingSphereWSPosition();
	DirectX::XMFLOAT3 GetBoundingSphereWSPosition3f();
#pragma endregion
	
};