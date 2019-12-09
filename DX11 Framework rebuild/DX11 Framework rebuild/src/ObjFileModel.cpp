//turned off fopen warnings
#define _CRT_SECURE_NO_WARNINGS

#include "ObjFileModel.h"

void ObjFileModel::Draw(void)
{
	UINT stride = sizeof(MODEL_POS_TEX_NORM_VERTEX);
	UINT offset = 0;
	pContext->IASetVertexBuffers(0, 1, pVertexBuffer.GetAddressOf(), &stride, &offset);
	pContext->Draw(numVerts, 0);
}

ObjFileModel::ObjFileModel(char* fname, ID3D11Device* device, ID3D11DeviceContext* context)
{
	pDevice = device;
	pContext = context;

	if (loadfile(fname)==0)
	{
		filename = (char*) "File not Loaded";
		return;
	}

	filename = fname;

	parsefile();

	createVB();

	delete[] fbuffer;
}

int ObjFileModel::loadfile(char* fname)
{
	FILE* pFile;
	pFile = fopen(fname, "r");
	if (pFile == nullptr) { MessageBox(nullptr, L"Failed to open model file!", L"FILE FAILURE", MB_OK); return 0; }

	//get file size
	fseek(pFile, 0, SEEK_END);
	fbuffersize = ftell(pFile);
	rewind(pFile);

	//allocate memory for entire file
	fbuffer = new char[fbuffersize + 1];
	if (fbuffer == nullptr) { fclose(pFile); MessageBox(nullptr, L"Failed to allocate memory for model file!", L"FILE FAILURE", MB_OK); return 0; }

	actualsize = fread(fbuffer, 1, fbuffersize, pFile);
	if (actualsize == 0) { fclose(pFile); MessageBox(nullptr, L"Failed to  read model file!", L"FILE FAILURE", MB_OK); return 0; }

	fbuffer[actualsize] = '\n';
	fclose(pFile);
	return 1;
}

void ObjFileModel::parsefile()
{
	tokenptr = 0;
	int tokenstart, tokenlength;

	xyz tempxyz;
	xy tempxy;

	bool success;
	int line = 0;

	do 
	{
		line++;

		if (!getnexttoken(tokenstart, tokenlength)) continue;

		if (strncmp(&fbuffer[tokenstart], "v ", 2) == 0)
		{
			success = true;
			success = success && getnexttoken(tokenstart, tokenlength);
			tempxyz.x = (float)atof(&fbuffer[tokenstart]);
			success = success && getnexttoken(tokenstart, tokenlength);
			tempxyz.y = (float)atof(&fbuffer[tokenstart]);
			success = success && getnexttoken(tokenstart, tokenlength);
			tempxyz.z = (float)atof(&fbuffer[tokenstart]);

			if (!success) { MessageBox(nullptr, L"ERROR: badly formatted vertex!", L"FILE ERROR", MB_OK); }
			position_list.push_back(tempxyz);
		}
		else if (strncmp(&fbuffer[tokenstart], "vt", 2) == 0)
		{
			success = true;
			success = success && getnexttoken(tokenstart, tokenlength);
			tempxy.x = (float)atof(&fbuffer[tokenstart]);
			success = success && getnexttoken(tokenstart, tokenlength);
			tempxy.y = (float)atof(&fbuffer[tokenstart]);

			if (!success) { MessageBox(nullptr, L"ERROR: badly formatted Texcoord!", L"FILE ERROR", MB_OK); }

			texcoord_list.push_back(tempxy);
		}
		else if (strncmp(&fbuffer[tokenstart], "vn", 2) == 0)
		{
			success = true;
			success = success && getnexttoken(tokenstart, tokenlength);
			tempxyz.x = (float)atof(&fbuffer[tokenstart]);
			success = success && getnexttoken(tokenstart, tokenlength);
			tempxyz.y = (float)atof(&fbuffer[tokenstart]);
			success = success && getnexttoken(tokenstart, tokenlength);
			tempxyz.z = (float)atof(&fbuffer[tokenstart]);

			if (!success) { MessageBox(nullptr, L"ERROR: badly formatted normal!", L"FILE ERROR", MB_OK); }

			normal_list.push_back(tempxyz);
		}
		else if (strncmp(&fbuffer[tokenstart], "f ", 2) == 0)
		{
			int tempptr = tokenstart + 2;
			int forwardslashcount = 0;
			bool adjecentslash = false;

			while (fbuffer[tempptr] != '\n')
			{
				if (fbuffer[tempptr] == '/')
				{
					forwardslashcount++;
					if (fbuffer[tempptr - 1] == '/') adjecentslash = true;
				}
				tempptr++;
			}

			success = true;

			for (int i = 0; i < 3; i++)
			{
				success = success && getnexttoken(tokenstart, tokenlength);
				pindices.push_back(atoi(&fbuffer[tokenstart]));

				if (forwardslashcount >= 3 && adjecentslash == false)
				{
					success = success && getnexttoken(tokenstart, tokenlength);
					tindices.push_back(atoi(&fbuffer[tokenstart]));
				}

				if (forwardslashcount == 6 || adjecentslash == true)
				{
					success = success && getnexttoken(tokenstart, tokenlength);
					nindices.push_back(atoi(&fbuffer[tokenstart]));
				}
			}

			if (!success) {MessageBox(nullptr, L"ERROR: badly formatted face!", L"FILE ERROR", MB_OK); }
		}
	} while (getnextline()==true);
}

