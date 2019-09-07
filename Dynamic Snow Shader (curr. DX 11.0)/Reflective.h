#pragma once
class Model;

class Reflective
{
public:
	
private:
	void CalculateModelCenterPoint();
	void CalculateBoundingSphereRadius();

	ID3D11Buffer* r_pCBPixelIn;
	struct Light_CB
	{
		XMMATRIX world;
		XMVECTOR EyePos;
		XMVECTOR DirToLight;
		XMFLOAT4 DirLightColor;
		XMFLOAT4 ambientdown;
		XMFLOAT4 ambientrange;

	};

	struct REFLECTIVE_CONSTANT_BUFFER {
		XMMATRIX WorldViewProjection; //64 bytes
		XMMATRIX WorldView; // 64 bytes
		//total of 128
	};

	ID3D11Device* r_pD3DDevice;
	ID3D11DeviceContext*  r_pImmediateContext;

	ObjFileModel* r_pObject;
	ID3D11VertexShader* r_pVShader;
	ID3D11PixelShader*  r_pPShader;
	ID3D11InputLayout* r_pInputLayout;
	ID3D11Buffer* r_pConstantBuffer;
	ID3D11ShaderResourceView* r_pTexture0;
	ID3D11SamplerState* r_pSampler0;

	//Collision 
	float r_bounding_sphere_center_x;
	float r_bounding_sphere_center_y;
	float r_bounding_sphere_center_z;
	float r_bounding_sphere_radius;


	float r_px, r_py, r_pz;
	float r_xAngle, r_yAngle, r_zAngle;
	float r_Scale;

	//Resources for HeightMapDrawing
	ID3D11VertexShader* r_pVSOpaque;
	ID3D11InputLayout* r_pIAOpaque;


public:
	Reflective(ID3D11Device* d3d11Device, ID3D11DeviceContext* d3d11DeviceContext);
	~Reflective();
	HRESULT LoadObjModel(char* filename);
	void Draw(XMMATRIX * view, XMMATRIX * projection, XMFLOAT4 AmbC, XMVECTOR DirV, XMFLOAT4 DirC);
	void DrawOpaque(XMMATRIX* view, XMMATRIX* projection);
	XMVECTOR GetBoundingSphereWorldSpacePosition();
	bool CheckCollision(Model* model);
	void IncX(float num);
	void IncY(float num);
	void IncZ(float num);
	void IncXAngle(float num);
	void IncYAngle(float num);
	void IncZAngle(float num);
	void IncScale(float num);
	void SetX(float x);
	void SetY(float y);
	void SetZ(float z);
	void SetXAngle(float xAngle);
	void SetYAngle(float yAngle);
	void SetZAngle(float zAngle);
	void SetScale(float scale);
	float GetX();
	float GetY();
	float GetZ();
	float GetXAngle();
	float GetYAngle();
	float GetZAngle();
	float GetScale();
	float GetBoundingSphereRadius();
	XMMATRIX GetWorld();
};

