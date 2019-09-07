#include "pch.h"
#include "scene_node.h"
#include "GameTimer.h"


scene_node::scene_node()
{
	m_p_model = nullptr;
	m_px = 0;
	m_py = 0;
	m_pz = 0;
	m_xAngle = 0.0f;
	m_yAngle = 0.0f;
	m_zAngle = 0.0f;
	m_Scale = 1.0f;
	vDeformPos = XMVectorZero();
	deformPos = new XMFLOAT3();

	bObjDraw = true;
}


scene_node::~scene_node()
{	
	if(m_pHelper)
	{
		//delete m_pHelper;
		m_pHelper = nullptr;
	}
	if(m_p_model)
	{
		//delete m_p_model;
		m_p_model = nullptr;
	}
}

void scene_node::addChildNode(scene_node* n)
{
	m_children.push_back(n);
}

bool scene_node::detachNode(scene_node * n)
{
	//traverse tree, find node, detach
	for (int i=0; i< (int) m_children.size(); i++)
	{
		if(n == m_children[i])
		{
			m_children.erase(m_children.begin() + i);
			return true;
		}
	}
	return false;
}

void scene_node::execute(XMMATRIX * world, XMMATRIX * view, XMMATRIX * projection, XMFLOAT4 AmbC, XMVECTOR DirV, XMFLOAT4 DirC)
{
	if (bObjDraw) 
	{
	
		XMMATRIX local_world = XMMatrixIdentity();

		local_world = XMMatrixRotationX(XMConvertToRadians(m_xAngle));
		local_world *= XMMatrixRotationY(XMConvertToRadians(m_yAngle));
		local_world *= XMMatrixRotationZ(XMConvertToRadians(m_zAngle));

		local_world *= XMMatrixScaling(m_Scale, m_Scale, m_Scale);

		local_world *= XMMatrixTranslation(m_px, m_py, m_pz);

		local_world *= *world;

		if (m_p_model) 
		{
			m_p_model->Draw(&local_world, view, projection, AmbC, DirV, DirC);
		}
		for(int i = 0; i< (int) m_children.size(); i++)
		{
			m_children[i]->execute(&local_world, view, projection, AmbC, DirV, DirC);
		}
	}
}

void scene_node::executeOpaque(XMMATRIX* world, XMMATRIX* view, XMMATRIX* projection)
{
	if (bObjDraw)
	{
		XMMATRIX local_world = XMMatrixIdentity();

		local_world = XMMatrixRotationX(XMConvertToRadians(m_xAngle));
		local_world *= XMMatrixRotationY(XMConvertToRadians(m_yAngle));
		local_world *= XMMatrixRotationZ(XMConvertToRadians(m_zAngle));

		local_world *= XMMatrixScaling(m_Scale, m_Scale, m_Scale);

		local_world *= XMMatrixTranslation(m_px, m_py, m_pz);

		local_world *= *world;

		if (m_p_model) m_p_model->DrawOpaque(&local_world, view, projection);

		for (int i = 0; i < (int) m_children.size(); i++) {
			m_children[i]->executeOpaque(&local_world, view, projection);

		}
	}
}

XMVECTOR scene_node::GetWorldCentrePosition()
{
	return XMVectorSet(m_world_centre_x, m_world_centre_y, m_world_centre_z, 0.0f);
}

//The world position minus 2
XMFLOAT3* scene_node::GetWorldDeformPosition()
{
	XMVECTOR e = XMVectorSet(0.0f, 2.0f, 0.0f, 0.0f);
	XMVECTOR t = XMVectorSet(m_world_centre_x, m_world_centre_y, m_world_centre_z, 0.0f);
    vDeformPos = XMVectorSubtract(t, e);
	XMStoreFloat3(deformPos, vDeformPos);
	return deformPos;
}

