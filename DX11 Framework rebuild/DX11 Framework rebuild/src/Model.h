#pragma once
#include "objectfilemodel.h"

class Model
{
private:
	struct MODEL_CB
	{
		XMMATRIX WVP;
		XMMATRIX World;
	};

	struct LIGHT_CB
	{
		XMFLOAT3 EyePos;
		float pack1;
		XMFLOAT3 DirToLight;
		float pack2;
		XMFLOAT4 DirLightColor;
		XMFLOAT4 Ambientdown;
		XMFLOAT4 AmbientRange;
	};

	Microsoft::WRL::ComPtr<ID3D11Device> m_pDevice;
	Microsoft::WRL::ComPtr<ID3D11DeviceContext> m_pContext;

	Microsoft::WRL::ComPtr<ObjFileModel> m_pObject;

	//Rendering Pipeline Objects
	Microsoft::WRL::ComPtr<ID3D11VertexShader> m_pVShader;
	Microsoft::WRL::ComPtr<ID3D11PixelShader> m_pPShader;
	Microsoft::WRL::ComPtr<ID3D11InputLayout>m_pInputLayout;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_pTexture0;
	Microsoft::WRL::ComPtr<ID3D11SamplerState> m_pSampler0;
	Microsoft::WRL::ComPtr<ID3D11RasterizerState> m_pRaster;
	
	Microsoft::WRL::ComPtr<ID3D11VertexShader> m_pVShaderDepth;
	Microsoft::WRL::ComPtr<ID3D11InputLayout> m_pIADepth;

	//Constant Buffers
	Microsoft::WRL::ComPtr<ID3D11Buffer> m_pModelCB;
	Microsoft::WRL::ComPtr<ID3D11Buffer> m_pLightCB;

	//Collision BSC shorthand for Bounding sphere Radius
	XMVECTOR m_BSC_Pos;
	float m_BSC_radius;

	//Positioning, angles and scale
	XMVECTOR m_Position;
	float m_xA, m_yA, m_zA, m_scale;
	 
	//what about these pointers??
	char* ObjectName;
	char* TextureName;
	bool IsReflective;

	void CalculateModelCenterPoint();
	void CalculateBoundingSphereRadius();

public:
	Model();
	Model(ID3D11Device* device, ID3D11DeviceContext* context, char* ObjectName, char* TextureName, bool IsReflective);
	~Model();
	HRESULT LoadObjModel(char* filename);
	void Draw(XMMATRIX& world, XMMATRIX& view, XMMATRIX& projection, XMFLOAT4& AmbColor, XMFLOAT3& DirVector, XMFLOAT4& DirC);
	void DepthPass(XMMATRIX& world, XMMATRIX& view, XMMATRIX& projection);

	
	bool CheckCollision(Model* model);
	ObjFileModel* GetObject();
	//Faces the model towards a point on the XZ-Plane
	void LookAt(float x, float z);
	void MoveForward(float d);

#pragma region GetterSetterIncrementerMethods
	void SetTexture(ID3D11ShaderResourceView* tex);
	void SetSampler(ID3D11SamplerState* sampler);
	void SetDeviceAndContext(ID3D11Device* Device, ID3D11DeviceContext* Context);
	void ChangePos(XMVECTOR delta);
	void IncXA(float a);
	void IncYA(float a);
	void IncZA(float a);
	void IncScale(float v);
	void SetPosition(XMVECTOR newPos);
	void SetXA(float x);
	void SetYA(float y);
	void SetZA(float z);
	void SetScale(float s);
	XMVECTOR GetPosition();
	float GetXA();
	float GetYA();
	float GetZA();
	float GetScale();
	float GetBoundingSphereRadius();
	XMMATRIX GetWorld(); ///CHANGE??
	XMVECTOR GetBoundingSpherePosition();
#pragma endregion
	
};