#pragma once
 
#define _XM_NO_INTRINSICS_
#define _XM_NO_ALIGNMENT

#include <d3d11.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <DirectXPackedVector.h>
#include <stdio.h>
#include <string>
#include <vector>
#include <wrl.h>

#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "D3D11.lib")
#pragma comment(lib, "dxgi.lib")

class ObjFileModel
{
private:
	Microsoft::WRL::ComPtr<ID3D11Device> pDevice;
	Microsoft::WRL::ComPtr<ID3D11DeviceContext> pContext;

	int loadfile(char* fname);

	char* fbuffer;
	long fbuffersize;
	size_t actualsize;

	void parsefile();
	bool getnextline();
	bool getnexttoken(int& tokenstart, int& tokenlenght);
	
	unsigned int tokenptr;

	bool createVB();

	Microsoft::WRL::ComPtr<ID3D11Buffer> pVertexBuffer;

public:
	struct xyz { float x, y, z; }; //used for vertices and normals during parse
	struct xy { float x, y; };	// used for texture coordinates during parse

	struct MODEL_POS_TEX_NORM_VERTEX
	{
		DirectX::XMFLOAT3 Pos;
	 	DirectX::XMFLOAT2 TexCoord;
		DirectX::XMFLOAT3 Normal;
	};

	std::string filename;

	ObjFileModel(char* filename, ID3D11Device* device, ID3D11DeviceContext* context);
	~ObjFileModel();

	void Draw(void);

	//lists for parsing
	std::vector <xyz> position_list;
	std::vector <xy> texcoord_list;
	std::vector <xyz> normal_list;
	std::vector <int> pindices, tindices, nindices;

	MODEL_POS_TEX_NORM_VERTEX* vertices;
	unsigned int numVerts;
};