void scene_node::UpdateCollisionTree(XMMATRIX * world, float scale)
{
	//the local world matrix will be used to calculate the local transformations
	m_local_world_matrix = XMMatrixIdentity();
	m_local_world_matrix = XMMatrixRotationX(XMConvertToRadians(m_xAngle));
	m_local_world_matrix *= XMMatrixRotationY(XMConvertToRadians(m_yAngle));
	m_local_world_matrix *= XMMatrixRotationZ(XMConvertToRadians(m_zAngle));

	m_local_world_matrix *= XMMatrixScaling(m_Scale, m_Scale, m_Scale);

	m_local_world_matrix *= XMMatrixTranslation(m_px, m_py, m_pz);
	//the local matrix is multiplied by the passed matrix to  transforms all nodes relative to their parents
	m_local_world_matrix *= *world;

	//same goes for scale also needed  to calculate the correctly sized bounding sphere
	m_world_scale = scale * m_Scale;

	XMVECTOR v;
	if(m_p_model)
	{
		v = m_p_model->GetBoundingSphereWorldSpacePosition();
	}else
	{
		v = XMVectorSet(0, 0, 0, 0);// no model default 0
	}

	//find and store the world space bounding sphere center
	v = XMVector3Transform(v, m_local_world_matrix);
	m_world_centre_x = XMVectorGetX(v);
	m_world_centre_y = XMVectorGetY(v);
	m_world_centre_z = XMVectorGetZ(v);

	//traverse all child nodes, passing in the concatenated world matrix and scale
	for(int i = 0; i < (int) m_children.size(); i++)
	{
		m_children[i]->UpdateCollisionTree(&m_local_world_matrix, m_world_scale);
	}

}

bool scene_node::CheckCollision(scene_node* compare_tree)
{
	return CheckCollision(compare_tree, this);
}

bool scene_node::CheckCollision(scene_node * compare_tree, scene_node * object_tree_root)
{
	//check to see if root of tree is being compared is same as root node of tre checked and stopping to check against children
	if (object_tree_root == compare_tree) return false;

	//only check for collisions if both nodes contain a model
	if(m_p_model && compare_tree->m_p_model)
	{
		XMVECTOR v1 = GetWorldCentrePosition();
		XMVECTOR v2 = compare_tree->GetWorldCentrePosition();
		XMVECTOR vdiff = v1 - v2;

		//XMVECTOR a = XMVector3Lenght(vdiff);
		float x1 = XMVectorGetX(v1);
		float x2 = XMVectorGetX(v2);
		float y1 = XMVectorGetY(v1);
		float y2 = XMVectorGetY(v2);
		float z1 = XMVectorGetZ(v1);
		float z2 = XMVectorGetZ(v2);

		float dx = x1 - x2;
		float dy = y1 - y2;
		float dz = z1 - z2;

		//check bounding sphere collision
		//if ((dx*dx + dy * dy + dz * dz)< pow((compare_tree->m_p_model->GetBoundingSphereRadius() * compare_tree->m_world_scale)+ (this->m_p_model->GetBoundingSphereRadius()* this->m_world_scale),2))
		if (sqrt(dx*dx + dy * dy + dz * dz) < (compare_tree->m_p_model->GetBoundingSphereRadius() * compare_tree->m_world_scale) + (this->m_p_model->GetBoundingSphereRadius()* this->m_world_scale))
		{
			return true;
			/*
			//per triangle collision for the compare trees model intersection of spheres
			for(int i = 0; i<compare_tree->m_p_model->GetObject()->numverts; i += 3)
			{
				XMVECTOR p1 = XMVectorSet(compare_tree->m_p_model->GetObject()->vertices[i].Pos.x,
					compare_tree->m_p_model->GetObject()->vertices[i].Pos.y,
					compare_tree->m_p_model->GetObject()->vertices[i].Pos.z, 0.0f);
				XMVECTOR p2 = XMVectorSet(compare_tree->m_p_model->GetObject()->vertices[i + 1].Pos.x,
					compare_tree->m_p_model->GetObject()->vertices[i + 1].Pos.y,
					compare_tree->m_p_model->GetObject()->vertices[i + 1].Pos.z, 0.0f);
				XMVECTOR p3 = XMVectorSet(compare_tree->m_p_model->GetObject()->vertices[i + 2].Pos.x,
					compare_tree->m_p_model->GetObject()->vertices[i + 2].Pos.y,
					compare_tree->m_p_model->GetObject()->vertices[i + 2].Pos.z, 0.0f);
				p1 = XMVector3Transform(p1, compare_tree->m_local_world_matrix);
				p2 = XMVector3Transform(p2, compare_tree->m_local_world_matrix);
				p3 = XMVector3Transform(p3, compare_tree->m_local_world_matrix);

				XMVECTOR r;
				r.x = p2.x - p1.x;
				r.y = p2.y - p1.y;
				r.z = p2.z - p1.z;
				r.w = 0.0f;
				if(checkCollisionRay(&p1, &r, false)) return true;

				r.x = p3.x - p1.x;
				r.y = p3.y - p1.y;
				r.z = p3.z - p1.z;
				r.w = 0.0f;
				if (checkCollisionRay(&p1, &r, false)) return true;
				
				r.x = p3.x - p2.x;
				r.y = p3.y - p2.y;
				r.z = p3.z - p2.z;
				r.w = 0.0f;
				if (checkCollisionRay(&p2, &r, false)) return true;
			}
			//check for the scene nodes model
			for (int i = 0; i < m_p_model->GetObject()->numverts; i += 3)
			{
				XMVECTOR p1 = XMVectorSet(m_p_model->GetObject()->vertices[i].Pos.x,
					m_p_model->GetObject()->vertices[i].Pos.y,
					m_p_model->GetObject()->vertices[i].Pos.z, 0.0f);
				XMVECTOR p2 = XMVectorSet(m_p_model->GetObject()->vertices[i + 1].Pos.x,
					m_p_model->GetObject()->vertices[i + 1].Pos.y,
					m_p_model->GetObject()->vertices[i + 1].Pos.z, 0.0f);
				XMVECTOR p3 = XMVectorSet(m_p_model->GetObject()->vertices[i + 2].Pos.x,
					m_p_model->GetObject()->vertices[i + 2].Pos.y,
					m_p_model->GetObject()->vertices[i + 2].Pos.z, 0.0f);
				p1 = XMVector3Transform(p1, m_local_world_matrix);
				p2 = XMVector3Transform(p2, m_local_world_matrix);
				p3 = XMVector3Transform(p3, m_local_world_matrix);

				XMVECTOR r;
				r.x = p2.x - p1.x;
				r.y = p2.y - p1.y;
				r.z = p2.z - p1.z;
				r.w = 0.0f;
				if (checkCollisionRay(&p1, &r, false)) return true;

				r.x = p3.x - p1.x;
				r.y = p3.y - p1.y;
				r.z = p3.z - p1.z;
				r.w = 0.0f;
				if (checkCollisionRay(&p1, &r, false)) return true;

				r.x = p3.x - p2.x;
				r.y = p3.y - p2.y;
				r.z = p3.z - p2.z;
				r.w = 0.0f;
				if (checkCollisionRay(&p2, &r, false)) return true;	
			}*/
		}

	}

	//iterate through compared tree child nodes
	for(int i=0; i< (int) compare_tree->m_children.size(); i++)
	{
		if (CheckCollision(compare_tree->m_children[i], object_tree_root)) return  true;
	}

	//iterate through coomposite object children nodes
	for(int i = 0; i< (int) m_children.size(); i++)
	{
		if (m_children[i]->CheckCollision(compare_tree, object_tree_root)) return true;
	}

	return false;
}

