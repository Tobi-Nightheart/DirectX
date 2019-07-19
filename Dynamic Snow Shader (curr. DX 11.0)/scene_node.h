#pragma once
#include "Model.h"
#include "maths.h"

class scene_node
{

private:
	std::vector<scene_node*> m_children;
	float m_px, m_py, m_pz;
	float m_xAngle, m_yAngle, m_zAngle;
	float m_Scale;
	Model* m_p_model;

	float m_world_centre_x, m_world_centre_y, m_world_centre_z, m_world_scale;

	XMMATRIX m_local_world_matrix;
	maths* m_pHelper;

	XMVECTOR vDeformPos;
	XMFLOAT3* deformPos;

public:
	scene_node();
	~scene_node();

	void addChildNode(scene_node* n);
	bool detachNode(scene_node* n);
	void execute(XMMATRIX* world, XMMATRIX* view, XMMATRIX* projection, XMFLOAT4 AmbC, XMVECTOR DirV, XMFLOAT4 DirC);
	void executeOpaque(XMMATRIX* world, XMMATRIX* view, XMMATRIX* projection);
	XMVECTOR GetWorldCentrePosition();
	XMFLOAT3* GetWorldDeformPosition();
	void UpdateCollisionTree(XMMATRIX* world, float scale);
	bool CheckCollision(scene_node* compare_tree);
	bool CheckCollision(scene_node* compare_tree, scene_node* object_tree_root);
	bool checkCollisionRay(XMVECTOR* ray_pos, XMVECTOR* ray_dir, bool called_from_camera);
	void LookAtAZ(float x, float z);
	bool MoveForward(float d, scene_node* root_node, float dt);
	void FluctuateHeight(float d, scene_node* root_node, float dt);

#pragma region SetterGetterIncrements
	void SetModel(Model* m);
	bool IncX(float num, scene_node* root_node);
	bool IncY(float num, scene_node* root_node);
	bool IncZ(float num, scene_node* root_node);
	bool IncXAngle(float num, scene_node* root_node);
	bool IncYAngle(float num, scene_node* root_node);
	bool IncZAngle(float num, scene_node* root_node);
	bool IncScale(float num, scene_node* root_node);
	bool SetX(float x, scene_node* root_node);
	bool SetY(float y, scene_node* root_node);
	bool SetZ(float z, scene_node* root_node);
	bool SetXAngle(float xAngle, scene_node* root_node);
	bool SetYAngle(float yAngle, scene_node* root_node);
	bool SetZAngle(float zAngle, scene_node* root_node);
	bool SetScale(float scale, scene_node* root_node);
	float GetX();
	float GetY();
	float GetZ();
	float GetXAngle();
	float GetYAngle();
	float GetZAngle();
	float GetScale();
#pragma endregion
	
};