bool ObjFileModel::getnexttoken(int& tokenstart, int& tokenlength)
{
	tokenstart = tokenptr;
	tokenlength = 1;
	int tokenend;

	while (fbuffer[tokenptr] == ' ' || fbuffer[tokenptr] == '\t' || fbuffer[tokenptr] == '/') tokenptr++;

	if (fbuffer[tokenptr] == '\n') return false;

	tokenend = tokenptr + 1;

	while (fbuffer[tokenend] != ' ' && fbuffer[tokenend] != '\t' && fbuffer[tokenend] != '\n' && fbuffer[tokenend] != '/') tokenend++;

	tokenlength = tokenend - tokenptr;
	tokenstart = tokenptr;
	tokenptr += tokenlength;

	return true;
}

bool ObjFileModel::getnextline()
{
	while (fbuffer[tokenptr] != '\n' && tokenptr < actualsize) tokenptr++;

	tokenptr++;

	if (tokenptr >= actualsize) return false;
	else return true;
}


bool ObjFileModel::createVB()
{
	numVerts = (unsigned int) pindices.size();

	vertices = new MODEL_POS_TEX_NORM_VERTEX[numVerts];

	for (unsigned int i = 0; i < numVerts; i++) 
	{
		int vindex = pindices[i] - 1;

		vertices[i].Pos.x = position_list[vindex].x;
		vertices[i].Pos.y = position_list[vindex].y;
		vertices[i].Pos.z = position_list[vindex].z;

		if (tindices.size() > 0)
		{
			int tindex = tindices[i] - 1;
			vertices[i].TexCoord.x = texcoord_list[tindex].x;
			vertices[i].TexCoord.y = texcoord_list[tindex].y;
		}

		if (nindices.size() > 0)
		{
			int nindex = nindices[i] - 1;
			vertices[i].Normal.x = normal_list[nindex].x;
			vertices[i].Normal.y = normal_list[nindex].y;
			vertices[i].Normal.z = normal_list[nindex].z;
		}				  
	}					  

	D3D11_BUFFER_DESC bufferDesc;
	ZeroMemory(&bufferDesc, sizeof(D3D11_BUFFER_DESC));
	bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	bufferDesc.ByteWidth = sizeof(vertices[0]) * numVerts;
	bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	
	HRESULT hr = pDevice->CreateBuffer(&bufferDesc, nullptr, &pVertexBuffer);
	if (FAILED(hr)) return false;

	
	D3D11_MAPPED_SUBRESOURCE ms;
	pContext->Map(pVertexBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, NULL, &ms);
	memcpy(ms.pData, vertices, sizeof(vertices[0]) * numVerts);
	pContext->Unmap(pVertexBuffer.Get(), NULL);
	
	return true;
}

ObjFileModel::~ObjFileModel()
{
	delete[] vertices;

	position_list.clear();
	texcoord_list.clear();
	normal_list.clear();
}