bool scene_node::checkCollisionRay(XMVECTOR * ray_pos, XMVECTOR * ray_dir, bool called_from_camera)
{
	if(m_p_model)
	{
		float dx, dy, dz;
		dx = ray_pos->x - m_world_centre_x;
		dy = ray_pos->y - m_world_centre_y;
		dz = ray_pos->z - m_world_centre_z;

		if((dx*dx + dy * dy + dz * dz) < pow((this->m_p_model->GetBoundingSphereRadius()* this->m_world_scale)+sqrt(ray_dir->x*ray_dir->x + ray_dir->y * ray_dir->y + ray_dir->z*ray_dir->z),2))
		{
			for(int i=0; i< (int) m_p_model->GetObject()->numverts; i += 3)
			{
				XMVECTOR p1 = XMVectorSet(m_p_model->GetObject()->vertices[i].Pos.x,
					m_p_model->GetObject()->vertices[i].Pos.y,
					m_p_model->GetObject()->vertices[i].Pos.z, 0.0f);
				XMVECTOR p2 = XMVectorSet(m_p_model->GetObject()->vertices[i+1].Pos.x,
					m_p_model->GetObject()->vertices[i+1].Pos.y,
					m_p_model->GetObject()->vertices[i+1].Pos.z, 0.0f);
				XMVECTOR p3 = XMVectorSet(m_p_model->GetObject()->vertices[i+2].Pos.x,
					m_p_model->GetObject()->vertices[i+2].Pos.y,
					m_p_model->GetObject()->vertices[i+2].Pos.z, 0.0f);
				p1 = XMVector3Transform(p1, m_local_world_matrix);
				p2 = XMVector3Transform(p2, m_local_world_matrix);
				p3 = XMVector3Transform(p3, m_local_world_matrix);

				maths::Plane plane{};
				plane= m_pHelper->planeEquation(&p1, &p2, &p3);

				float v1= m_pHelper->planeTest(&plane, ray_dir);
				float v2 = m_pHelper->planeTest(&plane, ray_pos);
				if (m_pHelper->sign(v1) == m_pHelper->sign(v2))
				{
					XMVECTOR iPoint = m_pHelper->planeIntersection(&plane, ray_pos, ray_dir);
					if(m_pHelper->in_triangle(&p1, &p2, &p3, &iPoint))
					return true;
				}
			}
		}
		if(called_from_camera)
		for(int i=0; i < (int) m_children.size();i++)
		{
			if (m_children[i]->checkCollisionRay(ray_pos, ray_dir, called_from_camera))return true;
		}
		
	}

	return false;
}

void scene_node::LookAtAZ(float x, float z)
{
	float dx, dz;
	dx = x - m_px;
	dz = z - m_pz;

	m_yAngle =  atan2(dx, dz) * (float) (180.0 / XM_PI);
}

bool scene_node::MoveForward(float d, scene_node* root_node, float dt)
{
	float oldx, oldz;
	oldx = m_px;
	oldz = m_pz;
	
	m_pz += (float) cos(m_yAngle * (XM_PI / 180.0))*d*dt;
	m_px += (float) sin(m_yAngle * (XM_PI / 180.0))*d*dt;

	XMMATRIX identity = XMMatrixIdentity();

	//update the collision tree according to change made starting at root
	root_node->UpdateCollisionTree(&identity, 1.0);

	if (CheckCollision(root_node))
	{
		m_px = oldx;
		m_pz = oldz;
		return true;
	}
	return false;
}

void scene_node::FluctuateHeight(float d, scene_node* root_node, float dt)
{
	float oldy = m_py;

	m_py -= d*sin(dt);

	XMMATRIX identity = XMMatrixIdentity();

	//update the collision tree according to change made starting at root
	root_node->UpdateCollisionTree(&identity, 1.0f);

	if (CheckCollision(root_node)) {
		m_py = oldy;
	}
}

void scene_node::ToggleDrawing()
{
	bObjDraw = !bObjDraw;
}

#pragma region SetterGetterIncrements
void scene_node::SetModel(Model* m)
{
	
	m_p_model = m;
}

bool scene_node::IncX(float num, scene_node* root_node)
{
	float old = m_px;
	m_px += num;
	XMMATRIX identity = XMMatrixIdentity();

	//update the collision tree according to change made starting at root
	root_node->UpdateCollisionTree(&identity, 1.0);

	if(CheckCollision(root_node))
	{
		m_px = old;
		return true;
	}
	return false;
}

bool scene_node::IncY(float num, scene_node* root_node)
{
	float old = m_py;
	m_py += num;
	XMMATRIX identity = XMMatrixIdentity();

	//update the collision tree according to change made starting at root
	root_node->UpdateCollisionTree(&identity, 1.0);

	if (CheckCollision(root_node))
	{
		m_py = old;
		return true;
	}
	return false;
}

bool scene_node::IncZ(float num, scene_node* root_node)
{
	float old = m_pz;
	m_pz += num;
	XMMATRIX identity = XMMatrixIdentity();

	//update the collision tree according to change made starting at root
	root_node->UpdateCollisionTree(&identity, 1.0);

	if (CheckCollision(root_node))
	{
		m_pz = old;
		return true;
	}
	return false;
}

bool scene_node::IncXAngle(float num, scene_node* root_node)
{
	float old = m_xAngle;
	m_xAngle += num;
	XMMATRIX identity = XMMatrixIdentity();

	//update the collision tree according to change made starting at root
	root_node->UpdateCollisionTree(&identity, 1.0);

	if (CheckCollision(root_node))
	{
		m_xAngle = old;
		return true;
	}
	return false;
}

bool scene_node::IncYAngle(float num, scene_node* root_node)
{
	float old = m_yAngle;
	m_yAngle += num;
	XMMATRIX identity = XMMatrixIdentity();

	//update the collision tree according to change made starting at root
	root_node->UpdateCollisionTree(&identity, 1.0);

	if (CheckCollision(root_node))
	{
		m_yAngle = old;
		return true;
	}
	return false;
}

bool scene_node::IncZAngle(float num, scene_node* root_node)
{
	float old = m_zAngle;
	m_zAngle += num;
	XMMATRIX identity = XMMatrixIdentity();

	//update the collision tree according to change made starting at root
	root_node->UpdateCollisionTree(&identity, 1.0);

	if (CheckCollision(root_node))
	{
		m_zAngle = old;
		return true;
	}
	return false;
}

bool scene_node::IncScale(float num, scene_node* root_node)
{
	float old = m_Scale;
	m_Scale += num;
	XMMATRIX identity = XMMatrixIdentity();

	//update the collision tree according to change made starting at root
	root_node->UpdateCollisionTree(&identity, 1.0);

	if (CheckCollision(root_node))
	{
		m_Scale = old;
		return true;
	}
	return false;
}

bool scene_node::SetX(float x, scene_node* root_node)
{
	float old = m_px;
	m_px = x;
	XMMATRIX identity = XMMatrixIdentity();

	//update the collision tree according to change made starting at root
	root_node->UpdateCollisionTree(&identity, 1.0);

	if (CheckCollision(root_node))
	{
		m_px = old;
		return true;
	}
	return false;
}

bool scene_node::SetY(float y, scene_node* root_node)
{
	float old = m_py;
	m_py = y;
	XMMATRIX identity = XMMatrixIdentity();

	//update the collision tree according to change made starting at root
	root_node->UpdateCollisionTree(&identity, 1.0);

	if (CheckCollision(root_node))
	{
		m_py = old;
		return true;
	}
	return false;
}

bool scene_node::SetZ(float z, scene_node* root_node)
{
	float old = m_pz;
	m_pz = z;
	XMMATRIX identity = XMMatrixIdentity();

	//update the collision tree according to change made starting at root
	root_node->UpdateCollisionTree(&identity, 1.0);

	if (CheckCollision(root_node))
	{
		m_pz = old;
		return true;
	}
	return false;
}

bool scene_node::SetXAngle(float xAngle, scene_node* root_node)
{
	float old = m_xAngle;
	m_xAngle = xAngle;
	XMMATRIX identity = XMMatrixIdentity();

	//update the collision tree according to change made starting at root
	root_node->UpdateCollisionTree(&identity, 1.0);

	if (CheckCollision(root_node))
	{
		m_xAngle = old;
		return true;
	}
	return false;
}

bool scene_node::SetYAngle(float yAngle, scene_node* root_node)
{
	float old = m_yAngle;
	m_yAngle = yAngle;
	XMMATRIX identity = XMMatrixIdentity();

	//update the collision tree according to change made starting at root
	root_node->UpdateCollisionTree(&identity, 1.0);

	if (CheckCollision(root_node))
	{
		m_yAngle = old;
		return true;
	}
	return false;
}

bool scene_node::SetZAngle(float zAngle, scene_node* root_node)
{
	float old = m_zAngle;
	m_zAngle = zAngle;
	XMMATRIX identity = XMMatrixIdentity();

	//update the collision tree according to change made starting at root
	root_node->UpdateCollisionTree(&identity, 1.0);

	if (CheckCollision(root_node))
	{
		m_zAngle = old;
		return true;
	}
	return false;
}

bool scene_node::SetScale(float scale, scene_node* root_node)
{
	float old = m_Scale;
	m_Scale = scale;
	XMMATRIX identity = XMMatrixIdentity();

	//update the collision tree according to change made starting at root
	root_node->UpdateCollisionTree(&identity, 1.0);

	if (CheckCollision(root_node))
	{
		m_Scale = old;
		return true;
	}
	return false;
}

float scene_node::GetX()
{
	return m_px;
}

float scene_node::GetY()
{
	return m_py;
}

float scene_node::GetZ()
{
	return m_pz;
}

float scene_node::GetXAngle()
{
	return m_xAngle;
}

float scene_node::GetYAngle()
{
	return m_yAngle;
}

float scene_node::GetZAngle()
{
	return m_zAngle;
}

float scene_node::GetScale()
{
	return m_Scale;
}
XMMATRIX scene_node::GetWorld()
{
	return m_local_world_matrix;
}
#pragma endregion

